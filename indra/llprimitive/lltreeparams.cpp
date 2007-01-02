/** 
 * @file lltreeparams.cpp
 * @brief implementation of the LLTreeParams class.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//////////////////////////////////////////////////////////////////////

#include "linden_common.h"

#include "llmath.h"

#include "lltreeparams.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


LLTreeParams::LLTreeParams()
{

//	llinfos << "TREE PARAMS INITIALIZED" << llendl;
	// init to basic something or other...
	mShape = SR_TEND_FLAME;
	mLevels = 1;
	mScale = 15;
	mScaleV = 0;

	mBaseSize = 0.3f;

	mRatio = 0.015f;
	mRatioPower = 1.3f;

	mLobes = 0;
	mLobeDepth = .1f;

	mFlare = 1.2f;
	mFlarePercentage = 0.1f;
	mFlareRes = 3;

	//mAttractionUp = .5f;

	mBaseSplits = 0;

	mScale0 = 2.0;		
	mScaleV0 = 0.0;

	// level 0

	// scaling
	mLength[0] = 1.0f;
	mLengthV[0] = 0;
	mTaper[0] = 1.0f;

	// stem splits
	mSegSplits[0] = 0.15f;
	mSplitAngle[0] = 15.0f;
	mSplitAngleV[0] = 10.0f;

	mVertices[0] = 5;
	
	// curvature
	mCurveRes[0] = 4;
	mCurve[0] = 0;
	mCurveV[0] = 25;
	mCurveBack[0] = 0;

	// level 1

	// scaling
	mLength[1] = .3f;
	mLengthV[1] = 0.05f;
	mTaper[1] = 1.0f;

	// angle params
	mDownAngle[0] = 60.0f;
	mDownAngleV[0] = 20.0f;
	mRotate[0] = 140.0f;
	mRotateV[0] = 0.0f;
	mBranches[0] = 35;

	mVertices[1] = 3; 

	// stem splits
	mSplitAngle[1] = 0.0f;
	mSplitAngleV[1] = 0.0f;
	mSegSplits[1] = 0.0f;

	// curvature
	mCurveRes[1] = 4;
	mCurve[1] = 0;
	mCurveV[1] = 0;
	mCurveBack[1] = 40;

	// level 2
	mLength[2] = .6f;
	mLengthV[2] = .1f;
	mTaper[2] = 1;

	mDownAngle[1] = 30;
	mDownAngleV[1] = 10;
	mRotate[1] = 140;
	mRotateV[1] = 0;

	mBranches[1] = 20;
	mVertices[2] = 3;

	mSplitAngle[2] = 0;
	mSplitAngleV[2] = 0;
	mSegSplits[2] = 0;

	mCurveRes[2] = 3;
	mCurve[2] = 10;
	mCurveV[2] = 150;
	mCurveBack[2] = 0;

	// level 3
	mLength[3] = .4f;
	mLengthV[3] = 0;
	mTaper[3] = 1;

	mDownAngle[2] = 45;
	mDownAngleV[2] = 10;
	mRotate[2] = 140;
	mRotateV[2] = 0;

	mBranches[2] = 5;
	mVertices[3] = 3;


	mSplitAngle[3] = 0;
	mSplitAngleV[3] = 0;
	mSegSplits[3] = 0;

	mCurveRes[3] = 2;
	mCurve[3] = 0;
	mCurveV[3] = 0;
	mCurveBack[3] = 0;

	mLeaves = 0;
	mLeafScaleX = 1.0f;
	mLeafScaleY = 1.0f;

	mLeafQuality = 1.25;
}

LLTreeParams::~LLTreeParams()
{

}

F32 LLTreeParams::ShapeRatio(EShapeRatio shape, F32 ratio) 
{
	switch (shape) {
		case (SR_CONICAL): 
			return (.2f + .8f * ratio);
		case (SR_SPHERICAL):
			return (.2f + .8f * sinf(F_PI*ratio));
		case (SR_HEMISPHERICAL):
			return (.2f + .8f * sinf(.5*F_PI*ratio));
		case (SR_CYLINDRICAL):
			return (1);
		case (SR_TAPERED_CYLINDRICAL):
			return (.5f + .5f * ratio);
		case (SR_FLAME):
			if (ratio <= .7f) {
				return ratio/.7f;
			} else {
				return ((1 - ratio)/.3f);
			}
		case (SR_INVERSE_CONICAL):
			return (1 - .8f * ratio);
		case (SR_TEND_FLAME):
			if (ratio <= .7) {
				return (.5f + .5f*(ratio/.7f));
			} else {
				return (.5f + .5f * (1 - ratio)/.3f);
			}
		case (SR_ENVELOPE):
			return 1;
		default:
			return 1;
	}
}
