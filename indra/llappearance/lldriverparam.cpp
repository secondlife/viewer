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

#include "linden_common.h"

#include "lldriverparam.h"

#include "llavatarappearance.h"
#include "llwearable.h"
#include "llwearabledata.h"

//-----------------------------------------------------------------------------
// LLDriverParamInfo
//-----------------------------------------------------------------------------

LLDriverParamInfo::LLDriverParamInfo() :
	mDriverParam(NULL)
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
			LL_ERRS() << "<driven> Unable to resolve driven parameter: " << driven_id << LL_ENDL;
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
	for (LLDrivenEntryInfo& driven : mDrivenInfoList)
	{
		out << driven.mDrivenID << "\t";
	}

	out << std::endl;

	// FIXME - this mDriverParam backlink makes no sense, because the
	// LLDriverParamInfos are static objects - there's only one copy
	// for each param type, so the backlink will just reference the
	// corresponding param in the most recently created
	// avatar. Apparently these toStream() methods are not currently
	// used anywhere, so it's not an urgent problem.
    LL_WARNS_ONCE() << "Invalid usage of mDriverParam." << LL_ENDL;
    
	if(mDriverParam && mDriverParam->getAvatarAppearance()->isSelf() &&
		mDriverParam->getAvatarAppearance()->isValid())
	{
		for (LLDrivenEntryInfo& driven : mDrivenInfoList)
		{
			LLViewerVisualParam *param = 
				(LLViewerVisualParam*)mDriverParam->getAvatarAppearance()->getVisualParam(driven.mDrivenID);
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
				LL_WARNS() << "could not get parameter " << driven.mDrivenID << " from avatar " 
						<< mDriverParam->getAvatarAppearance() 
						<< " for driver parameter " << getID() << LL_ENDL;
			}
			out << std::endl;
		}
	}
}

//-----------------------------------------------------------------------------
// LLDriverParam
//-----------------------------------------------------------------------------

LLDriverParam::LLDriverParam(LLAvatarAppearance *appearance, LLWearable* wearable /* = NULL */)
	: LLViewerVisualParam(),
	mDefaultVec(),
	mDriven(),
	mCurrentDistortionParam( NULL ), 
	mAvatarAppearance(appearance), 
	mWearablep(wearable)
{
	llassert(mAvatarAppearance);
	llassert((mWearablep == NULL) || mAvatarAppearance->isSelf());
	mDefaultVec.clear();
}

LLDriverParam::LLDriverParam(const LLDriverParam& pOther)
	: LLViewerVisualParam(pOther),
	mDefaultVec(pOther.mDefaultVec),
	mDriven(pOther.mDriven),
	mCurrentDistortionParam(pOther.mCurrentDistortionParam),
	mAvatarAppearance(pOther.mAvatarAppearance),
	mWearablep(pOther.mWearablep)
{
	llassert(mAvatarAppearance);
	llassert((mWearablep == NULL) || mAvatarAppearance->isSelf());
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
	info->mDriverParam = this;

	setWeight(getDefaultWeight());

	return TRUE;
}

/*virtual*/ LLViewerVisualParam* LLDriverParam::cloneParam(LLWearable* wearable) const
{
	llassert(wearable);
	return new LLDriverParam(*this);
}

void LLDriverParam::setWeight(F32 weight)
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

	for(LLDrivenEntry& driven : mDriven)
	{
		LLDrivenEntry* drivenp = &driven;
		LLDrivenEntryInfo* info = drivenp->mInfo;
		
		F32 driven_weight = 0.f;
		F32 driven_min = drivenp->mParam->getMinWeight();
		F32 driven_max = drivenp->mParam->getMaxWeight();

		if (mIsAnimating)
		{
			// driven param doesn't interpolate (textures, for example)
			if (!drivenp->mParam->getAnimating())
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
				
				setDrivenWeight(drivenp,driven_weight);
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

				setDrivenWeight(drivenp,driven_weight);
				continue;
			}
		}

		driven_weight = getDrivenWeight(drivenp, mCurWeight);
		setDrivenWeight(drivenp,driven_weight);
	}
}

F32	LLDriverParam::getTotalDistortion()
{
	F32 sum = 0.f;
	for(LLDrivenEntry& driven : mDriven)
	{
		sum += driven.mParam->getTotalDistortion();
	}

	return sum; 
}

const LLVector4a	&LLDriverParam::getAvgDistortion()	
{
	// It's not actually correct to take the average of averages, but it good enough here.
	LLVector4a sum;
	sum.clear();
	S32 count = 0;
	for(LLDrivenEntry& driven : mDriven)
	{
		sum.add(driven.mParam->getAvgDistortion());
		count++;
	}
	sum.mul( 1.f/(F32)count);

	mDefaultVec = sum;
	return mDefaultVec; 
}

F32	LLDriverParam::getMaxDistortion() 
{
	F32 max = 0.f;
	for(LLDrivenEntry& driven : mDriven)
	{
		F32 param_max = driven.mParam->getMaxDistortion();
		if( param_max > max )
		{
			max = param_max;
		}
	}

	return max; 
}


LLVector4a	LLDriverParam::getVertexDistortion(S32 index, LLPolyMesh *poly_mesh)
{
	LLVector4a sum;
	sum.clear();
	for(LLDrivenEntry& driven : mDriven)
	{
		sum.add(driven.mParam->getVertexDistortion(index, poly_mesh));
	}
	return sum;
}

const LLVector4a*	LLDriverParam::getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	mCurrentDistortionParam = NULL;
	const LLVector4a* v = NULL;
	for(LLDrivenEntry& driven : mDriven)
	{
		v = driven.mParam->getFirstDistortion(index, poly_mesh);
		if( v )
		{
			mCurrentDistortionParam = driven.mParam;
			break;
		}
	}

	return v;
};

const LLVector4a*	LLDriverParam::getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)
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
	const LLVector4a* v = driven->mParam->getNextDistortion( index, poly_mesh );
	if( (!v) && (iter != mDriven.end()) )
	{
		// This param is finished, so start the next param.  It might not have any
		// distortions, though, so we have to loop to find the next param that does.
		for( iter++; iter != mDriven.end(); iter++ )
		{
			driven = &(*iter);
			v = driven->mParam->getFirstDistortion(index, poly_mesh);
			if( v )
			{
				mCurrentDistortionParam = driven->mParam;
				break;
			}
		}
	}

	return v;
};

S32 LLDriverParam::getDrivenParamsCount() const
{
	return mDriven.size();
}

const LLViewerVisualParam* LLDriverParam::getDrivenParam(S32 index) const
{
	if (0 > index || index >= mDriven.size())
	{
		return NULL;
	}
	return mDriven[index].mParam;
}

//-----------------------------------------------------------------------------
// setAnimationTarget()
//-----------------------------------------------------------------------------
void LLDriverParam::setAnimationTarget( F32 target_value)
{
	LLVisualParam::setAnimationTarget(target_value);

	for(LLDrivenEntry& driven : mDriven)
	{
		LLDrivenEntry* drivenp = &driven;
		F32 driven_weight = getDrivenWeight(drivenp, mTargetWeight);

		// this isn't normally necessary, as driver params handle interpolation of their driven params
		// but texture params need to know to assume their final value at beginning of interpolation
		drivenp->mParam->setAnimationTarget(driven_weight);
	}
}

//-----------------------------------------------------------------------------
// stopAnimating()
//-----------------------------------------------------------------------------
void LLDriverParam::stopAnimating()
{
	LLVisualParam::stopAnimating();

	for(LLDrivenEntry& driven : mDriven)
	{
		driven.mParam->setAnimating(FALSE);
	}
}

/*virtual*/ 
BOOL LLDriverParam::linkDrivenParams(visual_param_mapper mapper, BOOL only_cross_params)
{
	BOOL success = TRUE;
	for (LLDrivenEntryInfo& driven_info : getInfo()->mDrivenInfoList)
	{
		S32 driven_id = driven_info.mDrivenID;

		// check for already existing links. Do not overwrite.
		BOOL found = FALSE;
		for (auto& driven : mDriven)
		{
			if (driven.mInfo->mDrivenID == driven_id)
			{
				found = TRUE;
			}
		}

		if (!found)
		{
			LLViewerVisualParam* param = (LLViewerVisualParam*)mapper(driven_id);
			if (param) param->setParamLocation(this->getParamLocation());
			bool push = param && (!only_cross_params || param->getCrossWearable());
			if (push)
			{
				mDriven.push_back(LLDrivenEntry( param, &driven_info ));
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
	for(LLDrivenEntry& driven : mDriven)
	{
		if (driven.mParam && driven.mParam->getCrossWearable() && driven.mParam->getWearableType() == driven_type)
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
		LLWearable *wearable = mAvatarAppearance->getWearableData()->getTopWearable(driver_type);
		if (wearable)
		{
			wearable->setVisualParamWeight(mID, wearable->getVisualParamWeight(mID));
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

void LLDriverParam::setDrivenWeight(LLDrivenEntry *driven, F32 driven_weight)
{
	bool use_self = false;
	if(mWearablep &&
		mAvatarAppearance->isValid() &&
		driven->mParam->getCrossWearable())
	{
		LLWearable* wearable = mWearablep;
		if (mAvatarAppearance->getWearableData()->isOnTop(wearable))
		{
			use_self = true;
		}
	}

	if (use_self)
	{
		// call setWeight through LLVOAvatarSelf so other wearables can be updated with the correct values
		mAvatarAppearance->setVisualParamWeight( (LLVisualParam*)driven->mParam, driven_weight);
	}
	else
	{
		driven->mParam->setWeight( driven_weight);
	}
}
