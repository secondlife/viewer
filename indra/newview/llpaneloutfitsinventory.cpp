/**
 * @file llpaneloutfitsinventory.cpp
 * @brief Outfits inventory panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneloutfitsinventory.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"

#include "llbutton.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llfoldervieweventlistener.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "lllandmark.h"
#include "lllineeditor.h"
#include "llmodaldialog.h"
#include "llnotificationsutil.h"
#include "lloutfitslist.h"
#include "llsidepanelappearance.h"
#include "llsidetray.h"
#include "lltabcontainer.h"
#include "llviewerfoldertype.h"
#include "llviewerjointattachment.h"
#include "llvoavatarself.h"

// List Commands
#include "lldndbutton.h"
#include "llmenugl.h"
#include "llviewermenu.h"

#include "llviewercontrol.h"

static const std::string OUTFITS_TAB_NAME = "outfitslist_tab";
static const std::string COF_TAB_NAME = "cof_tab";

static LLRegisterPanelClassWrapper<LLPanelOutfitsInventory> t_inventory("panel_outfits_inventory");


LLPanelOutfitsInventory::LLPanelOutfitsInventory() :
	mMyOutfitsPanel(NULL),
	mCurrentOutfitPanel(NULL),
	mParent(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	gAgentWearables.addLoadedCallback(boost::bind(&LLPanelOutfitsInventory::onWearablesLoaded, this));
}

LLPanelOutfitsInventory::~LLPanelOutfitsInventory()
{
	delete mSavedFolderState;
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
	
	return TRUE;
}

// virtual
void LLPanelOutfitsInventory::onOpen(const LLSD& key)
{
	// Make sure we know which tab is selected, update the filter,
	// and update verbs.
	onTabChange();
	
	// Auto open the first outfit newly created so new users can see sample outfit contents
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
}

void LLPanelOutfitsInventory::updateVerbs()
{
	if (mParent)
	{
		mParent->updateVerbs();
	}

	if (mListCommands)
	{
		updateListCommands();
	}
}

void LLPanelOutfitsInventory::setParent(LLSidepanelAppearance* parent)
{
	mParent = parent;
}

// virtual
void LLPanelOutfitsInventory::onSearchEdit(const std::string& string)
{
	mFilterSubString = string;

	// TODO: add handling "My Outfits" tab.
	if (!isCOFPanelActive())
	{
		mMyOutfitsPanel->setFilterSubString(string);
		return;
	}

	if (string == "")
	{
		getActivePanel()->setFilterSubString(LLStringUtil::null);

		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		getRootFolder()->applyFunctorRecursively(opener);
		getRootFolder()->scrollToShowSelection();
	}

	LLInventoryModelBackgroundFetch::instance().start();

	if (getActivePanel()->getFilterSubString().empty() && string.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}

	// save current folder open state if no filter currently applied
	if (getRootFolder()->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}

	// set new filter string
	getActivePanel()->setFilterSubString(string);
}

void LLPanelOutfitsInventory::onWearButtonClick()
{
	// TODO: Remove if/else, add common interface
	// for "My Outfits" and "Wearing" tabs.
	if (!isCOFPanelActive())
	{
		mMyOutfitsPanel->performAction("replaceoutfit");
		setWearablesLoading(true);
	}
	else
	{
		LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
		if (listenerp)
		{
			listenerp->performAction(NULL, "replaceoutfit");
		}
	}
}

void LLPanelOutfitsInventory::onAdd()
{
	// TODO: Remove if/else, add common interface
	// for "My Outfits" and "Wearing" tabs.
	if (!isCOFPanelActive())
	{
		mMyOutfitsPanel->performAction("addtooutfit");
	}
	else
	{
		LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
		if (listenerp)
		{
			listenerp->performAction(NULL, "addtooutfit");
		}
	}
}

void LLPanelOutfitsInventory::onRemove()
{
	LLFolderViewEventListener* listenerp = getCorrectListenerForAction();
	if (listenerp)
	{
		listenerp->performAction(NULL, "removefromoutfit");
	}
}

void LLPanelOutfitsInventory::onEdit()
{
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

			LLSidepanelAppearance* panel_appearance =
				dynamic_cast<LLSidepanelAppearance *>(LLSideTray::getInstance()->getPanel("sidepanel_appearance"));
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

	//)
	
/*
	LLOutfitSaveAsDialog* save_as_dialog = LLFloaterReg::showTypedInstance<LLOutfitSaveAsDialog>("outfit_save_as", LLSD(outfit_name), TRUE);
	if (save_as_dialog)
	{
		save_as_dialog->setSaveAsCommit(boost::bind(&LLPanelOutfitsInventory::onSaveCommit, this, _1 ));
	}*/
}

void LLPanelOutfitsInventory::onSelectionChange(const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	updateVerbs();

	// TODO: add handling "My Outfits" tab.
	if (!isCOFPanelActive())
		return;

	if (getRootFolder()->needsAutoRename() && items.size())
	{
		getRootFolder()->startRenamingSelectedItem();
		getRootFolder()->setNeedsAutoRename(FALSE);
	}
}

LLFolderViewEventListener *LLPanelOutfitsInventory::getCorrectListenerForAction()
{
	// TODO: add handling "My Outfits" tab.
	if (!isCOFPanelActive())
		return NULL;

	LLFolderViewItem* current_item = getRootFolder()->getCurSelectedItem();
	if (!current_item)
		return NULL;

	LLFolderViewEventListener* listenerp = current_item->getListener();
	if (getIsCorrectType(listenerp))
	{
		return listenerp;
	}
	return NULL;
}

bool LLPanelOutfitsInventory::getIsCorrectType(const LLFolderViewEventListener *listenerp) const
{
	if (listenerp->getInventoryType() == LLInventoryType::IT_CATEGORY)
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(listenerp->getUUID());
		if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			return true;
		}
	}
	return false;
}

LLFolderView *LLPanelOutfitsInventory::getRootFolder()
{
	return getActivePanel()->getRootFolder();
}

//static
LLPanelOutfitsInventory* LLPanelOutfitsInventory::findInstance()
{
	return dynamic_cast<LLPanelOutfitsInventory*>(LLSideTray::getInstance()->getPanel("panel_outfits_inventory"));
}

//////////////////////////////////////////////////////////////////////////////////
// List Commands                                                                //

void LLPanelOutfitsInventory::initListCommandsHandlers()
{
	mListCommands = getChild<LLPanel>("bottom_panel");

	mListCommands->childSetAction("options_gear_btn", boost::bind(&LLPanelOutfitsInventory::onGearButtonClick, this));
	mListCommands->childSetAction("trash_btn", boost::bind(&LLPanelOutfitsInventory::onTrashButtonClick, this));
	mListCommands->childSetAction("make_outfit_btn", boost::bind(&LLPanelOutfitsInventory::onAddButtonClick, this));
	mListCommands->childSetAction("wear_btn", boost::bind(&LLPanelOutfitsInventory::onWearButtonClick, this));

	LLDragAndDropButton* trash_btn = mListCommands->getChild<LLDragAndDropButton>("trash_btn");
	trash_btn->setDragAndDropHandler(boost::bind(&LLPanelOutfitsInventory::handleDragAndDropToTrash, this
				   ,       _4 // BOOL drop
				   ,       _5 // EDragAndDropType cargo_type
				   ,       _7 // EAcceptance* accept
				   ));

	mCommitCallbackRegistrar.add("panel_outfits_inventory_gear_default.Custom.Action",
								 boost::bind(&LLPanelOutfitsInventory::onCustomAction, this, _2));
	mEnableCallbackRegistrar.add("panel_outfits_inventory_gear_default.Enable",
								 boost::bind(&LLPanelOutfitsInventory::isActionEnabled, this, _2));
	mMenuGearDefault = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("panel_outfits_inventory_gear_default.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLPanelOutfitsInventory::updateListCommands()
{
	bool trash_enabled = isActionEnabled("delete");
	bool wear_enabled = isActionEnabled("wear");
	bool make_outfit_enabled = isActionEnabled("make_outfit");

	mListCommands->childSetEnabled("trash_btn", trash_enabled);
	mListCommands->childSetEnabled("wear_btn", wear_enabled);
	mListCommands->childSetVisible("wear_btn", wear_enabled);
	mListCommands->childSetEnabled("make_outfit_btn", make_outfit_enabled);
}

void LLPanelOutfitsInventory::onGearButtonClick()
{
	showActionMenu(mMenuGearDefault,"options_gear_btn");
}

void LLPanelOutfitsInventory::onAddButtonClick()
{
	onSave();
}

void LLPanelOutfitsInventory::showActionMenu(LLMenuGL* menu, std::string spawning_view_name)
{
	if (menu)
	{
		menu->buildDrawLabels();
		menu->updateParent(LLMenuGL::sMenuContainer);
		LLView* spawning_view = getChild<LLView> (spawning_view_name);
		S32 menu_x, menu_y;
		//show menu in co-ordinates of panel
		spawning_view->localPointToOtherView(0, spawning_view->getRect().getHeight(), &menu_x, &menu_y, this);
		menu_y += menu->getRect().getHeight();
		LLMenuGL::showPopup(this, menu, menu_x, menu_y);
	}
}

void LLPanelOutfitsInventory::onTrashButtonClick()
{
	onClipboardAction("delete");
}

void LLPanelOutfitsInventory::onClipboardAction(const LLSD& userdata)
{
	std::string command_name = userdata.asString();
	// TODO: add handling "My Outfits" tab.
	if (isCOFPanelActive())
	{
		getActivePanel()->getRootFolder()->doToSelected(getActivePanel()->getModel(),command_name);
	}
	updateListCommands();
	updateVerbs();
}

void LLPanelOutfitsInventory::onCustomAction(const LLSD& userdata)
{
	if (!isActionEnabled(userdata))
		return;

	const std::string command_name = userdata.asString();
	if (command_name == "new")
	{
		onSave();
	}
	if (command_name == "edit")
	{
		onEdit();
	}
	if (command_name == "wear")
	{
		onWearButtonClick();
	}
	// Note: This option has been removed from the gear menu.
	if (command_name == "add")
	{
		onAdd();
	}
	if (command_name == "remove")
	{
		onRemove();
	}
	if (command_name == "rename")
	{
		onClipboardAction("rename");
	}
	if (command_name == "remove_link")
	{
		onClipboardAction("delete");
	}
	if (command_name == "delete")
	{
		onClipboardAction("delete");
	}
	updateListCommands();
	updateVerbs();
}

BOOL LLPanelOutfitsInventory::isActionEnabled(const LLSD& userdata)
{
	const std::string command_name = userdata.asString();
	if (command_name == "delete" || command_name == "remove")
	{
		BOOL can_delete = FALSE;

		// TODO: add handling "My Outfits" tab.
		if (isCOFPanelActive())
		{
			LLFolderView* root = getActivePanel()->getRootFolder();
			if (root)
			{
				std::set<LLUUID> selection_set;
				root->getSelectionList(selection_set);
				can_delete = (selection_set.size() > 0);
				for (std::set<LLUUID>::iterator iter = selection_set.begin();
					 iter != selection_set.end();
					 ++iter)
				{
					const LLUUID &item_id = (*iter);
					LLFolderViewItem *item = root->getItemByID(item_id);
					can_delete &= item->getListener()->isItemRemovable();
				}
				return can_delete;
			}
		}
		return FALSE;
	}
	if (command_name == "remove_link")
	{
		BOOL can_delete = FALSE;
		LLFolderView* root = getActivePanel()->getRootFolder();
		if (root)
		{
			std::set<LLUUID> selection_set;
			root->getSelectionList(selection_set);
			can_delete = (selection_set.size() > 0);
			for (std::set<LLUUID>::iterator iter = selection_set.begin();
				 iter != selection_set.end();
				 ++iter)
			{
				const LLUUID &item_id = (*iter);
				LLViewerInventoryItem *item = gInventory.getItem(item_id);
				if (!item || !item->getIsLinkType())
					return FALSE;
			}
			return can_delete;
		}
		return FALSE;
	}
	if (command_name == "rename" ||
		command_name == "delete_outfit")
	{
		return (getCorrectListenerForAction() != NULL) && hasItemsSelected();
	}
	
	if (command_name == "wear")
	{
		if (isCOFPanelActive())
		{
			return FALSE;
		}
	}
	if (command_name == "make_outfit")
	{
		return TRUE;
	}
   
	if (command_name == "edit" || 
		command_name == "add"
		)
	{
		return (getCorrectListenerForAction() != NULL);
	}
	return TRUE;
}

bool LLPanelOutfitsInventory::hasItemsSelected()
{
	bool has_items_selected = false;

	// TODO: add handling "My Outfits" tab.
	if (isCOFPanelActive())
	{
		LLFolderView* root = getActivePanel()->getRootFolder();
		if (root)
		{
			std::set<LLUUID> selection_set;
			root->getSelectionList(selection_set);
			has_items_selected = (selection_set.size() > 0);
		}
	}
	return has_items_selected;
}

bool LLPanelOutfitsInventory::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, EAcceptance* accept)
{
	*accept = ACCEPT_NO;

	const bool is_enabled = isActionEnabled("delete");
	if (is_enabled) *accept = ACCEPT_YES_MULTI;

	if (is_enabled && drop)
	{
		onClipboardAction("delete");
	}
	return true;
}

// List Commands                                                              //
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Tab panels                                                                    //

void LLPanelOutfitsInventory::initTabPanels()
{
	mCurrentOutfitPanel = getChild<LLInventoryPanel>(COF_TAB_NAME);
	mCurrentOutfitPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mCurrentOutfitPanel->setSelectCallback(boost::bind(&LLPanelOutfitsInventory::onTabSelectionChange, this, mCurrentOutfitPanel, _1, _2));

	mMyOutfitsPanel = getChild<LLOutfitsList>(OUTFITS_TAB_NAME);

	mAppearanceTabs = getChild<LLTabContainer>("appearance_tabs");
	mAppearanceTabs->setCommitCallback(boost::bind(&LLPanelOutfitsInventory::onTabChange, this));
}

void LLPanelOutfitsInventory::onTabSelectionChange(LLInventoryPanel* tab_panel, const std::deque<LLFolderViewItem*> &items, BOOL user_action)
{
	if (user_action && items.size() > 0)
	{
		// TODO: add handling "My Outfits" tab.
		if (isCOFPanelActive())
		{
			onSelectionChange(items, user_action);
		}
		else
		{
			mCurrentOutfitPanel->getRootFolder()->clearSelection();
		}
	}
}

void LLPanelOutfitsInventory::onTabChange()
{
	// TODO: add handling "My Outfits" tab.
	if (isCOFPanelActive())
	{
		mCurrentOutfitPanel->setFilterSubString(mFilterSubString);
	}
	else
	{
		mMyOutfitsPanel->setFilterSubString(mFilterSubString);
	}

	updateVerbs();
}

BOOL LLPanelOutfitsInventory::isTabPanel(LLInventoryPanel *panel) const
{
	// TODO: add handling "My Outfits" tab.
	if (mCurrentOutfitPanel == panel)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLPanelOutfitsInventory::isCOFPanelActive() const
{
	return (childGetVisibleTab("appearance_tabs")->getName() == COF_TAB_NAME);
}

void LLPanelOutfitsInventory::setWearablesLoading(bool val)
{
	mListCommands->childSetEnabled("wear_btn", !val);

	llassert(mParent);
	if (mParent)
	{
		mParent->setWearablesLoading(val);
	}
}

void LLPanelOutfitsInventory::onWearablesLoaded()
{
	setWearablesLoading(false);
}
