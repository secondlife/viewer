/**
 * @file llfloaterinventorythumbnailshelper.h
 * @author Callum Prentice
 * @brief Helper floater for bulk processing of inventory thumbnails
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLFLOATERINVENTORYTHUMBNAILSHELPER_H
#define LL_LLFLOATERINVENTORYTHUMBNAILSHELPER_H

#include "llfloater.h"
class LLTextEditor;
class LLScrollListCtrl;
class LLMediaCtrl;
class LLViewerInventoryItem;
class LLUUID;

class LLFloaterInventoryThumbnailsHelper:
    public LLFloater
{
        friend class LLFloaterReg;
    private:
        LLFloaterInventoryThumbnailsHelper(const LLSD& key);
        BOOL postBuild() override;
        ~LLFloaterInventoryThumbnailsHelper();

        LLScrollListCtrl* mInventoryThumbnailsList;

        LLUICtrl* mPasteItemsBtn;
        void onPasteItems();

        LLUICtrl* mPasteTexturesBtn;
        void onPasteTextures();

        LLTextEditor* mOutputLog;

        void mergeItemsTextures();

        LLUICtrl* mWriteThumbnailsBtn;
        void onWriteThumbnails();

        void recordInventoryItemEntry(LLViewerInventoryItem* item);
        void recordTextureItemEntry(LLViewerInventoryItem* item);
        void populateThumbnailNames();

        std::map<std::string, LLUUID> mItemNamesIDs;
        std::map<std::string, LLUUID> mTextureNamesIDs;

        std::map<std::string, std::pair< LLUUID, LLUUID>> mNameItemIDTextureId;
};

#endif // LL_LLFLOATERINVENTORYTHUMBNAILSHELPER_H
