/** 
 * @file llfloatermyenvironment.h
 * @brief LLFloaterMyEnvironment class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLFLOATERMYENVIRONMENT_H
#define LL_LLFLOATERMYENVIRONMENT_H
#include <vector> 

#include "llfloater.h"
#include "llinventoryobserver.h"
#include "llinventoryfilter.h"
#include "llfiltereditor.h"

class LLInventoryPanel;

class LLFloaterMyEnvironment
:   public LLFloater, LLInventoryFetchDescendentsObserver
{
    LOG_CLASS(LLFloaterMyEnvironment);
public:
                                    LLFloaterMyEnvironment(const LLSD& key);
    virtual                         ~LLFloaterMyEnvironment();

    virtual BOOL                    postBuild() override;
    virtual void                    refresh() override;

    virtual void                    onOpen(const LLSD& key) override;

private:
    LLInventoryPanel *              mInventoryList;
    LLFilterEditor *                mFilterEdit;
    U64                             mTypeFilter;
    LLInventoryFilter::EFolderShow  mShowFolders;
    LLUUID                          mSelectedAsset;
    LLSaveFolderState               mSavedFolderState;

    void                            onShowFoldersChange();
    void                            onFilterCheckChange();
    void                            onFilterEdit(const std::string& search_string);
    void                            onSelectionChange();
    void                            onDeleteSelected();
    void                            onDoCreate(const LLSD &data);
    void                            onDoApply(const std::string &context);
    bool                            canAction(const std::string &context);
    bool                            canApply(const std::string &context);

    void                            getSelectedIds(uuid_vec_t& ids) const;
    void                            refreshButtonStates();

    bool                            isSettingSelected(LLUUID item_id);

    static LLUUID                   findItemByAssetId(LLUUID asset_id, bool copyable_only, bool ignore_library);
};


#endif
