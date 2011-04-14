/** 
 * @file llbreastmotion.cpp
 * @brief Implementation of LLBreastMotion class.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"
#include "linden_common.h"

#include "m3math.h"
#include "v3dmath.h"

#include "llbreastmotion.h"
#include "llcharacter.h"
#include "llviewercontrol.h"
#include "llviewervisualparam.h"
#include "llvoavatarself.h"

#define MIN_REQUIRED_PIXEL_AREA_BREAST_MOTION 0.f;

#define N_PARAMS 2

// User-set params
static const std::string breast_param_names_user[N_PARAMS] =
{
	"Breast_Female_Cleavage_Driver",
	"Breast_Gravity_Driver"
};

// Params driven by this algorithm
static const std::string breast_param_names_driven[N_PARAMS] =
{
	"Breast_Female_Cleavage",
	"Breast_Gravity"
};



LLBreastMotion::LLBreastMotion(const LLUUID &id) : 
	LLMotion(id),
	mCharacter(NULL)
{
	mName = "breast_motion";
	mChestState = new LLJointState;

	mBreastMassParam = (F32)1.0;
	mBreastDragParam = LLVector3((F32)0.1, (F32)0.1, (F32)0.1);
	mBreastSmoothingParam = (U32)2;
	mBreastGravityParam = (F32)0.0;

	mBreastSpringParam = LLVector3((F32)3.0, (F32)0.0, (F32)3.0);
	mBreastGainParam = LLVector3((F32)50.0, (F32)0.0, (F32)50.0);
	mBreastDampingParam = LLVector3((F32)0.3, (F32)0.0, (F32)0.3);
	mBreastMaxVelocityParam = LLVector3((F32)10.0, (F32)0.0, (F32)10.0);

	mBreastParamsUser[0] = mBreastParamsUser[1] = mBreastParamsUser[2] = NULL;
	mBreastParamsDriven[0] = mBreastParamsDriven[1] = mBreastParamsDriven[2] = NULL;

	mCharLastPosition_world_pt = LLVector3(0,0,0);
	mCharLastVelocity_local_vec = LLVector3(0,0,0);
	mCharLastAcceleration_local_vec = LLVector3(0,0,0);
	mBreastLastPosition_local_pt = LLVector3(0,0,0);
	mBreastLastUpdatePosition_local_pt = LLVector3(0,0,0);
	mBreastVelocity_local_vec = LLVector3(0,0,0);
}

LLBreastMotion::~LLBreastMotion()
{
}

BOOL LLBreastMotion::onActivate() 
{ 
	return TRUE; 
}

void LLBreastMotion::onDeactivate() 
{
}

LLMotion::LLMotionInitStatus LLBreastMotion::onInitialize(LLCharacter *character)
{
	mCharacter = character;

	if (!mChestState->setJoint(character->getJoint("mChest")))
	{
		return STATUS_FAILURE;
	}

	mChestState->setUsage(LLJointState::ROT);
	addJointState( mChestState );
	
	for (U32 i=0; i < N_PARAMS; i++)
	{
		mBreastParamsUser[i] = NULL;
		mBreastParamsDriven[i] = NULL;
		mBreastParamsMin[i] = 0;
		mBreastParamsMax[i] = 0;
		if (breast_param_names_user[i] != "" && breast_param_names_driven[i] != "")
		{
			mBreastParamsUser[i] = (LLViewerVisualParam*)mCharacter->getVisualParam(breast_param_names_user[i].c_str());
			mBreastParamsDriven[i] = (LLViewerVisualParam*)mCharacter->getVisualParam(breast_param_names_driven[i].c_str());
			if (mBreastParamsDriven[i])
			{
				mBreastParamsMin[i] = mBreastParamsDriven[i]->getMinWeight();
				mBreastParamsMax[i] = mBreastParamsDriven[i]->getMaxWeight();
			}
		}
	}
	
	mTimer.reset();
	return STATUS_SUCCESS;
}

F32 LLBreastMotion::getMinPixelArea() 
{
	return MIN_REQUIRED_PIXEL_AREA_BREAST_MOTION;
}
	

F32 LLBreastMotion::calculateTimeDelta()
{
	const F32 time = mTimer.getElapsedTimeF32();
	const F32 time_delta = time - mLastTime;
	mLastTime = time;
	return time_delta;
}

// Local space means "parameter space".
LLVector3 LLBreastMotion::toLocal(const LLVector3 &world_vector)
{
	LLVector3 local_vec(0,0,0);

	LLJoint *chest_joint = mChestState->getJoint();
	const LLQuaternion world_rot = chest_joint->getWorldRotation();
	
	// Cleavage
	LLVector3 breast_dir_world_vec = LLVector3(-1,0,0) * world_rot; // -1 b/c cleavage param changes opposite to direction
	breast_dir_world_vec.normalize();
	local_vec[0] = world_vector * breast_dir_world_vec;
	
	// Up-Down Bounce
	LLVector3 breast_up_dir_world_vec = LLVector3(0,0,1) * world_rot;
	breast_up_dir_world_vec.normalize();
	local_vec[1] = world_vector * breast_up_dir_world_vec;

	return local_vec;
}

LLVector3 LLBreastMotion::calculateVelocity_local(const F32 time_delta)
{
	LLJoint *chest_joint = mChestState->getJoint();
	const LLVector3 world_pos_pt = chest_joint->getWorldPosition();
	const LLQuaternion world_rot = chest_joint->getWorldRotation();
	const LLVector3 last_world_pos_pt = mCharLastPosition_world_pt;
	const LLVector3 char_velocity_world_vec = (world_pos_pt-last_world_pos_pt) / time_delta;
	const LLVector3 char_velocity_local_vec = toLocal(char_velocity_world_vec);

	return char_velocity_local_vec;
}

LLVector3 LLBreastMotion::calculateAcceleration_local(const LLVector3 &new_char_velocity_local_vec,
													  const F32 time_delta)
{
	LLVector3 char_acceleration_local_vec = new_char_velocity_local_vec - mCharLastVelocity_local_vec;
	
	char_acceleration_local_vec = 
		char_acceleration_local_vec * 1.0/mBreastSmoothingParam + 
		mCharLastAcceleration_local_vec * (mBreastSmoothingParam-1.0)/mBreastSmoothingParam;

	mCharLastAcceleration_local_vec = char_acceleration_local_vec;

	return char_acceleration_local_vec;
}

BOOL LLBreastMotion::onUpdate(F32 time, U8* joint_mask)
{
	// Skip if disabled globally.
	if (!gSavedSettings.getBOOL("AvatarPhysics"))
	{
		return TRUE;
	}

	// Higher LOD is better.  This controls the granularity
	// and frequency of updates for the motions.
	const F32 lod_factor = LLVOAvatar::sPhysicsLODFactor;
	if (lod_factor == 0)
	{
		return TRUE;
	}
	
	if (mCharacter->getSex() != SEX_FEMALE) return TRUE;
	const F32 time_delta = calculateTimeDelta();
	if (time_delta < .01 || time_delta > 10.0) return TRUE;


	////////////////////////////////////////////////////////////////////////////////
	// Get all parameters and settings
	//

	mBreastMassParam = mCharacter->getVisualParamWeight("Breast_Physics_Mass");
	mBreastSmoothingParam = (U32)(mCharacter->getVisualParamWeight("Breast_Physics_Smoothing"));
	mBreastGravityParam = mCharacter->getVisualParamWeight("Breast_Physics_Gravity");

	mBreastSpringParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Spring");
	mBreastGainParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Gain");
	mBreastDampingParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Damping");
	mBreastMaxVelocityParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Max_Velocity");
	mBreastDragParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Drag");

	mBreastSpringParam[1] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Spring");
	mBreastGainParam[1] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Gain");
	mBreastDampingParam[1] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Damping");
	mBreastMaxVelocityParam[1] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Max_Velocity");
	mBreastDragParam[1] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Drag");


	// Get the current morph parameters.
	LLVector3 breast_user_local_pt(0,0,0);
	for (U32 i=0; i < N_PARAMS; i++)
	{
		if (mBreastParamsUser[i] != NULL)
		{
			breast_user_local_pt[i] = mBreastParamsUser[i]->getWeight();
		}
	}
	
	LLVector3 breast_current_local_pt = mBreastLastPosition_local_pt;

	//
	// End parameters and settings
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Calculate velocity and acceleration in parameter space.
	//

	const LLVector3 char_velocity_local_vec = calculateVelocity_local(time_delta);
	const LLVector3 char_acceleration_local_vec = calculateAcceleration_local(char_velocity_local_vec, time_delta);
	mCharLastVelocity_local_vec = char_velocity_local_vec;

	LLJoint *chest_joint = mChestState->getJoint();
	mCharLastPosition_world_pt = chest_joint->getWorldPosition();

	//
	// End velocity and acceleration
	////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////
	// Calculate the total force 
	//

	// Spring force is a restoring force towards the original user-set breast position.
	// F = kx
	const LLVector3 spring_length_local = breast_current_local_pt-breast_user_local_pt;
	LLVector3 force_spring_local_vec = -spring_length_local; force_spring_local_vec *= mBreastSpringParam;

	// Acceleration is the force that comes from the change in velocity of the torso.
	// F = ma + mg
	LLVector3 force_accel_local_vec = char_acceleration_local_vec * mBreastMassParam;
	const LLVector3 force_gravity_local_vec = toLocal(LLVector3(0,0,1))* mBreastGravityParam * mBreastMassParam;
	force_accel_local_vec += force_gravity_local_vec;
	force_accel_local_vec *= mBreastGainParam;

	// Damping is a restoring force that opposes the current velocity.
	// F = -kv
	LLVector3 force_damping_local_vec = -mBreastDampingParam; 
	force_damping_local_vec *= mBreastVelocity_local_vec;
	
	// Drag is a force imparted by velocity, intuitively it is similar to wind resistance.
	// F = .5v*v
	LLVector3 force_drag_local_vec = .5*char_velocity_local_vec;
	force_drag_local_vec *= char_velocity_local_vec;
	force_drag_local_vec *= mBreastDragParam[0];

	LLVector3 force_net_local_vec = 
		force_accel_local_vec + 
		force_gravity_local_vec +
		force_spring_local_vec + 
		force_damping_local_vec + 
		force_drag_local_vec;

	//
	// End total force
	////////////////////////////////////////////////////////////////////////////////

	
	////////////////////////////////////////////////////////////////////////////////
	// Calculate new params
	//

	// Calculate the new acceleration based on the net force.
	// a = F/m
	LLVector3 acceleration_local_vec = force_net_local_vec / mBreastMassParam;
	mBreastVelocity_local_vec += acceleration_local_vec;
	mBreastVelocity_local_vec.clamp(-mBreastMaxVelocityParam*100.0, mBreastMaxVelocityParam*100.0);

	// Temporary debugging setting to cause all avatars to move, for profiling purposes.
	if (gSavedSettings.getBOOL("AvatarPhysicsTest"))
	{
		mBreastVelocity_local_vec[0] = sin(mTimer.getElapsedTimeF32()*4.0)*5.0;
		mBreastVelocity_local_vec[1] = sin(mTimer.getElapsedTimeF32()*3.0)*5.0;
	}
	// Calculate the new parameters and clamp them to the min/max ranges.
	LLVector3 new_local_pt = breast_current_local_pt + mBreastVelocity_local_vec*time_delta;
	new_local_pt.clamp(mBreastParamsMin,mBreastParamsMax);
		
	// Set the new parameters.
	for (U32 i=0; i < 3; i++)
	{
		// If the param is disabled, just set the param to the user value.
		if (mBreastMaxVelocityParam[i] == 0)
		{
			new_local_pt[i] = breast_user_local_pt[i];
		}
		if (mBreastParamsDriven[i])
		{
			mCharacter->setVisualParamWeight(mBreastParamsDriven[i],
											 new_local_pt[i],
											 FALSE);
		}
	}

	mBreastLastPosition_local_pt = new_local_pt;
	
	//
	// End calculate new params
	////////////////////////////////////////////////////////////////////////////////
	

	////////////////////////////////////////////////////////////////////////////////
	// Conditionally update the visual params
	//

	// Updating the visual params (i.e. what the user sees) is fairly expensive.
	// So only update if the params have changed enough, and also take into account
	// the graphics LOD settings.
	
	// For non-self, if the avatar is small enough visually, then don't update.
	const BOOL is_self = (dynamic_cast<LLVOAvatarSelf *>(this) != NULL);
	if (!is_self)
	{
		const F32 area_for_max_settings = 0.0;
		const F32 area_for_min_settings = 1400.0;

		const F32 area_for_this_setting = area_for_max_settings + (area_for_min_settings-area_for_max_settings)*(1.0-lod_factor);
		const F32 pixel_area = fsqrtf(mCharacter->getPixelArea());
		if (pixel_area < area_for_this_setting)
		{
			return TRUE;
		}
	}

	// If the parameter hasn't changed enough, then don't update.
	LLVector3 position_diff = mBreastLastUpdatePosition_local_pt-new_local_pt;
	for (U32 i=0; i < 3; i++)
	{
		const F32 min_delta = (1.0-lod_factor)*(mBreastParamsMax[i]-mBreastParamsMin[i])/2.0;
		if (llabs(position_diff[i]) > min_delta)
		{
			mCharacter->updateVisualParams();
			mBreastLastUpdatePosition_local_pt = new_local_pt;
			return TRUE;
		}
	}
	
	//
	// End update visual params
	////////////////////////////////////////////////////////////////////////////////

	return TRUE;
}
