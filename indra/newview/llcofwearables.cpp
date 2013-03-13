/** 
 * @file llcofwearables.cpp
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llcofwearables.h"

#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagentdata.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "lllistcontextmenu.h"
#include "llmenugl.h"
#include "llviewermenu.h"
#include "llwearableitemslist.h"
#include "llpaneloutfitedit.h"
#include "lltrans.h"

static LLRegisterPanelClassWrapper<LLCOFWearables> t_cof_wearables("cof_wearables");

const LLSD REARRANGE = LLSD().with("rearrange", LLSD());

static const LLWearableItemNameComparator WEARABLE_NAME_COMPARATOR;

//////////////////////////////////////////////////////////////////////////

class CofContextMenu : public LLListContextMenu
{
protected:
	CofContextMenu(LLCOFWearables* cof_wearables)
	:	mCOFWearables(cof_wearables)
	{
		llassert(mCOFWearables);
	}

	void updateCreateWearableLabel(LLMenuGL* menu, const LLUUID& item_id)
	{
		LLMenuItemGL* menu_item = menu->getChild<LLMenuItemGL>("create_new");
		LLWearableType::EType w_type = getWearableType(item_id);

		// Hide the "Create new <WEARABLE_TYPE>" if it's irrelevant.
		if (w_type == LLWearableType::WT_NONE)
		{
			menu_item->setVisible(FALSE);
			return;
		}

		// Set proper label for the "Create new <WEARABLE_TYPE>" menu item.
		std::string new_label = LLTrans::getString("create_new_" + LLWearableType::getTypeName(w_type));
		menu_item->setLabel(new_label);
	}

	void createNew(const LLUUID& item_id)
	{
		LLAgentWearables::createWearable(getWearableType(item_id), true);
	}

	// Get wearable type of the given item.
	//
	// There is a special case: so-called "dummy items"
	// (i.e. the ones that are there just to indicate that you're not wearing
	// any wearables of the corresponding type. They are currently grayed out
	// and suffixed with "not worn").
	// Those items don't have an UUID, but they do have an associated wearable type.
	// If the user has invoked context menu for such item,
	// we ignore the passed item_id and retrieve wearable type from the item.
	LLWearableType::EType getWearableType(const LLUUID& item_id)
	{
		if (!isDummyItem(item_id))
		{
			LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
			if (item && item->isWearableType())
			{
				return item->getWearableType();
			}
		}
		else if (mCOFWearables) // dummy item selected
		{
			LLPanelDummyClothingListItem* item;

			item = dynamic_cast<LLPanelDummyClothingListItem*>(mCOFWearables->getSelectedItem());
			if (item)
			{
				return item->getWearableType();
			}
		}

		return LLWearableType::WT_NONE;
	}

	static bool isDummyItem(const LLUUID& item_id)
	{
		return item_id.isNull();
	}

	LLCOFWearables* mCOFWearables;
};

//////////////////////////////////////////////////////////////////////////

class CofAttachmentContextMenu : public CofContextMenu
{
public:
	CofAttachmentContextMenu(LLCOFWearables* cof_wearables)
	:	CofContextMenu(cof_wearables)
	{
	}

protected:

	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		registrar.add("Attachment.Detach", boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), mUUIDs));

		return createFromFile("menu_cof_attachment.xml");
	}
};

//////////////////////////////////////////////////////////////////////////

class CofClothingContextMenu : public CofContextMenu
{
public:
	CofClothingContextMenu(LLCOFWearables* cof_wearables)
	:	CofContextMenu(cof_wearables)
	{
	}

protected:
	static void replaceWearable(const LLUUID& item_id)
	{
		LLPanelOutfitEdit	* panel_outfit_edit =
						dynamic_cast<LLPanelOutfitEdit*> (LLFloaterSidePanelContainer::getPanel("appearance",
								"panel_outfit_edit"));
		if (panel_outfit_edit != NULL)
		{
			panel_outfit_edit->onReplaceMenuItemClicked(item_id);
		}
	}

	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		LLUUID selected_id = mUUIDs.back();

		registrar.add("Clothing.TakeOff", boost::bind(&LLAppearanceMgr::removeItemsFromAvatar, LLAppearanceMgr::getInstance(), mUUIDs));
		registrar.add("Clothing.Replace", boost::bind(replaceWearable, selected_id));
		registrar.add("Clothing.Edit", boost::bind(LLAgentWearables::editWearable, selected_id));
		registrar.add("Clothing.Create", boost::bind(&CofClothingContextMenu::createNew, this, selected_id));

		enable_registrar.add("Clothing.OnEnable", boost::bind(&CofClothingContextMenu::onEnable, this, _2));

		LLContextMenu* menu = createFromFile("menu_cof_clothing.xml");
		llassert(menu);
		if (menu)
		{
			updateCreateWearableLabel(menu, selected_id);
		}
		return menu;
	}

	bool onEnable(const LLSD& data)
	{
		std::string param = data.asString();
		LLUUID selected_id = mUUIDs.back();

		if ("take_off" == param)
		{
			return get_is_item_worn(selected_id);
		}
		else if ("edit" == param)
		{
			return mUUIDs.size() == 1 && gAgentWearables.isWearableModifiable(selected_id);
		}
		else if ("replace" == param)
		{
			return get_is_item_worn(selected_id) && mUUIDs.size() == 1;
		}

		return true;
	}
};

//////////////////////////////////////////////////////////////////////////

class CofBodyPartContextMenu : public CofContextMenu
{
public:
	CofBodyPartContextMenu(LLCOFWearables* cof_wearables)
	:	CofContextMenu(cof_wearables)
	{
	}

protected:
	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		LLUUID selected_id = mUUIDs.back();

		LLPanelOutfitEdit* panel_oe = dynamic_cast<LLPanelOutfitEdit*>(LLFloaterSidePanelContainer::getPanel("appearance", "panel_outfit_edit"));
		registrar.add("BodyPart.Replace", boost::bind(&LLPanelOutfitEdit::onReplaceMenuItemClicked, panel_oe, selected_id));
		registrar.add("BodyPart.Edit", boost::bind(LLAgentWearables::editWearable, selected_id));
		registrar.add("BodyPart.Create", boost::bind(&CofBodyPartContextMenu::createNew, this, selected_id));

		enable_registrar.add("BodyPart.OnEnable", boost::bind(&CofBodyPartContextMenu::onEnable, this, _2));

		LLContextMenu* menu = createFromFile("menu_cof_body_part.xml");
		llassert(menu);
		if (menu)
		{
			updateCreateWearableLabel(menu, selected_id);
		}
		return menu;
	}

	bool onEnable(const LLSD& data)
	{
		std::string param = data.asString();
		LLUUID selected_id = mUUIDs.back();

		if ("edit" == param)
		{
			return mUUIDs.size() == 1 && gAgentWearables.isWearableModifiable(selected_id);
		}

		return true;
	}
};

//////////////////////////////////////////////////////////////////////////

LLCOFWearables::LLCOFWearables() : LLPanel(),
	mAttachments(NULL),
	mClothing(NULL),
	mBodyParts(NULL),
	mLastSelectedList(NULL),
	mClothingTab(NULL),
	mAttachmentsTab(NULL),
	mBodyPartsTab(NULL),
	mLastSelectedTab(NULL),
	mAccordionCtrl(NULL),
	mCOFVersion(-1)
{
	mClothingMenu = new CofClothingContextMenu(this);
	mAttachmentMenu = new CofAttachmentContextMenu(this);
	mBodyPartMenu = new CofBodyPartContextMenu(this);
};

LLCOFWearables::~LLCOFWearables()
{
	delete mClothingMenu;
	delete mAttachmentMenu;
	delete mBodyPartMenu;
}

// virtual
BOOL LLCOFWearables::postBuild()
{
	mAttachments = getChild<LLFlatListView>("list_attachments");
	mClothing = getChild<LLFlatListView>("list_clothing");
	mBodyParts = getChild<LLFlatListView>("list_body_parts");

	mClothing->setRightMouseDownCallback(boost::bind(&LLCOFWearables::onListRightClick, this, _1, _2, _3, mClothingMenu));
	mAttachments->setRightMouseDownCallback(boost::bind(&LLCOFWearables::onListRightClick, this, _1, _2, _3, mAttachmentMenu));
	mBodyParts->setRightMouseDownCallback(boost::bind(&LLCOFWearables::onListRightClick, this, _1, _2, _3, mBodyPartMenu));

	//selection across different list/tabs is not supported
	mAttachments->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mAttachments));
	mClothing->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mClothing));
	mBodyParts->setCommitCallback(boost::bind(&LLCOFWearables::onSelectionChange, this, mBodyParts));
	
	mAttachments->setCommitOnSelectionChange(true);
	mClothing->setCommitOnSelectionChange(true);
	mBodyParts->setCommitOnSelectionChange(true);

	//clothing is sorted according to its position relatively to the body
	mAttachments->setComparator(&WEARABLE_NAME_COMPARATOR);
	mBodyParts->setComparator(&WEARABLE_NAME_COMPARATOR);

	mClothingTab = getChild<LLAccordionCtrlTab>("tab_clothing");
	mClothingTab->setDropDownStateChangedCallback(boost::bind(&LLCOFWearables::onAccordionTabStateChanged, this, _1, _2));

	mAttachmentsTab = getChild<LLAccordionCtrlTab>("tab_attachments");
	mAttachmentsTab->setDropDownStateChangedCallback(boost::bind(&LLCOFWearables::onAccordionTabStateChanged, this, _1, _2));

	mBodyPartsTab = getChild<LLAccordionCtrlTab>("tab_body_parts");
	mBodyPartsTab->setDropDownStateChangedCallback(boost::bind(&LLCOFWearables::onAccordionTabStateChanged, this, _1, _2));

	mTab2AssetType[mClothingTab] = LLAssetType::AT_CLOTHING;
	mTab2AssetType[mAttachmentsTab] = LLAssetType::AT_OBJECT;
	mTab2AssetType[mBodyPartsTab] = LLAssetType::AT_BODYPART;

	mAccordionCtrl = getChild<LLAccordionCtrl>("cof_wearables_accordion");

	return LLPanel::postBuild();
}

void LLCOFWearables::setAttachmentsTitle()
{
	if (mAttachmentsTab)
	{
		U32 free_slots = MAX_AGENT_ATTACHMENTS - mAttachments->size();

		LLStringUtil::format_map_t args_attachments;
		args_attachments["[COUNT]"] = llformat ("%d", free_slots);
		std::string attachments_title = LLTrans::getString("Attachments remain", args_attachments);
		mAttachmentsTab->setTitle(attachments_title);
	}
}

void LLCOFWearables::onSelectionChange(LLFlatListView* selected_list)
{
	if (!selected_list) return;

	if (selected_list != mLastSelectedList)
	{
		if (selected_list != mAttachments) mAttachments->resetSelection(true);
		if (selected_list != mClothing) mClothing->resetSelection(true);
		if (selected_list != mBodyParts) mBodyParts->resetSelection(true);

		mLastSelectedList = selected_list;
	}

	onCommit();
}

void LLCOFWearables::onAccordionTabStateChanged(LLUICtrl* ctrl, const LLSD& expanded)
{
	bool had_selected_items = mClothing->numSelected() || mAttachments->numSelected() || mBodyParts->numSelected();
	mClothing->resetSelection(true);
	mAttachments->resetSelection(true);
	mBodyParts->resetSelection(true);

	bool tab_selection_changed = false;
	LLAccordionCtrlTab* tab = dynamic_cast<LLAccordionCtrlTab*>(ctrl);
	if (tab && tab != mLastSelectedTab)
	{
		mLastSelectedTab = tab;
		tab_selection_changed = true;
	}

	if (had_selected_items || tab_selection_changed)
	{
		//sending commit signal to indicate selection changes
		onCommit();
	}
}

void LLCOFWearables::refresh()
{
	const LLUUID cof_id = LLAppearanceMgr::instance().getCOF();
	if (cof_id.isNull())
	{
		llwarns << "COF ID cannot be NULL" << llendl;
		return;
	}

	LLViewerInventoryCategory* catp = gInventory.getCategory(cof_id);
	if (!catp)
	{
		llwarns << "COF category cannot be NULL" << llendl;
		return;
	}

	// BAP - this check has to be removed because an item name change does not
	// change cat version - ie, checking version is not a complete way
	// of finding out whether anything has changed in this category.
	//if (mCOFVersion == catp->getVersion()) return;

	mCOFVersion = catp->getVersion();

	// Save current scrollbar position.
	typedef std::map<LLFlatListView*, LLRect> scroll_pos_map_t;
	scroll_pos_map_t saved_scroll_pos;

	saved_scroll_pos[mAttachments] = mAttachments->getVisibleContentRect();
	saved_scroll_pos[mClothing] = mClothing->getVisibleContentRect();
	saved_scroll_pos[mBodyParts] = mBodyParts->getVisibleContentRect();

	// Save current selection.
	typedef std::vector<LLSD> values_vector_t;
	typedef std::map<LLFlatListView*, values_vector_t> selection_map_t;

	selection_map_t preserve_selection;

	mAttachments->getSelectedValues(preserve_selection[mAttachments]);
	mClothing->getSelectedValues(preserve_selection[mClothing]);
	mBodyParts->getSelectedValues(preserve_selection[mBodyParts]);

	clear();

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t cof_items;

	gInventory.collectDescendents(cof_id, cats, cof_items, LLInventoryModel::EXCLUDE_TRASH);

	populateAttachmentsAndBodypartsLists(cof_items);


	LLAppearanceMgr::wearables_by_type_t clothing_by_type(LLWearableType::WT_COUNT);
	LLAppearanceMgr::getInstance()->divvyWearablesByType(cof_items, clothing_by_type);
	
	populateClothingList(clothing_by_type);

	// Restore previous selection
	for (selection_map_t::iterator
			 iter = preserve_selection.begin(),
			 iter_end = preserve_selection.end();
		 iter != iter_end; ++iter)
	{
		LLFlatListView* list = iter->first;
		if (!list) continue;

		//restoring selection should not fire commit callbacks
		list->setCommitOnSelectionChange(false);

		const values_vector_t& values = iter->second;
		for (values_vector_t::const_iterator
				 value_it = values.begin(),
				 value_it_end = values.end();
			 value_it != value_it_end; ++value_it)
		{
			// value_it may be null because of dummy items
			// Dummy items have no ID
			if(value_it->asUUID().notNull())
			{
				list->selectItemByValue(*value_it);
			}
		}

		list->setCommitOnSelectionChange(true);
	}

	// Restore previous scrollbar position.
	for (scroll_pos_map_t::const_iterator it = saved_scroll_pos.begin(); it != saved_scroll_pos.end(); ++it)
	{
		LLFlatListView* list = it->first;
		LLRect scroll_pos = it->second;

		list->scrollToShowRect(scroll_pos);
	}
}


void LLCOFWearables::populateAttachmentsAndBodypartsLists(const LLInventoryModel::item_array_t& cof_items)
{
	for (U32 i = 0; i < cof_items.size(); ++i)
	{
		LLViewerInventoryItem* item = cof_items.get(i);
		if (!item) continue;

		const LLAssetType::EType item_type = item->getType();
		if (item_type == LLAssetType::AT_CLOTHING) continue;
		LLPanelInventoryListItemBase* item_panel = NULL;
		if (item_type == LLAssetType::AT_OBJECT)
		{
			item_panel = buildAttachemntListItem(item);
			mAttachments->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
		else if (item_type == LLAssetType::AT_BODYPART)
		{
			item_panel = buildBodypartListItem(item);
			if (!item_panel) continue;

			mBodyParts->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
	}

	if (mAttachments->size())
	{
		mAttachments->sort();
		mAttachments->notify(REARRANGE); //notifying the parent about the list's size change (cause items were added with rearrange=false)
		setAttachmentsTitle();
	}
	else
	{
		mAttachments->setNoItemsCommentText(LLTrans::getString("no_attachments"));
	}

	if (mBodyParts->size())
	{
		mBodyParts->sort();
		mBodyParts->notify(REARRANGE);
	}
}

//create a clothing list item, update verbs and show/hide line separator
LLPanelClothingListItem* LLCOFWearables::buildClothingListItem(LLViewerInventoryItem* item, bool first, bool last)
{
	llassert(item);
	if (!item) return NULL;
	LLPanelClothingListItem* item_panel = LLPanelClothingListItem::create(item);
	if (!item_panel) return NULL;

	//updating verbs
	//we don't need to use permissions of a link but of an actual/linked item
	if (item->getLinkedItem()) item = item->getLinkedItem();
	llassert(item);
	if (!item) return NULL;

	bool allow_modify = item->getPermissions().allowModifyBy(gAgentID);
	
	item_panel->setShowLockButton(!allow_modify);
	item_panel->setShowEditButton(allow_modify);

	item_panel->setShowMoveUpButton(!first);
	item_panel->setShowMoveDownButton(!last);

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", boost::bind(mCOFCallbacks.mDeleteWearable));
	item_panel->childSetAction("btn_move_up", boost::bind(mCOFCallbacks.mMoveWearableFurther));
	item_panel->childSetAction("btn_move_down", boost::bind(mCOFCallbacks.mMoveWearableCloser));
	item_panel->childSetAction("btn_edit", boost::bind(mCOFCallbacks.mEditWearable));
	
	//turning on gray separator line for the last item in the items group of the same wearable type
	item_panel->setSeparatorVisible(last);

	return item_panel;
}

LLPanelBodyPartsListItem* LLCOFWearables::buildBodypartListItem(LLViewerInventoryItem* item)
{
	llassert(item);
	if (!item) return NULL;
	LLPanelBodyPartsListItem* item_panel = LLPanelBodyPartsListItem::create(item);
	if (!item_panel) return NULL;

	//updating verbs
	//we don't need to use permissions of a link but of an actual/linked item
	if (item->getLinkedItem()) item = item->getLinkedItem();
	llassert(item);
	if (!item) return NULL;
	bool allow_modify = item->getPermissions().allowModifyBy(gAgentID);
	item_panel->setShowLockButton(!allow_modify);
	item_panel->setShowEditButton(allow_modify);

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", boost::bind(mCOFCallbacks.mDeleteWearable));
	item_panel->childSetAction("btn_edit", boost::bind(mCOFCallbacks.mEditWearable));

	return item_panel;
}

LLPanelDeletableWearableListItem* LLCOFWearables::buildAttachemntListItem(LLViewerInventoryItem* item)
{
	llassert(item);
	if (!item) return NULL;

	LLPanelAttachmentListItem* item_panel = LLPanelAttachmentListItem::create(item);
	if (!item_panel) return NULL;

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", boost::bind(mCOFCallbacks.mDeleteWearable));

	return item_panel;
}

void LLCOFWearables::populateClothingList(LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == LLWearableType::WT_COUNT);

	for (U32 type = LLWearableType::WT_SHIRT; type < LLWearableType::WT_COUNT; ++type)
	{
		U32 size = clothing_by_type[type].size();
		if (!size) continue;

		LLAppearanceMgr::sortItemsByActualDescription(clothing_by_type[type]);

		//clothing items are displayed in reverse order, from furthest ones to closest ones (relatively to the body)
		for (U32 i = size; i != 0; --i)
		{
			LLViewerInventoryItem* item = clothing_by_type[type][i-1];

			LLPanelClothingListItem* item_panel = buildClothingListItem(item, i == size, i == 1);
			if (!item_panel) continue;

			mClothing->addItem(item_panel, item->getUUID(), ADD_BOTTOM, false);
		}
	}

	addClothingTypesDummies(clothing_by_type);

	mClothing->notify(REARRANGE);
}

//adding dummy items for missing wearable types
void LLCOFWearables::addClothingTypesDummies(const LLAppearanceMgr::wearables_by_type_t& clothing_by_type)
{
	llassert(clothing_by_type.size() == LLWearableType::WT_COUNT);
	
	for (U32 type = LLWearableType::WT_SHIRT; type < LLWearableType::WT_COUNT; type++)
	{
		U32 size = clothing_by_type[type].size();
		if (size) continue;

		LLWearableType::EType w_type = static_cast<LLWearableType::EType>(type);
		LLPanelInventoryListItemBase* item_panel = LLPanelDummyClothingListItem::create(w_type);
		if(!item_panel) continue;
		item_panel->childSetAction("btn_add", boost::bind(mCOFCallbacks.mAddWearable));
		mClothing->addItem(item_panel, LLUUID::null, ADD_BOTTOM, false);
	}
}

LLUUID LLCOFWearables::getSelectedUUID()
{
	if (!mLastSelectedList) return LLUUID::null;
	
	return mLastSelectedList->getSelectedUUID();
}

bool LLCOFWearables::getSelectedUUIDs(uuid_vec_t& selected_ids)
{
	if (!mLastSelectedList) return false;

	mLastSelectedList->getSelectedUUIDs(selected_ids);
	return selected_ids.size() != 0;
}

LLPanel* LLCOFWearables::getSelectedItem()
{
	if (!mLastSelectedList) return NULL;

	return mLastSelectedList->getSelectedItem();
}

void LLCOFWearables::getSelectedItems(std::vector<LLPanel*>& selected_items) const
{
	if (mLastSelectedList)
	{
		mLastSelectedList->getSelectedItems(selected_items);
	}
}

void LLCOFWearables::clear()
{
	mAttachments->clear();
	mClothing->clear();
	mBodyParts->clear();
}

LLAssetType::EType LLCOFWearables::getExpandedAccordionAssetType()
{
	typedef std::map<std::string, LLAssetType::EType> type_map_t;

	static type_map_t type_map;

	if (mAccordionCtrl != NULL)
	{
		const LLAccordionCtrlTab* expanded_tab = mAccordionCtrl->getExpandedTab();

	return get_if_there(mTab2AssetType, expanded_tab, LLAssetType::AT_NONE);
	}

	return LLAssetType::AT_NONE;
}

LLAssetType::EType LLCOFWearables::getSelectedAccordionAssetType()
	{
	if (mAccordionCtrl != NULL)
	{
		const LLAccordionCtrlTab* selected_tab = mAccordionCtrl->getSelectedTab();

	return get_if_there(mTab2AssetType, selected_tab, LLAssetType::AT_NONE);
}

	return LLAssetType::AT_NONE;
}

void LLCOFWearables::expandDefaultAccordionTab()
{
	if (mAccordionCtrl != NULL)
	{
		mAccordionCtrl->expandDefaultTab();
	}
}

void LLCOFWearables::onListRightClick(LLUICtrl* ctrl, S32 x, S32 y, LLListContextMenu* menu)
{
	if(menu)
	{
		uuid_vec_t selected_uuids;
		if(getSelectedUUIDs(selected_uuids))
		{
			bool show_menu = false;
			for(uuid_vec_t::iterator it = selected_uuids.begin();it!=selected_uuids.end();++it)
			{
				if ((*it).notNull())
				{
					show_menu = true;
					break;
				}
			}

			if(show_menu)
			{
				menu->show(ctrl, selected_uuids, x, y);
			}
		}
	}
}

void LLCOFWearables::selectClothing(LLWearableType::EType clothing_type)
{
	std::vector<LLPanel*> clothing_items;

	mClothing->getItems(clothing_items);

	std::vector<LLPanel*>::iterator it;

	for (it = clothing_items.begin(); it != clothing_items.end(); ++it )
	{
		LLPanelClothingListItem* clothing_item = dynamic_cast<LLPanelClothingListItem*>(*it);

		if (clothing_item && clothing_item->getWearableType() == clothing_type)
		{ // clothing item has specified LLWearableType::EType. Select it and exit.

			mClothing->selectItem(clothing_item);
			break;
		}
	}
}

//EOF
