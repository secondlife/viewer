/** 
 * @file lljointsolverrp3.h
 * @brief Implementation of LLJointSolverRP3 class
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

#ifndef LL_LLJOINTSOLVERRP3_H
#define LL_LLJOINTSOLVERRP3_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "lljoint.h"

/* -some compilers don't like line continuation chars-
//-----------------------------------------------------------------------------
// class LLJointSolverRP3
//
// This class is a "poor man's" IK for simple 3 joint kinematic chains.
// It is modeled after the 'ikRPSolver' in Maya.
// This class takes 4 LLJoints:
//   jointA
//   jointB
//   jointC
//   jointGoal
//
// Such that jointA is the parent of jointB, jointB is the parent of jointC.
// When invoked, this class modifies the rotations of jointA and jointB such
// that the position of the jointC attempts to reach the position of jointGoal.
//
// At object initialization time, the distances between jointA - jointB and
// jointB - jointC are cached.  During evaluation these bone lengths are
// preserved.
//
//  A          A 
//  |          |
//  |          |
//  B          B---CG     A---B---C...G
//   \
//    \
//     CG
//
//
// In addition a "poleVector" is specified that does two things:
//
// a) defines the plane in which the solution occurs, thus
//    reducing an infinite number of solutions, down to 2.
//
// b) disambiguates the resulting two solutions as follows:
//
//  A             A            A--->poleVector
//  |              \            \
//  |               \            \
//  B       vs.      B   ==>      B
//   \               |            |
//    \              |            |
//     CG            CG           CG
//
// A "twist" setting allows the solution plane to be rotated about the
// line between A and C.  A handy animation feature.
//
// For "smarter" results for non-coplanar limbs, specify the joints axis
// of bend in the B's local frame (see setBAxis())
//-----------------------------------------------------------------------------
*/

class LLJointSolverRP3
{
protected:
	LLJoint		*mJointA;
	LLJoint		*mJointB;
	LLJoint		*mJointC;
	LLJoint		*mJointGoal;

	F32			mLengthAB;
	F32			mLengthBC;

	LLVector3	mPoleVector;
	LLVector3	mBAxis;
	bool		mbUseBAxis;

	F32			mTwist;

	bool		mFirstTime;
	LLMatrix4	mSavedJointAMat;
	LLMatrix4	mSavedInvPlaneMat;

	LLQuaternion	mJointABaseRotation;
	LLQuaternion	mJointBBaseRotation;

public:
	//-------------------------------------------------------------------------
	// Constructor/Destructor
	//-------------------------------------------------------------------------
	LLJointSolverRP3();
	virtual ~LLJointSolverRP3();

	//-------------------------------------------------------------------------
	// setupJoints()
	// This must be called one time to setup the solver.
	// This must be called AFTER the skeleton has been created, all parent/child
	// relationships are established, and after the joints are placed in
	// a valid configuration (as distances between them will be cached).
	//-------------------------------------------------------------------------
	void setupJoints(	LLJoint* jointA,
						LLJoint* jointB,
						LLJoint* jointC,
						LLJoint* jointGoal );

	//-------------------------------------------------------------------------
	// getPoleVector()
	// Returns the current pole vector.
	//-------------------------------------------------------------------------
	const LLVector3& getPoleVector();

	//-------------------------------------------------------------------------
	// setPoleVector()
	// Sets the pole vector.
	// The pole vector is defined relative to (in the space of) jointA's parent.
	// The default pole vector is (1,0,0), and this is used if this function
	// is never called.
	// This vector is normalized when set.
	//-------------------------------------------------------------------------
	void setPoleVector( const LLVector3& poleVector );

	//-------------------------------------------------------------------------
	// setBAxis()
	// Sets the joint's axis in B's local frame, and enable "smarter" solve(). 
	// This allows for smarter IK when for twisted limbs.
	//-------------------------------------------------------------------------
	void setBAxis( const LLVector3& bAxis );

	//-------------------------------------------------------------------------
	// getTwist()
	// Returns the current twist in radians.
	//-------------------------------------------------------------------------
	F32 getTwist();

	//-------------------------------------------------------------------------
	// setTwist()
	// Sets the twist value.
	// The default is 0.0.
	//-------------------------------------------------------------------------
	void setTwist( F32 twist );

	//-------------------------------------------------------------------------
	// solve()
	// This is the "work" function.
	// When called, the rotations of jointA and jointB will be modified
	// such that jointC attempts to reach jointGoal.
	//-------------------------------------------------------------------------
	void solve();
};

#endif // LL_LLJOINTSOLVERRP3_H

