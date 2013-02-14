/** 
 * @file llfloaterbulkpermissions.cpp
 * @author Michelle2 Zenovka
 * @brief A floater which allows task inventory item's properties to be changed on mass.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "llfloaterbulkpermission.h"
#include "llfloaterperms.h" // for utilities
#include "llagent.h"
#include "llchat.h"
#include "llinventorydefines.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "lscript_rt_interface.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "llbutton.h"
#include "lldir.h"
#include "llviewerstats.h"
#include "lluictrlfactory.h"
#include "llselectmgr.h"
#include "llcheckboxctrl.h"

#include "roles_constants.h" // for GP_OBJECT_MANIPULATE


LLFloaterBulkPermission::LLFloaterBulkPermission(const LLSD& seed) 
:	LLFloater(seed),
	mDone(FALSE)
{
	mID.generate();
	mCommitCallbackRegistrar.add("BulkPermission.Apply",	boost::bind(&LLFloaterBulkPermission::onApplyBtn, this));
	mCommitCallbackRegistrar.add("BulkPermission.Close",	boost::bind(&LLFloaterBulkPermission::onCloseBtn, this));
	mCommitCallbackRegistrar.add("BulkPermission.CheckAll",	boost::bind(&LLFloaterBulkPermission::onCheckAll, this));
	mCommitCallbackRegistrar.add("BulkPermission.UncheckAll",	boost::bind(&LLFloaterBulkPermission::onUncheckAll, this));
	mCommitCallbackRegistrar.add("BulkPermission.CommitCopy",	boost::bind(&LLFloaterBulkPermission::onCommitCopy, this));
}

BOOL LLFloaterBulkPermission::postBuild()
{
	return TRUE;
}

void LLFloaterBulkPermission::doApply()
{
	// Inspects a stream of selected object contents and adds modifiable ones to the given array.
	class ModifiableGatherer : public LLSelectedNodeFunctor
	{
	public:
		ModifiableGatherer(LLDynamicArray<LLUUID>& q) : mQueue(q) {}
		virtual bool apply(LLSelectNode* node)
		{
			if( node->allowOperationOnNode(PERM_MODIFY, GP_OBJECT_MANIPULATE) )
			{
				mQueue.put(node->getObject()->getID());
			}
			return true;
		}
	private:
		LLDynamicArray<LLUUID>& mQueue;
	};
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
	list->deleteAllItems();
	ModifiableGatherer gatherer(mObjectIDs);
	LLSelectMgr::getInstance()->getSelection()->applyToNodes(&gatherer);
	if(mObjectIDs.empty())
	{
		list->setCommentText(getString("nothing_to_modify_text"));
	}
	else
	{
		mDone = FALSE;
		if (!start())
		{
			llwarns << "Unexpected bulk permission change failure." << llendl;
		}
	}
}


// This is the callback method for the viewer object currently being
// worked on.
// NOT static, virtual!
void LLFloaterBulkPermission::inventoryChanged(LLViewerObject* viewer_object,
											 LLInventoryObject::object_list_t* inv,
											 S32,
											 void* q_id)
{
	//llinfos << "changed object: " << viewer_object->getID() << llendl;

	//Remove this listener from the object since its
	//listener callback is now being executed.
	
	//We remove the listener here because the function
	//removeVOInventoryListener removes the listener from a ViewerObject
	//which it internally stores.
	
	//If we call this further down in the function, calls to handleInventory
	//and nextObject may update the interally stored viewer object causing
	//the removal of the incorrect listener from an incorrect object.
	
	//Fixes SL-6119:Recompile scripts fails to complete
	removeVOInventoryListener();

	if (viewer_object && inv && (viewer_object->getID() == mCurrentObjectID) )
	{
		handleInventory(viewer_object, inv);
	}
	else
	{
		// something went wrong...
		// note that we're not working on this one, and move onto the
		// next object in the list.
		llwarns << "No inventory for " << mCurrentObjectID << llendl;
		nextObject();
	}
}

void LLFloaterBulkPermission::onApplyBtn()
{
	doApply();
}

void LLFloaterBulkPermission::onCloseBtn()
{
	closeFloater();
}

//static 
void LLFloaterBulkPermission::onCommitCopy()
{
	// Implements fair use
	BOOL copyable = gSavedSettings.getBOOL("BulkChangeNextOwnerCopy");
	if(!copyable)
	{
		gSavedSettings.setBOOL("BulkChangeNextOwnerTransfer", TRUE);
	}
	LLCheckBoxCtrl* xfer =getChild<LLCheckBoxCtrl>("next_owner_transfer");
	xfer->setEnabled(copyable);
}

BOOL LLFloaterBulkPermission::start()
{
	// note: number of top-level objects to modify is mObjectIDs.count().
	getChild<LLScrollListCtrl>("queue output")->setCommentText(getString("start_text"));
	return nextObject();
}

// Go to the next object and start if found. Returns false if no objects left, true otherwise.
BOOL LLFloaterBulkPermission::nextObject()
{
	S32 count;
	BOOL successful_start = FALSE;
	do
	{
		count = mObjectIDs.count();
		//llinfos << "Objects left to process = " << count << llendl;
		mCurrentObjectID.setNull();
		if(count > 0)
		{
			successful_start = popNext();
			//llinfos << (successful_start ? "successful" : "unsuccessful") << llendl; 
		}
	} while((mObjectIDs.count() > 0) && !successful_start);

	if(isDone() && !mDone)
	{
		getChild<LLScrollListCtrl>("queue output")->setCommentText(getString("done_text"));
		mDone = TRUE;
	}
	return successful_start;
}

// Pop the top object off of the queue.
// Return TRUE if the queue has started, otherwise FALSE.
BOOL LLFloaterBulkPermission::popNext()
{
	// get the head element from the container, and attempt to get its inventory.
	BOOL rv = FALSE;
	S32 count = mObjectIDs.count();
	if(mCurrentObjectID.isNull() && (count > 0))
	{
		mCurrentObjectID = mObjectIDs.get(0);
		//llinfos << "mCurrentID: " << mCurrentObjectID << llendl;
		mObjectIDs.remove(0);
		LLViewerObject* obj = gObjectList.findObject(mCurrentObjectID);
		if(obj)
		{
			//llinfos << "requesting inv for " << mCurrentObjectID << llendl;
			LLUUID* id = new LLUUID(mID);
			registerVOInventoryListener(obj,id);
			requestVOInventory();
			rv = TRUE;
		}
		else
		{
			llinfos<<"NULL LLViewerObject" <<llendl;
		}
	}

	return rv;
}


void LLFloaterBulkPermission::doCheckUncheckAll(BOOL check)
{
	gSavedSettings.setBOOL("BulkChangeIncludeAnimations", check);
	gSavedSettings.setBOOL("BulkChangeIncludeBodyParts" , check);
	gSavedSettings.setBOOL("BulkChangeIncludeClothing"  , check);
	gSavedSettings.setBOOL("BulkChangeIncludeGestures"  , check);
	gSavedSettings.setBOOL("BulkChangeIncludeNotecards" , check);
	gSavedSettings.setBOOL("BulkChangeIncludeObjects"   , check);
	gSavedSettings.setBOOL("BulkChangeIncludeScripts"   , check);
	gSavedSettings.setBOOL("BulkChangeIncludeSounds"    , check);
	gSavedSettings.setBOOL("BulkChangeIncludeTextures"  , check);
}


void LLFloaterBulkPermission::handleInventory(LLViewerObject* viewer_obj, LLInventoryObject::object_list_t* inv)
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");

	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		LLAssetType::EType asstype = (*it)->getType();
		if(
			( asstype == LLAssetType::AT_ANIMATION && gSavedSettings.getBOOL("BulkChangeIncludeAnimations")) ||
			( asstype == LLAssetType::AT_BODYPART  && gSavedSettings.getBOOL("BulkChangeIncludeBodyParts" )) ||
			( asstype == LLAssetType::AT_CLOTHING  && gSavedSettings.getBOOL("BulkChangeIncludeClothing"  )) ||
			( asstype == LLAssetType::AT_GESTURE   && gSavedSettings.getBOOL("BulkChangeIncludeGestures"  )) ||
			( asstype == LLAssetType::AT_NOTECARD  && gSavedSettings.getBOOL("BulkChangeIncludeNotecards" )) ||
			( asstype == LLAssetType::AT_OBJECT    && gSavedSettings.getBOOL("BulkChangeIncludeObjects"   )) ||
			( asstype == LLAssetType::AT_LSL_TEXT  && gSavedSettings.getBOOL("BulkChangeIncludeScripts"   )) ||
			( asstype == LLAssetType::AT_SOUND     && gSavedSettings.getBOOL("BulkChangeIncludeSounds"    )) ||
			( asstype == LLAssetType::AT_TEXTURE   && gSavedSettings.getBOOL("BulkChangeIncludeTextures"  )))
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLViewerInventoryItem* new_item = (LLViewerInventoryItem*)item;
				LLPermissions perm(new_item->getPermissions());

				// chomp the inventory name so it fits in the scroll window nicely
				// and the user can see the [OK]
				std::string invname;
				invname=item->getName().substr(0,item->getName().size() < 30 ? item->getName().size() : 30 );
				
				LLUIString status_text = getString("status_text");
				status_text.setArg("[NAME]", invname.c_str());
				// Check whether we appear to have the appropriate permissions to change permission on this item.
				// Although the server will disallow any forbidden changes, it is a good idea to guess correctly
				// so that we can warn the user. The risk of getting this check wrong is therefore the possibility
				// of incorrectly choosing to not attempt to make a valid change.
				//
				// Trouble is this is extremely difficult to do and even when we know the results
				// it is difficult to design the best messaging. Therefore in this initial implementation
				// we'll always try to set the requested permissions and consider all cases successful
				// and perhaps later try to implement a smarter, friendlier solution. -MG
				if(true
					//gAgent.allowOperation(PERM_MODIFY, perm, GP_OBJECT_MANIPULATE) // for group and everyone masks
					//|| something else // for next owner perms
					)
				{
					perm.setMaskNext(LLFloaterPerms::getNextOwnerPerms("BulkChange"));
					perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms("BulkChange"));
					perm.setMaskGroup(LLFloaterPerms::getGroupPerms("BulkChange"));
					new_item->setPermissions(perm); // here's the beef
					updateInventory(object,new_item,TASK_INVENTORY_ITEM_KEY,FALSE);
					//status_text.setArg("[STATUS]", getString("status_ok_text"));
					status_text.setArg("[STATUS]", "");
				}
				else
				{
					//status_text.setArg("[STATUS]", getString("status_bad_text"));
					status_text.setArg("[STATUS]", "");
				}
				
				list->setCommentText(status_text.getString());

				//TODO if we are an object inside an object we should check a recuse flag and if set
				//open the inventory of the object and recurse - Michelle2 Zenovka

				//	if(recurse &&  ( (*it)->getType() == LLAssetType::AT_OBJECT && processObject))
				//	{
				//		I think we need to get the UUID of the object inside the inventory
				//		call item->fetchFromServer();
				//		we need a call back to say item has arrived *sigh*
				//		we then need to do something like
				//		LLUUID* id = new LLUUID(mID);
				//		registerVOInventoryListener(obj,id);
				//		requestVOInventory();
				//	}
			}
		}
	}

	nextObject();
}


// Avoid inventory callbacks etc by just fire and forgetting the message with the permissions update
// we could do this via LLViewerObject::updateInventory but that uses inventory call backs and buggers
// us up and we would have a dodgy item iterator

void LLFloaterBulkPermission::updateInventory(LLViewerObject* object, LLViewerInventoryItem* item, U8 key, bool is_new)
{
	// This slices the object into what we're concerned about on the viewer. 
	// The simulator will take the permissions and transfer ownership.
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(),
								  item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_UpdateData);
	msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
	msg->addU8Fast(_PREHASH_Key, key);
	msg->nextBlockFast(_PREHASH_InventoryData);
	task_item->packMessage(msg);
	msg->sendReliable(object->getRegion()->getHost());
}

