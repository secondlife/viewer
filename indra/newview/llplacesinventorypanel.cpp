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

#include "llfolderviewmodel.h"
#include "llplacesfolderview.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llpanellandmarks.h"
#include "llplacesinventorybridge.h"
#include "llviewerfoldertype.h"

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


LLFolderView * LLPlacesInventoryPanel::createFolderRoot(LLUUID root_id )
{
    LLPlacesFolderView::Params p;
    
    p.name = getName();
    p.title = getLabel();
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = this;
    p.tool_tip = p.name;
    p.listener = mInvFVBridgeBuilder->createBridge(	LLAssetType::AT_CATEGORY,
        LLAssetType::AT_CATEGORY,
        LLInventoryType::IT_CATEGORY,
        this,
        &mInventoryViewModel,
        NULL,
        root_id);
    p.view_model = &mInventoryViewModel;
    p.use_label_suffix = mParams.use_label_suffix;
    p.allow_multiselect = mAllowMultiSelect;
    p.show_empty_message = mShowEmptyMessage;
    p.show_item_link_overlays = mShowItemLinkOverlays;
    p.root = NULL;
    p.use_ellipses = mParams.folder_view.use_ellipses;
    p.options_menu = "menu_inventory.xml";

    return LLUICtrlFactory::create<LLPlacesFolderView>(p);
}

// save current folder open state
void LLPlacesInventoryPanel::saveFolderState()
{
	mSavedFolderState->setApply(FALSE);
	mFolderRoot.get()->applyFunctorRecursively(*mSavedFolderState);
}

// re-open folders which state was saved
void LLPlacesInventoryPanel::restoreFolderState()
{
	mSavedFolderState->setApply(TRUE);
	mFolderRoot.get()->applyFunctorRecursively(*mSavedFolderState);
	LLOpenFoldersWithSelection opener;
	mFolderRoot.get()->applyFunctorRecursively(opener);
	mFolderRoot.get()->scrollToShowSelection();
}

S32	LLPlacesInventoryPanel::notify(const LLSD& info) 
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			return mFolderRoot.get()->notify(info);
		}
		else if(str_action == "select_last")
		{
			return mFolderRoot.get()->notify(info);
		}
	}
	return 0;
}
