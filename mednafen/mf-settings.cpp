/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* settings.cpp:
**  Copyright (C) 2005-2021 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 TODO: Setting changed callback on override setting loading/clearing.
*/

#include "mednafen.h"
#include <locale.h>
#include <map>
#include "mf-settings.h"
#include "string/mendafen_string.h"

namespace Mednafen
{
SettingsManager::SettingsManager()
{
}

SettingsManager::~SettingsManager()
{
 Kill();
}

static INLINE unsigned TranslateSettingValueUI(const char* v, uint64& tlated)
{
 unsigned error = 0;

 // Backwards-compat:
 v = MDFN_strskipspace(v);
 //
 tlated = MDFN_u64fromstr(v, 0, &error);

 return error;
}

static INLINE unsigned TranslateSettingValueI(const char* v, int64& tlated)
{
 unsigned error = 0;

 // Backwards-compat:
 v = MDFN_strskipspace(v);
 //
 tlated = MDFN_s64fromstr(v, 0, &error);
 
 return error;
}

//
// This function is a ticking time bomb of (semi-non-reentrant) wrong, but it's OUR ticking time bomb.
//
// Note to self: test it with something like: LANG="fr_FR.UTF-8" ./mednafen
//
static bool MR_StringToDouble(const char* string_value, double* dvalue) NO_INLINE;	// noinline for *potential* x87 FPU extra precision weirdness in regards to optimizations.
static bool MR_StringToDouble(const char* string_value, double* dvalue)
{
 static char MR_Radix = 0;
 const unsigned slen = strlen(string_value);
 char cpi_array[256 + 1];
 std::unique_ptr<char[]> cpi_heap;
 char* cpi = cpi_array;
 char* endptr = NULL;

 if(slen > 256)
 {
  cpi_heap.reset(new char[slen + 1]);
  cpi = cpi_heap.get();
 }

 if(!MR_Radix)
 {
  char buf[64]; // Use extra-large buffer since we're using sprintf() instead of snprintf() for portability reasons. //4];
  // Use libc snprintf() and not snprintf() here for out little abomination.
  //snprintf(buf, 4, "%.1f", (double)1);
  sprintf(buf, "%.1f", (double)1);
  if(buf[0] == '1' && buf[2] == '0' && buf[3] == 0)
  {
   MR_Radix = buf[1];
  }
  else
  {
   lconv* l = localeconv();
   assert(l != NULL);
   MR_Radix = *(l->decimal_point);
  }
 }

 for(unsigned i = 0; i < slen; i++)
 {
  char c = string_value[i];

  if(c == '.' || c == ',')
   c = MR_Radix;

  cpi[i] = c;
 }
 cpi[slen] = 0;

 *dvalue = strtod(cpi, &endptr);

 if(endptr == NULL || *endptr != 0 || !*cpi)
  return(false);

 return(true);
}

static void ValidateSetting(const char *value, const MDFNSetting *setting)
{
 MDFNSettingType base_type = setting->type;

 if(base_type == MDFNST_UINT)
 {
  uint64 ullvalue;

  switch(TranslateSettingValueUI(value, ullvalue))
  {
   case XFROMSTR_ERROR_NONE:
	break;

   case XFROMSTR_ERROR_UNDERFLOW:	// Shouldn't happen
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too small; the minimum acceptable value is \"%s\"."), setting->name, value, setting->minimum ? setting->minimum : "0");

   case XFROMSTR_ERROR_OVERFLOW:
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too large; the maximum acceptable value is \"%s\"."), setting->name, value, setting->maximum ? setting->maximum : "18446744073709551615");

   default:
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is not a valid integer."), setting->name, value);
  }
  if(setting->minimum)
  {
   uint64 minimum;
   if(MDFN_UNLIKELY(TranslateSettingValueUI(setting->minimum, minimum)))
    throw MDFN_Error(0, _("Minimum value \"%s\" for setting \"%s\" is invalid."), setting->minimum, setting->name);

   if(MDFN_UNLIKELY(ullvalue < minimum))
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too small; the minimum acceptable value is \"%s\"."), setting->name, value, setting->minimum);
  }
  if(setting->maximum)
  {
   uint64 maximum;
   if(MDFN_UNLIKELY(TranslateSettingValueUI(setting->maximum, maximum)))
    throw MDFN_Error(0, _("Maximum value \"%s\" for setting \"%s\" is invalid."), setting->maximum, setting->name);

   if(MDFN_UNLIKELY(ullvalue > maximum))
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too large; the maximum acceptable value is \"%s\"."), setting->name, value, setting->maximum);
  }
 }
 else if(base_type == MDFNST_INT)
 {
  int64 llvalue;

  switch(TranslateSettingValueI(value, llvalue))
  {
   case XFROMSTR_ERROR_NONE:
	break;

   case XFROMSTR_ERROR_UNDERFLOW:
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too small; the minimum acceptable value is \"%s\"."), setting->name, value, setting->minimum ? setting->minimum : "-9223372036854775808");

   case XFROMSTR_ERROR_OVERFLOW:
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too large; the maximum acceptable value is \"%s\"."), setting->name, value, setting->maximum ? setting->maximum : "9223372036854775807");

   default:
	throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is not a valid integer."), setting->name, value);
  }
  if(setting->minimum)
  {
   int64 minimum;

   if(MDFN_UNLIKELY(TranslateSettingValueI(setting->minimum, minimum)))
    throw MDFN_Error(0, _("Minimum value \"%s\" for setting \"%s\" is invalid."), setting->minimum, setting->name);

   if(MDFN_UNLIKELY(llvalue < minimum))
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too small; the minimum acceptable value is \"%s\"."), setting->name, value, setting->minimum);
  }
  if(setting->maximum)
  {
   int64 maximum;
   if(MDFN_UNLIKELY(TranslateSettingValueI(setting->maximum, maximum)))
    throw MDFN_Error(0, _("Maximum value \"%s\" for setting \"%s\" is invalid."), setting->maximum, setting->name);

   if(MDFN_UNLIKELY(llvalue > maximum))
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too large; the maximum acceptable value is \"%s\"."), setting->name, value, setting->maximum);
  }
 }
 else if(base_type == MDFNST_FLOAT)
 {
  double dvalue;

  if(!MR_StringToDouble(value, &dvalue) || std::isnan(dvalue))
   throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is not a valid real number."), setting->name, value);

  if(setting->minimum)
  {
   double minimum;

   if(MDFN_UNLIKELY(!MR_StringToDouble(setting->minimum, &minimum)))
    throw MDFN_Error(0, _("Minimum value, \"%f\", for setting \"%s\" is not set to a floating-point(real) number."), minimum, setting->name);

   if(MDFN_UNLIKELY(dvalue < minimum))
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is too small; the minimum acceptable value is \"%s\"."), setting->name, value, setting->minimum);
  }

  if(setting->maximum)
  {
   double maximum;

   if(MDFN_UNLIKELY(!MR_StringToDouble(setting->maximum, &maximum)))
    throw MDFN_Error(0, _("Maximum value, \"%f\", for setting \"%s\" is not set to a floating-point(real) number."), maximum, setting->name);

   if(MDFN_UNLIKELY(dvalue > maximum))
    throw MDFN_Error(0, _("Maximum value \"%s\" for setting \"%s\" is invalid."), setting->maximum, setting->name);
  }
 }
 else if(base_type == MDFNST_BOOL)
 {
  if(strlen(value) != 1 || (value[0] != '0' && value[0] != '1'))
  {
   throw MDFN_Error(0, _("Setting \"%s\", value \"%s\",  is not a valid boolean value."), setting->name, value);
  }
 }
 else if(base_type == MDFNST_ENUM)
 {
  const MDFNSetting_EnumList *enum_list = setting->enum_list;
  std::string valid_string_list;

  assert(enum_list);

  while(enum_list->string)
  {
   if(!MDFN_strazicmp(value, enum_list->string))
    break;

   if(enum_list->description)	// Don't list out undocumented and deprecated values.
    valid_string_list = valid_string_list + (enum_list == setting->enum_list ? "" : " ") + std::string(enum_list->string);

   enum_list++;
  }
 }
 else if(base_type == MDFNST_MULTI_ENUM)
 {
  std::vector<std::string> mel = MDFN_strsplit(value);

  assert(setting->enum_list);

  for(auto& mee : mel)
  {
   bool found = false;
   const MDFNSetting_EnumList* enum_list = setting->enum_list;

   MDFN_trim(&mee);

   while(enum_list->string)
   {
    if(!MDFN_strazicmp(mee.c_str(), enum_list->string))
    {
     found = true;
     break;
    }
    enum_list++;
   }

   if(!found)
   {
    std::string valid_string_list;

    enum_list = setting->enum_list;
    while(enum_list->string)
    {
     if(enum_list->description)	// Don't list out undocumented and deprecated values.
      valid_string_list = valid_string_list + (enum_list == setting->enum_list ? "" : " ") + std::string(enum_list->string);

     enum_list++;
    }
    throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" component \"%s\" is not a recognized string.  Recognized strings: %s"), setting->name, value, mee.c_str(), valid_string_list.c_str());
   }
  }
 }

 if(setting->validate_func && MDFN_UNLIKELY(!setting->validate_func(setting->name, value)))
 {
  if(base_type == MDFNST_STRING)
   throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is not an acceptable string."), setting->name, value);
  else
   throw MDFN_Error(0, _("Setting \"%s\" value \"%s\" is not an acceptable integer."), setting->name, value);
 }
}

static uint32 MakeNameHash(const char *name)
{
#if 0
 return crc32(0, (const Bytef *)name, strlen(name));
#else
 uint32 ret = 0;

 for(size_t i = 0; name[i]; i++)
  ret = (ret * 1103515245) + 12345 + (uint8)name[i];

 //printf("%s %08x\n", name, ret);

 return ret;
#endif
}

INLINE void SettingsManager::MergeSettingSub(const MDFNSetting& setting)
{
 MDFNCS TempSetting;

 assert(setting.name);
 assert(setting.default_value);

 TempSetting.name_hash = MakeNameHash(setting.name);
 TempSetting.value = strdup(setting.default_value);
 TempSetting.desc = setting;

 CurrentSettings.push_back(TempSetting);
}

void SettingsManager::Add(const MDFNSetting& setting)
{
 assert(!SettingsFinalized);

 MergeSettingSub(setting);
}

void SettingsManager::Merge(const MDFNSetting *setting)
{
 assert(!SettingsFinalized);

 while(setting->name != NULL)
 {
  MergeSettingSub(*setting);
  setting++;
 }
}

static bool CSHashSortFunc(const MDFNCS& a, const MDFNCS& b)
{
 return a.name_hash < b.name_hash;
}

static bool CSHashBoundFunc(const MDFNCS& a, const uint32 b)
{
 return a.name_hash < b;
}

void SettingsManager::Finalize(void)
{
 std::sort(CurrentSettings.begin(), CurrentSettings.end(), CSHashSortFunc);

 //
 // Ensure no duplicates.
 //
 for(size_t i = 0; i < CurrentSettings.size(); i++)
 {
  for(size_t j = i + 1; j < CurrentSettings.size() && CurrentSettings[j].name_hash == CurrentSettings[i].name_hash; j++)
  {
   if(!strcmp(CurrentSettings[i].desc.name, CurrentSettings[j].desc.name))
   {
    printf("Duplicate setting name %s\n", CurrentSettings[j].desc.name);
    abort();
   }
  }
 }

 SettingsFinalized = true;
/*
 for(size_t i = 0; i < CurrentSettings.size(); i++)
 {
  assert(CurrentSettings[i].desc->type == MDFNST_ALIAS || !strcmp(FindSetting(CurrentSettings[i].name)->name, CurrentSettings[i].name));
 }
*/
}

void SettingsManager::Kill(void)
{
 for(auto& sit : CurrentSettings)
 {
  free(sit.value);

  if(sit.desc.type == MDFNST_ALIAS)
   continue;

#if 1
  if(sit.desc.flags & MDFNSF_FREE_NAME)
   free((void*)sit.desc.name);

  if(sit.desc.flags & MDFNSF_FREE_DESC)
   free((void*)sit.desc.description);

  if(sit.desc.flags & MDFNSF_FREE_DESC_EXTRA)
   free((void*)sit.desc.description_extra);

  if(sit.desc.flags & MDFNSF_FREE_DEFAULT)
   free((void*)sit.desc.default_value);

  if(sit.desc.flags & MDFNSF_FREE_MINIMUM)
   free((void*)sit.desc.minimum);

  if(sit.desc.flags & MDFNSF_FREE_MAXIMUM)
   free((void*)sit.desc.maximum);

  if(sit.desc.enum_list)
  {
   if(sit.desc.flags & (MDFNSF_FREE_ENUMLIST_STRING | MDFNSF_FREE_ENUMLIST_DESC | MDFNSF_FREE_ENUMLIST_DESC_EXTRA))
   {
    const MDFNSetting_EnumList* enum_list = sit.desc.enum_list;

    while(enum_list->string)
    {
     if(sit.desc.flags & MDFNSF_FREE_ENUMLIST_STRING)
      free((void*)enum_list->string);

     if(sit.desc.flags & MDFNSF_FREE_ENUMLIST_DESC)
      free((void*)enum_list->description);

     if(sit.desc.flags & MDFNSF_FREE_ENUMLIST_DESC_EXTRA)
      free((void*)enum_list->description_extra);
     //
     enum_list++;
    }
   }
   if(sit.desc.flags & MDFNSF_FREE_ENUMLIST)
    free((void*)sit.desc.enum_list);
  }
#endif
 }

 CurrentSettings.clear();	// Call after the list is all handled
 SettingsFinalized = false;
}

MDFNCS* SettingsManager::FindSetting(const char* name, bool dont_freak_out_on_fail)
{
 assert(SettingsFinalized);
 //printf("Find: %s\n", name);
 const uint32 name_hash = MakeNameHash(name);
 std::vector<MDFNCS>::iterator it;

 it = std::lower_bound(CurrentSettings.begin(), CurrentSettings.end(), name_hash, CSHashBoundFunc);

 while(it != CurrentSettings.end() && it->name_hash == name_hash)
 {
  if(!strcmp(it->desc.name, name))
  {
   if(it->desc.type == MDFNST_ALIAS)
    return FindSetting(it->value, dont_freak_out_on_fail);

   return &*it;
  }
  //printf("OHNOS: %s(%08x) %s(%08x)\n", name, name_hash, it->desc.name, it->name_hash);
  it++;
 }

 if(!dont_freak_out_on_fail)
 {
  printf("\n\nINCONCEIVABLE!  Setting not found: %s\n\n", name);
  exit(1);
 }

 return NULL;
}

static const char *GetSetting(const MDFNCS *setting)
{
 return setting->value;
}

static int GetEnum(const MDFNCS *setting, const char *value)
{
 const MDFNSetting_EnumList *enum_list = setting->desc.enum_list;
 int ret = 0;

 assert(enum_list);

 while(enum_list->string)
 {
  if(!MDFN_strazicmp(value, enum_list->string))
  {
   ret = enum_list->number;
   break;
  }
  enum_list++;
 }

 return(ret);
}

template<typename T>
static std::vector<T> GetMultiEnum(const MDFNCS* setting, const char* value)
{
 std::vector<T> ret;
 std::vector<std::string> mel = MDFN_strsplit(value);

 assert(setting->desc.enum_list);

 for(auto& mee : mel)
 {
  const MDFNSetting_EnumList *enum_list = setting->desc.enum_list;

  MDFN_trim(&mee);

  while(enum_list->string)
  {
   if(!MDFN_strazicmp(mee.c_str(), enum_list->string))
   {
    ret.push_back(enum_list->number);
    break;
   }
   enum_list++;
  }
 }

 return ret;
}


uint64 SettingsManager::GetUI(const char *name)
{
 const MDFNCS *setting = FindSetting(name);
 const char *value = GetSetting(setting);

 if(setting->desc.type == MDFNST_ENUM)
  return(GetEnum(setting, value));
 else
 {
  uint64 ret;

  TranslateSettingValueUI(value, ret);

  return ret;
 }
}

int64 SettingsManager::GetI(const char *name)
{
 const MDFNCS *setting = FindSetting(name);
 const char *value = GetSetting(FindSetting(name));


 if(setting->desc.type == MDFNST_ENUM)
  return(GetEnum(setting, value));
 else
 {
  int64 ret;

  TranslateSettingValueI(value, ret);

  return ret;
 }
}

std::vector<uint64> SettingsManager::GetMultiUI(const char *name)
{
 const MDFNCS *setting = FindSetting(name);
 const char *value = GetSetting(setting);

 if(setting->desc.type == MDFNST_MULTI_ENUM)
  return GetMultiEnum<uint64>(setting, value);
 else
  abort();
}

std::vector<int64> SettingsManager::GetMultiI(const char *name)
{
 const MDFNCS *setting = FindSetting(name);
 const char *value = GetSetting(setting);

 if(setting->desc.type == MDFNST_MULTI_ENUM)
  return GetMultiEnum<int64>(setting, value);
 else
  abort();
}


double SettingsManager::GetF(const char *name)
{
 double ret;

 MR_StringToDouble(GetSetting(FindSetting(name)), &ret);

 return ret;
}

bool SettingsManager::GetB(const char *name)
{
 return (bool)GetUI(name);
}

std::string SettingsManager::GetS(const char *name)
{
 const MDFNCS *setting = FindSetting(name);
 const char *value = GetSetting(setting);

 // Even if we're getting the string value of an enum instead of the associated numeric value, we still need
 // to make sure it's a valid enum
 // (actually, not really, since it's handled in other places where the setting is actually set)
 //if(setting->desc.type == MDFNST_ENUM)
 // GetEnum(setting, value);

 return(std::string(value));
}

bool SettingsManager::Set(const char *name, const char *value, bool NetplayOverride)
{
 MDFNCS *zesetting = FindSetting(name, true);

 if(zesetting)
 {
  ValidateSetting(value, &zesetting->desc);

  if(zesetting->value)
   free(zesetting->value);
  zesetting->value = strdup(value);

  // TODO, always call driver notification function, regardless of whether a game is loaded.
  if(zesetting->desc.ChangeNotification)
  {
   if(MDFNGameInfo)
    zesetting->desc.ChangeNotification(name);
  }
  return(true);
 }
 else
 {
  MDFN_Notify(MDFN_NOTICE_ERROR, _("Unknown setting \"%s\""), name);
  return(false);
 }
}

#if 0
// TODO after a game is loaded, but should we?
void MDFN_CallSettingsNotification(void)
{
 for(unsigned int x = 0; x < CurrentSettings.size(); x++)
 {
  if(CurrentSettings[x].ChangeNotification)
  {
   // TODO, always call driver notification function, regardless of whether a game is loaded.
   if(MDFNGameInfo)
    CurrentSettings[x].ChangeNotification(CurrentSettings[x].name);
  }
 }
}
#endif

bool SettingsManager::SetB(const char *name, bool value)
{
 char tmp[2];

 tmp[0] = value ? '1' : '0';
 tmp[1] = 0;

 return Set(name, tmp, false);
}

bool SettingsManager::SetI(const char *name, int64 value)
{
 char tmp[32];

 MDFN_sndec_s64(tmp, sizeof(tmp), value);

 return Set(name, tmp, false);
}

bool SettingsManager::SetUI(const char *name, uint64 value)
{
 char tmp[32];

 MDFN_sndec_u64(tmp, sizeof(tmp), value);

 return Set(name, tmp, false);
}

}
