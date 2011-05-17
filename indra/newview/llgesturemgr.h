/** 
 * @file llgesturemgr.h
 * @brief Manager for playing gestures on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLGESTUREMGR_H
#define LL_LLGESTUREMGR_H

#include <map>
#include <string>
#include <vector>

#include "llassetstorage.h"	// LLAssetType
#include "llinventoryobserver.h"
#include "llsingleton.h"
#include "llviewerinventory.h"

class LLMultiGesture;
class LLGestureListener;
class LLGestureStep;
class LLUUID;
class LLVFS;

class LLGestureManagerObserver
{
public:
	virtual ~LLGestureManagerObserver() { };
	virtual void changed() = 0;
};

class LLGestureMgr : public LLSingleton<LLGestureMgr>, public LLInventoryFetchItemsObserver
{
public:

	typedef boost::function<void (LLMultiGesture* loaded_gesture)> gesture_loaded_callback_t;
	// Maps inventory item_id to gesture
	typedef std::map<LLUUID, LLMultiGesture*> item_map_t;
	typedef std::map<LLUUID, gesture_loaded_callback_t> callback_map_t;

	LLGestureMgr();
	~LLGestureMgr();

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

	BOOL isGesturePlaying(LLMultiGesture* gesture);

	const item_map_t& getActiveGestures() const { return mActive; }
	// Force a gesture to be played, for example, if it is being
	// previewed.
	void playGesture(LLMultiGesture* gesture);
	void playGesture(const LLUUID& item_id);

	// Stop all requested or playing anims for this gesture
	// Also remove from playing list
	void stopGesture(LLMultiGesture* gesture);
	void stopGesture(const LLUUID& item_id);
	/**
	 * Add cb into callbackMap.
	 * Note:
	 * Manager will call cb after gesture will be loaded and will remove cb automatically. 
	 */
	void setGestureLoadedCallback(LLUUID inv_item_id, gesture_loaded_callback_t cb)
	{
		mCallbackMap[inv_item_id] = cb;
	}
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

	// Overriding so we can update active gesture names and notify observers 
	void changed(U32 mask); 

	BOOL matchPrefix(const std::string& in_str, std::string* out_str);

	// Copy item ids into the vector
	void getItemIDs(uuid_vec_t* ids);

protected:
	// Handle the processing of a single gesture
	void stepGesture(LLMultiGesture* gesture);

	// Do a single step in a gesture
	void runStep(LLMultiGesture* gesture, LLGestureStep* step);

	// LLInventoryCompletionObserver trigger
	void done();

	// Used by loadGesture
	static void onLoadComplete(LLVFS *vfs,
						   const LLUUID& asset_uuid,
						   LLAssetType::EType type,
						   void* user_data, S32 status, LLExtStat ext_status);

	// Used by playGesture to load an asset file
	// required to play a gesture step
	static void onAssetLoadComplete(LLVFS *vfs,
									const LLUUID& asset_uuid,
									LLAssetType::EType type,
									void* user_data, S32 status, LLExtStat ext_status);

	// Checks whether all animation and sound assets
	// needed to play a gesture are loaded.
	static bool hasLoadingAssets(LLMultiGesture* gesture);

private:
	// Active gestures.
	// NOTE: The gesture pointer CAN BE NULL.  This means that
	// there is a gesture with that item_id, but the asset data
	// is still on its way down from the server.
	item_map_t mActive;

	S32 mLoadingCount;
	std::string mDeactivateSimilarNames;
	
	std::vector<LLGestureManagerObserver*> mObservers;
	callback_map_t mCallbackMap;
	std::vector<LLMultiGesture*> mPlaying;	
	BOOL mValid;

	std::set<LLUUID> mLoadingAssets;

	// LLEventHost interface
	boost::shared_ptr<LLGestureListener> mListener;
};

#endif
