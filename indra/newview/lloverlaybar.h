/** 
 * @file lloverlaybar.h
 * @brief LLOverlayBar class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLOVERLAYBAR_H
#define LL_LLOVERLAYBAR_H

#include "llpanel.h"
#include "llmediaremotectrl.h"

// "Constants" loaded from settings.xml at start time
extern S32 STATUS_BAR_HEIGHT;

class LLButton;
class LLLineEditor;
class LLMessageSystem;
class LLTextBox;
class LLTextEditor;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLStatGraph;
class LLSlider;
class LLVoiceRemoteCtrl;

class LLOverlayBar
:	public LLPanel
{
public:
	LLOverlayBar(const std::string& name, const LLRect& rect );
	~LLOverlayBar();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	/*virtual*/ void refresh();
	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);

	void layoutButtons();

	// helpers for returning desired state
	BOOL mediaPlaying() { return mMediaState == PLAYING; }
	BOOL musicPlaying() { return mMusicState == PLAYING; }
	
	static void onClickIMReceived(void* data);
	static void onClickSetNotBusy(void* data);
	static void onClickReleaseKeys(void* data);
	static void onClickMouselook(void* data);
	static void onClickStandUp(void* data);
	static void onClickResetView(void* data);

	//static media helper functions
	static void mediaPlay(void*);
	static void mediaPause(void*);
	static void mediaStop(void*);
	
	static void musicPlay(void*);
	static void musicPause(void*);
	static void musicStop(void*);

	static void toggleAudioVolumeFloater(void*);
	
	static void enableMediaButtons(LLPanel* panel);
	static void enableMusicButtons(LLPanel* panel);
	
protected:	
	static void* createMasterRemote(void* userdata);
	static void* createMusicRemote(void* userdata);
	static void* createMediaRemote(void* userdata);
	static void* createVoiceRemote(void* userdata);

protected:
	LLMediaRemoteCtrl*	mMasterRemote;
	LLMediaRemoteCtrl*	mMusicRemote;
	LLMediaRemoteCtrl*	mMediaRemote;
	LLVoiceRemoteCtrl*	mVoiceRemote;
	BOOL isBuilt;
	enum { STOPPED=0, PLAYING=1, PAUSED=2 };
	BOOL mMediaState;
	BOOL mMusicState;
};

extern LLOverlayBar* gOverlayBar;

#endif
