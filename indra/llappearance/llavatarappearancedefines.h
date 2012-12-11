/** 
 * @file llavatarappearancedefines.h
 * @brief Various LLAvatarAppearance related definitions
 * LLViewerObject
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

#ifndef LL_AVATARAPPEARANCE_DEFINES_H
#define LL_AVATARAPPEARANCE_DEFINES_H

#include <vector>
#include "lljointpickname.h"
#include "lldictionary.h"
#include "llwearabletype.h"
#include "lluuid.h"

namespace LLAvatarAppearanceDefines
{

extern const S32 SCRATCH_TEX_WIDTH;
extern const S32 SCRATCH_TEX_HEIGHT;
extern const S32 IMPOSTOR_PERIOD;

//--------------------------------------------------------------------
// Enums
//--------------------------------------------------------------------
enum ETextureIndex
{
	TEX_INVALID = -1,
	TEX_HEAD_BODYPAINT = 0,
	TEX_UPPER_SHIRT,
	TEX_LOWER_PANTS,
	TEX_EYES_IRIS,
	TEX_HAIR,
	TEX_UPPER_BODYPAINT,
	TEX_LOWER_BODYPAINT,
	TEX_LOWER_SHOES,
	TEX_HEAD_BAKED,		// Pre-composited
	TEX_UPPER_BAKED,	// Pre-composited
	TEX_LOWER_BAKED,	// Pre-composited
	TEX_EYES_BAKED,		// Pre-composited
	TEX_LOWER_SOCKS,
	TEX_UPPER_JACKET,
	TEX_LOWER_JACKET,
	TEX_UPPER_GLOVES,
	TEX_UPPER_UNDERSHIRT,
	TEX_LOWER_UNDERPANTS,
	TEX_SKIRT,
	TEX_SKIRT_BAKED,	// Pre-composited
	TEX_HAIR_BAKED,     // Pre-composited
	TEX_LOWER_ALPHA,
	TEX_UPPER_ALPHA,
	TEX_HEAD_ALPHA,
	TEX_EYES_ALPHA,
	TEX_HAIR_ALPHA,
	TEX_HEAD_TATTOO,
	TEX_UPPER_TATTOO,
	TEX_LOWER_TATTOO,
	TEX_NUM_INDICES
}; 

enum EBakedTextureIndex
{
	BAKED_HEAD = 0,
	BAKED_UPPER,
	BAKED_LOWER,
	BAKED_EYES,
	BAKED_SKIRT,
	BAKED_HAIR,
	BAKED_NUM_INDICES
};

// Reference IDs for each mesh. Used as indices for vector of joints
enum EMeshIndex
{
	MESH_ID_HAIR = 0,
	MESH_ID_HEAD,
	MESH_ID_EYELASH,
	MESH_ID_UPPER_BODY,
	MESH_ID_LOWER_BODY,
	MESH_ID_EYEBALL_LEFT,
	MESH_ID_EYEBALL_RIGHT,
	MESH_ID_SKIRT,
	MESH_ID_NUM_INDICES
};

//--------------------------------------------------------------------
// Vector Types
//--------------------------------------------------------------------
typedef std::vector<ETextureIndex> texture_vec_t;
typedef std::vector<EBakedTextureIndex> bakedtexture_vec_t;
typedef std::vector<EMeshIndex> mesh_vec_t;
typedef std::vector<LLWearableType::EType> wearables_vec_t;

//------------------------------------------------------------------------
// LLAvatarAppearanceDictionary
// 
// Holds dictionary static entries for textures, baked textures, meshes, etc.; i.e.
// information that is common to all avatars.
// 
// This holds const data - it is initialized once and the contents never change after that.
//------------------------------------------------------------------------
class LLAvatarAppearanceDictionary : public LLSingleton<LLAvatarAppearanceDictionary>
{
	//--------------------------------------------------------------------
	// Constructors and Destructors
	//--------------------------------------------------------------------
public:
	LLAvatarAppearanceDictionary();
	virtual ~LLAvatarAppearanceDictionary();
private:
	void createAssociations();
	
	//--------------------------------------------------------------------
	// Local and baked textures
	//--------------------------------------------------------------------
public:
	struct TextureEntry : public LLDictionaryEntry
	{
		TextureEntry(const std::string &name, // this must match the xml name used by LLTexLayerInfo::parseXml
					 bool is_local_texture, 
					 EBakedTextureIndex baked_texture_index = BAKED_NUM_INDICES,
					 const std::string& default_image_name = "",
					 LLWearableType::EType wearable_type = LLWearableType::WT_INVALID);
		const std::string 	mDefaultImageName;
		const LLWearableType::EType mWearableType;
		// It's either a local texture xor baked
		BOOL 				mIsLocalTexture;
		BOOL 				mIsBakedTexture;
		// If it's a local texture, it may be used by a baked texture
		BOOL 				mIsUsedByBakedTexture;
		EBakedTextureIndex 	mBakedTextureIndex;
	};

	struct Textures : public LLDictionary<ETextureIndex, TextureEntry>
	{
		Textures();
	} mTextures;
	const TextureEntry*		getTexture(ETextureIndex index) const { return mTextures.lookup(index); }
	const Textures&			getTextures() const { return mTextures; }
	
	//--------------------------------------------------------------------
	// Meshes
	//--------------------------------------------------------------------
public:
	struct MeshEntry : public LLDictionaryEntry
	{
		MeshEntry(EBakedTextureIndex baked_index, 
				  const std::string &name, // names of mesh types as they are used in avatar_lad.xml
				  U8 level,
				  LLJointPickName pick);
		// Levels of Detail for each mesh.  Must match levels of detail present in avatar_lad.xml
        // Otherwise meshes will be unable to be found, or levels of detail will be ignored
		const U8 						mLOD;
		const EBakedTextureIndex 		mBakedID;
		const LLJointPickName 	mPickName;
	};

	struct MeshEntries : public LLDictionary<EMeshIndex, MeshEntry>
	{
		MeshEntries();
	} mMeshEntries;
	const MeshEntry*		getMeshEntry(EMeshIndex index) const { return mMeshEntries.lookup(index); }
	const MeshEntries&		getMeshEntries() const { return mMeshEntries; }

	//--------------------------------------------------------------------
	// Baked Textures
	//--------------------------------------------------------------------
public:
	struct BakedEntry : public LLDictionaryEntry
	{
		BakedEntry(ETextureIndex tex_index, 
				   const std::string &name, // unused, but necessary for templating.
				   const std::string &hash_name,
				   U32 num_local_textures, ... ); // # local textures, local texture list, # wearables, wearable list
		// Local Textures
		const ETextureIndex mTextureIndex;
		texture_vec_t 		mLocalTextures;
		// Wearables
		const LLUUID 		mWearablesHashID;
		wearables_vec_t 	mWearables;
	};

	struct BakedTextures: public LLDictionary<EBakedTextureIndex, BakedEntry>
	{
		BakedTextures();
	} mBakedTextures;
	const BakedEntry*		getBakedTexture(EBakedTextureIndex index) const { return mBakedTextures.lookup(index); }
	const BakedTextures&	getBakedTextures() const { return mBakedTextures; }
	
	//--------------------------------------------------------------------
	// Convenience Functions
	//--------------------------------------------------------------------
public:
	// Convert from baked texture to associated texture; e.g. BAKED_HEAD -> TEX_HEAD_BAKED
	static ETextureIndex 		bakedToLocalTextureIndex(EBakedTextureIndex t);

	// find a baked texture index based on its name
	static EBakedTextureIndex 	findBakedByRegionName(std::string name);
	static EBakedTextureIndex 	findBakedByImageName(std::string name);

	// Given a texture entry, determine which wearable type owns it.
	static LLWearableType::EType 		getTEWearableType(ETextureIndex index);

}; // End LLAvatarAppearanceDictionary

} // End namespace LLAvatarAppearanceDefines

#endif //LL_AVATARAPPEARANCE_DEFINES_H
