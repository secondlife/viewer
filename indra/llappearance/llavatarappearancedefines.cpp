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
#include "indra_constants.h"

const S32 LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH = 2048;
const S32 LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT = 2048;

using namespace LLAvatarAppearanceDefines;

/*********************************************************************************
 * Edit this function to add/remove/change textures and mesh definitions for avatars.
 */

LLAvatarAppearanceDictionary::Textures::Textures()
{
    addEntry(TEX_HEAD_BODYPAINT,              new TextureEntry("head_bodypaint",   true,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
    addEntry(TEX_UPPER_SHIRT,                 new TextureEntry("upper_shirt",      true,  BAKED_NUM_INDICES, "UIImgDefaultShirtUUID",     LLWearableType::WT_SHIRT));
    addEntry(TEX_LOWER_PANTS,                 new TextureEntry("lower_pants",      true,  BAKED_NUM_INDICES, "UIImgDefaultPantsUUID",     LLWearableType::WT_PANTS));
    addEntry(TEX_EYES_IRIS,                   new TextureEntry("eyes_iris",        true,  BAKED_NUM_INDICES, "UIImgDefaultEyesUUID",      LLWearableType::WT_EYES));
    addEntry(TEX_HAIR,                        new TextureEntry("hair_grain",       true,  BAKED_NUM_INDICES, "UIImgDefaultHairUUID",      LLWearableType::WT_HAIR));
    addEntry(TEX_UPPER_BODYPAINT,             new TextureEntry("upper_bodypaint",  true,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
    addEntry(TEX_LOWER_BODYPAINT,             new TextureEntry("lower_bodypaint",  true,  BAKED_NUM_INDICES, "",                          LLWearableType::WT_SKIN));
    addEntry(TEX_LOWER_SHOES,                 new TextureEntry("lower_shoes",      true,  BAKED_NUM_INDICES, "UIImgDefaultShoesUUID",     LLWearableType::WT_SHOES));
    addEntry(TEX_LOWER_SOCKS,                 new TextureEntry("lower_socks",      true,  BAKED_NUM_INDICES, "UIImgDefaultSocksUUID",     LLWearableType::WT_SOCKS));
    addEntry(TEX_UPPER_JACKET,                new TextureEntry("upper_jacket",     true,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    LLWearableType::WT_JACKET));
    addEntry(TEX_LOWER_JACKET,                new TextureEntry("lower_jacket",     true,  BAKED_NUM_INDICES, "UIImgDefaultJacketUUID",    LLWearableType::WT_JACKET));
    addEntry(TEX_UPPER_GLOVES,                new TextureEntry("upper_gloves",     true,  BAKED_NUM_INDICES, "UIImgDefaultGlovesUUID",    LLWearableType::WT_GLOVES));
    addEntry(TEX_UPPER_UNDERSHIRT,            new TextureEntry("upper_undershirt", true,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", LLWearableType::WT_UNDERSHIRT));
    addEntry(TEX_LOWER_UNDERPANTS,            new TextureEntry("lower_underpants", true,  BAKED_NUM_INDICES, "UIImgDefaultUnderwearUUID", LLWearableType::WT_UNDERPANTS));
    addEntry(TEX_SKIRT,                       new TextureEntry("skirt",            true,  BAKED_NUM_INDICES, "UIImgDefaultSkirtUUID",     LLWearableType::WT_SKIRT));

    addEntry(TEX_LOWER_ALPHA,                 new TextureEntry("lower_alpha",      true,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
    addEntry(TEX_UPPER_ALPHA,                 new TextureEntry("upper_alpha",      true,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
    addEntry(TEX_HEAD_ALPHA,                  new TextureEntry("head_alpha",       true,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
    addEntry(TEX_EYES_ALPHA,                  new TextureEntry("eyes_alpha",       true,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));
    addEntry(TEX_HAIR_ALPHA,                  new TextureEntry("hair_alpha",       true,  BAKED_NUM_INDICES, "UIImgDefaultAlphaUUID",     LLWearableType::WT_ALPHA));

    addEntry(TEX_HEAD_TATTOO,                 new TextureEntry("head_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));
    addEntry(TEX_UPPER_TATTOO,                new TextureEntry("upper_tattoo",     true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));
    addEntry(TEX_LOWER_TATTOO,                new TextureEntry("lower_tattoo",     true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_TATTOO));

    addEntry(TEX_HEAD_UNIVERSAL_TATTOO,       new TextureEntry("head_universal_tattoo", true, BAKED_NUM_INDICES, "", LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_UPPER_UNIVERSAL_TATTOO,      new TextureEntry("upper_universal_tattoo", true, BAKED_NUM_INDICES, "", LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_LOWER_UNIVERSAL_TATTOO,      new TextureEntry("lower_universal_tattoo", true, BAKED_NUM_INDICES, "", LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_SKIRT_TATTOO,                new TextureEntry("skirt_tattoo",     true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_HAIR_TATTOO,                 new TextureEntry("hair_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_EYES_TATTOO,                 new TextureEntry("eyes_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_LEFT_ARM_TATTOO,             new TextureEntry("leftarm_tattoo",   true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_LEFT_LEG_TATTOO,             new TextureEntry("leftleg_tattoo",   true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_AUX1_TATTOO,                 new TextureEntry("aux1_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_AUX2_TATTOO,                 new TextureEntry("aux2_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));
    addEntry(TEX_AUX3_TATTOO,                 new TextureEntry("aux3_tattoo",      true,  BAKED_NUM_INDICES, "",     LLWearableType::WT_UNIVERSAL));


    addEntry(TEX_HEAD_BAKED,                  new TextureEntry("head-baked",       false, BAKED_HEAD, "head"));
    addEntry(TEX_UPPER_BAKED,                 new TextureEntry("upper-baked",      false, BAKED_UPPER, "upper"));
    addEntry(TEX_LOWER_BAKED,                 new TextureEntry("lower-baked",      false, BAKED_LOWER, "lower"));
    addEntry(TEX_EYES_BAKED,                  new TextureEntry("eyes-baked",       false, BAKED_EYES, "eyes"));
    addEntry(TEX_HAIR_BAKED,                  new TextureEntry("hair-baked",       false, BAKED_HAIR, "hair"));
    addEntry(TEX_SKIRT_BAKED,                 new TextureEntry("skirt-baked",      false, BAKED_SKIRT, "skirt"));
    addEntry(TEX_LEFT_ARM_BAKED,              new TextureEntry("leftarm-baked",   false, BAKED_LEFT_ARM, "leftarm"));
    addEntry(TEX_LEFT_LEG_BAKED,              new TextureEntry("leftleg-baked",  false, BAKED_LEFT_LEG, "leftleg"));
    addEntry(TEX_AUX1_BAKED,                  new TextureEntry("aux1-baked",       false, BAKED_AUX1, "aux1"));
    addEntry(TEX_AUX2_BAKED,                  new TextureEntry("aux2-baked",       false, BAKED_AUX2, "aux2"));
    addEntry(TEX_AUX3_BAKED,                  new TextureEntry("aux3-baked",       false, BAKED_AUX3, "aux3"));
}

LLAvatarAppearanceDictionary::BakedTextures::BakedTextures()
{
    // Baked textures
    addEntry(BAKED_HEAD,       new BakedEntry(TEX_HEAD_BAKED,
                                              "head", "a4b9dc38-e13b-4df9-b284-751efb0566ff",
                                              4, TEX_HEAD_BODYPAINT, TEX_HEAD_TATTOO, TEX_HEAD_ALPHA, TEX_HEAD_UNIVERSAL_TATTOO,
                                              6, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN, LLWearableType::WT_HAIR, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_UPPER,      new BakedEntry(TEX_UPPER_BAKED,
                                              "upper_body", "5943ff64-d26c-4a90-a8c0-d61f56bd98d4",
                                              8, TEX_UPPER_SHIRT,TEX_UPPER_BODYPAINT, TEX_UPPER_JACKET,
                                              TEX_UPPER_GLOVES, TEX_UPPER_UNDERSHIRT, TEX_UPPER_TATTOO, TEX_UPPER_ALPHA, TEX_UPPER_UNIVERSAL_TATTOO,
                                              9, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN, LLWearableType::WT_SHIRT, LLWearableType::WT_JACKET, LLWearableType::WT_GLOVES, LLWearableType::WT_UNDERSHIRT, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_LOWER,      new BakedEntry(TEX_LOWER_BAKED,
                                              "lower_body", "2944ee70-90a7-425d-a5fb-d749c782ed7d",
                                              9, TEX_LOWER_PANTS,TEX_LOWER_BODYPAINT,TEX_LOWER_SHOES, TEX_LOWER_SOCKS,
                                              TEX_LOWER_JACKET, TEX_LOWER_UNDERPANTS, TEX_LOWER_TATTOO, TEX_LOWER_ALPHA, TEX_LOWER_UNIVERSAL_TATTOO,
                                              10, LLWearableType::WT_SHAPE, LLWearableType::WT_SKIN,    LLWearableType::WT_PANTS, LLWearableType::WT_SHOES,  LLWearableType::WT_SOCKS,  LLWearableType::WT_JACKET, LLWearableType::WT_UNDERPANTS, LLWearableType::WT_TATTOO, LLWearableType::WT_ALPHA, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_EYES,       new BakedEntry(TEX_EYES_BAKED,
                                              "eyes", "27b1bc0f-979f-4b13-95fe-b981c2ba9788",
                                              3, TEX_EYES_IRIS, TEX_EYES_TATTOO, TEX_EYES_ALPHA,
                                              3, LLWearableType::WT_EYES, LLWearableType::WT_UNIVERSAL, LLWearableType::WT_ALPHA));

    addEntry(BAKED_SKIRT,      new BakedEntry(TEX_SKIRT_BAKED,
                                              "skirt", "03e7e8cb-1368-483b-b6f3-74850838ba63",
                                              2, TEX_SKIRT, TEX_SKIRT_TATTOO,
                                              2, LLWearableType::WT_SKIRT, LLWearableType::WT_UNIVERSAL ));

    addEntry(BAKED_HAIR,       new BakedEntry(TEX_HAIR_BAKED,
                                              "hair", "a60e85a9-74e8-48d8-8a2d-8129f28d9b61",
                                              3, TEX_HAIR, TEX_HAIR_TATTOO, TEX_HAIR_ALPHA,
                                              3, LLWearableType::WT_HAIR, LLWearableType::WT_UNIVERSAL, LLWearableType::WT_ALPHA));

    addEntry(BAKED_LEFT_ARM, new BakedEntry(TEX_LEFT_ARM_BAKED,
        "leftarm", "9f39febf-22d7-0087-79d1-e9e8c6c9ed19",
        1, TEX_LEFT_ARM_TATTOO,
        1, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_LEFT_LEG, new BakedEntry(TEX_LEFT_LEG_BAKED,
        "leftleg", "054a7a58-8ed5-6386-0add-3b636fb28b78",
        1, TEX_LEFT_LEG_TATTOO,
        1, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_AUX1, new BakedEntry(TEX_AUX1_BAKED,
        "aux1", "790c11be-b25c-c17e-b4d2-6a4ad786b752",
        1, TEX_AUX1_TATTOO,
        1, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_AUX2, new BakedEntry(TEX_AUX2_BAKED,
        "aux2", "d78c478f-48c7-5928-5864-8d99fb1f521e",
        1, TEX_AUX2_TATTOO,
        1, LLWearableType::WT_UNIVERSAL));

    addEntry(BAKED_AUX3, new BakedEntry(TEX_AUX3_BAKED,
        "aux3", "6a95dd53-edd9-aac8-f6d3-27ed99f3c3eb",
        1, TEX_AUX3_TATTOO,
        1, LLWearableType::WT_UNIVERSAL));
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
    for (BakedTextures::value_type& baked_pair : mBakedTextures)
    {
        const EBakedTextureIndex baked_index = baked_pair.first;
        const BakedEntry *dict = baked_pair.second;

        // For each texture that this baked texture index affects, associate those textures
        // with this baked texture index.
        for (const ETextureIndex local_texture_index : dict->mLocalTextures)
        {
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
    va_end(argp);
}

ETextureIndex LLAvatarAppearanceDictionary::bakedToLocalTextureIndex(EBakedTextureIndex index) const
{
    return getBakedTexture(index)->mTextureIndex;
}

EBakedTextureIndex LLAvatarAppearanceDictionary::findBakedByRegionName(std::string name)
{
    U8 index = 0;
    while (index < BAKED_NUM_INDICES)
    {
        const BakedEntry *be = getBakedTexture((EBakedTextureIndex) index);
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

EBakedTextureIndex LLAvatarAppearanceDictionary::findBakedByImageName(std::string name)
{
    U8 index = 0;
    while (index < BAKED_NUM_INDICES)
    {
        const BakedEntry *be = getBakedTexture((EBakedTextureIndex) index);
        if (be)
        {
            const TextureEntry *te = getTexture(be->mTextureIndex);
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

LLWearableType::EType LLAvatarAppearanceDictionary::getTEWearableType(ETextureIndex index ) const
{
    auto* tex = getTexture(index);
    return tex ? tex->mWearableType : LLWearableType::WT_INVALID;
}

// static
bool LLAvatarAppearanceDictionary::isBakedImageId(const LLUUID& id)
{
    if ((id == IMG_USE_BAKED_EYES) || (id == IMG_USE_BAKED_HAIR) || (id == IMG_USE_BAKED_HEAD) || (id == IMG_USE_BAKED_LOWER) || (id == IMG_USE_BAKED_SKIRT) || (id == IMG_USE_BAKED_UPPER)
        || (id == IMG_USE_BAKED_LEFTARM) || (id == IMG_USE_BAKED_LEFTLEG) || (id == IMG_USE_BAKED_AUX1) || (id == IMG_USE_BAKED_AUX2) || (id == IMG_USE_BAKED_AUX3) )
    {
        return true;
    }

    return false;
}

// static
EBakedTextureIndex LLAvatarAppearanceDictionary::assetIdToBakedTextureIndex(const LLUUID& id)
{
    if (id == IMG_USE_BAKED_EYES)
    {
        return BAKED_EYES;
    }
    else if (id == IMG_USE_BAKED_HAIR)
    {
        return BAKED_HAIR;
    }
    else if (id == IMG_USE_BAKED_HEAD)
    {
        return BAKED_HEAD;
    }
    else if (id == IMG_USE_BAKED_LOWER)
    {
        return BAKED_LOWER;
    }
    else if (id == IMG_USE_BAKED_SKIRT)
    {
        return BAKED_SKIRT;
    }
    else if (id == IMG_USE_BAKED_UPPER)
    {
        return BAKED_UPPER;
    }
    else if (id == IMG_USE_BAKED_LEFTARM)
    {
        return BAKED_LEFT_ARM;
    }
    else if (id == IMG_USE_BAKED_LEFTLEG)
    {
        return BAKED_LEFT_LEG;
    }
    else if (id == IMG_USE_BAKED_AUX1)
    {
        return BAKED_AUX1;
    }
    else if (id == IMG_USE_BAKED_AUX2)
    {
        return BAKED_AUX2;
    }
    else if (id == IMG_USE_BAKED_AUX3)
    {
        return BAKED_AUX3;
    }

    return BAKED_NUM_INDICES;
}

//static
LLUUID LLAvatarAppearanceDictionary::localTextureIndexToMagicId(ETextureIndex t)
{
    LLUUID id = LLUUID::null;

    switch (t)
    {
    case LLAvatarAppearanceDefines::TEX_HEAD_BAKED:
        id = IMG_USE_BAKED_HEAD;
        break;
    case LLAvatarAppearanceDefines::TEX_UPPER_BAKED:
        id = IMG_USE_BAKED_UPPER;
        break;
    case LLAvatarAppearanceDefines::TEX_LOWER_BAKED:
        id = IMG_USE_BAKED_LOWER;
        break;
    case LLAvatarAppearanceDefines::TEX_EYES_BAKED:
        id = IMG_USE_BAKED_EYES;
        break;
    case LLAvatarAppearanceDefines::TEX_SKIRT_BAKED:
        id = IMG_USE_BAKED_SKIRT;
        break;
    case LLAvatarAppearanceDefines::TEX_HAIR_BAKED:
        id = IMG_USE_BAKED_HAIR;
        break;
    case LLAvatarAppearanceDefines::TEX_LEFT_ARM_BAKED:
        id = IMG_USE_BAKED_LEFTARM;
        break;
    case LLAvatarAppearanceDefines::TEX_LEFT_LEG_BAKED:
        id = IMG_USE_BAKED_LEFTLEG;
        break;
    case LLAvatarAppearanceDefines::TEX_AUX1_BAKED:
        id = IMG_USE_BAKED_AUX1;
        break;
    case LLAvatarAppearanceDefines::TEX_AUX2_BAKED:
        id = IMG_USE_BAKED_AUX2;
        break;
    case LLAvatarAppearanceDefines::TEX_AUX3_BAKED:
        id = IMG_USE_BAKED_AUX3;
        break;
    default:
        break;
    }

    return id;
}
