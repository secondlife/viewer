/** 
 * @file llagentwearables.h
 * @brief LLAgentWearables class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLAGENTWEARABLES_H
#define LL_LLAGENTWEARABLES_H

// libraries
#include "llmemory.h"
#include "llui.h"
#include "lluuid.h"
#include "llinventory.h"

// newview
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llvoavatardefines.h"

class LLInventoryItem;
class LLVOAvatarSelf;
class LLWearable;
class LLInitialWearablesFetch;
class LLViewerObject;
class LLTexLayerTemplate;

class LLAgentWearables : public LLInitClass<LLAgentWearables>
{
	//--------------------------------------------------------------------
	// Constructors / destructors / Initializers
	//--------------------------------------------------------------------
public:
	friend class LLInitialWearablesFetch;

	LLAgentWearables();
	virtual ~LLAgentWearables();
	void 			setAvatarObject(LLVOAvatarSelf *avatar);
	void			createStandardWearables(); 
	void			cleanup();
	void			dump();

	// LLInitClass interface
	static void initClass();
protected:
	void			createStandardWearablesDone(S32 type, U32 index/* = 0*/);
	void			createStandardWearablesAllDone();
	
	//--------------------------------------------------------------------
	// Queries
	//--------------------------------------------------------------------
public:
	BOOL			isWearingItem(const LLUUID& item_id) const;
	BOOL			isWearableModifiable(LLWearableType::EType type, U32 index /*= 0*/) const;
	BOOL			isWearableModifiable(const LLUUID& item_id) const;

	BOOL			isWearableCopyable(LLWearableType::EType type, U32 index /*= 0*/) const;
	BOOL			areWearablesLoaded() const;
	bool			isCOFChangeInProgress() const { return mCOFChangeInProgress; }
	void			updateWearablesLoaded();
	void			checkWearablesLoaded() const;
	bool			canMoveWearable(const LLUUID& item_id, bool closer_to_body);
	
	// Note: False for shape, skin, eyes, and hair, unless you have MORE than 1.
	bool			canWearableBeRemoved(const LLWearable* wearable) const;

	void			animateAllWearableParams(F32 delta, BOOL upload_bake);

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID		getWearableItemID(LLWearableType::EType type, U32 index /*= 0*/) const;
	const LLUUID		getWearableAssetID(LLWearableType::EType type, U32 index /*= 0*/) const;
	const LLWearable*	getWearableFromItemID(const LLUUID& item_id) const;
	LLWearable*	getWearableFromItemID(const LLUUID& item_id);
	LLWearable*	getWearableFromAssetID(const LLUUID& asset_id);
	LLInventoryItem*	getWearableInventoryItem(LLWearableType::EType type, U32 index /*= 0*/);
	static BOOL			selfHasWearable(LLWearableType::EType type);
	LLWearable*			getWearable(const LLWearableType::EType type, U32 index /*= 0*/); 
	const LLWearable* 	getWearable(const LLWearableType::EType type, U32 index /*= 0*/) const;
	LLWearable*		getTopWearable(const LLWearableType::EType type);
	LLWearable*		getBottomWearable(const LLWearableType::EType type);
	U32				getWearableCount(const LLWearableType::EType type) const;
	U32				getWearableCount(const U32 tex_index) const;

	static const U32 MAX_CLOTHING_PER_TYPE = 5; 


	//--------------------------------------------------------------------
	// Setters
	//--------------------------------------------------------------------

private:
	// Low-level data structure setter - public access is via setWearableItem, etc.
	void 			setWearable(const LLWearableType::EType type, U32 index, LLWearable *wearable);
	U32 			pushWearable(const LLWearableType::EType type, LLWearable *wearable);
	void			wearableUpdated(LLWearable *wearable);
	void 			popWearable(LLWearable *wearable);
	void			popWearable(const LLWearableType::EType type, U32 index);
	
public:
	void			setWearableItem(LLInventoryItem* new_item, LLWearable* wearable, bool do_append = false);
	void			setWearableOutfit(const LLInventoryItem::item_array_t& items, const LLDynamicArray< LLWearable* >& wearables, BOOL remove);
	void			setWearableName(const LLUUID& item_id, const std::string& new_name);
	void			addLocalTextureObject(const LLWearableType::EType wearable_type, const LLVOAvatarDefines::ETextureIndex texture_type, U32 wearable_index);
	U32				getWearableIndex(const LLWearable *wearable) const;

protected:
	void			setWearableFinal(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append = false);
	static bool		onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable);

	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												LLWearable* wearable, 
												const LLUUID& category_id = LLUUID::null,
												BOOL notify = TRUE);
	void 			addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													const U32 index,
													const LLUUID& item_id,
													LLWearable* wearable);
	void			recoverMissingWearable(const LLWearableType::EType type, U32 index /*= 0*/);
	void			recoverMissingWearableDone();

	//--------------------------------------------------------------------
	// Editing/moving wearables
	//--------------------------------------------------------------------

public:
	static void		createWearable(LLWearableType::EType type, bool wear = false, const LLUUID& parent_id = LLUUID::null);
	static void		editWearable(const LLUUID& item_id);
	bool			moveWearable(const LLViewerInventoryItem* item, bool closer_to_body);

	void			requestEditingWearable(const LLUUID& item_id);
	void			editWearableIfRequested(const LLUUID& item_id);

private:
	LLUUID			mItemToEdit;

	//--------------------------------------------------------------------
	// Removing wearables
	//--------------------------------------------------------------------
public:
	void			removeWearable(const LLWearableType::EType type, bool do_remove_all /*= false*/, U32 index /*= 0*/);
private:
	void			removeWearableFinal(const LLWearableType::EType type, bool do_remove_all /*= false*/, U32 index /*= 0*/);
protected:
	static bool		onRemoveWearableDialog(const LLSD& notification, const LLSD& response);
	
	//--------------------------------------------------------------------
	// Server Communication
	//--------------------------------------------------------------------
public:
	// Processes the initial wearables update message (if necessary, since the outfit folder makes it redundant)
	static void		processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data);
	LLUUID			computeBakedTextureHash(LLVOAvatarDefines::EBakedTextureIndex baked_index,
											BOOL generate_valid_hash = TRUE);

protected:
	void			sendAgentWearablesUpdate();
	void			sendAgentWearablesRequest();
	void			queryWearableCache();
	void 			updateServer();
	static void		onInitialWearableAssetArrived(LLWearable* wearable, void* userdata);

	//--------------------------------------------------------------------
	// Outfits
	//--------------------------------------------------------------------
public:
	
	// Should only be called if we *know* we've never done so before, since users may
	// not want the Library outfits to stay in their quick outfit selector and can delete them.
	void			populateMyOutfitsFolder();

private:
	void			makeNewOutfitDone(S32 type, U32 index); 

	//--------------------------------------------------------------------
	// Save Wearables
	//--------------------------------------------------------------------
public:	
	void			saveWearableAs(const LLWearableType::EType type, const U32 index, const std::string& new_name, BOOL save_in_lost_and_found);
	void			saveWearable(const LLWearableType::EType type, const U32 index, BOOL send_update = TRUE,
								 const std::string new_name = "");
	void			saveAllWearables();
	void			revertWearable(const LLWearableType::EType type, const U32 index);

	//--------------------------------------------------------------------
	// Static UI hooks
	//--------------------------------------------------------------------
public:
	static void		userRemoveWearable(const LLWearableType::EType &type, const U32 &index);
	static void		userRemoveWearablesOfType(const LLWearableType::EType &type);
	
	typedef std::vector<LLViewerObject*> llvo_vec_t;

	static void 	userUpdateAttachments(LLInventoryModel::item_array_t& obj_item_array);
	static void		userRemoveMultipleAttachments(llvo_vec_t& llvo_array);
	static void		userRemoveAllAttachments();
	static void		userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array);

	BOOL			itemUpdatePending(const LLUUID& item_id) const;
	U32				itemUpdatePendingCount() const;

	//--------------------------------------------------------------------
	// Signals
	//--------------------------------------------------------------------
public:
	typedef boost::function<void()>			loading_started_callback_t;
	typedef boost::signals2::signal<void()>	loading_started_signal_t;
	boost::signals2::connection				addLoadingStartedCallback(loading_started_callback_t cb);

	typedef boost::function<void()>			loaded_callback_t;
	typedef boost::signals2::signal<void()>	loaded_signal_t;
	boost::signals2::connection				addLoadedCallback(loaded_callback_t cb);

	void									notifyLoadingStarted();
	void									notifyLoadingFinished();

private:
	loading_started_signal_t				mLoadingStartedSignal; // should be called before wearables are changed
	loaded_signal_t							mLoadedSignal; // emitted when all agent wearables get loaded

	//--------------------------------------------------------------------
	// Member variables
	//--------------------------------------------------------------------
private:
	typedef std::vector<LLWearable*> wearableentry_vec_t; // all wearables of a certain type (EG all shirts)
	typedef std::map<LLWearableType::EType, wearableentry_vec_t> wearableentry_map_t;	// wearable "categories" arranged by wearable type
	wearableentry_map_t mWearableDatas;

	static BOOL		mInitialWearablesUpdateReceived;
	BOOL			mWearablesLoaded;
	std::set<LLUUID>	mItemsAwaitingWearableUpdate;

	/**
	 * True if agent's outfit is being changed now.
	 */
	BOOL			mCOFChangeInProgress;
	
	//--------------------------------------------------------------------------------
	// Support classes
	//--------------------------------------------------------------------------------
private:
	class createStandardWearablesAllDoneCallback : public LLRefCount
	{
	protected:
		~createStandardWearablesAllDoneCallback();
	};
	class sendAgentWearablesUpdateCallback : public LLRefCount
	{
	protected:
		~sendAgentWearablesUpdateCallback();
	};

	class addWearableToAgentInventoryCallback : public LLInventoryCallback
	{
	public:
		enum ETodo
		{
			CALL_NONE = 0,
			CALL_UPDATE = 1,
			CALL_RECOVERDONE = 2,
			CALL_CREATESTANDARDDONE = 4,
			CALL_MAKENEWOUTFITDONE = 8,
			CALL_WEARITEM = 16
		};

		addWearableToAgentInventoryCallback(LLPointer<LLRefCount> cb,
											LLWearableType::EType type,
											U32 index,
											LLWearable* wearable,
											U32 todo = CALL_NONE);
		virtual void fire(const LLUUID& inv_item);
	private:
		LLWearableType::EType mType;
		U32 mIndex;
		LLWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
	};

}; // LLAgentWearables

extern LLAgentWearables gAgentWearables;

//--------------------------------------------------------------------
// Types
//--------------------------------------------------------------------	

#endif // LL_AGENTWEARABLES_H
