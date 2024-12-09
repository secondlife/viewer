/**
 * @file llfloaterbulkupload.h
 * @author Andrey Kleshchev
 * @brief LLFloaterBulkUpload class definition
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#ifndef LL_LLFLOATERBULKUPLOAD_H
#define LL_LLFLOATERBULKUPLOAD_H

#include "llmodaldialog.h"

class LLTextBox;

class LLFloaterBulkUpload : public LLModalDialog
{
public:
    LLFloaterBulkUpload(const LLSD& key);
    ~LLFloaterBulkUpload();

    bool postBuild() override;

    void update();

protected:
    void onUpload2KCheckBox();

    void onClickUpload();
    void onClickCancel();

private:
    LLUICtrl* mCheckboxUpload2K = nullptr;
    LLTextBox* mCountLabel = nullptr;
    LLTextBox* mCostLabel = nullptr;
    LLPanel* mCheckboxPanel = nullptr;
    LLPanel* mLinkPanel = nullptr;
    LLPanel* mWarningPanel = nullptr;

    std::vector<std::string> mFiles;
    bool mAllow2kTextures = true;
    bool mHas2kTextures = false;
    S32 mUploadCost = 0;
    S32 mUploadCount = 0;
};

#endif
