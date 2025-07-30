/**
 * @file llbvhloader.h
 * @brief Converts BVH motion capture files to Second Life's animation format.
 *
 * This system handles the complete pipeline for importing external animations into
 * Second Life, including parsing BVH (Biovision Hierarchy) files, applying joint
 * mappings and coordinate transformations, and optimizing the resulting keyframe data.
 *
 * The BVH loader supports:
 * - Standard BVH file format from motion capture systems and animation tools
 * - Joint name aliasing to map external skeletons to SL's avatar skeleton
 * - Translation files for coordinate system conversions and joint hierarchies
 * - Keyframe optimization to reduce animation file sizes
 * - Constraint systems for inverse kinematics and physics interactions
 * - Animation metadata (looping, priorities, hand poses, facial expressions)
 *
 * Usage workflow:
 * 1. User uploads BVH file through the animation preview floater (LLFloaterBVHPreview)
 * 2. LLBVHLoader constructor parses buffer, loads translation table from anim.ini
 * 3. Joint aliases are applied, keyframes are optimized to remove redundant data  
 * 4. Animation is validated against MAX_ANIM_DURATION limit before acceptance
 * 5. Final animation is serialized to SL's internal format for distribution
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

/// Maximum line length for BVH file parsing (includes joint names, keyframe data, etc.)
const S32 BVH_PARSER_LINE_SIZE = 2048;
class LLDataPacker;

/**
 * @brief RAII wrapper for automatically closing APR file handles.
 * 
 * Ensures that file handles are properly closed even if exceptions occur
 * during BVH file processing. Used internally by the BVH loader to manage
 * translation table files and other configuration files.
 * 
 * This is a simple utility class that follows RAII principles - the file
 * is closed automatically when the FileCloser goes out of scope.
 */
class FileCloser
{
public:
    /**
     * @brief Takes ownership of an APR file handle.
     * @param file The file handle to manage (must be valid)
     */
    FileCloser( apr_file_t *file )
    {
        mFile = file;
    }

    /**
     * @brief Automatically closes the file handle.
     */
    ~FileCloser()
    {
        apr_file_close(mFile);
    }
protected:
    apr_file_t* mFile;      /// File handle being managed
};


/**
 * @brief Single keyframe containing position and rotation data for a joint.
 * 
 * Represents one frame of animation data as parsed from a BVH file. Position
 * data is in the BVH coordinate system and must be converted to SL's coordinate
 * system during processing. Rotation data is stored as Euler angles in the
 * order specified by the joint definition.
 * 
 * The ignore flags are set during optimization to mark keyframes that can be
 * interpolated from surrounding frames, reducing the final animation size.
 */
struct Key
{
    /**
     * @brief Initializes a keyframe with zero position/rotation and no ignore flags.
     */
    Key()
    {
        mPos[0] = mPos[1] = mPos[2] = 0.0f;
        mRot[0] = mRot[1] = mRot[2] = 0.0f;
        mIgnorePos = false;
        mIgnoreRot = false;
    }

    F32 mPos[3];            /// Position offset in BVH coordinate system (X, Y, Z)
    F32 mRot[3];            /// Euler rotation angles in joint's specified order
    bool mIgnorePos;        /// True if this position keyframe can be optimized out
    bool mIgnoreRot;        /// True if this rotation keyframe can be optimized out
};

/// Vector of keyframes representing a complete animation sequence for one joint
typedef  std::vector<Key> KeyVector;

/**
 * @brief Complete joint definition including keyframe data and transformation parameters.
 * 
 * Represents a single joint (bone) from a BVH file with all its associated animation
 * data and metadata. Joints are processed through several stages:
 * 1. Parsed from BVH with original names and coordinate systems
 * 2. Mapped to SL joint names via translation tables
 * 3. Optimized to remove redundant keyframes
 * 4. Serialized to SL's internal animation format
 * 
 * The transformation matrices handle coordinate system conversions between
 * different animation tools and SL's avatar skeleton.
 */
struct Joint
{
    /**
     * @brief Initializes a joint with default values.
     * @param name The joint name from the BVH file
     */
    Joint(const char *name)
    {
        mName = name;
        mIgnore = false;
        mIgnorePositions = false;
        mRelativePositionKey = false;
        mRelativeRotationKey = false;
        mOutName = name;
        mOrder[0] = 'X';                /// Default Euler rotation order
        mOrder[1] = 'Y';
        mOrder[2] = 'Z';
        mOrder[3] = 0;
        mNumPosKeys = 0;
        mNumRotKeys = 0;
        mChildTreeMaxDepth = 0;
        mPriority = 0;
        mNumChannels = 3;               /// Default to rotation-only (3 channels)
    }

    // Include aligned members first
    LLMatrix3       mFrameMatrix;       /// Coordinate system conversion matrix
    LLMatrix3       mOffsetMatrix;      /// Additional transformation offset
    LLVector3       mRelativePosition;  /// Position offset from first frame
    
    // Joint identification and mapping
    std::string     mName;              /// Original joint name from BVH file
    std::string     mOutName;           /// Mapped SL joint name (e.g., "mPelvis")
    std::string     mMergeParentName;   /// Parent joint to merge rotations with
    std::string     mMergeChildName;    /// Child joint to merge rotations with
    
    // Processing flags
    bool            mIgnore;            /// True if joint should be excluded from output
    bool            mIgnorePositions;   /// True if position data should be ignored
    bool            mRelativePositionKey; /// True if positions are relative to first frame
    bool            mRelativeRotationKey; /// True if rotations are relative to first frame
    
    // BVH format data
    char            mOrder[4];          /// Euler rotation order (e.g., "XYZ", "ZXY")
    S32             mNumChannels;       /// 3=rotation only, 6=position+rotation
    
    // Animation data
    KeyVector       mKeys;              /// All keyframes for this joint
    S32             mNumPosKeys;        /// Count of position keyframes after optimization
    S32             mNumRotKeys;        /// Count of rotation keyframes after optimization
    
    // Hierarchy information  
    S32             mChildTreeMaxDepth; /// Maximum depth of child joints below this one
    S32             mPriority;          /// Animation priority for this joint
};


struct Constraint
{
    char            mSourceJointName[16];       /* Flawfinder: ignore */
    char            mTargetJointName[16];       /* Flawfinder: ignore */
    S32             mChainLength;
    LLVector3       mSourceOffset;
    LLVector3       mTargetOffset;
    LLVector3       mTargetDir;
    F32             mEaseInStart;
    F32             mEaseInStop;
    F32             mEaseOutStart;
    F32             mEaseOutStop;
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
        mIgnore = false;
        mIgnorePositions = false;
        mRelativePositionKey = false;
        mRelativeRotationKey = false;
        mPriorityModifier = 0;
    }

    std::string mOutName;
    bool        mIgnore;
    bool        mIgnorePositions;
    bool        mRelativePositionKey;
    bool        mRelativeRotationKey;
    LLMatrix3   mFrameMatrix;
    LLMatrix3   mOffsetMatrix;
    LLVector3   mRelativePosition;
    std::string mMergeParentName;
    std::string mMergeChildName;
    S32         mPriorityModifier;
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
    LLBVHLoader(const char* buffer, ELoadStatus &loadStatus, S32 &errorLine, std::map<std::string, std::string, std::less<>>& joint_alias_map );
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
    bool serialize(LLDataPacker& dp);

    // flags redundant keyframe data
    void optimize();

    void reset();

    F32 getDuration() { return mDuration; }

    bool isInitialized() { return mInitialized; }

    ELoadStatus getStatus() { return mStatus; }

protected:
    // Consumes one line of input from file.
    bool getLine(apr_file_t *fp);

    // parser state
    char        mLine[BVH_PARSER_LINE_SIZE];        /* Flawfinder: ignore */
    S32         mLineNumber;

    // parsed values
    S32                 mNumFrames;
    F32                 mFrameTime;
    JointVector         mJoints;
    ConstraintVector    mConstraints;
    TranslationMap      mTranslations;

    S32                 mPriority;
    bool                mLoop;
    F32                 mLoopInPoint;
    F32                 mLoopOutPoint;
    F32                 mEaseIn;
    F32                 mEaseOut;
    S32                 mHand;
    std::string         mEmoteName;

    bool                mInitialized;
    ELoadStatus         mStatus;

    // computed values
    F32 mDuration;
};

#endif // LL_LLBVHLOADER_H
