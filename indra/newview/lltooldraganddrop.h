/** 
 * @file lltooldraganddrop.h
 * @brief LLToolDragAndDrop class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TOOLDRAGANDDROP_H
#define LL_TOOLDRAGANDDROP_H

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

class LLToolDragAndDrop : public LLTool
{
public:
	LLToolDragAndDrop();

	// overridden from LLTool
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleKey(KEY key, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect *sticky_rect_screen);
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

	// dragOrDrop3dImpl points to a member of LLToolDragAndDrop that
	// takes parameters (LLViewerObject* obj, S32 face, MASK, BOOL
	// drop) and returns a BOOL if drop is ok
	typedef EAcceptance (LLToolDragAndDrop::*dragOrDrop3dImpl)
		(LLViewerObject*, S32, MASK, BOOL);

	void dragOrDrop(S32 x, S32 y, MASK mask, BOOL drop,
					EAcceptance* acceptance);
	void dragOrDrop3D(S32 x, S32 y, MASK mask, BOOL drop,
					  EAcceptance* acceptance);
	static void pickCallback(S32 x, S32 y, MASK mask);

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
	LLString		mToolTipMsg;

	// array of pointers to functions that implement the logic to
	// dragging and dropping into the simulator.
	static dragOrDrop3dImpl sDragAndDrop3d[DAD_COUNT][DT_COUNT];

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

	// This method converts mCargoID to an inventory item or
	// folder. If no item or category is found, both pointers will be
	// returned NULL.
	LLInventoryObject* locateInventory(LLViewerInventoryItem*& item,
									   LLViewerInventoryCategory*& cat);

	//LLInventoryObject* locateMultipleInventory(
	//	LLViewerInventoryCategory::cat_array_t& cats,
	//	LLViewerInventoryItem::item_array_t& items);

	void createContainer(LLViewerInventoryItem::item_array_t &items, const char* preferred_name);
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
	static void handleCopyProtectedItem(S32 option, void* data);
	static void commitGiveInventoryItem(const LLUUID& to_agent,
										LLInventoryItem* item);

	// give inventory category functionality
	static void handleCopyProtectedCategory(S32 option, void* data);
	static void commitGiveInventoryCategory(const LLUUID& to_agent,
											LLInventoryCategory* cat);
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

	static void giveInventory(const LLUUID& to_agent, LLInventoryItem* item);
	static void giveInventoryCategory(const LLUUID& to_agent,
									  LLInventoryCategory* item);
};

// Singleton
extern LLToolDragAndDrop *gToolDragAndDrop;

// utility functions
void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms);

#endif  // LL_TOOLDRAGANDDROP_H
