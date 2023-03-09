/**
 * @file llbvhloader.cpp
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
 * This library is distributed in the hope that it will be useful,7
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

#include "linden_common.h"

#include "llbvhloader.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "lldatapacker.h"
#include "lldir.h"
#include "llkeyframemotion.h"
#include "llquantize.h"
#include "llstl.h"
#include "llapr.h"
#include "llsdserialize.h"

#include <assimp/DefaultLogger.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


using namespace std;

#define INCHES_TO_METERS 0.02540005f

/// The .bvh does not have a formal spec, and different readers interpret things in their own way.
/// In OUR usage, frame 0 is used in optimization and is not considered to be part of the animation.
const S32 NUMBER_OF_IGNORED_FRAMES_AT_START = 1;
/// In our usage, the last frame is used only to indicate what the penultimate frame should be interpolated towards.
///  I.e., the animation only plays up to the start of the last frame. There is no hold or exptrapolation past that point..
/// Thus there are two frame of the total that do not contribute to the total running time of the animation.
const S32 NUMBER_OF_UNPLAYED_FRAMES = NUMBER_OF_IGNORED_FRAMES_AT_START + 1;

const F32 POSITION_KEYFRAME_THRESHOLD_SQUARED = 0.03f * 0.03f;
const F32 ROTATION_KEYFRAME_THRESHOLD = 0.01f;

const F32 POSITION_MOTION_THRESHOLD_SQUARED = 0.001f * 0.001f;
const F32 ROTATION_MOTION_THRESHOLD = 0.001f;

char gInFile[1024];		/* Flawfinder: ignore */
char gOutFile[1024];		/* Flawfinder: ignore */
/*
//------------------------------------------------------------------------
// Status Codes
//------------------------------------------------------------------------
const char *LLBVHLoader::ST_OK				= "Ok";
const char *LLBVHLoader::ST_EOF				= "Premature end of file.";
const char *LLBVHLoader::ST_NO_CONSTRAINT		= "Can't read constraint definition.";
const char *LLBVHLoader::ST_NO_FILE			= "Can't open BVH file.";
const char *LLBVHLoader::ST_NO_HIER			= "Invalid HIERARCHY header.";
const char *LLBVHLoader::ST_NO_JOINT			= "Can't find ROOT or JOINT.";
const char *LLBVHLoader::ST_NO_NAME			= "Can't get JOINT name.";
const char *LLBVHLoader::ST_NO_OFFSET			= "Can't find OFFSET.";
const char *LLBVHLoader::ST_NO_CHANNELS		= "Can't find CHANNELS.";
const char *LLBVHLoader::ST_NO_ROTATION		= "Can't get rotation order.";
const char *LLBVHLoader::ST_NO_AXIS			= "Can't get rotation axis.";
const char *LLBVHLoader::ST_NO_MOTION			= "Can't find MOTION.";
const char *LLBVHLoader::ST_NO_FRAMES			= "Can't get number of frames.";
const char *LLBVHLoader::ST_NO_FRAME_TIME		= "Can't get frame time.";
const char *LLBVHLoader::ST_NO_POS			= "Can't get position values.";
const char *LLBVHLoader::ST_NO_ROT			= "Can't get rotation values.";
const char *LLBVHLoader::ST_NO_XLT_FILE		= "Can't open translation file.";
const char *LLBVHLoader::ST_NO_XLT_HEADER		= "Can't read translation header.";
const char *LLBVHLoader::ST_NO_XLT_NAME		= "Can't read translation names.";
const char *LLBVHLoader::ST_NO_XLT_IGNORE		= "Can't read translation ignore value.";
const char *LLBVHLoader::ST_NO_XLT_RELATIVE	= "Can't read translation relative value.";
const char *LLBVHLoader::ST_NO_XLT_OUTNAME	= "Can't read translation outname value.";
const char *LLBVHLoader::ST_NO_XLT_MATRIX		= "Can't read translation matrix.";
const char *LLBVHLoader::ST_NO_XLT_MERGECHILD = "Can't get mergechild name.";
const char *LLBVHLoader::ST_NO_XLT_MERGEPARENT = "Can't get mergeparent name.";
const char *LLBVHLoader::ST_NO_XLT_PRIORITY	= "Can't get priority value.";
const char *LLBVHLoader::ST_NO_XLT_LOOP		= "Can't get loop value.";
const char *LLBVHLoader::ST_NO_XLT_EASEIN		= "Can't get easeIn values.";
const char *LLBVHLoader::ST_NO_XLT_EASEOUT	= "Can't get easeOut values.";
const char *LLBVHLoader::ST_NO_XLT_HAND		= "Can't get hand morph value.";
const char *LLBVHLoader::ST_NO_XLT_EMOTE		= "Can't read emote name.";
const char *LLBVHLoader::ST_BAD_ROOT        = "Illegal ROOT joint.";
*/



//------------------------------------------------------------------------
// bvhStringToOrder()
//
// XYZ order in BVH files must be passed to mayaQ() as ZYX.
// This function reverses the input string before passing it on
// to StringToOrder().
//------------------------------------------------------------------------
LLQuaternion::Order bvhStringToOrder( char *str )
{
	char order[4];		/* Flawfinder: ignore */
	order[0] = str[2];
	order[1] = str[1];
	order[2] = str[0];
	order[3] = 0;
	LLQuaternion::Order retVal = StringToOrder( order );
	return retVal;
}



//-----------------------------------------------------------------------------
// LLBVHLoader()
//-----------------------------------------------------------------------------

LLBVHLoader::LLBVHLoader() : mAssimpImporter(nullptr),
                             mAssimpScene(nullptr)
{
    reset();
}


void LLBVHLoader::loadAnimationData(const char* buffer,
                        S32 &errorLine,
                        const joint_alias_map_t& joint_alias_map,
                        const std::string& full_path_filename,
                        S32 transform_type)
{
    mFilenameAndPath = full_path_filename;
    mTransformType = transform_type;        // Passed in from globals for development

    errorLine = 0;
    std::string anim_ini("anim.ini");
	mStatus = loadTranslationTable(anim_ini);
	if (mStatus == E_ST_NO_XLT_FILE)
	{
		LL_WARNS("BVH") << "No translation table found." << LL_ENDL;
		return;
	}
	else if (mStatus != E_ST_OK)
	{
		LL_WARNS("BVH") << "ERROR in " << anim_ini << " [line: " << getLineNumber() << "] " << mStatus << LL_ENDL;
		errorLine = getLineNumber();
        return;
    }
    else
    {
        LL_INFOS("BVH") << "Loaded anim.ini with " << mTranslations.size() << " translations" << LL_ENDL;
    }

    // Recognize all names we've been told are legal from standard SL skeleton
    joint_alias_map_t::const_iterator iter;
    for (iter = joint_alias_map.begin(); iter != joint_alias_map.end(); iter++)
    {
        makeTranslation(iter->first, iter->second);
    }


// don't read BVH file!
//	char error_text[128];		/* Flawfinder: ignore */
//	S32 error_line;
//	mStatus = loadBVHFile(buffer, error_text, error_line); //Reads all joints in BVH file.
//    LL_DEBUGS("BVH") << "Joint data from file " << mFilenameAndPath << LL_ENDL;
//    dumpJointInfo(mJoints);

    mStatus = E_ST_OK;

    // Assimp test code
    loadAssimp();
    if (mAssimpScene)
    {
        dumpAssimp();
    }

	if (mStatus != E_ST_OK)
	{
		LL_WARNS("BVH") << "ERROR: [line: " << getLineNumber() << "] " << mStatus << LL_ENDL;
		errorLine = getLineNumber();
        return;
	}

	applyTranslations();    // Map between joints found in file and the aliased names.  Also detects mSkeletonType
	optimize();

	LL_DEBUGS("BVH") << "After translations and optimize from file " << mFilenameAndPath << LL_ENDL;
    dumpJointInfo(mJoints);      // to do - more useful before/after comparison

	mInitialized = true;
}


LLBVHLoader::~LLBVHLoader()
{
	std::for_each(mJoints.begin(),mJoints.end(),DeletePointer());
	mJoints.clear();
    reset();        // Clean up mAssimpImporter and mAssimpScene
}

//------------------------------------------------------------------------
// LLBVHLoader::loadTranslationTable() - reads anim.ini
//------------------------------------------------------------------------
ELoadStatus LLBVHLoader::loadTranslationTable(const std::string & fileName)
{
	//--------------------------------------------------------------------
	// open file
	//--------------------------------------------------------------------
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,fileName);

	LLAPRFile infile ;
	infile.open(path, LL_APR_R);
	apr_file_t *fp = infile.getFileHandle();
	if (!fp)
		return E_ST_NO_XLT_FILE;

	LL_INFOS("BVH") << "Loading translation table: " << fileName << LL_ENDL;

    //--------------------------------------------------------------------
	// load header
	//--------------------------------------------------------------------
	if ( ! getLine(fp) )
		return E_ST_EOF;
	if ( strnicmp(mLine, "Translations 1.0", 16) )
		return E_ST_NO_XLT_HEADER;


    // NOTE - some of this code appears to be gloriously broken.
    // SL shipped with nothing in the anim.ini
    // file, but now trying to make some name translations for it

	//--------------------------------------------------------------------
	// load data one line at a time
	//--------------------------------------------------------------------
	bool loadingGlobals = false;
	while ( getLine(fp) )
	{
		//----------------------------------------------------------------
		// check the 1st token on the line to determine if it's empty or a comment
		//----------------------------------------------------------------
		char token[128]; /* Flawfinder: ignore */
		if ( sscanf(mLine, " %127s", token) != 1 )	/* Flawfinder: ignore */
			continue;

		if (token[0] == '#')
			continue;

		//----------------------------------------------------------------
		// check if a [ALIAS aliasBoneName boneName] or [GLOBALS] was specified.
		//----------------------------------------------------------------
		if (token[0] == '[')
		{
			char name[128]; /* Flawfinder: ignore */
            if ( sscanf(mLine, " [%127s %*[^]]", name) != 1 )
				return E_ST_NO_XLT_NAME;

			if (stricmp(name, "GLOBALS")==0)
			{
				loadingGlobals = true;
				continue;
			}
            else if (stricmp(name, "ALIAS") == 0)
            {
                loadingGlobals = false;

                char alias_name[128];       /* Flawfinder: ignore */
                char joint_name[128];       /* Flawfinder: ignore */
                if (sscanf(mLine, " [ALIAS %127s %127[^]]s ]", &alias_name[0], &joint_name[0]) != 2)
                {
                    LL_WARNS("BVH") << "anim.ini line error: " << mLine << LL_ENDL;
                    return E_ST_NO_XLT_NAME;
                }
                LL_DEBUGS("BVH") << "Animation joint anim.ini alias: " << std::string(alias_name) << " to " << std::string(joint_name) << LL_ENDL;
                makeTranslation(alias_name, joint_name);
                continue;
            }
        }

		//----------------------------------------------------------------
		// check for optional emote
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "emote")==0)
		{
			char emote_str[1024];	/* Flawfinder: ignore */
			if ( sscanf(mLine, " %*s = %1023s", emote_str) != 1 )	/* Flawfinder: ignore */
				return E_ST_NO_XLT_EMOTE;

			mEmoteName.assign( emote_str );
            LL_DEBUGS("BVH") << "Emote name: " << mEmoteName.c_str() << LL_ENDL;
			continue;
		}


		//----------------------------------------------------------------
		// check for global priority setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "priority")==0)
		{
			S32 priority;
			if ( sscanf(mLine, " %*s = %d", &priority) != 1 )
				return E_ST_NO_XLT_PRIORITY;

			mPriority = priority;
            LL_DEBUGS("BVH") << "Priority set to " << mPriority << LL_ENDL;
			continue;
		}

		//----------------------------------------------------------------
		// check for global loop setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "loop")==0)
		{
			char trueFalse[128];		/* Flawfinder: ignore */
			trueFalse[0] = '\0';

            // At this point we don't have mDuration, so mLoopInPoint and mLoopOutPoint
            // are read and set between 0 and 1
            mLoopInPoint = 0.f;
            mLoopOutPoint = 1.f;

			if ( sscanf(mLine, " %*s = %f %f", &mLoopInPoint, &mLoopOutPoint) == 2 )
			{
                mLoopInPoint = llclampf(mLoopInPoint);                          // Clamp between 0 and 1
                mLoopOutPoint = llclamp(mLoopOutPoint, mLoopInPoint, 1.f);
                mLoop = (mLoopOutPoint > mLoopInPoint);
			}
			else if ( sscanf(mLine, " %*s = %127s", trueFalse) == 1 )	/* Flawfinder: ignore */
			{
				mLoop = (LLStringUtil::compareInsensitive(trueFalse, "true")==0);
			}
			else
			{
				return E_ST_NO_XLT_LOOP;
			}

			continue;
		}

		//----------------------------------------------------------------
		// check for global easeIn setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "easein")==0)
		{
			F32 duration;
			char type[128];	/* Flawfinder: ignore */
			if ( sscanf(mLine, " %*s = %f %127s", &duration, type) != 2 )	/* Flawfinder: ignore */
				return E_ST_NO_XLT_EASEIN;

			mEaseIn = duration;
			continue;
		}

		//----------------------------------------------------------------
		// check for global easeOut setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "easeout")==0)
		{
			F32 duration;
			char type[128];		/* Flawfinder: ignore */
			if ( sscanf(mLine, " %*s = %f %127s", &duration, type) != 2 )	/* Flawfinder: ignore */
				return E_ST_NO_XLT_EASEOUT;

			mEaseOut = duration;
			continue;
		}

		//----------------------------------------------------------------
		// check for global handMorph setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "hand")==0)
		{
			S32 handMorph;
			if (sscanf(mLine, " %*s = %d", &handMorph) != 1)
				return E_ST_NO_XLT_HAND;

			mHand = handMorph;
			continue;
		}

		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "constraint")==0)
		{
			LLAnimConstraint constraint;

			// try reading optional target direction
			if(sscanf( /* Flawfinder: ignore */
				mLine,
				" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f %f %f %f", 
				&constraint.mChainLength,
				&constraint.mEaseInStart,
				&constraint.mEaseInStop,
				&constraint.mEaseOutStart,
				&constraint.mEaseOutStop,
				constraint.mSourceJointName,
				&constraint.mSourceOffset.mV[VX],
				&constraint.mSourceOffset.mV[VY],
				&constraint.mSourceOffset.mV[VZ],
				constraint.mTargetJointName,
				&constraint.mTargetOffset.mV[VX],
				&constraint.mTargetOffset.mV[VY],
				&constraint.mTargetOffset.mV[VZ],
				&constraint.mTargetDir.mV[VX],
				&constraint.mTargetDir.mV[VY],
				&constraint.mTargetDir.mV[VZ]) != 16)
			{
				if(sscanf( /* Flawfinder: ignore */
					mLine,
					" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f", 
					&constraint.mChainLength,
					&constraint.mEaseInStart,
					&constraint.mEaseInStop,
					&constraint.mEaseOutStart,
					&constraint.mEaseOutStop,
					constraint.mSourceJointName,
					&constraint.mSourceOffset.mV[VX],
					&constraint.mSourceOffset.mV[VY],
					&constraint.mSourceOffset.mV[VZ],
					constraint.mTargetJointName,
					&constraint.mTargetOffset.mV[VX],
					&constraint.mTargetOffset.mV[VY],
					&constraint.mTargetOffset.mV[VZ]) != 13)
				{
					return E_ST_NO_CONSTRAINT;
				}
			}
			else
			{
				// normalize direction
				if (!constraint.mTargetDir.isExactlyZero())
				{
					constraint.mTargetDir.normVec();
				}

			}

			constraint.mConstraintType = CONSTRAINT_TYPE_POINT;
			mConstraints.push_back(constraint);
			continue;
		}

		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "planar_constraint")==0)
		{
			LLAnimConstraint constraint;

			// try reading optional target direction
			if(sscanf( /* Flawfinder: ignore */
				mLine,
				" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f %f %f %f", 
				&constraint.mChainLength,
				&constraint.mEaseInStart,
				&constraint.mEaseInStop,
				&constraint.mEaseOutStart,
				&constraint.mEaseOutStop,
				constraint.mSourceJointName,
				&constraint.mSourceOffset.mV[VX],
				&constraint.mSourceOffset.mV[VY],
				&constraint.mSourceOffset.mV[VZ],
				constraint.mTargetJointName,
				&constraint.mTargetOffset.mV[VX],
				&constraint.mTargetOffset.mV[VY],
				&constraint.mTargetOffset.mV[VZ],
				&constraint.mTargetDir.mV[VX],
				&constraint.mTargetDir.mV[VY],
				&constraint.mTargetDir.mV[VZ]) != 16)
			{
				if(sscanf( /* Flawfinder: ignore */
					mLine,
					" %*s = %d %f %f %f %f %15s %f %f %f %15s %f %f %f",
					&constraint.mChainLength,
					&constraint.mEaseInStart,
					&constraint.mEaseInStop,
					&constraint.mEaseOutStart,
					&constraint.mEaseOutStop,
					constraint.mSourceJointName,
					&constraint.mSourceOffset.mV[VX],
					&constraint.mSourceOffset.mV[VY],
					&constraint.mSourceOffset.mV[VZ],
					constraint.mTargetJointName,
					&constraint.mTargetOffset.mV[VX],
					&constraint.mTargetOffset.mV[VY],
					&constraint.mTargetOffset.mV[VZ]) != 13)
				{
					return E_ST_NO_CONSTRAINT;
				}
			}
			else
			{
				// normalize direction
				if (!constraint.mTargetDir.isExactlyZero())
				{
					constraint.mTargetDir.normVec();
				}
			}

			constraint.mConstraintType = CONSTRAINT_TYPE_PLANE;
			mConstraints.push_back(constraint);
			continue;
		}
	}

	infile.close() ;
	return E_ST_OK;
}

static S32 test_transform_chain = 3;

static std::string test_joint_chain[] =
{
    std::string("RightArm"),
    std::string("RightForeArm"),
    std::string("RightHand"),
    std::string("")         // end marker
};

static bool joint_in_test(const std::string & joint_name, const std::string & alias_name)
{
    if (test_transform_chain < 0)       // set -1 to transform all joints (original behavior)
        return true;

    for (S32 test_index = 0; test_index < test_transform_chain; test_index++)
    {
        if (test_joint_chain[test_index].length() == 0)
            break;
        if (joint_name == test_joint_chain[test_index] ||
            alias_name == test_joint_chain[test_index])
            return true;
    }
    return false;
}

void LLBVHLoader::selectTransformMatrix(const std::string & in_name, LLAnimTranslation & translation)
{   // Select the transform for coordinate system matching

    // Transform matrix numbers can be identified by a basic index starting at zero,
    // or a 3 digit number like the default identity 421.   This digit is the value
    // of the 3 vectors in the matrix if you read XYZ as 100, 010, 001, etc

    // 421 No change as default
    LLVector3 vect1(1, 0, 0);   // 4  x -> x
    LLVector3 vect2(0, 1, 0);   // 2  y -> y
    LLVector3 vect3(0, 0, 1);   // 1  z -> z

    static S32 test_transform = 999;

    S32 use_transform = mTransformType;
    if (mSkeletonType == "mixamo")
    {
        if (joint_in_test(in_name, translation.mOutName))
        {
            use_transform = test_transform;
        }
    }
    else if (use_transform != 2 && use_transform != 214)
    {
        LL_WARNS("BVH") << "Using non-standard transform matrix " << use_transform << " for importing SL skeleton animation, this may cause issues."
            << "  Try changing debug setting AnimationImportTransform to the default 2" << LL_ENDL;
    }

    // development code - clean this up when fully working
    LL_DEBUGS("BVH") << "transform matrix " << use_transform << " for " << translation.mOutName << LL_ENDL;
    switch (use_transform)
    {
        case 0:     // 421 default - no change
        case 421:
            break;
        case 1:     // 412 test
        case 412:
            vect1.set(1, 0, 0); // x -> x
            vect2.set(0, 0, 1); // z -> y
            vect3.set(0, 1, 0); // y -> z
            break;
        case 2:     // 214 Original bvh importer
        case 214:
            vect1.set(0, 1, 0); // y -> x
            vect2.set(0, 0, 1); // z -> y
            vect3.set(1, 0, 0); // x -> z
            break;
        case 3:     // 241 Mixamo test 1
        case 241:
            vect1.set(0, 1, 0); // y -> x
            vect2.set(1, 0, 0); // x -> y
            vect3.set(0, 0, 1); // z -> z
            break;
        case 4:     // 124
        case 124:
            vect1.set(0, 0, 1); // z -> x
            vect2.set(0, 1, 0); // y -> y
            vect3.set(1, 0, 0); // x -> z
            break;
        case 5:     // 142
        case 142:
            vect1.set(0, 0, 1); // z -> x
            vect2.set(1, 0, 0); // x -> y
            vect3.set(0, 1, 0); // y -> z
            break;

        // experiments
        case 6:     // 444  x everywhere
        case 444:
            vect1.set(1, 0, 0); // x -> x
            vect2.set(1, 0, 0); // x -> y
            vect3.set(1, 0, 0); // x -> z
            break;
        case 7:     // 222  y everywhere
        case 222:
            vect1.set(0, 1, 0); // x -> x
            vect2.set(0, 1, 0); // x -> y
            vect3.set(0, 1, 0); // x -> z
            break;
        case 8:     // 111  z everywhere
        case 111:
            vect1.set(0, 0, 1); // z -> x
            vect2.set(0, 0, 1); // z -> y
            vect3.set(0, 0, 1); // z -> z
            break;
        case 999:   // 000
            vect1.set(0, 0, 0); // zero it out
            vect2.set(0, 0, 0); // zero it out
            vect3.set(0, 0, 0); // zero it out
            break;
        default:
            break;
    }

    translation.mFrameMatrix.setRows(vect1, vect2, vect3);
}

void LLBVHLoader::makeTranslation(const std::string & alias_name, const std::string & joint_name)
{

    // Access existing value or create new - uses []'s implicit call to ctor.
    LLAnimTranslation &newTrans = mTranslations[ alias_name ];
    if (newTrans.mOutName.length() > 0)
    {   // Don't expect to have one already - clean up anim.ini?
        LL_WARNS("BVH") << "Replacing joint translation " << alias_name
            << ", existing output name " << newTrans.mOutName << ", new " << joint_name
            << ".   Check anim.ini ?" << LL_ENDL;
    }

    newTrans.mOutName = joint_name;
    // Note - can't call selectTransformMatrix() here because mSkeletonType isn't detected yet

    if (joint_name == "mPelvis")
    {
        newTrans.mRelativePositionKey = true;
        newTrans.mRelativeRotationKey = true;
    }
    else if (joint_name == "ignore")
    {   // This won't be included in output
        newTrans.mIgnore = true;
        newTrans.mIgnorePositions = true;
    }
}


// hack data
void LLBVHLoader::tweakJointData(LLAnimJointVector & joints)
{
    static bool enable_joint_tweaks = true;

    if (mSkeletonType != "mixamo")
        return;

    if (enable_joint_tweaks)
    {
        for (U32 j = 0; j < joints.size(); j++)
        {
            LLAnimJoint *joint = joints[j];
            if (joint_in_test(joint->mName, joint->mOutName))
            {
                LL_DEBUGS("BVH") << "No debug tweaks for " << joint->mName << "/" << joint->mOutName << " data" << LL_ENDL;
            }
            else
            {
                LL_DEBUGS("BVH") << "Zeroing all rotations for " << joint->mName << "/" << joint->mOutName << LL_ENDL;
                for (S32 i = 0; i < mNumFrames; i++)        // This is a LOT of data
                {
                    if (i < joint->mKeys.size())    // Check this in case file load failed.
                    {
                        LLAnimKey &key = joint->mKeys[i];
                        key.mRot[0] = 0;
                        key.mRot[1] = 0;
                        key.mRot[2] = 0;
                    }
                }
            }
        }
    }
}

void LLBVHLoader::dumpJointInfo(LLAnimJointVector & joints)
{
    LL_DEBUGS("BVHDump") << "mNumFrames " << mNumFrames << LL_ENDL;
    LL_DEBUGS("BVHDump") << "mFrameTime " << mFrameTime << LL_ENDL;
    LL_DEBUGS("BVHDump") << "Num joints " << mJoints.size() << LL_ENDL;

    for (U32 j = 0; j < joints.size(); j++)
	{
		LLAnimJoint *joint = joints[j];
        std::string joint_name_info(joint->mName);
        if (joint_name_info != joint->mOutName)
        {
            joint_name_info.append(" -> ");
            joint_name_info.append(joint->mOutName);
        }
		LL_DEBUGS("BVHDump") << joint_name_info << " : "
            << (joint->mIgnore ? " Ignored " : "")
            << (joint->mIgnorePositions ? " Ignore Positions " : "")
            << (joint->mRelativePositionKey ? " Relative Positions " : "")
            << (joint->mRelativeRotationKey ? " Relative Rotations " : "")
            << " order " << std::string(&joint->mOrder[0])
            << ", " << joint->mNumPosKeys << " positions, "
            << joint->mNumRotKeys << " rotations, "
            << "child tree max depth " << joint->mChildTreeMaxDepth
            << ", priority " << joint->mPriority
            << ", channels " << joint->mNumChannels
            << LL_ENDL;

        bool dump_all_keys = true;
        S32 skipped_joints = 0;
		for (S32 i = 0; i < mNumFrames; i++)        // This is a LOT of data
		{
            if (i < joint->mKeys.size()) // Check this in case file load failed.
            {
                LLAnimKey &prevkey = joint->mKeys[llmax(i-1,0)];
                LLAnimKey &key = joint->mKeys[i];
                if (dump_all_keys ||
                    (i == 0) ||
                    (key.mPos[0] != prevkey.mPos[0]) ||
                    (key.mPos[1] != prevkey.mPos[1]) ||
                    (key.mPos[2] != prevkey.mPos[2]) ||
                    (key.mRot[0] != prevkey.mRot[0]) ||
                    (key.mRot[1] != prevkey.mRot[1]) ||
                    (key.mRot[2] != prevkey.mRot[2]) ||
                    (key.mIgnorePos != prevkey.mIgnorePos) ||
                    (key.mIgnoreRot != prevkey.mIgnoreRot) )
                {
                    if (joint->mIgnorePositions)
                    {   // no position, just rotation data
                        LL_DEBUGS("BVHDump") << "  Frame " << i
                            << (key.mIgnoreRot ? " Ignore Rot " : "")
                            << " rot " << key.mRot[0] << "," << key.mRot[1] << "," << key.mRot[2] << LL_ENDL;
                    }
                    else
                    {   // Have both rotation and position
                        LL_DEBUGS("BVHDump") << "  Frame " << i
                            << (key.mIgnorePos ? " Ignore Pos " : "")
                            << (key.mIgnoreRot ? " Ignore Rot " : "")
                            << " pos " << key.mPos[0] << "," << key.mPos[1] << "," << key.mPos[2]
                            << " rot " << key.mRot[0] << "," << key.mRot[1] << "," << key.mRot[2] << LL_ENDL;
                    }
                }
            }
            else
            {
                skipped_joints++;
            }
		}
        if (skipped_joints > 0)
        {
            LL_DEBUGS("BVHDump") << "  Skipped " << skipped_joints << " identical joints for " << joint->mName << "/" << joint->mOutName << LL_ENDL;
        }
    }
}

//------------------------------------------------------------------------
// LLBVHLoader::loadBVHFile()
//------------------------------------------------------------------------
#if (0)
ELoadStatus LLBVHLoader::loadBVHFile(const char *buffer, char* error_text, S32 &err_line)
{
	std::string line;

	err_line = 0;
	error_text[127] = '\0';

	std::string str(buffer);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\r\n");
	tokenizer tokens(str, sep);
	tokenizer::iterator iter = tokens.begin();

	mLineNumber = 0;
	mJoints.clear();

	std::vector<S32> parent_joints;

	//--------------------------------------------------------------------
	// consume  hierarchy
	//--------------------------------------------------------------------
	if (iter == tokens.end())
		return E_ST_EOF;
	line = (*(iter++));
	err_line++;

	if ( !strstr(line.c_str(), "HIERARCHY") )
	{
//		LL_INFOS("BVH") << line << LL_ENDL;
		return E_ST_NO_HIER;
	}

	// consume joints until everything makes sense
	while (true)
	{
		// get next line
		if (iter == tokens.end())
			return E_ST_EOF;
		line = (*(iter++));
		err_line++;

		// consume }
		if ( strstr(line.c_str(), "}") )
		{   // end of joint definition
			if (parent_joints.size() > 0)
			{   // step back one level of parent joint chain
				parent_joints.pop_back();
			}
			continue;
		}

		// if MOTION, break out - done with joint HIERARCHY
		if ( strstr(line.c_str(), "MOTION") )
			break;

		// Should be either ROOT or JOINT or End Site
		if ( strstr(line.c_str(), "ROOT") )
		{
		}
		else if ( strstr(line.c_str(), "JOINT") )
		{
		}
		else if ( strstr(line.c_str(), "End Site") )
		{   // first skip OFFSET
			iter++; // {
			iter++; //     OFFSET
			iter++; // }
			S32 depth = 0;
			for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
			{
				LLAnimJoint *joint = mJoints[parent_joints[j]];
				if (depth > joint->mChildTreeMaxDepth)
				{
					joint->mChildTreeMaxDepth = depth;
				}
				depth++;
			}
			continue;
		}
		else
		{
			strncpy(error_text, line.c_str(), 127);	/* Flawfinder: ignore */
			return E_ST_NO_JOINT;
		}

		// ROOT or JOINT:  Get the joint name
		char jointName[80];	/* Flawfinder: ignore */
		if ( sscanf(line.c_str(), "%*s %79s", jointName) != 1 )	/* Flawfinder: ignore */
		{
			strncpy(error_text, line.c_str(), 127);	/* Flawfinder: ignore */
			return E_ST_NO_NAME;
		}

		// Require the root joint be "hip" or an recognized alias
        if (mJoints.size() == 0 )
        {
            // Use 'Hips' to identify Mixamo skeleton bvh file.
            if (strcmp(jointName, "Hips") == 0)
            {
                LL_INFOS("BVH") << "Detected Mixamo Hips root name in " << mFilenameAndPath << LL_ENDL;
            }

            //The root joint of the BVH file must be hip (mPelvis) or an alias of mPelvis.
            const char* FORCED_ROOT_NAME = "hip";

            LLAnimTranslationMap::iterator hip_joint = mTranslations.find( FORCED_ROOT_NAME );
            LLAnimTranslationMap::iterator root_joint = mTranslations.find( jointName );
            if ( hip_joint == mTranslations.end() || root_joint == mTranslations.end() || root_joint->second.mOutName != hip_joint->second.mOutName )
            {
                strncpy(error_text, line.c_str(), 127);	/* Flawfinder: ignore */
                return E_ST_BAD_ROOT;
            }
        }

		// add a set of keyframes for this joint
		mJoints.push_back( new LLAnimJoint( jointName ) );
		LLAnimJoint *joint = mJoints.back();
		LL_DEBUGS("BVH") << "Created joint " << jointName << " at index " << mJoints.size()-1 
            << " parent depth " << (S32)parent_joints.size() << LL_ENDL;

		S32 depth = 1;
		for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
		{
			LLAnimJoint *pjoint = mJoints[parent_joints[j]];
			LL_DEBUGS("BVH") << "- ancestor is " << pjoint->mName << " at depth " << j << LL_ENDL;
			if (depth > pjoint->mChildTreeMaxDepth)
			{
				pjoint->mChildTreeMaxDepth = depth;
			}
			depth++;
		}

		// Get next line
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		// it must be {
		if ( !strstr(line.c_str(), "{") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_OFFSET;
		}
		else
		{
			parent_joints.push_back((S32)mJoints.size() - 1);
		}

		// Get next line
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		// it must be OFFSET - ignore it
		if ( !strstr(line.c_str(), "OFFSET") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_OFFSET;
		}

		// get next line
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		// it must be CHANNELS
		if ( !strstr(line.c_str(), "CHANNELS") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_CHANNELS;
		}

        // Animating position (via mNumChannels = 6) is only supported for mPelvis.
        // CHANNELS 6 Xposition Yposition Zposition Yrotation Xrotation Zrotation
        // CHANNELS 3 Yrotation Xrotation Zrotation
        int res = sscanf(line.c_str(), " CHANNELS %d", &joint->mNumChannels);
		if ( res != 1 )
		{
			// Assume default if not otherwise specified.
			if (mJoints.size()==1)
			{
				joint->mNumChannels = 6;
			}
			else
			{
				joint->mNumChannels = 3;
			}
		}

		// get rotation order
		const char *p = line.c_str();
		for (S32 i=0; i<3; i++)
		{
			p = strstr(p, "rotation");
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
				return E_ST_NO_ROTATION;
			}

            // Extract axis from char before 'rotation':    Yrotation Xrotation Zrotation
			const char axis = *(p - 1);
			if ((axis != 'X') && (axis != 'Y') && (axis != 'Z'))
			{
				strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
				return E_ST_NO_AXIS;
			}

			joint->mOrder[i] = axis;

			p += 8;     // skip past the current "rotation" for the next strstr() scan
		}
	}   // end loop scanning "HIERARCHY"

	// Consume MOTION
	if ( !strstr(line.c_str(), "MOTION") )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_MOTION;
	}

	// Get number of frames
	if (iter == tokens.end())
	{
		return E_ST_EOF;
	}
	line = (*(iter++));
	err_line++;

	if ( !strstr(line.c_str(), "Frames:") )
	{   // Must have "Frames:"
		strncpy(error_text, line.c_str(), 127);	/*Flawfinder: ignore*/
		return E_ST_NO_FRAMES;
	}

	if ( sscanf(line.c_str(), "Frames: %d", &mNumFrames) != 1 )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_FRAMES;
	}

	// Get frame time
	if (iter == tokens.end())
	{
		return E_ST_EOF;
	}
	line = (*(iter++));
	err_line++;

	if ( !strstr(line.c_str(), "Frame Time:") )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_FRAME_TIME;
	}

	if ( sscanf(line.c_str(), "Frame Time: %f", &mFrameTime) != 1 )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_FRAME_TIME;
	}

	// If the file only supplies one animation frame (after the ignored reference frame 0), hold for mFrameTime.
	// If the file supples exactly one total frame, it isn't clear if that is a pose or reference frame, and the
	// behavior is not defined. In this case, retain historical undefined behavior.
	mDuration = llmax((F32)(mNumFrames - NUMBER_OF_UNPLAYED_FRAMES), 1.0f) * mFrameTime;
	if (!mLoop)
	{
		mLoopOutPoint = mDuration;
	}

	//--------------------------------------------------------------------
	// load frames
	//--------------------------------------------------------------------
	for (S32 i=0; i<mNumFrames; i++)
	{
		// get next line
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		// Split line into a collection of floats.
		std::deque<F32> floats;
		boost::char_separator<char> whitespace_sep("\t ");
		tokenizer float_tokens(line, whitespace_sep);
		tokenizer::iterator float_token_iter = float_tokens.begin();
		while (float_token_iter != float_tokens.end())
		{
            try
            {
                F32 val = boost::lexical_cast<float>(*float_token_iter);
                floats.push_back(val);
            }
            catch (const boost::bad_lexical_cast&)
            {
                LL_WARNS("BVH") << "Expected floating point data but found " << std::string(*float_token_iter)
                    << " in frame " << i << LL_ENDL;
                strncpy(error_text, line.c_str(), 127);	/*Flawfinder: ignore*/
				return E_ST_NO_POS;
            }
            float_token_iter++;
		}
		LL_DEBUGS("BVHFloats") << "Got " << floats.size() << " floats " << LL_ENDL;     // This is a lot of useless log data
		for (U32 j=0; j<mJoints.size(); j++)
		{
			LLAnimJoint *joint = mJoints[j];
			joint->mKeys.push_back( Key() );
			LLAnimKey &key = joint->mKeys.back();

			if (floats.size() < joint->mNumChannels)
			{
                LL_WARNS("BVH") << "Expected " << joint->mNumChannels << " BVH channel data but only have " << floats.size()
                    << " in frame " << i << LL_ENDL;
                strncpy(error_text, line.c_str(), 127);	/*Flawfinder: ignore*/
				return E_ST_NO_POS;
			}

			// assume either numChannels == 6, in which case we have pos + rot,
			// or numChannels == 3, in which case we have only rot.
			if (joint->mNumChannels == 6)
			{
				key.mPos[0] = floats.front(); floats.pop_front();
				key.mPos[1] = floats.front(); floats.pop_front();
				key.mPos[2] = floats.front(); floats.pop_front();
			}
			key.mRot[ joint->mOrder[0]-'X' ] = floats.front(); floats.pop_front();
			key.mRot[ joint->mOrder[1]-'X' ] = floats.front(); floats.pop_front();
			key.mRot[ joint->mOrder[2]-'X' ] = floats.front(); floats.pop_front();
		}
	}

	return E_ST_OK;
}
#endif

//------------------------------------------------------------------------
// LLBVHLoader::applyTranslation()
//------------------------------------------------------------------------
void LLBVHLoader::applyTranslations()
{
    // First calculate the translations mFrameMatrix values
    for (auto tmi = mTranslations.begin(); tmi != mTranslations.end(); ++tmi)
    {
        selectTransformMatrix(tmi->first, tmi->second);
    }


	for (auto ji = mJoints.begin(); ji != mJoints.end(); ++ji )
	{
		LLAnimJoint *joint = *ji;

        // Look for a translation for this joint.
		// If none, skip to next joint
		LLAnimTranslationMap::iterator tmi = mTranslations.find( joint->mName );
		if (tmi == mTranslations.end())
		{
            LL_DEBUGS("BVH") << "No translation to apply for " << joint->mName << LL_ENDL;
            continue;
		}

        LLAnimTranslation &trans = tmi->second;

		// Set the ignore flag if necessary
		if ( trans.mIgnore )
		{
            LL_DEBUGS("BVH") << "Ignoring animation joint " << joint->mName << LL_ENDL;
			joint->mIgnore = true;
			continue;
		}

		// Set the output name
		if ( ! trans.mOutName.empty() )
		{
            LL_DEBUGS("BVH") << "Replacing joint name " << joint->mName.c_str() << " with translation " << trans.mOutName.c_str() << LL_ENDL;
			joint->mOutName = trans.mOutName;
		}

        //Allow joint position changes as of SL-318
        joint->mIgnorePositions = false;
        if (joint->mNumChannels == 3)
        {
            joint->mIgnorePositions = true;
        }

		// Set the relative position and rotation flags if necessary
		if ( trans.mRelativePositionKey )
		{
			LL_DEBUGS("BVH") << "Removing 1st position offset from all keys for " << joint->mOutName.c_str() << LL_ENDL;
			joint->mRelativePositionKey = true;
		}

		if ( trans.mRelativeRotationKey )
		{
            LL_DEBUGS("BVH") << "Replacing 1st rotation from all keys for " << joint->mOutName.c_str() << LL_ENDL;
			joint->mRelativeRotationKey = true;
		}

		if ( trans.mRelativePosition.magVec() > 0.0f )
		{
			joint->mRelativePosition = trans.mRelativePosition;
            LL_DEBUGS("BVH") << "Replacing mRelativePosition with translation " <<
				joint->mRelativePosition.mV[0] << " " <<
				joint->mRelativePosition.mV[1] << " " <<
				joint->mRelativePosition.mV[2] <<
				" from all position keys in " <<
				joint->mOutName.c_str() << LL_ENDL;
		}

		// Set change of coordinate frame
		joint->mFrameMatrix = trans.mFrameMatrix;
		joint->mOffsetMatrix = trans.mOffsetMatrix;

		// Set mergeparent name
		if ( ! trans.mMergeParentName.empty() )
		{
			LL_DEBUGS("BVH") << "Merging "  << joint->mOutName.c_str() <<
				" with parent " << trans.mMergeParentName.c_str() << LL_ENDL;
			joint->mMergeParentName = trans.mMergeParentName;
		}

		// Set mergechild name
		if ( ! trans.mMergeChildName.empty() )
		{
			LL_DEBUGS("BVH") << "Merging " << joint->mName.c_str() <<
				" with child " << trans.mMergeChildName.c_str() << LL_ENDL;
			joint->mMergeChildName = trans.mMergeChildName;
		}

		// Set joint priority
		joint->mPriority = mPriority + trans.mPriorityModifier;
	}
}

//-----------------------------------------------------------------------------
// LLBVHLoader::optimize()
//-----------------------------------------------------------------------------
void LLBVHLoader::optimize()
{
	//RN: assumes motion blend, which is the default now
	if (!mLoop && mEaseIn + mEaseOut > mDuration && mDuration != 0.f)
	{
		F32 factor = mDuration / (mEaseIn + mEaseOut);
		mEaseIn *= factor;
		mEaseOut *= factor;
	}

	for (auto ji = mJoints.begin(); ji != mJoints.end(); ++ji)
	{
		LLAnimJoint *joint = *ji;
		bool pos_changed = false;
		bool rot_changed = false;

		if ( ! joint->mIgnore )
		{
			joint->mNumPosKeys = 0;
			joint->mNumRotKeys = 0;
			LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );

			LLAnimKeyVector::iterator first_key = joint->mKeys.begin();

			// no keys?
			if (first_key == joint->mKeys.end())
			{
				joint->mIgnore = true;
				continue;
			}

			LLVector3 first_frame_pos(first_key->mPos);
			LLQuaternion first_frame_rot = mayaQ( first_key->mRot[0], first_key->mRot[1], first_key->mRot[2], order);

			// skip first key
			LLAnimKeyVector::iterator ki = joint->mKeys.begin();
			if (joint->mKeys.size() == 1)
			{
				// *FIX: use single frame to move pelvis
				// if only one keyframe force output for this joint
				rot_changed = true;
			}
			else
			{
				// if more than one keyframe, use first frame as reference and skip to second
				first_key->mIgnorePos = true;
				first_key->mIgnoreRot = true;
				++ki;
			}

			LLAnimKeyVector::iterator ki_prev = ki;
			LLAnimKeyVector::iterator ki_last_good_pos = ki;
			LLAnimKeyVector::iterator ki_last_good_rot = ki;
			S32 numPosFramesConsidered = 2;
			S32 numRotFramesConsidered = 2;

			F32 rot_threshold = ROTATION_KEYFRAME_THRESHOLD / llmax((F32)joint->mChildTreeMaxDepth * 0.33f, 1.f);

			double diff_max = 0;
			LLAnimKeyVector::iterator ki_max = ki;
			for (; ki != joint->mKeys.end(); ++ki)
			{
				if (ki_prev == ki_last_good_pos)
				{
					joint->mNumPosKeys++;
					if (dist_vec_squared(LLVector3(ki_prev->mPos), first_frame_pos) > POSITION_MOTION_THRESHOLD_SQUARED)
					{
						pos_changed = true;
					}
				}
				else
				{
					//check position for noticeable effect
					LLVector3 test_pos(ki_prev->mPos);
					LLVector3 last_good_pos(ki_last_good_pos->mPos);
					LLVector3 current_pos(ki->mPos);
					LLVector3 interp_pos = lerp(current_pos, last_good_pos, 1.f / (F32)numPosFramesConsidered);

					if (dist_vec_squared(current_pos, first_frame_pos) > POSITION_MOTION_THRESHOLD_SQUARED)
					{
						pos_changed = true;
					}

					if (dist_vec_squared(interp_pos, test_pos) < POSITION_KEYFRAME_THRESHOLD_SQUARED)
					{
						ki_prev->mIgnorePos = true;
						numPosFramesConsidered++;
					}
					else
					{
						numPosFramesConsidered = 2;
						ki_last_good_pos = ki_prev;
						joint->mNumPosKeys++;
					}
				}

				if (ki_prev == ki_last_good_rot)
				{
					joint->mNumRotKeys++;
					LLQuaternion test_rot = mayaQ( ki_prev->mRot[0], ki_prev->mRot[1], ki_prev->mRot[2], order);
					F32 x_delta = dist_vec(LLVector3::x_axis * first_frame_rot, LLVector3::x_axis * test_rot);
					F32 y_delta = dist_vec(LLVector3::y_axis * first_frame_rot, LLVector3::y_axis * test_rot);
					F32 rot_test = x_delta + y_delta;

					if (rot_test > ROTATION_MOTION_THRESHOLD)
					{
						rot_changed = true;
					}
				}
				else
				{
					//check rotation for noticeable effect
					LLQuaternion test_rot = mayaQ( ki_prev->mRot[0], ki_prev->mRot[1], ki_prev->mRot[2], order);
					LLQuaternion last_good_rot = mayaQ( ki_last_good_rot->mRot[0], ki_last_good_rot->mRot[1], ki_last_good_rot->mRot[2], order);
					LLQuaternion current_rot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);
					LLQuaternion interp_rot = lerp(1.f / (F32)numRotFramesConsidered, current_rot, last_good_rot);

					F32 x_delta;
					F32 y_delta;
					F32 rot_test;

					// Test if the rotation has changed significantly since the very first frame.  If false
					// for all frames, then we'll just throw out this joint's rotation entirely.
					x_delta = dist_vec(LLVector3::x_axis * first_frame_rot, LLVector3::x_axis * test_rot);
					y_delta = dist_vec(LLVector3::y_axis * first_frame_rot, LLVector3::y_axis * test_rot);
					rot_test = x_delta + y_delta;
					if (rot_test > ROTATION_MOTION_THRESHOLD)
					{
						rot_changed = true;
					}
					x_delta = dist_vec(LLVector3::x_axis * interp_rot, LLVector3::x_axis * test_rot);
					y_delta = dist_vec(LLVector3::y_axis * interp_rot, LLVector3::y_axis * test_rot);
					rot_test = x_delta + y_delta;

					// Draw a line between the last good keyframe and current.  Test the distance between the last frame (current-1, i.e. ki_prev)
					// and the line.  If it's greater than some threshold, then it represents a significant frame and we want to include it.
					if (rot_test >= rot_threshold ||
						(ki+1 == joint->mKeys.end() && numRotFramesConsidered > 2))
					{
						// Add the current test keyframe (which is technically the previous key, i.e. ki_prev).
						numRotFramesConsidered = 2;
						ki_last_good_rot = ki_prev;
						joint->mNumRotKeys++;

						// Add another keyframe between the last good keyframe and current, at whatever point was the most "significant" (i.e.
						// had the largest deviation from the earlier tests).  Note that a more robust approach would be test all intermediate
						// keyframes against the line between the last good keyframe and current, but we're settling for this other method
						// because it's significantly faster.
						if (diff_max > 0)
						{
							if (ki_max->mIgnoreRot == true)
							{
								ki_max->mIgnoreRot = false;
								joint->mNumRotKeys++;
							}
							diff_max = 0;
						}
					}
					else
					{
						// This keyframe isn't significant enough, throw it away.
						ki_prev->mIgnoreRot = true;
						numRotFramesConsidered++;
						// Store away the keyframe that has the largest deviation from the interpolated line, for insertion later.
						if (rot_test > diff_max)
						{
							diff_max = rot_test;
							ki_max = ki;
						}
					}
				}

				ki_prev = ki;
			}
		}

		// don't output joints with no motion
		if (!(pos_changed || rot_changed))
		{
			LL_DEBUGS("BVH") << "Loader ignoring joint " << joint->mName
                << " due to no change" << LL_ENDL;
			joint->mIgnore = true;
		}
	}   // end big loop for each joint
}

void LLBVHLoader::reset()
{
	mLineNumber = 0;
	mNumFrames = 0;
	mFrameTime = 0.0f;
	mDuration = 0.0f;

	mPriority = 2;
	mLoop = false;
	mLoopInPoint = 0.f;
	mLoopOutPoint = 1.f;
	mEaseIn = 0.3f;
	mEaseOut = 0.3f;
	mHand = 1;
	mInitialized = false;

	mEmoteName = "";
	mLineNumber = 0;
	mTranslations.clear();
	mConstraints.clear();
    mSkeletonType = "SL";
    mTransformType = 214;

    if (mAssimpImporter)
    {
        delete mAssimpImporter;
        mAssimpImporter = nullptr;
    }
    mAssimpScene = nullptr;
}

//------------------------------------------------------------------------
// LLBVHLoader::getLine()
//------------------------------------------------------------------------
bool LLBVHLoader::getLine(apr_file_t* fp)
{
	if (apr_file_eof(fp) == APR_EOF)
	{
		return false;
	}
	if ( apr_file_gets(mLine, BVH_PARSER_LINE_SIZE, fp) == APR_SUCCESS)
	{
		mLineNumber++;
		return true;
	}

	return false;
}

// returns required size of output buffer
S32 LLBVHLoader::getOutputSize()
{   // Note - the default LLDataPackerBinaryBuffer constructor doesn't allocate
    // a buffer for data.   Thus the serialize() call will not actually write data
    // anywhere, but instead moves a pointer starting from 0 and in the end getCurrentSize()
    // will return the size needed without actually doing full serialization.
	LLDataPackerBinaryBuffer dp;
	serialize(dp);

	return dp.getCurrentSize();
}

// writes contents to datapacker
bool LLBVHLoader::serialize(LLDataPacker& dp)
{
	F32 time;

	// count number of non-ignored joints
	S32 numJoints = 0;
	for (auto ji = mJoints.begin(); ji != mJoints.end(); ++ji)
	{
		LLAnimJoint *joint = *ji;
		if ( ! joint->mIgnore )
			numJoints++;
	}

	// print header
	dp.packU16(KEYFRAME_MOTION_VERSION, "version");
	dp.packU16(KEYFRAME_MOTION_SUBVERSION, "sub_version");
	dp.packS32(mPriority, "base_priority");
	dp.packF32(mDuration, "duration");
	dp.packString(mEmoteName, "emote_name");
	dp.packF32(mLoopInPoint, "loop_in_point");
	dp.packF32(mLoopOutPoint, "loop_out_point");
	dp.packS32(mLoop, "loop");
	dp.packF32(mEaseIn, "ease_in_duration");
	dp.packF32(mEaseOut, "ease_out_duration");
	dp.packU32(mHand, "hand_pose");
	dp.packU32(numJoints, "num_joints");

	for (auto ji = mJoints.begin();
			ji != mJoints.end();
			++ji )
	{
		LLAnimJoint *joint = *ji;
		// if ignored, skip it
		if ( joint->mIgnore )
			continue;

		LLQuaternion first_frame_rot;
		//LLQuaternion fixup_rot;  not used?

		dp.packString(joint->mOutName, "joint_name");
		dp.packS32(joint->mPriority, "joint_priority");

		// compute coordinate frame rotation
		LLQuaternion frameRot( joint->mFrameMatrix );
		LLQuaternion frameRotInv = ~frameRot;

		LLQuaternion offsetRot( joint->mOffsetMatrix );

		// find mergechild and mergeparent joints, if specified
		LLQuaternion mergeParentRot;
		LLQuaternion mergeChildRot;
		LLAnimJoint *mergeParent = NULL;
		LLAnimJoint *mergeChild = NULL;

		for (auto merge_joint_iter = mJoints.begin(); merge_joint_iter != mJoints.end(); ++merge_joint_iter)
		{
			LLAnimJoint *mjoint = *merge_joint_iter;
			if ( !joint->mMergeParentName.empty() && (mjoint->mName == joint->mMergeParentName) )
			{
				mergeParent = *merge_joint_iter;
			}
			if ( !joint->mMergeChildName.empty() && (mjoint->mName == joint->mMergeChildName) )
			{
				mergeChild = *merge_joint_iter;
			}
		}

		dp.packS32(joint->mNumRotKeys, "num_rot_keys");

		LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );
		S32 outcount = 0;
		S32 frame = 0;
		for ( auto ki = joint->mKeys.begin();
				ki != joint->mKeys.end();
				++ki )
		{

			if ((frame == 0) && joint->mRelativeRotationKey)
			{
				first_frame_rot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);
				//fixup_rot.shortestArc(LLVector3::z_axis * first_frame_rot * frameRot, LLVector3::z_axis);
			}

			if (ki->mIgnoreRot)
			{
				frame++;
				continue;
			}

			time = llmax((F32)(frame - NUMBER_OF_IGNORED_FRAMES_AT_START), 0.0f) * mFrameTime; // Time elapsed before this frame starts.

			if (mergeParent)
			{
				mergeParentRot = mayaQ(	mergeParent->mKeys[frame-1].mRot[0], 
										mergeParent->mKeys[frame-1].mRot[1],
										mergeParent->mKeys[frame-1].mRot[2],
										bvhStringToOrder(mergeParent->mOrder) );
				LLQuaternion parentFrameRot( mergeParent->mFrameMatrix );
				LLQuaternion parentOffsetRot( mergeParent->mOffsetMatrix );
				mergeParentRot = ~parentFrameRot * mergeParentRot * parentFrameRot * parentOffsetRot;
			}
			else
			{
				mergeParentRot.loadIdentity();
			}

			if (mergeChild)
			{
				mergeChildRot = mayaQ(	mergeChild->mKeys[frame-1].mRot[0], 
										mergeChild->mKeys[frame-1].mRot[1],
										mergeChild->mKeys[frame-1].mRot[2],
										bvhStringToOrder(mergeChild->mOrder) );
				LLQuaternion childFrameRot( mergeChild->mFrameMatrix );
				LLQuaternion childOffsetRot( mergeChild->mOffsetMatrix );
				mergeChildRot = ~childFrameRot * mergeChildRot * childFrameRot * childOffsetRot;
			}
			else
			{
				mergeChildRot.loadIdentity();
			}

			LLQuaternion inRot = mayaQ( ki->mRot[0], ki->mRot[1], ki->mRot[2], order);

			LLQuaternion outRot =  frameRotInv* mergeChildRot * inRot * mergeParentRot * ~first_frame_rot * frameRot * offsetRot;

			U16 time_short = F32_to_U16(time, 0.f, mDuration);
			dp.packU16(time_short, "time");
			U16 x, y, z;
			LLVector3 rot_vec = outRot.packToVector3();
			rot_vec.quantize16(-1.f, 1.f, -1.f, 1.f);
			x = F32_to_U16(rot_vec.mV[VX], -1.f, 1.f);
			y = F32_to_U16(rot_vec.mV[VY], -1.f, 1.f);
			z = F32_to_U16(rot_vec.mV[VZ], -1.f, 1.f);
			dp.packU16(x, "rot_angle_x");
			dp.packU16(y, "rot_angle_y");
			dp.packU16(z, "rot_angle_z");
			outcount++;
			frame++;
		}

		// output position keys if joint has motion.
        if ( !joint->mIgnorePositions )
		{
			dp.packS32(joint->mNumPosKeys, "num_pos_keys");

			LLVector3 relPos = joint->mRelativePosition;
			LLVector3 relKey;

			frame = 0;
			for (auto ki = joint->mKeys.begin();
					ki != joint->mKeys.end();
					++ki )
			{
				if ((frame == 0) && joint->mRelativePositionKey)
				{
					relKey.setVec(ki->mPos);
				}

				if (ki->mIgnorePos)
				{
					frame++;
					continue;
				}

				time = llmax((F32)(frame - NUMBER_OF_IGNORED_FRAMES_AT_START), 0.0f) * mFrameTime; // Time elapsed before this frame starts.

				LLVector3 inPos = (LLVector3(ki->mPos) - relKey) * ~first_frame_rot;        // * fixup_rot;
				LLVector3 outPos = inPos * frameRot * offsetRot;

				outPos *= INCHES_TO_METERS;

                //SL-318  Pelvis position can only move 5m.  Limiting all joint position offsets to this dist.
				outPos -= relPos;
				outPos.clamp(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);

				U16 time_short = F32_to_U16(time, 0.f, mDuration);
				dp.packU16(time_short, "time");

				U16 x, y, z;
				outPos.quantize16(-LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				x = F32_to_U16(outPos.mV[VX], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				y = F32_to_U16(outPos.mV[VY], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				z = F32_to_U16(outPos.mV[VZ], -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET);
				dp.packU16(x, "pos_x");
				dp.packU16(y, "pos_y");
				dp.packU16(z, "pos_z");

				frame++;
			}
		}
		else
		{
			dp.packS32(0, "num_pos_keys");
		}
	}

	S32 num_constraints = (S32)mConstraints.size();
	dp.packS32(num_constraints, "num_constraints");

	for (auto constraint_it = mConstraints.begin();
		constraint_it != mConstraints.end();
		constraint_it++)
		{
			U8 byte = constraint_it->mChainLength;
			dp.packU8(byte, "chain_length");

			byte = constraint_it->mConstraintType;
			dp.packU8(byte, "constraint_type");
			dp.packBinaryDataFixed((U8*)constraint_it->mSourceJointName, 16, "source_volume");
			dp.packVector3(constraint_it->mSourceOffset, "source_offset");
			dp.packBinaryDataFixed((U8*)constraint_it->mTargetJointName, 16, "target_volume");
			dp.packVector3(constraint_it->mTargetOffset, "target_offset");
			dp.packVector3(constraint_it->mTargetDir, "target_dir");
			dp.packF32(constraint_it->mEaseInStart,	"ease_in_start");
			dp.packF32(constraint_it->mEaseInStop,	"ease_in_stop");
			dp.packF32(constraint_it->mEaseOutStart,	"ease_out_start");
			dp.packF32(constraint_it->mEaseOutStop,	"ease_out_stop");
		}

		return true;
}


//-----------------------------------------------------------------------------
// Test code - poke into assimp scene and look at the animation data
//-----------------------------------------------------------------------------
static const S32 MAX_ASSIMP_KEYS = 100;

static bool dump_key_data = true;

void LLBVHLoader::dumpAssimpAnimationQuatKeys(aiQuatKey * quat_keys, S32 count, llofstream & data_stream)
{
    if (!dump_key_data)
        return;

    for (S32 index = 0; index < count; index++)
    {
        const aiQuatKey & cur_quat = quat_keys[index];
        LLQuaternion ll_quat(cur_quat.mValue.x, cur_quat.mValue.y, cur_quat.mValue.z, cur_quat.mValue.w);
        F32 roll, pitch, yaw;
        ll_quat.getEulerAngles(&roll, &pitch, &yaw);

        data_stream << "       tick: " << cur_quat.mTime
            << "  <" << cur_quat.mValue.w
            << ",  " << cur_quat.mValue.x
            << ",  " << cur_quat.mValue.y
            << ",  " << cur_quat.mValue.z << ">"
            << " or euler: " << roll << ", " << pitch << ", " << yaw
            << " (" << roll * RAD_TO_DEG << ", " << pitch * RAD_TO_DEG << ", " << yaw * RAD_TO_DEG << ") degrees"
            << std::endl;

        if (index >= MAX_ASSIMP_KEYS)
            break;
    }
}

void LLBVHLoader::dumpAssimpAnimationVectorKeys(aiVectorKey * vector_keys, S32 count, llofstream & data_stream)
{
    if (!dump_key_data)
        return;

    for (S32 index = 0; index < count; index++)
    {
        const aiVectorKey & cur_vector = vector_keys[index];
        data_stream << "       tick: " << cur_vector.mTime
            << "  <" << cur_vector.mValue.x
            << ",  " << cur_vector.mValue.y
            << ",  " << cur_vector.mValue.z << ">" << std::endl;

        if (index >= MAX_ASSIMP_KEYS)
            break;
    }
}

void LLBVHLoader::dumpAssimpAnimationChannels(aiAnimation * cur_animation, llofstream & data_stream)
{
    for (S32 index = 0; index < cur_animation->mNumChannels; index++)
    {
        aiNodeAnim * cur_node = cur_animation->mChannels[index];
        if (cur_node && cur_node->mNodeName.C_Str())
        {
            std::string node_name(cur_node->mNodeName.C_Str());
            data_stream << "    Node name: " << node_name << std::endl;
            data_stream << "      mNumPositionKeys: " << (S32)(cur_node->mNumPositionKeys) << " for " << node_name << std::endl;
            if (cur_node->mNumPositionKeys > 0)
            {
                dumpAssimpAnimationVectorKeys(cur_node->mPositionKeys, cur_node->mNumPositionKeys, data_stream);
            }
            data_stream << "      mNumRotationKeys: " << (S32)(cur_node->mNumRotationKeys) << " for " << node_name << std::endl;
            if (cur_node->mNumRotationKeys > 0)
            {
                dumpAssimpAnimationQuatKeys(cur_node->mRotationKeys, cur_node->mNumRotationKeys, data_stream);
            }
            if (cur_node->mNumScalingKeys > 0)
            {   // see if scaling keys has anything interesting
                bool show_details = false;
                for (S32 scale_index = 0; scale_index < cur_node->mNumScalingKeys; scale_index++)
                {
                    const aiVectorKey & cur_vector = cur_node->mScalingKeys[scale_index];
                    if (cur_vector.mValue.x != 1 ||
                        cur_vector.mValue.y != 1 ||
                        cur_vector.mValue.z != 1)
                    {
                        show_details = true;
                        break;
                    }
                }

                if (show_details)
                {
                    data_stream << "      mNumScalingKeys: " << (S32)(cur_node->mNumScalingKeys) << " for " << node_name << std::endl;
                    dumpAssimpAnimationVectorKeys(cur_node->mScalingKeys, cur_node->mNumScalingKeys, data_stream);
                }
                else
                {
                    data_stream << "      mNumScalingKeys: " << (S32)(cur_node->mNumScalingKeys) << " for " << node_name
                        << " all <1,  1,  1>" << std::endl;
                }
            }
            else
            {   // no scaling keys
                data_stream << "      mNumScalingKeys: 0 for " << node_name << std::endl;
            }
        }
        else
        {
            data_stream << "    Unexpected missing aiNodeAnim channel " << index << std::endl;
            break;
        }
    }
}

void LLBVHLoader::dumpAssimpAnimations(llofstream & data_stream)
{
    aiAnimation * cur_animation = mAssimpScene->mAnimations[0];     // to do - support extracting Nth animation ?
    if (cur_animation)
    {
        if (cur_animation->mName.C_Str())
        {
            std::string animation_name(cur_animation->mName.C_Str());
            data_stream << "  Animation name: " << animation_name << std::endl;
        }
        else
        {
            data_stream << "  No animation name" << std::endl;
        }
        data_stream << "  Animation duration " << cur_animation->mDuration
            << " ticks, running at " << cur_animation->mTicksPerSecond << " per second for "
            << (cur_animation->mDuration / cur_animation->mTicksPerSecond) << " seconds"
            << std::endl;

        data_stream << "  Animation mNumChannels " << cur_animation->mNumChannels << std::endl;
        data_stream << "  Animation mNumMeshChannels " << cur_animation->mNumMeshChannels << std::endl;
        data_stream << "  Animation mNumMorphMeshChannels " << cur_animation->mNumMorphMeshChannels << std::endl;

        if (cur_animation->mNumChannels > 0)
        {
            dumpAssimpAnimationChannels(cur_animation, data_stream);
        }
    }
    else
    {
        LL_WARNS("Assimp") << "Unexpected missing animation data" << LL_ENDL;
    }
}

void LLBVHLoader::dumpAssimp()
{
    // Make a file stream so we can write data
    std::string assimp_data_name(mFilenameAndPath);
    assimp_data_name.append(".data");
    llofstream data_stream(assimp_data_name.c_str());
    if (!data_stream.is_open())
    {
        LL_WARNS("Assimp") << "Failed to open file for data stream " << assimp_data_name << LL_ENDL;
        return;
    }
    LL_INFOS("Assimp") << "Writing data file " << assimp_data_name << LL_ENDL;


    data_stream << "Assimp scene data read from " << mFilenameAndPath << std::endl;
    data_stream << "Time: " << LLDate::now() << std::endl;
    data_stream << std::endl;

    data_stream << "mNumAnimations: " << (S32)(mAssimpScene->mNumAnimations) << std::endl;
    data_stream << "mNumSkeletons: " << (S32)(mAssimpScene->mNumSkeletons) << std::endl;
    data_stream << "mNumMeshes: " << (S32)(mAssimpScene->mNumMeshes) << std::endl;
    data_stream << "mNumMaterials: " << (S32)(mAssimpScene->mNumMaterials) << std::endl;
    data_stream << "mNumTextures: " << (S32)(mAssimpScene->mNumTextures) << std::endl;
    data_stream << "mNumLights: " << (S32)(mAssimpScene->mNumLights) << std::endl;
    data_stream << "mNumCameras: " << (S32)(mAssimpScene->mNumCameras) << std::endl;

    if (mAssimpScene->mNumAnimations == 1)
    {
        dumpAssimpAnimations(data_stream);
    }
}

ELoadStatus LLBVHLoader::loadAssimp()
{
    using namespace Assimp;

    if (mAssimpImporter)
    {
        LL_WARNS("Assimp") << "Already have unexpected importer when loading assimp" << LL_ENDL;
        return E_ST_INTERNAL_ERROR;
    }
    if (mAssimpScene)
    {
        LL_WARNS("Assimp") << "Already have unexpected scene when loading assimp" << LL_ENDL;
        return E_ST_INTERNAL_ERROR;
    }

    // Create a logger instance.   To do - remove eventually?
    std::string assimp_log_name(mFilenameAndPath);
    assimp_log_name.append(".log");
    Assimp::DefaultLogger::create(assimp_log_name.c_str(), Logger::VERBOSE);
    LL_INFOS("Assimp") << "Writing log file " << assimp_log_name << LL_ENDL;

    // Test
    DefaultLogger::get()->info("SL assimp animation loading logging");
    try
    {
        // Create an instance of the Asset Importer class
        mAssimpImporter = new Assimp::Importer;

        // And have it read the given file with some example postprocessing
        // Usually - if speed is not the most important aspect for you - you'll
        // probably to request more postprocessing than we do in this example.

        LL_INFOS("Assimp") << "Loading BVH file into Assimp Importer " << mFilenameAndPath << LL_ENDL;
        mAssimpScene = mAssimpImporter->ReadFile(mFilenameAndPath,
            //aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

        // Check if the import failed
        if (mAssimpScene == nullptr)
        {
            LL_WARNS("Assimp") << "Unable to read and create Assimp scene from " << mFilenameAndPath << LL_ENDL;
        }
        extractJointsFromAssimp();
    }
    catch (...)
    {   // to do - some better handling / alert to user?
        LL_WARNS("Assimp") << "Unhandled exception trying to use Assimp asset import library" << LL_ENDL;
    }

    // Get rid of assimp logger
    DefaultLogger::kill();

    return E_ST_OK;
}

void LLBVHLoader::extractJointsFromAssimp()
{
    if (!mAssimpScene)
    {
        LL_WARNS("Assimp") << "No assimp scene, can't extract joint animations" << LL_ENDL;
        return;     // to do - add errors
    }
    if (mAssimpScene->mNumAnimations == 0)
    {
        LL_WARNS("Assimp") << "No assimp animations, can't extract joints" << LL_ENDL;
        return;
    }

    aiAnimation * cur_animation = mAssimpScene->mAnimations[0];     // to do - support extracting Nth animation ?
    if (cur_animation)
    {
        LL_INFOS("Assimp") << "assimp data duration " << cur_animation->mDuration << " vs. mNumFrames " << mNumFrames << LL_ENDL;

        mNumFrames = cur_animation->mDuration + 1;
        if (cur_animation->mTicksPerSecond <= 0.0)
        {   // Play it safe
            cur_animation->mTicksPerSecond = 1.0;
        }
        mFrameTime = (F32)(1.0 / cur_animation->mTicksPerSecond);

        // Copied from original BVH code:
        // If the file only supplies one animation frame (after the ignored reference frame 0), hold for mFrameTime.
        // If the file supples exactly one total frame, it isn't clear if that is a pose or reference frame, and the
        // behavior is not defined. In this case, retain historical undefined behavior.
        mDuration = llmax((F32)(mNumFrames - NUMBER_OF_UNPLAYED_FRAMES), 1.0f) * mFrameTime;
        if (mLoop)
        {
            mLoopInPoint *= mDuration;
            mLoopOutPoint *= mDuration;
        }
        else
        {
            mLoopInPoint = 0.f;
            mLoopOutPoint = mDuration;
        }

        // Extract nodes into mJoints
        for (S32 node_index = 0; node_index < cur_animation->mNumChannels; node_index++)
        {
            aiNodeAnim * cur_node = cur_animation->mChannels[node_index];
            if (cur_node && cur_node->mNodeName.C_Str())
            {
                std::string node_name(cur_node->mNodeName.C_Str());

                // Special filter for mixamo fbx format ... joints will look like "mixamorig:Hips"
                if (node_name.find("mixamorig:") == 0)
                {
                    LL_DEBUGS("Assimp") << "Converting mixamo joint name " << node_name << " to " << node_name.substr(10) << LL_ENDL;
                    node_name = node_name.substr(10);
                }

                if (node_name == "Hips")
                {
                    LL_INFOS("BVH") << "Detected mixamo style Hips, using that skeleton type" << LL_ENDL;
                    mSkeletonType = "mixamo";
                }

                mJoints.push_back(new LLAnimJoint(node_name));
                LLAnimJoint *joint = mJoints.back();
                LL_DEBUGS("Assimp") << "Created joint " << node_name << " at index " << mJoints.size() - 1 << LL_ENDL;

                joint->mNumChannels = (node_index == 0) ? 6 : 3;

                // To do - Can we figure out mChildTreeMaxDepth ?   It's used only for some threshold calculations about
                // detectable movement, but without it there may be a behavior change

                joint->mOrder[0] = 'X';
                joint->mOrder[1] = 'Y';
                joint->mOrder[2] = 'Z';

                // Add all animations frames
                for (S32 key_index = 0; key_index < cur_node->mNumRotationKeys; key_index++)
                {
                    joint->mKeys.push_back(LLAnimKey());
                    LLAnimKey &key = joint->mKeys.back();

                    const aiQuatKey & cur_quat = cur_node->mRotationKeys[key_index];
                    LLQuaternion ll_quat(cur_quat.mValue.x, cur_quat.mValue.y, cur_quat.mValue.z, cur_quat.mValue.w);
                    F32 roll, pitch, yaw;
                    ll_quat.getEulerAngles(&roll, &pitch, &yaw);
                    key.mRot[0] = roll * RAD_TO_DEG;
                    key.mRot[1] = pitch * RAD_TO_DEG;
                    key.mRot[2] = yaw * RAD_TO_DEG;

                    if (joint->mNumChannels == 6 &&
                        key_index < cur_node->mNumPositionKeys)
                    {
                        const aiVectorKey & cur_vector = cur_node->mPositionKeys[key_index];
                        key.mPos[0] = cur_vector.mValue.x;
                        key.mPos[1] = cur_vector.mValue.y;
                        key.mPos[2] = cur_vector.mValue.z;
                    }
                }
            }
            else
            {
                LL_WARNS("Assimp") << "Unexpected missing aiNodeAnim channel " << node_index << LL_ENDL;
                return;
            }
        }

        tweakJointData(mJoints);

        LL_INFOS("Assimp") << "Extracted " << mJoints.size() << " animation joints from assimp"
            << ", using " << mSkeletonType << " skeleton" << LL_ENDL;
        dumpJointInfo(mJoints);
    }
}

void            detectSkeletonType();           // Scan scene, detect skeleton type and adjust translations
