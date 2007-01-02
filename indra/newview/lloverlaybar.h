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
class LLVolumeSliderCtrl;

class LLOverlayBar
:	public LLPanel,
	public LLMediaRemoteCtrlObserver
{
public:
	LLOverlayBar(const std::string& name, const LLRect& rect );
	~LLOverlayBar();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent);

	void refresh();

	void layoutButtons();

	/*virtual*/ void draw();

	static void onClickIMReceived(void* data);
	static void onClickSetNotBusy(void* data);
	static void onClickReleaseKeys(void* data);
	static void onClickMouselook(void* data);
	static void onClickStandUp(void* data);
	static void onClickResetView(void* data);

	// observer overrides
	void onVolumeChange ( const LLMediaRemoteCtrlObserver::EventType& eventIn );
	void onStopButtonPressed ( const LLMediaRemoteCtrlObserver::EventType& eventIn );
	void onPlayButtonPressed ( const LLMediaRemoteCtrlObserver::EventType& eventIn );
	void onPauseButtonPressed ( const LLMediaRemoteCtrlObserver::EventType& eventIn );

	LLMediaRemoteCtrl*	getMusicRemoteControl () { return mMusicRemote; };

protected:
	
	static void* createMusicRemote(void* userdata);
	static void* createMediaRemote(void* userdata);

protected:
	LLMediaRemoteCtrl*	mMusicRemote;
	LLMediaRemoteCtrl*	mMediaRemote;
	BOOL isBuilt;
};

extern LLOverlayBar* gOverlayBar;

#endif
