/** 
 * @file lltooldraganddrop.h
 * @brief LLToolDragAndDrop class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_TOOLDRAGANDDROP_H
#define LL_TOOLDRAGANDDROP_H

#include "lldictionary.h"
#include "lltool.h"
#include "llview.h"
#include "lluuid.h"
#include "stdenums.h"
#include "llassetstorage.h"
#include "lldarray.h"
#include "llpermissions.h"
#include "llwindow.h"
#include "llviewerinventory.h"

class LLToolDragAndDrop;
class LLViewerRegion;
class LLVOAvatar;
class LLPickInfo;

class LLToolDragAndDrop : public LLTool, public LLSingleton<LLToolDragAndDrop>
{
public:
	typedef boost::signals2::signal<void ()> enddrag_signal_t;

	LLToolDragAndDrop();

	// overridden from LLTool
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKey(KEY key, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, MASK mask);
	virtual void	onMouseCaptureLost();
	virtual void	handleDeselect();

	void			setDragStart( S32 x, S32 y );			// In screen space
	BOOL			isOverThreshold( S32 x, S32 y );		// In screen space

	enum ESource
	{
		SOURCE_AGENT,
		SOURCE_WORLD,
		SOURCE_NOTECARD,
		SOURCE_LIBRARY
	};

	void beginDrag(EDragAndDropType type,
				   const LLUUID& cargo_id,
				   ESource source,
				   const LLUUID& source_id = LLUUID::null,
				   const LLUUID& object_id = LLUUID::null);
	void beginMultiDrag(const std::vector<EDragAndDropType> types,
						const std::vector<LLUUID>& cargo_ids,
						ESource source,
						const LLUUID& source_id = LLUUID::null);
	void endDrag();
	ESource getSource() const { return mSource; }
	const LLUUID& getSourceID() const { return mSourceID; }
	const LLUUID& getObjectID() const { return mObjectID; }
	EAcceptance getLastAccept() { return mLastAccept; }

	boost::signals2::connection setEndDragCallback( const enddrag_signal_t::slot_type& cb ) { return mEndDragSignal.connect(cb); }

protected:
	enum EDropTarget
	{
		DT_NONE = 0,
		DT_SELF = 1,
		DT_AVATAR = 2,
		DT_OBJECT = 3,
		DT_LAND = 4,
		DT_COUNT = 5
	};

protected:
	// dragOrDrop3dImpl points to a member of LLToolDragAndDrop that
	// takes parameters (LLViewerObject* obj, S32 face, MASK, BOOL
	// drop) and returns a BOOL if drop is ok
	typedef EAcceptance (LLToolDragAndDrop::*dragOrDrop3dImpl)
		(LLViewerObject*, S32, MASK, BOOL);

	void dragOrDrop(S32 x, S32 y, MASK mask, BOOL drop,
					EAcceptance* acceptance);
	void dragOrDrop3D(S32 x, S32 y, MASK mask, BOOL drop,
					  EAcceptance* acceptance);
	
	static void pickCallback(const LLPickInfo& pick_info);
	void pick(const LLPickInfo& pick_info);

protected:

	S32				mDragStartX;
	S32				mDragStartY;
	
	std::vector<EDragAndDropType> mCargoTypes;
	//void*			mCargoData;
	std::vector<LLUUID> mCargoIDs;
	ESource mSource;
	LLUUID mSourceID;
	LLUUID mObjectID;

	LLVector3d		mLastCameraPos;
	LLVector3d		mLastHitPos;

	ECursorType		mCursor;
	EAcceptance		mLastAccept;
	BOOL			mDrop;
	S32				mCurItemIndex;
	std::string		mToolTipMsg;

	enddrag_signal_t	mEndDragSignal;

protected:
	// 3d drop functions. these call down into the static functions
	// named drop<ThingToDrop> if drop is TRUE and permissions allow
	// that behavior.
	EAcceptance dad3dNULL(LLViewerObject*, S32, MASK, BOOL);
	EAcceptance dad3dRezObjectOnLand(LLViewerObject* obj, S32 face,
									 MASK mask, BOOL drop);
	EAcceptance dad3dRezObjectOnObject(LLViewerObject* obj, S32 face,
									   MASK mask, BOOL drop);
	EAcceptance dad3dRezScript(LLViewerObject* obj, S32 face,
							   MASK mask, BOOL drop);
	EAcceptance dad3dTextureObject(LLViewerObject* obj, S32 face,
								   MASK mask, BOOL drop);
//	EAcceptance dad3dTextureSelf(LLViewerObject* obj, S32 face,
//								 MASK mask, BOOL drop);
	EAcceptance dad3dWearItem(LLViewerObject* obj, S32 face,
								 MASK mask, BOOL drop);
	EAcceptance dad3dWearCategory(LLViewerObject* obj, S32 face,
								 MASK mask, BOOL drop);
	EAcceptance dad3dUpdateInventory(LLViewerObject* obj, S32 face,
									 MASK mask, BOOL drop);
	EAcceptance dad3dUpdateInventoryCategory(LLViewerObject* obj,
											 S32 face,
											 MASK mask,
											 BOOL drop);
	EAcceptance dad3dGiveInventoryObject(LLViewerObject* obj, S32 face,
								   MASK mask, BOOL drop);
	EAcceptance dad3dGiveInventory(LLViewerObject* obj, S32 face,
								   MASK mask, BOOL drop);
	EAcceptance dad3dGiveInventoryCategory(LLViewerObject* obj, S32 face,
										   MASK mask, BOOL drop);
	EAcceptance dad3dRezFromObjectOnLand(LLViewerObject* obj, S32 face,
										 MASK mask, BOOL drop);
	EAcceptance dad3dRezFromObjectOnObject(LLViewerObject* obj, S32 face,
										   MASK mask, BOOL drop);
	EAcceptance dad3dRezAttachmentFromInv(LLViewerObject* obj, S32 face,
										  MASK mask, BOOL drop);
	EAcceptance dad3dCategoryOnLand(LLViewerObject *obj, S32 face,
									MASK mask, BOOL drop);
	EAcceptance dad3dAssetOnLand(LLViewerObject *obj, S32 face,
								 MASK mask, BOOL drop);
	EAcceptance dad3dActivateGesture(LLViewerObject *obj, S32 face,
								 MASK mask, BOOL drop);

	// set the LLToolDragAndDrop's cursor based on the given acceptance
	ECursorType acceptanceToCursor( EAcceptance acceptance );

	// This method converts mCargoID to an inventory item or
	// folder. If no item or category is found, both pointers will be
	// returned NULL.
	LLInventoryObject* locateInventory(LLViewerInventoryItem*& item,
									   LLViewerInventoryCategory*& cat);

	//LLInventoryObject* locateMultipleInventory(
	//	LLViewerInventoryCategory::cat_array_t& cats,
	//	LLViewerInventoryItem::item_array_t& items);

	void dropObject(LLViewerObject* raycast_target,
			BOOL bypass_sim_raycast,
			BOOL from_task_inventory,
			BOOL remove_from_inventory);
	
	// accessor that looks at permissions, copyability, and names of
	// inventory items to determine if a drop would be ok.
	static EAcceptance willObjectAcceptInventory(LLViewerObject* obj, LLInventoryItem* item);

	// deal with permissions of object, etc. returns TRUE if drop can
	// proceed, otherwise FALSE.
	static BOOL handleDropTextureProtections(LLViewerObject* hit_obj,
						 LLInventoryItem* item,
						 LLToolDragAndDrop::ESource source,
						 const LLUUID& src_id);


	// give inventory item functionality
	static bool handleCopyProtectedItem(const LLSD& notification, const LLSD& response);
	static void commitGiveInventoryItem(const LLUUID& to_agent,
										LLInventoryItem* item,
										const LLUUID &im_session_id = LLUUID::null);

	// give inventory category functionality
	static bool handleCopyProtectedCategory(const LLSD& notification, const LLSD& response);
	static void commitGiveInventoryCategory(const LLUUID& to_agent,
											LLInventoryCategory* cat,
											const LLUUID &im_session_id = LLUUID::null);

	// log "Inventory item offered" to IM
	static void logInventoryOffer(const LLUUID& to_agent, 
									const LLUUID &im_session_id = LLUUID::null);

public:
	// helper functions
	static BOOL isInventoryDropAcceptable(LLViewerObject* obj, LLInventoryItem* item) { return (ACCEPT_YES_COPY_SINGLE <= willObjectAcceptInventory(obj, item)); }

	// This simple helper function assumes you are attempting to
	// transfer item. returns true if you can give, otherwise false.
	static BOOL isInventoryGiveAcceptable(LLInventoryItem* item);
	static BOOL isInventoryGroupGiveAcceptable(LLInventoryItem* item);

	BOOL dadUpdateInventory(LLViewerObject* obj, BOOL drop);
	BOOL dadUpdateInventoryCategory(LLViewerObject* obj, BOOL drop);

	// methods that act on the simulator state.
	static void dropScript(LLViewerObject* hit_obj,
						   LLInventoryItem* item,
						   BOOL active,
						   ESource source,
						   const LLUUID& src_id);
	static void dropTextureOneFace(LLViewerObject* hit_obj, S32 hit_face,
								   LLInventoryItem* item,
								   ESource source,
								   const LLUUID& src_id);
	static void dropTextureAllFaces(LLViewerObject* hit_obj,
									LLInventoryItem* item,
									ESource source,
									const LLUUID& src_id);
	//static void	dropTextureOneFaceAvatar(LLVOAvatar* avatar,S32 hit_face,
	//									 LLInventoryItem* item)

	static void dropInventory(LLViewerObject* hit_obj,
							  LLInventoryItem* item,
							  ESource source,
							  const LLUUID& src_id);

	static void giveInventory(const LLUUID& to_agent, 
							  LLInventoryItem* item,
							  const LLUUID &session_id = LLUUID::null);
	static void giveInventoryCategory(const LLUUID& to_agent,
									  LLInventoryCategory* item,
									  const LLUUID &session_id = LLUUID::null);

	static bool handleGiveDragAndDrop(LLUUID agent, LLUUID session, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept);

	// Classes used for determining 3d drag and drop types.
private:
	struct DragAndDropEntry : public LLDictionaryEntry
	{
		DragAndDropEntry(dragOrDrop3dImpl f_none,
						 dragOrDrop3dImpl f_self,
						 dragOrDrop3dImpl f_avatar,
						 dragOrDrop3dImpl f_object,
						 dragOrDrop3dImpl f_land);
		dragOrDrop3dImpl mFunctions[DT_COUNT];
	};	
	class LLDragAndDropDictionary : public LLSingleton<LLDragAndDropDictionary>,
									public LLDictionary<EDragAndDropType, DragAndDropEntry>
	{
	public:
		LLDragAndDropDictionary();
		dragOrDrop3dImpl get(EDragAndDropType dad_type, EDropTarget drop_target);
	};
};

// utility functions
void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms);

#endif  // LL_TOOLDRAGANDDROP_H
