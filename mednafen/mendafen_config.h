#define MDFN_DISABLE_PICPIE_ERRWARN 1
#define MDFN_SNES_FAUST_SUPAFAUST 1
#define MDFN_SNES_FAUST_SPC700_IPL_HLE 1
#define MDFN_SNES_FAUST_SKETCHYSPC700OPT 1
//#define MDFN_SNES_FAUST_SKETCHYPPUOPT 1
#define HAVE_SEM_TIMEDWAIT 1

#if !defined(ANDROID) && !defined(__APPLE__)
#define PTHREAD_AFFINITY_NP cpu_set_t
#endif

#define MEDNAFEN_VERSION "1.29.0"
#define MEDNAFEN_VERSION_NUMERIC 0x00102900

#define WANT_SNES_FAUST_EMU 1

#define PSS_STYLE 1

#if PSS_STYLE == 2
//#if defined(_MSC_VER) || defined(_WIN32)
// #define WIN32
//#endif
 #define PSS "\\"
 #define MDFN_PS '\\'
#elif PSS_STYLE == 1
 #define PSS "/"
 #define MDFN_PS '/'
#endif

