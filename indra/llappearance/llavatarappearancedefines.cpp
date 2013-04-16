/** 
 * @file llavatarappearancedefines.cpp
 * @brief Implementation of LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary 
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
#include "llavatarappearancedefines.h"

const S32 LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH = 512;
const S32 LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT = 512;
const S32 LLAvatarAppearanceDefines::IMPOSTOR_PERIOD = 2;

using namespace LLAvatarAppearanceDefines;

/*********************************************************************************
 * Edit this function to add/remove/change textures and mesh definitions for avatars.
 */

LLAvatarAppearanceDictionary::Textures::Textures()
{
	addEntry(TEX_HEAD_BODYPAINT,              new TextureEntry("head_bodypaint",   TRUE,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
	addEntry(TEX_UPPER_SHIRT,                 new TextureEntry("upper_shirt",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultShirtUUID",     LLWearableType::WT_SHIRT));
	addEntry(TEX_LOWER_PANTS,                 new TextureEntry("lower_pants",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultPantsUUID",     LLWearableType::WT_PANTS));
	addEntry(TEX_EYES_IRIS,                   new TextureEntry("eyes_iris",        TRUE,  BAKED_NUM_INDICES, "UIImgDefaultEyesUUID",      LLWearableType::WT_EYES));
	addEntry(TEX_HAIR,                        new TextureEntry("hair_grain",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultHairUUID",      LLWearableType::WT_HAIR));
	addEntry(TEX_UPPER_BODYPAINT,             new TextureEntry("upper_bodypaint",  TRUE,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
	addEntry(TEX_LOWER_BODYPAINT,             new TextureEntry("lower_bodypaint",  TRUE,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
	addEntry(TEX_LOWER_SHOES,                 new TextureEntry("lower_shoes",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultShoesUUID",     LLWearableType::WT_SHOES));
	addEntry(TEX_LOWER_SOCKS,                 new TextureEntry("lower_socks",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultSocksUUID",     LLWearableType::WT_SOCKS));
	addEntry(TEX_UPPER_JACKET,                new TextureEntry("upper_jacket",     TRUE,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    LLWearableType::WT_JACKET));
	addEntry(TEX_LOWER_JACKET,                new TextureEntry("lower_jacket",     TRUE,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    LLWearableType::WT_JACKET));
	addEntry(TEX_UPPER_GLOVES,                new TextureEntry("upper_gloves",     TRUE,  BAKED_NUM_INDICES, "UIImgDefaultGlovesUUID",    LLWearableType::WT_GLOVES));
	addEntry(TEX_UPPER_UNDERSHIRT,            new TextureEntry("upper_undershirt", TRUE,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", LLWearableType::WT_UNDERSHIRT));
	addEntry(TEX_LOWER_UNDERPANTS,            new TextureEntry("lower_underpants", TRUE,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", LLWearableType::WT_UNDERPANTS));
	addEntry(TEX_SKIRT,                       new TextureEntry("skirt",            TRUE,  BAKED_NUM_INDICES, "UIImgDefaultSkirtUUID",     LLWearableType::WT_SKIRT));

	addEntry(TEX_LOWER_ALPHA,                 new TextureEntry("lower_alpha",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
	addEntry(TEX_UPPER_ALPHA,                 new TextureEntry("upper_alpha",      TRUE,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
	addEntry(TEX_HEAD_ALPHA,                  new TextureEntry("head_alpha",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
	addEntry(TEX_EYES_ALPHA,                  new TextureEntry("eyes_alpha",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
	addEntry(TEX_HAIR_ALPHA,                  new TextureEntry("hair_alpha",       TRUE,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));

	addEntry(TEX_HEAD_TATTOO,                 new TextureEntry("head_tattoo",      TRUE,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));
	addEntry(TEX_UPPER_TATTOO,                new TextureEntry("upper_tattoo",     TRUE,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));
	addEntry(TEX_LOWER_TATTOO,                new TextureEntry("lower_tattoo",     TRUE,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));

	addEntry(TEX_HEAD_BAKED,                  new TextureEntry("head-baked",       FALSE, BAKED_HEAD, "head"));
	addEntry(TEX_UPPER_BAKED,                 new TextureEntry("upper-baked",      FALSE, BAKED_UPPER, "upper"));
	addEntry(TEX_LOWER_BAKED,                 new TextureEntry("lower-baked",      FALSE, BAKED_LOWER, "lower"));
	addEntry(TEX_EYES_BAKED,                  new TextureEntry("eyes-baked",       FALSE, BAKED_EYES, "eyes"));
	addEntry(TEX_HAIR_BAKED,                  new TextureEntry("hair-baked",       FALSE, BAKED_HAIR, "hair"));
	addEntry(TEX_SKIRT_BAKED,                 new TextureEntry("skirt-baked",      FALSE, BAKED_SKIRT, "skirt"));
}

LLAvatarAppearanceDictionary::BakedTextures::BakedTextures()
{
	// Baked textures
	addEntry(BAKED_HEAD,       new BakedEntry(TEX_HEAD_BAKED,  
											  "head", "a4b9dc38-e13b-4df9-b284-751efb0566ff", 
											  3, TEX_HEAD_BODYPAINT, TEX_HEAD_TATTOO, TEX_HEAD_ALPHA,
											  5, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN, LLWearableType::WT_HAIR, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA));

	addEntry(BAKED_UPPER,      new BakedEntry(TEX_UPPER_BAKED, 
											  "upper_body", "5943ff64-d26c-4a90-a8c0-d61f56bd98d4", 
											  7, TEX_UPPER_SHIRT,TEX_UPPER_BODYPAINT, TEX_UPPER_JACKET,
											  TEX_UPPER_GLOVES, TEX_UPPER_UNDERSHIRT, TEX_UPPER_TATTOO, TEX_UPPER_ALPHA,
											  8, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN,	LLWearableType::WT_SHIRT, LLWearableType::WT_JACKET, LLWearableType::WT_GLOVES, LLWearableType::WT_UNDERSHIRT, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA));											  

	addEntry(BAKED_LOWER,      new BakedEntry(TEX_LOWER_BAKED, 
											  "lower_body", "2944ee70-90a7-425d-a5fb-d749c782ed7d",
											  8, TEX_LOWER_PANTS,TEX_LOWER_BODYPAINT,TEX_LOWER_SHOES, TEX_LOWER_SOCKS,
											  TEX_LOWER_JACKET, TEX_LOWER_UNDERPANTS, TEX_LOWER_TATTOO, TEX_LOWER_ALPHA,
											  9, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN,	LLWearableType::WT_PANTS, LLWearableType::WT_SHOES,	 LLWearableType::WT_SOCKS,  LLWearableType::WT_JACKET, LLWearableType::WT_UNDERPANTS, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA));

	addEntry(BAKED_EYES,       new BakedEntry(TEX_EYES_BAKED,  
											  "eyes", "27b1bc0f-979f-4b13-95fe-b981c2ba9788",
											  2, TEX_EYES_IRIS, TEX_EYES_ALPHA,
											  2, LLWearableType::WT_EYES, LLWearableType::WT_ALPHA));

	addEntry(BAKED_SKIRT,      new BakedEntry(TEX_SKIRT_BAKED,
											  "skirt", "03e7e8cb-1368-483b-b6f3-74850838ba63", 
											  1, TEX_SKIRT,
											  1, LLWearableType::WT_SKIRT));

	addEntry(BAKED_HAIR,       new BakedEntry(TEX_HAIR_BAKED,
											  "hair", "a60e85a9-74e8-48d8-8a2d-8129f28d9b61", 
											  2, TEX_HAIR, TEX_HAIR_ALPHA,
											  2, LLWearableType::WT_HAIR, LLWearableType::WT_ALPHA));
}

LLAvatarAppearanceDictionary::MeshEntries::MeshEntries()
{
	// MeshEntries
	addEntry(MESH_ID_HAIR,             new MeshEntry(BAKED_HAIR,  "hairMesh",         6, PN_4));
	addEntry(MESH_ID_HEAD,             new MeshEntry(BAKED_HEAD,  "headMesh",         5, PN_5));
	addEntry(MESH_ID_EYELASH,          new MeshEntry(BAKED_HEAD,  "eyelashMesh",      1, PN_0)); // no baked mesh associated currently
	addEntry(MESH_ID_UPPER_BODY,       new MeshEntry(BAKED_UPPER, "upperBodyMesh",    5, PN_1));
	addEntry(MESH_ID_LOWER_BODY,       new MeshEntry(BAKED_LOWER, "lowerBodyMesh",    5, PN_2));
	addEntry(MESH_ID_EYEBALL_LEFT,     new MeshEntry(BAKED_EYES,  "eyeBallLeftMesh",  2, PN_3));
	addEntry(MESH_ID_EYEBALL_RIGHT,    new MeshEntry(BAKED_EYES,  "eyeBallRightMesh", 2, PN_3));
	addEntry(MESH_ID_SKIRT,            new MeshEntry(BAKED_SKIRT, "skirtMesh",        5, PN_5));
}

/*
 *
 *********************************************************************************/

LLAvatarAppearanceDictionary::LLAvatarAppearanceDictionary()
{
	createAssociations();
}

//virtual 
LLAvatarAppearanceDictionary::~LLAvatarAppearanceDictionary()
{
}

// Baked textures are composites of textures; for each such composited texture,
// map it to the baked texture.
void LLAvatarAppearanceDictionary::createAssociations()
{
	for (BakedTextures::const_iterator iter = mBakedTextures.begin(); iter != mBakedTextures.end(); iter++)
	{
		const EBakedTextureIndex baked_index = (iter->first);
		const BakedEntry *dict = (iter->second);

		// For each texture that this baked texture index affects, associate those textures
		// with this baked texture index.
		for (texture_vec_t::const_iterator local_texture_iter = dict->mLocalTextures.begin();
			 local_texture_iter != dict->mLocalTextures.end();
			 local_texture_iter++)
		{
			const ETextureIndex local_texture_index = (ETextureIndex) *local_texture_iter;
			mTextures[local_texture_index]->mIsUsedByBakedTexture = true;
			mTextures[local_texture_index]->mBakedTextureIndex = baked_index;
		}
	}
		
}

LLAvatarAppearanceDictionary::TextureEntry::TextureEntry(const std::string &name,
												 bool is_local_texture, 
												 EBakedTextureIndex baked_texture_index,
												 const std::string &default_image_name,
												 LLWearableType::EType wearable_type) :
	LLDictionaryEntry(name),
	mIsLocalTexture(is_local_texture),
	mIsBakedTexture(!is_local_texture),
	mIsUsedByBakedTexture(baked_texture_index != BAKED_NUM_INDICES),
	mBakedTextureIndex(baked_texture_index),
	mDefaultImageName(default_image_name),
	mWearableType(wearable_type)
{
}

LLAvatarAppearanceDictionary::MeshEntry::MeshEntry(EBakedTextureIndex baked_index, 
										   const std::string &name, 
										   U8 level,
										   LLJointPickName pick) :
	LLDictionaryEntry(name),
	mBakedID(baked_index),
	mLOD(level),
	mPickName(pick)
{
}
LLAvatarAppearanceDictionary::BakedEntry::BakedEntry(ETextureIndex tex_index, 
											 const std::string &name, 
											 const std::string &hash_name,
											 U32 num_local_textures,
											 ... ) :
	LLDictionaryEntry(name),
	mWearablesHashID(LLUUID(hash_name)),
	mTextureIndex(tex_index)
{
	va_list argp;

	va_start(argp, num_local_textures);

	// Read in local textures
	for (U8 i=0; i < num_local_textures; i++)
	{
		ETextureIndex t = (ETextureIndex)va_arg(argp,int);
		mLocalTextures.push_back(t);
	}

	// Read in number of wearables
	const U32 num_wearables = (U32)va_arg(argp,int);
	// Read in wearables
	for (U8 i=0; i < num_wearables; i++)
	{
		LLWearableType::EType t = (LLWearableType::EType)va_arg(argp,int);
		mWearables.push_back(t);
	}
}

// static
ETextureIndex LLAvatarAppearanceDictionary::bakedToLocalTextureIndex(EBakedTextureIndex index)
{
	return LLAvatarAppearanceDictionary::getInstance()->getBakedTexture(index)->mTextureIndex;
}

// static
EBakedTextureIndex LLAvatarAppearanceDictionary::findBakedByRegionName(std::string name)
{
	U8 index = 0;
	while (index < BAKED_NUM_INDICES)
	{
		const BakedEntry *be = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex) index);
		if (be && be->mName.compare(name) == 0)
		{
			// baked texture found
			return (EBakedTextureIndex) index;
		}
		index++;
	}
	// baked texture could not be found
	return BAKED_NUM_INDICES;
}

// static 
EBakedTextureIndex LLAvatarAppearanceDictionary::findBakedByImageName(std::string name)
{
	U8 index = 0;
	while (index < BAKED_NUM_INDICES)
	{
		const BakedEntry *be = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex) index);
		if (be)
		{
			const TextureEntry *te = LLAvatarAppearanceDictionary::getInstance()->getTexture(be->mTextureIndex);
			if (te && te->mDefaultImageName.compare(name) == 0)
			{
				// baked texture found
				return (EBakedTextureIndex) index;
			}
		}
		index++;
	}
	// baked texture could not be found
	return BAKED_NUM_INDICES;
}

// static
LLWearableType::EType LLAvatarAppearanceDictionary::getTEWearableType(ETextureIndex index )
{
	return getInstance()->getTexture(index)->mWearableType;
}

