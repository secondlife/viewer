/** 
 * @file llcharacter.cpp
 * @brief Implementation of LLCharacter class.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------

#include "linden_common.h"

#include "llcharacter.h"
#include "llstring.h"

#define SKEL_HEADER "Linden Skeleton 1.0"

LLStringTable LLCharacter::sVisualParamNames(1024);

std::vector< LLCharacter* > LLCharacter::sInstances;
BOOL LLCharacter::sAllowInstancesChange = TRUE ;

//-----------------------------------------------------------------------------
// LLCharacter()
// Class Constructor
//-----------------------------------------------------------------------------
LLCharacter::LLCharacter()
	: 
	mPreferredPelvisHeight( 0.f ),
	mSex( SEX_FEMALE ),
	mAppearanceSerialNum( 0 ),
	mSkeletonSerialNum( 0 )
{
	llassert_always(sAllowInstancesChange) ;
	sInstances.push_back(this);

	mMotionController.setCharacter( this );	
	mPauseRequest = new LLPauseRequestHandle();
}


//-----------------------------------------------------------------------------
// ~LLCharacter()
// Class Destructor
//-----------------------------------------------------------------------------
LLCharacter::~LLCharacter()
{	
	for (LLVisualParam *param = getFirstVisualParam(); 
		param;
		param = getNextVisualParam())
	{
		delete param;
	}

	U32 i ;
	U32 size = sInstances.size() ;
	for(i = 0 ; i < size ; i++)
	{
		if(sInstances[i] == this)
		{
			break ;
		}
	}

	llassert_always(i < size) ;

	llassert_always(sAllowInstancesChange) ;
	sInstances[i] = sInstances[size - 1] ;
	sInstances.pop_back() ;
}


//-----------------------------------------------------------------------------
// getJoint()
//-----------------------------------------------------------------------------
LLJoint *LLCharacter::getJoint( const std::string &name )
{
	LLJoint* joint = NULL;

	LLJoint *root = getRootJoint();
	if (root)
	{
		joint = root->findJoint(name);
	}

	if (!joint)
	{
		llwarns << "Failed to find joint." << llendl;
	}
	return joint;
}

//-----------------------------------------------------------------------------
// registerMotion()
//-----------------------------------------------------------------------------
BOOL LLCharacter::registerMotion( const LLUUID& id, LLMotionConstructor create )
{
	return mMotionController.registerMotion(id, create);
}

//-----------------------------------------------------------------------------
// removeMotion()
//-----------------------------------------------------------------------------
void LLCharacter::removeMotion( const LLUUID& id )
{
	mMotionController.removeMotion(id);
}

//-----------------------------------------------------------------------------
// findMotion()
//-----------------------------------------------------------------------------
LLMotion* LLCharacter::findMotion( const LLUUID &id )
{
	return mMotionController.findMotion( id );
}

//-----------------------------------------------------------------------------
// createMotion()
// NOTE: Always assign the result to a LLPointer!
//-----------------------------------------------------------------------------
LLMotion* LLCharacter::createMotion( const LLUUID &id )
{
	return mMotionController.createMotion( id );
}

//-----------------------------------------------------------------------------
// startMotion()
//-----------------------------------------------------------------------------
BOOL LLCharacter::startMotion(const LLUUID &id, F32 start_offset)
{
	return mMotionController.startMotion(id, start_offset);
}


//-----------------------------------------------------------------------------
// stopMotion()
//-----------------------------------------------------------------------------
BOOL LLCharacter::stopMotion(const LLUUID& id, BOOL stop_immediate)
{
	return mMotionController.stopMotionLocally(id, stop_immediate);
}

//-----------------------------------------------------------------------------
// isMotionActive()
//-----------------------------------------------------------------------------
BOOL LLCharacter::isMotionActive(const LLUUID& id)
{
	LLMotion *motionp = mMotionController.findMotion(id);
	if (motionp)
	{
		return mMotionController.isMotionActive(motionp);
	}

	return FALSE;
}


//-----------------------------------------------------------------------------
// onDeactivateMotion()
//-----------------------------------------------------------------------------
void LLCharacter::requestStopMotion( LLMotion* motion)
{
//	llinfos << "DEBUG: Char::onDeactivateMotion( " << motionName << " )" << llendl;
}


//-----------------------------------------------------------------------------
// updateMotions()
//-----------------------------------------------------------------------------
static LLFastTimer::DeclareTimer FTM_UPDATE_ANIMATION("Update Animation");
static LLFastTimer::DeclareTimer FTM_UPDATE_HIDDEN_ANIMATION("Update Hidden Anim");
static LLFastTimer::DeclareTimer FTM_UPDATE_MOTIONS("Update Motions");

void LLCharacter::updateMotions(e_update_t update_type)
{
	if (update_type == HIDDEN_UPDATE)
	{
		LLFastTimer t(FTM_UPDATE_HIDDEN_ANIMATION);
		mMotionController.updateMotionsMinimal();
	}
	else
	{
		LLFastTimer t(FTM_UPDATE_ANIMATION);
		// unpause if the number of outstanding pause requests has dropped to the initial one
		if (mMotionController.isPaused() && mPauseRequest->getNumRefs() == 1)
		{
			mMotionController.unpauseAllMotions();
		}
		bool force_update = (update_type == FORCE_UPDATE);
		{
			LLFastTimer t(FTM_UPDATE_MOTIONS);
			mMotionController.updateMotions(force_update);
		}
	}
}


//-----------------------------------------------------------------------------
// deactivateAllMotions()
//-----------------------------------------------------------------------------
void LLCharacter::deactivateAllMotions()
{
	mMotionController.deactivateAllMotions();
}


//-----------------------------------------------------------------------------
// flushAllMotions()
//-----------------------------------------------------------------------------
void LLCharacter::flushAllMotions()
{
	mMotionController.flushAllMotions();
}


//-----------------------------------------------------------------------------
// dumpCharacter()
//-----------------------------------------------------------------------------
void LLCharacter::dumpCharacter( LLJoint* joint )
{
	// handle top level entry into recursion
	if (joint == NULL)
	{
		llinfos << "DEBUG: Dumping Character @" << this << llendl;
		dumpCharacter( getRootJoint() );
		llinfos << "DEBUG: Done." << llendl;
		return;
	}

	// print joint info
	llinfos << "DEBUG: " << joint->getName() << " (" << (joint->getParent()?joint->getParent()->getName():std::string("ROOT")) << ")" << llendl;

	// recurse
	for (LLJoint::child_list_t::iterator iter = joint->mChildren.begin();
		 iter != joint->mChildren.end(); ++iter)
	{
		LLJoint* child_joint = *iter;
		dumpCharacter(child_joint);
	}
}

//-----------------------------------------------------------------------------
// setAnimationData()
//-----------------------------------------------------------------------------
void LLCharacter::setAnimationData(std::string name, void *data)
{
	mAnimationData[name] = data;
}

//-----------------------------------------------------------------------------
// getAnimationData()
//-----------------------------------------------------------------------------
void* LLCharacter::getAnimationData(std::string name)
{
	return get_if_there(mAnimationData, name, (void*)NULL);
}

//-----------------------------------------------------------------------------
// removeAnimationData()
//-----------------------------------------------------------------------------
void LLCharacter::removeAnimationData(std::string name)
{
	mAnimationData.erase(name);
}

//-----------------------------------------------------------------------------
// setVisualParamWeight()
//-----------------------------------------------------------------------------
BOOL LLCharacter::setVisualParamWeight(LLVisualParam* which_param, F32 weight, BOOL upload_bake)
{
	S32 index = which_param->getID();
	visual_param_index_map_t::iterator index_iter = mVisualParamIndexMap.find(index);
	if (index_iter != mVisualParamIndexMap.end())
	{
		index_iter->second->setWeight(weight, upload_bake);
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// setVisualParamWeight()
//-----------------------------------------------------------------------------
BOOL LLCharacter::setVisualParamWeight(const char* param_name, F32 weight, BOOL upload_bake)
{
	std::string tname(param_name);
	LLStringUtil::toLower(tname);
	char *tableptr = sVisualParamNames.checkString(tname);
	visual_param_name_map_t::iterator name_iter = mVisualParamNameMap.find(tableptr);
	if (name_iter != mVisualParamNameMap.end())
	{
		name_iter->second->setWeight(weight, upload_bake);
		return TRUE;
	}
	llwarns << "LLCharacter::setVisualParamWeight() Invalid visual parameter: " << param_name << llendl;
	return FALSE;
}

//-----------------------------------------------------------------------------
// setVisualParamWeight()
//-----------------------------------------------------------------------------
BOOL LLCharacter::setVisualParamWeight(S32 index, F32 weight, BOOL upload_bake)
{
	visual_param_index_map_t::iterator index_iter = mVisualParamIndexMap.find(index);
	if (index_iter != mVisualParamIndexMap.end())
	{
		index_iter->second->setWeight(weight, upload_bake);
		return TRUE;
	}
	llwarns << "LLCharacter::setVisualParamWeight() Invalid visual parameter index: " << index << llendl;
	return FALSE;
}

//-----------------------------------------------------------------------------
// getVisualParamWeight()
//-----------------------------------------------------------------------------
F32 LLCharacter::getVisualParamWeight(LLVisualParam *which_param)
{
	S32 index = which_param->getID();
	visual_param_index_map_t::iterator index_iter = mVisualParamIndexMap.find(index);
	if (index_iter != mVisualParamIndexMap.end())
	{
		return index_iter->second->getWeight();
	}
	else
	{
		llwarns << "LLCharacter::getVisualParamWeight() Invalid visual parameter*, index= " << index << llendl;
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// getVisualParamWeight()
//-----------------------------------------------------------------------------
F32 LLCharacter::getVisualParamWeight(const char* param_name)
{
	std::string tname(param_name);
	LLStringUtil::toLower(tname);
	char *tableptr = sVisualParamNames.checkString(tname);
	visual_param_name_map_t::iterator name_iter = mVisualParamNameMap.find(tableptr);
	if (name_iter != mVisualParamNameMap.end())
	{
		return name_iter->second->getWeight();
	}
	llwarns << "LLCharacter::getVisualParamWeight() Invalid visual parameter: " << param_name << llendl;
	return 0.f;
}

//-----------------------------------------------------------------------------
// getVisualParamWeight()
//-----------------------------------------------------------------------------
F32 LLCharacter::getVisualParamWeight(S32 index)
{
	visual_param_index_map_t::iterator index_iter = mVisualParamIndexMap.find(index);
	if (index_iter != mVisualParamIndexMap.end())
	{
		return index_iter->second->getWeight();
	}
	else
	{
		llwarns << "LLCharacter::getVisualParamWeight() Invalid visual parameter index: " << index << llendl;
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// clearVisualParamWeights()
//-----------------------------------------------------------------------------
void LLCharacter::clearVisualParamWeights()
{
	for (LLVisualParam *param = getFirstVisualParam(); 
		param;
		param = getNextVisualParam())
	{
		if (param->isTweakable())
		{
			param->setWeight( param->getDefaultWeight(), FALSE );
		}
	}
}

//-----------------------------------------------------------------------------
// getVisualParam()
//-----------------------------------------------------------------------------
LLVisualParam*	LLCharacter::getVisualParam(const char *param_name)
{
	std::string tname(param_name);
	LLStringUtil::toLower(tname);
	char *tableptr = sVisualParamNames.checkString(tname);
	visual_param_name_map_t::iterator name_iter = mVisualParamNameMap.find(tableptr);
	if (name_iter != mVisualParamNameMap.end())
	{
		return name_iter->second;
	}
	llwarns << "LLCharacter::getVisualParam() Invalid visual parameter: " << param_name << llendl;
	return NULL;
}

//-----------------------------------------------------------------------------
// addSharedVisualParam()
//-----------------------------------------------------------------------------
void LLCharacter::addSharedVisualParam(LLVisualParam *param)
{
	S32 index = param->getID();
	visual_param_index_map_t::iterator index_iter = mVisualParamIndexMap.find(index);
	LLVisualParam* current_param = 0;
	if (index_iter != mVisualParamIndexMap.end())
		current_param = index_iter->second;
	if( current_param )
	{
		LLVisualParam* next_param = current_param;
		while(next_param->getNextParam())
		{
			next_param = next_param->getNextParam();
		}
		next_param->setNextParam(param);
	}
	else
	{
		llwarns << "Shared visual parameter " << param->getName() << " does not already exist with ID " << 
			param->getID() << llendl;
	}
}

//-----------------------------------------------------------------------------
// addVisualParam()
//-----------------------------------------------------------------------------
void LLCharacter::addVisualParam(LLVisualParam *param)
{
	S32 index = param->getID();
	// Add Index map
	std::pair<visual_param_index_map_t::iterator, bool> idxres;
	idxres = mVisualParamIndexMap.insert(visual_param_index_map_t::value_type(index, param));
	if (!idxres.second)
	{
		llwarns << "Visual parameter " << param->getName() << " already exists with same ID as " << 
			param->getName() << llendl;
		visual_param_index_map_t::iterator index_iter = idxres.first;
		index_iter->second = param;
	}

	if (param->getInfo())
	{
		// Add name map
		std::string tname(param->getName());
		LLStringUtil::toLower(tname);
		char *tableptr = sVisualParamNames.addString(tname);
		std::pair<visual_param_name_map_t::iterator, bool> nameres;
		nameres = mVisualParamNameMap.insert(visual_param_name_map_t::value_type(tableptr, param));
		if (!nameres.second)
		{
			// Already exists, copy param
			visual_param_name_map_t::iterator name_iter = nameres.first;
			name_iter->second = param;
		}
	}
	//llinfos << "Adding Visual Param '" << param->getName() << "' ( " << index << " )" << llendl;
}

//-----------------------------------------------------------------------------
// updateVisualParams()
//-----------------------------------------------------------------------------
void LLCharacter::updateVisualParams()
{
	for (LLVisualParam *param = getFirstVisualParam(); 
		param;
		param = getNextVisualParam())
	{
		if (param->isAnimating())
		{
			continue;
		}
		// only apply parameters whose effective weight has changed
		F32 effective_weight = ( param->getSex() & mSex ) ? param->getWeight() : param->getDefaultWeight();
		if (effective_weight != param->getLastWeight())
		{
			param->apply( mSex );
		}
	}
}
 
LLAnimPauseRequest LLCharacter::requestPause()
{
	mMotionController.pauseAllMotions();
	return mPauseRequest;
}

