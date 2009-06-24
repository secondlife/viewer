/** 
 * @file llagentwearables.h
 * @brief LLAgentWearables class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLAGENTWEARABLES_H
#define LL_LLAGENTWEARABLES_H

#include "llmemory.h"
#include "lluuid.h"
#include "llinventory.h"
#include "llviewerinventory.h"

class LLInventoryItem;
class LLVOAvatarSelf;
class LLWearable;

class LLAgentWearables
{
	//--------------------------------------------------------------------
	// Constructors / destructors / Initializers
	//--------------------------------------------------------------------
public:
	LLAgentWearables();
	virtual ~LLAgentWearables();
	void 			setAvatarObject(LLVOAvatarSelf *avatar);
	void			createStandardWearables(BOOL female); 
	void			cleanup();
	void			dump();
protected:
	// MULTI-WEARABLE: assuming one per type.  Type is called index - rename.
	void			createStandardWearablesDone(S32 type, U32 index/* = 0*/);
	void			createStandardWearablesAllDone();
	
	//--------------------------------------------------------------------
	// Queries
	//--------------------------------------------------------------------
public:
	BOOL			isWearingItem(const LLUUID& item_id, const BOOL include_linked_items = FALSE) const;
	BOOL			isWearableModifiable(EWearableType type, U32 index /*= 0*/) const;
	BOOL			isWearableCopyable(EWearableType type, U32 index /*= 0*/) const;
	BOOL			areWearablesLoaded() const { return mWearablesLoaded; } 
	
	//--------------------------------------------------------------------
	// Accessors
	//--------------------------------------------------------------------
public:
	const LLUUID&	getWearableItem(EWearableType type, U32 index /*= 0*/) const;
	LLWearable*		getWearableFromWearableItem(const LLUUID& item_id) const;
	LLInventoryItem* getWearableInventoryItem(EWearableType type, U32 index /*= 0*/);
	// MULTI-WEARABLE: assuming one per type.
	static BOOL		selfHasWearable(void* userdata); // userdata is EWearableType
	LLWearable*		getWearable(const EWearableType type, U32 index /*= 0*/); 
	const LLWearable* 	getWearable(const EWearableType type, U32 index /*= 0*/) const;
	U32				getWearableCount(const EWearableType type) const;

private:
	struct LLWearableInv;
	LLWearableInv*	getWearableInv(const EWearableType type, U32 index /*= 0*/); 
	const LLWearableInv* getWearableInv(const EWearableType type, U32 /*index = 0*/) const;
	//--------------------------------------------------------------------
	// Setters
	//--------------------------------------------------------------------
public:
	void			setWearable(LLInventoryItem* new_item, LLWearable* wearable, bool do_append = false);
	void			setWearableOutfit(const LLInventoryItem::item_array_t& items, const LLDynamicArray< LLWearable* >& wearables, BOOL remove);
	void			setWearableName(const LLUUID& item_id, const std::string& new_name);
protected:
	void			setWearableFinal(LLInventoryItem* new_item, LLWearable* new_wearable, bool do_append = false);
	static bool		onSetWearableDialog(const LLSD& notification, const LLSD& response, LLWearable* wearable);

	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
												LLWearable* wearable, 
												const LLUUID& category_id = LLUUID::null,
												BOOL notify = TRUE);
	void 			addWearabletoAgentInventoryDone(const S32 type,
													const U32 index,
													const LLUUID& item_id,
													LLWearable* wearable);
	void			recoverMissingWearable(const EWearableType type, U32 index /*= 0*/);
	void			recoverMissingWearableDone();

	//--------------------------------------------------------------------
	// Removing wearables
	//--------------------------------------------------------------------
public:
	void			removeWearable(const EWearableType type, bool do_remove_all /*= false*/, U32 index /*= 0*/);
private:
	void			removeWearableFinal(const EWearableType type, bool do_remove_all /*= false*/, U32 index /*= 0*/);
protected:
	static bool		onRemoveWearableDialog(const LLSD& notification, const LLSD& response);
	static void		userRemoveAllClothesStep2(BOOL proceed); // userdata is NULL
	
	//--------------------------------------------------------------------
	// Server Communication
	//--------------------------------------------------------------------
public:
	static void		processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data);
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
	void			makeNewOutfit(const std::string& new_folder_name,
								  const LLDynamicArray<S32>& wearables_to_include,
								  const LLDynamicArray<S32>& attachments_to_include,
								  BOOL rename_clothing);protected:
private:
	void			makeNewOutfitDone(S32 type, U32 index); 

	//--------------------------------------------------------------------
	// Save Wearables
	//--------------------------------------------------------------------
public:	
    // MULTI-WEARABLE: assumes one per type.
	void			saveWearableAs(const EWearableType type, const U32 index, const std::string& new_name, BOOL save_in_lost_and_found);
	void			saveWearable(const EWearableType type, const U32 index, BOOL send_update = TRUE);
	void			saveAllWearables();
	void			revertWearable(const EWearableType type, const U32 index);

	//--------------------------------------------------------------------
	// Static UI hooks
	//--------------------------------------------------------------------
public:
	// MULTI-WEARABLE: assuming one wearable per type.  Need upstream changes.
	static void		userRemoveWearable(void* userdata);	// userdata is EWearableType
	static void		userRemoveAllClothes(void* userdata);	// userdata is NULL
	static void		userRemoveAllAttachments(void* userdata);	// userdata is NULL 

	//--------------------------------------------------------------------
	// Member variables
	//--------------------------------------------------------------------
private:
	typedef std::vector<LLWearableInv*> wearableentry_vec_t; // all wearables of a certain type (EG all shirts)
	typedef std::map<EWearableType, wearableentry_vec_t> wearableentry_map_t;	// wearable "categories" arranged by wearable type
	wearableentry_map_t mWearableDatas;

	static BOOL		mInitialWearablesUpdateReceived;
	BOOL			mWearablesLoaded;
	LLPointer<LLVOAvatarSelf> mAvatarObject; // NULL until avatar object sent down from simulator
	
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
		enum EType
		{
			CALL_NONE = 0,
			CALL_UPDATE = 1,
			CALL_RECOVERDONE = 2,
			CALL_CREATESTANDARDDONE = 4,
			CALL_MAKENEWOUTFITDONE = 8
		};

		// MULTI-WEARABLE: index is an EWearableType - more confusing usage.
		// MULTI-WEARABLE: need to have type and index args both?
		addWearableToAgentInventoryCallback(LLPointer<LLRefCount> cb,
											S32 type,
											U32 index,
											LLWearable* wearable,
											U32 todo = CALL_NONE);
		virtual void fire(const LLUUID& inv_item);
	private:
		S32 mType;
		U32 mIndex;
		LLWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
	};

	struct LLWearableInv // Make this subclass of llwearable? 
	{
		LLWearableInv() : mItemID(LLUUID::null), mWearable(NULL) {}
		// BOOL exists() const;
		LLUUID		mItemID; // ID of the inventory item in the agent's inventory.
		LLWearable*	mWearable;
	};
	const static LLWearableInv s_null_wearable;

}; // LLAgentWearables

extern LLAgentWearables gAgentWearables;

//--------------------------------------------------------------------
// Types
//--------------------------------------------------------------------	

#endif // LL_AGENTWEARABLES_H
