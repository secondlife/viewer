/** 
 * @file llviewerobject.cpp
 * @brief Base class for viewer objects
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

#include "llviewerobject.h"

#include "llaudioengine.h"
#include "imageids.h"
#include "indra_constants.h"
#include "llmath.h"
#include "llflexibleobject.h"
#include "llviewercontrol.h"
#include "lldatapacker.h"
#include "llfasttimer.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llinventory.h"
#include "llinventorydefines.h"
#include "llmaterialtable.h"
#include "llmutelist.h"
#include "llnamevalue.h"
#include "llprimitive.h"
#include "llquantize.h"
#include "llregionhandle.h"
#include "llsdserialize.h"
#include "lltree_common.h"
#include "llxfermanager.h"
#include "message.h"
#include "object_flags.h"
#include "timing.h"

#include "llaudiosourcevo.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llbbox.h"
#include "llbox.h"
#include "llcylinder.h"
#include "lldrawable.h"
#include "llface.h"
#include "llfloaterproperties.h"
#include "llfloatertools.h"
#include "llfollowcam.h"
#include "llhudtext.h"
#include "llselectmgr.h"
#include "llrendersphere.h"
#include "lltooldraganddrop.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerparceloverlay.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertextureanim.h"
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llvograss.h"
#include "llvoground.h"
#include "llvolume.h"
#include "llvolumemessage.h"
#include "llvopartgroup.h"
#include "llvosky.h"
#include "llvosurfacepatch.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvowater.h"
#include "llworld.h"
#include "llui.h"
#include "pipeline.h"
#include "llviewernetwork.h"
#include "llvowlsky.h"
#include "llmanip.h"
#include "lltrans.h"
#include "llsdutil.h"
#include "llmediaentry.h"

//#define DEBUG_UPDATE_TYPE

BOOL		LLViewerObject::sVelocityInterpolate = TRUE;
BOOL		LLViewerObject::sPingInterpolate = TRUE; 

U32			LLViewerObject::sNumZombieObjects = 0;
S32			LLViewerObject::sNumObjects = 0;
BOOL		LLViewerObject::sMapDebug = TRUE;
LLColor4	LLViewerObject::sEditSelectColor(	1.0f, 1.f, 0.f, 0.3f);	// Edit OK
LLColor4	LLViewerObject::sNoEditSelectColor(	1.0f, 0.f, 0.f, 0.3f);	// Can't edit
S32			LLViewerObject::sAxisArrowLength(50);
BOOL		LLViewerObject::sPulseEnabled(FALSE);
BOOL		LLViewerObject::sUseSharedDrawables(FALSE); // TRUE

// sMaxUpdateInterpolationTime must be greater than sPhaseOutUpdateInterpolationTime
F64			LLViewerObject::sMaxUpdateInterpolationTime = 3.0;		// For motion interpolation: after X seconds with no updates, don't predict object motion
F64			LLViewerObject::sPhaseOutUpdateInterpolationTime = 2.0;	// For motion interpolation: after Y seconds with no updates, taper off motion prediction


static LLFastTimer::DeclareTimer FTM_CREATE_OBJECT("Create Object");

// static
LLViewerObject *LLViewerObject::createObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
{
	LLViewerObject *res = NULL;
	LLFastTimer t1(FTM_CREATE_OBJECT);
	
	switch (pcode)
	{
	case LL_PCODE_VOLUME:
	  res = new LLVOVolume(id, pcode, regionp); break;
	case LL_PCODE_LEGACY_AVATAR:
	{
		if (id == gAgentID)
		{
			if (!gAgentAvatarp)
			{
				gAgentAvatarp = new LLVOAvatarSelf(id, pcode, regionp);
				gAgentAvatarp->initInstance();
			}
			else 
			{
				if (isAgentAvatarValid())
				{
					gAgentAvatarp->updateRegion(regionp);
				}
			}
			res = gAgentAvatarp;
		}
		else
		{
			LLVOAvatar *avatar = new LLVOAvatar(id, pcode, regionp); 
			avatar->initInstance();
			res = avatar;
		}
		break;
	}
	case LL_PCODE_LEGACY_GRASS:
	  res = new LLVOGrass(id, pcode, regionp); break;
	case LL_PCODE_LEGACY_PART_SYS:
// 	  llwarns << "Creating old part sys!" << llendl;
// 	  res = new LLVOPart(id, pcode, regionp); break;
	  res = NULL; break;
	case LL_PCODE_LEGACY_TREE:
	  res = new LLVOTree(id, pcode, regionp); break;
	case LL_PCODE_TREE_NEW:
// 	  llwarns << "Creating new tree!" << llendl;
// 	  res = new LLVOTree(id, pcode, regionp); break;
	  res = NULL; break;
	case LL_VO_SURFACE_PATCH:
	  res = new LLVOSurfacePatch(id, pcode, regionp); break;
	case LL_VO_SKY:
	  res = new LLVOSky(id, pcode, regionp); break;
	case LL_VO_VOID_WATER:
		res = new LLVOVoidWater(id, pcode, regionp); break;
	case LL_VO_WATER:
		res = new LLVOWater(id, pcode, regionp); break;
	case LL_VO_GROUND:
	  res = new LLVOGround(id, pcode, regionp); break;
	case LL_VO_PART_GROUP:
	  res = new LLVOPartGroup(id, pcode, regionp); break;
	case LL_VO_HUD_PART_GROUP:
	  res = new LLVOHUDPartGroup(id, pcode, regionp); break;
	case LL_VO_WL_SKY:
	  res = new LLVOWLSky(id, pcode, regionp); break;
	default:
	  llwarns << "Unknown object pcode " << (S32)pcode << llendl;
	  res = NULL; break;
	}
	return res;
}

LLViewerObject::LLViewerObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp, BOOL is_global)
:	LLPrimitive(),
	mChildList(),
	mID(id),
	mLocalID(0),
	mTotalCRC(0),
	mListIndex(-1),
	mTEImages(NULL),
	mGLName(0),
	mbCanSelect(TRUE),
	mFlags(0),
	mPhysicsShapeType(0),
	mPhysicsGravity(0),
	mPhysicsFriction(0),
	mPhysicsDensity(0),
	mPhysicsRestitution(0),
	mDrawable(),
	mCreateSelected(FALSE),
	mRenderMedia(FALSE),
	mBestUpdatePrecision(0),
	mText(),
	mLastInterpUpdateSecs(0.f),
	mLastMessageUpdateSecs(0.f),
	mLatestRecvPacketID(0),
	mData(NULL),
	mAudioSourcep(NULL),
	mAudioGain(1.f),
	mAppAngle(0.f),
	mPixelArea(1024.f),
	mInventory(NULL),
	mInventorySerialNum(0),
	mRegionp( regionp ),
	mInventoryPending(FALSE),
	mInventoryDirty(FALSE),
	mDead(FALSE),
	mOrphaned(FALSE),
	mUserSelected(FALSE),
	mOnActiveList(FALSE),
	mOnMap(FALSE),
	mStatic(FALSE),
	mNumFaces(0),
	mTimeDilation(1.f),
	mRotTime(0.f),
	mAngularVelocityRot(),
	mPreviousRotation(),
	mState(0),
	mMedia(NULL),
	mClickAction(0),
	mObjectCost(0),
	mLinksetCost(0),
	mPhysicsCost(0),
	mLinksetPhysicsCost(0.f),
	mCostStale(true),
	mPhysicsShapeUnknown(true),
	mAttachmentItemID(LLUUID::null),
	mLastUpdateType(OUT_UNKNOWN),
	mLastUpdateCached(FALSE)
{
	if (!is_global)
	{
		llassert(mRegionp);
	}

	LLPrimitive::init_primitive(pcode);

	// CP: added 12/2/2005 - this was being initialised to 0, not the current frame time
	mLastInterpUpdateSecs = LLFrameTimer::getElapsedSeconds();

	mPositionRegion = LLVector3(0.f, 0.f, 0.f);

	if (!is_global && mRegionp)
	{
		mPositionAgent = mRegionp->getOriginAgent();
	}
	resetRot();

	LLViewerObject::sNumObjects++;
}

LLViewerObject::~LLViewerObject()
{
	deleteTEImages();

	if(mInventory)
	{
		mInventory->clear();  // will deref and delete entries
		delete mInventory;
		mInventory = NULL;
	}

	if (mPartSourcep)
	{
		mPartSourcep->setDead();
		mPartSourcep = NULL;
	}

	// Delete memory associated with extra parameters.
	std::map<U16, ExtraParameter*>::iterator iter;
	for (iter = mExtraParameterList.begin(); iter != mExtraParameterList.end(); ++iter)
	{
		if(iter->second != NULL)
		{
			delete iter->second->data;
			delete iter->second;
		}
	}
	mExtraParameterList.clear();

	for_each(mNameValuePairs.begin(), mNameValuePairs.end(), DeletePairedPointer()) ;
	mNameValuePairs.clear();
	
	delete[] mData;
	mData = NULL;

	delete mMedia;
	mMedia = NULL;

	sNumObjects--;
	sNumZombieObjects--;
	llassert(mChildList.size() == 0);

	clearInventoryListeners();
}

void LLViewerObject::deleteTEImages()
{
	delete[] mTEImages;
	mTEImages = NULL;
}

void LLViewerObject::markDead()
{
	if (!mDead)
	{
		//llinfos << "Marking self " << mLocalID << " as dead." << llendl;
		
		// Root object of this hierarchy unlinks itself.
		if (getParent())
		{
			((LLViewerObject *)getParent())->removeChild(this);
		}

		// Mark itself as dead
		mDead = TRUE;
		gObjectList.cleanupReferences(this);

		LLViewerObject *childp;
		while (mChildList.size() > 0)
		{
			childp = mChildList.back();
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				//llinfos << "Marking child " << childp->getLocalID() << " as dead." << llendl;
				childp->setParent(NULL); // LLViewerObject::markDead 1
				childp->markDead();
			}
			else
			{
				// make sure avatar is no longer parented, 
				// so we can properly set it's position
				childp->setDrawableParent(NULL);
				((LLVOAvatar*)childp)->getOffObject();
				childp->setParent(NULL); // LLViewerObject::markDead 2
			}
			mChildList.pop_back();
		}

		if (mDrawable.notNull())
		{
			// Drawables are reference counted, mark as dead, then nuke the pointer.
			mDrawable->markDead();
			mDrawable = NULL;
		}

		if (mText)
		{
			mText->markDead();
			mText = NULL;
		}

		if (mIcon)
		{
			mIcon->markDead();
			mIcon = NULL;
		}

		if (mPartSourcep)
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}

		if (mAudioSourcep)
		{
			// Do some cleanup
			if (gAudiop)
			{
				gAudiop->cleanupAudioSource(mAudioSourcep);
			}
			mAudioSourcep = NULL;
		}

		if (flagAnimSource())
		{
			if (isAgentAvatarValid())
			{
				// stop motions associated with this object
				gAgentAvatarp->stopMotionFromSource(mID);
			}
		}

		if (flagCameraSource())
		{
			LLFollowCamMgr::removeFollowCamParams(mID);
		}

		sNumZombieObjects++;
	}
}

void LLViewerObject::dump() const
{
	llinfos << "Type: " << pCodeToString(mPrimitiveCode) << llendl;
	llinfos << "Drawable: " << (LLDrawable *)mDrawable << llendl;
	llinfos << "Update Age: " << LLFrameTimer::getElapsedSeconds() - mLastMessageUpdateSecs << llendl;

	llinfos << "Parent: " << getParent() << llendl;
	llinfos << "ID: " << mID << llendl;
	llinfos << "LocalID: " << mLocalID << llendl;
	llinfos << "PositionRegion: " << getPositionRegion() << llendl;
	llinfos << "PositionAgent: " << getPositionAgent() << llendl;
	llinfos << "PositionGlobal: " << getPositionGlobal() << llendl;
	llinfos << "Velocity: " << getVelocity() << llendl;
	if (mDrawable.notNull() && 
		mDrawable->getNumFaces() && 
		mDrawable->getFace(0))
	{
		LLFacePool *poolp = mDrawable->getFace(0)->getPool();
		if (poolp)
		{
			llinfos << "Pool: " << poolp << llendl;
			llinfos << "Pool reference count: " << poolp->mReferences.size() << llendl;
		}
	}
	//llinfos << "BoxTree Min: " << mDrawable->getBox()->getMin() << llendl;
	//llinfos << "BoxTree Max: " << mDrawable->getBox()->getMin() << llendl;
	/*
	llinfos << "Velocity: " << getVelocity() << llendl;
	llinfos << "AnyOwner: " << permAnyOwner() << " YouOwner: " << permYouOwner() << " Edit: " << mPermEdit << llendl;
	llinfos << "UsePhysics: " << flagUsePhysics() << " CanSelect " << mbCanSelect << " UserSelected " << mUserSelected << llendl;
	llinfos << "AppAngle: " << mAppAngle << llendl;
	llinfos << "PixelArea: " << mPixelArea << llendl;

	char buffer[1000];
	char *key;
	for (key = mNameValuePairs.getFirstKey(); key; key = mNameValuePairs.getNextKey() )
	{
		mNameValuePairs[key]->printNameValue(buffer);
		llinfos << buffer << llendl;
	}
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		llinfos << "  child " << child->getID() << llendl;
	}
	*/
}

void LLViewerObject::printNameValuePairs() const
{
	for (name_value_map_t::const_iterator iter = mNameValuePairs.begin();
		 iter != mNameValuePairs.end(); iter++)
	{
		LLNameValue* nv = iter->second;
		llinfos << nv->printNameValue() << llendl;
	}
}

void LLViewerObject::initVOClasses()
{
	// Initialized shared class stuff first.
	LLVOAvatar::initClass();
	LLVOTree::initClass();
	llinfos << "Viewer Object size: " << sizeof(LLViewerObject) << llendl;
	LLVOGrass::initClass();
	LLVOWater::initClass();
	LLVOVolume::initClass();
}

void LLViewerObject::cleanupVOClasses()
{
	LLVOGrass::cleanupClass();
	LLVOWater::cleanupClass();
	LLVOTree::cleanupClass();
	LLVOAvatar::cleanupClass();
	LLVOVolume::cleanupClass();
}

// Replaces all name value pairs with data from \n delimited list
// Does not update server
void LLViewerObject::setNameValueList(const std::string& name_value_list)
{
	// Clear out the old
	for_each(mNameValuePairs.begin(), mNameValuePairs.end(), DeletePairedPointer()) ;
	mNameValuePairs.clear();

	// Bring in the new
	std::string::size_type length = name_value_list.length();
	std::string::size_type start = 0;
	while (start < length)
	{
		std::string::size_type end = name_value_list.find_first_of("\n", start);
		if (end == std::string::npos) end = length;
		if (end > start)
		{
			std::string tok = name_value_list.substr(start, end - start);
			addNVPair(tok);
		}
		start = end+1;
	}
}

// This method returns true if the object is over land owned by the
// agent.
bool LLViewerObject::isReturnable()
{
	if (isAttachment())
	{
		return false;
	}
		
	std::vector<LLBBox> boxes;
	boxes.push_back(LLBBox(getPositionRegion(), getRotationRegion(), getScale() * -0.5f, getScale() * 0.5f).getAxisAligned());
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		boxes.push_back( LLBBox(child->getPositionRegion(), child->getRotationRegion(), child->getScale() * -0.5f, child->getScale() * 0.5f).getAxisAligned());
	}

	bool result = (mRegionp && mRegionp->objectIsReturnable(getPositionRegion(), boxes)) ? 1 : 0;
	
	if ( !result )
	{		
		//Get list of neighboring regions relative to this vo's region
		std::vector<LLViewerRegion*> uniqueRegions;
		mRegionp->getNeighboringRegions( uniqueRegions );
	
		//Build aabb's - for root and all children
		std::vector<PotentialReturnableObject> returnables;
		typedef std::vector<LLViewerRegion*>::iterator RegionIt;
		RegionIt regionStart = uniqueRegions.begin();
		RegionIt regionEnd   = uniqueRegions.end();
		
		for (; regionStart != regionEnd; ++regionStart )
		{
			LLViewerRegion* pTargetRegion = *regionStart;
			//Add the root vo as there may be no children and we still want
			//to test for any edge overlap
			buildReturnablesForChildrenVO( returnables, this, pTargetRegion );
			//Add it's children
			for (child_list_t::iterator iter = mChildList.begin();  iter != mChildList.end(); iter++)
			{
				LLViewerObject* pChild = *iter;		
				buildReturnablesForChildrenVO( returnables, pChild, pTargetRegion );
			}
		}	
	
		//TBD#Eventually create a region -> box list map 
		typedef std::vector<PotentialReturnableObject>::iterator ReturnablesIt;
		ReturnablesIt retCurrentIt = returnables.begin();
		ReturnablesIt retEndIt = returnables.end();
	
		for ( ; retCurrentIt !=retEndIt; ++retCurrentIt )
		{
			boxes.clear();
			LLViewerRegion* pRegion = (*retCurrentIt).pRegion;
			boxes.push_back( (*retCurrentIt).box );	
			bool retResult = 	pRegion
							 && pRegion->childrenObjectReturnable( boxes )
							 && pRegion->canManageEstate();
			if ( retResult )
			{ 
				result = true;
				break;
			}
		}
	}
	return result;
}

void LLViewerObject::buildReturnablesForChildrenVO( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion )
{
	if ( !pChild )
	{
		llerrs<<"child viewerobject is NULL "<<llendl;
	}
	
	constructAndAddReturnable( returnables, pChild, pTargetRegion );
	
	//We want to handle any children VO's as well
	for (child_list_t::iterator iter = pChild->mChildList.begin();  iter != pChild->mChildList.end(); iter++)
	{
		LLViewerObject* pChildofChild = *iter;
		buildReturnablesForChildrenVO( returnables, pChildofChild, pTargetRegion );
	}
}

void LLViewerObject::constructAndAddReturnable( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion )
{
	
	LLVector3 targetRegionPos;
	targetRegionPos.setVec( pChild->getPositionGlobal() );	
	
	LLBBox childBBox = LLBBox( targetRegionPos, pChild->getRotationRegion(), pChild->getScale() * -0.5f, 
							    pChild->getScale() * 0.5f).getAxisAligned();
	
	LLVector3 edgeA = targetRegionPos + childBBox.getMinLocal();
	LLVector3 edgeB = targetRegionPos + childBBox.getMaxLocal();
	
	LLVector3d edgeAd, edgeBd;
	edgeAd.setVec(edgeA);
	edgeBd.setVec(edgeB);
	
	//Only add the box when either of the extents are in a neighboring region
	if ( pTargetRegion->pointInRegionGlobal( edgeAd ) || pTargetRegion->pointInRegionGlobal( edgeBd ) )
	{
		PotentialReturnableObject returnableObj;
		returnableObj.box		= childBBox;
		returnableObj.pRegion	= pTargetRegion;
		returnables.push_back( returnableObj );
	}
}

bool LLViewerObject::crossesParcelBounds()
{
	std::vector<LLBBox> boxes;
	boxes.push_back(LLBBox(getPositionRegion(), getRotationRegion(), getScale() * -0.5f, getScale() * 0.5f).getAxisAligned());
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		boxes.push_back(LLBBox(child->getPositionRegion(), child->getRotationRegion(), child->getScale() * -0.5f, child->getScale() * 0.5f).getAxisAligned());
	}

	return mRegionp && mRegionp->objectsCrossParcel(boxes);
}

BOOL LLViewerObject::setParent(LLViewerObject* parent)
{
	if(mParent != parent)
	{
		LLViewerObject* old_parent = (LLViewerObject*)mParent ;		
		BOOL ret = LLPrimitive::setParent(parent);
		if(ret && old_parent && parent)
		{
			old_parent->removeChild(this) ;
		}
		return ret ;
	}

	return FALSE ;
}

void LLViewerObject::addChild(LLViewerObject *childp)
{
	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		if (*i == childp)
		{	//already has child
			return;
		}
	}
	
	if (!isAvatar())
	{
		// propagate selection properties
		childp->mbCanSelect = mbCanSelect;
	}

	if(childp->setParent(this))
	{
		mChildList.push_back(childp);
	}
}

void LLViewerObject::removeChild(LLViewerObject *childp)
{
	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		if (*i == childp)
		{
			if (!childp->isAvatar() && mDrawable.notNull() && mDrawable->isActive() && childp->mDrawable.notNull() && !isAvatar())
			{
				gPipeline.markRebuild(childp->mDrawable, LLDrawable::REBUILD_VOLUME);
			}

			mChildList.erase(i);

			if(childp->getParent() == this)
			{
				childp->setParent(NULL);			
			}
			break;
		}
	}
	
	if (childp->isSelected())
	{
		LLSelectMgr::getInstance()->deselectObjectAndFamily(childp);
		BOOL add_to_end = TRUE;
		LLSelectMgr::getInstance()->selectObjectAndFamily(childp, add_to_end);
	}
}

void LLViewerObject::addThisAndAllChildren(std::vector<LLViewerObject*>& objects)
{
	objects.push_back(this);
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		if (!child->isAvatar())
		{
			child->addThisAndAllChildren(objects);
		}
	}
}

void LLViewerObject::addThisAndNonJointChildren(std::vector<LLViewerObject*>& objects)
{
	objects.push_back(this);
	// don't add any attachments when temporarily selecting avatar
	if (isAvatar())
	{
		return;
	}
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		if ( (!child->isAvatar()))
		{
			child->addThisAndNonJointChildren(objects);
		}
	}
}

BOOL LLViewerObject::isChild(LLViewerObject *childp) const
{
	for (child_list_t::const_iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* testchild = *iter;
		if (testchild == childp)
			return TRUE;
	}
	return FALSE;
}


// returns TRUE if at least one avatar is sitting on this object
BOOL LLViewerObject::isSeat() const
{
	for (child_list_t::const_iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		if (child->isAvatar())
		{
			return TRUE;
		}
	}
	return FALSE;

}

BOOL LLViewerObject::setDrawableParent(LLDrawable* parentp)
{
	if (mDrawable.isNull())
	{
		return FALSE;
	}

	BOOL ret = mDrawable->mXform.setParent(parentp ? &parentp->mXform : NULL);
	if(!ret)
	{
		return FALSE ;
	}
	LLDrawable* old_parent = mDrawable->mParent;
	mDrawable->mParent = parentp; 
		
	if (parentp && mDrawable->isActive())
	{
		parentp->makeActive();
		parentp->setState(LLDrawable::ACTIVE_CHILD);
	}

	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	if(	(old_parent != parentp && old_parent)
		|| (parentp && parentp->isActive()))
	{
		// *TODO we should not be relying on setDrawable parent to call markMoved
		gPipeline.markMoved(mDrawable, FALSE);
	}
	else if (!mDrawable->isAvatar())
	{
		mDrawable->updateXform(TRUE);
		/*if (!mDrawable->getSpatialGroup())
		{
			mDrawable->movePartition();
		}*/
	}
	
	return ret;
}

// Show or hide particles, icon and HUD
void LLViewerObject::hideExtraDisplayItems( BOOL hidden )
{
	if( mPartSourcep.notNull() )
	{
		LLViewerPartSourceScript *partSourceScript = mPartSourcep.get();
		partSourceScript->setSuspended( hidden );
	}

	if( mText.notNull() )
	{
		LLHUDText *hudText = mText.get();
		hudText->setHidden( hidden );
	}

	if( mIcon.notNull() )
	{
		LLHUDIcon *hudIcon = mIcon.get();
		hudIcon->setHidden( hidden );
	}
}

U32 LLViewerObject::checkMediaURL(const std::string &media_url)
{
    U32 retval = (U32)0x0;
    if (!mMedia && !media_url.empty())
    {
        retval |= MEDIA_URL_ADDED;
        mMedia = new LLViewerObjectMedia;
        mMedia->mMediaURL = media_url;
        mMedia->mMediaType = LLViewerObject::MEDIA_SET;
        mMedia->mPassedWhitelist = FALSE;
    }
    else if (mMedia)
    {
        if (media_url.empty())
        {
            retval |= MEDIA_URL_REMOVED;
            delete mMedia;
            mMedia = NULL;
        }
        else if (mMedia->mMediaURL != media_url) // <-- This is an optimization.  If they are equal don't bother with below's test.
        {
            /*if (! (LLTextureEntry::getAgentIDFromMediaVersionString(media_url) == gAgent.getID() &&
                   LLTextureEntry::getVersionFromMediaVersionString(media_url) == 
                        LLTextureEntry::getVersionFromMediaVersionString(mMedia->mMediaURL) + 1))
			*/
            {
                // If the media URL is different and WE were not the one who
                // changed it, mark dirty.
                retval |= MEDIA_URL_UPDATED;
            }
            mMedia->mMediaURL = media_url;
            mMedia->mPassedWhitelist = FALSE;
        }
    }
    return retval;
}

U32 LLViewerObject::processUpdateMessage(LLMessageSystem *mesgsys,
					 void **user_data,
					 U32 block_num,
					 const EObjectUpdateType update_type,
					 LLDataPacker *dp)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	U32 retval = 0x0;
	
	// If region is removed from the list it is also deleted.
	if (!LLWorld::instance().isRegionListed(mRegionp))
	{
		llwarns << "Updating object in an invalid region" << llendl;
		return retval;
	}

	// Coordinates of objects on simulators are region-local.
	U64 region_handle;
	mesgsys->getU64Fast(_PREHASH_RegionData, _PREHASH_RegionHandle, region_handle);
	
	{
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromHandle(region_handle);
		if(regionp != mRegionp && regionp && mRegionp)//region cross
		{
			//this is the redundant position and region update, but it is necessary in case the viewer misses the following 
			//position and region update messages from sim.
			//this redundant update should not cause any problems.
			LLVector3 delta_pos =  mRegionp->getOriginAgent() - regionp->getOriginAgent();
			setPositionParent(getPosition() + delta_pos); //update to the new region position immediately.
			setRegion(regionp) ; //change the region.
		}
		else
		{
			mRegionp = regionp ;
		}
	}	
	
	if (!mRegionp)
	{
		U32 x, y;
		from_region_handle(region_handle, &x, &y);

		llerrs << "Object has invalid region " << x << ":" << y << "!" << llendl;
		return retval;
	}

	U16 time_dilation16;
	mesgsys->getU16Fast(_PREHASH_RegionData, _PREHASH_TimeDilation, time_dilation16);
	F32 time_dilation = ((F32) time_dilation16) / 65535.f;
	mTimeDilation = time_dilation;
	mRegionp->setTimeDilation(time_dilation);

	// this will be used to determine if we've really changed position
	// Use getPosition, not getPositionRegion, since this is what we're comparing directly against.
	LLVector3 test_pos_parent = getPosition();

	U8  data[60+16]; // This needs to match the largest size below.
#ifdef LL_BIG_ENDIAN
	U16 valswizzle[4];
#endif
	U16	*val;
	const F32 size = LLWorld::getInstance()->getRegionWidthInMeters();	
	const F32 MAX_HEIGHT = LLWorld::getInstance()->getRegionMaxHeight();
	const F32 MIN_HEIGHT = LLWorld::getInstance()->getRegionMinHeight();
	S32 length;
	S32	count;
	S32 this_update_precision = 32;		// in bits

	// Temporaries, because we need to compare w/ previous to set dirty flags...
	LLVector3 new_pos_parent;
	LLVector3 new_vel;
	LLVector3 new_acc;
	LLVector3 new_angv;
	LLVector3 old_angv = getAngularVelocity();
	LLQuaternion new_rot;
	LLVector3 new_scale = getScale();

	U32	parent_id = 0;
	U8	material = 0;
	U8 click_action = 0;
	U32 crc = 0;

	bool old_special_hover_cursor = specialHoverCursor();

	LLViewerObject *cur_parentp = (LLViewerObject *)getParent();

	if (cur_parentp)
	{
		parent_id = cur_parentp->mLocalID;
	}

	if (!dp)
	{
		switch(update_type)
		{
		case OUT_FULL:
			{
#ifdef DEBUG_UPDATE_TYPE
				llinfos << "Full:" << getID() << llendl;
#endif
				//clear cost and linkset cost
				mCostStale = true;
				if (isSelected())
				{
					gFloaterTools->dirty();
				}

				LLUUID audio_uuid;
				LLUUID owner_id;	// only valid if audio_uuid or particle system is not null
				F32    gain;
				U8     sound_flags;

				mesgsys->getU32Fast( _PREHASH_ObjectData, _PREHASH_CRC, crc, block_num);
				mesgsys->getU32Fast( _PREHASH_ObjectData, _PREHASH_ParentID, parent_id, block_num);
				mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_Sound, audio_uuid, block_num );
				// HACK: Owner id only valid if non-null sound id or particle system
				mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, owner_id, block_num );
				mesgsys->getF32Fast( _PREHASH_ObjectData, _PREHASH_Gain, gain, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_Flags, sound_flags, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_Material, material, block_num );
				mesgsys->getU8Fast(  _PREHASH_ObjectData, _PREHASH_ClickAction, click_action, block_num); 
				mesgsys->getVector3Fast(_PREHASH_ObjectData, _PREHASH_Scale, new_scale, block_num );
				length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_ObjectData);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_ObjectData, data, length, block_num);

				mTotalCRC = crc;

				// Owner ID used for sound muting or particle system muting
				setAttachedSound(audio_uuid, owner_id, gain, sound_flags);

				U8 old_material = getMaterial();
				if (old_material != material)
				{
					setMaterial(material);
					if (mDrawable.notNull())
					{
						gPipeline.markMoved(mDrawable, FALSE); // undamped
					}
				}
				setClickAction(click_action);

				count = 0;
				LLVector4 collision_plane;
				
				switch(length)
				{
				case (60 + 16):
					// pull out collision normal for avatar
					htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
					count += sizeof(LLVector4);
					// fall through
				case 60:
					this_update_precision = 32;
					// this is a terse update
					// pos
					htonmemcpy(new_pos_parent.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// vel
					htonmemcpy((void*)getVelocity().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// acc
					htonmemcpy((void*)getAcceleration().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// theta
					{
						LLVector3 vec;
						htonmemcpy(vec.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
						new_rot.unpackFromVector3(vec);
					}
					count += sizeof(LLVector3);
					// omega
					htonmemcpy((void*)new_angv.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					if (new_angv.isExactlyZero())
					{
						// reset rotation time
						resetRot();
					}
					setAngularVelocity(new_angv);
#if LL_DARWIN
					if (length == 76)
					{
						setAngularVelocity(LLVector3::zero);
					}
#endif
					break;
				case(32 + 16):
					// pull out collision normal for avatar
					htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
					count += sizeof(LLVector4);
					// fall through
				case 32:
					this_update_precision = 16;
					test_pos_parent.quantize16(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);

					// This is a terse 16 update, so treat data as an array of U16's.
#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					new_pos_parent.mV[VX] = U16_to_F32(val[VX], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VY] = U16_to_F32(val[VY], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VZ] = U16_to_F32(val[VZ], MIN_HEIGHT, MAX_HEIGHT);

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					setVelocity(LLVector3(U16_to_F32(val[VX], -size, size),
													   U16_to_F32(val[VY], -size, size),
													   U16_to_F32(val[VZ], -size, size)));

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					setAcceleration(LLVector3(U16_to_F32(val[VX], -size, size),
														   U16_to_F32(val[VY], -size, size),
														   U16_to_F32(val[VZ], -size, size)));

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Quat, 4); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*4;
					new_rot.mQ[VX] = U16_to_F32(val[VX], -1.f, 1.f);
					new_rot.mQ[VY] = U16_to_F32(val[VY], -1.f, 1.f);
					new_rot.mQ[VZ] = U16_to_F32(val[VZ], -1.f, 1.f);
					new_rot.mQ[VW] = U16_to_F32(val[VW], -1.f, 1.f);

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					new_angv.setVec(U16_to_F32(val[VX], -size, size),
										U16_to_F32(val[VY], -size, size),
										U16_to_F32(val[VZ], -size, size));
					if (new_angv.isExactlyZero())
					{
						// reset rotation time
						resetRot();
					}
					setAngularVelocity(new_angv);
					break;

				case 16:
					this_update_precision = 8;
					test_pos_parent.quantize8(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);
					// this is a terse 8 update
					new_pos_parent.mV[VX] = U8_to_F32(data[0], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VY] = U8_to_F32(data[1], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VZ] = U8_to_F32(data[2], MIN_HEIGHT, MAX_HEIGHT);

					setVelocity(U8_to_F32(data[3], -size, size),
								U8_to_F32(data[4], -size, size),
								U8_to_F32(data[5], -size, size) );

					setAcceleration(U8_to_F32(data[6], -size, size),
									U8_to_F32(data[7], -size, size),
									U8_to_F32(data[8], -size, size) );

					new_rot.mQ[VX] = U8_to_F32(data[9], -1.f, 1.f);
					new_rot.mQ[VY] = U8_to_F32(data[10], -1.f, 1.f);
					new_rot.mQ[VZ] = U8_to_F32(data[11], -1.f, 1.f);
					new_rot.mQ[VW] = U8_to_F32(data[12], -1.f, 1.f);

					new_angv.setVec(U8_to_F32(data[13], -size, size),
										U8_to_F32(data[14], -size, size),
										U8_to_F32(data[15], -size, size) );
					if (new_angv.isExactlyZero())
					{
						// reset rotation time
						resetRot();
					}
					setAngularVelocity(new_angv);
					break;
				}

				////////////////////////////////////////////////////
				//
				// Here we handle data specific to the full message.
				//

				U32 flags;
				mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_UpdateFlags, flags, block_num);
				// clear all but local flags
				mFlags &= FLAGS_LOCAL;
				mFlags |= flags;

				U8 state;
				mesgsys->getU8Fast(_PREHASH_ObjectData, _PREHASH_State, state, block_num );
				mState = state;

				// ...new objects that should come in selected need to be added to the selected list
				mCreateSelected = ((flags & FLAGS_CREATE_SELECTED) != 0);

				// Set all name value pairs
				S32 nv_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_NameValue);
				if (nv_size > 0)
				{
					std::string name_value_list;
					mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_NameValue, name_value_list, block_num);
					setNameValueList(name_value_list);
				}

				// Clear out any existing generic data
				if (mData)
				{
					delete [] mData;
				}

				// Check for appended generic data
				S32 data_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_Data);
				if (data_size <= 0)
				{
					mData = NULL;
				}
				else
				{
					// ...has generic data
					mData = new U8[data_size];
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_Data, mData, data_size, block_num);
				}

				S32 text_size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_Text);
				if (text_size > 1)
				{
					// Setup object text
					if (!mText)
					{
						mText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
						mText->setFont(LLFontGL::getFontSansSerif());
						mText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
						mText->setMaxLines(-1);
						mText->setSourceObject(this);
						mText->setOnHUDAttachment(isHUDAttachment());
					}

					std::string temp_string;
					mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_Text, temp_string, block_num );
					
					LLColor4U coloru;
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextColor, coloru.mV, 4, block_num);

					// alpha was flipped so that it zero encoded better
					coloru.mV[3] = 255 - coloru.mV[3];
					mText->setColor(LLColor4(coloru));
					mText->setString(temp_string);

					setChanged(MOVED | SILHOUETTE);
				}
				else if (mText.notNull())
				{
					mText->markDead();
					mText = NULL;
				}

				std::string media_url;
				mesgsys->getStringFast(_PREHASH_ObjectData, _PREHASH_MediaURL, media_url, block_num);
                retval |= checkMediaURL(media_url);
                
				//
				// Unpack particle system data
				//
				unpackParticleSource(block_num, owner_id);

				// Mark all extra parameters not used
				std::map<U16, ExtraParameter*>::iterator iter;
				for (iter = mExtraParameterList.begin(); iter != mExtraParameterList.end(); ++iter)
				{
					iter->second->in_use = FALSE;
				}

				// Unpack extra parameters
				S32 size = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_ExtraParams);
				if (size > 0)
				{
					U8 *buffer = new U8[size];
					mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_ExtraParams, buffer, size, block_num);
					LLDataPackerBinaryBuffer dp(buffer, size);

					U8 num_parameters;
					dp.unpackU8(num_parameters, "num_params");
					U8 param_block[MAX_OBJECT_PARAMS_SIZE];
					for (U8 param=0; param<num_parameters; ++param)
					{
						U16 param_type;
						S32 param_size;
						dp.unpackU16(param_type, "param_type");
						dp.unpackBinaryData(param_block, param_size, "param_data");
						//llinfos << "Param type: " << param_type << ", Size: " << param_size << llendl;
						LLDataPackerBinaryBuffer dp2(param_block, param_size);
						unpackParameterEntry(param_type, &dp2);
					}
					delete[] buffer;
				}

				for (iter = mExtraParameterList.begin(); iter != mExtraParameterList.end(); ++iter)
				{
					if (!iter->second->in_use)
					{
						// Send an update message in case it was formerly in use
						parameterChanged(iter->first, iter->second->data, FALSE, false);
					}
				}

				break;
			}

		case OUT_TERSE_IMPROVED:
			{
#ifdef DEBUG_UPDATE_TYPE
				llinfos << "TI:" << getID() << llendl;
#endif
				length = mesgsys->getSizeFast(_PREHASH_ObjectData, block_num, _PREHASH_ObjectData);
				mesgsys->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_ObjectData, data, length, block_num);
				count = 0;
				LLVector4 collision_plane;
				
				switch(length)
				{
				case(60 + 16):
					// pull out collision normal for avatar
					htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
					count += sizeof(LLVector4);
					// fall through
				case 60:
					// this is a terse 32 update
					// pos
					this_update_precision = 32;
					htonmemcpy(new_pos_parent.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// vel
					htonmemcpy((void*)getVelocity().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// acc
					htonmemcpy((void*)getAcceleration().mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					count += sizeof(LLVector3);
					// theta
					{
						LLVector3 vec;
						htonmemcpy(vec.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
						new_rot.unpackFromVector3(vec);
					}
					count += sizeof(LLVector3);
					// omega
					htonmemcpy((void*)new_angv.mV, &data[count], MVT_LLVector3, sizeof(LLVector3));
					if (new_angv.isExactlyZero())
					{
						// reset rotation time
						resetRot();
					}
					setAngularVelocity(new_angv);
#if LL_DARWIN
					if (length == 76)
					{
						setAngularVelocity(LLVector3::zero);
					}
#endif
					break;
				case(32 + 16):
					// pull out collision normal for avatar
					htonmemcpy(collision_plane.mV, &data[count], MVT_LLVector4, sizeof(LLVector4));
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
					count += sizeof(LLVector4);
					// fall through
				case 32:
					// this is a terse 16 update
					this_update_precision = 16;
					test_pos_parent.quantize16(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					new_pos_parent.mV[VX] = U16_to_F32(val[VX], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VY] = U16_to_F32(val[VY], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VZ] = U16_to_F32(val[VZ], MIN_HEIGHT, MAX_HEIGHT);

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					setVelocity(U16_to_F32(val[VX], -size, size),
								U16_to_F32(val[VY], -size, size),
								U16_to_F32(val[VZ], -size, size));

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*3;
					setAcceleration(U16_to_F32(val[VX], -size, size),
									U16_to_F32(val[VY], -size, size),
									U16_to_F32(val[VZ], -size, size));

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Quat, 8); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					count += sizeof(U16)*4;
					new_rot.mQ[VX] = U16_to_F32(val[VX], -1.f, 1.f);
					new_rot.mQ[VY] = U16_to_F32(val[VY], -1.f, 1.f);
					new_rot.mQ[VZ] = U16_to_F32(val[VZ], -1.f, 1.f);
					new_rot.mQ[VW] = U16_to_F32(val[VW], -1.f, 1.f);

#ifdef LL_BIG_ENDIAN
					htonmemcpy(valswizzle, &data[count], MVT_U16Vec3, 6); 
					val = valswizzle;
#else
					val = (U16 *) &data[count];
#endif
					new_angv.set(U16_to_F32(val[VX], -size, size),
										U16_to_F32(val[VY], -size, size),
										U16_to_F32(val[VZ], -size, size));
					setAngularVelocity(new_angv);
					break;

				case 16:
					// this is a terse 8 update
					this_update_precision = 8;
					test_pos_parent.quantize8(-0.5f*size, 1.5f*size, MIN_HEIGHT, MAX_HEIGHT);
					new_pos_parent.mV[VX] = U8_to_F32(data[0], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VY] = U8_to_F32(data[1], -0.5f*size, 1.5f*size);
					new_pos_parent.mV[VZ] = U8_to_F32(data[2], MIN_HEIGHT, MAX_HEIGHT);

					setVelocity(U8_to_F32(data[3], -size, size),
								U8_to_F32(data[4], -size, size),
								U8_to_F32(data[5], -size, size) );

					setAcceleration(U8_to_F32(data[6], -size, size),
									U8_to_F32(data[7], -size, size),
									U8_to_F32(data[8], -size, size) );

					new_rot.mQ[VX] = U8_to_F32(data[9], -1.f, 1.f);
					new_rot.mQ[VY] = U8_to_F32(data[10], -1.f, 1.f);
					new_rot.mQ[VZ] = U8_to_F32(data[11], -1.f, 1.f);
					new_rot.mQ[VW] = U8_to_F32(data[12], -1.f, 1.f);

					new_angv.set(U8_to_F32(data[13], -size, size),
										U8_to_F32(data[14], -size, size),
										U8_to_F32(data[15], -size, size) );
					setAngularVelocity(new_angv);
					break;
				}

				U8 state;
				mesgsys->getU8Fast(_PREHASH_ObjectData, _PREHASH_State, state, block_num );
				mState = state;
				break;
			}

		default:
			break;

		}
	}
	else
	{
		// handle the compressed case
		LLUUID sound_uuid;
		LLUUID	owner_id;
		F32    gain = 0;
		U8     sound_flags = 0;
		F32		cutoff = 0;

		U16 val[4];

		U8		state;

		dp->unpackU8(state, "State");
		mState = state;

		switch(update_type)
		{
			case OUT_TERSE_IMPROVED:
			{
#ifdef DEBUG_UPDATE_TYPE
				llinfos << "CompTI:" << getID() << llendl;
#endif
				U8		value;
				dp->unpackU8(value, "agent");
				if (value)
				{
					LLVector4 collision_plane;
					dp->unpackVector4(collision_plane, "Plane");
					((LLVOAvatar*)this)->setFootPlane(collision_plane);
				}
				test_pos_parent = getPosition();
				dp->unpackVector3(new_pos_parent, "Pos");
				dp->unpackU16(val[VX], "VelX");
				dp->unpackU16(val[VY], "VelY");
				dp->unpackU16(val[VZ], "VelZ");
				setVelocity(U16_to_F32(val[VX], -128.f, 128.f),
							U16_to_F32(val[VY], -128.f, 128.f),
							U16_to_F32(val[VZ], -128.f, 128.f));
				dp->unpackU16(val[VX], "AccX");
				dp->unpackU16(val[VY], "AccY");
				dp->unpackU16(val[VZ], "AccZ");
				setAcceleration(U16_to_F32(val[VX], -64.f, 64.f),
								U16_to_F32(val[VY], -64.f, 64.f),
								U16_to_F32(val[VZ], -64.f, 64.f));

				dp->unpackU16(val[VX], "ThetaX");
				dp->unpackU16(val[VY], "ThetaY");
				dp->unpackU16(val[VZ], "ThetaZ");
				dp->unpackU16(val[VS], "ThetaS");
				new_rot.mQ[VX] = U16_to_F32(val[VX], -1.f, 1.f);
				new_rot.mQ[VY] = U16_to_F32(val[VY], -1.f, 1.f);
				new_rot.mQ[VZ] = U16_to_F32(val[VZ], -1.f, 1.f);
				new_rot.mQ[VS] = U16_to_F32(val[VS], -1.f, 1.f);
				dp->unpackU16(val[VX], "AccX");
				dp->unpackU16(val[VY], "AccY");
				dp->unpackU16(val[VZ], "AccZ");
				new_angv.set(U16_to_F32(val[VX], -64.f, 64.f),
									U16_to_F32(val[VY], -64.f, 64.f),
									U16_to_F32(val[VZ], -64.f, 64.f));
				setAngularVelocity(new_angv);
			}
			break;
			case OUT_FULL_COMPRESSED:
			case OUT_FULL_CACHED:
			{
#ifdef DEBUG_UPDATE_TYPE
				llinfos << "CompFull:" << getID() << llendl;
#endif
				mCostStale = true;

				if (isSelected())
				{
					gFloaterTools->dirty();
				}
	
				dp->unpackU32(crc, "CRC");
				mTotalCRC = crc;
				dp->unpackU8(material, "Material");
				U8 old_material = getMaterial();
				if (old_material != material)
				{
					setMaterial(material);
					if (mDrawable.notNull())
					{
						gPipeline.markMoved(mDrawable, FALSE); // undamped
					}
				}
				dp->unpackU8(click_action, "ClickAction");
				setClickAction(click_action);
				dp->unpackVector3(new_scale, "Scale");
				dp->unpackVector3(new_pos_parent, "Pos");
				LLVector3 vec;
				dp->unpackVector3(vec, "Rot");
				new_rot.unpackFromVector3(vec);
				setAcceleration(LLVector3::zero);

				U32 value;
				dp->unpackU32(value, "SpecialCode");
				dp->setPassFlags(value);
				dp->unpackUUID(owner_id, "Owner");

				if (value & 0x80)
				{
					dp->unpackVector3(new_angv, "Omega");
					setAngularVelocity(new_angv);
				}

				if (value & 0x20)
				{
					dp->unpackU32(parent_id, "ParentID");
				}
				else
				{
					parent_id = 0;
				}

				S32 sp_size;
				U32 size;
				if (value & 0x2)
				{
					sp_size = 1;
					delete [] mData;
					mData = new U8[1];
					dp->unpackU8(((U8*)mData)[0], "TreeData");
				}
				else if (value & 0x1)
				{
					dp->unpackU32(size, "ScratchPadSize");
					delete [] mData;
					mData = new U8[size];
					dp->unpackBinaryData((U8 *)mData, sp_size, "PartData");
				}
				else
				{
					mData = NULL;
				}

				// Setup object text
				if (!mText && (value & 0x4))
				{
					mText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
					mText->setFont(LLFontGL::getFontSansSerif());
					mText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
					mText->setMaxLines(-1); // Set to match current agni behavior.
					mText->setSourceObject(this);
					mText->setOnHUDAttachment(isHUDAttachment());
				}

				if (value & 0x4)
				{
					std::string temp_string;
					dp->unpackString(temp_string, "Text");
					LLColor4U coloru;
					dp->unpackBinaryDataFixed(coloru.mV, 4, "Color");
					coloru.mV[3] = 255 - coloru.mV[3];
					mText->setColor(LLColor4(coloru));
					mText->setString(temp_string);

					setChanged(TEXTURE);
				}
				else if(mText.notNull())
				{
					mText->markDead();
					mText = NULL;
				}

                std::string media_url;
				if (value & 0x200)
				{
					dp->unpackString(media_url, "MediaURL");
				}
                retval |= checkMediaURL(media_url);

				//
				// Unpack particle system data
				//
				if (value & 0x8)
				{
					unpackParticleSource(*dp, owner_id);
				}
				else
				{
					deleteParticleSource();
				}
				
				// Mark all extra parameters not used
				std::map<U16, ExtraParameter*>::iterator iter;
				for (iter = mExtraParameterList.begin(); iter != mExtraParameterList.end(); ++iter)
				{
					iter->second->in_use = FALSE;
				}

				// Unpack extra params
				U8 num_parameters;
				dp->unpackU8(num_parameters, "num_params");
				U8 param_block[MAX_OBJECT_PARAMS_SIZE];
				for (U8 param=0; param<num_parameters; ++param)
				{
					U16 param_type;
					S32 param_size;
					dp->unpackU16(param_type, "param_type");
					dp->unpackBinaryData(param_block, param_size, "param_data");
					//llinfos << "Param type: " << param_type << ", Size: " << param_size << llendl;
					LLDataPackerBinaryBuffer dp2(param_block, param_size);
					unpackParameterEntry(param_type, &dp2);
				}

				for (iter = mExtraParameterList.begin(); iter != mExtraParameterList.end(); ++iter)
				{
					if (!iter->second->in_use)
					{
						// Send an update message in case it was formerly in use
						parameterChanged(iter->first, iter->second->data, FALSE, false);
					}
				}

				if (value & 0x10)
				{
					dp->unpackUUID(sound_uuid, "SoundUUID");
					dp->unpackF32(gain, "SoundGain");
					dp->unpackU8(sound_flags, "SoundFlags");
					dp->unpackF32(cutoff, "SoundRadius");
				}

				if (value & 0x100)
				{
					std::string name_value_list;
					dp->unpackString(name_value_list, "NV");

					setNameValueList(name_value_list);
				}

				mTotalCRC = crc;

				setAttachedSound(sound_uuid, owner_id, gain, sound_flags);

				// only get these flags on updates from sim, not cached ones
				// Preload these five flags for every object.
				// Finer shades require the object to be selected, and the selection manager
				// stores the extended permission info.
				U32 flags;
				mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_UpdateFlags, flags, block_num);
				// keep local flags and overwrite remote-controlled flags
				mFlags = (mFlags & FLAGS_LOCAL) | flags;

					// ...new objects that should come in selected need to be added to the selected list
				mCreateSelected = ((flags & FLAGS_CREATE_SELECTED) != 0);
			}
			break;

		default:
			break;
		}
	}

	//
	// Fix object parenting.
	//
	BOOL b_changed_status = FALSE;

	if (OUT_TERSE_IMPROVED != update_type)
	{
		// We only need to update parenting on full updates, terse updates
		// don't send parenting information.
		if (!cur_parentp)
		{
			if (parent_id == 0)
			{
				// No parent now, no parent in message -> do nothing
			}
			else
			{
				// No parent now, new parent in message -> attach to that parent if possible
				LLUUID parent_uuid;
				LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														mesgsys->getSenderIP(),
														mesgsys->getSenderPort());

				LLViewerObject *sent_parentp = gObjectList.findObject(parent_uuid);

				//
				// Check to see if we have the corresponding viewer object for the parent.
				//
				if (sent_parentp && sent_parentp->getParent() == this)
				{
					// Try to recover if we attempt to attach a parent to its child
					llwarns << "Attempt to attach a parent to it's child: " << this->getID() << " to " << sent_parentp->getID() << llendl;
					this->removeChild(sent_parentp);
					sent_parentp->setDrawableParent(NULL);
				}
				
				if (sent_parentp && (sent_parentp != this) && !sent_parentp->isDead())
				{
					//
					// We have a viewer object for the parent, and it's not dead.
					// Do the actual reparenting here.
					//

					// new parent is valid
					b_changed_status = TRUE;
					// ...no current parent, so don't try to remove child
					if (mDrawable.notNull())
					{
						if (mDrawable->isDead() || !mDrawable->getVObj())
						{
							llwarns << "Drawable is dead or no VObj!" << llendl;
							sent_parentp->addChild(this);
						}
						else
						{
							if (!setDrawableParent(sent_parentp->mDrawable)) // LLViewerObject::processUpdateMessage 1
							{
								// Bad, we got a cycle somehow.
								// Kill both the parent and the child, and
								// set cache misses for both of them.
								llwarns << "Attempting to recover from parenting cycle!" << llendl;
								llwarns << "Killing " << sent_parentp->getID() << " and " << getID() << llendl;
								llwarns << "Adding to cache miss list" << llendl;
								setParent(NULL);
								sent_parentp->setParent(NULL);
								getRegion()->addCacheMissFull(getLocalID());
								getRegion()->addCacheMissFull(sent_parentp->getLocalID());
								gObjectList.killObject(sent_parentp);
								gObjectList.killObject(this);
								return retval;
							}
							sent_parentp->addChild(this);
							// make sure this object gets a non-damped update
							if (sent_parentp->mDrawable.notNull())
							{
								gPipeline.markMoved(sent_parentp->mDrawable, FALSE); // undamped
							}
						}
					}
					else
					{
						sent_parentp->addChild(this);
					}
					
					// Show particles, icon and HUD
					hideExtraDisplayItems( FALSE );

					setChanged(MOVED | SILHOUETTE);
				}
				else
				{
					//
					// No corresponding viewer object for the parent, put the various
					// pieces on the orphan list.
					//
					
					//parent_id
					U32 ip = mesgsys->getSenderIP();
					U32 port = mesgsys->getSenderPort();
					
					gObjectList.orphanize(this, parent_id, ip, port);

					// Hide particles, icon and HUD
					hideExtraDisplayItems( TRUE );
				}
			}
		}
		else
		{
			// BUG: this is a bad assumption once border crossing is alowed
			if (  (parent_id == cur_parentp->mLocalID)
				&&(update_type == OUT_TERSE_IMPROVED))
			{
				// Parent now, same parent in message -> do nothing

				// Debugging for suspected problems with local ids.
				//LLUUID parent_uuid;
				//LLViewerObjectList::getUUIDFromLocal(parent_uuid, parent_id, mesgsys->getSenderIP(), mesgsys->getSenderPort() );
				//if (parent_uuid != cur_parentp->getID() )
				//{
				//	llerrs << "Local ID match but UUID mismatch of viewer object" << llendl;
				//}
			}
			else
			{
				// Parented now, different parent in message
				LLViewerObject *sent_parentp;
				if (parent_id == 0)
				{
					//
					// This object is no longer parented, we sent in a zero parent ID.
					//
					sent_parentp = NULL;
				}
				else
				{
					LLUUID parent_uuid;
					LLViewerObjectList::getUUIDFromLocal(parent_uuid,
														parent_id,
														gMessageSystem->getSenderIP(),
														gMessageSystem->getSenderPort());
					sent_parentp = gObjectList.findObject(parent_uuid);
					
					if (isAvatar())
					{
						// This logic is meant to handle the case where a sitting avatar has reached a new sim
						// ahead of the object she was sitting on (which is common as objects are transfered through
						// a slower route than agents)...
						// In this case, the local id for the object will not be valid, since the viewer has not received
						// a full update for the object from that sim yet, so we assume that the agent is still sitting
						// where she was originally. --RN
						if (!sent_parentp)
						{
							sent_parentp = cur_parentp;
						}
					}
					else if (!sent_parentp)
					{
						//
						// Switching parents, but we don't know the new parent.
						//
						U32 ip = mesgsys->getSenderIP();
						U32 port = mesgsys->getSenderPort();

						// We're an orphan, flag things appropriately.
						gObjectList.orphanize(this, parent_id, ip, port);
					}
				}

				// Reattach if possible.
				if (sent_parentp && sent_parentp != cur_parentp && sent_parentp != this)
				{
					// New parent is valid, detach and reattach
					b_changed_status = TRUE;
					if (mDrawable.notNull())
					{
						if (!setDrawableParent(sent_parentp->mDrawable)) // LLViewerObject::processUpdateMessage 2
						{
							// Bad, we got a cycle somehow.
							// Kill both the parent and the child, and
							// set cache misses for both of them.
							llwarns << "Attempting to recover from parenting cycle!" << llendl;
							llwarns << "Killing " << sent_parentp->getID() << " and " << getID() << llendl;
							llwarns << "Adding to cache miss list" << llendl;
							setParent(NULL);
							sent_parentp->setParent(NULL);
							getRegion()->addCacheMissFull(getLocalID());
							getRegion()->addCacheMissFull(sent_parentp->getLocalID());
							gObjectList.killObject(sent_parentp);
							gObjectList.killObject(this);
							return retval;
						}
						// make sure this object gets a non-damped update
					}
					cur_parentp->removeChild(this);
					sent_parentp->addChild(this);
					setChanged(MOVED | SILHOUETTE);
					sent_parentp->setChanged(MOVED | SILHOUETTE);
					if (sent_parentp->mDrawable.notNull())
					{
						gPipeline.markMoved(sent_parentp->mDrawable, FALSE); // undamped
					}
				}
				else if (!sent_parentp)
				{
					bool remove_parent = true;
					// No new parent, or the parent that we sent doesn't exist on the viewer.
					LLViewerObject *parentp = (LLViewerObject *)getParent();
					if (parentp)
					{
						if (parentp->getRegion() != getRegion())
						{
							// This is probably an object flying across a region boundary, the
							// object probably ISN'T being reparented, but just got an object
							// update out of order (child update before parent).
							//llinfos << "Don't reparent object handoffs!" << llendl;
							remove_parent = false;
						}
					}

					if (remove_parent)
					{
						b_changed_status = TRUE;
						if (mDrawable.notNull())
						{
							// clear parent to removeChild can put the drawable on the damped list
							setDrawableParent(NULL); // LLViewerObject::processUpdateMessage 3
						}

						cur_parentp->removeChild(this);

						setChanged(MOVED | SILHOUETTE);

						if (mDrawable.notNull())
						{
							// make sure this object gets a non-damped update
							gPipeline.markMoved(mDrawable, FALSE); // undamped
						}
					}
				}
			}
		}
	}

	new_rot.normQuat();

	if (sPingInterpolate)
	{ 
		LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit(mesgsys->getSender());
		if (cdp)
		{
			F32 ping_delay = 0.5f * mTimeDilation * ( ((F32)cdp->getPingDelay()) * 0.001f + gFrameDTClamped);
			LLVector3 diff = getVelocity() * ping_delay; 
			new_pos_parent += diff;
		}
		else
		{
			llwarns << "findCircuit() returned NULL; skipping interpolation" << llendl;
		}
	}

	//////////////////////////
	//
	// Set the generic change flags...
	//
	//

	// If we're going to skip this message, why are we 
	// doing all the parenting, etc above?
	U32 packet_id = mesgsys->getCurrentRecvPacketID(); 
	if (packet_id < mLatestRecvPacketID && 
		mLatestRecvPacketID - packet_id < 65536)
	{
		//skip application of this message, it's old
		return retval;
	}

	mLatestRecvPacketID = packet_id;

	// Set the change flags for scale
	if (new_scale != getScale())
	{
		setChanged(SCALED | SILHOUETTE);
		setScale(new_scale);  // Must follow setting permYouOwner()
	}

	// first, let's see if the new position is actually a change

	//static S32 counter = 0;

	F32 vel_mag_sq = getVelocity().magVecSquared();
	F32 accel_mag_sq = getAcceleration().magVecSquared();

	if (  ((b_changed_status)||(test_pos_parent != new_pos_parent))
		||(  (!isSelected())
		   &&(  (vel_mag_sq != 0.f)
			  ||(accel_mag_sq != 0.f)
			  ||(this_update_precision > mBestUpdatePrecision))))
	{
		mBestUpdatePrecision = this_update_precision;
		
		LLVector3 diff = new_pos_parent - test_pos_parent ;
		F32 mag_sqr = diff.magVecSquared() ;
		if(llfinite(mag_sqr)) 
		{
			setPositionParent(new_pos_parent);
		}
		else
		{
			llwarns << "Can not move the object/avatar to an infinite location!" << llendl ;	

			retval |= INVALID_UPDATE ;
		}

		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			// we have changed the position of an attachment, so we need to clamp it
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;

			avatar->clampAttachmentPositions();
		}
		
		// If we're snapping the position by more than 0.5m, update LLViewerStats::mAgentPositionSnaps
		if ( asAvatar() && asAvatar()->isSelf() && (mag_sqr > 0.25f) )
		{
			LLViewerStats::getInstance()->mAgentPositionSnaps.push( diff.length() );
		}
	}

	if ((new_rot != getRotation())
		|| (new_angv != old_angv))
	{
		if (new_rot != mPreviousRotation)
		{
			resetRot();
		}
		else if (new_angv != old_angv)
		{
			if (flagUsePhysics())
			{
				resetRot();
			}
			else
			{
				resetRotTime();
			}
		}

		// Remember the last rotation value
		mPreviousRotation = new_rot;

		// Set the rotation of the object followed by adjusting for the accumulated angular velocity (llSetTargetOmega)
		setRotation(new_rot * mAngularVelocityRot);
		setChanged(ROTATED | SILHOUETTE);
	}

	if ( gShowObjectUpdates )
	{
		LLColor4 color;
		if (update_type == OUT_TERSE_IMPROVED)
		{
			color.setVec(0.f, 0.f, 1.f, 1.f);
		}
		else
		{
			color.setVec(1.f, 0.f, 0.f, 1.f);
		}
		gPipeline.addDebugBlip(getPositionAgent(), color);
	}

	const F32 MAG_CUTOFF = F_APPROXIMATELY_ZERO;

	llassert(vel_mag_sq >= 0.f);
	llassert(accel_mag_sq >= 0.f);
	llassert(getAngularVelocity().magVecSquared() >= 0.f);

	if ((MAG_CUTOFF >= vel_mag_sq) && 
		(MAG_CUTOFF >= accel_mag_sq) &&
		(MAG_CUTOFF >= getAngularVelocity().magVecSquared()))
	{
		mStatic = TRUE; // This object doesn't move!
	}
	else
	{
		mStatic = FALSE;
	}

// BUG: This code leads to problems during group rotate and any scale operation.
// Small discepencies between the simulator and viewer representations cause the 
// selection center to creep, leading to objects moving around the wrong center.
// 
// Removing this, however, means that if someone else drags an object you have
// selected, your selection center and dialog boxes will be wrong.  It also means
// that higher precision information on selected objects will be ignored.
//
// I believe the group rotation problem is fixed.  JNC 1.21.2002
//
	// Additionally, if any child is selected, need to update the dialogs and selection
	// center.
	BOOL needs_refresh = mUserSelected;
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		needs_refresh = needs_refresh || child->mUserSelected;
	}

	if (needs_refresh)
	{
		LLSelectMgr::getInstance()->updateSelectionCenter();
		dialog_refresh_all();
	} 


	// Mark update time as approx. now, with the ping delay.
	// Ping delay is off because it's not set for velocity interpolation, causing
	// much jumping and hopping around...

//	U32 ping_delay = mesgsys->mCircuitInfo.getPingDelay();
	mLastInterpUpdateSecs = LLFrameTimer::getElapsedSeconds();
	mLastMessageUpdateSecs = mLastInterpUpdateSecs;
	if (mDrawable.notNull())
	{
		// Don't clear invisibility flag on update if still orphaned!
		if (mDrawable->isState(LLDrawable::FORCE_INVISIBLE) && !mOrphaned)
		{
// 			lldebugs << "Clearing force invisible: " << mID << ":" << getPCodeString() << ":" << getPositionAgent() << llendl;
			mDrawable->setState(LLDrawable::CLEAR_INVISIBLE);
		}
	}

	// Update special hover cursor status
	bool special_hover_cursor = specialHoverCursor();
	if (old_special_hover_cursor != special_hover_cursor
		&& mDrawable.notNull())
	{
		mDrawable->updateSpecialHoverCursor(special_hover_cursor);
	}

	return retval;
}

BOOL LLViewerObject::isActive() const
{
	return TRUE;
}



void LLViewerObject::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	//static LLFastTimer::DeclareTimer ftm("Viewer Object");
	//LLFastTimer t(ftm);

	if (!mDead)
	{
	// CRO - don't velocity interp linked objects!
	// Leviathan - but DO velocity interp joints
	if (!mStatic && sVelocityInterpolate && !isSelected())
	{
		// calculate dt from last update
		F32 dt_raw = (F32)(time - mLastInterpUpdateSecs);
		F32 dt = mTimeDilation * dt_raw;

			applyAngularVelocity(dt);

			if (isAttachment())
				{
					mLastInterpUpdateSecs = time;
				return;
		}
		else
		{	// Move object based on it's velocity and rotation
			interpolateLinearMotion(time, dt);
		}
	}

	updateDrawable(FALSE);
	}
}


// Move an object due to idle-time viewer side updates by iterpolating motion
void LLViewerObject::interpolateLinearMotion(const F64 & time, const F32 & dt)
{
	// linear motion
	// PHYSICS_TIMESTEP is used below to correct for the fact that the velocity in object
	// updates represents the average velocity of the last timestep, rather than the final velocity.
	// the time dilation above should guarantee that dt is never less than PHYSICS_TIMESTEP, theoretically
	// 
	// *TODO: should also wrap linear accel/velocity in check
	// to see if object is selected, instead of explicitly
	// zeroing it out	

	F64 time_since_last_update = time - mLastMessageUpdateSecs;
	if (time_since_last_update <= 0.0 || dt <= 0.f)
	{
		return;
	}

	LLVector3 accel = getAcceleration();
	LLVector3 vel 	= getVelocity();
	
	if (sMaxUpdateInterpolationTime <= 0.0)
	{	// Old code path ... unbounded, simple interpolation
		if (!(accel.isExactlyZero() && vel.isExactlyZero()))
		{
			LLVector3 pos   = (vel + (0.5f * (dt-PHYSICS_TIMESTEP)) * accel) * dt;  
		
			// region local  
			setPositionRegion(pos + getPositionRegion());
			setVelocity(vel + accel*dt);	
			
			// for objects that are spinning but not translating, make sure to flag them as having moved
			setChanged(MOVED | SILHOUETTE);
		}
	}
	else if (!accel.isExactlyZero() || !vel.isExactlyZero())		// object is moving
	{	// Object is moving, and hasn't been too long since we got an update from the server
		
		// Calculate predicted position and velocity
		LLVector3 new_pos = (vel + (0.5f * (dt-PHYSICS_TIMESTEP)) * accel) * dt;	
		LLVector3 new_v = accel * dt;

		if (time_since_last_update > sPhaseOutUpdateInterpolationTime &&
			sPhaseOutUpdateInterpolationTime > 0.0)
		{	// Haven't seen a viewer update in a while, check to see if the ciruit is still active
			if (mRegionp)
			{	// The simulator will NOT send updates if the object continues normally on the path
				// predicted by the velocity and the acceleration (often gravity) sent to the viewer
				// So check to see if the circuit is blocked, which means the sim is likely in a long lag
				LLCircuitData *cdp = gMessageSystem->mCircuitInfo.findCircuit( mRegionp->getHost() );
				if (cdp)
				{
					// Find out how many seconds since last packet arrived on the circuit
					F64 time_since_last_packet = LLMessageSystem::getMessageTimeSeconds() - cdp->getLastPacketInTime();

					if (!cdp->isAlive() ||		// Circuit is dead or blocked
						 cdp->isBlocked() ||	// or doesn't seem to be getting any packets
						 (time_since_last_packet > sPhaseOutUpdateInterpolationTime))
					{
						// Start to reduce motion interpolation since we haven't seen a server update in a while
						F64 time_since_last_interpolation = time - mLastInterpUpdateSecs;
						F64 phase_out = 1.0;
						if (time_since_last_update > sMaxUpdateInterpolationTime)
						{	// Past the time limit, so stop the object
							phase_out = 0.0;
							//llinfos << "Motion phase out to zero" << llendl;

							// Kill angular motion as well.  Note - not adding this due to paranoia
							// about stopping rotation for llTargetOmega objects and not having it restart
							// setAngularVelocity(LLVector3::zero);
						}
						else if (mLastInterpUpdateSecs - mLastMessageUpdateSecs > sPhaseOutUpdateInterpolationTime)
						{	// Last update was already phased out a bit
							phase_out = (sMaxUpdateInterpolationTime - time_since_last_update) / 
										(sMaxUpdateInterpolationTime - time_since_last_interpolation);
							//llinfos << "Continuing motion phase out of " << (F32) phase_out << llendl;
						}
						else
						{	// Phase out from full value
							phase_out = (sMaxUpdateInterpolationTime - time_since_last_update) / 
										(sMaxUpdateInterpolationTime - sPhaseOutUpdateInterpolationTime);
							//llinfos << "Starting motion phase out of " << (F32) phase_out << llendl;
						}
						phase_out = llclamp(phase_out, 0.0, 1.0);

						new_pos = new_pos * ((F32) phase_out);
						new_v = new_v * ((F32) phase_out);
					}
				}
			}
		}

		new_pos = new_pos + getPositionRegion();
		new_v = new_v + vel;


		// Clamp interpolated position to minimum underground and maximum region height
		LLVector3d new_pos_global = mRegionp->getPosGlobalFromRegion(new_pos);
		F32 min_height;
		if (isAvatar())
		{	// Make a better guess about AVs not going underground
			min_height = LLWorld::getInstance()->resolveLandHeightGlobal(new_pos_global);
			min_height += (0.5f * getScale().mV[VZ]);
		}
		else
		{	// This will put the object underground, but we can't tell if it will stop 
			// at ground level or not
			min_height = LLWorld::getInstance()->getMinAllowedZ(this, new_pos_global);
			// Cap maximum height
			new_pos.mV[VZ] = llmin(LLWorld::getInstance()->getRegionMaxHeight(), new_pos.mV[VZ]);
		}

		new_pos.mV[VZ] = llmax(min_height, new_pos.mV[VZ]);

		// Check to see if it's going off the region
		LLVector3 temp(new_pos);
		if (temp.clamp(0.f, mRegionp->getWidth()))
		{	// Going off this region, so see if we might end up on another region
			LLVector3d old_pos_global = mRegionp->getPosGlobalFromRegion(getPositionRegion());
			new_pos_global = mRegionp->getPosGlobalFromRegion(new_pos);		// Re-fetch in case it got clipped above

			// Clip the positions to known regions
			LLVector3d clip_pos_global = LLWorld::getInstance()->clipToVisibleRegions(old_pos_global, new_pos_global);
			if (clip_pos_global != new_pos_global)
			{	// Was clipped, so this means we hit a edge where there is no region to enter
				
				//llinfos << "Hit empty region edge, clipped predicted position to " << mRegionp->getPosRegionFromGlobal(clip_pos_global)
				//	<< " from " << new_pos << llendl;
				new_pos = mRegionp->getPosRegionFromGlobal(clip_pos_global);
				
				// Stop motion and get server update for bouncing on the edge
				new_v.clear();
				setAcceleration(LLVector3::zero);
			}
			else
			{	// Let predicted movement cross into another region
				//llinfos << "Predicting region crossing to " << new_pos << llendl;
			}
		}

		// Set new position and velocity
		setPositionRegion(new_pos);
		setVelocity(new_v);	
		
		// for objects that are spinning but not translating, make sure to flag them as having moved
		setChanged(MOVED | SILHOUETTE);
	}		

	// Update the last time we did anything
	mLastInterpUpdateSecs = time;
}



BOOL LLViewerObject::setData(const U8 *datap, const U32 data_size)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	delete [] mData;

	if (datap)
	{
		mData = new U8[data_size];
		if (!mData)
		{
			return FALSE;
		}
		memcpy(mData, datap, data_size);		/* Flawfinder: ignore */
	}
	return TRUE;
}

// delete an item in the inventory, but don't tell the server. This is
// used internally by remove, update, and savescript.
// This will only delete the first item with an item_id in the list
void LLViewerObject::deleteInventoryItem(const LLUUID& item_id)
{
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it )
		{
			if((*it)->getUUID() == item_id)
			{
				// This is safe only because we return immediatly.
				mInventory->erase(it); // will deref and delete it
				return;
			}
		}
		doInventoryCallback();
	}
}

void LLViewerObject::doUpdateInventory(
	LLPointer<LLViewerInventoryItem>& item,
	U8 key,
	bool is_new)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);

	LLViewerInventoryItem* old_item = NULL;
	if(TASK_INVENTORY_ITEM_KEY == key)
	{
		old_item = (LLViewerInventoryItem*)getInventoryObject(item->getUUID());
	}
	else if(TASK_INVENTORY_ASSET_KEY == key)
	{
		old_item = getInventoryItemByAsset(item->getAssetUUID());
	}
	LLUUID item_id;
	LLUUID new_owner;
	LLUUID new_group;
	BOOL group_owned = FALSE;
	if(old_item)
	{
		item_id = old_item->getUUID();
		new_owner = old_item->getPermissions().getOwner();
		new_group = old_item->getPermissions().getGroup();
		group_owned = old_item->getPermissions().isGroupOwned();
		old_item = NULL;
	}
	else
	{
		item_id = item->getUUID();
	}
	if(!is_new && mInventory)
	{
		// Attempt to update the local inventory. If we can get the
		// object perm, we have perfect visibility, so we want the
		// serial number to match. Otherwise, take our best guess and
		// make sure that the serial number does not match.
		deleteInventoryItem(item_id);
		LLPermissions perm(item->getPermissions());
		LLPermissions* obj_perm = LLSelectMgr::getInstance()->findObjectPermissions(this);
		bool is_atomic = ((S32)LLAssetType::AT_OBJECT == item->getType()) ? false : true;
		if(obj_perm)
		{
			perm.setOwnerAndGroup(LLUUID::null, obj_perm->getOwner(), obj_perm->getGroup(), is_atomic);
		}
		else
		{
			if(group_owned)
			{
				perm.setOwnerAndGroup(LLUUID::null, new_owner, new_group, is_atomic);
			}
			else if(!new_owner.isNull())
			{
				// The object used to be in inventory, so we can
				// assume the owner and group will match what they are
				// there.
				perm.setOwnerAndGroup(LLUUID::null, new_owner, new_group, is_atomic);
			}
			// *FIX: can make an even better guess by using the mPermGroup flags
			else if(permYouOwner())
			{
				// best guess.
				perm.setOwnerAndGroup(LLUUID::null, gAgent.getID(), item->getPermissions().getGroup(), is_atomic);
				--mInventorySerialNum;
			}
			else
			{
				// dummy it up.
				perm.setOwnerAndGroup(LLUUID::null, LLUUID::null, LLUUID::null, is_atomic);
				--mInventorySerialNum;
			}
		}
		LLViewerInventoryItem* oldItem = item;
		LLViewerInventoryItem* new_item = new LLViewerInventoryItem(oldItem);
		new_item->setPermissions(perm);
		mInventory->push_front(new_item);
		doInventoryCallback();
		++mInventorySerialNum;
	}
}

// save a script, which involves removing the old one, and rezzing
// in the new one. This method should be called with the asset id
// of the new and old script AFTER the bytecode has been saved.
void LLViewerObject::saveScript(
	const LLViewerInventoryItem* item,
	BOOL active,
	bool is_new)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);

	/*
	 * XXXPAM Investigate not making this copy.  Seems unecessary, but I'm unsure about the
	 * interaction with doUpdateInventory() called below.
	 */
	lldebugs << "LLViewerObject::saveScript() " << item->getUUID() << " " << item->getAssetUUID() << llendl;
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(), item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RezScript);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
	msg->nextBlockFast(_PREHASH_UpdateBlock);
	msg->addU32Fast(_PREHASH_ObjectLocalID, (mLocalID));
	U8 enabled = active;
	msg->addBOOLFast(_PREHASH_Enabled, enabled);
	msg->nextBlockFast(_PREHASH_InventoryBlock);
	task_item->packMessage(msg);
	msg->sendReliable(mRegionp->getHost());

	// do the internal logic
	doUpdateInventory(task_item, TASK_INVENTORY_ITEM_KEY, is_new);
}

void LLViewerObject::moveInventory(const LLUUID& folder_id,
								   const LLUUID& item_id)
{
	lldebugs << "LLViewerObject::moveInventory " << item_id << llendl;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MoveTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_FolderID, folder_id);
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->sendReliable(mRegionp->getHost());

	LLInventoryObject* inv_obj = getInventoryObject(item_id);
	if(inv_obj)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_obj;
		if(!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			deleteInventoryItem(item_id);
			++mInventorySerialNum;
		}
	}
}

void LLViewerObject::dirtyInventory()
{
	// If there aren't any LLVOInventoryListeners, we won't be
	// able to update our mInventory when it comes back from the
	// simulator, so we should not clear the inventory either.
	if(mInventory && !mInventoryCallbacks.empty())
	{
		mInventory->clear(); // will deref and delete entries
		delete mInventory;
		mInventory = NULL;
		mInventoryDirty = TRUE;
	}
}

void LLViewerObject::registerInventoryListener(LLVOInventoryListener* listener, void* user_data)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	LLInventoryCallbackInfo* info = new LLInventoryCallbackInfo;
	info->mListener = listener;
	info->mInventoryData = user_data;
	mInventoryCallbacks.push_front(info);
}

void LLViewerObject::removeInventoryListener(LLVOInventoryListener* listener)
{
	if (listener == NULL)
		return;
	for (callback_list_t::iterator iter = mInventoryCallbacks.begin();
		 iter != mInventoryCallbacks.end(); )
	{
		callback_list_t::iterator curiter = iter++;
		LLInventoryCallbackInfo* info = *curiter;
		if (info->mListener == listener)
		{
			delete info;
			mInventoryCallbacks.erase(curiter);
			break;
		}
	}
}

void LLViewerObject::clearInventoryListeners()
{
	for_each(mInventoryCallbacks.begin(), mInventoryCallbacks.end(), DeletePointer());
	mInventoryCallbacks.clear();
}

void LLViewerObject::requestInventory()
{
	mInventoryDirty = FALSE;
	if(mInventory)
	{
		//mInventory->clear() // will deref and delete it
		//delete mInventory;
		//mInventory = NULL;
		doInventoryCallback();
	}
	// throw away duplicate requests
	else
	{
		fetchInventoryFromServer();
	}
}

void LLViewerObject::fetchInventoryFromServer()
{
	if (!mInventoryPending)
	{
		delete mInventory;
		mInventory = NULL;
		mInventoryDirty = FALSE;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_RequestTaskInventory);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_InventoryData);
		msg->addU32Fast(_PREHASH_LocalID, mLocalID);
		msg->sendReliable(mRegionp->getHost());

		// this will get reset by dirtyInventory or doInventoryCallback
		mInventoryPending = TRUE;
	}
}

struct LLFilenameAndTask
{
	LLUUID mTaskID;
	std::string mFilename;
#ifdef _DEBUG
	static S32 sCount;
	LLFilenameAndTask()
	{
		++sCount;
		lldebugs << "Constructing LLFilenameAndTask: " << sCount << llendl;
	}
	~LLFilenameAndTask()
	{
		--sCount;
		lldebugs << "Destroying LLFilenameAndTask: " << sCount << llendl;
	}
private:
	LLFilenameAndTask(const LLFilenameAndTask& rhs);
	const LLFilenameAndTask& operator=(const LLFilenameAndTask& rhs) const;
#endif
};

#ifdef _DEBUG
S32 LLFilenameAndTask::sCount = 0;
#endif

// static
void LLViewerObject::processTaskInv(LLMessageSystem* msg, void** user_data)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	LLUUID task_id;
	msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_TaskID, task_id);
	LLViewerObject* object = gObjectList.findObject(task_id);
	if(!object)
	{
		llwarns << "LLViewerObject::processTaskInv object "
			<< task_id << " does not exist." << llendl;
		return;
	}

	msg->getS16Fast(_PREHASH_InventoryData, _PREHASH_Serial, object->mInventorySerialNum);
	LLFilenameAndTask* ft = new LLFilenameAndTask;
	ft->mTaskID = task_id;

	std::string unclean_filename;
	msg->getStringFast(_PREHASH_InventoryData, _PREHASH_Filename, unclean_filename);
	ft->mFilename = LLDir::getScrubbedFileName(unclean_filename);
	
	if(ft->mFilename.empty())
	{
		lldebugs << "Task has no inventory" << llendl;
		// mock up some inventory to make a drop target.
		if(object->mInventory)
		{
			object->mInventory->clear(); // will deref and delete it
		}
		else
		{
			object->mInventory = new LLInventoryObject::object_list_t();
		}
		LLPointer<LLInventoryObject> obj;
		obj = new LLInventoryObject(object->mID, LLUUID::null,
									LLAssetType::AT_CATEGORY,
									"Contents");
		object->mInventory->push_front(obj);
		object->doInventoryCallback();
		delete ft;
		return;
	}
	gXferManager->requestFile(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, ft->mFilename), 
								ft->mFilename, LL_PATH_CACHE,
								object->mRegionp->getHost(),
								TRUE,
								&LLViewerObject::processTaskInvFile,
								(void**)ft,
								LLXferManager::HIGH_PRIORITY);
}

void LLViewerObject::processTaskInvFile(void** user_data, S32 error_code, LLExtStat ext_status)
{
	LLFilenameAndTask* ft = (LLFilenameAndTask*)user_data;
	LLViewerObject* object = NULL;
	if(ft && (0 == error_code) &&
	   (object = gObjectList.findObject(ft->mTaskID)))
	{
		object->loadTaskInvFile(ft->mFilename);

		LLInventoryObject::object_list_t::iterator it = object->mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = object->mInventory->end();
		std::list<LLUUID>& pending_lst = object->mPendingInventoryItemsIDs;

		for (; it != end && pending_lst.size(); ++it)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(it->get());
			if(item && item->getType() != LLAssetType::AT_CATEGORY)
			{
				std::list<LLUUID>::iterator id_it = std::find(pending_lst.begin(), pending_lst.begin(), item->getAssetUUID());
				if (id_it != pending_lst.end())
				{
					pending_lst.erase(id_it);
				}
			}
		}
	}
	else
	{
		// This Occurs When to requests were made, and the first one
		// has already handled it.
		lldebugs << "Problem loading task inventory. Return code: "
				 << error_code << llendl;
	}
	delete ft;
}

void LLViewerObject::loadTaskInvFile(const std::string& filename)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	std::string filename_and_local_path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE, filename);
	llifstream ifs(filename_and_local_path);
	if(ifs.good())
	{
		char buffer[MAX_STRING];	/* Flawfinder: ignore */
		// *NOTE: This buffer size is hard coded into scanf() below.
		char keyword[MAX_STRING];	/* Flawfinder: ignore */
		if(mInventory)
		{
			mInventory->clear(); // will deref and delete it
		}
		else
		{
			mInventory = new LLInventoryObject::object_list_t;
		}
		while(ifs.good())
		{
			ifs.getline(buffer, MAX_STRING);
			sscanf(buffer, " %254s", keyword);	/* Flawfinder: ignore */
			if(0 == strcmp("inv_item", keyword))
			{
				LLPointer<LLInventoryObject> inv = new LLViewerInventoryItem;
				inv->importLegacyStream(ifs);
				mInventory->push_front(inv);
			}
			else if(0 == strcmp("inv_object", keyword))
			{
				LLPointer<LLInventoryObject> inv = new LLInventoryObject;
				inv->importLegacyStream(ifs);
				inv->rename("Contents");
				mInventory->push_front(inv);
			}
			else
			{
				llwarns << "Unknown token in inventory file '"
						<< keyword << "'" << llendl;
			}
		}
		ifs.close();
		LLFile::remove(filename_and_local_path);
	}
	else
	{
		llwarns << "unable to load task inventory: " << filename_and_local_path
				<< llendl;
	}
	doInventoryCallback();
}

void LLViewerObject::doInventoryCallback()
{
	for (callback_list_t::iterator iter = mInventoryCallbacks.begin();
		 iter != mInventoryCallbacks.end(); )
	{
		callback_list_t::iterator curiter = iter++;
		LLInventoryCallbackInfo* info = *curiter;
		if (info->mListener != NULL)
		{
			info->mListener->inventoryChanged(this,
								 mInventory,
								 mInventorySerialNum,
								 info->mInventoryData);
		}
		else
		{
			llinfos << "LLViewerObject::doInventoryCallback() deleting bad listener entry." << llendl;
			delete info;
			mInventoryCallbacks.erase(curiter);
		}
	}
	mInventoryPending = FALSE;
}

void LLViewerObject::removeInventory(const LLUUID& item_id)
{
	// close any associated floater properties
	LLFloaterReg::hideInstance("properties", item_id);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_InventoryData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->sendReliable(mRegionp->getHost());
	deleteInventoryItem(item_id);
	++mInventorySerialNum;
}

bool LLViewerObject::isTextureInInventory(LLViewerInventoryItem* item)
{
	bool result = false;

	if (item && LLAssetType::AT_TEXTURE == item->getType())
	{
		std::list<LLUUID>::iterator begin = mPendingInventoryItemsIDs.begin();
		std::list<LLUUID>::iterator end = mPendingInventoryItemsIDs.end();

		bool is_fetching = std::find(begin, end, item->getAssetUUID()) != end;
		bool is_fetched = getInventoryItemByAsset(item->getAssetUUID()) != NULL;

		result = is_fetched || is_fetching;
	}

	return result;
}

void LLViewerObject::updateTextureInventory(LLViewerInventoryItem* item, U8 key, bool is_new)
{
	if (item && !isTextureInInventory(item))
	{
		mPendingInventoryItemsIDs.push_back(item->getAssetUUID());
		updateInventory(item, key, is_new);
	}
}

void LLViewerObject::updateInventory(
	LLViewerInventoryItem* item,
	U8 key,
	bool is_new)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);

	// This slices the object into what we're concerned about on the
	// viewer. The simulator will take the permissions and transfer
	// ownership.
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(),
								  item->getFlags(),
								  item->getCreationDate());
	task_item->setTransactionID(item->getTransactionID());
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateTaskInventory);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_UpdateData);
	msg->addU32Fast(_PREHASH_LocalID, mLocalID);
	msg->addU8Fast(_PREHASH_Key, key);
	msg->nextBlockFast(_PREHASH_InventoryData);
	task_item->packMessage(msg);
	msg->sendReliable(mRegionp->getHost());

	// do the internal logic
	doUpdateInventory(task_item, key, is_new);
}

void LLViewerObject::updateInventoryLocal(LLInventoryItem* item, U8 key)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item->getUUID(), mID, item->getPermissions(),
								  item->getAssetUUID(), item->getType(),
								  item->getInventoryType(),
								  item->getName(), item->getDescription(),
								  item->getSaleInfo(), item->getFlags(),
								  item->getCreationDate());

	// do the internal logic
	const bool is_new = false;
	doUpdateInventory(task_item, key, is_new);
}

LLInventoryObject* LLViewerObject::getInventoryObject(const LLUUID& item_id)
{
	LLInventoryObject* rv = NULL;
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for ( ; it != end; ++it)
		{
			if((*it)->getUUID() == item_id)
			{
				rv = *it;
				break;
			}
		}		
	}
	return rv;
}

void LLViewerObject::getInventoryContents(LLInventoryObject::object_list_t& objects)
{
	if(mInventory)
	{
		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it)
		{
			if ((*it)->getType() != LLAssetType::AT_CATEGORY)
			{
				objects.push_back(*it);
			}
		}
	}
}

LLInventoryObject* LLViewerObject::getInventoryRoot()
{
	if (!mInventory || !mInventory->size())
	{
		return NULL;
	}
	return mInventory->back();
}

LLViewerInventoryItem* LLViewerObject::getInventoryItemByAsset(const LLUUID& asset_id)
{
	if (mInventoryDirty)
		llwarns << "Peforming inventory lookup for object " << mID << " that has dirty inventory!" << llendl;

	LLViewerInventoryItem* rv = NULL;
	if(mInventory)
	{
		LLViewerInventoryItem* item = NULL;

		LLInventoryObject::object_list_t::iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::iterator end = mInventory->end();
		for( ; it != end; ++it)
		{
			LLInventoryObject* obj = *it;
			if(obj->getType() != LLAssetType::AT_CATEGORY)
			{
				// *FIX: gank-ass down cast!
				item = (LLViewerInventoryItem*)obj;
				if(item->getAssetUUID() == asset_id)
				{
					rv = item;
					break;
				}
			}
		}		
	}
	return rv;
}

void LLViewerObject::updateViewerInventoryAsset(
					const LLViewerInventoryItem* item,
					const LLUUID& new_asset)
{
	LLPointer<LLViewerInventoryItem> task_item =
		new LLViewerInventoryItem(item);
	task_item->setAssetUUID(new_asset);

	// do the internal logic
	doUpdateInventory(task_item, TASK_INVENTORY_ITEM_KEY, false);
}

void LLViewerObject::setPixelAreaAndAngle(LLAgent &agent)
{
	if (getVolume())
	{	//volumes calculate pixel area and angle per face
		return;
	}
	
	LLVector3 viewer_pos_agent = gAgentCamera.getCameraPositionAgent();
	LLVector3 pos_agent = getRenderPosition();

	F32 dx = viewer_pos_agent.mV[VX] - pos_agent.mV[VX];
	F32 dy = viewer_pos_agent.mV[VY] - pos_agent.mV[VY];
	F32 dz = viewer_pos_agent.mV[VZ] - pos_agent.mV[VZ];

	F32 max_scale = getMaxScale();
	F32 mid_scale = getMidScale();
	F32 min_scale = getMinScale();

	// IW: estimate - when close to large objects, computing range based on distance from center is no good
	// to try to get a min distance from face, subtract min_scale/2 from the range.
	// This means we'll load too much detail sometimes, but that's better than not enough
	// I don't think there's a better way to do this without calculating distance per-poly
	F32 range = sqrt(dx*dx + dy*dy + dz*dz) - min_scale/2;

	LLViewerCamera* camera = LLViewerCamera::getInstance();
	if (range < 0.001f || isHUDAttachment())		// range == zero
	{
		mAppAngle = 180.f;
		mPixelArea = (F32)camera->getScreenPixelArea();
	}
	else
	{
		mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;

		F32 pixels_per_meter = camera->getPixelMeterRatio() / range;

		mPixelArea = (pixels_per_meter * max_scale) * (pixels_per_meter * mid_scale);
		if (mPixelArea > camera->getScreenPixelArea())
		{
			mAppAngle = 180.f;
			mPixelArea = (F32)camera->getScreenPixelArea();
		}
	}
}

BOOL LLViewerObject::updateLOD()
{
	return FALSE;
}

BOOL LLViewerObject::updateGeometry(LLDrawable *drawable)
{
	return TRUE;
}

void LLViewerObject::updateGL()
{

}

void LLViewerObject::updateFaceSize(S32 idx)
{
	
}

LLDrawable* LLViewerObject::createDrawable(LLPipeline *pipeline)
{
	return NULL;
}

void LLViewerObject::setScale(const LLVector3 &scale, BOOL damped)
{
	LLPrimitive::setScale(scale);
	if (mDrawable.notNull())
	{
		//encompass completely sheared objects by taking 
		//the most extreme point possible (<1,1,0.5>)
		mDrawable->setRadius(LLVector3(1,1,0.5f).scaleVec(scale).magVec());
		updateDrawable(damped);
	}

	if( (LL_PCODE_VOLUME == getPCode()) && !isDead() )
	{
		if (permYouOwner() || (scale.magVecSquared() > (7.5f * 7.5f)) )
		{
			if (!mOnMap)
			{
				llassert_always(LLWorld::getInstance()->getRegionFromHandle(getRegion()->getHandle()));

				gObjectList.addToMap(this);
				mOnMap = TRUE;
			}
		}
		else
		{
			if (mOnMap)
			{
				gObjectList.removeFromMap(this);
				mOnMap = FALSE;
			}
		}
	}
}

void LLViewerObject::setObjectCost(F32 cost)
{
	mObjectCost = cost;
	mCostStale = false;

	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}

void LLViewerObject::setLinksetCost(F32 cost)
{
	mLinksetCost = cost;
	mCostStale = false;
	
	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}

void LLViewerObject::setPhysicsCost(F32 cost)
{
	mPhysicsCost = cost;
	mCostStale = false;

	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}

void LLViewerObject::setLinksetPhysicsCost(F32 cost)
{
	mLinksetPhysicsCost = cost;
	mCostStale = false;
	
	if (isSelected())
	{
		gFloaterTools->dirty();
	}
}


F32 LLViewerObject::getObjectCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	
	return mObjectCost;
}

F32 LLViewerObject::getLinksetCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}

	return mLinksetCost;
}

F32 LLViewerObject::getPhysicsCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}
	
	return mPhysicsCost;
}

F32 LLViewerObject::getLinksetPhysicsCost()
{
	if (mCostStale)
	{
		gObjectList.updateObjectCost(this);
	}

	return mLinksetPhysicsCost;
}

F32 LLViewerObject::getStreamingCost(S32* bytes, S32* visible_bytes, F32* unscaled_value) const
{
	return 0.f;
}

U32 LLViewerObject::getTriangleCount(S32* vcount) const
{
	return 0;
}

U32 LLViewerObject::getHighLODTriangleCount()
{
	return 0;
}

void LLViewerObject::updateSpatialExtents(LLVector4a& newMin, LLVector4a &newMax)
{
	LLVector4a center;
	center.load3(getRenderPosition().mV);
	LLVector4a size;
	size.load3(getScale().mV);
	newMin.setSub(center, size);
	newMax.setAdd(center, size);
	
	mDrawable->setPositionGroup(center);
}

F32 LLViewerObject::getBinRadius()
{
	if (mDrawable.notNull())
	{
		const LLVector4a* ext = mDrawable->getSpatialExtents();
		LLVector4a diff;
		diff.setSub(ext[1], ext[0]);
		return diff.getLength3().getF32();
	}
	
	return getScale().magVec();
}

F32 LLViewerObject::getMaxScale() const
{
	return llmax(getScale().mV[VX],getScale().mV[VY], getScale().mV[VZ]);
}

F32 LLViewerObject::getMinScale() const
{
	return llmin(getScale().mV[0],getScale().mV[1],getScale().mV[2]);
}

F32 LLViewerObject::getMidScale() const
{
	if (getScale().mV[VX] < getScale().mV[VY])
	{
		if (getScale().mV[VY] < getScale().mV[VZ])
		{
			return getScale().mV[VY];
		}
		else if (getScale().mV[VX] < getScale().mV[VZ])
		{
			return getScale().mV[VZ];
		}
		else
		{
			return getScale().mV[VX];
		}
	}
	else if (getScale().mV[VX] < getScale().mV[VZ])
	{
		return getScale().mV[VX];
	}
	else if (getScale().mV[VY] < getScale().mV[VZ])
	{
		return getScale().mV[VZ];
	}
	else
	{
		return getScale().mV[VY];
	}
}


void LLViewerObject::updateTextures()
{
}

void LLViewerObject::boostTexturePriority(BOOL boost_children /* = TRUE */)
{
	if (isDead())
	{
		return;
	}

	S32 i;
	S32 tex_count = getNumTEs();
	for (i = 0; i < tex_count; i++)
	{
 		getTEImage(i)->setBoostLevel(LLGLTexture::BOOST_SELECTED);
	}

	if (isSculpted() && !isMesh())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *)getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID sculpt_id = sculpt_params->getSculptTexture();
		LLViewerTextureManager::getFetchedTexture(sculpt_id, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE)->setBoostLevel(LLGLTexture::BOOST_SELECTED);
	}
	
	if (boost_children)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->boostTexturePriority();
		}
	}
}


void LLViewerObject::setLineWidthForWindowSize(S32 window_width)
{
	if (window_width < 700)
	{
		LLUI::setLineWidth(2.0f);
	}
	else if (window_width < 1100)
	{
		LLUI::setLineWidth(3.0f);
	}
	else if (window_width < 2000)
	{
		LLUI::setLineWidth(4.0f);
	}
	else
	{
		// _damn_, what a nice monitor!
		LLUI::setLineWidth(5.0f);
	}
}

void LLViewerObject::increaseArrowLength()
{
/* ???
	if (mAxisArrowLength == 50)
	{
		mAxisArrowLength = 100;
	}
	else
	{
		mAxisArrowLength = 150;
	}
*/
}


void LLViewerObject::decreaseArrowLength()
{
/* ???
	if (mAxisArrowLength == 150)
	{
		mAxisArrowLength = 100;
	}
	else
	{
		mAxisArrowLength = 50;
	}
*/
}

// Culled from newsim LLTask::addNVPair
void LLViewerObject::addNVPair(const std::string& data)
{
	// cout << "LLViewerObject::addNVPair() with ---" << data << "---" << endl;
	LLNameValue *nv = new LLNameValue(data.c_str());

//	char splat[MAX_STRING];
//	temp->printNameValue(splat);
//	llinfos << "addNVPair " << splat << llendl;

	name_value_map_t::iterator iter = mNameValuePairs.find(nv->mName);
	if (iter != mNameValuePairs.end())
	{
		LLNameValue* foundnv = iter->second;
		if (foundnv->mClass != NVC_READ_ONLY)
		{
			delete foundnv;
			mNameValuePairs.erase(iter);
		}
		else
		{
			delete nv;
//			llinfos << "Trying to write to Read Only NVPair " << temp->mName << " in addNVPair()" << llendl;
			return;
		}
	}
	mNameValuePairs[nv->mName] = nv;
}

BOOL LLViewerObject::removeNVPair(const std::string& name)
{
	char* canonical_name = gNVNameTable.addString(name);

	lldebugs << "LLViewerObject::removeNVPair(): " << name << llendl;

	name_value_map_t::iterator iter = mNameValuePairs.find(canonical_name);
	if (iter != mNameValuePairs.end())
	{
		if( mRegionp )
		{
			LLNameValue* nv = iter->second;
/*
			std::string buffer = nv->printNameValue();
			gMessageSystem->newMessageFast(_PREHASH_RemoveNameValuePair);
			gMessageSystem->nextBlockFast(_PREHASH_TaskData);
			gMessageSystem->addUUIDFast(_PREHASH_ID, mID);
			
			gMessageSystem->nextBlockFast(_PREHASH_NameValueData);
			gMessageSystem->addStringFast(_PREHASH_NVPair, buffer);

			gMessageSystem->sendReliable( mRegionp->getHost() );
*/
			// Remove the NV pair from the local list.
			delete nv;
			mNameValuePairs.erase(iter);
			return TRUE;
		}
		else
		{
			lldebugs << "removeNVPair - No region for object" << llendl;
		}
	}
	return FALSE;
}


LLNameValue *LLViewerObject::getNVPair(const std::string& name) const
{
	char		*canonical_name;

	canonical_name = gNVNameTable.addString(name);

	// If you access a map with a name that isn't in it, it will add the name and a null pointer.
	// So first check if the data is in the map.
	name_value_map_t::const_iterator iter = mNameValuePairs.find(canonical_name);
	if (iter != mNameValuePairs.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

void LLViewerObject::updatePositionCaches() const
{
	// If region is removed from the list it is also deleted.
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		if (!isRoot())
		{
			mPositionRegion = ((LLViewerObject *)getParent())->getPositionRegion() + getPosition() * getParent()->getRotation();
			mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
		}
		else
		{
			mPositionRegion = getPosition();
			mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
		}
	}
}

const LLVector3d LLViewerObject::getPositionGlobal() const
{	
	// If region is removed from the list it is also deleted.
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		LLVector3d position_global = mRegionp->getPosGlobalFromRegion(getPositionRegion());

		if (isAttachment())
		{
			position_global = gAgent.getPosGlobalFromAgent(getRenderPosition());
		}		
		return position_global;
	}
	else
	{
		LLVector3d position_global(getPosition());
		return position_global;
	}	
}

const LLVector3 &LLViewerObject::getPositionAgent() const
{
	// If region is removed from the list it is also deleted.
	if(mRegionp && LLWorld::instance().isRegionListed(mRegionp))
	{
		if (mDrawable.notNull() && (!mDrawable->isRoot() && getParent()))
		{
			// Don't return cached position if you have a parent, recalc (until all dirtying is done correctly.
			LLVector3 position_region;
			position_region = ((LLViewerObject *)getParent())->getPositionRegion() + getPosition() * getParent()->getRotation();
			mPositionAgent = mRegionp->getPosAgentFromRegion(position_region);
		}
		else
		{
			mPositionAgent = mRegionp->getPosAgentFromRegion(getPosition());
		}
	}
	return mPositionAgent;
}

const LLVector3 &LLViewerObject::getPositionRegion() const
{
	if (!isRoot())
	{
		LLViewerObject *parent = (LLViewerObject *)getParent();
		mPositionRegion = parent->getPositionRegion() + (getPosition() * parent->getRotation());
	}
	else
	{
		mPositionRegion = getPosition();
	}

	return mPositionRegion;
}

const LLVector3 LLViewerObject::getPositionEdit() const
{
	if (isRootEdit())
	{
		return getPosition();
	}
	else
	{
		LLViewerObject *parent = (LLViewerObject *)getParent();
		LLVector3 position_edit = parent->getPositionEdit() + getPosition() * parent->getRotationEdit();
		return position_edit;
	}
}

const LLVector3 LLViewerObject::getRenderPosition() const
{
	if (mDrawable.notNull() && mDrawable->isState(LLDrawable::RIGGED))
	{
		LLVOAvatar* avatar = getAvatar();
		if (avatar)
		{
			return avatar->getPositionAgent();
		}
	}

	if (mDrawable.isNull() || mDrawable->getGeneration() < 0)
	{
		return getPositionAgent();
	}
	else
	{
		return mDrawable->getPositionAgent();
	}
}

const LLVector3 LLViewerObject::getPivotPositionAgent() const
{
	return getRenderPosition();
}

const LLQuaternion LLViewerObject::getRenderRotation() const
{
	LLQuaternion ret;
	if (mDrawable.notNull() && mDrawable->isState(LLDrawable::RIGGED))
	{
		return ret;
	}
	
	if (mDrawable.isNull() || mDrawable->isStatic())
	{
		ret = getRotationEdit();
	}
	else
	{
		if (!mDrawable->isRoot())
		{
			ret = getRotation() * LLQuaternion(mDrawable->getParent()->getWorldMatrix());
		}
		else
		{
			ret = LLQuaternion(mDrawable->getWorldMatrix());
		}
	}
	
	return ret;
}

const LLMatrix4 LLViewerObject::getRenderMatrix() const
{
	return mDrawable->getWorldMatrix();
}

const LLQuaternion LLViewerObject::getRotationRegion() const
{
	LLQuaternion global_rotation = getRotation();
	if (!((LLXform *)this)->isRoot())
	{
		global_rotation = global_rotation * getParent()->getRotation();
	}
	return global_rotation;
}

const LLQuaternion LLViewerObject::getRotationEdit() const
{
	LLQuaternion global_rotation = getRotation();
	if (!((LLXform *)this)->isRootEdit())
	{
		global_rotation = global_rotation * getParent()->getRotation();
	}
	return global_rotation;
}

void LLViewerObject::setPositionAbsoluteGlobal( const LLVector3d &pos_global, BOOL damped )
{
	if (isAttachment())
	{
		LLVector3 new_pos = mRegionp->getPosRegionFromGlobal(pos_global);
		if (isRootEdit())
		{
			new_pos -= mDrawable->mXform.getParent()->getWorldPosition();
			LLQuaternion world_rotation = mDrawable->mXform.getParent()->getWorldRotation();
			new_pos = new_pos * ~world_rotation;
		}
		else
		{
			LLViewerObject* parentp = (LLViewerObject*)getParent();
			new_pos -= parentp->getPositionAgent();
			new_pos = new_pos * ~parentp->getRotationRegion();
		}
		LLViewerObject::setPosition(new_pos);
		
		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			// we have changed the position of an attachment, so we need to clamp it
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;

			avatar->clampAttachmentPositions();
		}
	}
	else
	{
		if( isRoot() )
		{
			setPositionRegion(mRegionp->getPosRegionFromGlobal(pos_global));
		}
		else
		{
			// the relative position with the parent is not constant
			LLViewerObject* parent = (LLViewerObject *)getParent();
			//RN: this assumes we are only calling this function from the edit tools
			gPipeline.updateMoveNormalAsync(parent->mDrawable);

			LLVector3 pos_local = mRegionp->getPosRegionFromGlobal(pos_global) - parent->getPositionRegion();
			pos_local = pos_local * ~parent->getRotationRegion();
			LLViewerObject::setPosition( pos_local );
		}
	}
	//RN: assumes we always want to snap the object when calling this function
	gPipeline.updateMoveNormalAsync(mDrawable);
}

void LLViewerObject::setPosition(const LLVector3 &pos, BOOL damped)
{
	if (getPosition() != pos)
	{
		setChanged(TRANSLATED | SILHOUETTE);
	}
		
	LLXform::setPosition(pos);
	updateDrawable(damped);
	if (isRoot())
	{
		// position caches need to be up to date on root objects
		updatePositionCaches();
	}
}

void LLViewerObject::setPositionGlobal(const LLVector3d &pos_global, BOOL damped)
{
	if (isAttachment())
	{
		if (isRootEdit())
		{
			LLVector3 newPos = mRegionp->getPosRegionFromGlobal(pos_global);
			newPos = newPos - mDrawable->mXform.getParent()->getWorldPosition();

			LLQuaternion invWorldRotation = mDrawable->mXform.getParent()->getWorldRotation();
			invWorldRotation.transQuat();

			newPos = newPos * invWorldRotation;
			LLViewerObject::setPosition(newPos);
		}
		else
		{
			// assumes parent is root editable (root of attachment)
			LLVector3 newPos = mRegionp->getPosRegionFromGlobal(pos_global);
			newPos = newPos - mDrawable->mXform.getParent()->getWorldPosition();
			LLVector3 delta_pos = newPos - getPosition();

			LLQuaternion invRotation = mDrawable->getRotation();
			invRotation.transQuat();
			
			delta_pos = delta_pos * invRotation;

			// *FIX: is this right?  Shouldn't we be calling the
			// LLViewerObject version of setPosition?
			LLVector3 old_pos = mDrawable->mXform.getParent()->getPosition();
			mDrawable->mXform.getParent()->setPosition(old_pos + delta_pos);
			setChanged(TRANSLATED | SILHOUETTE);
		}
		if (mParent && ((LLViewerObject*)mParent)->isAvatar())
		{
			// we have changed the position of an attachment, so we need to clamp it
			LLVOAvatar *avatar = (LLVOAvatar*)mParent;

			avatar->clampAttachmentPositions();
		}
	}
	else
	{
		if (isRoot())
		{
			setPositionRegion(mRegionp->getPosRegionFromGlobal(pos_global));
		}
		else
		{
			// the relative position with the parent is constant, but the parent's position needs to be changed
			LLVector3d position_offset;
			position_offset.setVec(getPosition()*getParent()->getRotation());
			LLVector3d new_pos_global = pos_global - position_offset;
			((LLViewerObject *)getParent())->setPositionGlobal(new_pos_global);
		}
	}
	updateDrawable(damped);
}


void LLViewerObject::setPositionParent(const LLVector3 &pos_parent, BOOL damped)
{
	// Set position relative to parent, if no parent, relative to region
	if (!isRoot())
	{
		LLViewerObject::setPosition(pos_parent, damped);
		//updateDrawable(damped);
	}
	else
	{
		setPositionRegion(pos_parent, damped);
	}
}

void LLViewerObject::setPositionRegion(const LLVector3 &pos_region, BOOL damped)
{
	if (!isRootEdit())
	{
		LLViewerObject* parent = (LLViewerObject*) getParent();
		LLViewerObject::setPosition((pos_region-parent->getPositionRegion())*~parent->getRotationRegion());
	}
	else
	{
		LLViewerObject::setPosition(pos_region);
		mPositionRegion = pos_region;
		mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
	}
}

void LLViewerObject::setPositionAgent(const LLVector3 &pos_agent, BOOL damped)
{
	LLVector3 pos_region = getRegion()->getPosRegionFromAgent(pos_agent);
	setPositionRegion(pos_region, damped);
}

// identical to setPositionRegion() except it checks for child-joints 
// and doesn't also move the joint-parent
// TODO -- implement similar intelligence for joint-parents toward
// their joint-children
void LLViewerObject::setPositionEdit(const LLVector3 &pos_edit, BOOL damped)
{
	if (!isRootEdit())
	{
		// the relative position with the parent is constant, but the parent's position needs to be changed
		LLVector3 position_offset = getPosition() * getParent()->getRotation();

		((LLViewerObject *)getParent())->setPositionEdit(pos_edit - position_offset);
		updateDrawable(damped);
	}
	else
	{
		LLViewerObject::setPosition(pos_edit, damped);
		mPositionRegion = pos_edit;
		mPositionAgent = mRegionp->getPosAgentFromRegion(mPositionRegion);
	}	
}


LLViewerObject* LLViewerObject::getRootEdit() const
{
	const LLViewerObject* root = this;
	while (root->mParent 
		   && !((LLViewerObject*)root->mParent)->isAvatar()) 
	{
		root = (LLViewerObject*)root->mParent;
	}
	return (LLViewerObject*)root;
}


BOOL LLViewerObject::lineSegmentIntersect(const LLVector3& start, const LLVector3& end,
										  S32 face,
										  BOOL pick_transparent,
										  S32* face_hit,
										  LLVector3* intersection,
										  LLVector2* tex_coord,
										  LLVector3* normal,
										  LLVector3* bi_normal)
{
	return false;
}

BOOL LLViewerObject::lineSegmentBoundingBox(const LLVector3& start, const LLVector3& end)
{
	if (mDrawable.isNull() || mDrawable->isDead())
	{
		return FALSE;
	}

	const LLVector4a* ext = mDrawable->getSpatialExtents();

	//VECTORIZE THIS
	LLVector4a center;
	center.setAdd(ext[1], ext[0]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(ext[1], ext[0]);
	size.mul(0.5f);

	LLVector4a starta, enda;
	starta.load3(start.mV);
	enda.load3(end.mV);

	return LLLineSegmentBoxIntersect(starta, enda, center, size);
}

U8 LLViewerObject::getMediaType() const
{
	if (mMedia)
	{
		return mMedia->mMediaType;
	}
	else
	{
		return LLViewerObject::MEDIA_NONE;
	}
}

void LLViewerObject::setMediaType(U8 media_type)
{
	if (!mMedia)
	{
		// TODO what if we don't have a media pointer?
	}
	else if (mMedia->mMediaType != media_type)
	{
		mMedia->mMediaType = media_type;

		// TODO: update materials with new image
	}
}

std::string LLViewerObject::getMediaURL() const
{
	if (mMedia)
	{
		return mMedia->mMediaURL;
	}
	else
	{
		return std::string();
	}
}

void LLViewerObject::setMediaURL(const std::string& media_url)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	if (!mMedia)
	{
		mMedia = new LLViewerObjectMedia;
		mMedia->mMediaURL = media_url;
		mMedia->mPassedWhitelist = FALSE;

		// TODO: update materials with new image
	}
	else if (mMedia->mMediaURL != media_url)
	{
		mMedia->mMediaURL = media_url;
		mMedia->mPassedWhitelist = FALSE;

		// TODO: update materials with new image
	}
}

BOOL LLViewerObject::getMediaPassedWhitelist() const
{
	if (mMedia)
	{
		return mMedia->mPassedWhitelist;
	}
	else
	{
		return FALSE;
	}
}

void LLViewerObject::setMediaPassedWhitelist(BOOL passed)
{
	if (mMedia)
	{
		mMedia->mPassedWhitelist = passed;
	}
}

BOOL LLViewerObject::setMaterial(const U8 material)
{
	BOOL res = LLPrimitive::setMaterial(material);
	if (res)
	{
		setChanged(TEXTURE);
	}
	return res;
}

void LLViewerObject::setNumTEs(const U8 num_tes)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	
	U32 i;
	if (num_tes != getNumTEs())
	{
		if (num_tes)
		{
			LLPointer<LLViewerTexture> *new_images;
			new_images = new LLPointer<LLViewerTexture>[num_tes];
			for (i = 0; i < num_tes; i++)
			{
				if (i < getNumTEs())
				{
					new_images[i] = mTEImages[i];
				}
				else if (getNumTEs())
				{
					new_images[i] = mTEImages[getNumTEs()-1];
				}
				else
				{
					new_images[i] = NULL;
				}
			}

			deleteTEImages();
			
			mTEImages = new_images;
		}
		else
		{
			deleteTEImages();
		}
		LLPrimitive::setNumTEs(num_tes);
		setChanged(TEXTURE);

		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
}

void LLViewerObject::sendMaterialUpdate() const
{
	LLViewerRegion* regionp = getRegion();
	if(!regionp) return;
	gMessageSystem->newMessageFast(_PREHASH_ObjectMaterial);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID,	mLocalID );
	gMessageSystem->addU8Fast(_PREHASH_Material, getMaterial() );
	gMessageSystem->sendReliable( regionp->getHost() );

}

//formerly send_object_shape(LLViewerObject *object)
void LLViewerObject::sendShapeUpdate()
{
	gMessageSystem->newMessageFast(_PREHASH_ObjectShape);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );

	LLVolumeMessage::packVolumeParams(&getVolume()->getParams(), gMessageSystem);

	LLViewerRegion *regionp = getRegion();
	gMessageSystem->sendReliable( regionp->getHost() );
}


void LLViewerObject::sendTEUpdate() const
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectImage);

	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_ObjectData);
	msg->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );
	if (mMedia)
	{
		msg->addString("MediaURL", mMedia->mMediaURL);
	}
	else
	{
		msg->addString("MediaURL", NULL);
	}

	// TODO send media type

	packTEMessage(msg);

	LLViewerRegion *regionp = getRegion();
	msg->sendReliable( regionp->getHost() );
}

void LLViewerObject::setTE(const U8 te, const LLTextureEntry &texture_entry)
{
	LLPrimitive::setTE(te, texture_entry);
//  This doesn't work, don't get any textures.
//	if (mDrawable.notNull() && mDrawable->isVisible())
//	{
		const LLUUID& image_id = getTE(te)->getID();
		mTEImages[te] = LLViewerTextureManager::getFetchedTexture(image_id, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
//	}
}

void LLViewerObject::setTEImage(const U8 te, LLViewerTexture *imagep)
{
	if (mTEImages[te] != imagep)
	{
		mTEImages[te] = imagep;
		LLPrimitive::setTETexture(te, imagep->getID());
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
}


S32 LLViewerObject::setTETextureCore(const U8 te, const LLUUID& uuid, const std::string &url )
{
	S32 retval = 0;
	if (uuid != getTE(te)->getID() ||
		uuid == LLUUID::null)
	{
		retval = LLPrimitive::setTETexture(te, uuid);
		mTEImages[te] = LLViewerTextureManager::getFetchedTextureFromUrl  (url, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, uuid);
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}

S32 LLViewerObject::setTETextureCore(const U8 te, const LLUUID& uuid, LLHost host)
{
	S32 retval = 0;
	if (uuid != getTE(te)->getID() ||
		uuid == LLUUID::null)
	{
		retval = LLPrimitive::setTETexture(te, uuid);
		mTEImages[te] = LLViewerTextureManager::getFetchedTexture(uuid, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, host);
		setChanged(TEXTURE);
		if (mDrawable.notNull())
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}

//virtual
void LLViewerObject::changeTEImage(S32 index, LLViewerTexture* new_image) 
{
	if(index < 0 || index >= getNumTEs())
	{
		return ;
	}
	mTEImages[index] = new_image ;
}

S32 LLViewerObject::setTETexture(const U8 te, const LLUUID& uuid)
{
	// Invalid host == get from the agent's sim
	return setTETextureCore(te, uuid, LLHost::invalid);
}


S32 LLViewerObject::setTEColor(const U8 te, const LLColor3& color)
{
	return setTEColor(te, LLColor4(color));
}

S32 LLViewerObject::setTEColor(const U8 te, const LLColor4& color)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (color != tep->getColor())
	{
		retval = LLPrimitive::setTEColor(te, color);
		if (mDrawable.notNull() && retval)
		{
			// These should only happen on updates which are not the initial update.
			dirtyMesh();
		}
	}
	return retval;
}

S32 LLViewerObject::setTEBumpmap(const U8 te, const U8 bump)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (bump != tep->getBumpmap())
	{
		retval = LLPrimitive::setTEBumpmap(te, bump);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
		}
	}
	return retval;
}

S32 LLViewerObject::setTETexGen(const U8 te, const U8 texgen)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (texgen != tep->getTexGen())
	{
		retval = LLPrimitive::setTETexGen(te, texgen);
		setChanged(TEXTURE);
	}
	return retval;
}

S32 LLViewerObject::setTEMediaTexGen(const U8 te, const U8 media)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (media != tep->getMediaTexGen())
	{
		retval = LLPrimitive::setTEMediaTexGen(te, media);
		setChanged(TEXTURE);
	}
	return retval;
}

S32 LLViewerObject::setTEShiny(const U8 te, const U8 shiny)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (shiny != tep->getShiny())
	{
		retval = LLPrimitive::setTEShiny(te, shiny);
		setChanged(TEXTURE);
	}
	return retval;
}

S32 LLViewerObject::setTEFullbright(const U8 te, const U8 fullbright)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (fullbright != tep->getFullbright())
	{
		retval = LLPrimitive::setTEFullbright(te, fullbright);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}


S32 LLViewerObject::setTEMediaFlags(const U8 te, const U8 media_flags)
{
	// this might need work for media type
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (media_flags != tep->getMediaFlags())
	{
		retval = LLPrimitive::setTEMediaFlags(te, media_flags);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD, TRUE);
			gPipeline.markTextured(mDrawable);
			// JC - probably only need this if changes texture coords
			//gPipeline.markRebuild(mDrawable);
		}
	}
	return retval;
}

S32 LLViewerObject::setTEGlow(const U8 te, const F32 glow)
{
	S32 retval = 0;
	const LLTextureEntry *tep = getTE(te);
	if (!tep)
	{
		llwarns << "No texture entry for te " << (S32)te << ", object " << mID << llendl;
	}
	else if (glow != tep->getGlow())
	{
		retval = LLPrimitive::setTEGlow(te, glow);
		setChanged(TEXTURE);
		if (mDrawable.notNull() && retval)
		{
			gPipeline.markTextured(mDrawable);
		}
	}
	return retval;
}


S32 LLViewerObject::setTEScale(const U8 te, const F32 s, const F32 t)
{
	S32 retval = 0;
	retval = LLPrimitive::setTEScale(te, s, t);
	setChanged(TEXTURE);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}

S32 LLViewerObject::setTEScaleS(const U8 te, const F32 s)
{
	S32 retval = LLPrimitive::setTEScaleS(te, s);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}

	return retval;
}

S32 LLViewerObject::setTEScaleT(const U8 te, const F32 t)
{
	S32 retval = LLPrimitive::setTEScaleT(te, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}

	return retval;
}

S32 LLViewerObject::setTEOffset(const U8 te, const F32 s, const F32 t)
{
	S32 retval = LLPrimitive::setTEOffset(te, s, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}

S32 LLViewerObject::setTEOffsetS(const U8 te, const F32 s)
{
	S32 retval = LLPrimitive::setTEOffsetS(te, s);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}

	return retval;
}

S32 LLViewerObject::setTEOffsetT(const U8 te, const F32 t)
{
	S32 retval = LLPrimitive::setTEOffsetT(te, t);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}

	return retval;
}

S32 LLViewerObject::setTERotation(const U8 te, const F32 r)
{
	S32 retval = LLPrimitive::setTERotation(te, r);
	if (mDrawable.notNull() && retval)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_TCOORD);
	}
	return retval;
}


LLViewerTexture *LLViewerObject::getTEImage(const U8 face) const
{
//	llassert(mTEImages);

	if (face < getNumTEs())
	{
		LLViewerTexture* image = mTEImages[face];
		if (image)
		{
			return image;
		}
		else
		{
			return (LLViewerTexture*)(LLViewerFetchedTexture::sDefaultImagep);
		}
	}

	llerrs << llformat("Requested Image from invalid face: %d/%d",face,getNumTEs()) << llendl;

	return NULL;
}


void LLViewerObject::fitFaceTexture(const U8 face)
{
	llinfos << "fitFaceTexture not implemented" << llendl;
}


LLBBox LLViewerObject::getBoundingBoxAgent() const
{
	LLVector3 position_agent;
	LLQuaternion rot;
	LLViewerObject* avatar_parent = NULL;
	LLViewerObject* root_edit = (LLViewerObject*)getRootEdit();
	if (root_edit)
	{
		avatar_parent = (LLViewerObject*)root_edit->getParent();
	}
	
	if (avatar_parent && avatar_parent->isAvatar() &&
		root_edit && root_edit->mDrawable.notNull() && root_edit->mDrawable->getXform()->getParent())
	{
		LLXform* parent_xform = root_edit->mDrawable->getXform()->getParent();
		position_agent = (getPositionEdit() * parent_xform->getWorldRotation()) + parent_xform->getWorldPosition();
		rot = getRotationEdit() * parent_xform->getWorldRotation();
	}
	else
	{
		position_agent = getPositionAgent();
		rot = getRotationRegion();
	}
	
	return LLBBox( position_agent, rot, getScale() * -0.5f, getScale() * 0.5f );
}

U32 LLViewerObject::getNumVertices() const
{
	U32 num_vertices = 0;
	if (mDrawable.notNull())
	{
		S32 i, num_faces;
		num_faces = mDrawable->getNumFaces();
		for (i = 0; i < num_faces; i++)
		{
			LLFace * facep = mDrawable->getFace(i);
			if (facep)
			{
				num_vertices += facep->getGeomCount();
			}
		}
	}
	return num_vertices;
}

U32 LLViewerObject::getNumIndices() const
{
	U32 num_indices = 0;
	if (mDrawable.notNull())
	{
		S32 i, num_faces;
		num_faces = mDrawable->getNumFaces();
		for (i = 0; i < num_faces; i++)
		{
			LLFace * facep = mDrawable->getFace(i);
			if (facep)
			{
				num_indices += facep->getIndicesCount();
			}
		}
	}
	return num_indices;
}

// Find the number of instances of this object's inventory that are of the given type
S32 LLViewerObject::countInventoryContents(LLAssetType::EType type)
{
	S32 count = 0;
	if( mInventory )
	{
		LLInventoryObject::object_list_t::const_iterator it = mInventory->begin();
		LLInventoryObject::object_list_t::const_iterator end = mInventory->end();
		for(  ; it != end ; ++it )
		{
			if( (*it)->getType() == type )
			{
				++count;
			}
		}
	}
	return count;
}


void LLViewerObject::setCanSelect(BOOL canSelect)
{
	mbCanSelect = canSelect;
	for (child_list_t::iterator iter = mChildList.begin();
		 iter != mChildList.end(); iter++)
	{
		LLViewerObject* child = *iter;
		child->mbCanSelect = canSelect;
	}
}

void LLViewerObject::setDebugText(const std::string &utf8text)
{
	if (utf8text.empty() && !mText)
	{
		return;
	}

	if (!mText)
	{
		mText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
		mText->setFont(LLFontGL::getFontSansSerif());
		mText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
		mText->setMaxLines(-1);
		mText->setSourceObject(this);
		mText->setOnHUDAttachment(isHUDAttachment());
	}
	mText->setColor(LLColor4::white);
	mText->setString(utf8text);
	mText->setZCompare(FALSE);
	mText->setDoFade(FALSE);
	updateText();
}

void LLViewerObject::setIcon(LLViewerTexture* icon_image)
{
	if (!mIcon)
	{
		mIcon = (LLHUDIcon *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_ICON);
		mIcon->setSourceObject(this);
		mIcon->setImage(icon_image);
		// *TODO: make this user configurable
		mIcon->setScale(0.03f);
	}
	else
	{
		mIcon->restartLifeTimer();
	}
}

void LLViewerObject::clearIcon()
{
	if (mIcon)
	{
		mIcon = NULL;
	}
}

LLViewerObject* LLViewerObject::getSubParent() 
{ 
	return (LLViewerObject*) getParent();
}

const LLViewerObject* LLViewerObject::getSubParent() const
{
	return (const LLViewerObject*) getParent();
}

BOOL LLViewerObject::isOnMap()
{
	return mOnMap;
}


void LLViewerObject::updateText()
{
	if (!isDead())
	{
		if (mText.notNull())
		{		
			LLVector3 up_offset(0,0,0);
			up_offset.mV[2] = getScale().mV[VZ]*0.6f;
			
			if (mDrawable.notNull())
			{
				mText->setPositionAgent(getRenderPosition() + up_offset);
			}
			else
			{
				mText->setPositionAgent(getPositionAgent() + up_offset);
			}
		}
	}
}

LLVOAvatar* LLViewerObject::asAvatar()
{
	return NULL;
}

BOOL LLViewerObject::isParticleSource() const
{
	return !mPartSourcep.isNull() && !mPartSourcep->isDead();
}

void LLViewerObject::setParticleSource(const LLPartSysData& particle_parameters, const LLUUID& owner_id)
{
	if (mPartSourcep)
	{
		deleteParticleSource();
	}

	LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::createPSS(this, particle_parameters);
	mPartSourcep = pss;
	
	if (mPartSourcep)
	{
		mPartSourcep->setOwnerUUID(owner_id);

		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.tga");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
	LLViewerPartSim::getInstance()->addPartSource(pss);
}

void LLViewerObject::unpackParticleSource(const S32 block_num, const LLUUID& owner_id)
{
	if (!mPartSourcep.isNull() && mPartSourcep->isDead())
	{
		mPartSourcep = NULL;
	}
	if (mPartSourcep)
	{
		// If we've got one already, just update the existing source (or remove it)
		if (!LLViewerPartSourceScript::unpackPSS(this, mPartSourcep, block_num))
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}
	}
	else
	{
		LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::unpackPSS(this, NULL, block_num);
		//If the owner is muted, don't create the system
		if(LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagParticles)) return;

		// We need to be able to deal with a particle source that hasn't changed, but still got an update!
		if (pss)
		{
// 			llinfos << "Making particle system with owner " << owner_id << llendl;
			pss->setOwnerUUID(owner_id);
			mPartSourcep = pss;
			LLViewerPartSim::getInstance()->addPartSource(pss);
		}
	}
	if (mPartSourcep)
	{
		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
}

void LLViewerObject::unpackParticleSource(LLDataPacker &dp, const LLUUID& owner_id)
{
	if (!mPartSourcep.isNull() && mPartSourcep->isDead())
	{
		mPartSourcep = NULL;
	}
	if (mPartSourcep)
	{
		// If we've got one already, just update the existing source (or remove it)
		if (!LLViewerPartSourceScript::unpackPSS(this, mPartSourcep, dp))
		{
			mPartSourcep->setDead();
			mPartSourcep = NULL;
		}
	}
	else
	{
		LLPointer<LLViewerPartSourceScript> pss = LLViewerPartSourceScript::unpackPSS(this, NULL, dp);
		//If the owner is muted, don't create the system
		if(LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagParticles)) return;
		// We need to be able to deal with a particle source that hasn't changed, but still got an update!
		if (pss)
		{
// 			llinfos << "Making particle system with owner " << owner_id << llendl;
			pss->setOwnerUUID(owner_id);
			mPartSourcep = pss;
			LLViewerPartSim::getInstance()->addPartSource(pss);
		}
	}
	if (mPartSourcep)
	{
		if (mPartSourcep->getImage()->getID() != mPartSourcep->mPartSysData.mPartImageID)
		{
			LLViewerTexture* image;
			if (mPartSourcep->mPartSysData.mPartImageID == LLUUID::null)
			{
				image = LLViewerTextureManager::getFetchedTextureFromFile("pixiesmall.j2c");
			}
			else
			{
				image = LLViewerTextureManager::getFetchedTexture(mPartSourcep->mPartSysData.mPartImageID);
			}
			mPartSourcep->setImage(image);
		}
	}
}

void LLViewerObject::deleteParticleSource()
{
	if (mPartSourcep.notNull())
	{
		mPartSourcep->setDead();
		mPartSourcep = NULL;
	}
}

// virtual
void LLViewerObject::updateDrawable(BOOL force_damped)
{
	if (!isChanged(MOVED))
	{ //most common case, having an empty if case here makes for better branch prediction
	}
	else if (mDrawable.notNull() && 
		!mDrawable->isState(LLDrawable::ON_MOVE_LIST))
	{
		BOOL damped_motion = 
			!isChanged(SHIFTED) &&										// not shifted between regions this frame and...
			(	force_damped ||										// ...forced into damped motion by application logic or...
				(	!isSelected() &&									// ...not selected and...
					(	mDrawable->isRoot() ||								// ... is root or ...
						(getParent() && !((LLViewerObject*)getParent())->isSelected())// ... parent is not selected and ...
					) &&	
					getPCode() == LL_PCODE_VOLUME &&					// ...is a volume object and...
					getVelocity().isExactlyZero() &&					// ...is not moving physically and...
					mDrawable->getGeneration() != -1                    // ...was not created this frame.
				)					
			);
		gPipeline.markMoved(mDrawable, damped_motion);
	}
	clearChanged(SHIFTED);
}

// virtual, overridden by LLVOVolume
F32 LLViewerObject::getVObjRadius() const
{
	return mDrawable.notNull() ? mDrawable->getRadius() : 0.f;
}

void LLViewerObject::setAttachedSound(const LLUUID &audio_uuid, const LLUUID& owner_id, const F32 gain, const U8 flags)
{
	if (!gAudiop)
	{
		return;
	}
	
	if (audio_uuid.isNull())
	{
		if (!mAudioSourcep)
		{
			return;
		}
		if (mAudioSourcep->isLoop() && !mAudioSourcep->hasPendingPreloads())
		{
			// We don't clear the sound if it's a loop, it'll go away on its own.
			// At least, this appears to be how the scripts work.
			// The attached sound ID is set to NULL to avoid it playing back when the
			// object rezzes in on non-looping sounds.
			//llinfos << "Clearing attached sound " << mAudioSourcep->getCurrentData()->getID() << llendl;
			gAudiop->cleanupAudioSource(mAudioSourcep);
			mAudioSourcep = NULL;
		}
		else if (flags & LL_SOUND_FLAG_STOP)
        {
			// Just shut off the sound
			mAudioSourcep->play(LLUUID::null);
		}
		return;
	}
	if (flags & LL_SOUND_FLAG_LOOP
		&& mAudioSourcep && mAudioSourcep->isLoop() && mAudioSourcep->getCurrentData()
		&& mAudioSourcep->getCurrentData()->getID() == audio_uuid)
	{
		//llinfos << "Already playing this sound on a loop, ignoring" << llendl;
		return;
	}

	// don't clean up before previous sound is done. Solves: SL-33486
	if ( mAudioSourcep && mAudioSourcep->isDone() ) 
	{
		gAudiop->cleanupAudioSource(mAudioSourcep);
		mAudioSourcep = NULL;
	}

	if (mAudioSourcep && mAudioSourcep->isMuted() &&
	    mAudioSourcep->getCurrentData() && mAudioSourcep->getCurrentData()->getID() == audio_uuid)
	{
		//llinfos << "Already having this sound as muted sound, ignoring" << llendl;
		return;
	}

	getAudioSource(owner_id);

	if (mAudioSourcep)
	{
		BOOL queue = flags & LL_SOUND_FLAG_QUEUE;
		mAudioGain = gain;
		mAudioSourcep->setGain(gain);
		mAudioSourcep->setLoop(flags & LL_SOUND_FLAG_LOOP);
		mAudioSourcep->setSyncMaster(flags & LL_SOUND_FLAG_SYNC_MASTER);
		mAudioSourcep->setSyncSlave(flags & LL_SOUND_FLAG_SYNC_SLAVE);
		mAudioSourcep->setQueueSounds(queue);
		if(!queue) // stop any current sound first to avoid "farts of doom" (SL-1541) -MG
		{
			mAudioSourcep->play(LLUUID::null);
		}
		
		// Play this sound if region maturity permits
		if( gAgent.canAccessMaturityAtGlobal(this->getPositionGlobal()) )
		{
			//llinfos << "Playing attached sound " << audio_uuid << llendl;
			mAudioSourcep->play(audio_uuid);
		}
	}
}

LLAudioSource *LLViewerObject::getAudioSource(const LLUUID& owner_id)
{
	if (!mAudioSourcep)
	{
		// Arbitrary low gain for a sound that's not playing.
		// This is used for sound preloads, for example.
		LLAudioSourceVO *asvop = new LLAudioSourceVO(mID, owner_id, 0.01f, this);

		mAudioSourcep = asvop;
		if(gAudiop) gAudiop->addAudioSource(asvop);
	}

	return mAudioSourcep;
}

void LLViewerObject::adjustAudioGain(const F32 gain)
{
	if (!gAudiop)
	{
		return;
	}
	if (mAudioSourcep)
	{
		mAudioGain = gain;
		mAudioSourcep->setGain(mAudioGain);
	}
}

//----------------------------------------------------------------------------

bool LLViewerObject::unpackParameterEntry(U16 param_type, LLDataPacker *dp)
{
	if (LLNetworkData::PARAMS_MESH == param_type)
	{
		param_type = LLNetworkData::PARAMS_SCULPT;
	}
	ExtraParameter* param = getExtraParameterEntryCreate(param_type);
	if (param)
	{
		param->data->unpack(*dp);
		param->in_use = TRUE;
		parameterChanged(param_type, param->data, TRUE, false);
		return true;
	}
	else
	{
		return false;
	}
}

LLViewerObject::ExtraParameter* LLViewerObject::createNewParameterEntry(U16 param_type)
{
	LLNetworkData* new_block = NULL;
	switch (param_type)
	{
	  case LLNetworkData::PARAMS_FLEXIBLE:
	  {
		  new_block = new LLFlexibleObjectData();
		  break;
	  }
	  case LLNetworkData::PARAMS_LIGHT:
	  {
		  new_block = new LLLightParams();
		  break;
	  }
	  case LLNetworkData::PARAMS_SCULPT:
	  {
		  new_block = new LLSculptParams();
		  break;
	  }
	  case LLNetworkData::PARAMS_LIGHT_IMAGE:
	  {
		  new_block = new LLLightImageParams();
		  break;
	  }
	  default:
	  {
		  llinfos << "Unknown param type." << llendl;
		  break;
	  }
	};

	if (new_block)
	{
		ExtraParameter* new_entry = new ExtraParameter;
		new_entry->data = new_block;
		new_entry->in_use = false; // not in use yet
		mExtraParameterList[param_type] = new_entry;
		return new_entry;
	}
	return NULL;
}

LLViewerObject::ExtraParameter* LLViewerObject::getExtraParameterEntry(U16 param_type) const
{
	std::map<U16, ExtraParameter*>::const_iterator itor = mExtraParameterList.find(param_type);
	if (itor != mExtraParameterList.end())
	{
		return itor->second;
	}
	return NULL;
}

LLViewerObject::ExtraParameter* LLViewerObject::getExtraParameterEntryCreate(U16 param_type)
{
	ExtraParameter* param = getExtraParameterEntry(param_type);
	if (!param)
	{
		param = createNewParameterEntry(param_type);
	}
	return param;
}

LLNetworkData* LLViewerObject::getParameterEntry(U16 param_type) const
{
	ExtraParameter* param = getExtraParameterEntry(param_type);
	if (param)
	{
		return param->data;
	}
	else
	{
		return NULL;
	}
}

BOOL LLViewerObject::getParameterEntryInUse(U16 param_type) const
{
	ExtraParameter* param = getExtraParameterEntry(param_type);
	if (param)
	{
		return param->in_use;
	}
	else
	{
		return FALSE;
	}
}

bool LLViewerObject::setParameterEntry(U16 param_type, const LLNetworkData& new_value, bool local_origin)
{
	ExtraParameter* param = getExtraParameterEntryCreate(param_type);
	if (param)
	{
		if (param->in_use && new_value == *(param->data))
		{
			return false;
		}
		param->in_use = true;
		param->data->copy(new_value);
		parameterChanged(param_type, param->data, TRUE, local_origin);
		return true;
	}
	else
	{
		return false;
	}
}

// Assumed to be called locally
// If in_use is TRUE, will crate a new extra parameter if none exists.
// Should always return true.
bool LLViewerObject::setParameterEntryInUse(U16 param_type, BOOL in_use, bool local_origin)
{
	ExtraParameter* param = getExtraParameterEntryCreate(param_type);
	if (param && param->in_use != in_use)
	{
		param->in_use = in_use;
		parameterChanged(param_type, param->data, in_use, local_origin);
		return true;
	}
	return false;
}

void LLViewerObject::parameterChanged(U16 param_type, bool local_origin)
{
	ExtraParameter* param = getExtraParameterEntry(param_type);
	if (param)
	{
		parameterChanged(param_type, param->data, param->in_use, local_origin);
	}
}

void LLViewerObject::parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin)
{
	if (local_origin)
	{
		LLViewerRegion* regionp = getRegion();
		if(!regionp) return;

		// Change happened on the viewer. Send the change up
		U8 tmp[MAX_OBJECT_PARAMS_SIZE];
		LLDataPackerBinaryBuffer dpb(tmp, MAX_OBJECT_PARAMS_SIZE);
		if (data->pack(dpb))
		{
			U32 datasize = (U32)dpb.getCurrentSize();

			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_ObjectExtraParams);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addU32Fast(_PREHASH_ObjectLocalID, mLocalID );

			msg->addU16Fast(_PREHASH_ParamType, param_type);
			msg->addBOOLFast(_PREHASH_ParamInUse, in_use);

			msg->addU32Fast(_PREHASH_ParamSize, datasize);
			msg->addBinaryDataFast(_PREHASH_ParamData, tmp, datasize);

			msg->sendReliable( regionp->getHost() );
		}
		else
		{
			llwarns << "Failed to send object extra parameters: " << param_type << llendl;
		}
	}
}

void LLViewerObject::setDrawableState(U32 state, BOOL recursive)
{
	if (mDrawable)
	{
		mDrawable->setState(state);
	}
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->setDrawableState(state, recursive);
		}
	}
}

void LLViewerObject::clearDrawableState(U32 state, BOOL recursive)
{
	if (mDrawable)
	{
		mDrawable->clearState(state);
	}
	if (recursive)
	{
		for (child_list_t::iterator iter = mChildList.begin();
			 iter != mChildList.end(); iter++)
		{
			LLViewerObject* child = *iter;
			child->clearDrawableState(state, recursive);
		}
	}
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// RN: these functions assume a 2-level hierarchy 
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// Owned by anyone?
BOOL LLViewerObject::permAnyOwner() const
{ 
	if (isRootEdit())
	{
		return flagObjectAnyOwner(); 
	}
	else
	{
		return ((LLViewerObject*)getParent())->permAnyOwner();
	}
}	
// Owned by this viewer?
BOOL LLViewerObject::permYouOwner() const
{ 
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectYouOwner(); 
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permYouOwner();
	}
}

// Owned by a group?
BOOL LLViewerObject::permGroupOwner() const		
{ 
	if (isRootEdit())
	{
		return flagObjectGroupOwned(); 
	}
	else
	{
		return ((LLViewerObject*)getParent())->permGroupOwner();
	}
}

// Can the owner edit
BOOL LLViewerObject::permOwnerModify() const
{ 
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
	{
			return TRUE;
	}
# endif
		return flagObjectOwnerModify(); 
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permOwnerModify();
	}
}

// Can edit
BOOL LLViewerObject::permModify() const
{ 
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
	{
			return TRUE;
	}
# endif
		return flagObjectModify(); 
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permModify();
	}
}

// Can copy
BOOL LLViewerObject::permCopy() const
{ 
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectCopy();
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permCopy();
	}
}

// Can move
BOOL LLViewerObject::permMove() const
{
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectMove(); 
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permMove();
	}
}

// Can be transferred
BOOL LLViewerObject::permTransfer() const
{ 
	if (isRootEdit())
	{
#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!LLGridManager::getInstance()->isInProductionGrid()
            && (gAgent.getGodLevel() >= GOD_MAINTENANCE))
		{
			return TRUE;
		}
# endif
		return flagObjectTransfer(); 
#endif
	}
	else
	{
		return ((LLViewerObject*)getParent())->permTransfer();
	}
}

// Can only open objects that you own, or that someone has
// given you modify rights to.  JC
BOOL LLViewerObject::allowOpen() const
{
	return !flagInventoryEmpty() && (permYouOwner() || permModify());
}

LLViewerObject::LLInventoryCallbackInfo::~LLInventoryCallbackInfo()
{
	if (mListener)
	{
		mListener->clearVOInventoryListener();
	}
}

void LLViewerObject::updateVolume(const LLVolumeParams& volume_params)
{
	if (setVolume(volume_params, 1)) // *FIX: magic number, ack!
	{
		// Transmit the update to the simulator
		sendShapeUpdate();
		markForUpdate(TRUE);
	}
}

void LLViewerObject::markForUpdate(BOOL priority)
{
	if (mDrawable.notNull())
	{
		gPipeline.markTextured(mDrawable);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, priority);
	}
}

bool LLViewerObject::isPermanentEnforced() const
{
	return flagObjectPermanent() && (mRegionp != gAgent.getRegion()) && !gAgent.isGodlike();
}

bool LLViewerObject::getIncludeInSearch() const
{
	return flagIncludeInSearch();
}

void LLViewerObject::setIncludeInSearch(bool include_in_search)
{
	setFlags(FLAGS_INCLUDE_IN_SEARCH, include_in_search);
}

void LLViewerObject::setRegion(LLViewerRegion *regionp)
{
	if (!regionp)
	{
		llwarns << "viewer object set region to NULL" << llendl;
	}
	
	mLatestRecvPacketID = 0;
	mRegionp = regionp;

	for (child_list_t::iterator i = mChildList.begin(); i != mChildList.end(); ++i)
	{
		LLViewerObject* child = *i;
		child->setRegion(regionp);
	}

	setChanged(MOVED | SILHOUETTE);
	updateDrawable(FALSE);
}

// virtual
void	LLViewerObject::updateRegion(LLViewerRegion *regionp)
{
//	if (regionp)
//	{
//		F64 now = LLFrameTimer::getElapsedSeconds();
//		llinfos << "Updating to region " << regionp->getName()
//			<< ", ms since last update message: " << (F32)((now - mLastMessageUpdateSecs) * 1000.0)
//			<< ", ms since last interpolation: " << (F32)((now - mLastInterpUpdateSecs) * 1000.0) 
//			<< llendl;
//	}
}


bool LLViewerObject::specialHoverCursor() const
{
	return flagUsePhysics()
			|| flagHandleTouch()
			|| (mClickAction != 0);
}

void LLViewerObject::updateFlags(BOOL physics_changed)
{
	LLViewerRegion* regionp = getRegion();
	if(!regionp) return;
	gMessageSystem->newMessage("ObjectFlagUpdate");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, getLocalID() );
	gMessageSystem->addBOOLFast(_PREHASH_UsePhysics, flagUsePhysics() );
	gMessageSystem->addBOOL("IsTemporary", flagTemporaryOnRez() );
	gMessageSystem->addBOOL("IsPhantom", flagPhantom() );

	// stinson 02/28/2012 : This CastsShadows BOOL is no longer used in either the viewer or the simulator
	// The simulator code does not even unpack this value when the message is received.
	// This could be potentially hijacked in the future for another use should the urgent need arise.
	gMessageSystem->addBOOL("CastsShadows", FALSE );

	if (physics_changed)
	{
		gMessageSystem->nextBlock("ExtraPhysics");
		gMessageSystem->addU8("PhysicsShapeType", getPhysicsShapeType() );
		gMessageSystem->addF32("Density", getPhysicsDensity() );
		gMessageSystem->addF32("Friction", getPhysicsFriction() );
		gMessageSystem->addF32("Restitution", getPhysicsRestitution() );
		gMessageSystem->addF32("GravityMultiplier", getPhysicsGravity() );
	}
	gMessageSystem->sendReliable( regionp->getHost() );
}

BOOL LLViewerObject::setFlags(U32 flags, BOOL state)
{
	BOOL setit = setFlagsWithoutUpdate(flags, state);

	// BUG: Sometimes viewer physics and simulator physics get
	// out of sync.  To fix this, always send update to simulator.
// 	if (setit)
	{
		updateFlags();
	}
	return setit;
}

BOOL LLViewerObject::setFlagsWithoutUpdate(U32 flags, BOOL state)
{
	BOOL setit = FALSE;
	if (state)
	{
		if ((mFlags & flags) != flags)
		{
			mFlags |= flags;
			setit = TRUE;
		}
	}
	else
	{
		if ((mFlags & flags) != 0)
		{
			mFlags &= ~flags;
			setit = TRUE;
		}
	}
	return setit;
}

void LLViewerObject::setPhysicsShapeType(U8 type)
{
	mPhysicsShapeUnknown = false;
	if (type != mPhysicsShapeType)
	{
	mPhysicsShapeType = type;
	mCostStale = true;
}
}

void LLViewerObject::setPhysicsGravity(F32 gravity)
{
	mPhysicsGravity = gravity;
}

void LLViewerObject::setPhysicsFriction(F32 friction)
{
	mPhysicsFriction = friction;
}

void LLViewerObject::setPhysicsDensity(F32 density)
{
	mPhysicsDensity = density;
}

void LLViewerObject::setPhysicsRestitution(F32 restitution)
{
	mPhysicsRestitution = restitution;
}

U8 LLViewerObject::getPhysicsShapeType() const
{ 
	if (mPhysicsShapeUnknown)
	{
		gObjectList.updatePhysicsFlags(this);
	}

	return mPhysicsShapeType; 
}

void LLViewerObject::applyAngularVelocity(F32 dt)
{
	//do target omega here
	mRotTime += dt;
	LLVector3 ang_vel = getAngularVelocity();
	F32 omega = ang_vel.magVecSquared();
	F32 angle = 0.0f;
	LLQuaternion dQ;
	if (omega > 0.00001f)
	{
		omega = sqrt(omega);
		angle = omega * dt;

		ang_vel *= 1.f/omega;
		
		// calculate the delta increment based on the object's angular velocity
		dQ.setQuat(angle, ang_vel);

		// accumulate the angular velocity rotations to re-apply in the case of an object update
		mAngularVelocityRot *= dQ;
		
		// Just apply the delta increment to the current rotation
		setRotation(getRotation()*dQ);
		setChanged(MOVED | SILHOUETTE);
	}
}

void LLViewerObject::resetRotTime()
{
	mRotTime = 0.0f;
}

void LLViewerObject::resetRot()
{
	resetRotTime();

	// Reset the accumulated angular velocity rotation
	mAngularVelocityRot.loadIdentity(); 
}

U32 LLViewerObject::getPartitionType() const
{ 
	return LLViewerRegion::PARTITION_NONE; 
}

void LLViewerObject::dirtySpatialGroup(BOOL priority) const
{
	if (mDrawable)
	{
		LLSpatialGroup* group = mDrawable->getSpatialGroup();
		if (group)
		{
			group->dirtyGeom();
			gPipeline.markRebuild(group, priority);
		}
	}
}

void LLViewerObject::dirtyMesh()
{
	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL);
		/*LLSpatialGroup* group = mDrawable->getSpatialGroup();
		if (group)
		{
			group->dirtyMesh();
		}*/
	}
}

F32 LLAlphaObject::getPartSize(S32 idx)
{
	return 0.f;
}

// virtual
void LLStaticViewerObject::updateDrawable(BOOL force_damped)
{
	// Force an immediate rebuild on any update
	if (mDrawable.notNull())
	{
		mDrawable->updateXform(TRUE);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	clearChanged(SHIFTED);
}

void LLViewerObject::saveUnselectedChildrenPosition(std::vector<LLVector3>& positions)
{
	if(mChildList.empty() || !positions.empty())
	{
		return ;
	}

	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			positions.push_back(childp->getPositionEdit());		
		}
	}

	return ;
}

void LLViewerObject::saveUnselectedChildrenRotation(std::vector<LLQuaternion>& rotations)
{
	if(mChildList.empty())
	{
		return ;
	}

	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			rotations.push_back(childp->getRotationEdit());				
		}		
	}

	return ;
}

//counter-rotation
void LLViewerObject::resetChildrenRotationAndPosition(const std::vector<LLQuaternion>& rotations, 
											const std::vector<LLVector3>& positions)
{
	if(mChildList.empty())
	{
		return ;
	}

	S32 index = 0 ;
	LLQuaternion inv_rotation = ~getRotationEdit() ;
	LLVector3 offset = getPositionEdit() ;
	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				childp->setRotation(rotations[index] * inv_rotation);
				childp->setPosition((positions[index] - offset) * inv_rotation);
				LLManip::rebuild(childp);					
			}
			else //avatar
			{
				LLVector3 reset_pos = (positions[index] - offset) * inv_rotation ;
				LLQuaternion reset_rot = rotations[index] * inv_rotation ;

				((LLVOAvatar*)childp)->mDrawable->mXform.setPosition(reset_pos);				
				((LLVOAvatar*)childp)->mDrawable->mXform.setRotation(reset_rot) ;
				
				((LLVOAvatar*)childp)->mDrawable->getVObj()->setPosition(reset_pos, TRUE);				
				((LLVOAvatar*)childp)->mDrawable->getVObj()->setRotation(reset_rot, TRUE) ;

				LLManip::rebuild(childp);				
			}	
			index++;
		}				
	}

	return ;
}

//counter-translation
void LLViewerObject::resetChildrenPosition(const LLVector3& offset, BOOL simplified)
{
	if(mChildList.empty())
	{
		return ;
	}

	LLVector3 child_offset;
	if(simplified) //translation only, rotation matrix does not change
	{
		child_offset = offset * ~getRotation();
	}
	else //rotation matrix might change too.
	{
		if (isAttachment() && mDrawable.notNull())
		{
			LLXform* attachment_point_xform = mDrawable->getXform()->getParent();
			LLQuaternion parent_rotation = getRotation() * attachment_point_xform->getWorldRotation();
			child_offset = offset * ~parent_rotation;
		}
		else
		{
			child_offset = offset * ~getRenderRotation();
		}
	}

	for (LLViewerObject::child_list_t::const_iterator iter = mChildList.begin();
			iter != mChildList.end(); iter++)
	{
		LLViewerObject* childp = *iter;
		if (!childp->isSelected() && childp->mDrawable.notNull())
		{
			if (childp->getPCode() != LL_PCODE_LEGACY_AVATAR)
			{
				childp->setPosition(childp->getPosition() + child_offset);
				LLManip::rebuild(childp);
			}
			else //avatar
			{
				LLVector3 reset_pos = ((LLVOAvatar*)childp)->mDrawable->mXform.getPosition() + child_offset ;

				((LLVOAvatar*)childp)->mDrawable->mXform.setPosition(reset_pos);
				((LLVOAvatar*)childp)->mDrawable->getVObj()->setPosition(reset_pos);				
				
				LLManip::rebuild(childp);
			}			
		}		
	}

	return ;
}

const LLUUID &LLViewerObject::getAttachmentItemID() const
{
	return mAttachmentItemID;
}

void LLViewerObject::setAttachmentItemID(const LLUUID &id)
{
	mAttachmentItemID = id;
}

EObjectUpdateType LLViewerObject::getLastUpdateType() const
{
	return mLastUpdateType;
}

void LLViewerObject::setLastUpdateType(EObjectUpdateType last_update_type)
{
	mLastUpdateType = last_update_type;
}

BOOL LLViewerObject::getLastUpdateCached() const
{
	return mLastUpdateCached;
}

void LLViewerObject::setLastUpdateCached(BOOL last_update_cached)
{
	mLastUpdateCached = last_update_cached;
}

const LLUUID &LLViewerObject::extractAttachmentItemID()
{
	LLUUID item_id = LLUUID::null;
	LLNameValue* item_id_nv = getNVPair("AttachItemID");
	if( item_id_nv )
	{
		const char* s = item_id_nv->getString();
		if( s )
		{
			item_id.set(s);
		}
	}
	setAttachmentItemID(item_id);
	return getAttachmentItemID();
}

//virtual
LLVOAvatar* LLViewerObject::getAvatar() const
{
	if (isAttachment())
	{
		LLViewerObject* vobj = (LLViewerObject*) getParent();

		while (vobj && !vobj->asAvatar())
		{
			vobj = (LLViewerObject*) vobj->getParent();
		}

		return (LLVOAvatar*) vobj;
	}

	return NULL;
}


class ObjectPhysicsProperties : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD object_data = input["body"]["ObjectData"];
		S32 num_entries = object_data.size();
		
		for ( S32 i = 0; i < num_entries; i++ )
		{
			LLSD& curr_object_data = object_data[i];
			U32 local_id = curr_object_data["LocalID"].asInteger();

			// Iterate through nodes at end, since it can be on both the regular AND hover list
			struct f : public LLSelectedNodeFunctor
			{
				U32 mID;
				f(const U32& id) : mID(id) {}
				virtual bool apply(LLSelectNode* node)
				{
					return (node->getObject() && node->getObject()->mLocalID == mID );
				}
			} func(local_id);

			LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(&func);

			if (node)
			{
				// The LLSD message builder doesn't know how to handle U8, so we need to send as S8 and cast
				U8 type = (U8)curr_object_data["PhysicsShapeType"].asInteger();
				F32 density = (F32)curr_object_data["Density"].asReal();
				F32 friction = (F32)curr_object_data["Friction"].asReal();
				F32 restitution = (F32)curr_object_data["Restitution"].asReal();
				F32 gravity = (F32)curr_object_data["GravityMultiplier"].asReal();

				node->getObject()->setPhysicsShapeType(type);
				node->getObject()->setPhysicsGravity(gravity);
				node->getObject()->setPhysicsFriction(friction);
				node->getObject()->setPhysicsDensity(density);
				node->getObject()->setPhysicsRestitution(restitution);
			}	
		}
		
		dialog_refresh_all();
	};
};

LLHTTPRegistration<ObjectPhysicsProperties>
	gHTTPRegistrationObjectPhysicsProperties("/message/ObjectPhysicsProperties");

