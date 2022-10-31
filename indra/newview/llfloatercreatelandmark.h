/** 
 * @file llfloatercreatelandmark.h
 * @brief LLFloaterCreateLandmark class definition
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#ifndef LL_LLFLOATERCREATELANDMARK_H
#define LL_LLFLOATERCREATELANDMARK_H

#include "llfloater.h"

class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLTextEditor;
class LLLandmarksInventoryObserver;

class LLFloaterCreateLandmark:
    public LLFloater
{
    friend class LLFloaterReg;

public:

    LLFloaterCreateLandmark(const LLSD& key);
    ~LLFloaterCreateLandmark();

    BOOL postBuild();
    void onOpen(const LLSD& key);

    void setItem(const uuid_set_t& items);
    void updateItem(const uuid_set_t& items, U32 mask);

    LLInventoryItem* getItem() { return mItem; }

private:
    void setLandmarkInfo(const LLUUID &folder_id);
    void removeObserver();
    void populateFoldersList(const LLUUID &folder_id = LLUUID::null);
    void onCommitTextChanges();
    void onCreateFolderClicked();
    void onSaveClicked();
    void onCancelClicked();

    void folderCreatedCallback(LLUUID folder_id);

    LLComboBox*     mFolderCombo;
    LLLineEditor*   mLandmarkTitleEditor;
    LLTextEditor*   mNotesEditor;
    LLUUID          mLandmarksID;
    LLUUID          mAssetID;

    LLLandmarksInventoryObserver*   mInventoryObserver;
    LLPointer<LLInventoryItem>      mItem;
};

#endif
