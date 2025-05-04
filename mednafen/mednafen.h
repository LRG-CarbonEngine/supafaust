#ifndef __MDFN_MEDNAFEN_H
#define __MDFN_MEDNAFEN_H

#include "mendafen_types.h"
#include "memory.h"
#include "video.h"
#include "state.h"
#include "git.h"
#include "settings-common.h"
#include "NativeVFS.h"
#include "MemoryStream.h"
#include "string/mendafen_string.h"


namespace Mednafen
{


// Points to a dynamically-allocated object, created with data from Emulated*.
MDFN_HIDE extern MDFNGI* MDFNGameInfo;

enum MDFN_NoticeType : uint8
{
 // Terse status message for a user-initiated action during runtime(state loaded, screenshot saved, etc.).
 MDFN_NOTICE_STATUS = 0,

 // Something went slightly wrong, but we can mostly recover from it, but the user should still know because
 // it may cause behavior to differ from desired.
 MDFN_NOTICE_WARNING,

 // Something went horribly wrong(user-triggered action, or otherwise); generally prefer throwing MDFN_Error()
 // over sending this, where possible/applicable.
 MDFN_NOTICE_ERROR
};

void MDFN_Notify(MDFN_NoticeType t, const char* format, ...) MDFN_FORMATSTR(gnu_printf, 2, 3);

// Verbose status and informational messages, primarily during startup and exit.
void MDFN_printf(const char *format, ...) MDFN_FORMATSTR(gnu_printf, 1, 2);

void MDFN_StateAction(StateMem *sm, const unsigned load, const bool data_only);

MDFN_HIDE extern std::vector<MDFNGI *>MDFNSystems;

/* Indent stdout newlines +- "indent" amount */
void MDFN_indent(int indent);
struct MDFN_AutoIndent
{
 INLINE MDFN_AutoIndent() : indented(0) { }
 INLINE MDFN_AutoIndent(int amount) : indented(amount) { MDFN_indent(indented); }
 INLINE ~MDFN_AutoIndent() { MDFN_indent(-indented); }

 //INLINE void indent(int indoot) { indented += indoot; MDFN_indent(indoot); }
 private:
 int indented;
};
void MDFN_printf(const char *format, ...) MDFN_FORMATSTR(gnu_printf, 1, 2);

#define MDFNI_printf MDFN_printf

// MDFN_NOTICE_ERROR may block(e.g. for user confirmation), other notice types should be as non-blocking as possible.
void MDFND_OutputNotice(MDFN_NoticeType t, const char* s);

// Output from MDFN_printf(); fairly verbose informational messages.
void MDFND_OutputInfo(const char* s);

// Synchronize virtual time to actual time using members of espec:
//
//  MasterCycles and MasterCyclesALMS (coupled with MasterClock of MDFNGI)
//   and/or
//  SoundBuf, SoundBufSize, and SoundBufSizeALMS
//
// ...and after synchronization, update the data pointed to by the pointers passed to MDFNI_SetInput().
// DO NOT CALL MDFN_* or MDFNI_* functions from within MDFND_MidSync().
// Calling MDFN_printf(), MDFN_DispMessage(),and MDFND_PrintError() are ok, though.
//
// If you do not understand how to implement this function, you can leave it empty at first, but know that doing so
// will subtly break at least one PC Engine game(Takeda Shingen), and raise input latency on some other PC Engine games.
enum : unsigned
{
 MIDSYNC_FLAG_NONE		= 0,
 MIDSYNC_FLAG_UPDATE_INPUT	= 1U << 0,
 MIDSYNC_FLAG_SYNC_TIME		= 1U << 1,
};
void MDFN_MidSync(EmulateSpecStruct *espec, const unsigned flags = MIDSYNC_FLAG_UPDATE_INPUT | MIDSYNC_FLAG_SYNC_TIME);
void MDFND_MidSync(const EmulateSpecStruct *espec, const unsigned flags);

void MDFNI_Reset(void);
void MDFNI_Power(void);

//
MDFNGI* MDFNI_LoadGame(GameFile* gf) MDFN_COLD;

//
bool MDFNI_Initialize(void) MDFN_COLD;


uint8_t MDFNI_PeekWRAM(uint32_t addr) MDFN_COLD;
void MDFNI_PokeWRAM(uint32_t addr, uint8_t value) MDFN_COLD;


/* Emulates a frame. */
void MDFNI_Emulate(EmulateSpecStruct *espec);

/* Closes currently loaded game */
void MDFNI_CloseGame(void) MDFN_COLD;

/* Deallocates all allocated memory.  Call after MDFNI_Emulate() returns. */
void MDFNI_Kill(void) MDFN_COLD;

void MDFNI_SetLayerEnableMask(uint64 mask);

uint8* MDFNI_SetInput(const uint32 port, const uint32 type);

void MDFNI_AddCheat(const MemoryPatch& patch);
void MDFNI_DelCheats(void);
 
//
uint64 MDFN_GetSettingUI(const char *name);
int64 MDFN_GetSettingI(const char *name);
double MDFN_GetSettingF(const char *name);
bool MDFN_GetSettingB(const char *name);
std::string MDFN_GetSettingS(const char *name);

std::vector<uint64> MDFN_GetSettingMultiUI(const char *name);
std::vector<int64> MDFN_GetSettingMultiI(const char *name);

uint64 MDFN_GetSettingUI(const std::string& name);
int64 MDFN_GetSettingI(const std::string& name);
double MDFN_GetSettingF(const std::string& name);
bool MDFN_GetSettingB(const std::string& name);
std::string MDFN_GetSettingS(const std::string& name);
std::vector<uint64> MDFN_GetSettingMultiUI(const std::string& name);
std::vector<int64> MDFN_GetSettingMultiI(const std::string& name);

bool MDFNI_SetSetting(const char *name, const char *value, bool NetplayOverride = false);
bool MDFNI_SetSetting(const char *name, const std::string& value, bool NetplayOverride = false);
bool MDFNI_SetSetting(const std::string& name, const std::string& value, bool NetplayOverride = false);

bool MDFNI_SetSettingB(const char *name, bool value);
bool MDFNI_SetSettingB(const std::string& name, bool value);

bool MDFNI_SetSettingUI(const char *name, uint64 value);
bool MDFNI_SetSettingUI(const std::string& name, uint64 value);

}

#endif
