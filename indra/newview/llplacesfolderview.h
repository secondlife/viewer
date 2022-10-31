/** 
* @file   llplacesfolderview.h
* @brief  llplacesfolderview used within llplacesinventorypanel
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
#ifndef LL_LLPLACESFOLDERVIEW_H
#define LL_LLPLACESFOLDERVIEW_H

#include "llfolderview.h"
#include "llinventorypanel.h"

class LLLandmarksPanel;

class LLPlacesFolderView : public LLFolderView
{
public:

    struct Params : public LLInitParam::Block<Params, LLFolderView::Params>
    {
        Params()
        {}
    };

    LLPlacesFolderView(const LLFolderView::Params& p);
    /**
     *  Handles right mouse down
     *
     * Contains workaround for EXT-2786: sets current selected list for landmark
     * panel using @c mParentLandmarksPanel which is set in @c LLLandmarksPanel::initLandmarksPanel
     */
    /*virtual*/ BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );

    /*virtual*/ void updateMenu();

    void setupMenuHandle(LLInventoryType::EType asset_type, LLHandle<LLView> menu_handle);

    void setParentLandmarksPanel(LLLandmarksPanel* panel)
    {
        mParentLandmarksPanel = panel;
    }

private:
    /**
     * holds pointer to landmark panel. This pointer is used in @c LLPlacesFolderView::handleRightMouseDown
     */
    LLLandmarksPanel* mParentLandmarksPanel;
    typedef std::map<LLInventoryType::EType, LLHandle<LLView> > inventory_type_menu_handle_t;
    inventory_type_menu_handle_t mMenuHandlesByInventoryType;

};

#endif // LL_LLPLACESFOLDERVIEW_H

