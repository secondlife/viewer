/** 
 * @file llscenemonitor.h
 * @brief monitor the process of scene loading
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLSCENE_MONITOR_H
#define LL_LLSCENE_MONITOR_H

#include "llsingleton.h"
#include "llmath.h"
#include "llfloater.h"
#include "llcharacter.h"

class LLCharacter;
class LLRenderTarget;

class LLSceneMonitor :  public LLSingleton<LLSceneMonitor>
{
public:
	LLSceneMonitor();
	~LLSceneMonitor();

	void destroyClass();
	
	void freezeAvatar(LLCharacter* avatarp);
	void setDebugViewerVisible(BOOL visible);

	void capture(); //capture the main frame buffer
	void compare(); //compare the stored two buffers.	

	LLRenderTarget* getDiffTarget() const;
	bool isEnabled()const {return mEnabled;}
	
private:
	void freezeScene();
	void unfreezeScene();
	void reset();
	bool preCapture();
	void postCapture();
	
private:
	BOOL mEnabled;
	BOOL mNeedsUpdateDiff;
	BOOL mDebugViewerVisible;

	LLRenderTarget* mFrames[2];
	LLRenderTarget* mDiff;
	LLRenderTarget* mCurTarget;

	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;
};

class LLSceneMonitorView : public LLFloater
{
public:
	LLSceneMonitorView(const LLRect& rect);

	virtual void draw();
	virtual void setVisible(BOOL visible);

protected:
	virtual void onClickCloseBtn();
};

extern LLSceneMonitorView* gSceneMonitorView;

#endif

