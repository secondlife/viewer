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
#include "llchat.h"
#include "llfloaterreg.h"
#include "llviewerwindow.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
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
#include "llexperiencecache.h"

#include "llviewerassetupload.h"
#include "llcorehttputil.h"

namespace
{

    const std::string QUEUE_EVENTPUMP_NAME("ScriptActionQueue");
    const F32         TIMEOUT_INVENTORY_FETCH(5.0);


    class ObjectInventoryFetcher: public LLVOInventoryListener
    {
    public:
        typedef boost::shared_ptr<ObjectInventoryFetcher> ptr_t;

        ObjectInventoryFetcher(LLEventPump &pump, LLViewerObject* object, void* user_data) :
            mPump(pump),
            LLVOInventoryListener()
        {
            registerVOInventoryListener(object, this);
        }

        virtual void inventoryChanged(LLViewerObject* object,
            LLInventoryObject::object_list_t* inventory,
            S32 serial_num,
            void* user_data);

        void fetchInventory() 
        {
            requestVOInventory();
        }

        const LLInventoryObject::object_list_t & getInventoryList() const { return mInventoryList; }

    private:
        LLInventoryObject::object_list_t    mInventoryList;
        LLEventPump &                       mPump;
    };

    class HandleScriptUserData
    {
    public:
        HandleScriptUserData(const std::string &pumpname) :
            mPumpname(pumpname)
        { }

        const std::string &getPumpName() const { return mPumpname; }

    private:
        std::string mPumpname;
    };


}

// *NOTE$: A minor specialization of LLScriptAssetUpload, it does not require a buffer 
// (and does not save a buffer to the vFS) and it finds the compile queue window and 
// displays a compiling message.
class LLQueuedScriptAssetUpload : public LLScriptAssetUpload
{
public:
    LLQueuedScriptAssetUpload(LLUUID taskId, LLUUID itemId, LLUUID assetId, TargetType_t targetType,
            bool isRunning, std::string scriptName, LLUUID queueId, LLUUID exerienceId, taskUploadFinish_f finish) :
        LLScriptAssetUpload(taskId, itemId, targetType, isRunning, 
            exerienceId, std::string(), finish),
        mScriptName(scriptName),
        mQueueId(queueId)
    {
        setAssetId(assetId);
    }

    virtual LLSD prepareUpload()
    {
        /* *NOTE$: The parent class (LLScriptAssetUpload will attempt to save 
         * the script buffer into to the VFS.  Since the resource is already in 
         * the VFS we don't want to do that.  Just put a compiling message in
         * the window and move on
         */
        LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", LLSD(mQueueId));
        if (queue)
        {
            std::string message = std::string("Compiling \"") + getScriptName() + std::string("\"...");

            queue->getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
        }

        return LLSD().with("success", LLSD::Boolean(true));
    }


    std::string getScriptName() const { return mScriptName; }

private:
    void setScriptName(const std::string &scriptName) { mScriptName = scriptName; }

    LLUUID mQueueId;
    std::string mScriptName;
};

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

struct LLScriptQueueData
{
	LLUUID mQueueID;
	LLUUID mTaskId;
	LLPointer<LLInventoryItem> mItem;
	LLHost mHost;
	LLUUID mExperienceId;
	std::string mExperiencename;
	LLScriptQueueData(const LLUUID& q_id, const LLUUID& task_id, LLInventoryItem* item) :
		mQueueID(q_id), mTaskId(task_id), mItem(new LLInventoryItem(item)) {}

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

// static
void LLFloaterScriptQueue::onCloseBtn(void* user_data)
{
	LLFloaterScriptQueue* self = (LLFloaterScriptQueue*)user_data;
	self->closeFloater();
}

void LLFloaterScriptQueue::addObject(const LLUUID& id)
{
	mObjectIDs.insert(id);
}

BOOL LLFloaterScriptQueue::start()
{
	std::string buffer;

	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectIDs.size());
	buffer = getString ("Starting", args);
	
	getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);

	return startQueue();
}

void LLFloaterScriptQueue::addProcessingMessage(const std::string &message, const LLSD &args)
{
    std::string buffer(LLTrans::getString(message, args));

    getChild<LLScrollListCtrl>("queue output")->addSimpleElement(buffer, ADD_BOTTOM);
}

void LLFloaterScriptQueue::addStringMessage(const std::string &message)
{
    getChild<LLScrollListCtrl>("queue output")->addSimpleElement(message, ADD_BOTTOM);
}


BOOL LLFloaterScriptQueue::isDone() const
{
	return (mCurrentObjectID.isNull() && (mObjectIDs.size() == 0));
}

///----------------------------------------------------------------------------
/// Class LLFloaterCompileQueue
///----------------------------------------------------------------------------
LLFloaterCompileQueue::LLFloaterCompileQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("CompileQueueTitle"));
	setStartString(LLTrans::getString("CompileQueueStart"));
														 															 
//	mUploadQueue = new LLAssetUploadQueue(new LLCompileFloaterUploadQueueSupplier(key.asUUID()));
}

LLFloaterCompileQueue::~LLFloaterCompileQueue()
{ 
}

void LLFloaterCompileQueue::experienceIdsReceived( const LLSD& content )
{
	for(LLSD::array_const_iterator it  = content.beginArray(); it != content.endArray(); ++it)
	{
		mExperienceIds.insert(it->asUUID());
	}
//	nextObject();
}

BOOL LLFloaterCompileQueue::hasExperience( const LLUUID& id ) const
{
	return mExperienceIds.find(id) != mExperienceIds.end();
}

// //Attempt to record this asset ID.  If it can not be inserted into the set 
// //then it has already been processed so return false.
// bool LLFloaterCompileQueue::checkAssetId(const LLUUID &assetId)
// {
//     std::pair<uuid_list_t::iterator, bool> result = mAssetIds.insert(assetId);
//     return result.second;
// }

void LLFloaterCompileQueue::handleHTTPResponse(std::string pumpName, const LLSD &expresult)
{
    LLEventPumps::instance().post(pumpName, expresult);
}

// *TODO: handleSCriptRetrieval is passed into the VFS via a legacy C function pointer
// future project would be to convert these to C++ callables (std::function<>) so that 
// we can use bind and remove the userData parameter.
// 
void LLFloaterCompileQueue::handleScriptRetrieval(LLVFS *vfs, const LLUUID& assetId, 
    LLAssetType::EType type, void* userData, S32 status, LLExtStat extStatus)
{
    LLSD result(LLSD::emptyMap());

    result["asset_id"] = assetId;
    if (status)
    {
        result["error"] = status;
     
        if (status == LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE)
        {
            result["message"] = LLTrans::getString("CompileQueueProblemDownloading") + (":");
            result["alert"] = LLTrans::getString("CompileQueueScriptNotFound");
        }
        else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
        {
            result["message"] = LLTrans::getString("CompileQueueInsufficientPermFor") + (":");
            result["alert"] = LLTrans::getString("CompileQueueInsufficientPermDownload");
        }
        else
        {
            result["message"] = LLTrans::getString("CompileQueueUnknownFailure");
        }
    }

    LLEventPumps::instance().post(((HandleScriptUserData *)userData)->getPumpName(), result);

}

/*static*/
void LLFloaterCompileQueue::processExperienceIdResults(LLSD result, LLUUID parent)
{
    LLFloaterCompileQueue* queue = LLFloaterReg::findTypedInstance<LLFloaterCompileQueue>("compile_queue", parent);
    if (!queue)
        return;

    queue->experienceIdsReceived(result["experience_ids"]);

    LLHandle<LLFloaterScriptQueue> hFloater(queue->getDerivedHandle<LLFloaterScriptQueue>());

    fnQueueAction_t fn = boost::bind(LLFloaterCompileQueue::processScript,
        queue->getDerivedHandle<LLFloaterCompileQueue>(), _1, _2, _3);


    LLCoros::instance().launch("ScriptQueueCompile", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        queue->mStartString,
        hFloater,
        queue->mObjectIDs,
        fn));

}

bool LLFloaterCompileQueue::processScript(LLHandle<LLFloaterCompileQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLSD result;
    LLFloaterCompileQueue *that = hfloater.get();
    bool monocompile = that->mMono;

    if (!that)
        return false;

    // Initial test to see if we can (or should) attempt to compile the script.
    LLInventoryItem *item = dynamic_cast<LLInventoryItem *>(inventory);
    {

        if (!item->getPermissions().allowModifyBy(gAgent.getID(), gAgent.getGroupID()) ||
            !item->getPermissions().allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
        {
            std::string buffer = "Skipping: " + item->getName() + "(Permissions)";
            that->addStringMessage(buffer);
            return true;
        }

//         if (!that->checkAssetId(item->getAssetUUID()))
//         {
//             std::string buffer = "Skipping: " + item->getName() + "(Repeat)";
//             that->addStringMessage(buffer);
//             return true;
//         }
    }
    that = NULL;

    // Attempt to retrieve the experience
    LLUUID experienceId;
    {
        LLExperienceCache::instance().fetchAssociatedExperience(inventory->getParentUUID(), inventory->getUUID(),
            boost::bind(&LLFloaterCompileQueue::handleHTTPResponse, pump.getName(), _1));

        result = llcoro::suspendUntilEventOnWithTimeout(pump, TIMEOUT_INVENTORY_FETCH, 
            LLSD().with("timeout", LLSD::Boolean(true)));

        that = hfloater.get();
        if (!that)
        {
            return false;
        }

        if (result.has("timeout") && result["timeout"].asBoolean())
        {
            std::string buffer = that->getString("Timeout") + ": " + inventory->getName();
            that->addStringMessage(buffer);
            return true;
        }

        if (result.has(LLExperienceCache::EXPERIENCE_ID))
        {
            experienceId = result[LLExperienceCache::EXPERIENCE_ID].asUUID();
            if (!that->hasExperience(experienceId))
            {
                that->addProcessingMessage("CompileNoExperiencePerm", LLSD()
                    .with("SCRIPT", inventory->getName())
                    .with("EXPERIENCE", result[LLExperienceCache::NAME].asString()));
                return true;
            }
        }

    }
    that = NULL;

    {
        HandleScriptUserData    userData(pump.getName());


        // request the asset
        gAssetStorage->getInvItemAsset(LLHost(),
            gAgent.getID(),
            gAgent.getSessionID(),
            item->getPermissions().getOwner(),
            object->getID(),
            item->getUUID(),
            item->getAssetUUID(),
            item->getType(),
            &LLFloaterCompileQueue::handleScriptRetrieval,
            &userData);

        result = llcoro::suspendUntilEventOnWithTimeout(pump, TIMEOUT_INVENTORY_FETCH, 
            LLSD().with("timeout", LLSD::Boolean(true)));
    }

    that = hfloater.get();
    if (!that)
    {
        return false;
    }

    if (result.has("timeout"))
    {
        if (result.has("timeout") && result["timeout"].asBoolean())
        {
            std::string buffer = that->getString("Timeout") + ": " + inventory->getName();
            that->addStringMessage(buffer);
            return true;
        }
    }

    if (result.has("error"))
    {
        LL_WARNS("SCRIPTQ") << "Inventory fetch returned with error. Code: " << result["error"].asString() << LL_ENDL;
        std::string buffer = result["message"].asString() + " " + inventory->getName();
        that->addStringMessage(buffer);

        if (result.has("alert"))
        {
            LLSD args;
            args["MESSAGE"] = result["alert"].asString();
            LLNotificationsUtil::add("SystemMessage", args);
        }
        return true;
    }

    LLUUID assetId = result["asset_id"];
    that = NULL;


    std::string url = object->getRegion()->getCapability("UpdateScriptTask");


    {
        LLResourceUploadInfo::ptr_t uploadInfo(new LLQueuedScriptAssetUpload(object->getID(), 
            inventory->getUUID(), 
            assetId, 
            monocompile ? LLScriptAssetUpload::MONO : LLScriptAssetUpload::LSL2,
            true, 
            inventory->getName(), 
            LLUUID(), 
            experienceId, 
            boost::bind(&LLFloaterCompileQueue::handleHTTPResponse, pump.getName(), _4)));

        LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
    }

    result = llcoro::suspendUntilEventOnWithTimeout(pump, TIMEOUT_INVENTORY_FETCH, LLSD().with("timeout", LLSD::Boolean(true)));

    that = hfloater.get();
    if (!that)
    {
        return false;
    }

    if (result.has("timeout"))
    {
        if (result.has("timeout") && result["timeout"].asBoolean())
        {
            std::string buffer = that->getString("Timeout") + ": " + inventory->getName();
            that->addStringMessage(buffer);
            return true;
        }
    }

    // Bytecode save completed
    if (result["compiled"])
    {
        std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" succeeded");

        that->addStringMessage(buffer);
        LL_INFOS() << buffer << LL_ENDL;
    }
    else
    {
        LLSD compile_errors = result["errors"];
        std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" failed:");
        that->addStringMessage(buffer);
        for (LLSD::array_const_iterator line = compile_errors.beginArray();
            line < compile_errors.endArray(); line++)
        {
            std::string str = line->asString();
            str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

            that->addStringMessage(str);
        }
        LL_INFOS() << result["errors"] << LL_ENDL;
    }

    return true;
}

bool LLFloaterCompileQueue::startQueue()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        std::string lookup_url = region->getCapability("GetCreatorExperiences");
        if (!lookup_url.empty())
        {
            LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t success =
                boost::bind(&LLFloaterCompileQueue::processExperienceIdResults, _1, getKey().asUUID());

            LLCoreHttpUtil::HttpCoroutineAdapter::completionCallback_t failure =
                boost::bind(&LLFloaterCompileQueue::processExperienceIdResults, LLSD(), getKey().asUUID());

            LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpGet(lookup_url,
                success, failure);
            return TRUE;
        }
    }

    return true;
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

bool LLFloaterResetQueue::resetObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, 
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLFloaterScriptQueue *that = hfloater.get();
    if (that)
    {
        std::string buffer;
        buffer = that->getString("Resetting") + (": ") + inventory->getName();
        that->addStringMessage(buffer);
    }
    
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_ScriptReset);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterResetQueue::startQueue()
{
    fnQueueAction_t fn = boost::bind(LLFloaterResetQueue::resetObjectScripts,
        getDerivedHandle<LLFloaterScriptQueue>(), _1, _2, _3);

    LLCoros::instance().launch("ScriptResetQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        getDerivedHandle<LLFloaterScriptQueue>(),
        mObjectIDs,
        fn));

    return true;
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

bool LLFloaterRunQueue::runObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, 
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLFloaterScriptQueue *that = hfloater.get();
    if (that)
    {
        std::string buffer;
        buffer = that->getString("Running") + (": ") + inventory->getName();
        that->addStringMessage(buffer);
    }

    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_SetScriptRunning);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->addBOOLFast(_PREHASH_Running, TRUE);
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterRunQueue::startQueue()
{
    LLHandle<LLFloaterScriptQueue> hFloater(getDerivedHandle<LLFloaterScriptQueue>());
    fnQueueAction_t fn = boost::bind(LLFloaterRunQueue::runObjectScripts, hFloater, _1, _2, _3);

    LLCoros::instance().launch("ScriptRunQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        hFloater,
        mObjectIDs,
        fn));

    return true;
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

bool LLFloaterNotRunQueue::stopObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, 
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLFloaterScriptQueue *that = hfloater.get();
    if (that)
    {
        std::string buffer;
        buffer = that->getString("NotRunning") + (": ") + inventory->getName();
        that->addStringMessage(buffer);
    }

    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_SetScriptRunning);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
    msg->addUUIDFast(_PREHASH_ItemID, inventory->getUUID());
    msg->addBOOLFast(_PREHASH_Running, FALSE);
    msg->sendReliable(object->getRegion()->getHost());

    return true;
}

bool LLFloaterNotRunQueue::startQueue()
{
    LLHandle<LLFloaterScriptQueue> hFloater(getDerivedHandle<LLFloaterScriptQueue>());

    fnQueueAction_t fn = boost::bind(&LLFloaterNotRunQueue::stopObjectScripts, hFloater, _1, _2, _3);
    LLCoros::instance().launch("ScriptQueueNotRun", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        hFloater,
        mObjectIDs,
        fn));

    return true;
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
void ObjectInventoryFetcher::inventoryChanged(LLViewerObject* object,
        LLInventoryObject::object_list_t* inventory, S32 serial_num, void* user_data)
{
    mInventoryList.clear();
    mInventoryList.assign(inventory->begin(), inventory->end());

    mPump.post(LLSD().with("changed", LLSD::Boolean(true)));

}

void LLFloaterScriptQueue::objectScriptProcessingQueueCoro(std::string action, LLHandle<LLFloaterScriptQueue> hfloater,
    uuid_list_t objectList, fnQueueAction_t func)
{
    LLCoros::set_consuming(true);
    LLFloaterScriptQueue * floater(NULL);
    LLEventMailDrop        maildrop(QUEUE_EVENTPUMP_NAME, true);

//     floater = hfloater.get();
//     floater->addProcessingMessage("Starting",
//         LLSD()
//         .with("[START]", action)
//         .with("[COUNT]", LLSD::Integer(objectList.size())));
//     floater = NULL;

    for (uuid_list_t::iterator itObj(objectList.begin()); (itObj != objectList.end()); ++itObj)
    {
        bool firstForObject = true;
        LL_INFOS("SCRIPTQ") << "Next object in queue with ID=" << (*itObj).asString() << LL_ENDL;

        LLPointer<LLViewerObject> obj = gObjectList.findObject(*itObj);
        LLInventoryObject::object_list_t inventory;
        if (obj)
        {
            ObjectInventoryFetcher::ptr_t fetcher(new ObjectInventoryFetcher(maildrop, obj, NULL));

            fetcher->fetchInventory();

            LLSD result = llcoro::suspendUntilEventOnWithTimeout(maildrop, TIMEOUT_INVENTORY_FETCH,
                LLSD().with("timeout", LLSD::Boolean(true)));

            if (result.has("timeout") && result["timeout"].asBoolean())
            {
                LL_WARNS("SCRIPTQ") << "Unable to retrieve inventory for object " << (*itObj).asString() <<
                    ". Skipping to next object." << LL_ENDL;
                continue;
            }

            inventory.assign(fetcher->getInventoryList().begin(), fetcher->getInventoryList().end());
        }
        else
        {
            LL_WARNS("SCRIPTQ") << "Unable to retrieve object with ID of " << (*itObj) <<
                ". Skipping to next." << LL_ENDL;
            continue;
        }

        // TODO: Get the name of the object we are looking at here so that we can display it below.
        //std::string objName = (dynamic_cast<LLInventoryObject *>(obj.get()))->getName();
        LL_DEBUGS("SCRIPTQ") << "Object has " << inventory.size() << " items." << LL_ENDL;

        for (LLInventoryObject::object_list_t::iterator itInv = inventory.begin();
            itInv != inventory.end(); ++itInv)
        {
            floater = hfloater.get();
            if (!floater)
            {
                LL_WARNS("SCRIPTQ") << "Script Queue floater closed! Canceling remaining ops" << LL_ENDL;
                break;
            }

            // note, we have a smart pointer to the obj above... but if we didn't we'd check that 
            // it still exists here.

            if (((*itInv)->getType() == LLAssetType::AT_LSL_TEXT))
            {
                LL_DEBUGS("SCRIPTQ") << "Inventory item " << (*itInv)->getUUID().asString() << "\"" << (*itInv)->getName() << "\"" << LL_ENDL;
                if (firstForObject)
                {
                    //floater->addStringMessage(objName + ":");
                    firstForObject = false;
                }

                if (!func(obj, (*itInv), maildrop))
                {
                    continue;
                }
            }

            llcoro::suspend();
        }
        // Just test to be sure the floater is still present before calling the func
        if (!hfloater.get())
        {
            LL_WARNS("SCRIPTQ") << "Script Queue floater dismissed." << LL_ENDL;
            break;
        }

    }

    floater = hfloater.get();
    if (floater)
    {
        floater->addStringMessage("Done");
        floater->getChildView("close")->setEnabled(TRUE);
    }
}
