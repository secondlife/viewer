/** 
 * @file llplacesinventorypanel.cpp
 * @brief LLPlacesInventoryPanel  class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
		mFolderRoot->destroyView();
		mFolderRoot->getParent()->removeChild(mFolderRoot);
		mFolderRoot->die();

		if( mScroller )
		{
			removeChild( mScroller );
			mScroller->die();
			mScroller = NULL;
		}
		mFolderRoot = NULL;
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
		mFolderRoot = (LLFolderView*)LLUICtrlFactory::create<LLPlacesFolderView>(p);
		mFolderRoot->setAllowMultiSelect(mAllowMultiSelect);
	}

	mCommitCallbackRegistrar.popScope();

	mFolderRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);

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
	mScroller->addChild(mFolderRoot);

	mFolderRoot->setScrollContainer(mScroller);
	mFolderRoot->addChild(mFolderRoot->mStatusTextBox);


	// cut subitems
	mFolderRoot->setUseEllipses(true);

	return TRUE;
}

// save current folder open state
void LLPlacesInventoryPanel::saveFolderState()
{
	mSavedFolderState->setApply(FALSE);
	mFolderRoot->applyFunctorRecursively(*mSavedFolderState);
}

// re-open folders which state was saved
void LLPlacesInventoryPanel::restoreFolderState()
{
	mSavedFolderState->setApply(TRUE);
	mFolderRoot->applyFunctorRecursively(*mSavedFolderState);
	LLOpenFoldersWithSelection opener;
	mFolderRoot->applyFunctorRecursively(opener);
	mFolderRoot->scrollToShowSelection();
}

S32	LLPlacesInventoryPanel::notify(const LLSD& info) 
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			return mFolderRoot->notify(info);
		}
		else if(str_action == "select_last")
		{
			return mFolderRoot->notify(info);
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
