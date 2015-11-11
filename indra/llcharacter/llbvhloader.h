/** 
 * @file llbvhloader.h
 * @brief Translates a BVH files to LindenLabAnimation format.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLBVHLOADER_H
#define LL_LLBVHLOADER_H

#include "v3math.h"
#include "m3math.h"
#include "llmath.h"
#include "llapr.h"
#include "llbvhconsts.h"
#include <deque>

const S32 BVH_PARSER_LINE_SIZE = 2048;
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
		mNumChannels = 3;
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
	S32				mNumChannels;
};


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
		mIgnorePositions = FALSE;
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

typedef enum e_load_status
	{
		E_ST_OK,
		E_ST_EOF,
		E_ST_NO_CONSTRAINT,
		E_ST_NO_FILE,
		E_ST_NO_HIER,
		E_ST_NO_JOINT,
		E_ST_NO_NAME,
		E_ST_NO_OFFSET,
		E_ST_NO_CHANNELS,
		E_ST_NO_ROTATION,
		E_ST_NO_AXIS,
		E_ST_NO_MOTION,
		E_ST_NO_FRAMES,
		E_ST_NO_FRAME_TIME,
		E_ST_NO_POS,
		E_ST_NO_ROT,
		E_ST_NO_XLT_FILE,
		E_ST_NO_XLT_HEADER,
		E_ST_NO_XLT_NAME,
		E_ST_NO_XLT_IGNORE,
		E_ST_NO_XLT_RELATIVE,
		E_ST_NO_XLT_OUTNAME,
		E_ST_NO_XLT_MATRIX,
		E_ST_NO_XLT_MERGECHILD,
		E_ST_NO_XLT_MERGEPARENT,
		E_ST_NO_XLT_PRIORITY,
		E_ST_NO_XLT_LOOP,
		E_ST_NO_XLT_EASEIN,
		E_ST_NO_XLT_EASEOUT,
		E_ST_NO_XLT_HAND,
		E_ST_NO_XLT_EMOTE,
		E_ST_BAD_ROOT
	} ELoadStatus;

//------------------------------------------------------------------------
// TranslationMap
//------------------------------------------------------------------------
typedef std::map<std::string, Translation> TranslationMap;

class LLBVHLoader
{
	friend class LLKeyframeMotion;
public:
	// Constructor
//	LLBVHLoader(const char* buffer);
    LLBVHLoader(const char* buffer, ELoadStatus &loadStatus, S32 &errorLine);
	LLBVHLoader(const char* buffer, ELoadStatus &loadStatus, S32 &errorLine, std::deque<std::string>& legal_joint_names);
	~LLBVHLoader();

/*	
	// Status Codes
	typedef const char *status_t;
	static const char *ST_OK;
	static const char *ST_EOF;
	static const char *ST_NO_CONSTRAINT;
	static const char *ST_NO_FILE;
	static const char *ST_NO_HIER;
	static const char *ST_NO_JOINT;
	static const char *ST_NO_NAME;
	static const char *ST_NO_OFFSET;
	static const char *ST_NO_CHANNELS;
	static const char *ST_NO_ROTATION;
	static const char *ST_NO_AXIS;
	static const char *ST_NO_MOTION;
	static const char *ST_NO_FRAMES;
	static const char *ST_NO_FRAME_TIME;
	static const char *ST_NO_POS;
	static const char *ST_NO_ROT;
	static const char *ST_NO_XLT_FILE;
	static const char *ST_NO_XLT_HEADER;
	static const char *ST_NO_XLT_NAME;
	static const char *ST_NO_XLT_IGNORE;
	static const char *ST_NO_XLT_RELATIVE;
	static const char *ST_NO_XLT_OUTNAME;
	static const char *ST_NO_XLT_MATRIX;
	static const char *ST_NO_XLT_MERGECHILD;
	static const char *ST_NO_XLT_MERGEPARENT;
	static const char *ST_NO_XLT_PRIORITY;
	static const char *ST_NO_XLT_LOOP;
	static const char *ST_NO_XLT_EASEIN;
	static const char *ST_NO_XLT_EASEOUT;
	static const char *ST_NO_XLT_HAND;
	static const char *ST_NO_XLT_EMOTE;
	static const char *ST_BAD_ROOT;
*/

	// Loads the specified translation table.
	ELoadStatus loadTranslationTable(const char *fileName);

    //Create a new joint alias
    void makeTranslation(std::string key, std::string value);
    
    // Loads joint aliases from XML file.
    ELoadStatus loadAliases(const char * filename);

	// Load the specified BVH file.
	// Returns status code.
	ELoadStatus loadBVHFile(const char *buffer, char *error_text, S32 &error_line);

	void dumpBVHInfo();

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

	ELoadStatus getStatus() { return mStatus; }

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
	ELoadStatus			mStatus;

	// computed values
	F32	mDuration;
};

#endif // LL_LLBVHLOADER_H
