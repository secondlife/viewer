/** 
 * @file llplacesinventorypanel.cpp
 * @brief LLPlacesInventoryPanel  class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llscrollcontainer.h"

#include "llplacesinventorypanel.h"

#include "llfoldervieweventlistener.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanellandmarks.h"
#include "llplacesinventorybridge.h"

static LLDefaultChildRegistry::Register<LLPlacesInventoryPanel> r("places_inventory_panel");

static const LLPlacesInventoryBridgeBuilder PLACES_INVENTORY_BUILDER;

LLPlacesInventoryPanel::LLPlacesInventoryPanel(const Params& p) : 
	LLInventoryPanel(p),
	mSavedFolderState(NULL)

{
	mInvFVBridgeBuilder = &PLACES_INVENTORY_BUILDER;
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
}


LLPlacesInventoryPanel::~LLPlacesInventoryPanel()
{
	delete mSavedFolderState;
}

BOOL LLPlacesInventoryPanel::postBuild()
{
	LLInventoryPanel::postBuild();

	// clear Contents();
	{
		mFolders->destroyView();
		mFolders->getParent()->removeChild(mFolders);
		mFolders->die();

		if( mScroller )
		{
			removeChild( mScroller );
			mScroller->die();
			mScroller = NULL;
		}
		mFolders = NULL;
	}


	mCommitCallbackRegistrar.pushScope(); // registered as a widget; need to push callback scope ourselves

	// create root folder
	{
		LLRect folder_rect(0,
			0,
			getRect().getWidth(),
			0);
		LLPlacesFolderView::Params p;
		p.name = getName();
		p.title = getLabel();
		p.rect = folder_rect;
		p.parent_panel = this;
		mFolders = (LLFolderView*)LLUICtrlFactory::create<LLPlacesFolderView>(p);
		mFolders->setAllowMultiSelect(mAllowMultiSelect);
	}

	mCommitCallbackRegistrar.popScope();

	mFolders->setCallbackRegistrar(&mCommitCallbackRegistrar);

	// scroller
	{
		LLRect scroller_view_rect = getRect();
		scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
		LLScrollContainer::Params p;
		p.name("Inventory Scroller");
		p.rect(scroller_view_rect);
		p.follows.flags(FOLLOWS_ALL);
		p.reserve_scroll_corner(true);
		p.tab_stop(true);
		mScroller = LLUICtrlFactory::create<LLScrollContainer>(p);
	}
	addChild(mScroller);
	mScroller->addChild(mFolders);

	mFolders->setScrollContainer(mScroller);
	mFolders->addChild(mFolders->mStatusTextBox);


	// cut subitems
	mFolders->setUseEllipses(true);

	return TRUE;
}

// save current folder open state
void LLPlacesInventoryPanel::saveFolderState()
{
	mSavedFolderState->setApply(FALSE);
	getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
}

// re-open folders which state was saved
void LLPlacesInventoryPanel::restoreFolderState()
{
	mSavedFolderState->setApply(TRUE);
	getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	LLOpenFoldersWithSelection opener;
	getRootFolder()->applyFunctorRecursively(opener);
	getRootFolder()->scrollToShowSelection();
}

S32	LLPlacesInventoryPanel::notify(const LLSD& info) 
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			return getRootFolder()->notify(info);
		}
		else if(str_action == "select_last")
		{
			return getRootFolder()->notify(info);
		}
	}
	return 0;
}

/************************************************************************/
/* PROTECTED METHODS                                                    */
/************************************************************************/



/************************************************************************/
/*              LLPlacesFolderView implementation                       */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
//  PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

LLPlacesFolderView::LLPlacesFolderView(const LLFolderView::Params& p)
: LLFolderView(p)
{
	// we do not need auto select functionality in places landmarks, so override default behavior.
	// this disables applying of the LLSelectFirstFilteredItem in LLFolderView::doIdle.
	// Fixed issues: EXT-1631, EXT-4994.
	mAutoSelectOverride = TRUE;
}

BOOL LLPlacesFolderView::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// let children to change selection first
	childrenHandleRightMouseDown(x, y, mask);
	mParentLandmarksPanel->setCurrentSelectedList((LLPlacesInventoryPanel*)getParentPanel());

	// then determine its type and set necessary menu handle
	if (getCurSelectedItem())
	{
		LLInventoryType::EType inventory_type = getCurSelectedItem()->getListener()->getInventoryType();
		inventory_type_menu_handle_t::iterator it_handle = mMenuHandlesByInventoryType.find(inventory_type);

		if (it_handle != mMenuHandlesByInventoryType.end())
		{
			mPopupMenuHandle = (*it_handle).second;
		}
		else
		{
			llwarns << "Requested menu handle for non-setup inventory type: " << inventory_type << llendl;
		}

	}

	return LLFolderView::handleRightMouseDown(x, y, mask);
}

void LLPlacesFolderView::setupMenuHandle(LLInventoryType::EType asset_type, LLHandle<LLView> menu_handle)
{
	mMenuHandlesByInventoryType[asset_type] = menu_handle;
}

// EOF
