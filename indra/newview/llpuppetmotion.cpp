/**
 * @file llpuppetmotion.cpp
 * @brief Implementation of LLPuppetMotion class.
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
#include "llviewerprecompiledheaders.h"

#include <set>
#include <limits>

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentdata.h"
#include "llcharacter.h"
#include "llpuppetevent.h"
#include "llpuppetmotion.h"
#include "llpuppetmodule.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "v3dmath.h"
#include "llsdutil_math.h"        //For ll_sd_from_vector3

#include "llvoavatarself.h"

#define ENABLE_RIGHT_CONSTRAINTS

// BEGIN_HACK : hard-coded joint_ids
//constexpr U16 PELVIS_ID = 0;
constexpr U16 TORSO_ID = 3;
constexpr U16 CHEST_ID = 6;
constexpr U16 NECK_ID = 7;
constexpr U16 HEAD_ID = 8;
constexpr S16 COLLAR_LEFT_ID = 58;
constexpr S16 SHOULDER_LEFT_ID = 59;
constexpr S16 ELBOW_LEFT_ID = 60;
constexpr S16 WRIST_LEFT_ID = 61;

constexpr S16 HAND_MIDDLE_LEFT_1_ID = 62;
constexpr S16 HAND_MIDDLE_LEFT_2_ID = 63;
constexpr S16 HAND_MIDDLE_LEFT_3_ID = 64;
constexpr S16 HAND_INDEX_LEFT_1_ID = 65;
constexpr S16 HAND_INDEX_LEFT_2_ID = 66;
constexpr S16 HAND_INDEX_LEFT_3_ID = 67;
constexpr S16 HAND_RING_LEFT_1_ID = 68;
constexpr S16 HAND_RING_LEFT_2_ID = 69;
constexpr S16 HAND_RING_LEFT_3_ID = 70;
constexpr S16 HAND_PINKY_LEFT_1_ID = 71;
constexpr S16 HAND_PINKY_LEFT_2_ID = 72;
constexpr S16 HAND_PINKY_LEFT_3_ID = 73;
constexpr S16 HAND_THUMB_LEFT_1_ID = 74;
constexpr S16 HAND_THUMB_LEFT_2_ID = 75;
constexpr S16 HAND_THUMB_LEFT_3_ID = 76;

#ifdef ENABLE_RIGHT_CONSTRAINTS
constexpr S16 COLLAR_RIGHT_ID = 77;
constexpr S16 SHOULDER_RIGHT_ID = 78;
constexpr S16 ELBOW_RIGHT_ID = 79;
constexpr S16 WRIST_RIGHT_ID = 80;
constexpr S16 HAND_MIDDLE_RIGHT_1_ID = 81;
constexpr S16 HAND_MIDDLE_RIGHT_2_ID = 82;
constexpr S16 HAND_MIDDLE_RIGHT_3_ID = 83;
constexpr S16 HAND_INDEX_RIGHT_1_ID = 84;
constexpr S16 HAND_INDEX_RIGHT_2_ID = 85;
constexpr S16 HAND_INDEX_RIGHT_3_ID = 86;
constexpr S16 HAND_RING_RIGHT_1_ID = 87;
constexpr S16 HAND_RING_RIGHT_2_ID = 88;
constexpr S16 HAND_RING_RIGHT_3_ID = 89;
constexpr S16 HAND_PINKY_RIGHT_1_ID = 90;
constexpr S16 HAND_PINKY_RIGHT_2_ID = 91;
constexpr S16 HAND_PINKY_RIGHT_3_ID = 92;
constexpr S16 HAND_THUMB_RIGHT_1_ID = 93;
constexpr S16 HAND_THUMB_RIGHT_2_ID = 94;
constexpr S16 HAND_THUMB_RIGHT_3_ID = 95;
#endif //ENABLE_RIGHT_CONSTRAINTS

// TODO: implement "allow incremental updates" policy
// TODO: figure out how to handle scale of local_pos changes

// HACK: the gConstraintFactory is a static global rather than a singleton
// because I couldn't figure out how to make the singleton compile. - Leviathan
LLIKConstraintFactory gConstraintFactory;

namespace
{
    //-----------------------------------------------------------------------------
    // Constants
    //-----------------------------------------------------------------------------
    constexpr U32 PUPPET_MAX_MSG_BYTES = 255;  // This is the largest possible size event
    constexpr F32 PUPPET_BROADCAST_INTERVAL = 0.05f;  //Time in seconds
    constexpr S32 POSED_JOINT_EXPIRY_PERIOD = 3 * MSEC_PER_SEC;
}

const std::string PUPPET_ROOT_JOINT_NAME("mPelvis");    //Name of the root joint

bool    LLPuppetMotion::sIsPuppetryEnabled(false);
size_t  LLPuppetMotion::sPuppeteerEventMaxSize(0);

// BEGIN HACK - generate hard-coded constraints for various joints
LLIK::Constraint::ptr_t get_constraint_by_joint_id(S16 joint_id)
{
    LLIK::Constraint::Info info;

    switch (joint_id)
    {
        case TORSO_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(LLVector3::z_axis); // forward_axis
            info.mFloats.push_back(0.005f * F_PI); // cone_angle
            info.mFloats.push_back(-0.005f * F_PI); // min_twist
            info.mFloats.push_back(0.005f * F_PI); // max_twist
        }
        break;
        case NECK_ID:
        // yes, fallthrough
        case HEAD_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(LLVector3::z_axis); // forward_axis
            info.mFloats.push_back(0.25f * F_PI); // cone_angle
            info.mFloats.push_back(-0.25f * F_PI); // min_twist
            info.mFloats.push_back(0.25f * F_PI); // max_twist
        }
        break;
        case CHEST_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(LLVector3::z_axis); // forward_axis
            info.mFloats.push_back(0.02f * F_PI); // cone_angle
            info.mFloats.push_back(-0.02f * F_PI); // min_twist
            info.mFloats.push_back(0.02f * F_PI); // max_twist
        }
        break;
        case COLLAR_LEFT_ID:
        {
            LLVector3 axis(-0.021f, 0.085f, 0.165f); // from avatar_skeleton.xml
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(axis); // forward_axis
            info.mFloats.push_back(F_PI * 0.05f); // cone_angle
            info.mFloats.push_back(-F_PI * 0.1f); // min_twist
            info.mFloats.push_back(F_PI * 0.1f); // max_twist
        }
        break;
        case SHOULDER_LEFT_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(LLVector3::y_axis); // forward_axis
            info.mFloats.push_back(F_PI * 1.0f / 2.0f ); // cone_angle
            info.mFloats.push_back(-F_PI * 2.0f / 5.0f ); // min_twist
            info.mFloats.push_back(F_PI * 4.0f / 7.0f ); // max_twist
        }
        break;
        case ELBOW_LEFT_ID:
        {
            info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
            info.mVectors.push_back(LLVector3::y_axis); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // pivot_axis
            info.mFloats.push_back(- F_PI * 7.0f / 8.0f); // min_bend
            info.mFloats.push_back(0.0f); // max_bend
            info.mFloats.push_back(-F_PI * 1.0f / 4.0f ); // min_twist
            info.mFloats.push_back(F_PI * 3.0f / 4.0f ); // max_twist
        }
        break;
        case WRIST_LEFT_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(LLVector3::y_axis); // forward_axis
            info.mFloats.push_back(F_PI * 1.0f / 4.0f ); // cone_angle
            info.mFloats.push_back(-0.05f); // min_twist
            info.mFloats.push_back(0.05f); // max_twist
        }
        break;
        // MIDDLE LEFT
        case HAND_MIDDLE_LEFT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.013f, 0.101f, 0.015f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(0.3f * F_PI); // max_pitch
        }
        break;
        case HAND_MIDDLE_LEFT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.001f, 0.040f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_MIDDLE_LEFT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.001f, 0.049f, -0.008f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // INDEX LEFT
        break;
        case HAND_INDEX_LEFT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.038f, 0.097f, 0.015f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_INDEX_LEFT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.017f, 0.036f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_INDEX_LEFT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.014f, 0.032f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // RING LEFT
        break;
        case HAND_RING_LEFT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.010f, 0.099f, 0.009f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_RING_LEFT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.013f, 0.038f, -0.008f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_RING_LEFT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.013f, 0.040f, -0.009f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // PINKY LEFT
        break;
        case HAND_PINKY_LEFT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.031f, 0.095f, 0.003f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_PINKY_LEFT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.024f, 0.025f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_PINKY_LEFT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.015f, 0.018f, -0.004f)); // forward_axis
            info.mVectors.push_back(LLVector3(-LLVector3::x_axis)); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // THUMB LEFT
        break;
        case HAND_THUMB_LEFT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.031f, 0.026f, 0.004f)); // forward_axis
            info.mVectors.push_back(LLVector3(1.0f, -1.0f, 1.0f)); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(-0.1f ); // min_pitch
            info.mFloats.push_back(F_PI / 4.0f ); // max_pitch
        }
        break;
        case HAND_THUMB_LEFT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.028f, 0.032f, -0.001f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis - LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.3f * F_PI); // max_bend
        }
        break;
        case HAND_THUMB_LEFT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.023f, 0.031f, -0.001f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis - LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        break;
#ifdef ENABLE_RIGHT_CONSTRAINTS
        case COLLAR_RIGHT_ID:
        {
            LLVector3 axis(-0.021f, -0.085f, 0.165f); // from avatar_skeleton.xml
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(axis);
            info.mFloats.push_back(F_PI * 0.05f); // cone_angle
            info.mFloats.push_back(-F_PI * 0.1f); // min_twist
            info.mFloats.push_back(F_PI * 0.1f); // max_twist
        }
        break;
        case SHOULDER_RIGHT_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(-LLVector3::y_axis); // forward_axis
            info.mFloats.push_back(F_PI * 1.0f / 2.0f ); // cone_angle
            info.mFloats.push_back(-F_PI * 2.0f / 5.0f ); // min_twist
            info.mFloats.push_back(F_PI * 4.0f / 7.0f ); // max_twist
        }
        break;
        case ELBOW_RIGHT_ID:
        {
            info.mType = LLIK::Constraint::Info::ELBOW_CONSTRAINT;
            info.mVectors.push_back(-LLVector3::y_axis); // forward_axis
            info.mVectors.push_back(-LLVector3::z_axis); // pivot_axis
            info.mFloats.push_back(- F_PI * 7.0f / 8.0f); // min_bend
            info.mFloats.push_back(0.0f); // max_bend
            info.mFloats.push_back(-F_PI * 1.0f / 4.0f ); // min_twist
            info.mFloats.push_back(F_PI * 3.0f / 4.0f ); // max_twist
        }
        break;
        case WRIST_RIGHT_ID:
        {
            info.mType = LLIK::Constraint::Info::TWIST_LIMITED_CONE_CONSTRAINT;
            info.mVectors.push_back(-LLVector3::y_axis); // forward_axis
            info.mFloats.push_back(F_PI * 1.0f / 4.0f ); // cone_angle
            info.mFloats.push_back(-0.05f); // min_twist
            info.mFloats.push_back(0.05f); // max_twist
        }
        break;
        // MIDDLE LEFT
        case HAND_MIDDLE_RIGHT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.013f, -0.101f, 0.015f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(0.3f * F_PI); // max_pitch
        }
        break;
        case HAND_MIDDLE_RIGHT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.001f, -0.040f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_MIDDLE_RIGHT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.001f, -0.049f, -0.008f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // INDEX LEFT
        break;
        case HAND_INDEX_RIGHT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.038f, -0.097f, 0.015f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_INDEX_RIGHT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.017f, -0.036f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_INDEX_RIGHT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.014f, -0.032f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // RING LEFT
        break;
        case HAND_RING_RIGHT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.010f, -0.099f, 0.009f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_RING_RIGHT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.013f, -0.038f, -0.008f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_RING_RIGHT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.013f, -0.040f, -0.009f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // PINKY LEFT
        break;
        case HAND_PINKY_RIGHT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.031f, -0.095f, 0.003f)); // forward_axis
            info.mVectors.push_back(LLVector3::z_axis); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(0.0f ); // min_pitch
            info.mFloats.push_back(F_PI * 4.0f / 9.0f ); // max_pitch
        }
        break;
        case HAND_PINKY_RIGHT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.024f, -0.025f, -0.006f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.5f * F_PI); // max_bend
        }
        break;
        case HAND_PINKY_RIGHT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(-0.015f, -0.018f, -0.004f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        // THUMB LEFT
        break;
        case HAND_THUMB_RIGHT_1_ID:
        {
            info.mType = LLIK::Constraint::Info::DOUBLE_LIMITED_HINGE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.031f, -0.026f, 0.004f)); // forward_axis
            info.mVectors.push_back(LLVector3(1.0f, 1.0f, 1.0f)); // up_axis
            info.mFloats.push_back(-0.05f * F_PI); // min_yaw
            info.mFloats.push_back(0.05f * F_PI); // max_yaw
            info.mFloats.push_back(-0.1f ); // min_pitch
            info.mFloats.push_back(F_PI / 4.0f ); // max_pitch
        }
        break;
        case HAND_THUMB_RIGHT_2_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.028f, -0.032f, -0.001f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.3f * F_PI); // max_bend
        }
        break;
        case HAND_THUMB_RIGHT_3_ID:
        {
            info.mType = LLIK::Constraint::Info::KNEE_CONSTRAINT;
            info.mVectors.push_back(LLVector3(0.023f, -0.031f, -0.001f)); // forward_axis
            info.mVectors.push_back(LLVector3::x_axis); // pivot_axis
            info.mFloats.push_back(0.0f); // min_bend
            info.mFloats.push_back(0.4f * F_PI); // max_bend
        }
        break;
#endif //ENABLE_RIGHT_CONSTRAINTS
        default:
            break;
    }
    // TODO: add DoubleLimitedHinge Constraints for fingers
    return gConstraintFactory.getConstraint(info);
}
// END HACK


//-----------------------------------------------------------------------------
// LLPuppetMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLPuppetMotion::LLPuppetMotion(const LLUUID &id) :
    LLMotion(id),
    mCharacter(nullptr)
{
    mName = "puppet_motion";
}

// override
bool LLPuppetMotion::needsUpdate() const
{
    return !mExpressionEvents.empty() || LLMotion::needsUpdate();
}

F32 LLPuppetMotion::getEaseInDuration()
{
    constexpr F32 PUPPETRY_EASE_IN_DURATION = 1.0;
    return PUPPETRY_EASE_IN_DURATION;
}

F32 LLPuppetMotion::getEaseOutDuration()
{
    constexpr F32 PUPPETRY_EASE_OUT_DURATION = 1.0;
    return PUPPETRY_EASE_OUT_DURATION;
}

LLJoint::JointPriority LLPuppetMotion::getPriority()
{
    // Note: LLMotion::getPriority() is only used to delegate motion-wide priority
    // to LLJointStates added to mPose in LLMotion::addJointState()...
    // when they have LLJoint::USE_MOTION_PRIORITY.
    return mMotionPriority;
}

//-----------------------------------------------------------------------------
// LLPuppetMotion::onInitialize(LLCharacter *character)
//-----------------------------------------------------------------------------
LLMotion::LLMotionInitStatus LLPuppetMotion::onInitialize(LLCharacter *character)
{
    if (mCharacter != character)
    {
        mCharacter = character;
        mJointStates.clear();

        LLJoint* joint = mCharacter->getJoint(PUPPET_ROOT_JOINT_NAME);
        S16 joint_id = joint->getJointNum();
        mIKSolver.setRootID(joint_id);

        collectJoints(joint);
        mIKSolver.addWristID(WRIST_LEFT_ID);
        mIKSolver.addWristID(WRIST_RIGHT_ID);

        // compute arms reach
        measureArmSpan();

        //Generate reference rotation
        mIKSolver.resetSkeleton();

        // HACK: whitelist of sub-bases: joints that have only child Chains
        // and should always be Chain endpoints, never in the middle of a Chain.
        std::set<S16> ids;
        ids.insert(CHEST_ID);
        mIKSolver.setSubBaseIds(ids);

        //// HACK: whitelist of sub-roots.
        //// This HACK prevents the spine from being included in the IK solution,
        //// effectively preventing the spine from moving.
        //ids.clear();
        //ids.insert(CHEST_ID);
        //mIKSolver.setSubRootIds(ids);
    }
    return STATUS_SUCCESS;
}

void LLPuppetMotion::setAvatar(LLVOAvatar* avatar)
{
    mAvatar = avatar;
    mIsSelf = mAvatar && mAvatar->getID() == gAgentID;
}

void LLPuppetMotion::clearAll()
{
    mJointStateExpiries.clear();
    mJointsToRemoveFromPose.clear();
    mIKSolver.resetSkeleton();
}

void LLPuppetMotion::addExpressionEvent(const LLPuppetJointEvent& event)
{
    // We used to collect these events in a map, keyed by joint_id,
    // but now we just collect them onto a vector and process them
    // FIFO later.
    mExpressionEvents.push_back(event);
}

void LLPuppetMotion::addJointToSkeletonData(LLSD& skeleton_sd, LLJoint* joint, const LLVector3& parent_rel_pos, const LLVector3& tip_rel_end_pos)
{
    LLSD bone_sd;
    S16 joint_id = joint->getJointNum();
    bone_sd["id"] = joint_id;
    if (joint->getParent() != nullptr)
    {
        bone_sd["parent_id"] = joint->getParent()->getJointNum();
        bone_sd["parent_relative_position"] = ll_sd_from_vector3(parent_rel_pos );
    }
    bone_sd["tip_relative_end_position"] = ll_sd_from_vector3(tip_rel_end_pos );
    if (joint->getNumChildren() > 0)
    {
        bone_sd["children"] = LLSD::emptyArray();
        for (LLJoint::joints_t::iterator itr = joint->mChildren.begin();
            itr != joint->mChildren.end(); ++itr)
        {
            if ((*itr)->isBone())
            {
                LLJoint* child = *itr;
                bone_sd["children"].append(child->getJointNum());
                break;
            }
        }
    }
    skeleton_sd[joint->getName()] = bone_sd;
}

LLSD LLPuppetMotion::getSkeletonData()
{
    LLSD skeleton_sd;
    for (auto& data_pair : mJointStates)
    {
        LLJoint* joint = data_pair.second->getJoint();
        LLVector3 local_pos_in_parent_frame = joint->getPosition().scaledVec(joint->getScale());
        LLVector3 bone_in_local_frame = joint->getEnd().scaledVec(joint->getScale());
        addJointToSkeletonData(skeleton_sd, joint, local_pos_in_parent_frame, bone_in_local_frame);
    }
    skeleton_sd["scale"] = mArmSpan;
    return skeleton_sd;
}

void LLPuppetMotion::updateSkeletonGeometry()
{
    for (auto& data_pair : mJointStates)
    {
        S16 joint_id = data_pair.first;
        LLIK::Constraint::ptr_t constraint = get_constraint_by_joint_id(joint_id);
        mIKSolver.resetJointGeometry(joint_id, constraint);
    }
    measureArmSpan();
}

void LLPuppetMotion::rememberPosedJoint(S16 joint_id, LLPointer<LLJointState> joint_state, Timestamp now)
{
    Timestamp expiry = now + POSED_JOINT_EXPIRY_PERIOD;
    auto itr = mJointStateExpiries.find(joint_id);
    if (itr == mJointStateExpiries.end())
    {
        // always bump remembered joints to HIGHEST_PRIORITY
        joint_state->setPriority(LLJoint::USE_MOTION_PRIORITY);
        mJointStateExpiries.insert({joint_id, JointStateExpiry(joint_state, expiry)});
        addJointState(joint_state);

        // TODO: avoid this scan by only removing joint if expired
        // check for and remove mentions of joint_state in mJointsToRemoveFromPose
        size_t i = 0;
        while (i < mJointsToRemoveFromPose.size())
        {
            if (mJointsToRemoveFromPose[i] == joint_state)
            {
                size_t last_index = mJointsToRemoveFromPose.size() - 1;
                if (i < last_index)
                {
                    mJointsToRemoveFromPose[i] = mJointsToRemoveFromPose[last_index];
                }
                mJointsToRemoveFromPose.pop_back();
            }
            else
            {
                ++i;
            }
        }
    }
    else
    {
        itr->second.mExpiry = expiry;
    }
    if (expiry < mNextJointStateExpiry)
    {
        mNextJointStateExpiry = expiry;
    }
}

void LLPuppetMotion::solveIKAndHarvestResults(const LLIK::Solver::joint_config_map_t& configs, Timestamp now)
{
    // Note: this only ever called when mIsSelf is true and configs not empty
    bool local_puppetry = !LLPuppetModule::instance().getEcho();
    if (local_puppetry)
    {
        // don't apply Puppetry when local agent is in mouselook
        auto camera_mode = gAgentCamera.getCameraMode();
        local_puppetry = camera_mode != CAMERA_MODE_MOUSELOOK && camera_mode != CAMERA_MODE_CUSTOMIZE_AVATAR;
    }

    if (!local_puppetry)
    {
        return;
    }

    bool config_changed = mIKSolver.updateJointConfigs(configs);
    if (config_changed)
    {
        mIKSolver.solve();
    }
    else
    {
        // ATM we still need to constantly re-send unchanged Puppetry data
        // so we DO NOT bail early here... yet.
        //return;
        // TODO: figure out how to send partial updates, and how to
        // explicitly clear joint settings in the Puppetry stream.
    }

    const LLIK::Solver::joint_list_t& active_joints = mIKSolver.getActiveJoints();
    for (auto joint: active_joints)
    {
        S16 id = joint->getID();
        U8 flags = joint->getHarvestFlags();
        if (local_puppetry)
        {
            LLPointer<LLJointState> joint_state = mJointStates[id];
            joint_state->setUsage(U32(flags & LLIK::MASK_JOINT_STATE_USAGE));
            if (flags & LLIK::CONFIG_FLAG_LOCAL_POS)
            {
                joint_state->setPosition(joint->getPreScaledLocalPos());
            }
            if (flags & LLIK::CONFIG_FLAG_LOCAL_ROT)
            {
                joint_state->setRotation(joint->getLocalRot());
            }
            if (flags & LLIK::CONFIG_FLAG_LOCAL_SCALE)
            {
                joint_state->setScale(joint->getLocalScale());
            }
            rememberPosedJoint(id, joint_state, now);
        }
    }
}

void LLPuppetMotion::applyEvent(const LLPuppetJointEvent& event, U64 now, LLIK::Solver::joint_config_map_t& configs)
{
    S16 joint_id = event.getJointID();
    state_map_t::iterator state_iter = mJointStates.find(joint_id);
    if (state_iter != mJointStates.end())
    {
        LLIK::Joint::Config config;
        bool something_changed = false;
        U8 mask = event.getMask();
        bool local = (bool)(mask & LLIK::CONFIG_FLAG_LOCAL_ROT);
        if (mask & LLIK::MASK_ROT)
        {
            if (local)
            {
                config.setLocalRot(event.getRotation());
            }
            else
            {
                config.setTargetRot(event.getRotation());
            }
            something_changed = true;
        }
        if (mask & LLIK::MASK_POS)
        {
            if (local)
            {
                config.setLocalPos(event.getPosition());
            }
            else
            {
                // don't forget to scale by 0.5*mArmSpan
                config.setTargetPos(event.getPosition() * (0.5f * mArmSpan));
            }
            something_changed = true;
        }
        if (mask & LLIK::CONFIG_FLAG_DISABLE_CONSTRAINT)
        {
            config.disableConstraint();
            something_changed = true;
        }
        if (something_changed)
        {
            configs[joint_id] = config;
        }
    }
}

void LLPuppetMotion::updateFromExpression(Timestamp now)
{
    // Note: if we get here mIsSelf must be true
    LLIK::Solver::joint_config_map_t configs;
    for(const auto& event: mExpressionEvents)
    {
        S16 joint_id = event.getJointID();
        state_map_t::iterator state_iter = mJointStates.find(joint_id);
        if (state_iter == mJointStates.end())
        {
            continue;
        }

        LLIK::Joint::Config config;
        bool something_changed = false;
        U8 mask = event.getMask();
        if (mask & LLIK::MASK_ROT)
        {
            if (mask & LLIK::CONFIG_FLAG_LOCAL_ROT)
            {
                config.setLocalRot(event.getRotation());
            }
            else
            {
                config.setTargetRot(event.getRotation());
            }
            something_changed = true;
        }
        if (mask & LLIK::MASK_POS)
        {
            if (mask & LLIK::CONFIG_FLAG_LOCAL_POS)
            {
                config.setLocalPos(event.getPosition());
            }
            else
            {
                // don't forget to scale by 0.5*mArmSpan
                config.setTargetPos(event.getPosition() * (0.5f * mArmSpan));
            }
            something_changed = true;
        }
        if (mask & LLIK::CONFIG_FLAG_DISABLE_CONSTRAINT)
        {
            config.disableConstraint();
            something_changed = true;
        }
        if (something_changed)
        {
            LLIK::Solver::joint_config_map_t::iterator itr = configs.find(joint_id);
            if (itr == configs.end())
            {
                configs.insert({joint_id, config});
            }
            else
            {
                itr->second.updateFrom(config);
            }
        }
    }
    mExpressionEvents.clear();
    if (!configs.empty())
    {
        solveIKAndHarvestResults(configs, now);
    }
}

void LLPuppetMotion::measureArmSpan()
{
    // "arm span" is twice the y-component of the longest arm
    F32 reach_left = mIKSolver.computeReach(CHEST_ID, WRIST_LEFT_ID).mV[VY];
    F32 reach_right = mIKSolver.computeReach(CHEST_ID, WRIST_RIGHT_ID).mV[VY];
    mArmSpan = 2.0f * std::max(std::abs(reach_left), std::abs(reach_right));

    // NOTE: we expect Puppetry data to be in the "normalized-frame" where
    // the arm-span is 2.0 units.
    // We will scale the inbound data by 0.5*mArmSpan.
}

BOOL LLPuppetMotion::onUpdate(F32 time, U8* joint_mask)
{
    if (!sIsPuppetryEnabled)
    {
        return FALSE;
    }

    if (!mAvatar)
    {
        return FALSE;
    }

    if (mJointStates.empty())
    {
        return FALSE;
    }

    // On each update we push mStopTimestamp into the future.
    // If the updates stop happening then this Motion will be stopped.
    constexpr F32 INACTIVITY_TIMEOUT = 2.0f; // seconds
    if (!mStopped)
    {
        mStopTimestamp = mActivationTimestamp + time + INACTIVITY_TIMEOUT;
    }

    Timestamp now = (S32)(LLFrameTimer::getElapsedSeconds() * MSEC_PER_SEC);
    if (mIsSelf)
    {
        // TODO: combine the two event maps into one vector of targets
        // LLIK::Solver::joint_config_map_t targets;
        if (!mExpressionEvents.empty())
        {
            updateFromExpression(now);
        }
    }

    if (mJointsToRemoveFromPose.size() > 0)
    {
        // TODO: remove only if actually expired (e.g. need to lookup in mJointStateExpiries)
        for (auto& joint_state : mJointsToRemoveFromPose)
        {
            if (joint_state)
            {
                joint_state->setUsage(0);
                mPose.removeJointState(joint_state);
            }
        }
        mJointsToRemoveFromPose.clear();
    }

    // expire joints that haven't been updated in a while
    if (now > mNextJointStateExpiry)
    {
        mNextJointStateExpiry = DISTANT_FUTURE_TIMESTAMP;
        auto itr = mJointStateExpiries.begin();
        while (itr != mJointStateExpiries.end())
        {
            JointStateExpiry& joint_state_expiry = itr->second;
            if (now > joint_state_expiry.mExpiry)
            {
                // instead of removing the joint from mPose during this onUpdate
                // we set its priority LOW and clear its local rotation which
                // will reset the avastar's joint...  iff no other animations
                // contribute to it.  We'll remove it from mPose next onUpdate
                joint_state_expiry.mState->setPriority(LLJoint::LOW_PRIORITY);
                mJointsToRemoveFromPose.push_back(joint_state_expiry.mState);
                itr = mJointStateExpiries.erase(itr);
            }
            else
            {
                if (joint_state_expiry.mExpiry < mNextJointStateExpiry)
                {
                    mNextJointStateExpiry = joint_state_expiry.mExpiry;
                }
                ++itr;
            }
        }
    }

    // We must return TRUE else LLMotionController will stop and purge this motion.
    //
    // TODO?: figure out when to return FALSE so that LLMotionController can
    // reduce its idle load.  Also will need to plumb LLPuppetModule to be able to
    // reintroduce this motion to the controller when puppetry restarts.
    return TRUE;
}

BOOL LLPuppetMotion::onActivate()
{
    // LLMotionController calls this when it adds this motion
    // to its active list.  As of 2022.04.21 the return value
    // is never checked.

    // Reset mStopTimestamp to zero to indicate it should run forever.
    // It will be pushed to a future non-zero value in onUpdate().
    mStopTimestamp = 0.0f;
    return TRUE;
}

void LLPuppetMotion::onDeactivate()
{
    // LLMotionController calls this when it removes
    // this motion from its active list.
    mPose.removeAllJointStates();
    mJointsToRemoveFromPose.clear();
    for (auto& data_pair : mJointStates)
    {
        data_pair.second->setUsage(0);
    }
    LLIK::Solver::joint_config_map_t empty_configs;
    mIKSolver.updateJointConfigs(empty_configs); // clear solver memory
}

void LLPuppetMotion::collectJoints(LLJoint* joint)
{
    //The PuppetMotion controller starts with the passed joint and recurses
    //into its children, collecting all the joints and putting them
    //under control of this motion controller.

    if (!joint->isBone())
    {
        return;
    }

    S16 parent_id = joint->getParent()->getJointNum();

    // BEGIN HACK: bypass mSpine joints
    //
    // mTorso    6
    //    |
    // mSpine4   5
    //    |
    // mSpine3   4
    //    |
    // mChest    3
    //    |
    // mSpine2   2
    //    |
    // mSpine1   1
    //    |
    // mPelvis   0
    //
    while (joint->getName().rfind("mSpine", 0) == 0)
    {
        for (LLJoint::joints_t::iterator itr = joint->mChildren.begin();
             itr != joint->mChildren.end(); ++itr)
        {
            if ((*itr)->isBone())
            {
                joint = *itr;
                break;
            }
        }
    }
    // END HACK

    LLPointer<LLJointState> joint_state = new LLJointState(joint);
    S16 joint_id = joint->getJointNum();
    mJointStates[joint_id] = joint_state;

    LLIK::Constraint::ptr_t constraint = get_constraint_by_joint_id(joint_id);
    mIKSolver.addJoint(joint_id, parent_id, joint, constraint);

    // Recurse through the children of this joint and add them to our joint control list
    for (LLJoint::joints_t::iterator itr = joint->mChildren.begin();
         itr != joint->mChildren.end(); ++itr)
    {
        collectJoints(*itr);
    }
}

//=============================================================================
void LLPuppetMotion::SetPuppetryEnabled(bool enabled, size_t event_size)
{
    sPuppeteerEventMaxSize = std::min(event_size, size_t(255));
    sIsPuppetryEnabled = enabled && (sPuppeteerEventMaxSize > 0);

    LL_INFOS("Puppet") << "Puppetry is " << (sIsPuppetryEnabled ? "ENABLED" : "DISABLED");
    if (sIsPuppetryEnabled)
        LL_CONT << " with event window of " << event_size << " bytes.";
    LL_CONT << LL_ENDL;
}

void LLPuppetMotion::RequestPuppetryStatus(LLViewerRegion *regionp)
{
    std::string cap;

    LLPuppetMotion::SetPuppetryEnabled(false, 0);    // turn off puppetry while we ask the simulator

    if (regionp)
        cap = regionp->getCapability("Puppetry");

    if (!cap.empty())
    {
        LLCoros::instance().launch("RequestPuppetryStatusCoro",
            [cap]() { LLPuppetMotion::RequestPuppetryStatusCoro(cap); });
    }
}

void LLPuppetMotion::RequestPuppetryStatusCoro(const std::string& capurl)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("RequestPuppetryStatusCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    S32 retry_count(0);
    LLSD result;
    do
    {
        result = httpAdapter->getAndSuspend(httpRequest, capurl, httpOpts, httpHeaders);

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (status.getType() == HTTP_NOT_FOUND)
        {   // There seems to be a case at first login where the simulator is slow getting all of the caps
            // connected for the agent.  It has given us back the cap URL but returns a 404 when we try and
            // hit it.  Pause, take a breath and give it another shot.
            if (++retry_count >= 3)
            {
                LL_WARNS("Puppet") << "Failed to get puppetry information." << LL_ENDL;
                return;
            }
            llcoro::suspendUntilTimeout(0.25f);
            continue;
        }
        else if (!status)
        {
            LL_WARNS("Puppet") << "Failed to get puppetry information." << LL_ENDL;
            return;
        }

        break;
    } while (true);

    size_t event_size = result["event_size"].asInteger();
    LLPuppetMotion::SetPuppetryEnabled(true, event_size);    // turn on puppetry and set the event size
    LLPuppetModule::instance().parsePuppetryResponse(result);

    if (gAgentAvatarp)
    {
        if (result.has("update_period"))
        {
            gAgentAvatarp->setAttachmentUpdatePeriod(result["update_period"].asReal());
        }
        else if (gAgent.getRegion()->getRegionFlag(REGION_FLAGS_ENABLE_ANIMATION_TRACKING))
        {
            gAgentAvatarp->setAttachmentUpdatePeriod(LLVOAvatarSelf::DEFAULT_ATTACHMENT_UPDATE_PERIOD);
        }
    }
}

