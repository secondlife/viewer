/**
 * @file llik.h
 * @brief Implementation of LLIK::Solver class and related helpers.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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
#ifndef LL_LLIK_H
#define LL_LLIK_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "llmath.h"
#include "llquaternion.h"
#include "m4math.h"
#include "v3math.h"
#include "v4math.h"
#include "xform.h"

//#define DEBUG_LLIK_UNIT_TESTS

namespace LLIK
{
// Inverse Kinematics (IK) for humanoid character.
//
// The Solver uses Forward and Backward Reaching Inverse Kinematics (FABRIK)
// algorightm to iterate toward a solution:
//
//      http://andreasaristidou.com/FABRIK.html
//
// The Joints can have Constraints which limite their parent-local orientations.
//

class Solver;
class Joint;

constexpr F32 IK_DEFAULT_ACCEPTABLE_ERROR = 5.0e-4f; // half millimeter

// A Target represents the final position, or position+orientation of an
// end-effector Joint.  Also typically called the "goal" of the end-effector
// in IK parlance.
class Target
{
public:
    Target();

    // Note: we currently support Targets in three modes:
    // (a) mRot in parent-frame
    // (b) mPos in root-frame
    // (c) mPos and mRot in root-frame
    //
    // Modes (a) and (b) are independent of each other and can coexist.
    // Mode (c) obviates the other two.

    bool hasWorldPos() const { return (mMask & HAS_POS) > 0; }
    bool hasRot() const { return (mMask & HAS_ROT) > 0; }
    bool hasWorldRot() const { return (mMask & (HAS_ROT | ROT_IS_LOCAL)) == HAS_ROT; }
    bool fixesLocalRot() const { return (mMask & ROT_IS_LOCAL) > 0; }

    void setPos(const LLVector3& pos);
    void setRot(const LLQuaternion& rot, bool is_parent_local=false);

    const LLVector3& getPos() const { return mPos; }
    const LLQuaternion& getRot() const { return mRot; }

    // EXPERIMENTAL: keep the "delegate" stuff for now
    void delegate() { mMask |= HAS_DELEGATED; }
    bool hasDelegated() const { return (mMask & HAS_DELEGATED) > 0; }

    void useIK() { mMask |= USE_IK; }
    bool usesIK() const { return (mMask & USE_IK) > 0; }
private:
    LLVector3 mPos;
    LLQuaternion mRot;

    enum TargetBits
    {
        HAS_POS = 1 << 0,
        HAS_ROT = 1 << 1,
        POS_IS_LOCAL = 1 << 2, // not yet used
        ROT_IS_LOCAL = 1 << 3,
        USE_IK = 1 << 6,
        HAS_DELEGATED = 1 << 7 // EXPERIMENTAL
    };
    U8 mMask = 0; // per-feature bits
};

// A Constraint exists at the tip of Joint
// and limits the range of Joint.mLocalRot.
class Constraint
{
public:
    using ptr_t = std::shared_ptr<Constraint>;

    class Info
    {
    public:
        // public Constraint
        enum ConstraintType
        {
            NULL_CONSTRAINT,
            UNKNOWN_CONSTRAINT,
            SIMPLE_CONE_CONSTRAINT,
            TWIST_LIMITED_CONE_CONSTRAINT,
            ELBOW_CONSTRAINT,
            KNEE_CONSTRAINT,
            ACUTE_ELLIPSOIDAL_CONE_CONSTRAINT,
            DOUBLE_LIMITED_HINGE_CONSTRAINT
        };

        std::string getString() const;

        std::vector<LLVector3> mVectors;
        std::vector<F32> mFloats;
        ConstraintType mType = UNKNOWN_CONSTRAINT;
    };

    Constraint() {}
    virtual ~Constraint() {};
    Info::ConstraintType getType() const { return mType; }
    bool enforce(Joint& joint) const;
    virtual LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const = 0;
    virtual LLQuaternion minimizeTwist(const LLQuaternion& joint_local_rot) const;

    // all Constraints have a forward axis
    const LLVector3& getForwardAxis() const { return mForward; }
    virtual bool allowsTwist() const { return true; }
#ifdef DEBUG_LLIK_UNIT_TESTS
    virtual void dumpConfig() const = 0;
#endif
protected:
    LLVector3 mForward { LLVector3::zero };
    Info::ConstraintType mType = Info::NULL_CONSTRAINT;
};

// SimpleCone Constrainte can twist arbitrarily about its 'forward' axis
// but has a uniform bend limit for orientations perpendicular to 'forward'.
class SimpleCone : public Constraint
{
    // Constrains forward axis inside cone.
    //
    //        / max_angle
    //       /
    //   ---@--------> forward
    //       \
    //        \ max_angle
    //
public:
    SimpleCone(const LLVector3& forward_axis, F32 max_angle);
    ~SimpleCone(){}
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    F32 mCosConeAngle;
    F32 mSinConeAngle;
};

// TwistLimitedCone Constrainte can has limited twist about its 'forward' axis
// but has a uniform bend limit for orientations perpendicular to 'forward'.
class TwistLimitedCone : public Constraint
{
    // A constraint for the shoulder.  Like SimpleCone but with limited twist
    //
    // View from side:                 View with foward out of page:
    //                                         max_twist
    //        / cone_angle                  | /
    //       /                              |/
    //   ---@--------> forward_axis    ----(o)----> perp_axis
    //       \                             /|
    //        \ cone_angle                / |
    //                             min_twist
    //
public:
    TwistLimitedCone(const LLVector3& forward_axis, F32 cone_angle, F32 min_twist, F32 max_twist);
    ~TwistLimitedCone(){}
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
    LLQuaternion minimizeTwist(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    F32 mCosConeAngle;
    F32 mSinConeAngle;
    F32 mMinTwist;
    F32 mMaxTwist;
};

// ElbowConstraint can only bend (with limits) about its 'pivot' axis
// and allows limited twist about its 'forward' axis.
class ElbowConstraint : public Constraint
{
    // A Constraint for Elbow: limited hinge with limited twist about forward (forearm) axis.
    //
    // View from the side,             View with foreward axis out of page:
    // with pivot axis out of page:
    //                                      up  max_twist
    //        / max_bend                     | /
    //       /                               |/
    //  ---(o)--------+  forward        ----(o)----> left
    //       \                              /|
    //        \ min_bend                   / |
    //                              min_twist
public:
    ElbowConstraint(const LLVector3& forward_axis, const LLVector3& pivot_axis, F32 min_bend, F32 max_bend, F32 min_twist, F32 max_twist);
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
    LLQuaternion minimizeTwist(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    LLVector3 mPivotAxis;
    LLVector3 mLeft;
    F32 mMinBend;
    F32 mMaxBend;
    F32 mMinTwist;
    F32 mMaxTwist;
};

// KneeConstraint only allows bend (limited) about its 'pivot' axis
// but does not allow any twist about its 'forward' axis.
class KneeConstraint : public Constraint
{
    // A Constraint for knee, or finger. Like ElbowConstraint but
    // no twist allowed, min/max limits on angle about pivot.
    //
    // View from the side, with pivot axis out of page:
    //
    //        / max_bend
    //       /
    //  ---(o)--------+
    //       \
    //        \ min_bend
    //
public:
    KneeConstraint(const LLVector3& forward_axis, const LLVector3& pivot_axis, F32 min_bend, F32 max_bend);
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
    bool allowsTwist() const override { return false; }
    LLQuaternion minimizeTwist(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    LLVector3 mPivotAxis;
    LLVector3 mLeft;
    F32 mMinBend;
    F32 mMaxBend;
};

// AcuteEllipsoidalCone is like SimpleCone but with asymmetric radiuses in
// the up, left, down, right directions.  In other words: it has non-symmetric
// bend limits for axes perpendicular to its 'forward' axix.  It was implemented
// mostly as an exercise, since it is similar to the Constraint described in
// the original FABRIK papter. The geometry of the ellipsoidal boundary are
// described by defining the forward offset of the "cross" of radiuses.  Each
// quadrant of the cross in the left-up plane is bound by an elliptical curve that
// depends on its bounding radiuses.
//
//     up  left            |
//      | /                | /
//      |/                 |/
//   ---@------------------+
//           forward      /|
//                         |
//
class AcuteEllipsoidalCone : public Constraint
{
public:
    AcuteEllipsoidalCone(
            const LLVector3& forward_axis,
            const LLVector3& up_axis,
            F32 forward,
            F32 up,
            F32 left,
            F32 down,
            F32 right);
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    LLVector3 mUp;
    LLVector3 mLeft;

    // for each quadrant we cache these parameters to help
    // us project onto each partial ellipse.
    F32 mQuadrantScales[4];
    F32 mQuadrantCosAngles[4];
    F32 mQuadrantCotAngles[4];
};

// The DoubleLimitedHinge constraint is intended for uses on Joints like
// the wrist, or first finger Joints.  It allows for yaw and pitch bends
// but zero twist.
class DoubleLimitedHinge : public Constraint
{
    // A Constraint for first finger bones.
    // No twist allowed, min/max limits on yaw, then pitch.
    //
    // View from above                     View from right
    // with UP out of page                 (remember to use right-hand-rule)
    //
    //   left_axis                            up_axis
    //      |                                   |
    //      | / max_yaw_angle                   | / min_pitch_angle
    //      |/                                  |/
    //  ---(o)--------> forward_axis        ---(x)--------> forward_axis
    //    up \                              left \
    //        \ min_yaw_angle                     \ max_pitch_angle
    //
public:
    DoubleLimitedHinge(
            const LLVector3& forward_axis,
            const LLVector3& up_axis,
            F32 min_yaw,
            F32 max_yaw,
            F32 min_pitch,
            F32 max_pitch);
    LLQuaternion computeAdjustedLocalRot(const LLQuaternion& joint_local_rot) const override;
    LLQuaternion minimizeTwist(const LLQuaternion& joint_local_rot) const override;
#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const override;
#endif
private:
    LLVector3 mUp;
    LLVector3 mLeft; // mUp X mForward
    F32 mMinYaw;
    F32 mMaxYaw;
    F32 mMinPitch;
    F32 mMaxPitch;
};

// Joint represents a constrained bone in the skeleton heirarchy.
// It typically has a parent Joint, a fixed mLocalPos position in its
// parent's local-frame, and a fixed mBone to its 'end' position
// in its own local-frame.  A summary of its important data members
// is as follows:
//
//     mLocalPos = invarient tip position in parent's local-frame
//     mLocalRot = orientation of Joint's tip relative to parent's local-frame
//     mBone = invarient end position in local-frame
//     mPos = tip position in world-frame (we call it 'world-frame'
//         but really it is the 'root-frame' of the Skeleton hierarchy).
//     mRot = orientation of Joint in world-frame.
//
// Some imprtant formula to keep in mind:
//
//     mPos = mParent->mPos + mLocalPos * mParent->mRot
//     mRot = mLocalRot * mParent->mRot
//
// The world-frame 'end' position of the Joint can be calculated:
//
//     world_end_pos = mPos + mBone * mRot
//
class Joint
{
public:
    using ptr_t = std::shared_ptr<Joint>;
    using child_vec_t = std::vector<ptr_t>;

    Joint(S16 id, const LLVector3& local_pos, const LLVector3& bone);

    void addChild(const ptr_t& child);
    void setParent(const ptr_t& parent);
    void resetRotationsRecursively();
    void relaxRotationsRecursively(F32 blend_factor);
    F32 recursiveComputeLongestChainLength(F32 length) const;

    void reconfigure(const LLVector3& local_pos, const LLVector3& bone);

    LLVector3 computeEndTargetPos() const;
    LLVector3 computeWorldTipOffset() const;
    void updateEndInward();
    void updateEndOutward();
    void updateBranchRoot();
    void updateInward(const ptr_t& child);
    void updatePosAndRotFromParent();
    void updateOutward();
    void applyLocalRot();
    void updateLocalRot();
    LLQuaternion computeParentRot() const;
    void updateChildLocalRots() const;

    LLVector3 computePosFromParent() const;
    const LLVector3& getWorldTipPos() const { return mPos; }
    const LLQuaternion& getWorldRot() const { return mRot; }
    LLVector3 computeWorldEndPos() const;
    void setWorldPos(const LLVector3& pos);
    void setWorldRot(const LLQuaternion& rot);
    void setLocalRot(const LLQuaternion& new_local_rot);
    void adjustWorldRot(const LLQuaternion& adjustment);
    void shiftPos(const LLVector3& shift);

    void setTarget(const Target& target);
    void setTargetPos(const LLVector3& pos);
    LLVector3 getTargetPos() { return mTarget->getPos(); }
    const Target* getTarget() const { return mTarget; }
    bool hasWorldPosTarget() const { return mHasWorldPosTarget; }
    bool hasWorldRotTarget() const { return mHasWorldRotTarget; }
    void clearAllFlags();
    void lockLocalRot(const LLQuaternion& local_rot);

    void setConstraint(std::shared_ptr<Constraint> constraint) { mConstraint = constraint; }
    bool enforceConstraint();
    void updateWorldTransformsRecursively();

    const LLQuaternion& getLocalRot() const { return mLocalRot; }
    S16 getID() const { return mID; }
    ptr_t getSingleActiveChild();
    const LLVector3& getBone() const { return mBone; }
    const LLVector3& getLocalPos() const { return mLocalPos; }
    F32 getBoneLength() const { return mBone.length(); }
    F32 getLocalPosLength() const { return mLocalPosLength; }

    ptr_t getParent() { return mParent; }
    void activate() { mIsActive = true; }
    bool isActive() const { return mIsActive; }

    size_t getNumChildren() const { return mChildren.size(); }

    void transformTargetsToParentLocal(std::vector<LLVector3>& local_targets) const;
    bool swingTowardTargets(const std::vector<LLVector3>& local_targets, const std::vector<LLVector3>& world_targets);
    void twistTowardTargets(const std::vector<LLVector3>& local_targets, const std::vector<LLVector3>& world_targets);
    void untwist();

#ifdef DEBUG_LLIK_UNIT_TESTS
    void dumpConfig() const;
    void dumpState() const;
#endif

    void collectTargetPositions(std::vector<LLVector3>& local_targets, std::vector<LLVector3>& world_targets) const;

protected:
    void resetRot();
    void relaxRot(F32 blend_factor);

protected:
    std::vector<ptr_t> mChildren;      //List of joint_ids attached to this one.

    LLVector3 mLocalPos; // pos in parent-frame
    LLVector3 mPos; // pos in world-frame
    // The fundamental position formula is:
    //     mPos = mParent->mPos + mLocalPos * mParent->mRot;

    LLQuaternion mLocalRot; // orientations in parent-frame
    LLQuaternion mRot;
    // The fundamental orientations formula is:
    //     mRot = mLocalRot * mParent->mRot

    LLVector3 mBone;
    // There is another fundamental formula:
    //    world_end_pos = mPos + mBone * mRot

    ptr_t mParent;
    Constraint::ptr_t mConstraint;
    F32 mLocalPosLength = 0.0f; // cached copy of mLocalPos.length()
    S16 mID = -1;

    // pointer into Target list managed by Solver
    const Target* mTarget = nullptr;

    // mIsActive = set true by Solver when this joint is on a "chain".
    bool mIsActive = false;
    bool mLocalRotIsLocked = false; // Note: always 'true' for root Joints
    bool mHasWorldPosTarget = false;
    bool mHasWorldRotTarget = false;
};


// The Solver maintains a skeleton of connected Joints and computes the
// parent-relative orientations to allow end-effectors to reach their targets.
//
class Solver
{
public:
    using joint_map_t = std::map<S16, Joint::ptr_t>;
    using S16_vec_t = std::vector<S16>;
    using target_map_t = std::map<S16, Target>;
    using chain_t = std::vector<Joint::ptr_t>;
    using chain_map_t = std::map<S16, chain_t>;

public:
    Solver();

    // Put skeleton back into default orientation
    // (e.g. T-Pose for humanoid character)
    void resetRotations();

    // compute the offset from the "tip" of from_id to the "end" of to_id
    // or the negative when from_id > to_id
    LLVector3 computeReach(S16 to_id, S16 from_id) const;

    // Add a Joint to the skeleton.
    void addJoint(S16 joint_id, S16 parent_id, const LLVector3& local_pos, const LLVector3& bone, const Constraint::ptr_t& constraint);

    // Solve the IK problem for the given targets
    // return max_error of result
    F32 solveForTargets(target_map_t& targets);

    // Specify a joint as a 'wrist'.  Will be used to help 'drop the elbow'
    // of the arm to achieve a more realistic solution.
    void addWristID(S16 wrist_id);

    void setRootID(S16 root_id) { mRootID = root_id; }
    S16 getRootID() const { return mRootID; }

    const S16_vec_t getActiveIDs() const { return mActiveIDs; }

    // Specify list of joint ids that should be considered as sub-bases
    // e.g. joints that are known to have multipe child chains,
    // like the chest (chains on left and right collar children) or wrists
    // (chain for each fingers).
    void setSubBaseIds(const std::set<S16>& ids);

    // Set list of joint ids that should be considered sub-roots where
    // the IK chains stop.  This HACK was used to remove the spine from the
    // solver before spine constraints were working.
    void setSubRootIds(const std::set<S16>& ids);

    // per-Joint property accessors from outside Solver
    LLQuaternion getJointLocalRot(S16 joint_id) const;
    LLVector3 getJointWorldTipPos(S16 joint_id) const;
    LLVector3 getJointWorldEndPos(S16 joint_id) const;
    LLQuaternion getJointWorldRot(S16 joint_id) const;

    void reconfigureJoint(S16 joint_id, const LLVector3& local_pos, const LLVector3& bone, const Constraint::ptr_t& constraint);

#ifdef DEBUG_LLIK_UNIT_TESTS
    size_t getNumJoints() const { return mSkeleton.size(); }
    void enableDebug();
    void dumpConfig() const;
    void dumpActiveState() const;
    F32 getMaxError() const { return mLastError; }
    void updateBounds(const LLVector3& point);
#endif

    void setAcceptableError(F32 slop) { mAcceptableError = slop; }

private:
    bool isSubBase(S16 joint_id) const;
    bool isSubRoot(S16 joint_id) const;
    bool updateTargets(const target_map_t& targets);
    void adjustTargets(target_map_t& targets);
    void dropElbow(const Joint::ptr_t& wrist_joint);
    void rebuildAllChains();
    void buildChain(Joint::ptr_t joint, chain_t& chain, std::set<S16>& sub_bases);
    void executeFabrikInward(const chain_t& chain);
    void executeFabrikOutward(const chain_t& chain);
    void shiftChainToBase(const chain_t& chain);
    void executeFabrikPass();
    void enforceConstraintsOutward();
    void executeCcdPass(); // EXPERIMENTAL
    void executeCcdInward(const chain_t& chain);
    void untwistChain(const chain_t& chain);
    F32 measureMaxError();

private:
    joint_map_t mSkeleton;
    target_map_t mTargets;

    chain_map_t mChainMap;
    std::set<S16> mSubBaseIds; // HACK: whitelist of sub-bases
    std::set<S16> mSubRootIds; // HACK: whitelist of sub-roots
    std::set<Joint::ptr_t> mActiveRoots;
    S16_vec_t mActiveIDs;
    chain_t mWristJoints;
    F32 mAcceptableError = IK_DEFAULT_ACCEPTABLE_ERROR;
    F32 mLastError = 0.0f;
    S16 mRootID;                //ID number of the root joint for this skeleton
#ifdef DEBUG_LLIK_UNIT_TESTS
    LLVector3 mMinPos;
    LLVector3 mMaxPos;
    bool mDebugEnabled = false;
#endif
};

} // namespace LLIK

// Constraint's are 'stateless' configurations so we use a Factory pattern to
// allocate them, which allows multiple Joints with identical constraint configs
// to use a single Constraint instance.
//
class LLIKConstraintFactory
{
public:
    std::shared_ptr<LLIK::Constraint> getConstraint(const LLIK::Constraint::Info& info);
    size_t getNumConstraints() const { return mConstraints.size(); } // for unit-test
private:
    static std::shared_ptr<LLIK::Constraint> create(const LLIK::Constraint::Info& info);
    std::map<std::string, std::shared_ptr<LLIK::Constraint> > mConstraints;
};

#endif // LL_LLIK_H

