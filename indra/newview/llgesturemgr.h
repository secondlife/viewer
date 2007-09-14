/** 
 * @file llgesturemgr.h
 * @brief Manager for playing gestures on the viewer
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLGESTUREMGR_H
#define LL_LLGESTUREMGR_H

#include <map>
#include <string>
#include <vector>

#include "llassetstorage.h"	// LLAssetType
#include "llviewerinventory.h"

class LLMultiGesture;
class LLGestureStep;
class LLUUID;
class LLVFS;

class LLGestureManagerObserver
{
public:
	virtual ~LLGestureManagerObserver() { };
	virtual void changed() = 0;
};

class LLGestureManager
{
public:
	LLGestureManager();
	~LLGestureManager();

	void init();

	// Call once per frame to manage gestures
	void update();

	// Loads a gesture out of inventory into the in-memory active form
	// Note that the inventory item must exist, so we can look up the 
	// asset id.
	void activateGesture(const LLUUID& item_id);

	// Activate a list of gestures
	void activateGestures(LLViewerInventoryItem::item_array_t& items);

	// If you change a gesture, you need to build a new multigesture
	// and call this method.
	void replaceGesture(const LLUUID& item_id, LLMultiGesture* new_gesture, const LLUUID& asset_id);
	void replaceGesture(const LLUUID& item_id, const LLUUID& asset_id);

	// Load gesture into in-memory active form.
	// Can be called even if the inventory item isn't loaded yet.
	// inform_server TRUE will send message upstream to update database
	// user_gesture_active table, which isn't necessary on login.
	// deactivate_similar will cause other gestures with the same trigger phrase
	// or keybinding to be deactivated.
	void activateGestureWithAsset(const LLUUID& item_id, const LLUUID& asset_id, BOOL inform_server, BOOL deactivate_similar);

	// Takes gesture out of active list and deletes it.
	void deactivateGesture(const LLUUID& item_id);

	// Deactivates all gestures that match either this trigger phrase,
	// or this hot key.
	void deactivateSimilarGestures(LLMultiGesture* gesture, const LLUUID& in_item_id);

	BOOL isGestureActive(const LLUUID& item_id);

	BOOL isGesturePlaying(const LLUUID& item_id);

	// Force a gesture to be played, for example, if it is being
	// previewed.
	void playGesture(LLMultiGesture* gesture);
	void playGesture(const LLUUID& item_id);

	// Stop all requested or playing anims for this gesture
	// Also remove from playing list
	void stopGesture(LLMultiGesture* gesture);
	void stopGesture(const LLUUID& item_id);

	// Trigger the first gesture that matches this key.
	// Returns TRUE if it finds a gesture bound to that key.
	BOOL triggerGesture(KEY key, MASK mask);

	// Trigger all gestures referenced as substrings in this string
	BOOL triggerAndReviseString(const std::string &str, std::string *revised_string = NULL);

	// Does some gesture have this key bound?
	BOOL isKeyBound(KEY key, MASK mask);

	S32 getPlayingCount() const;

	void addObserver(LLGestureManagerObserver* observer);
	void removeObserver(LLGestureManagerObserver* observer);
	void notifyObservers();

	BOOL matchPrefix(const std::string& in_str, std::string* out_str);

	// Copy item ids into the vector
	void getItemIDs(std::vector<LLUUID>* ids);

protected:
	// Handle the processing of a single gesture
	void stepGesture(LLMultiGesture* gesture);

	// Do a single step in a gesture
	void runStep(LLMultiGesture* gesture, LLGestureStep* step);

	// Used by loadGesture
	static void onLoadComplete(LLVFS *vfs,
						   const LLUUID& asset_uuid,
						   LLAssetType::EType type,
						   void* user_data, S32 status, LLExtStat ext_status);

public:
	BOOL mValid;
	std::vector<LLMultiGesture*> mPlaying;

	// Maps inventory item_id to gesture
	typedef std::map<LLUUID, LLMultiGesture*> item_map_t;

	// Active gestures.
	// NOTE: The gesture pointer CAN BE NULL.  This means that
	// there is a gesture with that item_id, but the asset data
	// is still on its way down from the server.
	item_map_t mActive;

	S32 mLoadingCount;
	std::string mDeactivateSimilarNames;

	std::vector<LLGestureManagerObserver*> mObservers;
};

extern LLGestureManager gGestureManager;

#endif
