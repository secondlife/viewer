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

    // ObjectIventoryFetcher is an adapter between the LLVOInventoryListener::inventoryChanged 
    // callback mechanism and the LLEventPump coroutine architecture allowing the 
    // coroutine to wait for the inventory event.
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

        return LLSDMap("success", LLSD::Boolean(true));
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

void LLFloaterScriptQueue::addObject(const LLUUID& id, std::string name)
{
    ObjectData obj = { id, name };
    mObjectList.push_back(obj);
}

BOOL LLFloaterScriptQueue::start()
{
	std::string buffer;

	LLStringUtil::format_map_t args;
	args["[START]"] = mStartString;
	args["[COUNT]"] = llformat ("%d", mObjectList.size());
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
	return (mCurrentObjectID.isNull() && (mObjectList.size() == 0));
}

///----------------------------------------------------------------------------
/// Class LLFloaterCompileQueue
///----------------------------------------------------------------------------
LLFloaterCompileQueue::LLFloaterCompileQueue(const LLSD& key)
  : LLFloaterScriptQueue(key)
{
	setTitle(LLTrans::getString("CompileQueueTitle"));
	setStartString(LLTrans::getString("CompileQueueStart"));
														 															 
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
}

BOOL LLFloaterCompileQueue::hasExperience( const LLUUID& id ) const
{
	return mExperienceIds.find(id) != mExperienceIds.end();
}

// //Attempt to record this asset ID.  If it can not be inserted into the set 
// //then it has already been processed so return false.

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

    // getDerived handle gets a handle that can be resolved to a parent class of the derived object.
    LLHandle<LLFloaterScriptQueue> hFloater(queue->getDerivedHandle<LLFloaterScriptQueue>());

    // note subtle difference here: getDerivedHandle in this case is for an LLFloaterCompileQueue
    fnQueueAction_t fn = boost::bind(LLFloaterCompileQueue::processScript,
        queue->getDerivedHandle<LLFloaterCompileQueue>(), _1, _2, _3);


    LLCoros::instance().launch("ScriptQueueCompile", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        queue->mStartString,
        hFloater,
        queue->mObjectList,
        fn));

}

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterCompileQueue::processScript(LLHandle<LLFloaterCompileQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    if (LLApp::isQuitting())
    {
        // Reply from coroutine came on shutdown
        // We are quiting, don't start any more coroutines!
        return true;
    }

    LLSD result;
    LLCheckedHandle<LLFloaterCompileQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro
    bool monocompile = floater->mMono;
    F32 fetch_timeout = gSavedSettings.getF32("QueueInventoryFetchTimeout");


    // Initial test to see if we can (or should) attempt to compile the script.
    LLInventoryItem *item = dynamic_cast<LLInventoryItem *>(inventory);

    if (!item)
    {
        LL_WARNS("SCRIPTQ") << "item retrieved is not an LLInventoryItem." << LL_ENDL;
        return true;
    }

    if (!item->getPermissions().allowModifyBy(gAgent.getID(), gAgent.getGroupID()) ||
        !item->getPermissions().allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
    {
        std::string buffer = "Skipping: " + item->getName() + "(Permissions)";
        floater->addStringMessage(buffer);
        return true;
    }

    // Attempt to retrieve the experience
    LLUUID experienceId;
    {
        LLExperienceCache::instance().fetchAssociatedExperience(inventory->getParentUUID(), inventory->getUUID(),
            boost::bind(&LLFloaterCompileQueue::handleHTTPResponse, pump.getName(), _1));

        result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout,
            LLSDMap("timeout", LLSD::Boolean(true)));

        floater.check();

        if (result.has("timeout"))
        {   // A timeout filed in the result will always be true if present.
            LLStringUtil::format_map_t args;
            args["[OBJECT_NAME]"] = inventory->getName();
            std::string buffer = floater->getString("Timeout", args);
            floater->addStringMessage(buffer);
            return true;
        }

        if (result.has(LLExperienceCache::EXPERIENCE_ID))
        {
            experienceId = result[LLExperienceCache::EXPERIENCE_ID].asUUID();
            if (!floater->hasExperience(experienceId))
            {
                floater->addProcessingMessage("CompileNoExperiencePerm", 
                    LLSDMap("SCRIPT", inventory->getName())
                        ("EXPERIENCE", result[LLExperienceCache::NAME].asString()));
                return true;
            }
        }

    }

    if (!gAssetStorage)
    {
        // viewer likely is shutting down
        return true;
    }

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

        result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout,
            LLSDMap("timeout", LLSD::Boolean(true)));
    }

    if (result.has("timeout"))
    {   // A timeout filed in the result will always be true if present.
        LLStringUtil::format_map_t args;
        args["[OBJECT_NAME]"] = inventory->getName();
        std::string buffer = floater->getString("Timeout", args);
        floater->addStringMessage(buffer);
        return true;
    }

    if (result.has("error"))
    {
        LL_WARNS("SCRIPTQ") << "Inventory fetch returned with error. Code: " << result["error"].asString() << LL_ENDL;
        std::string buffer = result["message"].asString() + " " + inventory->getName();
        floater->addStringMessage(buffer);

        if (result.has("alert"))
        {
            LLSD args;
            args["MESSAGE"] = result["alert"].asString();
            LLNotificationsUtil::add("SystemMessage", args);
        }
        return true;
    }

    LLUUID assetId = result["asset_id"];

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

    result = llcoro::suspendUntilEventOnWithTimeout(pump, fetch_timeout, LLSDMap("timeout", LLSD::Boolean(true)));

    floater.check();

    if (result.has("timeout"))
    { // A timeout filed in the result will always be true if present.
        LLStringUtil::format_map_t args;
        args["[OBJECT_NAME]"] = inventory->getName();
        std::string buffer = floater->getString("Timeout", args);
        floater->addStringMessage(buffer);
        return true;
    }

    // Bytecode save completed
    if (result["compiled"])
    {
        std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" succeeded");

        floater->addStringMessage(buffer);
        LL_INFOS() << buffer << LL_ENDL;
    }
    else
    {
        LLSD compile_errors = result["errors"];
        std::string buffer = std::string("Compilation of \"") + inventory->getName() + std::string("\" failed:");
        floater->addStringMessage(buffer);
        for (LLSD::array_const_iterator line = compile_errors.beginArray();
            line < compile_errors.endArray(); line++)
        {
            std::string str = line->asString();
            str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

            floater->addStringMessage(str);
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

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterResetQueue::resetObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("Resetting") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);
    
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
    // Bind the resetObjectScripts method into a QueueAction function and pass it
    // into the object queue processing coroutine.
    fnQueueAction_t fn = boost::bind(LLFloaterResetQueue::resetObjectScripts,
        getDerivedHandle<LLFloaterScriptQueue>(), _1, _2, _3);

    LLCoros::instance().launch("ScriptResetQueue", boost::bind(LLFloaterScriptQueue::objectScriptProcessingQueueCoro,
        mStartString,
        getDerivedHandle<LLFloaterScriptQueue>(),
        mObjectList,
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

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterRunQueue::runObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater,
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("Running") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);

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
        mObjectList,
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

/// This is a utility function to be bound and called from objectScriptProcessingQueueCoro.
/// Do not call directly. It may throw a LLCheckedHandle<>::Stale exception.
bool LLFloaterNotRunQueue::stopObjectScripts(LLHandle<LLFloaterScriptQueue> hfloater, 
    const LLPointer<LLViewerObject> &object, LLInventoryObject* inventory, LLEventPump &pump)
{
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle.
    // which is caught in objectScriptProcessingQueueCoro

    std::string buffer;
    buffer = floater->getString("NotRunning") + (": ") + inventory->getName();
    floater->addStringMessage(buffer);

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
        mObjectList,
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

    mPump.post(LLSDMap("changed", LLSD::Boolean(true)));

}

void LLFloaterScriptQueue::objectScriptProcessingQueueCoro(std::string action, LLHandle<LLFloaterScriptQueue> hfloater,
    object_data_list_t objectList, fnQueueAction_t func)
{
    LLCoros::set_consuming(true);
    LLCheckedHandle<LLFloaterScriptQueue> floater(hfloater);    
    // Dereferencing floater may fail. If they do they throw LLExeceptionStaleHandle. 
    // This is expected if the dialog closes.
    LLEventMailDrop        maildrop(QUEUE_EVENTPUMP_NAME, true);
    F32 fetch_timeout = gSavedSettings.getF32("QueueInventoryFetchTimeout");


    try
    {
        for (object_data_list_t::iterator itObj(objectList.begin()); (itObj != objectList.end()); ++itObj)
        {
            bool firstForObject = true;
            LLUUID object_id = (*itObj).mObjectId;
            LL_INFOS("SCRIPTQ") << "Next object in queue with ID=" << object_id.asString() << LL_ENDL;

            LLPointer<LLViewerObject> obj = gObjectList.findObject(object_id);
            LLInventoryObject::object_list_t inventory;
            if (obj)
            {
                ObjectInventoryFetcher::ptr_t fetcher(new ObjectInventoryFetcher(maildrop, obj, NULL));

                fetcher->fetchInventory();

                LLStringUtil::format_map_t args;
                args["[OBJECT_NAME]"] = (*itObj).mObjectName;
                floater->addStringMessage(floater->getString("LoadingObjInv", args));

                LLSD result = llcoro::suspendUntilEventOnWithTimeout(maildrop, fetch_timeout,
                    LLSDMap("timeout", LLSD::Boolean(true)));

                if (result.has("timeout"))
                {    // A timeout filed in the result will always be true if present.
                    LL_WARNS("SCRIPTQ") << "Unable to retrieve inventory for object " << object_id.asString() <<
                        ". Skipping to next object." << LL_ENDL;

                    LLStringUtil::format_map_t args;
                    args["[OBJECT_NAME]"] = (*itObj).mObjectName;
                    floater->addStringMessage(floater->getString("Timeout", args));

                    continue;
                }

                inventory.assign(fetcher->getInventoryList().begin(), fetcher->getInventoryList().end());
            }
            else
            {
                LL_WARNS("SCRIPTQ") << "Unable to retrieve object with ID of " << object_id <<
                    ". Skipping to next." << LL_ENDL;
                continue;
            }

            // TODO: Get the name of the object we are looking at here so that we can display it below.
            //std::string objName = (dynamic_cast<LLInventoryObject *>(obj.get()))->getName();
            LL_DEBUGS("SCRIPTQ") << "Object has " << inventory.size() << " items." << LL_ENDL;

            for (LLInventoryObject::object_list_t::iterator itInv = inventory.begin();
                itInv != inventory.end(); ++itInv)
            {
                floater.check();

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

                // no other explicit suspension point in this loop.  func(...) MIGHT suspend
                // but offers no guarantee of doing so.
                llcoro::suspend();
            }
            floater.check();
        }

        floater->addStringMessage("Done");
        floater->getChildView("close")->setEnabled(TRUE);
    }
    catch (LLCheckedHandleBase::Stale &)
    {
        // This is expected.  It means that floater has been closed before 
        // processing was completed.
        LL_DEBUGS("SCRIPTQ") << "LLExeceptionStaleHandle caught! Floater has most likely been closed." << LL_ENDL;
    }
}
