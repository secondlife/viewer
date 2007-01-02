/** 
 * @file lldebugview.cpp
 * @brief A view containing UI elements only visible in build mode.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldebugview.h"

// library includes
#include "llframestatview.h"
#include "llfasttimerview.h"
#include "llmemoryview.h"
#include "llconsole.h"
#include "lltextureview.h"
#include "llresmgr.h"
#include "llaudiostatus.h"
#include "imageids.h"
#include "llvelocitybar.h"
#include "llviewerwindow.h"

//
// Globals
//

LLDebugView* gDebugView = NULL;

//
// Methods
//

LLDebugView::LLDebugView(const std::string& name, const LLRect &rect)
:	LLView(name, rect, FALSE)
{
	LLRect r;

	r.set(0, rect.getHeight() - 100, rect.getWidth()/2, 100);
	mDebugConsolep = new LLConsole("debug console", 20, r, -1, 0.f );
	mDebugConsolep->setFollowsBottom();
	mDebugConsolep->setFollowsLeft();
	mDebugConsolep->setVisible( FALSE );
	addChild(mDebugConsolep);

	r.set(150 - 25, rect.getHeight() - 50, rect.getWidth()/2 - 25, rect.getHeight() - 450);
	mFrameStatView = new LLFrameStatView("frame stat", r);
	mFrameStatView->setFollowsTop();
	mFrameStatView->setFollowsLeft();
	mFrameStatView->setVisible(FALSE);			// start invisible
	addChild(mFrameStatView);

	r.set(25, rect.getHeight() - 50, (S32) (gViewerWindow->getVirtualWindowRect().getWidth() * 0.75f), 
  									 (S32) (gViewerWindow->getVirtualWindowRect().getHeight() * 0.75f));
	mFastTimerView = new LLFastTimerView("fast timers", r);
	mFastTimerView->setFollowsTop();
	mFastTimerView->setFollowsLeft();
	mFastTimerView->setVisible(FALSE);			// start invisible
	addChild(mFastTimerView);

	r.set(25, rect.getHeight() - 50, rect.getWidth()/2, rect.getHeight() - 450);
	mMemoryView = new LLMemoryView("memory", r);
	mMemoryView->setFollowsTop();
	mMemoryView->setFollowsLeft();
	mMemoryView->setVisible(FALSE);			// start invisible
	addChild(mMemoryView);

	r.set(150, rect.getHeight() - 50, 820, 100);
	gTextureView = new LLTextureView("gTextureView", r);
	gTextureView->setRect(r);
	gTextureView->setFollowsBottom();
	gTextureView->setFollowsLeft();
	addChild(gTextureView);
	//gTextureView->reshape(r.getWidth(), r.getHeight(), TRUE);

	//
	// Debug statistics
	//
	r.set(rect.getWidth() - 250,
		  rect.getHeight(),
		  rect.getWidth(),
		  rect.getHeight() - 400);
	mStatViewp = new LLContainerView("statistics", r);
	mStatViewp->setLabel("Statistics");
	mStatViewp->setFollowsTop();
	mStatViewp->setFollowsRight();
	// Default to off
	mStatViewp->setVisible(FALSE);
	addChild(mStatViewp);

	//
	// Audio debugging stuff
	//
	const S32 AUDIO_STATUS_LEFT = rect.getWidth()/2-100;
	const S32 AUDIO_STATUS_WIDTH = 320;
	const S32 AUDIO_STATUS_TOP  = (rect.getHeight()/2)+400;
	const S32 AUDIO_STATUS_HEIGHT = 320;
	r.setLeftTopAndSize( AUDIO_STATUS_LEFT, AUDIO_STATUS_TOP, AUDIO_STATUS_WIDTH, AUDIO_STATUS_HEIGHT );
	LLAudiostatus*	gAudioStatus = new LLAudiostatus("AudioStatus", r);
	gAudioStatus->setFollowsTop();
	gAudioStatus->setFollowsRight();
	addChild(gAudioStatus);

	const S32 VELOCITY_LEFT = 10; // 370;
	const S32 VELOCITY_WIDTH = 500;
	const S32 VELOCITY_TOP = 140;
	const S32 VELOCITY_HEIGHT = 45;
	r.setLeftTopAndSize( VELOCITY_LEFT, VELOCITY_TOP, VELOCITY_WIDTH, VELOCITY_HEIGHT );
	gVelocityBar = new LLVelocityBar("Velocity Bar", r);
	gVelocityBar->setFollowsBottom();
	gVelocityBar->setFollowsLeft();
	addChild(gVelocityBar);
}


LLDebugView::~LLDebugView()
{
	// These have already been deleted.  Fix the globals appropriately.
	gDebugView = NULL;
	gTextureView = NULL;
}

EWidgetType LLDebugView::getWidgetType() const
{
	return WIDGET_TYPE_DEBUG_VIEW;
}

LLString LLDebugView::getWidgetTag() const
{
	return LL_DEBUG_VIEW_TAG;
}

