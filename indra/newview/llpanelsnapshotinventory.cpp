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
#include "llviewercontrol.h" // gSavedSettings

/**
 * The panel provides UI for saving snapshot as an inventory texture.
 */
class LLPanelSnapshotInventory
:	public LLPanelSnapshot
{
	LOG_CLASS(LLPanelSnapshotInventory);

public:
	LLPanelSnapshotInventory();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	void updateCustomResControls(); ///< Show/hide custom resolution controls (spinners and checkbox)

	/*virtual*/ std::string getWidthSpinnerName() const		{ return "inventory_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "inventory_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "inventory_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "texture_size_combo"; }
	/*virtual*/ void updateControls(const LLSD& info);

	void onResolutionComboCommit(LLUICtrl* ctrl);
	void onCustomResolutionCommit(LLUICtrl* ctrl);
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);
	void onSend();
	void onCancel();
};

static LLRegisterPanelClassWrapper<LLPanelSnapshotInventory> panel_class("llpanelsnapshotinventory");

LLPanelSnapshotInventory::LLPanelSnapshotInventory()
{
	mCommitCallbackRegistrar.add("Inventory.Save",		boost::bind(&LLPanelSnapshotInventory::onSend,		this));
	mCommitCallbackRegistrar.add("Inventory.Cancel",	boost::bind(&LLPanelSnapshotInventory::onCancel,	this));
}

// virtual
BOOL LLPanelSnapshotInventory::postBuild()
{
	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onResolutionComboCommit, this, _1));
	getChild<LLUICtrl>(getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onCustomResolutionCommit, this, _1));
	getChild<LLUICtrl>(getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onKeepAspectRatioCommit, this, _1));
	return TRUE;
}

// virtual
void LLPanelSnapshotInventory::onOpen(const LLSD& key)
{
#if 0
	getChild<LLComboBox>(getImageSizeComboName())->selectNthItem(0); // FIXME? has no effect
#endif
	updateCustomResControls();
}

void LLPanelSnapshotInventory::updateCustomResControls()
{
	LLComboBox* combo = getChild<LLComboBox>(getImageSizeComboName());
	S32 selected_idx = combo->getFirstSelectedIndex();
	bool show = selected_idx == 0 || selected_idx == (combo->getItemCount() - 1); // Current Window or Custom selected

	getChild<LLUICtrl>(getWidthSpinnerName())->setVisible(show);
	getChild<LLUICtrl>(getHeightSpinnerName())->setVisible(show);
	getChild<LLUICtrl>(getAspectRatioCBName())->setVisible(show);
}

// virtual
void LLPanelSnapshotInventory::updateControls(const LLSD& info)
{
	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("save_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotInventory::onResolutionComboCommit(LLUICtrl* ctrl)
{
	updateCustomResControls();

	LLSD info;
	info["combo-res-change"]["control-name"] = ctrl->getName();
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshotInventory::onCustomResolutionCommit(LLUICtrl* ctrl)
{
	LLSD info;
	info["w"] = getChild<LLUICtrl>(getWidthSpinnerName())->getValue().asInteger();;
	info["h"] = getChild<LLUICtrl>(getHeightSpinnerName())->getValue().asInteger();;
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshotInventory::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}

void LLPanelSnapshotInventory::onSend()
{
	// Switch to upload progress display.
	LLSideTrayPanelContainer* parent = getParentContainer();
	if (parent)
	{
		parent->openPanel("panel_post_progress", LLSD().with("post-type", "inventory"));
	}

	LLFloaterSnapshot::saveTexture();
}

void LLPanelSnapshotInventory::onCancel()
{
	LLSideTrayPanelContainer* parent = getParentContainer();
	if (parent)
	{
		parent->openPreviousPanel();
	}
}
