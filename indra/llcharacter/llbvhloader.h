/** 
 * @file llbvhloader.h
 * @brief Translates a BVH files to LindenLabAnimation format.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLBVHLOADER_H
#define LL_LLBVHLOADER_H

#include "v3math.h"
#include "m3math.h"
#include "llmath.h"
#include "llapr.h"

const S32 BVH_PARSER_LINE_SIZE = 2048;
const F32 MAX_ANIM_DURATION = 30.f;
class LLDataPacker;

//------------------------------------------------------------------------
// FileCloser
//------------------------------------------------------------------------
class FileCloser
{
public:
	FileCloser( apr_file_t *file )
	{
		mFile = file;
	}

	~FileCloser()
	{
		apr_file_close(mFile);
	}
protected:
	apr_file_t* mFile;
};


//------------------------------------------------------------------------
// Key
//------------------------------------------------------------------------
struct Key
{
	Key()
	{
		mPos[0] = mPos[1] = mPos[2] = 0.0f;
		mRot[0] = mRot[1] = mRot[2] = 0.0f;
		mIgnorePos = false;
		mIgnoreRot = false;
	}

	F32	mPos[3];
	F32	mRot[3];
	BOOL	mIgnorePos;
	BOOL	mIgnoreRot;
};


//------------------------------------------------------------------------
// KeyVector
//------------------------------------------------------------------------
typedef  std::vector<Key> KeyVector;

//------------------------------------------------------------------------
// Joint
//------------------------------------------------------------------------
struct Joint
{
	Joint(const char *name)
	{
		mName = name;
		mIgnore = FALSE;
		mIgnorePositions = FALSE;
		mRelativePositionKey = FALSE;
		mRelativeRotationKey = FALSE;
		mOutName = name;
		mOrder[0] = 'X';
		mOrder[1] = 'Y';
		mOrder[2] = 'Z';
		mOrder[3] = 0;
		mNumPosKeys = 0;
		mNumRotKeys = 0;
		mChildTreeMaxDepth = 0;
		mPriority = 0;
	}

	// Include aligned members first
	LLMatrix3		mFrameMatrix;
	LLMatrix3		mOffsetMatrix;
	LLVector3		mRelativePosition;
	//
	std::string		mName;
	BOOL			mIgnore;
	BOOL			mIgnorePositions;
	BOOL			mRelativePositionKey;
	BOOL			mRelativeRotationKey;
	std::string		mOutName;
	std::string		mMergeParentName;
	std::string		mMergeChildName;
	char			mOrder[4];			/* Flawfinder: ignore */
	KeyVector		mKeys;
	S32				mNumPosKeys;
	S32				mNumRotKeys;
	S32				mChildTreeMaxDepth;
	S32				mPriority;
};


typedef enum e_constraint_type
{
	CONSTRAINT_TYPE_POINT,
	CONSTRAINT_TYPE_PLANE
} EConstraintType;

struct Constraint
{
	char			mSourceJointName[16];		/* Flawfinder: ignore */
	char			mTargetJointName[16];		/* Flawfinder: ignore */
	S32				mChainLength;
	LLVector3		mSourceOffset;
	LLVector3		mTargetOffset;
	LLVector3		mTargetDir;
	F32				mEaseInStart;
	F32				mEaseInStop;
	F32				mEaseOutStart;
	F32				mEaseOutStop;
	EConstraintType mConstraintType;
};

//------------------------------------------------------------------------
// JointVector
//------------------------------------------------------------------------
typedef std::vector<Joint*> JointVector;

//------------------------------------------------------------------------
// ConstraintVector
//------------------------------------------------------------------------
typedef std::vector<Constraint> ConstraintVector;

//------------------------------------------------------------------------
// Translation
//------------------------------------------------------------------------
class Translation
{
public:
	Translation()
	{
		mIgnore = FALSE;
		mRelativePositionKey = FALSE;
		mRelativeRotationKey = FALSE;
		mPriorityModifier = 0;
	}

	std::string	mOutName;
	BOOL		mIgnore;
	BOOL		mIgnorePositions;
	BOOL		mRelativePositionKey;
	BOOL		mRelativeRotationKey;
	LLMatrix3	mFrameMatrix;
	LLMatrix3	mOffsetMatrix;
	LLVector3	mRelativePosition;
	std::string	mMergeParentName;
	std::string	mMergeChildName;
	S32			mPriorityModifier;
};

//------------------------------------------------------------------------
// TranslationMap
//------------------------------------------------------------------------
typedef std::map<std::string, Translation> TranslationMap;

class LLBVHLoader
{
	friend class LLKeyframeMotion;
public:
	// Constructor
	LLBVHLoader(const char* buffer);
	~LLBVHLoader();
	
	// Status Codes
	typedef char *Status;
	static char *ST_OK;
	static char *ST_EOF;
	static char *ST_NO_CONSTRAINT;
	static char *ST_NO_FILE;
	static char *ST_NO_HIER;
	static char *ST_NO_JOINT;
	static char *ST_NO_NAME;
	static char *ST_NO_OFFSET;
	static char *ST_NO_CHANNELS;
	static char *ST_NO_ROTATION;
	static char *ST_NO_AXIS;
	static char *ST_NO_MOTION;
	static char *ST_NO_FRAMES;
	static char *ST_NO_FRAME_TIME;
	static char *ST_NO_POS;
	static char *ST_NO_ROT;
	static char *ST_NO_XLT_FILE;
	static char *ST_NO_XLT_HEADER;
	static char *ST_NO_XLT_NAME;
	static char *ST_NO_XLT_IGNORE;
	static char *ST_NO_XLT_RELATIVE;
	static char *ST_NO_XLT_OUTNAME;
	static char *ST_NO_XLT_MATRIX;
	static char *ST_NO_XLT_MERGECHILD;
	static char *ST_NO_XLT_MERGEPARENT;
	static char *ST_NO_XLT_PRIORITY;
	static char *ST_NO_XLT_LOOP;
	static char *ST_NO_XLT_EASEIN;
	static char *ST_NO_XLT_EASEOUT;
	static char *ST_NO_XLT_HAND;
	static char *ST_NO_XLT_EMOTE;

	// Loads the specified translation table.
	Status loadTranslationTable(const char *fileName);

	// Load the specified BVH file.
	// Returns status code.
	Status loadBVHFile(const char *buffer, char *error_text, S32 &error_line);

	// Applies translations to BVH data loaded.
	void applyTranslations();

	// Returns the number of lines scanned.
	// Useful for error reporting.
	S32 getLineNumber() { return mLineNumber; }

	// returns required size of output buffer
	U32 getOutputSize();

	// writes contents to datapacker
	BOOL serialize(LLDataPacker& dp);

	// flags redundant keyframe data
	void optimize();

	void reset();

	F32 getDuration() { return mDuration; }

	BOOL isInitialized() { return mInitialized; }

	Status getStatus() { return mStatus; }

protected:
	// Consumes one line of input from file.
	BOOL getLine(apr_file_t *fp);

	// parser state
	char		mLine[BVH_PARSER_LINE_SIZE];		/* Flawfinder: ignore */
	S32			mLineNumber;

	// parsed values
	S32					mNumFrames;
	F32					mFrameTime;
	JointVector			mJoints;
	ConstraintVector	mConstraints;
	TranslationMap		mTranslations;

	S32					mPriority;
	BOOL				mLoop;
	F32					mLoopInPoint;
	F32					mLoopOutPoint;
	F32					mEaseIn;
	F32					mEaseOut;
	S32					mHand;
	std::string			mEmoteName;

	BOOL				mInitialized;
	Status				mStatus;
	// computed values
	F32	mDuration;
};

#endif // LL_LLBVHLOADER_H
