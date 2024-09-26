/**
 * @file llpanelsnapshotinventory.cpp
 * @brief The panel provides UI for saving snapshot as an inventory texture.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llcombobox.h"
#include "llsidetraypanelcontainer.h"
#include "llspinctrl.h"

#include "llfloatersnapshot.h" // FIXME: replace with a snapshot storage model
#include "llpanelsnapshot.h"
#include "llsnapshotlivepreview.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llstatusbar.h"    // can_afford_transaction()
#include "llnotificationsutil.h"

#include "llagentbenefits.h"

/**
 * The panel provides UI for saving snapshot as an inventory texture.
 */
class LLPanelSnapshotInventory
    : public LLPanelSnapshot
{
    LOG_CLASS(LLPanelSnapshotInventory);

public:
    LLPanelSnapshotInventory();
    bool postBuild() override;
    void onOpen(const LLSD& key) override;

    void onResolutionCommit(LLUICtrl* ctrl);

private:
    std::string getWidthSpinnerName() const override   { return "inventory_snapshot_width"; }
    std::string getHeightSpinnerName() const override  { return "inventory_snapshot_height"; }
    std::string getAspectRatioCBName() const override  { return "inventory_keep_aspect_check"; }
    std::string getImageSizeComboName() const override { return "texture_size_combo"; }
    std::string getImageSizePanelName() const override { return LLStringUtil::null; }
    LLSnapshotModel::ESnapshotType getSnapshotType() override;
    void updateControls(const LLSD& info) override;

    void onSend();
    void updateUploadCost();
    S32 calculateUploadCost();
};

static LLPanelInjector<LLPanelSnapshotInventory> panel_class1("llpanelsnapshotinventory");

LLSnapshotModel::ESnapshotType LLPanelSnapshotInventory::getSnapshotType()
{
    return LLSnapshotModel::SNAPSHOT_TEXTURE;
}

LLPanelSnapshotInventory::LLPanelSnapshotInventory()
{
    mCommitCallbackRegistrar.add("Inventory.Save", { boost::bind(&LLPanelSnapshotInventory::onSend, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Inventory.Cancel", { boost::bind(&LLPanelSnapshotInventory::cancel, this), cb_info::UNTRUSTED_BLOCK });
}

// virtual
bool LLPanelSnapshotInventory::postBuild()
{
    getChild<LLSpinCtrl>(getWidthSpinnerName())->setAllowEdit(false);
    getChild<LLSpinCtrl>(getHeightSpinnerName())->setAllowEdit(false);

    getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onResolutionCommit, this, _1));
    return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotInventory::onOpen(const LLSD& key)
{
    updateUploadCost();

    LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotInventory::updateControls(const LLSD& info)
{
    const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
    getChild<LLUICtrl>("save_btn")->setEnabled(have_snapshot);

    updateUploadCost();
}

void LLPanelSnapshotInventory::onResolutionCommit(LLUICtrl* ctrl)
{
    bool current_window_selected = (getChild<LLComboBox>(getImageSizeComboName())->getCurrentIndex() == 3);
    getChild<LLSpinCtrl>(getWidthSpinnerName())->setVisible(!current_window_selected);
    getChild<LLSpinCtrl>(getHeightSpinnerName())->setVisible(!current_window_selected);
}

void LLPanelSnapshotInventory::onSend()
{
    S32 expected_upload_cost = calculateUploadCost();
    if (can_afford_transaction(expected_upload_cost))
    {
        if (mSnapshotFloater)
        {
            mSnapshotFloater->saveTexture();
            mSnapshotFloater->postSave();
        }
    }
    else
    {
        LLSD args;
        args["COST"] = llformat("%d", expected_upload_cost);
        LLNotificationsUtil::add("ErrorPhotoCannotAfford", args);
        if (mSnapshotFloater)
        {
            mSnapshotFloater->inventorySaveFailed();
        }
    }
}

void LLPanelSnapshotInventory::updateUploadCost()
{
    getChild<LLUICtrl>("hint_lbl")->setTextArg("[UPLOAD_COST]", llformat("%d", calculateUploadCost()));
}

S32 LLPanelSnapshotInventory::calculateUploadCost()
{
    S32 w = 0;
    S32 h = 0;

    if (mSnapshotFloater)
    {
        if (LLSnapshotLivePreview* preview = mSnapshotFloater->getPreviewView())
        {
            w = preview->getEncodedImageWidth();
            h = preview->getEncodedImageHeight();
        }
    }

    return LLAgentBenefitsMgr::current().getTextureUploadCost(w, h);
}
