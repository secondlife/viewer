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
#include "llavatarappearancedefines.h"
#include "llwearabledata.h"
#include "llinitdestroyclass.h"

class LLInventoryItem;
class LLVOAvatarSelf;
class LLViewerWearable;
class LLViewerObject;

class LLAgentWearables : public LLInitClass<LLAgentWearables>, public LLWearableData
{
	//--------------------------------------------------------------------
	// Constructors / destructors / Initializers
	//--------------------------------------------------------------------
public:

	LLAgentWearables();
	virtual ~LLAgentWearables();
	void 			setAvatarObject(LLVOAvatarSelf *avatar);
	void			createStandardWearables(); 
	void			cleanup();
	void			dump();

	// LLInitClass interface
	static void initClass();
	
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
	F32				getCOFChangeTime() const { return mCOFChangeTimer.getElapsedTimeF32(); }
	void			updateWearablesLoaded();
	void			checkWearablesLoaded() const;
	bool			canMoveWearable(const LLUUID& item_id, bool closer_to_body) const;
	
	// Note: False for shape, skin, eyes, and hair, unless you have MORE than 1.
	bool			canWearableBeRemoved(const LLViewerWearable* wearable) const;

	void			animateAllWearableParams(F32 delta);

	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID		getWearableItemID(LLWearableType::EType type, U32 index /*= 0*/) const;
	const LLUUID		getWearableAssetID(LLWearableType::EType type, U32 index /*= 0*/) const;
	const LLViewerWearable*	getWearableFromItemID(const LLUUID& item_id) const;
	LLViewerWearable*	getWearableFromItemID(const LLUUID& item_id);
	LLViewerWearable*	getWearableFromAssetID(const LLUUID& asset_id);
	LLViewerWearable*		getViewerWearable(const LLWearableType::EType type, U32 index /*= 0*/); 
	const LLViewerWearable*	getViewerWearable(const LLWearableType::EType type, U32 index /*= 0*/) const;
	LLInventoryItem*	getWearableInventoryItem(LLWearableType::EType type, U32 index /*= 0*/);
	static BOOL			selfHasWearable(LLWearableType::EType type);

	//--------------------------------------------------------------------
	// Setters
	//--------------------------------------------------------------------
private:
	/*virtual*/void	wearableUpdated(LLWearable *wearable, BOOL removed);
public:
	void			setWearableItem(LLInventoryItem* new_item, LLViewerWearable* wearable, bool do_append = false);
	void			setWearableOutfit(const LLInventoryItem::item_array_t& items, const std::vector< LLViewerWearable* >& wearables);
	void			setWearableName(const LLUUID& item_id, const std::string& new_name);
	// *TODO: Move this into llappearance/LLWearableData ?
	void			addLocalTextureObject(const LLWearableType::EType wearable_type, const LLAvatarAppearanceDefines::ETextureIndex texture_type, U32 wearable_index);

protected:
	void			setWearableFinal(LLInventoryItem* new_item, LLViewerWearable* new_wearable, bool do_append = false);
	static bool		onSetWearableDialog(const LLSD& notification, const LLSD& response, LLViewerWearable* wearable);

	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												LLViewerWearable* wearable, 
												const LLUUID& category_id = LLUUID::null,
												BOOL notify = TRUE);
	void 			addWearabletoAgentInventoryDone(const LLWearableType::EType type,
													const U32 index,
													const LLUUID& item_id,
													LLViewerWearable* wearable);
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
	// Outfits
	//--------------------------------------------------------------------
private:
	void			makeNewOutfitDone(S32 type, U32 index); 

	//--------------------------------------------------------------------
	// Save Wearables
	//--------------------------------------------------------------------
public:	
	void			saveWearableAs(const LLWearableType::EType type, const U32 index, const std::string& new_name, const std::string& description, BOOL save_in_lost_and_found);
	void			saveWearable(const LLWearableType::EType type, const U32 index,
								 const std::string new_name = "");
	void			saveAllWearables();
	void			revertWearable(const LLWearableType::EType type, const U32 index);

	// We no longer need this message in the current viewer, but send
	// it for now to maintain compatibility with release viewers. Can
	// remove this function once the SH-3455 changesets are universally deployed.
	void			sendDummyAgentWearablesUpdate();

	//--------------------------------------------------------------------
	// Static UI hooks
	//--------------------------------------------------------------------
public:
	static void		userRemoveWearable(const LLWearableType::EType &type, const U32 &index);
	static void		userRemoveWearablesOfType(const LLWearableType::EType &type);
	
	typedef std::vector<LLViewerObject*> llvo_vec_t;

	static void     findAttachmentsAddRemoveInfo(LLInventoryModel::item_array_t& obj_item_array,
												 llvo_vec_t& objects_to_remove,
												 llvo_vec_t& objects_to_retain,
												 LLInventoryModel::item_array_t& items_to_add);
	static void		userRemoveMultipleAttachments(llvo_vec_t& llvo_array);
	static void		userAttachMultipleAttachments(LLInventoryModel::item_array_t& obj_item_array);

	static llvo_vec_t getTempAttachments();

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

	bool									changeInProgress() const;
	void									notifyLoadingStarted();
	void									notifyLoadingFinished();

private:
	loading_started_signal_t				mLoadingStartedSignal; // should be called before wearables are changed
	loaded_signal_t							mLoadedSignal; // emitted when all agent wearables get loaded

	//--------------------------------------------------------------------
	// Member variables
	//--------------------------------------------------------------------
private:
	static BOOL		mInitialWearablesUpdateReceived;
	BOOL			mWearablesLoaded;

	/**
	 * True if agent's outfit is being changed now.
	 */
	BOOL			mCOFChangeInProgress;
	LLTimer			mCOFChangeTimer;
	
	//--------------------------------------------------------------------------------
	// Support classes
	//--------------------------------------------------------------------------------
private:
	class AddWearableToAgentInventoryCallback : public LLInventoryCallback
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

		AddWearableToAgentInventoryCallback(LLPointer<LLRefCount> cb,
											LLWearableType::EType type,
											U32 index,
											LLViewerWearable* wearable,
											U32 todo = CALL_NONE,
											const std::string description = "");
		virtual void fire(const LLUUID& inv_item);
	private:
		LLWearableType::EType mType;
		U32 mIndex;
		LLViewerWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
		std::string mDescription;
	};

}; // LLAgentWearables

extern LLAgentWearables gAgentWearables;

//--------------------------------------------------------------------
// Types
//--------------------------------------------------------------------	

#endif // LL_AGENTWEARABLES_H
