/** 
 * @file llcompilequeue.h
 * @brief LLCompileQueue class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCOMPILEQUEUE_H
#define LL_LLCOMPILEQUEUE_H

#include "lldarray.h"
#include "llinventory.h"
#include "llviewerobject.h"
#include "llvoinventorylistener.h"
#include "llmap.h"
#include "lluuid.h"

#include "llfloater.h"
#include "llscrolllistctrl.h"

#include "llviewerinventory.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterScriptQueue
//
// This class provides a mechanism of adding objects to a list that
// will go through and execute action for the scripts on each object. The
// objects will be accessed serially and the scripts may be
// manipulated in parallel. For example, selecting two objects each
// with three scripts will result in the first object having all three
// scripts manipulated.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterScriptQueue : public LLFloater, public LLVOInventoryListener
{
public:
	// addObject() accepts an object id.
	void addObject(const LLUUID& id);

	// start() returns TRUE if the queue has started, otherwise FALSE.
	BOOL start();

protected:
	LLFloaterScriptQueue(const std::string& name, const LLRect& rect,
						 const char* title, const char* start_string);
	virtual ~LLFloaterScriptQueue();

	// This is the callback method for the viewer object currently
	// being worked on.
	/*virtual*/ void inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* queue);
	
	// This is called by inventoryChanged
	virtual void handleInventory(LLViewerObject* viewer_obj,
								InventoryObjectList* inv) = 0;

	static void onCloseBtn(void* user_data);

	// returns true if this is done
	BOOL isDone() const;

	// go to the next object. If no objects left, it falls out
	// silently and waits to be killed by the deleteIfDone() callback.
	BOOL nextObject();
	BOOL popNext();

	// Get this instances ID.
	const LLUUID& getID() const { return mID; } 

	// find an instance by ID. Return NULL if it does not exist.
	static LLFloaterScriptQueue* findInstance(const LLUUID& id);
	
protected:
	// UI
	LLScrollListCtrl* mMessages;
	LLButton* mCloseBtn;

	// Object Queue
	LLDynamicArray<LLUUID> mObjectIDs;
	LLUUID mCurrentObjectID;
	BOOL mDone;

	LLUUID mID;
	static LLMap<LLUUID, LLFloaterScriptQueue*> sInstances;

	const char* mStartString;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterCompileQueue
//
// This script queue will recompile each script.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterCompileQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a compile queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterCompileQueue* create();

protected:
	LLFloaterCompileQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterCompileQueue();
	
	// This is called by inventoryChanged
	virtual void handleInventory(LLViewerObject* viewer_obj,
								InventoryObjectList* inv);

	// This is the callback for when each script arrives
	static void scriptArrived(LLVFS *vfs, const LLUUID& asset_id,
								LLAssetType::EType type,
								void* user_data, S32 status, LLExtStat ext_status);

	static void onSaveTextComplete(const LLUUID& asset_id, void* user_data, S32 status, LLExtStat ext_status);
	static void onSaveBytecodeComplete(const LLUUID& asset_id,
									   void* user_data,
									   S32 status, LLExtStat ext_status);

	// compile the file given and save it out.
	void compile(const char* filename, const LLUUID& asset_id);
	
	// remove any object in mScriptScripts with the matching uuid.
	void removeItemByAssetID(const LLUUID& asset_id);

	// save the items indicatd by the asset id.
	void saveItemByAssetID(const LLUUID& asset_id);

	// find old_asst_id, and set the asset id to new_asset_id
	void updateAssetID(const LLUUID& old_asset_id, const LLUUID& new_asset_id);

protected:
	LLViewerInventoryItem::item_array_t mCurrentScripts;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterResetQueue
//
// This script queue will reset each script.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterResetQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a reset queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterResetQueue* create();

protected:
	LLFloaterResetQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterResetQueue();
	
	// This is called by inventoryChanged
	virtual void handleInventory(LLViewerObject* viewer_obj,
								InventoryObjectList* inv);

protected:
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterRunQueue
//
// This script queue will set each script as running.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterRunQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a run queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterRunQueue* create();

protected:
	LLFloaterRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterRunQueue();
	
	// This is called by inventoryChanged
	virtual void handleInventory(LLViewerObject* viewer_obj,
								InventoryObjectList* inv);

protected:
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterNotRunQueue
//
// This script queue will set each script as not running.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterNotRunQueue : public LLFloaterScriptQueue
{
public:
	// Use this method to create a not run queue. Once created, it
	// will be responsible for it's own destruction.
	static LLFloaterNotRunQueue* create();

protected:
	LLFloaterNotRunQueue(const std::string& name, const LLRect& rect);
	virtual ~LLFloaterNotRunQueue();
	
	// This is called by inventoryChanged
	virtual void handleInventory(LLViewerObject* viewer_obj,
								InventoryObjectList* inv);

protected:
};

#endif // LL_LLCOMPILEQUEUE_H
