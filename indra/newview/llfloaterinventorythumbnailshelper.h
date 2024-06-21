/**
 * @file llfloaterinventorythumbnailshelper.h
 * @author Callum Prentice
 * @brief Helper floater for bulk processing of inventory thumbnails tool
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
class LLViewerInventoryItem;
class LLUUID;

class LLFloaterInventoryThumbnailsHelper:
    public LLFloater
{
        friend class LLFloaterReg;
    private:
        LLFloaterInventoryThumbnailsHelper(const LLSD& key);
        bool postBuild() override;
        ~LLFloaterInventoryThumbnailsHelper();

        LLScrollListCtrl* mInventoryThumbnailsList;

        LLTextEditor* mOutputLog;

        LLUICtrl* mPasteItemsBtn;
        void onPasteItems();

        LLUICtrl* mPasteTexturesBtn;
        void onPasteTextures();

        LLUICtrl* mWriteThumbnailsBtn;
        void onWriteThumbnails();

        LLUICtrl* mLogMissingThumbnailsBtn;
        void onLogMissingThumbnails();

        LLUICtrl* mClearThumbnailsBtn;
        void onClearThumbnails();

        void recordInventoryItemEntry(LLViewerInventoryItem* item);
        void recordTextureItemEntry(LLViewerInventoryItem* item);
        void updateButtonStates();
        void updateDisplayList();
        void writeToLog(std::string logline, bool prepend_newline);

        std::map<std::string, LLViewerInventoryItem*> mItemNamesItems;
        std::map<std::string, LLUUID> mTextureNamesIDs;

        enum EListColumnNum
        {
            NAME = 0,
            EXISTING_TEXTURE = 1,
            NEW_TEXTURE = 2
        };
};

#endif // LL_LLFLOATERINVENTORYTHUMBNAILSHELPER_H
