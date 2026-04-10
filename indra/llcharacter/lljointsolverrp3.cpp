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

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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
    : mJointABaseRotation(1.f, 0.f, 0.f, 0.f),
      mJointBBaseRotation(1.f, 0.f, 0.f, 0.f)
{
    mJointA = NULL;
    mJointB = NULL;
    mJointC = NULL;
    mJointGoal = NULL;
    mLengthAB = 1.0f;
    mLengthBC = 1.0f;
    mbUseBAxis = false;
    mTwist = 0.0f;
    mFirstTime = true;
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
LLJointSolverRP3::~LLJointSolverRP3() = default;


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

    mLengthAB = glm::length(mJointB->getPosition());
    mLengthBC = glm::length(mJointC->getPosition());

    mJointABaseRotation = jointA->getRotation();
    mJointBBaseRotation = jointB->getRotation();
}


//-----------------------------------------------------------------------------
// getPoleVector()
//-----------------------------------------------------------------------------
const glm::vec3& LLJointSolverRP3::getPoleVector() const
{
    return mPoleVector;
}


//-----------------------------------------------------------------------------
// setPoleVector()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setPoleVector( const glm::vec3& poleVector )
{
    mPoleVector = glm::normalize(poleVector);
}


//-----------------------------------------------------------------------------
// setBAxis()
//-----------------------------------------------------------------------------
void LLJointSolverRP3::setBAxis( const glm::vec3& bAxis )
{
    mBAxis = glm::normalize(bAxis);
    mbUseBAxis = true;
}

//-----------------------------------------------------------------------------
// getTwist()
//-----------------------------------------------------------------------------
F32 LLJointSolverRP3::getTwist() const
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
                            << "bPosLocal = " << LLVector3(mJointB->getPosition()) << LL_NEWLINE
                            << "cPosLocal = " << LLVector3(mJointC->getPosition()) << LL_NEWLINE
                            << "bRotLocal = " << LLQuaternion(mJointB->getRotation()) << LL_NEWLINE
                            << "cRotLocal = " << LLQuaternion(mJointC->getRotation()) << LL_NEWLINE
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
    F32 abLen = abVec.length();
    F32 bcLen = bcVec.length();
    F32 agLen = agVec.length();

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
                    abcNorm = cross(poleVec, agVec);
                }
            }
            else
            {
                abcNorm = cross(poleVec, abVec);
            }
        }
        else
        {
            abcNorm = cross(abVec, bcVec);
        }
    }
    else
    {
        // Pre-emptive bridge: vec * LLJoint::getWorldRotation() will be
        // ambiguous when getWorldRotation() returns glm::quat.
        abcNorm = LLVector3(mBAxis) * LLQuaternion(mJointB->getWorldRotation());
    }

    //-------------------------------------------------------------------------
    // compute rotation of B
    //-------------------------------------------------------------------------
    // angle between A->B and B->C
    F32 abbcAng = angle_between(abVec, bcVec);

    // vector orthogonal to A->B and B->C
    LLVector3 abbcOrthoVec = cross(abVec, bcVec);
    if (abbcOrthoVec.lengthSquared() < 0.001f)
    {
        abbcOrthoVec = cross(poleVec, abVec);
        abacCompOrthoVec = poleVec;
    }
    abbcOrthoVec.normalize();

    F32 agLenSq = agLen * agLen;

    // angle arm for extension
    F32 cosTheta =  (agLenSq - abLen*abLen - bcLen*bcLen) / (2.0f * abLen * bcLen);
    if (cosTheta > 1.0f)
        cosTheta = 1.0f;
    else if (cosTheta < -1.0f)
        cosTheta = -1.0f;

    F32 theta = acos(cosTheta);

    // bRot: glm::quat (phase 2 quat migration cluster #14).
    // glm::angleAxis takes (angle, normalized_axis). abbcOrthoVec is
    // already normalized at line 266 above. The implicit
    // LLVector3 -> glm::vec3 conversion handles the axis param type.
    glm::quat bRot = glm::angleAxis(theta - abbcAng, glm::vec3(abbcOrthoVec));

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "abbcAng      : " << abbcAng << LL_NEWLINE
                            << "abbcOrthoVec : " << abbcOrthoVec << LL_NEWLINE
                            << "agLenSq      : " << agLenSq << LL_NEWLINE
                            << "cosTheta     : " << cosTheta << LL_NEWLINE
                            << "theta        : " << theta << LL_NEWLINE
                            << "bRot         : " << LLQuaternion(bRot) << LL_NEWLINE
                            << "theta abbcAng theta-abbcAng: "
                                << theta*180.0/F_PI << " "
                                << abbcAng*180.0f/F_PI << " "
                                << (theta - abbcAng)*180.0f/F_PI
    << LL_ENDL;
#endif

    //-------------------------------------------------------------------------
    // compute rotation that rotates new A->C to A->G
    //-------------------------------------------------------------------------
    // rotate B->C by bRot. Disambiguate the LLVector3 * quat overload by
    // forcing bRot back to LLQuaternion at the call site (cluster #14
    // bridge — bRot is glm::quat now but the surrounding vec math is LL).
    bcVec = bcVec * LLQuaternion(bRot);

    // update A->C
    acVec = abVec + bcVec;

    // cgRot: glm::quat (phase 2 quat migration cluster #15).
    // glm::rotation(from, to) requires both vectors to be NORMALIZED
    // (it computes cos(theta) = dot(from, to) directly). LLQuaternion::shortestArc
    // handles non-unit input internally; we replicate that by normalizing
    // here. Behaviorally identical because the produced rotation is unit-length.
    glm::quat cgRot = glm::rotation(glm::normalize(glm::vec3(acVec)),
                                    glm::normalize(glm::vec3(agVec)));

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "bcVec : " << bcVec << LL_NEWLINE
                            << "acVec : " << acVec << LL_NEWLINE
                            << "cgRot : " << LLQuaternion(cgRot) << LL_ENDL;
#endif

    // update A->B and B->C with rotation from C to G. Disambiguate the
    // LLVector3 * quat overload by forcing cgRot back to LLQuaternion at
    // each call site (cluster #15 bridge — cgRot is glm::quat now but
    // the surrounding vec math + the triple compose at the end of solve()
    // are still LL).
    const LLQuaternion cgRot_ll(cgRot);
    abVec = abVec * cgRot_ll;
    bcVec = bcVec * cgRot_ll;
    abcNorm = abcNorm * cgRot_ll;
    acVec = abVec + bcVec;

    //-------------------------------------------------------------------------
    // compute the normal of the APG plane
    //-------------------------------------------------------------------------
    if (are_parallel(agVec, poleVec, 0.001f))
    {
        // the solution plane is undefined ==> we're done
        return;
    }
    LLVector3 apgNorm = cross(poleVec, agVec);
    apgNorm.normalize();

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
            abcNorm = cross(abVec, bcVec);
        }
        abcNorm.normalize();
    }

    //-------------------------------------------------------------------------
    // calcuate plane rotation
    //-------------------------------------------------------------------------
    // pRot: glm::quat (phase 2 quat migration cluster #16). Default
    // identity is glm::quat(1, 0, 0, 0) (w, x, y, z).
    glm::quat pRot(1.f, 0.f, 0.f, 0.f);
    if ( are_parallel( abcNorm, apgNorm, 0.001f) )
    {
        if (dot(abcNorm, apgNorm) < 0.0f)
        {
            // we must be PI radians off ==> rotate by PI around agVec
            pRot = glm::angleAxis(F_PI, glm::normalize(glm::vec3(agVec)));
        }
        else
        {
            // we're done — pRot stays at identity
        }
    }
    else
    {
        // shortestArc -> glm::rotation. Both inputs must be normalized
        // (see cgRot construction in cluster #15 for the same pattern).
        pRot = glm::rotation(glm::normalize(glm::vec3(abcNorm)),
                             glm::normalize(glm::vec3(apgNorm)));
    }

    //-------------------------------------------------------------------------
    // compute twist rotation
    //-------------------------------------------------------------------------
    // twistRot: glm::quat (cluster #16). Axis-angle ctor replaced with
    // glm::angleAxis. agVec is normalized at line 197 (agLen = agVec.length(),
    // but we still need to normalize for glm::angleAxis since LL's
    // axis-angle ctor normalizes internally).
    glm::quat twistRot = glm::angleAxis(mTwist, glm::normalize(glm::vec3(agVec)));

#if DEBUG_JOINT_SOLVER
    LL_DEBUGS("JointSolver") << "abcNorm = " << abcNorm << LL_NEWLINE
                            << "apgNorm = " << apgNorm << LL_NEWLINE
                            << "pRot = " << LLQuaternion(pRot) << LL_NEWLINE
                            << "twist    : " << mTwist*180.0/F_PI << LL_NEWLINE
                            << "twistRot : " << LLQuaternion(twistRot) << LL_ENDL;
#endif

    //-------------------------------------------------------------------------
    // compute rotation of A
    //-------------------------------------------------------------------------
    // CRITICAL OPERAND-ORDER FLIP (cluster #16):
    //   LL form: aRot = cgRot * pRot * twistRot
    //     LL semantics = "apply cgRot first, then pRot, then twistRot"
    //   glm equivalent (same semantic, REVERSED operand order):
    //     aRot = twistRot * pRot * cgRot
    //     glm semantics: rightmost is applied first
    // Both expressions produce a quaternion that represents the same
    // total rotation. Test net pins this via bone-length preservation
    // and goal convergence.
    glm::quat aRot = twistRot * pRot * cgRot;

    //-------------------------------------------------------------------------
    // apply the rotations
    //-------------------------------------------------------------------------
    // Final composes: getWorldRotation() returns LLQuaternion (cluster #2
    // was setters-only). Bridge it to glm::quat at the call site so the
    // composition uses glm operand semantics. Original LL meaning was
    // "apply old worldrot, then apply (bRot|aRot)" — in glm operand order
    // that becomes "(bRot|aRot) * worldrot".
    mJointB->setWorldRotation( bRot * glm::quat(mJointB->getWorldRotation()) );
    mJointA->setWorldRotation( aRot * glm::quat(mJointA->getWorldRotation()) );
}


// End
