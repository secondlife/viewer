/**
 * @file llbvhconsts.h
 * @brief Constants and types for BVH animation processing and constraints.
 *
 * This file defines core constants and enums used by the BVH (Biovision Hierarchy)
 * animation loader and the LindenLab animation format. BVH files are a common
 * format for storing motion capture and keyframe animation data that Second Life
 * converts to its internal animation representation.
 *
 * Key components:
 * - Animation duration limits for user safety and performance
 * - Constraint system types for inverse kinematics and physics
 * - Constraint target types for body parts and environment interaction
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLBVHCONSTS_H
#define LL_LLBVHCONSTS_H

/**
 * @brief Maximum allowed duration for user-uploaded animations (in seconds).
 * 
 * This limit prevents excessively long animations from causing performance issues
 * or storage problems. Enforced in multiple locations:
 * - LLFloaterBVHPreview rejects uploads exceeding this duration
 * - LLKeyframeMotion validation checks against this limit during loading
 * - Animation upload tools validate duration before processing
 * 
 * This applies to user-uploaded content; built-in system animations are not
 * subject to this restriction.
 * 
 * The limit balances animation expressiveness with practical concerns:
 * - Memory usage for keyframe storage
 * - Network bandwidth for animation distribution  
 * - Viewer performance during animation playback
 */
const F32 MAX_ANIM_DURATION = 60.f;

/**
 * @brief Types of constraints that can be applied to avatar joints.
 * 
 * Constraints are used by the inverse kinematics system to create realistic
 * avatar movement by ensuring joints follow physical rules or remain positioned
 * relative to objects in the world.
 */
typedef enum e_constraint_type
    {
        CONSTRAINT_TYPE_POINT,      /// Joint must remain at a specific point in space
        CONSTRAINT_TYPE_PLANE,      /// Joint must remain on or within a plane
        NUM_CONSTRAINT_TYPES        /// Count of constraint types for array sizing
    } EConstraintType;

/**
 * @brief Targets that constraints can be anchored to.
 * 
 * Constraints can be attached to either avatar body parts or environmental
 * elements to create realistic interactions between avatars and their surroundings.
 */
typedef enum e_constraint_target_type
    {
        CONSTRAINT_TARGET_TYPE_BODY,    /// Constraint relative to avatar's body parts
        CONSTRAINT_TARGET_TYPE_GROUND,  /// Constraint relative to ground/world position
        NUM_CONSTRAINT_TARGET_TYPES     /// Count of target types for array sizing
    } EConstraintTargetType;

#endif // LL_LLBVHCONSTS_H
