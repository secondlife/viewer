/** 
 * @file lljointsolverrp3.cpp
 * @brief Implementation of LLJointSolverRP3 class.
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
#include "linden_common.h"

#include "lljointsolverrp3.h"

#include "llmath.h"

#define F_EPSILON 0.00001f


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
LLJointSolverRP3::LLJointSolverRP3()
{
	mJointA = NULL;
	mJointB = NULL;
	mJointC = NULL;
	mJointGoal = NULL;
	mLengthAB = 1.0f;
	mLengthBC = 1.0f;
	mPoleVector.setVec( 1.0f, 0.0f, 0.0f );
	mbUseBAxis = FALSE;
	mTwist = 0.0f;
	mFirstTime = TRUE;
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
/*virtual*/ LLJointSolverRP3::~LLJointSolverRP3()
{
}


//-----------------------------------------------------------------------------
// setupJoints()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setupJoints(	LLJoint* jointA,
									LLJoint* jointB,
									LLJoint* jointC,
									LLJoint* jointGoal )
{
	mJointA = jointA;
	mJointB = jointB;
	mJointC = jointC;
	mJointGoal = jointGoal;

	mLengthAB = mJointB->getPosition().magVec();
	mLengthBC = mJointC->getPosition().magVec();

	mJointABaseRotation = jointA->getRotation();
	mJointBBaseRotation = jointB->getRotation();
}


//-----------------------------------------------------------------------------
// getPoleVector()
//-----------------------------------------------------------------------------
const LLVector3& LLJointSolverRP3::getPoleVector()
{
	return mPoleVector;
}


//-----------------------------------------------------------------------------
// setPoleVector()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setPoleVector( const LLVector3& poleVector )
{
	mPoleVector = poleVector;
	mPoleVector.normVec();
}


//-----------------------------------------------------------------------------
// setPoleVector()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setBAxis( const LLVector3& bAxis )
{
	mBAxis = bAxis;
	mBAxis.normVec();
	mbUseBAxis = TRUE;
}

//-----------------------------------------------------------------------------
// getTwist()
//-----------------------------------------------------------------------------
F32 LLJointSolverRP3::getTwist()
{
	return mTwist;
}


//-----------------------------------------------------------------------------
// setTwist()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setTwist( F32 twist )
{
	mTwist = twist;
}


//-----------------------------------------------------------------------------
// solve()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::solve()
{
//	llinfos << llendl;
//	llinfos << "LLJointSolverRP3::solve()" << llendl;

	//-------------------------------------------------------------------------
	// setup joints in their base rotations
	//-------------------------------------------------------------------------
	mJointA->setRotation( mJointABaseRotation );
	mJointB->setRotation( mJointBBaseRotation );

	//-------------------------------------------------------------------------
	// get joint positions in world space
	//-------------------------------------------------------------------------
	LLVector3 aPos = mJointA->getWorldPosition();
	LLVector3 bPos = mJointB->getWorldPosition();
	LLVector3 cPos = mJointC->getWorldPosition();
	LLVector3 gPos = mJointGoal->getWorldPosition();

//	llinfos << "bPosLocal = " << mJointB->getPosition() << llendl;
//	llinfos << "cPosLocal = " << mJointC->getPosition() << llendl;
//	llinfos << "bRotLocal = " << mJointB->getRotation() << llendl;
//	llinfos << "cRotLocal = " << mJointC->getRotation() << llendl;

//	llinfos << "aPos : " << aPos << llendl;
//	llinfos << "bPos : " << bPos << llendl;
//	llinfos << "cPos : " << cPos << llendl;
//	llinfos << "gPos : " << gPos << llendl;

	//-------------------------------------------------------------------------
	// get the poleVector in world space
	//-------------------------------------------------------------------------
	LLMatrix4 worldJointAParentMat;
	if ( mJointA->getParent() )
	{
		worldJointAParentMat = mJointA->getParent()->getWorldMatrix();
	}
	LLVector3 poleVec = rotate_vector( mPoleVector, worldJointAParentMat );

	//-------------------------------------------------------------------------
	// compute the following:
	// vector from A to B
	// vector from B to C
	// vector from A to C
	// vector from A to G (goal)
	//-------------------------------------------------------------------------
	LLVector3 abVec = bPos - aPos;
	LLVector3 bcVec = cPos - bPos;
	LLVector3 acVec = cPos - aPos;
	LLVector3 agVec = gPos - aPos;

//	llinfos << "abVec : " << abVec << llendl;
//	llinfos << "bcVec : " << bcVec << llendl;
//	llinfos << "acVec : " << acVec << llendl;
//	llinfos << "agVec : " << agVec << llendl;

	//-------------------------------------------------------------------------
	// compute needed lengths of those vectors
	//-------------------------------------------------------------------------
	F32 abLen = abVec.magVec();
	F32 bcLen = bcVec.magVec();
	F32 agLen = agVec.magVec();

//	llinfos << "abLen : " << abLen << llendl;
//	llinfos << "bcLen : " << bcLen << llendl;
//	llinfos << "agLen : " << agLen << llendl;

	//-------------------------------------------------------------------------
	// compute component vector of (A->B) orthogonal to (A->C)
	//-------------------------------------------------------------------------
	LLVector3 abacCompOrthoVec = abVec - acVec * ((abVec * acVec)/(acVec * acVec));

//	llinfos << "abacCompOrthoVec : " << abacCompOrthoVec << llendl;

	//-------------------------------------------------------------------------
	// compute the normal of the original ABC plane (and store for later)
	//-------------------------------------------------------------------------
	LLVector3 abcNorm;
	if (!mbUseBAxis)
	{
		if( are_parallel(abVec, bcVec, 0.001f) )
		{
			// the current solution is maxed out, so we use the axis that is
			// orthogonal to both poleVec and A->B
			if ( are_parallel(poleVec, abVec, 0.001f) )
			{
				// ACK! the problem is singular
				if ( are_parallel(poleVec, agVec, 0.001f) )
				{
					// the solutions is also singular
					return;
				}
				else
				{
					abcNorm = poleVec % agVec;
				}
			}
			else
			{
				abcNorm = poleVec % abVec;
			}
		}
		else
		{
			abcNorm = abVec % bcVec;
		}
	}
	else
	{
		abcNorm = mBAxis * mJointB->getWorldRotation();
	}

	//-------------------------------------------------------------------------
	// compute rotation of B
	//-------------------------------------------------------------------------
	// angle between A->B and B->C
	F32 abbcAng = angle_between(abVec, bcVec);

	// vector orthogonal to A->B and B->C
	LLVector3 abbcOrthoVec = abVec % bcVec;
	if (abbcOrthoVec.magVecSquared() < 0.001f)
	{
		abbcOrthoVec = poleVec % abVec;
		abacCompOrthoVec = poleVec;
	}
	abbcOrthoVec.normVec();

	F32 agLenSq = agLen * agLen;

	// angle arm for extension
	F32 cosTheta =	(agLenSq - abLen*abLen - bcLen*bcLen) / (2.0f * abLen * bcLen);
	if (cosTheta > 1.0f)
		cosTheta = 1.0f;
	else if (cosTheta < -1.0f)
		cosTheta = -1.0f;

	F32 theta = acos(cosTheta);

	LLQuaternion bRot(theta - abbcAng, abbcOrthoVec);

//	llinfos << "abbcAng      : " << abbcAng << llendl;
//	llinfos << "abbcOrthoVec : " << abbcOrthoVec << llendl;
//	llinfos << "agLenSq      : " << agLenSq << llendl;
//	llinfos << "cosTheta     : " << cosTheta << llendl;
//	llinfos << "theta        : " << theta << llendl;
//	llinfos << "bRot         : " << bRot << llendl;
//	llinfos << "theta abbcAng theta-abbcAng: " << theta*180.0/F_PI << " " << abbcAng*180.0f/F_PI << " " << (theta - abbcAng)*180.0f/F_PI << llendl;

	//-------------------------------------------------------------------------
	// compute rotation that rotates new A->C to A->G
	//-------------------------------------------------------------------------
	// rotate B->C by bRot
	bcVec = bcVec * bRot;

	// update A->C
	acVec = abVec + bcVec;

	LLQuaternion cgRot;
	cgRot.shortestArc( acVec, agVec );

//	llinfos << "bcVec : " << bcVec << llendl;
//	llinfos << "acVec : " << acVec << llendl;
//	llinfos << "cgRot : " << cgRot << llendl;

	// update A->B and B->C with rotation from C to G
	abVec = abVec * cgRot;
	bcVec = bcVec * cgRot;
	abcNorm = abcNorm * cgRot;
	acVec = abVec + bcVec;

	//-------------------------------------------------------------------------
	// compute the normal of the APG plane
	//-------------------------------------------------------------------------
	if (are_parallel(agVec, poleVec, 0.001f))
	{
		// the solution plane is undefined ==> we're done
		return;
	}
	LLVector3 apgNorm = poleVec % agVec;
	apgNorm.normVec();

	if (!mbUseBAxis)
	{
		//---------------------------------------------------------------------
		// compute the normal of the new ABC plane
		// (only necessary if we're NOT using mBAxis)
		//---------------------------------------------------------------------
		if( are_parallel(abVec, bcVec, 0.001f) )
		{
			// G is either too close or too far away
			// we'll use the old ABCnormal 
		}
		else
		{
			abcNorm = abVec % bcVec;
		}
		abcNorm.normVec();
	}

	//-------------------------------------------------------------------------
	// calcuate plane rotation
	//-------------------------------------------------------------------------
	LLQuaternion pRot;
	if ( are_parallel( abcNorm, apgNorm, 0.001f) )
	{
		if (abcNorm * apgNorm < 0.0f)
		{
			// we must be PI radians off ==> rotate by PI around agVec
			pRot.setQuat(F_PI, agVec);
		}
		else
		{
			// we're done
		}
	}
	else
	{
		pRot.shortestArc( abcNorm, apgNorm );
	}

//	llinfos << "abcNorm = " << abcNorm << llendl;
//	llinfos << "apgNorm = " << apgNorm << llendl;
//	llinfos << "pRot = " << pRot << llendl;

	//-------------------------------------------------------------------------
	// compute twist rotation
	//-------------------------------------------------------------------------
	LLQuaternion twistRot( mTwist, agVec );

//	llinfos	<< "twist    : " << mTwist*180.0/F_PI << llendl;
//	llinfos << "agNormVec: " << agNormVec << llendl;
//	llinfos << "twistRot : " << twistRot << llendl;

	//-------------------------------------------------------------------------
	// compute rotation of A
	//-------------------------------------------------------------------------
	LLQuaternion aRot = cgRot * pRot * twistRot;

	//-------------------------------------------------------------------------
	// apply the rotations
	//-------------------------------------------------------------------------
	mJointB->setWorldRotation( mJointB->getWorldRotation() * bRot );
	mJointA->setWorldRotation( mJointA->getWorldRotation() * aRot );
}


// End
