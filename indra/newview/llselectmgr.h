/** 
 * @file llselectmgr.h
 * @brief A manager for selected objects and TEs.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSELECTMGR_H
#define LL_LLSELECTMGR_H

#include "llcharacter.h"
#include "lldarray.h"
#include "lleditmenuhandler.h"
#include "llstring.h"
#include "llundo.h"
#include "lluuid.h"
#include "llmemory.h"
#include "llsaleinfo.h"
#include "llcategory.h"
#include "v3dmath.h"
#include "llquaternion.h"
#include "llcoord.h"
#include "llframetimer.h"
#include "llbbox.h"
#include "llpermissions.h"

#include <deque>

class LLMessageSystem;
class LLViewerImage;
class LLViewerObject;
class LLColor4;
class LLVector3;
class LLSelectNode;

const S32 SELECT_ALL_TES = -1;
const S32 SELECT_MAX_TES = 32;

// Do something to all objects in the selection manager.
// The bool return value can be used to indicate if all
// objects are identical (gathering information) or if
// the operation was successful.
class LLSelectedObjectFunctor
{
public:
	virtual ~LLSelectedObjectFunctor() {};
	virtual bool apply(LLViewerObject* object) = 0;
};

// Do something to all select nodes in the selection manager.
// The bool return value can be used to indicate if all
// objects are identical (gathering information) or if
// the operation was successful.
class LLSelectedNodeFunctor
{
public:
	virtual ~LLSelectedNodeFunctor() {};
	virtual bool apply(LLSelectNode* node) = 0;
};

typedef enum e_send_type
{
	SEND_ONLY_ROOTS,
	SEND_INDIVIDUALS,
	SEND_ROOTS_FIRST, // useful for serial undos on linked sets
	SEND_CHILDREN_FIRST // useful for serial transforms of linked sets
} ESendType;

typedef enum e_grid_mode
{
	GRID_MODE_WORLD,
	GRID_MODE_LOCAL,
	GRID_MODE_REF_OBJECT
} EGridMode;

typedef enum e_action_type
{
	SELECT_ACTION_TYPE_BEGIN,
	SELECT_ACTION_TYPE_PICK,
	SELECT_ACTION_TYPE_MOVE,
	SELECT_ACTION_TYPE_ROTATE,
	SELECT_ACTION_TYPE_SCALE,
	NUM_ACTION_TYPES
}EActionType;

typedef enum e_selection_type
{
	SELECT_TYPE_WORLD,
	SELECT_TYPE_ATTACHMENT,
	SELECT_TYPE_HUD
}ESelectType;

class LLObjectSelection : public std::list<LLSelectNode*>, public LLRefCount
{
	friend class LLSelectMgr;
public:
	LLObjectSelection();
	virtual ~LLObjectSelection();

	void updateEffects();

	BOOL isEmpty();

	S32 getOwnershipCost(S32 &cost);

	LLSelectNode* getFirstNode();
	LLSelectNode* getCurrentNode(); // Warning!  This is NOT the same as the linked_list getCurrentNode
	LLSelectNode* getNextNode();

	LLSelectNode *getFirstRootNode();
	LLSelectNode *getNextRootNode();

	LLSelectNode*	getFirstMoveableNode(BOOL get_root = FALSE);

	// iterate through objects
	LLViewerObject* getFirstObject();
	LLViewerObject* getNextObject();

	// iterate through root objects
	LLViewerObject *getFirstRootObject();
	LLViewerObject *getNextRootObject();

	LLViewerObject*	getFirstEditableObject(BOOL get_root = FALSE);
	LLViewerObject*	getFirstCopyableObject(BOOL get_root = FALSE);
	LLViewerObject* getFirstDeleteableObject(BOOL get_root = FALSE);
	LLViewerObject*	getFirstMoveableObject(BOOL get_root = FALSE);

	// iterate through texture entries
	void getPrimaryTE(LLViewerObject* *object, S32 *te);
	void getFirstTE(LLViewerObject* *object, S32 *te);
	void getNextTE(LLViewerObject* *object, S32 *te);
	void getCurrentTE(LLViewerObject* *object, S32 *te);

	void addNode(LLSelectNode *nodep);
	void addNodeAtEnd(LLSelectNode *nodep);
	void removeNode(LLSelectNode *nodep);
	void deleteAllNodes();			// Delete all nodes
	S32 getNumNodes();
	LLSelectNode* findNode(LLViewerObject* objectp);

	// count members
	S32 getObjectCount();
	S32 getTECount();
	S32 getRootObjectCount();

	BOOL contains(LLViewerObject* object);
	BOOL contains(LLViewerObject* object, S32 te);

	// returns TRUE is any node is currenly worn as an attachment
	BOOL isAttachment();

	// Apply functors to various subsets of the selected objects
	// Returns the AND of all apply() calls.
	bool applyToRootObjects(LLSelectedObjectFunctor* func);
	bool applyToObjects(LLSelectedObjectFunctor* func);
	bool applyToNodes(LLSelectedNodeFunctor* func);

	ESelectType getSelectType() { return mSelectType; }

private:
	const LLObjectSelection &operator=(const LLObjectSelection &);

	std::list<LLSelectNode*>::iterator			mCurrentNode;
	S32											mCurrentTE;
	std::map<LLViewerObject*, LLSelectNode*>	mSelectNodeMap;
	ESelectType									mSelectType;
};

typedef LLHandle<LLObjectSelection> LLObjectSelectionHandle;

class LLSelectMgr : public LLEditMenuHandler
{
public:
	static BOOL					sRectSelectInclusive;	// do we need to surround an object to pick it?
	static BOOL					sRenderHiddenSelections;	// do we show selection silhouettes that are occluded?
	static BOOL					sRenderLightRadius;	// do we show the radius of selected lights?
	static F32					sHighlightThickness;
	static F32					sHighlightUScale;
	static F32					sHighlightVScale;
	static F32					sHighlightAlpha;
	static F32					sHighlightAlphaTest;
	static F32					sHighlightUAnim;
	static F32					sHighlightVAnim;
	static LLColor4				sSilhouetteParentColor;
	static LLColor4				sSilhouetteChildColor;
	static LLColor4				sHighlightParentColor;
	static LLColor4				sHighlightChildColor;
	static LLColor4				sHighlightInspectColor;
	static LLColor4				sContextSilhouetteColor;

public:
	LLSelectMgr();
	~LLSelectMgr();

	// LLEditMenuHandler interface
	virtual BOOL canUndo();
	virtual void undo();

	virtual BOOL canRedo();
	virtual void redo();

	virtual BOOL canDoDelete();
	virtual void doDelete();

	virtual void deselect();
	virtual BOOL canDeselect();

	virtual void duplicate();
	virtual BOOL canDuplicate();

	void updateEffects(); // Update HUD effects
	void overrideObjectUpdates();

	void setForceSelection(BOOL force) { mForceSelection = force; }

	////////////////////////////////////////////////////////////////
	// Selection methods
	////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////
	// Add
	////////////////////////////////////////////////////////////////

	// For when you want just a child object.
	LLObjectSelectionHandle selectObjectOnly(LLViewerObject* object, S32 face = SELECT_ALL_TES);

	// This method is meant to select an object, and then select all
	// of the ancestors and descendents. This should be the normal behavior.
	LLObjectSelectionHandle selectObjectAndFamily(LLViewerObject* object, BOOL add_to_end = FALSE);

	// Same as above, but takes a list of objects.  Used by rectangle select.
	LLObjectSelectionHandle selectObjectAndFamily(const LLDynamicArray<LLViewerObject*>& object_list, BOOL send_to_sim = TRUE);

	// converts all objects currently highlighted to a selection, and returns it
	LLObjectSelectionHandle selectHighlightedObjects();

	LLObjectSelectionHandle setHoverObject(LLViewerObject *objectp);

	void highlightObjectOnly(LLViewerObject *objectp);
	void highlightObjectAndFamily(LLViewerObject *objectp);
	void highlightObjectAndFamily(const LLDynamicArray<LLViewerObject*>& list);

	////////////////////////////////////////////////////////////////
	// Remove
	////////////////////////////////////////////////////////////////

	void deselectObjectOnly(LLViewerObject* object, BOOL send_to_sim = TRUE);
	void deselectObjectAndFamily(LLViewerObject* object, BOOL send_to_sim = TRUE);

	// Send deselect messages to simulator, then clear the list
	void deselectAll();

	// deselect only if nothing else currently referencing the selection
	void deselectUnused();

	// Deselect if the selection center is too far away from the agent.
	void deselectAllIfTooFar();

	// Removes all highlighted objects from current selection
	void deselectHighlightedObjects();

	void unhighlightObjectOnly(LLViewerObject *objectp);
	void unhighlightObjectAndFamily(LLViewerObject *objectp);
	void unhighlightAll();

	BOOL removeObjectFromSelections(const LLUUID &id);

	////////////////////////////////////////////////////////////////
	// Selection accessors
	////////////////////////////////////////////////////////////////
	LLObjectSelectionHandle	getHoverObjects() { return mHoverObjects; }
	LLObjectSelectionHandle	getSelection() { return mSelectedObjects; }
	// right now this just renders the selection with root/child colors instead of a single color
	LLObjectSelectionHandle	getEditSelection() { convertTransient(); return mSelectedObjects; }
	LLObjectSelectionHandle	getHighlightedObjects() { return mHighlightedObjects; }

	LLSelectNode *getHoverNode();

	////////////////////////////////////////////////////////////////
	// Grid manipulation
	////////////////////////////////////////////////////////////////
	void			addGridObject(LLViewerObject* objectp);
	void			clearGridObjects();
	void			setGridMode(EGridMode mode);
	EGridMode		getGridMode() { return mGridMode; }
	void			getGrid(LLVector3& origin, LLQuaternion& rotation, LLVector3 &scale);

	BOOL getTEMode()			{ return mTEMode; }
	void setTEMode(BOOL b)	{ mTEMode = b; }

	BOOL shouldShowSelection()	{ return mShowSelection; }

	LLBBox getBBoxOfSelection() const;
	LLBBox getSavedBBoxOfSelection() const { return mSavedSelectionBBox; }

	void dump();
	void cleanup();

	void updateSilhouettes();
	void renderSilhouettes(BOOL for_hud);
	void enableSilhouette(BOOL enable) { mRenderSilhouettes = enable; }
	
	////////////////////////////////////////////////////////////////
	// Utility functions that operate on the current selection
	////////////////////////////////////////////////////////////////
	void saveSelectedObjectTransform(EActionType action_type);
	void saveSelectedObjectColors();
	void saveSelectedObjectTextures();

	void selectionUpdatePhysics(BOOL use_physics);
	void selectionUpdateTemporary(BOOL is_temporary);
	void selectionUpdatePhantom(BOOL is_ghost);
	void selectionUpdateCastShadows(BOOL cast_shadows);
	void selectionDump();

	BOOL selectionAllPCode(LLPCode code);		// all objects have this PCode
	BOOL selectionGetMaterial(U8 *material);	// all objects have same material
	BOOL selectionGetTexUUID(LLUUID& id);		// true if all selected tes have same texture
	BOOL selectionGetColor(LLColor4 &color);	// all tes have same color
	BOOL selectionGetTexScale(F32 *u, F32 *v);	// true if all selected tes have same scale
	BOOL selectionGetTexOffset(F32 *u, F32 *v);	// true if all selected tes have same offset
	BOOL selectionGetTexRotation(F32 *rad);		// true if all selected tes have same rotation
	BOOL selectionGetBumpmap(U8 *bumpmap);			// true if all selected tes have same
	BOOL selectionGetShiny(U8 *shiny);			// true if all selected tes have same
	BOOL selectionGetFullbright(U8 *fullbright);// true if all selected tes have same
	bool selectionGetMediaType(U8 *media_type);	// true if all selected tes have same
	BOOL selectionGetClickAction(U8* action);

	void selectionSetMaterial(U8 material);
	void selectionSetImage(const LLUUID& imageid); // could be item or asset id
	void selectionSetColor(const LLColor4 &color);
	void selectionSetColorOnly(const LLColor4 &color); // Set only the RGB channels
	void selectionSetAlphaOnly(const F32 alpha); // Set only the alpha channel
	void selectionRevertColors();
	BOOL selectionRevertTextures();
	void selectionSetBumpmap( U8 bumpmap );
	void selectionSetTexGen( U8 texgen );
	void selectionSetShiny( U8 shiny );
	void selectionSetFullbright( U8 fullbright );
	void selectionSetMediaTypeAndURL( U8 media_type, const std::string& media_url );
	void selectionSetClickAction(U8 action);

	void selectionSetObjectPermissions(U8 perm_field, BOOL set, U32 perm_mask, BOOL override = FALSE);
	void selectionSetObjectName(const LLString& name);
	void selectionSetObjectDescription(const LLString& desc);
	void selectionSetObjectCategory(const LLCategory& category);
	void selectionSetObjectSaleInfo(const LLSaleInfo& sale_info);

	void selectionTexScaleAutofit(F32 repeats_per_meter);
	void selectionResetTexInfo(S32 te);						// sets S,T to 1
	void adjustTexturesByScale(BOOL send_to_sim, BOOL stretch);
	BOOL getTESTAxes(const LLViewerObject* object, const U8 te, U32* s_axis, U32* t_axis);	// Only for flex boxes

	void selectionResetRotation();				// sets rotation quat to identity
	void selectionRotateAroundZ(F32 degrees);
	void sendGodlikeRequest(const LLString& request, const LLString& parameter);


	// will make sure all selected object meet current criteria, or deselect them otherwise
	void validateSelection();

	// returns TRUE if it is possible to select this object
	BOOL canSelectObject(LLViewerObject* object);

	// Returns true if the viewer has information on all selected objects
	BOOL selectGetAllRootsValid();
	BOOL selectGetAllValid();

	// returns TRUE if you can modify all selected objects. 
	BOOL selectGetRootsModify();
	BOOL selectGetModify();

	// returns TRUE if selected objects can be transferred.
	BOOL selectGetRootsTransfer();

	// returns TRUE if selected objects can be copied.
	BOOL selectGetRootsCopy();
	
	BOOL selectGetCreator(LLUUID& id, LLString& name);					// true if all have same creator, returns id
	BOOL selectGetOwner(LLUUID& id, LLString& name);					// true if all objects have same owner, returns id
	BOOL selectGetLastOwner(LLUUID& id, LLString& name);				// true if all objects have same owner, returns id

	// returns TRUE if all are the same. id is stuffed with
	// the value found if available.
	BOOL selectGetGroup(LLUUID& id); 
	BOOL selectGetPerm(	U8 which_perm, U32* mask_on, U32* mask_off);	// true if all have data, returns two masks, each indicating which bits are all on and all off
	BOOL selectGetOwnershipCost(S32* cost);								// sum of all ownership costs

	BOOL selectIsGroupOwned();											// true if all root objects have valid data and are group owned.

	// returns TRUE if all the nodes are valid. Accumulates
	// permissions in the parameter.
	BOOL selectGetPermissions(LLPermissions& perm);

	// returns TRUE if anything is for sale. calculates the total
	// price and stores that value in price.
	BOOL selectIsForSale(S32& price);

	// returns TRUE if all nodes are valid. 
	BOOL selectGetCategory(LLCategory& category);
	
	// returns TRUE if all nodes are valid. method also stores an
	// accumulated sale info.
	BOOL selectGetSaleInfo(LLSaleInfo& sale_info);

	// returns TRUE if all nodes are valid. fills passed in object
	// with the aggregate permissions of the selection.
	BOOL selectGetAggregatePermissions(LLAggregatePermissions& ag_perm);

	// returns TRUE if all nodes are valid. fills passed in object
	// with the aggregate permissions for texture inventory items of the selection.
	BOOL selectGetAggregateTexturePermissions(LLAggregatePermissions& ag_perm);

	LLPermissions* findObjectPermissions(const LLViewerObject* object);

	void selectDelete();							// Delete on simulator
	void selectForceDelete();			// just delete, no into trash
	void selectDuplicate(const LLVector3& offset, BOOL select_copy);	// Duplicate on simulator
	void repeatDuplicate();
	void selectDuplicateOnRay(const LLVector3 &ray_start_region,
								const LLVector3 &ray_end_region,
								BOOL bypass_raycast,
								BOOL ray_end_is_intersection,
								const LLUUID &ray_target_id,
								BOOL copy_centers,
								BOOL copy_rotates,
								BOOL select_copy);

	void sendMultipleUpdate(U32 type);	// Position, rotation, scale all in one
	void sendOwner(const LLUUID& owner_id, const LLUUID& group_id, BOOL override = FALSE);
	void sendGroup(const LLUUID& group_id);

	// Category ID is the UUID of the folder you want to contain the purchase.
	// *NOTE: sale_info check doesn't work for multiple object buy,
	// which UI does not currently support sale info is used for
	// verification only, if it doesn't match region info then sale is
	// canceled
	void sendBuy(const LLUUID& buyer_id, const LLUUID& category_id, const LLSaleInfo sale_info);
	void sendAttach(U8 attachment_point);
	void sendDetach();
	void sendDropAttachment();
	void sendLink();
	void sendDelink();
	void sendHinge(U8 type);
	void sendDehinge();
	void sendSelect();

	void requestObjectPropertiesFamily(LLViewerObject* object);	// asks sim for creator, permissions, resources, etc.
	static void processObjectProperties(LLMessageSystem *mesgsys, void **user_data);
	static void processObjectPropertiesFamily(LLMessageSystem *mesgsys, void **user_data);
	static void processForceObjectSelect(LLMessageSystem* msg, void**);

	void requestGodInfo();

	LLVector3d		getSelectionCenterGlobal() const	{ return mSelectionCenterGlobal; }
	void			updateSelectionCenter();
	void			updatePointAt();

	// Internal list maintenance functions. TODO: Make these private!
	void remove(LLDynamicArray<LLViewerObject*>& objects);
	void remove(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void removeAll();
	void addAsIndividual(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void promoteSelectionToRoot();
	void demoteSelectionToIndividuals();

private:
	void convertTransient(); // converts temporarily selected objects to full-fledged selections
	ESelectType getSelectTypeForObject(LLViewerObject* object);
	void addAsFamily(LLDynamicArray<LLViewerObject*>& objects, BOOL add_to_end = FALSE);
	void generateSilhouette(LLSelectNode *nodep, const LLVector3& view_point);
	void getSilhouetteExtents(LLSelectNode* nodep, const LLQuaternion& orientation, LLVector3& min_extents, LLVector3& max_extents);
	// Send one message to each region containing an object on selection list.
	void sendListToRegions(	const LLString& message_name,
							void (*pack_header)(void *user_data), 
							void (*pack_body)(LLSelectNode* node, void *user_data), 
							void *user_data,
							ESendType send_type);

	static void packAgentID(	void *);
	static void packAgentAndSessionID(void* user_data);
	static void packAgentAndGroupID(void* user_data);
	static void packAgentAndSessionAndGroupID(void* user_data);
	static void packAgentIDAndSessionAndAttachment(void*);
	static void packAgentGroupAndCatID(void*);
	static void packDeleteHeader(void* userdata);
	static void packDeRezHeader(void* user_data);
	static void packObjectID(	LLSelectNode* node, void *);
	static void packObjectIDAsParam(LLSelectNode* node, void *);
	static void packObjectIDAndRotation(	LLSelectNode* node, void *);
	static void packObjectLocalID(LLSelectNode* node, void *);
	static void packObjectClickAction(LLSelectNode* node, void* data);
	static void packObjectName(LLSelectNode* node, void* user_data);
	static void packObjectDescription(LLSelectNode* node, void* user_data);
	static void packObjectCategory(LLSelectNode* node, void* user_data);
	static void packObjectSaleInfo(LLSelectNode* node, void* user_data);
	static void packBuyObjectIDs(LLSelectNode* node, void* user_data);
	static void packDuplicate(	LLSelectNode* node, void *duplicate_data);
	static void packDuplicateHeader(void*);
	static void packDuplicateOnRayHead(void *user_data);
	static void packPermissions(LLSelectNode* node, void *user_data);
	static void packDeselect(	LLSelectNode* node, void *user_data);
	static void packMultipleUpdate(LLSelectNode* node, void *user_data);
	static void packPhysics(LLSelectNode* node, void *user_data);
	static void packShape(LLSelectNode* node, void *user_data);
	static void packOwnerHead(void *user_data);
	static void packHingeHead(void *user_data);
	static void packPermissionsHead(void* user_data);
	static void packGodlikeHead(void* user_data);
	static void confirmDelete(S32 option, void* data);
	
private:
	LLPointer<LLViewerImage>				mSilhouetteImagep;
	LLObjectSelectionHandle					mSelectedObjects;
	LLObjectSelectionHandle					mHoverObjects;
	LLObjectSelectionHandle					mHighlightedObjects;
	std::set<LLPointer<LLViewerObject> >	mRectSelectedObjects;
	
	LLObjectSelection		mGridObjects;
	LLQuaternion			mGridRotation;
	LLVector3				mGridOrigin;
	LLVector3				mGridScale;
	EGridMode				mGridMode;
	BOOL					mGridValid;


	BOOL					mTEMode;			// render te
	LLVector3d				mSelectionCenterGlobal;
	LLBBox					mSelectionBBox;

	LLVector3d				mLastSentSelectionCenterGlobal;
	BOOL					mShowSelection; // do we send the selection center name value and do we animate this selection?
	LLVector3d				mLastCameraPos;		// camera position from last generation of selection silhouette
	BOOL					mRenderSilhouettes;	// do we render the silhouette
	LLBBox					mSavedSelectionBBox;

	LLFrameTimer			mEffectsTimer;
	BOOL					mForceSelection;

	LLAnimPauseRequest		mPauseRequest;
};


// Contains information about a selected object, particularly which
// tes are selected.
class LLSelectNode
{
public:
	LLSelectNode(LLViewerObject* object, BOOL do_glow);
	LLSelectNode(const LLSelectNode& nodep);
	~LLSelectNode();

	void selectAllTEs(BOOL b);
	void selectTE(S32 te_index, BOOL selected);
	BOOL isTESelected(S32 te_index);
	S32 getLastSelectedTE();
	void renderOneSilhouette(const LLColor4 &color);
	void setTransient(BOOL transient) { mTransient = transient; }
	BOOL isTransient() { return mTransient; }
	LLViewerObject *getObject();
	// *NOTE: invalidate stored textures and colors when # faces change
	void saveColors();
	void saveTextures(const std::vector<LLUUID>& textures);
	void saveTextureScaleRatios();

	BOOL allowOperationOnNode(PermissionBit op, U64 group_proxy_power) const;

public:
	BOOL			mIndividualSelection;		// For root objects and objects individually selected

	BOOL			mTransient;
	BOOL			mValid;				// is extra information valid?
	LLPermissions*	mPermissions;
	LLSaleInfo		mSaleInfo;
	LLAggregatePermissions mAggregatePerm;
	LLAggregatePermissions mAggregateTexturePerm;
	LLAggregatePermissions mAggregateTexturePermOwner;
	LLString		mName;
	LLString		mDescription;
	LLCategory		mCategory;
	S16				mInventorySerial;
	LLVector3		mSavedPositionLocal;	// for interactively modifying object position
	LLVector3		mLastPositionLocal;
	LLVector3d		mSavedPositionGlobal;	// for interactively modifying object position
	LLVector3		mSavedScale;			// for interactively modifying object scale
	LLVector3		mLastScale;
	LLQuaternion	mSavedRotation;			// for interactively modifying object rotation
	LLQuaternion	mLastRotation;
	BOOL			mDuplicated;
	LLVector3d		mDuplicatePos;
	LLQuaternion	mDuplicateRot;
	LLUUID			mItemID;
	LLUUID			mFolderID;
	LLUUID			mFromTaskID;
	LLString		mTouchName;
	LLString		mSitName;
	U64				mCreationDate;
	std::vector<LLColor4>	mSavedColors;
	std::vector<LLUUID>		mSavedTextures;
	std::vector<LLVector3>  mTextureScaleRatios;
	std::vector<LLVector3>	mSilhouetteVertices;	// array of vertices to render silhouette of object
	std::vector<LLVector3>	mSilhouetteNormals;	// array of normals to render silhouette of object
	std::vector<S32>		mSilhouetteSegments;	// array of normals to render silhouette of object
	BOOL					mSilhouetteExists;	// need to generate silhouette?

protected:
	LLPointer<LLViewerObject>	mObject;
	BOOL			mTESelected[SELECT_MAX_TES];
	S32				mLastTESelected;
};

extern LLSelectMgr* gSelectMgr;

// Utilities
void dialog_refresh_all();		// Update subscribers to the selection list

#endif
