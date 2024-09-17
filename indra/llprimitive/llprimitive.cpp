/**
 * @file llprimitive.cpp
 * @brief LLPrimitive base class
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

#include "material_codes.h"
#include "llerror.h"
#include "message.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "legacy_object_types.h"
#include "v4coloru.h"
#include "llvolumemgr.h"
#include "llstring.h"
#include "lldatapacker.h"
#include "llsdutil_math.h"
#include "llprimtexturelist.h"
#include "llmaterialid.h"
#include "llsdutil.h"

/**
 * exported constants
 */

const F32 OBJECT_CUT_MIN = 0.f;
const F32 OBJECT_CUT_MAX = 1.f;
const F32 OBJECT_CUT_INC = 0.05f;
const F32 OBJECT_MIN_CUT_INC = 0.02f;
const F32 OBJECT_ROTATION_PRECISION = 0.05f;

const F32 OBJECT_TWIST_MIN = -360.f;
const F32 OBJECT_TWIST_MAX =  360.f;
const F32 OBJECT_TWIST_INC =   18.f;

// This is used for linear paths,
// since twist is used in a slightly different manner.
const F32 OBJECT_TWIST_LINEAR_MIN   = -180.f;
const F32 OBJECT_TWIST_LINEAR_MAX   =  180.f;
const F32 OBJECT_TWIST_LINEAR_INC   =    9.f;

const F32 OBJECT_MIN_HOLE_SIZE = 0.05f;
const F32 OBJECT_MAX_HOLE_SIZE_X = 1.0f;
const F32 OBJECT_MAX_HOLE_SIZE_Y = 0.5f;

// Revolutions parameters.
const F32 OBJECT_REV_MIN = 1.0f;
const F32 OBJECT_REV_MAX = 4.0f;
const F32 OBJECT_REV_INC = 0.1f;

// lights
const F32 LIGHT_MIN_RADIUS = 0.0f;
const F32 LIGHT_DEFAULT_RADIUS = 5.0f;
const F32 LIGHT_MAX_RADIUS = 20.0f;
const F32 LIGHT_MIN_FALLOFF = 0.0f;
const F32 LIGHT_DEFAULT_FALLOFF = 1.0f;
const F32 LIGHT_MAX_FALLOFF = 2.0f;
const F32 LIGHT_MIN_CUTOFF = 0.0f;
const F32 LIGHT_DEFAULT_CUTOFF = 0.0f;
const F32 LIGHT_MAX_CUTOFF = 180.f;

// reflection probes
const F32 REFLECTION_PROBE_MIN_AMBIANCE = 0.f;
const F32 REFLECTION_PROBE_MAX_AMBIANCE = 100.f;
const F32 REFLECTION_PROBE_DEFAULT_AMBIANCE = 0.f;
// *NOTE: Clip distances are clamped in LLCamera::setNear. The max clip
// distance is currently limited by the skybox
const F32 REFLECTION_PROBE_MIN_CLIP_DISTANCE = 0.f;
const F32 REFLECTION_PROBE_MAX_CLIP_DISTANCE = 1024.f;
const F32 REFLECTION_PROBE_DEFAULT_CLIP_DISTANCE = 0.f;

// "Tension" => [0,10], increments of 0.1
const F32 FLEXIBLE_OBJECT_MIN_TENSION = 0.0f;
const F32 FLEXIBLE_OBJECT_DEFAULT_TENSION = 1.0f;
const F32 FLEXIBLE_OBJECT_MAX_TENSION = 10.0f;

// "Drag" => [0,10], increments of 0.1
const F32 FLEXIBLE_OBJECT_MIN_AIR_FRICTION = 0.0f;
const F32 FLEXIBLE_OBJECT_DEFAULT_AIR_FRICTION = 2.0f;
const F32 FLEXIBLE_OBJECT_MAX_AIR_FRICTION = 10.0f;

// "Gravity" = [-10,10], increments of 0.1
const F32 FLEXIBLE_OBJECT_MIN_GRAVITY = -10.0f;
const F32 FLEXIBLE_OBJECT_DEFAULT_GRAVITY = 0.3f;
const F32 FLEXIBLE_OBJECT_MAX_GRAVITY = 10.0f;

// "Wind" = [0,10], increments of 0.1
const F32 FLEXIBLE_OBJECT_MIN_WIND_SENSITIVITY = 0.0f;
const F32 FLEXIBLE_OBJECT_DEFAULT_WIND_SENSITIVITY = 0.0f;
const F32 FLEXIBLE_OBJECT_MAX_WIND_SENSITIVITY = 10.0f;

// I'll explain later...
const F32 FLEXIBLE_OBJECT_MAX_INTERNAL_TENSION_FORCE = 0.99f;

const F32 FLEXIBLE_OBJECT_DEFAULT_LENGTH = 1.0f;
const bool FLEXIBLE_OBJECT_DEFAULT_USING_COLLISION_SPHERE = false;
const bool FLEXIBLE_OBJECT_DEFAULT_RENDERING_COLLISION_SPHERE = false;

const LLUUID SCULPT_DEFAULT_TEXTURE("be293869-d0d9-0a69-5989-ad27f1946fd4"); // old inverted texture: "7595d345-a24c-e7ef-f0bd-78793792133e";

// Texture rotations are sent over the wire as a S16.  This is used to scale the actual float
// value to a S16.   Don't use 7FFF as it introduces some odd rounding with 180 since it
// can't be divided by 2.   See DEV-19108
const F32   TEXTURE_ROTATION_PACK_FACTOR = ((F32) 0x08000);

constexpr U32 MAX_TE_BUFFER = 4096;
constexpr U8 EXTRA_PROPERTY_ALPHA_GAMMA = 0x01;

struct material_id_type // originally from llrendermaterialtable
{
    material_id_type()
    {
        memset((void*)m_value, 0, sizeof(m_value));
    }

    bool operator==(const material_id_type& other) const
    {
        return (memcmp(m_value, other.m_value, sizeof(m_value)) == 0);
    }

    bool operator!=(const material_id_type& other) const
    {
        return !operator==(other);
    }

    bool isNull() const
    {
        return (memcmp(m_value, s_null_id, sizeof(m_value)) == 0);
    }

    U8 m_value[MATERIAL_ID_SIZE]; // server side this is MD5RAW_BYTES

    static const U8 s_null_id[MATERIAL_ID_SIZE];
};

const U8 material_id_type::s_null_id[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

//static
// LEGACY: by default we use the LLVolumeMgr::gVolumeMgr global
// TODO -- eliminate this global from the codebase!
LLVolumeMgr* LLPrimitive::sVolumeManager = NULL;

LLTEContents::LLTEContents(size_t N)
{
    llassert(N > 0);
    N = std::min(N, size_t(LLTEContents::MAX_TES));
    num_textures = N;

    // allocate one big buffer for all the data and fill it with zeros
    size_t bytes_per_texture = sizeof(LLUUID)
        + sizeof(LLMaterialID)
        + sizeof(LLColor4U)
        + 2 * sizeof(F32)
        + 3 * sizeof(S16)
        + 4 * sizeof(U8);
    size_t num_bytes = num_textures * bytes_per_texture;
    data = new U8[num_bytes];
    memset(data, 0, num_bytes);

    // compute offset pointers into data for the various fields
    size_t offset = 0;
    image_ids = reinterpret_cast<LLUUID*>(data);
    offset += num_textures * sizeof(LLUUID);

    material_ids = reinterpret_cast<LLMaterialID*>(data + offset);
    offset += num_textures * sizeof(LLMaterialID);

    colors = reinterpret_cast<LLColor4U*>(data + offset);
    offset += num_textures * sizeof(LLColor4U);

    scale_s = reinterpret_cast<F32*>(data + offset);
    offset += num_textures * sizeof(F32);

    scale_t = reinterpret_cast<F32*>(data + offset);
    offset += num_textures * sizeof(F32);

    offset_s = reinterpret_cast<S16*>(data + offset);
    offset += num_textures * sizeof(S16);

    offset_t = reinterpret_cast<S16*>(data + offset);
    offset += num_textures * sizeof(S16);

    rot = reinterpret_cast<S16*>(data + offset);
    offset += num_textures * sizeof(S16);

    bump = reinterpret_cast<U8*>(data + offset);
    offset += num_textures * sizeof(U8);

    media_flags = reinterpret_cast<U8*>(data + offset);
    offset += num_textures * sizeof(U8);

    glow = reinterpret_cast<U8*>(data + offset);
    offset += num_textures * sizeof(U8);

    alpha_gamma = reinterpret_cast<U8*>(data + offset);
    offset += num_textures * sizeof(U8);

    // verify we correctly computed data size
    llassert(offset == num_bytes);
}

LLTEContents::~LLTEContents()
{
    delete data;
    data = nullptr;
}

// static
void LLPrimitive::setVolumeManager( LLVolumeMgr* volume_manager )
{
    if ( !volume_manager || sVolumeManager )
    {
        LL_ERRS() << "LLPrimitive::sVolumeManager attempting to be set to NULL or it already has been set." << LL_ENDL;
    }
    sVolumeManager = volume_manager;
}

// static
bool LLPrimitive::cleanupVolumeManager()
{
    bool res = false;
    if (sVolumeManager)
    {
        res = sVolumeManager->cleanup();
        delete sVolumeManager;
        sVolumeManager = NULL;
    }
    return res;
}


//===============================================================
LLPrimitive::LLPrimitive()
:   mTextureList(),
    mNumTEs(0),
    mMiscFlags(0),
    mNumBumpmapTEs(0)
{
    mPrimitiveCode = 0;

    mMaterial = LL_MCODE_STONE;
    mVolumep  = NULL;

    mChanged  = UNCHANGED;

    mPosition.setVec(0.f,0.f,0.f);
    mVelocity.setVec(0.f,0.f,0.f);
    mAcceleration.setVec(0.f,0.f,0.f);

    mRotation.loadIdentity();
    mAngularVelocity.setVec(0.f,0.f,0.f);

    mScale.setVec(1.f,1.f,1.f);
}

//===============================================================
LLPrimitive::~LLPrimitive()
{
    clearTextureList();
    // Cleanup handled by volume manager
    if (mVolumep && sVolumeManager)
    {
        sVolumeManager->unrefVolume(mVolumep);
    }
    mVolumep = NULL;
}

void LLPrimitive::clearTextureList()
{
}

//===============================================================
// static
LLPrimitive *LLPrimitive::createPrimitive(LLPCode p_code)
{
    LLPrimitive *retval = new LLPrimitive();

    if (retval)
    {
        retval->init_primitive(p_code);
    }
    else
    {
        LL_ERRS() << "primitive allocation failed" << LL_ENDL;
    }

    return retval;
}

//===============================================================
void LLPrimitive::init_primitive(LLPCode p_code)
{
    clearTextureList();
    mPrimitiveCode = p_code;
}

void LLPrimitive::setPCode(const U8 p_code)
{
    mPrimitiveCode = p_code;
}

//===============================================================
LLTextureEntry* LLPrimitive::getTE(const U8 index) const
{
    return mTextureList.getTexture(index);
}

//===============================================================
void LLPrimitive::setNumTEs(const U8 num_tes)
{
    mTextureList.setSize(num_tes);
}

//===============================================================
void  LLPrimitive::setAllTETextures(const LLUUID &tex_id)
{
    mTextureList.setAllIDs(tex_id);
}

//===============================================================
void LLPrimitive::setTE(const U8 index, const LLTextureEntry& te)
{
    if(mTextureList.copyTexture(index, te) != TEM_CHANGE_NONE && te.getBumpmap() > 0)
    {
        mNumBumpmapTEs++;
    }
}

S32  LLPrimitive::setTETexture(const U8 index, const LLUUID &id)
{
    return mTextureList.setID(index, id);
}

S32  LLPrimitive::setTEColor(const U8 index, const LLColor4 &color)
{
    return mTextureList.setColor(index, color);
}

S32  LLPrimitive::setTEColor(const U8 index, const LLColor3 &color)
{
    return mTextureList.setColor(index, color);
}

S32  LLPrimitive::setTEAlpha(const U8 index, const F32 alpha)
{
    return mTextureList.setAlpha(index, alpha);
}

//===============================================================
S32  LLPrimitive::setTEScale(const U8 index, const F32 s, const F32 t)
{
    return mTextureList.setScale(index, s, t);
}


// BUG: slow - done this way because texture entries have some
// voodoo related to texture coords
S32 LLPrimitive::setTEScaleS(const U8 index, const F32 s)
{
    return mTextureList.setScaleS(index, s);
}


// BUG: slow - done this way because texture entries have some
// voodoo related to texture coords
S32 LLPrimitive::setTEScaleT(const U8 index, const F32 t)
{
    return mTextureList.setScaleT(index, t);
}


//===============================================================
S32  LLPrimitive::setTEOffset(const U8 index, const F32 s, const F32 t)
{
    return mTextureList.setOffset(index, s, t);
}


// BUG: slow - done this way because texture entries have some
// voodoo related to texture coords
S32 LLPrimitive::setTEOffsetS(const U8 index, const F32 s)
{
    return mTextureList.setOffsetS(index, s);
}


// BUG: slow - done this way because texture entries have some
// voodoo related to texture coords
S32 LLPrimitive::setTEOffsetT(const U8 index, const F32 t)
{
    return mTextureList.setOffsetT(index, t);
}


//===============================================================
S32  LLPrimitive::setTERotation(const U8 index, const F32 r)
{
    return mTextureList.setRotation(index, r);
}

S32 LLPrimitive::setTEMaterialID(const U8 index, const LLMaterialID& pMaterialID)
{
    return mTextureList.setMaterialID(index, pMaterialID);
}

S32 LLPrimitive::setTEMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams)
{
    return mTextureList.setMaterialParams(index, pMaterialParams);
}

LLMaterialPtr LLPrimitive::getTEMaterialParams(const U8 index)
{
    return mTextureList.getMaterialParams(index);
}

//===============================================================
S32  LLPrimitive::setTEBumpShinyFullbright(const U8 index, const U8 bump)
{
    updateNumBumpmap(index, bump);
    return mTextureList.setBumpShinyFullbright(index, bump);
}

S32  LLPrimitive::setTEMediaTexGen(const U8 index, const U8 media)
{
    return mTextureList.setMediaTexGen(index, media);
}

S32  LLPrimitive::setTEBumpmap(const U8 index, const U8 bump)
{
    updateNumBumpmap(index, bump);
    return mTextureList.setBumpMap(index, bump);
}

S32 LLPrimitive::setTEAlphaGamma(const U8 index, const U8 gamma)
{
    return mTextureList.setAlphaGamma(index, gamma);
}

S32  LLPrimitive::setTEBumpShiny(const U8 index, const U8 bump_shiny)
{
    updateNumBumpmap(index, bump_shiny);
    return mTextureList.setBumpShiny(index, bump_shiny);
}

S32  LLPrimitive::setTETexGen(const U8 index, const U8 texgen)
{
    return mTextureList.setTexGen(index, texgen);
}

S32  LLPrimitive::setTEShiny(const U8 index, const U8 shiny)
{
    return mTextureList.setShiny(index, shiny);
}

S32  LLPrimitive::setTEFullbright(const U8 index, const U8 fullbright)
{
    return mTextureList.setFullbright(index, fullbright);
}

S32  LLPrimitive::setTEMediaFlags(const U8 index, const U8 media_flags)
{
    return mTextureList.setMediaFlags(index, media_flags);
}

S32 LLPrimitive::setTEGlow(const U8 index, const F32 glow)
{
    return mTextureList.setGlow(index, glow);
}

void LLPrimitive::setAllTESelected(bool sel)
{
    for (int i = 0, cnt = getNumTEs(); i < cnt; i++)
    {
        setTESelected(i, sel);
    }
}

void LLPrimitive::setTESelected(const U8 te, bool sel)
{
    LLTextureEntry* tep = getTE(te);
    if ( (tep) && (tep->setSelected(sel)) && (!sel) && (tep->hasPendingMaterialUpdate()) )
    {
        LLMaterialID material_id = tep->getMaterialID();
        setTEMaterialID(te, material_id);
    }
}

LLPCode LLPrimitive::legacyToPCode(const U8 legacy)
{
    // TODO: Should this default to something valid?
    // Maybe volume?
    LLPCode pcode = 0;

    switch (legacy)
    {
        /*
    case BOX:
        pcode = LL_PCODE_CUBE;
        break;
    case CYLINDER:
        pcode = LL_PCODE_CYLINDER;
        break;
    case CONE:
        pcode = LL_PCODE_CONE;
        break;
    case HALF_CONE:
        pcode = LL_PCODE_CONE_HEMI;
        break;
    case HALF_CYLINDER:
        pcode = LL_PCODE_CYLINDER_HEMI;
        break;
    case HALF_SPHERE:
        pcode = LL_PCODE_SPHERE_HEMI;
        break;
    case PRISM:
        pcode = LL_PCODE_PRISM;
        break;
    case PYRAMID:
        pcode = LL_PCODE_PYRAMID;
        break;
    case SPHERE:
        pcode = LL_PCODE_SPHERE;
        break;
    case TETRAHEDRON:
        pcode = LL_PCODE_TETRAHEDRON;
        break;
    case DEMON:
        pcode = LL_PCODE_LEGACY_DEMON;
        break;
    case LSL_TEST:
        pcode = LL_PCODE_LEGACY_LSL_TEST;
        break;
    case ORACLE:
        pcode = LL_PCODE_LEGACY_ORACLE;
        break;
    case TEXTBUBBLE:
        pcode = LL_PCODE_LEGACY_TEXT_BUBBLE;
        break;
    case ATOR:
        pcode = LL_PCODE_LEGACY_ATOR;
        break;
    case BASIC_SHOT:
        pcode = LL_PCODE_LEGACY_SHOT;
        break;
    case BIG_SHOT:
        pcode = LL_PCODE_LEGACY_SHOT_BIG;
        break;
    case BIRD:
        pcode = LL_PCODE_LEGACY_BIRD;
        break;
    case ROCK:
        pcode = LL_PCODE_LEGACY_ROCK;
        break;
    case SMOKE:
        pcode = LL_PCODE_LEGACY_SMOKE;
        break;
    case SPARK:
        pcode = LL_PCODE_LEGACY_SPARK;
        break;
        */
    case PRIMITIVE_VOLUME:
        pcode = LL_PCODE_VOLUME;
        break;
    case GRASS:
        pcode = LL_PCODE_LEGACY_GRASS;
        break;
    case PART_SYS:
        pcode = LL_PCODE_LEGACY_PART_SYS;
        break;
    case PLAYER:
        pcode = LL_PCODE_LEGACY_AVATAR;
        break;
    case TREE:
        pcode = LL_PCODE_LEGACY_TREE;
        break;
    case TREE_NEW:
        pcode = LL_PCODE_TREE_NEW;
        break;
    default:
        LL_WARNS() << "Unknown legacy code " << legacy << " [" << (S32)legacy << "]!" << LL_ENDL;
    }

    return pcode;
}

U8 LLPrimitive::pCodeToLegacy(const LLPCode pcode)
{
    U8 legacy;
    switch (pcode)
    {
/*
    case LL_PCODE_CUBE:
        legacy = BOX;
        break;
    case LL_PCODE_CYLINDER:
        legacy = CYLINDER;
        break;
    case LL_PCODE_CONE:
        legacy = CONE;
        break;
    case LL_PCODE_CONE_HEMI:
        legacy = HALF_CONE;
        break;
    case LL_PCODE_CYLINDER_HEMI:
        legacy = HALF_CYLINDER;
        break;
    case LL_PCODE_SPHERE_HEMI:
        legacy = HALF_SPHERE;
        break;
    case LL_PCODE_PRISM:
        legacy = PRISM;
        break;
    case LL_PCODE_PYRAMID:
        legacy = PYRAMID;
        break;
    case LL_PCODE_SPHERE:
        legacy = SPHERE;
        break;
    case LL_PCODE_TETRAHEDRON:
        legacy = TETRAHEDRON;
        break;
    case LL_PCODE_LEGACY_ATOR:
        legacy = ATOR;
        break;
    case LL_PCODE_LEGACY_SHOT:
        legacy = BASIC_SHOT;
        break;
    case LL_PCODE_LEGACY_SHOT_BIG:
        legacy = BIG_SHOT;
        break;
    case LL_PCODE_LEGACY_BIRD:
        legacy = BIRD;
        break;
    case LL_PCODE_LEGACY_DEMON:
        legacy = DEMON;
        break;
    case LL_PCODE_LEGACY_LSL_TEST:
        legacy = LSL_TEST;
        break;
    case LL_PCODE_LEGACY_ORACLE:
        legacy = ORACLE;
        break;
    case LL_PCODE_LEGACY_ROCK:
        legacy = ROCK;
        break;
    case LL_PCODE_LEGACY_TEXT_BUBBLE:
        legacy = TEXTBUBBLE;
        break;
    case LL_PCODE_LEGACY_SMOKE:
        legacy = SMOKE;
        break;
    case LL_PCODE_LEGACY_SPARK:
        legacy = SPARK;
        break;
*/
    case LL_PCODE_VOLUME:
        legacy = PRIMITIVE_VOLUME;
        break;
    case LL_PCODE_LEGACY_GRASS:
        legacy = GRASS;
        break;
    case LL_PCODE_LEGACY_PART_SYS:
        legacy = PART_SYS;
        break;
    case LL_PCODE_LEGACY_AVATAR:
        legacy = PLAYER;
        break;
    case LL_PCODE_LEGACY_TREE:
        legacy = TREE;
        break;
    case LL_PCODE_TREE_NEW:
        legacy = TREE_NEW;
        break;
    default:
        LL_WARNS() << "Unknown pcode " << (S32)pcode << ":" << pcode << "!" << LL_ENDL;
        return 0;
    }
    return legacy;
}


// static
// Don't crash or LL_ERRS() here!  This function is used for debug strings.
std::string LLPrimitive::pCodeToString(const LLPCode pcode)
{
    std::string pcode_string;

    U8 base_code = pcode & LL_PCODE_BASE_MASK;
    if (!pcode)
    {
        pcode_string = "null";
    }
    else if ((base_code) == LL_PCODE_LEGACY)
    {
        // It's a legacy object
        switch (pcode)
        {
        case LL_PCODE_LEGACY_GRASS:
          pcode_string = "grass";
            break;
        case LL_PCODE_LEGACY_PART_SYS:
          pcode_string = "particle system";
            break;
        case LL_PCODE_LEGACY_AVATAR:
          pcode_string = "avatar";
            break;
        case LL_PCODE_LEGACY_TEXT_BUBBLE:
          pcode_string = "text bubble";
            break;
        case LL_PCODE_LEGACY_TREE:
          pcode_string = "tree";
            break;
        case LL_PCODE_TREE_NEW:
          pcode_string = "tree_new";
            break;
        default:
          pcode_string = llformat( "unknown legacy pcode %i",(U32)pcode);
        }
    }
    else
    {
        std::string shape;
        std::string mask;
        if (base_code == LL_PCODE_CUBE)
        {
            shape = "cube";
        }
        else if (base_code == LL_PCODE_CYLINDER)
        {
            shape = "cylinder";
        }
        else if (base_code == LL_PCODE_CONE)
        {
            shape = "cone";
        }
        else if (base_code == LL_PCODE_PRISM)
        {
            shape = "prism";
        }
        else if (base_code == LL_PCODE_PYRAMID)
        {
            shape = "pyramid";
        }
        else if (base_code == LL_PCODE_SPHERE)
        {
            shape = "sphere";
        }
        else if (base_code == LL_PCODE_TETRAHEDRON)
        {
            shape = "tetrahedron";
        }
        else if (base_code == LL_PCODE_VOLUME)
        {
            shape = "volume";
        }
        else if (base_code == LL_PCODE_APP)
        {
            shape = "app";
        }
        else
        {
            LL_WARNS() << "Unknown base mask for pcode: " << base_code << LL_ENDL;
        }

        U8 mask_code = pcode & (~LL_PCODE_BASE_MASK);
        if (base_code == LL_PCODE_APP)
        {
            mask = llformat( "%x", mask_code);
        }
        else if (mask_code & LL_PCODE_HEMI_MASK)
        {
            mask = "hemi";
        }
        else
        {
            mask = llformat( "%x", mask_code);
        }

        if (mask[0])
        {
            pcode_string = llformat( "%s-%s", shape.c_str(), mask.c_str());
        }
        else
        {
            pcode_string = llformat( "%s", shape.c_str());
        }
    }

    return pcode_string;
}


void LLPrimitive::copyTEs(const LLPrimitive *primitivep)
{
    U32 i;
    if (primitivep->getExpectedNumTEs() != getExpectedNumTEs())
    {
        LL_WARNS() << "Primitives don't have same expected number of TE's" << LL_ENDL;
    }
    U32 num_tes = llmin(primitivep->getExpectedNumTEs(), getExpectedNumTEs());
    if (mTextureList.size() < getExpectedNumTEs())
    {
        mTextureList.setSize(getExpectedNumTEs());
    }
    for (i = 0; i < num_tes; i++)
    {
        mTextureList.copyTexture(i, *(primitivep->getTE(i)));
    }
}

S32 face_index_from_id(LLFaceID face_ID, const std::vector<LLProfile::Face>& faceArray)
{
    S32 i;
    for (i = 0; i < (S32)faceArray.size(); i++)
    {
        if (faceArray[i].mFaceID == face_ID)
        {
            return i;
        }
    }
    return -1;
}

bool LLPrimitive::setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume)
{
    if (NO_LOD == detail)
    {
        // build the new object
        setChanged(GEOMETRY);
        sVolumeManager->unrefVolume(mVolumep);
        mVolumep = new LLVolume(volume_params, 1, true, true);
        setNumTEs(mVolumep->getNumFaces());
        return false;
    }

    LLVolume *volumep;
    if (unique_volume)
    {
        F32 volume_detail = LLVolumeLODGroup::getVolumeScaleFromDetail(detail);
        if (mVolumep.notNull() && volume_params == mVolumep->getParams() && (volume_detail == mVolumep->getDetail()))
        {
            return false;
        }
        volumep = new LLVolume(volume_params, volume_detail, false, true);
    }
    else
    {
        if (mVolumep.notNull())
        {
            F32 volume_detail = LLVolumeLODGroup::getVolumeScaleFromDetail(detail);
            if (volume_params == mVolumep->getParams() && (volume_detail == mVolumep->getDetail()))
            {
                return false;
            }
        }

        volumep = sVolumeManager->refVolume(volume_params, detail);
        if (volumep == mVolumep)
        {
            sVolumeManager->unrefVolume( volumep );  // LLVolumeMgr::refVolume() creates a reference, but we don't need a second one.
            return true;
        }
    }

    setChanged(GEOMETRY);


    if (!mVolumep)
    {
        mVolumep = volumep;
        //mFaceMask = mVolumep->generateFaceMask();
        setNumTEs(mVolumep->getNumFaces());
        return true;
    }

#if 0
    // #if 0'd out by davep
    // this is a lot of cruft to set texture entry values that just stay the same for LOD switch
    // or immediately get overridden by an object update message, also crashes occasionally
    U32 old_face_mask = mVolumep->mFaceMask;

    S32 face_bit = 0;
    S32 cur_mask = 0;

    // Grab copies of the old faces from the original shape, ordered by type.
    // We will use these to figure out what old texture info gets mapped to new
    // faces in the new shape.
    std::vector<LLProfile::Face> old_faces;
    for (S32 face = 0; face < mVolumep->getNumFaces(); face++)
    {
        old_faces.push_back(mVolumep->getProfile().mFaces[face]);
    }

    // Copy the old texture info off to the side, but not in the order in which
    // they live in the mTextureList, rather in order of ther "face id" which
    // is the corresponding value of LLVolueParams::LLProfile::mFaces::mIndex.
    //
    // Hence, some elements of old_tes::mEntryList will be invalid.  It is
    // initialized to a size of 9 (max number of possible faces on a volume?)
    // and only the ones with valid types are filled in.
    LLPrimTextureList old_tes;
    old_tes.setSize(9);
    for (face_bit = 0; face_bit < 9; face_bit++)
    {
        cur_mask = 0x1 << face_bit;
        if (old_face_mask & cur_mask)
        {
            S32 te_index = face_index_from_id(cur_mask, old_faces);
            old_tes.copyTexture(face_bit, *(getTE(te_index)));
            //LL_INFOS() << face_bit << ":" << te_index << ":" << old_tes[face_bit].getID() << LL_ENDL;
        }
    }


    // build the new object
    sVolumeManager->unrefVolume(mVolumep);
    mVolumep = volumep;

    U32 new_face_mask = mVolumep->mFaceMask;
    S32 i;

    if (old_face_mask == new_face_mask)
    {
        // nothing to do
        return true;
    }

    if (mVolumep->getNumFaces() == 0 && new_face_mask != 0)
    {
        LL_WARNS() << "Object with 0 faces found...INCORRECT!" << LL_ENDL;
        setNumTEs(mVolumep->getNumFaces());
        return true;
    }

    // initialize face_mapping
    S32 face_mapping[9];
    for (face_bit = 0; face_bit < 9; face_bit++)
    {
        face_mapping[face_bit] = face_bit;
    }

    // The new shape may have more faces than the original, but we can't just
    // add them to the end -- the ordering matters and it may be that we must
    // insert the new faces in the middle of the list.  When we add a face it
    // will pick up the texture/color info of one of the old faces an so we
    // now figure out which old face info gets mapped to each new face, and
    // store in the face_mapping lookup table.
    for (face_bit = 0; face_bit < 9; face_bit++)
    {
        cur_mask = 0x1 << face_bit;
        if (!(new_face_mask & cur_mask))
        {
            // Face doesn't exist in new map.
            face_mapping[face_bit] = -1;
            continue;
        }
        else if (old_face_mask & cur_mask)
        {
            // Face exists in new and old map.
            face_mapping[face_bit] = face_bit;
            continue;
        }

        // OK, how we've got a mismatch, where we have to fill a new face with one from
        // the old face.
        if (cur_mask & (LL_FACE_PATH_BEGIN | LL_FACE_PATH_END | LL_FACE_INNER_SIDE))
        {
            // It's a top/bottom/hollow interior face.
            if (old_face_mask & LL_FACE_PATH_END)
            {
                face_mapping[face_bit] = 1;
                continue;
            }
            else
            {
                S32 cur_outer_mask = LL_FACE_OUTER_SIDE_0;
                for (i = 0; i < 4; i++)
                {
                    if (old_face_mask & cur_outer_mask)
                    {
                        face_mapping[face_bit] = 5 + i;
                        break;
                    }
                    cur_outer_mask <<= 1;
                }
                if (i == 4)
                {
                    LL_WARNS() << "No path end or outer face in volume!" << LL_ENDL;
                }
                continue;
            }
        }

        if (cur_mask & (LL_FACE_PROFILE_BEGIN | LL_FACE_PROFILE_END))
        {
            // A cut slice.  Use the hollow interior if we have it.
            if (old_face_mask & LL_FACE_INNER_SIDE)
            {
                face_mapping[face_bit] = 2;
                continue;
            }

            // No interior, use the bottom face.
            // Could figure out which of the outer faces was nearest, but that would be harder.
            if (old_face_mask & LL_FACE_PATH_END)
            {
                face_mapping[face_bit] = 1;
                continue;
            }
            else
            {
                S32 cur_outer_mask = LL_FACE_OUTER_SIDE_0;
                for (i = 0; i < 4; i++)
                {
                    if (old_face_mask & cur_outer_mask)
                    {
                        face_mapping[face_bit] = 5 + i;
                        break;
                    }
                    cur_outer_mask <<= 1;
                }
                if (i == 4)
                {
                    LL_WARNS() << "No path end or outer face in volume!" << LL_ENDL;
                }
                continue;
            }
        }

        // OK, the face that's missing is an outer face...
        // Pull from the nearest adjacent outer face (there's always guaranteed to be one...
        S32 cur_outer = face_bit - 5;
        S32 min_dist = 5;
        S32 min_outer_bit = -1;
        S32 i;
        for (i = 0; i < 4; i++)
        {
            if (old_face_mask & (LL_FACE_OUTER_SIDE_0 << i))
            {
                S32 dist = abs(i - cur_outer);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    min_outer_bit = i + 5;
                }
            }
        }
        if (-1 == min_outer_bit)
        {
            LL_INFOS() << (LLVolume *)mVolumep << LL_ENDL;
            LL_WARNS() << "Bad!  No outer faces, impossible!" << LL_ENDL;
        }
        face_mapping[face_bit] = min_outer_bit;
    }


    setNumTEs(mVolumep->getNumFaces());
    for (face_bit = 0; face_bit < 9; face_bit++)
    {
        // For each possible face type on the new shape we check to see if that
        // face exists and if it does we create a texture entry that is a copy
        // of one of the originals.  Since the originals might not have a
        // matching face, we use the face_mapping lookup table to figure out
        // which face information to copy.
        cur_mask = 0x1 << face_bit;
        if (new_face_mask & cur_mask)
        {
            if (-1 == face_mapping[face_bit])
            {
                LL_WARNS() << "No mapping from old face to new face!" << LL_ENDL;
            }

            S32 te_num = face_index_from_id(cur_mask, mVolumep->getProfile().mFaces);
            setTE(te_num, *(old_tes.getTexture(face_mapping[face_bit])));
        }
    }
#else
    // build the new object
    sVolumeManager->unrefVolume(mVolumep);
    mVolumep = volumep;

    setNumTEs(mVolumep->getNumFaces());
#endif
    return true;
}

bool LLPrimitive::setMaterial(U8 material)
{
    if (material != mMaterial)
    {
        mMaterial = material;
        return true;
    }
    else
    {
        return false;
    }
}

S32 LLPrimitive::packTEField(U8 *cur_ptr, U8 *data_ptr, U8 data_size, U8 last_face_index, EMsgVariableType type) const
{
    S32 face_index;
    S32 i;
    U64 exception_faces;
    U8 *start_loc = cur_ptr;

    htolememcpy(cur_ptr,data_ptr + (last_face_index * data_size), type, data_size);
    cur_ptr += data_size;

    for (face_index = last_face_index-1; face_index >= 0; face_index--)
    {
        bool already_sent = false;
        for (i = face_index+1; i <= last_face_index; i++)
        {
            if (!memcmp(data_ptr+(data_size *face_index), data_ptr+(data_size *i), data_size))
            {
                already_sent = true;
                break;
            }
        }

        if (!already_sent)
        {
            exception_faces = 0;
            for (i = face_index; i >= 0; i--)
            {
                if (!memcmp(data_ptr+(data_size *face_index), data_ptr+(data_size *i), data_size))
                {
                    exception_faces |= ((U64)1 << i);
                }
            }

            //assign exception faces to cur_ptr
            if (exception_faces >= ((U64)0x1 << 7))
            {
                if (exception_faces >= ((U64)0x1 << 14))
                {
                    if (exception_faces >= ((U64)0x1 << 21))
                    {
                        if (exception_faces >= ((U64)0x1 << 28))
                        {
                            if (exception_faces >= ((U64)0x1 << 35))
                            {
                                if (exception_faces >= ((U64)0x1 << 42))
                                {
                                    if (exception_faces >= ((U64)0x1 << 49))
                                    {
                                        *cur_ptr++ = (U8)(((exception_faces >> 49) & 0x7F) | 0x80);
                                    }
                                    *cur_ptr++ = (U8)(((exception_faces >> 42) & 0x7F) | 0x80);
                                }
                                *cur_ptr++ = (U8)(((exception_faces >> 35) & 0x7F) | 0x80);
                            }
                            *cur_ptr++ = (U8)(((exception_faces >> 28) & 0x7F) | 0x80);
                        }
                        *cur_ptr++ = (U8)(((exception_faces >> 21) & 0x7F) | 0x80);
                    }
                    *cur_ptr++ = (U8)(((exception_faces >> 14) & 0x7F) | 0x80);
                }
                *cur_ptr++ = (U8)(((exception_faces >> 7) & 0x7F) | 0x80);
            }


            *cur_ptr++ = (U8)(exception_faces & 0x7F);

            htolememcpy(cur_ptr,data_ptr + (face_index * data_size), type, data_size);
            cur_ptr += data_size;
        }
    }
    return (S32)(cur_ptr - start_loc);
}

namespace
{
    template< typename T >
    bool unpack_TEField(T dest[], U8 dest_count, U8 * &source, U8 *source_end, EMsgVariableType type)
    {
        const size_t size(sizeof(T));

        if ((source + size + 1) > source_end)
        {
            // we add 1 above to take into account the byte that we know must follow the value.
            LL_WARNS("TEXTUREENTRY") << "Buffer exhausted! Requires " << size << " + 1 bytes for default, " << (source_end - source) << " bytes remaning." << LL_ENDL;
            source = source_end;
            return false;
        }

        // Extract the default value and fill the array.
        htolememcpy(dest, source, type, size);
        source += size;
        for (S32 idx = 1; idx < dest_count; ++idx)
        {
            dest[idx] = dest[0];
        }

        while (source < source_end)
        {
            U64 index_flags(0);
            U8  sbit(0);

            // Unpack the variable length bitfield. Each bit represents whether the following
            // value will be placed at the corresponding array index.
            do
            {
                if (source >= source_end)
                {
                    LL_WARNS("TEXTUREENTRY") << "Buffer exhausted! Reading index flags." << LL_ENDL;
                    source = source_end;
                    return false;
                }

                sbit = *source++;
                index_flags <<= 7;    // original code had this after?
                index_flags |= (sbit & 0x7F);
            } while (sbit & 0x80);

            if (!index_flags)
            {   // We've hit the terminating 0 byte.
                break;
            }

            if ((source + size + 1) > source_end)
            {
                // we add 1 above to take into account the byte that we know must follow the value.
                LL_WARNS("TEXTUREENTRY") << "Buffer exhausted! Requires " << size << " + 1 bytes for default, " << (source_end - source) << " bytes remaning." << LL_ENDL;
                source = source_end;
                return false;
            }

            // get the value for the indexs.
            T value;
            htolememcpy(&value, source, type, size);
            source += size;

            for (S32 idx = 0; idx < dest_count; idx++)
            {
                if (index_flags & 1ULL << idx)
                {
                    dest[idx] = value;
                }
            }

        }
        return true;
    }
}

S32 LLPrimitive::packTEMessageBuffer(U8* packed_buffer) const
{
    S32 bytes_packed = 0;
    U32 num_tes = llmin((U32) getNumTEs(), (U32) LLTEContents::MAX_TES);
    if (num_tes == 0)
    {
        return bytes_packed;
    }

    LLTEContents contents(num_tes);

    // note: packed_buffer is expected to be at least MAX_TE_BUFFER wide.
    //U8  packed_buffer[MAX_TE_BUFFER];
    U8 *cur_ptr = packed_buffer;

    // ...if we hit the front, send one image id
    S8 face_index;
    const LLColor4U white(255, 255, 255, 255);
    LLColor4U coloru;
    S32 last_face_index = num_tes- 1;
    for (face_index = 0; face_index <= last_face_index; ++face_index)
    {
        // Directly sending image_ids is not safe!
        //memcpy(contents.image_ids[face_index].mData, getTE(face_index)->getID().mData, sizeof(LLUUID));  /* Flawfinder: ignore */
        const LLTextureEntry* te = getTE(face_index);
        //contents.image_ids[face_index] = getTE(face_index)->getID();
        contents.image_ids[face_index] = te->getID();

        // Directly sending material_ids is not safe!
        //memcpy(&contents.material_ids[face_index].get(), getTE(face_index)->getMaterialID().get(), 16);  /* Flawfinder: ignore */
        contents.material_ids[face_index] = te->getMaterialID();

        // Cast LLColor4 to LLColor4U
        coloru.setVec( te->getColor() );

        // Note:  This is an optimization to send common colors (1.f, 1.f, 1.f, 1.f)
        // as all zeros.  However, the subtraction and addition must be done in unsigned
        // byte space, not in float space, otherwise off-by-one errors occur. JC
        contents.colors[face_index] = white - coloru;

        contents.scale_s[face_index] = te->mScaleS;
        contents.scale_t[face_index] = te->mScaleT;
        contents.offset_s[face_index] = (S16) ll_round((llclamp(te->mOffsetS,-1.0f,1.0f) * (F32)0x7FFF)) ;
        contents.offset_t[face_index] = (S16) ll_round((llclamp(te->mOffsetT,-1.0f,1.0f) * (F32)0x7FFF)) ;
        contents.rot[face_index] = (S16) ll_round(((fmod(te->mRotation, F_TWO_PI)/F_TWO_PI) * TEXTURE_ROTATION_PACK_FACTOR));
        contents.bump[face_index] = te->getBumpShinyFullbright();
        contents.media_flags[face_index] = te->getMediaTexGen();
        contents.glow[face_index] = (U8) ll_round((llclamp(te->getGlow(), 0.0f, 1.0f) * (F32)0xFF));
        contents.alpha_gamma[face_index]  = te->getAlphaGamma();
    }

    // pack required properties
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.image_ids), sizeof(LLUUID), last_face_index, MVT_LLUUID);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.colors), 4, last_face_index, MVT_U8);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.scale_s), 4, last_face_index, MVT_F32);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.scale_t), 4, last_face_index, MVT_F32);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.offset_s), 2, last_face_index, MVT_S16Array);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.offset_t), 2, last_face_index, MVT_S16Array);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.rot), 2, last_face_index, MVT_S16Array);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, contents.bump, 1, last_face_index, MVT_U8);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, contents.media_flags, 1, last_face_index, MVT_U8);
    *cur_ptr++ = 0;
    cur_ptr += packTEField(cur_ptr, contents.glow, 1, last_face_index, MVT_U8);
    *cur_ptr++ = 0;
    // end of required properties

    // we always pack material_ids however the receiving side can handle the case when it is not included
    // TODO?: don't bother to pack it when it is empty/default?
    cur_ptr += packTEField(cur_ptr, reinterpret_cast<U8*>(contents.material_ids), 16, last_face_index, MVT_LLUUID);
    *cur_ptr++ = 0;

    // pack extra properties
    *cur_ptr++ = EXTRA_PROPERTY_ALPHA_GAMMA; // indicator alpha_gamma is present
    cur_ptr += packTEField(cur_ptr, contents.alpha_gamma, 1, last_face_index, MVT_U8);
    // Note: last message is NOT null terminated when on the wire!

    bytes_packed = (S32)(cur_ptr - packed_buffer);
    return bytes_packed;
}

// Pack information about all texture entries into container:
// { TextureEntry Variable 2 }
// Includes information about image ID, color, scale S,T, offset S,T and rotation
bool LLPrimitive::packTEMessage(LLMessageSystem *mesgsys) const
{
    U8 packed_buffer[MAX_TE_BUFFER];
    S32 num_bytes = packTEMessageBuffer(packed_buffer);
    mesgsys->addBinaryDataFast(_PREHASH_TextureEntry, packed_buffer, num_bytes);
    return true;
}

S32 LLPrimitive::parseTEMessage(LLMessageSystem* mesgsys, char const* block_name, const S32 block_num, LLTEContents& tec)
{
    U32 data_size;
    S32 retval = 0;

    if (block_num < 0)
    {
        data_size = mesgsys->getSizeFast(block_name, _PREHASH_TextureEntry);
    }
    else
    {
        data_size = mesgsys->getSizeFast(block_name, block_num, _PREHASH_TextureEntry);
    }

    if (data_size == 0)
    {
        return retval;
    }
    else if (data_size >= MAX_TE_BUFFER)
    {
        LL_WARNS("TEXTUREENTRY") << "Excessive buffer size detected in Texture Entry! Truncating." << LL_ENDL;
        data_size = MAX_TE_BUFFER - 1;
    }

    U8 packed_buffer[MAX_TE_BUFFER];

    // if block_num < 0 ask for block 0
    mesgsys->getBinaryDataFast(block_name, _PREHASH_TextureEntry, packed_buffer, 0, std::max(block_num, 0), MAX_TE_BUFFER - 1);

    // The last field is not zero terminated.
    // Rather than special case the upack functions.  Just make it 0x00 terminated.
    packed_buffer[data_size] = 0x00;
    ++data_size;

    return parseTEMessage(packed_buffer, data_size, tec);
}

// static
S32 LLPrimitive::parseTEMessage(U8* packed_buffer, U32 data_size, LLTEContents& tec)
{
    // Note: the last TEFeild is not zero-terminated on the wire but we expect it to be for unpacking.
    // This to avoid special-casing the logic in unpack_TEField<>().
    // The external context is required to null-terminate packed_buffer and increment data_size accordingly.
    llassert(data_size > 0);
    llassert(packed_buffer[data_size - 1] == 0);

    S32 retval = 0;

    U8 *cur_ptr = packed_buffer;
    LL_DEBUGS("TEXTUREENTRY") << "Texture Entry with buffere sized: " << data_size << LL_ENDL;
    U8 *buffer_end = packed_buffer + data_size;

    S32 num_textures = tec.getNumTEs();
    if (!(  unpack_TEField<LLUUID>(tec.image_ids, num_textures, cur_ptr, buffer_end, MVT_LLUUID) &&
            unpack_TEField<LLColor4U>(tec.colors, num_textures, cur_ptr, buffer_end, MVT_U8) &&
            unpack_TEField<F32>(tec.scale_s, num_textures, cur_ptr, buffer_end, MVT_F32) &&
            unpack_TEField<F32>(tec.scale_t, num_textures, cur_ptr, buffer_end, MVT_F32) &&
            unpack_TEField<S16>(tec.offset_s, num_textures, cur_ptr, buffer_end, MVT_S16) &&
            unpack_TEField<S16>(tec.offset_t, num_textures, cur_ptr, buffer_end, MVT_S16) &&
            unpack_TEField<S16>(tec.rot, num_textures, cur_ptr, buffer_end, MVT_S16) &&
            unpack_TEField<U8>(tec.bump, num_textures, cur_ptr, buffer_end, MVT_U8) &&
            unpack_TEField<U8>(tec.media_flags, num_textures, cur_ptr, buffer_end, MVT_U8) &&
            unpack_TEField<U8>(tec.glow, num_textures, cur_ptr, buffer_end, MVT_U8)))
    {
        LL_WARNS("TEXTUREENTRY") << "Failure parsing Texture Entry Message due to malformed TE Field! Dropping changes on the floor. " << LL_ENDL;
        return 0;
    }

    if (cur_ptr < buffer_end)
    {
        if (!unpack_TEField<LLMaterialID>(tec.material_ids, num_textures, cur_ptr, buffer_end, MVT_LLUUID))
        {
            LL_INFOS("TEXTUREENTRY") << "Fail parse material_ids." << LL_ENDL;
            // material_ids are optional --> we don't consider this a failure
        }
    }

    // alpha_gamma is optional and has an indicator byte in front
    if (cur_ptr < buffer_end && *cur_ptr == EXTRA_PROPERTY_ALPHA_GAMMA)
    {
        ++cur_ptr; // skip the indicator
        if (!unpack_TEField<U8>(tec.alpha_gamma, num_textures, cur_ptr, buffer_end, MVT_U8))
        {
            LL_INFOS("TEXTUREENTRY") << "Fail parse AlphaGamma TEField." << LL_ENDL;
            // alpha_gamma is optional --> we don't consider this a failure
        }
    }

    // undo the zero-encode color optimization
    const LLColor4U white(255, 255, 255, 255);
    for (U8 i = 0; i < num_textures; ++i)
    {
        tec.colors[i] = white - tec.colors[i];
    }

    retval = 1;
    return retval;
}

S32 LLPrimitive::applyParsedTEMessage(const LLTEContents& tec)
{
    S32 retval = 0;
    for (U32 i = 0; i < tec.getNumTEs(); i++)
    {
        retval |= setTETexture(i, tec.image_ids[i]);
        retval |= setTEColor(i, LLColor4(tec.colors[i])); // already corrected for zero-encode optimization
        retval |= setTEMaterialID(i, tec.material_ids[i]);
        retval |= setTEScale(i, tec.scale_s[i], tec.scale_t[i]);
        retval |= setTEOffset(i, (F32)tec.offset_s[i] / (F32)0x7FFF, (F32) tec.offset_t[i] / (F32) 0x7FFF);
        retval |= setTERotation(i, ((F32)tec.rot[i] / TEXTURE_ROTATION_PACK_FACTOR) * F_TWO_PI);
        retval |= setTEBumpShinyFullbright(i, tec.bump[i]);
        retval |= setTEMediaTexGen(i, tec.media_flags[i]);
        retval |= setTEGlow(i, (F32)tec.glow[i] / (F32)0xFF);
        retval |= setTEAlphaGamma(i, tec.alpha_gamma[i]);
    }
    return retval;
}

S32 LLPrimitive::unpackTEMessage(LLMessageSystem* mesgsys, char const* block_name, const S32 block_num)
{
    LLTEContents tec(getNumTEs());
    S32 retval = parseTEMessage(mesgsys, block_name, block_num, tec);
    if (!retval)
        return retval;
    return applyParsedTEMessage(tec);
}

S32 LLPrimitive::unpackTEMessage(LLDataPacker &dp)
{
    // use a negative block_num to indicate a single-block read (a non-variable block)
    S32 retval = 0;
    constexpr U32 MAX_TES = 45;

    // Avoid construction of 32 UUIDs per call
    static LLMaterialID material_ids[MAX_TES];

    U8 packed_buffer[MAX_TE_BUFFER];
    memset((void*)packed_buffer, 0, MAX_TE_BUFFER);

    S32 data_size;
    if (!dp.unpackBinaryData(packed_buffer, data_size, "TextureEntry"))
    {
        retval = TEM_INVALID;
        LL_WARNS() << "Bad texture entry block!  Abort!" << LL_ENDL;
        return retval;
    }

    if (data_size == 0)
    {
        return retval;
    }
    else if (data_size >= MAX_TE_BUFFER)
    {
        LL_WARNS("TEXTUREENTRY") << "Excessive buffer size detected in Texture Entry! Truncating." << LL_ENDL;
        data_size = MAX_TE_BUFFER - 1;
    }

    // The last field is not zero terminated.
    // Rather than special case the upack functions.  Just make it 0x00 terminated.
    packed_buffer[data_size] = 0x00;
    ++data_size;

    LLTEContents tec(getNumTEs());
    parseTEMessage(packed_buffer, data_size, tec);
    return applyParsedTEMessage(tec);
}

U8  LLPrimitive::getExpectedNumTEs() const
{
    U8 expected_face_count = 0;
    if (mVolumep)
    {
        expected_face_count = mVolumep->getNumFaces();
    }
    return expected_face_count;
}

void LLPrimitive::copyTextureList(const LLPrimTextureList& other_list)
{
    mTextureList.copy(other_list);
}

void LLPrimitive::takeTextureList(LLPrimTextureList& other_list)
{
    mTextureList.take(other_list);
}

void LLPrimitive::updateNumBumpmap(const U8 index, const U8 bump)
{
    LLTextureEntry* te = getTE(index);
    if(!te)
    {
        return;
    }

    U8 old_bump = te->getBumpmap();
    if(old_bump > 0)
    {
        mNumBumpmapTEs--;
    }
    if((bump & TEM_BUMP_MASK) > 0)
    {
        mNumBumpmapTEs++;
    }

    return;
}
//============================================================================

// Moved from llselectmgr.cpp
// BUG: Only works for boxes.
// Face numbering for flex boxes as of 1.14.2

// static
bool LLPrimitive::getTESTAxes(const U8 face, U32* s_axis, U32* t_axis)
{
    if (face == 0)
    {
        *s_axis = VX; *t_axis = VY;
        return true;
    }
    else if (face == 1)
    {
        *s_axis = VX; *t_axis = VZ;
        return true;
    }
    else if (face == 2)
    {
        *s_axis = VY; *t_axis = VZ;
        return true;
    }
    else if (face == 3)
    {
        *s_axis = VX; *t_axis = VZ;
        return true;
    }
    else if (face == 4)
    {
        *s_axis = VY; *t_axis = VZ;
        return true;
    }
    else if (face >= 5)
    {
        *s_axis = VX; *t_axis = VY;
        return true;
    }
    else
    {
        // unknown face
        return false;
    }
}

//============================================================================

//static
bool LLNetworkData::isValid(U16 param_type, U32 size)
{
    // ew - better mechanism needed

    switch(param_type)
    {
    case PARAMS_FLEXIBLE:
        return (size == 16);
    case PARAMS_LIGHT:
        return (size == 16);
    case PARAMS_SCULPT:
        return (size == 17);
    case PARAMS_LIGHT_IMAGE:
        return (size == 28);
    case PARAMS_EXTENDED_MESH:
        return (size == 4);
    case PARAMS_RENDER_MATERIAL:
        return (size > 1);
    case PARAMS_REFLECTION_PROBE:
        return (size == 9);
    }

    return false;
}

//============================================================================

LLLightParams::LLLightParams()
{
    mColor.setToWhite();
    mRadius = 10.f;
    mCutoff = 0.0f;
    mFalloff = 0.75f;

    mType = PARAMS_LIGHT;
}

bool LLLightParams::pack(LLDataPacker &dp) const
{
    LLColor4U color4u(mColor);
    dp.packColor4U(color4u, "color");
    dp.packF32(mRadius, "radius");
    dp.packF32(mCutoff, "cutoff");
    dp.packF32(mFalloff, "falloff");
    return true;
}

bool LLLightParams::unpack(LLDataPacker &dp)
{
    LLColor4U color;
    dp.unpackColor4U(color, "color");
    setLinearColor(LLColor4(color));

    F32 radius;
    dp.unpackF32(radius, "radius");
    setRadius(radius);

    F32 cutoff;
    dp.unpackF32(cutoff, "cutoff");
    setCutoff(cutoff);

    F32 falloff;
    dp.unpackF32(falloff, "falloff");
    setFalloff(falloff);

    return true;
}

bool LLLightParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_LIGHT)
    {
        return false;
    }
    const LLLightParams *param = (const LLLightParams*)&data;
    if (param->mColor != mColor ||
        param->mRadius != mRadius ||
        param->mCutoff != mCutoff ||
        param->mFalloff != mFalloff)
    {
        return false;
    }
    return true;
}

void LLLightParams::copy(const LLNetworkData& data)
{
    const LLLightParams *param = (LLLightParams*)&data;
    mType = param->mType;
    mColor = param->mColor;
    mRadius = param->mRadius;
    mCutoff = param->mCutoff;
    mFalloff = param->mFalloff;
}

LLSD LLLightParams::asLLSD() const
{
    LLSD sd;

    sd["color"] = ll_sd_from_color4(getLinearColor());
    sd["radius"] = getRadius();
    sd["falloff"] = getFalloff();
    sd["cutoff"] = getCutoff();

    return sd;
}

bool LLLightParams::fromLLSD(LLSD& sd)
{
    const char *w;
    w = "color";
    if (sd.has(w))
    {
        setLinearColor( ll_color4_from_sd(sd["color"]) );
    } else goto fail;
    w = "radius";
    if (sd.has(w))
    {
        setRadius( (F32)sd[w].asReal() );
    } else goto fail;
    w = "falloff";
    if (sd.has(w))
    {
        setFalloff( (F32)sd[w].asReal() );
    } else goto fail;
    w = "cutoff";
    if (sd.has(w))
    {
        setCutoff( (F32)sd[w].asReal() );
    } else goto fail;

    return true;
 fail:
    return false;
}

//============================================================================

//============================================================================

LLReflectionProbeParams::LLReflectionProbeParams()
{
    mType = PARAMS_REFLECTION_PROBE;
}

bool LLReflectionProbeParams::pack(LLDataPacker &dp) const
{
    dp.packF32(mAmbiance, "ambiance");
    dp.packF32(mClipDistance, "clip_distance");
    dp.packU8(mFlags, "flags");
    return true;
}

bool LLReflectionProbeParams::unpack(LLDataPacker &dp)
{
    F32 ambiance;
    F32 clip_distance;

    dp.unpackF32(ambiance, "ambiance");
    setAmbiance(ambiance);

    dp.unpackF32(clip_distance, "clip_distance");
    setClipDistance(clip_distance);

    dp.unpackU8(mFlags, "flags");

    return true;
}

bool LLReflectionProbeParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_REFLECTION_PROBE)
    {
        return false;
    }
    const LLReflectionProbeParams *param = (const LLReflectionProbeParams*)&data;
    if (param->mAmbiance != mAmbiance)
    {
        return false;
    }
    if (param->mClipDistance != mClipDistance)
    {
        return false;
    }
    if (param->mFlags != mFlags)
    {
        return false;
    }
    return true;
}

void LLReflectionProbeParams::copy(const LLNetworkData& data)
{
    const LLReflectionProbeParams *param = (LLReflectionProbeParams*)&data;
    mType = param->mType;
    mAmbiance = param->mAmbiance;
    mClipDistance = param->mClipDistance;
    mFlags = param->mFlags;
}

LLSD LLReflectionProbeParams::asLLSD() const
{
    LLSD sd;
    sd["ambiance"] = getAmbiance();
    sd["clip_distance"] = getClipDistance();
    sd["flags"] = mFlags;
    return sd;
}

bool LLReflectionProbeParams::fromLLSD(LLSD& sd)
{
    if (!sd.has("ambiance") ||
        !sd.has("clip_distance") ||
        !sd.has("flags"))
    {
        return false;
    }

    setAmbiance((F32)sd["ambiance"].asReal());
    setClipDistance((F32)sd["clip_distance"].asReal());
    mFlags = (U8) sd["flags"].asInteger();

    return true;
}

void LLReflectionProbeParams::setIsBox(bool is_box)
{
    if (is_box)
    {
        mFlags |= FLAG_BOX_VOLUME;
    }
    else
    {
        mFlags &= ~FLAG_BOX_VOLUME;
    }
}

void LLReflectionProbeParams::setIsDynamic(bool is_dynamic)
{
    if (is_dynamic)
    {
        mFlags |= FLAG_DYNAMIC;
    }
    else
    {
        mFlags &= ~FLAG_DYNAMIC;
    }
}


void LLReflectionProbeParams::setIsMirror(bool is_mirror)
{
    if (is_mirror)
    {
        mFlags |= FLAG_MIRROR;
    }
    else
    {
        mFlags &= ~FLAG_MIRROR;
    }
}

//============================================================================
LLFlexibleObjectData::LLFlexibleObjectData()
{
    mSimulateLOD                = FLEXIBLE_OBJECT_DEFAULT_NUM_SECTIONS;
    mGravity                    = FLEXIBLE_OBJECT_DEFAULT_GRAVITY;
    mAirFriction                = FLEXIBLE_OBJECT_DEFAULT_AIR_FRICTION;
    mWindSensitivity            = FLEXIBLE_OBJECT_DEFAULT_WIND_SENSITIVITY;
    mTension                    = FLEXIBLE_OBJECT_DEFAULT_TENSION;
    //mUsingCollisionSphere     = FLEXIBLE_OBJECT_DEFAULT_USING_COLLISION_SPHERE;
    //mRenderingCollisionSphere = FLEXIBLE_OBJECT_DEFAULT_RENDERING_COLLISION_SPHERE;
    mUserForce                  = LLVector3(0.f, 0.f, 0.f);

    mType = PARAMS_FLEXIBLE;
}

bool LLFlexibleObjectData::pack(LLDataPacker &dp) const
{
    // Custom, uber-svelte pack "softness" in upper bits of tension & drag
    U8 bit1 = (mSimulateLOD & 2) << 6;
    U8 bit2 = (mSimulateLOD & 1) << 7;
    dp.packU8((U8)(mTension*10.01f) + bit1, "tension");
    dp.packU8((U8)(mAirFriction*10.01f) + bit2, "drag");
    dp.packU8((U8)((mGravity+10.f)*10.01f), "gravity");
    dp.packU8((U8)(mWindSensitivity*10.01f), "wind");
    dp.packVector3(mUserForce, "userforce");
    return true;
}

bool LLFlexibleObjectData::unpack(LLDataPacker &dp)
{
    U8 tension, friction, gravity, wind;
    U8 bit1, bit2;
    dp.unpackU8(tension, "tension");    bit1 = (tension >> 6) & 2;
                                        mTension = ((F32)(tension&0x7f))/10.f;
    dp.unpackU8(friction, "drag");      bit2 = (friction >> 7) & 1;
                                        mAirFriction = ((F32)(friction&0x7f))/10.f;
                                        mSimulateLOD = bit1 | bit2;
    dp.unpackU8(gravity, "gravity");    mGravity = ((F32)gravity)/10.f - 10.f;
    dp.unpackU8(wind, "wind");          mWindSensitivity = ((F32)wind)/10.f;
    if (dp.hasNext())
    {
        dp.unpackVector3(mUserForce, "userforce");
    }
    else
    {
        mUserForce.setVec(0.f, 0.f, 0.f);
    }
    return true;
}

bool LLFlexibleObjectData::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_FLEXIBLE)
    {
        return false;
    }
    LLFlexibleObjectData *flex_data = (LLFlexibleObjectData*)&data;
    return (mSimulateLOD == flex_data->mSimulateLOD &&
            mGravity == flex_data->mGravity &&
            mAirFriction == flex_data->mAirFriction &&
            mWindSensitivity == flex_data->mWindSensitivity &&
            mTension == flex_data->mTension &&
            mUserForce == flex_data->mUserForce);
            //mUsingCollisionSphere == flex_data->mUsingCollisionSphere &&
            //mRenderingCollisionSphere == flex_data->mRenderingCollisionSphere
}

void LLFlexibleObjectData::copy(const LLNetworkData& data)
{
    const LLFlexibleObjectData *flex_data = (LLFlexibleObjectData*)&data;
    mSimulateLOD = flex_data->mSimulateLOD;
    mGravity = flex_data->mGravity;
    mAirFriction = flex_data->mAirFriction;
    mWindSensitivity = flex_data->mWindSensitivity;
    mTension = flex_data->mTension;
    mUserForce = flex_data->mUserForce;
    //mUsingCollisionSphere = flex_data->mUsingCollisionSphere;
    //mRenderingCollisionSphere = flex_data->mRenderingCollisionSphere;
}

LLSD LLFlexibleObjectData::asLLSD() const
{
    LLSD sd;

    sd["air_friction"] = getAirFriction();
    sd["gravity"] = getGravity();
    sd["simulate_lod"] = getSimulateLOD();
    sd["tension"] = getTension();
    sd["user_force"] = getUserForce().getValue();
    sd["wind_sensitivity"] = getWindSensitivity();

    return sd;
}

bool LLFlexibleObjectData::fromLLSD(LLSD& sd)
{
    const char *w;
    w = "air_friction";
    if (sd.has(w))
    {
        setAirFriction( (F32)sd[w].asReal() );
    } else goto fail;
    w = "gravity";
    if (sd.has(w))
    {
        setGravity( (F32)sd[w].asReal() );
    } else goto fail;
    w = "simulate_lod";
    if (sd.has(w))
    {
        setSimulateLOD( sd[w].asInteger() );
    } else goto fail;
    w = "tension";
    if (sd.has(w))
    {
        setTension( (F32)sd[w].asReal() );
    } else goto fail;
    w = "user_force";
    if (sd.has(w))
    {
        LLVector3 user_force = ll_vector3_from_sd(sd[w], 0);
        setUserForce( user_force );
    } else goto fail;
    w = "wind_sensitivity";
    if (sd.has(w))
    {
        setWindSensitivity( (F32)sd[w].asReal() );
    } else goto fail;

    return true;
 fail:
    return false;
}

//============================================================================

LLSculptParams::LLSculptParams()
{
    mType = PARAMS_SCULPT;
    mSculptTexture = SCULPT_DEFAULT_TEXTURE;
    mSculptType = LL_SCULPT_TYPE_SPHERE;
}

bool LLSculptParams::pack(LLDataPacker &dp) const
{
    dp.packUUID(mSculptTexture, "texture");
    dp.packU8(mSculptType, "type");

    return true;
}

bool LLSculptParams::unpack(LLDataPacker &dp)
{
    U8 type;
    LLUUID id;
    dp.unpackUUID(id, "texture");
    dp.unpackU8(type, "type");

    setSculptTexture(id, type);
    return true;
}

bool LLSculptParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_SCULPT)
    {
        return false;
    }

    const LLSculptParams *param = (const LLSculptParams*)&data;
    if ( (param->mSculptTexture != mSculptTexture) ||
         (param->mSculptType != mSculptType) )

    {
        return false;
    }

    return true;
}

void LLSculptParams::copy(const LLNetworkData& data)
{
    const LLSculptParams *param = (LLSculptParams*)&data;
    setSculptTexture(param->mSculptTexture, param->mSculptType);
}



LLSD LLSculptParams::asLLSD() const
{
    LLSD sd;

    sd["texture"] = mSculptTexture;
    sd["type"] = mSculptType;

    return sd;
}

bool LLSculptParams::fromLLSD(LLSD& sd)
{
    const char *w;
    U8 type;
    w = "type";
    if (sd.has(w))
    {
        type = sd[w].asInteger();
    }
    else return false;

    w = "texture";
    if (sd.has(w))
    {
        setSculptTexture(sd[w], type);
    }
    else return false;

    return true;
}

void LLSculptParams::setSculptTexture(const LLUUID& texture_id, U8 sculpt_type)
{
    U8 type = sculpt_type & LL_SCULPT_TYPE_MASK;
    U8 flags = sculpt_type & LL_SCULPT_FLAG_MASK;
    if (sculpt_type != (type | flags) || type > LL_SCULPT_TYPE_MAX)
    {
        mSculptTexture = SCULPT_DEFAULT_TEXTURE;
        mSculptType = LL_SCULPT_TYPE_SPHERE;
    }
    else
    {
        mSculptTexture = texture_id;
        mSculptType = sculpt_type;
    }
}

//============================================================================

LLLightImageParams::LLLightImageParams()
{
    mType = PARAMS_LIGHT_IMAGE;
    mParams.setVec(F_PI*0.5f, 0.f, 0.f);
}

bool LLLightImageParams::pack(LLDataPacker &dp) const
{
    dp.packUUID(mLightTexture, "texture");
    dp.packVector3(mParams, "params");

    return true;
}

bool LLLightImageParams::unpack(LLDataPacker &dp)
{
    dp.unpackUUID(mLightTexture, "texture");
    dp.unpackVector3(mParams, "params");

    return true;
}

bool LLLightImageParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_LIGHT_IMAGE)
    {
        return false;
    }

    const LLLightImageParams *param = (const LLLightImageParams*)&data;
    if ( (param->mLightTexture != mLightTexture) )
    {
        return false;
    }

    if ( (param->mParams != mParams ) )
    {
        return false;
    }

    return true;
}

void LLLightImageParams::copy(const LLNetworkData& data)
{
    const LLLightImageParams *param = (LLLightImageParams*)&data;
    mLightTexture = param->mLightTexture;
    mParams = param->mParams;
}



LLSD LLLightImageParams::asLLSD() const
{
    LLSD sd;

    sd["texture"] = mLightTexture;
    sd["params"] = mParams.getValue();

    return sd;
}

bool LLLightImageParams::fromLLSD(LLSD& sd)
{
    if (sd.has("texture"))
    {
        setLightTexture( sd["texture"] );
        setParams( LLVector3( sd["params"] ) );
        return true;
    }

    return false;
}

//============================================================================

LLExtendedMeshParams::LLExtendedMeshParams()
{
    mType = PARAMS_EXTENDED_MESH;
    mFlags = 0;
}

bool LLExtendedMeshParams::pack(LLDataPacker &dp) const
{
    dp.packU32(mFlags, "flags");

    return true;
}

bool LLExtendedMeshParams::unpack(LLDataPacker &dp)
{
    dp.unpackU32(mFlags, "flags");

    return true;
}

bool LLExtendedMeshParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_EXTENDED_MESH)
    {
        return false;
    }

    const LLExtendedMeshParams *param = (const LLExtendedMeshParams*)&data;
    if ( (param->mFlags != mFlags) )
    {
        return false;
    }

    return true;
}

void LLExtendedMeshParams::copy(const LLNetworkData& data)
{
    const LLExtendedMeshParams *param = (LLExtendedMeshParams*)&data;
    mFlags = param->mFlags;
}

LLSD LLExtendedMeshParams::asLLSD() const
{
    LLSD sd;

    sd["flags"] = LLSD::Integer(mFlags);

    return sd;
}

bool LLExtendedMeshParams::fromLLSD(LLSD& sd)
{
    if (sd.has("flags"))
    {
        setFlags( sd["flags"].asInteger());
        return true;
    }

    return false;
}

//============================================================================

LLRenderMaterialParams::LLRenderMaterialParams()
{
    mType = PARAMS_RENDER_MATERIAL;
}

bool LLRenderMaterialParams::pack(LLDataPacker& dp) const
{
    U8 count = (U8)llmin((S32)mEntries.size(), 14); //limited to 255 bytes, no more than 14 material ids

    dp.packU8(count, "count");
    for (auto& entry : mEntries)
    {
        dp.packU8(entry.te_idx, "te_idx");
        dp.packUUID(entry.id, "id");
    }

    return true;
}

bool LLRenderMaterialParams::unpack(LLDataPacker& dp)
{
    U8 count;
    dp.unpackU8(count, "count");
    mEntries.resize(count);
    for (auto& entry : mEntries)
    {
        dp.unpackU8(entry.te_idx, "te_idx");
        dp.unpackUUID(entry.id, "te_id");
    }

    return true;
}

bool LLRenderMaterialParams::operator==(const LLNetworkData& data) const
{
    if (data.mType != PARAMS_RENDER_MATERIAL)
    {
        return false;
    }

    const LLRenderMaterialParams& param = static_cast<const LLRenderMaterialParams&>(data);

    if (param.mEntries.size() != mEntries.size())
    {
        return false;
    }

    for (auto& entry : mEntries)
    {
        if (param.getMaterial(entry.te_idx) != entry.id)
        {
            return false;
        }
    }

    return true;
}

void LLRenderMaterialParams::copy(const LLNetworkData& data)
{
    llassert_always(data.mType == PARAMS_RENDER_MATERIAL);
    const LLRenderMaterialParams& param = static_cast<const LLRenderMaterialParams&>(data);
    mEntries = param.mEntries;
}


void LLRenderMaterialParams::setMaterial(U8 te, const LLUUID& id)
{
    for (int i = 0; i < mEntries.size(); ++i)
    {
        if (mEntries[i].te_idx == te)
        {
            if (id.isNull())
            {
                mEntries.erase(mEntries.begin() + i);
            }
            else
            {
                mEntries[i].id = id;
            }
            return;
        }
    }

    mEntries.push_back({ te, id });
}

const LLUUID& LLRenderMaterialParams::getMaterial(U8 te) const
{
    for (int i = 0; i < mEntries.size(); ++i)
    {
        if (mEntries[i].te_idx == te)
        {
            return mEntries[i].id;
        }
    }

    return LLUUID::null;
}

