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
#include "imageids.h"
#include "lldir.h"
#include "lldeleteutils.h"
#include "llpolymorph.h"
#include "llpolymesh.h"
#include "llpolyskeletaldistortion.h"
#include "llstl.h"
#include "lltexglobalcolor.h"
#include "llwearabledata.h"


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
	LLAvatarBoneInfo() : mIsJoint(FALSE) {}
	~LLAvatarBoneInfo()
	{
		std::for_each(mChildList.begin(), mChildList.end(), DeletePointer());
	}
	BOOL parseXml(LLXmlTreeNode* node);
	
private:
	std::string mName;
	BOOL mIsJoint;
	LLVector3 mPos;
	LLVector3 mRot;
	LLVector3 mScale;
	LLVector3 mPivot;
	typedef std::vector<LLAvatarBoneInfo*> child_list_t;
	child_list_t mChildList;
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
	}
	BOOL parseXml(LLXmlTreeNode* node);
	S32 getNumBones() const { return mNumBones; }
	S32 getNumCollisionVolumes() const { return mNumCollisionVolumes; }
	
private:
	S32 mNumBones;
	S32 mNumCollisionVolumes;
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
	std::for_each(mSkeletalDistortionInfoList.begin(), mSkeletalDistortionInfoList.end(), DeletePointer());		
	std::for_each(mAttachmentInfoList.begin(), mAttachmentInfoList.end(), DeletePointer());
	deleteAndClear(mTexSkinColorInfo);
	deleteAndClear(mTexHairColorInfo);
	deleteAndClear(mTexEyeColorInfo);
	std::for_each(mLayerInfoList.begin(), mLayerInfoList.end(), DeletePointer());		
	std::for_each(mDriverInfoList.begin(), mDriverInfoList.end(), DeletePointer());
	std::for_each(mMorphMaskInfoList.begin(), mMorphMaskInfoList.end(), DeletePointer());
}


/**
 **
 ** End LLAvatarAppearance Support classes
 **                                                                             **
 *********************************************************************************/

//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
LLXmlTree LLAvatarAppearance::sXMLTree;
LLXmlTree LLAvatarAppearance::sSkeletonXMLTree;
LLAvatarSkeletonInfo* LLAvatarAppearance::sAvatarSkeletonInfo = NULL;
LLAvatarAppearance::LLAvatarXmlInfo* LLAvatarAppearance::sAvatarXmlInfo = NULL;


LLAvatarAppearance::LLAvatarAppearance(LLWearableData* wearable_data) :
	LLCharacter(),
	mIsDummy(FALSE),
	mTexSkinColor( NULL ),
	mTexHairColor( NULL ),
	mTexEyeColor( NULL ),
	mPelvisToFoot(0.f),
	mHeadOffset(),
	mRoot(NULL),
	mWearableData(wearable_data)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);

	llassert_always(mWearableData);
	mBakedTextureDatas.resize(LLAvatarAppearanceDefines::BAKED_NUM_INDICES);
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++ )
	{
		mBakedTextureDatas[i].mLastTextureID = IMG_DEFAULT_AVATAR;
		mBakedTextureDatas[i].mTexLayerSet = NULL;
		mBakedTextureDatas[i].mIsLoaded = false;
		mBakedTextureDatas[i].mIsUsed = false;
		mBakedTextureDatas[i].mMaskTexName = 0;
		mBakedTextureDatas[i].mTextureIndex = LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::bakedToLocalTextureIndex((LLAvatarAppearanceDefines::EBakedTextureIndex)i);
	}

	mIsBuilt = FALSE;

	mNumCollisionVolumes = 0;
	mCollisionVolumes = NULL;
}

// virtual
void LLAvatarAppearance::initInstance()
{
	//-------------------------------------------------------------------------
	// initialize joint, mesh and shape members
	//-------------------------------------------------------------------------
	mRoot = createAvatarJoint();
	mRoot->setName( "mRoot" );

	for (LLAvatarAppearanceDictionary::MeshEntries::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().end();
		 ++iter)
	{
		const EMeshIndex mesh_index = iter->first;
		const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = iter->second;
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
			mesh->setIsTransparent(FALSE);
			switch((int)mesh_index)
			{
				case MESH_ID_HAIR:
					mesh->setIsTransparent(TRUE);
					break;
				case MESH_ID_SKIRT:
					mesh->setIsTransparent(TRUE);
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
	for (LLAvatarAppearanceDictionary::MeshEntries::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().end();
		 ++iter)
	{
		const EMeshIndex mesh_index = iter->first;
		const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = iter->second;
		const EBakedTextureIndex baked_texture_index = mesh_dict->mBakedID;
		// Skip it if there's no associated baked texture.
		if (baked_texture_index == BAKED_NUM_INDICES) continue;
		
		for (avatar_joint_mesh_list_t::iterator iter = mMeshLOD[mesh_index]->mMeshParts.begin();
			 iter != mMeshLOD[mesh_index]->mMeshParts.end(); 
			 ++iter)
		{
			LLAvatarJointMesh* mesh = (*iter);
			mBakedTextureDatas[(int)baked_texture_index].mJointMeshes.push_back(mesh);
		}
	}

	buildCharacter();

}

// virtual
LLAvatarAppearance::~LLAvatarAppearance()
{
	deleteAndClear(mTexSkinColor);
	deleteAndClear(mTexHairColor);
	deleteAndClear(mTexEyeColor);

	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		deleteAndClear(mBakedTextureDatas[i].mTexLayerSet);
		mBakedTextureDatas[i].mJointMeshes.clear();

		for (morph_list_t::iterator iter2 = mBakedTextureDatas[i].mMaskedMorphs.begin();
			 iter2 != mBakedTextureDatas[i].mMaskedMorphs.end(); iter2++)
		{
			LLMaskedMorph* masked_morph = (*iter2);
			delete masked_morph;
		}
	}

	if (mRoot) mRoot->removeAllChildren();
	mJointMap.clear();

	clearSkeleton();
	deleteAndClearArray(mCollisionVolumes);

	deleteAndClear(mTexSkinColor);
	deleteAndClear(mTexHairColor);
	deleteAndClear(mTexEyeColor);

	std::for_each(mPolyMeshes.begin(), mPolyMeshes.end(), DeletePairedPointer());
	mPolyMeshes.clear();

	for (avatar_joint_list_t::iterator jointIter = mMeshLOD.begin();
		 jointIter != mMeshLOD.end(); 
		 ++jointIter)
	{
		LLAvatarJoint* joint = *jointIter;
		std::for_each(joint->mMeshParts.begin(), joint->mMeshParts.end(), DeletePointer());
		joint->mMeshParts.clear();
	}
	std::for_each(mMeshLOD.begin(), mMeshLOD.end(), DeletePointer());
	mMeshLOD.clear();
}

//static
void LLAvatarAppearance::initClass()
{
	std::string xmlFile;

	xmlFile = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,AVATAR_DEFAULT_CHAR) + "_lad.xml";
	BOOL success = sXMLTree.parseFile( xmlFile, FALSE );
	if (!success)
	{
		llerrs << "Problem reading avatar configuration file:" << xmlFile << llendl;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = sXMLTree.getRoot();
	if (!root) 
	{
		llerrs << "No root node found in avatar configuration file: " << xmlFile << llendl;
		return;
	}

	//-------------------------------------------------------------------------
	// <linden_avatar version="1.0"> (root)
	//-------------------------------------------------------------------------
	if( !root->hasName( "linden_avatar" ) )
	{
		llerrs << "Invalid avatar file header: " << xmlFile << llendl;
	}
	
	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || (version != "1.0") )
	{
		llerrs << "Invalid avatar file version: " << version << " in file: " << xmlFile << llendl;
	}

	S32 wearable_def_version = 1;
	static LLStdStringHandle wearable_definition_version_string = LLXmlTree::addAttributeString("wearable_definition_version");
	root->getFastAttributeS32( wearable_definition_version_string, wearable_def_version );
	LLWearable::setCurrentDefinitionVersion( wearable_def_version );

	std::string mesh_file_name;

	LLXmlTreeNode* skeleton_node = root->getChildByName( "skeleton" );
	if (!skeleton_node)
	{
		llerrs << "No skeleton in avatar configuration file: " << xmlFile << llendl;
		return;
	}
	
	std::string skeleton_file_name;
	static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
	if (!skeleton_node->getFastAttributeString(file_name_string, skeleton_file_name))
	{
		llerrs << "No file name in skeleton node in avatar config file: " << xmlFile << llendl;
	}
	
	std::string skeleton_path;
	skeleton_path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,skeleton_file_name);
	if (!parseSkeletonFile(skeleton_path))
	{
		llerrs << "Error parsing skeleton file: " << skeleton_path << llendl;
	}

	// Process XML data

	// avatar_skeleton.xml
	if (sAvatarSkeletonInfo)
	{ //this can happen if a login attempt failed
		delete sAvatarSkeletonInfo;
	}
	sAvatarSkeletonInfo = new LLAvatarSkeletonInfo;
	if (!sAvatarSkeletonInfo->parseXml(sSkeletonXMLTree.getRoot()))
	{
		llerrs << "Error parsing skeleton XML file: " << skeleton_path << llendl;
	}
	// parse avatar_lad.xml
	if (sAvatarXmlInfo)
	{ //this can happen if a login attempt failed
		deleteAndClear(sAvatarXmlInfo);
	}
	sAvatarXmlInfo = new LLAvatarXmlInfo;
	if (!sAvatarXmlInfo->parseXmlSkeletonNode(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlMeshNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlColorNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlLayerNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlDriverNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlMorphNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
}

void LLAvatarAppearance::cleanupClass()
{
	deleteAndClear(sAvatarXmlInfo);
	// *TODO: What about sAvatarSkeletonInfo ???
	sSkeletonXMLTree.cleanup();
	sXMLTree.cleanup();
}

using namespace LLAvatarAppearanceDefines;

//------------------------------------------------------------------------
// The viewer can only suggest a good size for the agent,
// the simulator will keep it inside a reasonable range.
void LLAvatarAppearance::computeBodySize() 
{
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

	if (new_body_size != mBodySize)
	{
		mBodySize = new_body_size;
		bodySizeChanged();
	}
}

//-----------------------------------------------------------------------------
// parseSkeletonFile()
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::parseSkeletonFile(const std::string& filename)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//-------------------------------------------------------------------------
	// parse the file
	//-------------------------------------------------------------------------
	BOOL parsesuccess = sSkeletonXMLTree.parseFile( filename, FALSE );

	if (!parsesuccess)
	{
		llerrs << "Can't parse skeleton file: " << filename << llendl;
		return FALSE;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = sSkeletonXMLTree.getRoot();
	if (!root) 
	{
		llerrs << "No root node found in avatar skeleton file: " << filename << llendl;
		return FALSE;
	}

	if( !root->hasName( "linden_skeleton" ) )
	{
		llerrs << "Invalid avatar skeleton file header: " << filename << llendl;
		return FALSE;
	}

	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || (version != "1.0") )
	{
		llerrs << "Invalid avatar skeleton file version: " << version << " in file: " << filename << llendl;
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// setupBone()
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::setupBone(const LLAvatarBoneInfo* info, LLJoint* parent, S32 &volume_num, S32 &joint_num)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	LLJoint* joint = NULL;

	if (info->mIsJoint)
	{
		joint = getCharacterJoint(joint_num);
		if (!joint)
		{
			llwarns << "Too many bones" << llendl;
			return FALSE;
		}
		joint->setName( info->mName );
	}
	else // collision volume
	{
		if (volume_num >= (S32)mNumCollisionVolumes)
		{
			llwarns << "Too many bones" << llendl;
			return FALSE;
		}
		joint = (&mCollisionVolumes[volume_num]);
		joint->setName( info->mName );
	}

	// add to parent
	if (parent)
	{
		parent->addChild( joint );
	}

	joint->setPosition(info->mPos);
	joint->setRotation(mayaQ(info->mRot.mV[VX], info->mRot.mV[VY],
							 info->mRot.mV[VZ], LLQuaternion::XYZ));
	joint->setScale(info->mScale);

	joint->setDefaultFromCurrentXform();
	
	if (info->mIsJoint)
	{
		joint->setSkinOffset( info->mPivot );
		joint_num++;
	}
	else // collision volume
	{
		volume_num++;
	}

	// setup children
	LLAvatarBoneInfo::child_list_t::const_iterator iter;
	for (iter = info->mChildList.begin(); iter != info->mChildList.end(); ++iter)
	{
		LLAvatarBoneInfo *child_info = *iter;
		if (!setupBone(child_info, joint, volume_num, joint_num))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// allocateCharacterJoints()
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::allocateCharacterJoints( U32 num )
{
	clearSkeleton();

	for(S32 joint_num = 0; joint_num < (S32)num; joint_num++)
	{
		mSkeleton.push_back(createAvatarJoint(joint_num));
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// buildSkeleton()
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::buildSkeleton(const LLAvatarSkeletonInfo *info)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//-------------------------------------------------------------------------
	// allocate joints
	//-------------------------------------------------------------------------
	if (!allocateCharacterJoints(info->mNumBones))
	{
		llerrs << "Can't allocate " << info->mNumBones << " joints" << llendl;
		return FALSE;
	}
	
	//-------------------------------------------------------------------------
	// allocate volumes
	//-------------------------------------------------------------------------
	if (info->mNumCollisionVolumes)
	{
		if (!allocateCollisionVolumes(info->mNumCollisionVolumes))
		{
			llerrs << "Can't allocate " << info->mNumCollisionVolumes << " collision volumes" << llendl;
			return FALSE;
		}
	}

	S32 current_joint_num = 0;
	S32 current_volume_num = 0;
	LLAvatarSkeletonInfo::bone_info_list_t::const_iterator iter;
	for (iter = info->mBoneInfoList.begin(); iter != info->mBoneInfoList.end(); ++iter)
	{
		LLAvatarBoneInfo *info = *iter;
		if (!setupBone(info, NULL, current_volume_num, current_joint_num))
		{
			llerrs << "Error parsing bone in skeleton file" << llendl;
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// clearSkeleton()
//-----------------------------------------------------------------------------
void LLAvatarAppearance::clearSkeleton()
{
	std::for_each(mSkeleton.begin(), mSkeleton.end(), DeletePointer());
	mSkeleton.clear();
}

//-----------------------------------------------------------------------------
// LLAvatarAppearance::buildCharacter()
// Deferred initialization and rebuild of the avatar.
//-----------------------------------------------------------------------------
void LLAvatarAppearance::buildCharacter()
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
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
	mIsBuilt = FALSE;

	//-------------------------------------------------------------------------
	// clear mesh data
	//-------------------------------------------------------------------------
	for (avatar_joint_list_t::iterator jointIter = mMeshLOD.begin();
		 jointIter != mMeshLOD.end(); ++jointIter)
	{
		LLAvatarJoint* joint = *jointIter;
		for (avatar_joint_mesh_list_t::iterator meshIter = joint->mMeshParts.begin();
			 meshIter != joint->mMeshParts.end(); ++meshIter)
		{
			LLAvatarJointMesh * mesh = *meshIter;
			mesh->setMesh(NULL);
		}
	}

	//-------------------------------------------------------------------------
	// (re)load our skeleton and meshes
	//-------------------------------------------------------------------------
	LLTimer timer;

	BOOL status = loadAvatar();
	stop_glerror();

// 	gPrintMessagesThisFrame = TRUE;
	lldebugs << "Avatar load took " << timer.getElapsedTimeF32() << " seconds." << llendl;

	if (!status)
	{
		if (isSelf())
		{
			llerrs << "Unable to load user's avatar" << llendl;
		}
		else
		{
			llwarns << "Unable to load other's avatar" << llendl;
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
		llerrs << "Failed to create avatar." << llendl;
		return;
	}

	//-------------------------------------------------------------------------
	// initialize the pelvis
	//-------------------------------------------------------------------------
	mPelvisp->setPosition( LLVector3(0.0f, 0.0f, 0.0f) );

	mIsBuilt = TRUE;
	stop_glerror();

}

BOOL LLAvatarAppearance::loadAvatar()
{
// 	LLFastTimer t(FTM_LOAD_AVATAR);
	
	// avatar_skeleton.xml
	if( !buildSkeleton(sAvatarSkeletonInfo) )
	{
		llwarns << "avatar file: buildSkeleton() failed" << llendl;
		return FALSE;
	}

	// avatar_lad.xml : <skeleton>
	if( !loadSkeletonNode() )
	{
		llwarns << "avatar file: loadNodeSkeleton() failed" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <mesh>
	if( !loadMeshNodes() )
	{
		llwarns << "avatar file: loadNodeMesh() failed" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <global_color>
	if( sAvatarXmlInfo->mTexSkinColorInfo )
	{
		mTexSkinColor = new LLTexGlobalColor( this );
		if( !mTexSkinColor->setInfo( sAvatarXmlInfo->mTexSkinColorInfo ) )
		{
			llwarns << "avatar file: mTexSkinColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"skin_color\" not found" << llendl;
		return FALSE;
	}
	if( sAvatarXmlInfo->mTexHairColorInfo )
	{
		mTexHairColor = new LLTexGlobalColor( this );
		if( !mTexHairColor->setInfo( sAvatarXmlInfo->mTexHairColorInfo ) )
		{
			llwarns << "avatar file: mTexHairColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"hair_color\" not found" << llendl;
		return FALSE;
	}
	if( sAvatarXmlInfo->mTexEyeColorInfo )
	{
		mTexEyeColor = new LLTexGlobalColor( this );
		if( !mTexEyeColor->setInfo( sAvatarXmlInfo->mTexEyeColorInfo ) )
		{
			llwarns << "avatar file: mTexEyeColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"eye_color\" not found" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <layer_set>
	if (sAvatarXmlInfo->mLayerInfoList.empty())
	{
		llwarns << "avatar file: missing <layer_set> node" << llendl;
		return FALSE;
	}

	if (sAvatarXmlInfo->mMorphMaskInfoList.empty())
	{
		llwarns << "avatar file: missing <morph_masks> node" << llendl;
		return FALSE;
	}

	// avatar_lad.xml : <morph_masks>
	for (LLAvatarXmlInfo::morph_info_list_t::iterator iter = sAvatarXmlInfo->mMorphMaskInfoList.begin();
		 iter != sAvatarXmlInfo->mMorphMaskInfoList.end();
		 ++iter)
	{
		LLAvatarXmlInfo::LLAvatarMorphInfo *info = *iter;

		EBakedTextureIndex baked = LLAvatarAppearanceDictionary::findBakedByRegionName(info->mRegion); 
		if (baked != BAKED_NUM_INDICES)
		{
			LLVisualParam* morph_param;
			const std::string *name = &info->mName;
			morph_param = getVisualParam(name->c_str());
			if (morph_param)
			{
				BOOL invert = info->mInvert;
				addMaskedMorph(baked, morph_param, invert, info->mLayer);
			}
		}

	}

	loadLayersets();
	
	// avatar_lad.xml : <driver_parameters>
	for (LLAvatarXmlInfo::driver_info_list_t::iterator iter = sAvatarXmlInfo->mDriverInfoList.begin();
		 iter != sAvatarXmlInfo->mDriverInfoList.end(); 
		 ++iter)
	{
		LLDriverParamInfo *info = *iter;
		LLDriverParam* driver_param = new LLDriverParam( this );
		if (driver_param->setInfo(info))
		{
			addVisualParam( driver_param );
			driver_param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
			LLVisualParam*(LLAvatarAppearance::*avatar_function)(S32)const = &LLAvatarAppearance::getVisualParam; 
			if( !driver_param->linkDrivenParams(boost::bind(avatar_function,(LLAvatarAppearance*)this,_1 ), false))
			{
				llwarns << "could not link driven params for avatar " << getID().asString() << " param id: " << driver_param->getID() << llendl;
				continue;
			}
		}
		else
		{
			delete driver_param;
			llwarns << "avatar file: driver_param->parseData() failed" << llendl;
			return FALSE;
		}
	}

	
	return TRUE;
}

//-----------------------------------------------------------------------------
// loadSkeletonNode(): loads <skeleton> node from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::loadSkeletonNode ()
{
	mRoot->addChild( mSkeleton[0] );

	// make meshes children before calling parent version of the function
	for (avatar_joint_list_t::iterator iter = mMeshLOD.begin();
		 iter != mMeshLOD.end(); 
		 ++iter)
	{
		LLAvatarJoint *joint = *iter;
		joint->mUpdateXform = FALSE;
		joint->setMeshesToChildren();
	}

	mRoot->addChild(mMeshLOD[MESH_ID_HEAD]);
	mRoot->addChild(mMeshLOD[MESH_ID_EYELASH]);
	mRoot->addChild(mMeshLOD[MESH_ID_UPPER_BODY]);
	mRoot->addChild(mMeshLOD[MESH_ID_LOWER_BODY]);
	mRoot->addChild(mMeshLOD[MESH_ID_SKIRT]);
	mRoot->addChild(mMeshLOD[MESH_ID_HEAD]);

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
		LLAvatarXmlInfo::skeletal_distortion_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mSkeletalDistortionInfoList.begin();
			 iter != sAvatarXmlInfo->mSkeletalDistortionInfoList.end(); 
			 ++iter)
		{
			LLPolySkeletalDistortionInfo *info = (LLPolySkeletalDistortionInfo*)*iter;
			LLPolySkeletalDistortion *param = new LLPolySkeletalDistortion(this);
			if (!param->setInfo(info))
			{
				delete param;
				return FALSE;
			}
			else
			{
				addVisualParam(param);
				param->setParamLocation(isSelf() ? LOC_AV_SELF : LOC_AV_OTHER);
			}				
		}
	}


	return TRUE;
}

//-----------------------------------------------------------------------------
// loadMeshNodes(): loads <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::loadMeshNodes()
{
	for (LLAvatarXmlInfo::mesh_info_list_t::const_iterator meshinfo_iter = sAvatarXmlInfo->mMeshInfoList.begin();
		 meshinfo_iter != sAvatarXmlInfo->mMeshInfoList.end(); 
		 ++meshinfo_iter)
	{
		const LLAvatarXmlInfo::LLAvatarMeshInfo *info = *meshinfo_iter;
		const std::string &type = info->mType;
		S32 lod = info->mLOD;

		LLAvatarJointMesh* mesh = NULL;
		U8 mesh_id = 0;
		BOOL found_mesh_id = FALSE;

		/* if (type == "hairMesh")
			switch(lod)
			  case 0:
				mesh = &mHairMesh0; */
		for (LLAvatarAppearanceDictionary::MeshEntries::const_iterator mesh_iter = LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().begin();
			 mesh_iter != LLAvatarAppearanceDictionary::getInstance()->getMeshEntries().end();
			 ++mesh_iter)
		{
			const EMeshIndex mesh_index = mesh_iter->first;
			const LLAvatarAppearanceDictionary::MeshEntry *mesh_dict = mesh_iter->second;
			if (type.compare(mesh_dict->mName) == 0)
			{
				mesh_id = mesh_index;
				found_mesh_id = TRUE;
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
				llwarns << "Avatar file: <mesh> has invalid lod setting " << lod << llendl;
				return FALSE;
			}
		}
		else 
		{
			llwarns << "Ignoring unrecognized mesh type: " << type << llendl;
			return FALSE;
		}

		//	llinfos << "Parsing mesh data for " << type << "..." << llendl;

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
			}
		}
		else
		{
			poly_mesh = LLPolyMesh::getMesh(info->mMeshFileName);
			poly_mesh->setAvatar(this);
		}

		if( !poly_mesh )
		{
			llwarns << "Failed to load mesh of type " << type << llendl;
			return FALSE;
		}

		// Multimap insert
		mPolyMeshes.insert(std::make_pair(info->mMeshFileName, poly_mesh));
	
		mesh->setMesh( poly_mesh );
		mesh->setLOD( info->mMinPixelArea );

		for (LLAvatarXmlInfo::LLAvatarMeshInfo::morph_info_list_t::const_iterator xmlinfo_iter = info->mPolyMorphTargetInfoList.begin();
			 xmlinfo_iter != info->mPolyMorphTargetInfoList.end(); 
			 ++xmlinfo_iter)
		{
			const LLAvatarXmlInfo::LLAvatarMeshInfo::morph_info_pair_t *info_pair = &(*xmlinfo_iter);
			LLPolyMorphTarget *param = new LLPolyMorphTarget(mesh->getMesh());
			if (!param->setInfo((LLPolyMorphTargetInfo*)info_pair->first))
			{
				delete param;
				return FALSE;
			}
			else
			{
				if (info_pair->second)
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

	return TRUE;
}

//-----------------------------------------------------------------------------
// loadLayerSets()
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::loadLayersets()
{
	BOOL success = TRUE;
	for (LLAvatarXmlInfo::layer_info_list_t::const_iterator layerset_iter = sAvatarXmlInfo->mLayerInfoList.begin();
		 layerset_iter != sAvatarXmlInfo->mLayerInfoList.end(); 
		 ++layerset_iter)
	{
		LLTexLayerSetInfo *layerset_info = *layerset_iter;
		if (isSelf())
		{
			// Construct a layerset for each one specified in avatar_lad.xml and initialize it as such.
			LLTexLayerSet* layer_set = createTexLayerSet();
			
			if (!layer_set->setInfo(layerset_info))
			{
				stop_glerror();
				delete layer_set;
				llwarns << "avatar file: layer_set->setInfo() failed" << llendl;
				return FALSE;
			}

			// scan baked textures and associate the layerset with the appropriate one
			EBakedTextureIndex baked_index = BAKED_NUM_INDICES;
			for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator baked_iter = LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().begin();
				 baked_iter != LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().end();
				 ++baked_iter)
			{
				const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = baked_iter->second;
				if (layer_set->isBodyRegion(baked_dict->mName))
				{
					baked_index = baked_iter->first;
					// ensure both structures are aware of each other
					mBakedTextureDatas[baked_index].mTexLayerSet = layer_set;
					layer_set->setBakedTexIndex(baked_index);
					break;
				}
			}
			// if no baked texture was found, warn and cleanup
			if (baked_index == BAKED_NUM_INDICES)
			{
				llwarns << "<layer_set> has invalid body_region attribute" << llendl;
				delete layer_set;
				return FALSE;
			}

			// scan morph masks and let any affected layers know they have an associated morph
			for (LLAvatarAppearance::morph_list_t::const_iterator morph_iter = mBakedTextureDatas[baked_index].mMaskedMorphs.begin();
				morph_iter != mBakedTextureDatas[baked_index].mMaskedMorphs.end();
				 ++morph_iter)
			{
				LLMaskedMorph *morph = *morph_iter;
				LLTexLayerInterface* layer = layer_set->findLayerByName(morph->mLayer);
				if (layer)
				{
					layer->setHasMorph(TRUE);
				}
				else
				{
					llwarns << "Could not find layer named " << morph->mLayer << " to set morph flag" << llendl;
					success = FALSE;
				}
			}
		}
		else // !isSelf()
		{
			// Construct a layerset for each one specified in avatar_lad.xml and initialize it as such.
			LLTexLayerSetInfo *layerset_info = *layerset_iter;
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
LLJoint* LLAvatarAppearance::findCollisionVolume(U32 volume_id)
{
	if ((S32)volume_id > mNumCollisionVolumes)
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
BOOL LLAvatarAppearance::isValid() const
{
	// This should only be called on ourself.
	if (!isSelf())
	{
		llerrs << "Called LLAvatarAppearance::isValid() on when isSelf() == false" << llendl;
	}
	return TRUE;
}


// adds a morph mask to the appropriate baked texture structure
void LLAvatarAppearance::addMaskedMorph(EBakedTextureIndex index, LLVisualParam* morph_target, BOOL invert, std::string layer)
{
	if (index < BAKED_NUM_INDICES)
	{
		LLMaskedMorph *morph = new LLMaskedMorph(morph_target, invert, layer);
		mBakedTextureDatas[index].mMaskedMorphs.push_front(morph);
	}
}


//static
BOOL LLAvatarAppearance::teToColorParams( ETextureIndex te, U32 *param_name )
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

		default:
			llassert(0);
			return FALSE;
	}

	return TRUE;
}

void LLAvatarAppearance::setClothesColor( ETextureIndex te, const LLColor4& new_color, BOOL upload_bake )
{
	U32 param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		setVisualParamWeight( param_name[0], new_color.mV[VX], upload_bake );
		setVisualParamWeight( param_name[1], new_color.mV[VY], upload_bake );
		setVisualParamWeight( param_name[2], new_color.mV[VZ], upload_bake );
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
BOOL LLAvatarAppearance::isWearingWearableType(LLWearableType::EType type) const
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
BOOL LLAvatarAppearance::allocateCollisionVolumes( U32 num )
{
	deleteAndClearArray(mCollisionVolumes);
	mNumCollisionVolumes = 0;

	mCollisionVolumes = new LLAvatarJointCollisionVolume[num];
	if (!mCollisionVolumes)
	{
		return FALSE;
	}

	mNumCollisionVolumes = num;
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLAvatarBoneInfo::parseXml()
//-----------------------------------------------------------------------------
BOOL LLAvatarBoneInfo::parseXml(LLXmlTreeNode* node)
{
	if (node->hasName("bone"))
	{
		mIsJoint = TRUE;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			llwarns << "Bone without name" << llendl;
			return FALSE;
		}
	}
	else if (node->hasName("collision_volume"))
	{
		mIsJoint = FALSE;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			mName = "Collision Volume";
		}
	}
	else
	{
		llwarns << "Invalid node " << node->getName() << llendl;
		return FALSE;
	}

	static LLStdStringHandle pos_string = LLXmlTree::addAttributeString("pos");
	if (!node->getFastAttributeVector3(pos_string, mPos))
	{
		llwarns << "Bone without position" << llendl;
		return FALSE;
	}

	static LLStdStringHandle rot_string = LLXmlTree::addAttributeString("rot");
	if (!node->getFastAttributeVector3(rot_string, mRot))
	{
		llwarns << "Bone without rotation" << llendl;
		return FALSE;
	}
	
	static LLStdStringHandle scale_string = LLXmlTree::addAttributeString("scale");
	if (!node->getFastAttributeVector3(scale_string, mScale))
	{
		llwarns << "Bone without scale" << llendl;
		return FALSE;
	}

	if (mIsJoint)
	{
		static LLStdStringHandle pivot_string = LLXmlTree::addAttributeString("pivot");
		if (!node->getFastAttributeVector3(pivot_string, mPivot))
		{
			llwarns << "Bone without pivot" << llendl;
			return FALSE;
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
			return FALSE;
		}
		mChildList.push_back(child_info);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLAvatarSkeletonInfo::parseXml()
//-----------------------------------------------------------------------------
BOOL LLAvatarSkeletonInfo::parseXml(LLXmlTreeNode* node)
{
	static LLStdStringHandle num_bones_string = LLXmlTree::addAttributeString("num_bones");
	if (!node->getFastAttributeS32(num_bones_string, mNumBones))
	{
		llwarns << "Couldn't find number of bones." << llendl;
		return FALSE;
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
			llwarns << "Error parsing bone in skeleton file" << llendl;
			return FALSE;
		}
		mBoneInfoList.push_back(info);
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
// parseXmlSkeletonNode(): parses <skeleton> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlSkeletonNode(LLXmlTreeNode* root)
{
	LLXmlTreeNode* node = root->getChildByName( "skeleton" );
	if( !node )
	{
		llwarns << "avatar file: missing <skeleton>" << llendl;
		return FALSE;
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
				llwarns << "Can't specify morph param in skeleton definition." << llendl;
			}
			else
			{
				llwarns << "Unknown param type." << llendl;
			}
			continue;
		}
		
		LLPolySkeletalDistortionInfo *info = new LLPolySkeletalDistortionInfo;
		if (!info->parseXml(child))
		{
			delete info;
			return FALSE;
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
			llwarns << "No name supplied for attachment point." << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle joint_string = LLXmlTree::addAttributeString("joint");
		if (!child->getFastAttributeString(joint_string, info->mJointName))
		{
			llwarns << "No bone declared in attachment point " << info->mName << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle position_string = LLXmlTree::addAttributeString("position");
		if (child->getFastAttributeVector3(position_string, info->mPosition))
		{
			info->mHasPosition = TRUE;
		}

		static LLStdStringHandle rotation_string = LLXmlTree::addAttributeString("rotation");
		if (child->getFastAttributeVector3(rotation_string, info->mRotationEuler))
		{
			info->mHasRotation = TRUE;
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
			llwarns << "No id supplied for attachment point " << info->mName << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle slot_string = LLXmlTree::addAttributeString("pie_slice");
		child->getFastAttributeS32(slot_string, info->mPieMenuSlice);
			
		static LLStdStringHandle visible_in_first_person_string = LLXmlTree::addAttributeString("visible_in_first_person");
		child->getFastAttributeBOOL(visible_in_first_person_string, info->mVisibleFirstPerson);

		static LLStdStringHandle hud_attachment_string = LLXmlTree::addAttributeString("hud");
		child->getFastAttributeBOOL(hud_attachment_string, info->mIsHUDAttachment);

		mAttachmentInfoList.push_back(info);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlMeshNodes(): parses <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlMeshNodes(LLXmlTreeNode* root)
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
			llwarns << "Avatar file: <mesh> is missing type attribute.  Ignoring element. " << llendl;
			delete info;
			return FALSE;  // Ignore this element
		}
		
		static LLStdStringHandle lod_string = LLXmlTree::addAttributeString("lod");
		if (!node->getFastAttributeS32( lod_string, info->mLOD ))
		{
			llwarns << "Avatar file: <mesh> is missing lod attribute.  Ignoring element. " << llendl;
			delete info;
			return FALSE;  // Ignore this element
		}

		static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
		if( !node->getFastAttributeString( file_name_string, info->mMeshFileName ) )
		{
			llwarns << "Avatar file: <mesh> is missing file_name attribute.  Ignoring: " << info->mType << llendl;
			delete info;
			return FALSE;  // Ignore this element
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
					llwarns << "Can't specify skeleton param in a mesh definition." << llendl;
				}
				else
				{
					llwarns << "Unknown param type." << llendl;
				}
				continue;
			}

			LLPolyMorphTargetInfo *morphinfo = new LLPolyMorphTargetInfo();
			if (!morphinfo->parseXml(child))
			{
				delete morphinfo;
				delete info;
				return -1;
			}
			BOOL shared = FALSE;
			static LLStdStringHandle shared_string = LLXmlTree::addAttributeString("shared");
			child->getFastAttributeBOOL(shared_string, shared);

			info->mPolyMorphTargetInfoList.push_back(LLAvatarMeshInfo::morph_info_pair_t(morphinfo, shared));
		}

		mMeshInfoList.push_back(info);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlColorNodes(): parses <global_color> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlColorNodes(LLXmlTreeNode* root)
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
					llwarns << "avatar file: multiple instances of skin_color" << llendl;
					return FALSE;
				}
				mTexSkinColorInfo = new LLTexGlobalColorInfo;
				if( !mTexSkinColorInfo->parseXml( color_node ) )
				{
					deleteAndClear(mTexSkinColorInfo);
					llwarns << "avatar file: mTexSkinColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
			else if( global_color_name == "hair_color" )
			{
				if (mTexHairColorInfo)
				{
					llwarns << "avatar file: multiple instances of hair_color" << llendl;
					return FALSE;
				}
				mTexHairColorInfo = new LLTexGlobalColorInfo;
				if( !mTexHairColorInfo->parseXml( color_node ) )
				{
					deleteAndClear(mTexHairColorInfo);
					llwarns << "avatar file: mTexHairColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
			else if( global_color_name == "eye_color" )
			{
				if (mTexEyeColorInfo)
				{
					llwarns << "avatar file: multiple instances of eye_color" << llendl;
					return FALSE;
				}
				mTexEyeColorInfo = new LLTexGlobalColorInfo;
				if( !mTexEyeColorInfo->parseXml( color_node ) )
				{
					llwarns << "avatar file: mTexEyeColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlLayerNodes(): parses <layer_set> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlLayerNodes(LLXmlTreeNode* root)
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
			llwarns << "avatar file: layer_set->parseXml() failed" << llendl;
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlDriverNodes(): parses <driver_parameters> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlDriverNodes(LLXmlTreeNode* root)
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
					llwarns << "avatar file: driver_param->parseXml() failed" << llendl;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlDriverNodes(): parses <driver_parameters> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLAvatarAppearance::LLAvatarXmlInfo::parseXmlMorphNodes(LLXmlTreeNode* root)
{
	LLXmlTreeNode* masks = root->getChildByName( "morph_masks" );
	if( !masks )
	{
		return FALSE;
	}

	for (LLXmlTreeNode* grand_child = masks->getChildByName( "mask" );
		 grand_child;
		 grand_child = masks->getNextNamedChild())
	{
		LLAvatarMorphInfo* info = new LLAvatarMorphInfo();

		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("morph_name");
		if (!grand_child->getFastAttributeString(name_string, info->mName))
		{
			llwarns << "No name supplied for morph mask." << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle region_string = LLXmlTree::addAttributeString("body_region");
		if (!grand_child->getFastAttributeString(region_string, info->mRegion))
		{
			llwarns << "No region supplied for morph mask." << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle layer_string = LLXmlTree::addAttributeString("layer");
		if (!grand_child->getFastAttributeString(layer_string, info->mLayer))
		{
			llwarns << "No layer supplied for morph mask." << llendl;
			delete info;
			continue;
		}

		// optional parameter. don't throw a warning if not present.
		static LLStdStringHandle invert_string = LLXmlTree::addAttributeString("invert");
		grand_child->getFastAttributeBOOL(invert_string, info->mInvert);

		mMorphMaskInfoList.push_back(info);
	}

	return TRUE;
}

//virtual 
LLAvatarAppearance::LLMaskedMorph::LLMaskedMorph(LLVisualParam *morph_target, BOOL invert, std::string layer) :
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



