/**
 * @file llpanelsnapshotprofile.cpp
 * @brief Posts a snapshot to My Profile feed.
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

// libs
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llpanel.h"
#include "llspinctrl.h"

// newview
#include "llfloatersnapshot.h"
#include "llpanelsnapshot.h"
#include "llsidetraypanelcontainer.h"
#include "llwebprofile.h"

/**
 * Posts a snapshot to My Profile feed.
 */
class LLPanelSnapshotProfile
:   public LLPanelSnapshot
{
    LOG_CLASS(LLPanelSnapshotProfile);

public:
    LLPanelSnapshotProfile();

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    std::string getWidthSpinnerName() const override   { return "profile_snapshot_width"; }
    std::string getHeightSpinnerName() const override  { return "profile_snapshot_height"; }
    std::string getAspectRatioCBName() const override  { return "profile_keep_aspect_check"; }
    std::string getImageSizeComboName() const override { return "profile_size_combo"; }
    std::string getImageSizePanelName() const override { return "profile_image_size_lp"; }
    LLSnapshotModel::ESnapshotFormat getImageFormat() const override { return LLSnapshotModel::SNAPSHOT_FORMAT_PNG; }
    void updateControls(const LLSD& info) override;

    void onSend();
};

static LLPanelInjector<LLPanelSnapshotProfile> panel_class("llpanelsnapshotprofile");

LLPanelSnapshotProfile::LLPanelSnapshotProfile()
{
    mCommitCallbackRegistrar.add("PostToProfile.Send",      { boost::bind(&LLPanelSnapshotProfile::onSend, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("PostToProfile.Cancel",    { boost::bind(&LLPanelSnapshotProfile::cancel, this), cb_info::UNTRUSTED_BLOCK });
}

// virtual
bool LLPanelSnapshotProfile::postBuild()
{
    return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotProfile::onOpen(const LLSD& key)
{
    LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotProfile::updateControls(const LLSD& info)
{
    const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
    getChild<LLUICtrl>("post_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotProfile::onSend()
{
    std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
    bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

    LLWebProfile::uploadImage(mSnapshotFloater->getImageData(), caption, add_location);
    mSnapshotFloater->postSave();
}
