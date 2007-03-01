/** 
 * @file llselectmgr.cpp
 * @brief A manager for selected objects and faces.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

// file include
#include "llselectmgr.h"

// library includes
#include "llcachename.h"
#include "lldbstrings.h"
#include "lleconomy.h"
#include "llgl.h"
#include "llpermissions.h"
#include "llpermissionsflags.h"
#include "llptrskiplist.h"
#include "llundo.h"
#include "lluuid.h"
#include "llvolume.h"
#include "message.h"
#include "object_flags.h"
#include "llquaternion.h"

// viewer includes
#include "llagent.h"
#include "llviewerwindow.h"
#include "lldrawable.h"
#include "llfloaterinspect.h"
#include "llfloaterproperties.h"
#include "llfloaterrate.h"
#include "llfloaterreporter.h"
#include "llfloatertools.h"
#include "llframetimer.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventorymodel.h"
#include "llmenugl.h"
#include "llstatusbar.h"
#include "llsurface.h"
#include "lltool.h"
#include "lltooldraganddrop.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvoavatar.h"
#include "llvovolume.h"
#include "pipeline.h"

#include "llglheaders.h"

//
// Consts
//

const S32 NUM_SELECTION_UNDO_ENTRIES = 200;
const F32 SILHOUETTE_UPDATE_THRESHOLD_SQUARED = 0.02f;
const S32 OWNERSHIP_COST_PER_OBJECT = 10; // Must be the same as economy_constants.price_object_claim in the database.
const S32 MAX_ACTION_QUEUE_SIZE = 20;
const S32 MAX_SILS_PER_FRAME = 50;
const S32 MAX_OBJECTS_PER_PACKET = 254;

extern LLGlobalEconomy *gGlobalEconomy;
extern LLUUID			gLastHitObjectID;
extern LLVector3d		gLastHitObjectOffset;

//
// Globals
//
LLSelectMgr* gSelectMgr = NULL;

BOOL gDebugSelectMgr = FALSE;

BOOL gHideSelectedObjects = FALSE;
BOOL gAllowSelectAvatar = FALSE;

BOOL LLSelectMgr::sRectSelectInclusive = TRUE;
BOOL LLSelectMgr::sRenderHiddenSelections = TRUE;
BOOL LLSelectMgr::sRenderLightRadius = FALSE;
F32	LLSelectMgr::sHighlightThickness = 0.f;
F32	LLSelectMgr::sHighlightUScale = 0.f;
F32	LLSelectMgr::sHighlightVScale = 0.f;
F32	LLSelectMgr::sHighlightAlpha = 0.f;
F32	LLSelectMgr::sHighlightAlphaTest = 0.f;
F32	LLSelectMgr::sHighlightUAnim = 0.f;
F32	LLSelectMgr::sHighlightVAnim = 0.f;
LLColor4 LLSelectMgr::sSilhouetteParentColor;
LLColor4 LLSelectMgr::sSilhouetteChildColor;
LLColor4 LLSelectMgr::sHighlightInspectColor;
LLColor4 LLSelectMgr::sHighlightParentColor;
LLColor4 LLSelectMgr::sHighlightChildColor;
LLColor4 LLSelectMgr::sContextSilhouetteColor;

static LLObjectSelection* get_null_object_selection();
template<> 
	const LLHandle<LLObjectSelection>::NullFunc 
		LLHandle<LLObjectSelection>::sNullFunc = get_null_object_selection;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// struct LLDeRezInfo
//
// Used to keep track of important derez info. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct LLDeRezInfo
{
	EDeRezDestination mDestination;
	LLUUID mDestinationID;
	LLDeRezInfo(EDeRezDestination dest, const LLUUID& dest_id) :
		mDestination(dest), mDestinationID(dest_id) {}
};

//
// Imports
//


//
// Functions
//

LLObjectSelection* get_null_object_selection()
{
	static LLObjectSelectionHandle null_ptr(new LLObjectSelection());
	return (LLObjectSelection*)null_ptr;
}


//-----------------------------------------------------------------------------
// LLSelectMgr()
//-----------------------------------------------------------------------------
LLSelectMgr::LLSelectMgr()
{
	mTEMode = FALSE;
	mLastCameraPos.clearVec();

	sHighlightThickness	= gSavedSettings.getF32("SelectionHighlightThickness");
	sHighlightUScale	= gSavedSettings.getF32("SelectionHighlightUScale");
	sHighlightVScale	= gSavedSettings.getF32("SelectionHighlightVScale");
	sHighlightAlpha		= gSavedSettings.getF32("SelectionHighlightAlpha");
	sHighlightAlphaTest	= gSavedSettings.getF32("SelectionHighlightAlphaTest");
	sHighlightUAnim		= gSavedSettings.getF32("SelectionHighlightUAnim");
	sHighlightVAnim		= gSavedSettings.getF32("SelectionHighlightVAnim");

	sSilhouetteParentColor = gColors.getColor("SilhouetteParentColor");
	sSilhouetteChildColor = gColors.getColor("SilhouetteChildColor");
	sHighlightParentColor = gColors.getColor("HighlightParentColor");
	sHighlightChildColor = gColors.getColor("HighlightChildColor");
	sHighlightInspectColor = gColors.getColor("HighlightInspectColor");
	sContextSilhouetteColor = gColors.getColor("ContextSilhouetteColor")*0.5f;

	sRenderLightRadius = gSavedSettings.getBOOL("RenderLightRadius");
	
	mRenderSilhouettes = TRUE;

	mGridMode = GRID_MODE_WORLD;
	gSavedSettings.setS32("GridMode", (S32)GRID_MODE_WORLD);
	mGridValid = FALSE;

	mSelectedObjects = new LLObjectSelection();
	mHoverObjects = new LLObjectSelection();
	mHighlightedObjects = new LLObjectSelection();
}


//-----------------------------------------------------------------------------
// ~LLSelectMgr()
//-----------------------------------------------------------------------------
LLSelectMgr::~LLSelectMgr()
{
	mHoverObjects->deleteAllNodes();
	mSelectedObjects->deleteAllNodes();
	mHighlightedObjects->deleteAllNodes();
	mRectSelectedObjects.clear();
	mGridObjects.deleteAllNodes();
}

void LLSelectMgr::updateEffects()
{
	if (mEffectsTimer.getElapsedTimeF32() > 1.f)
	{
		mSelectedObjects->updateEffects();
		mEffectsTimer.reset();
	}
}

//-----------------------------------------------------------------------------
// Select just the object, not any other group members.
//-----------------------------------------------------------------------------
LLObjectSelectionHandle LLSelectMgr::selectObjectOnly(LLViewerObject* object, S32 face)
{
	llassert( object );

	// Don't add an object that is already in the list
	if (object->isSelected() ) {
		// make sure point at position is updated
		updatePointAt();
		gEditMenuHandler = this;
		return NULL;
	}

	if (!canSelectObject(object))
	{
		//make_ui_sound("UISndInvalidOp");
		return NULL;
	}

	// llinfos << "Adding object to selected object list" << llendl;

	// Place it in the list and tag it.
	// This will refresh dialogs.
	addAsIndividual(object, face);

	// Stop the object from moving (this anticipates changes on the
	// simulator in LLTask::userSelect)
	// *FIX: shouldn't zero out these either
	object->setVelocity(LLVector3::zero);
	object->setAcceleration(LLVector3::zero);
	//object->setAngularVelocity(LLVector3::zero);
	object->resetRot();

	// Always send to simulator, so you get a copy of the 
	// permissions structure back.
	gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID() );
	LLViewerRegion* regionp = object->getRegion();
	gMessageSystem->sendReliable( regionp->getHost());

	updatePointAt();
	updateSelectionCenter();
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);

	// have selection manager handle edit menu immediately after 
	// user selects an object
	if (mSelectedObjects->getObjectCount())
	{
		gEditMenuHandler = this;
	}

	return mSelectedObjects;
}

//-----------------------------------------------------------------------------
// Select the object, parents and children.
//-----------------------------------------------------------------------------
LLObjectSelectionHandle LLSelectMgr::selectObjectAndFamily(LLViewerObject* obj, BOOL add_to_end)
{
	llassert( obj );

	// This may be incorrect if things weren't family selected before... - djs 07/08/02
	// Don't add an object that is already in the list
	if (obj->isSelected() ) 
	{
		// make sure pointat position is updated
		updatePointAt();
		gEditMenuHandler = this;
		return NULL;
	}

	if (!canSelectObject(obj))
	{
		//make_ui_sound("UISndInvalidOp");
		return NULL;
	}

	// Since we're selecting a family, start at the root, but
	// don't include an avatar.
	LLViewerObject* root = obj;
	
	while(!root->isAvatar() && root->getParent() && !root->isJointChild())
	{
		LLViewerObject* parent = (LLViewerObject*)root->getParent();
		if (parent->isAvatar())
		{
			break;
		}
		root = parent;
	}

	// Collect all of the objects
	LLDynamicArray<LLViewerObject*> objects;

	root->addThisAndNonJointChildren(objects);
	addAsFamily(objects, add_to_end);

	updateSelectionCenter();
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	updatePointAt();

	dialog_refresh_all();

	// Always send to simulator, so you get a copy of the permissions
	// structure back.
	sendSelect();

	// Stop the object from moving (this anticipates changes on the
	// simulator in LLTask::userSelect)
	root->setVelocity(LLVector3::zero);
	root->setAcceleration(LLVector3::zero);
	//root->setAngularVelocity(LLVector3::zero);
	root->resetRot();

	// leave component mode
	if (!gSavedSettings.getBOOL("SelectLinkedSet"))
	{
		gSavedSettings.setBOOL("SelectLinkedSet", TRUE);
		promoteSelectionToRoot();
	}

	// have selection manager handle edit menu immediately after 
	// user selects an object
	if (mSelectedObjects->getObjectCount())
	{
		gEditMenuHandler = this;
	}

	return mSelectedObjects;
}

//-----------------------------------------------------------------------------
// Select the object, parents and children.
//-----------------------------------------------------------------------------
LLObjectSelectionHandle LLSelectMgr::selectObjectAndFamily(const LLDynamicArray<LLViewerObject*>& object_list,
										BOOL send_to_sim)
{
	// Collect all of the objects, children included
	LLDynamicArray<LLViewerObject*> objects;
	LLViewerObject *object;
	S32 i;

	if (object_list.count() < 1) return NULL;

	// NOTE -- we add the objects in REVERSE ORDER 
	// to preserve the order in the mSelectedObjects list
	for (i = object_list.count() - 1; i >= 0; i--)
	{
		object = object_list.get(i);

		llassert( object );

		if (!canSelectObject(object)) continue;

		object->addThisAndNonJointChildren(objects);
		addAsFamily(objects);

		// Stop the object from moving (this anticipates changes on the
		// simulator in LLTask::userSelect)
		object->setVelocity(LLVector3::zero);
		object->setAcceleration(LLVector3::zero);
		//object->setAngularVelocity(LLVector3::zero);
		object->resetRot();
	}

	updateSelectionCenter();
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	updatePointAt();
	dialog_refresh_all();

	// Almost always send to simulator, so you get a copy of the permissions
	// structure back.
	// JC: The one case where you don't want to do this is if you're selecting
	// all the objects on a sim.
	if (send_to_sim)
	{
		sendSelect();
	}

	// leave component mode
	if (!gSavedSettings.getBOOL("SelectLinkedSet"))
	{		
		gSavedSettings.setBOOL("SelectLinkedSet", TRUE);
		promoteSelectionToRoot();
	}

	// have selection manager handle edit menu immediately after 
	// user selects an object
	if (mSelectedObjects->getObjectCount())
	{
		gEditMenuHandler = this;
	}

	return mSelectedObjects;
}

// Use for when the simulator kills an object.  This version also
// handles informing the current tool of the object's deletion.
//
// Caller needs to call dialog_refresh_all if necessary.
BOOL LLSelectMgr::removeObjectFromSelections(const LLUUID &id)
{
	BOOL object_found = FALSE;
	LLTool *tool = NULL;
	if (!gNoRender)
	{
		tool = gToolMgr->getCurrentTool();

		// It's possible that the tool is editing an object that is not selected
		LLViewerObject* tool_editing_object = tool->getEditingObject();
		if( tool_editing_object && tool_editing_object->mID == id)
		{
			tool->stopEditing();
			object_found = TRUE;
		}
	}

	// Iterate through selected objects list and kill the object
	if( !object_found )
	{
		LLViewerObject* prevobjp = NULL;
		for( LLViewerObject* tobjp = mSelectedObjects->getFirstObject(); tobjp != NULL; tobjp = mSelectedObjects->getNextObject() )
		{
			if (tobjp == prevobjp)
			{
				// Somehow we got stuck in an infinite loop... (DaveP)
				//  this logic is kind of twisted, not sure how this is happening, so...
				llwarns << "Detected infinite loop #1 in LLSelectMgr::removeObjectFromSelections:|" << llendl;
				//MikeS. adding warning and comment...
				//These infinite loops happen because the LLSelectMgr iteration routines are non-reentrant.
				//deselectObjectAndFamily uses getFirstObject and getNextObject to mess with the array,
				//resetting the arrays internal iterator state. This needs fixing BAD.
				continue;
			}
			// It's possible the item being removed has an avatar sitting on it
			// So remove the avatar that is sitting on the object.
			if (tobjp->mID == id || tobjp->isAvatar())
			{
				if (!gNoRender)
				{
					tool->stopEditing();
				}

				// lose the selection, don't tell simulator, it knows
				deselectObjectAndFamily(tobjp, FALSE);

				if (tobjp->mID == id)
				{
					if(object_found == TRUE){
						//MikeS. adding warning... This happens when removing a linked attachment while sitting on an object..
						//I think the selection manager needs to be rewritten. BAD.
						llwarns << "Detected infinite loop #2 in LLSelectMgr::removeObjectFromSelections:|" << llendl;
						break;
					}
					object_found = TRUE;
				}
			}
			prevobjp = tobjp;
		}
	}

	return object_found;
}

void LLSelectMgr::deselectObjectAndFamily(LLViewerObject* object, BOOL send_to_sim)
{
	// bail if nothing selected or if object wasn't selected in the first place
	if(!object) return;
	if(!object->isSelected()) return;

	// Collect all of the objects, and remove them
	LLDynamicArray<LLViewerObject*> objects;
	object = (LLViewerObject*)object->getRoot();
	object->addThisAndAllChildren(objects);
	remove(objects);

	if (!send_to_sim) return;

	//-----------------------------------------------------------
	// Inform simulator of deselection
	//-----------------------------------------------------------
	LLViewerRegion* regionp = object->getRegion();

	BOOL start_new_message = TRUE;
	S32 select_count = 0;

	LLMessageSystem* msg = gMessageSystem;
	for (S32 i = 0; i < objects.count(); i++)
	{
		if (start_new_message)
		{
			msg->newMessageFast(_PREHASH_ObjectDeselect);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			select_count++;
			start_new_message = FALSE;
		}

		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_ObjectLocalID, (objects[i])->getLocalID());
		select_count++;

		if(msg->mCurrentSendTotal >= MTUBYTES || select_count >= MAX_OBJECTS_PER_PACKET)
		{
			msg->sendReliable(regionp->getHost() );
			select_count = 0;
			start_new_message = TRUE;
		}
	}

	if (!start_new_message)
	{
		msg->sendReliable(regionp->getHost() );
	}

	updatePointAt();
	updateSelectionCenter();
}

void LLSelectMgr::deselectObjectOnly(LLViewerObject* object, BOOL send_to_sim)
{
	// bail if nothing selected or if object wasn't selected in the first place
	if (!object) return;
	if (!object->isSelected() ) return;

	if (send_to_sim)
	{
		LLViewerRegion* region = object->getRegion();
		gMessageSystem->newMessageFast(_PREHASH_ObjectDeselect);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID() );
		gMessageSystem->sendReliable(region->getHost());
	}

	// This will refresh dialogs.
	remove( object );

	updatePointAt();
	updateSelectionCenter();
}


//-----------------------------------------------------------------------------
// addAsFamily
//-----------------------------------------------------------------------------

void LLSelectMgr::addAsFamily(LLDynamicArray<LLViewerObject*>& objects, BOOL add_to_end)
{
	S32 count = objects.count();
	LLViewerObject *objectp = NULL;

	LLSelectNode *nodep = NULL;
	for (S32 i = 0; i < count; i++)
	{
		objectp = objects.get(i);
		
		// Can't select yourself
		if (objectp->mID == gAgentID
			&& !gAllowSelectAvatar)
		{
			continue;
		}

		if (!objectp->isSelected())
		{
			nodep = new LLSelectNode(objectp, TRUE);
			if (add_to_end)
			{
				mSelectedObjects->addNodeAtEnd(nodep);
			}
			else
			{
				mSelectedObjects->addNode(nodep);
			}
			objectp->setSelected(TRUE);

			if (objectp->getNumTEs() > 0)
			{
				nodep->selectAllTEs(TRUE);
			}
			else
			{
				// object has no faces, so don't mess with faces
			}
		}
		else
		{
			// we want this object to be selected for real
			// so clear transient flag
			LLSelectNode* select_node = mSelectedObjects->findNode(objectp);
			if (select_node)
			{
				select_node->setTransient(FALSE);
			}
		}
	}
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
}

//-----------------------------------------------------------------------------
// addAsIndividual() - a single object, face, etc
//-----------------------------------------------------------------------------
void LLSelectMgr::addAsIndividual(LLViewerObject *objectp, S32 face, BOOL undoable)
{
	// check to see if object is already in list
	LLSelectNode *nodep = mSelectedObjects->findNode(objectp);

	// if not in list, add it
	if (!nodep)
	{
		nodep = new LLSelectNode(objectp, TRUE);
		mSelectedObjects->addNode(nodep);
	}
	else
	{
		// make this a full-fledged selection
		nodep->setTransient(FALSE);
		// Move it to the front of the list
		mSelectedObjects->removeNode(nodep);
		mSelectedObjects->addNode(nodep);
	}

	// Make sure the object is tagged as selected
	objectp->setSelected( TRUE );

	// And make sure we don't consider it as part of a family
	nodep->mIndividualSelection = TRUE;

	// Handle face selection
	if (objectp->getNumTEs() <= 0)
	{
		// object has no faces, so don't do anything
	}
	else if (face == SELECT_ALL_TES)
	{
		nodep->selectAllTEs(TRUE);
	}
	else if (0 <= face && face < SELECT_MAX_TES)
	{
		nodep->selectTE(face, TRUE);
	}
	else
	{
		llerrs << "LLSelectMgr::add face " << face << " out-of-range" << llendl;
		return;
	}

	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	updateSelectionCenter();
	dialog_refresh_all();
}


LLObjectSelectionHandle LLSelectMgr::setHoverObject(LLViewerObject *objectp)
{
	// Always blitz hover list when setting
	mHoverObjects->deleteAllNodes();

	if (!objectp)
	{
		return NULL;
	}

	// Can't select yourself
	if (objectp->mID == gAgentID)
	{
		return NULL;
	}

	// Can't select land
	if (objectp->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH)
	{
		return NULL;
	}

	// Collect all of the objects
	LLDynamicArray<LLViewerObject*> objects;
	objectp = objectp->getRootEdit();
	objectp->addThisAndNonJointChildren(objects);


	S32 count = objects.count();
	LLViewerObject* cur_objectp = NULL;
	LLSelectNode* nodep = NULL;
	for(S32 i = 0; i < count; i++)
	{
		cur_objectp = objects[i];
		nodep = new LLSelectNode(cur_objectp, FALSE);
		mHoverObjects->addNodeAtEnd(nodep);
	}

	requestObjectPropertiesFamily(objectp);
	return mHoverObjects;
}

LLSelectNode *LLSelectMgr::getHoverNode()
{
	return getHoverObjects()->getFirstRootNode();
}

void LLSelectMgr::highlightObjectOnly(LLViewerObject* objectp)
{
	if (!objectp)
	{
		return;
	}

	if (objectp->getPCode() != LL_PCODE_VOLUME)
	{
		return;
	}
	
	if ((gSavedSettings.getBOOL("SelectOwnedOnly") && !objectp->permYouOwner()) ||
		(gSavedSettings.getBOOL("SelectMovableOnly") && !objectp->permMove()))
	{
		// only select my own objects
		return;
	}

	mRectSelectedObjects.insert(objectp);
}

void LLSelectMgr::highlightObjectAndFamily(LLViewerObject* objectp)
{
	if (!objectp)
	{
		return;
	}

	LLViewerObject* root_obj = (LLViewerObject*)objectp->getRoot();

	highlightObjectOnly(root_obj);

	for(U32 i = 0; i < root_obj->mChildList.size(); i++)
	{
		highlightObjectOnly(root_obj->mChildList[i]);
	}
}

// Note that this ignores the "select owned only" flag
// It's also more efficient than calling the single-object version over and
// over.
void LLSelectMgr::highlightObjectAndFamily(const LLDynamicArray<LLViewerObject*>& list)
{
	S32 i;
	S32 count = list.count();

	for (i = 0; i < count; i++)
	{
		LLViewerObject* object = list.get(i);

		if (!object) continue;
		if (object->getPCode() != LL_PCODE_VOLUME) continue;

		LLViewerObject* root = (LLViewerObject*)object->getRoot();
		mRectSelectedObjects.insert(root);

		S32 j;
		S32 child_count = root->mChildList.size();
		for (j = 0; j < child_count; j++)
		{
			LLViewerObject* child = root->mChildList[j];
			mRectSelectedObjects.insert(child);
		}
	}
}

void LLSelectMgr::unhighlightObjectOnly(LLViewerObject* objectp)
{
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}

	mRectSelectedObjects.erase(objectp);
}

void LLSelectMgr::unhighlightObjectAndFamily(LLViewerObject* objectp)
{
	if (!objectp)
	{
		return;
	}

	LLViewerObject* root_obj = (LLViewerObject*)objectp->getRoot();

	unhighlightObjectOnly(root_obj);

	for(U32 i = 0; i < root_obj->mChildList.size(); i++)
	{
		unhighlightObjectOnly(root_obj->mChildList[i]);
	}
}


void LLSelectMgr::unhighlightAll()
{
	mRectSelectedObjects.clear();
	mHighlightedObjects->deleteAllNodes();
}

LLObjectSelectionHandle LLSelectMgr::selectHighlightedObjects()
{
	if (!mHighlightedObjects->getNumNodes())
	{
		return NULL;
	}

	LLSelectNode *nodep;
	for (nodep = mHighlightedObjects->getFirstNode();
		nodep;
		nodep = mHighlightedObjects->getNextNode())
	{
		LLViewerObject* objectp = nodep->getObject();

		if (!canSelectObject(objectp))
		{
			continue;
		}

		// already selected
		if (objectp->isSelected())
		{
			continue;
		}

		LLSelectNode* new_nodep = new LLSelectNode(*nodep);
		mSelectedObjects->addNode(new_nodep);

		// flag this object as selected
		objectp->setSelected(TRUE);

		mSelectedObjects->mSelectType = getSelectTypeForObject(objectp);

		// request properties on root objects
		if (objectp->isRootEdit())
		{
			requestObjectPropertiesFamily(objectp);
		}
	}

	// pack up messages to let sim know these objects are selected
	sendSelect();
	unhighlightAll();
	updateSelectionCenter();
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	updatePointAt();

	if (mSelectedObjects->getObjectCount())
	{
		gEditMenuHandler = this;
	}

	return mSelectedObjects;
}

void LLSelectMgr::deselectHighlightedObjects()
{
	BOOL select_linked_set = gSavedSettings.getBOOL("SelectLinkedSet");
	for (std::set<LLPointer<LLViewerObject> >::iterator iter = mRectSelectedObjects.begin();
		 iter != mRectSelectedObjects.end(); iter++)
	{
		LLViewerObject *objectp = *iter;
		if (!select_linked_set)
		{
			deselectObjectOnly(objectp);
		}
		else
		{
			LLViewerObject* root_object = (LLViewerObject*)objectp->getRoot();
			if (root_object->isSelected())
			{
				deselectObjectAndFamily(root_object);
			}
		}
	}

	unhighlightAll();
}

void LLSelectMgr::addGridObject(LLViewerObject* objectp)
{
	LLSelectNode* nodep = new LLSelectNode(objectp, FALSE);
	mGridObjects.addNodeAtEnd(nodep);
	for (U32 i = 0; i < objectp->mChildList.size(); i++)
	{
		nodep = new LLSelectNode(objectp->mChildList[i], FALSE);
		mGridObjects.addNodeAtEnd(nodep);
	}
}

void LLSelectMgr::clearGridObjects()
{
	mGridObjects.deleteAllNodes();
}

void LLSelectMgr::setGridMode(EGridMode mode)
{
	mGridMode = mode;
	gSavedSettings.setS32("GridMode", mode);
	updateSelectionCenter();
	mGridValid = FALSE;
}

void LLSelectMgr::getGrid(LLVector3& origin, LLQuaternion &rotation, LLVector3 &scale)
{
	LLSelectNode* grid_node = mGridObjects.getFirstNode();
	LLViewerObject* grid_object = mGridObjects.getFirstObject();
	// *TODO: get to work with multiple grid objects
	if (grid_node && grid_node->getObject()->isDead())
	{
		mGridObjects.removeNode(grid_node);
		grid_object = NULL;
	}

	if (mGridMode == GRID_MODE_LOCAL && mSelectedObjects->getObjectCount())
	{
		LLBBox bbox = mSavedSelectionBBox;
		mGridOrigin = mSavedSelectionBBox.getCenterAgent();
		mGridRotation = mSavedSelectionBBox.getRotation();
		mGridScale = mSavedSelectionBBox.getExtentLocal() * 0.5f;
	}
	else if (mGridMode == GRID_MODE_REF_OBJECT && grid_object && grid_object->mDrawable.notNull())
	{
		mGridRotation = grid_object->getRenderRotation();
		LLVector3 first_grid_obj_pos = grid_object->getRenderPosition();

		LLVector3 min_extents(F32_MAX, F32_MAX, F32_MAX);
		LLVector3 max_extents(F32_MIN, F32_MIN, F32_MIN);
		BOOL grid_changed = FALSE;
		LLSelectNode* grid_nodep;
		for (grid_nodep = mGridObjects.getFirstNode();
			grid_nodep;
			grid_nodep = mGridObjects.getNextNode())
			{
				grid_object = grid_nodep->getObject();

				LLVector3 local_min_extents(F32_MAX, F32_MAX, F32_MAX);
				LLVector3 local_max_extents(F32_MIN, F32_MIN, F32_MIN);

				// *FIX: silhouette flag is insufficient as it gets
				// cleared by view update.
				if (!mGridValid || 
					grid_object->isChanged(LLXform::SILHOUETTE)
					|| (grid_object->getParent() && grid_object->getParent()->isChanged(LLXform::SILHOUETTE)))
				{
					getSilhouetteExtents(grid_nodep, mGridRotation, local_min_extents, local_max_extents);
					grid_changed = TRUE;
					LLVector3 object_offset = (grid_object->getRenderPosition() - first_grid_obj_pos) * ~mGridRotation;
					local_min_extents += object_offset;
					local_max_extents += object_offset;
				}
				min_extents.mV[VX] = llmin(min_extents.mV[VX], local_min_extents.mV[VX]);
				min_extents.mV[VY] = llmin(min_extents.mV[VY], local_min_extents.mV[VY]);
				min_extents.mV[VZ] = llmin(min_extents.mV[VZ], local_min_extents.mV[VZ]);
				max_extents.mV[VX] = llmax(max_extents.mV[VX], local_max_extents.mV[VX]);
				max_extents.mV[VY] = llmax(max_extents.mV[VY], local_max_extents.mV[VY]);
				max_extents.mV[VZ] = llmax(max_extents.mV[VZ], local_max_extents.mV[VZ]);
			}
		if (grid_changed)
		{
			mGridOrigin = lerp(min_extents, max_extents, 0.5f);
			mGridOrigin = mGridOrigin * ~mGridRotation;
			mGridOrigin += first_grid_obj_pos;
			mGridScale = (max_extents - min_extents) * 0.5f;
		}
	}
	else // GRID_MODE_WORLD or just plain default
	{
		LLViewerObject* first_object = mSelectedObjects->getFirstRootObject();
		if (!first_object)
		{
			first_object = mSelectedObjects->getFirstObject();
		}

		mGridOrigin.clearVec();
		mGridRotation.loadIdentity();

		mSelectedObjects->mSelectType = getSelectTypeForObject( first_object );

		switch (mSelectedObjects->mSelectType)
		{
		case SELECT_TYPE_ATTACHMENT:
			if (first_object)
			{
				// this means this object *has* to be an attachment
				LLXform* attachment_point_xform = first_object->getRootEdit()->mDrawable->mXform.getParent();
				mGridOrigin = attachment_point_xform->getWorldPosition();
				mGridRotation = attachment_point_xform->getWorldRotation();
				mGridScale = LLVector3(1.f, 1.f, 1.f) * gSavedSettings.getF32("GridResolution");
			}
			break;
		case SELECT_TYPE_HUD:
			// use HUD-scaled grid
			mGridScale = LLVector3(0.25f, 0.25f, 0.25f);
			break;
		case SELECT_TYPE_WORLD:
			mGridScale = LLVector3(1.f, 1.f, 1.f) * gSavedSettings.getF32("GridResolution");
			break;
		}
	}
	llassert(mGridOrigin.isFinite());

	origin = mGridOrigin;
	rotation = mGridRotation;
	scale = mGridScale;
	mGridValid = TRUE;
}

//-----------------------------------------------------------------------------
// remove() - an array of objects
//-----------------------------------------------------------------------------

void LLSelectMgr::remove(LLDynamicArray<LLViewerObject*>& objects)
{
	S32 count = objects.count();
	LLViewerObject *objectp = NULL;
	LLSelectNode *nodep = NULL;
	for(S32 i = 0; i < count; i++)
	{
		objectp = objects.get(i);
		for(nodep = mSelectedObjects->getFirstNode();
			nodep != NULL;
			nodep = mSelectedObjects->getNextNode())
		{
			if(nodep->getObject() == objectp)
			{
				objectp->setSelected(FALSE);
				mSelectedObjects->removeNode(nodep);
				break;
			}
		}
			}
	updateSelectionCenter();
	dialog_refresh_all();
}


//-----------------------------------------------------------------------------
// remove() - a single object
//-----------------------------------------------------------------------------
void LLSelectMgr::remove(LLViewerObject *objectp, S32 te, BOOL undoable)
{
	// check if object already in list
	// *FIX: can we just check isSelected()?
	LLSelectNode *nodep = mSelectedObjects->findNode(objectp);

	if (!nodep)
	{
		return;
	}


	// if face = all, remove object from list
	if (objectp->getNumTEs() <= 0)
	{
		// object doesn't have faces, so blow it away
		mSelectedObjects->removeNode(nodep);
		objectp->setSelected( FALSE );
	}
	else if (te == SELECT_ALL_TES)
	{
		mSelectedObjects->removeNode(nodep);
		objectp->setSelected( FALSE );
	}
	else if (0 <= te && te < SELECT_MAX_TES)
	{
		// ...valid face, check to see if it was on
		if (nodep->isTESelected(te))
		{
			nodep->selectTE(te, FALSE);
		}
		else
		{
			llerrs << "LLSelectMgr::remove - tried to remove TE " << te << " that wasn't selected" << llendl;
			return;
		}

		// ...check to see if this operation turned off all faces
		BOOL found = FALSE;
		for (S32 i = 0; i < nodep->getObject()->getNumTEs(); i++)
		{
			found = found || nodep->isTESelected(i);
		}

		// ...all faces now turned off, so remove
		if (!found)
		{
			mSelectedObjects->removeNode(nodep);
			objectp->setSelected( FALSE );

			// BUG: Doesn't update simulator that object is gone.
		}
	}
	else
	{
		// ...out of range face
		llerrs << "LLSelectMgr::remove - TE " << te << " out of range" << llendl;
	}

	updateSelectionCenter();
	dialog_refresh_all();
}


//-----------------------------------------------------------------------------
// removeAll()
//-----------------------------------------------------------------------------
void LLSelectMgr::removeAll()
{
	LLViewerObject *objectp;
	for (objectp = mSelectedObjects->getFirstObject(); objectp; objectp = mSelectedObjects->getNextObject() )
	{
		objectp->setSelected( FALSE );
	}

	mSelectedObjects->deleteAllNodes();

	updateSelectionCenter();
	dialog_refresh_all();
}

//-----------------------------------------------------------------------------
// promoteSelectionToRoot()
//-----------------------------------------------------------------------------
void LLSelectMgr::promoteSelectionToRoot()
{
	std::set<LLViewerObject*> selection_set;

	BOOL selection_changed = FALSE;

	LLSelectNode* nodep;
	LLViewerObject *objectp;
	for (nodep = mSelectedObjects->getFirstNode(); 
		 nodep; 
		 nodep = mSelectedObjects->getNextNode() )
	{
		if (nodep->mIndividualSelection)
		{
			selection_changed = TRUE;
		}

		objectp = nodep->getObject();
		LLViewerObject* parentp = objectp;
		while(parentp->getParent() && !(parentp->isRootEdit() || parentp->isJointChild()))
		{
			parentp = (LLViewerObject*)parentp->getParent();
		}
	
		selection_set.insert(parentp);
	}

	if (selection_changed)
	{
		deselectAll();

		std::set<LLViewerObject*>::iterator set_iter;
		for (set_iter = selection_set.begin(); set_iter != selection_set.end(); ++set_iter)
		{
			selectObjectAndFamily(*set_iter);
		}
	}
}

//-----------------------------------------------------------------------------
// demoteSelectionToIndividuals()
//-----------------------------------------------------------------------------
void LLSelectMgr::demoteSelectionToIndividuals()
{
	LLDynamicArray<LLViewerObject*> objects;

	for (LLViewerObject* root_objectp = mSelectedObjects->getFirstRootObject();
		root_objectp;
		root_objectp = mSelectedObjects->getNextRootObject())
	{
		root_objectp->addThisAndNonJointChildren(objects);
	}

	if (objects.getLength())
	{
		deselectAll();
		for(S32 i = 0; i < objects.count(); i++)
		{
			selectObjectOnly(objects[i]);
		}
	}
}

//-----------------------------------------------------------------------------
// dump()
//-----------------------------------------------------------------------------
void LLSelectMgr::dump()
{
	llinfos << "Selection Manager: " << mSelectedObjects->getNumNodes() << " items" << llendl;

	llinfos << "TE mode " << mTEMode << llendl;

	S32 i = 0;

	LLViewerObject *objectp;
	for (objectp = mSelectedObjects->getFirstObject();
		 objectp;
		 objectp = mSelectedObjects->getNextObject())
	{
		llinfos << "Object " << i << " type " << LLPrimitive::pCodeToString(objectp->getPCode()) << llendl;
		llinfos << "  hasLSL " << objectp->flagScripted() << llendl;
		llinfos << "  hasTouch " << objectp->flagHandleTouch() << llendl;
		llinfos << "  hasMoney " << objectp->flagTakesMoney() << llendl;
		llinfos << "  getposition " << objectp->getPosition() << llendl;
		llinfos << "  getpositionAgent " << objectp->getPositionAgent() << llendl;
		llinfos << "  getpositionRegion " << objectp->getPositionRegion() << llendl;
		llinfos << "  getpositionGlobal " << objectp->getPositionGlobal() << llendl;
		LLDrawable* drawablep = objectp->mDrawable;
		llinfos << "  " << (drawablep&& drawablep->isVisible() ? "visible" : "invisible") << llendl;
		llinfos << "  " << (drawablep&& drawablep->isState(LLDrawable::FORCE_INVISIBLE) ? "force_invisible" : "") << llendl;
		i++;
	}

	// Face iterator
	S32 te;
	for (mSelectedObjects->getFirstTE(&objectp, &te);
		 objectp;
		 mSelectedObjects->getNextTE(&objectp, &te))
	{
		llinfos << "Object " << objectp << " te " << te << llendl;
	}

	llinfos << mHighlightedObjects->getNumNodes() << " objects currently highlighted." << llendl;

	llinfos << "Center global " << mSelectionCenterGlobal << llendl;
}

//-----------------------------------------------------------------------------
// cleanup()
//-----------------------------------------------------------------------------
void LLSelectMgr::cleanup()
{
	mSilhouetteImagep = NULL;
}


//---------------------------------------------------------------------------
// Manipulate properties of selected objects
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// selectionSetImage()
//-----------------------------------------------------------------------------
// *TODO: re-arch texture applying out of lltooldraganddrop
void LLSelectMgr::selectionSetImage(const LLUUID& imageid)
{
	// First for (no copy) textures and multiple object selection
	LLViewerInventoryItem* item = gInventory.getItem(imageid);

	if(item 
		&& !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())
		&& (mSelectedObjects->getNumNodes() > 1) )
	{
		llwarns << "Attempted to apply no-copy texture to multiple objects"
				<< llendl;
		return;
	}

	LLViewerObject* objectp;
	S32 te;

	mSelectedObjects->getFirstTE(&objectp, &te);
	
	for (objectp = mSelectedObjects->getFirstObject(); objectp; objectp = mSelectedObjects->getNextObject())
	{
		if (item)
		{
			LLToolDragAndDrop::dropTextureAllFaces(objectp,
											item,
											LLToolDragAndDrop::SOURCE_AGENT,
											LLUUID::null);
		}
		else
		{
			S32 num_faces = objectp->getNumTEs();
			for( S32 face = 0; face < num_faces; face++ )
			{
				// Texture picker defaults aren't inventory items
				// * Don't need to worry about permissions for them
				// * Can just apply the texture and be done with it.
				objectp->setTEImage(face, gImageList.getImage(imageid));
			}
			objectp->sendTEUpdate();
		}
	}


	// 1 particle effect per object
	if (mSelectedObjects->mSelectType != SELECT_TYPE_HUD)
	{
		for (objectp = mSelectedObjects->getFirstObject(); objectp; objectp = mSelectedObjects->getNextObject())
		{
			LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
			effectp->setSourceObject(gAgent.getAvatarObject());     
			effectp->setTargetObject(objectp);                      
			effectp->setDuration(LL_HUD_DUR_SHORT);                 
			effectp->setColor(LLColor4U(gAgent.getEffectColor()));  
		}
	}
}

//-----------------------------------------------------------------------------
// selectionSetColor()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetColor(const LLColor4 &color)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTEColor(te, color);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

//-----------------------------------------------------------------------------
// selectionSetColorOnly()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetColorOnly(const LLColor4 &color)
{
	LLViewerObject* object;
	LLColor4 new_color = color;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			LLColor4 prev_color = object->getTE(te)->getColor();
			new_color.mV[VALPHA] = prev_color.mV[VALPHA];
			// update viewer side color in anticipation of update from simulator
			object->setTEColor(te, new_color);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

//-----------------------------------------------------------------------------
// selectionSetAlphaOnly()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetAlphaOnly(const F32 alpha)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			LLColor4 prev_color = object->getTE(te)->getColor();
			prev_color.mV[VALPHA] = alpha;
			// update viewer side color in anticipation of update from simulator
			object->setTEColor(te, prev_color);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

void LLSelectMgr::selectionRevertColors()
{
	LLViewerObject* object;
	S32 te;

	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			LLSelectNode* nodep = mSelectedObjects->findNode(object);
			if (nodep && te < (S32)nodep->mSavedColors.size())
			{
				LLColor4 color = nodep->mSavedColors[te];
				// update viewer side color in anticipation of update from simulator
				object->setTEColor(te, color);
			}
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

BOOL LLSelectMgr::selectionRevertTextures()
{
	LLViewerObject* object;
	S32 te;

	BOOL revert_successful = TRUE;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			LLSelectNode* nodep = mSelectedObjects->findNode(object);
			if (nodep && te < (S32)nodep->mSavedTextures.size())
			{
				LLUUID id = nodep->mSavedTextures[te];
				// update textures on viewer side
				if (id.isNull())
				{
					// this was probably a no-copy texture, leave image as-is
					revert_successful = FALSE;
				}
				else
				{
					object->setTEImage(te, gImageList.getImage(id));
				}
			}
		}
	}

	// propagate texture changes to server
	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}

	return revert_successful;
}

void LLSelectMgr::selectionSetBumpmap(U8 bumpmap)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTEBumpmap(te, bumpmap);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

void LLSelectMgr::selectionSetTexGen(U8 texgen)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTETexGen(te, texgen);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}


void LLSelectMgr::selectionSetShiny(U8 shiny)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTEShiny(te, shiny);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}

void LLSelectMgr::selectionSetFullbright(U8 fullbright)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTEFullbright(te, fullbright);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
			if (fullbright)
			{
				U8 material = object->getMaterial();
				U8 mcode = material & LL_MCODE_MASK;
				if (mcode == LL_MCODE_LIGHT)
				{
					mcode = LL_MCODE_GLASS;
					material = (material & ~LL_MCODE_MASK) | mcode;
					object->setMaterial(material);
					object->sendMaterialUpdate();
				}
			}
		}
	}
}

void LLSelectMgr::selectionSetMediaTypeAndURL(U8 media_type, const std::string& media_url)
{
	LLViewerObject* object;
	S32 te;
	U8 media_flags = LLTextureEntry::MF_NONE;
	if (media_type == LLViewerObject::MEDIA_TYPE_WEB_PAGE)
	{
		media_flags = LLTextureEntry::MF_WEB_PAGE;
	}

	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te) )
	{
		if (object->permModify())
		{
			// update viewer side color in anticipation of update from simulator
			object->setTEMediaFlags(te, media_flags);
		}
	}

	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			// JAMESDEBUG TODO set object media type
			object->setMediaType(media_type);
			object->setMediaURL(media_url);

			object->sendTEUpdate();
		}
	}
}



//-----------------------------------------------------------------------------
// findObjectPermissions()
//-----------------------------------------------------------------------------
LLPermissions* LLSelectMgr::findObjectPermissions(const LLViewerObject* object)
{
	LLSelectNode* nodep;

	for (nodep = mSelectedObjects->getFirstNode(); nodep; nodep = mSelectedObjects->getNextNode() )
	{
		if((nodep->getObject() == object) && nodep->mValid)
		{
			return nodep->mPermissions;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// selectionGetTexUUID()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetTexUUID(LLUUID& id)
{
	LLViewerObject* first_objectp;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_objectp, &first_te);

	// nothing selected
	if (!first_objectp)
	{
		return FALSE;
	}
	
	LLViewerImage* first_imagep = first_objectp->getTEImage(first_te);
	
	if (!first_imagep)
	{
		return FALSE;
	}

	BOOL identical = TRUE;
	LLViewerObject *objectp;
	S32 te;
	for (mSelectedObjects->getFirstTE(&objectp, &te); objectp; mSelectedObjects->getNextTE(&objectp, &te) )
	{
		if (objectp->getTEImage(te) != first_imagep)
		{
			identical = FALSE;
			break;
		}
	}

	id = first_imagep->getID();
	return identical;
}

//-----------------------------------------------------------------------------
// selectionGetColor()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetColor(LLColor4 &color)
{
	LLViewerObject* first_object;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_object, &first_te);

	// nothing selected
	if (!first_object)
	{
		return FALSE;
	}

	LLColor4 first_color;
	if (!first_object->getTE(first_te))
	{
		return FALSE;
	}
	else
	{
		first_color = first_object->getTE(first_te)->getColor();
	}

	BOOL identical = TRUE;
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->getTE(te) || (object->getTE(te)->getColor() != first_color))
		{
			identical = FALSE;
			break;
		}
	}

	color = first_color;
	return identical;
}


//-----------------------------------------------------------------------------
// selectionGetBumpmap()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetBumpmap(U8 *bumpmap)
{
	LLViewerObject* first_object;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_object, &first_te);

	// nothing selected
	if (!first_object)
	{
		return FALSE;
	}

	U8 first_value;
	if (!first_object->getTE(first_te))
	{
		return FALSE;
	}
	else
	{
		first_value = first_object->getTE(first_te)->getBumpmap();
	}

	BOOL identical = TRUE;
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->getTE(te) || (object->getTE(te)->getBumpmap() != first_value))
		{
			identical = FALSE;
			break;
		}
	}

	*bumpmap = first_value;
	return identical;
}

//-----------------------------------------------------------------------------
// selectionGetShiny()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetShiny(U8 *shiny)
{
	LLViewerObject* first_object;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_object, &first_te);

	// nothing selected
	if (!first_object)
	{
		return FALSE;
	}

	U8 first_value;
	if (!first_object->getTE(first_te))
	{
		return FALSE;
	}
	else
	{
		first_value = first_object->getTE(first_te)->getShiny();
	}

	BOOL identical = TRUE;
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->getTE(te) || (object->getTE(te)->getShiny() != first_value))
		{
			identical = FALSE;
			break;
		}
	}

	*shiny = first_value;
	return identical;
}

//-----------------------------------------------------------------------------
// selectionGetFullbright()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetFullbright(U8 *fullbright)
{
	LLViewerObject* first_object;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_object, &first_te);

	// nothing selected
	if (!first_object)
	{
		return FALSE;
	}

	U8 first_value;
	if (!first_object->getTE(first_te))
	{
		return FALSE;
	}
	else
	{
		first_value = first_object->getTE(first_te)->getFullbright();
	}

	BOOL identical = TRUE;
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->getTE(te) || (object->getTE(te)->getFullbright() != first_value))
		{
			identical = FALSE;
			break;
		}
	}

	*fullbright = first_value;
	return identical;
}

// JAMESDEBUG TODO make this return mediatype off viewer object
bool LLSelectMgr::selectionGetMediaType(U8 *media_type)
{
	LLViewerObject* first_object;
	S32 first_te;
	mSelectedObjects->getPrimaryTE(&first_object, &first_te);

	// nothing selected
	if (!first_object)
	{
		return false;
	}

	U8 first_value;
	if (!first_object->getTE(first_te))
	{
		return false;
	}
	else
	{
		first_value = first_object->getTE(first_te)->getMediaFlags();
	}

	bool identical = true;
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->getTE(te) || (object->getTE(te)->getMediaFlags() != first_value))
		{
			identical = false;
			break;
		}
	}

	*media_type = first_value;
	return identical;
}



//-----------------------------------------------------------------------------
// selectionSetMaterial()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetMaterial(U8 material)
{
	LLViewerObject* object;
	for (object = mSelectedObjects->getFirstObject(); object != NULL; object = mSelectedObjects->getNextObject() )
	{
		if (object->permModify())
		{
			U8 cur_material = object->getMaterial();
			material |= (cur_material & ~LL_MCODE_MASK);
			object->setMaterial(material);
			object->sendMaterialUpdate();
		}
	}
}

// True if all selected objects have this PCode
BOOL LLSelectMgr::selectionAllPCode(LLPCode code)
{
	LLViewerObject *object;
	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (object->getPCode() != code)
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectionGetMaterial()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetMaterial(U8 *out_material)
{
	LLViewerObject *object = mSelectedObjects->getFirstObject();
	if (!object) return FALSE;

	U8 material = object->getMaterial();

	BOOL identical = TRUE;
	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if ( material != object->getMaterial())
		{
			identical = FALSE;
			break;
		}
	}

	*out_material = material;
	return identical;
}

BOOL LLSelectMgr::selectionGetClickAction(U8 *out_action)
{
	LLViewerObject *object = mSelectedObjects->getFirstObject();
	if (!object) return FALSE;

	U8 action = object->getClickAction();

	BOOL identical = TRUE;
	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if ( action != object->getClickAction())
		{
			identical = FALSE;
			break;
		}
	}

	*out_action = action;
	return identical;
}

void LLSelectMgr::selectionSetClickAction(U8 action)
{
	LLViewerObject* object = NULL;
	for ( object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		object->setClickAction(action);
	}
	sendListToRegions(
		"ObjectClickAction",
		packAgentAndSessionID,
		packObjectClickAction, 
		&action,
		SEND_INDIVIDUALS);
}


//-----------------------------------------------------------------------------
// godlike requests
//-----------------------------------------------------------------------------

typedef std::pair<const LLString, const LLString> godlike_request_t;
void LLSelectMgr::sendGodlikeRequest(const LLString& request, const LLString& param)
{
	// If the agent is neither godlike nor an estate owner, the server
	// will reject the request.
	LLString message_type;
	if (gAgent.isGodlike())
	{
		message_type = "GodlikeMessage";
	}
	else
	{
		message_type = "EstateOwnerMessage";
	}

	godlike_request_t data(request, param);
	if(!mSelectedObjects->getRootObjectCount())
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage(message_type.c_str());
		LLSelectMgr::packGodlikeHead(&data);
		gAgent.sendReliableMessage();
	}
	else
	{
		sendListToRegions(message_type, packGodlikeHead, packObjectIDAsParam, &data, SEND_ONLY_ROOTS);
	}
}

void LLSelectMgr::packGodlikeHead(void* user_data)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUID("TransactionID", LLUUID::null);
	godlike_request_t* data = (godlike_request_t*)user_data;
	msg->nextBlock("MethodData");
	msg->addString("Method", data->first.c_str());
	msg->addUUID("Invoice", LLUUID::null);

	// The parameters used to be restricted to either string or
	// integer. This mimics that behavior under the new 'string-only'
	// parameter list by not packing a string if there wasn't one
	// specified. The object ids will be packed in the
	// packObjectIDAsParam() method.
	if(data->second.size() > 0)
	{
		msg->nextBlock("ParamList");
		msg->addString("Parameter", data->second);
	}
}

// static
void LLSelectMgr::packObjectIDAsParam(LLSelectNode* node, void *)
{
	char buf [MAX_STRING];		/* Flawfinder: ignore */
	snprintf(buf, MAX_STRING, "%u", node->getObject()->getLocalID());		/* Flawfinder: ignore */
	gMessageSystem->nextBlock("ParamList");
	gMessageSystem->addString("Parameter", buf);
}

//-----------------------------------------------------------------------------
// Rotation options
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionResetRotation()
{
	LLQuaternion identity(0.f, 0.f, 0.f, 1.f);

	LLViewerObject* object;
	for (object = mSelectedObjects->getFirstRootObject(); object; object = mSelectedObjects->getNextRootObject() )
	{
		object->setRotation(identity);
		if (object->mDrawable.notNull())
		{
			gPipeline.markMoved(object->mDrawable, TRUE);
		}
		object->sendRotationUpdate();
	}
}

void LLSelectMgr::selectionRotateAroundZ(F32 degrees)
{
	LLQuaternion rot( degrees * DEG_TO_RAD, LLVector3(0,0,1) );

	LLViewerObject* object;
	for (object = mSelectedObjects->getFirstRootObject(); object; object = mSelectedObjects->getNextRootObject() )
	{
		object->setRotation( object->getRotationEdit() * rot );
		if (object->mDrawable.notNull())
		{
			gPipeline.markMoved(object->mDrawable, TRUE);
		}
		object->sendRotationUpdate();
	}
}


//-----------------------------------------------------------------------------
// selectionTexScaleAutofit()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionTexScaleAutofit(F32 repeats_per_meter)
{
	LLViewerObject* object;
	S32 te;
	for (mSelectedObjects->getFirstTE(&object, &te); object; mSelectedObjects->getNextTE(&object, &te))
	{
		if (!object->permModify())
		{
			continue;
		}

		if (object->getNumTEs() == 0)
		{
			continue;
		}

		// Compute S,T to axis mapping
		U32 s_axis, t_axis;
		if (!getTESTAxes(object, te, &s_axis, &t_axis))
		{
			continue;
		}

		F32 new_s = object->getScale().mV[s_axis] * repeats_per_meter;
		F32 new_t = object->getScale().mV[t_axis] * repeats_per_meter;

		object->setTEScale(te, new_s, new_t);
	}

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject())
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
	}
}


// BUG: Only works for boxes.
// Face numbering for flex boxes as of 1.14.2002
//-----------------------------------------------------------------------------
// getFaceSTAxes()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::getTESTAxes(const LLViewerObject* object, const U8 face, U32* s_axis, U32* t_axis)
{
	if (face == 0)
	{
		*s_axis = VX; *t_axis = VY;
		return TRUE;
	}
	else if (face == 1)
	{
		*s_axis = VX; *t_axis = VZ;
		return TRUE;
	}
	else if (face == 2)
	{
		*s_axis = VY; *t_axis = VZ;
		return TRUE;
	}
	else if (face == 3)
	{
		*s_axis = VX; *t_axis = VZ;
		return TRUE;
	}
	else if (face == 4)
	{
		*s_axis = VY; *t_axis = VZ;
		return TRUE;
	}
	else if (face == 5)
	{
		*s_axis = VX; *t_axis = VY;
		return TRUE;
	}
	else
	{
		// unknown face
		return FALSE;
	}
}

// Called at the end of a scale operation, this adjusts the textures to attempt to
// maintain a constant repeats per meter.
// BUG: Only works for flex boxes.
//-----------------------------------------------------------------------------
// adjustTexturesByScale()
//-----------------------------------------------------------------------------
void LLSelectMgr::adjustTexturesByScale(BOOL send_to_sim, BOOL stretch)
{
	LLViewerObject* object;
	LLSelectNode* selectNode;

	BOOL send = FALSE;
	
	for (selectNode = mSelectedObjects->getFirstNode(); selectNode; selectNode = mSelectedObjects->getNextNode())
	{
		object = selectNode->getObject();
		if (!object->permModify())
		{
			continue;
		}

		if (object->getNumTEs() == 0)
		{
			continue;
		}

		for (U8 te_num = 0; te_num < object->getNumTEs(); te_num++)
		{
			const LLTextureEntry* tep = object->getTE(te_num);

			BOOL planar = tep->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR;
			if (planar == stretch)
			{
				// Figure out how S,T changed with scale operation
				U32 s_axis, t_axis;
				if (!getTESTAxes(object, te_num, &s_axis, &t_axis)) continue;

				LLVector3 scale_ratio = selectNode->mTextureScaleRatios[te_num]; 
				LLVector3 object_scale = object->getScale();
				
				// Apply new scale to face
				if (planar)
				{
					object->setTEScale(te_num, 1.f/object_scale.mV[s_axis]*scale_ratio.mV[s_axis],
										1.f/object_scale.mV[t_axis]*scale_ratio.mV[t_axis]);
				}
				else
				{
					object->setTEScale(te_num, scale_ratio.mV[s_axis]*object_scale.mV[s_axis],
											scale_ratio.mV[t_axis]*object_scale.mV[t_axis]);
				}
				send = send_to_sim;
			}
		}

		if (send)
		{
			object->sendTEUpdate();
		}
	}
}


//-----------------------------------------------------------------------------
// selectionResetTexInfo()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionResetTexInfo(S32 selected_face)
{
	S32 start_face, end_face;

	LLViewerObject* object;
	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject())
	{
		if (!object->permModify())
		{
			continue;
		}
		if (object->getNumTEs() == 0)
		{
			continue;
		}

		if (selected_face == -1)
		{
			start_face = 0;
			end_face = object->getNumTEs() - 1;
		}
		else
		{
			start_face = selected_face;
			end_face = selected_face;
		}

		for (S32 face = start_face; face <= end_face; face++)
		{
			// Actually, each object should reset to its appropriate value.
			object->setTEScale(face, 1.f, 1.f);
			object->setTEOffset(face, 0.f, 0.f);
			object->setTERotation(face, 0.f);
		}

		object->sendTEUpdate();
	}
}

//-----------------------------------------------------------------------------
// selectGetAllRootsValid()
// Returns true if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetAllRootsValid()
{
	for( LLSelectNode* node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
	
		if( !node->mValid )
		{
			return FALSE;
		}

		if( !node->getObject() )
		{
			return FALSE;
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// selectGetAllValid()
// Returns true if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetAllValid()
{
	for( LLSelectNode* node = mSelectedObjects->getFirstNode(); node; node = mSelectedObjects->getNextNode() )
	{
	
		if( !node->mValid )
		{
			return FALSE;
		}

		if( !node->getObject() )
		{
			return FALSE;
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// selectGetModify() - return true if current agent can modify all
// selected objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetModify()
{
	for( LLSelectNode* node = mSelectedObjects->getFirstNode(); node; node = mSelectedObjects->getNextNode() )
	{
		if( !node->mValid )
		{
			return FALSE;
		}
		LLViewerObject* object = node->getObject();
		if( !object || !object->permModify() )
		{
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetRootsModify() - return true if current agent can modify all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsModify()
{
	for( LLSelectNode* node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if( !node->mValid )
		{
			return FALSE;
		}
		LLViewerObject* object = node->getObject();
		if( !object || !object->permModify() )
		{
			return FALSE;
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// selectGetRootsTransfer() - return true if current agent can transfer all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsTransfer()
{
	for(LLSelectNode* node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode())
	{
		if(!node->mValid)
		{
			return FALSE;
		}
		LLViewerObject* object = node->getObject();
		if(!object || !object->permTransfer())
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetRootsCopy() - return true if current agent can copy all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsCopy()
{
	for(LLSelectNode* node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode())
	{
		if(!node->mValid)
		{
			return FALSE;
		}
		LLViewerObject* object = node->getObject();
		if(!object || !object->permCopy())
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetCreator()
// Creator information only applies to root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetCreator(LLUUID& id, LLString& name)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if(!node) node = mSelectedObjects->getFirstNode();
	if(!node) return FALSE;
	if(!node->mValid) return FALSE;
	LLViewerObject* obj = node->getObject();
	if(!obj) return FALSE;
	if(!(obj->isRoot() || obj->isJointChild())) return FALSE;

	id = node->mPermissions->getCreator();

	BOOL identical = TRUE;
	for ( node = mSelectedObjects->getNextRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (!node->mValid)
		{
			identical = FALSE;
			break;
		}

		if ( !(id == node->mPermissions->getCreator() ) )
		{
			identical = FALSE;
			break;
		}
	}

	if (identical)
	{
		char firstname[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		char lastname[DB_LAST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		gCacheName->getName(id, firstname, lastname);
		name.assign( firstname );
		name.append( " " );
		name.append( lastname );
	}
	else
	{
		name.assign( "(multiple)" );
	}

	return identical;
}


//-----------------------------------------------------------------------------
// selectGetOwner()
// Owner information only applies to roots.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetOwner(LLUUID& id, LLString& name)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if(!node) node = mSelectedObjects->getFirstNode();
	if(!node) return FALSE;
	if(!node->mValid) return FALSE;
	LLViewerObject* obj = node->getObject();
	if(!obj) return FALSE;
	if(!(obj->isRootEdit() || obj->isRoot() || obj->isJointChild())) return FALSE;

	BOOL group_owner = FALSE;
	id.setNull();
	node->mPermissions->getOwnership(id, group_owner);

	BOOL identical = TRUE;
	for ( node = mSelectedObjects->getNextRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (!node->mValid)
		{
			identical = FALSE;
			break;
		}

		LLUUID owner_id;
		BOOL is_group_owned = FALSE;
		if (!(node->mPermissions->getOwnership(owner_id, is_group_owned))
			|| owner_id != id )
		{
			identical = FALSE;
			break;
		}
	}

	BOOL public_owner = (id.isNull() && !group_owner);

	if (identical)
	{
		if (group_owner)
		{
			name.assign( "(Group Owned)");
		}
		else if(!public_owner)
		{
			char firstname[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			char lastname[DB_LAST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			gCacheName->getName(id, firstname, lastname);
			name.assign( firstname );
			name.append( " " );
			name.append( lastname );
		}
		else
		{
			name.assign("Public");
		}
	}
	else
	{
		name.assign( "(multiple)" );
	}

	return identical;
}


//-----------------------------------------------------------------------------
// selectGetLastOwner()
// Owner information only applies to roots.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetLastOwner(LLUUID& id, LLString& name)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if(!node) node = mSelectedObjects->getFirstNode();
	if(!node) return FALSE;
	if(!node->mValid) return FALSE;
	LLViewerObject* obj = node->getObject();
	if(!obj) return FALSE;
	if(!(obj->isRoot() || obj->isJointChild())) return FALSE;

	id = node->mPermissions->getLastOwner();

	BOOL identical = TRUE;
	for ( node = mSelectedObjects->getNextRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (!node->mValid)
		{
			identical = FALSE;
			break;
		}

		if ( !(id == node->mPermissions->getLastOwner() ) )
		{
			identical = FALSE;
			break;
		}
	}

	BOOL public_owner = (id.isNull());

	if (identical)
	{
		if(!public_owner)
		{
			char firstname[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			char lastname[DB_LAST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			gCacheName->getName(id, firstname, lastname);
			name.assign( firstname );
			name.append( " " );
			name.append( lastname );
		}
		else
		{
			name.assign("Public or Group");
		}
	}
	else
	{
		name.assign( "" );
	}

	return identical;
}


//-----------------------------------------------------------------------------
// selectGetGroup()
// Group information only applies to roots.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetGroup(LLUUID& id)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if(!node) node = mSelectedObjects->getFirstNode();
	if(!node) return FALSE;
	if(!node->mValid) return FALSE;
	LLViewerObject* obj = node->getObject();
	if(!obj) return FALSE;
	if(!(obj->isRoot() || obj->isJointChild())) return FALSE;

	id = node->mPermissions->getGroup();

	BOOL identical = TRUE;
	for ( node = mSelectedObjects->getNextRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (!node->mValid)
		{
			identical = FALSE;
			break;
		}

		if ( !(id == node->mPermissions->getGroup() ) )
		{
			identical = FALSE;
			break;
		}
	}

	return identical;
}

//-----------------------------------------------------------------------------
// selectIsGroupOwned()
// Only operates on root nodes.  
// Returns TRUE if all have valid data and they are all group owned.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectIsGroupOwned()
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if(!node) node = mSelectedObjects->getFirstNode();
	if(!node) return FALSE;
	if(!node->mValid) return FALSE;
	LLViewerObject* obj = node->getObject();
	if(!obj) return FALSE;
	if(!(obj->isRoot() || obj->isJointChild())) return FALSE;

	BOOL is_group_owned = node->mPermissions->isGroupOwned();

	if(is_group_owned)
	{
		for ( node = mSelectedObjects->getNextRootNode(); node; node = mSelectedObjects->getNextRootNode() )
		{
			if (!node->mValid)
			{
				is_group_owned = FALSE;
				break;
			}

			if ( !( node->mPermissions->isGroupOwned() ) )
			{
				is_group_owned = FALSE;
				break;
			}
		}
	}
	return is_group_owned;
}

//-----------------------------------------------------------------------------
// selectGetPerm()
// Only operates on root nodes.
// Returns TRUE if all have valid data.
// mask_on has bits set to true where all permissions are true
// mask_off has bits set to true where all permissions are false
// if a bit is off both in mask_on and mask_off, the values differ within
// the selection.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetPerm(U8 which_perm, U32* mask_on, U32* mask_off)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if (!node) return FALSE;
	if (!node->mValid)	return FALSE;

	U32 mask;
	U32 mask_and	= 0xffffffff;
	U32 mask_or		= 0x00000000;
	BOOL all_valid	= TRUE;

	for ( node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (!node->mValid)
		{
			all_valid = FALSE;
			break;
		}

		switch( which_perm )
		{
		case PERM_BASE:
			mask = node->mPermissions->getMaskBase();
			break;
		case PERM_OWNER:
			mask = node->mPermissions->getMaskOwner();
			break;
		case PERM_GROUP:
			mask = node->mPermissions->getMaskGroup();
			break;
		case PERM_EVERYONE:
			mask = node->mPermissions->getMaskEveryone();
			break;
		case PERM_NEXT_OWNER:
			mask = node->mPermissions->getMaskNextOwner();
			break;
		default:
			mask = 0x0;
			break;
		}
		mask_and &= mask;
		mask_or	 |= mask;
	}

	if (all_valid)
	{
		// ...true through all ANDs means all true
		*mask_on  = mask_and;

		// ...false through all ORs means all false
		*mask_off = ~mask_or;
		return TRUE;
	}
	else
	{
		*mask_on  = 0;
		*mask_off = 0;
		return FALSE;
	}
}



BOOL LLSelectMgr::selectGetOwnershipCost(S32* out_cost)
{
	return mSelectedObjects->getOwnershipCost(*out_cost);
}

BOOL LLSelectMgr::selectGetPermissions(LLPermissions& perm)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if (!node) return FALSE;
	if (!node->mValid)	return FALSE;
	BOOL valid = TRUE;
	perm = *(node->mPermissions);
	for(node = mSelectedObjects->getNextRootNode(); node != NULL; node = mSelectedObjects->getNextRootNode())
	{
		if(!node->mValid)
		{
			valid = FALSE;
			break;
		}
		perm.accumulate(*(node->mPermissions));
	}
	return valid;
}


void LLSelectMgr::selectDelete()
{
	S32 deleteable_count = 0;

	BOOL locked_but_deleteable_object = FALSE;
	BOOL no_copy_but_deleteable_object = FALSE;
	BOOL all_owned_by_you = TRUE;
	for(LLViewerObject* obj = mSelectedObjects->getFirstObject();
		obj != NULL;
		obj = mSelectedObjects->getNextObject())
	{
		if( obj->isAttachment() )
		{
			continue;
		}

		deleteable_count++;

		// Check to see if you can delete objects which are locked.
		if(!obj->permMove())
		{
			locked_but_deleteable_object = TRUE;
		}
		if(!obj->permCopy())
		{
			no_copy_but_deleteable_object = TRUE;
		}
		if(!obj->permYouOwner())
		{
			all_owned_by_you = FALSE;
		}
	}

	if( 0 == deleteable_count )
	{
		make_ui_sound("UISndInvalidOp");
		return;
	}

	if(locked_but_deleteable_object ||
	   no_copy_but_deleteable_object ||
	   !all_owned_by_you)
	{
		// convert any transient pie-menu selections to full selection so this operation
		// has some context
		// NOTE: if user cancels delete operation, this will potentially leave objects selected outside of build mode
		// but this is ok, if not ideal
		convertTransient();

		//This is messy, but needed to get all english our of the UI.
		if(locked_but_deleteable_object && !no_copy_but_deleteable_object && all_owned_by_you)
		{
			//Locked only
			gViewerWindow->alertXml(  "ConfirmObjectDeleteLock",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else if(!locked_but_deleteable_object && no_copy_but_deleteable_object && all_owned_by_you)
		{
			//No Copy only
			gViewerWindow->alertXml(  "ConfirmObjectDeleteNoCopy",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else if(!locked_but_deleteable_object && !no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//not owned only
			gViewerWindow->alertXml(  "ConfirmObjectDeleteNoOwn",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else if(locked_but_deleteable_object && no_copy_but_deleteable_object && all_owned_by_you)
		{
			//locked and no copy
			gViewerWindow->alertXml(  "ConfirmObjectDeleteLockNoCopy",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else if(locked_but_deleteable_object && !no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//locked and not owned
			gViewerWindow->alertXml(  "ConfirmObjectDeleteLockNoOwn",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else if(!locked_but_deleteable_object && no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//no copy and not owned
			gViewerWindow->alertXml(  "ConfirmObjectDeleteNoCopyNoOwn",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		else
		{
			//locked, no copy and not owned
			gViewerWindow->alertXml(  "ConfirmObjectDeleteLockNoCopyNoOwn",
								  &LLSelectMgr::confirmDelete,
								  this);
		}
		
		
		
	}
	else
	{
		confirmDelete(0, (void*)this);
	}
}

// static
void LLSelectMgr::confirmDelete(S32 option, void* data)
{
	LLSelectMgr* self = (LLSelectMgr*)data;
	if(!self) return;
	switch(option)
	{
	case 0:
		{
			// TODO: Make sure you have delete permissions on all of them.
			LLUUID trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
			// attempt to derez into the trash.
			LLDeRezInfo* info = new LLDeRezInfo(DRD_TRASH, trash_id);
			self->sendListToRegions("DeRezObject",
									packDeRezHeader,
									packObjectLocalID,
									(void*)info,
									SEND_ONLY_ROOTS);
			// VEFFECT: Delete Object - one effect for all deletes
			if (self->mSelectedObjects->mSelectType != SELECT_TYPE_HUD)
			{
				LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
				effectp->setPositionGlobal( self->getSelectionCenterGlobal() );
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
				F32 duration = 0.5f;
				duration += self->mSelectedObjects->getObjectCount() / 64.f;
				effectp->setDuration(duration);
			}

			gAgent.setLookAt(LOOKAT_TARGET_CLEAR);

			// Keep track of how many objects have been deleted.
			F64 obj_delete_count = gViewerStats->getStat(LLViewerStats::ST_OBJECT_DELETE_COUNT);
			obj_delete_count += self->mSelectedObjects->getObjectCount();
			gViewerStats->setStat(LLViewerStats::ST_OBJECT_DELETE_COUNT, obj_delete_count );
		}
		break;
	case 1:
	default:
		break;
	}
}


void LLSelectMgr::selectForceDelete()
{
	sendListToRegions(
		"ObjectDelete",
		packDeleteHeader,
		packObjectLocalID,
		(void*)TRUE,
		SEND_ONLY_ROOTS);
}


// returns TRUE if anything is for sale. calculates the total price
// and stores that value in price.
BOOL LLSelectMgr::selectIsForSale(S32& price)
{
	BOOL any_for_sale = FALSE;
	price = 0;

	LLSelectNode *node;
	for (node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode() )
	{
		if (node->mSaleInfo.isForSale())
		{
			price += node->mSaleInfo.getSalePrice();
			any_for_sale = TRUE;
		}
	}

	return any_for_sale;

}

// returns TRUE if all nodes are valid. method also stores an
// accumulated sale info.
BOOL LLSelectMgr::selectGetSaleInfo(LLSaleInfo& sale_info)
{
	LLSelectNode* node = mSelectedObjects->getFirstRootNode();
	if (!node) return FALSE;
	if (!node->mValid)	return FALSE;
	BOOL valid = TRUE;
	sale_info = node->mSaleInfo;
	for(node = mSelectedObjects->getNextRootNode(); node != NULL; node = mSelectedObjects->getNextRootNode())
	{
		if(!node->mValid)
		{
			valid = FALSE;
			break;
		}
		sale_info.accumulate(node->mSaleInfo);
	}
	return valid;
}

BOOL LLSelectMgr::selectGetAggregatePermissions(LLAggregatePermissions& ag_perm)
{
	LLSelectNode* node = mSelectedObjects->getFirstNode();
	if (!node) return FALSE;
	if (!node->mValid)	return FALSE;
	BOOL valid = TRUE;
	ag_perm = node->mAggregatePerm;
	for(node = mSelectedObjects->getNextNode(); node != NULL; node = mSelectedObjects->getNextNode())
	{
		if(!node->mValid)
		{
			valid = FALSE;
			break;
		}
		ag_perm.aggregate(node->mAggregatePerm);
	}
	return valid;
}

BOOL LLSelectMgr::selectGetAggregateTexturePermissions(LLAggregatePermissions& ag_perm)
{
	LLSelectNode* node = mSelectedObjects->getFirstNode();
	if (!node) return FALSE;
	if (!node->mValid)	return FALSE;
	BOOL valid = TRUE;
	ag_perm = node->getObject()->permYouOwner() ? node->mAggregateTexturePermOwner : node->mAggregateTexturePerm;
	for(node = mSelectedObjects->getNextNode(); node != NULL; node = mSelectedObjects->getNextNode())
	{
		if(!node->mValid)
		{
			valid = FALSE;
			break;
		}
		ag_perm.aggregate(node->getObject()->permYouOwner() ? node->mAggregateTexturePermOwner : node->mAggregateTexturePerm);
	}
	return valid;
}


//--------------------------------------------------------------------
// Duplicate objects
//--------------------------------------------------------------------

// JC - If this doesn't work right, duplicate the selection list
// before doing anything, do a deselect, then send the duplicate
// messages.
struct LLDuplicateData
{
	LLVector3	offset;
	U32			flags;
};

void LLSelectMgr::selectDuplicate(const LLVector3& offset, BOOL select_copy)
{
	if (mSelectedObjects->isAttachment())
	{
		//RN: do not duplicate attachments
		make_ui_sound("UISndInvalidOp");
		return;
	}
	LLDuplicateData	data;

	data.offset = offset;
	data.flags = (select_copy ? FLAGS_CREATE_SELECTED : 0x0);

	sendListToRegions("ObjectDuplicate", packDuplicateHeader, packDuplicate, &data, SEND_ONLY_ROOTS);

	if (select_copy)
	{
		// the new copy will be coming in selected
		deselectAll();
	}
	else
	{
		for (LLSelectNode* node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode())
		{
			node->mDuplicated = TRUE;
			node->mDuplicatePos = node->getObject()->getPositionGlobal();
			node->mDuplicateRot = node->getObject()->getRotation();
		}
	}
}

void LLSelectMgr::repeatDuplicate()
{
	if (mSelectedObjects->isAttachment())
	{
		//RN: do not duplicate attachments
		make_ui_sound("UISndInvalidOp");
		return;
	}

	LLSelectNode* node;
	LLDynamicArray<LLViewerObject*> non_duplicated_objects;

	for (node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode())
	{
		if (!node->mDuplicated)
		{
			non_duplicated_objects.put(node->getObject());
		}
	}

	// make sure only previously duplicated objects are selected
	for (S32 i = 0; i < non_duplicated_objects.count(); i++)
	{
		deselectObjectAndFamily(non_duplicated_objects[i]);
	}
	
	// duplicate objects in place
	LLDuplicateData	data;

	data.offset = LLVector3::zero;
	data.flags = 0x0;

	sendListToRegions("ObjectDuplicate", packDuplicateHeader, packDuplicate, &data, SEND_ONLY_ROOTS);

	// move current selection based on delta from duplication position and update duplication position
	for (node = mSelectedObjects->getFirstRootNode(); node; node = mSelectedObjects->getNextRootNode())
	{
		if (node->mDuplicated)
		{
			LLQuaternion cur_rot = node->getObject()->getRotation();
			LLQuaternion rot_delta = (~node->mDuplicateRot * cur_rot);
			LLQuaternion new_rot = cur_rot * rot_delta;
			LLVector3d cur_pos = node->getObject()->getPositionGlobal();
			LLVector3d new_pos = cur_pos + ((cur_pos - node->mDuplicatePos) * rot_delta);

			node->mDuplicatePos = node->getObject()->getPositionGlobal();
			node->mDuplicateRot = node->getObject()->getRotation();
			node->getObject()->setPositionGlobal(new_pos);
			node->getObject()->setRotation(new_rot);
		}
	}

	sendMultipleUpdate(UPD_ROTATION | UPD_POSITION);
}

// static 
void LLSelectMgr::packDuplicate( LLSelectNode* node, void *duplicate_data )
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID());
}


//--------------------------------------------------------------------
// Duplicate On Ray
//--------------------------------------------------------------------

// Duplicates the selected objects, but places the copy along a cast
// ray.
struct LLDuplicateOnRayData
{
	LLVector3	mRayStartRegion;
	LLVector3	mRayEndRegion;
	BOOL		mBypassRaycast;
	BOOL		mRayEndIsIntersection;
	LLUUID		mRayTargetID;
	BOOL		mCopyCenters;
	BOOL		mCopyRotates;
	U32			mFlags;
};

void LLSelectMgr::selectDuplicateOnRay(const LLVector3 &ray_start_region,
									   const LLVector3 &ray_end_region,
									   BOOL bypass_raycast,
									   BOOL ray_end_is_intersection,
									   const LLUUID &ray_target_id,
									   BOOL copy_centers,
									   BOOL copy_rotates,
									   BOOL select_copy)
{
	if (mSelectedObjects->isAttachment())
	{
		// do not duplicate attachments
		make_ui_sound("UISndInvalidOp");
		return;
	}
	
	LLDuplicateOnRayData	data;

	data.mRayStartRegion	= ray_start_region;
	data.mRayEndRegion		= ray_end_region;
	data.mBypassRaycast		= bypass_raycast;
	data.mRayEndIsIntersection = ray_end_is_intersection;
	data.mRayTargetID		= ray_target_id;
	data.mCopyCenters		= copy_centers;
	data.mCopyRotates		= copy_rotates;
	data.mFlags				= (select_copy ? FLAGS_CREATE_SELECTED : 0x0);

	sendListToRegions("ObjectDuplicateOnRay", 
		packDuplicateOnRayHead, packObjectLocalID, &data, SEND_ONLY_ROOTS);

	if (select_copy)
	{
		// the new copy will be coming in selected
		deselectAll();
	}
}

// static
void LLSelectMgr::packDuplicateOnRayHead(void *user_data)
{
	LLMessageSystem *msg = gMessageSystem;
	LLDuplicateOnRayData *data = (LLDuplicateOnRayData *)user_data;

	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID() );
	msg->addVector3Fast(_PREHASH_RayStart, data->mRayStartRegion );
	msg->addVector3Fast(_PREHASH_RayEnd, data->mRayEndRegion );
	msg->addBOOLFast(_PREHASH_BypassRaycast, data->mBypassRaycast );
	msg->addBOOLFast(_PREHASH_RayEndIsIntersection, data->mRayEndIsIntersection );
	msg->addBOOLFast(_PREHASH_CopyCenters, data->mCopyCenters );
	msg->addBOOLFast(_PREHASH_CopyRotates, data->mCopyRotates );
	msg->addUUIDFast(_PREHASH_RayTargetID, data->mRayTargetID );
	msg->addU32Fast(_PREHASH_DuplicateFlags, data->mFlags );
}



//------------------------------------------------------------------------
// Object position, scale, rotation update, all-in-one
//------------------------------------------------------------------------

void LLSelectMgr::sendMultipleUpdate(U32 type)
{
	if (type == UPD_NONE) return;
	// send individual updates when selecting textures or individual objects
	ESendType send_type = (gSavedSettings.getBOOL("SelectLinkedSet") && !getTEMode()) ? SEND_ONLY_ROOTS : SEND_ROOTS_FIRST;
	if (send_type == SEND_ONLY_ROOTS)
	{
		// tell simulator to apply to whole linked sets
		type |= UPD_LINKED_SETS;
	}

	sendListToRegions(
		"MultipleObjectUpdate",
		packAgentAndSessionID,
		packMultipleUpdate,
		&type,
		send_type);
}

// static
void LLSelectMgr::packMultipleUpdate(LLSelectNode* node, void *user_data)
{
	LLViewerObject* object = node->getObject();
	U32	*type32 = (U32 *)user_data;
	U8 type = (U8)*type32;
	U8	data[256];

	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID,	object->getLocalID() );
	gMessageSystem->addU8Fast(_PREHASH_Type, type );

	S32 offset = 0;

	// JC: You MUST pack the data in this order.  The receiving
	// routine process_multiple_update_message on simulator will
	// extract them in this order.

	if (type & UPD_POSITION)
	{
		htonmemcpy(&data[offset], &(object->getPosition().mV), MVT_LLVector3, 12); 
		offset += 12;
	}
	if (type & UPD_ROTATION)
	{
		LLQuaternion quat = object->getRotation();
		LLVector3 vec = quat.packToVector3();
		htonmemcpy(&data[offset], &(vec.mV), MVT_LLQuaternion, 12); 
		offset += 12;
	}
	if (type & UPD_SCALE)
	{
		//llinfos << "Sending object scale " << object->getScale() << llendl;
		htonmemcpy(&data[offset], &(object->getScale().mV), MVT_LLVector3, 12); 
		offset += 12;
	}
	gMessageSystem->addBinaryDataFast(_PREHASH_Data, data, offset);
}

//------------------------------------------------------------------------
// Ownership
//------------------------------------------------------------------------
struct LLOwnerData
{
	LLUUID	owner_id;
	LLUUID	group_id;
	BOOL	override;
};

void LLSelectMgr::sendOwner(const LLUUID& owner_id,
							const LLUUID& group_id,
							BOOL override)
{
	LLOwnerData data;

	data.owner_id = owner_id;
	data.group_id = group_id;
	data.override = override;

	sendListToRegions("ObjectOwner", packOwnerHead, packObjectLocalID, &data, SEND_ONLY_ROOTS);
}

// static
void LLSelectMgr::packOwnerHead(void *user_data)
{
	LLOwnerData *data = (LLOwnerData *)user_data;

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	gMessageSystem->nextBlockFast(_PREHASH_HeaderData);
	gMessageSystem->addBOOLFast(_PREHASH_Override, data->override);
	gMessageSystem->addUUIDFast(_PREHASH_OwnerID, data->owner_id);
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, data->group_id);
}

//------------------------------------------------------------------------
// Group
//------------------------------------------------------------------------

void LLSelectMgr::sendGroup(const LLUUID& group_id)
{
	LLUUID local_group_id(group_id);
	sendListToRegions("ObjectGroup", packAgentAndSessionAndGroupID, packObjectLocalID, &local_group_id, SEND_ONLY_ROOTS);
}


//------------------------------------------------------------------------
// Buy
//------------------------------------------------------------------------

struct LLBuyData
{
	LLDynamicArray<LLViewerObject*> mObjectsSent;
	LLUUID mCategoryID;
	LLSaleInfo mSaleInfo;
};

// *NOTE: does not work for multiple object buy, which UI does not
// currently support sale info is used for verification only, if it
// doesn't match region info then sale is canceled Need to get sale
// info -as displayed in the UI- for every item.
void LLSelectMgr::sendBuy(const LLUUID& buyer_id, const LLUUID& category_id, const LLSaleInfo sale_info)
{
	LLBuyData buy;
	buy.mCategoryID = category_id;
	buy.mSaleInfo = sale_info;
	sendListToRegions("ObjectBuy", packAgentGroupAndCatID, packBuyObjectIDs, &buy, SEND_ONLY_ROOTS);
}

// static
void LLSelectMgr::packBuyObjectIDs(LLSelectNode* node, void* data)
{
	LLBuyData* buy = (LLBuyData*)data;

	LLViewerObject* object = node->getObject();
	if(buy->mObjectsSent.find(object) == LLDynamicArray<LLViewerObject*>::FAIL)
	{
		buy->mObjectsSent.put(object);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID() );
		gMessageSystem->addU8Fast(_PREHASH_SaleType, buy->mSaleInfo.getSaleType());
		gMessageSystem->addS32Fast(_PREHASH_SalePrice, buy->mSaleInfo.getSalePrice());
	}
}

//------------------------------------------------------------------------
// Permissions
//------------------------------------------------------------------------

struct LLPermData
{
	U8 mField;
	BOOL mSet;
	U32 mMask;
	BOOL mOverride;
};

// TODO: Make this able to fail elegantly.
void LLSelectMgr::selectionSetObjectPermissions(U8 field,
									   BOOL set,
									   U32 mask,
									   BOOL override)
{
	LLPermData data;

	data.mField = field;
	data.mSet = set;
	data.mMask = mask;
	data.mOverride = override;

	sendListToRegions("ObjectPermissions", packPermissionsHead, packPermissions, &data, SEND_ONLY_ROOTS);
}

void LLSelectMgr::packPermissionsHead(void* user_data)
{
	LLPermData* data = (LLPermData*)user_data;
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_HeaderData);
	gMessageSystem->addBOOLFast(_PREHASH_Override, data->mOverride);
}	


// Now that you've added a bunch of objects, send a select message
// on the entire list for efficiency.
/*
void LLSelectMgr::sendSelect()
{
	llerrs << "Not implemented" << llendl;
}
*/

void LLSelectMgr::deselectAll()
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	sendListToRegions(
		"ObjectDeselect",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_INDIVIDUALS);

	removeAll();
	
	mLastSentSelectionCenterGlobal.clearVec();

	updatePointAt();
	gHUDManager->clearJoints();
	updateSelectionCenter();
}

void LLSelectMgr::deselectUnused()
{
	// no more outstanding references to this selection
	if (mSelectedObjects->getNumRefs() == 1)
	{
		deselectAll();
	}
}

void LLSelectMgr::convertTransient()
{
	// use STL-style iteration to avoid recursive iteration problems
	LLObjectSelection::iterator node_it;
	for (node_it = mSelectedObjects->begin(); node_it != mSelectedObjects->end(); ++node_it)
	{
		LLSelectNode *nodep = *node_it;
		nodep->setTransient(FALSE);
	}
}

void LLSelectMgr::deselectAllIfTooFar()
{
	if (mSelectedObjects->isEmpty() || mSelectedObjects->mSelectType == SELECT_TYPE_HUD)
	{
		return;
	}

	// HACK: Don't deselect when we're navigating to rate an object's
	// owner or creator.  JC
	if (gPieObject->getVisible() || gPieRate->getVisible() )
	{
		return;
	}

	LLVector3d selectionCenter = getSelectionCenterGlobal();
	if (gSavedSettings.getBOOL("LimitSelectDistance") 
		&& !selectionCenter.isExactlyZero())
	{
		F32 deselect_dist = gSavedSettings.getF32("MaxSelectDistance");
		F32 deselect_dist_sq = deselect_dist * deselect_dist;

		LLVector3d select_delta = gAgent.getPositionGlobal() - selectionCenter;
		F32 select_dist_sq = (F32) select_delta.magVecSquared();

		if (select_dist_sq > deselect_dist_sq)
		{
			if (gDebugSelectMgr)
			{
				llinfos << "Selection manager: auto-deselecting, select_dist = " << fsqrtf(select_dist_sq) << llendl;
				llinfos << "agent pos global = " << gAgent.getPositionGlobal() << llendl;
				llinfos << "selection pos global = " << selectionCenter << llendl;
			}

			deselectAll();
		}
	}
}


void LLSelectMgr::selectionSetObjectName(const LLString& name)
{
	// we only work correctly if 1 object is selected.
	if(mSelectedObjects->getRootObjectCount() == 1)
	{
		sendListToRegions("ObjectName",
						  packAgentAndSessionID,
						  packObjectName,
						  (void*)name.c_str(),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectName",
						  packAgentAndSessionID,
						  packObjectName,
						  (void*)name.c_str(),
						  SEND_INDIVIDUALS);
	}
}

void LLSelectMgr::selectionSetObjectDescription(const LLString& desc)
{
	// we only work correctly if 1 object is selected.
	if(mSelectedObjects->getRootObjectCount() == 1)
	{
		sendListToRegions("ObjectDescription",
						  packAgentAndSessionID,
						  packObjectDescription,
						  (void*)desc.c_str(),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectDescription",
						  packAgentAndSessionID,
						  packObjectDescription,
						  (void*)desc.c_str(),
						  SEND_INDIVIDUALS);
	}
}

void LLSelectMgr::selectionSetObjectCategory(const LLCategory& category)
{
	// for now, we only want to be able to set one root category at
	// a time.
	if(mSelectedObjects->getRootObjectCount() != 1) return;
	sendListToRegions("ObjectCategory",
					  packAgentAndSessionID,
					  packObjectCategory,
					  (void*)(&category),
					  SEND_ONLY_ROOTS);
}

void LLSelectMgr::selectionSetObjectSaleInfo(const LLSaleInfo& sale_info)
{
	// Only one sale info at a time for now
	if(mSelectedObjects->getRootObjectCount() != 1) return;
	sendListToRegions("ObjectSaleInfo",
					  packAgentAndSessionID,
					  packObjectSaleInfo,
					  (void*)(&sale_info),
					  SEND_ONLY_ROOTS);
}

//----------------------------------------------------------------------
// Attachments
//----------------------------------------------------------------------

void LLSelectMgr::sendAttach(U8 attachment_point)
{
	LLViewerObject* attach_object = mSelectedObjects->getFirstRootObject();

	if (!attach_object || !gAgent.getAvatarObject() || mSelectedObjects->mSelectType != SELECT_TYPE_WORLD)
	{
		return;
	}

	BOOL build_mode = gToolMgr->inEdit();
	// Special case: Attach to default location for this object.
	if (0 == attachment_point)
	{
		sendListToRegions(
			"ObjectAttach",
			packAgentIDAndSessionAndAttachment, 
			packObjectIDAndRotation, 
			&attachment_point, 
			SEND_ONLY_ROOTS );
		if (!build_mode)
		{
			deselectAll();
		}
	}
	else
	{
		LLViewerJointAttachment* attachment = gAgent.getAvatarObject()->mAttachmentPoints.getIfThere(attachment_point);
		if (attachment)
		{
			LLQuaternion object_world_rot = attach_object->getRenderRotation();
			LLQuaternion attachment_pt__world_rot = attachment->getWorldRotation();
			LLQuaternion local_rot = object_world_rot * ~attachment_pt__world_rot;

			F32 x,y,z;
			local_rot.getEulerAngles(&x, &y, &z);
			// snap to nearest 90 degree rotation
			// make sure all euler angles are positive
			if (x < F_PI_BY_TWO) x += F_TWO_PI;
			if (y < F_PI_BY_TWO) y += F_TWO_PI;
			if (z < F_PI_BY_TWO) z += F_TWO_PI;

			// add 45 degrees so that rounding down becomes rounding off
			x += F_PI_BY_TWO / 2.f;
			y += F_PI_BY_TWO / 2.f;
			z += F_PI_BY_TWO / 2.f;
			// round down to nearest multiple of 90 degrees
			x -= fmodf(x, F_PI_BY_TWO);
			y -= fmodf(y, F_PI_BY_TWO);
			z -= fmodf(z, F_PI_BY_TWO);

			// pass the requested rotation on to the simulator
			local_rot.setQuat(x, y, z);
			attach_object->setRotation(local_rot);

			sendListToRegions(
				"ObjectAttach",
				packAgentIDAndSessionAndAttachment,
				packObjectIDAndRotation,
				&attachment_point,
				SEND_ONLY_ROOTS );
			if (!build_mode)
			{
				deselectAll();
			}
		}
	}
}

void LLSelectMgr::sendDetach()
{
	if (!mSelectedObjects->getNumNodes() || mSelectedObjects->mSelectType == SELECT_TYPE_WORLD)
	{
		return;
	}

	sendListToRegions(
		"ObjectDetach",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_ONLY_ROOTS );
}


void LLSelectMgr::sendDropAttachment()
{
	if (!mSelectedObjects->getNumNodes() || mSelectedObjects->mSelectType == SELECT_TYPE_WORLD)
	{
		return;
	}

	sendListToRegions(
		"ObjectDrop",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_ONLY_ROOTS);
}

//----------------------------------------------------------------------
// Links
//----------------------------------------------------------------------

void LLSelectMgr::sendLink()
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	sendListToRegions(
		"ObjectLink",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_ONLY_ROOTS);
}

void LLSelectMgr::sendDelink()
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	// Delink needs to send individuals so you can unlink a single object from
	// a linked set.
	sendListToRegions(
		"ObjectDelink",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_INDIVIDUALS);
}


//----------------------------------------------------------------------
// Hinges
//----------------------------------------------------------------------

void LLSelectMgr::sendHinge(U8 type)
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	sendListToRegions(
		"ObjectHinge",
		packHingeHead,
		packObjectLocalID,
		&type,
		SEND_ONLY_ROOTS);
}


void LLSelectMgr::sendDehinge()
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	sendListToRegions(
		"ObjectDehinge",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_ONLY_ROOTS);
}

void LLSelectMgr::sendSelect()
{
	if (!mSelectedObjects->getNumNodes())
	{
		return;
	}

	sendListToRegions(
		"ObjectSelect",
		packAgentAndSessionID,
		packObjectLocalID,
		NULL,
		SEND_INDIVIDUALS);
}

// static
void LLSelectMgr::packHingeHead(void *user_data)
{
	U8	*type = (U8 *)user_data;

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	gMessageSystem->nextBlockFast(_PREHASH_JointType);
	gMessageSystem->addU8Fast(_PREHASH_Type, *type );
}


void LLSelectMgr::selectionDump()
{
	LLViewerObject *object;

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		object->dump();
	}
}

void LLSelectMgr::saveSelectedObjectColors()
{
	LLSelectNode* selectNode;
	for (selectNode = mSelectedObjects->getFirstNode(); selectNode; selectNode = mSelectedObjects->getNextNode() )
	{
		selectNode->saveColors();
	}
}

void LLSelectMgr::saveSelectedObjectTextures()
{
	LLSelectNode*		selectNode;

	// invalidate current selection so we update saved textures
	for (selectNode = mSelectedObjects->getFirstNode(); selectNode; selectNode = mSelectedObjects->getNextNode() )
	{
		selectNode->mValid = FALSE;
	}

	// request object properties message to get updated permissions data
	sendSelect();
}


// This routine should be called whenever a drag is initiated.
// also need to know to which simulator to send update message
void LLSelectMgr::saveSelectedObjectTransform(EActionType action_type)
{
	LLSelectNode*		selectNode;

	if (mSelectedObjects->isEmpty())
	{
		// nothing selected, so nothing to save
		return;
	}

	for (selectNode = mSelectedObjects->getFirstNode(); selectNode; selectNode = mSelectedObjects->getNextNode() )
	{
		LLViewerObject*		object;
		object = selectNode->getObject();
		selectNode->mSavedPositionLocal = object->getPosition();
		if (object->isAttachment())
		{
			if (object->isRootEdit())
			{
				LLXform* parent_xform = object->mDrawable->getXform()->getParent();
				selectNode->mSavedPositionGlobal = gAgent.getPosGlobalFromAgent((object->getPosition() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition());
			}
			else
			{
				LLViewerObject* attachment_root = (LLViewerObject*)object->getParent();
				LLXform* parent_xform = attachment_root->mDrawable->getXform()->getParent();
				LLVector3 root_pos = (attachment_root->getPosition() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();
				LLQuaternion root_rot = (attachment_root->getRotation() * parent_xform->getWorldRotation());
				selectNode->mSavedPositionGlobal = gAgent.getPosGlobalFromAgent((object->getPosition() * root_rot) + root_pos);
			}
			selectNode->mSavedRotation = object->getRenderRotation();
		}
		else
		{
			selectNode->mSavedPositionGlobal = object->getPositionGlobal();
			selectNode->mSavedRotation = object->getRotationRegion();
		}
		
		selectNode->mSavedScale = object->getScale();
		selectNode->saveTextureScaleRatios();

	}
	mSavedSelectionBBox = getBBoxOfSelection();
}

void LLSelectMgr::selectionUpdatePhysics(BOOL physics)
{
	LLViewerObject *object;

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (   !object->permModify()  		// preemptive permissions check
			|| !(object->isRoot()			// don't send for child objects
			|| object->isJointChild()))
		{
			continue;
		}
		object->setFlags( FLAGS_USE_PHYSICS, physics);
	}
}

void LLSelectMgr::selectionUpdateTemporary(BOOL is_temporary)
{
	LLViewerObject *object;

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (   !object->permModify()  		// preemptive permissions check
			|| !(object->isRoot()			// don't send for child objects
			|| object->isJointChild()))
		{
			continue;
		}
		object->setFlags( FLAGS_TEMPORARY_ON_REZ, is_temporary);
	}
}

void LLSelectMgr::selectionUpdatePhantom(BOOL is_phantom)
{
	LLViewerObject *object;

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (   !object->permModify()  		// preemptive permissions check
			|| !(object->isRoot()			// don't send for child objects
			|| object->isJointChild()))
		{
			continue;
		}
		object->setFlags( FLAGS_PHANTOM, is_phantom);
	}
}

void LLSelectMgr::selectionUpdateCastShadows(BOOL cast_shadows)
{
	LLViewerObject *object;

	for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
	{
		if (   !object->permModify()  		// preemptive permissions check
			|| object->isJointChild())
		{
			continue;
		}
		object->setFlags( FLAGS_CAST_SHADOWS, cast_shadows);
	}
}


//----------------------------------------------------------------------
// Helpful packing functions for sendObjectMessage()
//----------------------------------------------------------------------

// static 
void LLSelectMgr::packAgentIDAndSessionAndAttachment( void *user_data)
{
	U8 *attachment_point = (U8*)user_data;
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addU8Fast(_PREHASH_AttachmentPoint, *attachment_point);
}

// static
void LLSelectMgr::packAgentID(	void *user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
}

// static
void LLSelectMgr::packAgentAndSessionID(void* user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
}

// static
void LLSelectMgr::packAgentAndGroupID(void* user_data)
{
	LLOwnerData *data = (LLOwnerData *)user_data;

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, data->owner_id );
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, data->group_id );
}

// static
void LLSelectMgr::packAgentAndSessionAndGroupID(void* user_data)
{
	LLUUID* group_idp = (LLUUID*) user_data;
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, *group_idp);
}

// static
void LLSelectMgr::packDuplicateHeader(void* data)
{
	LLUUID group_id(gAgent.getGroupID());
	packAgentAndSessionAndGroupID(&group_id);

	LLDuplicateData* dup_data = (LLDuplicateData*) data;

	gMessageSystem->nextBlockFast(_PREHASH_SharedData);
	gMessageSystem->addVector3Fast(_PREHASH_Offset, dup_data->offset);
	gMessageSystem->addU32Fast(_PREHASH_DuplicateFlags, dup_data->flags);
}

// static
void LLSelectMgr::packDeleteHeader(void* userdata)
{
	BOOL force = (BOOL)(intptr_t)userdata;

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addBOOLFast(_PREHASH_Force, force);
}

// static
void LLSelectMgr::packAgentGroupAndCatID(void* user_data)
{
	LLBuyData* buy = (LLBuyData*)user_data;
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
	gMessageSystem->addUUIDFast(_PREHASH_CategoryID, buy->mCategoryID);
}

//static
void LLSelectMgr::packDeRezHeader(void* user_data)
{
	LLDeRezInfo* info = (LLDeRezInfo*)user_data;
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_AgentBlock);
	gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
	gMessageSystem->addU8Fast(_PREHASH_Destination, (U8)info->mDestination);
	gMessageSystem->addUUIDFast(_PREHASH_DestinationID, info->mDestinationID);
	LLUUID tid;
	tid.generate();
	gMessageSystem->addUUIDFast(_PREHASH_TransactionID, tid);
	const U8 PACKET = 1;
	gMessageSystem->addU8Fast(_PREHASH_PacketCount, PACKET);
	gMessageSystem->addU8Fast(_PREHASH_PacketNumber, PACKET);
}

// static 
void LLSelectMgr::packObjectID(LLSelectNode* node, void *user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addUUIDFast(_PREHASH_ObjectID, node->getObject()->mID );
}

void LLSelectMgr::packObjectIDAndRotation(LLSelectNode* node, void *user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID() );
	gMessageSystem->addQuatFast(_PREHASH_Rotation, node->getObject()->getRotation());
}

void LLSelectMgr::packObjectClickAction(LLSelectNode* node, void *user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID() );
	gMessageSystem->addU8("ClickAction", node->getObject()->getClickAction());
}

// static
void LLSelectMgr::packObjectLocalID(LLSelectNode* node, void *)
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID());
}

// static
void LLSelectMgr::packObjectName(LLSelectNode* node, void* user_data)
{
	char* name = (char*)user_data;
	if(!name) return;
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
	gMessageSystem->addStringFast(_PREHASH_Name, name);
}

// static
void LLSelectMgr::packObjectDescription(LLSelectNode* node,
										void* user_data)
{
	char* desc = (char*)user_data;
	if(!desc) return;
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
	gMessageSystem->addStringFast(_PREHASH_Description, desc);
}

// static
void LLSelectMgr::packObjectCategory(LLSelectNode* node, void* user_data)
{
	LLCategory* category = (LLCategory*)user_data;
	if(!category) return;
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
	category->packMessage(gMessageSystem);
}

// static
void LLSelectMgr::packObjectSaleInfo(LLSelectNode* node, void* user_data)
{
	LLSaleInfo* sale_info = (LLSaleInfo*)user_data;
	if(!sale_info) return;
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
	sale_info->packMessage(gMessageSystem);
}

// static
void LLSelectMgr::packPhysics(LLSelectNode* node, void *user_data)
{
}

// static
void LLSelectMgr::packShape(LLSelectNode* node, void *user_data)
{
}

// static 
void LLSelectMgr::packPermissions(LLSelectNode* node, void *user_data)
{
	LLPermData *data = (LLPermData *)user_data;

	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID());

	gMessageSystem->addU8Fast(_PREHASH_Field,	data->mField);
	gMessageSystem->addBOOLFast(_PREHASH_Set,		data->mSet);
	gMessageSystem->addU32Fast(_PREHASH_Mask,		data->mMask);
}

// Utility function to send some information to every region containing
// an object on the selection list.  We want to do this to reduce the total
// number of packets sent by the viewer.
void LLSelectMgr::sendListToRegions(const LLString& message_name,
									void (*pack_header)(void *user_data), 
									void (*pack_body)(LLSelectNode* node, void *user_data), 
									void *user_data,
									ESendType send_type)
{
	LLSelectNode* node;
	LLViewerRegion*	last_region;
	LLViewerRegion*	current_region;

	S32 objects_sent = 0;
	S32 packets_sent = 0;
	S32 objects_in_this_packet = 0;

	std::queue<LLSelectNode*> nodes_to_send;
	
	switch(send_type)
	{
	case SEND_ONLY_ROOTS:
		node = mSelectedObjects->getFirstRootNode();
		while(node)
		{
			nodes_to_send.push(node);
			node = mSelectedObjects->getNextRootNode();
		}
		break;
	case SEND_INDIVIDUALS:
		node = mSelectedObjects->getFirstNode();
		while(node)
		{
			nodes_to_send.push(node);
			node = mSelectedObjects->getNextNode();
		}
		break;
	case SEND_ROOTS_FIRST:
		// first roots...
		node = mSelectedObjects->getFirstNode();
		while(node)
		{
			if (node->getObject()->isRootEdit())
			{
				nodes_to_send.push(node);
			}
			node = mSelectedObjects->getNextNode();
		}

		// then children...
		node = mSelectedObjects->getFirstNode();
		while(node)
		{
			if (!node->getObject()->isRootEdit())
			{
				nodes_to_send.push(node);
			}
			node = mSelectedObjects->getNextNode();
		}
		break;
	case SEND_CHILDREN_FIRST:
		// first children...
		node = mSelectedObjects->getFirstNode();
		while(node)
		{
			if (!node->getObject()->isRootEdit())
			{
				nodes_to_send.push(node);
			}
			node = mSelectedObjects->getNextNode();
		}

		// ...then roots
		node = mSelectedObjects->getFirstNode();
		while(node)
		{
			if (node->getObject()->isRootEdit())
			{
				nodes_to_send.push(node);
			}
			node = mSelectedObjects->getNextNode();
		}
		break;

	default:
		llerrs << "Bad send type " << send_type << " passed to SendListToRegions()" << llendl;
	}

	// bail if nothing selected
	if (nodes_to_send.empty()) return;

	node = nodes_to_send.front();
	nodes_to_send.pop();

	// cache last region information
	current_region = node->getObject()->getRegion();

	// Start duplicate message
	// CRO: this isn't 
	gMessageSystem->newMessage(message_name.c_str());
	(*pack_header)(user_data);

	// For each object
	while (node != NULL)
	{
		// remember the last region, look up the current one
		last_region = current_region;
		current_region = node->getObject()->getRegion();

		// if to same simulator and message not too big
		if ((current_region == last_region)
			&& (gMessageSystem->mCurrentSendTotal < MTUBYTES)
			&& (objects_in_this_packet < MAX_OBJECTS_PER_PACKET))
		{
			// add another instance of the body of the data
			(*pack_body)(node, user_data);
			++objects_sent;
			++objects_in_this_packet;

			// and on to the next object
			if(nodes_to_send.empty())
			{
				node = NULL;
			}
			else
			{
				node =  nodes_to_send.front();
				nodes_to_send.pop();
			}
		}
		else
		{
			// otherwise send current message and start new one
			gMessageSystem->sendReliable( last_region->getHost());
			packets_sent++;
			objects_in_this_packet = 0;

			gMessageSystem->newMessage(message_name.c_str());
			(*pack_header)(user_data);

			// don't move to the next object, we still need to add the
			// body data. 
		}
	}

	// flush messages
	if (gMessageSystem->mCurrentSendTotal > 0)
	{
		gMessageSystem->sendReliable( current_region->getHost());
		packets_sent++;
	}
	else
	{
		gMessageSystem->clearMessage();
	}

	// llinfos << "sendListToRegions " << message_name << " obj " << objects_sent << " pkt " << packets_sent << llendl;
}


//
// Network communications
//

void LLSelectMgr::requestObjectPropertiesFamily(LLViewerObject* object)
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_RequestObjectPropertiesFamily);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_RequestFlags, 0x0 );
	msg->addUUIDFast(_PREHASH_ObjectID, object->mID );

	LLViewerRegion* regionp = object->getRegion();
	msg->sendReliable( regionp->getHost() );
}


// static
void LLSelectMgr::processObjectProperties(LLMessageSystem* msg, void** user_data)
{
	S32 i;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_ObjectData);
	for (i = 0; i < count; i++)
	{
		LLUUID id;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, id, i);

		LLUUID creator_id;
		LLUUID owner_id;
		LLUUID group_id;
		LLUUID last_owner_id;
		U64 creation_date;
		LLUUID extra_id;
		U32 base_mask, owner_mask, group_mask, everyone_mask, next_owner_mask;
		LLSaleInfo sale_info;
		LLCategory category;
		LLAggregatePermissions ag_perms;
		LLAggregatePermissions ag_texture_perms;
		LLAggregatePermissions ag_texture_perms_owner;
		
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_CreatorID, creator_id, i);
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, owner_id, i);
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, group_id, i);
		msg->getU64Fast(_PREHASH_ObjectData, _PREHASH_CreationDate, creation_date, i);
		msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_BaseMask, base_mask, i);
		msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_OwnerMask, owner_mask, i);
		msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_GroupMask, group_mask, i);
		msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_EveryoneMask, everyone_mask, i);
		msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_NextOwnerMask, next_owner_mask, i);
		sale_info.unpackMultiMessage(msg, _PREHASH_ObjectData, i);

		ag_perms.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePerms, i);
		ag_texture_perms.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePermTextures, i);
		ag_texture_perms_owner.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePermTexturesOwner, i);
		category.unpackMultiMessage(msg, _PREHASH_ObjectData, i);

		S16 inv_serial = 0;
		msg->getS16Fast(_PREHASH_ObjectData, _PREHASH_InventorySerial, inv_serial, i);

		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ItemID, item_id, i);
		LLUUID folder_id;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_FolderID, folder_id, i);
		LLUUID from_task_id;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_FromTaskID, from_task_id, i);

		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_LastOwnerID, last_owner_id, i);

		char name[DB_INV_ITEM_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, DB_INV_ITEM_NAME_BUF_SIZE, name, i);
		char desc[DB_INV_ITEM_DESC_BUF_SIZE];		/* Flawfinder: ignore */
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, DB_INV_ITEM_DESC_BUF_SIZE, desc, i);

		char touch_name[DB_INV_ITEM_NAME_BUF_SIZE];		/* Flawfinder: ignore */
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_TouchName, DB_INV_ITEM_NAME_BUF_SIZE, touch_name, i);
		char sit_name[DB_INV_ITEM_DESC_BUF_SIZE];		/* Flawfinder: ignore */
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_SitName, DB_INV_ITEM_DESC_BUF_SIZE, sit_name, i);

		//unpack TE IDs
		std::vector<LLUUID> texture_ids;
		S32 size = msg->getSizeFast(_PREHASH_ObjectData, i, _PREHASH_TextureID);
		if (size > 0)
		{
			S8 packed_buffer[SELECT_MAX_TES * UUID_BYTES];
			msg->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureID, packed_buffer, 0, i, SELECT_MAX_TES * UUID_BYTES);

			for (S32 buf_offset = 0; buf_offset < size; buf_offset += UUID_BYTES)
			{
				LLUUID id;
				memcpy(id.mData, packed_buffer + buf_offset, UUID_BYTES);		/* Flawfinder: ignore */
				texture_ids.push_back(id);
			}
		}


		// Iterate through nodes at end, since it can be on both the regular AND hover list
		BOOL found = FALSE;
		LLSelectNode* node;
		for (node = gSelectMgr->mSelectedObjects->getFirstNode();
			 node;
			 node = gSelectMgr->mSelectedObjects->getNextNode())
		{
			if (node->getObject()->mID == id)
			{
				found = TRUE;
				break;
			}
		}


		if (node)
		{
			if (node->mInventorySerial != inv_serial)
			{
				node->getObject()->dirtyInventory();
			}

			// save texture data as soon as we get texture perms first time
			if (!node->mValid)
			{
				BOOL can_copy = FALSE;
				BOOL can_transfer = FALSE;

				LLAggregatePermissions::EValue value = LLAggregatePermissions::AP_NONE;
				if(node->getObject()->permYouOwner())
				{
					value = ag_texture_perms_owner.getValue(PERM_COPY);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_copy = TRUE;
					}
					value = ag_texture_perms_owner.getValue(PERM_TRANSFER);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_transfer = TRUE;
					}
				}
				else
				{
					value = ag_texture_perms.getValue(PERM_COPY);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_copy = TRUE;
					}
					value = ag_texture_perms.getValue(PERM_TRANSFER);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_transfer = TRUE;
					}
				}

				if (can_copy && can_transfer)
				{
					// this should be the only place that saved textures is called
					node->saveTextures(texture_ids);
				}
			}

			node->mValid = TRUE;
			node->mPermissions->init(creator_id, owner_id,
									 last_owner_id, group_id);
			node->mPermissions->initMasks(base_mask, owner_mask, everyone_mask, group_mask, next_owner_mask);
			node->mCreationDate = creation_date;
			node->mItemID = item_id;
			node->mFolderID = folder_id;
			node->mFromTaskID = from_task_id;
			node->mName.assign(name);
			node->mDescription.assign(desc);
			node->mSaleInfo = sale_info;
			node->mAggregatePerm = ag_perms;
			node->mAggregateTexturePerm = ag_texture_perms;
			node->mAggregateTexturePermOwner = ag_texture_perms_owner;
			node->mCategory = category;
			node->mInventorySerial = inv_serial;
			node->mSitName.assign(sit_name);
			node->mTouchName.assign(touch_name);
		}
	}

	dialog_refresh_all();

	// silly hack to allow 'save into inventory' 
	if(gPopupMenuView->getVisible())
	{
		gPopupMenuView->setItemEnabled(SAVE_INTO_INVENTORY,
									   enable_save_into_inventory(NULL));
	}

	// hack for left-click buy object
	LLToolPie::selectionPropertiesReceived();
}

// static
void LLSelectMgr::processObjectPropertiesFamily(LLMessageSystem* msg, void** user_data)
{
	LLUUID id;

	U32 request_flags;
	LLUUID creator_id;
	LLUUID owner_id;
	LLUUID group_id;
	LLUUID extra_id;
	U32 base_mask, owner_mask, group_mask, everyone_mask, next_owner_mask;
	LLSaleInfo sale_info;
	LLCategory category;
	
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_RequestFlags,	request_flags );
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID,		id );
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID,		owner_id );
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID,		group_id );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_BaseMask,		base_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_OwnerMask,		owner_mask );
	msg->getU32Fast(_PREHASH_ObjectData,_PREHASH_GroupMask,		group_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_EveryoneMask,	everyone_mask );
	msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_NextOwnerMask, next_owner_mask);
	sale_info.unpackMessage(msg, _PREHASH_ObjectData);
	category.unpackMessage(msg, _PREHASH_ObjectData);

	LLUUID last_owner_id;
	msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_LastOwnerID, last_owner_id );

	// unpack name & desc
	char name[DB_INV_ITEM_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, DB_INV_ITEM_NAME_BUF_SIZE, name);

	char desc[DB_INV_ITEM_DESC_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, DB_INV_ITEM_DESC_BUF_SIZE, desc);

	// the reporter widget askes the server for info about picked objects
	if (request_flags & (COMPLAINT_REPORT_REQUEST | BUG_REPORT_REQUEST))
	{
		EReportType report_type = (COMPLAINT_REPORT_REQUEST & request_flags) ? COMPLAINT_REPORT : BUG_REPORT;
		LLFloaterReporter *reporterp = LLFloaterReporter::getReporter(report_type);
		if (reporterp)
		{
			char first_name[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			char last_name[DB_LAST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
			gCacheName->getName(owner_id, first_name, last_name);
			LLString fullname(first_name);
			fullname.append(" ");
			fullname.append(last_name);
			reporterp->setPickedObjectProperties(name, fullname.c_str());
		}
	}

	// Now look through all of the hovered nodes
	BOOL found = FALSE;
	LLSelectNode* node;
	for (node = gSelectMgr->mHoverObjects->getFirstNode();
		 node;
		 node = gSelectMgr->mHoverObjects->getNextNode())
	{
		if (node->getObject()->mID == id)
		{
			found = TRUE;
			break;
		}
	}

	if (node)
	{
		node->mValid = TRUE;
		node->mPermissions->init(LLUUID::null, owner_id,
								 last_owner_id, group_id);
		node->mPermissions->initMasks(base_mask, owner_mask, everyone_mask, group_mask, next_owner_mask);
		node->mSaleInfo = sale_info;
		node->mCategory = category;
		node->mName.assign(name);
		node->mDescription.assign(desc);
	}

	dialog_refresh_all();
}


// static
void LLSelectMgr::processForceObjectSelect(LLMessageSystem* msg, void**)
{
	BOOL reset_list;
	msg->getBOOL("Header", "ResetList", reset_list);

	if (reset_list)
	{
		gSelectMgr->deselectAll();
	}

	LLUUID full_id;
	S32 local_id;
	LLViewerObject* object;
	LLDynamicArray<LLViewerObject*> objects;
	S32 i;
	S32 block_count = msg->getNumberOfBlocks("Data");

	for (i = 0; i < block_count; i++)
	{
		msg->getS32("Data", "LocalID", local_id, i);

		gObjectList.getUUIDFromLocal(full_id, 
									 local_id, 
									 msg->getSenderIP(),
									 msg->getSenderPort());
		object = gObjectList.findObject(full_id);
		if (object)
		{
			objects.put(object);
		}
	}

	// Don't select, just highlight
	gSelectMgr->highlightObjectAndFamily(objects);
}


extern LLGLdouble	gGLModelView[16];

void LLSelectMgr::updateSilhouettes()
{
	LLSelectNode *node;
	S32 num_sils_genned = 0;

	LLVector3d	cameraPos = gAgent.getCameraPositionGlobal();
	F32 currentCameraZoom = gAgent.getCurrentCameraBuildOffset();

	if (!mSilhouetteImagep)
	{
		LLUUID id;
		id.set( gViewerArt.getString("silhouette.tga") );
		mSilhouetteImagep = gImageList.getImage(id, TRUE, TRUE);
	}


	if((cameraPos - mLastCameraPos).magVecSquared() > SILHOUETTE_UPDATE_THRESHOLD_SQUARED * currentCameraZoom * currentCameraZoom)
	{
		for (node = mSelectedObjects->getFirstNode(); node; node = mSelectedObjects->getNextNode() )
		{
			if (node->getObject())
			{
				node->getObject()->setChanged(LLXform::SILHOUETTE);
			}
		}
		
		mLastCameraPos = gAgent.getCameraPositionGlobal();
	}
	
	LLDynamicArray<LLViewerObject*> changed_objects;

	if (mSelectedObjects->getNumNodes())
	{
		//gGLSPipelineSelection.set();

		//mSilhouetteImagep->bindTexture();
		//glAlphaFunc(GL_GREATER, sHighlightAlphaTest);

		for (S32 pass = 0; pass < 2; pass++)
		{
			for (node = mSelectedObjects->getFirstNode(); node; node = mSelectedObjects->getNextNode() )
			{
				LLViewerObject* objectp = node->getObject();

				// do roots first, then children so that root flags are cleared ASAP
				BOOL roots_only = (pass == 0);
				BOOL is_root = (objectp->isRootEdit());
				if (roots_only != is_root || objectp->mDrawable.isNull())
				{
					continue;
				}

				if (!node->mSilhouetteExists 
					|| objectp->isChanged(LLXform::SILHOUETTE)
					|| (objectp->getParent() && objectp->getParent()->isChanged(LLXform::SILHOUETTE)))
				{
					if (num_sils_genned++ < MAX_SILS_PER_FRAME && objectp->mDrawable->isVisible())
					{
						generateSilhouette(node, gCamera->getOrigin());
						changed_objects.put(objectp);
					}
					else if (objectp->isAttachment())
					{
						//RN: hack for orthogonal projection of HUD attachments
						LLViewerJointAttachment* attachment_pt = (LLViewerJointAttachment*)objectp->getRootEdit()->mDrawable->getParent();
						if (attachment_pt && attachment_pt->getIsHUDAttachment())
						{
							LLVector3 camera_pos = LLVector3(-10000.f, 0.f, 0.f);
							generateSilhouette(node, camera_pos);
						}
					}
				}
			}
		}
	}

	if (mRectSelectedObjects.size() > 0)
	{
		//gGLSPipelineSelection.set();

		//mSilhouetteImagep->bindTexture();
		//glAlphaFunc(GL_GREATER, sHighlightAlphaTest);

		std::set<LLViewerObject*> roots;

		// sync mHighlightedObjects with mRectSelectedObjects since the latter is rebuilt every frame and former
		// persists from frame to frame to avoid regenerating object silhouettes
		// mHighlightedObjects includes all siblings of rect selected objects

		BOOL select_linked_set = gSavedSettings.getBOOL("SelectLinkedSet");

		// generate list of roots from current object selection
		for (std::set<LLPointer<LLViewerObject> >::iterator iter = mRectSelectedObjects.begin();
			 iter != mRectSelectedObjects.end(); iter++)
		{
			LLViewerObject *objectp = *iter;
			if (select_linked_set)
			{
				LLViewerObject *rootp = (LLViewerObject*)objectp->getRoot();
				roots.insert(rootp);
			}
			else
			{
				roots.insert(objectp);
			}
		}

		// remove highlight nodes not in roots list
		LLDynamicArray<LLSelectNode*> remove_these_nodes;
		LLDynamicArray<LLViewerObject*> remove_these_roots;
		for (LLSelectNode* nodep = mHighlightedObjects->getFirstNode(); nodep; nodep = mHighlightedObjects->getNextNode())
		{
			LLViewerObject* objectp = nodep->getObject();
			if (objectp->isRoot() || !select_linked_set)
			{
				if (roots.count(objectp) == 0)
				{
					remove_these_nodes.put(nodep);
				}
				else
				{
					remove_these_roots.put(objectp);
				}
			}
			else
			{
				LLViewerObject* rootp = (LLViewerObject*)objectp->getRoot();

				if (roots.count(rootp) == 0)
				{
					remove_these_nodes.put(nodep);
				}
			}
		}

		// remove all highlight nodes no longer in rectangle selection
		S32 i;
		for (i = 0; i < remove_these_nodes.count(); i++)
		{
			mHighlightedObjects->removeNode(remove_these_nodes[i]);
		}

		// remove all root objects already being highlighted
		for (i = 0; i < remove_these_roots.count(); i++)
		{
			roots.erase(remove_these_roots[i]);
		}

		// add all new objects in rectangle selection
		for (std::set<LLViewerObject*>::iterator iter = roots.begin();
			 iter != roots.end(); iter++)
		{
			LLViewerObject* objectp = *iter;
			LLSelectNode* rect_select_node = new LLSelectNode(objectp, TRUE);
			rect_select_node->selectAllTEs(TRUE);

			if (!canSelectObject(objectp))
			{
				continue;
			}

			mHighlightedObjects->addNode(rect_select_node);

			if (!select_linked_set)
			{
				rect_select_node->mIndividualSelection = TRUE;
			}
			else
			{
				for (U32 i = 0; i < objectp->mChildList.size(); i++)
				{
					LLViewerObject* child_objectp = objectp->mChildList[i];

					if (!canSelectObject(child_objectp))
					{
						continue;
					}

					rect_select_node = new LLSelectNode(objectp->mChildList[i], TRUE);
					rect_select_node->selectAllTEs(TRUE);
					mHighlightedObjects->addNode(rect_select_node);
				}
			}
		}

		num_sils_genned	= 0;

		// render silhouettes for highlighted objects
		//BOOL subtracting_from_selection = (gKeyboard->currentMask(TRUE) == MASK_CONTROL);
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (node = mHighlightedObjects->getFirstNode(); node; node = mHighlightedObjects->getNextNode() )
			{
				LLViewerObject* objectp = node->getObject();

				// do roots first, then children so that root flags are cleared ASAP
				BOOL roots_only = (pass == 0);
				BOOL is_root = objectp->isRootEdit();
				if (roots_only != is_root)
				{
					continue;
				}

				if (!node->mSilhouetteExists 
					|| objectp->isChanged(LLXform::SILHOUETTE)
					|| (objectp->getParent() && objectp->getParent()->isChanged(LLXform::SILHOUETTE)))
				{
					if (num_sils_genned++ < MAX_SILS_PER_FRAME)
					{
						generateSilhouette(node, gCamera->getOrigin());
						changed_objects.put(objectp);			
					}
					else if (objectp->isAttachment() && objectp->getRootEdit()->mDrawable.notNull())
					{
						//RN: hack for orthogonal projection of HUD attachments
						LLViewerJointAttachment* attachment_pt = (LLViewerJointAttachment*)objectp->getRootEdit()->mDrawable->getParent();
						if (attachment_pt && attachment_pt->getIsHUDAttachment())
						{
							LLVector3 camera_pos = LLVector3(-10000.f, 0.f, 0.f);
							generateSilhouette(node, camera_pos);
						}
					}
				}
				//LLColor4 highlight_color;
				//
				//if (subtracting_from_selection)
				//{
				//	node->renderOneSilhouette(LLColor4::red);
				//}
				//else if (!objectp->isSelected())
				//{
				//	highlight_color = objectp->isRoot() ? sHighlightParentColor : sHighlightChildColor;
				//	node->renderOneSilhouette(highlight_color);
				//}
			}
		}
		//mSilhouetteImagep->unbindTexture(0, GL_TEXTURE_2D);
	}
	else
	{
		mHighlightedObjects->deleteAllNodes();
	}

	for (S32 i = 0; i < changed_objects.count(); i++)
	{
		// clear flags after traversing node list (as child objects need to refer to parent flags, etc)
		changed_objects[i]->clearChanged(LLXform::MOVED | LLXform::SILHOUETTE);
	}
	
	//glAlphaFunc(GL_GREATER, 0.01f);
}

void LLSelectMgr::renderSilhouettes(BOOL for_hud)
{
	if (!mRenderSilhouettes)
	{
		return;
	}

	LLSelectNode *node;
	LLViewerImage::bindTexture(gSelectMgr->mSilhouetteImagep);
	LLGLSPipelineSelection gls_select;
	glAlphaFunc(GL_GREATER, 0.0f);
	LLGLEnable blend(GL_BLEND);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (for_hud && avatar)
	{
		LLBBox hud_bbox = avatar->getHUDBBox();

		F32 cur_zoom = avatar->mHUDCurZoom;

		// set up transform to encompass bounding box of HUD
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		F32 depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		glOrtho(-0.5f * gCamera->getAspect(), 0.5f * gCamera->getAspect(), -0.5f, 0.5f, 0.f, depth);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glLoadMatrixf(OGL_TO_CFR_ROTATION);		// Load Cory's favorite reference frame
		glTranslatef(-hud_bbox.getCenterLocal().mV[VX] + (depth *0.5f), 0.f, 0.f);
		glScalef(cur_zoom, cur_zoom, cur_zoom);
	}
	if (mSelectedObjects->getNumNodes())
	{
		glPushAttrib(GL_FOG_BIT);
		LLUUID inspect_item_id = LLFloaterInspect::getSelectedUUID();
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (node = mSelectedObjects->getFirstNode(); node; node = mSelectedObjects->getNextNode() )
			{
				LLViewerObject* objectp = node->getObject();
				if (objectp->isHUDAttachment() != for_hud)
				{
					continue;
				}
				if(objectp->getID() == inspect_item_id)
				{
					node->renderOneSilhouette(sHighlightInspectColor);
				}
				else if (node->isTransient())
				{
					BOOL oldHidden = LLSelectMgr::sRenderHiddenSelections;
					LLSelectMgr::sRenderHiddenSelections = FALSE;
					node->renderOneSilhouette(sContextSilhouetteColor);
					LLSelectMgr::sRenderHiddenSelections = oldHidden;
				}
				else if (objectp->isRootEdit())
				{
					node->renderOneSilhouette(sSilhouetteParentColor);
				}
				else
				{
					node->renderOneSilhouette(sSilhouetteChildColor);
				}
			}
		}
		glPopAttrib();
	}

	if (mHighlightedObjects->getNumNodes())
	{
		// render silhouettes for highlighted objects
		BOOL subtracting_from_selection = (gKeyboard->currentMask(TRUE) == MASK_CONTROL);
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (node = mHighlightedObjects->getFirstNode(); node; node = mHighlightedObjects->getNextNode() )
			{
				LLViewerObject* objectp = node->getObject();
				if (objectp->isHUDAttachment() != for_hud)
				{
					continue;
				}

				if (subtracting_from_selection)
				{
					node->renderOneSilhouette(LLColor4::red);
				}
				else if (!objectp->isSelected())
				{
					LLColor4 highlight_color = objectp->isRoot() ? sHighlightParentColor : sHighlightChildColor;
					node->renderOneSilhouette(highlight_color);
				}
			}
		}
	}

	if (for_hud && avatar)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		stop_glerror();
	}

	gSelectMgr->mSilhouetteImagep->unbindTexture(0, GL_TEXTURE_2D);
	glAlphaFunc(GL_GREATER, 0.01f);
}

void LLSelectMgr::generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point)
{
	LLViewerObject* objectp = nodep->getObject();

	if (objectp && objectp->getPCode() == LL_PCODE_VOLUME)
	{
		((LLVOVolume*)objectp)->generateSilhouette(nodep, view_point);
	}
}

void LLSelectMgr::getSilhouetteExtents(LLSelectNode* nodep, const LLQuaternion& orientation, LLVector3& min_extents, LLVector3& max_extents)
{
	LLViewerObject* objectp = nodep->getObject();

	if (objectp->mDrawable.isNull())
	{
		return;
	}
	
	LLQuaternion test_rot = orientation * ~objectp->getRenderRotation();
	LLVector3 x_axis_rot = LLVector3::x_axis * test_rot;
	LLVector3 y_axis_rot = LLVector3::y_axis * test_rot;
	LLVector3 z_axis_rot = LLVector3::z_axis * test_rot;

	x_axis_rot.scaleVec(objectp->mDrawable->getScale());
	y_axis_rot.scaleVec(objectp->mDrawable->getScale());
	z_axis_rot.scaleVec(objectp->mDrawable->getScale());

	generateSilhouette(nodep, objectp->mDrawable->getPositionAgent() + x_axis_rot * 100.f);

	S32 num_vertices = nodep->mSilhouetteVertices.size();
	if (num_vertices)
	{
		min_extents.mV[VY] = llmin(min_extents.mV[VY], nodep->mSilhouetteVertices[0] * y_axis_rot);
		max_extents.mV[VY] = llmax(max_extents.mV[VY], nodep->mSilhouetteVertices[0] * y_axis_rot);

		min_extents.mV[VZ] = llmin(min_extents.mV[VZ], nodep->mSilhouetteVertices[0] * z_axis_rot);
		max_extents.mV[VZ] = llmax(min_extents.mV[VZ], nodep->mSilhouetteVertices[0] * z_axis_rot);

		for (S32 vert = 1; vert < num_vertices; vert++)
		{
			F32 y_pos = nodep->mSilhouetteVertices[vert] * y_axis_rot;
			F32 z_pos = nodep->mSilhouetteVertices[vert] * z_axis_rot;
			min_extents.mV[VY] = llmin(y_pos, min_extents.mV[VY]);
			max_extents.mV[VY] = llmax(y_pos, max_extents.mV[VY]);
			min_extents.mV[VZ] = llmin(z_pos, min_extents.mV[VZ]);
			max_extents.mV[VZ] = llmax(z_pos, max_extents.mV[VZ]);
		}
	}

	generateSilhouette(nodep, objectp->mDrawable->getPositionAgent() + y_axis_rot * 100.f);

	num_vertices = nodep->mSilhouetteVertices.size();
	if (num_vertices)
	{
		min_extents.mV[VX] = llmin(min_extents.mV[VX], nodep->mSilhouetteVertices[0] * x_axis_rot);
		max_extents.mV[VX] = llmax(max_extents.mV[VX], nodep->mSilhouetteVertices[0] * x_axis_rot);

		for (S32 vert = 1; vert < num_vertices; vert++)
		{
			F32 x_pos = nodep->mSilhouetteVertices[vert] * x_axis_rot;
			min_extents.mV[VX] = llmin(x_pos, min_extents.mV[VX]);
			max_extents.mV[VX] = llmax(x_pos, max_extents.mV[VX]);
		}
	}

	generateSilhouette(nodep, gCamera->getOrigin());
}


//
// Utility classes
//
LLSelectNode::LLSelectNode(LLViewerObject* object, BOOL glow)
{
	mObject = object;
	selectAllTEs(FALSE);
	mIndividualSelection	= FALSE;
	mTransient		= FALSE;
	mValid			= FALSE;
	mPermissions	= new LLPermissions();
	mInventorySerial = 0;
	mName = LLString::null;
	mDescription = LLString::null;
	mTouchName = LLString::null;
	mSitName = LLString::null;
	mSilhouetteExists = FALSE;
	mDuplicated = FALSE;

	saveColors();
}

LLSelectNode::LLSelectNode(const LLSelectNode& nodep)
{
	S32 i;
	for (i = 0; i < SELECT_MAX_TES; i++)
	{
		mTESelected[i] = nodep.mTESelected[i];
	}
	mLastTESelected = nodep.mLastTESelected;

	mIndividualSelection = nodep.mIndividualSelection;

	mValid = nodep.mValid;
	mTransient		= nodep.mTransient;
	mPermissions = new LLPermissions(*nodep.mPermissions);
	mSaleInfo = nodep.mSaleInfo;;
	mAggregatePerm = nodep.mAggregatePerm;
	mAggregateTexturePerm = nodep.mAggregateTexturePerm;
	mAggregateTexturePermOwner = nodep.mAggregateTexturePermOwner;
	mName = nodep.mName;
	mDescription = nodep.mDescription;
	mCategory = nodep.mCategory;
	mSavedPositionLocal = nodep.mSavedPositionLocal;
	mSavedPositionGlobal = nodep.mSavedPositionGlobal;
	mSavedScale = nodep.mSavedScale;
	mSavedRotation = nodep.mSavedRotation;
	mDuplicated = nodep.mDuplicated;
	mDuplicatePos = nodep.mDuplicatePos;
	mDuplicateRot = nodep.mDuplicateRot;
	mItemID = nodep.mItemID;
	mFolderID = nodep.mFolderID;
	mFromTaskID = nodep.mFromTaskID;
	mTouchName = nodep.mTouchName;
	mSitName = nodep.mSitName;

	mSilhouetteVertices = nodep.mSilhouetteVertices;
	mSilhouetteNormals = nodep.mSilhouetteNormals;
	mSilhouetteSegments = nodep.mSilhouetteSegments;
	mSilhouetteExists = nodep.mSilhouetteExists;
	mObject = nodep.mObject;

	std::vector<LLColor4>::const_iterator color_iter;
	mSavedColors.clear();
	for (color_iter = nodep.mSavedColors.begin(); color_iter != nodep.mSavedColors.end(); ++color_iter)
	{
		mSavedColors.push_back(*color_iter);
	}
	
	saveTextures(nodep.mSavedTextures);
}

LLSelectNode::~LLSelectNode()
{
	delete mPermissions;
	mPermissions = NULL;
}

void LLSelectNode::selectAllTEs(BOOL b)
{
	for (S32 i = 0; i < SELECT_MAX_TES; i++)
	{
		mTESelected[i] = b;
	}
	mLastTESelected = 0;
}

void LLSelectNode::selectTE(S32 te_index, BOOL selected)
{
	if (te_index < 0 || te_index >= SELECT_MAX_TES)
	{
		return;
	}
	mTESelected[te_index] = selected;
	mLastTESelected = te_index;
}

BOOL LLSelectNode::isTESelected(S32 te_index)
{
	if (te_index < 0 || te_index >= mObject->getNumTEs())
	{
		return FALSE;
	}
	return mTESelected[te_index];
}

S32 LLSelectNode::getLastSelectedTE()
{
	if (!isTESelected(mLastTESelected))
	{
		return -1;
	}
	return mLastTESelected;
}

LLViewerObject *LLSelectNode::getObject()
{
	if (!mObject)
	{
		return NULL;
	}
	else if (mObject->isDead())
	{
		mObject = NULL;
	}
	return mObject;
}

void LLSelectNode::saveColors()
{
	if (mObject.notNull())
	{
		mSavedColors.clear();
		for (S32 i = 0; i < mObject->getNumTEs(); i++)
		{
			const LLTextureEntry* tep = mObject->getTE(i);
			mSavedColors.push_back(tep->getColor());
		}
	}
}

void LLSelectNode::saveTextures(const std::vector<LLUUID>& textures)
{
	if (mObject.notNull())
	{
		mSavedTextures.clear();

		std::vector<LLUUID>::const_iterator texture_it;
		for (texture_it = textures.begin(); texture_it != textures.end(); ++texture_it)
		{
			mSavedTextures.push_back(*texture_it);
		}
	}
}

void LLSelectNode::saveTextureScaleRatios()
{
	mTextureScaleRatios.clear();
	if (mObject.notNull())
	{
		for (U8 i = 0; i < mObject->getNumTEs(); i++)
		{
			F32 s,t;
			const LLTextureEntry* tep = mObject->getTE(i);
			tep->getScale(&s,&t);
			U32 s_axis, t_axis;

			gSelectMgr->getTESTAxes(mObject, i, &s_axis, &t_axis);

			LLVector3 v;
			LLVector3 scale = mObject->getScale();

			if (tep->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR)
			{
				v.mV[s_axis] = s*scale.mV[s_axis];
				v.mV[t_axis] = t*scale.mV[t_axis];
			}
			else
			{
				v.mV[s_axis] = s/scale.mV[s_axis];
				v.mV[t_axis] = t/scale.mV[t_axis];
			}

			mTextureScaleRatios.push_back(v);
		}
	}
}


// This implementation should be similar to LLTask::allowOperationOnTask
BOOL LLSelectNode::allowOperationOnNode(PermissionBit op, U64 group_proxy_power) const
{
	// Extract ownership.
	BOOL object_is_group_owned = FALSE;
	LLUUID object_owner_id;
	mPermissions->getOwnership(object_owner_id, object_is_group_owned);

	// Operations on invalid or public objects is not allowed.
	if (!mObject || (mObject->isDead()) || !mPermissions->isOwned())
	{
		return FALSE;
	}

	// The transfer permissions can never be given through proxy.
	if (PERM_TRANSFER == op)
	{
		// The owner of an agent-owned object can transfer to themselves.
		if ( !object_is_group_owned 
			&& (gAgent.getID() == object_owner_id) )
		{
			return TRUE;
		}
		else
		{
			// Otherwise check aggregate permissions.
			return mObject->permTransfer();
		}
	}

	if (PERM_MOVE == op
		|| PERM_MODIFY == op)
	{
		// only owners can move or modify their attachments
		// no proxy allowed.
		if (mObject->isAttachment() && object_owner_id != gAgent.getID())
		{
			return FALSE;
		}
	}

	// Calculate proxy_agent_id and group_id to use for permissions checks.
	// proxy_agent_id may be set to the object owner through group powers.
	// group_id can only be set to the object's group, if the agent is in that group.
	LLUUID group_id = LLUUID::null;
	LLUUID proxy_agent_id = gAgent.getID();

	// Gods can always operate.
	if (gAgent.isGodlike())
	{
		return TRUE;
	}

	// Check if the agent is in the same group as the object.
	LLUUID object_group_id = mPermissions->getGroup();
	if (object_group_id.notNull() &&
		gAgent.isInGroup(object_group_id))
	{
		// Assume the object's group during this operation.
		group_id = object_group_id;
	}

	// Only allow proxy powers for PERM_COPY if the actual agent can
	// receive the item (ie has PERM_TRANSFER permissions).
	// NOTE: op == PERM_TRANSFER has already been handled, but if
	// that ever changes we need to BLOCK proxy powers for PERM_TRANSFER.  DK 03/28/06
	if (PERM_COPY != op || mPermissions->allowTransferTo(gAgent.getID()))
	{
		// Check if the agent can assume ownership through group proxy or agent-granted proxy.
		if (   ( object_is_group_owned 
				&& gAgent.hasPowerInGroup(object_owner_id, group_proxy_power))
				// Only allow proxy for move, modify, and copy.
				|| ( (PERM_MOVE == op || PERM_MODIFY == op || PERM_COPY == op)
					&& (!object_is_group_owned
						&& gAgent.isGrantedProxy(*mPermissions))))
		{
			// This agent is able to assume the ownership role for this operation.
			proxy_agent_id = object_owner_id;
		}
	}
	
	// We now have max ownership information.
	if (PERM_OWNER == op)
	{
		// This this was just a check for ownership, we can now return the answer.
		return (proxy_agent_id == object_owner_id ? TRUE : FALSE);
	}

	// check permissions to see if the agent can operate
	return (mPermissions->allowOperationBy(op, proxy_agent_id, group_id));
}

//-----------------------------------------------------------------------------
// renderOneSilhouette()
//-----------------------------------------------------------------------------
void LLSelectNode::renderOneSilhouette(const LLColor4 &color)
{
	LLViewerObject* objectp = getObject();
	if (!objectp)
	{
		return;
	}

	LLDrawable* drawable = objectp->mDrawable;
	if(!drawable)
	{
		return;
	}

	if (!mSilhouetteExists)
	{
		return;
	}

	BOOL is_hud_object = objectp->isHUDAttachment();

	if (!drawable->isVisible() && !is_hud_object)
	{
		return;
	}

	if (mSilhouetteVertices.size() == 0 || mSilhouetteNormals.size() != mSilhouetteVertices.size())
	{
		return;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	if (!is_hud_object)
	{
		glLoadIdentity();
		glMultMatrixd(gGLModelView);
	}
	
	
	if (drawable->isActive())
	{
		glMultMatrixf((F32*) objectp->getRenderMatrix().mMatrix);
	}

	LLVolume *volume = objectp->getVolume();
	if (volume)
	{
		F32 silhouette_thickness;
		if (is_hud_object && gAgent.getAvatarObject())
		{
			silhouette_thickness = LLSelectMgr::sHighlightThickness / gAgent.getAvatarObject()->mHUDCurZoom;
		}
		else
		{
			LLVector3 view_vector = gCamera->getOrigin() - objectp->getRenderPosition();
			silhouette_thickness = drawable->mDistanceWRTCamera * LLSelectMgr::sHighlightThickness * (gCamera->getView() / gCamera->getDefaultFOV());
		}		
		F32 animationTime = (F32)LLFrameTimer::getElapsedSeconds();

		F32 u_coord = fmod(animationTime * LLSelectMgr::sHighlightUAnim, 1.f);
		F32 v_coord = 1.f - fmod(animationTime * LLSelectMgr::sHighlightVAnim, 1.f);
		F32 u_divisor = 1.f / ((F32)(mSilhouetteVertices.size() - 1));

		if (LLSelectMgr::sRenderHiddenSelections) // && gFloaterTools && gFloaterTools->getVisible())
		{
			glBlendFunc(GL_SRC_COLOR, GL_ONE);
			LLGLEnable fog(GL_FOG);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			float d = (gCamera->getPointOfInterest()-gCamera->getOrigin()).magVec();
			LLColor4 fogCol = color * (F32)llclamp((gSelectMgr->getSelectionCenterGlobal()-gAgent.getCameraPositionGlobal()).magVec()/(gSelectMgr->getBBoxOfSelection().getExtentLocal().magVec()*4), 0.0, 1.0);
			glFogf(GL_FOG_START, d);
			glFogf(GL_FOG_END, d*(1 + (gCamera->getView() / gCamera->getDefaultFOV())));
			glFogfv(GL_FOG_COLOR, fogCol.mV);

			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GEQUAL);
			glAlphaFunc(GL_GREATER, 0.01f);
			glBegin(GL_LINES);
			{
				S32 i = 0;
				for (S32 seg_num = 0; seg_num < (S32)mSilhouetteSegments.size(); seg_num++)
				{
// 					S32 first_i = i;
					for(; i < mSilhouetteSegments[seg_num]; i++)
					{
						u_coord += u_divisor * LLSelectMgr::sHighlightUScale;

						glColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
						glTexCoord2f( u_coord, v_coord );
						glVertex3fv( mSilhouetteVertices[i].mV );
					}

					/*u_coord += u_divisor * LLSelectMgr::sHighlightUScale;
					glColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
					glTexCoord2f( u_coord, v_coord );
					glVertex3fv( mSilhouetteVertices[first_i].mV );*/
				}
			}
            glEnd();
			u_coord = fmod(animationTime * LLSelectMgr::sHighlightUAnim, 1.f);
		}

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glAlphaFunc(GL_GREATER, LLSelectMgr::sHighlightAlphaTest);
		glBegin(GL_TRIANGLES);
		{
			S32 i = 0;
			for (S32 seg_num = 0; seg_num < (S32)mSilhouetteSegments.size(); seg_num++)
			{
				S32 first_i = i;
				LLVector3 v;
				LLVector2 t;

				for(; i < mSilhouetteSegments[seg_num]; i++)
				{

					if (i == first_i) {
					    LLVector3 vert = (mSilhouetteNormals[i]) * silhouette_thickness;
						vert += mSilhouetteVertices[i];

						glColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.0f); //LLSelectMgr::sHighlightAlpha);
						glTexCoord2f( u_coord, v_coord + LLSelectMgr::sHighlightVScale );
						glVertex3fv( vert.mV ); 
						
						u_coord += u_divisor * LLSelectMgr::sHighlightUScale;

						glColor4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha*2);
						glTexCoord2f( u_coord, v_coord );
						glVertex3fv( mSilhouetteVertices[i].mV );

						v = mSilhouetteVertices[i];
						t = LLVector2(u_coord, v_coord);
					}
					else {
                        LLVector3 vert = (mSilhouetteNormals[i]) * silhouette_thickness;
						vert += mSilhouetteVertices[i];

						glColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.0f); //LLSelectMgr::sHighlightAlpha);
						glTexCoord2f( u_coord, v_coord + LLSelectMgr::sHighlightVScale );
						glVertex3fv( vert.mV ); 
						glVertex3fv( vert.mV ); 
						
						glTexCoord2fv(t.mV);
						u_coord += u_divisor * LLSelectMgr::sHighlightUScale;
						glColor4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha*2);
						glVertex3fv(v.mV);
						glTexCoord2f( u_coord, v_coord );
						glVertex3fv( mSilhouetteVertices[i].mV );

					}
				}
			}
		}
		glEnd();

	}
	glPopMatrix();
}

//
// Utility Functions
//

// Update everyone who cares about the selection list
void dialog_refresh_all()
{
	if (gNoRender)
	{
		return;
	}

	//could refresh selected object info in toolbar here

	gFloaterTools->dirty();

	if( gPieObject->getVisible() )
	{
		gPieObject->arrange();
	}

	LLFloaterProperties::dirtyAll();
	LLFloaterInspect::dirty();
}

S32 get_family_count(LLViewerObject *parent)
{
	if (!parent)
	{
		llwarns << "Trying to get_family_count on null parent!" << llendl;
	}
	S32 count = 1;	// for this object
	for (U32 i = 0; i < parent->mChildList.size(); i++)
	{
		LLViewerObject* child = parent->mChildList[i];

		if (!child)
		{
			llwarns << "Family object has NULL child!  Show Doug." << llendl;
		}
		else if (child->isDead())
		{
			llwarns << "Family object has dead child object.  Show Doug." << llendl;
		}
		else
		{
			if (gSelectMgr->canSelectObject(child))
			{
				count += get_family_count( child );
			}
		}
	}
	return count;
}

//-----------------------------------------------------------------------------
// updateSelectionCenter
//-----------------------------------------------------------------------------
void LLSelectMgr::updateSelectionCenter()
{
	const F32 MOVE_SELECTION_THRESHOLD = 1.f;		//  Movement threshold in meters for updating selection
													//  center (tractor beam)

	LLViewerObject* object = mSelectedObjects->getFirstObject();
	if (!object)
	{
		// nothing selected, probably grabbing
		// Ignore by setting to avatar origin.
		mSelectionCenterGlobal.clearVec();
		mShowSelection = FALSE;
		mSelectionBBox = LLBBox(); 
		mPauseRequest = NULL;
		if (gAgent.getAvatarObject())
		{
			gAgent.getAvatarObject()->mHUDTargetZoom = 1.f;
			gAgent.getAvatarObject()->mHUDCurZoom = 1.f;
		}
	}
	else
	{
		mSelectedObjects->mSelectType = getSelectTypeForObject(object);

		if (mSelectedObjects->mSelectType == SELECT_TYPE_ATTACHMENT && gAgent.getAvatarObject())
		{
			mPauseRequest = gAgent.getAvatarObject()->requestPause();
		}
		else
		{
			mPauseRequest = NULL;
		}

		if (mSelectedObjects->mSelectType != SELECT_TYPE_HUD && gAgent.getAvatarObject())
		{
			// reset hud ZOOM
			gAgent.getAvatarObject()->mHUDTargetZoom = 1.f;
			gAgent.getAvatarObject()->mHUDCurZoom = 1.f;
		}

		mShowSelection = FALSE;
		LLBBox bbox;

		// have stuff selected
		LLVector3d select_center;
		// keep a list of jointed objects for showing the joint HUDEffects
		gHUDManager->clearJoints();
		LLDynamicArray < LLViewerObject *> jointed_objects;

		for (object = mSelectedObjects->getFirstObject(); object; object = mSelectedObjects->getNextObject() )
		{
			LLViewerObject *myAvatar = gAgent.getAvatarObject();
			LLViewerObject *root = object->getRootEdit();
			if (mSelectedObjects->mSelectType == SELECT_TYPE_WORLD && // not an attachment
				!root->isChild(myAvatar) && // not the object you're sitting on
				!object->isAvatar()) // not another avatar
			{
				mShowSelection = TRUE;
			}

			bbox.addBBoxAgent( object->getBoundingBoxAgent() );

			if (object->isJointChild())
			{
				jointed_objects.put(object);
			}
		} // end for
		
		LLVector3 bbox_center_agent = bbox.getCenterAgent();
		mSelectionCenterGlobal = gAgent.getPosGlobalFromAgent(bbox_center_agent);
		mSelectionBBox = bbox;

		if (jointed_objects.count())
		{
			gHUDManager->showJoints(&jointed_objects);
		}
	}
	
	if ( !(gAgentID == LLUUID::null) && gToolMgr) 
	{
		LLTool		*tool = gToolMgr->getCurrentTool();
		if (mShowSelection)
		{
			LLVector3d select_center_global;

			if( tool->isEditing() )
			{
				select_center_global = tool->getEditingPointGlobal();
			}
			else
			{
				select_center_global = mSelectionCenterGlobal;
			}

			// Send selection center if moved beyond threshold (used to animate tractor beam)	
			LLVector3d diff;
			diff = select_center_global - mLastSentSelectionCenterGlobal;

			if ( diff.magVecSquared() > MOVE_SELECTION_THRESHOLD*MOVE_SELECTION_THRESHOLD )
			{
				//  Transmit updated selection center 
				mLastSentSelectionCenterGlobal = select_center_global;
			}
		}
	}

	// give up edit menu if no objects selected
	if (gEditMenuHandler == this && mSelectedObjects->getObjectCount() == 0)
	{
		gEditMenuHandler = NULL;
	}
}

void LLSelectMgr::updatePointAt()
{
	if (mShowSelection)
	{
		if (mSelectedObjects->getObjectCount())
		{					
			LLVector3 select_offset;
			LLViewerObject *click_object = gObjectList.findObject(gLastHitObjectID);
			if (click_object && click_object->isSelected())
			{
				// clicked on another object in our selection group, use that as target
				select_offset.setVec(gLastHitObjectOffset);
				select_offset.rotVec(~click_object->getRenderRotation());
		
				gAgent.setPointAt(POINTAT_TARGET_SELECT, click_object, select_offset);
				gAgent.setLookAt(LOOKAT_TARGET_SELECT, click_object, select_offset);
			}
			else
			{
				// didn't click on an object this time, revert to pointing at center of first object
				gAgent.setPointAt(POINTAT_TARGET_SELECT, mSelectedObjects->getFirstObject());
				gAgent.setLookAt(LOOKAT_TARGET_SELECT, mSelectedObjects->getFirstObject());
			}
		}
		else
		{
			gAgent.setPointAt(POINTAT_TARGET_CLEAR);
			gAgent.setLookAt(LOOKAT_TARGET_CLEAR);
		}
	}
	else
	{
		gAgent.setPointAt(POINTAT_TARGET_CLEAR);
		gAgent.setLookAt(LOOKAT_TARGET_CLEAR);
	}
}

//-----------------------------------------------------------------------------
// getBBoxOfSelection()
//-----------------------------------------------------------------------------
LLBBox LLSelectMgr::getBBoxOfSelection() const
{
	return mSelectionBBox;
}


//-----------------------------------------------------------------------------
// canUndo()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canUndo()
{
	return mSelectedObjects->getFirstEditableObject() != NULL;
}

//-----------------------------------------------------------------------------
// undo()
//-----------------------------------------------------------------------------
void LLSelectMgr::undo()
{
	BOOL select_linked_set = gSavedSettings.getBOOL("SelectLinkedSet");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Undo", packAgentAndSessionAndGroupID, packObjectID, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canRedo()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canRedo()
{
	return mSelectedObjects->getFirstEditableObject() != NULL;
}

//-----------------------------------------------------------------------------
// redo()
//-----------------------------------------------------------------------------
void LLSelectMgr::redo()
{
	BOOL select_linked_set = gSavedSettings.getBOOL("SelectLinkedSet");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Redo", packAgentAndSessionAndGroupID, packObjectID, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canDoDelete()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canDoDelete()
{
	return mSelectedObjects->getFirstDeleteableObject() != NULL;
}

//-----------------------------------------------------------------------------
// doDelete()
//-----------------------------------------------------------------------------
void LLSelectMgr::doDelete()
{
	selectDelete();
}

//-----------------------------------------------------------------------------
// canDeselect()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canDeselect()
{
	return !mSelectedObjects->isEmpty();
}

//-----------------------------------------------------------------------------
// deselect()
//-----------------------------------------------------------------------------
void LLSelectMgr::deselect()
{
	deselectAll();
}
//-----------------------------------------------------------------------------
// canDuplicate()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canDuplicate()
{
	return mSelectedObjects->getFirstCopyableObject() != NULL;
}
//-----------------------------------------------------------------------------
// duplicate()
//-----------------------------------------------------------------------------
void LLSelectMgr::duplicate()
{
	LLVector3 offset(0.5f, 0.5f, 0.f);
	selectDuplicate(offset, TRUE);
}

ESelectType LLSelectMgr::getSelectTypeForObject(LLViewerObject* object)
{
	if (!object)
	{
		return SELECT_TYPE_WORLD;
	}
	if (object->isHUDAttachment())
	{
		return SELECT_TYPE_HUD;
	}
	else if (object->isAttachment())
	{
		return SELECT_TYPE_ATTACHMENT;
	}
	else
	{
		return SELECT_TYPE_WORLD;
	}
}

void LLSelectMgr::validateSelection()
{
	LLViewerObject* objectp;
	for (objectp = mSelectedObjects->getFirstObject(); objectp; objectp = mSelectedObjects->getNextObject())
	{
		if (!canSelectObject(objectp))
		{
			deselectObjectOnly(objectp);
		}
	}
}

BOOL LLSelectMgr::canSelectObject(LLViewerObject* object)
{
	if (mForceSelection)
	{
		return TRUE;
	}

	if ((gSavedSettings.getBOOL("SelectOwnedOnly") && !object->permYouOwner()) ||
		(gSavedSettings.getBOOL("SelectMovableOnly") && !object->permMove()))
	{
		// only select my own objects
		return FALSE;
	}

	// Can't select dead objects
	if (object->isDead()) return FALSE;

	// Can't select orphans
	if (object->isOrphaned()) return FALSE;
	
	// Can't select avatars
	if (object->isAvatar()) return FALSE;

	// Can't select land
	if (object->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH) return FALSE;

	ESelectType selection_type = getSelectTypeForObject(object);
	if (mSelectedObjects->getObjectCount() > 0 && mSelectedObjects->mSelectType != selection_type) return FALSE;

	return TRUE;
}

LLObjectSelection::LLObjectSelection() : 
	std::list<LLSelectNode*>(), 
	LLRefCount(),
	mCurrentTE(-1),
	mCurrentNode(end()),
	mSelectType(SELECT_TYPE_WORLD)
{
}

LLObjectSelection::~LLObjectSelection()
{
	std::for_each(begin(), end(), DeletePointer());
}

void LLObjectSelection::updateEffects()
{
}

S32 LLObjectSelection::getNumNodes()
{
	return size();
}

void LLObjectSelection::addNode(LLSelectNode *nodep)
{
	push_front(nodep);
	mSelectNodeMap[nodep->getObject()] = nodep;
}

void LLObjectSelection::addNodeAtEnd(LLSelectNode *nodep)
{
	push_back(nodep);
	mSelectNodeMap[nodep->getObject()] = nodep;
}

void LLObjectSelection::removeNode(LLSelectNode *nodep)
{
	std::list<LLSelectNode*>::iterator iter = begin();
	while(iter != end())
	{
		if ((*iter) == nodep)
		{
			mSelectNodeMap.erase(nodep->getObject());
			iter = erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void LLObjectSelection::deleteAllNodes()
{
	std::for_each(begin(), end(), DeletePointer());
	clear();
	mSelectNodeMap.clear();
}

LLSelectNode* LLObjectSelection::findNode(LLViewerObject* objectp)
{
	std::map<LLViewerObject*, LLSelectNode*>::iterator found_it = mSelectNodeMap.find(objectp);
	if (found_it != mSelectNodeMap.end())
	{
		return found_it->second;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// getFirstNode()
//-----------------------------------------------------------------------------
LLSelectNode *LLObjectSelection::getFirstNode()
{
	mCurrentNode = begin();//getFirstData();

	while (mCurrentNode != end() && !(*mCurrentNode)->getObject())
	{
		// The object on this was killed at some point, delete it.
		erase(mCurrentNode++);
	}

	if (mCurrentNode != end())
	{
		return *mCurrentNode;
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// getCurrentNode()
//-----------------------------------------------------------------------------
LLSelectNode *LLObjectSelection::getCurrentNode()
{
	while (mCurrentNode != end() && !(*mCurrentNode)->getObject())
	{
		// The object on this was killed at some point, delete it.
		erase(mCurrentNode++);
	}

	if (mCurrentNode != end())
	{
		return *mCurrentNode;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// getNextNode()
//-----------------------------------------------------------------------------
LLSelectNode *LLObjectSelection::getNextNode()
{
	++mCurrentNode;

	while (mCurrentNode != end() && !(*mCurrentNode)->getObject())
	{
		// The object on this was killed at some point, delete it.
		erase(mCurrentNode++);
	}

	if (mCurrentNode != end())
	{
		return *mCurrentNode;
	}
	return NULL;
}



//-----------------------------------------------------------------------------
// getFirstObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstObject()
{
	mCurrentNode = begin();

	while (mCurrentNode != end() && !(*mCurrentNode)->getObject())
	{
		// The object on this was killed at some point, delete it.
		erase(mCurrentNode++);
	}

	if (mCurrentNode != end())
	{
		return (*mCurrentNode)->getObject();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// getNextObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getNextObject()
{
	++mCurrentNode;// = getNextData();

	while (mCurrentNode != end() && !(*mCurrentNode)->getObject())
	{
		// The object on this was killed at some point, delete it.
		erase(mCurrentNode++);
	}

	if (mCurrentNode != end())
	{
		return (*mCurrentNode)->getObject();
	}

	return NULL;
}



//-----------------------------------------------------------------------------
// getPrimaryTE()
//-----------------------------------------------------------------------------
void LLObjectSelection::getPrimaryTE(LLViewerObject* *object, S32 *te)
{
	// initialize object and te
	*te = 0;
	*object = NULL;

	BOOL searching_roots = TRUE;

	// try for root node first, then first child node
	LLSelectNode *primary_node = getFirstNode(); //getFirstRootNode();
	if (!primary_node)
	{
		primary_node = getFirstNode();
		searching_roots = FALSE;
	}

	while (primary_node)
	{
		S32 last_selected_te = primary_node->getLastSelectedTE();
		if (last_selected_te >= 0)
		{
			*object = primary_node->getObject();
			*te = last_selected_te;
			return;
		}
		for(S32 cur_te = 0; cur_te < primary_node->getObject()->getNumTEs(); cur_te++)
		{
			// if face selected
			if (primary_node->isTESelected(cur_te))
			{
				// return this object and face
				*object = primary_node->getObject();
				*te = cur_te;
				return;
			}
		}
		if (searching_roots)
		{
			primary_node = getNextRootNode();
			if (!primary_node)
			{
				primary_node = getFirstNode();
				searching_roots = FALSE;
			}
		}
		else
		{
			primary_node = getNextNode();
		}
	}
}

//-----------------------------------------------------------------------------
// getFirstTE()
//-----------------------------------------------------------------------------
void LLObjectSelection::getFirstTE(LLViewerObject* *object, S32 *te)
{
	// start with first face
	mCurrentTE = 0;

	LLSelectNode *cur_node = getFirstNode();

	// repeat over all selection nodes
	while (cur_node)
	{
		// skip objects with no faces
		if (cur_node->getObject()->getNumTEs() == 0)
		{
			mCurrentTE = 0;
			cur_node = getNextNode();
			continue;
		}

		// repeat over all faces for this object
		while (mCurrentTE < cur_node->getObject()->getNumTEs())
		{
			// if face selected
			if (cur_node->isTESelected(mCurrentTE))
			{
				// return this object and face
				*object = cur_node->getObject();
				*te = mCurrentTE;
				return;
			}

			mCurrentTE++;
		}

		// Couldn't find a selected face.
		// This can happen if an object's volume parameters are changed in such a way
		// that texture entries are eliminated.
		//
		// TODO: Consider selecting all faces in this case?  Subscribe the selection
		// list to the volume changing code?

		mCurrentTE = 0;
		cur_node = getNextNode();
	}

	// The list doesn't contain any nodes.  Return NULL.
	*object = NULL;
	*te = -1;
	return;
}


//-----------------------------------------------------------------------------
// getNextFace()
//-----------------------------------------------------------------------------
void LLObjectSelection::getNextTE(LLViewerObject* *object, S32 *te)
{
	// try next face
	mCurrentTE++;

	LLSelectNode *cur_node = getCurrentNode();
	// repeat over remaining selection nodes
	while ( cur_node )
	{
		// skip objects with no faces
		if (cur_node->getObject()->getNumTEs() == 0)
		{
			mCurrentTE = 0;
			cur_node = getNextNode();
			continue;
		}

		// repeat over all faces for this object
		// CRO: getNumTEs() no longer equals mFaces.count(), so use mFaces.count() instead
		while ( mCurrentTE < cur_node->getObject()->getNumTEs() )
		{
			// if face selected
			if (cur_node->isTESelected(mCurrentTE))
			{
				// return this object and face
				*object = cur_node->getObject();
				*te = mCurrentTE;
				return;
			}

			mCurrentTE++;
		}

		mCurrentTE = 0;
		cur_node = getNextNode();
	}

	// The list doesn't contain any nodes.  Return NULL.
	*object = NULL;
	*te = -1;
	return;
}

void LLObjectSelection::getCurrentTE(LLViewerObject* *object, S32 *te)
{
	if (mCurrentNode != end())
	{
		*object = (*mCurrentNode)->getObject();
		*te = mCurrentTE;
	}
	else
	{
		*object = NULL;
		*te = -1;
	}
}
//-----------------------------------------------------------------------------
// getFirstRootNode()
//-----------------------------------------------------------------------------
LLSelectNode *LLObjectSelection::getFirstRootNode()
{
	LLSelectNode *cur_node = getFirstNode();

	// scan through child objects and roots set to ignore
	while (cur_node && 
				(!(cur_node->getObject()->isRootEdit() || cur_node->getObject()->isJointChild()) ||
					cur_node->mIndividualSelection))
	{
		cur_node = getNextNode();
	}

	return cur_node;
}


//-----------------------------------------------------------------------------
// getNextRootNode()
//-----------------------------------------------------------------------------
LLSelectNode *LLObjectSelection::getNextRootNode()
{
	LLSelectNode *cur_node = getNextNode();

	while (cur_node && 
				(!(cur_node->getObject()->isRootEdit() || cur_node->getObject()->isJointChild()) ||
					cur_node->mIndividualSelection))
	{
		cur_node = getNextNode();
	}

	return cur_node;
}


//-----------------------------------------------------------------------------
// getFirstRootObject()
//-----------------------------------------------------------------------------
LLViewerObject *LLObjectSelection::getFirstRootObject()
{
	LLSelectNode *node = getFirstRootNode();

	if (node)
	{
		return node->getObject();
	}
	else
	{
		return NULL;
	}
}


//-----------------------------------------------------------------------------
// getNextRootObject()
//-----------------------------------------------------------------------------
LLViewerObject *LLObjectSelection::getNextRootObject()
{
	LLSelectNode *node = getNextRootNode();

	if (node)
	{
		return node->getObject();
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// isEmpty()
//-----------------------------------------------------------------------------
BOOL LLObjectSelection::isEmpty()
{
	return (size() == 0);
}

//-----------------------------------------------------------------------------
// getOwnershipCost()
//-----------------------------------------------------------------------------
BOOL LLObjectSelection::getOwnershipCost(S32 &cost)
{
	S32 count = 0;
	for( LLSelectNode* nodep = getFirstNode(); nodep; nodep = getNextNode() )
	{
		count++;
	}

	cost = count * OWNERSHIP_COST_PER_OBJECT;

	return (count > 0);
}



//-----------------------------------------------------------------------------
// getObjectCount()
//-----------------------------------------------------------------------------
S32 LLObjectSelection::getObjectCount()
{
	return getNumNodes();
}


//-----------------------------------------------------------------------------
// getTECount()
//-----------------------------------------------------------------------------
S32 LLObjectSelection::getTECount()
{
	S32 count = 0;

	LLSelectNode* nodep;
	for (nodep = getFirstNode(); nodep; nodep = getNextNode() )
	{
		if (nodep->getObject())
		{
			S32 num_tes = nodep->getObject()->getNumTEs();
			for (S32 te = 0; te < num_tes; te++)
			{
				if (nodep->isTESelected(te))
				{
					count++;
				}
			}
		}
	}

	return count;
}

//-----------------------------------------------------------------------------
// getRootObjectCount()
//-----------------------------------------------------------------------------
S32 LLObjectSelection::getRootObjectCount()
{
	LLSelectNode *nodep;

	S32 count = 0;
	for(nodep = getFirstRootNode(); nodep; nodep = getNextRootNode())
	{
		++count;
	}
	return count;
}

bool LLObjectSelection::applyToObjects(LLSelectedObjectFunctor* func)
{
	bool result = true;
	LLViewerObject* object;
	for (object = getFirstObject(); object != NULL; object = getNextObject())
	{
		result = result && func->apply(object);
	}
	return result;
}

bool LLObjectSelection::applyToRootObjects(LLSelectedObjectFunctor* func)
{
	bool result = true;
	LLViewerObject* object;
	for (object = getFirstRootObject(); 
		 object != NULL; 
		 object = getNextRootObject())
	{
		result = result && func->apply(object);
	}
	return result;
}

bool LLObjectSelection::applyToNodes(LLSelectedNodeFunctor *func)
{
	bool result = true;
	LLSelectNode* node;
	for (node = getFirstNode(); node != NULL; node = getNextNode())
	{
		result = result && func->apply(node);
	}
	return result;
}

//-----------------------------------------------------------------------------
// contains()
//-----------------------------------------------------------------------------
BOOL LLObjectSelection::contains(LLViewerObject* object)
{
	return findNode(object) != NULL;
}


//-----------------------------------------------------------------------------
// contains()
//-----------------------------------------------------------------------------
BOOL LLObjectSelection::contains(LLViewerObject* object, S32 te)
{
	LLSelectNode *nodep;
	if (te == SELECT_ALL_TES)
	{
		// ...all faces
		for (nodep = getFirstNode(); nodep; nodep = getNextNode() )
		{
			if (nodep->getObject() == object)
			{
				BOOL all_selected = TRUE;
				for (S32 i = 0; i < SELECT_MAX_TES; i++)
				{
					all_selected = all_selected && nodep->isTESelected(i);
				}
				return all_selected;
			}
		}
		return FALSE;
	}
	else
	{
		// ...one face
		for (nodep = getFirstNode(); nodep; nodep = getNextNode() )
		{
			if (nodep->getObject() == object && nodep->isTESelected(te))
			{
				return TRUE;
			}
		}
		return FALSE;
	}
}

// returns TRUE is any node is currenly worn as an attachment
BOOL LLObjectSelection::isAttachment()
{
	return (mSelectType == SELECT_TYPE_ATTACHMENT || mSelectType == SELECT_TYPE_HUD);
}

//-----------------------------------------------------------------------------
// getFirstMoveableNode()
//-----------------------------------------------------------------------------
LLSelectNode* LLObjectSelection::getFirstMoveableNode(BOOL get_root)
{
	LLSelectNode* selectNode = NULL;

	if (get_root)
	{
		for(selectNode = getFirstRootNode(); selectNode; selectNode = getNextRootNode())
		{
			if( selectNode->getObject()->permMove() )
			{
				return selectNode;
				break;
			}
		}
	}
	for(selectNode = getFirstNode(); selectNode; selectNode = getNextNode())
	{
		if( selectNode->getObject()->permMove() )
		{
			return selectNode;
			break;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// getFirstCopyableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstCopyableObject(BOOL get_root)
{
	LLViewerObject* object = NULL;
	for(LLViewerObject* cur = getFirstObject(); cur; cur = getNextObject())
	{
		if( cur->permCopy() && !cur->isAttachment())
		{
			object = cur;
			break;
		}
	}	

	if (get_root && object)
	{
		LLViewerObject *parent;
		while ((parent = (LLViewerObject*)object->getParent()))
		{
			if (parent->isSelected())
			{
				object = parent;
			}
			else
			{
				break;
			}
		}
	}

	return object;
}


//-----------------------------------------------------------------------------
// getFirstDeleteableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstDeleteableObject(BOOL get_root)
{
	//RN: don't currently support deletion of child objects, as that requires separating them first
	// then derezzing to trash
	get_root = TRUE;

	LLViewerObject* object = NULL;
	if (get_root)
	{
		for(LLViewerObject* current = getFirstRootObject();
			current != NULL;
			current = getNextRootObject())
		{
			// you can delete an object if permissions allow it, you are
			// the owner, you are an officer in the group that owns the
			// object, or you are not the owner but it is on land you own
			// or land owned by your group. (whew!)
			if(   (current->permModify()) 
			|| (current->permYouOwner())
			|| (!current->permAnyOwner())			// public
			|| (current->isOverAgentOwnedLand())
			|| (current->isOverGroupOwnedLand())
			)
			{

				if( !current->isAttachment() )
				{
					object = current;
					break;
				}
			}
		}	
	}
	else
	{
		for(LLViewerObject* current = getFirstObject();
			current != NULL;
			current = getNextObject())
		{
			// you can delete an object if permissions allow it, you are
			// the owner, you are an officer in the group that owns the
			// object, or you are not the owner but it is on land you own
			// or land owned by your group. (whew!)
			if(   (current->permModify()) 
			|| (current->permYouOwner())
			|| (!current->permAnyOwner())			// public
			|| (current->isOverAgentOwnedLand())
			|| (current->isOverGroupOwnedLand())
			)
			{
				if( !current->isAttachment() )
				{
					object = current;
					break;
				}
			}
		}	
	}

	return object;
}


//-----------------------------------------------------------------------------
// getFirstEditableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstEditableObject(BOOL get_root)
{
	LLViewerObject* object = NULL;
	for(LLViewerObject* cur = getFirstObject(); cur; cur = getNextObject())
	{
		if( cur->permModify() )
		{
			object = cur;
			break;
		}
	}	

	if (get_root && object)
	{
		LLViewerObject *parent;
		while ((parent = (LLViewerObject*)object->getParent()))
		{
			if (parent->isSelected())
			{
				object = parent;
			}
			else
			{
				break;
			}
		}
	}

	return object;
}

//-----------------------------------------------------------------------------
// getFirstMoveableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstMoveableObject(BOOL get_root)
{
	LLViewerObject* object = NULL;
	for(LLViewerObject* cur = getFirstObject(); cur; cur = getNextObject())
	{
		if( cur->permMove() )
		{
			object = cur;
			break;
		}
	}	

	if (get_root && object && !object->isJointChild())
	{
		LLViewerObject *parent;
		while ((parent = (LLViewerObject*)object->getParent()))
		{
			if (parent->isSelected())
			{
				object = parent;
			}
			else
			{
				break;
			}
		}
	}

	return object;
}
