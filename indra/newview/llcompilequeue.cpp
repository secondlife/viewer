/** 
 * @file llcompilequeue.cpp
 * @brief LLCompileQueueData class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

/**
 *
 * Implementation of the script queue which keeps an array of object
 * UUIDs and manipulates all of the scripts on each of them. 
 *
 */


#include "llviewerprecompiledheaders.h"

#include "llcompilequeue.h"

#include "llagent.h"
#include "llassetuploadqueue.h"
#include "llassetuploadresponders.h"
#include "llchat.h"
#include "llfloaterreg.h"
#include "llviewerwindow.h"
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
#include "llnotificationsutil.h"
#include "llviewerstats.h"
#include "llvfile.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

#include "llselectmgr.h"

// *TODO: This should be separated into the script queue, and the floater views of that queue.
// There should only be one floater class that can view any queue type

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

struct LLScriptQueueData
{
	LLUUID mQueueID;
	std::string mScriptName;
	LLUUID mTaskId;
	LLUUID mItemId;
	LLScriptQueueData(const LLUUID& q_id, const std::string& name, const LLUUID& task_id, const LLUUID& item_id) :
		mQueueID(q_id), mScriptName(name), mTaskId(task_id), mItemId(item_id) {}

};

///----------------------------------------------------------------------------
/// Class LLFloaterScriptQueue
///----------------------------------------------------------------------------

// Default constructor
LLFloaterScriptQueue::LLFloaterScriptQueue(const LLSD& key) :
	LLFloater(key),
	mDone(false),
	mMono(false)
{
}

// Destroys the object
LLFloaterScriptQueue::~LLFloaterScriptQueue()
{
}

BOOL LLFloaterScriptQueue::postBuild()
{
	childSetAction("close",onCloseBtn,this);
	getChildView("close")->setEnabled(FALSE);
	setVisible(true);
	return TRUE;
}

// This is the callback method for the viewer object currently being
// worked on.
// NOT static, virtual!
void LLFloaterScriptQueue::inventoryChanged(LLViewerObject* viewer_object,
											 LLInventoryObject::object_list_t* inv,
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
	self->closeFloater();
}

void LLFloaterScriptQueue::addObject(const LLUUID& id)
{
	mObjectIDs.put(id);
}

BOOL LLFloaterScriptQueue::start()
{
	std::string buffer;

	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectIDs.count());
	buffer = getString ("Starting", args);
	
	getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);

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
		mDone = true;
		getChild<LLScrollListCtrl>("queue output")->addSimpleElement(getString("Done"), ADD_BOTTOM);
		getChildView("close")->setEnabled(TRUE);
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
			LLUUID* id = new LLUUID(getKey().asUUID());
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

class LLCompileFloaterUploadQueueSupplier : public LLAssetUploadQueueSupplier
{
public:
	
	LLCompileFloaterUploadQueueSupplier(const LLUUID& queue_id) :
		mQueueId(queue_id)
	{
	}
		
	virtual LLAssetUploadQueue* get() const 
	{
		LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(mQueueId));
		if(NULL == queue)
		{
			return NULL;
		}
		return queue->getUploadQueue();
	}

	virtual void log(std::string message) const
	{
		LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(mQueueId));
		if(NULL == queue)
		{
			return;
		}

		queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
	}
		
private:
	LLUUID mQueueId;
};

LLFloaterCompileQueue::LLFloaterCompileQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("CompileQueueTitle"));
	setStartString(LLTrans::getString("CompileQueueStart"));
														 															 
	mUploadQueue = new LLAssetUploadQueue(new LLCompileFloaterUploadQueueSupplier(key.asUUID()));
}

LLFloaterCompileQueue::~LLFloaterCompileQueue()
{ 
}

void LLFloaterCompileQueue::handleInventory(LLViewerObject *viewer_object,
											LLInventoryObject::object_list_t* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.

	typedef std::multimap<LLUUID, LLPointer<LLInventoryItem> > uuid_item_map;
	uuid_item_map asset_item_map;

	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
			// Check permissions before allowing the user to retrieve data.
			if (item->getPermissions().allowModifyBy(gAgent.getID(), gAgent.getGroupID())  &&
				item->getPermissions().allowCopyBy(gAgent.getID(), gAgent.getGroupID()) )
			{
				LLPointer<LLViewerInventoryItem> script = new LLViewerInventoryItem(item);
				mCurrentScripts.put(script);
				asset_item_map.insert(std::make_pair(item->getAssetUUID(), item));
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
			LLScriptQueueData* datap = new LLScriptQueueData(getKey().asUUID(),
												 itemp->getName(),
												 viewer_object->getID(),
												 itemp->getUUID());

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
	if(!data)
	{
		return;
	}
	LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", data->mQueueID);
	
	std::string buffer;
	if(queue && (0 == status))
	{
		//llinfos << "ITEM NAME 3: " << data->mScriptName << llendl;

		// Dump this into a file on the local disk so we can compile it.
		std::string filename;
		LLVFile file(vfs, asset_id, type);
		std::string uuid_str;
		asset_id.toString(uuid_str);
		filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_str) + llformat(".%s",LLAssetType::lookup(type));
		
		const bool is_running = true;
		LLViewerObject* object = gObjectList.findObject(data->mTaskId);
		if (object)
		{
			std::string url = object->getRegion()->getCapability("UpdateScriptTask");
			if(!url.empty())
			{
				// Read script source in to buffer.
				U32 script_size = file.getSize();
				U8* script_data = new U8[script_size];
				file.read(script_data, script_size);

				queue->mUploadQueue->queue(filename, data->mTaskId, 
										   data->mItemId, is_running, queue->mMono, queue->getKey().asUUID(),
										   script_data, script_size, data->mScriptName);
			}
			else
			{
				// It's now in the file, now compile it.
				buffer = LLTrans::getString("CompileQueueDownloadedCompiling") + (": ") + data->mScriptName;

				// Write script to local file for compilation.
				LLFILE *fp = LLFile::fopen(filename, "wb");	 /*Flawfinder: ignore*/
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
				else
				{
					llerrs << "Unable to find object to compile" << llendl;
				}

				// TODO: babbage: No compile if no cap.
				queue->compile(filename, data->mItemId);
					
				// Delete it after we're done compiling?
				LLFile::remove(filename);
			}
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status )
		{
			LLSD args;
			args["MESSAGE"] = LLTrans::getString("CompileQueueScriptNotFound");
			LLNotificationsUtil::add("SystemMessage", args);
			
			buffer = LLTrans::getString("CompileQueueProblemDownloading") + (": ") + data->mScriptName;
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			LLSD args;
			args["MESSAGE"] = LLTrans::getString("CompileQueueInsufficientPermDownload");
			LLNotificationsUtil::add("SystemMessage", args);

			buffer = LLTrans::getString("CompileQueueInsufficientPermFor") + (": ") + data->mScriptName;
		}
		else
		{
			buffer = LLTrans::getString("CompileQueueUnknownFailure") + (" ") + data->mScriptName;
		}

		llwarns << "Problem downloading script asset." << llendl;
		if(queue) queue->removeItemByItemID(data->mItemId);
	}
	if(queue && (buffer.size() > 0)) 
	{
		queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
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
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("CompileQueueSaveText", args);
	}
}

// static
void LLFloaterCompileQueue::onSaveBytecodeComplete(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	llinfos << "LLFloaterCompileQueue::onSaveBytecodeComplete()" << llendl;
	LLCompileQueueData* data = (LLCompileQueueData*)user_data;
	LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", data->mQueueID);
	if(queue && (0 == status) && data)
	{
		queue->saveItemByItemID(data->mItemId);
		queue->removeItemByItemID(data->mItemId);
	}
	else
	{
		llwarns << "Unable to save bytecode for script." << llendl;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("CompileQueueSaveBytecode", args);
	}
	delete data;
	data = NULL;
}

// compile the file given and save it out.
void LLFloaterCompileQueue::compile(const std::string& filename,
									const LLUUID& item_id)
{
	LLUUID new_asset_id;
	LLTransactionID tid;
	tid.generate();
	new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
	
	std::string uuid_string;
	new_asset_id.toString(uuid_string);
	std::string dst_filename;
	dst_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string) + ".lso";
	std::string err_filename;
	err_filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string) + ".out";

	gAssetStorage->storeAssetData(filename, tid,
								  LLAssetType::AT_LSL_TEXT,
								  &onSaveTextComplete, NULL, FALSE);

	const BOOL compile_to_mono = FALSE;
	if(!lscript_compile(filename.c_str(), dst_filename.c_str(),
						err_filename.c_str(), compile_to_mono,
						uuid_string.c_str(), gAgent.isGodlike()))
	{
		llwarns << "compile failed" << llendl;
		removeItemByItemID(item_id);
	}
	else
	{
		llinfos << "compile successful." << llendl;
		
		// Save LSL bytecode
		LLCompileQueueData* data = new LLCompileQueueData(getKey().asUUID(), item_id);
		gAssetStorage->storeAssetData(dst_filename, new_asset_id,
									LLAssetType::AT_LSL_BYTECODE,
									&LLFloaterCompileQueue::onSaveBytecodeComplete,
									(void*)data, FALSE);
	}
}

void LLFloaterCompileQueue::removeItemByItemID(const LLUUID& asset_id)
{
	llinfos << "LLFloaterCompileQueue::removeItemByAssetID()" << llendl;
	for(S32 i = 0; i < mCurrentScripts.count(); )
	{
		if(asset_id == mCurrentScripts.get(i)->getUUID())
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

const LLInventoryItem* LLFloaterCompileQueue::findItemByItemID(const LLUUID& asset_id) const
{
	LLInventoryItem* result = NULL;
	S32 count = mCurrentScripts.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(asset_id == mCurrentScripts.get(i)->getUUID())
		{
			result = mCurrentScripts.get(i);
		}
	}
	return result;
}

void LLFloaterCompileQueue::saveItemByItemID(const LLUUID& asset_id)
{
	llinfos << "LLFloaterCompileQueue::saveItemByAssetID()" << llendl;
	LLViewerObject* viewer_object = gObjectList.findObject(mCurrentObjectID);
	if(viewer_object)
	{
		S32 count = mCurrentScripts.count();
		for(S32 i = 0; i < count; ++i)
		{
			if(asset_id == mCurrentScripts.get(i)->getUUID())
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

///----------------------------------------------------------------------------
/// Class LLFloaterResetQueue
///----------------------------------------------------------------------------

LLFloaterResetQueue::LLFloaterResetQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("ResetQueueTitle"));
	setStartString(LLTrans::getString("ResetQueueStart"));
}

LLFloaterResetQueue::~LLFloaterResetQueue()
{ 
}

void LLFloaterResetQueue::handleInventory(LLViewerObject* viewer_obj,
										  LLInventoryObject::object_list_t* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				std::string buffer;
				buffer = getString("Resetting") + (": ") + item->getName();
				getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
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

LLFloaterRunQueue::LLFloaterRunQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("RunQueueTitle"));
	setStartString(LLTrans::getString("RunQueueStart"));
}

LLFloaterRunQueue::~LLFloaterRunQueue()
{ 
}

void LLFloaterRunQueue::handleInventory(LLViewerObject* viewer_obj,
										  LLInventoryObject::object_list_t* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
				std::string buffer;
				buffer = getString("Running") + (": ") + item->getName();
				list->addSimpleElement(buffer, ADD_BOTTOM);

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

LLFloaterNotRunQueue::LLFloaterNotRunQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("NotRunQueueTitle"));
	setStartString(LLTrans::getString("NotRunQueueStart"));
}

LLFloaterNotRunQueue::~LLFloaterNotRunQueue()
{ 
}

void LLFloaterNotRunQueue::handleInventory(LLViewerObject* viewer_obj,
										  LLInventoryObject::object_list_t* inv)
{
	// find all of the lsl, leaving off duplicates. We'll remove
	// all matching asset uuids on compilation success.
	LLDynamicArray<const char*> names;
	
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();
	for ( ; it != end; ++it)
	{
		if((*it)->getType() == LLAssetType::AT_LSL_TEXT)
		{
			LLViewerObject* object = gObjectList.findObject(viewer_obj->getID());

			if (object)
			{
				LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
				LLScrollListCtrl* list = getChild<LLScrollListCtrl>("queue output");
				std::string buffer;
				buffer = getString("NotRunning") + (": ") +item->getName();
				list->addSimpleElement(buffer, ADD_BOTTOM);
	
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
