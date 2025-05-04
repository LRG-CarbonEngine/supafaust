#include "mendafen_types.h"

#if defined (SNES_ENGINE)
 
#if defined(PLATFORM_PC)

#include <windows.h>
#include <wchar.h>
#include <codecvt>

std::wstring SNESUTF8ToWide(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
}

std::string SNESWideToUTF8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(str);
}

#endif 
 
#include <mednafen/mednafen.h>
#include <mednafen/string/mendafen_string.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/ExtMemStream.h>

#include <mednafen/MThreading.h>
 
using namespace Mednafen;

#if defined (thread_audio)
SDL_Thread* g_AudioThread;
#endif
 
static int ports_active = 1;
static uint8* port_data[16] = { 0 };
static MDFNGI* cgi = nullptr;

static double SoundRate;
static int32 SoundBufMaxSize;
static std::unique_ptr<int16[]> SoundBuf;
static std::unique_ptr<int32[]> lw;
static std::unique_ptr<MDFN_Surface> surf;

static size_t SaveStateSize;
static uint64 affinity_save;
 
static int hFilter = 1;
 

// rewind stuff
void handle_rewind(bool doRewind); 
 
void rewind_reset()
{
 
}

void handleForwardsFrame()
{
 
}

void handleBackwardFrame()
{
 
}

void rewind_push_state()
{
 
}

void rewind_pop_state()
{
 
}

void rewind_load_top_state()
{
	rewind_handle_state(false);
}

void rewind_handle_state(bool isSave)
{
	// like save/load state but just handle it in memory
	uint8_t* buf = rewindBuf + (0x44000 * rewindBufIdx);

	if (isSave == true)
	{
		auto t1 = std::chrono::high_resolution_clock::now();
		
		ExtMemStream ssms(buf, SaveStateSize);
		MDFNSS_SaveSM(&ssms, true);
		auto t2 = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
		state_size_map[rewindBufIdx].filesize =SaveStateSize;
		state_size_map[rewindBufIdx].width = 256;
		state_size_map[rewindBufIdx].height = 240;
		hasSave = true;
	}
	else
	{
		if (hasSave == true)
		{
			auto t1 = std::chrono::high_resolution_clock::now();

			int64_t size = state_size_map[rewindBufIdx].filesize;
			int64_t offset = 0;

			if (size)
			{
				ExtMemStream ssms(buf, SaveStateSize);
				MDFNSS_LoadSM(&ssms, true);
			}

			auto t2 = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double, std::milli> fp_ms = t2 - t1;

			//printf("Rewind deserialize() : %f\n", fp_ms.count());
		}
	}
}

void handle_rewind(bool doRewind)
{
 
	isSnesRewinding = doRewind;

	if (doRewind == true && SNESRewindRecordedFrames > 0)
	{
		handleBackwardFrame();
		handleBackwardFrame();
	}
	else
	{
		handleForwardsFrame();
	}
}

//// end rewind stuff


struct snesEngineAudio
{
    int audioTransferBufferUsed;
    uint32_t* audioTransferBuffer;
    int audio_samples_written;
    unsigned int g_audio_frames;
    bool audioRunning;
    float audioVol;
};

static snesEngineAudio snesAudio;

void snes_audio_queue(int16_t* finalWave, int length)
{
  
}

void snes_audio_thread_main(void*)
{
 
}

 
static void UpdateInput(void)
{
 
    for (size_t port = 0; port < ports_active; port++)
    {
        uint16 bs;
        bs = 0;
        switch (port)
        {
		case 0:
			break;
		case 1:
			if (MDFN_UNLIKELY(snes2PlayerEnabled))
			{
 
			}
			break;
        }
      
        MDFN_en16lsb<false>(port_data[port], bs);
    }
}
 
SNESEngine::SNESEngine()
    : trophyCount(0),
    trophyCheckFrameCount(0)
{
    memset(&snesAudio, 0, sizeof(snesAudio));
}

SNESEngine::~SNESEngine()
{
}

void SNESEngine::initSaveStates(std::string gameName, int gameIndex)
{
 
 
}

#if defined (thread_audio)

int snes_audio_thread_main_win(void*)
{
    // opening an audio device:
    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = 48000;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 2;
    audio_spec.samples = 48000 / 60 / 2;
    audio_spec.callback = NULL;

    audio_device = SDL_OpenAudioDevice(
        NULL, 0, &audio_spec, NULL, 0);

    SDL_PauseAudioDevice(audio_device, 0);

    Uint32 time1, time2, dt, fps_time = 0, fps_count = 0, delay = 15;
    double FPS;
    time1 = SDL_GetTicks();
    while (audioRunning)
    {
        time2 = SDL_GetTicks();
        dt = time2 - time1;
        time1 = time2;

        SystemNode->run();

        SDL_Delay(delay);

        //FPS
        fps_time += dt; fps_count++;
        if (fps_time > 200)
        {
            FPS = (double)fps_count / fps_time * 1000;
            if (FPS > 60) delay++;
            else if (delay > 0) delay--;
            fps_time = 0; fps_count = 0;
        }
    }
}

#endif

void reset2p()
{
 
}

void SNESEngine::start(std::string romName, GameTitle* pGameTitle)
{
	MDFNI_Initialize();

	rewindFrameNum = 0;
	this->frameNum = 0;
	std::string newName = std::string(romName.c_str());

	MDFN_PixelFormat nf = MDFN_PixelFormat::RGB16_565;

	ports_active = 2;

	MDFNI_SetSetting("snes_faust.spex", "0");
	MDFNI_SetSetting("snes_faust.spex.sound", "0");

	MDFNI_SetSetting("snes_faust.input.sport1.multitap", "0");
	MDFNI_SetSetting("snes_faust.input.sport2.multitap", "0");
	MDFNI_SetSetting("snes_faust.renderer", "MT");

	auto filter = g_System->GetCurrentFilter();
 

	std::vector<uint8_t> gamerom;

	int romSize = 0;

	bool bUsePakFile = (pGameTitle != NULL) ? pGameTitle->usePakFile : false;
	 
	if (bUsePakFile)
	{
		//
		//	PAK File
		//
		std::string pakFile = "";
#if defined (PLATFORM_PC)
		pakFile.append(SDL_GetBasePath());
 
#endif
		pakFile.append(pGameTitle->pakFileName.c_str());

		const char* assetName = pGameTitle->romName.c_str();

		uint8_t* romBuffer = NULL;

		auto* archive = new CarbonPakFile();
		archive->PakLoadOneFile(pakFile.c_str(), assetName, (void**)&romBuffer, &romSize);

		for (int i = 0; i < romSize; i++) {
			gamerom.push_back((uint8_t)romBuffer[i]);
		}

		free(romBuffer);
		romBuffer = NULL;
		delete archive;
	}
	else
	{
		//
		//	Uncompressed File
		//


		std::ifstream romFileStream(romName, std::ios::in | std::ios::binary | std::ios::ate);

		if (romFileStream.is_open()) {
			// Create the data array
			romSize = (int)romFileStream.tellg();
			romFileStream.seekg(0, std::ios::beg);	// Set file stream to beginning
				 // Copy data to array
			for (int i = 0; i < romSize; i++) {
				char oneByte;
				romFileStream.read((&oneByte), 1);
				gamerom.push_back((uint8_t)oneByte);
			}

			romFileStream.close();
		}
	}
 
	ExtMemStream gs(gamerom.data(), romSize);
	GameFile gf({ "/", &gs,"sfc", "game", { "/", "game"} });
	cgi = MDFNI_LoadGame(&gf);

	for (size_t i = 0; i < ports_active; i++)
	{
		port_data[i] = MDFNI_SetInput(i/*port*/, 1/*device type id, gamepad*/);
	}
 
	SoundRate = 48080;
 
	SoundBufMaxSize = ceil((SoundRate + 9) / 10);
	SoundBuf.reset(new int16[SoundBufMaxSize * cgi->soundchan]);

	lw.reset(new int32[cgi->fb_height]);
	memset(lw.get(), 0, sizeof(int32) * cgi->fb_height);
	surf.reset(new MDFN_Surface(nullptr, cgi->fb_width, cgi->fb_height, cgi->fb_width / 2, nf));
	{
		MemoryStream ssms(65536);
		MDFNSS_SaveSM(&ssms, true);

		SaveStateSize = ssms.size(); // FIXME for other than snes_faust, may grow.
		SaveStateSize += 64 * 2 * 4 * 2; // OwlBuffer-related kludge, leftover, 2 channels(l/r), 4 bytes per sample, *2 again for MSU1
	}
 
#if defined (PLATFORM_PC)
	snesAudio.audioRunning = true;
	audioFadeIn = 1;
	snesAudio.audioVol = 0.0f;
	snesAudio.audioTransferBuffer = (uint32_t*)malloc(AUDIO_TRANSFERBUF_SIZE * sizeof(uint32_t));
	memset(snesAudio.audioTransferBuffer, 0, AUDIO_TRANSFERBUF_SIZE * sizeof(uint32_t));
	snesAudio.audioTransferBufferUsed = AUDIO_TRANSFERBUF_SIZE;
	memset(snes_out_buf, 0, snes_buffer_safe);

#if defined (thread_audio)
	g_AudioThread = SDL_CreateThread(snes_audio_thread_main_win, "SFC Audio Thread", (void*)0);
	SDL_SetThreadPriority(SDL_ThreadPriority::SDL_THREAD_PRIORITY_HIGH);
#else
	SDL_AudioSpec audio_spec;
	SDL_zero(audio_spec);
	audio_spec.freq = 48000;
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = 2;
	audio_spec.samples = 48000 / 60;
	audio_spec.callback = NULL;

	audio_device = SDL_OpenAudioDevice(
		NULL, 0, &audio_spec, NULL, 0);

	SDL_PauseAudioDevice(audio_device, 0);

#endif
 
	reset();
}

void SNESEngine::stop()
{
	snesAudio.audioRunning = false;

	if (rewind_style == REWIND_STYLE_DYNAMIC)
	{
		if (rewindBuf)
		{
			free(rewindBuf);
			rewindBuf = NULL;
		}
	}
	else if (rewind_style == REWIND_STYLE_STATIC)
	{
		rewindWorkerContext.running = false;
		rewindWorkerContext.isReady = false;

#if defined(PLATFORM_PC)
		if (rewindWorkerContext.conditionLock != NULL)
		{
			SDL_DestroyMutex(rewindWorkerContext.conditionLock);
			rewindWorkerContext.conditionLock = NULL;
		}
 

		m_rewindStack.clear();
		m_rewindStack.shrink_to_fit();

		recordedFrames = 0;
 
	}
 
    if (cgi)
    {
        MDFNI_CloseGame();
        cgi = nullptr;
    }

    lw.reset(nullptr);
    surf.reset(nullptr);

	for (size_t i = 0; i < 16; i++)
	{
		port_data[i] = nullptr;
	}

    SoundBufMaxSize = 0;
    SoundBuf.reset(nullptr);

    if (affinity_save)
    {
        MThreading::Thread_SetAffinity(nullptr, affinity_save);
        //
        affinity_save = 0;
    }

    MDFNI_Kill();

 
}
 
void SNESEngine::signal_rewind()
{
 
}

void SNESEngine::rewind_load_state_at_index(int idx)
{
	rewind_handle_state(false);
}

typedef uint32(*snes_event_handler)(const uint32 timestamp);
#include "ppu.h"

 
// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 2208)
const int epd_bitmap_allArray_LEN = 3;
const unsigned char* epd_bitmap_allArray[3] = {
	epd_bitmap_presstomenu,
	epd_bitmap_presstooptions,
	epd_bitmap_presstoplus
};

static bool renderContinueMessage = 0;
 
uint8_t fontP[64] = { 0xFF,0xFE,0xFF,0x82,0xFF,0x82,0xFF,0xFE,0xFF,0x80,0xFF,0x80,0xFF,0x80,0xFF,0x00,0xFE,0xFF,0xFE,0xFF,0xFE,0xFF,0xFE,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontR[64] = { 0xFF,0xFE,0xFF,0x82,0xFF,0x82,0xFF,0xFE,0xFF,0xE0,0xFF,0x98,0xFF,0x86,0xFF,0x00,0xFF,0xFF,0xFB,0xFB,0xFB,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontE[64] = { 0xFF,0xFE,0xFF,0x80,0xFF,0x80,0xFF,0xF8,0xFF,0x80,0xFF,0x80,0xFF,0xFE,0xFF,0x00,0xFE,0xFF,0xFE,0xFF,0xFE,0xFF,0xFE,0xF9,0xFE,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontS[64] = { 0xFF,0xFE,0xFF,0x82,0xFF,0x80,0xFF,0xFE,0xFF,0x02,0xFF,0x82,0xFF,0xFE,0xFF,0x00,0xFE,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontT[64] = { 0xFF,0xFE,0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0xFF,0xEE,0xFB,0xEF,0xFF,0xEF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontN[64] = { 0xFF,0xC2,0xFF,0xA2,0xFF,0x92,0xFF,0x8A,0xFF,0x86,0xFF,0x82,0xFF,0x82,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontO[64] = { 0xFF,0xFE,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0xFE,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontC[64] = { 0xFF,0xFE,0xFF,0x82,0xFF,0x80,0xFF,0x80,0xFF,0x80,0xFF,0x82,0xFF,0xFE,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontI[64] = { 0xFF,0xE0,0xFF,0x40,0xFF,0x40,0xFF,0x40,0xFF,0x40,0xFF,0x40,0xFF,0xE0,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontU[64] = { 0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0x82,0xFF,0xFE,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontPlus[64] = { 0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0xFE,0xFF,0x10,0xFF,0x10,0xFF,0x10,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
uint8_t fontMenu[64] = { 0xFF,0x00,0xFF,0xFE,0xFF,0x00,0xFF,0xFE,0xFF,0x00,0xFF,0xFE,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

inline void RenderFont(int8_t ord, uint16_t memoryLoc)
{


	uint16_t* pbuff;

	switch (ord)
	{
		case 0:
			pbuff = (uint16_t*)fontPlus;
			break;
		case 1:
			pbuff = (uint16_t*)fontMenu;
			break;

		case 0x43: // C 
			pbuff = (uint16_t*)fontC;
			break;

		case 0x45: // E 
			pbuff = (uint16_t*)fontE;
			break;
		case 0x49: // I 
			pbuff = (uint16_t*)fontI;
			break;

		case 0x4D: // N
			pbuff = (uint16_t*)fontN;
			break;
		case 0x4F: // O
			pbuff = (uint16_t*)fontO;
			break;
		case 0x50: // P
			pbuff=(uint16_t*)fontP;
			break;
		case 0x52: // S
			pbuff = (uint16_t*)fontS;
			break;
		case 0x53: // T
			pbuff = (uint16_t*)fontT;
			break;
		case 0x54: // U
			pbuff = (uint16_t*)fontU;
			break;
		default:
			pbuff = (uint16_t*)fontR;
			break;

	}

	for (int i = 0; i < 32; i++) 
	{
		MDFN_IEN_SNES_FAUST::PPU_PokeVRAM(memoryLoc+i, pbuff[i],0);
	}

}

void ApplyPatchPreRender(EmulateSpecStruct* espec)
{
 
}

static EmulateSpecStruct espec_static;
static NO_INLINE void DoFrame(MDFN_Surface* s)
{
    UpdateInput();
 
	auto rewind_style = g_SNESEngine->get_rewind_style();
 
	espec_static = { 0 };
 
    lw[0] = ~0;	// Must be ~0 and not another value.
	espec_static.surface = s;
	espec_static.LineWidths = lw.get();
	espec_static.skip = false; //!(lr_avenable & 0x1);		// Skips drawing the frame if true; espec.surface and espec.LineWidths must still be valid when true, however.
	espec_static.hFilter = hFilter;
	if (g_SNESEngine->is_rewinding() == 0)
	{
		espec_static.SoundRate = SoundRate;
		espec_static.SoundBuf = SoundBuf.get(); // Should have enough room for 100ms of sound to be on the safe side.
		espec_static.SoundBufMaxSize = SoundBufMaxSize;
	}
	else
	{
		// no sound if we are rewinding
 
	}

    MDFNI_Emulate(&espec_static);
	ApplyPatchPreRender(&espec_static);
	espec_static.DisplayRect.y * espec_static.surface->pitchinpix), 0, 2);
##if defined(PLATFORM_PC)
	Texture2D SNESTexture = ResourceManager::GetTexture("SNESFrameBuffer");
	SNESTexture.Set((lw[0] == ~0) ? espec_static.DisplayRect.w : lw[espec_static.DisplayRect.y],
		espec_static.DisplayRect.h, (uint8_t*)(espec_static.surface->pix<uint16>() + espec_static.DisplayRect.x + espec_static.DisplayRect.y * espec_static.surface->pitchinpix), 0, GL_UNSIGNED_SHORT_5_6_5);
#endif 

}
 
void SNESEngine::run_core()
{
    if (!g_System->isEmulatorActive)
    {
        return;
    }
 
	DoFrame(surf.get());

	this->frameNum++;
	rewindFrameNum++;
 
}
 
#include "stb_image_write.h"
 
void SNESEngine::rewind_handle_state(bool isSave)
{
 
}


void SNESEngine::save_state(uint8_t slot, int gameIndex)
{

}

void SNESEngine::load_state(uint8_t slot, int gameIndex)
{
	 
}

void SNESEngine::delete_state(uint8_t slot, int gameIndex)
{
 
}

void SNESEngine::reset()
{
	MDFNI_Power(); // cold reset. wipes WRAM
 
	fb_width = 256;
	fb_height = 224;

	snes2PlayerEnabled = false;
	reset2p();
}

void SNESEngine::pause()
{
 
}
 
void SNESEngine::set_blend(std::string blendType)
{  	 
	if (blendType == "512_blend")
		hFilter = 5;
	else
		hFilter = 1;

	// hack! hack! hack!  
	// we need to run one frame so it can apply the new blending

	EmulateSpecStruct espec;
 
	lw[0] = ~0;	// Must be ~0 and not another value.
	espec.surface = surf.get();
	espec.LineWidths = lw.get();
	espec.skip = false;  
	espec.hFilter = hFilter;
 
	MDFNI_Emulate(&espec);
 
}
#endif 

static void trophyComplete(std::string description, int trophyID)
{
	printf("Unlocking Trophy: %s, id = %d\n", description.c_str(), trophyID);
}

void SNESEngine::check2Player()
{
 
}

void SNESEngine::checkTrophies()
{
 
}
 

using namespace Mednafen;

#include <mednafen/mednafen.h>
#include <mednafen/string/mendafen_string.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/ExtMemStream.h>

#include <mednafen/MThreading.h>

// MDFN_NOTICE_STATUS, MDFN_NOTICE_WARNING, MDFN_NOTICE_ERROR
void Mednafen::MDFND_OutputNotice(MDFN_NoticeType t, const char* s)
{
	///printf("%s\n", s);
}

// Output from MDFN_printf(); fairly verbose informational messages.
void Mednafen::MDFND_OutputInfo(const char* s)
{
///	printf("%s\n", s);
}
 
void Mednafen::MDFND_MidSync(const EmulateSpecStruct* espec, const unsigned flags)
{

	int16_t* const sbuf = espec->SoundBuf + espec->SoundBufSizeALMS;
	const int32 scount = espec->SoundBufSize - espec->SoundBufSizeALMS;

	if (scount)
	{
		snes_audio_queue(sbuf, scount * 2);
	}
	//
	if (flags & MIDSYNC_FLAG_UPDATE_INPUT)
		UpdateInput();

}
