/** 
 * @File llavatarappearance.cpp
 * @brief Implementation of LLAvatarAppearance class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#if LL_MSVC
// disable warning about boost::lexical_cast returning uninitialized data
// when it fails to parse the string
#pragma warning (disable:4701)
#endif

#include "linden_common.h"

#include "llavatarappearance.h"
#include "llavatarappearancedefines.h"
#include "llavatarjointmesh.h"
#include "llstl.h"
#include "lldir.h"
#include "llpolymorph.h"
#include "llpolymesh.h"
#include "llpolyskeletaldistortion.h"
#include "llstl.h"
#include "lltexglobalcolor.h"
#include "llwearabledata.h"
#include "boost/bind.hpp"
#include "boost/tokenizer.hpp"


#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

#include <boost/lexical_cast.hpp>

using namespace LLAvatarAppearanceDefines;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

const std::string AVATAR_DEFAULT_CHAR = "avatar";
const LLColor4 DUMMY_COLOR = LLColor4(0.5,0.5,0.5,1.0);

/*********************************************************************************
 **                                                                             **
 ** Begin private LLAvatarAppearance Support classes
 **
 **/

//------------------------------------------------------------------------
// LLAvatarBoneInfo
// Trans/Scale/Rot etc. info about each avatar bone.  Used by LLVOAvatarSkeleton.
//------------------------------------------------------------------------
class LLAvatarBoneInfo
{
	friend class LLAvatarAppearance;
	friend class LLAvatarSkeletonInfo;
public:
	LLAvatarBoneInfo() : mIsJoint(false) {}
	~LLAvatarBoneInfo()
	{
		std::for_each(mChildren.begin(), mChildren.end(), DeletePointer());
		mChildren.clear();
	}
	bool parseXml(LLXmlTreeNode* node);
	
private:
	std::string mName;
    std::string mSupport;
    std::string mAliases;
	bool mIsJoint;
	LLVector3 mPos;
    LLVector3 mEnd;
	LLVector3 mRot;
	LLVector3 mScale;
	LLVector3 mPivot;
	typedef std::vector<LLAvatarBoneInfo*> bones_t;
	bones_t mChildren;
};

//------------------------------------------------------------------------
// LLAvatarSkeletonInfo
// Overall avatar skeleton
//------------------------------------------------------------------------
class LLAvatarSkeletonInfo
{
	friend class LLAvatarAppearance;
public:
	LLAvatarSkeletonInfo() :
		mNumBones(0), mNumCollisionVolumes(0) {}
	~LLAvatarSkeletonInfo()
	{
		std::for_each(mBoneInfoList.begin(), mBoneInfoList.end(), DeletePointer());
		mBoneInfoList.clear();
	}
	bool parseXml(LLXmlTreeNode* node);
	S32 getNumBones() const { return mNumBones; }
	S32 getNumCollisionVolumes() const { return mNumCollisionVolumes; }
	
private:
	S32 mNumBones;
	S32 mNumCollisionVolumes;
    LLAvatarAppearance::joint_alias_map_t mJointAliasMap;
	typedef std::vector<LLAvatarBoneInfo*> bone_info_list_t;
	bone_info_list_t mBoneInfoList;
};

//-----------------------------------------------------------------------------
// LLAvatarXmlInfo
//-----------------------------------------------------------------------------

LLAvatarAppearance::LLAvatarXmlInfo::LLAvatarXmlInfo()
	: mTexSkinColorInfo(0), mTexHairColorInfo(0), mTexEyeColorInfo(0)
{
}

LLAvatarAppearance::LLAvatarXmlInfo::~LLAvatarXmlInfo()
{
	std::for_each(mMeshInfoList.begin(), mMeshInfoList.end(), DeletePointer());
	mMeshInfoList.clear();

	std::for_each(mSkeletalDistortionInfoList.begin(), mSkeletalDistortionInfoList.end(), DeletePointer());		
	mSkeletalDistortionInfoList.clear();

	std::for_each(mAttachmentInfoList.begin(), mAttachmentInfoList.end(), DeletePointer());
	mAttachmentInfoList.clear();

	delete_and_clear(mTexSkinColorInfo);
	delete_and_clear(mTexHairColorInfo);
	delete_and_clear(mTexEyeColorInfo);

	std::for_each(mLayerInfoList.begin(), mLayerInfoList.end(), DeletePointer());		
	mLayerInfoList.clear();

	std::for_each(mDriverInfoList.begin(), mDriverInfoList.end(), DeletePointer());
	mDriverInfoList.clear();

	std::for_each(mMorphMaskInfoList.begin(), mMorphMaskInfoList.end(), DeletePointer());
	mMorphMaskInfoList.clear();
}


/**
 **
 ** End LLAvatarAppearance Support classes
 **                                                                             **
 *********************************************************************************/

//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
LLAvatarSkeletonInfo* LLAvatarAppearance::sAvatarSkeletonInfo = NULL;
LLAvatarAppearance::LLAvatarXmlInfo* LLAvatarAppearance::sAvatarXmlInfo = NULL;
LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary* LLAvatarAppearance::sAvatarDictionary = NULL;


LLAvatarAppearance::LLAvatarAppearance(LLWearableData* wearable_data) :
	LLCharacter(),
	mIsDummy(false),
	mTexSkinColor( NULL ),
	mTexHairColor( NULL ),
	mTexEyeColor( NULL ),
	mPelvisToFoot(0.f),
	mHeadOffset(),
	mRoot(NULL),
	mWearableData(wearable_data),
    mNumBones(0),
    mNumCollisionVolumes(0),
    mCollisionVolumes(NULL),
    mIsBuilt(false),
    mInitFlags(0)
{
	llassert_always(mWearableData);
	mBakedTextureDatas.resize(LLAvatarAppearanceDefines::BAKED_NUM_INDICES);
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++ )
	{
		mBakedTextureDatas[i].mLastTextureID = IMG_DEFAULT_AVATAR;
		mBakedTextureDatas[i].mTexLayerSet = NULL;
		mBakedTextureDatas[i].mIsLoaded = false;
		mBakedTextureDatas[i].mIsUsed = false;
		mBakedTextureDatas[i].mMaskTexName = 0;
		mBakedTextureDatas[i].mTextureIndex = sAvatarDictionary->bakedToLocalTextureIndex((LLAvatarAppearanceDefines::EBakedTextureIndex)i);
	}
}

// virtual
void LLAvatarAppearance::initInstance()
{
	//-------------------------------------------------------------------------
	// initialize joint, mesh and shape members
	//-------------------------------------------------------------------------
	mRoot = createAvatarJoint();
	mRoot->setName( "mRoot" );

	for (const LLAvatarAppearanceDictionary::MeshEntries::value_type& mesh_pair : sAvatarDictionary->getMeshEntries())
	{
		const EMeshIndex mesh_index = mesh_pair.first;
		const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = mesh_pair.second;
		LLAvatarJoint* joint = createAvatarJoint();
		joint->setName(mesh_dict->mName);
		joint->setMeshID(mesh_index);
		mMeshLOD.push_back(joint);
		
		/* mHairLOD.setName("mHairLOD");
		   mHairMesh0.setName("mHairMesh0");
		   mHairMesh0.setMeshID(MESH_ID_HAIR);
		   mHairMesh1.setName("mHairMesh1"); */
		for (U32 lod = 0; lod < mesh_dict->mLOD; lod++)
		{
			LLAvatarJointMesh* mesh = createAvatarJointMesh();
			std::string mesh_name = "m" + mesh_dict->mName + boost::lexical_cast<std::string>(lod);
			// We pre-pended an m - need to capitalize first character for camelCase
			mesh_name[1] = toupper(mesh_name[1]);
			mesh->setName(mesh_name);
			mesh->setMeshID(mesh_index);
			mesh->setPickName(mesh_dict->mPickName);
			mesh->setIsTransparent(false);
			switch((S32)mesh_index)
			{
				case MESH_ID_HAIR:
					mesh->setIsTransparent(true);
					break;
				case MESH_ID_SKIRT:
					mesh->setIsTransparent(true);
					break;
				case MESH_ID_EYEBALL_LEFT:
				case MESH_ID_EYEBALL_RIGHT:
					mesh->setSpecular( LLColor4( 1.0f, 1.0f, 1.0f, 1.0f ), 1.f );
					break;
			}
			
			joint->mMeshParts.push_back(mesh);
		}
	}

	//-------------------------------------------------------------------------
	// associate baked textures with meshes
	//-------------------------------------------------------------------------
	for (const LLAvatarAppearanceDictionary::MeshEntries::value_type& mesh_pair : sAvatarDictionary->getMeshEntries())
	{
		const EMeshIndex mesh_index = mesh_pair.first;
		const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = mesh_pair.second;
		const EBakedTextureIndex baked_texture_index = mesh_dict->mBakedID;
		// Skip it if there's no associated baked texture.
		if (baked_texture_index == BAKED_NUM_INDICES) continue;
		
		for (LLAvatarJointMesh* mesh : mMeshLOD[mesh_index]->mMeshParts)
		{
			mBakedTextureDatas[(S32)baked_texture_index].mJointMeshes.push_back(mesh);
		}
	}

	buildCharacter();

    mInitFlags |= 1<<0;

}

// virtual
LLAvatarAppearance::~LLAvatarAppearance()
{
	delete_and_clear(mTexSkinColor);
	delete_and_clear(mTexHairColor);
	delete_and_clear(mTexEyeColor);

	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		delete_and_clear(mBakedTextureDatas[i].mTexLayerSet);
		mBakedTextureDatas[i].mJointMeshes.clear();

		for (LLMaskedMorph* masked_morph : mBakedTextureDatas[i].mMaskedMorphs)
		{
			delete masked_morph;
		}
	}

	if (mRoot) 
	{
		mRoot->removeAllChildren();
		delete mRoot;
		mRoot = nullptr;
	}
	mJointMap.clear();

	clearSkeleton();
	delete_and_clear_array(mCollisionVolumes);

	std::for_each(mPolyMeshes.begin(), mPolyMeshes.end(), DeletePairedPointer());
	mPolyMeshes.clear();

	for (LLAvatarJoint* joint : mMeshLOD)
	{
		std::for_each(joint->mMeshParts.begin(), joint->mMeshParts.end(), DeletePointer());
		joint->mMeshParts.clear();
	}
	std::for_each(mMeshLOD.begin(), mMeshLOD.end(), DeletePointer());
	mMeshLOD.clear();
}

//static
void LLAvatarAppearance::initClass()
{
    initClass("","");
}

//static
void LLAvatarAppearance::initClass(const std::string& avatar_file_name_arg, const std::string& skeleton_file_name_arg)
{
    // init dictionary (don't repeat on second login attempt)
    if (!sAvatarDictionary)
    {
        sAvatarDictionary = new LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary();
    }

	std::string avatar_file_name;

    if (!avatar_file_name_arg.empty())
    {
        avatar_file_name = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,avatar_file_name_arg);
    }
    else
    {
        avatar_file_name = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,AVATAR_DEFAULT_CHAR + "_lad.xml");
    }
	LLXmlTree xml_tree;
	bool success = xml_tree.parseFile( avatar_file_name, false );
	if (!success)
	{
		LL_ERRS() << "Problem reading avatar configuration file:" << avatar_file_name << LL_ENDL;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = xml_tree.getRoot();
	if (!root) 
	{
		LL_ERRS() << "No root node found in avatar configuration file: " << avatar_file_name << LL_ENDL;
		return;
	}

	//-------------------------------------------------------------------------
	// <linden_avatar version="2.0"> (root)
	//-------------------------------------------------------------------------
	if( !root->hasName( "linden_avatar" ) )
	{
		LL_ERRS() << "Invalid avatar file header: " << avatar_file_name << LL_ENDL;
	}
	
	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || ((version != "1.0") && (version != "2.0")))
	{
		LL_ERRS() << "Invalid avatar file version: " << version << " in file: " << avatar_file_name << LL_ENDL;
	}

	S32 wearable_def_version = 1;
	static LLStdStringHandle wearable_definition_version_string = LLXmlTree::addAttributeString("wearable_definition_version");
	root->getFastAttributeS32( wearable_definition_version_string, wearable_def_version );
	LLWearable::setCurrentDefinitionVersion( wearable_def_version );

	std::string mesh_file_name;

	LLXmlTreeNode* skeleton_node = root->getChildByName( "skeleton" );
	if (!skeleton_node)
	{
		LL_ERRS() << "No skeleton in avatar configuration file: " << avatar_file_name << LL_ENDL;
		return;
	}

    std::string skeleton_file_name = skeleton_file_name_arg;
    if (skeleton_file_name.empty())
    {
        static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
        if (!skeleton_node->getFastAttributeString(file_name_string, skeleton_file_name))
        {
            LL_ERRS() << "No file name in skeleton node in avatar config file: " << avatar_file_name << LL_ENDL;
        }
    }
	
	std::string skeleton_path;
	LLXmlTree skeleton_xml_tree;
	skeleton_path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,skeleton_file_name);
	if (!parseSkeletonFile(skeleton_path, skeleton_xml_tree))
	{
		LL_ERRS() << "Error parsing skeleton file: " << skeleton_path << LL_ENDL;
	}

	// Process XML data

	// avatar_skeleton.xml
	if (sAvatarSkeletonInfo)
	{ //this can happen if a login attempt failed
		delete sAvatarSkeletonInfo;
	}
	sAvatarSkeletonInfo = new LLAvatarSkeletonInfo;
	if (!sAvatarSkeletonInfo->parseXml(skeleton_xml_tree.getRoot()))
	{
		LL_ERRS() << "Error parsing skeleton XML file: " << skeleton_path << LL_ENDL;
	}
	// parse avatar_lad.xml
	if (sAvatarXmlInfo)
	{ //this can happen if a login attempt failed
		delete_and_clear(sAvatarXmlInfo);
	}
	sAvatarXmlInfo = new LLAvatarXmlInfo;
	if (!sAvatarXmlInfo->parseXmlSkeletonNode(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
	if (!sAvatarXmlInfo->parseXmlMeshNodes(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
	if (!sAvatarXmlInfo->parseXmlColorNodes(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
	if (!sAvatarXmlInfo->parseXmlLayerNodes(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
	if (!sAvatarXmlInfo->parseXmlDriverNodes(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
	if (!sAvatarXmlInfo->parseXmlMorphNodes(root))
	{
		LL_ERRS() << "Error parsing skeleton node in avatar XML file: " << skeleton_path << LL_ENDL;
	}
}

void LLAvatarAppearance::cleanupClass()
{
	delete_and_clear(sAvatarXmlInfo);
    delete_and_clear(sAvatarDictionary);
    delete_and_clear(sAvatarSkeletonInfo);
}

using namespace LLAvatarAppearanceDefines;

void LLAvatarAppearance::compareJointStateMaps(joint_state_map_t& last_state,
                                               joint_state_map_t& curr_state)
{
    if (!last_state.empty() && (last_state != curr_state))
    {
        S32 diff_count = 0;
        for (joint_state_map_t::value_type& pair : last_state)
        {
            const std::string& key = pair.first;
            if (last_state[key] != curr_state[key])
            {
                LL_DEBUGS("AvatarBodySize") << "BodySize change " << key << " " << last_state[key] << "->" << curr_state[key] << LL_ENDL;
                diff_count++;
            }
        }
        if (diff_count > 0)
        {
            LL_DEBUGS("AvatarBodySize") << "Total of BodySize changes " << diff_count << LL_ENDL;
        }
        
    }
}

//------------------------------------------------------------------------
// The viewer can only suggest a good size for the agent,
// the simulator will keep it inside a reasonable range.
void LLAvatarAppearance::computeBodySize() 
{
    mLastBodySizeState = mCurrBodySizeState;

    mCurrBodySizeState["mPelvis scale"] = mPelvisp->getScale();
    mCurrBodySizeState["mSkull pos"] = mSkullp->getPosition();
    mCurrBodySizeState["mSkull scale"] = mSkullp->getScale();
    mCurrBodySizeState["mNeck pos"] = mNeckp->getPosition();
    mCurrBodySizeState["mNeck scale"] = mNeckp->getScale();
    mCurrBodySizeState["mChest pos"] = mChestp->getPosition();
    mCurrBodySizeState["mChest scale"] = mChestp->getScale();
    mCurrBodySizeState["mHead pos"] = mHeadp->getPosition();
    mCurrBodySizeState["mHead scale"] = mHeadp->getScale();
    mCurrBodySizeState["mTorso pos"] = mTorsop->getPosition();
    mCurrBodySizeState["mTorso scale"] = mTorsop->getScale();
    mCurrBodySizeState["mHipLeft pos"] = mHipLeftp->getPosition();
    mCurrBodySizeState["mHipLeft scale"] = mHipLeftp->getScale();
    mCurrBodySizeState["mKneeLeft pos"] = mKneeLeftp->getPosition();
    mCurrBodySizeState["mKneeLeft scale"] = mKneeLeftp->getScale();
    mCurrBodySizeState["mAnkleLeft pos"] = mAnkleLeftp->getPosition();
    mCurrBodySizeState["mAnkleLeft scale"] = mAnkleLeftp->getScale();
    mCurrBodySizeState["mFootLeft pos"] = mFootLeftp->getPosition();

	LLVector3 pelvis_scale = mPelvisp->getScale();

	// some of the joints have not been cached
	LLVector3 skull = mSkullp->getPosition();
	//LLVector3 skull_scale = mSkullp->getScale();

	LLVector3 neck = mNeckp->getPosition();
	LLVector3 neck_scale = mNeckp->getScale();

	LLVector3 chest = mChestp->getPosition();
	LLVector3 chest_scale = mChestp->getScale();

	// the rest of the joints have been cached
	LLVector3 head = mHeadp->getPosition();
	LLVector3 head_scale = mHeadp->getScale();

	LLVector3 torso = mTorsop->getPosition();
	LLVector3 torso_scale = mTorsop->getScale();

	LLVector3 hip = mHipLeftp->getPosition();
	LLVector3 hip_scale = mHipLeftp->getScale();

	LLVector3 knee = mKneeLeftp->getPosition();
	LLVector3 knee_scale = mKneeLeftp->getScale();

	LLVector3 ankle = mAnkleLeftp->getPosition();
	LLVector3 ankle_scale = mAnkleLeftp->getScale();

	LLVector3 foot  = mFootLeftp->getPosition();

	F32 old_offset = mAvatarOffset.mV[VZ];

	mAvatarOffset.mV[VZ] = getVisualParamWeight(AVATAR_HOVER);

	mPelvisToFoot = hip.mV[VZ] * pelvis_scale.mV[VZ] -
				 	knee.mV[VZ] * hip_scale.mV[VZ] -
				 	ankle.mV[VZ] * knee_scale.mV[VZ] -
				 	foot.mV[VZ] * ankle_scale.mV[VZ];

	LLVector3 new_body_size;
	new_body_size.mV[VZ] = mPelvisToFoot +
					   // the sqrt(2) correction below is an approximate
					   // correction to get to the top of the head
					   F_SQRT2 * (skull.mV[VZ] * head_scale.mV[VZ]) + 
					   head.mV[VZ] * neck_scale.mV[VZ] + 
					   neck.mV[VZ] * chest_scale.mV[VZ] + 
					   chest.mV[VZ] * torso_scale.mV[VZ] + 
					   torso.mV[VZ] * pelvis_scale.mV[VZ]; 

	// TODO -- measure the real depth and width
	new_body_size.mV[VX] = DEFAULT_AGENT_DEPTH;
	new_body_size.mV[VY] = DEFAULT_AGENT_WIDTH;

	mAvatarOffset.mV[VX] = 0.0f;
	mAvatarOffset.mV[VY] = 0.0f;

	if (new_body_size != mBodySize || old_offset != mAvatarOffset.mV[VZ])
	{
		mBodySize = new_body_size;

        compareJointStateMaps(mLastBodySizeState, mCurrBodySizeState);
	}
}

//-----------------------------------------------------------------------------
// parseSkeletonFile()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::parseSkeletonFile(const std::string& filename, LLXmlTree& skeleton_xml_tree)
{
	//-------------------------------------------------------------------------
	// parse the file
	//-------------------------------------------------------------------------
	bool parsesuccess = skeleton_xml_tree.parseFile( filename, false );

	if (!parsesuccess)
	{
		LL_ERRS() << "Can't parse skeleton file: " << filename << LL_ENDL;
		return false;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = skeleton_xml_tree.getRoot();
	if (!root) 
	{
		LL_ERRS() << "No root node found in avatar skeleton file: " << filename << LL_ENDL;
		return false;
	}

	if( !root->hasName( "linden_skeleton" ) )
	{
		LL_ERRS() << "Invalid avatar skeleton file header: " << filename << LL_ENDL;
		return false;
	}

	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || ((version != "1.0") && (version != "2.0")))
	{
		LL_ERRS() << "Invalid avatar skeleton file version: " << version << " in file: " << filename << LL_ENDL;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// setupBone()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::setupBone(const LLAvatarBoneInfo* info, LLJoint* parent, S32 &volume_num, S32 &joint_num)
{
	LLJoint* joint = NULL;

    LL_DEBUGS("BVH") << "bone info: name " << info->mName
                     << " isJoint " << info->mIsJoint
                     << " volume_num " << volume_num
                     << " joint_num " << joint_num
                     << LL_ENDL;

	if (info->mIsJoint)
	{
		joint = getCharacterJoint(joint_num);
		if (!joint)
		{
			LL_WARNS() << "Too many bones" << LL_ENDL;
			return false;
		}
		joint->setName( info->mName );
	}
	else // collision volume
	{
		if (volume_num >= (S32)mNumCollisionVolumes)
		{
			LL_WARNS() << "Too many collision volumes" << LL_ENDL;
			return false;
		}
		joint = (&mCollisionVolumes[volume_num]);
		joint->setName( info->mName );
	}

	// add to parent
	if (parent && (joint->getParent()!=parent))
	{
		parent->addChild( joint );
	}

	// SL-315
	joint->setPosition(info->mPos);
    joint->setDefaultPosition(info->mPos);
	joint->setRotation(mayaQ(info->mRot.mV[VX], info->mRot.mV[VY],
							 info->mRot.mV[VZ], LLQuaternion::XYZ));
	joint->setScale(info->mScale);
	joint->setDefaultScale(info->mScale);
    joint->setSupport(info->mSupport);
	joint->setEnd(info->mEnd);

	if (info->mIsJoint)
	{
		joint->setSkinOffset( info->mPivot );
        joint->setJointNum(joint_num);
		joint_num++;
	}
	else // collision volume
	{
        joint->setJointNum(mNumBones+volume_num);
		volume_num++;
	}


	// setup children
	for (LLAvatarBoneInfo* child_info : info->mChildren)
	{
		if (!setupBone(child_info, joint, volume_num, joint_num))
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// allocateCharacterJoints()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::allocateCharacterJoints( U32 num )
{
    if (mSkeleton.size() != num)
    {
        clearSkeleton();
        mSkeleton = avatar_joint_list_t(num,NULL);
        mNumBones = num;
    }

	return true;
}


//-----------------------------------------------------------------------------
// buildSkeleton()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::buildSkeleton(const LLAvatarSkeletonInfo *info)
{
    LL_DEBUGS("BVH") << "numBones " << info->mNumBones << " numCollisionVolumes " << info->mNumCollisionVolumes << LL_ENDL;

	// allocate joints
	if (!allocateCharacterJoints(info->mNumBones))
	{
		LL_ERRS() << "Can't allocate " << info->mNumBones << " joints" << LL_ENDL;
		return false;
	}
	
	// allocate volumes
	if (info->mNumCollisionVolumes)
	{
		if (!allocateCollisionVolumes(info->mNumCollisionVolumes))
		{
			LL_ERRS() << "Can't allocate " << info->mNumCollisionVolumes << " collision volumes" << LL_ENDL;
			return false;
		}
	}

	S32 current_joint_num = 0;
	S32 current_volume_num = 0;
	for (LLAvatarBoneInfo* bone_info : info->mBoneInfoList)
	{
		if (!setupBone(bone_info, NULL, current_volume_num, current_joint_num))
		{
			LL_ERRS() << "Error parsing bone in skeleton file" << LL_ENDL;
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// clearSkeleton()
//-----------------------------------------------------------------------------
void LLAvatarAppearance::clearSkeleton()
{
	std::for_each(mSkeleton.begin(), mSkeleton.end(), DeletePointer());
	mSkeleton.clear();
}

//------------------------------------------------------------------------
// addPelvisFixup
//------------------------------------------------------------------------
void LLAvatarAppearance::addPelvisFixup( F32 fixup, const LLUUID& mesh_id ) 
{
	LLVector3 pos(0.0,0.0,fixup);
	mPelvisFixups.add(mesh_id,pos);
}

//------------------------------------------------------------------------
// addPelvisFixup
//------------------------------------------------------------------------
void LLAvatarAppearance::removePelvisFixup( const LLUUID& mesh_id )
{
	mPelvisFixups.remove(mesh_id);
}

//------------------------------------------------------------------------
// hasPelvisFixup
//------------------------------------------------------------------------
bool LLAvatarAppearance::hasPelvisFixup( F32& fixup, LLUUID& mesh_id ) const
{
	LLVector3 pos;
	if (mPelvisFixups.findActiveOverride(mesh_id,pos))
	{
		fixup = pos[2];
		return true;
	}
	return false;
}

bool LLAvatarAppearance::hasPelvisFixup( F32& fixup ) const
{
	LLUUID mesh_id;
	return hasPelvisFixup( fixup, mesh_id );
}
//-----------------------------------------------------------------------------
// LLAvatarAppearance::buildCharacter()
// Deferred initialization and rebuild of the avatar.
//-----------------------------------------------------------------------------
void LLAvatarAppearance::buildCharacter()
{
	//-------------------------------------------------------------------------
	// remove all references to our existing skeleton
	// so we can rebuild it
	//-------------------------------------------------------------------------
	flushAllMotions();

	//-------------------------------------------------------------------------
	// remove all of mRoot's children
	//-------------------------------------------------------------------------
	mRoot->removeAllChildren();
	mJointMap.clear();
	mIsBuilt = false;

	//-------------------------------------------------------------------------
	// clear mesh data
	//-------------------------------------------------------------------------
	for (LLAvatarJoint* joint : mMeshLOD)
	{
		for (LLAvatarJointMesh* mesh : joint->mMeshParts)
		{
			mesh->setMesh(NULL);
		}
	}

	//-------------------------------------------------------------------------
	// (re)load our skeleton and meshes
	//-------------------------------------------------------------------------
	LLTimer timer;

	bool status = loadAvatar();
	stop_glerror();

// 	gPrintMessagesThisFrame = true;
	LL_DEBUGS() << "Avatar load took " << timer.getElapsedTimeF32() << " seconds." << LL_ENDL;

	if (!status)
	{
		if (isSelf())
		{
			LL_ERRS() << "Unable to load user's avatar" << LL_ENDL;
		}
		else
		{
			LL_WARNS() << "Unable to load other's avatar" << LL_ENDL;
		}
		return;
	}

	//-------------------------------------------------------------------------
	// initialize "well known" joint pointers
	//-------------------------------------------------------------------------
	mPelvisp		= mRoot->findJoint("mPelvis");
	mTorsop			= mRoot->findJoint("mTorso");
	mChestp			= mRoot->findJoint("mChest");
	mNeckp			= mRoot->findJoint("mNeck");
	mHeadp			= mRoot->findJoint("mHead");
	mSkullp			= mRoot->findJoint("mSkull");
	mHipLeftp		= mRoot->findJoint("mHipLeft");
	mHipRightp		= mRoot->findJoint("mHipRight");
	mKneeLeftp		= mRoot->findJoint("mKneeLeft");
	mKneeRightp		= mRoot->findJoint("mKneeRight");
	mAnkleLeftp		= mRoot->findJoint("mAnkleLeft");
	mAnkleRightp	= mRoot->findJoint("mAnkleRight");
	mFootLeftp		= mRoot->findJoint("mFootLeft");
	mFootRightp		= mRoot->findJoint("mFootRight");
	mWristLeftp		= mRoot->findJoint("mWristLeft");
	mWristRightp	= mRoot->findJoint("mWristRight");
	mEyeLeftp		= mRoot->findJoint("mEyeLeft");
	mEyeRightp		= mRoot->findJoint("mEyeRight");

	//-------------------------------------------------------------------------
	// Make sure "well known" pointers exist
	//-------------------------------------------------------------------------
	if (!(mPelvisp && 
		  mTorsop &&
		  mChestp &&
		  mNeckp &&
		  mHeadp &&
		  mSkullp &&
		  mHipLeftp &&
		  mHipRightp &&
		  mKneeLeftp &&
		  mKneeRightp &&
		  mAnkleLeftp &&
		  mAnkleRightp &&
		  mFootLeftp &&
		  mFootRightp &&
		  mWristLeftp &&
		  mWristRightp &&
		  mEyeLeftp &&
		  mEyeRightp))
	{
		LL_ERRS() << "Failed to create avatar." << LL_ENDL;
		return;
	}

	//-------------------------------------------------------------------------
	// initialize the pelvis
	//-------------------------------------------------------------------------
	// SL-315
	mPelvisp->setPosition( LLVector3(0.0f, 0.0f, 0.0f) );

	mIsBuilt = true;
	stop_glerror();

}

bool LLAvatarAppearance::loadAvatar()
{
// 	LL_RECORD_BLOCK_TIME(FTM_LOAD_AVATAR);
	
	// avatar_skeleton.xml
	if( !buildSkeleton(sAvatarSkeletonInfo) )
	{
		LL_ERRS() << "avatar file: buildSkeleton() failed" << LL_ENDL;
		return false;
	}

	// initialize mJointAliasMap
	getJointAliases();

	// avatar_lad.xml : <skeleton>
	if( !loadSkeletonNode() )
	{
		LL_ERRS() << "avatar file: loadNodeSkeleton() failed" << LL_ENDL;
		return false;
	}
	
	// avatar_lad.xml : <mesh>
	if( !loadMeshNodes() )
	{
		LL_ERRS() << "avatar file: loadNodeMesh() failed" << LL_ENDL;
		return false;
	}
	
	// avatar_lad.xml : <global_color>
	if( sAvatarXmlInfo->mTexSkinColorInfo )
	{
		mTexSkinColor = new LLTexGlobalColor( this );
		if( !mTexSkinColor->setInfo( sAvatarXmlInfo->mTexSkinColorInfo ) )
		{
			LL_ERRS() << "avatar file: mTexSkinColor->setInfo() failed" << LL_ENDL;
			return false;
		}
	}
	else
	{
		LL_ERRS() << "<global_color> name=\"skin_color\" not found" << LL_ENDL;
		return false;
	}
	if( sAvatarXmlInfo->mTexHairColorInfo )
	{
		mTexHairColor = new LLTexGlobalColor( this );
		if( !mTexHairColor->setInfo( sAvatarXmlInfo->mTexHairColorInfo ) )
		{
			LL_ERRS() << "avatar file: mTexHairColor->setInfo() failed" << LL_ENDL;
			return false;
		}
	}
	else
	{
		LL_ERRS() << "<global_color> name=\"hair_color\" not found" << LL_ENDL;
		return false;
	}
	if( sAvatarXmlInfo->mTexEyeColorInfo )
	{
		mTexEyeColor = new LLTexGlobalColor( this );
		if( !mTexEyeColor->setInfo( sAvatarXmlInfo->mTexEyeColorInfo ) )
		{
			LL_ERRS() << "avatar file: mTexEyeColor->setInfo() failed" << LL_ENDL;
			return false;
		}
	}
	else
	{
		LL_ERRS() << "<global_color> name=\"eye_color\" not found" << LL_ENDL;
		return false;
	}
	
	// avatar_lad.xml : <layer_set>
	if (sAvatarXmlInfo->mLayerInfoList.empty())
	{
		LL_ERRS() << "avatar file: missing <layer_set> node" << LL_ENDL;
		return false;
	}

	if (sAvatarXmlInfo->mMorphMaskInfoList.empty())
	{
		LL_ERRS() << "avatar file: missing <morph_masks> node" << LL_ENDL;
		return false;
	}

	// avatar_lad.xml : <morph_masks>
	for (LLAvatarXmlInfo::LLAvatarMorphInfo* info : sAvatarXmlInfo->mMorphMaskInfoList)
	{
		EBakedTextureIndex baked = sAvatarDictionary->findBakedByRegionName(info->mRegion);
		if (baked != BAKED_NUM_INDICES)
		{
			LLVisualParam* morph_param;
			const std::string *name = &info->mName;
			morph_param = getVisualParam(name->c_str());
			if (morph_param)
			{
				bool invert = info->mInvert;
				addMaskedMorph(baked, morph_param, invert, info->mLayer);
			}
		}

	}

	loadLayersets();
	
	// avatar_lad.xml : <driver_parameters>
	for (LLDriverParamInfo* info : sAvatarXmlInfo->mDriverInfoList)
	{
		LLDriverParam* driver_param = new LLDriverParam( this );
		if (driver_param->setInfo(info))
		{
			addVisualParam( driver_param );
			driver_param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
			LLVisualParam*(LLAvatarAppearance::*avatar_function)(S32)const = &LLAvatarAppearance::getVisualParam; 
			if( !driver_param->linkDrivenParams(boost::bind(avatar_function,(LLAvatarAppearance*)this,_1 ), false))
			{
				LL_WARNS() << "could not link driven params for avatar " << getID().asString() << " param id: " << driver_param->getID() << LL_ENDL;
				continue;
			}
		}
		else
		{
			delete driver_param;
			LL_WARNS() << "avatar file: driver_param->parseData() failed" << LL_ENDL;
			return false;
		}
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// loadSkeletonNode(): loads <skeleton> node from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::loadSkeletonNode ()
{
	mRoot->addChild( mSkeleton[0] );

	// make meshes children before calling parent version of the function
	for (LLAvatarJoint* joint : mMeshLOD)
	{
		joint->mUpdateXform = false;
		joint->setMeshesToChildren();
	}

	mRoot->addChild(mMeshLOD[MESH_ID_HEAD]);
	mRoot->addChild(mMeshLOD[MESH_ID_EYELASH]);
	mRoot->addChild(mMeshLOD[MESH_ID_UPPER_BODY]);
	mRoot->addChild(mMeshLOD[MESH_ID_LOWER_BODY]);
	mRoot->addChild(mMeshLOD[MESH_ID_SKIRT]);

	LLAvatarJoint *skull = (LLAvatarJoint*)mRoot->findJoint("mSkull");
	if (skull)
	{
		skull->addChild(mMeshLOD[MESH_ID_HAIR] );
	}

	LLAvatarJoint *eyeL = (LLAvatarJoint*)mRoot->findJoint("mEyeLeft");
	if (eyeL)
	{
		eyeL->addChild( mMeshLOD[MESH_ID_EYEBALL_LEFT] );
	}

	LLAvatarJoint *eyeR = (LLAvatarJoint*)mRoot->findJoint("mEyeRight");
	if (eyeR)
	{
		eyeR->addChild( mMeshLOD[MESH_ID_EYEBALL_RIGHT] );
	}

	// SKELETAL DISTORTIONS
	{
		for (LLViewerVisualParamInfo* visual_param_info : sAvatarXmlInfo->mSkeletalDistortionInfoList)
		{
			LLPolySkeletalDistortionInfo *info = (LLPolySkeletalDistortionInfo*)visual_param_info;
			LLPolySkeletalDistortion *param = new LLPolySkeletalDistortion(this);
			if (!param->setInfo(info))
			{
				delete param;
				return false;
			}
			else
			{
				addVisualParam(param);
				param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
			}				
		}
	}


	return true;
}

//-----------------------------------------------------------------------------
// loadMeshNodes(): loads <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::loadMeshNodes()
{
	for (const LLAvatarXmlInfo::LLAvatarMeshInfo* info : sAvatarXmlInfo->mMeshInfoList)
	{
		const std::string &type = info->mType;
		S32 lod = info->mLOD;

		LLAvatarJointMesh* mesh = NULL;
		U8 mesh_id = 0;
		bool found_mesh_id = false;

		/* if (type == "hairMesh")
			switch(lod)
			  case 0:
				mesh = &mHairMesh0; */
		for (const LLAvatarAppearanceDictionary::MeshEntries::value_type& mesh_pair : sAvatarDictionary->getMeshEntries())
		{
			const EMeshIndex mesh_index = mesh_pair.first;
			const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = mesh_pair.second;
			if (type.compare(mesh_dict->mName) == 0)
			{
				mesh_id = mesh_index;
				found_mesh_id = true;
				break;
			}
		}

		if (found_mesh_id)
		{
			if (lod < (S32)mMeshLOD[mesh_id]->mMeshParts.size())
			{
				mesh = mMeshLOD[mesh_id]->mMeshParts[lod];
			}
			else
			{
				LL_WARNS() << "Avatar file: <mesh> has invalid lod setting " << lod << LL_ENDL;
				return false;
			}
		}
		else 
		{
			LL_WARNS() << "Ignoring unrecognized mesh type: " << type << LL_ENDL;
			return false;
		}

		//	LL_INFOS() << "Parsing mesh data for " << type << "..." << LL_ENDL;

		// If this isn't set to white (1.0), avatars will *ALWAYS* be darker than their surroundings.
		// Do not touch!!!
		mesh->setColor( LLColor4::white );

		LLPolyMesh *poly_mesh = NULL;

		if (!info->mReferenceMeshName.empty())
		{
			polymesh_map_t::const_iterator polymesh_iter = mPolyMeshes.find(info->mReferenceMeshName);
			if (polymesh_iter != mPolyMeshes.end())
			{
				poly_mesh = LLPolyMesh::getMesh(info->mMeshFileName, polymesh_iter->second);
				poly_mesh->setAvatar(this);
			}
			else
			{
				// This should never happen
				LL_WARNS("Avatar") << "Could not find avatar mesh: " << info->mReferenceMeshName << LL_ENDL;
                return false;
			}
		}
		else
		{
			poly_mesh = LLPolyMesh::getMesh(info->mMeshFileName);
			poly_mesh->setAvatar(this);
		}

		if( !poly_mesh )
		{
			LL_WARNS() << "Failed to load mesh of type " << type << LL_ENDL;
			return false;
		}

		// Multimap insert
		mPolyMeshes.insert(std::make_pair(info->mMeshFileName, poly_mesh));
	
		mesh->setMesh( poly_mesh );
		mesh->setLOD( info->mMinPixelArea );

		for (const LLAvatarXmlInfo::LLAvatarMeshInfo::morph_info_pair_t& info_pair : info->mPolyMorphTargetInfoList)
		{
			LLPolyMorphTarget *param = new LLPolyMorphTarget(mesh->getMesh());
			if (!param->setInfo((LLPolyMorphTargetInfo*)info_pair.first))
			{
				delete param;
				return false;
			}
			else
			{
				if (info_pair.second)
				{
					addSharedVisualParam(param);
					param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
				}
				else
				{
					addVisualParam(param);
					param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
				}
			}				
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// loadLayerSets()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::loadLayersets()
{
	bool success = true;
	for (LLTexLayerSetInfo* layerset_info : sAvatarXmlInfo->mLayerInfoList)
	{
		if (isSelf())
		{
			// Construct a layerset for each one specified in avatar_lad.xml and initialize it as such.
			LLTexLayerSet* layer_set = createTexLayerSet();
			
			if (!layer_set->setInfo(layerset_info))
			{
				stop_glerror();
				delete layer_set;
				LL_WARNS() << "avatar file: layer_set->setInfo() failed" << LL_ENDL;
				return false;
			}

			// scan baked textures and associate the layerset with the appropriate one
			EBakedTextureIndex baked_index = BAKED_NUM_INDICES;
			for (const LLAvatarAppearanceDictionary::BakedTextures::value_type& baked_pair : sAvatarDictionary->getBakedTextures())
			{
				const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = baked_pair.second;
				if (layer_set->isBodyRegion(baked_dict->mName))
				{
					baked_index = baked_pair.first;
					// ensure both structures are aware of each other
					mBakedTextureDatas[baked_index].mTexLayerSet = layer_set;
					layer_set->setBakedTexIndex(baked_index);
					break;
				}
			}
			// if no baked texture was found, warn and cleanup
			if (baked_index == BAKED_NUM_INDICES)
			{
				LL_WARNS() << "<layer_set> has invalid body_region attribute" << LL_ENDL;
				delete layer_set;
				return false;
			}

			// scan morph masks and let any affected layers know they have an associated morph
			for (LLMaskedMorph* morph : mBakedTextureDatas[baked_index].mMaskedMorphs)
			{
				LLTexLayerInterface* layer = layer_set->findLayerByName(morph->mLayer);
				if (layer)
				{
					layer->setHasMorph(true);
				}
				else
				{
					LL_WARNS() << "Could not find layer named " << morph->mLayer << " to set morph flag" << LL_ENDL;
					success = false;
				}
			}
		}
		else // !isSelf()
		{
			// Construct a layerset for each one specified in avatar_lad.xml and initialize it as such.
			layerset_info->createVisualParams(this);
		}
	}
	return success;
}

//-----------------------------------------------------------------------------
// getCharacterJoint()
//-----------------------------------------------------------------------------
LLJoint *LLAvatarAppearance::getCharacterJoint( U32 num )
{
	if ((S32)num >= mSkeleton.size()
	    || (S32)num < 0)
	{
		return NULL;
	}
    if (!mSkeleton[num])
    {
        mSkeleton[num] = createAvatarJoint();
    }
	return mSkeleton[num];
}


//-----------------------------------------------------------------------------
// getVolumePos()
//-----------------------------------------------------------------------------
LLVector3 LLAvatarAppearance::getVolumePos(S32 joint_index, LLVector3& volume_offset)
{
	if (joint_index > mNumCollisionVolumes)
	{
		return LLVector3::zero;
	}

	return mCollisionVolumes[joint_index].getVolumePos(volume_offset);
}

//-----------------------------------------------------------------------------
// findCollisionVolume()
//-----------------------------------------------------------------------------
LLJoint* LLAvatarAppearance::findCollisionVolume(S32 volume_id)
{
	if ((volume_id < 0) || (volume_id >= mNumCollisionVolumes))
	{
		return NULL;
	}
	
	return &mCollisionVolumes[volume_id];
}

//-----------------------------------------------------------------------------
// findCollisionVolume()
//-----------------------------------------------------------------------------
S32 LLAvatarAppearance::getCollisionVolumeID(std::string &name)
{
	for (S32 i = 0; i < mNumCollisionVolumes; i++)
	{
		if (mCollisionVolumes[i].getName() == name)
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// LLAvatarAppearance::getHeadMesh()
//-----------------------------------------------------------------------------
LLPolyMesh*	LLAvatarAppearance::getHeadMesh()
{
	return mMeshLOD[MESH_ID_HEAD]->mMeshParts[0]->getMesh();
}


//-----------------------------------------------------------------------------
// LLAvatarAppearance::getUpperBodyMesh()
//-----------------------------------------------------------------------------
LLPolyMesh*	LLAvatarAppearance::getUpperBodyMesh()
{
	return mMeshLOD[MESH_ID_UPPER_BODY]->mMeshParts[0]->getMesh();
}



// virtual
bool LLAvatarAppearance::isValid() const
{
	// This should only be called on ourself.
	if (!isSelf())
	{
		LL_ERRS() << "Called LLAvatarAppearance::isValid() on when isSelf() == false" << LL_ENDL;
	}
	return true;
}


// adds a morph mask to the appropriate baked texture structure
void LLAvatarAppearance::addMaskedMorph(EBakedTextureIndex index, LLVisualParam* morph_target, bool invert, std::string layer)
{
	if (index < BAKED_NUM_INDICES)
	{
		LLMaskedMorph *morph = new LLMaskedMorph(morph_target, invert, layer);
		mBakedTextureDatas[index].mMaskedMorphs.push_front(morph);
	}
}


//static
bool LLAvatarAppearance::teToColorParams( ETextureIndex te, U32 *param_name )
{
	switch( te )
	{
		case TEX_UPPER_SHIRT:
			param_name[0] = 803; //"shirt_red";
			param_name[1] = 804; //"shirt_green";
			param_name[2] = 805; //"shirt_blue";
			break;

		case TEX_LOWER_PANTS:
			param_name[0] = 806; //"pants_red";
			param_name[1] = 807; //"pants_green";
			param_name[2] = 808; //"pants_blue";
			break;

		case TEX_LOWER_SHOES:
			param_name[0] = 812; //"shoes_red";
			param_name[1] = 813; //"shoes_green";
			param_name[2] = 817; //"shoes_blue";
			break;

		case TEX_LOWER_SOCKS:
			param_name[0] = 818; //"socks_red";
			param_name[1] = 819; //"socks_green";
			param_name[2] = 820; //"socks_blue";
			break;

		case TEX_UPPER_JACKET:
		case TEX_LOWER_JACKET:
			param_name[0] = 834; //"jacket_red";
			param_name[1] = 835; //"jacket_green";
			param_name[2] = 836; //"jacket_blue";
			break;

		case TEX_UPPER_GLOVES:
			param_name[0] = 827; //"gloves_red";
			param_name[1] = 829; //"gloves_green";
			param_name[2] = 830; //"gloves_blue";
			break;

		case TEX_UPPER_UNDERSHIRT:
			param_name[0] = 821; //"undershirt_red";
			param_name[1] = 822; //"undershirt_green";
			param_name[2] = 823; //"undershirt_blue";
			break;
	
		case TEX_LOWER_UNDERPANTS:
			param_name[0] = 824; //"underpants_red";
			param_name[1] = 825; //"underpants_green";
			param_name[2] = 826; //"underpants_blue";
			break;

		case TEX_SKIRT:
			param_name[0] = 921; //"skirt_red";
			param_name[1] = 922; //"skirt_green";
			param_name[2] = 923; //"skirt_blue";
			break;

		case TEX_HEAD_TATTOO:
		case TEX_LOWER_TATTOO:
		case TEX_UPPER_TATTOO:
			param_name[0] = 1071; //"tattoo_red";
			param_name[1] = 1072; //"tattoo_green";
			param_name[2] = 1073; //"tattoo_blue";
			break;
		case TEX_HEAD_UNIVERSAL_TATTOO:
		case TEX_UPPER_UNIVERSAL_TATTOO:
		case TEX_LOWER_UNIVERSAL_TATTOO:
		case TEX_SKIRT_TATTOO:
		case TEX_HAIR_TATTOO:
		case TEX_EYES_TATTOO:
		case TEX_LEFT_ARM_TATTOO:
		case TEX_LEFT_LEG_TATTOO:
		case TEX_AUX1_TATTOO:
		case TEX_AUX2_TATTOO:
		case TEX_AUX3_TATTOO:
			param_name[0] = 1238; //"tattoo_universal_red";
			param_name[1] = 1239; //"tattoo_universal_green";
			param_name[2] = 1240; //"tattoo_universal_blue";
			break;	

		default:
			llassert(0);
			return false;
	}

	return true;
}

void LLAvatarAppearance::setClothesColor( ETextureIndex te, const LLColor4& new_color)
{
	U32 param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		setVisualParamWeight( param_name[0], new_color.mV[VX]);
		setVisualParamWeight( param_name[1], new_color.mV[VY]);
		setVisualParamWeight( param_name[2], new_color.mV[VZ]);
	}
}

LLColor4 LLAvatarAppearance::getClothesColor( ETextureIndex te )
{
	LLColor4 color;
	U32 param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		color.mV[VX] = getVisualParamWeight( param_name[0] );
		color.mV[VY] = getVisualParamWeight( param_name[1] );
		color.mV[VZ] = getVisualParamWeight( param_name[2] );
	}
	return color;
}

// static
LLColor4 LLAvatarAppearance::getDummyColor()
{
	return DUMMY_COLOR;
}

LLColor4 LLAvatarAppearance::getGlobalColor( const std::string& color_name ) const
{
	if (color_name=="skin_color" && mTexSkinColor)
	{
		return mTexSkinColor->getColor();
	}
	else if(color_name=="hair_color" && mTexHairColor)
	{
		return mTexHairColor->getColor();
	}
	if(color_name=="eye_color" && mTexEyeColor)
	{
		return mTexEyeColor->getColor();
	}
	else
	{
//		return LLColor4( .5f, .5f, .5f, .5f );
		return LLColor4( 0.f, 1.f, 1.f, 1.f ); // good debugging color
	}
}

// Unlike most wearable functions, this works for both self and other.
// virtual
bool LLAvatarAppearance::isWearingWearableType(LLWearableType::EType type) const
{
	return mWearableData->getWearableCount(type) > 0;
}

LLTexLayerSet* LLAvatarAppearance::getAvatarLayerSet(EBakedTextureIndex baked_index) const
{
	/* switch(index)
		case TEX_HEAD_BAKED:
		case TEX_HEAD_BODYPAINT:
			return mHeadLayerSet; */
	return mBakedTextureDatas[baked_index].mTexLayerSet;
}

//-----------------------------------------------------------------------------
// allocateCollisionVolumes()
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::allocateCollisionVolumes( U32 num )
{
    if (mNumCollisionVolumes !=num)
    {
        delete_and_clear_array(mCollisionVolumes);
        mNumCollisionVolumes = 0;

        mCollisionVolumes = new LLAvatarJointCollisionVolume[num];
        if (!mCollisionVolumes)
        {
            LL_WARNS() << "Failed to allocate collision volumes" << LL_ENDL;
            return false;
        }
        
        mNumCollisionVolumes = num;
    }
	return true;
}

//-----------------------------------------------------------------------------
// LLAvatarBoneInfo::parseXml()
//-----------------------------------------------------------------------------
bool LLAvatarBoneInfo::parseXml(LLXmlTreeNode* node)
{
	if (node->hasName("bone"))
	{
		mIsJoint = true;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			LL_WARNS() << "Bone without name" << LL_ENDL;
			return false;
		}
        
        static LLStdStringHandle aliases_string = LLXmlTree::addAttributeString("aliases");
        node->getFastAttributeString(aliases_string, mAliases ); //Aliases are not required.
	}
	else if (node->hasName("collision_volume"))
	{
		mIsJoint = false;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			mName = "Collision Volume";
		}
	}
	else
	{
		LL_WARNS() << "Invalid node " << node->getName() << LL_ENDL;
		return false;
	}

	static LLStdStringHandle pos_string = LLXmlTree::addAttributeString("pos");
	if (!node->getFastAttributeVector3(pos_string, mPos))
	{
		LL_WARNS() << "Bone without position" << LL_ENDL;
		return false;
	}

	static LLStdStringHandle rot_string = LLXmlTree::addAttributeString("rot");
	if (!node->getFastAttributeVector3(rot_string, mRot))
	{
		LL_WARNS() << "Bone without rotation" << LL_ENDL;
		return false;
	}
	
	static LLStdStringHandle scale_string = LLXmlTree::addAttributeString("scale");
	if (!node->getFastAttributeVector3(scale_string, mScale))
	{
		LL_WARNS() << "Bone without scale" << LL_ENDL;
		return false;
	}

	static LLStdStringHandle end_string = LLXmlTree::addAttributeString("end");
	if (!node->getFastAttributeVector3(end_string, mEnd))
	{
		LL_WARNS() << "Bone without end " << mName << LL_ENDL;
        mEnd = LLVector3(0.0f, 0.0f, 0.0f);
	}

	static LLStdStringHandle support_string = LLXmlTree::addAttributeString("support");
    if (!node->getFastAttributeString(support_string,mSupport))
    {
        LL_WARNS() << "Bone without support " << mName << LL_ENDL;
        mSupport = "base";
    }

	if (mIsJoint)
	{
		static LLStdStringHandle pivot_string = LLXmlTree::addAttributeString("pivot");
		if (!node->getFastAttributeVector3(pivot_string, mPivot))
		{
			LL_WARNS() << "Bone without pivot" << LL_ENDL;
			return false;
		}
	}

	// parse children
	LLXmlTreeNode* child;
	for( child = node->getFirstChild(); child; child = node->getNextChild() )
	{
		LLAvatarBoneInfo *child_info = new LLAvatarBoneInfo;
		if (!child_info->parseXml(child))
		{
			delete child_info;
			return false;
		}
		mChildren.push_back(child_info);
	}
	return true;
}

//-----------------------------------------------------------------------------
// LLAvatarSkeletonInfo::parseXml()
//-----------------------------------------------------------------------------
bool LLAvatarSkeletonInfo::parseXml(LLXmlTreeNode* node)
{
	static LLStdStringHandle num_bones_string = LLXmlTree::addAttributeString("num_bones");
	if (!node->getFastAttributeS32(num_bones_string, mNumBones))
	{
		LL_WARNS() << "Couldn't find number of bones." << LL_ENDL;
		return false;
	}

	static LLStdStringHandle num_collision_volumes_string = LLXmlTree::addAttributeString("num_collision_volumes");
	node->getFastAttributeS32(num_collision_volumes_string, mNumCollisionVolumes);

	LLXmlTreeNode* child;
	for( child = node->getFirstChild(); child; child = node->getNextChild() )
	{
		LLAvatarBoneInfo *info = new LLAvatarBoneInfo;
		if (!info->parseXml(child))
		{
			delete info;
			LL_WARNS() << "Error parsing bone in skeleton file" << LL_ENDL;
			return false;
		}
		mBoneInfoList.push_back(info);
	}
	return true;
}

//Make aliases for joint and push to map.
void LLAvatarAppearance::makeJointAliases(LLAvatarBoneInfo *bone_info)
{
    if (! bone_info->mIsJoint )
    {
        return;
    }
    
    std::string bone_name = bone_info->mName;
    mJointAliasMap[bone_name] = bone_name; //Actual name is a valid alias.
    
    std::string aliases = bone_info->mAliases;
    
    boost::char_separator<char> sep(" ");
    boost::tokenizer<boost::char_separator<char> > tok(aliases, sep);
    for(const std::string& i : tok)
    {
        if ( mJointAliasMap.find(i) != mJointAliasMap.end() )
        {
            LL_WARNS() << "avatar skeleton:  Joint alias \"" << i << "\" remapped from " << mJointAliasMap[i] << " to " << bone_name << LL_ENDL;
        }
        mJointAliasMap[i] = bone_name;
    }

    for (LLAvatarBoneInfo* bone : bone_info->mChildren)
    {
        makeJointAliases(bone);
    }
}

const LLAvatarAppearance::joint_alias_map_t& LLAvatarAppearance::getJointAliases ()
{
    LLAvatarAppearance::joint_alias_map_t alias_map;
    if (mJointAliasMap.empty())
    {
        
        for (LLAvatarBoneInfo* bone_info : sAvatarSkeletonInfo->mBoneInfoList)
        {
            //LLAvatarBoneInfo *bone_info = *iter;
            makeJointAliases(bone_info);
        }

        for (LLAvatarXmlInfo::LLAvatarAttachmentInfo* info : sAvatarXmlInfo->mAttachmentInfoList)
        {
            std::string bone_name = info->mName;
            
            // Also accept the name with spaces substituted with
            // underscores. This gives a mechanism for referencing such joints
            // in daes, which don't allow spaces.
            std::string sub_space_to_underscore = bone_name;
            LLStringUtil::replaceChar(sub_space_to_underscore, ' ', '_');
            if (sub_space_to_underscore != bone_name)
            {
                mJointAliasMap[sub_space_to_underscore] = bone_name;
            }
        }
    }

    return mJointAliasMap;
} 


//-----------------------------------------------------------------------------
// parseXmlSkeletonNode(): parses <skeleton> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlSkeletonNode(LLXmlTreeNode* root)
{
	LLXmlTreeNode* node = root->getChildByName( "skeleton" );
	if( !node )
	{
		LL_WARNS() << "avatar file: missing <skeleton>" << LL_ENDL;
		return false;
	}

	LLXmlTreeNode* child;

	// SKELETON DISTORTIONS
	for (child = node->getChildByName( "param" );
		 child;
		 child = node->getNextNamedChild())
	{
		if (!child->getChildByName("param_skeleton"))
		{
			if (child->getChildByName("param_morph"))
			{
				LL_WARNS() << "Can't specify morph param in skeleton definition." << LL_ENDL;
			}
			else
			{
				LL_WARNS() << "Unknown param type." << LL_ENDL;
			}
            return false;
		}
		
		LLPolySkeletalDistortionInfo *info = new LLPolySkeletalDistortionInfo;
		if (!info->parseXml(child))
		{
			delete info;
			return false;
		}

		mSkeletalDistortionInfoList.push_back(info);
	}

	// ATTACHMENT POINTS
	for (child = node->getChildByName( "attachment_point" );
		 child;
		 child = node->getNextNamedChild())
	{
		LLAvatarAttachmentInfo* info = new LLAvatarAttachmentInfo();

		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!child->getFastAttributeString(name_string, info->mName))
		{
			LL_WARNS() << "No name supplied for attachment point." << LL_ENDL;
			delete info;
            return false;
		}

		static LLStdStringHandle joint_string = LLXmlTree::addAttributeString("joint");
		if (!child->getFastAttributeString(joint_string, info->mJointName))
		{
			LL_WARNS() << "No bone declared in attachment point " << info->mName << LL_ENDL;
			delete info;
            return false;
		}

		static LLStdStringHandle position_string = LLXmlTree::addAttributeString("position");
		if (child->getFastAttributeVector3(position_string, info->mPosition))
		{
			info->mHasPosition = true;
		}

		static LLStdStringHandle rotation_string = LLXmlTree::addAttributeString("rotation");
		if (child->getFastAttributeVector3(rotation_string, info->mRotationEuler))
		{
			info->mHasRotation = true;
		}
		 static LLStdStringHandle group_string = LLXmlTree::addAttributeString("group");
		if (child->getFastAttributeS32(group_string, info->mGroup))
		{
			if (info->mGroup == -1)
				info->mGroup = -1111; // -1 = none parsed, < -1 = bad value
		}

		static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
		if (!child->getFastAttributeS32(id_string, info->mAttachmentID))
		{
			LL_WARNS() << "No id supplied for attachment point " << info->mName << LL_ENDL;
			delete info;
            return false;
		}

		static LLStdStringHandle slot_string = LLXmlTree::addAttributeString("pie_slice");
		child->getFastAttributeS32(slot_string, info->mPieMenuSlice);
			
		static LLStdStringHandle visible_in_first_person_string = LLXmlTree::addAttributeString("visible_in_first_person");
		child->getFastAttributeBOOL(visible_in_first_person_string, info->mVisibleFirstPerson);

		static LLStdStringHandle hud_attachment_string = LLXmlTree::addAttributeString("hud");
		child->getFastAttributeBOOL(hud_attachment_string, info->mIsHUDAttachment);

		mAttachmentInfoList.push_back(info);
	}

	return true;
}

//-----------------------------------------------------------------------------
// parseXmlMeshNodes(): parses <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlMeshNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* node = root->getChildByName( "mesh" );
		 node;
		 node = root->getNextNamedChild())
	{
		LLAvatarMeshInfo *info = new LLAvatarMeshInfo;

		// attribute: type
		static LLStdStringHandle type_string = LLXmlTree::addAttributeString("type");
		if( !node->getFastAttributeString( type_string, info->mType ) )
		{
			LL_WARNS() << "Avatar file: <mesh> is missing type attribute.  Ignoring element. " << LL_ENDL;
			delete info;
			return false;  // Ignore this element
		}
		
		static LLStdStringHandle lod_string = LLXmlTree::addAttributeString("lod");
		if (!node->getFastAttributeS32( lod_string, info->mLOD ))
		{
			LL_WARNS() << "Avatar file: <mesh> is missing lod attribute.  Ignoring element. " << LL_ENDL;
			delete info;
			return false;  // Ignore this element
		}

		static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
		if( !node->getFastAttributeString( file_name_string, info->mMeshFileName ) )
		{
			LL_WARNS() << "Avatar file: <mesh> is missing file_name attribute.  Ignoring: " << info->mType << LL_ENDL;
			delete info;
			return false;  // Ignore this element
		}

		static LLStdStringHandle reference_string = LLXmlTree::addAttributeString("reference");
		node->getFastAttributeString( reference_string, info->mReferenceMeshName );
		
		// attribute: min_pixel_area
		static LLStdStringHandle min_pixel_area_string = LLXmlTree::addAttributeString("min_pixel_area");
		static LLStdStringHandle min_pixel_width_string = LLXmlTree::addAttributeString("min_pixel_width");
		if (!node->getFastAttributeF32( min_pixel_area_string, info->mMinPixelArea ))
		{
			F32 min_pixel_area = 0.1f;
			if (node->getFastAttributeF32( min_pixel_width_string, min_pixel_area ))
			{
				// this is square root of pixel area (sensible to use linear space in defining lods)
				min_pixel_area = min_pixel_area * min_pixel_area;
			}
			info->mMinPixelArea = min_pixel_area;
		}
		
		// Parse visual params for this node only if we haven't already
		for (LLXmlTreeNode* child = node->getChildByName( "param" );
			 child;
			 child = node->getNextNamedChild())
		{
			if (!child->getChildByName("param_morph"))
			{
				if (child->getChildByName("param_skeleton"))
				{
					LL_WARNS() << "Can't specify skeleton param in a mesh definition." << LL_ENDL;
				}
				else
				{
					LL_WARNS() << "Unknown param type." << LL_ENDL;
				}
                return false;
			}

			LLPolyMorphTargetInfo *morphinfo = new LLPolyMorphTargetInfo();
			if (!morphinfo->parseXml(child))
			{
				delete morphinfo;
				delete info;
				return false;
			}
			bool shared = false;
			static LLStdStringHandle shared_string = LLXmlTree::addAttributeString("shared");
			child->getFastAttributeBOOL(shared_string, shared);

			info->mPolyMorphTargetInfoList.push_back(LLAvatarMeshInfo::morph_info_pair_t(morphinfo, shared));
		}

		mMeshInfoList.push_back(info);
	}
	return true;
}

//-----------------------------------------------------------------------------
// parseXmlColorNodes(): parses <global_color> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlColorNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* color_node = root->getChildByName( "global_color" );
		 color_node;
		 color_node = root->getNextNamedChild())
	{
		std::string global_color_name;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (color_node->getFastAttributeString( name_string, global_color_name ) )
		{
			if( global_color_name == "skin_color" )
			{
				if (mTexSkinColorInfo)
				{
					LL_WARNS() << "avatar file: multiple instances of skin_color" << LL_ENDL;
					return false;
				}
				mTexSkinColorInfo = new LLTexGlobalColorInfo;
				if( !mTexSkinColorInfo->parseXml( color_node ) )
				{
					delete_and_clear(mTexSkinColorInfo);
					LL_WARNS() << "avatar file: mTexSkinColor->parseXml() failed" << LL_ENDL;
					return false;
				}
			}
			else if( global_color_name == "hair_color" )
			{
				if (mTexHairColorInfo)
				{
					LL_WARNS() << "avatar file: multiple instances of hair_color" << LL_ENDL;
					return false;
				}
				mTexHairColorInfo = new LLTexGlobalColorInfo;
				if( !mTexHairColorInfo->parseXml( color_node ) )
				{
					delete_and_clear(mTexHairColorInfo);
					LL_WARNS() << "avatar file: mTexHairColor->parseXml() failed" << LL_ENDL;
					return false;
				}
			}
			else if( global_color_name == "eye_color" )
			{
				if (mTexEyeColorInfo)
				{
					LL_WARNS() << "avatar file: multiple instances of eye_color" << LL_ENDL;
					return false;
				}
				mTexEyeColorInfo = new LLTexGlobalColorInfo;
				if( !mTexEyeColorInfo->parseXml( color_node ) )
				{
					LL_WARNS() << "avatar file: mTexEyeColor->parseXml() failed" << LL_ENDL;
					return false;
				}
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// parseXmlLayerNodes(): parses <layer_set> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlLayerNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* layer_node = root->getChildByName( "layer_set" );
		 layer_node;
		 layer_node = root->getNextNamedChild())
	{
		LLTexLayerSetInfo* layer_info = new LLTexLayerSetInfo();
		if( layer_info->parseXml( layer_node ) )
		{
			mLayerInfoList.push_back(layer_info);
		}
		else
		{
			delete layer_info;
			LL_WARNS() << "avatar file: layer_set->parseXml() failed" << LL_ENDL;
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// parseXmlDriverNodes(): parses <driver_parameters> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlDriverNodes(LLXmlTreeNode* root)
{
	LLXmlTreeNode* driver = root->getChildByName( "driver_parameters" );
	if( driver )
	{
		for (LLXmlTreeNode* grand_child = driver->getChildByName( "param" );
			 grand_child;
			 grand_child = driver->getNextNamedChild())
		{
			if( grand_child->getChildByName( "param_driver" ) )
			{
				LLDriverParamInfo* driver_info = new LLDriverParamInfo();
				if( driver_info->parseXml( grand_child ) )
				{
					mDriverInfoList.push_back(driver_info);
				}
				else
				{
					delete driver_info;
					LL_WARNS() << "avatar file: driver_param->parseXml() failed" << LL_ENDL;
					return false;
				}
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// parseXmlDriverNodes(): parses <driver_parameters> nodes from XML tree
//-----------------------------------------------------------------------------
bool LLAvatarAppearance::LLAvatarXmlInfo::parseXmlMorphNodes(LLXmlTreeNode* root)
{
	LLXmlTreeNode* masks = root->getChildByName( "morph_masks" );
	if( !masks )
	{
		return false;
	}

	for (LLXmlTreeNode* grand_child = masks->getChildByName( "mask" );
		 grand_child;
		 grand_child = masks->getNextNamedChild())
	{
		LLAvatarMorphInfo* info = new LLAvatarMorphInfo();

		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("morph_name");
		if (!grand_child->getFastAttributeString(name_string, info->mName))
		{
			LL_WARNS() << "No name supplied for morph mask." << LL_ENDL;
			delete info;
            return false;
		}

		static LLStdStringHandle region_string = LLXmlTree::addAttributeString("body_region");
		if (!grand_child->getFastAttributeString(region_string, info->mRegion))
		{
			LL_WARNS() << "No region supplied for morph mask." << LL_ENDL;
			delete info;
            return false;
		}

		static LLStdStringHandle layer_string = LLXmlTree::addAttributeString("layer");
		if (!grand_child->getFastAttributeString(layer_string, info->mLayer))
		{
			LL_WARNS() << "No layer supplied for morph mask." << LL_ENDL;
			delete info;
            return false;
		}

		// optional parameter. don't throw a warning if not present.
		static LLStdStringHandle invert_string = LLXmlTree::addAttributeString("invert");
		grand_child->getFastAttributeBOOL(invert_string, info->mInvert);

		mMorphMaskInfoList.push_back(info);
	}

	return true;
}

//virtual 
LLAvatarAppearance::LLMaskedMorph::LLMaskedMorph(LLVisualParam *morph_target, bool invert, std::string layer) :
			mMorphTarget(morph_target),
			mInvert(invert),
			mLayer(layer)
{
	LLPolyMorphTarget *target = dynamic_cast<LLPolyMorphTarget*>(morph_target);
	if (target)
	{
		target->addPendingMorphMask();
	}
}
