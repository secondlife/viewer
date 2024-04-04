/** 
 * @file llpanelsnapshotoptions.cpp
 * @brief Snapshot posting options panel.
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

#include "llpanel.h"
#include "llsidetraypanelcontainer.h"

#include "llfloatersnapshot.h" // FIXME: create a snapshot model
#include "llfloaterreg.h"

#include "llagentbenefits.h"


/**
 * Provides several ways to save a snapshot.
 */
class LLPanelSnapshotOptions
:	public LLPanel
{
	LOG_CLASS(LLPanelSnapshotOptions);

public:
	LLPanelSnapshotOptions();
	~LLPanelSnapshotOptions();
	/*virtual*/ bool postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

private:
	void updateUploadCost();
	void openPanel(const std::string& panel_name);
	void onSaveToProfile();
	void onSaveToEmail();
	void onSaveToInventory();
	void onSaveToComputer();

	LLFloaterSnapshotBase* mSnapshotFloater;
};

static LLPanelInjector<LLPanelSnapshotOptions> panel_class("llpanelsnapshotoptions");

LLPanelSnapshotOptions::LLPanelSnapshotOptions()
{
	mCommitCallbackRegistrar.add("Snapshot.SaveToProfile",		boost::bind(&LLPanelSnapshotOptions::onSaveToProfile,	this));
	mCommitCallbackRegistrar.add("Snapshot.SaveToEmail",		boost::bind(&LLPanelSnapshotOptions::onSaveToEmail,		this));
	mCommitCallbackRegistrar.add("Snapshot.SaveToInventory",	boost::bind(&LLPanelSnapshotOptions::onSaveToInventory,	this));
	mCommitCallbackRegistrar.add("Snapshot.SaveToComputer",		boost::bind(&LLPanelSnapshotOptions::onSaveToComputer,	this));
}

LLPanelSnapshotOptions::~LLPanelSnapshotOptions()
{
}

// virtual
bool LLPanelSnapshotOptions::postBuild()
{
	mSnapshotFloater = getParentByType<LLFloaterSnapshotBase>();
	return LLPanel::postBuild();
}

// virtual
void LLPanelSnapshotOptions::onOpen(const LLSD& key)
{
	updateUploadCost();
}

void LLPanelSnapshotOptions::updateUploadCost()
{
	S32 upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
	getChild<LLUICtrl>("save_to_inventory_btn")->setLabelArg("[AMOUNT]", llformat("%d", upload_cost));
}

void LLPanelSnapshotOptions::openPanel(const std::string& panel_name)
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if (!parent)
	{
		LL_WARNS() << "Cannot find panel container" << LL_ENDL;
		return;
	}

	parent->openPanel(panel_name);
	parent->getCurrentPanel()->onOpen(LLSD());
	mSnapshotFloater->postPanelSwitch();
}

void LLPanelSnapshotOptions::onSaveToProfile()
{
	openPanel("panel_snapshot_profile");
}

void LLPanelSnapshotOptions::onSaveToEmail()
{
	openPanel("panel_snapshot_postcard");
}

void LLPanelSnapshotOptions::onSaveToInventory()
{
	openPanel("panel_snapshot_inventory");
}

void LLPanelSnapshotOptions::onSaveToComputer()
{
	openPanel("panel_snapshot_local");
}

