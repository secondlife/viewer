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

#include "assimp/Importer.hpp"      // C++ importer interface

const S32 BVH_PARSER_LINE_SIZE = 2048;
class LLDataPacker;


// LLAnimKey
struct LLAnimKey
{
    LLAnimKey()
	{
		mPos[0] = mPos[1] = mPos[2] = 0.0f;
		mRot[0] = mRot[1] = mRot[2] = 0.0f;
		mIgnorePos = false;
		mIgnoreRot = false;
	}

	F32	    mPos[3];
	F32	    mRot[3];        // in degrees
	bool	mIgnorePos;
	bool	mIgnoreRot;
};

typedef  std::vector<LLAnimKey> LLAnimKeyVector;

// LLAnimJoint
struct LLAnimJoint
{
	LLAnimJoint(const std::string &name)
	{
		mName = name;
		mIgnore = false;
		mIgnorePositions = false;
		mRelativePositionKey = false;
		mRelativeRotationKey = false;
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
	bool			mIgnore;                // Ignore this joint, don't add to animation
	bool			mIgnorePositions;
	bool			mRelativePositionKey;
	bool			mRelativeRotationKey;
	std::string		mOutName;
	std::string		mMergeParentName;
	std::string		mMergeChildName;
	char			mOrder[4];			/* Flawfinder: ignore */
    LLAnimKeyVector	mKeys;
	S32				mNumPosKeys;
	S32				mNumRotKeys;
	S32				mChildTreeMaxDepth;
	S32				mPriority;
	S32				mNumChannels;
};


struct LLAnimConstraint
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

typedef std::vector<LLAnimJoint*> LLAnimJointVector;
typedef std::vector<LLAnimConstraint> LLAnimConstraintVector;

// LLAnimTranslation
class LLAnimTranslation
{
public:
	LLAnimTranslation()
	{
		mIgnore = FALSE;
		mIgnorePositions = FALSE;
		mRelativePositionKey = FALSE;
		mRelativeRotationKey = FALSE;
		mPriorityModifier = 0;
	}

	std::string	mOutName;
	bool		mIgnore;                // Ignore this joint, don't add to animation
	bool		mIgnorePositions;
	bool		mRelativePositionKey;
	bool		mRelativeRotationKey;
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
		E_ST_BAD_ROOT,
        E_ST_INTERNAL_ERROR
	} ELoadStatus;


// LLBVHLoader
typedef std::map<std::string, LLAnimTranslation> LLAnimTranslationMap;
typedef std::map<std::string, std::string> joint_alias_map_t;

struct aiScene;
struct aiAnimation;
struct aiVectorKey;
struct aiQuatKey;

class LLBVHLoader
{
	friend class LLKeyframeMotion;
public:
	// Constructor
    LLBVHLoader();
	~LLBVHLoader();

    void loadAnimationData(const char* buffer,
        S32 &errorLine,
        const joint_alias_map_t& joint_alias_map,
        const std::string& full_path_filename,
        S32 transform_type);

    bool    isInitialized()     { return mInitialized; }
    F32     getDuration()       { return mDuration; }
    ELoadStatus getStatus()     { return mStatus; }
    S32     getOutputSize();                // calculates required size of output buffer

    bool serialize(LLDataPacker& dp);       // writes contents to datapacker


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

protected:
    //Create a new joint translation
    void makeTranslation(const std::string & LLAnimKey, const std::string & value);

    // Mangle coordinate systems
    void selectTransformMatrix(const std::string & in_name, LLAnimTranslation & translation);

	// Load the specified BVH file.
	// Returns status code.
//	ELoadStatus loadBVHFile(const char *buffer, char *error_text, S32 &error_line);

    // Debugging and testing
	void dumpJointInfo(LLAnimJointVector & joints);

	// Applies translations to BVH data loaded.
	void applyTranslations();

	// Returns the number of lines scanned.
	// Useful for error reporting.
	S32 getLineNumber() { return mLineNumber; }

	// flags redundant keyframe data to ignore in output
	void            optimize();

    // reset everything
	void            reset();

    // Assimp functions
    ELoadStatus     loadAssimp();                   // Load library and scene from file
    void            extractJointsFromAssimp();      // Extract data out of assimp scene into joints

    void            dumpAssimp();       // Diagnostic dump to file
    void            dumpAssimpAnimations(llofstream & data_stream);
    void            dumpAssimpAnimationChannels(aiAnimation * cur_animation, llofstream & data_stream);
    void            dumpAssimpAnimationVectorKeys(aiVectorKey * vector_keys, S32 count, llofstream & data_stream);
    void            dumpAssimpAnimationQuatKeys(aiQuatKey * quat_keys, S32 count, llofstream & data_stream);

	// Consumes one line of input from file.
	bool getLine(apr_file_t *fp);

	// parser state
	char		mLine[BVH_PARSER_LINE_SIZE];		/* Flawfinder: ignore */
	S32			mLineNumber;

	// parsed values
	S32					mNumFrames;
	F32					mFrameTime;
	LLAnimJointVector		mJoints;
	LLAnimConstraintVector	mConstraints;
	LLAnimTranslationMap	mTranslations;

	S32					mPriority;
	bool				mLoop;
	F32					mLoopInPoint;
	F32					mLoopOutPoint;
	F32					mEaseIn;
	F32					mEaseOut;
	S32					mHand;
	std::string			mEmoteName;
    S32                 mTransformType;     // Selects rotation transform applied to imported data
	bool				mInitialized;
	ELoadStatus			mStatus;

	// computed values
	F32	mDuration;

    // Assimp data
    std::string         mFilenameAndPath;   // Source file (bvh)
    Assimp::Importer *  mAssimpImporter;    // Main assimp library object
    const aiScene *     mAssimpScene;       // scene data after reading a file
};

#endif // LL_LLBVHLOADER_H
