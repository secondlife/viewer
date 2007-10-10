/** 
 * @file llcompilequeue.cpp
 * @brief LLCompileQueueData class implementation
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

/**
 *
 * Implementation of the script queue which keeps an array of object
 * UUIDs and manipulates all of the scripts on each of them. 
 *
 */


#include "llviewerprecompiledheaders.h"

#include "llcompilequeue.h"

#include "llagent.h"
#include "llchat.h"
#include "llviewerwindow.h"
#include "llcallbacklist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "lscript_rt_interface.h"
#include "llviewercontrol.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llresmgr.h"
#include "llbutton.h"
#include "lldir.h"
#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "llvieweruictrlfactory.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

// *TODO:Translate
const char* COMPILE_QUEUE_TITLE = "Recompilation Progress";
const char* COMPILE_START_STRING = "recompile";
const char* RESET_QUEUE_TITLE = "Reset Progress";
const char* RESET_START_STRING = "reset";
const char* RUN_QUEUE_TITLE = "Set Running Progress";
const char* RUN_START_STRING = "set running";
const char* NOT_RUN_QUEUE_TITLE = "Set Not Running Progress";
const char* NOT_RUN_START_STRING = "set not running";

struct LLCompileQueueData
{
	LLUUID mQueueID;
	LLUUID mOldAssetID;
	LLCompileQueueData(const LLUUID& q_id, const LLUUID& old_asset_id) :
		mQueueID(q_id), mOldAssetID(old_asset_id) {}
};

struct LLScriptQueueData
{
	LLUUID mQueueID;
	LLString mScriptName;
	LLScriptQueueData(const LLUUID& q_id, const char* name) :
		mQueueID(q_id), mScriptName(name) {}
};

///----------------------------------------------------------------------------
/// Class LLFloaterScriptQueue
///----------------------------------------------------------------------------

// static
LLMap<LLUUID, LLFloaterScriptQueue*> LLFloaterScriptQueue::sInstances;


// Default constructor
LLFloaterScriptQueue::LLFloaterScriptQueue(const std::string& name,
											 const LLRect& rect,
											 const char* title,
											 const char* start_string) :
	LLFloater(name, rect, title,
			  RESIZE_YES, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT,
			  DRAG_ON_TOP, MINIMIZE_YES, CLOSE_YES)
{

	gUICtrlFactory->buildFloater(this,"floater_script_queue.xml");

	childSetAction("close",onCloseBtn,this);
	childSetEnabled("close",FALSE);

	setTitle(title);
	
	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}
	
	mStartString = start_string;
	mDone = FALSE;
	sInstances.addData(mID, this);
}

// Destroys the object
LLFloaterScriptQueue::~LLFloaterScriptQueue()
{
	sInstances.removeData(mID);
}

// find an instance by ID. Return NULL if it does not exist.
// static
LLFloaterScriptQueue* LLFloaterScriptQueue::findInstance(const LLUUID& id)
{
	if(sInstances.checkData(id))
	{
		return sInstances.getData(id);
	}
	return NULL;
}


// This is the callback method for the viewer object currently being
// worked on.
// NOT static, virtual!
void LLFloaterScriptQueue::inventoryChanged(LLViewerObject* viewer_object,
											 InventoryObjectList* inv,
											 S32,
											 void* q_id)
{
	llinfos << "LLFloaterScriptQueue::inventoryChanged() for  object "
			<< viewer_object->getID() << llendl;

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
		llwarns << "No inventory for " << mCurrentObjectID
				<< llendl;
		nextObject();
	}
}


// static
void LLFloaterScriptQueue::onCloseBtn(void* user_data)
{
	LLFloaterScriptQueue* self = (LLFloaterScriptQueue*)user_data;
	self->close();
}

void LLFloaterScriptQueue::addObject(const LLUUID& id)
{
	mObjectIDs.put(id);
}

BOOL LLFloaterScriptQueue::start()
{
	//llinfos << "LLFloaterCompileQueue::start()" << llendl;
	char buffer[MAX_STRING]; 				/*Flawfinder: ignore*/
	snprintf(buffer, sizeof(buffer), "Starting %s of %d items.", mStartString, mObjectIDs.count()); 		/* Flawfinder: ignore */
	
	LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "queue output");
	list->addSimpleItem(buffer);

	return nextObject();
}

BOOL LLFloaterScriptQueue::isDone() const
{
	return (mCurrentObjectID.isNull() && (mObjectIDs.count() == 0));
}

// go to the next object. If no objects left, it falls out silently
// and waits to be killed by the window being closed.
BOOL LLFloaterScriptQueue::nextObject()
{
	S32 count;
	BOOL successful_start = FALSE;
	do
	{
		count = mObjectIDs.count();
		llinfos << "LLFloaterScriptQueue::nextObject() - " << count
				<< " objects left to process." << llendl;
		mCurrentObjectID.setNull();
		if(count > 0)
		{
			successful_start = popNext();
		}
		llinfos << "LLFloaterScriptQueue::nextObject() "
				<< (successful_start ? "successful" : "unsuccessful")
				<< llendl; 
	} while((mObjectIDs.count() > 0) && !successful_start);
	if(isDone() && !mDone)
	{
		
		LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "queue output");

		mDone = TRUE;
		char buffer[MAX_STRING];		/*Flawfinder: ignore*/
		snprintf(buffer, sizeof(buffer), "Done.");				/* Flawfinder: ignore */
		list->addSimpleItem(buffer);
		childSetEnabled("close",TRUE);
	}
	return successful_start;
}

// returns true if the queue has started, otherwise false.  This
// method pops the top object off of the queue.
BOOL LLFloaterScriptQueue::popNext()
{
	// get the first element off of the container, and attempt to get
	// the inventory.
	BOOL rv = FALSE;
	S32 count = mObjectIDs.count();
	if(mCurrentObjectID.isNull() && (count > 0))
	{
		mCurrentObjectID = mObjectIDs.get(0);
		llinfos << "LLFloaterScriptQueue::popNext() - mCurrentID: "
				<< mCurrentObjectID << llendl;
		mObjectIDs.remove(0);
		LLViewerObject* obj = gObjectList.findObject(mCurrentObjectID);
		if(obj)
		{
			llinfos << "LLFloaterScriptQueue::popNext() requesting inv for "
					<< mCurrentObjectID << llendl;
			LLUUID* id = new LLUUID(mID);
			registerVOInventoryListener(obj,id);
			requestVOInventory();
			rv = TRUE;
		}
	}
	return rv;
}


///----------------------------------------------------------------------------
/// Class LLFloaterCompileQueue
///----------------------------------------------------------------------------

// static
LLFloaterCompileQueue* LLFloaterCompileQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterCompileQueue* new_queue = new LLFloaterCompileQueue("queue",
																 rect);
	new_queue->open(); 	 	/*Flawfinder: ignore*/ 
	return new_queue;
}

LLFloaterCompileQueue::LLFloaterCompileQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, COMPILE_QUEUE_TITLE, COMPILE_START_STRING)
{ }

LLFloaterCompileQueue::~LLFloaterCompileQueue()
{ 
}

void LLFloaterCompileQueue::handleInventory(LLViewerObject *viewer_object,
											InventoryObjectList* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.

	typedef std::map<LLUUID, LLPointer<LLInventoryItem> > uuid_item_map;
	uuid_item_map asset_item_map;

	InventoryObjectList::const_iterator it = inv->begin();
	InventoryObjectList::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
			// Check permissions before allowing the user to retrieve data.
			if (item->getPermissions().allowModifyBy(gAgent.getID())  &&
				item->getPermissions().allowCopyBy(gAgent.getID()) )
			{
				LLPointer<LLViewerInventoryItem> script = new LLViewerInventoryItem(item);
				mCurrentScripts.put(script);

				if (!asset_item_map.count(item->getAssetUUID()))
				{
					// No entry, put in an entry for this supposedly permissive script
					asset_item_map[item->getAssetUUID()] = item;
				}
			}
		}
	}

	if (asset_item_map.empty())
	{
		// There are no scripts in this object.  move on.
		nextObject();
	}
	else
	{
		// request all of the assets.
		uuid_item_map::iterator iter;
		for(iter = asset_item_map.begin(); iter != asset_item_map.end(); iter++)
		{
			LLInventoryItem *itemp = iter->second;
			LLScriptQueueData* datap = new LLScriptQueueData(getID(), itemp->getName().c_str());

			//llinfos << "ITEM NAME 2: " << names.get(i) << llendl;
			gAssetStorage->getInvItemAsset(viewer_object->getRegion()->getHost(),
				gAgent.getID(),
				gAgent.getSessionID(),
				itemp->getPermissions().getOwner(),
				viewer_object->getID(),
				itemp->getUUID(),
				itemp->getAssetUUID(),
				itemp->getType(),
				LLFloaterCompileQueue::scriptArrived,
				(void*)datap);
		}
	}
}


// This is the callback for when each script arrives
// static
void LLFloaterCompileQueue::scriptArrived(LLVFS *vfs, const LLUUID& asset_id,
										  LLAssetType::EType type,
										  void* user_data, S32 status, LLExtStat ext_status)
{
	llinfos << "LLFloaterCompileQueue::scriptArrived()" << llendl;
	LLScriptQueueData* data = (LLScriptQueueData*)user_data;
	if(!data) return;
	LLFloaterCompileQueue* queue = static_cast<LLFloaterCompileQueue*> 
				(LLFloaterScriptQueue::findInstance(data->mQueueID));
	char buffer[MAX_STRING];		/*Flawfinder: ignore*/	
	buffer[0] = '\0';
	if(queue && (0 == status))
	{
		//llinfos << "ITEM NAME 3: " << data->mScriptName << llendl;

		// Dump this into a file on the local disk so we can compile it.
		char filename[LL_MAX_PATH] = "";		/*Flawfinder: ignore*/
		LLVFile file(vfs, asset_id, type);
		char uuid_str[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
		asset_id.toString(uuid_str);
		snprintf(filename, sizeof(filename), "%s.%s",gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str).c_str(),LLAssetType::lookup(type)); 	/* Flawfinder: ignore */

		FILE *fp = LLFile::fopen(filename, "wb");	 /*Flawfinder: ignore*/
		if (fp)
		{
			const S32 buf_size = 65536;
			U8 copy_buf[buf_size];
			while (file.read(copy_buf, buf_size)) 	 /*Flawfinder: ignore*/
			{
				if (fwrite(copy_buf, file.getLastBytesRead(), 1, fp) < 1)
				{
					// return a bad file error if we can't write the whole thing
					status = LL_ERR_CANNOT_OPEN_FILE;
				}
			}

			fclose(fp);
		}

		// It's now in the file, now compile it.
		snprintf(buffer, sizeof(buffer), "Downloaded, now compiling '%s'.", data->mScriptName.c_str());  	/* Flawfinder: ignore */
		queue->compile(filename, asset_id);

		// Delete it after we're done compiling?
		LLFile::remove(filename);
	}
	else
	{
		if( gViewerStats )
		{
			gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		}

		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status )
		{
			LLChat chat("Script not found on server.");
			LLFloaterChat::addChat(chat);
			snprintf(buffer, sizeof(buffer), "Problem downloading %s.", 	/* Flawfinder: ignore */
				data->mScriptName.c_str());
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			LLChat chat("Insufficient permissions to download a script.");
			LLFloaterChat::addChat(chat);
			snprintf(buffer, sizeof(buffer), "Insufficient permissions for '%s'.", 	/* Flawfinder: ignore */
				data->mScriptName.c_str());
		}
		else
		{
			snprintf(buffer, sizeof(buffer), "Unknown failure to download %s.", 	/* Flawfinder: ignore */
				data->mScriptName.c_str());
		}

		llwarns << "Problem downloading script asset." << llendl;
		if(queue) queue->removeItemByAssetID(asset_id);
	}
	if(queue) 
	{
		LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(queue, "queue output");
		list->addSimpleItem(buffer);
	}
	delete data;
}

// static
void LLFloaterCompileQueue::onSaveTextComplete(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	llinfos << "LLFloaterCompileQueue::onSaveTextComplete()" << llendl;
	if (status)
	{
		llwarns << "Unable to save text for script." << llendl;
		LLString::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("CompileQueueSaveText", args);
	}
}

// static
void LLFloaterCompileQueue::onSaveBytecodeComplete(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	llinfos << "LLFloaterCompileQueue::onSaveBytecodeComplete()" << llendl;
	LLCompileQueueData* data = (LLCompileQueueData*)user_data;
	LLFloaterCompileQueue* queue = static_cast<LLFloaterCompileQueue*> 
				(LLFloaterScriptQueue::findInstance(data->mQueueID));
	if(queue && (0 == status) && data)
	{
		queue->updateAssetID(data->mOldAssetID, asset_id);
		queue->saveItemByAssetID(asset_id);
		queue->removeItemByAssetID(asset_id);
	}
	else
	{
		llwarns << "Unable to save bytecode for script." << llendl;
		LLStringBase<char>::format_map_t args;
		args["[REASON]"] = std::string(LLAssetStorage::getErrorString(status));
		gViewerWindow->alertXml("CompileQueueSaveBytecode", args);
	}
	delete data;
	data = NULL;
}

// compile the file given and save it out.
void LLFloaterCompileQueue::compile(const char* filename,
									const LLUUID& asset_id)
{
	LLUUID new_asset_id;
	LLTransactionID tid;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	char uuid_string[UUID_STR_LENGTH];  /*Flawfinder: ignore*/
	new_asset_id.toString(uuid_string);
	char dst_filename[LL_MAX_PATH];	 /*Flawfinder: ignore*/
	snprintf(dst_filename, sizeof(dst_filename), "%s.lso", gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string).c_str());	 	/* Flawfinder: ignore */
	char err_filename[LL_MAX_PATH];	 /*Flawfinder: ignore*/
	snprintf(err_filename, sizeof(err_filename), "%s.out", gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string).c_str());	 	/* Flawfinder: ignore */

	gAssetStorage->storeAssetData(filename, tid,
								  LLAssetType::AT_LSL_TEXT,
								  &onSaveTextComplete, NULL, FALSE);
	if(!lscript_compile(filename, dst_filename, err_filename, gAgent.isGodlike()))
	{
		llwarns << "compile failed" << llendl;
		removeItemByAssetID(asset_id);
	}
	else
	{
		llinfos << "compile successful." << llendl;
		// Save the bytecode
		LLCompileQueueData* data = new LLCompileQueueData(mID, asset_id);
		gAssetStorage->storeAssetData(dst_filename, tid,
									  LLAssetType::AT_LSL_BYTECODE,
									  &onSaveBytecodeComplete,
									  (void*)data, FALSE);
	}
}

void LLFloaterCompileQueue::removeItemByAssetID(const LLUUID& asset_id)
{
	llinfos << "LLFloaterCompileQueue::removeItemByAssetID()" << llendl;
	for(S32 i = 0; i < mCurrentScripts.count(); )
	{
		if(asset_id == mCurrentScripts.get(i)->getAssetUUID())
		{
			mCurrentScripts.remove(i);
		}
		else
		{
			++i;
		}
	}
	if(mCurrentScripts.count() == 0)
	{
		nextObject();
	}
}

void LLFloaterCompileQueue::saveItemByAssetID(const LLUUID& asset_id)
{
	llinfos << "LLFloaterCompileQueue::saveItemByAssetID()" << llendl;
	LLViewerObject* viewer_object = gObjectList.findObject(mCurrentObjectID);
	if(viewer_object)
	{
		S32 count = mCurrentScripts.count();
		for(S32 i = 0; i < count; ++i)
		{
			if(asset_id == mCurrentScripts.get(i)->getAssetUUID())
			{
				// *FIX: this auto-resets active to TRUE. That might
				// be a bad idea.
				viewer_object->saveScript(mCurrentScripts.get(i), TRUE, false);
			}
		}
	}
	else
	{
		llwarns << "Unable to finish save!" << llendl;
	}
}

// find old_asst_id, and set the asset id to new_asset_id
void LLFloaterCompileQueue::updateAssetID(const LLUUID& old_asset_id,
										  const LLUUID& new_asset_id)
{
	S32 count = mCurrentScripts.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(old_asset_id == mCurrentScripts.get(i)->getAssetUUID())
		{
			mCurrentScripts.get(i)->setAssetUUID(new_asset_id);
		}
	}
}

///----------------------------------------------------------------------------
/// Class LLFloaterResetQueue
///----------------------------------------------------------------------------

// static
LLFloaterResetQueue* LLFloaterResetQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterResetQueue* new_queue = new LLFloaterResetQueue("queue",
																 rect);
	new_queue->open();	 /*Flawfinder: ignore*/
	return new_queue;
}

LLFloaterResetQueue::LLFloaterResetQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, RESET_QUEUE_TITLE, RESET_START_STRING)
{ }

LLFloaterResetQueue::~LLFloaterResetQueue()
{ 
}

void LLFloaterResetQueue::handleInventory(LLViewerObject* viewer_obj,
										  InventoryObjectList* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	InventoryObjectList::const_iterator it = inv->begin();
	InventoryObjectList::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "queue output");
				char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
				snprintf(buffer, sizeof(buffer), "Resetting '%s'.", item->getName().c_str());		 	/* Flawfinder: ignore */
				list->addSimpleItem(buffer);
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_ScriptReset);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}

	nextObject();	
}

///----------------------------------------------------------------------------
/// Class LLFloaterRunQueue
///----------------------------------------------------------------------------

// static
LLFloaterRunQueue* LLFloaterRunQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterRunQueue* new_queue = new LLFloaterRunQueue("queue",
																 rect);
	new_queue->open();		 /*Flawfinder: ignore*/
	return new_queue;
}

LLFloaterRunQueue::LLFloaterRunQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, RUN_QUEUE_TITLE, RUN_START_STRING)
{ }

LLFloaterRunQueue::~LLFloaterRunQueue()
{ 
}

void LLFloaterRunQueue::handleInventory(LLViewerObject* viewer_obj,
										  InventoryObjectList* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	InventoryObjectList::const_iterator it = inv->begin();
	InventoryObjectList::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "queue output");
				char buffer[MAX_STRING];  	/*Flawfinder: ignore*/
				snprintf(buffer, sizeof(buffer), "Running '%s'.", item->getName().c_str());		 	/* Flawfinder: ignore */
				list->addSimpleItem(buffer);

				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_SetScriptRunning);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->addBOOLFast(_PREHASH_Running, TRUE);
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}

	nextObject();	
}

///----------------------------------------------------------------------------
/// Class LLFloaterNotRunQueue
///----------------------------------------------------------------------------

// static
LLFloaterNotRunQueue* LLFloaterNotRunQueue::create()
{
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect = gSavedSettings.getRect("CompileOutputRect");
	rect.translate(left - rect.mLeft, top - rect.mTop);
	LLFloaterNotRunQueue* new_queue = new LLFloaterNotRunQueue("queue",
																 rect);
	new_queue->open();	 /*Flawfinder: ignore*/
	return new_queue;
}

LLFloaterNotRunQueue::LLFloaterNotRunQueue(const std::string& name, const LLRect& rect)
: LLFloaterScriptQueue(name, rect, NOT_RUN_QUEUE_TITLE, NOT_RUN_START_STRING)
{ }

LLFloaterNotRunQueue::~LLFloaterNotRunQueue()
{ 
}

void LLFloaterNotRunQueue::handleInventory(LLViewerObject* viewer_obj,
										  InventoryObjectList* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	InventoryObjectList::const_iterator it = inv->begin();
	InventoryObjectList::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "queue output");
				char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
				snprintf(buffer, sizeof(buffer), "Not running '%s'.", item->getName().c_str());	 	/* Flawfinder: ignore */
				list->addSimpleItem(buffer);
	
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_SetScriptRunning);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_Script);
				msg->addUUIDFast(_PREHASH_ObjectID, viewer_obj->getID());
				msg->addUUIDFast(_PREHASH_ItemID, (*it)->getUUID());
				msg->addBOOLFast(_PREHASH_Running, FALSE);
				msg->sendReliable(object->getRegion()->getHost());
			}
		}
	}

	nextObject();	
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
