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

//
// Globals
//

LLDebugView* gDebugView = NULL;

//
// Methods
//
static LLDefaultChildRegistry::Register<LLDebugView> r("debug_view");

LLDebugView::LLDebugView(const LLDebugView::Params& p)
:	LLView(p)
{}

void LLDebugView::init()
{
	LLRect r;
	LLRect rect = getLocalRect();

	r.set(10, rect.getHeight() - 100, rect.getWidth()/2, 100);
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
	
	mFastTimerView = new LLFastTimerView(r);
	mFastTimerView->setFollowsTop();
	mFastTimerView->setFollowsLeft();
	mFastTimerView->setVisible(FALSE);			// start invisible
	addChild(mFastTimerView);
	mFastTimerView->setRect(rect);

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
	tvp.follows.flags(FOLLOWS_BOTTOM|FOLLOWS_LEFT);
	tvp.visible(false);
	gTextureView = LLUICtrlFactory::create<LLTextureView>(tvp);
	addChild(gTextureView);
	//gTextureView->reshape(r.getWidth(), r.getHeight(), TRUE);

	


	if(gAuditTexture)
	{
		r.set(150, rect.getHeight() - 50, 900 + LLImageGL::sTextureLoadedCounter.size() * 30, 100);
		LLTextureSizeView::Params tsv ;
		tsv.name("gTextureSizeView");
		tsv.rect(r);
		tsv.follows.flags(FOLLOWS_BOTTOM|FOLLOWS_LEFT);
		tsv.visible(false);
		gTextureSizeView = LLUICtrlFactory::create<LLTextureSizeView>(tsv);
		addChild(gTextureSizeView);
		gTextureSizeView->setType(LLTextureSizeView::TEXTURE_MEM_OVER_SIZE) ;

		r.set(150, rect.getHeight() - 50, 900 + LLViewerTexture::getTotalNumOfCategories() * 30, 100);
		LLTextureSizeView::Params tcv ;
		tcv.name("gTextureCategoryView");
		tcv.rect(r);
		tcv.follows.flags(FOLLOWS_BOTTOM|FOLLOWS_LEFT);
		tcv.visible(false);
		gTextureCategoryView = LLUICtrlFactory::create<LLTextureSizeView>(tcv);
		gTextureCategoryView->setType(LLTextureSizeView::TEXTURE_MEM_OVER_CATEGORY);
		addChild(gTextureCategoryView);
	}
}


LLDebugView::~LLDebugView()
{
	// These have already been deleted.  Fix the globals appropriately.
	gDebugView = NULL;
	gTextureView = NULL;
	gSceneView = NULL;
	gTextureSizeView = NULL;
	gTextureCategoryView = NULL;
}

