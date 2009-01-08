/** 
 * @file lldebugview.cpp
 * @brief A view containing UI elements only visible in build mode.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
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
#include "imageids.h"
#include "llvelocitybar.h"
#include "llviewerwindow.h"
#include "llfloaterstats.h"

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

	r.set(10, rect.getHeight() - 100, rect.getWidth()/2, 100);
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
		  rect.getHeight() - 50,
		  rect.getWidth(),
		  rect.getHeight() - 450);
	mFloaterStatsp = new LLFloaterStats(r);

	mFloaterStatsp->setFollowsTop();
	mFloaterStatsp->setFollowsRight();
	// since this is a floater, it belongs to LLFloaterView
	//addChild(mFloaterStatsp);

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

