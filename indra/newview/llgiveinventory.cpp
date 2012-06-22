/**
 * @file llgiveinventory.cpp
 * @brief LLGiveInventory class implementation
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
#include "llgiveinventory.h"

// library includes
#include "llnotificationsutil.h"
#include "lltrans.h"

// newview includes
#include "llagent.h"
#include "llagentdata.h"
#include "llagentui.h"
#include "llagentwearables.h"
#include "llfloatertools.h" // for gFloaterTool
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimview.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llmutelist.h"
#include "llrecentpeople.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"

// MAX ITEMS is based on (sizeof(uuid)+2) * count must be < MTUBYTES
// or 18 * count < 1200 => count < 1200/18 => 66. I've cut it down a
// bit from there to give some pad.
const S32 MAX_ITEMS = 42;

class LLGiveable : public LLInventoryCollectFunctor
{
public:
	LLGiveable() : mCountLosing(0) {}
	virtual ~LLGiveable() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);

	S32 countNoCopy() const { return mCountLosing; }
protected:
	S32 mCountLosing;
};

bool LLGiveable::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	// All categories can be given.
	if (cat)
		return true;

	bool allowed = false;
	if (item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if (allowed &&
		   !item->getPermissions().allowOperationBy(PERM_TRANSFER,
							    gAgent.getID()))
		{
			allowed = FALSE;
		}
		if (allowed &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return allowed;
}

class LLUncopyableItems : public LLInventoryCollectFunctor
{
public:
	LLUncopyableItems() {}
	virtual ~LLUncopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};

bool LLUncopyableItems::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	bool uncopyable = false;
	if (item)
	{
		if (itemTransferCommonlyAllowed(item) &&
			!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			uncopyable = true;
		}
	}
	return uncopyable;
}

// static
bool LLGiveInventory::isInventoryGiveAcceptable(const LLInventoryItem* item)
{
	if (!item) return false;

	if (!isAgentAvatarValid()) return false;

	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID))
	{
		return false;
	}

	bool acceptable = true;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		if (get_is_item_worn(item->getUUID()))
		{
			acceptable = false;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		{
			BOOL copyable = false;
			if (item->getPermissions().allowCopyBy(gAgentID)) copyable = true;

			if (!copyable && get_is_item_worn(item->getUUID()))
			{
				acceptable = false;
			}
		}
		break;
	default:
		break;
	}
	return acceptable;
}

// static
bool LLGiveInventory::isInventoryGroupGiveAcceptable(const LLInventoryItem* item)
{
	if (!item) return false;

	if (!isAgentAvatarValid()) return false;

	// These permissions are double checked in the simulator in
	// LLGroupNoticeInventoryItemFetch::result().
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID))
	{
		return false;
	}
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return false;
	}


	bool acceptable = true;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		if (gAgentAvatarp->isWearingAttachment(item->getUUID()))
		{
			acceptable = false;
		}
		break;
	default:
		break;
	}
	return acceptable;
}

// static
bool LLGiveInventory::doGiveInventoryItem(const LLUUID& to_agent,
									  const LLInventoryItem* item,
									  const LLUUID& im_session_id/* = LLUUID::null*/)

{
	bool res = true;
	llinfos << "LLGiveInventory::giveInventory()" << llendl;
	if (!isInventoryGiveAcceptable(item))
	{
		return false;
	}
	if (item->getPermissions().allowCopyBy(gAgentID))
	{
		// just give it away.
		LLGiveInventory::commitGiveInventoryItem(to_agent, item, im_session_id);
	}
	else
	{
		// ask if the agent is sure.
		LLSD substitutions;
		substitutions["ITEMS"] = item->getName();
		LLSD payload;
		payload["agent_id"] = to_agent;
		LLSD items = LLSD::emptyArray();
		items.append(item->getUUID());
		payload["items"] = items;
		LLNotificationsUtil::add("CannotCopyWarning", substitutions, payload,
			&LLGiveInventory::handleCopyProtectedItem);
		res = false;
	}

	return res;
}

bool LLGiveInventory::doGiveInventoryCategory(const LLUUID& to_agent,
											  const LLInventoryCategory* cat,
											  const LLUUID& im_session_id,
											  const std::string& notification_name)

{
	if (!cat)
	{
		return false;
	}
	llinfos << "LLGiveInventory::giveInventoryCategory() - "
		<< cat->getUUID() << llendl;

	if (!isAgentAvatarValid())
	{
		return false;
	}

	bool give_successful = true;
	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf (cat->getUUID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		giveable);
	S32 count = cats.count();
	bool complete = true;
	for(S32 i = 0; i < count; ++i)
	{
		if (!gInventory.isCategoryComplete(cats.get(i)->getUUID()))
		{
			complete = false;
			break;
		}
	}
	if (!complete)
	{
		LLNotificationsUtil::add("IncompleteInventory");
		give_successful = false;
	}
	count = items.count() + cats.count();
	if (count > MAX_ITEMS)
	{
		LLNotificationsUtil::add("TooManyItems");
		give_successful = false;
	}
	else if (count == 0)
	{
		LLNotificationsUtil::add("NoItems");
		give_successful = false;
	}
	else if (give_successful)
	{
		if (0 == giveable.countNoCopy())
		{
			give_successful = LLGiveInventory::commitGiveInventoryCategory(to_agent, cat, im_session_id);
		}
		else
		{
			LLSD args;
			args["COUNT"] = llformat("%d",giveable.countNoCopy());
			LLSD payload;
			payload["agent_id"] = to_agent;
			payload["folder_id"] = cat->getUUID();
			if (!notification_name.empty())
			{
				payload["success_notification"] = notification_name;
			}
			LLNotificationsUtil::add("CannotCopyCountItems", args, payload, &LLGiveInventory::handleCopyProtectedCategory);
			give_successful = false;
		}
	}

	return give_successful;
}

//////////////////////////////////////////////////////////////////////////
//     PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//static
void LLGiveInventory::logInventoryOffer(const LLUUID& to_agent, const LLUUID &im_session_id)
{
	// compute id of possible IM session with agent that has "to_agent" id
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, to_agent);
	// If this item was given by drag-and-drop into an IM panel, log this action in the IM panel chat.
	LLSD args;
	args["user_id"] = to_agent;
	if (im_session_id.notNull())
	{
		gIMMgr->addSystemMessage(im_session_id, "inventory_item_offered", args);
	}
	// If this item was given by drag-and-drop on avatar while IM panel was open, log this action in the IM panel chat.
	else if (LLIMModel::getInstance()->findIMSession(session_id))
	{
		gIMMgr->addSystemMessage(session_id, "inventory_item_offered", args);
	}
	// If this item was given by drag-and-drop on avatar while IM panel wasn't open, log this action to IM history.
	else
	{
		std::string full_name;
		if (gCacheName->getFullName(to_agent, full_name))
		{
			// Build a new format username or firstname_lastname for legacy names
			// to use it for a history log filename.
			full_name = LLCacheName::buildUsername(full_name);
			LLIMModel::instance().logToFile(full_name, LLTrans::getString("SECOND_LIFE"), im_session_id, LLTrans::getString("inventory_item_offered-im"));
		}
	}
}

// static
bool LLGiveInventory::handleCopyProtectedItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLSD itmes = notification["payload"]["items"];
	LLInventoryItem* item = NULL;
	bool give_successful = true;
	switch(option)
	{
	case 0:  // "Yes"
		for (LLSD::array_iterator it = itmes.beginArray(); it != itmes.endArray(); it++)
		{
			item = gInventory.getItem((*it).asUUID());
			if (item)
			{
				LLGiveInventory::commitGiveInventoryItem(notification["payload"]["agent_id"].asUUID(),
					item);
				// delete it for now - it will be deleted on the server
				// quickly enough.
				gInventory.deleteObject(item->getUUID());
				gInventory.notifyObservers();
			}
			else
			{
				LLNotificationsUtil::add("CannotGiveItem");
				give_successful = false;
			}
		}
		if (give_successful && notification["payload"]["success_notification"].isDefined())
		{
			LLNotificationsUtil::add(notification["payload"]["success_notification"].asString());
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		LLNotificationsUtil::add("TransactionCancelled");
		give_successful = false;
		break;
	}
	return give_successful;
}

// static
void LLGiveInventory::commitGiveInventoryItem(const LLUUID& to_agent,
												const LLInventoryItem* item,
												const LLUUID& im_session_id)
{
	if (!item) return;
	std::string name;
	LLAgentUI::buildFullname(name);
	LLUUID transaction_id;
	transaction_id.generate();
	const S32 BUCKET_SIZE = sizeof(U8) + UUID_BYTES;
	U8 bucket[BUCKET_SIZE];
	bucket[0] = (U8)item->getType();
	memcpy(&bucket[1], &(item->getUUID().mData), UUID_BYTES);		/* Flawfinder: ignore */
	pack_instant_message(
		gMessageSystem,
		gAgentID,
		FALSE,
		gAgentSessionID,
		to_agent,
		name,
		item->getName(),
		IM_ONLINE,
		IM_INVENTORY_OFFERED,
		transaction_id,
		0,
		LLUUID::null,
		gAgent.getPositionAgent(),
		NO_TIMESTAMP,
		bucket,
		BUCKET_SIZE);
	gAgent.sendReliableMessage();

	// VEFFECT: giveInventory
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgentAvatarp);
	effectp->setTargetObject(gObjectList.findObject(to_agent));
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();

	LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);

	logInventoryOffer(to_agent, im_session_id);

	// add buddy to recent people list
	LLRecentPeople::instance().add(to_agent);
}

// static
bool LLGiveInventory::handleCopyProtectedCategory(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLInventoryCategory* cat = NULL;
	bool give_successful = true;
	switch(option)
	{
	case 0:  // "Yes"
		cat = gInventory.getCategory(notification["payload"]["folder_id"].asUUID());
		if (cat)
		{
			give_successful = LLGiveInventory::commitGiveInventoryCategory(notification["payload"]["agent_id"].asUUID(),
				cat);
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLUncopyableItems remove;
			gInventory.collectDescendentsIf (cat->getUUID(),
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				remove);
			S32 count = items.count();
			for(S32 i = 0; i < count; ++i)
			{
				gInventory.deleteObject(items.get(i)->getUUID());
			}
			gInventory.notifyObservers();

			if (give_successful && notification["payload"]["success_notification"].isDefined())
			{
				LLNotificationsUtil::add(notification["payload"]["success_notification"].asString());
			}
		}
		else
		{
			LLNotificationsUtil::add("CannotGiveCategory");
			give_successful = false;
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		LLNotificationsUtil::add("TransactionCancelled");
		give_successful = false;
		break;
	}
	return give_successful;
}

// static
bool LLGiveInventory::commitGiveInventoryCategory(const LLUUID& to_agent,
													const LLInventoryCategory* cat,
													const LLUUID& im_session_id)

{
	if (!cat)
	{
		return false;
	}
	llinfos << "LLGiveInventory::commitGiveInventoryCategory() - "
		<< cat->getUUID() << llendl;

	// add buddy to recent people list
	LLRecentPeople::instance().add(to_agent);

	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf (cat->getUUID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		giveable);

	bool give_successful = true;
	// MAX ITEMS is based on (sizeof(uuid)+2) * count must be <
	// MTUBYTES or 18 * count < 1200 => count < 1200/18 =>
	// 66. I've cut it down a bit from there to give some pad.
	S32 count = items.count() + cats.count();
	if (count > MAX_ITEMS)
	{
		LLNotificationsUtil::add("TooManyItems");
		give_successful = false;
	}
	else if (count == 0)
	{
		LLNotificationsUtil::add("NoItems");
		give_successful = false;
	}
	else
	{
		std::string name;
		LLAgentUI::buildFullname(name);
		LLUUID transaction_id;
		transaction_id.generate();
		S32 bucket_size = (sizeof(U8) + UUID_BYTES) * (count + 1);
		U8* bucket = new U8[bucket_size];
		U8* pos = bucket;
		U8 type = (U8)cat->getType();
		memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
		pos += sizeof(U8);
		memcpy(pos, &(cat->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
		pos += UUID_BYTES;
		S32 i;
		count = cats.count();
		for(i = 0; i < count; ++i)
		{
			memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
			pos += sizeof(U8);
			memcpy(pos, &(cats.get(i)->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
			pos += UUID_BYTES;
		}
		count = items.count();
		for(i = 0; i < count; ++i)
		{
			type = (U8)items.get(i)->getType();
			memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
			pos += sizeof(U8);
			memcpy(pos, &(items.get(i)->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
			pos += UUID_BYTES;
		}
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			to_agent,
			name,
			cat->getName(),
			IM_ONLINE,
			IM_INVENTORY_OFFERED,
			transaction_id,
			0,
			LLUUID::null,
			gAgent.getPositionAgent(),
			NO_TIMESTAMP,
			bucket,
			bucket_size);
		gAgent.sendReliableMessage();
		delete[] bucket;

		// VEFFECT: giveInventoryCategory
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(gObjectList.findObject(to_agent));
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		gFloaterTools->dirty();

		LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);

		logInventoryOffer(to_agent, im_session_id);
	}

	return give_successful;
}

// EOF
