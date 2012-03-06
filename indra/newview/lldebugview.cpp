/** 
 * @file lldebugview.cpp
 * @brief A view containing UI elements only visible in build mode.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lldebugview.h"

// library includes
#include "llfasttimerview.h"
#include "llmemoryview.h"
#include "llconsole.h"
#include "lltextureview.h"
#include "llresmgr.h"
#include "imageids.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llmemoryview.h"
#include "llsceneview.h"
#include "llviewertexture.h"
#include "llfloaterreg.h"

//
// Globals
//

LLDebugView* gDebugView = NULL;

//
// Methods
//
static LLDefaultChildRegistry::Register<LLDebugView> r("debug_view");

LLDebugView::LLDebugView(const LLDebugView::Params& p)
:	LLView(p),
	mFastTimerView(NULL),
	mMemoryView(NULL),
	mDebugConsolep(NULL),
	mFloaterSnapRegion(NULL)
{}

LLDebugView::~LLDebugView()
{
	// These have already been deleted.  Fix the globals appropriately.
	gDebugView = NULL;
	gTextureView = NULL;
	gSceneView = NULL;
}

void LLDebugView::init()
{
	LLRect r;
	LLRect rect = getLocalRect();

	// Rectangle to draw debug data in (full height, 3/4 width)
	r.set(10, rect.getHeight() - 100, ((rect.getWidth()*3)/4), 100);
	LLConsole::Params cp;
	cp.name("debug console");
	cp.max_lines(20);
	cp.rect(r);
	cp.font(LLFontGL::getFontMonospace());
	cp.follows.flags(FOLLOWS_BOTTOM | FOLLOWS_LEFT);
	cp.visible(false);
	mDebugConsolep = LLUICtrlFactory::create<LLConsole>(cp);
	addChild(mDebugConsolep);

	r.set(150 - 25, rect.getHeight() - 50, rect.getWidth()/2 - 25, rect.getHeight() - 450);

	r.setLeftTopAndSize(25, rect.getHeight() - 50, (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.75f), 
  									 (S32) (gViewerWindow->getWindowRectScaled().getHeight() * 0.75f));
	
	mFastTimerView = dynamic_cast<LLFastTimerView*>(LLFloaterReg::getInstance("fast_timers"));

	gSceneView = new LLSceneView(r);
	gSceneView->setFollowsTop();
	gSceneView->setFollowsLeft();
	gSceneView->setVisible(FALSE);
	addChild(gSceneView);
	gSceneView->setRect(rect);
	
	r.setLeftTopAndSize(25, rect.getHeight() - 50, (S32) (gViewerWindow->getWindowRectScaled().getWidth() * 0.75f), 
									 (S32) (gViewerWindow->getWindowRectScaled().getHeight() * 0.75f));
	LLMemoryView::Params mp;
	mp.name("memory");
	mp.rect(r);
	mp.follows.flags(FOLLOWS_TOP | FOLLOWS_LEFT);
	mp.visible(false);
	mMemoryView = LLUICtrlFactory::create<LLMemoryView>(mp);
	addChild(mMemoryView);

	r.set(150, rect.getHeight() - 50, 820, 100);
	LLTextureView::Params tvp;
	tvp.name("gTextureView");
	tvp.rect(r);
	tvp.follows.flags(FOLLOWS_TOP|FOLLOWS_LEFT);
	tvp.visible(false);
	gTextureView = LLUICtrlFactory::create<LLTextureView>(tvp);
	addChild(gTextureView);
	//gTextureView->reshape(r.getWidth(), r.getHeight(), TRUE);
}

void LLDebugView::draw()
{
	if (mFloaterSnapRegion == NULL)
	{
		mFloaterSnapRegion = getRootView()->getChildView("floater_snap_region");
	}

	LLRect debug_rect;
	mFloaterSnapRegion->localRectToOtherView(mFloaterSnapRegion->getLocalRect(), &debug_rect, getParent());

	setShape(debug_rect);
	LLView::draw();
}
