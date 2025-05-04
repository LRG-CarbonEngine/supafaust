#pragma once

 
#include <map>
 
class SNESEngine
{
public:
	SNESEngine();
	~SNESEngine();

	enum RewindStyle
	{
		REWIND_STYLE_DYNAMIC = 0,	 
		REWIND_STYLE_STATIC = 1      
	};

	void start(std::string rom, GameTitle * pGameTitle = NULL);
	void stop();
	void rewind_handle_state(bool isSave);
	void signal_rewind();
	void rewind_load_state_at_index(int idx);
	void run_core();
	void save_state(uint8_t slot, int gameIndex);
	void load_state(uint8_t slot, int gameIndex);
	void delete_state(uint8_t slot, int gameIndex);
	void reset();
	void pause();
 
	bool is_rewinding() { return isRewinding; };
	void set_rewinding(bool rewind) { isRewinding = rewind; };
 
	void checkTrophies();
	void initSaveStates(std::string gameName, int gameIndex);
 
	int frameNum;
	saveStateMetaData saveData[3];
	bool isGameCompleted() { return m_bGameCompleted; };
	void setGameCompleted(bool isComplete) { m_bGameCompleted = isComplete; }
	void check2Player();
	bool m_bGameCompleted = false;
 
	int GetRewindBufIdx() { return currentRewindIdx; };
	int GetRewindBufFrames() { return maxRewindFrames; };
	int GetRewindRecordedFrames() { return recordedFrames; };
 
	int fb_width = 0;
	int fb_height = 0;
 
 
private:
	int trophyCount;
	int trophyCheckFrameCount;
 
};

