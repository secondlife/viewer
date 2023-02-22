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
// find_next_whitespace()
//------------------------------------------------------------------------
const char *find_next_whitespace(const char *p)
{
	while(*p && isspace(*p)) p++;
	while(*p && !isspace(*p)) p++;
	return p;
}


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

LLBVHLoader::LLBVHLoader(const char* buffer, ELoadStatus &loadStatus, S32 &errorLine, std::map<std::string, std::string>& joint_alias_map )
{
	reset();
	errorLine = 0;
	mStatus = loadTranslationTable("anim.ini");
	loadStatus = mStatus;
	LL_INFOS("BVH") << "Load Status 00 : " << loadStatus << LL_ENDL;
	if (mStatus == E_ST_NO_XLT_FILE)
	{
		LL_WARNS("BVH") << "NOTE: No translation table found." << LL_ENDL;
		loadStatus = mStatus;
		return;
	}
	else
	{
		if (mStatus != E_ST_OK)
		{
			LL_WARNS("BVH") << "ERROR: [line: " << getLineNumber() << "] " << mStatus << LL_ENDL;
			errorLine = getLineNumber();
			loadStatus = mStatus;
			return;
		}
	}
    
    // Recognize all names we've been told are legal.
    for (std::map<std::string, std::string>::value_type& alias_pair : joint_alias_map)
    {
        makeTranslation( alias_pair.first , alias_pair.second );
    }
	
	char error_text[128];		/* Flawfinder: ignore */
	S32 error_line;
	mStatus = loadBVHFile(buffer, error_text, error_line); //Reads all joints in BVH file.

	LL_DEBUGS("BVH") << "============================================================" << LL_ENDL;
	LL_DEBUGS("BVH") << "Raw data from file" << LL_ENDL;
	dumpBVHInfo();
	
	if (mStatus != E_ST_OK)
	{
		LL_WARNS("BVH") << "ERROR: [line: " << getLineNumber() << "] " << mStatus << LL_ENDL;
		loadStatus = mStatus;
		errorLine = getLineNumber();
		return;
	}
	
	applyTranslations();  //Maps between joints found in file and the aliased names.
	optimize();
	
	LL_DEBUGS("BVH") << "============================================================" << LL_ENDL;
	LL_DEBUGS("BVH") << "After translations and optimize" << LL_ENDL;
	dumpBVHInfo();

	mInitialized = TRUE;
}


LLBVHLoader::~LLBVHLoader()
{
	std::for_each(mJoints.begin(),mJoints.end(),DeletePointer());
	mJoints.clear();
}

//------------------------------------------------------------------------
// LLBVHLoader::loadTranslationTable()
//------------------------------------------------------------------------
ELoadStatus LLBVHLoader::loadTranslationTable(const char *fileName)
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

	LL_INFOS("BVH") << "NOTE: Loading translation table: " << fileName << LL_ENDL;

	//--------------------------------------------------------------------
	// register file to be closed on function exit
	//--------------------------------------------------------------------
	
	//--------------------------------------------------------------------
	// load header
	//--------------------------------------------------------------------
	if ( ! getLine(fp) )
		return E_ST_EOF;
	if ( strncmp(mLine, "Translations 1.0", 16) )
		return E_ST_NO_XLT_HEADER;

	//--------------------------------------------------------------------
	// load data one line at a time
	//--------------------------------------------------------------------
	BOOL loadingGlobals = FALSE;
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
		// check if a [jointName] or [GLOBALS] was specified.
		//----------------------------------------------------------------
		if (token[0] == '[')
		{
			char name[128]; /* Flawfinder: ignore */
			if ( sscanf(mLine, " [%127[^]]", name) != 1 )
				return E_ST_NO_XLT_NAME;

			if (strcmp(name, "GLOBALS")==0)
			{
				loadingGlobals = TRUE;
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
//			LL_INFOS() << "NOTE: Emote: " << mEmoteName.c_str() << LL_ENDL;
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
//			LL_INFOS() << "NOTE: Priority: " << mPriority << LL_ENDL;
			continue;
		}

		//----------------------------------------------------------------
		// check for global loop setting
		//----------------------------------------------------------------
		if (loadingGlobals && LLStringUtil::compareInsensitive(token, "loop")==0)
		{
			char trueFalse[128];		/* Flawfinder: ignore */
			trueFalse[0] = '\0';
			
			F32 loop_in = 0.f;
			F32 loop_out = 1.f;

			if ( sscanf(mLine, " %*s = %f %f", &loop_in, &loop_out) == 2 )
			{
				mLoop = TRUE;
			}
			else if ( sscanf(mLine, " %*s = %127s", trueFalse) == 1 )	/* Flawfinder: ignore */	
			{
				mLoop = (LLStringUtil::compareInsensitive(trueFalse, "true")==0);
			}
			else
			{
				return E_ST_NO_XLT_LOOP;
			}

			mLoopInPoint = loop_in * mDuration;
			mLoopOutPoint = loop_out * mDuration;

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
			Constraint constraint;

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
			Constraint constraint;

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
void LLBVHLoader::makeTranslation(std::string alias_name, std::string joint_name)
{
    //Translation &newTrans = (foomap.insert(value_type(alias_name, Translation()))).first();
    Translation &newTrans = mTranslations[ alias_name ];  //Uses []'s implicit call to ctor.
    
    newTrans.mOutName = joint_name;
    LLMatrix3 fm;
    LLVector3 vect1(0, 1, 0);
    LLVector3 vect2(0, 0, 1);
    LLVector3 vect3(1, 0, 0);
    fm.setRows(vect1, vect2, vect3);
    
    newTrans.mFrameMatrix = fm;
    
if (joint_name == "mPelvis")
    {
        newTrans.mRelativePositionKey = TRUE;
        newTrans.mRelativeRotationKey = TRUE;
    }

}

ELoadStatus LLBVHLoader::loadAliases(const char * filename)
{
    LLSD aliases_sd;
 
    std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,filename);
    
    llifstream input_stream;
    input_stream.open(fullpath.c_str(), std::ios::in | std::ios::binary);
    
    if(input_stream.is_open())
    {
        if ( LLSDSerialize::fromXML(aliases_sd, input_stream) )
        {
            for(LLSD::map_iterator alias_iter = aliases_sd.beginMap();
                alias_iter != aliases_sd.endMap();
                ++alias_iter)
            {
                LLSD::String alias_name = alias_iter->first;
                LLSD::String joint_name = alias_iter->second;
                makeTranslation(alias_name, joint_name);
                
            }
        }
        else
        {
            return E_ST_NO_XLT_HEADER;
        }
        input_stream.close();
    }
    else
    {
        LL_WARNS("BVH") << "Can't open joint alias file " << fullpath << LL_ENDL;
        return E_ST_NO_XLT_FILE;
    }

    return E_ST_OK;
}

void LLBVHLoader::dumpBVHInfo()
{
	for (U32 j=0; j<mJoints.size(); j++)
	{
		Joint *joint = mJoints[j];
		LL_DEBUGS("BVH") << joint->mName << LL_ENDL;
		for (S32 i=0; i<mNumFrames; i++)
		{
            if (i<joint->mKeys.size()) // Check this in case file load failed.
            {
                Key &prevkey = joint->mKeys[llmax(i-1,0)];
                Key &key = joint->mKeys[i];
                if ((i==0) ||
                    (key.mPos[0] != prevkey.mPos[0]) ||
                    (key.mPos[1] != prevkey.mPos[1]) ||
                    (key.mPos[2] != prevkey.mPos[2]) ||
                    (key.mRot[0] != prevkey.mRot[0]) ||
                    (key.mRot[1] != prevkey.mRot[1]) ||
                    (key.mRot[2] != prevkey.mRot[2])
                    )
                {
                    LL_DEBUGS("BVH") << "FRAME " << i 
                                     << " POS " << key.mPos[0] << "," << key.mPos[1] << "," << key.mPos[2]
                                     << " ROT " << key.mRot[0] << "," << key.mRot[1] << "," << key.mRot[2] << LL_ENDL;
                }
            }
		}
	}

}

//------------------------------------------------------------------------
// LLBVHLoader::loadBVHFile()
//------------------------------------------------------------------------
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
//		LL_INFOS() << line << LL_ENDL;
		return E_ST_NO_HIER;
	}

	//--------------------------------------------------------------------
	// consume joints
	//--------------------------------------------------------------------
	while (TRUE)
	{
		//----------------------------------------------------------------
		// get next line
		//----------------------------------------------------------------
		if (iter == tokens.end())
			return E_ST_EOF;
		line = (*(iter++));
		err_line++;

		//----------------------------------------------------------------
		// consume }
		//----------------------------------------------------------------
		if ( strstr(line.c_str(), "}") )
		{
			if (parent_joints.size() > 0)
			{
				parent_joints.pop_back();
			}
			continue;
		}

		//----------------------------------------------------------------
		// if MOTION, break out
		//----------------------------------------------------------------
		if ( strstr(line.c_str(), "MOTION") )
			break;

		//----------------------------------------------------------------
		// it must be either ROOT or JOINT or EndSite
		//----------------------------------------------------------------
		if ( strstr(line.c_str(), "ROOT") )
		{
		}
		else if ( strstr(line.c_str(), "JOINT") )
		{
		}
		else if ( strstr(line.c_str(), "End Site") )
		{
			iter++; // {
			iter++; //     OFFSET
			iter++; // }
			S32 depth = 0;
			for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
			{
				Joint *joint = mJoints[parent_joints[j]];
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

		//----------------------------------------------------------------
		// get the joint name
		//----------------------------------------------------------------
		char jointName[80];	/* Flawfinder: ignore */
		if ( sscanf(line.c_str(), "%*s %79s", jointName) != 1 )	/* Flawfinder: ignore */
		{
			strncpy(error_text, line.c_str(), 127);	/* Flawfinder: ignore */
			return E_ST_NO_NAME;
		}

		//---------------------------------------------------------------
		// we require the root joint be "hip" - DEV-26188
		//---------------------------------------------------------------
        if (mJoints.size() == 0 )
        {
            //The root joint of the BVH file must be hip (mPelvis) or an alias of mPelvis.
            const char* FORCED_ROOT_NAME = "hip";
            
            TranslationMap::iterator hip_joint = mTranslations.find( FORCED_ROOT_NAME );
            TranslationMap::iterator root_joint = mTranslations.find( jointName );
            if ( hip_joint == mTranslations.end() || root_joint == mTranslations.end() || root_joint->second.mOutName != hip_joint->second.mOutName )
            {
                strncpy(error_text, line.c_str(), 127);	/* Flawfinder: ignore */
                return E_ST_BAD_ROOT;
            }
        }

		
		//----------------------------------------------------------------
		// add a set of keyframes for this joint
		//----------------------------------------------------------------
		mJoints.push_back( new Joint( jointName ) );
		Joint *joint = mJoints.back();
		LL_DEBUGS("BVH") << "Created joint " << jointName << LL_ENDL;
		LL_DEBUGS("BVH") << "- index " << mJoints.size()-1 << LL_ENDL;

		S32 depth = 1;
		for (S32 j = (S32)parent_joints.size() - 1; j >= 0; j--)
		{
			Joint *pjoint = mJoints[parent_joints[j]];
			LL_DEBUGS("BVH") << "- ancestor " << pjoint->mName << LL_ENDL;
			if (depth > pjoint->mChildTreeMaxDepth)
			{
				pjoint->mChildTreeMaxDepth = depth;
			}
			depth++;
		}

		//----------------------------------------------------------------
		// get next line
		//----------------------------------------------------------------
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		//----------------------------------------------------------------
		// it must be {
		//----------------------------------------------------------------
		if ( !strstr(line.c_str(), "{") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_OFFSET;
		}
		else
		{
			parent_joints.push_back((S32)mJoints.size() - 1);
		}

		//----------------------------------------------------------------
		// get next line
		//----------------------------------------------------------------
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		//----------------------------------------------------------------
		// it must be OFFSET
		//----------------------------------------------------------------
		if ( !strstr(line.c_str(), "OFFSET") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_OFFSET;
		}

		//----------------------------------------------------------------
		// get next line
		//----------------------------------------------------------------
		if (iter == tokens.end())
		{
			return E_ST_EOF;
		}
		line = (*(iter++));
		err_line++;

		//----------------------------------------------------------------
		// it must be CHANNELS
		//----------------------------------------------------------------
		if ( !strstr(line.c_str(), "CHANNELS") )
		{
			strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
			return E_ST_NO_CHANNELS;
		}

        // Animating position (via mNumChannels = 6) is only supported for mPelvis.
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

		//----------------------------------------------------------------
		// get rotation order
		//----------------------------------------------------------------
		const char *p = line.c_str();
		for (S32 i=0; i<3; i++)
		{
			p = strstr(p, "rotation");
			if (!p)
			{
				strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
				return E_ST_NO_ROTATION;
			}

			const char axis = *(p - 1);
			if ((axis != 'X') && (axis != 'Y') && (axis != 'Z'))
			{
				strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
				return E_ST_NO_AXIS;
			}

			joint->mOrder[i] = axis;

			p++;
		}
	}

	//--------------------------------------------------------------------
	// consume motion
	//--------------------------------------------------------------------
	if ( !strstr(line.c_str(), "MOTION") )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_MOTION;
	}

	//--------------------------------------------------------------------
	// get number of frames
	//--------------------------------------------------------------------
	if (iter == tokens.end())
	{
		return E_ST_EOF;
	}
	line = (*(iter++));
	err_line++;

	if ( !strstr(line.c_str(), "Frames:") )
	{
		strncpy(error_text, line.c_str(), 127);	/*Flawfinder: ignore*/
		return E_ST_NO_FRAMES;
	}

	if ( sscanf(line.c_str(), "Frames: %d", &mNumFrames) != 1 )
	{
		strncpy(error_text, line.c_str(), 127);		/*Flawfinder: ignore*/
		return E_ST_NO_FRAMES;
	}

	//--------------------------------------------------------------------
	// get frame time
	//--------------------------------------------------------------------
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

	// If the user only supplies one animation frame (after the ignored reference frame 0), hold for mFrameTime.
	// If the user supples exactly one total frame, it isn't clear if that is a pose or reference frame, and the
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
				strncpy(error_text, line.c_str(), 127);	/*Flawfinder: ignore*/
				return E_ST_NO_POS;
            }
            float_token_iter++;
		}
		LL_DEBUGS("BVH") << "Got " << floats.size() << " floats " << LL_ENDL;
		for (U32 j=0; j<mJoints.size(); j++)
		{
			Joint *joint = mJoints[j];
			joint->mKeys.push_back( Key() );
			Key &key = joint->mKeys.back();

			if (floats.size() < joint->mNumChannels)
			{
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


//------------------------------------------------------------------------
// LLBVHLoader::applyTranslation()
//------------------------------------------------------------------------
void LLBVHLoader::applyTranslations()
{
	for (Joint* joint : mJoints)
	{
		//----------------------------------------------------------------
		// Look for a translation for this joint.
		// If none, skip to next joint
		//----------------------------------------------------------------
		TranslationMap::iterator ti = mTranslations.find( joint->mName );
		if ( ti == mTranslations.end() )
		{
			continue;
		}

		Translation &trans = ti->second;

		//----------------------------------------------------------------
		// Set the ignore flag if necessary
		//----------------------------------------------------------------
		if ( trans.mIgnore )
		{
            //LL_INFOS() << "NOTE: Ignoring " << joint->mName.c_str() << LL_ENDL;
			joint->mIgnore = TRUE;
			continue;
		}

		//----------------------------------------------------------------
		// Set the output name
		//----------------------------------------------------------------
		if ( ! trans.mOutName.empty() )
		{
			//LL_INFOS() << "NOTE: Changing " << joint->mName.c_str() << " to " << trans.mOutName.c_str() << LL_ENDL;
			joint->mOutName = trans.mOutName;
		}

        //Allow joint position changes as of SL-318
        joint->mIgnorePositions = FALSE;
        if (joint->mNumChannels == 3)
        {
            joint->mIgnorePositions = TRUE;
        }

		//----------------------------------------------------------------
		// Set the relativepos flags if necessary
		//----------------------------------------------------------------
		if ( trans.mRelativePositionKey )
		{
//			LL_INFOS() << "NOTE: Removing 1st position offset from all keys for " << joint->mOutName.c_str() << LL_ENDL;
			joint->mRelativePositionKey = TRUE;
		}

		if ( trans.mRelativeRotationKey )
		{
//			LL_INFOS() << "NOTE: Removing 1st rotation from all keys for " << joint->mOutName.c_str() << LL_ENDL;
			joint->mRelativeRotationKey = TRUE;
		}
		
		if ( trans.mRelativePosition.magVec() > 0.0f )
		{
			joint->mRelativePosition = trans.mRelativePosition;
//			LL_INFOS() << "NOTE: Removing " <<
//				joint->mRelativePosition.mV[0] << " " <<
//				joint->mRelativePosition.mV[1] << " " <<
//				joint->mRelativePosition.mV[2] <<
//				" from all position keys in " <<
//				joint->mOutName.c_str() << LL_ENDL;
		}

		//----------------------------------------------------------------
		// Set change of coordinate frame
		//----------------------------------------------------------------
		joint->mFrameMatrix = trans.mFrameMatrix;
		joint->mOffsetMatrix = trans.mOffsetMatrix;

		//----------------------------------------------------------------
		// Set mergeparent name
		//----------------------------------------------------------------
		if ( ! trans.mMergeParentName.empty() )
		{
//			LL_INFOS() << "NOTE: Merging "  << joint->mOutName.c_str() << 
//				" with parent " << 
//				trans.mMergeParentName.c_str() << LL_ENDL;
			joint->mMergeParentName = trans.mMergeParentName;
		}

		//----------------------------------------------------------------
		// Set mergechild name
		//----------------------------------------------------------------
		if ( ! trans.mMergeChildName.empty() )
		{
//			LL_INFOS() << "NOTE: Merging " << joint->mName.c_str() <<
//				" with child " << trans.mMergeChildName.c_str() << LL_ENDL;
			joint->mMergeChildName = trans.mMergeChildName;
		}

		//----------------------------------------------------------------
		// Set joint priority
		//----------------------------------------------------------------
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

	for (Joint* joint : mJoints)
	{
		BOOL pos_changed = FALSE;
		BOOL rot_changed = FALSE;

		if ( ! joint->mIgnore )
		{
			joint->mNumPosKeys = 0;
			joint->mNumRotKeys = 0;
			LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );

			KeyVector::iterator first_key = joint->mKeys.begin();

			// no keys?
			if (first_key == joint->mKeys.end())
			{
				joint->mIgnore = TRUE;
				continue;
			}

			LLVector3 first_frame_pos(first_key->mPos);
			LLQuaternion first_frame_rot = mayaQ( first_key->mRot[0], first_key->mRot[1], first_key->mRot[2], order);
	
			// skip first key
			KeyVector::iterator ki = joint->mKeys.begin();
			if (joint->mKeys.size() == 1)
			{
				// *FIX: use single frame to move pelvis
				// if only one keyframe force output for this joint
				rot_changed = TRUE;
			}
			else
			{
				// if more than one keyframe, use first frame as reference and skip to second
				first_key->mIgnorePos = TRUE;
				first_key->mIgnoreRot = TRUE;
				++ki;
			}

			KeyVector::iterator ki_prev = ki;
			KeyVector::iterator ki_last_good_pos = ki;
			KeyVector::iterator ki_last_good_rot = ki;
			S32 numPosFramesConsidered = 2;
			S32 numRotFramesConsidered = 2;

			F32 rot_threshold = ROTATION_KEYFRAME_THRESHOLD / llmax((F32)joint->mChildTreeMaxDepth * 0.33f, 1.f);

			double diff_max = 0;
			KeyVector::iterator ki_max = ki;
			for (; ki != joint->mKeys.end(); ++ki)
			{
				if (ki_prev == ki_last_good_pos)
				{
					joint->mNumPosKeys++;
					if (dist_vec_squared(LLVector3(ki_prev->mPos), first_frame_pos) > POSITION_MOTION_THRESHOLD_SQUARED)
					{
						pos_changed = TRUE;
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
						pos_changed = TRUE;
					}

					if (dist_vec_squared(interp_pos, test_pos) < POSITION_KEYFRAME_THRESHOLD_SQUARED)
					{
						ki_prev->mIgnorePos = TRUE;
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
						rot_changed = TRUE;
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
						rot_changed = TRUE;
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
							if (ki_max->mIgnoreRot == TRUE)
							{
								ki_max->mIgnoreRot = FALSE;
								joint->mNumRotKeys++;
							}
							diff_max = 0;
						}
					}
					else
					{
						// This keyframe isn't significant enough, throw it away.
						ki_prev->mIgnoreRot = TRUE;
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
			//LL_INFOS() << "Ignoring joint " << joint->mName << LL_ENDL;
			joint->mIgnore = TRUE;
		}
	}
}

void LLBVHLoader::reset()
{
	mLineNumber = 0;
	mNumFrames = 0;
	mFrameTime = 0.0f;
	mDuration = 0.0f;

	mPriority = 2;
	mLoop = FALSE;
	mLoopInPoint = 0.f;
	mLoopOutPoint = 0.f;
	mEaseIn = 0.3f;
	mEaseOut = 0.3f;
	mHand = 1;
	mInitialized = FALSE;

	mEmoteName = "";
	mLineNumber = 0;
	mTranslations.clear();
	mConstraints.clear();
}

//------------------------------------------------------------------------
// LLBVHLoader::getLine()
//------------------------------------------------------------------------
BOOL LLBVHLoader::getLine(apr_file_t* fp)
{
	if (apr_file_eof(fp) == APR_EOF)
	{
		return FALSE;
	}
	if ( apr_file_gets(mLine, BVH_PARSER_LINE_SIZE, fp) == APR_SUCCESS)
	{
		mLineNumber++;
		return TRUE;
	}

	return FALSE;
}

// returns required size of output buffer
U32 LLBVHLoader::getOutputSize()
{
	LLDataPackerBinaryBuffer dp;
	serialize(dp);

	return dp.getCurrentSize();
}

// writes contents to datapacker
BOOL LLBVHLoader::serialize(LLDataPacker& dp)
{
	F32 time;

	// count number of non-ignored joints
	S32 numJoints = 0;
	for (Joint* joint : mJoints)
	{
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

	for (Joint* joint : mJoints)
	{
		// if ignored, skip it
		if ( joint->mIgnore )
			continue;

		LLQuaternion first_frame_rot;
		LLQuaternion fixup_rot;

		dp.packString(joint->mOutName, "joint_name");
		dp.packS32(joint->mPriority, "joint_priority");

		// compute coordinate frame rotation
		LLQuaternion frameRot( joint->mFrameMatrix );
		LLQuaternion frameRotInv = ~frameRot;

		LLQuaternion offsetRot( joint->mOffsetMatrix );

		// find mergechild and mergeparent joints, if specified
		LLQuaternion mergeParentRot;
		LLQuaternion mergeChildRot;
		Joint *mergeParent = NULL;
		Joint *mergeChild = NULL;

		for (Joint* mjoint : mJoints)
		{
			if ( !joint->mMergeParentName.empty() && (mjoint->mName == joint->mMergeParentName) )
			{
				mergeParent = mjoint;
			}
			if ( !joint->mMergeChildName.empty() && (mjoint->mName == joint->mMergeChildName) )
			{
				mergeChild = mjoint;
			}
		}

		dp.packS32(joint->mNumRotKeys, "num_rot_keys");

		LLQuaternion::Order order = bvhStringToOrder( joint->mOrder );
		S32 outcount = 0;
		S32 frame = 0;
		for (Key& key : joint->mKeys)
		{

			if ((frame == 0) && joint->mRelativeRotationKey)
			{
				first_frame_rot = mayaQ( key.mRot[0], key.mRot[1], key.mRot[2], order);
				
				fixup_rot.shortestArc(LLVector3::z_axis * first_frame_rot * frameRot, LLVector3::z_axis);
			}

			if (key.mIgnoreRot)
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

			LLQuaternion inRot = mayaQ( key.mRot[0], key.mRot[1], key.mRot[2], order);

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
			for (Key& key : joint->mKeys)
			{
				if ((frame == 0) && joint->mRelativePositionKey)
				{
					relKey.setVec(key.mPos);
				}

				if (key.mIgnorePos)
				{
					frame++;
					continue;
				}

				time = llmax((F32)(frame - NUMBER_OF_IGNORED_FRAMES_AT_START), 0.0f) * mFrameTime; // Time elapsed before this frame starts.

				LLVector3 inPos = (LLVector3(key.mPos) - relKey) * ~first_frame_rot;// * fixup_rot;
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

	for (Constraint& constraint : mConstraints)
		{
			U8 byte = constraint.mChainLength;
			dp.packU8(byte, "chain_length");
			
			byte = constraint.mConstraintType;
			dp.packU8(byte, "constraint_type");
			dp.packBinaryDataFixed((U8*)constraint.mSourceJointName, 16, "source_volume");
			dp.packVector3(constraint.mSourceOffset, "source_offset");
			dp.packBinaryDataFixed((U8*)constraint.mTargetJointName, 16, "target_volume");
			dp.packVector3(constraint.mTargetOffset, "target_offset");
			dp.packVector3(constraint.mTargetDir, "target_dir");
			dp.packF32(constraint.mEaseInStart,	"ease_in_start");
			dp.packF32(constraint.mEaseInStop,	"ease_in_stop");
			dp.packF32(constraint.mEaseOutStart,	"ease_out_start");
			dp.packF32(constraint.mEaseOutStop,	"ease_out_stop");
		}

	
	return TRUE;
}
