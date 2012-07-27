/** 
 * @file llselectmgr.cpp
 * @brief A manager for selected objects and faces.
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

#include "llviewerprecompiledheaders.h"

// file include
#define LLSELECTMGR_CPP
#include "llselectmgr.h"

// library includes
#include "llcachename.h"
#include "lldbstrings.h"
#include "lleconomy.h"
#include "llgl.h"
#include "llmediaentry.h"
#include "llrender.h"
#include "llnotifications.h"
#include "llpermissions.h"
#include "llpermissionsflags.h"
#include "lltrans.h"
#include "llundo.h"
#include "lluuid.h"
#include "llvolume.h"
#include "message.h"
#include "object_flags.h"
#include "llquaternion.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewerwindow.h"
#include "lldrawable.h"
#include "llfloaterinspect.h"
#include "llfloaterproperties.h"
#include "llfloaterreporter.h"
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llframetimer.h"
#include "llfocusmgr.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventorymodel.h"
#include "llmenugl.h"
#include "llmeshrepository.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llsidepaneltaskinfo.h"
#include "llslurl.h"
#include "llstatusbar.h"
#include "llsurface.h"
#include "lltool.h"
#include "lltooldraganddrop.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewermedia.h"
#include "llviewermediafocus.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llviewershadermgr.h"

#include "llglheaders.h"

LLViewerObject* getSelectedParentObject(LLViewerObject *object) ;
//
// Consts
//

const S32 NUM_SELECTION_UNDO_ENTRIES = 200;
const F32 SILHOUETTE_UPDATE_THRESHOLD_SQUARED = 0.02f;
const S32 MAX_ACTION_QUEUE_SIZE = 20;
const S32 MAX_SILS_PER_FRAME = 50;
const S32 MAX_OBJECTS_PER_PACKET = 254;
const S32 TE_SELECT_MASK_ALL = 0xFFFFFFFF;

//
// Globals
//

//BOOL gDebugSelectMgr = FALSE;

//BOOL gHideSelectedObjects = FALSE;
//BOOL gAllowSelectAvatar = FALSE;

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

static LLObjectSelection *get_null_object_selection();
template<> 
	const LLSafeHandle<LLObjectSelection>::NullFunc 
		LLSafeHandle<LLObjectSelection>::sNullFunc = get_null_object_selection;

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


static LLPointer<LLObjectSelection> sNullSelection;

//
// Functions
//

void LLSelectMgr::cleanupGlobals()
{
	sNullSelection = NULL;
	LLSelectMgr::getInstance()->clearSelections();
}

LLObjectSelection *get_null_object_selection()
{
	if (sNullSelection.isNull())
	{
		sNullSelection = new LLObjectSelection;
	}
	return sNullSelection;
}

// Build time optimization, generate this function once here
template class LLSelectMgr* LLSingleton<class LLSelectMgr>::getInstance();
//-----------------------------------------------------------------------------
// LLSelectMgr()
//-----------------------------------------------------------------------------
LLSelectMgr::LLSelectMgr()
 : mHideSelectedObjects(LLCachedControl<bool>(gSavedSettings, "HideSelectedObjects", FALSE)),
   mRenderHighlightSelections(LLCachedControl<bool>(gSavedSettings, "RenderHighlightSelections", TRUE)),
   mAllowSelectAvatar( LLCachedControl<bool>(gSavedSettings, "AllowSelectAvatar", FALSE)),
   mDebugSelectMgr(LLCachedControl<bool>(gSavedSettings, "DebugSelectMgr", FALSE))
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

	sSilhouetteParentColor =LLUIColorTable::instance().getColor("SilhouetteParentColor");
	sSilhouetteChildColor = LLUIColorTable::instance().getColor("SilhouetteChildColor");
	sHighlightParentColor = LLUIColorTable::instance().getColor("HighlightParentColor");
	sHighlightChildColor = LLUIColorTable::instance().getColor("HighlightChildColor");
	sHighlightInspectColor = LLUIColorTable::instance().getColor("HighlightInspectColor");
	sContextSilhouetteColor = LLUIColorTable::instance().getColor("ContextSilhouetteColor")*0.5f;

	sRenderLightRadius = gSavedSettings.getBOOL("RenderLightRadius");
	
	mRenderSilhouettes = TRUE;

	mGridMode = GRID_MODE_WORLD;
	gSavedSettings.setS32("GridMode", (S32)GRID_MODE_WORLD);
	mGridValid = FALSE;

	mSelectedObjects = new LLObjectSelection();
	mHoverObjects = new LLObjectSelection();
	mHighlightedObjects = new LLObjectSelection();

	mForceSelection = FALSE;
	mShowSelection = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLSelectMgr()
//-----------------------------------------------------------------------------
LLSelectMgr::~LLSelectMgr()
{
	clearSelections();
}

void LLSelectMgr::clearSelections()
{
	mHoverObjects->deleteAllNodes();
	mSelectedObjects->deleteAllNodes();
	mHighlightedObjects->deleteAllNodes();
	mRectSelectedObjects.clear();
	mGridObjects.deleteAllNodes();
}

void LLSelectMgr::update()
{
	mSelectedObjects->cleanupNodes();
}

void LLSelectMgr::updateEffects()
{
	//keep reference grid objects active
	struct f : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			LLDrawable* drawable = object->mDrawable;
			if (drawable)
			{
				gPipeline.markMoved(drawable);
			}
			return true;
		}
	} func;
	mGridObjects.applyToObjects(&func);

	if (mEffectsTimer.getElapsedTimeF32() > 1.f)
	{
		mSelectedObjects->updateEffects();
		mEffectsTimer.reset();
	}
}

void LLSelectMgr::overrideObjectUpdates()
{
	//override any position updates from simulator on objects being edited
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* selectNode)
		{
			LLViewerObject* object = selectNode->getObject();
			if (object && object->permMove())
			{
				if (!selectNode->mLastPositionLocal.isExactlyZero())
				{
					object->setPosition(selectNode->mLastPositionLocal);
				}
				if (selectNode->mLastRotation != LLQuaternion())
				{
					object->setRotation(selectNode->mLastRotation);
				}
				if (!selectNode->mLastScale.isExactlyZero())
				{
					object->setScale(selectNode->mLastScale);
				}
			}
			return true;
		}
	} func;
	getSelection()->applyToNodes(&func);
}

//-----------------------------------------------------------------------------
// Select just the object, not any other group members.
//-----------------------------------------------------------------------------
LLObjectSelectionHandle LLSelectMgr::selectObjectOnly(LLViewerObject* object, S32 face)
{
	llassert( object );

	//remember primary object
	mSelectedObjects->mPrimaryObject = object;

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

	//remember primary object
	mSelectedObjects->mPrimaryObject = obj;

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
	std::vector<LLViewerObject*> objects;

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
	if (gSavedSettings.getBOOL("EditLinkedParts"))
	{
		gSavedSettings.setBOOL("EditLinkedParts", FALSE);
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
LLObjectSelectionHandle LLSelectMgr::selectObjectAndFamily(const std::vector<LLViewerObject*>& object_list,
														   BOOL send_to_sim)
{
	// Collect all of the objects, children included
	std::vector<LLViewerObject*> objects;

	//clear primary object (no primary object)
	mSelectedObjects->mPrimaryObject = NULL;

	if (object_list.size() < 1)
	{
		return NULL;
	}
	
	// NOTE -- we add the objects in REVERSE ORDER 
	// to preserve the order in the mSelectedObjects list
	for (std::vector<LLViewerObject*>::const_reverse_iterator riter = object_list.rbegin();
		 riter != object_list.rend(); ++riter)
	{
		LLViewerObject *object = *riter;

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
	if (gSavedSettings.getBOOL("EditLinkedParts"))
	{		
		gSavedSettings.setBOOL("EditLinkedParts", FALSE);
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

	tool = LLToolMgr::getInstance()->getCurrentTool();

	// It's possible that the tool is editing an object that is not selected
	LLViewerObject* tool_editing_object = tool->getEditingObject();
	if( tool_editing_object && tool_editing_object->mID == id)
	{
		tool->stopEditing();
		object_found = TRUE;
	}

	// Iterate through selected objects list and kill the object
	if( !object_found )
	{
		for (LLObjectSelection::iterator iter = getSelection()->begin();
			 iter != getSelection()->end(); )
		{
			LLObjectSelection::iterator curiter = iter++;
			LLViewerObject* object = (*curiter)->getObject();
			if (object->mID == id)
			{
				if (tool)
				{
					tool->stopEditing();
				}

				// lose the selection, don't tell simulator, it knows
				deselectObjectAndFamily(object, FALSE);
				object_found = TRUE;
				break; // must break here, may have removed multiple objects from list
			}
			else if (object->isAvatar() && object->getParent() && ((LLViewerObject*)object->getParent())->mID == id)
			{
				// It's possible the item being removed has an avatar sitting on it
				// So remove the avatar that is sitting on the object.
				deselectObjectAndFamily(object, FALSE);
				break; // must break here, may have removed multiple objects from list
			}
		}
	}

	return object_found;
}

bool LLSelectMgr::linkObjects()
{
	if (!LLSelectMgr::getInstance()->selectGetAllRootsValid())
	{
		LLNotificationsUtil::add("UnableToLinkWhileDownloading");
		return true;
	}

	S32 object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
	if (object_count > MAX_CHILDREN_PER_TASK + 1)
	{
		LLSD args;
		args["COUNT"] = llformat("%d", object_count);
		int max = MAX_CHILDREN_PER_TASK+1;
		args["MAX"] = llformat("%d", max);
		LLNotificationsUtil::add("UnableToLinkObjects", args);
		return true;
	}

	if (LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() < 2)
	{
		LLNotificationsUtil::add("CannotLinkIncompleteSet");
		return true;
	}

	if (!LLSelectMgr::getInstance()->selectGetRootsModify())
	{
		LLNotificationsUtil::add("CannotLinkModify");
		return true;
	}

	LLUUID owner_id;
	std::string owner_name;
	if (!LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name))
	{
		// we don't actually care if you're the owner, but novices are
		// the most likely to be stumped by this one, so offer the
		// easiest and most likely solution.
		LLNotificationsUtil::add("CannotLinkDifferentOwners");
		return true;
	}

	LLSelectMgr::getInstance()->sendLink();

	return true;
}

bool LLSelectMgr::unlinkObjects()
{
	LLSelectMgr::getInstance()->sendDelink();
	return true;
}

// in order to link, all objects must have the same owner, and the
// agent must have the ability to modify all of the objects. However,
// we're not answering that question with this method. The question
// we're answering is: does the user have a reasonable expectation
// that a link operation should work? If so, return true, false
// otherwise. this allows the handle_link method to more finely check
// the selection and give an error message when the uer has a
// reasonable expectation for the link to work, but it will fail.
bool LLSelectMgr::enableLinkObjects()
{
	bool new_value = false;
	// check if there are at least 2 objects selected, and that the
	// user can modify at least one of the selected objects.

	// in component mode, can't link
	if (!gSavedSettings.getBOOL("EditLinkedParts"))
	{
		if(LLSelectMgr::getInstance()->selectGetAllRootsValid() && LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() >= 2)
		{
			struct f : public LLSelectedObjectFunctor
			{
				virtual bool apply(LLViewerObject* object)
				{
					return object->permModify();
				}
			} func;
			const bool firstonly = true;
			new_value = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
		}
	}
	return new_value;
}

bool LLSelectMgr::enableUnlinkObjects()
{
	LLViewerObject* first_editable_object = LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject();

	bool new_value = LLSelectMgr::getInstance()->selectGetAllRootsValid() &&
		first_editable_object &&
		!first_editable_object->isAttachment();

	return new_value;
}

void LLSelectMgr::deselectObjectAndFamily(LLViewerObject* object, BOOL send_to_sim, BOOL include_entire_object)
{
	// bail if nothing selected or if object wasn't selected in the first place
	if(!object) return;
	if(!object->isSelected()) return;

	// Collect all of the objects, and remove them
	std::vector<LLViewerObject*> objects;

	if (include_entire_object)
	{
		// Since we're selecting a family, start at the root, but
		// don't include an avatar.
		LLViewerObject* root = object;
	
		while(!root->isAvatar() && root->getParent() && !root->isJointChild())
		{
			LLViewerObject* parent = (LLViewerObject*)root->getParent();
			if (parent->isAvatar())
			{
				break;
			}
			root = parent;
		}
	
		object = root;
	}
	else
	{
		object = (LLViewerObject*)object->getRoot();
	}

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
	for (U32 i = 0; i < objects.size(); i++)
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

		// Zap the angular velocity, as the sim will set it to zero
		objects[i]->setAngularVelocity( 0,0,0 );
		objects[i]->setVelocity( 0,0,0 );

		if(msg->isSendFull(NULL) || select_count >= MAX_OBJECTS_PER_PACKET)
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

	// Zap the angular velocity, as the sim will set it to zero
	object->setAngularVelocity( 0,0,0 );
	object->setVelocity( 0,0,0 );

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

void LLSelectMgr::addAsFamily(std::vector<LLViewerObject*>& objects, BOOL add_to_end)
{
	for (std::vector<LLViewerObject*>::iterator iter = objects.begin();
		 iter != objects.end(); ++iter)
	{
		LLViewerObject* objectp = *iter;
		
		// Can't select yourself
		if (objectp->mID == gAgentID
			&& !LLSelectMgr::getInstance()->mAllowSelectAvatar)
		{
			continue;
		}

		if (!objectp->isSelected())
		{
			LLSelectNode *nodep = new LLSelectNode(objectp, TRUE);
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
		llassert_always(nodep->getObject());
	}
	else
	{
		// make this a full-fledged selection
		nodep->setTransient(FALSE);
		// Move it to the front of the list
		mSelectedObjects->moveNodeToFront(nodep);
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


LLObjectSelectionHandle LLSelectMgr::setHoverObject(LLViewerObject *objectp, S32 face)
{
	if (!objectp)
	{
		mHoverObjects->deleteAllNodes();
		return NULL;
	}

	// Can't select yourself
	if (objectp->mID == gAgentID)
	{
		mHoverObjects->deleteAllNodes();
		return NULL;
	}

	// Can't select land
	if (objectp->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH)
	{
		mHoverObjects->deleteAllNodes();
		return NULL;
	}

	mHoverObjects->mPrimaryObject = objectp; 

	objectp = objectp->getRootEdit();

	// is the requested object the same as the existing hover object root?
	// NOTE: there is only ever one linked set in mHoverObjects
	if (mHoverObjects->getFirstRootObject() != objectp) 
	{

		// Collect all of the objects
		std::vector<LLViewerObject*> objects;
		objectp = objectp->getRootEdit();
		objectp->addThisAndNonJointChildren(objects);

		mHoverObjects->deleteAllNodes();
		for (std::vector<LLViewerObject*>::iterator iter = objects.begin();
			 iter != objects.end(); ++iter)
		{
			LLViewerObject* cur_objectp = *iter;
			LLSelectNode* nodep = new LLSelectNode(cur_objectp, FALSE);
			nodep->selectTE(face, TRUE);
			mHoverObjects->addNodeAtEnd(nodep);
		}

		requestObjectPropertiesFamily(objectp);
	}

	return mHoverObjects;
}

LLSelectNode *LLSelectMgr::getHoverNode()
{
	return mHoverObjects->getFirstRootNode();
}

LLSelectNode *LLSelectMgr::getPrimaryHoverNode()
{
	return mHoverObjects->mSelectNodeMap[mHoverObjects->mPrimaryObject];
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
	
	if ((gSavedSettings.getBOOL("SelectOwnedOnly") && !objectp->permYouOwner()) 
		|| (gSavedSettings.getBOOL("SelectMovableOnly") && !objectp->permMove()))
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

	LLViewerObject::const_child_list_t& child_list = root_obj->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;
		highlightObjectOnly(child);
	}
}

// Note that this ignores the "select owned only" flag
// It's also more efficient than calling the single-object version over and over.
void LLSelectMgr::highlightObjectAndFamily(const std::vector<LLViewerObject*>& objects)
{
	for (std::vector<LLViewerObject*>::const_iterator iter1 = objects.begin();
		 iter1 != objects.end(); ++iter1)
	{
		LLViewerObject* object = *iter1;

		if (!object)
		{
			continue;
		}
		if (object->getPCode() != LL_PCODE_VOLUME)
		{
			continue;
		}

		LLViewerObject* root = (LLViewerObject*)object->getRoot();
		mRectSelectedObjects.insert(root);

		LLViewerObject::const_child_list_t& child_list = root->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter2 = child_list.begin();
			 iter2 != child_list.end(); iter2++)
		{
			LLViewerObject* child = *iter2;
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

	LLViewerObject::const_child_list_t& child_list = root_obj->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;
		unhighlightObjectOnly(child);
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

	//clear primary object
	mSelectedObjects->mPrimaryObject = NULL;

	for (LLObjectSelection::iterator iter = getHighlightedObjects()->begin();
		 iter != getHighlightedObjects()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
	
		LLSelectNode *nodep = *curiter;
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
	BOOL select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
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

	LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;
		nodep = new LLSelectNode(child, FALSE);
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
	mGridObjects.cleanupNodes();
	
	LLViewerObject* first_grid_object = mGridObjects.getFirstObject();

	if (mGridMode == GRID_MODE_LOCAL && mSelectedObjects->getObjectCount())
	{
		//LLViewerObject* root = getSelectedParentObject(mSelectedObjects->getFirstObject());
		LLBBox bbox = mSavedSelectionBBox;
		mGridOrigin = mSavedSelectionBBox.getCenterAgent();
		mGridScale = mSavedSelectionBBox.getExtentLocal() * 0.5f;

		// DEV-12570 Just taking the saved selection box rotation prevents
		// wild rotations of linked sets while in local grid mode
		//if(mSelectedObjects->getObjectCount() < 2 || !root || root->mDrawable.isNull())
		{
			mGridRotation = mSavedSelectionBBox.getRotation();
		}
		/*else //set to the root object
		{
			mGridRotation = root->getRenderRotation();			
		}*/
	}
	else if (mGridMode == GRID_MODE_REF_OBJECT && first_grid_object && first_grid_object->mDrawable.notNull())
	{
		mGridRotation = first_grid_object->getRenderRotation();
		LLVector3 first_grid_obj_pos = first_grid_object->getRenderPosition();

		LLVector4a min_extents(F32_MAX);
		LLVector4a max_extents(-F32_MAX);
		BOOL grid_changed = FALSE;
		for (LLObjectSelection::iterator iter = mGridObjects.begin();
			 iter != mGridObjects.end(); ++iter)
		{
			LLViewerObject* object = (*iter)->getObject();
			LLDrawable* drawable = object->mDrawable;
			if (drawable)
			{
				const LLVector4a* ext = drawable->getSpatialExtents();
				update_min_max(min_extents, max_extents, ext[0]);
				update_min_max(min_extents, max_extents, ext[1]);
				grid_changed = TRUE;
			}
		}
		if (grid_changed)
		{
			LLVector4a center, size;
			center.setAdd(min_extents, max_extents);
			center.mul(0.5f);
			size.setSub(max_extents, min_extents);
			size.mul(0.5f);

			mGridOrigin.set(center.getF32ptr());
			LLDrawable* drawable = first_grid_object->mDrawable;
			if (drawable && drawable->isActive())
			{
				mGridOrigin = mGridOrigin * first_grid_object->getRenderMatrix();
			}
			mGridScale.set(size.getF32ptr());
		}
	}
	else // GRID_MODE_WORLD or just plain default
	{
		const BOOL non_root_ok = TRUE;
		LLViewerObject* first_object = mSelectedObjects->getFirstRootObject(non_root_ok);

		mGridOrigin.clearVec();
		mGridRotation.loadIdentity();

		mSelectedObjects->mSelectType = getSelectTypeForObject( first_object );

		switch (mSelectedObjects->mSelectType)
		{
		case SELECT_TYPE_ATTACHMENT:
			if (first_object && first_object->getRootEdit()->mDrawable.notNull())
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

void LLSelectMgr::remove(std::vector<LLViewerObject*>& objects)
{
	for (std::vector<LLViewerObject*>::iterator iter = objects.begin();
		 iter != objects.end(); ++iter)
	{
		LLViewerObject* objectp = *iter;
		LLSelectNode* nodep = mSelectedObjects->findNode(objectp);
		if (nodep)
		{
			objectp->setSelected(FALSE);
			mSelectedObjects->removeNode(nodep);
			nodep = NULL;
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
	// get object node (and verify it is in the selected list)
	LLSelectNode *nodep = mSelectedObjects->findNode(objectp);
	if (!nodep)
	{
		return;
	}

	// if face = all, remove object from list
	if ((objectp->getNumTEs() <= 0) || (te == SELECT_ALL_TES))
	{
		// Remove all faces (or the object doesn't have faces) so remove the node
		mSelectedObjects->removeNode(nodep);
		nodep = NULL;
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
			nodep = NULL;
			objectp->setSelected( FALSE );
			// *FIXME: Doesn't update simulator that object is no longer selected
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
	for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
		 iter != mSelectedObjects->end(); iter++ )
	{
		LLViewerObject *objectp = (*iter)->getObject();
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

	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLSelectNode* nodep = *curiter;
		LLViewerObject* object = nodep->getObject();

		if (nodep->mIndividualSelection)
		{
			selection_changed = TRUE;
		}

		LLViewerObject* parentp = object;
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
	std::vector<LLViewerObject*> objects;

	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLViewerObject* object = (*iter)->getObject();
		object->addThisAndNonJointChildren(objects);
	}

	if (!objects.empty())
	{
		deselectAll();
		for (std::vector<LLViewerObject*>::iterator iter = objects.begin();
			 iter != objects.end(); ++iter)
		{
			LLViewerObject* objectp = *iter;
			selectObjectOnly(objectp);
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

	S32 count = 0;
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLViewerObject* objectp = (*iter)->getObject();
		llinfos << "Object " << count << " type " << LLPrimitive::pCodeToString(objectp->getPCode()) << llendl;
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
		count++;
	}

	// Face iterator
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* objectp = node->getObject();
		if (!objectp)
			continue;
		for (S32 te = 0; te < objectp->getNumTEs(); ++te )
		{
			if (node->isTESelected(te))
			{
				llinfos << "Object " << objectp << " te " << te << llendl;
			}
		}
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

struct LLSelectMgrSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		if (object->permModify())
		{
			object->sendTEUpdate();
		}
		return true;
	}
};

void LLObjectSelection::applyNoCopyTextureToTEs(LLViewerInventoryItem* item)
{
	if (!item)
	{
		return;
	}
	LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(item->getAssetUUID());

	for (iterator iter = begin(); iter != end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = (*iter)->getObject();
		if (!object)
		{
			continue;
		}

		S32 num_tes = llmin((S32)object->getNumTEs(), (S32)object->getNumFaces());
		bool texture_copied = false;
		for (S32 te = 0; te < num_tes; ++te)
		{
			if (node->isTESelected(te))
			{
				//(no-copy) textures must be moved to the object's inventory only once
				// without making any copies
				if (!texture_copied)
				{
					LLToolDragAndDrop::handleDropTextureProtections(object, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
					texture_copied = true;
				}

				// apply texture for the selected faces
				LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
				object->setTEImage(te, image);
				dialog_refresh_all();

				// send the update to the simulator
				object->sendTEUpdate();
			}
		}
	}
}

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

	struct f : public LLSelectedTEFunctor
	{
		LLViewerInventoryItem* mItem;
		LLUUID mImageID;
		f(LLViewerInventoryItem* item, const LLUUID& id) : mItem(item), mImageID(id) {}
		bool apply(LLViewerObject* objectp, S32 te)
		{
			if (mItem)
			{
				if (te == -1) // all faces
				{
					LLToolDragAndDrop::dropTextureAllFaces(objectp,
														   mItem,
														   LLToolDragAndDrop::SOURCE_AGENT,
														   LLUUID::null);
				}
				else // one face
				{
					LLToolDragAndDrop::dropTextureOneFace(objectp,
														  te,
														  mItem,
														  LLToolDragAndDrop::SOURCE_AGENT,
														  LLUUID::null);
				}
			}
			else // not an inventory item
			{
				// Texture picker defaults aren't inventory items
				// * Don't need to worry about permissions for them
				// * Can just apply the texture and be done with it.
				objectp->setTEImage(te, LLViewerTextureManager::getFetchedTexture(mImageID, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
			}
			return true;
		}
	};

	if (item && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
	{
		getSelection()->applyNoCopyTextureToTEs(item);
	}
	else
	{
		f setfunc(item, imageid);
		getSelection()->applyToTEs(&setfunc);
	}


	struct g : public LLSelectedObjectFunctor
	{
		LLViewerInventoryItem* mItem;
		g(LLViewerInventoryItem* item) : mItem(item) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (!mItem)
			{
				object->sendTEUpdate();
				// 1 particle effect per object				
				LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
				effectp->setSourceObject(gAgentAvatarp);
				effectp->setTargetObject(object);
				effectp->setDuration(LL_HUD_DUR_SHORT);
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			}
			return true;
		}
	} sendfunc(item);
	getSelection()->applyToObjects(&sendfunc);
}

//-----------------------------------------------------------------------------
// selectionSetColor()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetColor(const LLColor4 &color)
{
	struct f : public LLSelectedTEFunctor
	{
		LLColor4 mColor;
		f(const LLColor4& c) : mColor(c) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				object->setTEColor(te, mColor);
			}
			return true;
		}
	} setfunc(color);
	getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

//-----------------------------------------------------------------------------
// selectionSetColorOnly()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetColorOnly(const LLColor4 &color)
{
	struct f : public LLSelectedTEFunctor
	{
		LLColor4 mColor;
		f(const LLColor4& c) : mColor(c) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				LLColor4 prev_color = object->getTE(te)->getColor();
				mColor.mV[VALPHA] = prev_color.mV[VALPHA];
				// update viewer side color in anticipation of update from simulator
				object->setTEColor(te, mColor);
			}
			return true;
		}
	} setfunc(color);
	getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

//-----------------------------------------------------------------------------
// selectionSetAlphaOnly()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetAlphaOnly(const F32 alpha)
{
	struct f : public LLSelectedTEFunctor
	{
		F32 mAlpha;
		f(const F32& a) : mAlpha(a) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				LLColor4 prev_color = object->getTE(te)->getColor();
				prev_color.mV[VALPHA] = mAlpha;
				// update viewer side color in anticipation of update from simulator
				object->setTEColor(te, prev_color);
			}
			return true;
		}
	} setfunc(alpha);
	getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionRevertColors()
{
	struct f : public LLSelectedTEFunctor
	{
		LLObjectSelectionHandle mSelectedObjects;
		f(LLObjectSelectionHandle sel) : mSelectedObjects(sel) {}
		bool apply(LLViewerObject* object, S32 te)
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
			return true;
		}
	} setfunc(mSelectedObjects);
	getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

BOOL LLSelectMgr::selectionRevertTextures()
{
	struct f : public LLSelectedTEFunctor
	{
		LLObjectSelectionHandle mSelectedObjects;
		f(LLObjectSelectionHandle sel) : mSelectedObjects(sel) {}
		bool apply(LLViewerObject* object, S32 te)
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
						return FALSE;
					}
					else
					{
						object->setTEImage(te, LLViewerTextureManager::getFetchedTexture(id, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
					}
				}
			}
			return true;
		}
	} setfunc(mSelectedObjects);
	BOOL revert_successful = getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);

	return revert_successful;
}

void LLSelectMgr::selectionSetBumpmap(U8 bumpmap)
{
	struct f : public LLSelectedTEFunctor
	{
		U8 mBump;
		f(const U8& b) : mBump(b) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				// update viewer side color in anticipation of update from simulator
				object->setTEBumpmap(te, mBump);
			}
			return true;
		}
	} setfunc(bumpmap);
	getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetTexGen(U8 texgen)
{
	struct f : public LLSelectedTEFunctor
	{
		U8 mTexgen;
		f(const U8& t) : mTexgen(t) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				// update viewer side color in anticipation of update from simulator
				object->setTETexGen(te, mTexgen);
			}
			return true;
		}
	} setfunc(texgen);
	getSelection()->applyToTEs(&setfunc);

	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}


void LLSelectMgr::selectionSetShiny(U8 shiny)
{
	struct f : public LLSelectedTEFunctor
	{
		U8 mShiny;
		f(const U8& t) : mShiny(t) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				// update viewer side color in anticipation of update from simulator
				object->setTEShiny(te, mShiny);
			}
			return true;
		}
	} setfunc(shiny);
	getSelection()->applyToTEs(&setfunc);

	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetFullbright(U8 fullbright)
{
	struct f : public LLSelectedTEFunctor
	{
		U8 mFullbright;
		f(const U8& t) : mFullbright(t) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				// update viewer side color in anticipation of update from simulator
				object->setTEFullbright(te, mFullbright);
			}
			return true;
		}
	} setfunc(fullbright);
	getSelection()->applyToTEs(&setfunc);

	struct g : public LLSelectedObjectFunctor
	{
		U8 mFullbright;
		g(const U8& t) : mFullbright(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->sendTEUpdate();
				if (mFullbright)
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
			return true;
		}
	} sendfunc(fullbright);
	getSelection()->applyToObjects(&sendfunc);
}

// This function expects media_data to be a map containing relevant
// media data name/value pairs (e.g. home_url, etc.)
void LLSelectMgr::selectionSetMedia(U8 media_type, const LLSD &media_data)
{	
	struct f : public LLSelectedTEFunctor
	{
		U8 mMediaFlags;
		const LLSD &mMediaData;
		f(const U8& t, const LLSD& d) : mMediaFlags(t), mMediaData(d) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			if (object->permModify())
			{
				// If we are adding media, then check the current state of the
				// media data on this face.  
				//  - If it does not have media, AND we are NOT setting the HOME URL, then do NOT add media to this
				// face.
				//  - If it does not have media, and we ARE setting the HOME URL, add media to this face.
				//  - If it does already have media, add/update media to/on this face
				// If we are removing media, just do it (ignore the passed-in LLSD).
				if (mMediaFlags & LLTextureEntry::MF_HAS_MEDIA)
				{
					llassert(mMediaData.isMap());
					const LLTextureEntry *texture_entry = object->getTE(te);
					if (!mMediaData.isMap() ||
						(NULL != texture_entry) && !texture_entry->hasMedia() && !mMediaData.has(LLMediaEntry::HOME_URL_KEY))
					{
						// skip adding/updating media
					}
					else {
						// Add/update media
						object->setTEMediaFlags(te, mMediaFlags);
						LLVOVolume *vo = dynamic_cast<LLVOVolume*>(object);
						llassert(NULL != vo);
						if (NULL != vo) 
						{
							vo->syncMediaData(te, mMediaData, true/*merge*/, true/*ignore_agent*/);
						}
					}
				}
				else
				{
					// delete media (or just set the flags)
					object->setTEMediaFlags(te, mMediaFlags);
				}
			}
			return true;
		}
	} setfunc(media_type, media_data);
	getSelection()->applyToTEs(&setfunc);
	
	struct f2 : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->sendTEUpdate();
				LLVOVolume *vo = dynamic_cast<LLVOVolume*>(object);
				llassert(NULL != vo);
				// It's okay to skip this object if hasMedia() is false...
				// the sendTEUpdate() above would remove all media data if it were
				// there.
                if (NULL != vo && vo->hasMedia())
                {
                    // Send updated media data FOR THE ENTIRE OBJECT
                    vo->sendMediaDataUpdate();
                }
			}
			return true;
		}
	} func2;
	mSelectedObjects->applyToObjects( &func2 );
}

void LLSelectMgr::selectionSetGlow(F32 glow)
{
	struct f1 : public LLSelectedTEFunctor
	{
		F32 mGlow;
		f1(F32 glow) : mGlow(glow) {};
		bool apply(LLViewerObject* object, S32 face)
		{
			if (object->permModify())
			{
				// update viewer side color in anticipation of update from simulator
				object->setTEGlow(face, mGlow);
			}
			return true;
		}
	} func1(glow);
	mSelectedObjects->applyToTEs( &func1 );

	struct f2 : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->sendTEUpdate();
			}
			return true;
		}
	} func2;
	mSelectedObjects->applyToObjects( &func2 );
}


//-----------------------------------------------------------------------------
// findObjectPermissions()
//-----------------------------------------------------------------------------
LLPermissions* LLSelectMgr::findObjectPermissions(const LLViewerObject* object)
{
	for (LLObjectSelection::valid_iterator iter = getSelection()->valid_begin();
		 iter != getSelection()->valid_end(); iter++ )
	{
		LLSelectNode* nodep = *iter;
		if (nodep->getObject() == object)
		{
			return nodep->mPermissions;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// selectionGetGlow()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectionGetGlow(F32 *glow)
{
	BOOL identical;
	F32 lglow = 0.f;
	struct f1 : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			return object->getTE(face)->getGlow();
		}
	} func;
	identical = mSelectedObjects->getSelectedTEValue( &func, lglow );

	*glow = lglow;
	return identical;
}


void LLSelectMgr::selectionSetPhysicsType(U8 type)
{
	struct f : public LLSelectedObjectFunctor
	{
		U8 mType;
		f(const U8& t) : mType(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->setPhysicsShapeType(mType);
				object->updateFlags(TRUE);
			}
			return true;
		}
	} sendfunc(type);
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetFriction(F32 friction)
{
	struct f : public LLSelectedObjectFunctor
	{
		F32 mFriction;
		f(const F32& friction) : mFriction(friction) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->setPhysicsFriction(mFriction);
				object->updateFlags(TRUE);
			}
			return true;
		}
	} sendfunc(friction);
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetGravity(F32 gravity )
{
	struct f : public LLSelectedObjectFunctor
	{
		F32 mGravity;
		f(const F32& gravity) : mGravity(gravity) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->setPhysicsGravity(mGravity);
				object->updateFlags(TRUE);
			}
			return true;
		}
	} sendfunc(gravity);
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetDensity(F32 density )
{
	struct f : public LLSelectedObjectFunctor
	{
		F32 mDensity;
		f(const F32& density ) : mDensity(density) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->setPhysicsDensity(mDensity);
				object->updateFlags(TRUE);
			}
			return true;
		}
	} sendfunc(density);
	getSelection()->applyToObjects(&sendfunc);
}

void LLSelectMgr::selectionSetRestitution(F32 restitution)
{
	struct f : public LLSelectedObjectFunctor
	{
		F32 mRestitution;
		f(const F32& restitution ) : mRestitution(restitution) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				object->setPhysicsRestitution(mRestitution);
				object->updateFlags(TRUE);
			}
			return true;
		}
	} sendfunc(restitution);
	getSelection()->applyToObjects(&sendfunc);
}


//-----------------------------------------------------------------------------
// selectionSetMaterial()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionSetMaterial(U8 material)
{
	struct f : public LLSelectedObjectFunctor
	{
		U8 mMaterial;
		f(const U8& t) : mMaterial(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				U8 cur_material = object->getMaterial();
				U8 material = mMaterial | (cur_material & ~LL_MCODE_MASK);
				object->setMaterial(material);
				object->sendMaterialUpdate();
			}
			return true;
		}
	} sendfunc(material);
	getSelection()->applyToObjects(&sendfunc);
}

// TRUE if all selected objects have this PCode
BOOL LLSelectMgr::selectionAllPCode(LLPCode code)
{
	struct f : public LLSelectedObjectFunctor
	{
		LLPCode mCode;
		f(const LLPCode& t) : mCode(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->getPCode() != mCode)
			{
				return FALSE;
			}
			return true;
		}
	} func(code);
	BOOL res = getSelection()->applyToObjects(&func);
	return res;
}

bool LLSelectMgr::selectionGetIncludeInSearch(bool* include_in_search_out)
{
	LLViewerObject *object = mSelectedObjects->getFirstRootObject();
	if (!object) return FALSE;

	bool include_in_search = object->getIncludeInSearch();

	bool identical = true;

	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLViewerObject* object = (*iter)->getObject();

		if ( include_in_search != object->getIncludeInSearch())
		{
			identical = false;
			break;
		}
	}

	*include_in_search_out = include_in_search;
	return identical;
}

void LLSelectMgr::selectionSetIncludeInSearch(bool include_in_search)
{
	LLViewerObject* object = NULL;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		object = (*iter)->getObject();
		object->setIncludeInSearch(include_in_search);
	}
	sendListToRegions(
		"ObjectIncludeInSearch",
		packAgentAndSessionID,
		packObjectIncludeInSearch, 
		&include_in_search,
		SEND_ONLY_ROOTS);
}

BOOL LLSelectMgr::selectionGetClickAction(U8 *out_action)
{
	LLViewerObject *object = mSelectedObjects->getFirstObject();
	if (!object)
	{
		return FALSE;
	}
	
	U8 action = object->getClickAction();
	*out_action = action;

	struct f : public LLSelectedObjectFunctor
	{
		U8 mAction;
		f(const U8& t) : mAction(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if ( mAction != object->getClickAction())
			{
				return false;
			}
			return true;
		}
	} func(action);
	BOOL res = getSelection()->applyToObjects(&func);
	return res;
}

void LLSelectMgr::selectionSetClickAction(U8 action)
{
	struct f : public LLSelectedObjectFunctor
	{
		U8 mAction;
		f(const U8& t) : mAction(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			object->setClickAction(mAction);
			return true;
		}
	} func(action);
	getSelection()->applyToObjects(&func);

	sendListToRegions("ObjectClickAction",
					  packAgentAndSessionID,
					  packObjectClickAction, 
					  &action,
					  SEND_INDIVIDUALS);
}


//-----------------------------------------------------------------------------
// godlike requests
//-----------------------------------------------------------------------------

typedef std::pair<const std::string, const std::string> godlike_request_t;

void LLSelectMgr::sendGodlikeRequest(const std::string& request, const std::string& param)
{
	// If the agent is neither godlike nor an estate owner, the server
	// will reject the request.
	std::string message_type;
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
	msg->addString("Method", data->first);
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
	std::string buf = llformat("%u", node->getObject()->getLocalID());
	gMessageSystem->nextBlock("ParamList");
	gMessageSystem->addString("Parameter", buf);
}

//-----------------------------------------------------------------------------
// Rotation options
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionResetRotation()
{
	struct f : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			LLQuaternion identity(0.f, 0.f, 0.f, 1.f);
			object->setRotation(identity);
			if (object->mDrawable.notNull())
			{
				gPipeline.markMoved(object->mDrawable, TRUE);
			}
			object->sendRotationUpdate();
			return true;
		}
	} func;
	getSelection()->applyToRootObjects(&func);
}

void LLSelectMgr::selectionRotateAroundZ(F32 degrees)
{
	LLQuaternion rot( degrees * DEG_TO_RAD, LLVector3(0,0,1) );
	struct f : public LLSelectedObjectFunctor
	{
		LLQuaternion mRot;
		f(const LLQuaternion& rot) : mRot(rot) {}
		virtual bool apply(LLViewerObject* object)
		{
			object->setRotation( object->getRotationEdit() * mRot );
			if (object->mDrawable.notNull())
			{
				gPipeline.markMoved(object->mDrawable, TRUE);
			}
			object->sendRotationUpdate();
			return true;
		}
	} func(rot);
	getSelection()->applyToRootObjects(&func);	
}


//-----------------------------------------------------------------------------
// selectionTexScaleAutofit()
//-----------------------------------------------------------------------------
void LLSelectMgr::selectionTexScaleAutofit(F32 repeats_per_meter)
{
	struct f : public LLSelectedTEFunctor
	{
		F32 mRepeatsPerMeter;
		f(const F32& t) : mRepeatsPerMeter(t) {}
		bool apply(LLViewerObject* object, S32 te)
		{
			
			if (object->permModify())
			{
				// Compute S,T to axis mapping
				U32 s_axis, t_axis;
				if (!LLPrimitive::getTESTAxes(te, &s_axis, &t_axis))
				{
					return TRUE;
				}

				F32 new_s = object->getScale().mV[s_axis] * mRepeatsPerMeter;
				F32 new_t = object->getScale().mV[t_axis] * mRepeatsPerMeter;

				object->setTEScale(te, new_s, new_t);
			}
			return true;
		}
	} setfunc(repeats_per_meter);
	getSelection()->applyToTEs(&setfunc);

	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}



// Called at the end of a scale operation, this adjusts the textures to attempt to
// maintain a constant repeats per meter.
// BUG: Only works for flex boxes.
//-----------------------------------------------------------------------------
// adjustTexturesByScale()
//-----------------------------------------------------------------------------
void LLSelectMgr::adjustTexturesByScale(BOOL send_to_sim, BOOL stretch)
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();

		if (!object)
		{
			continue;
		}
		
		if (!object->permModify())
		{
			continue;
		}

		if (object->getNumTEs() == 0)
		{
			continue;
		}

		BOOL send = FALSE;
		
		for (U8 te_num = 0; te_num < object->getNumTEs(); te_num++)
		{
			const LLTextureEntry* tep = object->getTE(te_num);

			BOOL planar = tep->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR;
			if (planar == stretch)
			{
				// Figure out how S,T changed with scale operation
				U32 s_axis, t_axis;
				if (!LLPrimitive::getTESTAxes(te_num, &s_axis, &t_axis))
				{
					continue;
				}
				
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
// selectGetAllRootsValid()
// Returns TRUE if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetAllRootsValid()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); ++iter )
	{
		LLSelectNode* node = *iter;
		if( !node->mValid )
		{
			return FALSE;
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// selectGetAllValid()
// Returns TRUE if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetAllValid()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); ++iter )
	{
		LLSelectNode* node = *iter;
		if( !node->mValid )
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetAllValidAndObjectsFound() - return TRUE if selections are
// valid and objects are found.
//
// For EXT-3114 - same as selectGetModify() without the modify check.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetAllValidAndObjectsFound()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetModify() - return TRUE if current agent can modify all
// selected objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetModify()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return FALSE;
		}
		if( !object->permModify() )
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetRootsModify() - return TRUE if current agent can modify all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsModify()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return FALSE;
		}
		if( !object->permModify() )
		{
			return FALSE;
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// selectGetRootsTransfer() - return TRUE if current agent can transfer all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsTransfer()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return FALSE;
		}
		if(!object->permTransfer())
		{
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// selectGetRootsCopy() - return TRUE if current agent can copy all
// selected root objects.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetRootsCopy()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return FALSE;
		}
		if(!object->permCopy())
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
BOOL LLSelectMgr::selectGetCreator(LLUUID& result_id, std::string& name)
{
	BOOL identical = TRUE;
	BOOL first = TRUE;
	LLUUID first_id;
	for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
		 iter != getSelection()->root_object_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			first_id = node->mPermissions->getCreator();
			first = FALSE;
		}
		else
		{
			if ( !(first_id == node->mPermissions->getCreator() ) )
			{
				identical = FALSE;
				break;
			}
		}
	}
	if (first_id.isNull())
	{
		name = LLTrans::getString("AvatarNameNobody");
		return FALSE;
	}
	
	result_id = first_id;
	
	if (identical)
	{
		name = LLSLURL("agent", first_id, "inspect").getSLURLString();
	}
	else
	{
		name = LLTrans::getString("AvatarNameMultiple");
	}

	return identical;
}


//-----------------------------------------------------------------------------
// selectGetOwner()
// Owner information only applies to roots.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetOwner(LLUUID& result_id, std::string& name)
{
	BOOL identical = TRUE;
	BOOL first = TRUE;
	BOOL first_group_owned = FALSE;
	LLUUID first_id;
	for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
		 iter != getSelection()->root_object_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}
		
		if (first)
		{
			node->mPermissions->getOwnership(first_id, first_group_owned);
			first = FALSE;
		}
		else
		{
			LLUUID owner_id;
			BOOL is_group_owned = FALSE;
			if (!(node->mPermissions->getOwnership(owner_id, is_group_owned))
				|| owner_id != first_id || is_group_owned != first_group_owned)
			{
				identical = FALSE;
				break;
			}
		}
	}
	if (first_id.isNull())
	{
		return FALSE;
	}

	result_id = first_id;
	
	if (identical)
	{
		BOOL public_owner = (first_id.isNull() && !first_group_owned);
		if (first_group_owned)
		{
			name = LLSLURL("group", first_id, "inspect").getSLURLString();
		}
		else if(!public_owner)
		{
			name = LLSLURL("agent", first_id, "inspect").getSLURLString();
		}
		else
		{
			name = LLTrans::getString("AvatarNameNobody");
		}
	}
	else
	{
		name = LLTrans::getString("AvatarNameMultiple");
	}

	return identical;
}


//-----------------------------------------------------------------------------
// selectGetLastOwner()
// Owner information only applies to roots.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetLastOwner(LLUUID& result_id, std::string& name)
{
	BOOL identical = TRUE;
	BOOL first = TRUE;
	LLUUID first_id;
	for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
		 iter != getSelection()->root_object_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			first_id = node->mPermissions->getLastOwner();
			first = FALSE;
		}
		else
		{
			if ( !(first_id == node->mPermissions->getLastOwner() ) )
			{
				identical = FALSE;
				break;
			}
		}
	}
	if (first_id.isNull())
	{
		return FALSE;
	}

	result_id = first_id;
	
	if (identical)
	{
		BOOL public_owner = (first_id.isNull());
		if(!public_owner)
		{
			name = LLSLURL("agent", first_id, "inspect").getSLURLString();
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
BOOL LLSelectMgr::selectGetGroup(LLUUID& result_id)
{
	BOOL identical = TRUE;
	BOOL first = TRUE;
	LLUUID first_id;
	for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
		 iter != getSelection()->root_object_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			first_id = node->mPermissions->getGroup();
			first = FALSE;
		}
		else
		{
			if ( !(first_id == node->mPermissions->getGroup() ) )
			{
				identical = FALSE;
				break;
			}
		}
	}

	result_id = first_id;

	return identical;
}

//-----------------------------------------------------------------------------
// selectIsGroupOwned()
// Only operates on root nodes.  
// Returns TRUE if all have valid data and they are all group owned.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectIsGroupOwned()
{
	BOOL found_one = FALSE;
	for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
		 iter != getSelection()->root_object_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}
		found_one = TRUE;
		if (!node->mPermissions->isGroupOwned())
		{
			return FALSE;
		}
	}	
	return found_one ? TRUE : FALSE;
}

//-----------------------------------------------------------------------------
// selectGetPerm()
// Only operates on root nodes.
// Returns TRUE if all have valid data.
// mask_on has bits set to TRUE where all permissions are TRUE
// mask_off has bits set to TRUE where all permissions are FALSE
// if a bit is off both in mask_on and mask_off, the values differ within
// the selection.
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::selectGetPerm(U8 which_perm, U32* mask_on, U32* mask_off)
{
	U32 mask;
	U32 mask_and	= 0xffffffff;
	U32 mask_or		= 0x00000000;
	BOOL all_valid	= FALSE;

	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;

		if (!node->mValid)
		{
			all_valid = FALSE;
			break;
		}

		all_valid = TRUE;
		
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
		// ...TRUE through all ANDs means all TRUE
		*mask_on  = mask_and;

		// ...FALSE through all ORs means all FALSE
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



BOOL LLSelectMgr::selectGetPermissions(LLPermissions& result_perm)
{
	BOOL first = TRUE;
	LLPermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			perm = *(node->mPermissions);
			first = FALSE;
		}
		else
		{
			perm.accumulate(*(node->mPermissions));
		}
	}

	result_perm = perm;

	return TRUE;
}


void LLSelectMgr::selectDelete()
{
	S32 deleteable_count = 0;

	BOOL locked_but_deleteable_object = FALSE;
	BOOL no_copy_but_deleteable_object = FALSE;
	BOOL all_owned_by_you = TRUE;

	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++)
	{
		LLViewerObject* obj = (*iter)->getObject();

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

	LLNotification::Params params("ConfirmObjectDeleteLock");
	params.functor.function(boost::bind(&LLSelectMgr::confirmDelete, _1, _2, getSelection()));

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
			params.name("ConfirmObjectDeleteLock");
		}
		else if(!locked_but_deleteable_object && no_copy_but_deleteable_object && all_owned_by_you)
		{
			//No Copy only
			params.name("ConfirmObjectDeleteNoCopy");
		}
		else if(!locked_but_deleteable_object && !no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//not owned only
			params.name("ConfirmObjectDeleteNoOwn");
		}
		else if(locked_but_deleteable_object && no_copy_but_deleteable_object && all_owned_by_you)
		{
			//locked and no copy
			params.name("ConfirmObjectDeleteLockNoCopy");
		}
		else if(locked_but_deleteable_object && !no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//locked and not owned
			params.name("ConfirmObjectDeleteLockNoOwn");
		}
		else if(!locked_but_deleteable_object && no_copy_but_deleteable_object && !all_owned_by_you)
		{
			//no copy and not owned
			params.name("ConfirmObjectDeleteNoCopyNoOwn");
		}
		else
		{
			//locked, no copy and not owned
			params.name("ConfirmObjectDeleteLockNoCopyNoOwn");
		}
		
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}

// static
bool LLSelectMgr::confirmDelete(const LLSD& notification, const LLSD& response, LLObjectSelectionHandle handle)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (!handle->getObjectCount())
	{
		llwarns << "Nothing to delete!" << llendl;
		return false;
	}

	switch(option)
	{
	case 0:
		{
			// TODO: Make sure you have delete permissions on all of them.
			const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
			// attempt to derez into the trash.
			LLDeRezInfo info(DRD_TRASH, trash_id);
			LLSelectMgr::getInstance()->sendListToRegions("DeRezObject",
										  packDeRezHeader,
										  packObjectLocalID,
										  (void*) &info,
										  SEND_ONLY_ROOTS);
			// VEFFECT: Delete Object - one effect for all deletes
			if (LLSelectMgr::getInstance()->mSelectedObjects->mSelectType != SELECT_TYPE_HUD)
			{
				LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
				effectp->setPositionGlobal( LLSelectMgr::getInstance()->getSelectionCenterGlobal() );
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
				F32 duration = 0.5f;
				duration += LLSelectMgr::getInstance()->mSelectedObjects->getObjectCount() / 64.f;
				effectp->setDuration(duration);
			}

			gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);

			// Keep track of how many objects have been deleted.
			F64 obj_delete_count = LLViewerStats::getInstance()->getStat(LLViewerStats::ST_OBJECT_DELETE_COUNT);
			obj_delete_count += LLSelectMgr::getInstance()->mSelectedObjects->getObjectCount();
			LLViewerStats::getInstance()->setStat(LLViewerStats::ST_OBJECT_DELETE_COUNT, obj_delete_count );
		}
		break;
	case 1:
	default:
		break;
	}
	return false;
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

void LLSelectMgr::selectGetAggregateSaleInfo(U32 &num_for_sale,
											 BOOL &is_for_sale_mixed, 
											 BOOL &is_sale_price_mixed,
											 S32 &total_sale_price,
											 S32 &individual_sale_price)
{
	num_for_sale = 0;
	is_for_sale_mixed = FALSE;
	is_sale_price_mixed = FALSE;
	total_sale_price = 0;
	individual_sale_price = 0;


	// Empty set.
	if (getSelection()->root_begin() == getSelection()->root_end())
		return;
	
	LLSelectNode *node = *(getSelection()->root_begin());
	const BOOL first_node_for_sale = node->mSaleInfo.isForSale();
	const S32 first_node_sale_price = node->mSaleInfo.getSalePrice();
	
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		const BOOL node_for_sale = node->mSaleInfo.isForSale();
		const S32 node_sale_price = node->mSaleInfo.getSalePrice();
		
		// Set mixed if the fields don't match the first node's fields.
		if (node_for_sale != first_node_for_sale) 
			is_for_sale_mixed = TRUE;
		if (node_sale_price != first_node_sale_price)
			is_sale_price_mixed = TRUE;
		
		if (node_for_sale)
		{
			total_sale_price += node_sale_price;
			num_for_sale ++;
		}
	}
	
	individual_sale_price = first_node_sale_price;
	if (is_for_sale_mixed)
	{
		is_sale_price_mixed = TRUE;
		individual_sale_price = 0;
	}
}

// returns TRUE if all nodes are valid. method also stores an
// accumulated sale info.
BOOL LLSelectMgr::selectGetSaleInfo(LLSaleInfo& result_sale_info)
{
	BOOL first = TRUE;
	LLSaleInfo sale_info;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			sale_info = node->mSaleInfo;
			first = FALSE;
		}
		else
		{
			sale_info.accumulate(node->mSaleInfo);
		}
	}

	result_sale_info = sale_info;

	return TRUE;
}

BOOL LLSelectMgr::selectGetAggregatePermissions(LLAggregatePermissions& result_perm)
{
	BOOL first = TRUE;
	LLAggregatePermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		if (first)
		{
			perm = node->mAggregatePerm;
			first = FALSE;
		}
		else
		{
			perm.aggregate(node->mAggregatePerm);
		}
	}

	result_perm = perm;

	return TRUE;
}

BOOL LLSelectMgr::selectGetAggregateTexturePermissions(LLAggregatePermissions& result_perm)
{
	BOOL first = TRUE;
	LLAggregatePermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return FALSE;
		}

		LLAggregatePermissions t_perm = node->getObject()->permYouOwner() ? node->mAggregateTexturePermOwner : node->mAggregateTexturePerm;
		if (first)
		{
			perm = t_perm;
			first = FALSE;
		}
		else
		{
			perm.aggregate(t_perm);
		}
	}

	result_perm = perm;

	return TRUE;
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
		for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
			 iter != getSelection()->root_end(); iter++ )
		{
			LLSelectNode* node = *iter;
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

	std::vector<LLViewerObject*> non_duplicated_objects;

	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mDuplicated)
		{
			non_duplicated_objects.push_back(node->getObject());
		}
	}

	// make sure only previously duplicated objects are selected
	for (std::vector<LLViewerObject*>::iterator iter = non_duplicated_objects.begin();
		 iter != non_duplicated_objects.end(); ++iter)
	{
		LLViewerObject* objectp = *iter;
		deselectObjectAndFamily(objectp);
	}
	
	// duplicate objects in place
	LLDuplicateData	data;

	data.offset = LLVector3::zero;
	data.flags = 0x0;

	sendListToRegions("ObjectDuplicate", packDuplicateHeader, packDuplicate, &data, SEND_ONLY_ROOTS);

	// move current selection based on delta from duplication position and update duplication position
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
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
	ESendType send_type = (!gSavedSettings.getBOOL("EditLinkedParts") && !getTEMode()) ? SEND_ONLY_ROOTS : SEND_ROOTS_FIRST;
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
	std::vector<LLViewerObject*> mObjectsSent;
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
	if (std::find(buy->mObjectsSent.begin(), buy->mObjectsSent.end(), object) == buy->mObjectsSent.end())
	{
		buy->mObjectsSent.push_back(object);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID() );
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
		
	// Zap the angular velocity, as the sim will set it to zero
	for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
		 iter != mSelectedObjects->end(); iter++ )
	{
		LLViewerObject *objectp = (*iter)->getObject();
		objectp->setAngularVelocity( 0,0,0 );
		objectp->setVelocity( 0,0,0 );
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
}

void LLSelectMgr::deselectAllForStandingUp()
{
	/*
	This function is similar deselectAll() except for the first if statement
	which was removed. This is needed as a workaround for DEV-2854
	*/

	// Zap the angular velocity, as the sim will set it to zero
	for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
		 iter != mSelectedObjects->end(); iter++ )
	{
		LLViewerObject *objectp = (*iter)->getObject();
		objectp->setAngularVelocity( 0,0,0 );
		objectp->setVelocity( 0,0,0 );
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
	if (gMenuObject->getVisible())
	{
		return;
	}

	LLVector3d selectionCenter = getSelectionCenterGlobal();
	if (gSavedSettings.getBOOL("LimitSelectDistance")
		&& (!mSelectedObjects->getPrimaryObject() || !mSelectedObjects->getPrimaryObject()->isAvatar())
		&& (mSelectedObjects->getPrimaryObject() != LLViewerMediaFocus::getInstance()->getFocusedObject())
		&& !mSelectedObjects->isAttachment()
		&& !selectionCenter.isExactlyZero())
	{
		F32 deselect_dist = gSavedSettings.getF32("MaxSelectDistance");
		F32 deselect_dist_sq = deselect_dist * deselect_dist;

		LLVector3d select_delta = gAgent.getPositionGlobal() - selectionCenter;
		F32 select_dist_sq = (F32) select_delta.magVecSquared();

		if (select_dist_sq > deselect_dist_sq)
		{
			if (mDebugSelectMgr)
			{
				llinfos << "Selection manager: auto-deselecting, select_dist = " << (F32) sqrt(select_dist_sq) << llendl;
				llinfos << "agent pos global = " << gAgent.getPositionGlobal() << llendl;
				llinfos << "selection pos global = " << selectionCenter << llendl;
			}

			deselectAll();
		}
	}
}


void LLSelectMgr::selectionSetObjectName(const std::string& name)
{
	std::string name_copy(name);

	// we only work correctly if 1 object is selected.
	if(mSelectedObjects->getRootObjectCount() == 1)
	{
		sendListToRegions("ObjectName",
						  packAgentAndSessionID,
						  packObjectName,
						  (void*)(&name_copy),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectName",
						  packAgentAndSessionID,
						  packObjectName,
						  (void*)(&name_copy),
						  SEND_INDIVIDUALS);
	}
}

void LLSelectMgr::selectionSetObjectDescription(const std::string& desc)
{
	std::string desc_copy(desc);

	// we only work correctly if 1 object is selected.
	if(mSelectedObjects->getRootObjectCount() == 1)
	{
		sendListToRegions("ObjectDescription",
						  packAgentAndSessionID,
						  packObjectDescription,
						  (void*)(&desc_copy),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectDescription",
						  packAgentAndSessionID,
						  packObjectDescription,
						  (void*)(&desc_copy),
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
	sendListToRegions("ObjectSaleInfo",
					  packAgentAndSessionID,
					  packObjectSaleInfo,
					  (void*)(&sale_info),
					  SEND_ONLY_ROOTS);
}

//----------------------------------------------------------------------
// Attachments
//----------------------------------------------------------------------

void LLSelectMgr::sendAttach(U8 attachment_point, bool replace)
{
	LLViewerObject* attach_object = mSelectedObjects->getFirstRootObject();

	if (!attach_object || !isAgentAvatarValid() || mSelectedObjects->mSelectType != SELECT_TYPE_WORLD)
	{
		return;
	}

	BOOL build_mode = LLToolMgr::getInstance()->inEdit();
	// Special case: Attach to default location for this object.
	if (0 == attachment_point ||
		get_if_there(gAgentAvatarp->mAttachmentPoints, (S32)attachment_point, (LLViewerJointAttachment*)NULL))
	{
		if (!replace || attachment_point != 0)
		{
			// If we know the attachment point then we got here by clicking an
			// "Attach to..." context menu item, so we should add, not replace.
			attachment_point |= ATTACHMENT_ADD;
		}

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

	struct f : public LLSelectedObjectFunctor
	{ //on delink, any modifyable object should
		f() {}

		virtual bool apply(LLViewerObject* object)
		{
			if (object->permModify())
			{
				if (object->getPhysicsShapeType() == LLViewerObject::PHYSICS_SHAPE_NONE)
				{
					object->setPhysicsShapeType(LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL);
					object->updateFlags();
				}
			}
			return true;
		}
	} sendfunc;
	getSelection()->applyToObjects(&sendfunc);


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

/*
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
}*/

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
	struct f : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			object->dump();
			return true;
		}
	} func;
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::saveSelectedObjectColors()
{
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* node)
		{
			node->saveColors();
			return true;
		}
	} func;
	getSelection()->applyToNodes(&func);	
}

void LLSelectMgr::saveSelectedObjectTextures()
{
	// invalidate current selection so we update saved textures
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* node)
		{
			node->mValid = FALSE;
			return true;
		}
	} func;
	getSelection()->applyToNodes(&func);	

	// request object properties message to get updated permissions data
	sendSelect();
}


// This routine should be called whenever a drag is initiated.
// also need to know to which simulator to send update message
void LLSelectMgr::saveSelectedObjectTransform(EActionType action_type)
{
	if (mSelectedObjects->isEmpty())
	{
		// nothing selected, so nothing to save
		return;
	}

	struct f : public LLSelectedNodeFunctor
	{
		EActionType mActionType;
		f(EActionType a) : mActionType(a) {}
		virtual bool apply(LLSelectNode* selectNode)
		{
			LLViewerObject*	object = selectNode->getObject();
			if (!object)
			{
				return true; // skip
			}
			selectNode->mSavedPositionLocal = object->getPosition();
			if (object->isAttachment())
			{
				if (object->isRootEdit())
				{
					LLXform* parent_xform = object->mDrawable->getXform()->getParent();
					if (parent_xform)
					{
						selectNode->mSavedPositionGlobal = gAgent.getPosGlobalFromAgent((object->getPosition() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition());
					}
					else
					{
						selectNode->mSavedPositionGlobal = object->getPositionGlobal();
					}
				}
				else
				{
					LLViewerObject* attachment_root = (LLViewerObject*)object->getParent();
					LLXform* parent_xform = attachment_root ? attachment_root->mDrawable->getXform()->getParent() : NULL;
					if (parent_xform)
					{
						LLVector3 root_pos = (attachment_root->getPosition() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();
						LLQuaternion root_rot = (attachment_root->getRotation() * parent_xform->getWorldRotation());
						selectNode->mSavedPositionGlobal = gAgent.getPosGlobalFromAgent((object->getPosition() * root_rot) + root_pos);
					}
					else
					{
						selectNode->mSavedPositionGlobal = object->getPositionGlobal();
					}
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
			return true;
		}
	} func(action_type);
	getSelection()->applyToNodes(&func);	
	
	mSavedSelectionBBox = getBBoxOfSelection();
}

struct LLSelectMgrApplyFlags : public LLSelectedObjectFunctor
{
	LLSelectMgrApplyFlags(U32 flags, BOOL state) : mFlags(flags), mState(state) {}
	U32 mFlags;
	BOOL mState;
	virtual bool apply(LLViewerObject* object)
	{
		if ( object->permModify() &&	// preemptive permissions check
			 object->isRoot() &&		// don't send for child objects
			 !object->isJointChild())
		{
			object->setFlags( mFlags, mState);
		}
		return true;
	}
};

void LLSelectMgr::selectionUpdatePhysics(BOOL physics)
{
	LLSelectMgrApplyFlags func(	FLAGS_USE_PHYSICS, physics);
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::selectionUpdateTemporary(BOOL is_temporary)
{
	LLSelectMgrApplyFlags func(	FLAGS_TEMPORARY_ON_REZ, is_temporary);
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::selectionUpdatePhantom(BOOL is_phantom)
{
	LLSelectMgrApplyFlags func(	FLAGS_PHANTOM, is_phantom);
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::selectionUpdateCastShadows(BOOL cast_shadows)
{
	LLSelectMgrApplyFlags func(	FLAGS_CAST_SHADOWS, cast_shadows);
	getSelection()->applyToObjects(&func);	
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

void LLSelectMgr::packObjectIncludeInSearch(LLSelectNode* node, void *user_data)
{
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, node->getObject()->getLocalID() );
	gMessageSystem->addBOOL("IncludeInSearch", node->getObject()->getIncludeInSearch());
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
	const std::string* name = (const std::string*)user_data;
	if(!name->empty())
	{
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
		gMessageSystem->addStringFast(_PREHASH_Name, *name);
	}
}

// static
void LLSelectMgr::packObjectDescription(LLSelectNode* node, void* user_data)
{
	const std::string* desc = (const std::string*)user_data;
	if(desc)
	{	// Empty (non-null, but zero length) descriptions are OK
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_LocalID, node->getObject()->getLocalID());
		gMessageSystem->addStringFast(_PREHASH_Description, *desc);
	}
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
void LLSelectMgr::sendListToRegions(const std::string& message_name,
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

	//clear update override data (allow next update through)
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* node)
		{
			node->mLastPositionLocal.setVec(0,0,0);
			node->mLastRotation = LLQuaternion();
			node->mLastScale.setVec(0,0,0);
			return true;
		}
	} func;
	getSelection()->applyToNodes(&func);	

	std::queue<LLSelectNode*> nodes_to_send;

	struct push_all : public LLSelectedNodeFunctor
	{
		std::queue<LLSelectNode*>& nodes_to_send;
		push_all(std::queue<LLSelectNode*>& n) : nodes_to_send(n) {}
		virtual bool apply(LLSelectNode* node)
		{
			if (node->getObject())
			{
				nodes_to_send.push(node);
			}
			return true;
		}
	};
	struct push_some : public LLSelectedNodeFunctor
	{
		std::queue<LLSelectNode*>& nodes_to_send;
		bool mRoots;
		push_some(std::queue<LLSelectNode*>& n, bool roots) : nodes_to_send(n), mRoots(roots) {}
		virtual bool apply(LLSelectNode* node)
		{
			if (node->getObject())
			{
				BOOL is_root = node->getObject()->isRootEdit();
				if ((mRoots && is_root) || (!mRoots && !is_root))
				{
					nodes_to_send.push(node);
				}
			}
			return true;
		}
	};
	struct push_all  pushall(nodes_to_send);
	struct push_some pushroots(nodes_to_send, TRUE);
	struct push_some pushnonroots(nodes_to_send, FALSE);
	
	switch(send_type)
	{
	  case SEND_ONLY_ROOTS:
		  if(message_name == "ObjectBuy")
			getSelection()->applyToRootNodes(&pushroots);
		  else
			getSelection()->applyToRootNodes(&pushall);
		  
		break;
	  case SEND_INDIVIDUALS:
		getSelection()->applyToNodes(&pushall);
		break;
	  case SEND_ROOTS_FIRST:
		// first roots...
		getSelection()->applyToNodes(&pushroots);
		// then children...
		getSelection()->applyToNodes(&pushnonroots);
		break;
	  case SEND_CHILDREN_FIRST:
		// first children...
		getSelection()->applyToNodes(&pushnonroots);
		// then roots...
		getSelection()->applyToNodes(&pushroots);
		break;

	default:
		llerrs << "Bad send type " << send_type << " passed to SendListToRegions()" << llendl;
	}

	// bail if nothing selected
	if (nodes_to_send.empty())
	{
		return;
	}
	
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
			&& (! gMessageSystem->isSendFull(NULL))
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
				node = nodes_to_send.front();
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
	if (gMessageSystem->getCurrentSendTotal() > 0)
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

		std::string name;
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name, i);
		std::string desc;
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc, i);

		std::string touch_name;
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_TouchName, touch_name, i);
		std::string sit_name;
		msg->getStringFast(_PREHASH_ObjectData, _PREHASH_SitName, sit_name, i);

		//unpack TE IDs
		uuid_vec_t texture_ids;
		S32 size = msg->getSizeFast(_PREHASH_ObjectData, i, _PREHASH_TextureID);
		if (size > 0)
		{
			S8 packed_buffer[SELECT_MAX_TES * UUID_BYTES];
			msg->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureID, packed_buffer, 0, i, SELECT_MAX_TES * UUID_BYTES);

			for (S32 buf_offset = 0; buf_offset < size; buf_offset += UUID_BYTES)
			{
				LLUUID tid;
				memcpy(tid.mData, packed_buffer + buf_offset, UUID_BYTES);		/* Flawfinder: ignore */
				texture_ids.push_back(tid);
			}
		}


		// Iterate through nodes at end, since it can be on both the regular AND hover list
		struct f : public LLSelectedNodeFunctor
		{
			LLUUID mID;
			f(const LLUUID& id) : mID(id) {}
			virtual bool apply(LLSelectNode* node)
			{
				return (node->getObject() && node->getObject()->mID == mID);
			}
		} func(id);
		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(&func);

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
	std::string name;
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name);

	std::string desc;
	msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc);

	// the reporter widget askes the server for info about picked objects
	if (request_flags & COMPLAINT_REPORT_REQUEST )
	{
		LLFloaterReporter *reporterp = LLFloaterReg::findTypedInstance<LLFloaterReporter>("reporter");
		if (reporterp)
		{
			std::string fullname;
			gCacheName->getFullName(owner_id, fullname);
			reporterp->setPickedObjectProperties(name, fullname, owner_id);
		}
	}
	else if (request_flags & OBJECT_PAY_REQUEST)
	{
		// check if the owner of the paid object is muted
		LLMuteList::getInstance()->autoRemove(owner_id, LLMuteList::AR_MONEY);
	}

	// Now look through all of the hovered nodes
	struct f : public LLSelectedNodeFunctor
	{
		LLUUID mID;
		f(const LLUUID& id) : mID(id) {}
		virtual bool apply(LLSelectNode* node)
		{
			return (node->getObject() && node->getObject()->mID == mID);
		}
	} func(id);
	LLSelectNode* node = LLSelectMgr::getInstance()->mHoverObjects->getFirstNode(&func);

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
		LLSelectMgr::getInstance()->deselectAll();
	}

	LLUUID full_id;
	S32 local_id;
	LLViewerObject* object;
	std::vector<LLViewerObject*> objects;
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
			objects.push_back(object);
		}
	}

	// Don't select, just highlight
	LLSelectMgr::getInstance()->highlightObjectAndFamily(objects);
}

extern F32	gGLModelView[16];

void LLSelectMgr::updateSilhouettes()
{
	S32 num_sils_genned = 0;

	LLVector3d	cameraPos = gAgentCamera.getCameraPositionGlobal();
	F32 currentCameraZoom = gAgentCamera.getCurrentCameraBuildOffset();

	if (!mSilhouetteImagep)
	{
		mSilhouetteImagep = LLViewerTextureManager::getFetchedTextureFromFile("silhouette.j2c", TRUE, LLViewerTexture::BOOST_UI);
	}

	mHighlightedObjects->cleanupNodes();

	if((cameraPos - mLastCameraPos).magVecSquared() > SILHOUETTE_UPDATE_THRESHOLD_SQUARED * currentCameraZoom * currentCameraZoom)
	{
		struct f : public LLSelectedObjectFunctor
		{
			virtual bool apply(LLViewerObject* object)
			{
				object->setChanged(LLXform::SILHOUETTE);
				return true;
			}
		} func;
		getSelection()->applyToObjects(&func);	
		
		mLastCameraPos = gAgentCamera.getCameraPositionGlobal();
	}
	
	std::vector<LLViewerObject*> changed_objects;

	updateSelectionSilhouette(mSelectedObjects, num_sils_genned, changed_objects);
	if (mRectSelectedObjects.size() > 0)
	{
		//gGLSPipelineSelection.set();

		//mSilhouetteImagep->bindTexture();
		//glAlphaFunc(GL_GREATER, sHighlightAlphaTest);

		std::set<LLViewerObject*> roots;

		// sync mHighlightedObjects with mRectSelectedObjects since the latter is rebuilt every frame and former
		// persists from frame to frame to avoid regenerating object silhouettes
		// mHighlightedObjects includes all siblings of rect selected objects

		BOOL select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");

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
		std::vector<LLSelectNode*> remove_these_nodes;
		std::vector<LLViewerObject*> remove_these_roots;

		for (LLObjectSelection::iterator iter = mHighlightedObjects->begin();
			 iter != mHighlightedObjects->end(); iter++)
		{
			LLSelectNode* node = *iter;
			LLViewerObject* objectp = node->getObject();
			if (!objectp)
				continue;
			if (objectp->isRoot() || !select_linked_set)
			{
				if (roots.count(objectp) == 0)
				{
					remove_these_nodes.push_back(node);
				}
				else
				{
					remove_these_roots.push_back(objectp);
				}
			}
			else
			{
				LLViewerObject* rootp = (LLViewerObject*)objectp->getRoot();

				if (roots.count(rootp) == 0)
				{
					remove_these_nodes.push_back(node);
				}
			}
		}

		// remove all highlight nodes no longer in rectangle selection
		for (std::vector<LLSelectNode*>::iterator iter = remove_these_nodes.begin();
			 iter != remove_these_nodes.end(); ++iter)
		{
			LLSelectNode* nodep = *iter;
			mHighlightedObjects->removeNode(nodep);
		}

		// remove all root objects already being highlighted
		for (std::vector<LLViewerObject*>::iterator iter = remove_these_roots.begin();
			 iter != remove_these_roots.end(); ++iter)
		{
			LLViewerObject* objectp = *iter;
			roots.erase(objectp);
		}

		// add all new objects in rectangle selection
		for (std::set<LLViewerObject*>::iterator iter = roots.begin();
			 iter != roots.end(); iter++)
		{
			LLViewerObject* objectp = *iter;
			if (!canSelectObject(objectp))
			{
				continue;
			}

			LLSelectNode* rect_select_root_node = new LLSelectNode(objectp, TRUE);
			rect_select_root_node->selectAllTEs(TRUE);

			if (!select_linked_set)
			{
				rect_select_root_node->mIndividualSelection = TRUE;
			}
			else
			{
				LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* child_objectp = *iter;
				
					if (!canSelectObject(child_objectp))
					{
						continue;
					}

					LLSelectNode* rect_select_node = new LLSelectNode(child_objectp, TRUE);
					rect_select_node->selectAllTEs(TRUE);
					mHighlightedObjects->addNodeAtEnd(rect_select_node);
				}
			}

			// Add the root last, to preserve order for link operations.
			mHighlightedObjects->addNodeAtEnd(rect_select_root_node);
		}

		num_sils_genned	= 0;

		// render silhouettes for highlighted objects
		//BOOL subtracting_from_selection = (gKeyboard->currentMask(TRUE) == MASK_CONTROL);
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (LLObjectSelection::iterator iter = mHighlightedObjects->begin();
				 iter != mHighlightedObjects->end(); iter++)
			{
				LLSelectNode* node = *iter;
				LLViewerObject* objectp = node->getObject();
				if (!objectp)
					continue;
				
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
						generateSilhouette(node, LLViewerCamera::getInstance()->getOrigin());
						changed_objects.push_back(objectp);			
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

	for (std::vector<LLViewerObject*>::iterator iter = changed_objects.begin();
		 iter != changed_objects.end(); ++iter)
	{
		// clear flags after traversing node list (as child objects need to refer to parent flags, etc)
		LLViewerObject* objectp = *iter;
		objectp->clearChanged(LLXform::MOVED | LLXform::SILHOUETTE);
	}
	
	//gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}

void LLSelectMgr::updateSelectionSilhouette(LLObjectSelectionHandle object_handle, S32& num_sils_genned, std::vector<LLViewerObject*>& changed_objects)
{
	if (object_handle->getNumNodes())
	{
		//gGLSPipelineSelection.set();

		//mSilhouetteImagep->bindTexture();
		//glAlphaFunc(GL_GREATER, sHighlightAlphaTest);

		for (S32 pass = 0; pass < 2; pass++)
		{
			for (LLObjectSelection::iterator iter = object_handle->begin();
				iter != object_handle->end(); iter++)
			{
				LLSelectNode* node = *iter;
				LLViewerObject* objectp = node->getObject();
				if (!objectp)
					continue;
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
					if (num_sils_genned++ < MAX_SILS_PER_FRAME)// && objectp->mDrawable->isVisible())
					{
						generateSilhouette(node, LLViewerCamera::getInstance()->getOrigin());
						changed_objects.push_back(objectp);
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
}
void LLSelectMgr::renderSilhouettes(BOOL for_hud)
{
	if (!mRenderSilhouettes || !mRenderHighlightSelections)
	{
		return;
	}

	gGL.getTexUnit(0)->bind(mSilhouetteImagep);
	LLGLSPipelineSelection gls_select;
	LLGLEnable blend(GL_BLEND);
	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	if (isAgentAvatarValid() && for_hud)
	{
		LLBBox hud_bbox = gAgentAvatarp->getHUDBBox();

		F32 cur_zoom = gAgentCamera.mHUDCurZoom;

		// set up transform to encompass bounding box of HUD
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.pushMatrix();
		gGL.loadIdentity();
		F32 depth = llmax(1.f, hud_bbox.getExtentLocal().mV[VX] * 1.1f);
		gGL.ortho(-0.5f * LLViewerCamera::getInstance()->getAspect(), 0.5f * LLViewerCamera::getInstance()->getAspect(), -0.5f, 0.5f, 0.f, depth);

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		gGL.pushUIMatrix();
		gGL.loadUIIdentity();
		gGL.loadIdentity();
		gGL.loadMatrix(OGL_TO_CFR_ROTATION);		// Load Cory's favorite reference frame
		gGL.translatef(-hud_bbox.getCenterLocal().mV[VX] + (depth *0.5f), 0.f, 0.f);
		gGL.scalef(cur_zoom, cur_zoom, cur_zoom);
	}
	if (mSelectedObjects->getNumNodes())
	{
		LLUUID inspect_item_id= LLUUID::null;
		LLFloaterInspect* inspect_instance = LLFloaterReg::getTypedInstance<LLFloaterInspect>("inspect");
		if(inspect_instance && inspect_instance->getVisible())
		{
			inspect_item_id = inspect_instance->getSelectedUUID();
		}
		else
		{
			LLSidepanelTaskInfo *panel_task_info = LLSidepanelTaskInfo::getActivePanel();
			if (panel_task_info)
			{
				inspect_item_id = panel_task_info->getSelectedUUID();
			}
		}

		LLUUID focus_item_id = LLViewerMediaFocus::getInstance()->getFocusedObjectID();
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
				 iter != mSelectedObjects->end(); iter++)
			{
				LLSelectNode* node = *iter;
				LLViewerObject* objectp = node->getObject();
				if (!objectp)
					continue;
				if (objectp->isHUDAttachment() != for_hud)
				{
					continue;
				}
				if (objectp->getID() == focus_item_id)
				{
					node->renderOneSilhouette(gFocusMgr.getFocusColor());
				}
				else if(objectp->getID() == inspect_item_id)
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
	}

	if (mHighlightedObjects->getNumNodes())
	{
		// render silhouettes for highlighted objects
		BOOL subtracting_from_selection = (gKeyboard->currentMask(TRUE) == MASK_CONTROL);
		for (S32 pass = 0; pass < 2; pass++)
		{
			for (LLObjectSelection::iterator iter = mHighlightedObjects->begin();
				 iter != mHighlightedObjects->end(); iter++)
			{
				LLSelectNode* node = *iter;
				LLViewerObject* objectp = node->getObject();
				if (!objectp)
					continue;
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

	if (isAgentAvatarValid() && for_hud)
	{
		gGL.matrixMode(LLRender::MM_PROJECTION);
		gGL.popMatrix();

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.popMatrix();
		gGL.popUIMatrix();
		stop_glerror();
	}

	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

void LLSelectMgr::generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point)
{
	LLViewerObject* objectp = nodep->getObject();

	if (objectp && objectp->getPCode() == LL_PCODE_VOLUME)
	{
		((LLVOVolume*)objectp)->generateSilhouette(nodep, view_point);
	}
}

//
// Utility classes
//
LLSelectNode::LLSelectNode(LLViewerObject* object, BOOL glow)
:	mObject(object),
	mIndividualSelection(FALSE),
	mTransient(FALSE),
	mValid(FALSE),
	mPermissions(new LLPermissions()),
	mInventorySerial(0),
	mSilhouetteExists(FALSE),
	mDuplicated(FALSE),
	mTESelectMask(0),
	mLastTESelected(0),
	mName(LLStringUtil::null),
	mDescription(LLStringUtil::null),
	mTouchName(LLStringUtil::null),
	mSitName(LLStringUtil::null),
	mCreationDate(0)
{
	saveColors();
}

LLSelectNode::LLSelectNode(const LLSelectNode& nodep)
{
	mTESelectMask = nodep.mTESelectMask;
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
	mInventorySerial = 0;
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
	mCreationDate = nodep.mCreationDate;

	mSilhouetteVertices = nodep.mSilhouetteVertices;
	mSilhouetteNormals = nodep.mSilhouetteNormals;
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
	mTESelectMask = b ? TE_SELECT_MASK_ALL : 0x0;
	mLastTESelected = 0;
}

void LLSelectNode::selectTE(S32 te_index, BOOL selected)
{
	if (te_index < 0 || te_index >= SELECT_MAX_TES)
	{
		return;
	}
	S32 mask = 0x1 << te_index;
	if(selected)
	{	
		mTESelectMask |= mask;
	}
	else
	{
		mTESelectMask &= ~mask;
	}
	mLastTESelected = te_index;
}

BOOL LLSelectNode::isTESelected(S32 te_index)
{
	if (te_index < 0 || te_index >= mObject->getNumTEs())
	{
		return FALSE;
	}
	return (mTESelectMask & (0x1 << te_index)) != 0;
}

S32 LLSelectNode::getLastSelectedTE()
{
	if (!isTESelected(mLastTESelected))
	{
		return -1;
	}
	return mLastTESelected;
}

LLViewerObject* LLSelectNode::getObject()
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

void LLSelectNode::setObject(LLViewerObject* object)
{
	mObject = object;
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

void LLSelectNode::saveTextures(const uuid_vec_t& textures)
{
	if (mObject.notNull())
	{
		mSavedTextures.clear();

		for (uuid_vec_t::const_iterator texture_it = textures.begin();
			 texture_it != textures.end(); ++texture_it)
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
			U32 s_axis = 0;
			U32 t_axis = 0;

			LLPrimitive::getTESTAxes(i, &s_axis, &t_axis);

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


//helper function for pushing relevant vertices from drawable to GL
void pushWireframe(LLDrawable* drawable)
{
	LLVOVolume* vobj = drawable->getVOVolume();
	if (vobj)
	{
		LLVertexBuffer::unbind();
		gGL.pushMatrix();
		gGL.multMatrix((F32*) vobj->getRelativeXform().mMatrix);

		LLVolume* volume = NULL;

		if (drawable->isState(LLDrawable::RIGGED))
		{
				vobj->updateRiggedVolume();
				volume = vobj->getRiggedVolume();
		}
		else
		{
			volume = vobj->getVolume();
		}

		if (volume)
		{
			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				LLVertexBuffer::drawElements(LLRender::TRIANGLES, face.mPositions, NULL, face.mNumIndices, face.mIndices);
			}
		}

		gGL.popMatrix();
	}
	
}

void LLSelectNode::renderOneWireframe(const LLColor4& color)
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

	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	if (shader)
	{
		gDebugProgram.bind();
	}

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	
	BOOL is_hud_object = objectp->isHUDAttachment();

	if (drawable->isActive())
	{
		gGL.loadMatrix(gGLModelView);
		gGL.multMatrix((F32*) objectp->getRenderMatrix().mMatrix);
	}
	else if (!is_hud_object)
	{
		gGL.loadIdentity();
		gGL.multMatrix(gGLModelView);
		LLVector3 trans = objectp->getRegion()->getOriginAgent();		
		gGL.translatef(trans.mV[0], trans.mV[1], trans.mV[2]);		
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	if (LLSelectMgr::sRenderHiddenSelections) // && gFloaterTools && gFloaterTools->getVisible())
	{
		gGL.blendFunc(LLRender::BF_SOURCE_COLOR, LLRender::BF_ONE);
		LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GEQUAL);
		if (shader)
		{
			gGL.diffuseColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
			pushWireframe(drawable);
		}
		else
		{
			LLGLEnable fog(GL_FOG);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			float d = (LLViewerCamera::getInstance()->getPointOfInterest()-LLViewerCamera::getInstance()->getOrigin()).magVec();
			LLColor4 fogCol = color * (F32)llclamp((LLSelectMgr::getInstance()->getSelectionCenterGlobal()-gAgentCamera.getCameraPositionGlobal()).magVec()/(LLSelectMgr::getInstance()->getBBoxOfSelection().getExtentLocal().magVec()*4), 0.0, 1.0);
			glFogf(GL_FOG_START, d);
			glFogf(GL_FOG_END, d*(1 + (LLViewerCamera::getInstance()->getView() / LLViewerCamera::getInstance()->getDefaultFOV())));
			glFogfv(GL_FOG_COLOR, fogCol.mV);

			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
			{
				gGL.diffuseColor4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
				pushWireframe(drawable);
			}
		}
	}

	gGL.flush();
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	gGL.diffuseColor4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha*2);
	
	LLGLEnable offset(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(3.f, 3.f);
	glLineWidth(3.f);
	pushWireframe(drawable);
	glLineWidth(1.f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	gGL.popMatrix();

	if (shader)
	{
		shader->bind();
	}
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

	LLVOVolume* vobj = drawable->getVOVolume();
	if (vobj && vobj->isMesh())
	{
		renderOneWireframe(color);
		return;
	}

	if (!mSilhouetteExists)
	{
		return;
	}

	BOOL is_hud_object = objectp->isHUDAttachment();
	
	if (mSilhouetteVertices.size() == 0 || mSilhouetteNormals.size() != mSilhouetteVertices.size())
	{
		return;
	}


	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	if (shader)
	{ //switch to "solid color" program for SH-2690 -- works around driver bug causing bad triangles when rendering silhouettes
		gSolidColorProgram.bind();
	}

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.pushUIMatrix();
	gGL.loadUIIdentity();

	if (!is_hud_object)
	{
		gGL.loadIdentity();
		gGL.multMatrix(gGLModelView);
	}
	
	
	if (drawable->isActive())
	{
		gGL.multMatrix((F32*) objectp->getRenderMatrix().mMatrix);
	}

	LLVolume *volume = objectp->getVolume();
	if (volume)
	{
		F32 silhouette_thickness;
		if (isAgentAvatarValid() && is_hud_object)
		{
			silhouette_thickness = LLSelectMgr::sHighlightThickness / gAgentCamera.mHUDCurZoom;
		}
		else
		{
			LLVector3 view_vector = LLViewerCamera::getInstance()->getOrigin() - objectp->getRenderPosition();
			silhouette_thickness = view_vector.magVec() * LLSelectMgr::sHighlightThickness * (LLViewerCamera::getInstance()->getView() / LLViewerCamera::getInstance()->getDefaultFOV());
		}		
		F32 animationTime = (F32)LLFrameTimer::getElapsedSeconds();

		F32 u_coord = fmod(animationTime * LLSelectMgr::sHighlightUAnim, 1.f);
		F32 v_coord = 1.f - fmod(animationTime * LLSelectMgr::sHighlightVAnim, 1.f);
		F32 u_divisor = 1.f / ((F32)(mSilhouetteVertices.size() - 1));

		if (LLSelectMgr::sRenderHiddenSelections) // && gFloaterTools && gFloaterTools->getVisible())
		{
			gGL.flush();
			gGL.blendFunc(LLRender::BF_SOURCE_COLOR, LLRender::BF_ONE);
			LLGLEnable fog(GL_FOG);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			float d = (LLViewerCamera::getInstance()->getPointOfInterest()-LLViewerCamera::getInstance()->getOrigin()).magVec();
			LLColor4 fogCol = color * (F32)llclamp((LLSelectMgr::getInstance()->getSelectionCenterGlobal()-gAgentCamera.getCameraPositionGlobal()).magVec()/(LLSelectMgr::getInstance()->getBBoxOfSelection().getExtentLocal().magVec()*4), 0.0, 1.0);
			glFogf(GL_FOG_START, d);
			glFogf(GL_FOG_END, d*(1 + (LLViewerCamera::getInstance()->getView() / LLViewerCamera::getInstance()->getDefaultFOV())));
			glFogfv(GL_FOG_COLOR, fogCol.mV);

			LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GEQUAL);
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
			gGL.begin(LLRender::LINES);
			{
				for(S32 i = 0; i < mSilhouetteVertices.size(); i += 2)
				{
					u_coord += u_divisor * LLSelectMgr::sHighlightUScale;
					gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);
					gGL.texCoord2f( u_coord, v_coord );
					gGL.vertex3fv( mSilhouetteVertices[i].mV);
					u_coord += u_divisor * LLSelectMgr::sHighlightUScale;
					gGL.texCoord2f( u_coord, v_coord );
					gGL.vertex3fv(mSilhouetteVertices[i+1].mV);
				}
			}
            gGL.end();
			u_coord = fmod(animationTime * LLSelectMgr::sHighlightUAnim, 1.f);
		}

		gGL.flush();
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
		gGL.begin(LLRender::TRIANGLES);
		{
			for(S32 i = 0; i < mSilhouetteVertices.size(); i+=2)
			{
				if (!mSilhouetteNormals[i].isFinite() ||
					!mSilhouetteNormals[i+1].isFinite())
				{ //skip skewed segments
					continue;
				}

				LLVector3 v[4];
				LLVector2 tc[4];
				v[0] = mSilhouetteVertices[i] + (mSilhouetteNormals[i] * silhouette_thickness);
				tc[0].set(u_coord, v_coord + LLSelectMgr::sHighlightVScale);

				v[1] = mSilhouetteVertices[i];
				tc[1].set(u_coord, v_coord);

				u_coord += u_divisor * LLSelectMgr::sHighlightUScale;

				v[2] = mSilhouetteVertices[i+1] + (mSilhouetteNormals[i+1] * silhouette_thickness);
				tc[2].set(u_coord, v_coord + LLSelectMgr::sHighlightVScale);
				
				v[3] = mSilhouetteVertices[i+1];
				tc[3].set(u_coord,v_coord);

				gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.0f); //LLSelectMgr::sHighlightAlpha);
				gGL.texCoord2fv(tc[0].mV);
				gGL.vertex3fv( v[0].mV ); 
				
				gGL.color4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha*2);
				gGL.texCoord2fv( tc[1].mV );
				gGL.vertex3fv( v[1].mV );

				gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.0f); //LLSelectMgr::sHighlightAlpha);
				gGL.texCoord2fv( tc[2].mV );
				gGL.vertex3fv( v[2].mV );

				gGL.vertex3fv( v[2].mV );

				gGL.color4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha*2);
				gGL.texCoord2fv( tc[1].mV );
				gGL.vertex3fv( v[1].mV );

				gGL.texCoord2fv( tc[3].mV );
				gGL.vertex3fv( v[3].mV );			
			}
		}
		gGL.end();
		gGL.flush();
	}
	gGL.popMatrix();
	gGL.popUIMatrix();

	if (shader)
	{
		shader->bind();
	}
}

//
// Utility Functions
//

// *DEPRECATED: See header comment.
void dialog_refresh_all()
{
	// This is the easiest place to fire the update signal, as it will
	// make cleaning up the functions below easier.  Also, sometimes entities
	// outside the selection manager change properties of selected objects
	// and call into this function.  Yuck.
	LLSelectMgr::getInstance()->mUpdateSignal();

	// *TODO: Eliminate all calls into outside classes below, make those
	// objects register with the update signal.

	gFloaterTools->dirty();

	gMenuObject->needsArrange();

	if( gMenuAttachmentSelf->getVisible() )
	{
		gMenuAttachmentSelf->arrange();
	}
	if( gMenuAttachmentOther->getVisible() )
	{
		gMenuAttachmentOther->arrange();
	}

	LLFloaterProperties::dirtyAll();

	LLFloaterInspect* inspect_instance = LLFloaterReg::getTypedInstance<LLFloaterInspect>("inspect");
	if(inspect_instance)
	{
		inspect_instance->dirty();
	}

	LLSidepanelTaskInfo *panel_task_info = LLSidepanelTaskInfo::getActivePanel();
	if (panel_task_info)
	{
		panel_task_info->dirty();
	}
}

S32 get_family_count(LLViewerObject *parent)
{
	if (!parent)
	{
		llwarns << "Trying to get_family_count on null parent!" << llendl;
	}
	S32 count = 1;	// for this object
	LLViewerObject::const_child_list_t& child_list = parent->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;

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
			if (LLSelectMgr::getInstance()->canSelectObject(child))
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

	//override any object updates received
	//for selected objects
	overrideObjectUpdates();

	LLViewerObject* object = mSelectedObjects->getFirstObject();
	if (!object)
	{
		// nothing selected, probably grabbing
		// Ignore by setting to avatar origin.
		mSelectionCenterGlobal.clearVec();
		mShowSelection = FALSE;
		mSelectionBBox = LLBBox(); 
		mPauseRequest = NULL;
		resetAgentHUDZoom();

	}
	else
	{
		mSelectedObjects->mSelectType = getSelectTypeForObject(object);

		if (mSelectedObjects->mSelectType == SELECT_TYPE_ATTACHMENT && isAgentAvatarValid())
		{
			mPauseRequest = gAgentAvatarp->requestPause();
		}
		else
		{
			mPauseRequest = NULL;
		}

		if (mSelectedObjects->mSelectType != SELECT_TYPE_HUD && isAgentAvatarValid())
		{
			// reset hud ZOOM
			gAgentCamera.mHUDTargetZoom = 1.f;
			gAgentCamera.mHUDCurZoom = 1.f;
		}

		mShowSelection = FALSE;
		LLBBox bbox;

		// have stuff selected
		LLVector3d select_center;
		// keep a list of jointed objects for showing the joint HUDEffects

		// Initialize the bounding box to the root prim, so the BBox orientation 
		// matches the root prim's (affecting the orientation of the manipulators). 
		bbox.addBBoxAgent( (mSelectedObjects->getFirstRootObject(TRUE))->getBoundingBoxAgent() ); 
	                 
		std::vector < LLViewerObject *> jointed_objects;

		for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
			 iter != mSelectedObjects->end(); iter++)
		{
			LLSelectNode* node = *iter;
			LLViewerObject* object = node->getObject();
			if (!object)
				continue;
			
			LLViewerObject *root = object->getRootEdit();
			if (mSelectedObjects->mSelectType == SELECT_TYPE_WORLD && // not an attachment
				!root->isChild(gAgentAvatarp) && // not the object you're sitting on
				!object->isAvatar()) // not another avatar
			{
				mShowSelection = TRUE;
			}

			bbox.addBBoxAgent( object->getBoundingBoxAgent() );

			if (object->isJointChild())
			{
				jointed_objects.push_back(object);
			}
		}
		
		LLVector3 bbox_center_agent = bbox.getCenterAgent();
		mSelectionCenterGlobal = gAgent.getPosGlobalFromAgent(bbox_center_agent);
		mSelectionBBox = bbox;

	}
	
	if ( !(gAgentID == LLUUID::null)) 
	{
		LLTool		*tool = LLToolMgr::getInstance()->getCurrentTool();
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
			const LLPickInfo& pick = gViewerWindow->getLastPick();
			LLViewerObject *click_object = pick.getObject();
			if (click_object && click_object->isSelected())
			{
				// clicked on another object in our selection group, use that as target
				select_offset.setVec(pick.mObjectOffset);
				select_offset.rotVec(~click_object->getRenderRotation());
		
				gAgentCamera.setPointAt(POINTAT_TARGET_SELECT, click_object, select_offset);
				gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, click_object, select_offset);
			}
			else
			{
				// didn't click on an object this time, revert to pointing at center of first object
				gAgentCamera.setPointAt(POINTAT_TARGET_SELECT, mSelectedObjects->getFirstObject());
				gAgentCamera.setLookAt(LOOKAT_TARGET_SELECT, mSelectedObjects->getFirstObject());
			}
		}
		else
		{
			gAgentCamera.setPointAt(POINTAT_TARGET_CLEAR);
			gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);
		}
	}
	else
	{
		gAgentCamera.setPointAt(POINTAT_TARGET_CLEAR);
		gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);
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
BOOL LLSelectMgr::canUndo() const
{
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstEditableObject() != NULL; // HACK: casting away constness - MG
}

//-----------------------------------------------------------------------------
// undo()
//-----------------------------------------------------------------------------
void LLSelectMgr::undo()
{
	BOOL select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Undo", packAgentAndSessionAndGroupID, packObjectID, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canRedo()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canRedo() const
{
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstEditableObject() != NULL; // HACK: casting away constness - MG
}

//-----------------------------------------------------------------------------
// redo()
//-----------------------------------------------------------------------------
void LLSelectMgr::redo()
{
	BOOL select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Redo", packAgentAndSessionAndGroupID, packObjectID, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canDoDelete()
//-----------------------------------------------------------------------------
BOOL LLSelectMgr::canDoDelete() const
{
	bool can_delete = false;
	// This function is "logically const" - it does not change state in
	// a way visible outside the selection manager.
	LLSelectMgr* self = const_cast<LLSelectMgr*>(this);
	LLViewerObject* obj = self->mSelectedObjects->getFirstDeleteableObject();
	// Note: Can only delete root objects (see getFirstDeleteableObject() for more info)
	if (obj!= NULL)
	{
		// all the faces needs to be selected
		if(self->mSelectedObjects->contains(obj,SELECT_ALL_TES ))
		{
			can_delete = true;
		}
	}
	
	return can_delete;
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
BOOL LLSelectMgr::canDeselect() const
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
BOOL LLSelectMgr::canDuplicate() const
{
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstCopyableObject() != NULL; // HACK: casting away constness - MG
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
	struct f : public LLSelectedObjectFunctor
	{
		virtual bool apply(LLViewerObject* object)
		{
			if (!LLSelectMgr::getInstance()->canSelectObject(object))
			{
				LLSelectMgr::getInstance()->deselectObjectOnly(object);
			}
			return true;
		}
	} func;
	getSelection()->applyToObjects(&func);	
}

BOOL LLSelectMgr::canSelectObject(LLViewerObject* object)
{
	// Never select dead objects
	if (!object || object->isDead())
	{
		return FALSE;
	}
	
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

BOOL LLSelectMgr::setForceSelection(BOOL force) 
{ 
	std::swap(mForceSelection,force); 
	return force; 
}

void LLSelectMgr::resetAgentHUDZoom()
{
	gAgentCamera.mHUDTargetZoom = 1.f;
	gAgentCamera.mHUDCurZoom = 1.f;
}

void LLSelectMgr::getAgentHUDZoom(F32 &target_zoom, F32 &current_zoom) const
{
	target_zoom = gAgentCamera.mHUDTargetZoom;
	current_zoom = gAgentCamera.mHUDCurZoom;
}

void LLSelectMgr::setAgentHUDZoom(F32 target_zoom, F32 current_zoom)
{
	gAgentCamera.mHUDTargetZoom = target_zoom;
	gAgentCamera.mHUDCurZoom = current_zoom;
}

/////////////////////////////////////////////////////////////////////////////
// Object selection iterator helpers
/////////////////////////////////////////////////////////////////////////////
bool LLObjectSelection::is_root::operator()(LLSelectNode *node)
{
	LLViewerObject* object = node->getObject();
	return (object != NULL) && !node->mIndividualSelection && (object->isRootEdit() || object->isJointChild());
}

bool LLObjectSelection::is_valid_root::operator()(LLSelectNode *node)
{
	LLViewerObject* object = node->getObject();
	return (object != NULL) && node->mValid && !node->mIndividualSelection && (object->isRootEdit() || object->isJointChild());
}

bool LLObjectSelection::is_root_object::operator()(LLSelectNode *node)
{
	LLViewerObject* object = node->getObject();
	return (object != NULL) && (object->isRootEdit() || object->isJointChild());
}

LLObjectSelection::LLObjectSelection() : 
	LLRefCount(),
	mSelectType(SELECT_TYPE_WORLD)
{
}

LLObjectSelection::~LLObjectSelection()
{
	deleteAllNodes();
}

void LLObjectSelection::cleanupNodes()
{
	for (list_t::iterator iter = mList.begin(); iter != mList.end(); )
	{
		list_t::iterator curiter = iter++;
		LLSelectNode* node = *curiter;
		if (node->getObject() == NULL || node->getObject()->isDead())
		{
			mList.erase(curiter);
			delete node;
		}
	}
}

void LLObjectSelection::updateEffects()
{
}

S32 LLObjectSelection::getNumNodes()
{
	return mList.size();
}

void LLObjectSelection::addNode(LLSelectNode *nodep)
{
	llassert_always(nodep->getObject() && !nodep->getObject()->isDead());
	mList.push_front(nodep);
	mSelectNodeMap[nodep->getObject()] = nodep;
}

void LLObjectSelection::addNodeAtEnd(LLSelectNode *nodep)
{
	llassert_always(nodep->getObject() && !nodep->getObject()->isDead());
	mList.push_back(nodep);
	mSelectNodeMap[nodep->getObject()] = nodep;
}

void LLObjectSelection::moveNodeToFront(LLSelectNode *nodep)
{
	mList.remove(nodep);
	mList.push_front(nodep);
}

void LLObjectSelection::removeNode(LLSelectNode *nodep)
{
	mSelectNodeMap.erase(nodep->getObject());
	if (nodep->getObject() == mPrimaryObject)
	{
		mPrimaryObject = NULL;
	}
	nodep->setObject(NULL); // Will get erased in cleanupNodes()
	mList.remove(nodep);
}

void LLObjectSelection::deleteAllNodes()
{
	std::for_each(mList.begin(), mList.end(), DeletePointer());
	mList.clear();
	mSelectNodeMap.clear();
	mPrimaryObject = NULL;
}

LLSelectNode* LLObjectSelection::findNode(LLViewerObject* objectp)
{
	std::map<LLPointer<LLViewerObject>, LLSelectNode*>::iterator found_it = mSelectNodeMap.find(objectp);
	if (found_it != mSelectNodeMap.end())
	{
		return found_it->second;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// isEmpty()
//-----------------------------------------------------------------------------
BOOL LLObjectSelection::isEmpty() const
{
	return (mList.size() == 0);
}


//-----------------------------------------------------------------------------
// getObjectCount() - returns number of non null objects
//-----------------------------------------------------------------------------
S32 LLObjectSelection::getObjectCount()
{
	cleanupNodes();
	S32 count = mList.size();

	return count;
}

F32 LLObjectSelection::getSelectedObjectCost()
{
	cleanupNodes();
	F32 cost = 0.f;

	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			cost += object->getObjectCost();
		}
	}

	return cost;
}

F32 LLObjectSelection::getSelectedLinksetCost()
{
	cleanupNodes();
	F32 cost = 0.f;

	std::set<LLViewerObject*> me_roots;

	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			LLViewerObject* root = static_cast<LLViewerObject*>(object->getRoot());
			if (root)
			{
				if (me_roots.find(root) == me_roots.end())
				{
					me_roots.insert(root);
					cost += root->getLinksetCost();
				}
			}
		}
	}

	return cost;
}

F32 LLObjectSelection::getSelectedPhysicsCost()
{
	cleanupNodes();
	F32 cost = 0.f;

	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			cost += object->getPhysicsCost();
		}
	}

	return cost;
}

F32 LLObjectSelection::getSelectedLinksetPhysicsCost()
{
	cleanupNodes();
	F32 cost = 0.f;

	std::set<LLViewerObject*> me_roots;

	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			LLViewerObject* root = static_cast<LLViewerObject*>(object->getRoot());
			if (root)
			{
				if (me_roots.find(root) == me_roots.end())
				{
					me_roots.insert(root);
					cost += root->getLinksetPhysicsCost();
				}
			}
		}
	}

	return cost;
}

F32 LLObjectSelection::getSelectedObjectStreamingCost(S32* total_bytes, S32* visible_bytes)
{
	F32 cost = 0.f;
	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			S32 bytes = 0;
			S32 visible = 0;
			cost += object->getStreamingCost(&bytes, &visible);

			if (total_bytes)
			{
				*total_bytes += bytes;
			}

			if (visible_bytes)
			{
				*visible_bytes += visible;
			}
		}
	}

	return cost;
}

U32 LLObjectSelection::getSelectedObjectTriangleCount(S32* vcount)
{
	U32 count = 0;
	for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		
		if (object)
		{
			count += object->getTriangleCount(vcount);
		}
	}

	return count;
}

S32 LLObjectSelection::getSelectedObjectRenderCost()
{
       S32 cost = 0;
       LLVOVolume::texture_cost_t textures;
       typedef std::set<LLUUID> uuid_list_t;
       uuid_list_t computed_objects;

	   typedef std::list<LLPointer<LLViewerObject> > child_list_t;
	   typedef const child_list_t const_child_list_t;

	   // add render cost of complete linksets first, to get accurate texture counts
       for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
       {
               LLSelectNode* node = *iter;
			   
               LLVOVolume* object = (LLVOVolume*)node->getObject();

               if (object && object->isRootEdit())
               {
				   cost += object->getRenderCost(textures);
				   computed_objects.insert(object->getID());

				   const_child_list_t children = object->getChildren();
				   for (const_child_list_t::const_iterator child_iter = children.begin();
						 child_iter != children.end();
						 ++child_iter)
				   {
					   LLViewerObject* child_obj = *child_iter;
					   LLVOVolume *child = dynamic_cast<LLVOVolume*>( child_obj );
					   if (child)
					   {
						   cost += child->getRenderCost(textures);
						   computed_objects.insert(child->getID());
					   }
				   }

				   for (LLVOVolume::texture_cost_t::iterator iter = textures.begin(); iter != textures.end(); ++iter)
				   {
					   // add the cost of each individual texture in the linkset
					   cost += iter->second;
				   }

				   textures.clear();
               }
       }
	
	   // add any partial linkset objects, texture cost may be slightly misleading
		for (list_t::iterator iter = mList.begin(); iter != mList.end(); ++iter)
		{
			LLSelectNode* node = *iter;
			LLVOVolume* object = (LLVOVolume*)node->getObject();

			if (object && computed_objects.find(object->getID()) == computed_objects.end()  )
			{
					cost += object->getRenderCost(textures);
					computed_objects.insert(object->getID());
			}

			for (LLVOVolume::texture_cost_t::iterator iter = textures.begin(); iter != textures.end(); ++iter)
			{
				// add the cost of each individual texture in the linkset
				cost += iter->second;
			}

			textures.clear();
		}

       return cost;
}

//-----------------------------------------------------------------------------
// getTECount()
//-----------------------------------------------------------------------------
S32 LLObjectSelection::getTECount()
{
	S32 count = 0;
	for (LLObjectSelection::iterator iter = begin(); iter != end(); iter++)
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if (!object)
			continue;
		S32 num_tes = object->getNumTEs();
		for (S32 te = 0; te < num_tes; te++)
		{
			if (node->isTESelected(te))
			{
				++count;
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
	S32 count = 0;
	for (LLObjectSelection::root_iterator iter = root_begin(); iter != root_end(); iter++)
	{
		++count;
	}
	return count;
}

bool LLObjectSelection::applyToObjects(LLSelectedObjectFunctor* func)
{
	bool result = true;
	for (iterator iter = begin(); iter != end(); )
	{
		iterator nextiter = iter++;
		LLViewerObject* object = (*nextiter)->getObject();
		if (!object)
			continue;
		bool r = func->apply(object);
		result = result && r;
	}
	return result;
}

bool LLObjectSelection::applyToRootObjects(LLSelectedObjectFunctor* func, bool firstonly)
{
	bool result = firstonly ? false : true;
	for (root_iterator iter = root_begin(); iter != root_end(); )
	{
		root_iterator nextiter = iter++;
		LLViewerObject* object = (*nextiter)->getObject();
		if (!object)
			continue;
		bool r = func->apply(object);
		if (firstonly && r)
			return true;
		else
			result = result && r;
	}
	return result;
}

bool LLObjectSelection::applyToTEs(LLSelectedTEFunctor* func, bool firstonly)
{
	bool result = firstonly ? false : true;
	for (iterator iter = begin(); iter != end(); )
	{
		iterator nextiter = iter++;
		LLSelectNode* node = *nextiter;
		LLViewerObject* object = (*nextiter)->getObject();
		if (!object)
			continue;
		S32 num_tes = llmin((S32)object->getNumTEs(), (S32)object->getNumFaces()); // avatars have TEs but no faces
		for (S32 te = 0; te < num_tes; ++te)
		{
			if (node->isTESelected(te))
			{
				bool r = func->apply(object, te);
				if (firstonly && r)
					return true;
				else
					result = result && r;
			}
		}
	}
	return result;
}

bool LLObjectSelection::applyToNodes(LLSelectedNodeFunctor *func, bool firstonly)
{
	bool result = firstonly ? false : true;
	for (iterator iter = begin(); iter != end(); )
	{
		iterator nextiter = iter++;
		LLSelectNode* node = *nextiter;
		bool r = func->apply(node);
		if (firstonly && r)
			return true;
		else
			result = result && r;
	}
	return result;
}

bool LLObjectSelection::applyToRootNodes(LLSelectedNodeFunctor *func, bool firstonly)
{
	bool result = firstonly ? false : true;
	for (root_iterator iter = root_begin(); iter != root_end(); )
	{
		root_iterator nextiter = iter++;
		LLSelectNode* node = *nextiter;
		bool r = func->apply(node);
		if (firstonly && r)
			return true;
		else
			result = result && r;
	}
	return result;
}

BOOL LLObjectSelection::isMultipleTESelected()
{
	BOOL te_selected = FALSE;
	// ...all faces
	for (LLObjectSelection::iterator iter = begin();
		 iter != end(); iter++)
	{
		LLSelectNode* nodep = *iter;
		for (S32 i = 0; i < SELECT_MAX_TES; i++)
		{
			if(nodep->isTESelected(i))
			{
				if(te_selected)
				{
					return TRUE;
				}
				te_selected = TRUE;
			}
		}
	}
	return FALSE;
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
	if (te == SELECT_ALL_TES)
	{
		// ...all faces
		for (LLObjectSelection::iterator iter = begin();
			 iter != end(); iter++)
		{
			LLSelectNode* nodep = *iter;
			if (nodep->getObject() == object)
			{
				// Optimization
				if (nodep->getTESelectMask() == TE_SELECT_MASK_ALL)
				{
					return TRUE;
				}

				BOOL all_selected = TRUE;
				for (S32 i = 0; i < object->getNumTEs(); i++)
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
		for (LLObjectSelection::iterator iter = begin(); iter != end(); iter++)
		{
			LLSelectNode* nodep = *iter;
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
// getSelectedParentObject()
//-----------------------------------------------------------------------------
LLViewerObject* getSelectedParentObject(LLViewerObject *object)
{
	LLViewerObject *parent;
	while (object && (parent = (LLViewerObject*)object->getParent()))
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
	return object;
}

//-----------------------------------------------------------------------------
// getFirstNode
//-----------------------------------------------------------------------------
LLSelectNode* LLObjectSelection::getFirstNode(LLSelectedNodeFunctor* func)
{
	for (iterator iter = begin(); iter != end(); ++iter)
	{
		LLSelectNode* node = *iter;
		if (func == NULL || func->apply(node))
		{
			return node;
		}
	}
	return NULL;
}

LLSelectNode* LLObjectSelection::getFirstRootNode(LLSelectedNodeFunctor* func, BOOL non_root_ok)
{
	for (root_iterator iter = root_begin(); iter != root_end(); ++iter)
	{
		LLSelectNode* node = *iter;
		if (func == NULL || func->apply(node))
		{
			return node;
		}
	}
	if (non_root_ok)
	{
		// Get non root
		return getFirstNode(func);
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// getFirstSelectedObject
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstSelectedObject(LLSelectedNodeFunctor* func, BOOL get_parent)
{
	LLSelectNode* res = getFirstNode(func);
	if (res && get_parent)
	{
		return getSelectedParentObject(res->getObject());
	}
	else if (res)
	{
		return res->getObject();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// getFirstObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstObject()
{
	LLSelectNode* res = getFirstNode(NULL);
	return res ? res->getObject() : NULL;
}

//-----------------------------------------------------------------------------
// getFirstRootObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstRootObject(BOOL non_root_ok)
{
	LLSelectNode* res = getFirstRootNode(NULL, non_root_ok);
	return res ? res->getObject() : NULL;
}

//-----------------------------------------------------------------------------
// getFirstMoveableNode()
//-----------------------------------------------------------------------------
LLSelectNode* LLObjectSelection::getFirstMoveableNode(BOOL get_root_first)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permMove();
		}
	} func;
	LLSelectNode* res = get_root_first ? getFirstRootNode(&func, TRUE) : getFirstNode(&func);
	return res;
}

//-----------------------------------------------------------------------------
// getFirstCopyableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstCopyableObject(BOOL get_parent)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permCopy() && !obj->isAttachment();
		}
	} func;
	return getFirstSelectedObject(&func, get_parent);
}

//-----------------------------------------------------------------------------
// getFirstDeleteableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstDeleteableObject()
{
	//RN: don't currently support deletion of child objects, as that requires separating them first
	// then derezzing to trash
	
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			// you can delete an object if you are the owner
			// or you have permission to modify it.
			if( obj && ( (obj->permModify()) ||
						 (obj->permYouOwner()) ||
						 (!obj->permAnyOwner())	))		// public
			{
				if( !obj->isAttachment() )
				{
					return true;
				}
			}
			return false;
		}
	} func;
	LLSelectNode* node = getFirstNode(&func);
	return node ? node->getObject() : NULL;
}

//-----------------------------------------------------------------------------
// getFirstEditableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstEditableObject(BOOL get_parent)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permModify();
		}
	} func;
	return getFirstSelectedObject(&func, get_parent);
}

//-----------------------------------------------------------------------------
// getFirstMoveableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstMoveableObject(BOOL get_parent)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permMove();
		}
	} func;
	return getFirstSelectedObject(&func, get_parent);
}

//-----------------------------------------------------------------------------
// Position + Rotation update methods called from LLViewerJoystick
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectionMove(const LLVector3& displ,
                                  F32 roll, F32 pitch, F32 yaw, U32 update_type)
{
	if (update_type == UPD_NONE)
	{
		return false;
	}
	
	LLVector3 displ_global;
	bool update_success = true;
	bool update_position = update_type & UPD_POSITION;
	bool update_rotation = update_type & UPD_ROTATION;
	const bool noedit_linked_parts = !gSavedSettings.getBOOL("EditLinkedParts");
	
	if (update_position)
	{
		// calculate the distance of the object closest to the camera origin
		F32 min_dist_squared = F32_MAX; // value will be overridden in the loop
		
		LLVector3 obj_pos;
		for (LLObjectSelection::root_iterator it = getSelection()->root_begin();
			 it != getSelection()->root_end(); ++it)
		{
			obj_pos = (*it)->getObject()->getPositionEdit();
			
			F32 obj_dist_squared = dist_vec_squared(obj_pos, LLViewerCamera::getInstance()->getOrigin());
			if (obj_dist_squared < min_dist_squared)
			{
				min_dist_squared = obj_dist_squared;
			}
		}
		
		// factor the distance into the displacement vector. This will get us
		// equally visible movements for both close and far away selections.
		F32 min_dist = sqrt((F32) sqrtf(min_dist_squared)) / 2;
		displ_global.setVec(displ.mV[0] * min_dist,
							displ.mV[1] * min_dist,
							displ.mV[2] * min_dist);

		// equates to: Displ_global = Displ * M_cam_axes_in_global_frame
		displ_global = LLViewerCamera::getInstance()->rotateToAbsolute(displ_global);
	}

	LLQuaternion new_rot;
	if (update_rotation)
	{
		// let's calculate the rotation around each camera axes 
		LLQuaternion qx(roll, LLViewerCamera::getInstance()->getAtAxis());
		LLQuaternion qy(pitch, LLViewerCamera::getInstance()->getLeftAxis());
		LLQuaternion qz(yaw, LLViewerCamera::getInstance()->getUpAxis());
		new_rot.setQuat(qx * qy * qz);
	}
	
	LLViewerObject *obj;
	S32 obj_count = getSelection()->getObjectCount();
	for (LLObjectSelection::root_iterator it = getSelection()->root_begin();
		 it != getSelection()->root_end(); ++it )
	{
		obj = (*it)->getObject();
		bool enable_pos = false, enable_rot = false;
		bool perm_move = obj->permMove();
		bool perm_mod = obj->permModify();
		
		LLVector3d sel_center(getSelectionCenterGlobal());
		
		if (update_rotation)
		{
			enable_rot = perm_move 
				&& ((perm_mod && !obj->isAttachment()) || noedit_linked_parts);

			if (enable_rot)
			{
				int children_count = obj->getChildren().size();
				if (obj_count > 1 && children_count > 0)
				{
					// for linked sets, rotate around the group center
					const LLVector3 t(obj->getPositionGlobal() - sel_center);

					// Ra = T x R x T^-1
					LLMatrix4 mt;	mt.setTranslation(t);
					const LLMatrix4 mnew_rot(new_rot);
					LLMatrix4 mt_1;	mt_1.setTranslation(-t);
					mt *= mnew_rot;
					mt *= mt_1;
					
					// Rfin = Rcur * Ra
					obj->setRotation(obj->getRotationEdit() * mt.quaternion());
					displ_global += mt.getTranslation();
				}
				else
				{
					obj->setRotation(obj->getRotationEdit() * new_rot);
				}
			}
			else
			{
				update_success = false;
			}
		}

		if (update_position)
		{
			// establish if object can be moved or not
			enable_pos = perm_move && !obj->isAttachment() 
			&& (perm_mod || noedit_linked_parts);
			
			if (enable_pos)
			{
				obj->setPosition(obj->getPositionEdit() + displ_global);
			}
			else
			{
				update_success = false;
			}
		}
		
		if (enable_pos && enable_rot && obj->mDrawable.notNull())
		{
			gPipeline.markMoved(obj->mDrawable, TRUE);
		}
	}
	
	if (update_position && update_success && obj_count > 1)
	{
		updateSelectionCenter();
	}
	
	return update_success;
}

void LLSelectMgr::sendSelectionMove()
{
	LLSelectNode *node = mSelectedObjects->getFirstRootNode();
	if (node == NULL)
	{
		return;
	}
	
	//saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
	
	U32 update_type = UPD_POSITION | UPD_ROTATION;
	LLViewerRegion *last_region, *curr_region = node->getObject()->getRegion();
	S32 objects_in_this_packet = 0;

	// apply to linked objects if unable to select their individual parts 
	if (!gSavedSettings.getBOOL("EditLinkedParts") && !getTEMode())
	{
		// tell simulator to apply to whole linked sets
		update_type |= UPD_LINKED_SETS;
	}

	// prepare first bulk message
	gMessageSystem->newMessage("MultipleObjectUpdate");
	packAgentAndSessionID(&update_type);

	LLViewerObject *obj = NULL;
	for (LLObjectSelection::root_iterator it = getSelection()->root_begin();
		 it != getSelection()->root_end(); ++it)
	{
		obj = (*it)->getObject();

		// note: following code adapted from sendListToRegions() (@3924)
		last_region = curr_region;
		curr_region = obj->getRegion();

		// if not simulator or message too big
		if (curr_region != last_region
			|| gMessageSystem->isSendFull(NULL)
			|| objects_in_this_packet >= MAX_OBJECTS_PER_PACKET)
		{
			// send sim the current message and start new one
			gMessageSystem->sendReliable(last_region->getHost());
			objects_in_this_packet = 0;
			gMessageSystem->newMessage("MultipleObjectUpdate");
			packAgentAndSessionID(&update_type);
		}

		// add another instance of the body of data
		packMultipleUpdate(*it, &update_type);
		++objects_in_this_packet;
	}

	// flush remaining messages
	if (gMessageSystem->getCurrentSendTotal() > 0)
	{
		gMessageSystem->sendReliable(curr_region->getHost());
	}
	else
	{
		gMessageSystem->clearMessage();
	}

	//saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
}
