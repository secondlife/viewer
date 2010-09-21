/** 
 * @file lljointsolverrp3.h
 * @brief Implementation of LLJointSolverRP3 class
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
	BOOL		mbUseBAxis;

	F32			mTwist;

	BOOL		mFirstTime;
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

