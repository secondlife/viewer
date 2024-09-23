/**
 * @file llfloaterbulkupload.cpp
 * @author Andrey Kleshchev
 * @brief LLFloaterBulkUpload class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterbulkupload.h"

#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"

constexpr S32 MAX_HEIGH = 211;

LLFloaterBulkUpload::LLFloaterBulkUpload(const LLSD& key)
:   LLModalDialog(key, true)
{
    mUploadCost = key["upload_cost"].asInteger();
    mUploadCount = key["upload_count"].asInteger();
    mHas2kTextures = key["has_2k_textures"].asBoolean();
    if (key["files"].isArray())
    {
        const LLSD& files = key["files"];
        for (LLSD::array_const_iterator it = files.beginArray();
            it != files.endArray();
            ++it)
        {
            mFiles.push_back(it->asString());
        }
    }
}

LLFloaterBulkUpload::~LLFloaterBulkUpload()
{
}

bool LLFloaterBulkUpload::postBuild()
{
    childSetAction("upload_btn", [this](void*) { onClickUpload(); }, this);
    childSetAction("cancel_btn", [this](void*) { onClickCancel(); }, this);

    mCountLabel = getChild<LLTextBox>("number_of_items", true);
    mCostLabel = getChild<LLTextBox>("upload_cost", true);

    mCheckboxPanel = getChild<LLPanel>("checkbox_panel", true);
    mLinkPanel = getChild<LLPanel>("link_panel", true);
    mWarningPanel = getChild<LLPanel>("warning_panel", true);

    mCheckboxUpload2K = getChild<LLUICtrl>("upload_2k");
    mCheckboxUpload2K->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& data) { onUpload2KCheckBox(); });

    mAllow2kTextures = gSavedSettings.getBOOL("BulkUpload2KTextures");
    mCheckboxUpload2K->setValue(!mAllow2kTextures);

    if (!mAllow2kTextures && mHas2kTextures)
    {
        // provided cost is for 2K textures, recalculate cost
        S32 bvh_count;
        S32 textures_2k_count;
        get_bulk_upload_expected_cost(mFiles, mAllow2kTextures, mUploadCost, mUploadCount, bvh_count, textures_2k_count);

        update();
    }


    update();

    return LLModalDialog::postBuild();
}

void LLFloaterBulkUpload::update()
{
    mCountLabel->setTextArg("[COUNT]", llformat("%d", mUploadCount));
    mCostLabel->setTextArg("[COST]", llformat("%d", mUploadCost));

    mCheckboxPanel->setVisible(mHas2kTextures);
    mLinkPanel->setVisible(mHas2kTextures);
    mWarningPanel->setVisible(mHas2kTextures);

    S32 new_height = MAX_HEIGH;
    if (!mHas2kTextures)
    {
        new_height -= mCheckboxPanel->getRect().getHeight();
        new_height -= mLinkPanel->getRect().getHeight();
        new_height -= mWarningPanel->getRect().getHeight();
    }
    reshape(getRect().getWidth(), new_height, false);
}

void LLFloaterBulkUpload::onUpload2KCheckBox()
{
    mAllow2kTextures = !mCheckboxUpload2K->getValue().asBoolean();
    gSavedSettings.setBOOL("BulkUpload2KTextures", mAllow2kTextures);

    S32 bvh_count;
    S32 textures_2k_count;
    get_bulk_upload_expected_cost(mFiles, mAllow2kTextures, mUploadCost, mUploadCount, bvh_count, textures_2k_count);
    // keep old value of mHas2kTextures to show checkbox

    update();
}

void LLFloaterBulkUpload::onClickUpload()
{
    do_bulk_upload(mFiles, mAllow2kTextures);
    closeFloater();
}


void LLFloaterBulkUpload::onClickCancel()
{
    closeFloater();
}
