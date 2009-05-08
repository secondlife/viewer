/** 
 * @file llvoavatar.cpp
 * @brief Implementation of LLVOAvatar class which is a derivation fo LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llvoavatardefines.h"

const S32 LLVOAvatarDefines::SCRATCH_TEX_WIDTH = 512;
const S32 LLVOAvatarDefines::SCRATCH_TEX_HEIGHT = 512;
const S32 LLVOAvatarDefines::IMPOSTOR_PERIOD = 2;

using namespace LLVOAvatarDefines;

/*********************************************************************************
 * Edit this function to add/remove/change textures and mesh definitions for avatars.
 */
void LLVOAvatarDictionary::initData()
{
	// Textures
	mTextureMap[TEX_HEAD_BODYPAINT] =            new TextureDictionaryEntry("head bodypaint",   TRUE,  BAKED_NUM_INDICES, "",                          WT_SKIN);
	mTextureMap[TEX_UPPER_SHIRT] =               new TextureDictionaryEntry("shirt",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultShirtUUID",     WT_SHIRT);
	mTextureMap[TEX_LOWER_PANTS] =               new TextureDictionaryEntry("pants",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultPantsUUID",     WT_PANTS);
	mTextureMap[TEX_EYES_IRIS] =                 new TextureDictionaryEntry("iris",             TRUE,  BAKED_NUM_INDICES, "UIImgDefaultEyesUUID",      WT_EYES);
	mTextureMap[TEX_HAIR] =                      new TextureDictionaryEntry("hair",             TRUE,  BAKED_NUM_INDICES, "UIImgDefaultHairUUID",      WT_HAIR);
	mTextureMap[TEX_UPPER_BODYPAINT] =           new TextureDictionaryEntry("upper bodypaint",  TRUE,  BAKED_NUM_INDICES, "",                          WT_SKIN);
	mTextureMap[TEX_LOWER_BODYPAINT] =           new TextureDictionaryEntry("lower bodypaint",  TRUE,  BAKED_NUM_INDICES, "",                          WT_SKIN);
	mTextureMap[TEX_LOWER_SHOES] =               new TextureDictionaryEntry("shoes",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultShoesUUID",     WT_SHOES);
	mTextureMap[TEX_LOWER_SOCKS] =               new TextureDictionaryEntry("socks",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultSocksUUID",     WT_SOCKS);
	mTextureMap[TEX_UPPER_JACKET] =              new TextureDictionaryEntry("upper jacket",     TRUE,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    WT_JACKET);
	mTextureMap[TEX_LOWER_JACKET] =              new TextureDictionaryEntry("lower jacket",     TRUE,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    WT_JACKET);
	mTextureMap[TEX_UPPER_GLOVES] =              new TextureDictionaryEntry("gloves",           TRUE,  BAKED_NUM_INDICES, "UIImgDefaultGlovesUUID",    WT_GLOVES);
	mTextureMap[TEX_UPPER_UNDERSHIRT] =          new TextureDictionaryEntry("undershirt",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", WT_UNDERSHIRT);
	mTextureMap[TEX_LOWER_UNDERPANTS] =          new TextureDictionaryEntry("underpants",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", WT_UNDERPANTS);
	mTextureMap[TEX_SKIRT] =                     new TextureDictionaryEntry("skirt",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultSkirtUUID",     WT_SKIRT);
	mTextureMap[TEX_HEAD_BAKED] =                new TextureDictionaryEntry("head-baked",       FALSE, BAKED_HEAD);
	mTextureMap[TEX_UPPER_BAKED] =               new TextureDictionaryEntry("upper-baked",      FALSE, BAKED_UPPER);
	mTextureMap[TEX_LOWER_BAKED] =               new TextureDictionaryEntry("lower-baked",      FALSE, BAKED_LOWER);
	mTextureMap[TEX_EYES_BAKED] =                new TextureDictionaryEntry("eyes-baked",       FALSE, BAKED_EYES);
	mTextureMap[TEX_HAIR_BAKED] =                new TextureDictionaryEntry("hair-baked",       FALSE, BAKED_HAIR);
	mTextureMap[TEX_SKIRT_BAKED] =               new TextureDictionaryEntry("skirt-baked",      FALSE, BAKED_SKIRT);

	// Baked textures
	mBakedTextureMap[BAKED_HEAD] =     new BakedDictionaryEntry(TEX_HEAD_BAKED,  "head",       1, TEX_HEAD_BODYPAINT);
	mBakedTextureMap[BAKED_UPPER] =    new BakedDictionaryEntry(TEX_UPPER_BAKED, "upper_body", 5, TEX_UPPER_SHIRT,TEX_UPPER_BODYPAINT,TEX_UPPER_JACKET,TEX_UPPER_GLOVES,TEX_UPPER_UNDERSHIRT);
	mBakedTextureMap[BAKED_LOWER] =    new BakedDictionaryEntry(TEX_LOWER_BAKED, "lower_body", 6, TEX_LOWER_PANTS,TEX_LOWER_BODYPAINT,TEX_LOWER_SHOES,TEX_LOWER_SOCKS,TEX_LOWER_JACKET,TEX_LOWER_UNDERPANTS);
	mBakedTextureMap[BAKED_EYES] =     new BakedDictionaryEntry(TEX_EYES_BAKED,  "eyes",       1, TEX_EYES_IRIS);
	mBakedTextureMap[BAKED_SKIRT] =    new BakedDictionaryEntry(TEX_SKIRT_BAKED, "skirt",      1, TEX_SKIRT);
	mBakedTextureMap[BAKED_HAIR] =     new BakedDictionaryEntry(TEX_HAIR_BAKED,  "hair",       1, TEX_HAIR);
		
	// Meshes
	mMeshMap[MESH_ID_HAIR] =           new MeshDictionaryEntry(BAKED_HAIR,  "hairMesh",         6, LLViewerJoint::PN_4);
	mMeshMap[MESH_ID_HEAD] =           new MeshDictionaryEntry(BAKED_HEAD,  "headMesh",         5, LLViewerJoint::PN_5);
	mMeshMap[MESH_ID_EYELASH] =        new MeshDictionaryEntry(BAKED_HEAD,  "eyelashMesh",      1, LLViewerJoint::PN_0); // no baked mesh associated currently
	mMeshMap[MESH_ID_UPPER_BODY] =     new MeshDictionaryEntry(BAKED_UPPER, "upperBodyMesh",    5, LLViewerJoint::PN_1);
	mMeshMap[MESH_ID_LOWER_BODY] =     new MeshDictionaryEntry(BAKED_LOWER, "lowerBodyMesh",    5, LLViewerJoint::PN_2);
	mMeshMap[MESH_ID_EYEBALL_LEFT] =   new MeshDictionaryEntry(BAKED_EYES,  "eyeBallLeftMesh",  2, LLViewerJoint::PN_3);
	mMeshMap[MESH_ID_EYEBALL_RIGHT] =  new MeshDictionaryEntry(BAKED_EYES,  "eyeBallRightMesh", 2, LLViewerJoint::PN_3);
	mMeshMap[MESH_ID_SKIRT] =          new MeshDictionaryEntry(BAKED_SKIRT, "skirtMesh",        5, LLViewerJoint::PN_5);

	// Wearables
	mWearableMap[BAKED_HEAD] =   new WearableDictionaryEntry("18ded8d6-bcfc-e415-8539-944c0f5ea7a6", 3, WT_SHAPE, WT_SKIN, WT_HAIR);
	mWearableMap[BAKED_UPPER] =  new WearableDictionaryEntry("338c29e3-3024-4dbb-998d-7c04cf4fa88f", 6, WT_SHAPE, WT_SKIN,	WT_SHIRT, WT_JACKET, WT_GLOVES, WT_UNDERSHIRT);
	mWearableMap[BAKED_LOWER] =  new WearableDictionaryEntry("91b4a2c7-1b1a-ba16-9a16-1f8f8dcc1c3f", 7, WT_SHAPE, WT_SKIN,	WT_PANTS, WT_SHOES,	 WT_SOCKS,  WT_JACKET,		WT_UNDERPANTS);
	mWearableMap[BAKED_EYES] =   new WearableDictionaryEntry("b2cf28af-b840-1071-3c6a-78085d8128b5", 1, WT_EYES);
	mWearableMap[BAKED_SKIRT] =  new WearableDictionaryEntry("ea800387-ea1a-14e0-56cb-24f2022f969a", 1, WT_SKIRT);
	mWearableMap[BAKED_HAIR] =   new WearableDictionaryEntry("0af1ef7c-ad24-11dd-8790-001f5bf833e8", 1, WT_HAIR);
}

/*
 *
 *********************************************************************************/

LLVOAvatarDictionary::LLVOAvatarDictionary()
{
	initData();
	createAssociations();
}

// Baked textures are composites of textures; for each such composited texture,
// map it to the baked texture.
void LLVOAvatarDictionary::createAssociations()
{
	for (baked_map_t::const_iterator iter = mBakedTextureMap.begin(); iter != mBakedTextureMap.end(); iter++)
	{
		const EBakedTextureIndex baked_index = (iter->first);
		const BakedDictionaryEntry *dict = (iter->second);

		// For each texture that this baked texture index affects, associate those textures
		// with this baked texture index.
		for (texture_vec_t::const_iterator local_texture_iter = dict->mLocalTextures.begin();
			 local_texture_iter != dict->mLocalTextures.end();
			 local_texture_iter++)
		{
			const ETextureIndex local_texture_index = (ETextureIndex) *local_texture_iter;
			mTextureMap[local_texture_index]->mIsUsedByBakedTexture = true;
			mTextureMap[local_texture_index]->mBakedTextureIndex = baked_index;
		}
	}
		
}

LLVOAvatarDictionary::TextureDictionaryEntry::TextureDictionaryEntry(const std::string &name, 
																	 bool is_local_texture, 
																	 EBakedTextureIndex baked_texture_index,
																	 const std::string &default_image_name,
																	 EWearableType wearable_type) :
	mName(name),
	mIsLocalTexture(is_local_texture),
	mIsBakedTexture(!is_local_texture),
	mIsUsedByBakedTexture(baked_texture_index != BAKED_NUM_INDICES),
	mBakedTextureIndex(baked_texture_index),
	mDefaultImageName(default_image_name),
	mWearableType(wearable_type)
{
}

LLVOAvatarDictionary::MeshDictionaryEntry::MeshDictionaryEntry(EBakedTextureIndex baked_index, 
															   const std::string &name, 
															   U8 level,
															   LLViewerJoint::PickName pick) :
	mBakedID(baked_index),
	mName(name),
	mLOD(level),
	mPickName(pick)
{
}
LLVOAvatarDictionary::BakedDictionaryEntry::BakedDictionaryEntry(ETextureIndex tex_index, 
																 const std::string &name, 
																 U32 num_local_textures, ... ) :
	mName(name),
	mTextureIndex(tex_index)

{
	va_list argp;
	va_start(argp, num_local_textures);
	for (U8 i=0; i < num_local_textures; i++)
	{
		ETextureIndex t = (ETextureIndex)va_arg(argp,int);
		mLocalTextures.push_back(t);
	}
}

LLVOAvatarDictionary::WearableDictionaryEntry::WearableDictionaryEntry(const std::string &hash_name, 
																	   U32 num_wearables, ... ) :
	mHashID(LLUUID(hash_name))
{
	va_list argp;
	va_start(argp, num_wearables);
	for (U8 i=0; i < num_wearables; i++)
	{
		EWearableType t = (EWearableType)va_arg(argp,int);
		mWearablesVec.push_back(t);
	}
}

//virtual 
LLVOAvatarDictionary::~LLVOAvatarDictionary()
{
	for (mesh_map_t::iterator iter = mMeshMap.begin(); iter != mMeshMap.end(); iter++)
		delete (iter->second);
	for (baked_map_t::iterator iter = mBakedTextureMap.begin(); iter != mBakedTextureMap.end(); iter++)
		delete (iter->second);
	for (texture_map_t::iterator iter = mTextureMap.begin(); iter != mTextureMap.end(); iter++)
		delete (iter->second);
}

const LLVOAvatarDictionary::MeshDictionaryEntry *LLVOAvatarDictionary::getMesh(EMeshIndex index) const
{
	mesh_map_t::const_iterator mesh_iter = mMeshMap.find(index);
	if (mesh_iter == mMeshMap.end()) return NULL;
	return mesh_iter->second; 
}

const LLVOAvatarDictionary::BakedDictionaryEntry *LLVOAvatarDictionary::getBakedTexture(EBakedTextureIndex index) const
{
	baked_map_t::const_iterator baked_iter = mBakedTextureMap.find(index);
	if (baked_iter == mBakedTextureMap.end()) return NULL;
	return baked_iter->second; 
}

const LLVOAvatarDictionary::TextureDictionaryEntry *LLVOAvatarDictionary::getTexture(ETextureIndex index) const
{
	texture_map_t::const_iterator texture_iter = mTextureMap.find(index);
	if (texture_iter == mTextureMap.end()) return NULL;
	return texture_iter->second;
}

const LLVOAvatarDictionary::WearableDictionaryEntry *LLVOAvatarDictionary::getWearable(EBakedTextureIndex index) const
{
	wearable_map_t::const_iterator wearable_iter = mWearableMap.find(index);
	if (wearable_iter == mWearableMap.end()) return NULL;
	return wearable_iter->second;
}



ETextureIndex LLVOAvatarDefines::getTextureIndex(EBakedTextureIndex index)
{
	return LLVOAvatarDictionary::getInstance()->getBakedTexture(index)->mTextureIndex;
}
