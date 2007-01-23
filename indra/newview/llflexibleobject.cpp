/** 
 * @file llflexibleobject.cpp
 * @brief Flexible object implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llflexibleobject.h"
#include "llglheaders.h"
#include "llsphere.h"
#include "llviewerobject.h"
#include "llimagegl.h"
#include "llagent.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"

/*static*/ LLVolumeImplFlexible::lodset_t LLVolumeImplFlexible::sLODBins[ FLEXIBLE_OBJECT_MAX_LOD ];
/*static*/ U64 LLVolumeImplFlexible::sCurrentUpdateFrame = 0;
/*static*/ U32 LLVolumeImplFlexible::sDebugInserted = 0;
/*static*/ U32 LLVolumeImplFlexible::sDebugVisible = 0;

/*static*/ F32 LLVolumeImplFlexible::sUpdateFactor = 1.0f;

// LLFlexibleObjectData::pack/unpack now in llprimitive.cpp

//-----------------------------------------------
// constructor
//-----------------------------------------------
LLVolumeImplFlexible::LLVolumeImplFlexible(LLViewerObject* vo, LLFlexibleObjectData* attributes) :
		mVO(vo), mAttributes(attributes)
{
	mInitialized = FALSE;
	mUpdated = FALSE;
	mJustShifted = FALSE;
	mInitializedRes = -1;
	mSimulateRes = 0;
	mFrameNum = 0;
	mLastUpdate = 0;

}//-----------------------------------------------

LLVector3 LLVolumeImplFlexible::getFramePosition() const
{
	return mVO->getRenderPosition();
}

LLQuaternion LLVolumeImplFlexible::getFrameRotation() const
{
	return mVO->getRenderRotation();
}

void LLVolumeImplFlexible::onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin)
{
	if (param_type == LLNetworkData::PARAMS_FLEXIBLE)
	{
		mAttributes = (LLFlexibleObjectData*)data;
		setAttributesOfAllSections();
	}
}

void LLVolumeImplFlexible::onShift(const LLVector3 &shift_vector)
{	
	for (int section = 0; section < (1<<FLEXIBLE_OBJECT_MAX_SECTIONS)+1; ++section)
	{
		mSection[section].mPosition += shift_vector;	
	}
	mVO->getVolume()->mBounds[0] += shift_vector;
}

//-----------------------------------------------------------------------------------------------
void LLVolumeImplFlexible::setParentPositionAndRotationDirectly( LLVector3 p, LLQuaternion r )
{
	mParentPosition = p;
	mParentRotation = r;

}//-----------------------------------------------------------------------------------------------------

void LLVolumeImplFlexible::remapSections(LLFlexibleObjectSection *source, S32 source_sections,
										 LLFlexibleObjectSection *dest, S32 dest_sections)
{
	S32 num_output_sections = 1<<dest_sections;
	LLVector3 scale = mVO->mDrawable->getScale();
	F32 source_section_length = scale.mV[VZ] / (F32)(1<<source_sections);
	F32 section_length = scale.mV[VZ] / (F32)num_output_sections;
	if (source_sections == -1)
	{
		// Generate all from section 0
		dest[0] = source[0];
		for (S32 section=0; section<num_output_sections; ++section)
		{
			dest[section+1] = dest[section];
			dest[section+1].mPosition += dest[section].mDirection * section_length;
			dest[section+1].mVelocity.setVec( LLVector3::zero );
		}
	}
	else if (source_sections > dest_sections)
	{
		// Copy, skipping sections

		S32 num_steps = 1<<(source_sections-dest_sections);

		// Copy from left to right since it may be an in-place computation
		for (S32 section=0; section<num_output_sections; ++section)
		{
			dest[section+1] = source[(section+1)*num_steps];
		}
		dest[0] = source[0];
	}
	else if (source_sections < dest_sections)
	{
		// Interpolate section info
		// Iterate from right to left since it may be an in-place computation
		S32 step_shift = dest_sections-source_sections;
		S32 num_steps = 1<<step_shift;
		for (S32 section=num_output_sections-num_steps; section>=0; section -= num_steps)
		{
			LLFlexibleObjectSection *last_source_section = &source[section>>step_shift];
			LLFlexibleObjectSection *source_section = &source[(section>>step_shift)+1];

			// Cubic interpolation of position
			// At^3 + Bt^2 + Ct + D = f(t)
			LLVector3 D = last_source_section->mPosition;
			LLVector3 C = last_source_section->mdPosition * source_section_length;
			LLVector3 Y = source_section->mdPosition * source_section_length - C; // Helper var
			LLVector3 X = (source_section->mPosition - D - C); // Helper var
			LLVector3 A = Y - 2*X;
			LLVector3 B = X - A;

			F32 t_inc = 1.f/F32(num_steps);
			F32 t = t_inc;
			for (S32 step=1; step<num_steps; ++step)
			{
				dest[section+step].mScale = 
					lerp(last_source_section->mScale, source_section->mScale, t);
				dest[section+step].mAxisRotation = 
					slerp(t, last_source_section->mAxisRotation, source_section->mAxisRotation);

				// Evaluate output interpolated values
				F32 t_sq = t*t;
				dest[section+step].mPosition = t_sq*(t*A + B) + t*C + D;
				dest[section+step].mRotation = 
					slerp(t, last_source_section->mRotation, source_section->mRotation);
				dest[section+step].mVelocity = lerp(last_source_section->mVelocity, source_section->mVelocity, t);
				dest[section+step].mDirection = lerp(last_source_section->mDirection, source_section->mDirection, t);
				dest[section+step].mdPosition = lerp(last_source_section->mdPosition, source_section->mdPosition, t);
				dest[section+num_steps] = *source_section;
				t += t_inc;
			}
		}
		dest[0] = source[0];
	}
	else
	{
		// numbers are equal. copy info
		for (S32 section=0; section <= num_output_sections; ++section)
		{
			dest[section] = source[section];
		}
	}
}


//-----------------------------------------------------------------------------
void LLVolumeImplFlexible::setAttributesOfAllSections()
{
	LLVector2 bottom_scale, top_scale;
	F32 begin_rot = 0, end_rot = 0;
	if (mVO->getVolume())
	{
		const LLPathParams &params = mVO->getVolume()->getParams().getPathParams();
		bottom_scale = params.getBeginScale();
		top_scale = params.getEndScale();
		begin_rot = F_PI * params.getTwistBegin();
		end_rot = F_PI * params.getTwist();
	}

	if (!mVO->mDrawable)
	{
		return;
	}
	
	S32 num_sections = 1 << mSimulateRes;

	LLVector3 scale = mVO->mDrawable->getScale();
									
	mSection[0].mPosition = getAnchorPosition();
	mSection[0].mDirection = LLVector3::z_axis * getFrameRotation();
	mSection[0].mdPosition = mSection[0].mDirection;
	mSection[0].mScale.setVec(scale.mV[VX]*bottom_scale.mV[0], scale.mV[VY]*bottom_scale.mV[1]);
	mSection[0].mVelocity.setVec(0,0,0);
	mSection[0].mAxisRotation.setQuat(begin_rot,0,0,1);

	LLVector3 parentSectionPosition = mSection[0].mPosition;
	LLVector3 last_direction = mSection[0].mDirection;

	remapSections(mSection, mInitializedRes, mSection, mSimulateRes);
	mInitializedRes = mSimulateRes;

	F32 t_inc = 1.f/F32(num_sections);
	F32 t = t_inc;
	for ( int i=1; i<= num_sections; i++)
	{
		mSection[i].mAxisRotation.setQuat(lerp(begin_rot,end_rot,t),0,0,1);
		mSection[i].mScale = LLVector2(
			scale.mV[VX] * lerp(bottom_scale.mV[0], top_scale.mV[0], t), 
			scale.mV[VY] * lerp(bottom_scale.mV[1], top_scale.mV[1], t));
		t += t_inc;
	}
	mLastUpdate = 0;
}//-----------------------------------------------------------------------------------


void LLVolumeImplFlexible::onSetVolume(const LLVolumeParams &volume_params, const S32 detail)
{
	doIdleUpdate(gAgent, *gWorldp, 0.0);
	if (mVO && mVO->mDrawable.notNull())
	{
		gPipeline.markRebuild(mVO->mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
		gPipeline.markMoved(mVO->mDrawable);
	}
}

//---------------------------------------------------------------------------------
// This calculates the physics of the flexible object. Note that it has to be 0
// updated every time step. In the future, perhaps there could be an 
// optimization similar to what Havok does for objects that are stationary. 
//---------------------------------------------------------------------------------
BOOL LLVolumeImplFlexible::doIdleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	if (mVO->mDrawable.isNull())
	{
		// Don't do anything until we have a drawable
		return TRUE;
	}

	//flexible objects never go static
	mVO->mDrawable->mQuietCount = 0;
	if (!mVO->mDrawable->isRoot())
	{
		mVO->mDrawable->getParent()->mQuietCount = 0;
	}
	
	if (((LLVOVolume*)mVO)->mLODChanged || 
		mVO->mDrawable->isState(LLDrawable::IN_REBUILD_Q1))
	{
		mLastUpdate = 0; // Force an immediate update
	}
	// Relegate invisible objects to the lowest priority bin
	S32 lod = 0;
	F32 app_angle = mVO->getAppAngle()*DEG_TO_RAD/gCamera->getView();
	if (mVO->mDrawable->isVisible())
	{
		sDebugVisible++;
		if (mVO->isSelected())
		{
			// Force selected objects to update *every* frame
			lod = FLEXIBLE_OBJECT_MAX_LOD-1;
		}
		else
		{
			if (app_angle > 0)
			{
				lod = 5 - (S32)(1.0f/sqrtf(app_angle));
				if (lod < 1)
				{
					lod = 1;
				}
			}

			if (mVO->isAttachment())
			{
				lod += 3;
			}
		}
	}

	S32 new_res = mAttributes->getSimulateLOD();
	// Rendering sections increases with visible angle on the screen
	mRenderRes = (S32)(FLEXIBLE_OBJECT_MAX_SECTIONS*4*app_angle);
	if (mRenderRes > FLEXIBLE_OBJECT_MAX_SECTIONS)
	{
		mRenderRes = FLEXIBLE_OBJECT_MAX_SECTIONS;
	}
	// Bottom cap at 1/4 the original number of sections
	if (mRenderRes < mAttributes->getSimulateLOD()-1)
	{
		mRenderRes = mAttributes->getSimulateLOD()-1;
	}
	// Throttle back simulation of segments we're not rendering
	if (mRenderRes < new_res)
	{
		new_res = mRenderRes;
	}

	if (!mInitialized || (mSimulateRes != new_res))
	{
		mSimulateRes = new_res;
		setAttributesOfAllSections();
		mInitialized = TRUE;
	}

	sLODBins[lod].insert(this);
	sDebugInserted++;
	return TRUE;
}

// static
void LLVolumeImplFlexible::resetUpdateBins()
{
	U32 lod;
	for (lod=0; lod<FLEXIBLE_OBJECT_MAX_LOD; ++lod)
	{
		sLODBins[lod].clear();
	}
	++sCurrentUpdateFrame;
	sDebugInserted = 0;
	sDebugVisible = 0;
}

inline S32 log2(S32 x)
{
	S32 ret = 0;
	while (x > 1)
	{
		++ret;
		x >>= 1;
	}
	return ret;
}

// static
void LLVolumeImplFlexible::doFlexibleUpdateBins()
{
	U32 lod;
	U32 updated = 0;
	U32 regen = 0;
	U32 newflexies = 0;
	F32 time_alloc[FLEXIBLE_OBJECT_MAX_LOD];
	F32 total_time_alloc = 0;

	bool new_objects_only = false;

	if (!gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FLEXIBLE))
	{
		new_objects_only = true;
	}

	for (lod=0; lod<FLEXIBLE_OBJECT_MAX_LOD; ++lod)
	{
		int count = sLODBins[lod].size();
		if (count > 0)
		{
			time_alloc[lod] = (F32)((lod+1)*(log2(count)));
		}
		else
		{
			time_alloc[lod] = 0;
		}
		total_time_alloc += time_alloc[lod];
	}
	total_time_alloc = FLEXIBLE_OBJECT_TIMESLICE * (sUpdateFactor+0.01f) / total_time_alloc;

	{
		LLFastTimer t(LLFastTimer::FTM_FLEXIBLE_UPDATE);
		LLTimer timer;
		for (lod=0; lod<FLEXIBLE_OBJECT_MAX_LOD; ++lod)
		{
			LLVolumeImplFlexible::lodset_t::iterator itor = sLODBins[lod].begin();
			int bin_count = 0;
			if (!new_objects_only)
			{
				timer.reset();
				double end_time = time_alloc[lod] * total_time_alloc;
				for (; itor!=sLODBins[lod].end(); ++itor)
				{

					(*itor)->doFlexibleUpdate();
					++updated;
					(*itor)->doFlexibleRebuild();
					++bin_count;
					++regen;
					if (timer.getElapsedTimeF64() > end_time)
					{
						break;
					}
				}
			}
			for (; itor != sLODBins[lod].end(); ++itor)
			{
				if ((*itor)->getLastUpdate() == 0)
				{
					// *Always* update newly-created objects, or objects which have changed LOD
					(*itor)->doFlexibleUpdate();
					(*itor)->doFlexibleRebuild();
					++newflexies;
				}
			}
		}
	}
}

void LLVolumeImplFlexible::doFlexibleUpdate()
{
	LLPath *path = &mVO->getVolume()->getPath();
	S32 num_sections = 1 << mSimulateRes;

    F32 secondsThisFrame = mTimer.getElapsedTimeAndResetF32();
	if (secondsThisFrame > 0.2f)
	{
		secondsThisFrame = 0.2f;
	}

	LLVector3 BasePosition = getFramePosition();
	LLQuaternion BaseRotation = getFrameRotation();
	LLQuaternion parentSegmentRotation = BaseRotation;
	LLVector3 anchorDirectionRotated = LLVector3::z_axis * parentSegmentRotation;
	LLVector3 anchorScale = mVO->mDrawable->getScale();
	
	F32 section_length = anchorScale.mV[VZ] / (F32)num_sections;
	F32 inv_section_length = 1.f / section_length;

	S32 i;

	// ANCHOR position is offset from BASE position (centroid) by half the length
	LLVector3 AnchorPosition = BasePosition - (anchorScale.mV[VZ]/2 * anchorDirectionRotated);
	
	mSection[0].mPosition = AnchorPosition;
	mSection[0].mDirection = anchorDirectionRotated;
	mSection[0].mRotation = BaseRotation;

	LLQuaternion deltaRotation;

	LLVector3 lastPosition;

	// Coefficients which are constant across sections
	F32 t_factor = mAttributes->getTension() * 0.1f;
	t_factor = t_factor*(1 - pow(0.85f, secondsThisFrame*30));
	if ( t_factor > FLEXIBLE_OBJECT_MAX_INTERNAL_TENSION_FORCE )
	{
		t_factor = FLEXIBLE_OBJECT_MAX_INTERNAL_TENSION_FORCE;
	}

	F32 friction_coeff = (mAttributes->getAirFriction()*2+1);
	friction_coeff = pow(10.f, friction_coeff*secondsThisFrame);
	friction_coeff = (friction_coeff > 1) ? friction_coeff : 1;
	F32 momentum = 1.0f / friction_coeff;

	F32 wind_factor = (mAttributes->getWindSensitivity()*0.1f) * section_length * secondsThisFrame;
	F32 max_angle = atan(section_length*2.f);

	F32 force_factor = section_length * secondsThisFrame;

	// Update simulated sections
	for (i=1; i<=num_sections; ++i)
	{
		LLVector3 parentSectionVector;
		LLVector3 parentSectionPosition;
		LLVector3 parentDirection;

		//---------------------------------------------------
		// save value of position as lastPosition
		//---------------------------------------------------
		lastPosition = mSection[i].mPosition;

		//------------------------------------------------------------------------------------------
		// gravity
		//------------------------------------------------------------------------------------------
		mSection[i].mPosition.mV[2] -= mAttributes->getGravity() * force_factor;

		//------------------------------------------------------------------------------------------
		// wind force
		//------------------------------------------------------------------------------------------
		if (mAttributes->getWindSensitivity() > 0.001f)
		{
			mSection[i].mPosition += gAgent.getRegion()->mWind.getVelocity( mSection[i].mPosition ) * wind_factor;
		}

		//------------------------------------------------------------------------------------------
		// user-defined force
		//------------------------------------------------------------------------------------------
		mSection[i].mPosition += mAttributes->getUserForce() * force_factor;

		//---------------------------------------------------
		// tension (rigidity, stiffness)
		//---------------------------------------------------
		parentSectionPosition = mSection[i-1].mPosition;
		parentDirection = mSection[i-1].mDirection;

		if ( i == 1 )
		{
			parentSectionVector = mSection[0].mDirection;
		}
		else
		{
			parentSectionVector = mSection[i-2].mDirection;
		}

		LLVector3 currentVector = mSection[i].mPosition - parentSectionPosition;

		LLVector3 difference = (parentSectionVector*section_length) - currentVector;
		LLVector3 tensionForce = difference * t_factor;

		mSection[i].mPosition += tensionForce;

		//------------------------------------------------------------------------------------------
		// sphere collision, currently not used
		//------------------------------------------------------------------------------------------
		/*if ( mAttributes->mUsingCollisionSphere )
		{
			LLVector3 vectorToCenterOfCollisionSphere = mCollisionSpherePosition - mSection[i].mPosition;
			if ( vectorToCenterOfCollisionSphere.magVecSquared() < mCollisionSphereRadius * mCollisionSphereRadius )
			{
				F32 distanceToCenterOfCollisionSphere = vectorToCenterOfCollisionSphere.magVec();
				F32 penetration = mCollisionSphereRadius - distanceToCenterOfCollisionSphere;

				LLVector3 normalToCenterOfCollisionSphere;
				
				if ( distanceToCenterOfCollisionSphere > 0.0f )
				{
					normalToCenterOfCollisionSphere = vectorToCenterOfCollisionSphere / distanceToCenterOfCollisionSphere;
				}
				else // rare
				{
					normalToCenterOfCollisionSphere = LLVector3::x_axis; // arbitrary
				}

				// push the position out to the surface of the collision sphere
				mSection[i].mPosition -= normalToCenterOfCollisionSphere * penetration;
			}
		}*/

		//------------------------------------------------------------------------------------------
		// inertia
		//------------------------------------------------------------------------------------------
		mSection[i].mPosition += mSection[i].mVelocity * momentum;

		//------------------------------------------------------------------------------------------
		// clamp length & rotation
		//------------------------------------------------------------------------------------------
		mSection[i].mDirection = mSection[i].mPosition - parentSectionPosition;
		mSection[i].mDirection.normVec();
		deltaRotation.shortestArc( parentDirection, mSection[i].mDirection );

		F32 angle;
		LLVector3 axis;
		deltaRotation.getAngleAxis(&angle, axis);
		if (angle > F_PI) angle -= 2.f*F_PI;
		if (angle < -F_PI) angle += 2.f*F_PI;
		if (angle > max_angle)
		{
			//angle = 0.5f*(angle+max_angle);
			deltaRotation.setQuat(max_angle, axis);
		} else if (angle < -max_angle)
		{
			//angle = 0.5f*(angle-max_angle);
			deltaRotation.setQuat(-max_angle, axis);
		}
		LLQuaternion segment_rotation = parentSegmentRotation * deltaRotation;
		parentSegmentRotation = segment_rotation;

		mSection[i].mDirection = (parentDirection * deltaRotation);
		mSection[i].mPosition = parentSectionPosition + mSection[i].mDirection * section_length;
		mSection[i].mRotation = segment_rotation;

		if (i > 1)
		{
			// Propogate half the rotation up to the parent
			LLQuaternion halfDeltaRotation(angle/2, axis);
			mSection[i-1].mRotation = mSection[i-1].mRotation * halfDeltaRotation;
		}

		//------------------------------------------------------------------------------------------
		// calculate velocity
		//------------------------------------------------------------------------------------------
		mSection[i].mVelocity = mSection[i].mPosition - lastPosition;
	}

	// Calculate derivatives (not necessary until normals are automagically generated)
	mSection[0].mdPosition = (mSection[1].mPosition - mSection[0].mPosition) * inv_section_length;
	// i = 1..NumSections-1
	for (i=1; i<num_sections; ++i)
	{
		// Quadratic numerical derivative of position

		// f(-L1) = aL1^2 - bL1 + c = f1
		// f(0)   =               c = f2
		// f(L2)  = aL2^2 + bL2 + c = f3
		// f = ax^2 + bx + c
		// d/dx f = 2ax + b
		// d/dx f(0) = b

		// c = f2
		// a = [(f1-c)/L1 + (f3-c)/L2] / (L1+L2)
		// b = (f3-c-aL2^2)/L2

		LLVector3 a = (mSection[i-1].mPosition-mSection[i].mPosition +
					mSection[i+1].mPosition-mSection[i].mPosition) * 0.5f * inv_section_length * inv_section_length;
		LLVector3 b = (mSection[i+1].mPosition-mSection[i].mPosition - a*(section_length*section_length));
		b *= inv_section_length;

		mSection[i].mdPosition = b;
	}

	// i = NumSections
	mSection[i].mdPosition = (mSection[i].mPosition - mSection[i-1].mPosition) * inv_section_length;

	// Create points
	S32 num_render_sections = 1<<mRenderRes;
	path->resizePath(num_render_sections+1);

	LLPath::PathPt *new_point;

	LLFlexibleObjectSection newSection[ (1<<FLEXIBLE_OBJECT_MAX_SECTIONS)+1 ];
	remapSections(mSection, mSimulateRes, newSection, mRenderRes);

	for (i=0; i<=num_render_sections; ++i)
	{
		new_point = &path->mPath[i];
		new_point->mPos = newSection[i].mPosition;
		new_point->mRot = mSection[i].mAxisRotation * newSection[i].mRotation;
		new_point->mScale = newSection[i].mScale;
		new_point->mTexT = ((F32)i)/(num_render_sections);
	}

	mLastSegmentRotation = parentSegmentRotation;
}

void LLVolumeImplFlexible::doFlexibleRebuild()
{
	mVO->getVolume()->regen();

	mVO->markForUpdate(TRUE);

	mUpdated = TRUE;

	mLastUpdate = sCurrentUpdateFrame;
}//------------------------------------------------------------------

void LLVolumeImplFlexible::onSetScale(const LLVector3& scale, BOOL damped)
{
	setAttributesOfAllSections();
}

BOOL LLVolumeImplFlexible::doUpdateGeometry(LLDrawable *drawable)
{
	BOOL compiled = FALSE;

	LLVOVolume *volume = (LLVOVolume*)mVO;

	if (volume->mDrawable.isNull()) // Not sure why this is happening, but it is...
	{
		return TRUE; // No update to complete
	}

	volume->calcAllTEsSame();

	if (volume->mVolumeChanged || volume->mFaceMappingChanged)
	{
		compiled = TRUE;
		volume->regenFaces();
	}
	else if (volume->mLODChanged)
	{
		LLPointer<LLVolume> old_volumep, new_volumep;
		F32 old_lod, new_lod;

		old_volumep = volume->getVolume();
		old_lod = old_volumep->getDetail();

 		LLVolumeParams volume_params = volume->getVolume()->getParams();
		volume->setVolume(volume_params, 0);
		doFlexibleUpdate();
		volume->getVolume()->regen();
		
		new_volumep = volume->getVolume();
		new_lod = new_volumep->getDetail();

		if (new_lod != old_lod)
		{
			compiled = TRUE;
			if (new_volumep->getNumFaces() != old_volumep->getNumFaces())
			{
				volume->regenFaces();
			}
		}
	}

	if (mUpdated)
	{
		compiled = TRUE;
		mUpdated = FALSE;
	}

	if(compiled)
	{
		volume->updateRelativeXform(isVolumeGlobal());
		volume->genTriangles(isVolumeGlobal());
		LLPipeline::sCompiles++;
	}
	
	volume->mVolumeChanged = FALSE;
	volume->mLODChanged = FALSE;
	volume->mFaceMappingChanged = FALSE;

	// clear UV flag
	drawable->clearState(LLDrawable::UV);

	drawable->movePartition();
	
	return TRUE;
}

//----------------------------------------------------------------------------------
void LLVolumeImplFlexible::setCollisionSphere( LLVector3 p, F32 r )
{
	mCollisionSpherePosition = p;
	mCollisionSphereRadius   = r;

}//------------------------------------------------------------------


//----------------------------------------------------------------------------------
void LLVolumeImplFlexible::setUsingCollisionSphere( bool u )
{
}//------------------------------------------------------------------


//----------------------------------------------------------------------------------
void LLVolumeImplFlexible::setRenderingCollisionSphere( bool r )
{
}//------------------------------------------------------------------

//------------------------------------------------------------------
LLVector3 LLVolumeImplFlexible::getEndPosition()
{
	S32 num_sections = 1 << mAttributes->getSimulateLOD();
	return mSection[ num_sections ].mPosition;

}//------------------------------------------------------------------


//------------------------------------------------------------------
LLVector3 LLVolumeImplFlexible::getNodePosition( int nodeIndex )
{
	S32 num_sections = 1 << mAttributes->getSimulateLOD();
	if ( nodeIndex > num_sections - 1 )
	{
		nodeIndex = num_sections - 1;
	}
	else if ( nodeIndex < 0 ) 
	{
		nodeIndex = 0;
	}

	return mSection[ nodeIndex ].mPosition;

}//------------------------------------------------------------------

LLVector3 LLVolumeImplFlexible::getPivotPosition() const
{
	return getAnchorPosition();
}

//------------------------------------------------------------------
LLVector3 LLVolumeImplFlexible::getAnchorPosition() const
{
	LLVector3 BasePosition = getFramePosition();
	LLQuaternion parentSegmentRotation = getFrameRotation();
	LLVector3 anchorDirectionRotated = LLVector3::z_axis * parentSegmentRotation;
	LLVector3 anchorScale = mVO->mDrawable->getScale();
	return BasePosition - (anchorScale.mV[VZ]/2 * anchorDirectionRotated);

}//------------------------------------------------------------------


//------------------------------------------------------------------
LLQuaternion LLVolumeImplFlexible::getEndRotation()
{
	return mLastSegmentRotation;

}//------------------------------------------------------------------


void LLVolumeImplFlexible::updateRelativeXform(BOOL global_volume)
{
	LLVOVolume* vo = (LLVOVolume*) mVO;
	
	LLVector3 delta_scale = LLVector3(1,1,1);
	LLVector3 delta_pos;
	LLQuaternion delta_rot;

	if (!mVO->mDrawable->isRoot())
	{	//global to parent relative
		LLViewerObject* parent = (LLViewerObject*) vo->getParent();
		delta_rot = ~parent->getRenderRotation();
		delta_pos = -parent->getRenderPosition()*delta_rot;
	}
	else
	{	//global to local
		delta_rot = ~getFrameRotation();
		delta_pos = -getFramePosition()*delta_rot;
	}
	
	// Vertex transform (4x4)
	LLVector3 x_axis = LLVector3(delta_scale.mV[VX], 0.f, 0.f) * delta_rot;
	LLVector3 y_axis = LLVector3(0.f, delta_scale.mV[VY], 0.f) * delta_rot;
	LLVector3 z_axis = LLVector3(0.f, 0.f, delta_scale.mV[VZ]) * delta_rot;

	vo->mRelativeXform.initRows(LLVector4(x_axis, 0.f),
								LLVector4(y_axis, 0.f),
								LLVector4(z_axis, 0.f),
								LLVector4(delta_pos, 1.f));
			
	x_axis.normVec();
	y_axis.normVec();
	z_axis.normVec();
	
	vo->mRelativeXformInvTrans.setRows(x_axis, y_axis, z_axis);

}

const LLMatrix4& LLVolumeImplFlexible::getWorldMatrix(LLXformMatrix* xform) const
{
	return xform->getWorldMatrix();
}
