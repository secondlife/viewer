/** 
 * @file lljointsolverrp3.cpp
 * @brief Implementation of Joint Solver in 3D Real Projective space (RP3). See: https://en.wikipedia.org/wiki/Real_projective_space
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
#include "linden_common.h"

#include "lljointsolverrp3.h"

#include "llmath.h"

#define F_EPSILON 0.00001f

#if LL_RELEASE
    #define DEBUG_JOINT_SOLVER 0
#else
    #define DEBUG_JOINT_SOLVER 1
#endif

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
void LLJointSolverRP3::setupJoints( LLJoint* jointA,
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

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "LLJointSolverRP3::solve()" << LL_NEWLINE
                            << "bPosLocal = " << mJointB->getPosition() << LL_NEWLINE
                            << "cPosLocal = " << mJointC->getPosition() << LL_NEWLINE
                            << "bRotLocal = " << mJointB->getRotation() << LL_NEWLINE
                            << "cRotLocal = " << mJointC->getRotation() << LL_NEWLINE
                            << "aPos : " << aPos << LL_NEWLINE
                            << "bPos : " << bPos << LL_NEWLINE
                            << "cPos : " << cPos << LL_NEWLINE
                            << "gPos : " << gPos << LL_ENDL;
#endif

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

    //-------------------------------------------------------------------------
    // compute needed lengths of those vectors
    //-------------------------------------------------------------------------
    F32 abLen = abVec.magVec();
    F32 bcLen = bcVec.magVec();
    F32 agLen = agVec.magVec();

    //-------------------------------------------------------------------------
    // compute component vector of (A->B) orthogonal to (A->C)
    //-------------------------------------------------------------------------
    LLVector3 abacCompOrthoVec = abVec - acVec * ((abVec * acVec)/(acVec * acVec));

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "abVec : " << abVec << LL_NEWLINE
        << "bcVec : " << bcVec << LL_NEWLINE
        << "acVec : " << acVec << LL_NEWLINE
        << "agVec : " << agVec << LL_NEWLINE
        << "abLen : " << abLen << LL_NEWLINE
        << "bcLen : " << bcLen << LL_NEWLINE
        << "agLen : " << agLen << LL_NEWLINE
        << "abacCompOrthoVec : " << abacCompOrthoVec << LL_ENDL;
#endif

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
    F32 cosTheta =  (agLenSq - abLen*abLen - bcLen*bcLen) / (2.0f * abLen * bcLen);
    if (cosTheta > 1.0f)
        cosTheta = 1.0f;
    else if (cosTheta < -1.0f)
        cosTheta = -1.0f;

    F32 theta = acos(cosTheta);

    LLQuaternion bRot(theta - abbcAng, abbcOrthoVec);

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "abbcAng      : " << abbcAng << LL_NEWLINE
                            << "abbcOrthoVec : " << abbcOrthoVec << LL_NEWLINE
                            << "agLenSq      : " << agLenSq << LL_NEWLINE
                            << "cosTheta     : " << cosTheta << LL_NEWLINE
                            << "theta        : " << theta << LL_NEWLINE
                            << "bRot         : " << bRot << LL_NEWLINE
                            << "theta abbcAng theta-abbcAng: " 
                                << theta*180.0/F_PI << " " 
                                << abbcAng*180.0f/F_PI << " " 
                                << (theta - abbcAng)*180.0f/F_PI 
    << LL_ENDL;
#endif

    //-------------------------------------------------------------------------
    // compute rotation that rotates new A->C to A->G
    //-------------------------------------------------------------------------
    // rotate B->C by bRot
    bcVec = bcVec * bRot;

    // update A->C
    acVec = abVec + bcVec;

    LLQuaternion cgRot;
    cgRot.shortestArc( acVec, agVec );

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "bcVec : " << bcVec << LL_NEWLINE
                            << "acVec : " << acVec << LL_NEWLINE
                            << "cgRot : " << cgRot << LL_ENDL;
#endif

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

    //-------------------------------------------------------------------------
    // compute twist rotation
    //-------------------------------------------------------------------------
    LLQuaternion twistRot( mTwist, agVec );

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "abcNorm = " << abcNorm << LL_NEWLINE
                            << "apgNorm = " << apgNorm << LL_NEWLINE
                            << "pRot = " << pRot << LL_NEWLINE
                            << "twist    : " << mTwist*180.0/F_PI << LL_NEWLINE
                            << "twistRot : " << twistRot << LL_ENDL;
#endif

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
