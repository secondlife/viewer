/** 
 * @file llbreastmotion.cpp
 * @brief Implementation of LLBreastMotion class.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

// #define OUTPUT_BREAST_DATA

//-----------------------------------------------------------------------------
// LLBreastMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLBreastMotion::LLBreastMotion(const LLUUID &id) : 
	LLMotion(id),
	mCharacter(NULL),
	mFileWrite(NULL)
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



//-----------------------------------------------------------------------------
// ~LLBreastMotion()
// Class Destructor
//-----------------------------------------------------------------------------
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
	BOOL success = true;

	if ( !mChestState->setJoint( character->getJoint( "mChest" ) ) ) { success = false; }

	if (!success)
	{
		return STATUS_FAILURE;
	}
		
	mChestState->setUsage(LLJointState::ROT);
	addJointState( mChestState );

	// User-set params
	static const std::string breast_param_names_user[3] =
		{
			"Breast_Female_Cleavage_Driver",
			"",
			"Breast_Gravity_Driver"
		};

	// Params driven by this algorithm
	static const std::string breast_param_names_driven[3] =
		{
			"Breast_Female_Cleavage",
			"",
			"Breast_Gravity"
		};
		
	for (U32 i=0; i < 3; i++)
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

#ifdef OUTPUT_BREAST_DATA
	//if (mCharacter->getSex() == SEX_FEMALE)
	if (dynamic_cast<LLVOAvatarSelf *>(mCharacter))
	{
		mFileWrite = fopen("c:\\temp\\data.txt","w");
		if (mFileWrite != NULL)
		{
			fprintf(mFileWrite,"Pos\tParam\tNet\tVel\t\tAccel\tSpring\tDamp\n");
		}
	}
#endif

	mTimer.reset();
	return STATUS_SUCCESS;
}



F32 LLBreastMotion::calculateTimeDelta()
{
	const F32 time = mTimer.getElapsedTimeF32();
	const F32 time_delta = time - mLastTime;

	mLastTime = time;

	return time_delta;
}

LLVector3 LLBreastMotion::toLocal(const LLVector3 &world_vector)
{
	LLVector3 local_vec(0,0,0);

	LLJoint *chest_joint = mChestState->getJoint();
	const LLQuaternion world_rot = chest_joint->getWorldRotation();
		
	// -1 because cleavage param changes opposite to direction.
	LLVector3 breast_dir_world_vec = LLVector3(-1,0,0) * world_rot;
	breast_dir_world_vec.normalize();
	local_vec[0] = world_vector * breast_dir_world_vec;

	LLVector3 breast_up_dir_world_vec = LLVector3(0,0,1) * world_rot;
	breast_up_dir_world_vec.normalize();
	local_vec[2] = world_vector * breast_up_dir_world_vec;

	/*
	  {
	  llinfos << "Dir: " << breast_dir_world_vec << "V: " << world_vector << "DP: " << local_vec[0] << " time: " << llendl;
	  }
	*/

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

// called per time step
// must return TRUE while it is active, and
// must return FALSE when the motion is completed.
BOOL LLBreastMotion::onUpdate(F32 time, U8* joint_mask)
{
	if (!gSavedSettings.getBOOL("AvatarPhysics"))
	{
		return TRUE;
	}

	const F32 lod_factor = LLVOAvatar::sPhysicsLODFactor;
	if (lod_factor == 0)
	{
		return TRUE;
	}

	/* TEST:
	   1. Change outfits
	   2. FPS effect
	   3. Add disable
	   4. Disappearing chests
	   5. Overwrites breast params
	   6. Threshold for not setting param
	   7. Switch params or take off wearable makes breasts jump
	*/

	mBreastMassParam = mCharacter->getVisualParamWeight("Breast_Physics_Mass");
	mBreastSmoothingParam = (U32)(mCharacter->getVisualParamWeight("Breast_Physics_Smoothing"));
	mBreastGravityParam = mCharacter->getVisualParamWeight("Breast_Physics_Gravity");

	mBreastSpringParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Spring");
	mBreastGainParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Gain");
	mBreastDampingParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Damping");
	mBreastMaxVelocityParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Max_Velocity");
	mBreastDragParam[0] = mCharacter->getVisualParamWeight("Breast_Physics_Side_Drag");

	mBreastSpringParam[2] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Spring");
	mBreastGainParam[2] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Gain");
	mBreastDampingParam[2] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Damping");
	mBreastMaxVelocityParam[2] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Max_Velocity");
	mBreastDragParam[2] = mCharacter->getVisualParamWeight("Breast_Physics_UpDown_Drag");

	if (mCharacter->getSex() != SEX_FEMALE) return TRUE;
	const F32 time_delta = calculateTimeDelta();
	if (time_delta < .01 || time_delta > 10.0) return TRUE;

		
	LLVector3 breast_user_local_pt(0,0,0);
		
	for (U32 i=0; i < 3; i++)
	{
		if (mBreastParamsUser[i] != NULL)
		{
			breast_user_local_pt[i] = mBreastParamsUser[i]->getWeight();
		}
	}
		
	LLVector3 breast_current_local_pt = mBreastLastPosition_local_pt;
		
	const LLVector3 char_velocity_local_vec = calculateVelocity_local(time_delta);
	const LLVector3 char_acceleration_local_vec = calculateAcceleration_local(char_velocity_local_vec, time_delta);
	mCharLastVelocity_local_vec = char_velocity_local_vec;

	LLJoint *chest_joint = mChestState->getJoint();
	mCharLastPosition_world_pt = chest_joint->getWorldPosition();
		

	const LLVector3 spring_length_local = breast_current_local_pt-breast_user_local_pt;
	LLVector3 force_spring_local_vec = -spring_length_local; force_spring_local_vec *= mBreastSpringParam;

	LLVector3 force_accel_local_vec = char_acceleration_local_vec * mBreastMassParam;
	const LLVector3 force_gravity_local_vec = toLocal(LLVector3(0,0,1))* mBreastGravityParam * mBreastMassParam;
	force_accel_local_vec += force_gravity_local_vec;
	force_accel_local_vec[0] *= mBreastGainParam[0];
	force_accel_local_vec[1] *= mBreastGainParam[1];
	force_accel_local_vec[2] *= mBreastGainParam[2];

	LLVector3 force_damping_local_vec = -mBreastDampingParam; force_damping_local_vec *= mBreastVelocity_local_vec;

	LLVector3 force_drag_local_vec = .5*char_velocity_local_vec; // should square char_velocity_vec
	force_drag_local_vec[0] *= mBreastDragParam[0];
	force_drag_local_vec[1] *= mBreastDragParam[1];
	force_drag_local_vec[2] *= mBreastDragParam[2];

	LLVector3 force_net_local_vec = 
		force_accel_local_vec + 
		force_gravity_local_vec +
		force_spring_local_vec + 
		force_damping_local_vec + 
		force_drag_local_vec;


	LLVector3 acceleration_local_vec = force_net_local_vec / mBreastMassParam;
	mBreastVelocity_local_vec += acceleration_local_vec;
	mBreastVelocity_local_vec.clamp(-mBreastMaxVelocityParam*100.0, mBreastMaxVelocityParam*100.0);

	LLVector3 new_local_pt = breast_current_local_pt + mBreastVelocity_local_vec*time_delta;
	new_local_pt.clamp(mBreastParamsMin,mBreastParamsMax);
		
		
	for (U32 i=0; i < 3; i++)
	{
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

	if (mFileWrite != NULL)
	{
		fprintf(mFileWrite,"%f\t%f\t%f\t%f\t\t%f\t%f\t%f\t \t%f\t%f\t%f\t%f\t%f\t%f\n",
				mCharLastPosition_world_pt[2],
				breast_current_local_pt[2],
				acceleration_local_vec[2],
				mBreastVelocity_local_vec[2],
					
				force_accel_local_vec[2],
				force_spring_local_vec[2],
				force_damping_local_vec[2],
					
				force_accel_local_vec[2],
				force_damping_local_vec[2],
				force_drag_local_vec[2],
				force_net_local_vec[2],
				time_delta,
				mBreastMassParam
			);
	}
	
	LLVector3 position_diff = mBreastLastUpdatePosition_local_pt-new_local_pt;
	for (U32 i=0; i < 3; i++)
	{
		const F32 min_delta = (1.0-lod_factor)*(mBreastParamsMax[i]-mBreastParamsMin[i])/2.0;
		if (llabs(position_diff[i]) > min_delta)
		{
			mCharacter->updateVisualParams();
			mBreastLastUpdatePosition_local_pt = new_local_pt;
			break;
		}
	}

	mBreastLastPosition_local_pt = new_local_pt;

	return TRUE;
}
