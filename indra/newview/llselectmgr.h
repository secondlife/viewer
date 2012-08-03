/** 
 * @file llselectmgr.h
 * @brief A manager for selected objects and TEs.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLSELECTMGR_H
#define LL_LLSELECTMGR_H

#include "llcharacter.h"
#include "lleditmenuhandler.h"
#include "llundo.h"
#include "lluuid.h"
#include "llpointer.h"
#include "llsafehandle.h"
#include "llsaleinfo.h"
#include "llcategory.h"
#include "v3dmath.h"
#include "llquaternion.h"
#include "llcoord.h"
#include "llframetimer.h"
#include "llbbox.h"
#include "llpermissions.h"
#include "llcontrol.h"
#include "llviewerobject.h"	// LLObjectSelection::getSelectedTEValue template

#include <deque>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/signals2.hpp>

class LLMessageSystem;
class LLViewerTexture;
class LLColor4;
class LLVector3;
class LLSelectNode;

const S32 SELECT_ALL_TES = -1;
const S32 SELECT_MAX_TES = 32;

// Do something to all objects in the selection manager.
// The BOOL return value can be used to indicate if all
// objects are identical (gathering information) or if
// the operation was successful.
struct LLSelectedObjectFunctor
{
	virtual ~LLSelectedObjectFunctor() {};
	virtual bool apply(LLViewerObject* object) = 0;
};

// Do something to all select nodes in the selection manager.
// The BOOL return value can be used to indicate if all
// objects are identical (gathering information) or if
// the operation was successful.
struct LLSelectedNodeFunctor
{
	virtual ~LLSelectedNodeFunctor() {};
	virtual bool apply(LLSelectNode* node) = 0;
};

struct LLSelectedTEFunctor
{
	virtual ~LLSelectedTEFunctor() {};
	virtual bool apply(LLViewerObject* object, S32 face) = 0;
};

template <typename T> struct LLSelectedTEGetFunctor
{
	virtual ~LLSelectedTEGetFunctor() {};
	virtual T get(LLViewerObject* object, S32 te) = 0;
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

// Contains information about a selected object, particularly which TEs are selected.
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
	S32 getTESelectMask() { return mTESelectMask; }
	void renderOneWireframe(const LLColor4& color);
	void renderOneSilhouette(const LLColor4 &color);
	void setTransient(BOOL transient) { mTransient = transient; }
	BOOL isTransient() { return mTransient; }
	LLViewerObject* getObject();
	void setObject(LLViewerObject* object);
	// *NOTE: invalidate stored textures and colors when # faces change
	void saveColors();
	void saveTextures(const uuid_vec_t& textures);
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
	std::string		mName;
	std::string		mDescription;
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
	std::string		mTouchName;
	std::string		mSitName;
	U64				mCreationDate;
	std::vector<LLColor4>	mSavedColors;
	uuid_vec_t		mSavedTextures;
	std::vector<LLVector3>  mTextureScaleRatios;
	std::vector<LLVector3>	mSilhouetteVertices;	// array of vertices to render silhouette of object
	std::vector<LLVector3>	mSilhouetteNormals;	// array of normals to render silhouette of object
	BOOL					mSilhouetteExists;	// need to generate silhouette?

protected:
	LLPointer<LLViewerObject>	mObject;
	S32				mTESelectMask;
	S32				mLastTESelected;
};

class LLObjectSelection : public LLRefCount
{
	friend class LLSelectMgr;

protected:
	~LLObjectSelection();

public:
	typedef std::list<LLSelectNode*> list_t;

	// Iterators
	struct is_non_null
	{
		bool operator()(LLSelectNode* node)
		{
			return (node->getObject() != NULL);
		}
	};
	typedef boost::filter_iterator<is_non_null, list_t::iterator > iterator;
	iterator begin() { return iterator(mList.begin(), mList.end()); }
	iterator end() { return iterator(mList.end(), mList.end()); }

	struct is_valid
	{
		bool operator()(LLSelectNode* node)
		{
			return (node->getObject() != NULL) && node->mValid;
		}
	};
	typedef boost::filter_iterator<is_valid, list_t::iterator > valid_iterator;
	valid_iterator valid_begin() { return valid_iterator(mList.begin(), mList.end()); }
	valid_iterator valid_end() { return valid_iterator(mList.end(), mList.end()); }

	struct is_root
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_root, list_t::iterator > root_iterator;
	root_iterator root_begin() { return root_iterator(mList.begin(), mList.end()); }
	root_iterator root_end() { return root_iterator(mList.end(), mList.end()); }
	
	struct is_valid_root
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_valid_root, list_t::iterator > valid_root_iterator;
	valid_root_iterator valid_root_begin() { return valid_root_iterator(mList.begin(), mList.end()); }
	valid_root_iterator valid_root_end() { return valid_root_iterator(mList.end(), mList.end()); }
	
	struct is_root_object
	{
		bool operator()(LLSelectNode* node);
	};
	typedef boost::filter_iterator<is_root_object, list_t::iterator > root_object_iterator;
	root_object_iterator root_object_begin() { return root_object_iterator(mList.begin(), mList.end()); }
	root_object_iterator root_object_end() { return root_object_iterator(mList.end(), mList.end()); }
	
public:
	LLObjectSelection();

	void updateEffects();

	BOOL isEmpty() const;

	LLSelectNode*	getFirstNode(LLSelectedNodeFunctor* func = NULL);
	LLSelectNode*	getFirstRootNode(LLSelectedNodeFunctor* func = NULL, BOOL non_root_ok = FALSE);
	LLViewerObject* getFirstSelectedObject(LLSelectedNodeFunctor* func, BOOL get_parent = FALSE);
	LLViewerObject*	getFirstObject();
	LLViewerObject*	getFirstRootObject(BOOL non_root_ok = FALSE);
	
	LLSelectNode*	getFirstMoveableNode(BOOL get_root_first = FALSE);

	LLViewerObject*	getFirstEditableObject(BOOL get_parent = FALSE);
	LLViewerObject*	getFirstCopyableObject(BOOL get_parent = FALSE);
	LLViewerObject* getFirstDeleteableObject();
	LLViewerObject*	getFirstMoveableObject(BOOL get_parent = FALSE);

	/// Return the object that lead to this selection, possible a child
	LLViewerObject* getPrimaryObject() { return mPrimaryObject; }

	// iterate through texture entries
	template <typename T> bool getSelectedTEValue(LLSelectedTEGetFunctor<T>* func, T& res);
	template <typename T> bool isMultipleTEValue(LLSelectedTEGetFunctor<T>* func, const T& ignore_value);
	
	S32 getNumNodes();
	LLSelectNode* findNode(LLViewerObject* objectp);

	// count members
	S32 getObjectCount();
	F32 getSelectedObjectCost();
	F32 getSelectedLinksetCost();
	F32 getSelectedPhysicsCost();
	F32 getSelectedLinksetPhysicsCost();
	S32 getSelectedObjectRenderCost();
	
	F32 getSelectedObjectStreamingCost(S32* total_bytes = NULL, S32* visible_bytes = NULL);
	U32 getSelectedObjectTriangleCount(S32* vcount = NULL);

	S32 getTECount();
	S32 getRootObjectCount();

	BOOL isMultipleTESelected();
	BOOL contains(LLViewerObject* object);
	BOOL contains(LLViewerObject* object, S32 te);

	// returns TRUE is any node is currenly worn as an attachment
	BOOL isAttachment();

	// Apply functors to various subsets of the selected objects
	// If firstonly is FALSE, returns the AND of all apply() calls.
	// Else returns TRUE immediately if any apply() call succeeds (i.e. OR with early exit)
	bool applyToRootObjects(LLSelectedObjectFunctor* func, bool firstonly = false);
	bool applyToObjects(LLSelectedObjectFunctor* func);
	bool applyToTEs(LLSelectedTEFunctor* func, bool firstonly = false);
	bool applyToRootNodes(LLSelectedNodeFunctor* func, bool firstonly = false);
	bool applyToNodes(LLSelectedNodeFunctor* func, bool firstonly = false);

	/*
	 * Used to apply (no-copy) textures to the selected object or
	 * selected face/faces of the object.
	 * This method moves (no-copy) texture to the object's inventory
	 * and doesn't make copy of the texture for each face.
	 * Then this only texture is used for all selected faces.
	 */
	void applyNoCopyTextureToTEs(LLViewerInventoryItem* item);

	ESelectType getSelectType() const { return mSelectType; }

private:
	void addNode(LLSelectNode *nodep);
	void addNodeAtEnd(LLSelectNode *nodep);
	void moveNodeToFront(LLSelectNode *nodep);
	void removeNode(LLSelectNode *nodep);
	void deleteAllNodes();
	void cleanupNodes();


private:
	list_t mList;
	const LLObjectSelection &operator=(const LLObjectSelection &);

	LLPointer<LLViewerObject> mPrimaryObject;
	std::map<LLPointer<LLViewerObject>, LLSelectNode*> mSelectNodeMap;
	ESelectType mSelectType;
};

typedef LLSafeHandle<LLObjectSelection> LLObjectSelectionHandle;

// Build time optimization, generate this once in .cpp file
#ifndef LLSELECTMGR_CPP
extern template class LLSelectMgr* LLSingleton<class LLSelectMgr>::getInstance();
#endif

class LLSelectMgr : public LLEditMenuHandler, public LLSingleton<LLSelectMgr>
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

	LLCachedControl<bool>					mHideSelectedObjects;
	LLCachedControl<bool>					mRenderHighlightSelections;
	LLCachedControl<bool>					mAllowSelectAvatar;
	LLCachedControl<bool>					mDebugSelectMgr;

public:
	LLSelectMgr();
	~LLSelectMgr();

	static void cleanupGlobals();

	// LLEditMenuHandler interface
	virtual BOOL canUndo() const;
	virtual void undo();

	virtual BOOL canRedo() const;
	virtual void redo();

	virtual BOOL canDoDelete() const;
	virtual void doDelete();

	virtual void deselect();
	virtual BOOL canDeselect() const;

	virtual void duplicate();
	virtual BOOL canDuplicate() const;

	void clearSelections();
	void update();
	void updateEffects(); // Update HUD effects
	void overrideObjectUpdates();

	// Returns the previous value of mForceSelection
	BOOL setForceSelection(BOOL force);

	////////////////////////////////////////////////////////////////
	// Selection methods
	////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////
	// Add
	////////////////////////////////////////////////////////////////

	// This method is meant to select an object, and then select all
	// of the ancestors and descendants. This should be the normal behavior.
	//
	// *NOTE: You must hold on to the object selection handle, otherwise
	// the objects will be automatically deselected in 1 frame.
	LLObjectSelectionHandle selectObjectAndFamily(LLViewerObject* object, BOOL add_to_end = FALSE);

	// For when you want just a child object.
	LLObjectSelectionHandle selectObjectOnly(LLViewerObject* object, S32 face = SELECT_ALL_TES);

	// Same as above, but takes a list of objects.  Used by rectangle select.
	LLObjectSelectionHandle selectObjectAndFamily(const std::vector<LLViewerObject*>& object_list, BOOL send_to_sim = TRUE);

	// converts all objects currently highlighted to a selection, and returns it
	LLObjectSelectionHandle selectHighlightedObjects();

	LLObjectSelectionHandle setHoverObject(LLViewerObject *objectp, S32 face = -1);
	LLSelectNode *getHoverNode();
	LLSelectNode *getPrimaryHoverNode();

	void highlightObjectOnly(LLViewerObject *objectp);
	void highlightObjectAndFamily(LLViewerObject *objectp);
	void highlightObjectAndFamily(const std::vector<LLViewerObject*>& list);

	////////////////////////////////////////////////////////////////
	// Remove
	////////////////////////////////////////////////////////////////

	void deselectObjectOnly(LLViewerObject* object, BOOL send_to_sim = TRUE);
	void deselectObjectAndFamily(LLViewerObject* object, BOOL send_to_sim = TRUE, BOOL include_entire_object = FALSE);

	// Send deselect messages to simulator, then clear the list
	void deselectAll();
	void deselectAllForStandingUp();

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
	// Selection editing
	////////////////////////////////////////////////////////////////
	bool linkObjects();

	bool unlinkObjects();

	bool enableLinkObjects();

	bool enableUnlinkObjects();

	////////////////////////////////////////////////////////////////
	// Selection accessors
	////////////////////////////////////////////////////////////////
	LLObjectSelectionHandle	getSelection() { return mSelectedObjects; }
	// right now this just renders the selection with root/child colors instead of a single color
	LLObjectSelectionHandle	getEditSelection() { convertTransient(); return mSelectedObjects; }
	LLObjectSelectionHandle	getHighlightedObjects() { return mHighlightedObjects; }

	////////////////////////////////////////////////////////////////
	// Grid manipulation
	////////////////////////////////////////////////////////////////
	void			addGridObject(LLViewerObject* objectp);
	void			clearGridObjects();
	void			setGridMode(EGridMode mode);
	EGridMode		getGridMode() { return mGridMode; }
	void			getGrid(LLVector3& origin, LLQuaternion& rotation, LLVector3 &scale);

	BOOL getTEMode()		{ return mTEMode; }
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
	void selectionDump();

	BOOL selectionAllPCode(LLPCode code);		// all objects have this PCode
	BOOL selectionGetClickAction(U8 *out_action);
	bool selectionGetIncludeInSearch(bool* include_in_search_out); // true if all selected objects have same
	BOOL selectionGetGlow(F32 *glow);

	void selectionSetPhysicsType(U8 type);
	void selectionSetGravity(F32 gravity);
	void selectionSetFriction(F32 friction);
	void selectionSetDensity(F32 density);
	void selectionSetRestitution(F32 restitution);
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
	void selectionSetMedia( U8 media_type, const LLSD &media_data );
	void selectionSetClickAction(U8 action);
	void selectionSetIncludeInSearch(bool include_in_search);
	void selectionSetGlow(const F32 glow);

	void selectionSetObjectPermissions(U8 perm_field, BOOL set, U32 perm_mask, BOOL override = FALSE);
	void selectionSetObjectName(const std::string& name);
	void selectionSetObjectDescription(const std::string& desc);
	void selectionSetObjectCategory(const LLCategory& category);
	void selectionSetObjectSaleInfo(const LLSaleInfo& sale_info);

	void selectionTexScaleAutofit(F32 repeats_per_meter);
	void adjustTexturesByScale(BOOL send_to_sim, BOOL stretch);

	bool selectionMove(const LLVector3& displ, F32 rx, F32 ry, F32 rz,
					   U32 update_type);
	void sendSelectionMove();

	void sendGodlikeRequest(const std::string& request, const std::string& parameter);


	// will make sure all selected object meet current criteria, or deselect them otherwise
	void validateSelection();

	// returns TRUE if it is possible to select this object
	BOOL canSelectObject(LLViewerObject* object);

	// Returns TRUE if the viewer has information on all selected objects
	BOOL selectGetAllRootsValid();
	BOOL selectGetAllValid();
	BOOL selectGetAllValidAndObjectsFound();

	// returns TRUE if you can modify all selected objects. 
	BOOL selectGetRootsModify();
	BOOL selectGetModify();

	// returns TRUE if is all objects are non-permanent-enforced
	BOOL selectGetRootsNonPermanentEnforced();
	BOOL selectGetNonPermanentEnforced();

	// returns TRUE if is all objects are permanent
	BOOL selectGetRootsPermanent();
	BOOL selectGetPermanent();

	// returns TRUE if is all objects are character
	BOOL selectGetRootsCharacter();
	BOOL selectGetCharacter();

	// returns TRUE if is all objects are not permanent
	BOOL selectGetRootsNonPathfinding();
	BOOL selectGetNonPathfinding();

	// returns TRUE if is all objects are not permanent
	BOOL selectGetRootsNonPermanent();
	BOOL selectGetNonPermanent();

	// returns TRUE if is all objects are not character
	BOOL selectGetRootsNonCharacter();
	BOOL selectGetNonCharacter();

	BOOL selectGetEditableLinksets();
	BOOL selectGetViewableCharacters();

	// returns TRUE if selected objects can be transferred.
	BOOL selectGetRootsTransfer();

	// returns TRUE if selected objects can be copied.
	BOOL selectGetRootsCopy();
	
	BOOL selectGetCreator(LLUUID& id, std::string& name);					// TRUE if all have same creator, returns id
	BOOL selectGetOwner(LLUUID& id, std::string& name);					// TRUE if all objects have same owner, returns id
	BOOL selectGetLastOwner(LLUUID& id, std::string& name);				// TRUE if all objects have same owner, returns id

	// returns TRUE if all are the same. id is stuffed with
	// the value found if available.
	BOOL selectGetGroup(LLUUID& id); 
	BOOL selectGetPerm(	U8 which_perm, U32* mask_on, U32* mask_off);	// TRUE if all have data, returns two masks, each indicating which bits are all on and all off

	BOOL selectIsGroupOwned();											// TRUE if all root objects have valid data and are group owned.

	// returns TRUE if all the nodes are valid. Accumulates
	// permissions in the parameter.
	BOOL selectGetPermissions(LLPermissions& perm);
	
	// Get a bunch of useful sale information for the object(s) selected.
	// "_mixed" is true if not all objects have the same setting.
	void selectGetAggregateSaleInfo(U32 &num_for_sale,
									BOOL &is_for_sale_mixed, 
									BOOL &is_sale_price_mixed,
									S32 &total_sale_price,
									S32 &individual_sale_price);

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
	void sendAttach(U8 attachment_point, bool replace);
	void sendDetach();
	void sendDropAttachment();
	void sendLink();
	void sendDelink();
	//void sendHinge(U8 type);
	//void sendDehinge();
	void sendSelect();

	void requestObjectPropertiesFamily(LLViewerObject* object);	// asks sim for creator, permissions, resources, etc.
	static void processObjectProperties(LLMessageSystem *mesgsys, void **user_data);
	static void processObjectPropertiesFamily(LLMessageSystem *mesgsys, void **user_data);
	static void processForceObjectSelect(LLMessageSystem* msg, void**);

	void requestGodInfo();

	LLVector3d		getSelectionCenterGlobal() const	{ return mSelectionCenterGlobal; }
	void			updateSelectionCenter();

	void resetAgentHUDZoom();
	void setAgentHUDZoom(F32 target_zoom, F32 current_zoom);
	void getAgentHUDZoom(F32 &target_zoom, F32 &current_zoom) const;

	void updatePointAt();

	// Internal list maintenance functions. TODO: Make these private!
	void remove(std::vector<LLViewerObject*>& objects);
	void remove(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void removeAll();
	void addAsIndividual(LLViewerObject* object, S32 te = SELECT_ALL_TES, BOOL undoable = TRUE);
	void promoteSelectionToRoot();
	void demoteSelectionToIndividuals();

private:
	void convertTransient(); // converts temporarily selected objects to full-fledged selections
	ESelectType getSelectTypeForObject(LLViewerObject* object);
	void addAsFamily(std::vector<LLViewerObject*>& objects, BOOL add_to_end = FALSE);
	void generateSilhouette(LLSelectNode *nodep, const LLVector3& view_point);
	void updateSelectionSilhouette(LLObjectSelectionHandle object_handle, S32& num_sils_genned, std::vector<LLViewerObject*>& changed_objects);
	// Send one message to each region containing an object on selection list.
	void sendListToRegions(	const std::string& message_name,
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
	static void packObjectIDAndRotation(LLSelectNode* node, void *);
	static void packObjectLocalID(LLSelectNode* node, void *);
	static void packObjectClickAction(LLSelectNode* node, void* data);
	static void packObjectIncludeInSearch(LLSelectNode* node, void* data);
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
	static bool confirmDelete(const LLSD& notification, const LLSD& response, LLObjectSelectionHandle handle);

public:
	// Observer/callback support for when object selection changes or
	// properties are received/updated
	typedef boost::signals2::signal< void ()> update_signal_t;
	update_signal_t mUpdateSignal;

private:
	LLPointer<LLViewerTexture>				mSilhouetteImagep;
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

// *DEPRECATED: For callbacks or observers, use
// LLSelectMgr::getInstance()->mUpdateSignal.connect( callback )
// Update subscribers to the selection list
void dialog_refresh_all();		

// Templates
//-----------------------------------------------------------------------------
// getSelectedTEValue
//-----------------------------------------------------------------------------
template <typename T> bool LLObjectSelection::getSelectedTEValue(LLSelectedTEGetFunctor<T>* func, T& res)
{
	bool have_first = false;
	bool have_selected = false;
	T selected_value = T();

	// Now iterate through all TEs to test for sameness
	bool identical = TRUE;
	for (iterator iter = begin(); iter != end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		S32 selected_te = -1;
		if (object == getPrimaryObject())
		{
			selected_te = node->getLastSelectedTE();
		}
		for (S32 te = 0; te < object->getNumTEs(); ++te)
		{
			if (!node->isTESelected(te))
			{
				continue;
			}
			T value = func->get(object, te);
			if (!have_first)
			{
				have_first = true;
				if (!have_selected)
				{
					selected_value = value;
				}
			}
			else
			{
				if ( value != selected_value )
				{
					identical = false;
				}
				if (te == selected_te)
				{
					selected_value = value;
					have_selected = true;
				}
			}
		}
		if (!identical && have_selected)
		{
			break;
		}
	}
	if (have_first || have_selected)
	{
		res = selected_value;
	}
	return identical;
}

// Templates
//-----------------------------------------------------------------------------
// isMultipleTEValue iterate through all TEs and test for uniqueness 
// with certain return value ignored when performing the test. 
// e.g. when testing if the selection has a unique non-empty homeurl :
// you can set ignore_value = "" and it will only compare among the non-empty  
// homeUrls and ignore the empty ones.
//-----------------------------------------------------------------------------
template <typename T> bool LLObjectSelection::isMultipleTEValue(LLSelectedTEGetFunctor<T>* func, const T& ignore_value)
{
	bool have_first = false;
	T selected_value = T();
	
	// Now iterate through all TEs to test for sameness
	bool unique = TRUE;
	for (iterator iter = begin(); iter != end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		for (S32 te = 0; te < object->getNumTEs(); ++te)
		{
			if (!node->isTESelected(te))
			{
				continue;
			}
			T value = func->get(object, te);
			if(value == ignore_value)
			{
				continue;
			}
			if (!have_first)
			{
				have_first = true;
			}
			else
			{
				if (value !=selected_value  )
				{
					unique = false;
					return !unique;
				}
			}
		}
	}
	return !unique;
}


#endif
