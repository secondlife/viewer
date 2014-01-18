/** 
 * @file llprogressview.h
 * @brief LLProgressView class definition
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

#ifndef LL_LLPROGRESSVIEW_H
#define LL_LLPROGRESSVIEW_H

#include "llpanel.h"
#include "llmediactrl.h"
#include "llframetimer.h"
#include "llevents.h"

class LLImageRaw;
class LLButton;
class LLProgressBar;

class LLProgressView : 
	public LLPanel,
	public LLViewerMediaObserver

{
	LOG_CLASS(LLProgressView);

public:
	LLProgressView();
	virtual ~LLProgressView();
	
	BOOL postBuild();

	/*virtual*/ void draw();
	void drawStartTexture(F32 alpha);

	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ void setVisible(BOOL visible);

	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void setText(const std::string& text);
	void setPercent(const F32 percent);

	// Set it to NULL when you want to eliminate the message.
	void setMessage(const std::string& msg);
	
	// turns on (under certain circumstances) the into video after login
	void revealIntroPanel();

	void setStartupComplete();

	void setCancelButtonVisible(BOOL b, const std::string& label);

	static void onCancelButtonClicked( void* );
	static void onClickMessage(void*);
	bool onAlertModal(const LLSD& sd);

protected:
	LLProgressBar* mProgressBar;
	LLMediaCtrl* mMediaCtrl;
	F32 mPercentDone;
	std::string mMessage;
	LLButton*	mCancelBtn;
	LLFrameTimer mFadeToWorldTimer;
	LLFrameTimer mFadeFromLoginTimer;
	LLRect mOutlineRect;
	bool mMouseDownInActiveArea;
	bool mStartupComplete;

	// The LLEventStream mUpdateEvents depends upon this class being a singleton
	// to avoid pump name conflicts.
	static LLProgressView* sInstance;
	LLEventStream mUpdateEvents; 

	bool handleUpdate(const LLSD& event_data);
	static void onIdle(void* user_data);
};

#endif // LL_LLPROGRESSVIEW_H
