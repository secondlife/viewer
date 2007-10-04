/** 
 * @file llfloaterproperties.cpp
 * @brief A floater which shows an inventory item's properties.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llfloaterproperties.h"

#include <algorithm>
#include <functional>
#include "llcachename.h"
#include "lldbstrings.h"
#include "llinventory.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llfloateravatarinfo.h"
#include "llfloatergroupinfo.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "roles_constants.h"
#include "llselectmgr.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"

#include "llvieweruictrlfactory.h"


///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const char* YOU_CAN = "You can:";
const char* OWNER_CAN = "Owner can: ";


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLPropertiesObserver
//
// helper class to watch the inventory. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLPropertiesObserver : public LLInventoryObserver
{
public:
	LLPropertiesObserver() {}
	virtual ~LLPropertiesObserver() {}
	virtual void changed(U32 mask);
};

LLPropertiesObserver* gPropertiesObserver = NULL;
void LLPropertiesObserver::changed(U32 mask)
{
	// if there's a change we're interested in.
	if((mask & (LLInventoryObserver::LABEL | LLInventoryObserver::INTERNAL | LLInventoryObserver::REMOVE)) != 0)
	{
		LLFloaterProperties::dirtyAll();
	}
}



///----------------------------------------------------------------------------
/// Class LLFloaterProperties
///----------------------------------------------------------------------------

LLFloaterProperties::instance_map LLFloaterProperties::sInstances;

// static
LLFloaterProperties* LLFloaterProperties::find(const LLUUID& item_id,
											   const LLUUID& object_id)
{
	// for simplicity's sake, we key the properties window with a
	// single uuid. However, the items are keyed by item and object
	// (obj == null -> agent inventory). So, we xor the two ids, and
	// use that as a lookup key
	instance_map::iterator it = sInstances.find(item_id ^ object_id);
	if(it != sInstances.end())
	{
		return (*it).second;
	}
	return NULL;
}

// static
LLFloaterProperties* LLFloaterProperties::show(const LLUUID& item_id,
											   const LLUUID& object_id)
{
	LLFloaterProperties* instance = find(item_id, object_id);
	if(instance)
	{
		if (LLFloater::getFloaterHost() && LLFloater::getFloaterHost() != instance->getHost())
		{
			// this properties window is being opened in a new context
			// needs to be rehosted
			LLFloater::getFloaterHost()->addFloater(instance, TRUE);
		}

		instance->refresh();
		instance->open();		/* Flawfinder: ignore */
	}
	return instance;
}

void LLFloaterProperties::dirtyAll()
{
	// ...this is more clear. Possibly more correct, because the
	// refresh method may delete the object.
	for(instance_map::iterator it = sInstances.begin(); it!=sInstances.end(); )
	{
		(*it++).second->dirty();
	}
}

// Default constructor
LLFloaterProperties::LLFloaterProperties(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_id, const LLUUID& object_id) :
	LLFloater(name, rect, title),
	mItemID(item_id),
	mObjectID(object_id),
	mDirty(TRUE)
{
	gUICtrlFactory->buildFloater(this,"floater_inventory_item_properties.xml");

	// hack to make sure these floaters are observing the inventory.
	if(!gPropertiesObserver)
	{
		gPropertiesObserver = new LLPropertiesObserver;
		gInventory.addObserver(gPropertiesObserver);
	}
	// add the object to the static structure
	LLUUID key = mItemID ^ mObjectID;
	sInstances.insert(instance_map::value_type(key, this));
	// build the UI
	// item name & description
	childSetPrevalidate("LabelItemName",&LLLineEditor::prevalidatePrintableNotPipe);
	childSetCommitCallback("LabelItemName",onCommitName,this);
	childSetPrevalidate("LabelItemDesc",&LLLineEditor::prevalidatePrintableNotPipe);
	childSetCommitCallback("LabelItemDesc", onCommitDescription, this);
	// Creator information
	childSetAction("BtnCreator",onClickCreator,this);
	// owner information
	childSetAction("BtnOwner",onClickOwner,this);
	// acquired date
	// owner permissions
	// Permissions debug text
	// group permissions
	childSetCommitCallback("CheckShareWithGroup",&onCommitPermissions, this);
	// everyone permissions
	childSetCommitCallback("CheckEveryoneCopy",&onCommitPermissions, this);
	// next owner permissions
	childSetCommitCallback("CheckNextOwnerModify",&onCommitPermissions, this);
	childSetCommitCallback("CheckNextOwnerCopy",&onCommitPermissions, this);
	childSetCommitCallback("CheckNextOwnerTransfer",&onCommitPermissions, this);
	// Mark for sale or not, and sale info
	childSetCommitCallback("CheckPurchase",&onCommitSaleInfo, this);
	childSetCommitCallback("RadioSaleType",&onCommitSaleType, this);
	// "Price" label for edit
	childSetCommitCallback("EditPrice",&onCommitSaleInfo, this);
	// The UI has been built, now fill in all the values
	refresh();
}

// Destroys the object
LLFloaterProperties::~LLFloaterProperties()
{
	// clean up the static data.
	instance_map::iterator it = sInstances.find(mItemID ^ mObjectID);
	if(it != sInstances.end())
	{
		sInstances.erase(it);
	}
}

void LLFloaterProperties::refresh()
{
	LLInventoryItem* item = findItem();
	if(item)
	{
		refreshFromItem(item);
	}
	else
	{
		//RN: it is possible that the container object is in the middle of an inventory refresh
		// causing findItem() to fail, so just temporarily disable everything
		
		mDirty = TRUE;

		const char* enableNames[]={
			"LabelItemName",
			"LabelItemDesc",
			"LabelCreatorName",
			"BtnCreator",
			"LabelOwnerName",
			"BtnOwner",
			"CheckOwnerModify",
			"CheckOwnerCopy",
			"CheckOwnerTransfer",
			"CheckShareWithGroup",
			"CheckEveryoneCopy",
			"CheckNextOwnerModify",
			"CheckNextOwnerCopy",
			"CheckNextOwnerTransfer",
			"CheckPurchase",
			"RadioSaleType",
			"EditPrice"
		};
		for(size_t t=0; t<sizeof(enableNames)/sizeof(char*); ++t)
		{
			childSetEnabled(enableNames[t],false);
		}
		const char* hideNames[]={
			"BaseMaskDebug",
			"OwnerMaskDebug",
			"GroupMaskDebug",
			"EveryoneMaskDebug",
			"NextMaskDebug"
		};
		for(size_t t=0; t<sizeof(hideNames)/sizeof(char*); ++t)
		{
			childSetVisible(hideNames[t],false);
		}
	}
}

void LLFloaterProperties::draw()
{
	if (mDirty)
	{
		// RN: clear dirty first because refresh can set dirty to TRUE
		mDirty = FALSE;
		refresh();
	}

	LLFloater::draw();
}

void LLFloaterProperties::refreshFromItem(LLInventoryItem* item)
{
	////////////////////////
	// PERMISSIONS LOOKUP //
	////////////////////////

	// do not enable the UI for incomplete items.
	LLViewerInventoryItem* i = (LLViewerInventoryItem*)item;
	BOOL is_complete = i->isComplete();

	const LLPermissions& perm = item->getPermissions();
	BOOL can_agent_manipulate = gAgent.allowOperation(PERM_OWNER, perm, 
												GP_OBJECT_MANIPULATE);
	BOOL can_agent_sell = gAgent.allowOperation(PERM_OWNER, perm, 
												GP_OBJECT_SET_SALE);

	// You need permission to modify the object to modify an inventory
	// item in it.
	LLViewerObject* object = NULL;
	if(!mObjectID.isNull()) object = gObjectList.findObject(mObjectID);
	BOOL is_obj_modify = TRUE;
	if(object)
	{
		is_obj_modify = object->permOwnerModify();
	}

	//////////////////////
	// ITEM NAME & DESC //
	//////////////////////
	BOOL is_modifiable = gAgent.allowOperation(PERM_MODIFY, perm,
												GP_OBJECT_MANIPULATE)
							&& is_obj_modify && is_complete;

	childSetEnabled("LabelItemNameTitle",TRUE);
	childSetEnabled("LabelItemName",is_modifiable);
	const char EMPTY_STRING[1] = "";		/* Flawfinder: ignore */
	const char* txt = EMPTY_STRING;
	if(!item->getName().empty())
	{
		txt = item->getName().c_str();
	}
	childSetText("LabelItemName",txt);
	childSetEnabled("LabelItemDescTitle",TRUE);
	childSetEnabled("LabelItemDesc",is_modifiable);
	childSetVisible("IconLocked",!is_modifiable);
	txt = EMPTY_STRING;
	if(!item->getDescription().empty())
	{
		txt = item->getDescription().c_str();
	}
	childSetText("LabelItemDesc",txt);

	//////////////////
	// CREATOR NAME //
	//////////////////
	char first_name[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	char last_name[DB_LAST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	if(!gCacheName) return;
	if(!gAgent.getRegion()) return;

	if (item->getCreatorUUID().notNull())
	{
		gCacheName->getName(item->getCreatorUUID(), first_name, last_name);
		LLString name(first_name);
		name.append(1, ' ');
		name.append(last_name);

		childSetEnabled("BtnCreator",TRUE);
		childSetEnabled("LabelCreatorTitle",TRUE);
		childSetEnabled("LabelCreatorName",TRUE);
		childSetText("LabelCreatorName",name);
	}
	else
	{
		childSetEnabled("BtnCreator",FALSE);
		childSetEnabled("LabelCreatorTitle",FALSE);
		childSetEnabled("LabelCreatorName",FALSE);
		childSetText("LabelCreatorName","(unknown)"); // XUI:translate
	}

	////////////////
	// OWNER NAME //
	////////////////
	if(perm.isOwned())
	{
		LLString name;
		if (perm.isGroupOwned())
		{
			char group_name[DB_GROUP_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			gCacheName->getGroupName(perm.getGroup(), group_name);
			name.assign(group_name);
		}
		else
		{
			gCacheName->getName(perm.getOwner(), first_name, last_name);
			name.assign(first_name);
			name.append(1, ' ');
			name.append(last_name);
		}
		childSetEnabled("BtnOwner",TRUE);
		childSetEnabled("LabelOwnerTitle",TRUE);
		childSetEnabled("LabelOwnerName",TRUE);
		childSetText("LabelOwnerName",name);
	}
	else
	{
		childSetEnabled("BtnOwner",FALSE);
		childSetEnabled("LabelOwnerTitle",FALSE);
		childSetEnabled("LabelOwnerName",FALSE);
		childSetText("LabelOwnerName","(public)"); // XUI:translate
	}
	
	//////////////////
	// ACQUIRE DATE //
	//////////////////
	time_t time_utc = (time_t)item->getCreationDate();
	if (0 == time_utc)
	{
		childSetText("LabelAcquiredDate","(unknown)");
	}
	else
	{
		childSetText("LabelAcquiredDate", ctime(&time_utc) );
	}

	///////////////////////
	// OWNER PERMISSIONS //
	///////////////////////
	if(can_agent_manipulate)
	{
		childSetText("OwnerLabel",YOU_CAN);
	}
	else
	{
		childSetText("OwnerLabel",OWNER_CAN);
	}

	U32 base_mask		= perm.getMaskBase();
	U32 owner_mask		= perm.getMaskOwner();
	U32 group_mask		= perm.getMaskGroup();
	U32 everyone_mask	= perm.getMaskEveryone();
	U32 next_owner_mask	= perm.getMaskNextOwner();

	childSetEnabled("OwnerLabel",TRUE);
	childSetEnabled("CheckOwnerModify",FALSE);
	childSetValue("CheckOwnerModify",LLSD((BOOL)(owner_mask & PERM_MODIFY)));
	childSetEnabled("CheckOwnerCopy",FALSE);
	childSetValue("CheckOwnerCopy",LLSD((BOOL)(owner_mask & PERM_COPY)));
	childSetEnabled("CheckOwnerTransfer",FALSE);
	childSetValue("CheckOwnerTransfer",LLSD((BOOL)(owner_mask & PERM_TRANSFER)));

	///////////////////////
	// DEBUG PERMISSIONS //
	///////////////////////

	if( gSavedSettings.getBOOL("DebugPermissions") )
	{
		BOOL slam_perm 			= FALSE;
		BOOL overwrite_group	= FALSE;
		BOOL overwrite_everyone	= FALSE;

		if (item->getType() == LLAssetType::AT_OBJECT)
		{
			U32 flags = item->getFlags();
			slam_perm 			= flags & LLInventoryItem::II_FLAGS_OBJECT_SLAM_PERM;
			overwrite_everyone	= flags & LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
			overwrite_group		= flags & LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
		}
		
		char perm_string[11];		/* Flawfinder: ignore */

		snprintf(perm_string, sizeof(perm_string), "B: ");			/* Flawfinder: ignore */
		mask_to_string(base_mask, perm_string+3);
		childSetText("BaseMaskDebug",perm_string);
		childSetVisible("BaseMaskDebug",TRUE);
		
		snprintf(perm_string, sizeof(perm_string), "O: ");			/* Flawfinder: ignore */
		mask_to_string(owner_mask, perm_string+3);
		childSetText("OwnerMaskDebug",perm_string);
		childSetVisible("OwnerMaskDebug",TRUE);
		
		snprintf(perm_string, sizeof(perm_string), "G%s: ", overwrite_group ? "*" : "");			/* Flawfinder: ignore */
		mask_to_string(group_mask, perm_string + (overwrite_group ? 4 : 3));
		childSetText("GroupMaskDebug",perm_string);
		childSetVisible("GroupMaskDebug",TRUE);
		
		snprintf(perm_string, sizeof(perm_string), "E%s: ", overwrite_everyone ? "*" : "");			/* Flawfinder: ignore */
		mask_to_string(everyone_mask, perm_string + (overwrite_everyone ? 4 : 3));
		childSetText("EveryoneMaskDebug",perm_string);
		childSetVisible("EveryoneMaskDebug",TRUE);
		
		snprintf(perm_string, sizeof(perm_string), "N%s: ", slam_perm ? "*" : "");			/* Flawfinder: ignore */
		mask_to_string(next_owner_mask, perm_string + (slam_perm ? 4 : 3));
		childSetText("NextMaskDebug",perm_string);
		childSetVisible("NextMaskDebug",TRUE);
	}
	else
	{
		childSetVisible("BaseMaskDebug",FALSE);
		childSetVisible("OwnerMaskDebug",FALSE);
		childSetVisible("GroupMaskDebug",FALSE);
		childSetVisible("EveryoneMaskDebug",FALSE);
		childSetVisible("NextMaskDebug",FALSE);
	}

	/////////////
	// SHARING //
	/////////////

	// Check for ability to change values.
	if (is_obj_modify && can_agent_manipulate)
	{
		childSetEnabled("CheckShareWithGroup",TRUE);
		childSetEnabled("CheckEveryoneCopy",(owner_mask & PERM_COPY) && (owner_mask & PERM_TRANSFER));
	}
	else
	{
		childSetEnabled("CheckShareWithGroup",FALSE);
		childSetEnabled("CheckEveryoneCopy",FALSE);
	}

	// Set values.
	BOOL is_group_copy = (group_mask & PERM_COPY) ? TRUE : FALSE;
	BOOL is_group_modify = (group_mask & PERM_MODIFY) ? TRUE : FALSE;
	BOOL is_group_move = (group_mask & PERM_MOVE) ? TRUE : FALSE;

	if (is_group_copy && is_group_modify && is_group_move)
	{
		childSetValue("CheckShareWithGroup",LLSD((BOOL)TRUE));

		LLCheckBoxCtrl* ctl = LLUICtrlFactory::getCheckBoxByName(this,"CheckShareWithGroup");
		if(ctl)
		{
			ctl->setTentative(FALSE);
		}
	}
	else if (!is_group_copy && !is_group_modify && !is_group_move)
	{
		childSetValue("CheckShareWithGroup",LLSD((BOOL)FALSE));
		LLCheckBoxCtrl* ctl = LLUICtrlFactory::getCheckBoxByName(this,"CheckShareWithGroup");
		if(ctl)
		{
			ctl->setTentative(FALSE);
		}
	}
	else
	{
		LLCheckBoxCtrl* ctl = LLUICtrlFactory::getCheckBoxByName(this,"CheckShareWithGroup");
		if(ctl)
		{
			ctl->setTentative(TRUE);
			ctl->set(TRUE);
		}
	}
	
	childSetValue("CheckEveryoneCopy",LLSD((BOOL)(everyone_mask & PERM_COPY)));

	///////////////
	// SALE INFO //
	///////////////

	const LLSaleInfo& sale_info = item->getSaleInfo();
	BOOL is_for_sale = sale_info.isForSale();
	// Check for ability to change values.
	if (is_obj_modify && can_agent_sell 
		&& gAgent.allowOperation(PERM_TRANSFER, perm, GP_OBJECT_MANIPULATE))
	{
		childSetEnabled("SaleLabel",is_complete);
		childSetEnabled("CheckPurchase",is_complete);

		childSetEnabled("NextOwnerLabel",TRUE);
		childSetEnabled("CheckNextOwnerModify",base_mask & PERM_MODIFY);
		childSetEnabled("CheckNextOwnerCopy",base_mask & PERM_COPY);
		childSetEnabled("CheckNextOwnerTransfer",next_owner_mask & PERM_COPY);

		childSetEnabled("RadioSaleType",is_complete && is_for_sale);
		childSetEnabled("TextPrice",is_complete && is_for_sale);
		childSetEnabled("EditPrice",is_complete && is_for_sale);
	}
	else
	{
		childSetEnabled("SaleLabel",FALSE);
		childSetEnabled("CheckPurchase",FALSE);

		childSetEnabled("NextOwnerLabel",FALSE);
		childSetEnabled("CheckNextOwnerModify",FALSE);
		childSetEnabled("CheckNextOwnerCopy",FALSE);
		childSetEnabled("CheckNextOwnerTransfer",FALSE);

		childSetEnabled("RadioSaleType",FALSE);
		childSetEnabled("TextPrice",FALSE);
		childSetEnabled("EditPrice",FALSE);
	}

	// Set values.
	childSetValue("CheckPurchase", is_for_sale);
	childSetValue("CheckNextOwnerModify",LLSD(BOOL(next_owner_mask & PERM_MODIFY)));
	childSetValue("CheckNextOwnerCopy",LLSD(BOOL(next_owner_mask & PERM_COPY)));
	childSetValue("CheckNextOwnerTransfer",LLSD(BOOL(next_owner_mask & PERM_TRANSFER)));

	LLRadioGroup* radioSaleType = LLUICtrlFactory::getRadioGroupByName(this,"RadioSaleType");
	if (is_for_sale)
	{
		radioSaleType->setSelectedIndex((S32)sale_info.getSaleType() - 1);
		char numerical_price[MAX_STRING];		/* Flawfinder: ignore */
		snprintf(numerical_price, MAX_STRING, "%d", sale_info.getSalePrice());			/* Flawfinder: ignore */
		childSetText("EditPrice",numerical_price);
	}
	else
	{
		radioSaleType->setSelectedIndex(-1);
		childSetText("EditPrice","");
	}
}

// static
void LLFloaterProperties::onClickCreator(void* data)
{
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	LLInventoryItem* item = self->findItem();
	if(!item) return;
	if(!item->getCreatorUUID().isNull())
	{
		LLFloaterAvatarInfo::showFromObject(item->getCreatorUUID());
	}
}

// static
void LLFloaterProperties::onClickOwner(void* data)
{
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	LLInventoryItem* item = self->findItem();
	if(!item) return;
	if(item->getPermissions().isGroupOwned())
	{
		LLFloaterGroupInfo::showFromUUID(item->getPermissions().getGroup());
	}
	else
	{
		if(!item->getPermissions().getOwner().isNull())
		{
			LLFloaterAvatarInfo::showFromObject(item->getPermissions().getOwner());
		}
	}
}

// static
void LLFloaterProperties::onCommitName(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLFloaterProperties::onCommitName()" << llendl;
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self)
	{
		return;
	}
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->findItem();
	if(!item)
	{
		return;
	}
	LLLineEditor* labelItemName = LLUICtrlFactory::getLineEditorByName(self,"LabelItemName");

	if(labelItemName&&
	   (item->getName() != labelItemName->getText()) && 
	   (gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE)) )
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(labelItemName->getText());
		if(self->mObjectID.isNull())
		{
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
		else
		{
			LLViewerObject* object = gObjectList.findObject(self->mObjectID);
			if(object)
			{
				object->updateInventory(
					new_item,
					TASK_INVENTORY_ITEM_KEY,
					false);
			}
		}
	}
}

// static
void LLFloaterProperties::onCommitDescription(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLFloaterProperties::onCommitDescription()" << llendl;
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->findItem();
	if(!item) return;

	LLLineEditor* labelItemDesc = LLUICtrlFactory::getLineEditorByName(self,"LabelItemDesc");
	if(!labelItemDesc)
	{
		return;
	}
	if((item->getDescription() != labelItemDesc->getText()) && 
	   (gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE)))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);

		new_item->setDescription(labelItemDesc->getText());
		if(self->mObjectID.isNull())
		{
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
		else
		{
			LLViewerObject* object = gObjectList.findObject(self->mObjectID);
			if(object)
			{
				object->updateInventory(
					new_item,
					TASK_INVENTORY_ITEM_KEY,
					false);
			}
		}
	}
}

// static
void LLFloaterProperties::onCommitPermissions(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLFloaterProperties::onCommitPermissions()" << llendl;
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->findItem();
	if(!item) return;
	LLPermissions perm(item->getPermissions());


	LLCheckBoxCtrl* CheckShareWithGroup = LLUICtrlFactory::getCheckBoxByName(self,"CheckShareWithGroup");

	if(CheckShareWithGroup)
	{
		perm.setGroupBits(gAgent.getID(), gAgent.getGroupID(),
						CheckShareWithGroup->get(),
						PERM_MODIFY | PERM_MOVE | PERM_COPY);
	}
	LLCheckBoxCtrl* CheckEveryoneCopy = LLUICtrlFactory::getCheckBoxByName(self,"CheckEveryoneCopy");
	if(CheckEveryoneCopy)
	{
		perm.setEveryoneBits(gAgent.getID(), gAgent.getGroupID(),
						 CheckEveryoneCopy->get(), PERM_COPY);
	}

	LLCheckBoxCtrl* CheckNextOwnerModify = LLUICtrlFactory::getCheckBoxByName(self,"CheckNextOwnerModify");
	if(CheckNextOwnerModify)
	{
		perm.setNextOwnerBits(gAgent.getID(), gAgent.getGroupID(),
							CheckNextOwnerModify->get(), PERM_MODIFY);
	}
	LLCheckBoxCtrl* CheckNextOwnerCopy = LLUICtrlFactory::getCheckBoxByName(self,"CheckNextOwnerCopy");
	if(CheckNextOwnerCopy)
	{
		perm.setNextOwnerBits(gAgent.getID(), gAgent.getGroupID(),
							CheckNextOwnerCopy->get(), PERM_COPY);
	}
	LLCheckBoxCtrl* CheckNextOwnerTransfer = LLUICtrlFactory::getCheckBoxByName(self,"CheckNextOwnerTransfer");
	if(CheckNextOwnerTransfer)
	{
		perm.setNextOwnerBits(gAgent.getID(), gAgent.getGroupID(),
							CheckNextOwnerTransfer->get(), PERM_TRANSFER);
	}
	if(perm != item->getPermissions()
		&& item->isComplete())
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setPermissions(perm);
		U32 flags = new_item->getFlags();
		// If next owner permissions have changed (and this is an object)
		// then set the slam permissions flag so that they are applied on rez.
		if((perm.getMaskNextOwner()!=item->getPermissions().getMaskNextOwner())
		   && (item->getType() == LLAssetType::AT_OBJECT))
		{
			flags |= LLInventoryItem::II_FLAGS_OBJECT_SLAM_PERM;
		}
		// If everyone permissions have changed (and this is an object)
		// then set the overwrite everyone permissions flag so they
		// are applied on rez.
		if ((perm.getMaskEveryone()!=item->getPermissions().getMaskEveryone())
			&& (item->getType() == LLAssetType::AT_OBJECT))
		{
			flags |= LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
		}
		// If group permissions have changed (and this is an object)
		// then set the overwrite group permissions flag so they
		// are applied on rez.
		if ((perm.getMaskGroup()!=item->getPermissions().getMaskGroup())
			&& (item->getType() == LLAssetType::AT_OBJECT))
		{
			flags |= LLInventoryItem::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
		}
		new_item->setFlags(flags);
		if(self->mObjectID.isNull())
		{
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
		else
		{
			LLViewerObject* object = gObjectList.findObject(self->mObjectID);
			if(object)
			{
				object->updateInventory(
					new_item,
					TASK_INVENTORY_ITEM_KEY,
					false);
			}
		}
	}
	else
	{
		// need to make sure we don't just follow the click
		self->refresh();
	}
}

// static
void LLFloaterProperties::onCommitSaleInfo(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLFloaterProperties::onCommitSaleInfo()" << llendl;
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	self->updateSaleInfo();
}

// static
void LLFloaterProperties::onCommitSaleType(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLFloaterProperties::onCommitSaleType()" << llendl;
	LLFloaterProperties* self = (LLFloaterProperties*)data;
	if(!self) return;
	self->updateSaleInfo();
}

void LLFloaterProperties::updateSaleInfo()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)findItem();
	if(!item) return;
	LLSaleInfo sale_info(item->getSaleInfo());
	if(!gAgent.allowOperation(PERM_TRANSFER, item->getPermissions(), GP_OBJECT_SET_SALE))
	{
		childSetValue("CheckPurchase",LLSD((BOOL)FALSE));
	}

	if((BOOL)childGetValue("CheckPurchase"))
	{
		// turn on sale info
		LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_COPY;
	
		LLRadioGroup* RadioSaleType = LLUICtrlFactory::getRadioGroupByName(this,"RadioSaleType");
		if(RadioSaleType)
		{
			switch (RadioSaleType->getSelectedIndex())
			{
			case 0:
				sale_type = LLSaleInfo::FS_ORIGINAL;
				break;
			case 1:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			case 2:
				sale_type = LLSaleInfo::FS_CONTENTS;
				break;
			default:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			}
		}

		if (sale_type == LLSaleInfo::FS_COPY 
			&& !gAgent.allowOperation(PERM_COPY, item->getPermissions(), 
									  GP_OBJECT_SET_SALE))
		{
			sale_type = LLSaleInfo::FS_ORIGINAL;
		}

		LLLineEditor* EditPrice = LLUICtrlFactory::getLineEditorByName(this,"EditPrice");
		
		S32 price = -1;
		if(EditPrice)
		{
			price = atoi(EditPrice->getText().c_str());
		}
		// Invalid data - turn off the sale
		if (price < 0)
		{
			sale_type = LLSaleInfo::FS_NOT;
			price = 0;
		}

		sale_info.setSaleType(sale_type);
		sale_info.setSalePrice(price);
	}
	else
	{
		sale_info.setSaleType(LLSaleInfo::FS_NOT);
	}
	if(sale_info != item->getSaleInfo()
		&& item->isComplete())
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setSaleInfo(sale_info);
		if(mObjectID.isNull())
		{
			// This is in the agent's inventory.
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
		else
		{
			// This is in an object's contents.
			LLViewerObject* object = gObjectList.findObject(mObjectID);
			if(object)
			{
				object->updateInventory(
					new_item,
					TASK_INVENTORY_ITEM_KEY,
					false);
			}
		}
	}
	else
	{
		// need to make sure we don't just follow the click
		refresh();
	}
}

LLInventoryItem* LLFloaterProperties::findItem() const
{
	LLInventoryItem* item = NULL;
	if(mObjectID.isNull())
	{
		// it is in agent inventory
		item = gInventory.getItem(mItemID);
	}
	else
	{
		LLViewerObject* object = gObjectList.findObject(mObjectID);
		if(object)
		{
			item = (LLInventoryItem*)object->getInventoryObject(mItemID);
		}
	}
	return item;
}

void LLFloaterProperties::closeByID(const LLUUID& item_id, const LLUUID &object_id)
{
	LLFloaterProperties* floaterp = find(item_id, object_id);

	if (floaterp)
	{
		floaterp->close();
	}
}

///----------------------------------------------------------------------------
/// LLMultiProperties
///----------------------------------------------------------------------------

LLMultiProperties::LLMultiProperties(const LLRect &rect) : LLMultiFloater("Properties", rect)
{
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
