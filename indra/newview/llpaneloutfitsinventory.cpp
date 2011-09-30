/**
 * @file llpaneloutfitsinventory.cpp
 * @brief Outfits inventory panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llpaneloutfitsinventory.h"

#include "llnotificationsutil.h"
#include "lltabcontainer.h"

#include "llfloatersidepanelcontainer.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lloutfitobserver.h"
#include "lloutfitslist.h"
#include "llpanelwearing.h"
#include "llsaveoutfitcombobtn.h"
#include "llsidepanelappearance.h"
#include "llviewerfoldertype.h"

static const std::string OUTFITS_TAB_NAME = "outfitslist_tab";
static const std::string COF_TAB_NAME = "cof_tab";

static LLRegisterPanelClassWrapper<LLPanelOutfitsInventory> t_inventory("panel_outfits_inventory");

LLPanelOutfitsInventory::LLPanelOutfitsInventory() :
	mMyOutfitsPanel(NULL),
	mCurrentOutfitPanel(NULL),
	mActivePanel(NULL),
	mInitialized(false)
{
	gAgentWearables.addLoadedCallback(boost::bind(&LLPanelOutfitsInventory::onWearablesLoaded, this));
	gAgentWearables.addLoadingStartedCallback(boost::bind(&LLPanelOutfitsInventory::onWearablesLoading, this));

	LLOutfitObserver& observer = LLOutfitObserver::instance();
	observer.addBOFChangedCallback(boost::bind(&LLPanelOutfitsInventory::updateVerbs, this));
	observer.addCOFChangedCallback(boost::bind(&LLPanelOutfitsInventory::updateVerbs, this));
	observer.addOutfitLockChangedCallback(boost::bind(&LLPanelOutfitsInventory::updateVerbs, this));
}

LLPanelOutfitsInventory::~LLPanelOutfitsInventory()
{
}

// virtual
BOOL LLPanelOutfitsInventory::postBuild()
{
	initTabPanels();
	initListCommandsHandlers();

	// Fetch your outfits folder so that the links are in memory.
	// ( This is only necessary if we want to show a warning if a user deletes an item that has a
	// a link in an outfit, see "ConfirmItemDeleteHasLinks". )
	const LLUUID &outfits_cat = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTFIT, false);
	if (outfits_cat.notNull())
	{
		LLInventoryModelBackgroundFetch::instance().start(outfits_cat);
	}
	
	mSaveComboBtn.reset(new LLSaveOutfitComboBtn(this, true));

	return TRUE;
}

// virtual
void LLPanelOutfitsInventory::onOpen(const LLSD& key)
{
	if (!mInitialized)
	{
		LLSidepanelAppearance* panel_appearance = getAppearanceSP();
		if (panel_appearance)
		{
			// *TODO: move these methods to LLPanelOutfitsInventory?
			panel_appearance->fetchInventory();
			panel_appearance->refreshCurrentOutfitName();
		}
		mInitialized = true;
	}

	// Make sure we know which tab is selected, update the filter,
	// and update verbs.
	onTabChange();
	
	// *TODO: Auto open the first outfit newly created so new users can see sample outfit contents
	/*
	static bool should_open_outfit = true;
	if (should_open_outfit && gAgent.isFirstLogin())
	{
		LLInventoryPanel* outfits_panel = getChild<LLInventoryPanel>(OUTFITS_TAB_NAME);
		if (outfits_panel)
		{
			LLUUID my_outfits_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS);
			LLFolderViewFolder* my_outfits_folder = outfits_panel->getRootFolder()->getFolderByID(my_outfits_id);
			if (my_outfits_folder)
			{
				LLFolderViewFolder* first_outfit = dynamic_cast<LLFolderViewFolder*>(my_outfits_folder->getFirstChild());
				if (first_outfit)
				{
					first_outfit->setOpen(TRUE);
				}
			}
		}
	}
	should_open_outfit = false;
	*/
}

void LLPanelOutfitsInventory::updateVerbs()
{
	if (mListCommands)
	{
		updateListCommands();
	}
}

// virtual
void LLPanelOutfitsInventory::onSearchEdit(const std::string& string)
{
	if (!mActivePanel) return;

	mFilterSubString = string;

	if (string == "")
	{
		mActivePanel->setFilterSubString(LLStringUtil::null);
	}

	LLInventoryModelBackgroundFetch::instance().start();

	if (mActivePanel->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// set new filter string
	mActivePanel->setFilterSubString(string);
}

void LLPanelOutfitsInventory::onWearButtonClick()
{
	if (mMyOutfitsPanel->hasItemSelected())
	{
		mMyOutfitsPanel->wearSelectedItems();
	}
	else
	{
		mMyOutfitsPanel->performAction("replaceoutfit");
	}
}

bool LLPanelOutfitsInventory::onSaveCommit(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		std::string outfit_name = response["message"].asString();
		LLStringUtil::trim(outfit_name);
		if( !outfit_name.empty() )
		{
			LLUUID outfit_folder = LLAppearanceMgr::getInstance()->makeNewOutfitLinks(outfit_name);

			LLSidepanelAppearance* panel_appearance = getAppearanceSP();
			if (panel_appearance)
			{
				panel_appearance->showOutfitsInventoryPanel();
			}

			if (mAppearanceTabs)
			{
				mAppearanceTabs->selectTabByName(OUTFITS_TAB_NAME);
			}	
		}
	}

	return false;
}

void LLPanelOutfitsInventory::onSave()
{
	std::string outfit_name;

	if (!LLAppearanceMgr::getInstance()->getBaseOutfitName(outfit_name))
	{
		outfit_name = LLViewerFolderType::lookupNewCategoryName(LLFolderType::FT_OUTFIT);
	}

	LLSD args;
	args["DESC"] = outfit_name;

	LLSD payload;
	//payload["ids"].append(*it);
	
	LLNotificationsUtil::add("SaveOutfitAs", args, payload, boost::bind(&LLPanelOutfitsInventory::onSaveCommit, this, _1, _2));
}

//static
LLPanelOutfitsInventory* LLPanelOutfitsInventory::findInstance()
{
	return dynamic_cast<LLPanelOutfitsInventory*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfits_inventory"));
}

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelOutfitsInventory::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");
	mListCommands->childSetAction("wear_btn", boost::bind(&LLPanelOutfitsInventory::onWearButtonClick, this));
	mMyOutfitsPanel->childSetAction("trash_btn", boost::bind(&LLPanelOutfitsInventory::onTrashButtonClick, this));
}

void LLPanelOutfitsInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");
	bool wear_enabled =  isActionEnabled("wear");
	bool wear_visible = !isCOFPanelActive();
	bool make_outfit_enabled = isActionEnabled("save_outfit");

	mMyOutfitsPanel->childSetEnabled("trash_btn", trash_enabled);
	mListCommands->childSetEnabled("wear_btn", wear_enabled);
	mListCommands->childSetVisible("wear_btn", wear_visible);
	mSaveComboBtn->setMenuItemEnabled("save_outfit", make_outfit_enabled);
	if (mMyOutfitsPanel->hasItemSelected())
	{
		mListCommands->childSetToolTip("wear_btn", getString("wear_items_tooltip"));
	}
	else
	{
		mListCommands->childSetToolTip("wear_btn", getString("wear_outfit_tooltip"));
	}
}

void LLPanelOutfitsInventory::onTrashButtonClick()
{
	mMyOutfitsPanel->removeSelected();
}

bool LLPanelOutfitsInventory::isActionEnabled(const LLSD& userdata)
{
	return mActivePanel && mActivePanel->isActionEnabled(userdata);
}
// List Commands                                                                //
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Tab panels                                                                   //

void LLPanelOutfitsInventory::initTabPanels()
{
	mCurrentOutfitPanel = findChild<LLPanelWearing>(COF_TAB_NAME);
	mCurrentOutfitPanel->setSelectionChangeCallback(boost::bind(&LLPanelOutfitsInventory::updateVerbs, this));

	mMyOutfitsPanel = findChild<LLOutfitsList>(OUTFITS_TAB_NAME);
	mMyOutfitsPanel->setSelectionChangeCallback(boost::bind(&LLPanelOutfitsInventory::updateVerbs, this));

	mAppearanceTabs = getChild<LLTabContainer>("appearance_tabs");
	mAppearanceTabs->setCommitCallback(boost::bind(&LLPanelOutfitsInventory::onTabChange, this));
}

void LLPanelOutfitsInventory::onTabChange()
{
	mActivePanel = dynamic_cast<LLPanelAppearanceTab*>(mAppearanceTabs->getCurrentPanel());
	if (!mActivePanel) return;

	mActivePanel->setFilterSubString(mFilterSubString);
	mActivePanel->onOpen(LLSD());

	updateVerbs();
}

bool LLPanelOutfitsInventory::isCOFPanelActive() const
{
	if (!mActivePanel) return false;

	return mActivePanel->getName() == COF_TAB_NAME;
}

void LLPanelOutfitsInventory::setWearablesLoading(bool val)
{
	updateVerbs();
}

void LLPanelOutfitsInventory::onWearablesLoaded()
{
	setWearablesLoading(false);
}

void LLPanelOutfitsInventory::onWearablesLoading()
{
	setWearablesLoading(true);
}

// static
LLSidepanelAppearance* LLPanelOutfitsInventory::getAppearanceSP()
{
	LLSidepanelAppearance* panel_appearance =
		dynamic_cast<LLSidepanelAppearance*>(LLFloaterSidePanelContainer::getPanel("appearance"));
	return panel_appearance;
}
