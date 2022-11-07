/** 
* @file llplacesfolderview.cpp
* @brief llplacesfolderview used within llplacesinventorypanel
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "llplacesfolderview.h"

#include "llplacesinventorypanel.h"
#include "llpanellandmarks.h"
#include "llmenugl.h"

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
        LLInventoryType::EType inventory_type = static_cast<LLFolderViewModelItemInventory*>(getCurSelectedItem()->getViewModelItem())->getInventoryType();
        inventory_type_menu_handle_t::iterator it_handle = mMenuHandlesByInventoryType.find(inventory_type);

        if (it_handle != mMenuHandlesByInventoryType.end())
        {
            mPopupMenuHandle = (*it_handle).second;
        }
        else
        {
            LL_WARNS() << "Requested menu handle for non-setup inventory type: " << inventory_type << LL_ENDL;
        }

    }

    return LLFolderView::handleRightMouseDown(x, y, mask);
}

void LLPlacesFolderView::updateMenu()
{
    LLFolderView::updateMenu();
    LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();
    if (menu && menu->getVisible())
    {
        mParentLandmarksPanel->updateMenuVisibility(menu);
    }
}

void LLPlacesFolderView::setupMenuHandle(LLInventoryType::EType asset_type, LLHandle<LLView> menu_handle)
{
    mMenuHandlesByInventoryType[asset_type] = menu_handle;
}

