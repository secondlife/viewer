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


void LLPuppetMotion::DelayedEventQueue::addEvent(
        Timestamp remote_timestamp,
        Timestamp local_timestamp,
        const LLPuppetJointEvent& event)
{
    if (mLastRemoteTimestamp != -1)
    {
        // dynamically measure mEventPeriod and mEventJitter
        constexpr F32 DEL = 0.1f;
        Timestamp this_period = remote_timestamp - mLastRemoteTimestamp;
        mEventJitter = (1.0f - DEL) * mEventJitter + DEL * std::abs(mEventPeriod - (F32)this_period);

        // mEventPeriod is a running average of the period between events
        mEventPeriod = (1.0f - DEL) * mEventPeriod + DEL * this_period;
    }
    mLastRemoteTimestamp = remote_timestamp;

    // we push event into the future so we have something to interpolate toward
    // while we wait for the next
    Timestamp delayed_timestamp = local_timestamp + (Timestamp)(mEventPeriod + mEventJitter);
    mQueue.push_back({delayed_timestamp, event});
}

//-----------------------------------------------------------------------------
// LLPuppetMotion()
// Class Constructor
//-----------------------------------------------------------------------------
LLPuppetMotion::LLPuppetMotion(const LLUUID &id) :
    LLMotion(id),
    mCharacter(nullptr)
{
    mName = "puppet_motion";
    mBroadcastTimer.resetWithExpiry(PUPPET_BROADCAST_INTERVAL);
    mRemoteToLocalClockOffset = std::numeric_limits<F32>::min();
}

// override
bool LLPuppetMotion::needsUpdate() const
{
    return !mExpressionEvents.empty() || !mEventQueues.empty() || LLMotion::needsUpdate();
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
    //mJointStates.clear();   This kills puppetry - need a correct way to reset joint data when module restarts
    mEventQueues.clear();
    mOutgoingEvents.clear();
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
    for (state_map_t::iterator iter=mJointStates.begin(); iter!=mJointStates.end(); ++iter)
    {
        LLPointer<LLJointState> joint_state = iter->second;
        LLJoint* joint = joint_state->getJoint();
        LLVector3 local_pos_in_parent_frame = joint->getPosition().scaledVec(joint->getScale());
        LLVector3 bone_in_local_frame = joint->getEnd().scaledVec(joint->getScale());
        addJointToSkeletonData(skeleton_sd, joint, local_pos_in_parent_frame, bone_in_local_frame);
    }
    skeleton_sd["scale"] = mArmSpan;
    return skeleton_sd;
}

void LLPuppetMotion::reconfigureJoints()
{
    LLSD skeleton_sd;
    for (state_map_t::iterator iter=mJointStates.begin(); iter!=mJointStates.end(); ++iter)
    {
        S16 joint_id = iter->first;
        LLPointer<LLJointState> joint_state = iter->second;
        LLJoint* joint = joint_state->getJoint();
        LLVector3 local_pos_in_parent_frame = joint->getPosition().scaledVec(joint->getScale());
        LLVector3 bone_in_local_frame = joint->getEnd().scaledVec(joint->getScale());
        LLIK::Constraint::ptr_t constraint = get_constraint_by_joint_id(joint_id);
        mIKSolver.reconfigureJoint(joint_id, local_pos_in_parent_frame, bone_in_local_frame, constraint);
        addJointToSkeletonData(skeleton_sd, joint, local_pos_in_parent_frame, bone_in_local_frame);
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
    bool local_puppetry = !LLPuppetModule::instance().getEcho();
    if (mIsSelf && local_puppetry)
    {
        // don't apply Puppetry when local agent is in mouselook
        auto camera_mode = gAgentCamera.getCameraMode();
        local_puppetry = camera_mode != CAMERA_MODE_MOUSELOOK && camera_mode != CAMERA_MODE_CUSTOMIZE_AVATAR;
    }

	bool remote_puppetry = LLPuppetModule::instance().isSending();
    if (!local_puppetry && !remote_puppetry)
    {
        return;
    }

    bool something_changed = mIKSolver.configureAndSolve(configs);

	LLPuppetEvent broadcast_event;
    const LLIK::Solver::joint_list_t& active_joints = mIKSolver.getActiveJoints();
    LLVector3 pos;
    LLQuaternion rot;
    for (auto joint: active_joints)
    {
        S16 id = joint->getID();
        U8 flags = joint->getConfigFlags();
        pos = joint->getLocalPos();
        rot = joint->getLocalRot();
        if (local_puppetry)
        {
            LLPointer<LLJointState> joint_state = mJointStates[id];
            if (flags | LLIK::FLAG_LOCAL_POS)
            {
                joint_state->setPosition(pos);
            }
            if (flags | LLIK::FLAG_LOCAL_ROT)
            {
                joint_state->setRotation(rot);
            }
            rememberPosedJoint(id, joint_state, now);
        }
        if (remote_puppetry)
        {
			LLPuppetJointEvent joint_event;
			joint_event.setJointID(id);
            joint_event.setReferenceFrame(LLPuppetJointEvent::PARENT_FRAME);
            if (flags | LLIK::FLAG_LOCAL_POS)
            {
                joint_event.setPosition(pos);
            }
            if (flags | LLIK::FLAG_LOCAL_ROT)
            {
                joint_event.setRotation(rot);
            }
            if (flags | LLIK::FLAG_DISABLE_CONSTRAINT)
            {
                joint_event.disableConstraint();
            }
			broadcast_event.addJointEvent(joint_event);
        }
    }

    if (remote_puppetry)
    {
	    broadcast_event.updateTimestamp();
		queueOutgoingEvent(broadcast_event);
    }

	if (something_changed &&
        LLPuppetModule::instance().isSending())
	{
		queueOutgoingEvent(broadcast_event);
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
        bool local = (bool)(mask & LLIK::FLAG_LOCAL_ROT);
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
            // don't forget to scale by 0.5*mArmSpan
            if (local)
            {
                // TODO: figure out what to do about scale of local positions
                config.setLocalPos(event.getPosition());
            }
            else
            {
                config.setTargetPos(event.getPosition() * 0.5f * mArmSpan);
            }
            something_changed = true;
        }
        if (mask & LLIK::FLAG_DISABLE_CONSTRAINT)
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
            if (mask & LLIK::FLAG_LOCAL_ROT)
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
            // don't forget to scale by 0.5*mArmSpan
            if (mask & LLIK::FLAG_LOCAL_POS)
            {
                // TODO: figure out what to do about scale of local positions
                config.setLocalPos(event.getPosition());
            }
            else
            {
                config.setTargetPos(event.getPosition() * 0.5f * mArmSpan);
            }
            something_changed = true;
        }
        if (mask & LLIK::FLAG_DISABLE_CONSTRAINT)
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

void LLPuppetMotion::applyBroadcastEvent(const LLPuppetJointEvent& event, Timestamp now, bool local_puppetry)
{
    if (mIsSelf && local_puppetry)
    {
        return;
    }

    S16 joint_id = event.getJointID();
    state_map_t::iterator state_iter = mJointStates.find(joint_id);
    if (state_iter != mJointStates.end())
    {
        U8 mask = event.getMask();
        llassert((mask & LLIK::MASK_TARGET) == 0); // broadcast event always in parent-frame
        if (mask & LLIK::MASK_ROT)
        {
            state_iter->second->setRotation(event.getRotation());
        }
        if (mask & LLIK::MASK_POS)
        {
            // Note: scale already set by broadcaster (e.g. 1.0 = 0.5*mArmSpan)
            state_iter->second->setPosition(event.getPosition());
        }
        rememberPosedJoint(joint_id, state_iter->second, now);
    }
}

void LLPuppetMotion::updateFromBroadcast(Timestamp now)
{
    if (!LLPuppetModule::instance().isReceiving())
    {   // If receiving is disabled, just drop all the data and don't apply it
        mEventQueues.clear();
        return;
    }

    bool local_puppetry = !LLPuppetModule::instance().getEcho();

    // We walk the queue looking for the two bounding events: the last previous
    // and the next pending: we will interpolate between them.  If we don't
    // find bounding events we'll use whatever we've got.
    auto queue_itr = mEventQueues.begin();
    while (queue_itr != mEventQueues.end())
    {
        auto& queue = queue_itr->second.getEventQueue();
        auto event_itr = queue.begin();
        while (event_itr != queue.end())
        {
            Timestamp timestamp = event_itr->first;
            const LLPuppetJointEvent& event = event_itr->second;
            if (timestamp > now)
            {
                // first available event is in the future
                // we have no choice but to apply what we have
                applyBroadcastEvent(event, now, local_puppetry);
                break;
            }

            // event is in the past --> check next event
            ++event_itr;
            if (event_itr == queue.end())
            {
                // we're at the end of the queue
                constexpr Timestamp STALE_QUEUE_DURATION = 3 * MSEC_PER_SEC;
                if (timestamp < now - STALE_QUEUE_DURATION)
                {
                    // this queue is stale
                    // the "remembered pose" will be purged elewhere
                    queue.clear();
                }
                else
                {
                    // presumeably we already interpolated close to this event
                    // but just in case we didn't quite reach it yet: apply
                    applyBroadcastEvent(event, now, local_puppetry);
                }
                break;
            }

            Timestamp next_timestamp = event_itr->first;
            if (next_timestamp < now)
            {
                // event is stale --> drop it
                queue.pop_front();
                event_itr = queue.begin();
                continue;
            }

            // next event is in the future
            // which means we've found the two events that straddle 'now'
            // --> create an interpolated event and apply that
            F32 del = (F32)(now - timestamp) / (F32)(next_timestamp - timestamp);
            LLPuppetJointEvent interpolated_event;
            const LLPuppetJointEvent& next_event = event_itr->second;
            interpolated_event.interpolate(del, event, next_event);
            applyBroadcastEvent(interpolated_event, now, local_puppetry);
            break;
        }
        if (queue.empty())
        {
            queue_itr = mEventQueues.erase(queue_itr);
        }
        else
        {
            ++queue_itr;
        }
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

void LLPuppetMotion::queueEvent(const LLPuppetEvent& puppet_event)
{
    // adjust the timestamp for local clock
    // and push into the future to allow interpolation
    Timestamp remote_timestamp = puppet_event.getTimestamp();
    Timestamp now = (S32)(LLFrameTimer::getElapsedSeconds() * MSEC_PER_SEC);
    Timestamp clock_skew = now - remote_timestamp;
    if (std::numeric_limits<F32>::min() == mRemoteToLocalClockOffset)
    {
        mRemoteToLocalClockOffset = (F32)(clock_skew);
    }
    else
    {
        // compute a running average
        constexpr F32 DEL = 0.05f;
        mRemoteToLocalClockOffset = (1.0 - DEL) * mRemoteToLocalClockOffset + DEL * clock_skew;
    }
    Timestamp local_timestamp = remote_timestamp + (Timestamp)(mRemoteToLocalClockOffset);

    // split puppet_event into joint-specific streams
    for (const auto& joint_event : puppet_event.mJointEvents)
    {
        S16 joint_id = joint_event.getJointID();
        state_map_t::iterator state_iter = mJointStates.find(joint_id);
        if (state_iter == mJointStates.end())
        {
            // ignore this unknown joint_id
            continue;
        }
        DelayedEventQueue& queue = mEventQueues[joint_id];
        queue.addEvent(remote_timestamp, local_timestamp, joint_event);
    }
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
            pumpOutgoingEvents();
        }

        if (LLPuppetModule::instance().getEcho())
        {   // Check for updates from server if we're echoing from there
            updateFromBroadcast(now);
        }
    }
    else
    {   // Some other agent - just update from any incoming data
        updateFromBroadcast(now);
    }

    if (mJointsToRemoveFromPose.size() > 0)
    {
        for (auto& joint_ptr : mJointsToRemoveFromPose)
        {
            if (joint_ptr)
            {
                mPose.removeJointState(joint_ptr);
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
                joint_state_expiry.mState->setRotation(LLQuaternion::DEFAULT);
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

    LLPointer<LLJointState> joint_state = new LLJointState;
    joint_state->setJoint(joint);
    joint_state->setUsage(LLJointState::ROT | LLJointState::POS);
    S16 joint_id = joint->getJointNum();
    mJointStates[joint_id] = joint_state;
    LLVector3 local_pos_in_parent_frame = joint->getPosition().scaledVec(joint->getScale());
    LLVector3 bone_in_local_frame = joint->getEnd().scaledVec(joint->getScale());
    LLIK::Constraint::ptr_t constraint = get_constraint_by_joint_id(joint_id);
    mIKSolver.addJoint(joint_id, parent_id, local_pos_in_parent_frame, bone_in_local_frame, constraint);

    // Recurse through the children of this joint and add them to our joint control list
    for (LLJoint::joints_t::iterator itr = joint->mChildren.begin();
         itr != joint->mChildren.end(); ++itr)
    {
        collectJoints(*itr);
    }
}

void LLPuppetMotion::pumpOutgoingEvents()
{
    if (!mBroadcastTimer.hasExpired())
    {
        return;
    }
    packEvents();
    mBroadcastTimer.resetWithExpiry(PUPPET_BROADCAST_INTERVAL);
}

void LLPuppetMotion::queueOutgoingEvent(const LLPuppetEvent& event)
{
    mOutgoingEvents.push_back(event);
}


//-----------------------------------------------------------------------------
// packEvents()
// - packs as many events from Outgoing event queue as possible into the network data pipe
//-----------------------------------------------------------------------------
void    LLPuppetMotion::packEvents()
{
    if (mOutgoingEvents.empty())
    {
        return;
    }

    if (!LLPuppetMotion::GetPuppetryEnabled() || (LLPuppetMotion::sPuppeteerEventMaxSize < 30))
    {
        LL_WARNS_ONCE("Puppet") << "Puppetry enabled=" << LLPuppetMotion::GetPuppetryEnabled()
            << " event_window=" << LLPuppetMotion::sPuppeteerEventMaxSize << LL_ENDL;
        mOutgoingEvents.clear();
        return;
    }

    std::array<U8, PUPPET_MAX_MSG_BYTES> puppet_pack_buffer;

    LLDataPackerBinaryBuffer dataPacker(puppet_pack_buffer.data(), LLPuppetMotion::sPuppeteerEventMaxSize);

    //Send the agent and session information.
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_AgentAnimation);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

    S32 msgblock_count(0);
    S32 joint_count(0);

    auto event = mOutgoingEvents.begin();
    while(event != mOutgoingEvents.end())
    {
        dataPacker.reset();

        // while the datapacker can fit at least some of the current event in the buffer...
        while ((event != mOutgoingEvents.end()) &&
            ((dataPacker.getCurrentSize() + event->getMinEventSize()) < dataPacker.getBufferSize()))
        {
            S32 packed_joints(0);
            bool all_done = event->pack(dataPacker, packed_joints);
            joint_count += packed_joints;
            msgblock_count++;
            if (!all_done)
            {   // pack was not able to fit everything into this buffer
                // it's full so time to send it.
                break;
            }
            ++event;
        }

        // if datapacker has some data, we should put it into the message and perhaps send it.
        if (dataPacker.getCurrentSize() > 0)
        {
            if ((msg->getCurrentSendTotal() + dataPacker.getCurrentSize() + 16) >= MTUBYTES)
            {   // send the old message and get a new one ready.
                LL_DEBUGS("PUPPET_SPAM") << "Message would overflow MTU, sending message with " << msgblock_count << " blocks and "
                        << joint_count << " joints in frame " << (S32) gFrameCount << LL_ENDL;
                joint_count = 0;

                gAgent.sendMessage();

                // Create the next message header
                msgblock_count = 0;
                msg->newMessageFast(_PREHASH_AgentAnimation);
                msg->nextBlockFast(_PREHASH_AgentData);
                msg->addUUIDFast(_PREHASH_AgentID, gAgentID);
                msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
            }

            msg->nextBlockFast(_PREHASH_PhysicalAvatarEventList);
            msg->addBinaryDataFast(_PREHASH_TypeData, puppet_pack_buffer.data(), dataPacker.getCurrentSize());
        }
    }

    mOutgoingEvents.clear();

    if (msgblock_count)
    {   // there are some events that weren't sent above.  Send them along.
        LL_DEBUGS("PUPPET_SPAM") << "Sending message with " << msgblock_count << " blocks and "
            << joint_count << " joints in frame " << (S32) gFrameCount << LL_ENDL;
        gAgent.sendMessage();
    }
    else
    {   // clean up message we started
        msg->clearMessage();
    }
}

void LLPuppetMotion::unpackEvents(LLMessageSystem *mesgsys,int blocknum)
{
    std::array<U8, PUPPET_MAX_MSG_BYTES> puppet_pack_buffer;

    LLDataPackerBinaryBuffer dataPacker( puppet_pack_buffer.data(), PUPPET_MAX_MSG_BYTES );
    dataPacker.reset();

    S32 data_size = mesgsys->getSizeFast(_PREHASH_PhysicalAvatarEventList, blocknum, _PREHASH_TypeData);
    mesgsys->getBinaryDataFast(_PREHASH_PhysicalAvatarEventList, _PREHASH_TypeData, puppet_pack_buffer.data(), data_size , blocknum,PUPPET_MAX_MSG_BYTES);

    LL_DEBUGS_IF(data_size > 0, "PUPPET_SPAM") << "Have puppet buffer " << data_size << " bytes in frame " << (S32) gFrameCount << LL_ENDL;

    LLPuppetEvent event;
    if (event.unpack(dataPacker))
    {
        queueEvent(event);
    }
    else
    {
        LL_WARNS_ONCE("Puppet") << "Invalid puppetry packet received. Rejecting!" << LL_ENDL;
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
            llcoro::suspendUntilTimeout(0.25);
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
}

