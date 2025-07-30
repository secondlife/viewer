/**
 * @file lljointstate.h
 * @brief Implementation of LLJointState class.
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

#ifndef LL_LLJOINTSTATE_H
#define LL_LLJOINTSTATE_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"
#include "llrefcount.h"

/**
 * @brief Represents the animated state of a joint during motion playback.
 * 
 * LLJointState encapsulates the transformation values (position, rotation, scale)
 * that an animation wants to apply to a specific joint. Unlike LLJoint which
 * represents the actual joint in the skeleton, LLJointState represents desired
 * animation values that get blended and applied to joints.
 * 
 * This class serves as the bridge between animation data and joint transformations:
 * - Animation files specify joint states for each keyframe
 * - Multiple LLJointState objects can target the same joint with different priorities
 * - The motion controller blends these states based on weight and priority
 * - Final blended values are applied to the target LLJoint
 * 
 * Key features:
 * - Reference counting for memory management in animation systems
 * - Usage flags to specify which transformation components are animated
 * - Priority system for resolving conflicts between multiple animations
 * - Weight-based blending support for smooth animation transitions
 * 
 * @code
 * // Real usage patterns from motion implementations
 * // LLEditingMotion creates joint states in constructor
 * mShoulderState = new LLJointState;
 * mElbowState = new LLJointState;
 * mWristState = new LLJointState;
 * 
 * // During onInitialize(), associate with actual joints
 * mShoulderState->setJoint(character->getJoint("mShoulderLeft"));
 * mShoulderState->setUsage(LLJointState::ROT);  // Only rotating, not translating
 * addJointState(mShoulderState);  // Add to motion's joint state list
 * 
 * // LLKeyframeMotion creates states from animation data
 * LLPointer<LLJointState> joint_state = new LLJointState;
 * joint_state->setJoint(joint);
 * joint_state->setUsage(LLJointState::POS | LLJointState::ROT);
 * @endcode
 */
class LLJointState : public LLRefCount
{
public:
    /**
     * @brief Animation blending phases for smooth transitions.
     * 
     * These phases control how animations transition in and out,
     * allowing for smooth blending between different motions.
     */
    enum BlendPhase
    {
        INACTIVE,   /// Animation is not contributing to joint transformation
        EASE_IN,    /// Animation is gradually increasing its influence
        ACTIVE,     /// Animation is at full influence
        EASE_OUT    /// Animation is gradually decreasing its influence
    };
protected:
    /// Target joint that this animation state will be applied to
    LLJoint *mJoint;

    /// Bitmask indicating which transformation components are animated (POS, ROT, SCALE)
    U32     mUsage;

    /// Blending weight from 0.0 to 1.0 controlling this state's influence
    F32     mWeight;

    /// Desired position relative to parent joint
    LLVector3       mPosition;
    /// Desired rotation relative to parent joint  
    LLQuaternion    mRotation;
    /// Desired scale relative to rotated frame
    LLVector3       mScale;
    /// Priority level for resolving conflicts with other animation states
    LLJoint::JointPriority  mPriority;
public:
    /**
     * @brief Default constructor creates an uninitialized joint state.
     * 
     * Creates a joint state with no target joint and zero weight.
     * You must call setJoint() and configure other properties before use.
     */
    LLJointState()
        : mUsage(0)
        , mJoint(NULL)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    /**
     * @brief Constructor targeting a specific joint.
     * 
     * Creates a joint state that will animate the specified joint.
     * Usage flags, weight, and transformation values must still be set.
     * 
     * @param joint Target joint for this animation state
     */
    LLJointState(LLJoint* joint)
        : mUsage(0)
        , mJoint(joint)
        , mWeight(0.f)
        , mPriority(LLJoint::USE_MOTION_PRIORITY)
    {}

    /**
     * @brief Gets the target joint for this animation state.
     * 
     * @return Target joint that will receive this animation data
     */
    LLJoint* getJoint()             { return mJoint; }
    
    /**
     * @brief Gets the target joint for this animation state (const version).
     * 
     * @return Target joint that will receive this animation data
     */
    const LLJoint* getJoint() const { return mJoint; }
    
    /**
     * @brief Sets the target joint for this animation state.
     * 
     * @param joint Joint to receive this animation data
     * @return true if joint was set successfully, false if joint is NULL
     */
    bool setJoint( LLJoint *joint ) { mJoint = joint; return mJoint != NULL; }

    /**
     * @brief Flags indicating which transformation components are animated.
     * 
     * These flags specify which parts of the joint transformation this
     * animation state controls. Multiple flags can be combined with bitwise OR.
     * Usage flags are automatically set when transformation values are assigned.
     */
    enum Usage
    {
        POS     = 1,    /// Animates joint position
        ROT     = 2,    /// Animates joint rotation
        SCALE   = 4,    /// Animates joint scale
    };
    
    /**
     * @brief Gets the usage flags indicating which components are animated.
     * 
     * @return Bitmask of Usage flags (POS, ROT, SCALE)
     */
    U32 getUsage() const            { return mUsage; }
    
    /**
     * @brief Sets the usage flags for which components are animated.
     * 
     * Typically set automatically when calling setPosition(), setRotation(),
     * or setScale(), but can be set manually for advanced use cases.
     * 
     * @param usage Bitmask of Usage flags to enable
     */
    void setUsage( U32 usage )      { mUsage = usage; }
    
    /**
     * @brief Gets the blending weight for this animation state.
     * 
     * @return Weight from 0.0 (no influence) to 1.0 (full influence)
     */
    F32 getWeight() const           { return mWeight; }
    
    /**
     * @brief Sets the blending weight for this animation state.
     * 
     * The weight determines how much this animation state influences
     * the final joint transformation when blended with other states.
     * 
     * @param weight Influence weight from 0.0 to 1.0
     */
    void setWeight( F32 weight )    { mWeight = weight; }

    /**
     * @brief Gets the desired position for the target joint.
     * 
     * @return Position relative to parent joint
     */
    const LLVector3& getPosition() const        { return mPosition; }
    
    /**
     * @brief Sets the desired position for the target joint.
     * 
     * Automatically sets the POS usage flag. The position is relative
     * to the joint's parent in the skeleton hierarchy.
     * 
     * @param pos Desired position relative to parent joint
     */
    void setPosition( const LLVector3& pos )    { mUsage |= POS; mPosition = pos; }

    /**
     * @brief Gets the desired rotation for the target joint.
     * 
     * @return Rotation quaternion relative to parent joint
     */
    const LLQuaternion& getRotation() const     { return mRotation; }
    
    /**
     * @brief Sets the desired rotation for the target joint.
     * 
     * Automatically sets the ROT usage flag. The rotation is relative
     * to the joint's parent in the skeleton hierarchy.
     * 
     * @param rot Desired rotation quaternion relative to parent joint
     */
    void setRotation( const LLQuaternion& rot ) { mUsage |= ROT; mRotation = rot; }

    /**
     * @brief Gets the desired scale for the target joint.
     * 
     * @return Scale vector relative to rotated frame
     */
    const LLVector3& getScale() const           { return mScale; }
    
    /**
     * @brief Sets the desired scale for the target joint.
     * 
     * Automatically sets the SCALE usage flag. The scale is applied
     * in the joint's rotated coordinate frame.
     * 
     * @param scale Desired scale vector
     */
    void setScale( const LLVector3& scale )     { mUsage |= SCALE; mScale = scale; }

    /**
     * @brief Gets the priority level for animation blending.
     * 
     * @return Priority level used to resolve conflicts with other animation states
     */
    LLJoint::JointPriority getPriority() const  { return mPriority; }
    
    /**
     * @brief Sets the priority level for animation blending.
     * 
     * When multiple animation states target the same joint, priority
     * determines which animations take precedence during blending.
     * 
     * @param priority Priority level from LLJoint::JointPriority enum
     */
    void setPriority( LLJoint::JointPriority priority ) { mPriority = priority; }

protected:
    /**
     * @brief Virtual destructor for proper cleanup in inheritance hierarchy.
     * 
     * Protected destructor enforces reference counting through LLRefCount.
     * LLJointState objects should be managed with LLPointer smart pointers.
     */
    virtual ~LLJointState()
    {
    }

};

#endif // LL_LLJOINTSTATE_H

