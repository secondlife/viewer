/** 
 * @file llcofwearables.cpp
 * @brief LLCOFWearables displayes wearables from the current outfit split into three lists (attachments, clothing and body parts)
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llcofwearables.h"

#include "llagentdata.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "lllistcontextmenu.h"
#include "llmenugl.h"
#include "llviewermenu.h"
#include "llwearableitemslist.h"
#include "llpaneloutfitedit.h"
#include "llsidetray.h"
#include "lltrans.h"

static LLRegisterPanelClassWrapper<LLCOFWearables> t_cof_wearables("cof_wearables");

const LLSD REARRANGE = LLSD().with("rearrange", LLSD());

static const LLWearableItemNameComparator WEARABLE_NAME_COMPARATOR;

//////////////////////////////////////////////////////////////////////////

class CofContextMenu : public LLListContextMenu
{
protected:
	static void updateCreateWearableLabel(LLMenuGL* menu, const LLUUID& item_id)
	{
		LLMenuItemGL* menu_item = menu->getChild<LLMenuItemGL>("create_new");

		// Hide the "Create new <WEARABLE_TYPE>" if it's irrelevant.
		LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
		if (!item || !item->isWearableType())
		{
			menu_item->setVisible(FALSE);
			return;
		}

		// Set proper label for the "Create new <WEARABLE_TYPE>" menu item.
		LLStringUtil::format_map_t args;
		LLWearableType::EType w_type = item->getWearableType();
		args["[WEARABLE_TYPE]"] = LLWearableType::getTypeDefaultNewName(w_type);
		std::string new_label = LLTrans::getString("CreateNewWearable", args);
		menu_item->setLabel(new_label);
	}

	static void createNew(const LLUUID& item_id)
	{
		LLViewerInventoryItem* item = gInventory.getLinkedItem(item_id);
		if (!item || !item->isWearableType()) return;

		LLAgentWearables::createWearable(item->getWearableType(), true);
	}
};

//////////////////////////////////////////////////////////////////////////

class CofAttachmentContextMenu : public LLListContextMenu
{
protected:

	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;

		functor_t take_off = boost::bind(&LLAppearanceMgr::removeItemFromAvatar, LLAppearanceMgr::getInstance(), _1);
		registrar.add("Attachment.Detach", boost::bind(handleMultiple, take_off, mUUIDs));

		return createFromFile("menu_cof_attachment.xml");
	}
};

//////////////////////////////////////////////////////////////////////////

class CofClothingContextMenu : public CofContextMenu
{
protected:

	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		LLUUID selected_id = mUUIDs.back();
		functor_t take_off = boost::bind(&LLAppearanceMgr::removeItemFromAvatar, LLAppearanceMgr::getInstance(), _1);

		registrar.add("Clothing.TakeOff", boost::bind(handleMultiple, take_off, mUUIDs));
		registrar.add("Clothing.MoveUp", boost::bind(moveWearable, selected_id, false));
		registrar.add("Clothing.MoveDown", boost::bind(moveWearable, selected_id, true));
		registrar.add("Clothing.Edit", boost::bind(LLAgentWearables::editWearable, selected_id));
		registrar.add("Clothing.Create", boost::bind(createNew, selected_id));

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

		if ("move_up" == param)
		{
			return gAgentWearables.canMoveWearable(selected_id, false);
		}
		else if ("move_down" == param)
		{
			return gAgentWearables.canMoveWearable(selected_id, true);
		}
		else if ("take_off" == param)
		{
			return get_is_item_worn(selected_id);
		}
		else if ("edit" == param)
		{
			return gAgentWearables.isWearableModifiable(selected_id);
		}
		return true;
	}

	// We don't use LLAppearanceMgr::moveWearable() directly because
	// the item may be invalidated between setting the callback and calling it.
	static bool moveWearable(const LLUUID& item_id, bool closer_to_body)
	{
		LLViewerInventoryItem* item = gInventory.getItem(item_id);
		return LLAppearanceMgr::instance().moveWearable(item, closer_to_body);
	}
};

//////////////////////////////////////////////////////////////////////////

class CofBodyPartContextMenu : public CofContextMenu
{
protected:

	/*virtual*/ LLContextMenu* createMenu()
	{
		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
		LLUUID selected_id = mUUIDs.back();

		// *HACK* need to pass pointer to LLPanelOutfitEdit instead of LLSideTray::getInstance()->getPanel().
		// LLSideTray::getInstance()->getPanel() is rather slow variant
		LLPanelOutfitEdit* panel_oe = dynamic_cast<LLPanelOutfitEdit*>(LLSideTray::getInstance()->getPanel("panel_outfit_edit"));
		registrar.add("BodyPart.Replace", boost::bind(&LLPanelOutfitEdit::onReplaceBodyPartMenuItemClicked, panel_oe, selected_id));
		registrar.add("BodyPart.Edit", boost::bind(LLAgentWearables::editWearable, selected_id));
		registrar.add("BodyPart.Create", boost::bind(createNew, selected_id));

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
			return gAgentWearables.isWearableModifiable(selected_id);
		}

		return true;
	}
};

//////////////////////////////////////////////////////////////////////////

LLCOFWearables::LLCOFWearables() : LLPanel(),
	mAttachments(NULL),
	mClothing(NULL),
	mBodyParts(NULL),
	mLastSelectedList(NULL)
{
	mClothingMenu = new CofClothingContextMenu();
	mAttachmentMenu = new CofAttachmentContextMenu();
	mBodyPartMenu = new CofBodyPartContextMenu();
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

	return LLPanel::postBuild();
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

void LLCOFWearables::refresh()
{
	typedef std::vector<LLSD> values_vector_t;
	typedef std::map<LLFlatListView*, values_vector_t> selection_map_t;

	selection_map_t preserve_selection;

	// Save current selection
	mAttachments->getSelectedValues(preserve_selection[mAttachments]);
	mClothing->getSelectedValues(preserve_selection[mClothing]);
	mBodyParts->getSelectedValues(preserve_selection[mBodyParts]);

	clear();

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t cof_items;

	gInventory.collectDescendents(LLAppearanceMgr::getInstance()->getCOF(), cats, cof_items, LLInventoryModel::EXCLUDE_TRASH);

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
		const values_vector_t& values = iter->second;
		for (values_vector_t::const_iterator
				 value_it = values.begin(),
				 value_it_end = values.end();
			 value_it != value_it_end; ++value_it)
		{
			list->selectItemByValue(*value_it);
		}
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
	item_panel->childSetAction("btn_delete", mCOFCallbacks.mDeleteWearable);
	item_panel->childSetAction("btn_move_up", mCOFCallbacks.mMoveWearableFurther);
	item_panel->childSetAction("btn_move_down", mCOFCallbacks.mMoveWearableCloser);
	item_panel->childSetAction("btn_edit", mCOFCallbacks.mEditWearable);
	
	//turning on gray separator line for the last item in the items group of the same wearable type
	item_panel->childSetVisible("wearable_type_separator_panel", last);

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
	item_panel->childSetAction("btn_delete", mCOFCallbacks.mDeleteWearable);
	item_panel->childSetAction("btn_edit", mCOFCallbacks.mEditWearable);

	return item_panel;
}

LLPanelDeletableWearableListItem* LLCOFWearables::buildAttachemntListItem(LLViewerInventoryItem* item)
{
	llassert(item);
	if (!item) return NULL;

	LLPanelDeletableWearableListItem* item_panel = LLPanelDeletableWearableListItem::create(item);
	if (!item_panel) return NULL;

	//setting callbacks
	//*TODO move that item panel's inner structure disclosing stuff into the panels
	item_panel->childSetAction("btn_delete", mCOFCallbacks.mDeleteWearable);

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
		item_panel->childSetAction("btn_add", mCOFCallbacks.mAddWearable);
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

void LLCOFWearables::clear()
{
	mAttachments->clear();
	mClothing->clear();
	mBodyParts->clear();
}

void LLCOFWearables::onListRightClick(LLUICtrl* ctrl, S32 x, S32 y, LLListContextMenu* menu)
{
	if(menu)
	{
		uuid_vec_t selected_uuids;
		if(getSelectedUUIDs(selected_uuids))
		{
			menu->show(ctrl, selected_uuids, x, y);
		}
	}
}

//EOF
