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
:	public LLPanelSnapshot
{
	LOG_CLASS(LLPanelSnapshotProfile);

public:
	LLPanelSnapshotProfile();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "profile_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "profile_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "profile_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "profile_size_combo"; }
	/*virtual*/ LLFloaterSnapshot::ESnapshotFormat getImageFormat() const { return LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG; }
	/*virtual*/ void updateControls(const LLSD& info);

	void updateCustomResControls(); ///< Enable/disable custom resolution controls (spinners and checkbox)

	void onSend();
	void onResolutionComboCommit(LLUICtrl* ctrl);
	void onCustomResolutionCommit(LLUICtrl* ctrl);
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);
};

static LLRegisterPanelClassWrapper<LLPanelSnapshotProfile> panel_class("llpanelsnapshotprofile");

LLPanelSnapshotProfile::LLPanelSnapshotProfile()
{
	mCommitCallbackRegistrar.add("PostToProfile.Send",		boost::bind(&LLPanelSnapshotProfile::onSend,		this));
	mCommitCallbackRegistrar.add("PostToProfile.Cancel",	boost::bind(&LLPanelSnapshotProfile::cancel,		this));
}

// virtual
BOOL LLPanelSnapshotProfile::postBuild()
{
	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotProfile::onResolutionComboCommit, this, _1));
	getChild<LLUICtrl>(getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotProfile::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotProfile::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshotProfile::onKeepAspectRatioCommit, this, _1));

	return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotProfile::onOpen(const LLSD& key)
{
	updateCustomResControls();
	LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotProfile::updateControls(const LLSD& info)
{
	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("post_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotProfile::updateCustomResControls() ///< Enable/disable custom resolution controls (spinners and checkbox)
{
	LLComboBox* combo = getChild<LLComboBox>(getImageSizeComboName());
	S32 selected_idx = combo->getFirstSelectedIndex();
	bool enable = selected_idx == 0 || selected_idx == (combo->getItemCount() - 1); // Current Window or Custom selected

	getChild<LLUICtrl>(getWidthSpinnerName())->setEnabled(enable);
	getChild<LLSpinCtrl>(getWidthSpinnerName())->setAllowEdit(enable);
	getChild<LLUICtrl>(getHeightSpinnerName())->setEnabled(enable);
	getChild<LLSpinCtrl>(getHeightSpinnerName())->setAllowEdit(enable);
	getChild<LLUICtrl>(getAspectRatioCBName())->setEnabled(enable);
}

void LLPanelSnapshotProfile::onSend()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	LLWebProfile::uploadImage(LLFloaterSnapshot::getImageData(), caption, add_location);
	LLFloaterSnapshot::postSave();
}

void LLPanelSnapshotProfile::onResolutionComboCommit(LLUICtrl* ctrl)
{
	updateCustomResControls();

	LLSD info;
	info["combo-res-change"]["control-name"] = ctrl->getName();
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshotProfile::onCustomResolutionCommit(LLUICtrl* ctrl)
{
	S32 w = getChild<LLUICtrl>(getWidthSpinnerName())->getValue().asInteger();
	S32 h = getChild<LLUICtrl>(getHeightSpinnerName())->getValue().asInteger();
	LLSD info;
	info["w"] = w;
	info["h"] = h;
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshotProfile::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}
