/** 
 * @file lltreeparams.h
 * @brief Implementation of the LLTreeParams class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTREEPARAMS_H
#define LL_LLTREEPARAMS_H 

/* for information about formulas associated with each type
 * check the Weber + Penn paper
 */
typedef enum EShapeRatio { SR_CONICAL, SR_SPHERICAL, SR_HEMISPHERICAL, 
				SR_CYLINDRICAL, SR_TAPERED_CYLINDRICAL, SR_FLAME, 
				SR_INVERSE_CONICAL, SR_TEND_FLAME, SR_ENVELOPE};

const U32 TREE_BLOCK_SIZE = 16;

const U8 MAX_NUM_LEVELS = 4;

class LLTreeParams  
{
public:
	LLTreeParams();
	virtual ~LLTreeParams();

	static F32 ShapeRatio(EShapeRatio shape, F32 ratio);

public:

	// Variables with an asterick (*) cannot be modified without a re-instancing the
	// trunk/branches

	// Variables with an exclamation point (!) should probably not be modified outside and instead
	// be tied directly to the species

	// Variables with a tilde (~) should be tied to a range specified by the 
	// species type but still slightly controllable by the user

	// GENERAL

	//! determines length/radius of branches on tree -- ie: general 'shape' 
	EShapeRatio mShape;
	
	//! number of recursive branch levels...limit to MAX_NUM_LEVELS
	U8 mLevels;

	//~ percentage of trunk at bottom without branches 
	F32 mBaseSize;
	
	//~ the general scale + variance of tree
	F32 mScale, mScaleV;

	// general scale of tree
	F32 mScale0, mScaleV0;



	// LOBING

	//*! number of peaks in the radial distance about the perimeter
	U8 mLobes;
		//	even numbers = obvius symmetry ... use odd numbers
	
	//*! magnitude of the variations as a fraction of the radius
	F32 mLobeDepth;



	// FLARE

	//*! causes exponential expansion near base of trunk
	F32 mFlare;
		//	scales radius base by min 1 to '1 + flare'

	//*! percentage of the height of the trunk to flair -- likely less than baseSize
	F32 mFlarePercentage;

	//*! number of cross sections to make for the flair
	U8 mFlareRes;



	// LEAVES

	//~ number of leaves to make
	U8 mLeaves;

	//! scale of the leaves
	F32 mLeafScaleX, mLeafScaleY;

	// quality/density of leaves
	F32 mLeafQuality;

	// several params don't have level 0 values

	// BRANCHES 

	//~ angle away from parent
	F32 mDownAngle[MAX_NUM_LEVELS - 1];		
	F32 mDownAngleV[MAX_NUM_LEVELS - 1];
	
	//~ rotation around parent
	F32 mRotate[MAX_NUM_LEVELS - 1];
	F32 mRotateV[MAX_NUM_LEVELS - 1];

	//~ num branches to spawn
	U8 mBranches[MAX_NUM_LEVELS - 1];

	//~ fractional length of branch. 1 = same length as parent branch
	F32 mLength[MAX_NUM_LEVELS];
	F32 mLengthV[MAX_NUM_LEVELS];

	//!~ ratio and ratiopower determine radius/length
	F32 mRatio, mRatioPower;	

	//*! taper of branches
	F32 mTaper[MAX_NUM_LEVELS];
			// 0 - non-tapering cylinder
			// 1 - taper to a point
			// 2 - taper to a spherical end
			// 3 - periodic tapering (concatenated spheres)
	
	//! SEG SPLITTING
	U8 mBaseSplits;						//! num segsplits at first curve cross section of trunk
	F32 mSegSplits[MAX_NUM_LEVELS];		//~ splits per cross section. 1 = 1 split per section
	F32 mSplitAngle[MAX_NUM_LEVELS];	//~ angle that splits go from parent (tempered by height)
	F32 mSplitAngleV[MAX_NUM_LEVELS];	//~ variance of the splits

	// CURVE
	F32 mCurve[MAX_NUM_LEVELS];		//* general, 1-axis, overall curve of branch
	F32 mCurveV[MAX_NUM_LEVELS];	//* curve variance at each cross section from general overall curve
	U8 mCurveRes[MAX_NUM_LEVELS];	//* number of cross sections for curve
	F32 mCurveBack[MAX_NUM_LEVELS];	//* curveback is amount branch curves back towards 
	
	//  vertices per cross section
	U8 mVertices[MAX_NUM_LEVELS];

	// * no longer useful with pre-instanced branches
	// specifies upward tendency of branches. 
	//F32 mAttractionUp;	
		//	1 = each branch will slightly go upwards by the end of the branch
		//  >1 = branches tend to go upwards earlier in their length
	// pruning not implemented
	// Prune parameters
	//F32 mPruneRatio;
	//F32 mPruneWidth, mPruneWidthPeak;
	//F32 mPrunePowerLow, mPrunePowerHigh;


	// NETWORK MESSAGE DATA
	// Below is the outline for network messages regarding trees.  
	// The general idea is that a user would pick a general 'tree type' (the first variable)
	// and then several 'open ended' variables like 'branchiness' and 'leafiness'. 
	// The effect that each of these general user variables would then affect the actual
	// tree parameters (like # branches, # segsplits) in different ways depending on
	// the tree type selected.  Essentially, each tree type should have a formula
	// that expands the 'leafiness' and 'branchiness' user variables into actual
	// values for the tree parameters.  

	// These formulas aren't made yet and will certainly require some tuning.  The
	// estimates below for the # bits required seems like a good guesstimate.

	// VARIABLE			-	# bits (range)	-	VARIABLES AFFECTED
	// tree type		-	5 bits (32)		-	
	// branches			-	6 bits (64)		-	numBranches
	// splits			-	6 bits (64)		-	segsplits
	// leafiness		-	3 bits (8)		-	numLeaves
	// branch spread	-	5 bits (32)		-	splitAngle(V), rotate(V)
	// angle			-	5 bits (32)		-	downAngle(V)
	// branch length	-	6 bits (64)		-	branchlength(V)
	// randomness		-	7 bits (128)	-	percentage for randomness of the (V)'s
	// basesize			-	5 bits (32)		-	basesize
	
	// total			-	48 bits

	//U8 mNetSpecies;

};

#endif
