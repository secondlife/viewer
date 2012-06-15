/**
 * @file llpaneloutfitedit.cpp
 * @brief Displays outfit edit information in Side Tray.
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

#include "llpaneloutfitedit.h"

// *TODO: reorder includes to match the coding standard
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lloutfitobserver.h"
#include "llcofwearables.h"
#include "llfilteredwearablelist.h"
#include "llfolderview.h"
#include "llinventory.h"
#include "llinventoryitemslist.h"
#include "llviewercontrol.h"
#include "llui.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llinventoryfunctions.h"
#include "llinventorypanel.h"
#include "llviewermenu.h"
#include "llviewerwindow.h"
#include "llviewerinventory.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfloaterinventory.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llloadingindicator.h"
#include "llmenubutton.h"
#include "llpaneloutfitsinventory.h"
#include "lluiconstants.h"
#include "llsaveoutfitcombobtn.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltoggleablemenu.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llsdutil.h"
#include "llsidepanelappearance.h"
#include "lltoggleablemenu.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"
#include "llwearableitemslist.h"
#include "llwearabletype.h"
#include "llweb.h"

static LLRegisterPanelClassWrapper<LLPanelOutfitEdit> t_outfit_edit("panel_outfit_edit");

const U64 WEARABLE_MASK = (1LL << LLInventoryType::IT_WEARABLE);
const U64 ATTACHMENT_MASK = (1LL << LLInventoryType::IT_ATTACHMENT) | (1LL << LLInventoryType::IT_OBJECT);
const U64 ALL_ITEMS_MASK = WEARABLE_MASK | ATTACHMENT_MASK;

static const std::string REVERT_BTN("revert_btn");


///////////////////////////////////////////////////////////////////////////////
// LLShopURLDispatcher
///////////////////////////////////////////////////////////////////////////////

class LLShopURLDispatcher
{
public:
	std::string resolveURL(LLWearableType::EType wearable_type, ESex sex);
	std::string resolveURL(LLAssetType::EType asset_type, ESex sex);
};

std::string LLShopURLDispatcher::resolveURL(LLWearableType::EType wearable_type, ESex sex)
{
	const std::string prefix = "MarketplaceURL";
	const std::string sex_str = (sex == SEX_MALE) ? "Male" : "Female";
	const std::string type_str = LLWearableType::getTypeName(wearable_type);

	std::string setting_name = prefix;

	switch (wearable_type)
	{
	case LLWearableType::WT_ALPHA:
	case LLWearableType::WT_NONE:
	case LLWearableType::WT_INVALID:	// just in case, this shouldn't happen
	case LLWearableType::WT_COUNT:		// just in case, this shouldn't happen
		break;

	default:
		setting_name += '_';
		setting_name += type_str;
		setting_name += sex_str;
		break;
	}

	return gSavedSettings.getString(setting_name);
}

std::string LLShopURLDispatcher::resolveURL(LLAssetType::EType asset_type, ESex sex)
{
	const std::string prefix = "MarketplaceURL";
	const std::string sex_str = (sex == SEX_MALE) ? "Male" : "Female";
	const std::string type_str = LLAssetType::lookup(asset_type);

	std::string setting_name = prefix;

	switch (asset_type)
	{
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_OBJECT:
	case LLAssetType::AT_BODYPART:
		setting_name += '_';
		setting_name += type_str;
		setting_name += sex_str;
		break;

	// to suppress warnings
	default:
		break;
	}

	return gSavedSettings.getString(setting_name);
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelOutfitEditGearMenu
///////////////////////////////////////////////////////////////////////////////

class LLPanelOutfitEditGearMenu
{
public:
	static LLToggleableMenu* create()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		registrar.add("Wearable.Create", boost::bind(onCreate, _2));

		LLToggleableMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_cof_gear.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());
		llassert(menu);
		if (menu)
		{
			populateCreateWearableSubmenus(menu);
		}

		return menu;
	}

private:
	static void onCreate(const LLSD& param)
	{
		LLWearableType::EType type = LLWearableType::typeNameToType(param.asString());
		if (type == LLWearableType::WT_NONE)
		{
			llwarns << "Invalid wearable type" << llendl;
			return;
		}

		LLAgentWearables::createWearable(type, true);
	}

	// Populate the menu with items like "New Skin", "New Pants", etc.
	static void populateCreateWearableSubmenus(LLMenuGL* menu)
	{
		LLView* menu_clothes	= gMenuHolder->getChildView("COF.Gear.New_Clothes", FALSE);
		LLView* menu_bp			= gMenuHolder->getChildView("COF.Geear.New_Body_Parts", FALSE);

		for (U8 i = LLWearableType::WT_SHAPE; i != (U8) LLWearableType::WT_COUNT; ++i)
		{
			LLWearableType::EType type = (LLWearableType::EType) i;
			const std::string& type_name = LLWearableType::getTypeName(type);

			LLMenuItemCallGL::Params p;
			p.name = type_name;
			p.label = LLTrans::getString(LLWearableType::getTypeDefaultNewName(type));
			p.on_click.function_name = "Wearable.Create";
			p.on_click.parameter = LLSD(type_name);

			LLView* parent = LLWearableType::getAssetType(type) == LLAssetType::AT_CLOTHING ?
				menu_clothes : menu_bp;
			LLUICtrlFactory::create<LLMenuItemCallGL>(p, parent);
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// LLAddWearablesGearMenu
///////////////////////////////////////////////////////////////////////////////

class LLAddWearablesGearMenu : public LLInitClass<LLAddWearablesGearMenu>
{
public:
	static LLToggleableMenu* create(LLWearableItemsList* flat_list, LLInventoryPanel* inventory_panel)
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

		llassert(flat_list);
		llassert(inventory_panel);

		LLHandle<LLView> flat_list_handle = flat_list->getHandle();
		LLHandle<LLPanel> inventory_panel_handle = inventory_panel->getHandle();

		registrar.add("AddWearable.Gear.Sort", boost::bind(onSort, flat_list_handle, inventory_panel_handle, _2));
		enable_registrar.add("AddWearable.Gear.Check", boost::bind(onCheck, flat_list_handle, inventory_panel_handle, _2));
		enable_registrar.add("AddWearable.Gear.Visible", boost::bind(onVisible, inventory_panel_handle, _2));

		LLToggleableMenu* menu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>(
			"menu_add_wearable_gear.xml",
			LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());

		return menu;
	}

private:
	static void onSort(LLHandle<LLView> flat_list_handle,
					   LLHandle<LLPanel> inventory_panel_handle,
					   LLSD::String sort_order_str)
	{
		if (flat_list_handle.isDead() || inventory_panel_handle.isDead()) return;

		LLWearableItemsList* flat_list = dynamic_cast<LLWearableItemsList*>(flat_list_handle.get());
		LLInventoryPanel* inventory_panel = dynamic_cast<LLInventoryPanel*>(inventory_panel_handle.get());

		if (!flat_list || !inventory_panel) return;

		LLWearableItemsList::ESortOrder	sort_order;

		if ("by_most_recent" == sort_order_str)
		{
			sort_order = LLWearableItemsList::E_SORT_BY_MOST_RECENT;
		}
		else if ("by_name" == sort_order_str)
		{
			sort_order = LLWearableItemsList::E_SORT_BY_NAME;
		}
		else if ("by_type" == sort_order_str)
		{
			sort_order = LLWearableItemsList::E_SORT_BY_TYPE_NAME;
		}
		else
		{
			llwarns << "Unrecognized sort order action" << llendl;
			return;
		}

		if (inventory_panel->getVisible())
		{
			inventory_panel->getViewModel()->setSorter(sort_order);
		}
		else
		{
			flat_list->setSortOrder(sort_order);
		}
	}

	static bool onCheck(LLHandle<LLView> flat_list_handle,
						LLHandle<LLPanel> inventory_panel_handle,
						LLSD::String sort_order_str)
	{
		if (flat_list_handle.isDead() || inventory_panel_handle.isDead()) return false;

		LLWearableItemsList* flat_list = dynamic_cast<LLWearableItemsList*>(flat_list_handle.get());
		LLInventoryPanel* inventory_panel = dynamic_cast<LLInventoryPanel*>(inventory_panel_handle.get());

		if (!inventory_panel || !flat_list) return false;

		// Inventory panel uses its own sort order independent from
		// flat list view so this flag is used to distinguish between
		// currently visible "tree" or "flat" representation of inventory.
		bool inventory_tree_visible = inventory_panel->getVisible();

		if (inventory_tree_visible)
		{
			U32 sort_order = inventory_panel->getSortOrder();

			if ("by_most_recent" == sort_order_str)
			{
				return LLWearableItemsList::E_SORT_BY_MOST_RECENT & sort_order;
			}
			else if ("by_name" == sort_order_str)
			{
				// If inventory panel is not sorted by date then it is sorted by name.
				return LLWearableItemsList::E_SORT_BY_MOST_RECENT & ~sort_order;
			}
			llwarns << "Unrecognized inventory panel sort order" << llendl;
		}
		else
		{
			LLWearableItemsList::ESortOrder	sort_order = flat_list->getSortOrder();

			if ("by_most_recent" == sort_order_str)
			{
				return LLWearableItemsList::E_SORT_BY_MOST_RECENT == sort_order;
			}
			else if ("by_name" == sort_order_str)
			{
				return LLWearableItemsList::E_SORT_BY_NAME == sort_order;
			}
			else if ("by_type" == sort_order_str)
			{
				return LLWearableItemsList::E_SORT_BY_TYPE_NAME == sort_order;
			}
			llwarns << "Unrecognized wearable list sort order" << llendl;
		}
		return false;
	}

	static bool onVisible(LLHandle<LLPanel> inventory_panel_handle,
						  LLSD::String sort_order_str)
	{
		if (inventory_panel_handle.isDead()) return false;

		LLInventoryPanel* inventory_panel = dynamic_cast<LLInventoryPanel*>(inventory_panel_handle.get());

		// Enable sorting by type only for the flat list of items
		// because inventory panel doesn't support this kind of sorting.
		return ( "by_type" == sort_order_str )
				&&	( !inventory_panel || !inventory_panel->getVisible() );
	}
};

///////////////////////////////////////////////////////////////////////////////
// LLCOFDragAndDropObserver
///////////////////////////////////////////////////////////////////////////////

class LLCOFDragAndDropObserver : public LLInventoryAddItemByAssetObserver
{
public:
	LLCOFDragAndDropObserver(LLInventoryModel* model);

	virtual ~LLCOFDragAndDropObserver();

	virtual void done();

private:
	LLInventoryModel* mModel;
};

inline LLCOFDragAndDropObserver::LLCOFDragAndDropObserver(LLInventoryModel* model):
		mModel(model)
{
	if (model != NULL)
	{
		model->addObserver(this);
	}
}

inline LLCOFDragAndDropObserver::~LLCOFDragAndDropObserver()
{
	if (mModel != NULL && mModel->containsObserver(this))
	{
		mModel->removeObserver(this);
	}
}

void LLCOFDragAndDropObserver::done()
{
	LLAppearanceMgr::instance().updateAppearanceFromCOF();
}

///////////////////////////////////////////////////////////////////////////////
// LLPanelOutfitEdit
///////////////////////////////////////////////////////////////////////////////

LLPanelOutfitEdit::LLPanelOutfitEdit()
:	LLPanel(), 
	mSearchFilter(NULL),
	mCOFWearables(NULL),
	mInventoryItemsPanel(NULL),
	mGearMenu(NULL),
	mAddWearablesGearMenu(NULL),
	mCOFDragAndDropObserver(NULL),
	mInitialized(false),
	mAddWearablesPanel(NULL),
	mFolderViewFilterCmbBox(NULL),
	mListViewFilterCmbBox(NULL),
	mWearableListManager(NULL),
	mPlusBtn(NULL),
	mWearablesGearMenuBtn(NULL),
	mGearMenuBtn(NULL)
{
	mSavedFolderState = new LLSaveFolderState();
	mSavedFolderState->setApply(FALSE);
	

	LLOutfitObserver& observer = LLOutfitObserver::instance();
	observer.addBOFReplacedCallback(boost::bind(&LLPanelOutfitEdit::updateCurrentOutfitName, this));
	observer.addBOFChangedCallback(boost::bind(&LLPanelOutfitEdit::updateVerbs, this));
	observer.addOutfitLockChangedCallback(boost::bind(&LLPanelOutfitEdit::updateVerbs, this));
	observer.addCOFChangedCallback(boost::bind(&LLPanelOutfitEdit::onCOFChanged, this));

	gAgentWearables.addLoadingStartedCallback(boost::bind(&LLPanelOutfitEdit::onOutfitChanging, this, true));
	gAgentWearables.addLoadedCallback(boost::bind(&LLPanelOutfitEdit::onOutfitChanging, this, false));
	
	mFolderViewItemTypes.reserve(NUM_FOLDER_VIEW_ITEM_TYPES);
	for (U32 i = 0; i < NUM_FOLDER_VIEW_ITEM_TYPES; i++)
	{
		mFolderViewItemTypes.push_back(LLLookItemType());
	}

}

LLPanelOutfitEdit::~LLPanelOutfitEdit()
{
	delete mWearableListManager;
	delete mSavedFolderState;

	delete mCOFDragAndDropObserver;

	while (!mListViewItemTypes.empty()) {
		delete mListViewItemTypes.back();
		mListViewItemTypes.pop_back();
	}
}

BOOL LLPanelOutfitEdit::postBuild()
{
	// gInventory.isInventoryUsable() no longer needs to be tested per Richard's fix for race conditions between inventory and panels
	
	mFolderViewItemTypes[FVIT_ALL] = LLLookItemType(getString("Filter.All"), ALL_ITEMS_MASK);
	mFolderViewItemTypes[FVIT_WEARABLE] = LLLookItemType(getString("Filter.Clothes/Body"), WEARABLE_MASK);
	mFolderViewItemTypes[FVIT_ATTACHMENT] = LLLookItemType(getString("Filter.Objects"), ATTACHMENT_MASK);

	//order is important, see EListViewItemType for order information
	mListViewItemTypes.push_back(new LLFilterItem(getString("Filter.All"), new LLFindNonLinksByMask(ALL_ITEMS_MASK)));
	mListViewItemTypes.push_back(new LLFilterItem(getString("Filter.Clothing"), new LLIsTypeActual(LLAssetType::AT_CLOTHING)));
	mListViewItemTypes.push_back(new LLFilterItem(getString("Filter.Bodyparts"), new LLIsTypeActual(LLAssetType::AT_BODYPART)));
	mListViewItemTypes.push_back(new LLFilterItem(getString("Filter.Objects"), new LLFindNonLinksByMask(ATTACHMENT_MASK)));;
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("shape"), new LLFindActualWearablesOfType(LLWearableType::WT_SHAPE)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("skin"), new LLFindActualWearablesOfType(LLWearableType::WT_SKIN)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("hair"), new LLFindActualWearablesOfType(LLWearableType::WT_HAIR)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("eyes"), new LLFindActualWearablesOfType(LLWearableType::WT_EYES)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("shirt"), new LLFindActualWearablesOfType(LLWearableType::WT_SHIRT)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("pants"), new LLFindActualWearablesOfType(LLWearableType::WT_PANTS)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("shoes"), new LLFindActualWearablesOfType(LLWearableType::WT_SHOES)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("socks"), new LLFindActualWearablesOfType(LLWearableType::WT_SOCKS)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("jacket"), new LLFindActualWearablesOfType(LLWearableType::WT_JACKET)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("gloves"), new LLFindActualWearablesOfType(LLWearableType::WT_GLOVES)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("undershirt"), new LLFindActualWearablesOfType(LLWearableType::WT_UNDERSHIRT)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("underpants"), new LLFindActualWearablesOfType(LLWearableType::WT_UNDERPANTS)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("skirt"), new LLFindActualWearablesOfType(LLWearableType::WT_SKIRT)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("alpha"), new LLFindActualWearablesOfType(LLWearableType::WT_ALPHA)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("tattoo"), new LLFindActualWearablesOfType(LLWearableType::WT_TATTOO)));
	mListViewItemTypes.push_back(new LLFilterItem(LLTrans::getString("physics"), new LLFindActualWearablesOfType(LLWearableType::WT_PHYSICS)));

	mCurrentOutfitName = getChild<LLTextBox>("curr_outfit_name"); 
	mStatus = getChild<LLTextBox>("status");

	mFolderViewBtn = getChild<LLButton>("folder_view_btn");
	mListViewBtn = getChild<LLButton>("list_view_btn");

	childSetCommitCallback("filter_button", boost::bind(&LLPanelOutfitEdit::showWearablesFilter, this), NULL);
	childSetCommitCallback("folder_view_btn", boost::bind(&LLPanelOutfitEdit::showWearablesFolderView, this), NULL);
	childSetCommitCallback("folder_view_btn", boost::bind(&LLPanelOutfitEdit::saveListSelection, this), NULL);
	childSetCommitCallback("list_view_btn", boost::bind(&LLPanelOutfitEdit::showWearablesListView, this), NULL);
	childSetCommitCallback("list_view_btn", boost::bind(&LLPanelOutfitEdit::saveListSelection, this), NULL);
	childSetCommitCallback("shop_btn_1", boost::bind(&LLPanelOutfitEdit::onShopButtonClicked, this), NULL);
	childSetCommitCallback("shop_btn_2", boost::bind(&LLPanelOutfitEdit::onShopButtonClicked, this), NULL);

	setVisibleCallback(boost::bind(&LLPanelOutfitEdit::onVisibilityChange, this, _2));

	mWearablesGearMenuBtn = getChild<LLMenuButton>("wearables_gear_menu_btn");
	mGearMenuBtn = getChild<LLMenuButton>("gear_menu_btn");

	mCOFWearables = findChild<LLCOFWearables>("cof_wearables_list");
	mCOFWearables->setCommitCallback(boost::bind(&LLPanelOutfitEdit::filterWearablesBySelectedItem, this));

	mCOFWearables->getCOFCallbacks().mAddWearable = boost::bind(&LLPanelOutfitEdit::onAddWearableClicked, this);
	mCOFWearables->getCOFCallbacks().mEditWearable = boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this);
	mCOFWearables->getCOFCallbacks().mDeleteWearable = boost::bind(&LLPanelOutfitEdit::onRemoveFromOutfitClicked, this);
	mCOFWearables->getCOFCallbacks().mMoveWearableCloser = boost::bind(&LLPanelOutfitEdit::moveWearable, this, true);
	mCOFWearables->getCOFCallbacks().mMoveWearableFurther = boost::bind(&LLPanelOutfitEdit::moveWearable, this, false);

	mAddWearablesPanel = getChild<LLPanel>("add_wearables_panel");

	mInventoryItemsPanel = getChild<LLInventoryPanel>("folder_view");
	mInventoryItemsPanel->setFilterTypes(ALL_ITEMS_MASK);
	mInventoryItemsPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
	mInventoryItemsPanel->setSelectCallback(boost::bind(&LLPanelOutfitEdit::updatePlusButton, this));
	mInventoryItemsPanel->getRootFolder()->setReshapeCallback(boost::bind(&LLPanelOutfitEdit::updatePlusButton, this));

	mCOFDragAndDropObserver = new LLCOFDragAndDropObserver(mInventoryItemsPanel->getModel());

	mFolderViewFilterCmbBox = getChild<LLComboBox>("folder_view_filter_combobox");
	mFolderViewFilterCmbBox->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onFolderViewFilterCommitted, this, _1));
	mFolderViewFilterCmbBox->removeall();
	for (U32 i = 0; i < mFolderViewItemTypes.size(); ++i)
	{
		mFolderViewFilterCmbBox->add(mFolderViewItemTypes[i].displayName);
	}
	mFolderViewFilterCmbBox->setCurrentByIndex(FVIT_ALL);
	
	mListViewFilterCmbBox = getChild<LLComboBox>("list_view_filter_combobox");
	mListViewFilterCmbBox->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onListViewFilterCommitted, this, _1));
	mListViewFilterCmbBox->removeall();
	for (U32 i = 0; i < mListViewItemTypes.size(); ++i)
	{
		mListViewFilterCmbBox->add(mListViewItemTypes[i]->displayName);
	}
	mListViewFilterCmbBox->setCurrentByIndex(LVIT_ALL);

	mSearchFilter = getChild<LLFilterEditor>("look_item_filter");
	mSearchFilter->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onSearchEdit, this, _2));

	childSetAction("show_add_wearables_btn", boost::bind(&LLPanelOutfitEdit::onAddMoreButtonClicked, this));

	mPlusBtn = getChild<LLButton>("plus_btn");
	mPlusBtn->setClickedCallback(boost::bind(&LLPanelOutfitEdit::onPlusBtnClicked, this));
	
	mEditWearableBtn = getChild<LLButton>("edit_wearable_btn");
	mEditWearableBtn->setEnabled(FALSE);
	mEditWearableBtn->setVisible(FALSE);
	mEditWearableBtn->setCommitCallback(boost::bind(&LLPanelOutfitEdit::onEditWearableClicked, this));

	childSetAction(REVERT_BTN, boost::bind(&LLAppearanceMgr::wearBaseOutfit, LLAppearanceMgr::getInstance()));

	/*
	 * By default AT_CLOTHING are sorted by (in in MY OUTFITS):
	 *  - by type (types order determined in LLWearableType::EType)
	 *  - each LLWearableType::EType by outer layer on top
	 *
	 * In Add More panel AT_CLOTHING should be sorted in a such way:
	 *  - by type (types order determined in LLWearableType::EType)
	 *  - each LLWearableType::EType by name (EXT-8205)
	*/
	mWearableListViewItemsComparator = new LLWearableItemTypeNameComparator();
	mWearableListViewItemsComparator->setOrder(LLAssetType::AT_CLOTHING, LLWearableItemTypeNameComparator::ORDER_RANK_1, false, true);

	mWearablesListViewPanel = getChild<LLPanel>("filtered_wearables_panel");
	mWearableItemsList = getChild<LLWearableItemsList>("list_view");
	mWearableItemsList->setCommitOnSelectionChange(true);
	mWearableItemsList->setCommitCallback(boost::bind(&LLPanelOutfitEdit::updatePlusButton, this));
	mWearableItemsList->setDoubleClickCallback(boost::bind(&LLPanelOutfitEdit::onPlusBtnClicked, this));

	mWearableItemsList->setComparator(mWearableListViewItemsComparator);

	// Creating "Add Wearables" panel gear menu after initialization of mWearableItemsList and mInventoryItemsPanel.
	mAddWearablesGearMenu = LLAddWearablesGearMenu::create(mWearableItemsList, mInventoryItemsPanel);
	mWearablesGearMenuBtn->setMenu(mAddWearablesGearMenu);

	mGearMenu = LLPanelOutfitEditGearMenu::create();
	mGearMenuBtn->setMenu(mGearMenu);

	mSaveComboBtn.reset(new LLSaveOutfitComboBtn(this));
	return TRUE;
}

// virtual
void LLPanelOutfitEdit::onOpen(const LLSD& key)
{
	if (!mInitialized)
	{
		// *TODO: this method is called even panel is not visible to user because its parent layout panel is hidden.
		// So, we can defer initializing a bit.
		mWearableListManager = new LLFilteredWearableListManager(mWearableItemsList, mListViewItemTypes[LVIT_ALL]->collector);
		mWearableListManager->populateList();
		displayCurrentOutfit();
		mInitialized = true;
	}
}

void LLPanelOutfitEdit::moveWearable(bool closer_to_body)
{
	LLUUID item_id = mCOFWearables->getSelectedUUID();
	if (item_id.isNull()) return;
	
	LLViewerInventoryItem* wearable_to_move = gInventory.getItem(item_id);
	LLAppearanceMgr::getInstance()->moveWearable(wearable_to_move, closer_to_body);
}

void LLPanelOutfitEdit::toggleAddWearablesPanel()
{
	BOOL current_visibility = mAddWearablesPanel->getVisible();
	showAddWearablesPanel(!current_visibility);
}

void LLPanelOutfitEdit::showAddWearablesPanel(bool show_add_wearables)
{
	mAddWearablesPanel->setVisible(show_add_wearables);
	
	getChild<LLUICtrl>("show_add_wearables_btn")->setValue(show_add_wearables);

	updateFiltersVisibility();
	getChildView("filter_button")->setVisible( show_add_wearables);

	//search filter should be disabled
	if (!show_add_wearables)
	{
		getChild<LLUICtrl>("filter_button")->setValue(false);

		mFolderViewFilterCmbBox->setVisible(false);
		mListViewFilterCmbBox->setVisible(false);

		showWearablesFilter();

		/*
		 * By default AT_CLOTHING are sorted by (in in MY OUTFITS):
		 *  - by type (types order determined in LLWearableType::EType)
		 *  - each LLWearableType::EType by outer layer on top
		 *
		 * In Add More panel AT_CLOTHING should be sorted in a such way:
		 *  - by type (types order determined in LLWearableType::EType)
		 *  - each LLWearableType::EType by name (EXT-8205)
		*/
		mWearableItemsList->setSortOrder(LLWearableItemsList::E_SORT_BY_TYPE_NAME);

		// Reset mWearableItemsList position to top. See EXT-8180.
		mWearableItemsList->goToTop();
	}

	//switching button bars
	getChildView("no_add_wearables_button_bar")->setVisible( !show_add_wearables);
	getChildView("add_wearables_button_bar")->setVisible( show_add_wearables);
}

void LLPanelOutfitEdit::showWearablesFilter()
{
	bool filter_visible = getChild<LLUICtrl>("filter_button")->getValue();

	getChildView("filter_panel")->setVisible( filter_visible);

	if(!filter_visible)
	{
		mSearchFilter->clear();
		onSearchEdit(LLStringUtil::null);
	}
	else
	{
		mSearchFilter->setFocus(TRUE);
	}
}

void LLPanelOutfitEdit::showWearablesListView()
{
	if(switchPanels(mInventoryItemsPanel, mWearablesListViewPanel))
	{
		updateWearablesPanelVerbButtons();
		updateFiltersVisibility();
	}
	mListViewBtn->setToggleState(TRUE);
}

void LLPanelOutfitEdit::showWearablesFolderView()
{
	if(switchPanels(mWearablesListViewPanel, mInventoryItemsPanel))
	{
		updateWearablesPanelVerbButtons();
		updateFiltersVisibility();
	}
	mFolderViewBtn->setToggleState(TRUE);
}

void LLPanelOutfitEdit::updateFiltersVisibility()
{
	mListViewFilterCmbBox->setVisible(mWearablesListViewPanel->getVisible());
	mFolderViewFilterCmbBox->setVisible(mInventoryItemsPanel->getVisible());
}

void LLPanelOutfitEdit::onFolderViewFilterCommitted(LLUICtrl* ctrl)
{
	S32 curr_filter_type = mFolderViewFilterCmbBox->getCurrentIndex();
	if (curr_filter_type < 0) return;

	mInventoryItemsPanel->setFilterTypes(mFolderViewItemTypes[curr_filter_type].inventoryMask);

	mSavedFolderState->setApply(TRUE);
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	
	LLOpenFoldersWithSelection opener;
	mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
	mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	
	LLInventoryModelBackgroundFetch::instance().start();
}

void LLPanelOutfitEdit::onListViewFilterCommitted(LLUICtrl* ctrl)
{
	S32 curr_filter_type = mListViewFilterCmbBox->getCurrentIndex();
	if (curr_filter_type < 0) return;

	mWearableListManager->setFilterCollector(mListViewItemTypes[curr_filter_type]->collector);
}

void LLPanelOutfitEdit::onSearchEdit(const std::string& string)
{
	if (mSearchString != string)
	{
		mSearchString = string;
		
		// Searches are case-insensitive
		LLStringUtil::toUpper(mSearchString);
		LLStringUtil::trimHead(mSearchString);
	}
	
	if (mSearchString == "")
	{
		mInventoryItemsPanel->setFilterSubString(LLStringUtil::null);
		mWearableItemsList->setFilterSubString(LLStringUtil::null);
		// re-open folders that were initially open
		mSavedFolderState->setApply(TRUE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
		LLOpenFoldersWithSelection opener;
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(opener);
		mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	}
	
	LLInventoryModelBackgroundFetch::instance().start();
	
	if (mInventoryItemsPanel->getFilterSubString().empty() && mSearchString.empty())
	{
		// current filter and new filter empty, do nothing
		return;
	}
	
	// save current folder open state if no filter currently applied
	if (mInventoryItemsPanel->getFilterSubString().empty())
	{
		mSavedFolderState->setApply(FALSE);
		mInventoryItemsPanel->getRootFolder()->applyFunctorRecursively(*mSavedFolderState);
	}
	
	// set new filter string
	mInventoryItemsPanel->setFilterSubString(mSearchString);
	mWearableItemsList->setFilterSubString(mSearchString);

}

void LLPanelOutfitEdit::onPlusBtnClicked(void)
{
	uuid_vec_t selected_items;
	getSelectedItemsUUID(selected_items);

	LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
	
	for(uuid_vec_t::iterator iter = selected_items.begin(); iter != selected_items.end(); iter++)
	{
		LLUUID selected_id = *iter;
		if (!selected_id.isNull())
		{
			//replacing instead of adding the item
			LLAppearanceMgr::getInstance()->wearItemOnAvatar(selected_id, false, true, link_waiter);
		}
	}
}

void LLPanelOutfitEdit::onVisibilityChange(const LLSD &in_visible_chain)
{
	showAddWearablesPanel(false);
	mWearableItemsList->resetSelection();
	mInventoryItemsPanel->clearSelection();

	if (in_visible_chain.asBoolean())
	{
		update();
	}
}

void LLPanelOutfitEdit::onAddWearableClicked(void)
{
	LLPanelDummyClothingListItem* item = dynamic_cast<LLPanelDummyClothingListItem*>(mCOFWearables->getSelectedItem());

	if(item)
	{
		showFilteredWearablesListView(item->getWearableType());
	}
}

void LLPanelOutfitEdit::onReplaceMenuItemClicked(LLUUID selected_item_id)
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(selected_item_id);

	if (item)
	{
		showFilteredWearablesListView(item->getWearableType());
	}
}

void LLPanelOutfitEdit::onShopButtonClicked()
{
	static LLShopURLDispatcher url_resolver;

	// will contain the resultant URL
	std::string url;

	if (isAgentAvatarValid())
	{
		// try to get wearable type from 'Add More' panel first (EXT-7639)
		selection_info_t selection_info = getAddMorePanelSelectionType();

		LLWearableType::EType type = selection_info.first;

		if (selection_info.second > 1)
		{
			// the second argument is not important in this case: generic market place will be opened
			url = url_resolver.resolveURL(LLWearableType::WT_NONE, SEX_FEMALE);
		}
		else
		{
		if (type == LLWearableType::WT_NONE)
		{
			type = getCOFWearablesSelectionType();
		}

		ESex sex = gAgentAvatarp->getSex();

		// WT_INVALID comes for attachments
		if (type != LLWearableType::WT_INVALID && type != LLWearableType::WT_NONE)
		{
			url = url_resolver.resolveURL(type, sex);
		}

		if (url.empty())
		{
				url = url_resolver.resolveURL(
						mCOFWearables->getExpandedAccordionAssetType(), sex);
			}
		}
	}
	else
	{
		llwarns << "Agent avatar is invalid" << llendl;

		// the second argument is not important in this case: generic market place will be opened
		url = url_resolver.resolveURL(LLWearableType::WT_NONE, SEX_FEMALE);
	}

	LLWeb::loadURLExternal(url);
}

LLWearableType::EType LLPanelOutfitEdit::getCOFWearablesSelectionType() const
{
	std::vector<LLPanel*> selected_items;
	LLWearableType::EType type = LLWearableType::WT_NONE;

	mCOFWearables->getSelectedItems(selected_items);

	if (selected_items.size() == 1)
	{
		LLPanel* item = selected_items.front();

		// LLPanelDummyClothingListItem is lower then LLPanelInventoryListItemBase in hierarchy tree
		if (LLPanelDummyClothingListItem* dummy_item = dynamic_cast<LLPanelDummyClothingListItem*>(item))
		{
			type = dummy_item->getWearableType();
		}
		else if (LLPanelInventoryListItemBase* real_item = dynamic_cast<LLPanelInventoryListItemBase*>(item))
		{
			type = real_item->getWearableType();
		}
	}

	return type;
}

LLPanelOutfitEdit::selection_info_t LLPanelOutfitEdit::getAddMorePanelSelectionType() const
{
	selection_info_t result = std::make_pair(LLWearableType::WT_NONE, 0);

	if (mAddWearablesPanel != NULL && mAddWearablesPanel->getVisible())
	{
		if (mInventoryItemsPanel != NULL && mInventoryItemsPanel->getVisible())
		{
			std::set<LLFolderViewItem*> selected_items =    mInventoryItemsPanel->getRootFolder()->getSelectionList();

			result.second = selected_items.size();

			if (result.second == 1)
			{
				result.first =    getWearableTypeByItemUUID((*selected_items.begin())->getListener()->getUUID());
			}
		}
		else if (mWearableItemsList != NULL && mWearableItemsList->getVisible())
		{
			std::vector<LLUUID> selected_uuids;
			mWearableItemsList->getSelectedUUIDs(selected_uuids);

			result.second = selected_uuids.size();

			if (result.second == 1)
			{
				result.first = getWearableTypeByItemUUID(selected_uuids.front());
			}
		}
	}

	return result;
}

LLWearableType::EType LLPanelOutfitEdit::getWearableTypeByItemUUID(const LLUUID& item_uuid) const
{
	LLViewerInventoryItem* item = gInventory.getLinkedItem(item_uuid);
	return (item != NULL) ? item->getWearableType() : LLWearableType::WT_NONE;
}

void LLPanelOutfitEdit::onRemoveFromOutfitClicked(void)
{
	LLUUID id_to_remove = mCOFWearables->getSelectedUUID();
	LLWearableType::EType type = getWearableTypeByItemUUID(id_to_remove);
	
	LLAppearanceMgr::getInstance()->removeItemFromAvatar(id_to_remove);

	if (!mCOFWearables->getSelectedItem())
	{
		mCOFWearables->selectClothing(type);
	}
}


void LLPanelOutfitEdit::onEditWearableClicked(void)
{
	LLUUID selected_item_id = mCOFWearables->getSelectedUUID();
	if (selected_item_id.notNull())
	{
		gAgentWearables.editWearable(selected_item_id);
	}
}

void LLPanelOutfitEdit::updatePlusButton()
{
	uuid_vec_t selected_items;
	getSelectedItemsUUID(selected_items);
	if (selected_items.empty())
	{
		mPlusBtn->setEnabled(false);
		return;
	}

	// If any of the selected items are not wearable (due to already being worn OR being of the wrong type), disable the add button.
	uuid_vec_t::iterator unwearable_item = std::find_if(selected_items.begin(), selected_items.end(), !boost::bind(&get_can_item_be_worn, _1));
	bool can_add = ( unwearable_item == selected_items.end() );

	mPlusBtn->setEnabled(can_add);

	LLViewerInventoryItem* first_item(gInventory.getItem(selected_items.front()));

	if (can_add && 
		first_item &&
		selected_items.size() == 1 && 
		first_item->getType() == LLAssetType::AT_BODYPART)
	{
		mPlusBtn->setToolTip(getString("replace_body_part"));
	}
	else
	{
		mPlusBtn->setToolTip(LLStringUtil::null);
	}

	/* Removing add to look inline button (not part of mvp for viewer 2)
	LLRect btn_rect(current_item->getLocalRect().mRight - 50,
					current_item->getLocalRect().mTop,
					current_item->getLocalRect().mRight - 30,
					current_item->getLocalRect().mBottom);
	
	mAddToLookBtn->setRect(btn_rect);
	mAddToLookBtn->setEnabled(TRUE);
	if (!mAddToLookBtn->getVisible())
	{
		mAddToLookBtn->setVisible(TRUE);
	}
	
	current_item->addChild(mAddToLookBtn); */
}


void LLPanelOutfitEdit::applyFolderViewFilter(EFolderViewItemType type)
{
	mFolderViewFilterCmbBox->setCurrentByIndex(type);
	mFolderViewFilterCmbBox->onCommit();
}

void LLPanelOutfitEdit::applyListViewFilter(EListViewItemType type)
{
	mListViewFilterCmbBox->setCurrentByIndex(type);
	mListViewFilterCmbBox->onCommit();
}

void LLPanelOutfitEdit::filterWearablesBySelectedItem(void)
{
	if (!mAddWearablesPanel->getVisible()) return;
	
	uuid_vec_t ids;
	mCOFWearables->getSelectedUUIDs(ids);

	bool nothing_selected = ids.empty();
	bool one_selected = ids.size() == 1;
	bool more_than_one_selected = ids.size() > 1;
	bool is_dummy_item = (ids.size() && dynamic_cast<LLPanelDummyClothingListItem*>(mCOFWearables->getSelectedItem()));

	// selected, expanded accordion tabs and selection in flat list view determine filtering when no item is selected in COF
	// selection in flat list view participates in determining filtering because of EXT-7963
	// So the priority of criterions in is:
	//                   1. Selected accordion tab            |  IF (any accordion selected)
	//                                                        |     filter_type = selected_accordion_type
	//                   2. Selected item in flat list view   |  ELSEIF (any item in flat list view selected)
	//                                                        |     filter_type = selected_item_type
	//                   3. Expanded accordion tab            |  ELSEIF (any accordion expanded)
	//                                                        |      filter_type = expanded accordion_type
	if (nothing_selected)
	{
		if (mInventoryItemsPanel->getVisible())
		{
			return;
		}
		showWearablesListView();

		//selected accordion tab is more priority than expanded tab
		//and selected item in flat list view of 'Add more' panel when
		//determining filtering
		LLAssetType::EType type = mCOFWearables->getSelectedAccordionAssetType();
		if (type == LLAssetType::AT_NONE)
		{ //no accordion selected

			// when no accordion selected then selected item from flat list view
			// has more priority than expanded when determining filtering
			LLUUID selected_item_id = mWearableItemsList->getSelectedUUID();
			LLViewerInventoryItem* item = gInventory.getLinkedItem(selected_item_id);
			if(item)
		{
				showFilteredWearablesListView(item->getWearableType());
				return;
			}

			// when no accordion selected and no selected items in flat list view
			// determine filtering according to expanded accordion
			type = mCOFWearables->getExpandedAccordionAssetType();
		}

		switch (type)
		{
		case LLAssetType::AT_OBJECT:
			applyListViewFilter(LVIT_ATTACHMENT);
			break;
		case LLAssetType::AT_BODYPART:
			applyListViewFilter(LVIT_BODYPART);
			break;
		case LLAssetType::AT_CLOTHING:
		default:
			applyListViewFilter(LVIT_CLOTHING);
			break;
		}

		return;
	}

	//resetting selection if more than one item is selected
	if (more_than_one_selected)
	{
		if (mInventoryItemsPanel->getVisible())
		{
			applyFolderViewFilter(FVIT_ALL);
			return;
		}

		showWearablesListView();
		applyListViewFilter(LVIT_ALL);
		return;
	}


	//filter wearables by a type represented by a dummy item
	if (one_selected && is_dummy_item)
	{
		if (mInventoryItemsPanel->getVisible())
		{
			applyFolderViewFilter(FVIT_WEARABLE);
			return;
		}

		onAddWearableClicked();
		return;
	}

	LLViewerInventoryItem* item = gInventory.getItem(ids[0]);
	if (!item && ids[0].notNull())
	{
		if (mInventoryItemsPanel->getVisible())
		{
			applyFolderViewFilter(FVIT_ALL);
			return;
		}
		//Inventory misses an item with non-zero id
		showWearablesListView();
		applyListViewFilter(LVIT_ALL);
		return;
	}

	if (item && one_selected && !is_dummy_item)
	{
		if (item->isWearableType())
		{
			if (mInventoryItemsPanel->getVisible())
			{
				applyFolderViewFilter(FVIT_WEARABLE);
				return;
			}
			//single clothing or bodypart item is selected
			showFilteredWearablesListView(item->getWearableType());
			return;
		}
		else
		{
			if (mInventoryItemsPanel->getVisible())
			{
				applyFolderViewFilter(FVIT_ATTACHMENT);
				return;
			}
			//attachment is selected
			showWearablesListView();
			applyListViewFilter(LVIT_ATTACHMENT);
			return;
		}
	}

}



void LLPanelOutfitEdit::update()
{
	mCOFWearables->refresh();

	updateVerbs();
}

BOOL LLPanelOutfitEdit::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										  EDragAndDropType cargo_type,
										  void* cargo_data,
										  EAcceptance* accept,
										  std::string& tooltip_msg)
{
	if (cargo_data == NULL)
	{
		llwarns << "cargo_data is NULL" << llendl;
		return TRUE;
	}

	switch (cargo_type)
	{
	case DAD_BODYPART:
	case DAD_CLOTHING:
	case DAD_OBJECT:
	case DAD_LINK:
		*accept = ACCEPT_YES_MULTI;
		break;
	default:
		*accept = ACCEPT_NO;
	}

	if (drop)
	{
		LLInventoryItem* item = static_cast<LLInventoryItem*>(cargo_data);

		if (LLAssetType::lookupIsAssetIDKnowable(item->getType()))
		{
			mCOFDragAndDropObserver->watchAsset(item->getAssetUUID());

			/*
			 * Adding request to wear item. If the item is a link, then getLinkedUUID() will
			 * return the ID of the linked item. Otherwise it will return the item's ID. The
			 * second argument is used to delay the appearance update until all dragged items
			 * are added to optimize user experience.
			 */
			LLAppearanceMgr::instance().addCOFItemLink(item->getLinkedUUID(), false);
		}
		else
		{
			// if asset id is not available for the item we must wear it immediately (attachments only)
			LLAppearanceMgr::instance().addCOFItemLink(item->getLinkedUUID(), true);
		}
	}

	return TRUE;
}

void LLPanelOutfitEdit::displayCurrentOutfit()
{
	if (!getVisible())
	{
		setVisible(TRUE);
	}

	updateCurrentOutfitName();

	update();
}

void LLPanelOutfitEdit::updateCurrentOutfitName()
{
	std::string current_outfit_name;
	if (LLAppearanceMgr::getInstance()->getBaseOutfitName(current_outfit_name))
	{
		mCurrentOutfitName->setText(current_outfit_name);
	}
	else
	{
		mCurrentOutfitName->setText(getString("No Outfit"));
	}
}

//private
void LLPanelOutfitEdit::updateVerbs()
{
	bool outfit_is_dirty = LLAppearanceMgr::getInstance()->isOutfitDirty();
	bool outfit_locked = LLAppearanceMgr::getInstance()->isOutfitLocked();
	bool has_baseoutfit = LLAppearanceMgr::getInstance()->getBaseOutfitUUID().notNull();

	mSaveComboBtn->setSaveBtnEnabled(!outfit_locked && outfit_is_dirty);
	getChildView(REVERT_BTN)->setEnabled(outfit_is_dirty && has_baseoutfit);

	mSaveComboBtn->setMenuItemEnabled("save_outfit", !outfit_locked && outfit_is_dirty);

	mStatus->setText(outfit_is_dirty ? getString("unsaved_changes") : getString("now_editing"));

	updateCurrentOutfitName();

	//updating state of "Wear Item" button previously known as "Plus" button
	updatePlusButton();
}

bool LLPanelOutfitEdit::switchPanels(LLPanel* switch_from_panel, LLPanel* switch_to_panel)
{
	if(switch_from_panel && switch_to_panel && !switch_to_panel->getVisible())
	{
		switch_from_panel->setVisible(FALSE);
		switch_to_panel->setVisible(TRUE);
		return true;
	}
	return false;
}

void LLPanelOutfitEdit::resetAccordionState()
{
	if (mCOFWearables != NULL)
	{
		mCOFWearables->expandDefaultAccordionTab();
	}
	else
	{
		llwarns << "mCOFWearables is NULL" << llendl;
	}
}

void LLPanelOutfitEdit::onAddMoreButtonClicked()
{
	toggleAddWearablesPanel();
	filterWearablesBySelectedItem();
}

void LLPanelOutfitEdit::showFilteredWearablesListView(LLWearableType::EType type)
{
	showAddWearablesPanel(true);
	showWearablesListView();

	//e_list_view_item_type implicitly contains LLWearableType::EType starting from LVIT_SHAPE
	applyListViewFilter((EListViewItemType) (LVIT_SHAPE + type));
}

static void update_status_widget_rect(LLView * widget, S32 right_border)
{
	LLRect rect = widget->getRect();
	rect.mRight = right_border;

	widget->setShape(rect);
}

void LLPanelOutfitEdit::onOutfitChanging(bool started)
{
	static LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("edit_outfit_loading_indicator");
	static LLView* status_panel = getChild<LLView>("outfit_name_and_status");
	static S32 indicator_delta = status_panel->getRect().getWidth() - indicator->getRect().mLeft;

	S32 delta = started ? indicator_delta : 0;
	S32 right_border = status_panel->getRect().getWidth() - delta;

	update_status_widget_rect(mCurrentOutfitName, right_border);
	update_status_widget_rect(mStatus, right_border);

	indicator->setVisible(started);
}

void LLPanelOutfitEdit::getCurrentItemUUID(LLUUID& selected_id)
{
	if (mInventoryItemsPanel->getVisible())
	{
		LLFolderViewItem* curr_item = mInventoryItemsPanel->getRootFolder()->getCurSelectedItem();
		if (!curr_item) return;

		LLFolderViewModelItemInventory* listenerp  = curr_item->getListener();
		if (!listenerp) return;

		selected_id = listenerp->getUUID();
	}
	else if (mWearablesListViewPanel->getVisible())
	{
		selected_id = mWearableItemsList->getSelectedUUID();
	}
}


void LLPanelOutfitEdit::getSelectedItemsUUID(uuid_vec_t& uuid_list)
{
	void (uuid_vec_t::* tmp)(LLUUID const &) = &uuid_vec_t::push_back;
	if (mInventoryItemsPanel->getVisible())
	{
		std::set<LLFolderViewItem*> item_set =    mInventoryItemsPanel->getRootFolder()->getSelectionList();
		for (std::set<LLFolderViewItem*>::iterator it = item_set.begin(),    end_it = item_set.end();
			it != end_it;
			++it)
		{
			uuid_list.push_back((*it)->getListener()->getUUID());
		}
	}
	else if (mWearablesListViewPanel->getVisible())
	{
		std::vector<LLSD> item_set;
		mWearableItemsList->getSelectedValues(item_set);

		std::for_each(item_set.begin(), item_set.end(), boost::bind( tmp, &uuid_list, boost::bind(&LLSD::asUUID, _1 )));
	}

//	return selected_id;
}

void LLPanelOutfitEdit::onCOFChanged()
{
	//the panel is only updated when is visible to a user

	// BAP - this check has to be removed because otherwise item name
	// changes made when the panel is not visible will not be
	// propagated to the panel.
	// if (!isInVisibleChain()) return;

	update();
}

void LLPanelOutfitEdit::updateWearablesPanelVerbButtons()
{
	if(mWearablesListViewPanel->getVisible())
	{
		mFolderViewBtn->setToggleState(FALSE);
		mFolderViewBtn->setImageOverlay(getString("folder_view_off"), mFolderViewBtn->getImageOverlayHAlign());
		mListViewBtn->setImageOverlay(getString("list_view_on"), mListViewBtn->getImageOverlayHAlign());
	}
	else if(mInventoryItemsPanel->getVisible())
	{
		mListViewBtn->setToggleState(FALSE);
		mListViewBtn->setImageOverlay(getString("list_view_off"), mListViewBtn->getImageOverlayHAlign());
		mFolderViewBtn->setImageOverlay(getString("folder_view_on"), mFolderViewBtn->getImageOverlayHAlign());
	}
}

void LLPanelOutfitEdit::saveListSelection()
{
	if(mWearablesListViewPanel->getVisible())
	{
		std::set<LLFolderViewItem*> selected_ids =    mInventoryItemsPanel->getRootFolder()->getSelectionList();

		if(!selected_ids.size()) return;

		for (std::set<LLFolderViewItem*>::const_iterator item_id =    selected_ids.begin(); item_id != selected_ids.end(); ++item_id)
		{
			mWearableItemsList->selectItemByUUID((*item_id)->getListener()->getUUID(),    true);
		}
		mWearableItemsList->scrollToShowFirstSelectedItem();
	}
	else if(mInventoryItemsPanel->getVisible())
	{
		std::vector<LLUUID> selected_ids;
		mWearableItemsList->getSelectedUUIDs(selected_ids);

		if(!selected_ids.size()) return;

		mInventoryItemsPanel->clearSelection();
		LLFolderView* root = mInventoryItemsPanel->getRootFolder();

		if(!root) return;

		for(std::vector<LLUUID>::const_iterator item_id = selected_ids.begin(); item_id != selected_ids.end(); ++item_id)
		{
			LLFolderViewItem* item = mInventoryItemsPanel->getItemByID(*item_id);
			if (!item) continue;

			LLFolderViewFolder* parent = item->getParentFolder();
			if(parent)
			{
				parent->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
			}
			mInventoryItemsPanel->getRootFolder()->changeSelection(item, TRUE);
		}
		mInventoryItemsPanel->getRootFolder()->scrollToShowSelection();
	}
}

// EOF
