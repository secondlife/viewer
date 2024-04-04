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
#include "llmaterialmgr.h"

// library includes
#include "llcachename.h"
#include "llavatarnamecache.h"
#include "lldbstrings.h"
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
#include "llcontrolavatar.h"
#include "message.h"
#include "object_flags.h"
#include "llquaternion.h"

// viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llattachmentsmgr.h"
#include "llviewerwindow.h"
#include "lldrawable.h"
#include "llfloaterinspect.h"
#include "llfloaterreporter.h"
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llframetimer.h"
#include "llfocusmgr.h"
#include "llgltfmateriallist.h"
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
#include "llpanelface.h"
#include "llglheaders.h"
#include "llinventoryobserver.h"

LLViewerObject* getSelectedParentObject(LLViewerObject *object) ;
//
// Consts
//

const F32 SILHOUETTE_UPDATE_THRESHOLD_SQUARED = 0.02f;
const S32 MAX_SILS_PER_FRAME = 50;
const S32 MAX_OBJECTS_PER_PACKET = 254;
// For linked sets
const S32 MAX_CHILDREN_PER_TASK = 255;

//
// Globals
//

//bool gDebugSelectMgr = false;

//bool gHideSelectedObjects = false;
//bool gAllowSelectAvatar = false;

bool LLSelectMgr::sRectSelectInclusive = true;
bool LLSelectMgr::sRenderHiddenSelections = true;
bool LLSelectMgr::sRenderLightRadius = false;
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

//-----------------------------------------------------------------------------
// ~LLSelectionCallbackData()
//-----------------------------------------------------------------------------

LLSelectionCallbackData::LLSelectionCallbackData()
{
    LLSelectMgr *instance = LLSelectMgr::getInstance();
    LLObjectSelectionHandle selection = instance->getSelection();
    if (!selection->getNumNodes())
    {
        return;
    }
    mSelectedObjects = new LLObjectSelection();

    for (LLObjectSelection::iterator iter = selection->begin();
        iter != selection->end();)
    {
        LLObjectSelection::iterator curiter = iter++;

        LLSelectNode *nodep = *curiter;
        LLViewerObject* objectp = nodep->getObject();

        if (!objectp)
        {
            mSelectedObjects->mSelectType = SELECT_TYPE_WORLD;
        }
        else
        {
            LLSelectNode* new_nodep = new LLSelectNode(*nodep);
            mSelectedObjects->addNode(new_nodep);

            if (objectp->isHUDAttachment())
            {
                mSelectedObjects->mSelectType = SELECT_TYPE_HUD;
            }
            else if (objectp->isAttachment())
            {
                mSelectedObjects->mSelectType = SELECT_TYPE_ATTACHMENT;
            }
            else
            {
                mSelectedObjects->mSelectType = SELECT_TYPE_WORLD;
            }
        }
    }
}


//
// Functions
//

void LLSelectMgr::cleanupGlobals()
{
	LLSelectMgr::getInstance()->clearSelections();
}

//-----------------------------------------------------------------------------
// LLSelectMgr()
//-----------------------------------------------------------------------------
LLSelectMgr::LLSelectMgr()
 : mHideSelectedObjects(LLCachedControl<bool>(gSavedSettings, "HideSelectedObjects", false)),
   mRenderHighlightSelections(LLCachedControl<bool>(gSavedSettings, "RenderHighlightSelections", true)),
   mAllowSelectAvatar( LLCachedControl<bool>(gSavedSettings, "AllowSelectAvatar", false)),
   mDebugSelectMgr(LLCachedControl<bool>(gSavedSettings, "DebugSelectMgr", false))
{
	mTEMode = false;
	mTextureChannel = LLRender::DIFFUSE_MAP;
	mLastCameraPos.clearVec();

	sHighlightThickness	= gSavedSettings.getF32("SelectionHighlightThickness");
	sHighlightUScale	= gSavedSettings.getF32("SelectionHighlightUScale");
	sHighlightVScale	= gSavedSettings.getF32("SelectionHighlightVScale");
	sHighlightAlpha		= gSavedSettings.getF32("SelectionHighlightAlpha") * 2;
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
	
	mRenderSilhouettes = true;

	mGridMode = GRID_MODE_WORLD;
	gSavedSettings.setS32("GridMode", (S32)GRID_MODE_WORLD);

	mSelectedObjects = new LLObjectSelection();
	mHoverObjects = new LLObjectSelection();
	mHighlightedObjects = new LLObjectSelection();

	mForceSelection = false;
	mShowSelection = false;
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

	LLPipeline::setRenderHighlightTextureChannel(LLRender::DIFFUSE_MAP);
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

void LLSelectMgr::resetObjectOverrides()
{
    resetObjectOverrides(getSelection());
}

void LLSelectMgr::resetObjectOverrides(LLObjectSelectionHandle selected_handle)
{
    struct f : public LLSelectedNodeFunctor
    {
        f(bool a, LLSelectMgr* p) : mAvatarOverridesPersist(a), mManager(p) {}
        bool mAvatarOverridesPersist;
        LLSelectMgr* mManager;
        virtual bool apply(LLSelectNode* node)
        {
            if (mAvatarOverridesPersist)
            {
                LLViewerObject* object = node->getObject();
                if (object && !object->getParent())
                {
                    LLVOAvatar* avatar = object->asAvatar();
                    if (avatar)
                    {
                        mManager->mAvatarOverridesMap.emplace(avatar->getID(), AvatarPositionOverride(node->mLastPositionLocal, node->mLastRotation, object));
                    }
                }
            }
            node->mLastPositionLocal.setVec(0, 0, 0);
            node->mLastRotation = LLQuaternion();
            node->mLastScale.setVec(0, 0, 0);
            return true;
        }
    } func(mAllowSelectAvatar, this);

    selected_handle->applyToNodes(&func);
}

void LLSelectMgr::overrideObjectUpdates()
{
	//override any position updates from simulator on objects being edited
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* selectNode)
		{
			LLViewerObject* object = selectNode->getObject();
			if (object && object->permMove() && !object->isPermanentEnforced())
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

void LLSelectMgr::resetAvatarOverrides()
{
    mAvatarOverridesMap.clear();
}

void LLSelectMgr::overrideAvatarUpdates()
{
    if (mAvatarOverridesMap.size() == 0)
    {
        return;
    }

    if (!mAllowSelectAvatar || !gFloaterTools)
    {
        resetAvatarOverrides();
        return;
    }

    if (!gFloaterTools->getVisible() && getSelection()->isEmpty())
    {
        // when user switches selection, floater is invisible and selection is empty
        LLToolset *toolset = LLToolMgr::getInstance()->getCurrentToolset();
        if (toolset->isShowFloaterTools()
            && toolset->isToolSelected(0)) // Pie tool
        {
            resetAvatarOverrides();
            return;
        }
    }

    // remove selected avatars from this list,
    // but set object overrides to make sure avatar won't snap back 
    struct f : public LLSelectedNodeFunctor
    {
        f(LLSelectMgr* p) : mManager(p) {}
        LLSelectMgr* mManager;
        virtual bool apply(LLSelectNode* selectNode)
        {
            LLViewerObject* object = selectNode->getObject();
            if (object && !object->getParent())
            {
                LLVOAvatar* avatar = object->asAvatar();
                if (avatar)
                {
                    uuid_av_override_map_t::iterator iter = mManager->mAvatarOverridesMap.find(avatar->getID());
                    if (iter != mManager->mAvatarOverridesMap.end())
                    {
                        if (selectNode->mLastPositionLocal.isExactlyZero())
                        {
                            selectNode->mLastPositionLocal = iter->second.mLastPositionLocal;
                        }
                        if (selectNode->mLastRotation == LLQuaternion())
                        {
                            selectNode->mLastRotation = iter->second.mLastRotation;
                        }
                        mManager->mAvatarOverridesMap.erase(iter);
                    }
                }
            }
            return true;
        }
    } func(this);
    getSelection()->applyToNodes(&func);

    // Override avatar positions
    uuid_av_override_map_t::iterator it = mAvatarOverridesMap.begin();
    while (it != mAvatarOverridesMap.end())
    {
        if (it->second.mObject->isDead())
        {
            it = mAvatarOverridesMap.erase(it);
        }
        else
        {
            if (!it->second.mLastPositionLocal.isExactlyZero())
            {
                it->second.mObject->setPosition(it->second.mLastPositionLocal);
            }
            if (it->second.mLastRotation != LLQuaternion())
            {
                it->second.mObject->setRotation(it->second.mLastRotation);
            }
            it++;
        }
    }
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

	// LL_INFOS() << "Adding object to selected object list" << LL_ENDL;

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
LLObjectSelectionHandle LLSelectMgr::selectObjectAndFamily(LLViewerObject* obj, bool add_to_end, bool ignore_select_owned)
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

	if (!canSelectObject(obj,ignore_select_owned))
	{
		//make_ui_sound("UISndInvalidOp");
		return NULL;
	}

	// Since we're selecting a family, start at the root, but
	// don't include an avatar.
	LLViewerObject* root = obj;
	
	while(!root->isAvatar() && root->getParent())
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
		gSavedSettings.setBOOL("EditLinkedParts", false);
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
														   bool send_to_sim)
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
		gSavedSettings.setBOOL("EditLinkedParts", false);
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
bool LLSelectMgr::removeObjectFromSelections(const LLUUID &id)
{
	bool object_found = false;
	LLTool *tool = NULL;

	tool = LLToolMgr::getInstance()->getCurrentTool();

	// It's possible that the tool is editing an object that is not selected
	LLViewerObject* tool_editing_object = tool->getEditingObject();
	if( tool_editing_object && tool_editing_object->mID == id)
	{
		tool->stopEditing();
		object_found = true;
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
				deselectObjectAndFamily(object, false);
				object_found = true;
				break; // must break here, may have removed multiple objects from list
			}
			else if (object->isAvatar() && object->getParent() && ((LLViewerObject*)object->getParent())->mID == id)
			{
				// It's possible the item being removed has an avatar sitting on it
				// So remove the avatar that is sitting on the object.
				deselectObjectAndFamily(object, false);
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

	if (!LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
	{
		LLNotificationsUtil::add("CannotLinkPermanent");
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

	if (!LLSelectMgr::getInstance()->selectGetSameRegion())
	{
		LLNotificationsUtil::add("CannotLinkAcrossRegions");
		return true;
	}

	LLSelectMgr::getInstance()->sendLink();

	return true;
}

bool LLSelectMgr::unlinkObjects()
{
	S32 min_objects_for_confirm = gSavedSettings.getS32("MinObjectsForUnlinkConfirm");
	S32 unlink_object_count = mSelectedObjects->getObjectCount(); // clears out nodes with NULL objects
	if (unlink_object_count >= min_objects_for_confirm
		&& unlink_object_count > mSelectedObjects->getRootObjectCount())
	{
		// total count > root count means that there are childer inside and that there are linksets that will be unlinked
		LLNotificationsUtil::add("ConfirmUnlink", LLSD(), LLSD(), boost::bind(&LLSelectMgr::confirmUnlinkObjects, this, _1, _2));
		return true;
	}

	LLSelectMgr::getInstance()->sendDelink();
	return true;
}

void LLSelectMgr::confirmUnlinkObjects(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// if Cancel pressed
	if (option == 1)
	{
		return;
	}

	LLSelectMgr::getInstance()->sendDelink();
	return;
}

// in order to link, all objects must have the same owner, and the
// agent must have the ability to modify all of the objects. However,
// we're not answering that question with this method. The question
// we're answering is: does the user have a reasonable expectation
// that a link operation should work? If so, return true, false
// otherwise. this allows the handle_link method to more finely check
// the selection and give an error message when the uer has a
// reasonable expectation for the link to work, but it will fail.
//
// For animated objects, there's additional check that if the
// selection includes at least one animated object, the total mesh
// triangle count cannot exceed the designated limit.
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
					LLViewerObject *root_object = (object == NULL) ? NULL : object->getRootEdit();
					return object->permModify() && !object->isPermanentEnforced() &&
						((root_object == NULL) || !root_object->isPermanentEnforced());
				}
			} func;
			const bool firstonly = true;
			new_value = LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, firstonly);
		}
	}
    if (!LLSelectMgr::getInstance()->getSelection()->checkAnimatedObjectLinkable())
    {
        new_value = false;
    }
	return new_value;
}

bool LLSelectMgr::enableUnlinkObjects()
{
	LLViewerObject* first_editable_object = LLSelectMgr::getInstance()->getSelection()->getFirstEditableObject();
	LLViewerObject *root_object = (first_editable_object == NULL) ? NULL : first_editable_object->getRootEdit();

	bool new_value = LLSelectMgr::getInstance()->selectGetAllRootsValid() &&
		first_editable_object &&
		!first_editable_object->isAttachment() && !first_editable_object->isPermanentEnforced() &&
		((root_object == NULL) || !root_object->isPermanentEnforced());

	return new_value;
}

void LLSelectMgr::deselectObjectAndFamily(LLViewerObject* object, bool send_to_sim, bool include_entire_object)
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
	
		while(!root->isAvatar() && root->getParent())
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

	bool start_new_message = true;
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
			start_new_message = false;
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
			start_new_message = true;
		}
	}

	if (!start_new_message)
	{
		msg->sendReliable(regionp->getHost() );
	}

	updatePointAt();
	updateSelectionCenter();
}

void LLSelectMgr::deselectObjectOnly(LLViewerObject* object, bool send_to_sim)
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

void LLSelectMgr::addAsFamily(std::vector<LLViewerObject*>& objects, bool add_to_end)
{
	for (std::vector<LLViewerObject*>::iterator iter = objects.begin();
		 iter != objects.end(); ++iter)
	{
		LLViewerObject* objectp = *iter;
		
		// Can't select yourself
		if (objectp->mID == gAgentID
			&& !mAllowSelectAvatar)
		{
			continue;
		}

		if (!objectp->isSelected())
		{
			LLSelectNode *nodep = new LLSelectNode(objectp, true);
			if (add_to_end)
			{
				mSelectedObjects->addNodeAtEnd(nodep);
			}
			else
			{
				mSelectedObjects->addNode(nodep);
			}
			objectp->setSelected(true);

			if (objectp->getNumTEs() > 0)
			{
				nodep->selectAllTEs(true);
				objectp->setAllTESelected(true);
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
				select_node->setTransient(false);
			}
		}
	}
	saveSelectedObjectTransform(SELECT_ACTION_TYPE_PICK);
}

//-----------------------------------------------------------------------------
// addAsIndividual() - a single object, face, etc
//-----------------------------------------------------------------------------
void LLSelectMgr::addAsIndividual(LLViewerObject *objectp, S32 face, bool undoable)
{
	// check to see if object is already in list
	LLSelectNode *nodep = mSelectedObjects->findNode(objectp);

	// if not in list, add it
	if (!nodep)
	{
		nodep = new LLSelectNode(objectp, true);
		mSelectedObjects->addNode(nodep);
		llassert_always(nodep->getObject());
	}
	else
	{
		// make this a full-fledged selection
		nodep->setTransient(false);
		// Move it to the front of the list
		mSelectedObjects->moveNodeToFront(nodep);
	}

	// Make sure the object is tagged as selected
	objectp->setSelected( true );

	// And make sure we don't consider it as part of a family
	nodep->mIndividualSelection = true;

	// Handle face selection
	if (objectp->getNumTEs() <= 0)
	{
		// object has no faces, so don't do anything
	}
	else if (face == SELECT_ALL_TES)
	{
		nodep->selectAllTEs(true);
		objectp->setAllTESelected(true);
	}
	else if (0 <= face && face < SELECT_MAX_TES)
	{
		nodep->selectTE(face, true);
		objectp->setTESelected(face, true);
	}
	else
	{
		LL_ERRS() << "LLSelectMgr::add face " << face << " out-of-range" << LL_ENDL;
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
			if(!cur_objectp || cur_objectp->isDead())
			{
				continue;
			}
			LLSelectNode* nodep = new LLSelectNode(cur_objectp, false);
			nodep->selectTE(face, true);
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
        || (gSavedSettings.getBOOL("SelectMovableOnly") && (!objectp->permMove() || objectp->isPermanentEnforced())))
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
		objectp->setSelected(true);
		objectp->setAllTESelected(true);

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
	bool select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
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
	LLSelectNode* nodep = new LLSelectNode(objectp, false);
	mGridObjects.addNodeAtEnd(nodep);

	LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;
		nodep = new LLSelectNode(child, false);
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
}

void LLSelectMgr::getGrid(LLVector3& origin, LLQuaternion &rotation, LLVector3 &scale, bool for_snap_guides)
{
	mGridObjects.cleanupNodes();
	
	LLViewerObject* first_grid_object = mGridObjects.getFirstObject();

	if (mGridMode == GRID_MODE_LOCAL && mSelectedObjects->getObjectCount())
	{
		//LLViewerObject* root = getSelectedParentObject(mSelectedObjects->getFirstObject());
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
		LLSelectNode *node = mSelectedObjects->findNode(first_grid_object);
		if (!for_snap_guides && node)
		{
			mGridRotation = node->mSavedRotation;
		}
		else
		{
			mGridRotation = first_grid_object->getRenderRotation();
		}

		LLVector4a min_extents(F32_MAX);
		LLVector4a max_extents(-F32_MAX);
		bool grid_changed = false;
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
				grid_changed = true;
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
		const bool non_root_ok = true;
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
			mGridScale = LLVector3(1.f, 1.f, 1.f) * llmin(gSavedSettings.getF32("GridResolution"), 0.5f);
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
			objectp->setSelected(false);
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
void LLSelectMgr::remove(LLViewerObject *objectp, S32 te, bool undoable)
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
		objectp->setSelected( false );
	}
	else if (0 <= te && te < SELECT_MAX_TES)
	{
		// ...valid face, check to see if it was on
		if (nodep->isTESelected(te))
		{
			nodep->selectTE(te, false);
			objectp->setTESelected(te, false);
		}
		else
		{
			LL_ERRS() << "LLSelectMgr::remove - tried to remove TE " << te << " that wasn't selected" << LL_ENDL;
			return;
		}

		// ...check to see if this operation turned off all faces
		bool found = false;
		for (S32 i = 0; i < nodep->getObject()->getNumTEs(); i++)
		{
			found = found || nodep->isTESelected(i);
		}

		// ...all faces now turned off, so remove
		if (!found)
		{
			mSelectedObjects->removeNode(nodep);
			nodep = NULL;
			objectp->setSelected( false );
			// *FIXME: Doesn't update simulator that object is no longer selected
		}
	}
	else
	{
		// ...out of range face
		LL_ERRS() << "LLSelectMgr::remove - TE " << te << " out of range" << LL_ENDL;
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
		objectp->setSelected( false );
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

	bool selection_changed = false;

	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); )
	{
		LLObjectSelection::iterator curiter = iter++;
		LLSelectNode* nodep = *curiter;
		LLViewerObject* object = nodep->getObject();

		if (nodep->mIndividualSelection)
		{
			selection_changed = true;
		}

		LLViewerObject* parentp = object;
		while(parentp->getParent() && !(parentp->isRootEdit()))
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
	LL_INFOS() << "Selection Manager: " << mSelectedObjects->getNumNodes() << " items" << LL_ENDL;

	LL_INFOS() << "TE mode " << mTEMode << LL_ENDL;

	S32 count = 0;
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLViewerObject* objectp = (*iter)->getObject();
		LL_INFOS() << "Object " << count << " type " << LLPrimitive::pCodeToString(objectp->getPCode()) << LL_ENDL;
		LL_INFOS() << "  hasLSL " << objectp->flagScripted() << LL_ENDL;
		LL_INFOS() << "  hasTouch " << objectp->flagHandleTouch() << LL_ENDL;
		LL_INFOS() << "  hasMoney " << objectp->flagTakesMoney() << LL_ENDL;
		LL_INFOS() << "  getposition " << objectp->getPosition() << LL_ENDL;
		LL_INFOS() << "  getpositionAgent " << objectp->getPositionAgent() << LL_ENDL;
		LL_INFOS() << "  getpositionRegion " << objectp->getPositionRegion() << LL_ENDL;
		LL_INFOS() << "  getpositionGlobal " << objectp->getPositionGlobal() << LL_ENDL;
		LLDrawable* drawablep = objectp->mDrawable;
		LL_INFOS() << "  " << (drawablep&& drawablep->isVisible() ? "visible" : "invisible") << LL_ENDL;
		LL_INFOS() << "  " << (drawablep&& drawablep->isState(LLDrawable::FORCE_INVISIBLE) ? "force_invisible" : "") << LL_ENDL;
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
				LL_INFOS() << "Object " << objectp << " te " << te << LL_ENDL;
			}
		}
	}

	LL_INFOS() << mHighlightedObjects->getNumNodes() << " objects currently highlighted." << LL_ENDL;

	LL_INFOS() << "Center global " << mSelectionCenterGlobal << LL_ENDL;
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
        bool updated = false;
		for (S32 te = 0; te < num_tes; ++te)
		{
			if (node->isTESelected(te))
			{
				//(no-copy) textures must be moved to the object's inventory only once
				// without making any copies
				if (!texture_copied)
				{
					LLToolDragAndDrop::handleDropMaterialProtections(object, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
					texture_copied = true;
				}

				// apply texture for the selected faces
				add(LLStatViewer::EDIT_TEXTURE, 1);
				object->setTEImage(te, image);
                updated = true;
			}
		}

        if (updated) // not nessesary? sendTEUpdate update supposed to be done by sendfunc
        {
            dialog_refresh_all();

            // send the update to the simulator
            object->sendTEUpdate();
        }
	}
}

bool LLObjectSelection::applyRestrictedPbrMaterialToTEs(LLViewerInventoryItem* item)
{
    if (!item)
    {
        return false;
    }

    LLUUID asset_id = item->getAssetUUID();
    if (asset_id.isNull())
    {
        asset_id = LLGLTFMaterialList::BLANK_MATERIAL_ASSET_ID;
    }

    bool material_copied_all_faces = true;

    for (iterator iter = begin(); iter != end(); ++iter)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* object = (*iter)->getObject();
        if (!object)
        {
            continue;
        }

        S32 num_tes = llmin((S32)object->getNumTEs(), (S32)object->getNumFaces());
        bool material_copied = false;
        for (S32 te = 0; te < num_tes; ++te)
        {
            if (node->isTESelected(te))
            {
                //(no-copy), (no-modify), and (no-transfer) materials must be moved to the object's inventory only once
                // without making any copies
                if (!material_copied && asset_id.notNull())
                {
                    material_copied = (bool)LLToolDragAndDrop::handleDropMaterialProtections(object, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
                }
                if (!material_copied)
                {
                    // Applying the material is not possible for this object given the current inventory
					material_copied_all_faces = false;
                    break;
                }

                // apply texture for the selected faces
                // blank out most override data on the server
                //add(LLStatViewer::EDIT_TEXTURE, 1);
                object->setRenderMaterialID(te, asset_id);
            }
        }
    }

    LLGLTFMaterialList::flushUpdates();

    return material_copied_all_faces;
}


//-----------------------------------------------------------------------------
// selectionSetImage()
//-----------------------------------------------------------------------------
// *TODO: re-arch texture applying out of lltooldraganddrop
bool LLSelectMgr::selectionSetImage(const LLUUID& imageid)
{
	// First for (no copy) textures and multiple object selection
	LLViewerInventoryItem* item = gInventory.getItem(imageid);
	if(item 
		&& !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())
		&& (mSelectedObjects->getNumNodes() > 1) )
	{
         LL_DEBUGS() << "Attempted to apply no-copy texture " << imageid
             << " to multiple objects" << LL_ENDL;

        LLNotificationsUtil::add("FailedToApplyTextureNoCopyToMultiple");
        return false;
	}

	struct f : public LLSelectedTEFunctor
	{
		LLViewerInventoryItem* mItem;
		LLUUID mImageID;
		f(LLViewerInventoryItem* item, const LLUUID& id) : mItem(item), mImageID(id) {}
		bool apply(LLViewerObject* objectp, S32 te)
		{
		    if(!objectp || !objectp->permModify())
		    {
		        return false;
		    }

            // Might be better to run willObjectAcceptInventory
            if (mItem && objectp->isAttachment())
            {
                const LLPermissions& perm = mItem->getPermissions();
                bool unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? true : false;
                if (!unrestricted)
                {
                    // Attachments are in world and in inventory simultaneously,
                    // at the moment server doesn't support such a situation.
                    return false;
                }
            }

		    if (mItem)
			{
                LLToolDragAndDrop::dropTextureOneFace(objectp,
                                                      te,
                                                      mItem,
                                                      LLToolDragAndDrop::SOURCE_AGENT,
                                                      LLUUID::null,
                                                      false);
			}
			else // not an inventory item
			{
				// Texture picker defaults aren't inventory items
				// * Don't need to worry about permissions for them
				// * Can just apply the texture and be done with it.
				objectp->setTEImage(te, LLViewerTextureManager::getFetchedTexture(mImageID, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
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
				LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, true);
				effectp->setSourceObject(gAgentAvatarp);
				effectp->setTargetObject(object);
				effectp->setDuration(LL_HUD_DUR_SHORT);
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			}
			return true;
		}
	} sendfunc(item);
	getSelection()->applyToObjects(&sendfunc);

    return true;
}

//-----------------------------------------------------------------------------
// selectionSetGLTFMaterial()
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectionSetGLTFMaterial(const LLUUID& mat_id)
{
    // First for (no copy) textures and multiple object selection
    LLViewerInventoryItem* item = gInventory.getItem(mat_id);
    if (item
        && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())
        && (mSelectedObjects->getNumNodes() > 1))
    {
        LL_DEBUGS() << "Attempted to apply no-copy material " << mat_id
            << "to multiple objects" << LL_ENDL;

        LLNotificationsUtil::add("FailedToApplyGLTFNoCopyToMultiple");
        return false;
    }

    struct f : public LLSelectedTEFunctor
    {
        LLViewerInventoryItem* mItem;
        LLUUID mMatId;
        bool material_copied_any_face = false;
        bool material_copied_all_faces = true;
        f(LLViewerInventoryItem* item, const LLUUID& id) : mItem(item), mMatId(id) {}
        bool apply(LLViewerObject* objectp, S32 te)
        {
            if (!objectp || !objectp->permModify())
            {
                return false;
            }
            LLUUID asset_id = mMatId;
            if (mItem)
            {
                const LLPermissions& perm = mItem->getPermissions();
                bool from_library = perm.getOwner() == ALEXANDRIA_LINDEN_ID;
                if (objectp->isAttachment())
                {
                    bool unrestricted = (perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED;

                    if (!unrestricted && !from_library)
                    {
                        // Attachments are in world and in inventory simultaneously,
                        // at the moment server doesn't support such a situation.
                        return false;
                    }
                }

                if (!from_library
                    // Check if item may be copied into the object's inventory
                    && !LLToolDragAndDrop::handleDropMaterialProtections(objectp, mItem, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null))
                {
                    return false;
                }

                asset_id = mItem->getAssetUUID();
                if (asset_id.isNull())
                {
                    asset_id = LLGLTFMaterialList::BLANK_MATERIAL_ASSET_ID;
                }
            }

            // Blank out most override data on the object and send to server
            objectp->setRenderMaterialID(te, asset_id);

            return true;
        }
    };

    bool success = true;
    if (item
        &&  (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()) ||
             !item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()) ||
             !item->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID())
            )
        && item->getPermissions().getOwner() != ALEXANDRIA_LINDEN_ID
        )
    {
        success = success && getSelection()->applyRestrictedPbrMaterialToTEs(item);
    }
    else
    {
        f setfunc(item, mat_id);
        success = success && getSelection()->applyToTEs(&setfunc);
    }

    struct g : public LLSelectedObjectFunctor
    {
        LLViewerInventoryItem* mItem;
        g(LLViewerInventoryItem* item) : mItem(item) {}
        virtual bool apply(LLViewerObject* object)
        {
            if (object && !object->permModify())
            {
                return false;
            }

            if (!mItem)
            {
                // 1 particle effect per object				
                LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, true);
                effectp->setSourceObject(gAgentAvatarp);
                effectp->setTargetObject(object);
                effectp->setDuration(LL_HUD_DUR_SHORT);
                effectp->setColor(LLColor4U(gAgent.getEffectColor()));
            }

            dialog_refresh_all();
            object->sendTEUpdate();
            return true;
        }
    } sendfunc(item);
    success = success && getSelection()->applyToObjects(&sendfunc);

    LLGLTFMaterialList::flushUpdates();

    return success;
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

void LLSelectMgr::selectionRevertShinyColors()
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
				if (nodep && te < (S32)nodep->mSavedShinyColors.size())
				{
					LLColor4 color = nodep->mSavedShinyColors[te];
					// update viewer side color in anticipation of update from simulator
					LLMaterialPtr old_mat = object->getTE(te)->getMaterialParams();
					if (!old_mat.isNull())
					{
						LLMaterialPtr new_mat = gFloaterTools->getPanelFace()->createDefaultMaterial(old_mat);
						new_mat->setSpecularLightColor(color);
						object->getTE(te)->setMaterialParams(new_mat);
						LLMaterialMgr::getInstance()->put(object->getID(), te, *new_mat);
					}
				}
			}
			return true;
		}
	} setfunc(mSelectedObjects);
	getSelection()->applyToTEs(&setfunc);

	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);
}

bool LLSelectMgr::selectionRevertTextures()
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
						return false;
					}
					else
					{
						object->setTEImage(te, LLViewerTextureManager::getFetchedTexture(id, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));

					}
				}
			}
			return true;
		}
	} setfunc(mSelectedObjects);
	bool revert_successful = getSelection()->applyToTEs(&setfunc);
	
	LLSelectMgrSendFunctor sendfunc;
	getSelection()->applyToObjects(&sendfunc);

	return revert_successful;
}

void LLSelectMgr::selectionRevertGLTFMaterials()
{
    struct f : public LLSelectedTEFunctor
    {
        LLObjectSelectionHandle mSelectedObjects;
        f(LLObjectSelectionHandle sel) : mSelectedObjects(sel) {}
        bool apply(LLViewerObject* objectp, S32 te)
        {
            if (objectp && !objectp->permModify())
            {
                return false;
            }

            LLSelectNode* nodep = mSelectedObjects->findNode(objectp);
            if (nodep && te < (S32)nodep->mSavedGLTFMaterialIds.size())
            {
                // Restore base material
                LLUUID asset_id = nodep->mSavedGLTFMaterialIds[te];

                // Update material locally
                objectp->setRenderMaterialID(te, asset_id, false /*wait for LLGLTFMaterialList update*/);
                objectp->setTEGLTFMaterialOverride(te, nodep->mSavedGLTFOverrideMaterials[te]);

                // Enqueue update to server
                if (asset_id.notNull())
                {
                    // Restore overrides and base material
                    LLGLTFMaterialList::queueApply(objectp, te, asset_id, nodep->mSavedGLTFOverrideMaterials[te]);
                } 
                else
                {
                    //blank override out
                    LLGLTFMaterialList::queueApply(objectp, te, asset_id);
                }

            }
            return true;
        }
    } setfunc(mSelectedObjects);
    getSelection()->applyToTEs(&setfunc);
}

void LLSelectMgr::selectionSetBumpmap(U8 bumpmap, const LLUUID &image_id)
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

    LLViewerInventoryItem* item = gInventory.getItem(image_id);
    if(item 
        && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())
        && (mSelectedObjects->getNumNodes() > 1) )
    {
        LL_WARNS() << "Attempted to apply no-copy texture to multiple objects" << LL_ENDL;
        return;
    }
    if (item && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
    {
        LLViewerObject *object = mSelectedObjects->getFirstRootObject();
        if (!object)
        {
            return;
        }
        const LLPermissions& perm = item->getPermissions();
        bool unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? true : false;
        bool attached = object->isAttachment();
        if (attached && !unrestricted)
        {
            // Attachments are in world and in inventory simultaneously,
            // at the moment server doesn't support such a situation.
            return;
        }
        LLToolDragAndDrop::handleDropMaterialProtections(object, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
    }
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


void LLSelectMgr::selectionSetShiny(U8 shiny, const LLUUID &image_id)
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

    LLViewerInventoryItem* item = gInventory.getItem(image_id);
    if(item 
        && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())
        && (mSelectedObjects->getNumNodes() > 1) )
    {
        LL_WARNS() << "Attempted to apply no-copy texture to multiple objects" << LL_ENDL;
        return;
    }
    if (item && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
    {
        LLViewerObject *object = mSelectedObjects->getFirstRootObject();
        if (!object)
        {
            return;
        }
        const LLPermissions& perm = item->getPermissions();
        bool unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? true : false;
        bool attached = object->isAttachment();
        if (attached && !unrestricted)
        {
            // Attachments are in world and in inventory simultaneously,
            // at the moment server doesn't support such a situation.
            return;
        }
        LLToolDragAndDrop::handleDropMaterialProtections(object, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
    }
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
						((NULL != texture_entry) && !texture_entry->hasMedia() && !mMediaData.has(LLMediaEntry::HOME_URL_KEY)))
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

void LLSelectMgr::selectionSetMaterialParams(LLSelectedTEMaterialFunctor* material_func, int te)
{
	struct f1 : public LLSelectedTEFunctor
	{
		LLMaterialPtr mMaterial;
		f1(LLSelectedTEMaterialFunctor* material_func, int te) : _material_func(material_func), _specific_te(te) {}

		bool apply(LLViewerObject* object, S32 te)
		{
            if (_specific_te == -1 || (te == _specific_te))
            {
			    if (object && object->permModify() && _material_func)
			    {
				    LLTextureEntry* tep = object->getTE(te);
				    if (tep)
				    {
					    LLMaterialPtr current_material = tep->getMaterialParams();
					    _material_func->apply(object, te, tep, current_material);
				    }
			    }
            }
			return true;
		}

		LLSelectedTEMaterialFunctor* _material_func;
        int _specific_te;
	} func1(material_func, te);
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

void LLSelectMgr::selectionRemoveMaterial()
{
	struct f1 : public LLSelectedTEFunctor
	{
		bool apply(LLViewerObject* object, S32 face)
		{
			if (object->permModify())
			{
			        LL_DEBUGS("Materials") << "Removing material from object " << object->getID() << " face " << face << LL_ENDL;
				LLMaterialMgr::getInstance()->remove(object->getID(),face);
				object->setTEMaterialParams(face, NULL);
			}
			return true;
		}
	} func1;
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
bool LLSelectMgr::selectionGetGlow(F32 *glow)
{
	bool identical;
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
				object->updateFlags(true);
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
				object->updateFlags(true);
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
				object->updateFlags(true);
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
				object->updateFlags(true);
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
				object->updateFlags(true);
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

// true if all selected objects have this PCode
bool LLSelectMgr::selectionAllPCode(LLPCode code)
{
	struct f : public LLSelectedObjectFunctor
	{
		LLPCode mCode;
		f(const LLPCode& t) : mCode(t) {}
		virtual bool apply(LLViewerObject* object)
		{
			if (object->getPCode() != mCode)
			{
				return false;
			}
			return true;
		}
	} func(code);
	bool res = getSelection()->applyToObjects(&func);
	return res;
}

bool LLSelectMgr::selectionGetIncludeInSearch(bool* include_in_search_out)
{
	LLViewerObject *object = mSelectedObjects->getFirstRootObject();
	if (!object) return false;

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
        logNoOp,
		&include_in_search,
		SEND_ONLY_ROOTS);
}

bool LLSelectMgr::selectionGetClickAction(U8 *out_action)
{
	LLViewerObject *object = mSelectedObjects->getFirstObject();
	if (!object)
	{
		return false;
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
	bool res = getSelection()->applyToObjects(&func);
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
                      logNoOp,
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
		sendListToRegions(message_type, packGodlikeHead, packObjectIDAsParam, logNoOp, &data, SEND_ONLY_ROOTS);
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
void LLSelectMgr::logNoOp(LLSelectNode* node, void *)
{
}

// static
void LLSelectMgr::logAttachmentRequest(LLSelectNode* node, void *)
{
    LLAttachmentsMgr::instance().onAttachmentRequested(node->mItemID);
}

// static
void LLSelectMgr::logDetachRequest(LLSelectNode* node, void *)
{
    LLAttachmentsMgr::instance().onDetachRequested(node->mItemID);
}

// static
void LLSelectMgr::packObjectIDAsParam(LLSelectNode* node, void *)
{
	std::string buf = llformat("%u", node->getObject()->getLocalID());
	gMessageSystem->nextBlock("ParamList");
	gMessageSystem->addString("Parameter", buf);
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
					return true;
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
void LLSelectMgr::adjustTexturesByScale(bool send_to_sim, bool stretch)
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

		bool send = false;
		
		for (U8 te_num = 0; te_num < object->getNumTEs(); te_num++)
		{
			const LLTextureEntry* tep = object->getTE(te_num);

			bool planar = tep->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR;
			if (planar == stretch)
			{
				// Figure out how S,T changed with scale operation
				U32 s_axis, t_axis;
				if (!LLPrimitive::getTESTAxes(te_num, &s_axis, &t_axis))
				{
					continue;
				}
				
				LLVector3 object_scale = object->getScale();
				LLVector3 diffuse_scale_ratio  = selectNode->mTextureScaleRatios[te_num]; 

				// We like these to track together. NORSPEC-96
				//
				LLVector3 normal_scale_ratio   = diffuse_scale_ratio; 
				LLVector3 specular_scale_ratio = diffuse_scale_ratio; 
				
				// Apply new scale to face
				if (planar)
				{
					F32 diffuse_scale_s = diffuse_scale_ratio.mV[s_axis]/object_scale.mV[s_axis];
					F32 diffuse_scale_t = diffuse_scale_ratio.mV[t_axis]/object_scale.mV[t_axis];

					F32 normal_scale_s = normal_scale_ratio.mV[s_axis]/object_scale.mV[s_axis];
					F32 normal_scale_t = normal_scale_ratio.mV[t_axis]/object_scale.mV[t_axis];

					F32 specular_scale_s = specular_scale_ratio.mV[s_axis]/object_scale.mV[s_axis];
					F32 specular_scale_t = specular_scale_ratio.mV[t_axis]/object_scale.mV[t_axis];

					object->setTEScale(te_num, diffuse_scale_s, diffuse_scale_t);

					LLTextureEntry* tep = object->getTE(te_num);

					if (tep && !tep->getMaterialParams().isNull())
					{
						LLMaterialPtr orig = tep->getMaterialParams();
						LLMaterialPtr p = gFloaterTools->getPanelFace()->createDefaultMaterial(orig);
						p->setNormalRepeat(normal_scale_s, normal_scale_t);
						p->setSpecularRepeat(specular_scale_s, specular_scale_t);

						LLMaterialMgr::getInstance()->put(object->getID(), te_num, *p);
					}
				}
				else
				{

					F32 diffuse_scale_s = diffuse_scale_ratio.mV[s_axis]*object_scale.mV[s_axis];
					F32 diffuse_scale_t = diffuse_scale_ratio.mV[t_axis]*object_scale.mV[t_axis];

					F32 normal_scale_s = normal_scale_ratio.mV[s_axis]*object_scale.mV[s_axis];
					F32 normal_scale_t = normal_scale_ratio.mV[t_axis]*object_scale.mV[t_axis];

					F32 specular_scale_s = specular_scale_ratio.mV[s_axis]*object_scale.mV[s_axis];
					F32 specular_scale_t = specular_scale_ratio.mV[t_axis]*object_scale.mV[t_axis];

					object->setTEScale(te_num, diffuse_scale_s,diffuse_scale_t);

					LLTextureEntry* tep = object->getTE(te_num);

					if (tep && !tep->getMaterialParams().isNull())
					{
						LLMaterialPtr orig = tep->getMaterialParams();
						LLMaterialPtr p = gFloaterTools->getPanelFace()->createDefaultMaterial(orig);

						p->setNormalRepeat(normal_scale_s, normal_scale_t);
						p->setSpecularRepeat(specular_scale_s, specular_scale_t);

						LLMaterialMgr::getInstance()->put(object->getID(), te_num, *p);
					}
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
// Returns true if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetAllRootsValid()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); ++iter )
	{
		LLSelectNode* node = *iter;
		if( !node->mValid )
		{
			return false;
		}
	}
	return true;
}


//-----------------------------------------------------------------------------
// selectGetAllValid()
// Returns true if the viewer has information on all selected objects
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetAllValid()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); ++iter )
	{
		LLSelectNode* node = *iter;
		if( !node->mValid )
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetAllValidAndObjectsFound() - return true if selections are
// valid and objects are found.
//
// For EXT-3114 - same as selectGetModify() without the modify check.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetAllValidAndObjectsFound()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetModify() - return true if current agent can modify all
// selected objects.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetModify()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( !object->permModify() )
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsModify() - return true if current agent can modify all
// selected root objects.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsModify()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( !object->permModify() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetSameRegion() - return true if all objects are in same region
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetSameRegion()
{
    if (getSelection()->isEmpty())
    {
        return true;
    }
    LLViewerObject* object = getSelection()->getFirstObject();
    if (!object)
    {
        return false;
    }
    LLViewerRegion* current_region = object->getRegion();

    for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
        iter != getSelection()->root_end(); iter++)
    {
        LLSelectNode* node = *iter;
        object = node->getObject();
        if (!node->mValid || !object || current_region != object->getRegion())
        {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// selectGetNonPermanentEnforced() - return true if all objects are not
// permanent enforced
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetNonPermanentEnforced()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( object->isPermanentEnforced())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsNonPermanentEnforced() - return true if all root objects are
// not permanent enforced
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsNonPermanentEnforced()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( object->isPermanentEnforced())
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetPermanent() - return true if all objects are permanent
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetPermanent()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( !object->flagObjectPermanent())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsPermanent() - return true if all root objects are
// permanent
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsPermanent()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( !object->flagObjectPermanent())
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetCharacter() - return true if all objects are character
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetCharacter()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( !object->flagCharacter())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsCharacter() - return true if all root objects are
// character
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsCharacter()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( !object->flagCharacter())
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetNonPathfinding() - return true if all objects are not pathfinding
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetNonPathfinding()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( object->flagObjectPermanent() || object->flagCharacter())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsNonPathfinding() - return true if all root objects are not
// pathfinding
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsNonPathfinding()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( object->flagObjectPermanent() || object->flagCharacter())
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetNonPermanent() - return true if all objects are not permanent
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetNonPermanent()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( object->flagObjectPermanent())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsNonPermanent() - return true if all root objects are not
// permanent
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsNonPermanent()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( object->flagObjectPermanent())
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// selectGetNonCharacter() - return true if all objects are not character
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetNonCharacter()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( object->flagCharacter())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsNonCharacter() - return true if all root objects are not 
// character
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsNonCharacter()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if( object->flagCharacter())
		{
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// selectGetEditableLinksets() - return true if all objects are editable
//                               pathfinding linksets
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetEditableLinksets()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if (object->flagUsePhysics() ||
			object->flagTemporaryOnRez() ||
			object->flagCharacter() ||
			object->flagVolumeDetect() ||
			object->flagAnimSource() ||
			(object->getRegion() != gAgent.getRegion()) ||
			(!gAgent.isGodlike() && 
			!gAgent.canManageEstate() &&
			!object->permYouOwner() &&
			!object->permMove()))
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetViewableCharacters() - return true if all objects are characters
//                        viewable within the pathfinding characters floater
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetViewableCharacters()
{
	for (LLObjectSelection::iterator iter = getSelection()->begin();
		 iter != getSelection()->end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !object || !node->mValid )
		{
			return false;
		}
		if( !object->flagCharacter() ||
			(object->getRegion() != gAgent.getRegion()))
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsTransfer() - return true if current agent can transfer all
// selected root objects.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsTransfer()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if(!object->permTransfer())
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// selectGetRootsCopy() - return true if current agent can copy all
// selected root objects.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetRootsCopy()
{
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;
		LLViewerObject* object = node->getObject();
		if( !node->mValid )
		{
			return false;
		}
		if(!object->permCopy())
		{
			return false;
		}
	}
	return true;
}

struct LLSelectGetFirstTest
{
	LLSelectGetFirstTest() : mIdentical(true), mFirst(true)	{ }
	virtual ~LLSelectGetFirstTest() { }

	// returns false to break out of the iteration.
	bool checkMatchingNode(LLSelectNode* node)
	{
		if (!node || !node->mValid)
		{
			return false;
		}

		if (mFirst)
		{
			mFirstValue = getValueFromNode(node);
			mFirst = false;
		}
		else
		{
			if ( mFirstValue != getValueFromNode(node) )
			{
				mIdentical = false;
				// stop testing once we know not all selected are identical.
				return false;
			}
		}
		// continue testing.
		return true;
	}

	bool mIdentical;
	LLUUID mFirstValue;

protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node) = 0;

private:
	bool mFirst;
};

void LLSelectMgr::getFirst(LLSelectGetFirstTest* test)
{
	if (gSavedSettings.getBOOL("EditLinkedParts"))
	{
		for (LLObjectSelection::valid_iterator iter = getSelection()->valid_begin();
			iter != getSelection()->valid_end(); ++iter )
		{
			if (!test->checkMatchingNode(*iter))
			{
				break;
			}
		}
	}
	else
	{
		for (LLObjectSelection::root_object_iterator iter = getSelection()->root_object_begin();
			iter != getSelection()->root_object_end(); ++iter )
		{
			if (!test->checkMatchingNode(*iter))
			{
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// selectGetCreator()
// Creator information only applies to roots unless editing linked parts.
//-----------------------------------------------------------------------------
struct LLSelectGetFirstCreator : public LLSelectGetFirstTest
{
protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node)
	{
		return node->mPermissions->getCreator();
	}
};

bool LLSelectMgr::selectGetCreator(LLUUID& result_id, std::string& name)
{
	LLSelectGetFirstCreator test;
	getFirst(&test);

	if (test.mFirstValue.isNull())
	{
		name = LLTrans::getString("AvatarNameNobody");
		return false;
	}
	
	result_id = test.mFirstValue;
	
	if (test.mIdentical)
	{
		name = LLSLURL("agent", test.mFirstValue, "inspect").getSLURLString();
	}
	else
	{
		name = LLTrans::getString("AvatarNameMultiple");
	}

	return test.mIdentical;
}

//-----------------------------------------------------------------------------
// selectGetOwner()
// Owner information only applies to roots unless editing linked parts.
//-----------------------------------------------------------------------------
struct LLSelectGetFirstOwner : public LLSelectGetFirstTest
{
protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node)
	{
		// Don't use 'getOwnership' since we return a reference, not a copy.
		// Will return LLUUID::null if unowned (which is not allowed and should never happen.)
		return node->mPermissions->isGroupOwned() ? node->mPermissions->getGroup() : node->mPermissions->getOwner();
	}
};

bool LLSelectMgr::selectGetOwner(LLUUID& result_id, std::string& name)
{
	LLSelectGetFirstOwner test;
	getFirst(&test);

	if (test.mFirstValue.isNull())
	{
		return false;
	}

	result_id = test.mFirstValue;
	
	if (test.mIdentical)
	{
		bool group_owned = selectIsGroupOwned();
		if (group_owned)
		{
			name = LLSLURL("group", test.mFirstValue, "inspect").getSLURLString();
		}
		else
		{
			name = LLSLURL("agent", test.mFirstValue, "inspect").getSLURLString();
		}
	}
	else
	{
		name = LLTrans::getString("AvatarNameMultiple");
	}

	return test.mIdentical;
}

//-----------------------------------------------------------------------------
// selectGetLastOwner()
// Owner information only applies to roots unless editing linked parts.
//-----------------------------------------------------------------------------
struct LLSelectGetFirstLastOwner : public LLSelectGetFirstTest
{
protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node)
	{
		return node->mPermissions->getLastOwner();
	}
};

bool LLSelectMgr::selectGetLastOwner(LLUUID& result_id, std::string& name)
{
	LLSelectGetFirstLastOwner test;
	getFirst(&test);

	if (test.mFirstValue.isNull())
	{
		return false;
	}

	result_id = test.mFirstValue;
	
	if (test.mIdentical)
	{
		name = LLSLURL("agent", test.mFirstValue, "inspect").getSLURLString();
	}
	else
	{
		name.assign( "" );
	}

	return test.mIdentical;
}

//-----------------------------------------------------------------------------
// selectGetGroup()
// Group information only applies to roots unless editing linked parts.
//-----------------------------------------------------------------------------
struct LLSelectGetFirstGroup : public LLSelectGetFirstTest
{
protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node)
	{
		return node->mPermissions->getGroup();
	}
};

bool LLSelectMgr::selectGetGroup(LLUUID& result_id)
{
	LLSelectGetFirstGroup test;
	getFirst(&test);

	result_id = test.mFirstValue;
	return test.mIdentical;
}

//-----------------------------------------------------------------------------
// selectIsGroupOwned()
// Only operates on root nodes unless editing linked parts.  
// Returns true if the first selected is group owned.
//-----------------------------------------------------------------------------
struct LLSelectGetFirstGroupOwner : public LLSelectGetFirstTest
{
protected:
	virtual const LLUUID& getValueFromNode(LLSelectNode* node)
	{
		if (node->mPermissions->isGroupOwned())
		{
			return node->mPermissions->getGroup();
		}
		return LLUUID::null;
	}
};

bool LLSelectMgr::selectIsGroupOwned()
{
	LLSelectGetFirstGroupOwner test;
	getFirst(&test);

	return test.mFirstValue.notNull() ? true : false;
}

//-----------------------------------------------------------------------------
// selectGetPerm()
// Only operates on root nodes.
// Returns true if all have valid data.
// mask_on has bits set to true where all permissions are true
// mask_off has bits set to true where all permissions are false
// if a bit is off both in mask_on and mask_off, the values differ within
// the selection.
//-----------------------------------------------------------------------------
bool LLSelectMgr::selectGetPerm(U8 which_perm, U32* mask_on, U32* mask_off)
{
	U32 mask;
	U32 mask_and	= 0xffffffff;
	U32 mask_or		= 0x00000000;
	bool all_valid	= false;

	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;

		if (!node->mValid)
		{
			all_valid = false;
			break;
		}

		all_valid = true;
		
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
		return true;
	}
	else
	{
		*mask_on  = 0;
		*mask_off = 0;
		return false;
	}
}



bool LLSelectMgr::selectGetPermissions(LLPermissions& result_perm)
{
	bool first = true;
	LLPermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return false;
		}

		if (first)
		{
			perm = *(node->mPermissions);
			first = false;
		}
		else
		{
			perm.accumulate(*(node->mPermissions));
		}
	}

	result_perm = perm;

	return true;
}


void LLSelectMgr::selectDelete()
{
	S32 deleteable_count = 0;

	bool locked_but_deleteable_object = false;
	bool no_copy_but_deleteable_object = false;
	bool all_owned_by_you = true;

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
			locked_but_deleteable_object = true;
		}
		if(!obj->permCopy())
		{
			no_copy_but_deleteable_object = true;
		}
		if(!obj->permYouOwner())
		{
			all_owned_by_you = false;
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
		LL_WARNS() << "Nothing to delete!" << LL_ENDL;
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
                                                          logNoOp,
                                                          (void*) &info,
                                                          SEND_ONLY_ROOTS);
			// VEFFECT: Delete Object - one effect for all deletes
			if (LLSelectMgr::getInstance()->mSelectedObjects->mSelectType != SELECT_TYPE_HUD)
			{
				LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, true);
				effectp->setPositionGlobal( LLSelectMgr::getInstance()->getSelectionCenterGlobal() );
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
				F32 duration = 0.5f;
				duration += LLSelectMgr::getInstance()->mSelectedObjects->getObjectCount() / 64.f;
				effectp->setDuration(duration);
			}

			gAgentCamera.setLookAt(LOOKAT_TARGET_CLEAR);

			// Keep track of how many objects have been deleted.
			add(LLStatViewer::DELETE_OBJECT, LLSelectMgr::getInstance()->mSelectedObjects->getObjectCount());
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
        logNoOp,
		(void*)true,
		SEND_ONLY_ROOTS);
}

bool LLSelectMgr::selectGetEditMoveLinksetPermissions(bool &move, bool &modify)
{
    move = true;
    modify = true;
    bool selecting_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");

    for (LLObjectSelection::iterator iter = getSelection()->begin();
        iter != getSelection()->end(); iter++)
    {
        LLSelectNode* nodep = *iter;
        LLViewerObject* object = nodep->getObject();
        if (!object || !nodep->mValid)
        {
            move = false;
            modify = false;
            return false;
        }

        LLViewerObject *root_object = object->getRootEdit();
        bool this_object_movable = false;
        if (object->permMove() && !object->isPermanentEnforced() &&
            ((root_object == NULL) || !root_object->isPermanentEnforced()) &&
            (object->permModify() || selecting_linked_set))
        {
            this_object_movable = true;
        }
        move = move && this_object_movable;
        modify = modify && object->permModify();
    }

    return true;
}

void LLSelectMgr::selectGetAggregateSaleInfo(U32 &num_for_sale,
											 bool &is_for_sale_mixed, 
											 bool &is_sale_price_mixed,
											 S32 &total_sale_price,
											 S32 &individual_sale_price)
{
	num_for_sale = 0;
	is_for_sale_mixed = false;
	is_sale_price_mixed = false;
	total_sale_price = 0;
	individual_sale_price = 0;


	// Empty set.
	if (getSelection()->root_begin() == getSelection()->root_end())
		return;
	
	LLSelectNode *node = *(getSelection()->root_begin());
	const bool first_node_for_sale = node->mSaleInfo.isForSale();
	const S32 first_node_sale_price = node->mSaleInfo.getSalePrice();
	
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++)
	{
		LLSelectNode* node = *iter;
		const bool node_for_sale = node->mSaleInfo.isForSale();
		const S32 node_sale_price = node->mSaleInfo.getSalePrice();
		
		// Set mixed if the fields don't match the first node's fields.
		if (node_for_sale != first_node_for_sale) 
			is_for_sale_mixed = true;
		if (node_sale_price != first_node_sale_price)
			is_sale_price_mixed = true;
		
		if (node_for_sale)
		{
			total_sale_price += node_sale_price;
			num_for_sale ++;
		}
	}
	
	individual_sale_price = first_node_sale_price;
	if (is_for_sale_mixed)
	{
		is_sale_price_mixed = true;
		individual_sale_price = 0;
	}
}

// returns true if all nodes are valid. method also stores an
// accumulated sale info.
bool LLSelectMgr::selectGetSaleInfo(LLSaleInfo& result_sale_info)
{
	bool first = true;
	LLSaleInfo sale_info;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return false;
		}

		if (first)
		{
			sale_info = node->mSaleInfo;
			first = false;
		}
		else
		{
			sale_info.accumulate(node->mSaleInfo);
		}
	}

	result_sale_info = sale_info;

	return true;
}

bool LLSelectMgr::selectGetAggregatePermissions(LLAggregatePermissions& result_perm)
{
	bool first = true;
	LLAggregatePermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return false;
		}

		if (first)
		{
			perm = node->mAggregatePerm;
			first = false;
		}
		else
		{
			perm.aggregate(node->mAggregatePerm);
		}
	}

	result_perm = perm;

	return true;
}

bool LLSelectMgr::selectGetAggregateTexturePermissions(LLAggregatePermissions& result_perm)
{
	bool first = true;
	LLAggregatePermissions perm;
	for (LLObjectSelection::root_iterator iter = getSelection()->root_begin();
		 iter != getSelection()->root_end(); iter++ )
	{
		LLSelectNode* node = *iter;	
		if (!node->mValid)
		{
			return false;
		}

		LLAggregatePermissions t_perm = node->getObject()->permYouOwner() ? node->mAggregateTexturePermOwner : node->mAggregateTexturePerm;
		if (first)
		{
			perm = t_perm;
			first = false;
		}
		else
		{
			perm.aggregate(t_perm);
		}
	}

	result_perm = perm;

	return true;
}

bool LLSelectMgr::isMovableAvatarSelected()
{
	if (mAllowSelectAvatar)
	{
		return (getSelection()->getObjectCount() == 1) && (getSelection()->getFirstRootObject()->isAvatar()) && getSelection()->getFirstMoveableNode(true);
	}
	return false;
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

void LLSelectMgr::selectDuplicate(const LLVector3& offset, bool select_copy)
{
	if (mSelectedObjects->isAttachment())
	{
		//RN: do not duplicate attachments
		make_ui_sound("UISndInvalidOp");
		return;
	}
	if (!canDuplicate())
	{
		LLSelectNode* node = getSelection()->getFirstRootNode(NULL, true);
		if (node)
		{
			LLSD args;
			args["OBJ_NAME"] = node->mName;
			LLNotificationsUtil::add("NoCopyPermsNoObject", args);
			return;
		}
	}
	LLDuplicateData	data;

	data.offset = offset;
	data.flags = (select_copy ? FLAGS_CREATE_SELECTED : 0x0);

	sendListToRegions("ObjectDuplicate", packDuplicateHeader, packDuplicate, logNoOp, &data, SEND_ONLY_ROOTS);

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
			node->mDuplicated = true;
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

	sendListToRegions("ObjectDuplicate", packDuplicateHeader, packDuplicate, logNoOp, &data, SEND_ONLY_ROOTS);

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
	bool		mBypassRaycast;
	bool		mRayEndIsIntersection;
	LLUUID		mRayTargetID;
	bool		mCopyCenters;
	bool		mCopyRotates;
	U32			mFlags;
};

void LLSelectMgr::selectDuplicateOnRay(const LLVector3 &ray_start_region,
									   const LLVector3 &ray_end_region,
									   bool bypass_raycast,
									   bool ray_end_is_intersection,
									   const LLUUID &ray_target_id,
									   bool copy_centers,
									   bool copy_rotates,
									   bool select_copy)
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
                      packDuplicateOnRayHead, packObjectLocalID, logNoOp, &data, SEND_ONLY_ROOTS);

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
        logNoOp,
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
		htolememcpy(&data[offset], &(object->getPosition().mV), MVT_LLVector3, 12); 
		offset += 12;
	}
	if (type & UPD_ROTATION)
	{
		LLQuaternion quat = object->getRotation();
		LLVector3 vec = quat.packToVector3();
		htolememcpy(&data[offset], &(vec.mV), MVT_LLQuaternion, 12); 
		offset += 12;
	}
	if (type & UPD_SCALE)
	{
		//LL_INFOS() << "Sending object scale " << object->getScale() << LL_ENDL;
		htolememcpy(&data[offset], &(object->getScale().mV), MVT_LLVector3, 12); 
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
	bool	override;
};

void LLSelectMgr::sendOwner(const LLUUID& owner_id,
							const LLUUID& group_id,
							bool override)
{
	LLOwnerData data;

	data.owner_id = owner_id;
	data.group_id = group_id;
	data.override = override;

	sendListToRegions("ObjectOwner", packOwnerHead, packObjectLocalID, logNoOp, &data, SEND_ONLY_ROOTS);
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
	sendListToRegions("ObjectGroup", packAgentAndSessionAndGroupID, packObjectLocalID, logNoOp, &local_group_id, SEND_ONLY_ROOTS);
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
	sendListToRegions("ObjectBuy", packAgentGroupAndCatID, packBuyObjectIDs, logNoOp, &buy, SEND_ONLY_ROOTS);
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
	bool mSet;
	U32 mMask;
	bool mOverride;
};

// TODO: Make this able to fail elegantly.
void LLSelectMgr::selectionSetObjectPermissions(U8 field,
									   bool set,
									   U32 mask,
									   bool override)
{
	LLPermData data;

	data.mField = field;
	data.mSet = set;
	data.mMask = mask;
	data.mOverride = override;

	sendListToRegions("ObjectPermissions", packPermissionsHead, packPermissions, logNoOp, &data, SEND_ONLY_ROOTS);
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
	LL_ERRS() << "Not implemented" << LL_ENDL;
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
        logNoOp,
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
        logNoOp,
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
		nodep->setTransient(false);
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
				LL_INFOS() << "Selection manager: auto-deselecting, select_dist = " << (F32) sqrt(select_dist_sq) << LL_ENDL;
				LL_INFOS() << "agent pos global = " << gAgent.getPositionGlobal() << LL_ENDL;
				LL_INFOS() << "selection pos global = " << selectionCenter << LL_ENDL;
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
                          logNoOp,
						  (void*)(&name_copy),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectName",
						  packAgentAndSessionID,
						  packObjectName,
                          logNoOp,
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
                          logNoOp,
						  (void*)(&desc_copy),
						  SEND_ONLY_ROOTS);
	}
	else if(mSelectedObjects->getObjectCount() == 1)
	{
		sendListToRegions("ObjectDescription",
						  packAgentAndSessionID,
						  packObjectDescription,
                          logNoOp,
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
                      logNoOp,
					  (void*)(&category),
					  SEND_ONLY_ROOTS);
}

void LLSelectMgr::selectionSetObjectSaleInfo(const LLSaleInfo& sale_info)
{
	sendListToRegions("ObjectSaleInfo",
					  packAgentAndSessionID,
					  packObjectSaleInfo,
                      logNoOp,
					  (void*)(&sale_info),
					  SEND_ONLY_ROOTS);
}

//----------------------------------------------------------------------
// Attachments
//----------------------------------------------------------------------

void LLSelectMgr::sendAttach(U8 attachment_point, bool replace)
{
    sendAttach(mSelectedObjects, attachment_point, replace);
}

void LLSelectMgr::sendAttach(LLObjectSelectionHandle selection_handle, U8 attachment_point, bool replace)
{
	if (selection_handle.isNull())
	{
		return;
	}

	LLViewerObject* attach_object = selection_handle->getFirstRootObject();

	if (!attach_object || !isAgentAvatarValid() || selection_handle->mSelectType != SELECT_TYPE_WORLD)
	{
		return;
	}

	bool build_mode = LLToolMgr::getInstance()->inEdit();
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
			selection_handle,
			"ObjectAttach",
			packAgentIDAndSessionAndAttachment, 
			packObjectIDAndRotation, 
            logAttachmentRequest,
			&attachment_point, 
			SEND_ONLY_ROOTS );
		if (!build_mode)
		{
			// After "ObjectAttach" server will unsubscribe us from properties updates
			// so either deselect objects or resend selection after attach packet reaches server
			// In case of build_mode LLPanelObjectInventory::refresh() will deal with selection
			// Still unsubscribe even in case selection_handle is not current selection
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
        logDetachRequest,
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
        logDetachRequest,
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
        logNoOp,
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
        logNoOp,
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
        logNoOp,
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

void LLSelectMgr::saveSelectedShinyColors()
{
	struct f : public LLSelectedNodeFunctor
	{
		virtual bool apply(LLSelectNode* node)
		{
			node->saveShinyColors();
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
			node->mValid = false;
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
		LLSelectMgr* mManager;
		f(EActionType a, LLSelectMgr* p) : mActionType(a), mManager(p) {}
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
			selectNode->saveTextureScaleRatios(mManager->mTextureChannel);			
			return true;
		}
	} func(action_type, this);
	getSelection()->applyToNodes(&func);	
	
	mSavedSelectionBBox = getBBoxOfSelection();
}

struct LLSelectMgrApplyFlags : public LLSelectedObjectFunctor
{
	LLSelectMgrApplyFlags(U32 flags, bool state) : mFlags(flags), mState(state) {}
	U32 mFlags;
	bool mState;
	virtual bool apply(LLViewerObject* object)
	{
		if ( object->permModify())
		{
			if (object->isRoot()) 		// don't send for child objects
			{
				object->setFlags( mFlags, mState);
			}
			else if (FLAGS_WORLD & mFlags && ((LLViewerObject*)object->getRoot())->isSelected())
			{
				// FLAGS_WORLD are shared by all items in linkset
				object->setFlagsWithoutUpdate(FLAGS_WORLD & mFlags, mState);
			}
		};
		return true;
	}
};

void LLSelectMgr::selectionUpdatePhysics(bool physics)
{
	LLSelectMgrApplyFlags func(	FLAGS_USE_PHYSICS, physics);
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::selectionUpdateTemporary(bool is_temporary)
{
	LLSelectMgrApplyFlags func(	FLAGS_TEMPORARY_ON_REZ, is_temporary);
	getSelection()->applyToObjects(&func);	
}

void LLSelectMgr::selectionUpdatePhantom(bool is_phantom)
{
	LLSelectMgrApplyFlags func(	FLAGS_PHANTOM, is_phantom);
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
	bool force = (bool)(intptr_t)userdata;

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
									void (*log_func)(LLSelectNode* node, void *user_data), 
									void *user_data,
									ESendType send_type)
{
    sendListToRegions(mSelectedObjects, message_name, pack_header, pack_body, log_func, user_data, send_type);
}
void LLSelectMgr::sendListToRegions(LLObjectSelectionHandle selected_handle,
									const std::string& message_name,
									void (*pack_header)(void *user_data), 
									void (*pack_body)(LLSelectNode* node, void *user_data), 
									void (*log_func)(LLSelectNode* node, void *user_data), 
									void *user_data,
									ESendType send_type)
{
	LLSelectNode* node;
	LLSelectNode* linkset_root = NULL;
	LLViewerRegion*	last_region;
	LLViewerRegion*	current_region;
	S32 objects_in_this_packet = 0;

	bool link_operation = message_name == "ObjectLink";

    if (mAllowSelectAvatar)
    {
        if (selected_handle->getObjectCount() == 1
            && selected_handle->getFirstObject() != NULL
            && selected_handle->getFirstObject()->isAvatar())
        {
            // Server doesn't move avatars at the moment, it is a local debug feature,
            // but server does update position regularly, so do not drop mLastPositionLocal
            // Position override for avatar gets reset in LLAgentCamera::resetView().
        }
        else
        {
            // drop mLastPositionLocal (allow next update through)
            resetObjectOverrides(selected_handle);
        }
    }
    else
    {
        //clear update override data (allow next update through)
        resetObjectOverrides(selected_handle);
    }

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
				bool is_root = node->getObject()->isRootEdit();
				if ((mRoots && is_root) || (!mRoots && !is_root))
				{
					nodes_to_send.push(node);
				}
			}
			return true;
		}
	};
	struct push_all  pushall(nodes_to_send);
	struct push_some pushroots(nodes_to_send, true);
	struct push_some pushnonroots(nodes_to_send, false);
	
	switch(send_type)
	{
	  case SEND_ONLY_ROOTS:
		  if(message_name == "ObjectBuy")
			selected_handle->applyToRootNodes(&pushroots);
		  else
			selected_handle->applyToRootNodes(&pushall);
		  
		break;
	  case SEND_INDIVIDUALS:
		selected_handle->applyToNodes(&pushall);
		break;
	  case SEND_ROOTS_FIRST:
		// first roots...
		selected_handle->applyToNodes(&pushroots);
		// then children...
		selected_handle->applyToNodes(&pushnonroots);
		break;
	  case SEND_CHILDREN_FIRST:
		// first children...
		selected_handle->applyToNodes(&pushnonroots);
		// then roots...
		selected_handle->applyToNodes(&pushroots);
		break;

	default:
		LL_ERRS() << "Bad send type " << send_type << " passed to SendListToRegions()" << LL_ENDL;
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
			if (link_operation && linkset_root == NULL)
			{
				// linksets over 254 will be split into multiple messages,
				// but we need to provide same root for all messages or we will get separate linksets
				linkset_root = node;
			}
			// add another instance of the body of the data
			(*pack_body)(node, user_data);
            // do any related logging
            (*log_func)(node, user_data);
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
			objects_in_this_packet = 0;

			gMessageSystem->newMessage(message_name.c_str());
			(*pack_header)(user_data);

			if (linkset_root != NULL)
			{
				if (current_region != last_region)
				{
					// root should be in one region with the child, reset it
					linkset_root = NULL;
				}
				else
				{
					// add root instance into new message
					(*pack_body)(linkset_root, user_data);
					++objects_in_this_packet;
				}
			}

			// don't move to the next object, we still need to add the
			// body data. 
		}
	}

	// flush messages
	if (gMessageSystem->getCurrentSendTotal() > 0)
	{
		gMessageSystem->sendReliable( current_region->getHost());
	}
	else
	{
		gMessageSystem->clearMessage();
	}

	// LL_INFOS() << "sendListToRegions " << message_name << " obj " << objects_sent << " pkt " << packets_sent << LL_ENDL;
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

		if (!node)
		{
			LL_WARNS() << "Couldn't find object " << id << " selected." << LL_ENDL;
		}
		else
		{
            // save texture data as soon as we get texture perms first time
            bool save_textures = !node->mValid;
			if (node->mInventorySerial != inv_serial && node->getObject())
			{
				node->getObject()->dirtyInventory();

                // Even if this isn't object's first udpate, inventory changed
                // and some of the applied textures might have been in inventory
                // so update texture list.
                save_textures = true;
			}

			if (save_textures)
			{
				bool can_copy = false;
				bool can_transfer = false;

				LLAggregatePermissions::EValue value = LLAggregatePermissions::AP_NONE;
				if(node->getObject()->permYouOwner())
				{
					value = ag_texture_perms_owner.getValue(PERM_COPY);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_copy = true;
					}
					value = ag_texture_perms_owner.getValue(PERM_TRANSFER);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_transfer = true;
					}
				}
				else
				{
					value = ag_texture_perms.getValue(PERM_COPY);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_copy = true;
					}
					value = ag_texture_perms.getValue(PERM_TRANSFER);
					if (value == LLAggregatePermissions::AP_EMPTY || value == LLAggregatePermissions::AP_ALL)
					{
						can_transfer = true;
					}
				}

				if (can_copy && can_transfer)
				{
					node->saveTextures(texture_ids);
				}

                if (can_copy && can_transfer && node->getObject()->getVolume())
                {
                    uuid_vec_t material_ids;
                    gltf_materials_vec_t override_materials;
                    LLVOVolume* vobjp = (LLVOVolume*)node->getObject();
                    for (int i = 0; i < vobjp->getNumTEs(); ++i)
                    {
                        material_ids.push_back(vobjp->getRenderMaterialID(i));

                        // Make a copy to ensure we won't affect live material
                        // with any potential changes nor live changes will be
                        // reflected in a saved copy.
                        // Like changes from local material (reuses pointer) or
                        // from live editor (revert mechanics might modify this)
                        LLGLTFMaterial* old_override = node->getObject()->getTE(i)->getGLTFMaterialOverride();
                        if (old_override)
                        {
                            LLPointer<LLGLTFMaterial> mat = new LLGLTFMaterial(*old_override);
                            override_materials.push_back(mat);
                        }
                        else
                        {
                            override_materials.push_back(nullptr);
                        }
                    }
                    // processObjectProperties does not include overrides so this
                    // might need to be moved to LLGLTFMaterialOverrideDispatchHandler
                    node->saveGLTFMaterials(material_ids, override_materials);
                }
			}

			node->mValid = true;
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
			LLAvatarName av_name;
			LLAvatarNameCache::get(owner_id, &av_name);
			reporterp->setPickedObjectProperties(name, av_name.getUserName(), owner_id);
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
		node->mValid = true;
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
	bool reset_list;
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
		mSilhouetteImagep = LLViewerTextureManager::getFetchedTextureFromFile("silhouette.j2c", FTT_LOCAL_FILE, true, LLGLTexture::BOOST_UI);
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

		bool select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");

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

			LLSelectNode* rect_select_root_node = new LLSelectNode(objectp, true);
			rect_select_root_node->selectAllTEs(true);

			if (!select_linked_set)
			{
				rect_select_root_node->mIndividualSelection = true;
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

					LLSelectNode* rect_select_node = new LLSelectNode(child_objectp, true);
					rect_select_node->selectAllTEs(true);
					mHighlightedObjects->addNodeAtEnd(rect_select_node);
				}
			}

			// Add the root last, to preserve order for link operations.
			mHighlightedObjects->addNodeAtEnd(rect_select_root_node);
		}

		num_sils_genned	= 0;

		// render silhouettes for highlighted objects
		//bool subtracting_from_selection = (gKeyboard->currentMask(true) == MASK_CONTROL);
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
				bool roots_only = (pass == 0);
				bool is_root = objectp->isRootEdit();
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
				bool roots_only = (pass == 0);
				bool is_root = (objectp->isRootEdit());
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
void LLSelectMgr::renderSilhouettes(bool for_hud)
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

	bool wireframe_selection = (gFloaterTools && gFloaterTools->getVisible()) || LLSelectMgr::sRenderHiddenSelections;
	F32 fogCfx = (F32)llclamp((LLSelectMgr::getInstance()->getSelectionCenterGlobal() - gAgentCamera.getCameraPositionGlobal()).magVec() / (LLSelectMgr::getInstance()->getBBoxOfSelection().getExtentLocal().magVec() * 4), 0.0, 1.0);

	static LLColor4 sParentColor = LLColor4(sSilhouetteParentColor[VRED], sSilhouetteParentColor[VGREEN], sSilhouetteParentColor[VBLUE], LLSelectMgr::sHighlightAlpha);
	static LLColor4 sChildColor = LLColor4(sSilhouetteChildColor[VRED], sSilhouetteChildColor[VGREEN], sSilhouetteChildColor[VBLUE], LLSelectMgr::sHighlightAlpha);

	auto renderMeshSelection_f = [fogCfx, wireframe_selection](LLSelectNode* node, LLViewerObject* objectp, LLColor4 hlColor)
	{
		LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

		if (shader)
		{
			gDebugProgram.bind();
		}

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();

		bool is_hud_object = objectp->isHUDAttachment();

		if (!is_hud_object)
		{
			gGL.loadIdentity();
			gGL.multMatrix(gGLModelView);
		}

		if (objectp->mDrawable->isActive())
		{
			gGL.multMatrix((F32*)objectp->getRenderMatrix().mMatrix);
		}
		else if (!is_hud_object)
		{
			LLVector3 trans = objectp->getRegion()->getOriginAgent();
			gGL.translatef(trans.mV[0], trans.mV[1], trans.mV[2]);
		}

		bool bRenderHidenSelection = node->isTransient() ? false : LLSelectMgr::sRenderHiddenSelections;


		LLVOVolume* vobj = objectp->mDrawable->getVOVolume();
		if (vobj)
		{
			LLVertexBuffer::unbind();
			gGL.pushMatrix();
			gGL.multMatrix((F32*)vobj->getRelativeXform().mMatrix);

			if (objectp->mDrawable->isState(LLDrawable::RIGGED))
			{
				vobj->updateRiggedVolume(true);
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces()); // avatars have TEs but no faces
		for (S32 te = 0; te < num_tes; ++te)
		{
			if (node->isTESelected(te))
			{
				objectp->mDrawable->getFace(te)->renderOneWireframe(hlColor, fogCfx, wireframe_selection, bRenderHidenSelection, nullptr != shader);
			}
		}

		gGL.popMatrix();
		gGL.popMatrix();

		glLineWidth(1.f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (shader)
		{
			shader->bind();
		}
	};

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
                
                if (getTEMode() && !node->hasSelectedTE())
                    continue;

				LLViewerObject* objectp = node->getObject();
				if (!objectp)
					continue;

                if (objectp->mDrawable 
                    && objectp->mDrawable->getVOVolume() 
                    && objectp->mDrawable->getVOVolume()->isMesh())
                {
                    LLColor4 hlColor = objectp->isRootEdit() ? sParentColor : sChildColor;
                    if (objectp->getID() == inspect_item_id)
                    {
                        hlColor = sHighlightInspectColor;
                    }
                    else if (node->isTransient())
                    {
                        hlColor = sContextSilhouetteColor;
                    }
                    renderMeshSelection_f(node, objectp, hlColor);
                }
                else
                {
                    if (objectp->isHUDAttachment() != for_hud)
                    {
                        continue;
                    }
                    if (objectp->getID() == focus_item_id)
                    {
                        node->renderOneSilhouette(gFocusMgr.getFocusColor());
                    }
                    else if (objectp->getID() == inspect_item_id)
                    {
                        node->renderOneSilhouette(sHighlightInspectColor);
                    }
                    else if (node->isTransient())
                    {
                        bool oldHidden = LLSelectMgr::sRenderHiddenSelections;
                        LLSelectMgr::sRenderHiddenSelections = false;
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
			} //for all selected node's
		} //for pass
	}

	if (mHighlightedObjects->getNumNodes())
	{
		// render silhouettes for highlighted objects
		bool subtracting_from_selection = (gKeyboard->currentMask(true) == MASK_CONTROL);
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
				
				LLColor4 highlight_color = objectp->isRoot() ? sHighlightParentColor : sHighlightChildColor;
				if (objectp->mDrawable
					&& objectp->mDrawable->getVOVolume()
					&& objectp->mDrawable->getVOVolume()->isMesh())
				{
					renderMeshSelection_f(node, objectp, subtracting_from_selection ? LLColor4::red : highlight_color);
				}
				else if (subtracting_from_selection)
				{
					node->renderOneSilhouette(LLColor4::red);
				}
				else if (!objectp->isSelected())
				{
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
LLSelectNode::LLSelectNode(LLViewerObject* object, bool glow)
:	mObject(object),
	mIndividualSelection(false),
	mTransient(false),
	mValid(false),
	mPermissions(new LLPermissions()),
	mInventorySerial(0),
	mSilhouetteExists(false),
	mDuplicated(false),
	mTESelectMask(0),
	mLastTESelected(0),
	mName(LLStringUtil::null),
	mDescription(LLStringUtil::null),
	mTouchName(LLStringUtil::null),
	mSitName(LLStringUtil::null),
	mCreationDate(0)
{
	saveColors();
	saveShinyColors();
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
	mSavedShinyColors.clear();
	for (color_iter = nodep.mSavedShinyColors.begin(); color_iter != nodep.mSavedShinyColors.end(); ++color_iter)
	{
		mSavedShinyColors.push_back(*color_iter);
	}
	
	saveTextures(nodep.mSavedTextures);
    saveGLTFMaterials(nodep.mSavedGLTFMaterialIds, nodep.mSavedGLTFOverrideMaterials);
}

LLSelectNode::~LLSelectNode()
{
    LLSelectMgr *manager = LLSelectMgr::getInstance();
    if (manager->mAllowSelectAvatar
        && (!mLastPositionLocal.isExactlyZero()
            || mLastRotation != LLQuaternion()))
    {
        LLViewerObject* object = getObject(); //isDead() check
        if (object && !object->getParent())
        {
            LLVOAvatar* avatar = object->asAvatar();
            if (avatar)
            {
                // Avatar was moved and needs to stay that way
                manager->mAvatarOverridesMap.emplace(avatar->getID(), LLSelectMgr::AvatarPositionOverride(mLastPositionLocal, mLastRotation, object));
            }
        }
    }


	delete mPermissions;
	mPermissions = NULL;
}

void LLSelectNode::selectAllTEs(bool b)
{
	mTESelectMask = b ? TE_SELECT_MASK_ALL : 0x0;
	mLastTESelected = 0;
}

void LLSelectNode::selectTE(S32 te_index, bool selected)
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

bool LLSelectNode::isTESelected(S32 te_index) const
{
	if (te_index < 0 || te_index >= mObject->getNumTEs())
	{
		return false;
	}
	return (mTESelectMask & (0x1 << te_index)) != 0;
}

S32 LLSelectNode::getLastSelectedTE() const
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

void LLSelectNode::saveShinyColors()
{
	if (mObject.notNull())
	{
		mSavedShinyColors.clear();
		for (S32 i = 0; i < mObject->getNumTEs(); i++)
		{
			const LLMaterialPtr mat = mObject->getTE(i)->getMaterialParams();
			if (!mat.isNull())
			{
				mSavedShinyColors.push_back(mat->getSpecularLightColor());
			}
			else
			{
				mSavedShinyColors.push_back(LLColor4::white);
			}
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

void LLSelectNode::saveGLTFMaterials(const uuid_vec_t& materials, const gltf_materials_vec_t& override_materials)
{
    if (mObject.notNull())
    {
        mSavedGLTFMaterialIds.clear();
        mSavedGLTFOverrideMaterials.clear();

        for (uuid_vec_t::const_iterator materials_it = materials.begin();
            materials_it != materials.end(); ++materials_it)
        {
            mSavedGLTFMaterialIds.push_back(*materials_it);
        }

        for (gltf_materials_vec_t::const_iterator mat_it = override_materials.begin();
            mat_it != override_materials.end(); ++mat_it)
        {
            mSavedGLTFOverrideMaterials.push_back(*mat_it);
        }
    }
}

void LLSelectNode::saveTextureScaleRatios(LLRender::eTexIndex index_to_query)
{
	mTextureScaleRatios.clear();

	if (mObject.notNull())
	{
		
		LLVector3 scale = mObject->getScale();

		for (U8 i = 0; i < mObject->getNumTEs(); i++)
		{
			F32 diffuse_s = 1.0f;
			F32 diffuse_t = 1.0f;
			
			LLVector3 v;
			const LLTextureEntry* tep = mObject->getTE(i);
			if (!tep)
				continue;

			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(i, &s_axis, &t_axis);

			tep->getScale(&diffuse_s,&diffuse_t);

			if (tep->getTexGen() == LLTextureEntry::TEX_GEN_PLANAR)
			{
				v.mV[s_axis] = diffuse_s*scale.mV[s_axis];
				v.mV[t_axis] = diffuse_t*scale.mV[t_axis];
				mTextureScaleRatios.push_back(v);
			}
			else
			{
				v.mV[s_axis] = diffuse_s/scale.mV[s_axis];
				v.mV[t_axis] = diffuse_t/scale.mV[t_axis];
				mTextureScaleRatios.push_back(v);
			}
		}
	}
}


// This implementation should be similar to LLTask::allowOperationOnTask
bool LLSelectNode::allowOperationOnNode(PermissionBit op, U64 group_proxy_power) const
{
	// Extract ownership.
	bool object_is_group_owned = false;
	LLUUID object_owner_id;
	mPermissions->getOwnership(object_owner_id, object_is_group_owned);

	// Operations on invalid or public objects is not allowed.
	if (!mObject || (mObject->isDead()) || !mPermissions->isOwned())
	{
		return false;
	}

	// The transfer permissions can never be given through proxy.
	if (PERM_TRANSFER == op)
	{
		// The owner of an agent-owned object can transfer to themselves.
		if ( !object_is_group_owned 
			&& (gAgent.getID() == object_owner_id) )
		{
			return true;
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
			return false;
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
		return true;
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
		return (proxy_agent_id == object_owner_id ? true : false);
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

	LLVOVolume* vobj = drawable->getVOVolume();
	if (vobj && vobj->isMesh())
	{
		//This check (if(...)) with assert here just for ensure that this situation will not happens, and can be removed later. For example on the next release.
		llassert(!"renderOneWireframe() was removed SL-10194");
		return;
	}

	if (!mSilhouetteExists)
	{
		return;
	}

	bool is_hud_object = objectp->isHUDAttachment();
	
	if (mSilhouetteVertices.size() == 0 || mSilhouetteNormals.size() != mSilhouetteVertices.size())
	{
		return;
	}


	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	if (shader)
	{ //use UI program for selection highlights (texture color modulated by vertex color)
		gUIProgram.bind();
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
			
            LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE, GL_GEQUAL);
            gGL.flush();
			gGL.begin(LLRender::LINES);
			{
				gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.4f);

				for(S32 i = 0; i < mSilhouetteVertices.size(); i += 2)
				{
					u_coord += u_divisor * LLSelectMgr::sHighlightUScale;
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
				
				gGL.color4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha);
				gGL.texCoord2fv( tc[1].mV );
				gGL.vertex3fv( v[1].mV );

				gGL.color4f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE], 0.0f); //LLSelectMgr::sHighlightAlpha);
				gGL.texCoord2fv( tc[2].mV );
				gGL.vertex3fv( v[2].mV );

				gGL.vertex3fv( v[2].mV );

				gGL.color4f(color.mV[VRED]*2, color.mV[VGREEN]*2, color.mV[VBLUE]*2, LLSelectMgr::sHighlightAlpha);
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
		LL_WARNS() << "Trying to get_family_count on null parent!" << LL_ENDL;
	}
	S32 count = 1;	// for this object
	LLViewerObject::const_child_list_t& child_list = parent->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child = *iter;

		if (!child)
		{
			LL_WARNS() << "Family object has NULL child!  Show Doug." << LL_ENDL;
		}
		else if (child->isDead())
		{
			LL_WARNS() << "Family object has dead child object.  Show Doug." << LL_ENDL;
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
//
// FIXME this is a grab bag of functionality only some of which has to do
// with the selection center
// -----------------------------------------------------------------------------
void LLSelectMgr::updateSelectionCenter()
{
	const F32 MOVE_SELECTION_THRESHOLD = 1.f;		//  Movement threshold in meters for updating selection
													//  center (tractor beam)

    // override any avatar updates received
    // Works only if avatar was repositioned
    // and edit floater is visible
    overrideAvatarUpdates();
	//override any object updates received
	//for selected objects
	overrideObjectUpdates();

	LLViewerObject* object = mSelectedObjects->getFirstObject();
	if (!object)
	{
		// nothing selected, probably grabbing
		// Ignore by setting to avatar origin.
		mSelectionCenterGlobal.clearVec();
		mShowSelection = false;
		mSelectionBBox = LLBBox(); 
		resetAgentHUDZoom();
	}
	else
	{
		mSelectedObjects->mSelectType = getSelectTypeForObject(object);

		if (mSelectedObjects->mSelectType != SELECT_TYPE_HUD && isAgentAvatarValid())
		{
			// reset hud ZOOM
			resetAgentHUDZoom();
		}

		mShowSelection = false;
		LLBBox bbox;

		// have stuff selected
		LLVector3d select_center;
		// keep a list of jointed objects for showing the joint HUDEffects

		// Initialize the bounding box to the root prim, so the BBox orientation 
		// matches the root prim's (affecting the orientation of the manipulators). 
		bbox.addBBoxAgent( (mSelectedObjects->getFirstRootObject(true))->getBoundingBoxAgent() ); 
	                 
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
				mShowSelection = true;
			}

			bbox.addBBoxAgent( object->getBoundingBoxAgent() );
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

    pauseAssociatedAvatars();
}

//-----------------------------------------------------------------------------
// pauseAssociatedAvatars
//
// If the selection includes an attachment or an animated object, the
// associated avatars should pause their animations until they are no
// longer selected.
//-----------------------------------------------------------------------------
void LLSelectMgr::pauseAssociatedAvatars()
{
    mPauseRequests.clear();

    for (LLObjectSelection::iterator iter = mSelectedObjects->begin();
         iter != mSelectedObjects->end(); iter++)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* object = node->getObject();
        if (!object)
            continue;
			
        mSelectedObjects->mSelectType = getSelectTypeForObject(object);

        LLVOAvatar* parent_av = NULL;
        if (mSelectedObjects->mSelectType == SELECT_TYPE_ATTACHMENT)
        {
            // Selection can be obsolete, confirm that this is an attachment
            // and find parent avatar
            parent_av = object->getAvatarAncestor();
        }

        // Can be both an attachment and animated object
        if (parent_av)
        {
            // It's an attachment. Pause the avatar it's attached to.
            mPauseRequests.push_back(parent_av->requestPause());
        }

        if (object->isAnimatedObject() && object->getControlAvatar())
        {
            // It's an animated object. Pause the control avatar.
            mPauseRequests.push_back(object->getControlAvatar()->requestPause());
        }
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
bool LLSelectMgr::canUndo() const
{
	// Can edit or can move
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstUndoEnabledObject() != NULL; // HACK: casting away constness - MG;
}

//-----------------------------------------------------------------------------
// undo()
//-----------------------------------------------------------------------------
void LLSelectMgr::undo()
{
	bool select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Undo", packAgentAndSessionAndGroupID, packObjectID, logNoOp, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canRedo()
//-----------------------------------------------------------------------------
bool LLSelectMgr::canRedo() const
{
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstEditableObject() != NULL; // HACK: casting away constness - MG
}

//-----------------------------------------------------------------------------
// redo()
//-----------------------------------------------------------------------------
void LLSelectMgr::redo()
{
	bool select_linked_set = !gSavedSettings.getBOOL("EditLinkedParts");
	LLUUID group_id(gAgent.getGroupID());
	sendListToRegions("Redo", packAgentAndSessionAndGroupID, packObjectID, logNoOp, &group_id, select_linked_set ? SEND_ONLY_ROOTS : SEND_CHILDREN_FIRST);
}

//-----------------------------------------------------------------------------
// canDoDelete()
//-----------------------------------------------------------------------------
bool LLSelectMgr::canDoDelete() const
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
bool LLSelectMgr::canDeselect() const
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
bool LLSelectMgr::canDuplicate() const
{
	return const_cast<LLSelectMgr*>(this)->mSelectedObjects->getFirstCopyableObject() != NULL; // HACK: casting away constness - MG
}
//-----------------------------------------------------------------------------
// duplicate()
//-----------------------------------------------------------------------------
void LLSelectMgr::duplicate()
{
	LLVector3 offset(0.5f, 0.5f, 0.f);
	selectDuplicate(offset, true);
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

bool LLSelectMgr::canSelectObject(LLViewerObject* object, bool ignore_select_owned)
{
	// Never select dead objects
	if (!object || object->isDead())
	{
		return false;
	}

	if (mForceSelection)
	{
		return true;
	}

	if(!ignore_select_owned)
	{
		if ((gSavedSettings.getBOOL("SelectOwnedOnly") && !object->permYouOwner()) ||
				(gSavedSettings.getBOOL("SelectMovableOnly") && (!object->permMove() ||  object->isPermanentEnforced())))
		{
			// only select my own objects
			return false;
		}
	}

	// Can't select orphans
	if (object->isOrphaned()) return false;

	// Can't select avatars
	if (object->isAvatar()) return false;

	// Can't select land
	if (object->getPCode() == LLViewerObject::LL_VO_SURFACE_PATCH) return false;

	ESelectType selection_type = getSelectTypeForObject(object);
	if (mSelectedObjects->getObjectCount() > 0 && mSelectedObjects->mSelectType != selection_type) return false;

	return true;
}

bool LLSelectMgr::setForceSelection(bool force) 
{ 
	std::swap(mForceSelection,force); 
	return force; 
}

void LLSelectMgr::resetAgentHUDZoom()
{
	if (gAgentCamera.mHUDTargetZoom != 1)
	{
		gAgentCamera.mHUDTargetZoom = 1.f;
		gAgentCamera.mHUDCurZoom = 1.f;
	}
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
	return (object != NULL) && !node->mIndividualSelection && (object->isRootEdit());
}

bool LLObjectSelection::is_valid_root::operator()(LLSelectNode *node)
{
	LLViewerObject* object = node->getObject();
	return (object != NULL) && node->mValid && !node->mIndividualSelection && (object->isRootEdit());
}

bool LLObjectSelection::is_root_object::operator()(LLSelectNode *node)
{
	LLViewerObject* object = node->getObject();
	return (object != NULL) && (object->isRootEdit());
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
bool LLObjectSelection::isEmpty() const
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
		
		if (object && !object->isAttachment())
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
			cost += object->getStreamingCost();

            S32 bytes = 0;
            S32 visible = 0;
            LLMeshCostData costs;
            if (object->getCostData(costs))
            {
                bytes = costs.getSizeTotal();
                visible = costs.getSizeByLOD(object->getLOD());
            }
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
			S32 vt = 0;
			count += object->getTriangleCount(&vt);
			*vcount += vt;
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
					   cost += LLVOVolume::getTextureCost(*iter);
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
				cost += LLVOVolume::getTextureCost(*iter);
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

bool LLObjectSelection::checkAnimatedObjectEstTris()
{
    F32 est_tris = 0;
    F32 max_tris = 0;
    S32 anim_count = 0;
	for (root_iterator iter = root_begin(); iter != root_end(); ++iter)
	{
		LLViewerObject* object = (*iter)->getObject();
		if (!object)
			continue;
        if (object->isAnimatedObject())
        {
            anim_count++;
        }
        est_tris += object->recursiveGetEstTrianglesMax();
        max_tris = llmax((F32)max_tris,(F32)object->getAnimatedObjectMaxTris());
	}
	return anim_count==0 || est_tris <= max_tris;
}

bool LLObjectSelection::checkAnimatedObjectLinkable()
{
    return checkAnimatedObjectEstTris();
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

bool LLObjectSelection::isMultipleTESelected()
{
	bool te_selected = false;
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
					return true;
				}
				te_selected = true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// contains()
//-----------------------------------------------------------------------------
bool LLObjectSelection::contains(LLViewerObject* object)
{
	return findNode(object) != NULL;
}


//-----------------------------------------------------------------------------
// contains()
//-----------------------------------------------------------------------------
bool LLObjectSelection::contains(LLViewerObject* object, S32 te)
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
					return true;
				}

				bool all_selected = true;
				for (S32 i = 0; i < object->getNumTEs(); i++)
				{
					all_selected = all_selected && nodep->isTESelected(i);
				}
				return all_selected;
			}
		}
		return false;
	}
	else
	{
		// ...one face
		for (LLObjectSelection::iterator iter = begin(); iter != end(); iter++)
		{
			LLSelectNode* nodep = *iter;
			if (nodep->getObject() == object && nodep->isTESelected(te))
			{
				return true;
			}
		}
		return false;
	}
}

// returns true is any node is currenly worn as an attachment
bool LLObjectSelection::isAttachment()
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

LLSelectNode* LLObjectSelection::getFirstRootNode(LLSelectedNodeFunctor* func, bool non_root_ok)
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
LLViewerObject* LLObjectSelection::getFirstSelectedObject(LLSelectedNodeFunctor* func, bool get_parent)
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
LLViewerObject* LLObjectSelection::getFirstRootObject(bool non_root_ok)
{
	LLSelectNode* res = getFirstRootNode(NULL, non_root_ok);
	return res ? res->getObject() : NULL;
}

//-----------------------------------------------------------------------------
// getFirstMoveableNode()
//-----------------------------------------------------------------------------
LLSelectNode* LLObjectSelection::getFirstMoveableNode(bool get_root_first)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permMove() && !obj->isPermanentEnforced();
		}
	} func;
	LLSelectNode* res = get_root_first ? getFirstRootNode(&func, true) : getFirstNode(&func);
	return res;
}

//-----------------------------------------------------------------------------
// getFirstCopyableObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstCopyableObject(bool get_parent)
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
			if( obj && !obj->isPermanentEnforced() &&
				( (obj->permModify()) ||
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
LLViewerObject* LLObjectSelection::getFirstEditableObject(bool get_parent)
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
LLViewerObject* LLObjectSelection::getFirstMoveableObject(bool get_parent)
{
	struct f : public LLSelectedNodeFunctor
	{
		bool apply(LLSelectNode* node)
		{
			LLViewerObject* obj = node->getObject();
			return obj && obj->permMove() && !obj->isPermanentEnforced();
		}
	} func;
	return getFirstSelectedObject(&func, get_parent);
}

//-----------------------------------------------------------------------------
// getFirstUndoEnabledObject()
//-----------------------------------------------------------------------------
LLViewerObject* LLObjectSelection::getFirstUndoEnabledObject(bool get_parent)
{
    struct f : public LLSelectedNodeFunctor
    {
        bool apply(LLSelectNode* node)
        {
            LLViewerObject* obj = node->getObject();
            return obj && (obj->permModify() || (obj->permMove() && !obj->isPermanentEnforced()));
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
		bool perm_move = obj->permMove() && !obj->isPermanentEnforced();
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
			gPipeline.markMoved(obj->mDrawable, true);
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

template<>
bool LLCheckIdenticalFunctor<F32>::same(const F32& a, const F32& b, const F32& tolerance)
{
    F32 delta = (a - b);
    F32 abs_delta = fabs(delta);
    return abs_delta <= tolerance;
}

#define DEF_DUMMY_CHECK_FUNCTOR(T)                                                  \
template<>                                                                          \
bool LLCheckIdenticalFunctor<T>::same(const T& a, const T& b, const T& tolerance)   \
{                                                                                   \
    (void)tolerance;                                                                \
    return a == b;                                                                  \
}

DEF_DUMMY_CHECK_FUNCTOR(LLUUID)
DEF_DUMMY_CHECK_FUNCTOR(LLGLenum)
DEF_DUMMY_CHECK_FUNCTOR(LLTextureEntry)
DEF_DUMMY_CHECK_FUNCTOR(LLTextureEntry::e_texgen)
DEF_DUMMY_CHECK_FUNCTOR(bool)
DEF_DUMMY_CHECK_FUNCTOR(U8)
DEF_DUMMY_CHECK_FUNCTOR(int)
DEF_DUMMY_CHECK_FUNCTOR(LLColor4)
DEF_DUMMY_CHECK_FUNCTOR(LLMediaEntry)
DEF_DUMMY_CHECK_FUNCTOR(LLPointer<LLMaterial>)
DEF_DUMMY_CHECK_FUNCTOR(LLPointer<LLGLTFMaterial>)
DEF_DUMMY_CHECK_FUNCTOR(std::string)
DEF_DUMMY_CHECK_FUNCTOR(std::vector<std::string>)

template<>
bool LLCheckIdenticalFunctor<class LLFace *>::same(class LLFace* const & a, class LLFace* const & b, class LLFace* const & tolerance)   \
{                                                                                   \
    (void)tolerance;                                                                \
    return a == b;                                                                  \
}

