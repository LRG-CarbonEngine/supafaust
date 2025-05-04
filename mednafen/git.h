#ifndef __MDFN_GIT_H
#define __MDFN_GIT_H

#include "video.h"
#include "VirtualFS.h"

#include "state.h"
#include "settings-common.h"

namespace Mednafen
{

struct FileExtensionSpecStruct
{
 const char *extension; // Example ".nes"

 /*
  Priorities used in heuristics to determine which file to load from a multi-file archive:

    10 = mai (Apple II, and others in future, system configuration file; should be highest)
     0 = woz, nes, unif, vboy, pce, gb, gba, sms, smc, sfc, etc.
   -10 = dsk, d13, etc.
   -20 = nsf, nsfe, hes, psf, ssf, gsf, etc.
   -30 = vb
   -40 = m3u (should be higher than single CD image format extensions, but lower than ripped music format extensions).
   -50 = ccd
   -60 = cue
   -70 = toc (not guaranteed to be cdrdao format, so prefer ccd or cue)
   -80 = bin (lower than everything due to be overused)
 */
 int priority;

 const char *description; // Example "iNES Format ROM Image"
};

enum
{
 MDFN_MSC_POWER = 0,
 MDFN_MSC_RESET = 1
};

typedef enum
{
 VIDSYS_NONE, // Can be used internally in system emulation code, but it is an error condition to let it continue to be
	      // after the Load() or LoadCD() function returns!
 VIDSYS_PAL,
 VIDSYS_PAL_M, // Same timing as NTSC, but uses PAL-style colour encoding
 VIDSYS_NTSC,
 VIDSYS_SECAM
} VideoSystems;

enum InputDeviceInputType : uint8
{
 IDIT_PADDING = 0,	// n-bit, zero

 IDIT_BUTTON,		// 1-bit
 IDIT_BUTTON_CAN_RAPID, // 1-bit

 IDIT_SWITCH,		// ceil(log2(n))-bit
			// Current switch position(default 0).
			// Persistent, and bidirectional communication(can be modified driver side, and Mednafen core and emulation module side)

 IDIT_STATUS,		// ceil(log2(n))-bit
			// emulation module->driver communication

 IDIT_AXIS,		// 16-bits; 0 through 65535; 32768 is centered position

 IDIT_POINTER_X,	// mouse pointer, 16-bits, signed - in-screen/window range before scaling/offseting normalized coordinates: [0.0, 1.0)
 IDIT_POINTER_Y,	// see: mouse_scale_x, mouse_scale_y, mouse_offs_x, mouse_offs_y

 IDIT_AXIS_REL,		// mouse relative motion, 16-bits, signed

 IDIT_BYTE_SPECIAL,

 IDIT_RESET_BUTTON,	// 1-bit

 IDIT_BUTTON_ANALOG,	// 16-bits, 0 - 65535

 IDIT_RUMBLE,		// 16-bits, lower 8 bits are weak rumble(0-255), next 8 bits are strong rumble(0-255), 0=no rumble, 255=max rumble.  Somewhat subjective, too...
			// It's a rather special case of game module->driver code communication.
};

enum : uint8
{
 IDIT_AXIS_FLAG_SQLR		= 0x01,	// Denotes analog data that may need to be scaled to ensure a more squareish logical range(for emulated analog sticks).
 IDIT_AXIS_FLAG_INVERT_CO	= 0x02,	// Invert config order of the two components(neg,pos) of the axis.
 IDIT_AXIS_REL_FLAG_INVERT_CO 	= IDIT_AXIS_FLAG_INVERT_CO,
 IDIT_FLAG_AUX_SETTINGS_UNDOC	= 0x80,
};

struct IDIIS_StatusState
{
	const char* ShortName;
	const char* Name;
	int32 Color;	// (msb)0RGB(lsb), -1 for unused.
};

struct IDIIS_SwitchPos
{
	const char* SettingName;
	const char* Name;
	const char* Description;
};

struct InputDeviceInputInfoStruct
{
	const char *SettingName;	// No spaces, shouldbe all a-z0-9 and _. Definitely no ~!
	const char *Name;
        int16 ConfigOrder;          	// Configuration order during in-game config process, -1 for no config.
	InputDeviceInputType Type;

	uint8 Flags;
	uint8 BitSize;
	uint16 BitOffset;

	union
	{
	 struct
	 {
	  const char *ExcludeName;	// SettingName of a button that can't be pressed at the same time as this button
					// due to physical limitations.
	 } Button;
	 //
	 //
	 //
	 struct
	 {
	  const char* sname_dir[2];
	  const char* name_dir[2];
	 } Axis;

	 struct
	 {
	  const char* sname_dir[2];
	  const char* name_dir[2];
	 } AxisRel;

         struct
         {
	  const IDIIS_SwitchPos* Pos;
	  uint32 NumPos;
	  uint32 DefPos;
         } Switch;

	 struct
	 {
	  const IDIIS_StatusState* States;
	  uint32 NumStates;
	 } Status;
	};
};

struct IDIISG : public std::vector<InputDeviceInputInfoStruct>
{
 IDIISG();
 IDIISG(std::initializer_list<InputDeviceInputInfoStruct> l);
 uint32 InputByteSize;
};

MDFN_HIDE extern const IDIISG IDII_Empty;

static INLINE InputDeviceInputInfoStruct IDIIS_Button(const char* sname, const char* name, int16 co, const char* exn)
{
 return { sname, name, co, IDIT_BUTTON, 0, 0, 0, { { exn } } };
}

static INLINE InputDeviceInputInfoStruct IDIIS_ButtonCR(const char* sname, const char* name, int16 co, const char* exn)
{
 return { sname, name, co, IDIT_BUTTON_CAN_RAPID, 0, 0, 0, { { exn } } };
}

struct InputDeviceInfoStruct
{
 const char *ShortName;
 const char *FullName;
 const char *Description;

 const IDIISG& IDII;

 unsigned Flags;

 enum
 {
  FLAG_KEYBOARD = (1U << 0)
 };
};

struct InputPortInfoStruct
{
 const char *ShortName;
 const char *FullName;
 const std::vector<InputDeviceInfoStruct> &DeviceInfo;
 const char *DefaultDevice;	// Default device for this port.
};

struct MemoryPatch;

struct CheatFormatStruct
{
 const char *FullName;		//"Game Genie", "GameShark", "Pro Action Catplay", etc.
 const char *Description;	// Whatever?

 bool (*DecodeCheat)(const std::string& cheat_string, MemoryPatch* patch);	// *patch should be left as initialized by MemoryPatch::MemoryPatch(), unless this is the
										// second(or third or whatever) part of a multipart cheat.
										//
										// Will throw an std::exception(or derivative) on format error.
										//
										// Will return true if this is part of a multipart cheat.
};

MDFN_HIDE extern const std::vector<CheatFormatStruct> CheatFormatInfo_Empty;

struct CheatInfoStruct
{
 //
 // InstallReadPatch and RemoveReadPatches should be non-NULL(even if only pointing to dummy functions) if the emulator module supports
 // read-substitution and read-substitution-with-compare style(IE Game Genie-style) cheats.
 //
 // See also "SubCheats" global stuff in mempatcher.h.
 //
 void (*InstallReadPatch)(uint32 address, uint8 value, int compare); // Compare is >= 0 when utilized.
 void (*RemoveReadPatches)(void);
 uint8 (*MemRead)(uint32 addr);
 void (*MemWrite)(uint32 addr, uint8 val);

 const std::vector<CheatFormatStruct>& CheatFormatInfo;

 bool BigEndian;	// UI default for cheat search and new cheats.
};

MDFN_HIDE extern const CheatInfoStruct CheatInfo_Empty;

struct EmulateSpecStruct
{
	// The surface pixel data is the framebuffer written to by the system emulation code, and must be aligned to at least what malloc() guarantees.
	// Pitch must either be == MDFNGI::fb_width, or a value > MDFNGI::fb_width that preserves the pixel data alignment requirement for successive rows.
	// Height must be >= MDFNGI::fb_height
	MDFN_Surface* surface = nullptr;

	// Will be set to true if the video pixel format has changed since the last call to Emulate(), false otherwise.
	// Will be set to true on the first call to the Emulate() function/method
	//
	// Driver-side can set it to true if it has changed the custom palette.
	bool VideoFormatChanged = false;

	// Set by the system emulation code every frame, to denote the horizontal and vertical offsets of the image, and the size
	// of the image.  If the emulated system sets the elements of LineWidths, then the width(w) of this structure
	// is ignored while drawing the image.
	MDFN_Rect DisplayRect = { 0, 0, 0, 0 };

	// Pointer to an array of int32, number of elements = fb_height, set by the driver code.  Individual elements written
	// to by system emulation code.  If the emulated system doesn't support multiple screen widths per frame, or if you handle
	// such a situation by outputting at a constant width-per-frame that is the least-common-multiple of the screen widths, then
	// you can ignore this.  If you do wish to use this, you must set all elements every frame.
	int32 *LineWidths = nullptr;

	// Set(optionally) by emulation code.  If InterlaceOn is true, then assume field height is 1/2 DisplayRect.h, and
	// only every other line in surface (with the start line defined by InterlacedField) has valid data
	// (it's up to internal Mednafen code to deinterlace it).
	bool InterlaceOn = false;
	bool InterlaceField = false;

	// Skip rendering this frame if true.  Set by the driver code.
	int skip = false;

	//
	// If sound is disabled, the driver code must set SoundRate to false, SoundBuf to NULL, SoundBufMaxSize to 0.

        // Will be set to true if the sound format(only rate for now, at least) has changed since the last call to Emulate(), false otherwise.
        // Will be set to true on the first call to the Emulate() function/method
	bool SoundFormatChanged = false;

	// Sound rate.  Set by driver side.
	double SoundRate = 0;

	// Pointer to sound buffer, set by the driver code, that the emulation code should render sound to.
	// Guaranteed to be at least 500ms in length, but emulation code really shouldn't exceed 40ms or so.  Additionally, if emulation code
	// generates >= 100ms,
	// DEPRECATED: Emulation code may set this pointer to a sound buffer internal to the emulation module.
	int16 *SoundBuf = nullptr;

	// Maximum size of the sound buffer, in frames.  Set by the driver code.
	int32 SoundBufMaxSize = 0;

	// Number of frames currently in internal sound buffer.  Set by the system emulation code, to be read by the driver code.
	int32 SoundBufSize = 0;
	int32 SoundBufSizeALMS = 0;	// SoundBufSize value at last MidSync(), 0
					// if mid sync isn't implemented for the emulation module in use.

	// Number of cycles that this frame consumed, using MDFNGI::MasterClock as a time base.
	// Set by emulation code.
	int64 MasterCycles = 0;
	int64 MasterCyclesALMS = 0;	// MasterCycles value at last MidSync(), 0
					// if mid sync isn't implemented for the emulation module in use.


	// [dg added 9/6/2023 - handle filter easier]
	int hFilter = 1;
};

typedef enum
{
 MODPRIO_INTERNAL_EXTRA_LOW = 0,	// For "cdplay" module, mostly.

 MODPRIO_INTERNAL_LOW = 10,
 MODPRIO_EXTERNAL_LOW = 20,
 MODPRIO_INTERNAL_HIGH = 30,
 MODPRIO_EXTERNAL_HIGH = 40
} ModPrio;

// 4-byte RGB-colorspace xxx888 support is required by all emulation modules, so it's not specified here.
enum
{
 EVFSUPPORT_NONE    = 0,

 EVFSUPPORT_8BPP    = 0x01,	// Palette
 EVFSUPPORT_RGB555  = 0x02,
 EVFSUPPORT_RGB565  = 0x04
};

struct GameFile
{

 const std::string dir;	// path = vfs->eval_fip(dir, whatever);
 Stream* const stream;

 const std::string ext;		// Lowercase.
 const std::string fbase;

 // Outside archive.
 struct
 {
  const std::string dir;
  const std::string fbase;
 } outside;
};

struct DesiredInputType
{
 DesiredInputType() : device_name(nullptr) { }
 DesiredInputType(const char* dn) : device_name(dn) { }

 // nullptr for don't care
 const char* device_name;

 // Should override any *.defpos settings.
 std::map<std::string, uint32> switches;
};

typedef struct
{
 /* Private functions to Mednafen.  Do not call directly
    from the driver code, or else bad things shall happen.  Maybe.  Probably not, but don't
    do it(yet)!
 */
 // Short system name, lowercase a-z, 0-9, and _ are the only allowable characters!
 const char *shortname;

 // Full system name.  Preferably English letters, but can be UTF8
 const char *fullname;

 // Pointer to an array of FileExtensionSpecStruct, with the last entry being { NULL, NULL } to terminate the list.
 // This list is used to make best-guess choices, when calling the TestMagic*() functions would be unreasonable, such
 // as when scanning a ZIP archive for a file to load.  The list may also be used in the future for GUI file open windows.
 const FileExtensionSpecStruct *FileExtensions;

 ModPrio ModulePriority;

 #ifdef WANT_DEBUGGER
 DebuggerInfoStruct *Debugger;
 #else
 void *Debugger;
 #endif
 const std::vector<InputPortInfoStruct> &PortInfo;

 //
 // throws exception on fatal error.
 //
 // gf->stream's stream position is guaranteed to be 0 when this function is called.
 //
 // any streams created via vfs->open() should be delete'd before returning(use std::unique_ptr<Stream>)
 //
 void (*Load)(GameFile* gf);

 //
 // Return true if the file is a recognized type, false if not.
 //
 // gf->stream's position is guaranteed to be 0 when this function is called.
 //
 // TODO/REEVALUATE: gf->dir will be an empty string, and gf->vfs will be nullptr.
 //
 bool (*TestMagic)(GameFile* gf);

 //
 // CloseGame() must only be called after a matching Load() or LoadCD() completes successfully.  Calling it before Load*(), or after Load*() throws an exception or
 // returns error status, may cause undesirable effects such as nonvolatile memory save game file corruption.
 //
 void (*CloseGame)(void);

 void (*SetLayerEnableMask)(uint64 mask);	// Video
 const char *LayerNames;

 void (*SetChanEnableMask)(uint64 mask);	// Audio(TODO, placeholder)
 const char *ChanNames;

 const CheatInfoStruct& CheatInfo;

 bool SaveStateAltersState;	// true for bsnes and some libco-style emulators, false otherwise.

 // Main save state routine, called by the save state code in state.cpp.
 // When saving, load is set to 0.  When loading, load is set to the version field of the save state being loaded.
 //
 // data_only is true when the save state data is temporary, such as being saved into memory for state rewinding.
 //
 // IMPORTANT: Game module save state code should avoid dynamically allocating memory and doing other things that can throw exceptions, unless
 // it's done in a manner that ensures that the variable sanitizing code will run after the call to MDFNSS_StateAction().
 //
 void (*StateAction)(StateMem *sm, const unsigned load, const bool data_only);

 void (*Emulate)(EmulateSpecStruct *espec);

 void (*SetInput)(unsigned port, const char *type, uint8* data);
 void (*SetMedia)(uint32 drive_idx, uint32 state_idx, uint32 media_idx, uint32 orientation_idx);

 void (*DoSimpleCommand)(int cmd);
 //uint8* (*GetNV)(uint32* size);

 // Called when netplay starts, or the controllers controlled by local players changes during
 // an existing netplay session.  Called with (uint64)-1 when netplay ends.
 // (For future use in implementing portable console netplay)
 void (*NPControlNotif)(uint64 c);

 uint8_t(*PEEKRAM)(uint32 addr);
 void(*POKERAM)(uint32 addr, uint8_t value);

 const MDFNSetting *Settings;

 // Time base for EmulateSpecStruct::MasterCycles
 // MasterClock must be >= MDFN_MASTERCLOCK_FIXED(1.0)
 // All or part of the fractional component may be ignored in some timekeeping operations in the emulator to prevent integer overflow,
 // so it is unwise to have a fractional component when the integral component is very small(less than say, 10000).
 #define MDFN_MASTERCLOCK_FIXED(n)	((int64)((double)(n) * (1LL << 32)))
 int64 MasterClock;

 // Nominal frames per second * 65536 * 256, truncated.
 // May be deprecated in the future due to many systems having slight frame rate programmability.
 uint32 fps;

 // Additional video format support, in addition to the required 4-byte RGB-colorspace xxx888 support.
 uint32 ExtraVideoFormatSupport; // = EVFSUPPORT_NONE

 // multires is a hint that, if set, indicates that the system has fairly programmable video modes(particularly, the ability
 // to display multiple horizontal resolutions, such as the PCE, PC-FX, or Genesis).  In practice, it will cause the driver
 // code to set the linear interpolation on by default.
 //
 // lcm_width and lcm_height are the least common multiples of all possible
 // resolutions in the frame buffer as specified by DisplayRect/LineWidths(Ex for PCE: widths of 256, 341.333333, 512,
 // lcm = 1024)
 //
 // nominal_width and nominal_height specify the resolution that Mednafen should display
 // the framebuffer image in at 1x scaling, scaled from the dimensions of DisplayRect, and optionally the LineWidths array
 // passed through espec to the Emulate() function.
 //
 int multires;

 int lcm_width;
 int lcm_height;

 void *dummy_separator;	//

 int nominal_width;
 int nominal_height;

 int fb_width;		// Width of the framebuffer(not necessarily width of the image).  MDFN_Surface width should be >= this.
 int fb_height;		// Height of the framebuffer passed to the Emulate() function(not necessarily height of the image)

 int soundchan; 	// Number of output sound channels.  Only values of 1 and 2 are currently supported.

 uint8 MD5[16];

 VideoSystems VideoSystem;

 std::vector<DesiredInputType> DesiredInput; // Desired input devices and default switch positions for the input ports

 double IdealSoundRate;

 // For mouse relative motion.
 double mouse_sensitivity;


 //
 // For absolute coordinates(IDIT_X_AXIS and IDIT_Y_AXIS), usually mapped to a mouse(hence the naming).
 //
 float mouse_scale_x, mouse_scale_y;
 float mouse_offs_x, mouse_offs_y;
} MDFNGI;

}

#endif
