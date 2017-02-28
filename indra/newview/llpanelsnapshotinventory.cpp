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
#include "lleconomy.h"
#include "llsidetraypanelcontainer.h"
#include "llspinctrl.h"

#include "llfloatersnapshot.h" // FIXME: replace with a snapshot storage model
#include "llpanelsnapshot.h"
#include "llsnapshotlivepreview.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llstatusbar.h"	// can_afford_transaction()
#include "llnotificationsutil.h"

/**
 * The panel provides UI for saving snapshot as an inventory texture.
 */
class LLPanelSnapshotInventoryBase
    : public LLPanelSnapshot
{
    LOG_CLASS(LLPanelSnapshotInventoryBase);

public:
    LLPanelSnapshotInventoryBase();

	/*virtual*/ BOOL postBuild();
protected:
    void onSend();
    /*virtual*/ LLSnapshotModel::ESnapshotType getSnapshotType();
};

class LLPanelSnapshotInventory
    : public LLPanelSnapshotInventoryBase
{
	LOG_CLASS(LLPanelSnapshotInventory);

public:
	LLPanelSnapshotInventory();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	void onResolutionCommit(LLUICtrl* ctrl);

private:
	/*virtual*/ std::string getWidthSpinnerName() const		{ return "inventory_snapshot_width"; }
	/*virtual*/ std::string getHeightSpinnerName() const	{ return "inventory_snapshot_height"; }
	/*virtual*/ std::string getAspectRatioCBName() const	{ return "inventory_keep_aspect_check"; }
	/*virtual*/ std::string getImageSizeComboName() const	{ return "texture_size_combo"; }
	/*virtual*/ std::string getImageSizePanelName() const	{ return LLStringUtil::null; }
	/*virtual*/ void updateControls(const LLSD& info);

};

class LLPanelOutfitSnapshotInventory
    : public LLPanelSnapshotInventoryBase
{
    LOG_CLASS(LLPanelOutfitSnapshotInventory);

public:
    LLPanelOutfitSnapshotInventory();
    	/*virtual*/ BOOL postBuild();
    	/*virtual*/ void onOpen(const LLSD& key);
        
private:
    /*virtual*/ std::string getWidthSpinnerName() const		{ return ""; }
    /*virtual*/ std::string getHeightSpinnerName() const	{ return ""; }
    /*virtual*/ std::string getAspectRatioCBName() const	{ return ""; }
    /*virtual*/ std::string getImageSizeComboName() const	{ return "texture_size_combo"; }
    /*virtual*/ std::string getImageSizePanelName() const	{ return LLStringUtil::null; }
    /*virtual*/ void updateControls(const LLSD& info);

    /*virtual*/ void cancel();
};

static LLPanelInjector<LLPanelSnapshotInventory> panel_class1("llpanelsnapshotinventory");

static LLPanelInjector<LLPanelOutfitSnapshotInventory> panel_class2("llpaneloutfitsnapshotinventory");

LLPanelSnapshotInventoryBase::LLPanelSnapshotInventoryBase()
{
}

BOOL LLPanelSnapshotInventoryBase::postBuild()
{
    return LLPanelSnapshot::postBuild();
}

LLSnapshotModel::ESnapshotType LLPanelSnapshotInventoryBase::getSnapshotType()
{
    return LLSnapshotModel::SNAPSHOT_TEXTURE;
}

LLPanelSnapshotInventory::LLPanelSnapshotInventory()
{
	mCommitCallbackRegistrar.add("Inventory.Save",		boost::bind(&LLPanelSnapshotInventory::onSend,		this));
	mCommitCallbackRegistrar.add("Inventory.Cancel",	boost::bind(&LLPanelSnapshotInventory::cancel,		this));
}

// virtual
BOOL LLPanelSnapshotInventory::postBuild()
{
	getChild<LLSpinCtrl>(getWidthSpinnerName())->setAllowEdit(FALSE);
	getChild<LLSpinCtrl>(getHeightSpinnerName())->setAllowEdit(FALSE);

	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshotInventory::onResolutionCommit, this, _1));
	return LLPanelSnapshotInventoryBase::postBuild();
}

// virtual
void LLPanelSnapshotInventory::onOpen(const LLSD& key)
{
	getChild<LLUICtrl>("hint_lbl")->setTextArg("[UPLOAD_COST]", llformat("%d", LLGlobalEconomy::getInstance()->getPriceUpload()));
	LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelSnapshotInventory::updateControls(const LLSD& info)
{
	const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
	getChild<LLUICtrl>("save_btn")->setEnabled(have_snapshot);
}

void LLPanelSnapshotInventory::onResolutionCommit(LLUICtrl* ctrl)
{
	BOOL current_window_selected = (getChild<LLComboBox>(getImageSizeComboName())->getCurrentIndex() == 3);
	getChild<LLSpinCtrl>(getWidthSpinnerName())->setVisible(!current_window_selected);
	getChild<LLSpinCtrl>(getHeightSpinnerName())->setVisible(!current_window_selected);
}

void LLPanelSnapshotInventoryBase::onSend()
{
    S32 expected_upload_cost = LLGlobalEconomy::getInstance()->getPriceUpload();
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

LLPanelOutfitSnapshotInventory::LLPanelOutfitSnapshotInventory()
{
    mCommitCallbackRegistrar.add("Inventory.SaveOutfitPhoto", boost::bind(&LLPanelOutfitSnapshotInventory::onSend, this));
    mCommitCallbackRegistrar.add("Inventory.SaveOutfitCancel", boost::bind(&LLPanelOutfitSnapshotInventory::cancel, this));
}

// virtual
BOOL LLPanelOutfitSnapshotInventory::postBuild()
{
    return LLPanelSnapshotInventoryBase::postBuild();
}

// virtual
void LLPanelOutfitSnapshotInventory::onOpen(const LLSD& key)
{
    getChild<LLUICtrl>("hint_lbl")->setTextArg("[UPLOAD_COST]", llformat("%d", LLGlobalEconomy::getInstance()->getPriceUpload()));
    LLPanelSnapshot::onOpen(key);
}

// virtual
void LLPanelOutfitSnapshotInventory::updateControls(const LLSD& info)
{
    const bool have_snapshot = info.has("have-snapshot") ? info["have-snapshot"].asBoolean() : true;
    getChild<LLUICtrl>("save_btn")->setEnabled(have_snapshot);
}

void LLPanelOutfitSnapshotInventory::cancel()
{
    if (mSnapshotFloater)
    {
        mSnapshotFloater->closeFloater();
    }
}
