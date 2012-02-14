/** 
 * @file lldriverparam.cpp
 * @brief A visual parameter that drives (controls) other visual parameters.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lldriverparam.h"

#include "llfasttimer.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llagent.h"
#include "llwearable.h"
#include "llagentwearables.h"

//-----------------------------------------------------------------------------
// LLDriverParamInfo
//-----------------------------------------------------------------------------

LLDriverParamInfo::LLDriverParamInfo()
{
}

BOOL LLDriverParamInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "param" ) && node->getChildByName( "param_driver" ) );

	if( !LLViewerVisualParamInfo::parseXml( node ))
		return FALSE;

	LLXmlTreeNode* param_driver_node = node->getChildByName( "param_driver" );
	if( !param_driver_node )
		return FALSE;

	for (LLXmlTreeNode* child = param_driver_node->getChildByName( "driven" );
		 child;
		 child = param_driver_node->getNextNamedChild())
	{
		S32 driven_id;
		static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
		if( child->getFastAttributeS32( id_string, driven_id ) )
		{
			F32 min1 = mMinWeight;
			F32 max1 = mMaxWeight;
			F32 max2 = max1;
			F32 min2 = max1;

			//	driven    ________							//
			//	^        /|       |\						//
			//	|       / |       | \						//
			//	|      /  |       |  \						//
			//	|     /   |       |   \						//
			//	|    /    |       |    \					//
			//-------|----|-------|----|-------> driver		//
			//  | min1   max1    max2  min2

			static LLStdStringHandle min1_string = LLXmlTree::addAttributeString("min1");
			child->getFastAttributeF32( min1_string, min1 ); // optional
			static LLStdStringHandle max1_string = LLXmlTree::addAttributeString("max1");
			child->getFastAttributeF32( max1_string, max1 ); // optional
			static LLStdStringHandle max2_string = LLXmlTree::addAttributeString("max2");
			child->getFastAttributeF32( max2_string, max2 ); // optional
			static LLStdStringHandle min2_string = LLXmlTree::addAttributeString("min2");
			child->getFastAttributeF32( min2_string, min2 ); // optional

			// Push these on the front of the deque, so that we can construct
			// them in order later (faster)
			mDrivenInfoList.push_front( LLDrivenEntryInfo( driven_id, min1, max1, max2, min2 ) );
		}
		else
		{
			llerrs << "<driven> Unable to resolve driven parameter: " << driven_id << llendl;
			return FALSE;
		}
	}
	return TRUE;
}

//virtual 
void LLDriverParamInfo::toStream(std::ostream &out)
{
	LLViewerVisualParamInfo::toStream(out);
	out << "driver" << "\t";
	out << mDrivenInfoList.size() << "\t";
	for (entry_info_list_t::iterator iter = mDrivenInfoList.begin(); iter != mDrivenInfoList.end(); iter++)
	{
		LLDrivenEntryInfo driven = *iter;
		out << driven.mDrivenID << "\t";
	}

	out << std::endl;

	if(isAgentAvatarValid())
	{
		for (entry_info_list_t::iterator iter = mDrivenInfoList.begin(); iter != mDrivenInfoList.end(); iter++)
		{
			LLDrivenEntryInfo driven = *iter;
			LLViewerVisualParam *param = (LLViewerVisualParam*)gAgentAvatarp->getVisualParam(driven.mDrivenID);
			if (param)
			{
				param->getInfo()->toStream(out);
				if (param->getWearableType() != mWearableType)
				{
					if(param->getCrossWearable())
					{
						out << "cross-wearable" << "\t";
					}
					else
					{
						out << "ERROR!" << "\t";
					}
				}
				else
				{
					out << "valid" << "\t";
				}
			}
			else
			{
				llwarns << "could not get parameter " << driven.mDrivenID << " from avatar " << gAgentAvatarp.get() << " for driver parameter " << getID() << llendl;
			}
			out << std::endl;
		}
	}
}

//-----------------------------------------------------------------------------
// LLDriverParam
//-----------------------------------------------------------------------------

LLDriverParam::LLDriverParam(LLVOAvatar *avatarp) : 
	mCurrentDistortionParam( NULL ), 
	mAvatarp(avatarp), 
	mWearablep(NULL)
{
}

LLDriverParam::LLDriverParam(LLWearable *wearablep) : 
	mCurrentDistortionParam( NULL ), 
	mAvatarp(NULL), 
	mWearablep(wearablep)
{
}

LLDriverParam::~LLDriverParam()
{
}

BOOL LLDriverParam::setInfo(LLDriverParamInfo *info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
		return FALSE;
	mInfo = info;
	mID = info->mID;

	setWeight(getDefaultWeight(), FALSE );

	return TRUE;
}

void LLDriverParam::setWearable(LLWearable *wearablep)
{
	if (wearablep)
	{
		mWearablep = wearablep;
		mAvatarp = NULL;
	}
}

void LLDriverParam::setAvatar(LLVOAvatar *avatarp)
{
	if (avatarp)
	{
		mWearablep = NULL;
		mAvatarp = avatarp;
	}
}

/*virtual*/ LLViewerVisualParam* LLDriverParam::cloneParam(LLWearable* wearable) const
{
	LLDriverParam *new_param;
	if (wearable)
	{
		new_param = new LLDriverParam(wearable);
	}
	else
	{
		if (mWearablep)
		{
			new_param = new LLDriverParam(mWearablep);
		}
		else
		{
			new_param = new LLDriverParam(mAvatarp);
		}
	}
	*new_param = *this;
	return new_param;
}

#if 0 // obsolete
BOOL LLDriverParam::parseData(LLXmlTreeNode* node)
{
	LLDriverParamInfo* info = new LLDriverParamInfo;

	info->parseXml(node);
	if (!setInfo(info))
	{
		delete info;
		return FALSE;
	}
	return TRUE;
}
#endif

void LLDriverParam::setWeight(F32 weight, BOOL upload_bake)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	if (mIsAnimating)
	{
		// allow overshoot when animating
		mCurWeight = weight;
	}
	else
	{
		mCurWeight = llclamp(weight, min_weight, max_weight);
	}

	//	driven    ________
	//	^        /|       |\       ^
	//	|       / |       | \      |
	//	|      /  |       |  \     |
	//	|     /   |       |   \    |
	//	|    /    |       |    \   |
	//-------|----|-------|----|-------> driver
	//  | min1   max1    max2  min2

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		LLDrivenEntryInfo* info = driven->mInfo;
		
		F32 driven_weight = 0.f;
		F32 driven_min = driven->mParam->getMinWeight();
		F32 driven_max = driven->mParam->getMaxWeight();

		if (mIsAnimating)
		{
			// driven param doesn't interpolate (textures, for example)
			if (!driven->mParam->getAnimating())
			{
				continue;
			}
			if( mCurWeight < info->mMin1 )
			{
				if (info->mMin1 == min_weight)
				{
					if (info->mMin1 == info->mMax1)
					{
						driven_weight = driven_max;
					}
					else
					{
						//up slope extrapolation
						F32 t = (mCurWeight - info->mMin1) / (info->mMax1 - info->mMin1 );
						driven_weight = driven_min + t * (driven_max - driven_min);
					}
				}
				else
				{
					driven_weight = driven_min;
				}
				
				setDrivenWeight(driven,driven_weight,upload_bake);
				continue;
			}
			else 
			if ( mCurWeight > info->mMin2 )
			{
				if (info->mMin2 == max_weight)
				{
					if (info->mMin2 == info->mMax2)
					{
						driven_weight = driven_max;
					}
					else
					{
						//down slope extrapolation					
						F32 t = (mCurWeight - info->mMax2) / (info->mMin2 - info->mMax2 );
						driven_weight = driven_max + t * (driven_min - driven_max);
					}
				}
				else
				{
					driven_weight = driven_min;
				}

				setDrivenWeight(driven,driven_weight,upload_bake);
				continue;
			}
		}

		driven_weight = getDrivenWeight(driven, mCurWeight);
		setDrivenWeight(driven,driven_weight,upload_bake);
	}
}

F32	LLDriverParam::getTotalDistortion()
{
	F32 sum = 0.f;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getTotalDistortion();
	}

	return sum; 
}

const LLVector3	&LLDriverParam::getAvgDistortion()	
{
	// It's not actually correct to take the average of averages, but it good enough here.
	LLVector3 sum;
	S32 count = 0;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getAvgDistortion();
		count++;
	}
	sum /= (F32)count;

	mDefaultVec = sum;
	return mDefaultVec; 
}

F32	LLDriverParam::getMaxDistortion() 
{
	F32 max = 0.f;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		F32 param_max = driven->mParam->getMaxDistortion();
		if( param_max > max )
		{
			max = param_max;
		}
	}

	return max; 
}


LLVector3	LLDriverParam::getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)
{
	LLVector3 sum;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		sum += driven->mParam->getVertexDistortion( index, poly_mesh );
	}
	return sum;
}

const LLVector3*	LLDriverParam::getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	mCurrentDistortionParam = NULL;
	const LLVector3* v = NULL;
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		v = driven->mParam->getFirstDistortion( index, poly_mesh );
		if( v )
		{
			mCurrentDistortionParam = driven->mParam;
			break;
		}
	}

	return v;
};

const LLVector3*	LLDriverParam::getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	llassert( mCurrentDistortionParam );
	if( !mCurrentDistortionParam )
	{
		return NULL;
	}

	LLDrivenEntry* driven = NULL;
	entry_list_t::iterator iter;
	
	// Set mDriven iteration to the right point
	for( iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		driven = &(*iter);
		if( driven->mParam == mCurrentDistortionParam )
		{
			break;
		}
	}

	llassert(driven);
	if (!driven)
	{
		return NULL; // shouldn't happen, but...
	}

	// We're already in the middle of a param's distortions, so get the next one.
	const LLVector3* v = driven->mParam->getNextDistortion( index, poly_mesh );
	if( (!v) && (iter != mDriven.end()) )
	{
		// This param is finished, so start the next param.  It might not have any
		// distortions, though, so we have to loop to find the next param that does.
		for( iter++; iter != mDriven.end(); iter++ )
		{
			driven = &(*iter);
			v = driven->mParam->getFirstDistortion( index, poly_mesh );
			if( v )
			{
				mCurrentDistortionParam = driven->mParam;
				break;
			}
		}
	}

	return v;
};

//-----------------------------------------------------------------------------
// setAnimationTarget()
//-----------------------------------------------------------------------------
void LLDriverParam::setAnimationTarget( F32 target_value, BOOL upload_bake )
{
	LLVisualParam::setAnimationTarget(target_value, upload_bake);

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		F32 driven_weight = getDrivenWeight(driven, mTargetWeight);

		// this isn't normally necessary, as driver params handle interpolation of their driven params
		// but texture params need to know to assume their final value at beginning of interpolation
		driven->mParam->setAnimationTarget(driven_weight, upload_bake);
	}
}

//-----------------------------------------------------------------------------
// stopAnimating()
//-----------------------------------------------------------------------------
void LLDriverParam::stopAnimating(BOOL upload_bake)
{
	LLVisualParam::stopAnimating(upload_bake);

	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		driven->mParam->setAnimating(FALSE);
	}
}

/*virtual*/ 
BOOL LLDriverParam::linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params)
{
	BOOL success = TRUE;
	LLDriverParamInfo::entry_info_list_t::iterator iter;
	for (iter = getInfo()->mDrivenInfoList.begin(); iter != getInfo()->mDrivenInfoList.end(); ++iter)
	{
		LLDrivenEntryInfo *driven_info = &(*iter);
		S32 driven_id = driven_info->mDrivenID;

		// check for already existing links. Do not overwrite.
		BOOL found = FALSE;
		for (entry_list_t::iterator driven_iter = mDriven.begin(); driven_iter != mDriven.end() && !found; ++driven_iter)
		{
			if (driven_iter->mInfo->mDrivenID == driven_id)
			{
				found = TRUE;
			}
		}

		if (!found)
		{
			LLViewerVisualParam* param = (LLViewerVisualParam*)mapper(driven_id);
			bool push = param && (!only_cross_params || param->getCrossWearable());
			if (push)
			{
				mDriven.push_back(LLDrivenEntry( param, driven_info ));
			}
			else
			{
				success = FALSE;
			}
		}
	}
	
	return success;	
}

void LLDriverParam::resetDrivenParams()
{
	mDriven.clear();
	mDriven.reserve(getInfo()->mDrivenInfoList.size());
}

void LLDriverParam::updateCrossDrivenParams(LLWearableType::EType driven_type)
{
	bool needs_update = (getWearableType()==driven_type);

	// if the driver has a driven entry for the passed-in wearable type, we need to refresh the value
	for( entry_list_t::iterator iter = mDriven.begin(); iter != mDriven.end(); iter++ )
	{
		LLDrivenEntry* driven = &(*iter);
		if (driven && driven->mParam && driven->mParam->getCrossWearable() && driven->mParam->getWearableType() == driven_type)
		{
			needs_update = true;
		}
	}


	if (needs_update)
	{
		LLWearableType::EType driver_type = (LLWearableType::EType)getWearableType();
		
		// If we've gotten here, we've added a new wearable of type "type"
		// Thus this wearable needs to get updates from the driver wearable.
		// The call to setVisualParamWeight seems redundant, but is necessary
		// as the number of driven wearables has changed since the last update. -Nyx
		LLWearable *wearable = gAgentWearables.getTopWearable(driver_type);
		if (wearable)
		{
			wearable->setVisualParamWeight(mID, wearable->getVisualParamWeight(mID), false);
		}
	}
}


//-----------------------------------------------------------------------------
// getDrivenWeight()
//-----------------------------------------------------------------------------
F32 LLDriverParam::getDrivenWeight(const LLDrivenEntry* driven, F32 input_weight)
{
	F32 min_weight = getMinWeight();
	F32 max_weight = getMaxWeight();
	const LLDrivenEntryInfo* info = driven->mInfo;

	F32 driven_weight = 0.f;
	F32 driven_min = driven->mParam->getMinWeight();
	F32 driven_max = driven->mParam->getMaxWeight();

	if( input_weight <= info->mMin1 )
	{
		if( info->mMin1 == info->mMax1 && 
			info->mMin1 <= min_weight)
		{
			driven_weight = driven_max;
		}
		else
		{
			driven_weight = driven_min;
		}
	}
	else
	if( input_weight <= info->mMax1 )
	{
		F32 t = (input_weight - info->mMin1) / (info->mMax1 - info->mMin1 );
		driven_weight = driven_min + t * (driven_max - driven_min);
	}
	else
	if( input_weight <= info->mMax2 )
	{
		driven_weight = driven_max;
	}
	else
	if( input_weight <= info->mMin2 )
	{
		F32 t = (input_weight - info->mMax2) / (info->mMin2 - info->mMax2 );
		driven_weight = driven_max + t * (driven_min - driven_max);
	}
	else
	{
		if (info->mMax2 >= max_weight)
		{
			driven_weight = driven_max;
		}
		else
		{
			driven_weight = driven_min;
		}
	}

	return driven_weight;
}

void LLDriverParam::setDrivenWeight(LLDrivenEntry *driven, F32 driven_weight, bool upload_bake)
{
	if(isAgentAvatarValid() &&
	   mWearablep && 
	   driven->mParam->getCrossWearable() &&
	   mWearablep->isOnTop())
	{
		// call setWeight through LLVOAvatarSelf so other wearables can be updated with the correct values
		gAgentAvatarp->setVisualParamWeight( (LLVisualParam*)driven->mParam, driven_weight, upload_bake );
	}
	else
	{
		driven->mParam->setWeight( driven_weight, upload_bake );
	}
}
