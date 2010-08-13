/** 
 * @file lloverlaybar.h
 * @brief LLOverlayBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLOVERLAYBAR_H
#define LL_LLOVERLAYBAR_H

#include "llpanel.h"

// "Constants" loaded from settings.xml at start time
extern S32 STATUS_BAR_HEIGHT;

class LLButton;
class LLLineEditor;
class LLMediaRemoteCtrl;
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
	LLOverlayBar();
	~LLOverlayBar();

	/*virtual*/ void refresh();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ BOOL postBuild();

	void layoutButtons();

	// helpers for returning desired state
	BOOL musicPlaying() { return mMusicState == PLAYING; }
	
	static void onClickSetNotBusy(void* data);
	static void onClickMouselook(void* data);
	static void onClickStandUp(void* data);
	static void onClickResetView(void* data);
 	static void onClickFlycam(void* data);

	//static media helper functions
	static void toggleMediaPlay(void*);
	static void toggleMusicPlay(void*);
	static void musicPause(void*);
	static void musicStop(void*);
	static void mediaStop(void*);

	static void toggleAudioVolumeFloater(void*);

protected:	
	static void* createMediaRemote(void* userdata);
	static void* createVoiceRemote(void* userdata);

	void enableMediaButtons();

protected:
	LLMediaRemoteCtrl*	mMediaRemote;
	LLVoiceRemoteCtrl*	mVoiceRemote;
	bool mBuilt;	// dialog constructed yet?
	enum { STOPPED=0, PLAYING=1, PAUSED=2 };
	S32 mMusicState;
};

extern LLOverlayBar* gOverlayBar;

#endif
