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
#include "lltracerecording.h"

class LLCharacter;
class LLRenderTarget;
class LLViewerTexture;

class LLSceneMonitor : public LLSingleton<LLSceneMonitor>
{
	LLSINGLETON(LLSceneMonitor);
	~LLSceneMonitor();
	LOG_CLASS(LLSceneMonitor);
public:

	void freezeAvatar(LLCharacter* avatarp);
	void setDebugViewerVisible(bool visible);

	void capture(); //capture the main frame buffer
	void compare(); //compare the stored two buffers.	
	void fetchQueryResult();
	void calcDiffAggregate();
	void setDiffTolerance(F32 tol) {mDiffTolerance = tol;}

	const LLRenderTarget* getDiffTarget() const {return mDiff;}
	F32  getDiffTolerance() const {return mDiffTolerance;}
	F32  getDiffResult() const { return mDiffResult;}
	F32  getDiffPixelRatio() const { return mDiffPixelRatio;}
	bool isEnabled()const {return mEnabled;}
	bool needsUpdate() const;
	
	const LLTrace::ExtendablePeriodicRecording* getRecording() const {return &mSceneLoadRecording;}
	void dumpToFile(std::string file_name);
	bool hasResults() const { return mSceneLoadRecording.getResults().getDuration() != S32Seconds(0);}

	void reset();

private:
	void freezeScene();
	void unfreezeScene();

	LLRenderTarget& getCaptureTarget();
	void generateDitheringTexture(S32 width, S32 height);

private:
	bool									mEnabled,
											mDebugViewerVisible;

	enum EDiffState
	{
		WAITING_FOR_NEXT_DIFF,
		NEED_DIFF,
		EXECUTE_DIFF,
		WAIT_ON_RESULT,
		VIEWER_QUITTING
	}										mDiffState;

	LLRenderTarget*							mFrames[2];
	LLRenderTarget*							mDiff;

	GLuint									mQueryObject; //used for glQuery
	F32										mDiffResult,  //aggregate results of mDiff.
											mDiffTolerance, //pixels are filtered out when R+G+B < mDiffTolerance
											mDiffPixelRatio; //ratio of pixels used for comparison against the original mDiff size along one dimension

	LLPointer<LLViewerTexture>				mDitheringTexture;
	S32										mDitherMatrixWidth;
	F32										mDitherScale,
											mDitherScaleS,
											mDitherScaleT;

	std::vector<LLAnimPauseRequest>			mAvatarPauseHandles;

	LLTimer									mRecordingTimer;
	LLTrace::ExtendablePeriodicRecording	mSceneLoadRecording;
	LLTrace::Recording						mMonitorRecording;
};

class LLSceneMonitorView : public LLFloater
{
public:
	LLSceneMonitorView(const LLRect& rect);
	~LLSceneMonitorView();
	virtual void draw();

	virtual void onVisibilityChange(BOOL visible);

protected:
	virtual void onClose(bool app_quitting=false);
	virtual void onClickCloseBtn(bool app_quitting=false);
	void onTeleportFinished();
	boost::signals2::connection sTeleportFinishConnection;
};

extern LLSceneMonitorView* gSceneMonitorView;

#endif

