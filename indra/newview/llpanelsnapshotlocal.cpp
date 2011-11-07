/** 
 * @file llpanelsnapshotlocal.cpp
 * @brief The panel provides UI for saving snapshot to a local folder.
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
#include "llsliderctrl.h"
#include "llspinctrl.h"

#include "llfloatersnapshot.h" // FIXME: replace with a snapshot storage model
#include "llpanelsnapshot.h"
#include "llviewercontrol.h" // gSavedSettings

/**
 * The panel provides UI for saving snapshot to a local folder.
 */
class LLPanelSnapshotLocal
:	public LLPanelSnapshot
{
	LOG_CLASS(LLPanelSnapshotLocal);

public:
	LLPanelSnapshotLocal();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "local_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "local_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "local_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "local_size_combo"; }
	/*virtual*/ LLFloaterSnapshot::ESnapshotFormat getImageFormat() const;
	/*virtual*/ void updateControls(const LLSD& info);

	void updateCustomResControls(); ///< Show/hide custom resolution controls (spinners and checkbox)

	void onFormatComboCommit(LLUICtrl* ctrl);
	void onResolutionComboCommit(LLUICtrl* ctrl);
	void onCustomResolutionCommit(LLUICtrl* ctrl);
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);
	void onQualitySliderCommit(LLUICtrl* ctrl);
	void onSend();
};

static LLRegisterPanelClassWrapper<LLPanelSnapshotLocal> panel_class("llpanelsnapshotlocal");

LLPanelSnapshotLocal::LLPanelSnapshotLocal()
{
	mCommitCallbackRegistrar.add("Local.Save",		boost::bind(&LLPanelSnapshotLocal::onSend,		this));
	mCommitCallbackRegistrar.add("Local.Cancel",	boost::bind(&LLPanelSnapshotLocal::cancel,		this));
}

// virtual
BOOL LLPanelSnapshotLocal::postBuild()
{
	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onResolutionComboCommit, this, _1));
	getChild<LLUICtrl>(getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onKeepAspectRatioCommit, this, _1));
	getChild<LLUICtrl>("image_quality_slider")->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onQualitySliderCommit, this, _1));
	getChild<LLUICtrl>("local_format_combo")->setCommitCallback(boost::bind(&LLPanelSnapshotLocal::onFormatComboCommit, this, _1));

	return LLPanelSnapshot::postBuild();
}

// virtual
void LLPanelSnapshotLocal::onOpen(const LLSD& key)
{
	updateCustomResControls();
	LLPanelSnapshot::onOpen(key);
}

// virtual
LLFloaterSnapshot::ESnapshotFormat LLPanelSnapshotLocal::getImageFormat() const
{
	LLFloaterSnapshot::ESnapshotFormat fmt = LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG;

	LLComboBox* local_format_combo = getChild<LLComboBox>("local_format_combo");
	const std::string id  = local_format_combo->getValue().asString();
	if (id == "PNG")
	{
		fmt = LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG;
	}
	else if (id == "JPEG")
	{
		fmt = LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG;
	}
	else if (id == "BMP")
	{
		fmt = LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP;
	}

	return fmt;
}

// virtual
void LLPanelSnapshotLocal::updateControls(const LLSD& info)
{
	LLFloaterSnapshot::ESnapshotFormat fmt =
		(LLFloaterSnapshot::ESnapshotFormat) gSavedSettings.getS32("SnapshotFormat");
	getChild<LLComboBox>("local_format_combo")->selectNthItem((S32) fmt);

	const bool show_quality_ctrls = (fmt == LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);
	getChild<LLUICtrl>("image_quality_slider")->setVisible(show_quality_ctrls);
	getChild<LLUICtrl>("image_quality_level")->setVisible(show_quality_ctrls);

	getChild<LLUICtrl>("image_quality_slider")->setValue(gSavedSettings.getS32("SnapshotQuality"));
	updateImageQualityLevel();

	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("save_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotLocal::updateCustomResControls()
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

void LLPanelSnapshotLocal::onFormatComboCommit(LLUICtrl* ctrl)
{
	// will call updateControls()
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("image-format-change", true));
}

void LLPanelSnapshotLocal::onResolutionComboCommit(LLUICtrl* ctrl)
{
	updateCustomResControls();

	LLSD info;
	info["combo-res-change"]["control-name"] = ctrl->getName();
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshotLocal::onCustomResolutionCommit(LLUICtrl* ctrl)
{
	LLSD info;
	info["w"] = getChild<LLUICtrl>(getWidthSpinnerName())->getValue().asInteger();
	info["h"] = getChild<LLUICtrl>(getHeightSpinnerName())->getValue().asInteger();
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshotLocal::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}

void LLPanelSnapshotLocal::onQualitySliderCommit(LLUICtrl* ctrl)
{
	updateImageQualityLevel();

	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());
	LLSD info;
	info["image-quality-change"] = quality_val;
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshotLocal::onSend()
{
	LLFloaterSnapshot* floater = LLFloaterSnapshot::getInstance();

	floater->notify(LLSD().with("set-working", true));
	LLFloaterSnapshot::saveLocal();
	LLFloaterSnapshot::postSave();
	goBack();
	floater->notify(LLSD().with("set-finished", LLSD().with("ok", true).with("msg", "local")));
}
