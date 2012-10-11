/** 
 * @file llphysicsmotion.cpp
 * @brief Implementation of LLPhysicsMotion class.
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

#include "llphysicsmotion.h"
#include "llagent.h"
#include "llcharacter.h"
#include "llviewercontrol.h"
#include "llviewervisualparam.h"
#include "llvoavatarself.h"

typedef std::map<std::string, std::string> controller_map_t;
typedef std::map<std::string, F32> default_controller_map_t;

#define MIN_REQUIRED_PIXEL_AREA_AVATAR_PHYSICS_MOTION 0.f
#define TIME_ITERATION_STEP 0.1f

inline F64 llsgn(const F64 a)
{
        if (a >= 0)
                return 1;
        return -1;
}

/* 
   At a high level, this works by setting temporary parameters that are not stored
   in the avatar's list of params, and are not conveyed to other users.  We accomplish
   this by creating some new temporary driven params inside avatar_lad that are then driven
   by the actual params that the user sees and sets.  For example, in the old system,
   the user sets a param called breast bouyancy, which controls the Z value of the breasts.
   In our new system, the user still sets the breast bouyancy, but that param is redefined
   as a driver param so that affects a new temporary driven param that the bounce is applied
   to.
*/

class LLPhysicsMotion
{
public:
	typedef enum
	{
		SMOOTHING = 0,
		MASS,
		GRAVITY,
		SPRING,
		GAIN,
		DAMPING,
		DRAG,
		MAX_EFFECT,
		NUM_PARAMS
	} eParamName;

        /*
          param_driver_name: The param that controls the params that are being affected by the physics.
          joint_name: The joint that the body part is attached to.  The joint is
          used to determine the orientation (rotation) of the body part.

          character: The avatar that this physics affects.

          motion_direction_vec: The direction (in world coordinates) that determines the
          motion.  For example, (0,0,1) is up-down, and means that up-down motion is what
          determines how this joint moves.

          controllers: The various settings (e.g. spring force, mass) that determine how
          the body part behaves.
        */
        LLPhysicsMotion(const std::string &param_driver_name, 
                        const std::string &joint_name,
                        LLCharacter *character,
                        const LLVector3 &motion_direction_vec,
                        const controller_map_t &controllers) :
                mParamDriverName(param_driver_name),
                mJointName(joint_name),
                mMotionDirectionVec(motion_direction_vec),
                mParamDriver(NULL),
                mParamControllers(controllers),
                mCharacter(character),
                mLastTime(0),
                mPosition_local(0),
                mVelocityJoint_local(0),
                mPositionLastUpdate_local(0)
        {
                mJointState = new LLJointState;

				for (U32 i = 0; i < NUM_PARAMS; ++i)
				{
					mParamCache[i] = NULL;
				}
        }

        BOOL initialize();

        ~LLPhysicsMotion() {}

        BOOL onUpdate(F32 time);

        LLPointer<LLJointState> getJointState() 
        {
                return mJointState;
        }
protected:

		F32 getParamValue(eParamName param)
		{
			static std::string controller_key[] = 
			{
				"Smoothing",
				"Mass",
				"Gravity",
				"Spring",
				"Gain",
				"Damping",
				"Drag",
				"MaxEffect"
			};

			if (!mParamCache[param])
			{
				const controller_map_t::const_iterator& entry = mParamControllers.find(controller_key[param]);
                if (entry == mParamControllers.end())
                {
                        return sDefaultController[controller_key[param]];
                }
                const std::string& param_name = (*entry).second.c_str();
                mParamCache[param] = mCharacter->getVisualParam(param_name.c_str());
			}
				
			if (mParamCache[param])
			{
				return mParamCache[param]->getWeight();
			}
			else
			{
				return sDefaultController[controller_key[param]];
			}
		}

        
        void setParamValue(const LLViewerVisualParam *param,
                           const F32 new_value_local,
                                                   F32 behavior_maxeffect);

        F32 toLocal(const LLVector3 &world);
        F32 calculateVelocity_local();
        F32 calculateAcceleration_local(F32 velocity_local);
private:
        const std::string mParamDriverName;
        const std::string mParamControllerName;
        const LLVector3 mMotionDirectionVec;
        const std::string mJointName;

        F32 mPosition_local;
        F32 mVelocityJoint_local; // How fast the joint is moving
        F32 mAccelerationJoint_local; // Acceleration on the joint

        F32 mVelocity_local; // How fast the param is moving
        F32 mPositionLastUpdate_local;
        LLVector3 mPosition_world;

        LLViewerVisualParam *mParamDriver;
        const controller_map_t mParamControllers;
        
        LLPointer<LLJointState> mJointState;
        LLCharacter *mCharacter;

        F32 mLastTime;
        
		LLVisualParam* mParamCache[NUM_PARAMS];

        static default_controller_map_t sDefaultController;
};

default_controller_map_t initDefaultController()
{
        default_controller_map_t controller;
        controller["Mass"] = 0.2f;
        controller["Gravity"] = 0.0f;
        controller["Damping"] = .05f;
        controller["Drag"] = 0.15f;
        controller["MaxEffect"] = 0.1f;
        controller["Spring"] = 0.1f;
        controller["Gain"] = 10.0f;
        return controller;
}

default_controller_map_t LLPhysicsMotion::sDefaultController = initDefaultController();

BOOL LLPhysicsMotion::initialize()
{
        if (!mJointState->setJoint(mCharacter->getJoint(mJointName.c_str())))
                return FALSE;
        mJointState->setUsage(LLJointState::ROT);

        mParamDriver = (LLViewerVisualParam*)mCharacter->getVisualParam(mParamDriverName.c_str());
        if (mParamDriver == NULL)
        {
                llinfos << "Failure reading in  [ " << mParamDriverName << " ]" << llendl;
                return FALSE;
        }

        return TRUE;
}

LLPhysicsMotionController::LLPhysicsMotionController(const LLUUID &id) : 
        LLMotion(id),
        mCharacter(NULL)
{
        mName = "breast_motion";
}

LLPhysicsMotionController::~LLPhysicsMotionController()
{
        for (motion_vec_t::iterator iter = mMotions.begin();
             iter != mMotions.end();
             ++iter)
        {
                delete (*iter);
        }
}

BOOL LLPhysicsMotionController::onActivate() 
{ 
        return TRUE; 
}

void LLPhysicsMotionController::onDeactivate() 
{
}

LLMotion::LLMotionInitStatus LLPhysicsMotionController::onInitialize(LLCharacter *character)
{
        mCharacter = character;

        mMotions.clear();

        // Breast Cleavage
        {
                controller_map_t controller;
                controller["Mass"] = "Breast_Physics_Mass";
                controller["Gravity"] = "Breast_Physics_Gravity";
                controller["Drag"] = "Breast_Physics_Drag";
                controller["Damping"] = "Breast_Physics_InOut_Damping";
                controller["MaxEffect"] = "Breast_Physics_InOut_Max_Effect";
                controller["Spring"] = "Breast_Physics_InOut_Spring";
                controller["Gain"] = "Breast_Physics_InOut_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Breast_Physics_InOut_Controller",
                                                                                                          "mChest",
                                                                                                          character,
                                                                                                          LLVector3(-1,0,0),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }

        // Breast Bounce
        {
                controller_map_t controller;
                controller["Mass"] = "Breast_Physics_Mass";
                controller["Gravity"] = "Breast_Physics_Gravity";
                controller["Drag"] = "Breast_Physics_Drag";
                controller["Damping"] = "Breast_Physics_UpDown_Damping";
                controller["MaxEffect"] = "Breast_Physics_UpDown_Max_Effect";
                controller["Spring"] = "Breast_Physics_UpDown_Spring";
                controller["Gain"] = "Breast_Physics_UpDown_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Breast_Physics_UpDown_Controller",
                                                                                                          "mChest",
                                                                                                          character,
                                                                                                          LLVector3(0,0,1),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }

        // Breast Sway
        {
                controller_map_t controller;
                controller["Mass"] = "Breast_Physics_Mass";
                controller["Gravity"] = "Breast_Physics_Gravity";
                controller["Drag"] = "Breast_Physics_Drag";
                controller["Damping"] = "Breast_Physics_LeftRight_Damping";
                controller["MaxEffect"] = "Breast_Physics_LeftRight_Max_Effect";
                controller["Spring"] = "Breast_Physics_LeftRight_Spring";
                controller["Gain"] = "Breast_Physics_LeftRight_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Breast_Physics_LeftRight_Controller",
                                                                                                          "mChest",
                                                                                                          character,
                                                                                                          LLVector3(0,-1,0),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }
        // Butt Bounce
        {
                controller_map_t controller;
                controller["Mass"] = "Butt_Physics_Mass";
                controller["Gravity"] = "Butt_Physics_Gravity";
                controller["Drag"] = "Butt_Physics_Drag";
                controller["Damping"] = "Butt_Physics_UpDown_Damping";
                controller["MaxEffect"] = "Butt_Physics_UpDown_Max_Effect";
                controller["Spring"] = "Butt_Physics_UpDown_Spring";
                controller["Gain"] = "Butt_Physics_UpDown_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Butt_Physics_UpDown_Controller",
                                                                                                          "mPelvis",
                                                                                                          character,
                                                                                                          LLVector3(0,0,-1),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }

        // Butt LeftRight
        {
                controller_map_t controller;
                controller["Mass"] = "Butt_Physics_Mass";
                controller["Gravity"] = "Butt_Physics_Gravity";
                controller["Drag"] = "Butt_Physics_Drag";
                controller["Damping"] = "Butt_Physics_LeftRight_Damping";
                controller["MaxEffect"] = "Butt_Physics_LeftRight_Max_Effect";
                controller["Spring"] = "Butt_Physics_LeftRight_Spring";
                controller["Gain"] = "Butt_Physics_LeftRight_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Butt_Physics_LeftRight_Controller",
                                                                                                          "mPelvis",
                                                                                                          character,
                                                                                                          LLVector3(0,-1,0),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }

        // Belly Bounce
        {
                controller_map_t controller;
                controller["Mass"] = "Belly_Physics_Mass";
                controller["Gravity"] = "Belly_Physics_Gravity";
                controller["Drag"] = "Belly_Physics_Drag";
                controller["Damping"] = "Belly_Physics_UpDown_Damping";
                controller["MaxEffect"] = "Belly_Physics_UpDown_Max_Effect";
                controller["Spring"] = "Belly_Physics_UpDown_Spring";
                controller["Gain"] = "Belly_Physics_UpDown_Gain";
                LLPhysicsMotion *motion = new LLPhysicsMotion("Belly_Physics_UpDown_Controller",
                                                                                                          "mPelvis",
                                                                                                          character,
                                                                                                          LLVector3(0,0,-1),
                                                                                                          controller);
                if (!motion->initialize())
                {
                        llassert_always(FALSE);
                        return STATUS_FAILURE;
                }
                addMotion(motion);
        }
        
        return STATUS_SUCCESS;
}

void LLPhysicsMotionController::addMotion(LLPhysicsMotion *motion)
{
        addJointState(motion->getJointState());
        mMotions.push_back(motion);
}

F32 LLPhysicsMotionController::getMinPixelArea() 
{
        return MIN_REQUIRED_PIXEL_AREA_AVATAR_PHYSICS_MOTION;
}

// Local space means "parameter space".
F32 LLPhysicsMotion::toLocal(const LLVector3 &world)
{
        LLJoint *joint = mJointState->getJoint();
        const LLQuaternion rotation_world = joint->getWorldRotation();
        
        LLVector3 dir_world = mMotionDirectionVec * rotation_world;
        dir_world.normalize();
        return world * dir_world;
}

F32 LLPhysicsMotion::calculateVelocity_local()
{
	const F32 world_to_model_scale = 100.0f;
	LLJoint *joint = mJointState->getJoint();
	const LLVector3 position_world = joint->getWorldPosition();
	const LLVector3 last_position_world = mPosition_world;
	const LLVector3 positionchange_world = (position_world-last_position_world) * world_to_model_scale;
	const LLVector3 velocity_world = positionchange_world;
	const F32 velocity_local = toLocal(velocity_world);
	return velocity_local;
}

F32 LLPhysicsMotion::calculateAcceleration_local(const F32 velocity_local)
{
//        const F32 smoothing = getParamValue("Smoothing");
        static const F32 smoothing = 3.0f; // Removed smoothing param since it's probably not necessary
        const F32 acceleration_local = velocity_local - mVelocityJoint_local;
        
        const F32 smoothed_acceleration_local = 
                acceleration_local * 1.0/smoothing + 
                mAccelerationJoint_local * (smoothing-1.0)/smoothing;
        
        return smoothed_acceleration_local;
}

BOOL LLPhysicsMotionController::onUpdate(F32 time, U8* joint_mask)
{
        // Skip if disabled globally.
        if (!gSavedSettings.getBOOL("AvatarPhysics"))
        {
                return TRUE;
        }
        
        BOOL update_visuals = FALSE;
        for (motion_vec_t::iterator iter = mMotions.begin();
             iter != mMotions.end();
             ++iter)
        {
                LLPhysicsMotion *motion = (*iter);
                update_visuals |= motion->onUpdate(time);
        }
                
        if (update_visuals)
                mCharacter->updateVisualParams();
        
        return TRUE;
}

// Return TRUE if character has to update visual params.
BOOL LLPhysicsMotion::onUpdate(F32 time)
{
        // static FILE *mFileWrite = fopen("c:\\temp\\avatar_data.txt","w");
        
        if (!mParamDriver)
                return FALSE;

        if (!mLastTime)
        {
                mLastTime = time;
                return FALSE;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Get all parameters and settings
        //

        const F32 time_delta = time - mLastTime;

	// Don't update too frequently, to avoid precision errors from small time slices.
	if (time_delta <= .01)
	{
		return FALSE;
	}
	
	// If less than 1FPS, we don't want to be spending time updating physics at all.
        if (time_delta > 1.0)
        {
                mLastTime = time;
                return FALSE;
        }

        // Higher LOD is better.  This controls the granularity
        // and frequency of updates for the motions.
        const F32 lod_factor = LLVOAvatar::sPhysicsLODFactor;
        if (lod_factor == 0)
        {
                return TRUE;
        }

        LLJoint *joint = mJointState->getJoint();

		const F32 behavior_mass = getParamValue(MASS);
		const F32 behavior_gravity = getParamValue(GRAVITY);
		const F32 behavior_spring = getParamValue(SPRING);
		const F32 behavior_gain = getParamValue(GAIN);
		const F32 behavior_damping = getParamValue(DAMPING);
		const F32 behavior_drag = getParamValue(DRAG);
		F32 behavior_maxeffect = getParamValue(MAX_EFFECT);
		
		const BOOL physics_test = FALSE; // Enable this to simulate bouncing on all parts.
        
        if (physics_test)
                behavior_maxeffect = 1.0f;

	// Normalize the param position to be from [0,1].
	// We have to use normalized values because there may be more than one driven param,
	// and each of these driven params may have its own range.
	// This means we'll do all our calculations in normalized [0,1] local coordinates.
	const F32 position_user_local = (mParamDriver->getWeight() - mParamDriver->getMinWeight()) / (mParamDriver->getMaxWeight() - mParamDriver->getMinWeight());
       	
	//
	// End parameters and settings
	////////////////////////////////////////////////////////////////////////////////
	
	
	////////////////////////////////////////////////////////////////////////////////
	// Calculate velocity and acceleration in parameter space.
	//
        
	//const F32 velocity_joint_local = calculateVelocity_local(time_iteration_step);
	const F32 velocity_joint_local = calculateVelocity_local();
	const F32 acceleration_joint_local = calculateAcceleration_local(velocity_joint_local);
	
	//
	// End velocity and acceleration
	////////////////////////////////////////////////////////////////////////////////
	
	BOOL update_visuals = FALSE;
	
	// Break up the physics into a bunch of iterations so that differing framerates will show
	// roughly the same behavior.
	for (F32 time_iteration = 0; time_iteration <= time_delta; time_iteration += TIME_ITERATION_STEP)
	{
		F32 time_iteration_step = TIME_ITERATION_STEP;
		if (time_iteration + TIME_ITERATION_STEP > time_delta)
		{
			time_iteration_step = time_delta-time_iteration;
		}
		
		// mPositon_local should be in normalized 0,1 range already.  Just making sure...
		const F32 position_current_local = llclamp(mPosition_local,
							   0.0f,
							   1.0f);
		// If the effect is turned off then don't process unless we need one more update
		// to set the position to the default (i.e. user) position.
		if ((behavior_maxeffect == 0) && (position_current_local == position_user_local))
		{
			return update_visuals;
		}

		////////////////////////////////////////////////////////////////////////////////
		// Calculate the total force 
		//

		// Spring force is a restoring force towards the original user-set breast position.
		// F = kx
		const F32 spring_length = position_current_local - position_user_local;
		const F32 force_spring = -spring_length * behavior_spring;

		// Acceleration is the force that comes from the change in velocity of the torso.
		// F = ma
		const F32 force_accel = behavior_gain * (acceleration_joint_local * behavior_mass);

		// Gravity always points downward in world space.
		// F = mg
		const LLVector3 gravity_world(0,0,1);
		const F32 force_gravity = (toLocal(gravity_world) * behavior_gravity * behavior_mass);
                
		// Damping is a restoring force that opposes the current velocity.
		// F = -kv
		const F32 force_damping = -behavior_damping * mVelocity_local;
                
		// Drag is a force imparted by velocity (intuitively it is similar to wind resistance)
		// F = .5kv^2
		const F32 force_drag = .5*behavior_drag*velocity_joint_local*velocity_joint_local*llsgn(velocity_joint_local);

		const F32 force_net = (force_accel + 
				       force_gravity +
				       force_spring + 
				       force_damping + 
				       force_drag);

		//
		// End total force
		////////////////////////////////////////////////////////////////////////////////

        
		////////////////////////////////////////////////////////////////////////////////
		// Calculate new params
		//

		// Calculate the new acceleration based on the net force.
		// a = F/m
		const F32 acceleration_new_local = force_net / behavior_mass;
		static const F32 max_velocity = 100.0f; // magic number, used to be customizable.
		F32 velocity_new_local = mVelocity_local + acceleration_new_local*time_iteration_step;
		velocity_new_local = llclamp(velocity_new_local, 
					     -max_velocity, max_velocity);
        
		// Temporary debugging setting to cause all avatars to move, for profiling purposes.
		if (physics_test)
		{
			velocity_new_local = sin(time*4.0);
		}
		// Calculate the new parameters, or remain unchanged if max speed is 0.
		F32 position_new_local = position_current_local + velocity_new_local*time_iteration_step;
		if (behavior_maxeffect == 0)
			position_new_local = position_user_local;

		// Zero out the velocity if the param is being pushed beyond its limits.
		if ((position_new_local < 0 && velocity_new_local < 0) || 
		    (position_new_local > 1 && velocity_new_local > 0))
		{
			velocity_new_local = 0;
		}
	
		// Check for NaN values.  A NaN value is detected if the variables doesn't equal itself.  
		// If NaN, then reset everything.
		if ((mPosition_local != mPosition_local) ||
		    (mVelocity_local != mVelocity_local) ||
		    (position_new_local != position_new_local))
		{
			position_new_local = 0;
			mVelocity_local = 0;
			mVelocityJoint_local = 0;
			mAccelerationJoint_local = 0;
			mPosition_local = 0;
			mPosition_world = LLVector3(0,0,0);
		}

		const F32 position_new_local_clamped = llclamp(position_new_local,
							       0.0f,
							       1.0f);

		LLDriverParam *driver_param = dynamic_cast<LLDriverParam *>(mParamDriver);
		llassert_always(driver_param);
		if (driver_param)
		{
			// If this is one of our "hidden" driver params, then make sure it's
			// the default value.
			if ((driver_param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) &&
			    (driver_param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT))
			{
				mCharacter->setVisualParamWeight(driver_param,
								 0,
								 FALSE);
			}
			S32 num_driven = driver_param->getDrivenParamsCount();
			for (S32 i = 0; i < num_driven; ++i)
			{
				const LLViewerVisualParam *driven_param = driver_param->getDrivenParam(i);
				setParamValue(driven_param,position_new_local_clamped, behavior_maxeffect);
			}
		}
        
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
		const F32 area_for_max_settings = 0.0;
		const F32 area_for_min_settings = 1400.0;
		const F32 area_for_this_setting = area_for_max_settings + (area_for_min_settings-area_for_max_settings)*(1.0-lod_factor);
	        const F32 pixel_area = sqrtf(mCharacter->getPixelArea());
        
		const BOOL is_self = (dynamic_cast<LLVOAvatarSelf *>(mCharacter) != NULL);
		if ((pixel_area > area_for_this_setting) || is_self)
		{
			const F32 position_diff_local = llabs(mPositionLastUpdate_local-position_new_local_clamped);
			const F32 min_delta = (1.0001f-lod_factor)*0.4f;
			if (llabs(position_diff_local) > min_delta)
			{
				update_visuals = TRUE;
				mPositionLastUpdate_local = position_new_local;
			}
		}

		//
		// End update visual params
		////////////////////////////////////////////////////////////////////////////////

		mVelocity_local = velocity_new_local;
		mAccelerationJoint_local = acceleration_joint_local;
		mPosition_local = position_new_local;
	}
	mLastTime = time;
	mPosition_world = joint->getWorldPosition();
	mVelocityJoint_local = velocity_joint_local;


        /*
          // Write out debugging info into a spreadsheet.
          if (mFileWrite != NULL && is_self)
          {
          fprintf(mFileWrite,"%f\t%f\t%f \t\t%f \t\t%f\t%f\t%f\t \t\t%f\t%f\t%f\t%f\t%f \t\t%f\t%f\t%f\n",
          position_new_local,
          velocity_new_local,
          acceleration_new_local,

          time_delta,

          mPosition_world[0],
          mPosition_world[1],
          mPosition_world[2],

          force_net,
          force_spring,
          force_accel,
          force_damping,
          force_drag,

          spring_length,
          velocity_joint_local,
          acceleration_joint_local
          );
          }
        */

        return update_visuals;
}

// Range of new_value_local is assumed to be [0 , 1] normalized.
void LLPhysicsMotion::setParamValue(const LLViewerVisualParam *param,
                                    F32 new_value_normalized,
				    F32 behavior_maxeffect)
{
        const F32 value_min_local = param->getMinWeight();
        const F32 value_max_local = param->getMaxWeight();
        const F32 min_val = 0.5f-behavior_maxeffect/2.0;
        const F32 max_val = 0.5f+behavior_maxeffect/2.0;

	// Scale from [0,1] to [min_val,max_val]
	const F32 new_value_rescaled = min_val + (max_val-min_val) * new_value_normalized;
	
	// Scale from [0,1] to [value_min_local,value_max_local]
        const F32 new_value_local = value_min_local + (value_max_local-value_min_local) * new_value_rescaled;

        mCharacter->setVisualParamWeight(param,
                                         new_value_local,
                                         FALSE);
}
