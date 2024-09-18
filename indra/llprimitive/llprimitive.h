/**
 * @file llprimitive.h
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

#ifndef LL_LLPRIMITIVE_H
#define LL_LLPRIMITIVE_H

#include "lluuid.h"
#include "v3math.h"
#include "xform.h"
#include "message.h"
#include "llpointer.h"
#include "llvolume.h"
#include "lltextureentry.h"
#include "llprimtexturelist.h"

// Moved to stdtypes.h --JC
// typedef U8 LLPCode;
class LLMessageSystem;
class LLVolumeParams;
class LLColor4;
class LLColor3;
class LLMaterialID;
class LLTextureEntry;
class LLDataPacker;
class LLVolumeMgr;

enum LLGeomType // NOTE: same vals as GL Ids
{
    LLInvalid   = 0,
    LLLineLoop  = 2,
    LLLineStrip = 3,
    LLTriangles = 4,
    LLTriStrip  = 5,
    LLTriFan    = 6,
    LLQuads     = 7,
    LLQuadStrip = 8
};

class LLVolume;

/**
 * exported constants
 */
extern const F32 OBJECT_CUT_MIN;
extern const F32 OBJECT_CUT_MAX;
extern const F32 OBJECT_CUT_INC;
extern const F32 OBJECT_MIN_CUT_INC;
extern const F32 OBJECT_ROTATION_PRECISION;

extern const F32 OBJECT_TWIST_MIN;
extern const F32 OBJECT_TWIST_MAX;
extern const F32 OBJECT_TWIST_INC;

// This is used for linear paths,
// since twist is used in a slightly different manner.
extern const F32 OBJECT_TWIST_LINEAR_MIN;
extern const F32 OBJECT_TWIST_LINEAR_MAX;
extern const F32 OBJECT_TWIST_LINEAR_INC;

extern const F32 OBJECT_MIN_HOLE_SIZE;
extern const F32 OBJECT_MAX_HOLE_SIZE_X;
extern const F32 OBJECT_MAX_HOLE_SIZE_Y;

// Revolutions parameters.
extern const F32 OBJECT_REV_MIN;
extern const F32 OBJECT_REV_MAX;
extern const F32 OBJECT_REV_INC;

extern const LLUUID SCULPT_DEFAULT_TEXTURE;

//============================================================================

// TomY: Base class for things that pack & unpack themselves
class LLNetworkData
{
public:
    // Extra parameter IDs
    enum
    {
        PARAMS_FLEXIBLE = 0x10,
        PARAMS_LIGHT    = 0x20,
        PARAMS_SCULPT   = 0x30,
        PARAMS_LIGHT_IMAGE = 0x40,
        PARAMS_RESERVED = 0x50, // Used on server-side
        PARAMS_MESH     = 0x60,
        PARAMS_EXTENDED_MESH = 0x70,
        PARAMS_RENDER_MATERIAL = 0x80,
        PARAMS_REFLECTION_PROBE = 0x90,
    };

public:
    U16 mType;
    virtual ~LLNetworkData() {};
    virtual bool pack(LLDataPacker &dp) const = 0;
    virtual bool unpack(LLDataPacker &dp) = 0;
    virtual bool operator==(const LLNetworkData& data) const = 0;
    virtual void copy(const LLNetworkData& data) = 0;
    static bool isValid(U16 param_type, U32 size);
};

extern const F32 LIGHT_MIN_RADIUS;
extern const F32 LIGHT_DEFAULT_RADIUS;
extern const F32 LIGHT_MAX_RADIUS;
extern const F32 LIGHT_MIN_FALLOFF;
extern const F32 LIGHT_DEFAULT_FALLOFF;
extern const F32 LIGHT_MAX_FALLOFF;
extern const F32 LIGHT_MIN_CUTOFF;
extern const F32 LIGHT_DEFAULT_CUTOFF;
extern const F32 LIGHT_MAX_CUTOFF;

class LLLightParams : public LLNetworkData
{
private:
    LLColor4 mColor; // linear color (not gamma corrected), alpha = intensity
    F32 mRadius;
    F32 mFalloff;
    F32 mCutoff;

public:
    LLLightParams();
    /*virtual*/ bool pack(LLDataPacker &dp) const;
    /*virtual*/ bool unpack(LLDataPacker &dp);
    /*virtual*/ bool operator==(const LLNetworkData& data) const;
    /*virtual*/ void copy(const LLNetworkData& data);
    // LLSD implementations here are provided by Eddy Stryker.
    // NOTE: there are currently unused in protocols
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);

    // set the color by gamma corrected color value
    //  color - gamma corrected color value (directly taken from an on-screen color swatch)
    void setSRGBColor(const LLColor4& color) { setLinearColor(linearColor4(color)); }

    // set the color by linear color value
    //  color - linear color value (value as it appears in shaders)
    void setLinearColor(const LLColor4& color)  { mColor = color; mColor.clamp(); }
    void setRadius(F32 radius)              { mRadius = llclamp(radius, LIGHT_MIN_RADIUS, LIGHT_MAX_RADIUS); }
    void setFalloff(F32 falloff)            { mFalloff = llclamp(falloff, LIGHT_MIN_FALLOFF, LIGHT_MAX_FALLOFF); }
    void setCutoff(F32 cutoff)              { mCutoff = llclamp(cutoff, LIGHT_MIN_CUTOFF, LIGHT_MAX_CUTOFF); }

    // get the linear space color of this light.  This value can be fed directly to shaders
    LLColor4 getLinearColor() const             { return mColor; }
    // get the sRGB (gamma corrected) color of this light, this is the value that should be displayed in the UI
    LLColor4 getSRGBColor() const           { return srgbColor4(mColor); }

    F32 getRadius() const                   { return mRadius; }
    F32 getFalloff() const                  { return mFalloff; }
    F32 getCutoff() const                   { return mCutoff; }
};

extern const F32 REFLECTION_PROBE_MIN_AMBIANCE;
extern const F32 REFLECTION_PROBE_MAX_AMBIANCE;
extern const F32 REFLECTION_PROBE_DEFAULT_AMBIANCE;
extern const F32 REFLECTION_PROBE_MIN_CLIP_DISTANCE;
extern const F32 REFLECTION_PROBE_MAX_CLIP_DISTANCE;
extern const F32 REFLECTION_PROBE_DEFAULT_CLIP_DISTANCE;

class LLReflectionProbeParams : public LLNetworkData
{
public:
    enum EFlags : U8
    {
        FLAG_BOX_VOLUME     = 0x01, // use a box influence volume
        FLAG_DYNAMIC        = 0x02, // render dynamic objects (avatars) into this Reflection Probe
        FLAG_MIRROR         = 0x04, // This probe is used for reflections on realtime mirrors.
    };

protected:
    F32 mAmbiance = REFLECTION_PROBE_DEFAULT_AMBIANCE;
    F32 mClipDistance = REFLECTION_PROBE_DEFAULT_CLIP_DISTANCE;
    U8 mFlags = 0;

public:
    LLReflectionProbeParams();
    /*virtual*/ bool pack(LLDataPacker& dp) const;
    /*virtual*/ bool unpack(LLDataPacker& dp);
    /*virtual*/ bool operator==(const LLNetworkData& data) const;
    /*virtual*/ void copy(const LLNetworkData& data);
    // LLSD implementations here are provided by Eddy Stryker.
    // NOTE: there are currently unused in protocols
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);

    void setAmbiance(F32 ambiance) { mAmbiance = llclamp(ambiance, REFLECTION_PROBE_MIN_AMBIANCE, REFLECTION_PROBE_MAX_AMBIANCE); }
    void setClipDistance(F32 distance) { mClipDistance = llclamp(distance, REFLECTION_PROBE_MIN_CLIP_DISTANCE, REFLECTION_PROBE_MAX_CLIP_DISTANCE); }
    void setIsBox(bool is_box);
    void setIsDynamic(bool is_dynamic);
    void setIsMirror(bool is_mirror);

    F32 getAmbiance() const { return mAmbiance; }
    F32 getClipDistance() const { return mClipDistance; }
    bool getIsBox() const { return (mFlags & FLAG_BOX_VOLUME) != 0; }
    bool getIsDynamic() const { return (mFlags & FLAG_DYNAMIC) != 0; }
    bool getIsMirror() const { return (mFlags & FLAG_MIRROR) != 0; }
};

//-------------------------------------------------
// This structure is also used in the part of the
// code that creates new flexible objects.
//-------------------------------------------------

// These were made into enums so that they could be used as fixed size
// array bounds.
enum EFlexibleObjectConst
{
    // "Softness" => [0,3], increments of 1
    // Represents powers of 2: 0 -> 1, 3 -> 8
    FLEXIBLE_OBJECT_MIN_SECTIONS = 0,
    FLEXIBLE_OBJECT_DEFAULT_NUM_SECTIONS = 2,
    FLEXIBLE_OBJECT_MAX_SECTIONS = 3
};

// "Tension" => [0,10], increments of 0.1
extern const F32 FLEXIBLE_OBJECT_MIN_TENSION;
extern const F32 FLEXIBLE_OBJECT_DEFAULT_TENSION;
extern const F32 FLEXIBLE_OBJECT_MAX_TENSION;

// "Drag" => [0,10], increments of 0.1
extern const F32 FLEXIBLE_OBJECT_MIN_AIR_FRICTION;
extern const F32 FLEXIBLE_OBJECT_DEFAULT_AIR_FRICTION;
extern const F32 FLEXIBLE_OBJECT_MAX_AIR_FRICTION;

// "Gravity" = [-10,10], increments of 0.1
extern const F32 FLEXIBLE_OBJECT_MIN_GRAVITY;
extern const F32 FLEXIBLE_OBJECT_DEFAULT_GRAVITY;
extern const F32 FLEXIBLE_OBJECT_MAX_GRAVITY;

// "Wind" = [0,10], increments of 0.1
extern const F32 FLEXIBLE_OBJECT_MIN_WIND_SENSITIVITY;
extern const F32 FLEXIBLE_OBJECT_DEFAULT_WIND_SENSITIVITY;
extern const F32 FLEXIBLE_OBJECT_MAX_WIND_SENSITIVITY;

extern const F32 FLEXIBLE_OBJECT_MAX_INTERNAL_TENSION_FORCE;

extern const F32 FLEXIBLE_OBJECT_DEFAULT_LENGTH;
extern const bool FLEXIBLE_OBJECT_DEFAULT_USING_COLLISION_SPHERE;
extern const bool FLEXIBLE_OBJECT_DEFAULT_RENDERING_COLLISION_SPHERE;


class LLFlexibleObjectData : public LLNetworkData
{
protected:
    S32         mSimulateLOD;       // 2^n = number of simulated sections
    F32         mGravity;
    F32         mAirFriction;       // higher is more stable, but too much looks like it's underwater
    F32         mWindSensitivity;   // interacts with tension, air friction, and gravity
    F32         mTension;           //interacts in complex ways with other parameters
    LLVector3   mUserForce;         // custom user-defined force vector
    //bool      mUsingCollisionSphere;
    //bool      mRenderingCollisionSphere;

public:
    void        setSimulateLOD(S32 lod)         { mSimulateLOD = llclamp(lod, (S32)FLEXIBLE_OBJECT_MIN_SECTIONS, (S32)FLEXIBLE_OBJECT_MAX_SECTIONS); }
    void        setGravity(F32 gravity)         { mGravity = llclamp(gravity, FLEXIBLE_OBJECT_MIN_GRAVITY, FLEXIBLE_OBJECT_MAX_GRAVITY); }
    void        setAirFriction(F32 friction)    { mAirFriction = llclamp(friction, FLEXIBLE_OBJECT_MIN_AIR_FRICTION, FLEXIBLE_OBJECT_MAX_AIR_FRICTION); }
    void        setWindSensitivity(F32 wind)    { mWindSensitivity = llclamp(wind, FLEXIBLE_OBJECT_MIN_WIND_SENSITIVITY, FLEXIBLE_OBJECT_MAX_WIND_SENSITIVITY); }
    void        setTension(F32 tension)         { mTension = llclamp(tension, FLEXIBLE_OBJECT_MIN_TENSION, FLEXIBLE_OBJECT_MAX_TENSION); }
    void        setUserForce(LLVector3 &force)  { mUserForce = force; }

    S32         getSimulateLOD() const          { return mSimulateLOD; }
    F32         getGravity() const              { return mGravity; }
    F32         getAirFriction() const          { return mAirFriction; }
    F32         getWindSensitivity() const      { return mWindSensitivity; }
    F32         getTension() const              { return mTension; }
    LLVector3   getUserForce() const            { return mUserForce; }

    //------ the constructor for the structure ------------
    LLFlexibleObjectData();
    bool pack(LLDataPacker &dp) const;
    bool unpack(LLDataPacker &dp);
    bool operator==(const LLNetworkData& data) const;
    void copy(const LLNetworkData& data);
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);
};// end of attributes structure



class LLSculptParams : public LLNetworkData
{
protected:
    LLUUID mSculptTexture;
    U8 mSculptType;

public:
    LLSculptParams();
    /*virtual*/ bool pack(LLDataPacker &dp) const;
    /*virtual*/ bool unpack(LLDataPacker &dp);
    /*virtual*/ bool operator==(const LLNetworkData& data) const;
    /*virtual*/ void copy(const LLNetworkData& data);
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);

    void setSculptTexture(const LLUUID& texture_id, U8 sculpt_type);
    LLUUID getSculptTexture() const         { return mSculptTexture; }
    U8 getSculptType() const                { return mSculptType; }
};

class LLLightImageParams : public LLNetworkData
{
protected:
    LLUUID mLightTexture;
    LLVector3 mParams;

public:
    LLLightImageParams();
    /*virtual*/ bool pack(LLDataPacker &dp) const;
    /*virtual*/ bool unpack(LLDataPacker &dp);
    /*virtual*/ bool operator==(const LLNetworkData& data) const;
    /*virtual*/ void copy(const LLNetworkData& data);
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);

    void setLightTexture(const LLUUID& id) { mLightTexture = id; }
    LLUUID getLightTexture() const         { return mLightTexture; }
    bool isLightSpotlight() const         { return mLightTexture.notNull(); }
    void setParams(const LLVector3& params) { mParams = params; }
    LLVector3 getParams() const            { return mParams; }

};

class LLExtendedMeshParams : public LLNetworkData
{
protected:
    U32 mFlags;

public:
    static const U32 ANIMATED_MESH_ENABLED_FLAG = 0x1 << 0;

    LLExtendedMeshParams();
    /*virtual*/ bool pack(LLDataPacker &dp) const;
    /*virtual*/ bool unpack(LLDataPacker &dp);
    /*virtual*/ bool operator==(const LLNetworkData& data) const;
    /*virtual*/ void copy(const LLNetworkData& data);
    LLSD asLLSD() const;
    operator LLSD() const { return asLLSD(); }
    bool fromLLSD(LLSD& sd);

    void setFlags(const U32& flags) { mFlags = flags; }
    U32 getFlags() const { return mFlags; }

};

class LLRenderMaterialParams : public LLNetworkData
{
private:
    struct Entry
    {
        U8 te_idx;
        LLUUID id;
    };
    std::vector< Entry > mEntries;

public:
    LLRenderMaterialParams();
    bool pack(LLDataPacker& dp) const override;
    bool unpack(LLDataPacker& dp) override;
    bool operator==(const LLNetworkData& data) const override;
    void copy(const LLNetworkData& data) override;

    void setMaterial(U8 te_idx, const LLUUID& id);
    const LLUUID& getMaterial(U8 te_idx) const;

    bool isEmpty() { return mEntries.empty(); }
};


// This code is not naming-standards compliant. Leaving it like this for
// now to make the connection to code in
//  bool packTEMessage(LLDataPacker &dp) const;
// more obvious. This should be refactored to remove the duplication, at which
// point we can fix the names as well.
// - Vir
class LLTEContents
{
public:
    static const size_t MAX_TES = 45;
    static const size_t MAX_TE_BUFFER = 4096;

    // delete the default ctor
    LLTEContents() = delete;

    // please use ctor which expects the number of textures as argument
    LLTEContents(size_t N);

    ~LLTEContents();

    U8 getNumTEs() const { return (U8)(num_textures); }

private:
    U8* data; // one big chunk of data
    size_t num_textures;

public:
    // re-cast offsets into data
    LLUUID* image_ids;
    LLMaterialID* material_ids;
    LLColor4U* colors;
    F32* scale_s;
    F32* scale_t;
    S16* offset_s;
    S16* offset_t;
    S16* rot;
    U8* bump;
    U8* media_flags;
    U8* glow;
    U8* alpha_gamma;
    // Note: we keep larger elements near the front so they are always 16-byte aligned,
    // even for odd num_textures, and byte-sized elements to the back.
};

class LLPrimitive : public LLXform
{
public:

    // HACK for removing LLPrimitive's dependency on gVolumeMgr global.
    // If a different LLVolumeManager is instantiated and set early enough
    // then the LLPrimitive class will use it instead of gVolumeMgr.
    static LLVolumeMgr* getVolumeManager() { return sVolumeManager; }
    static void setVolumeManager( LLVolumeMgr* volume_manager);
    static bool cleanupVolumeManager();

    // these flags influence how the RigidBody representation is built
    static const U32 PRIM_FLAG_PHANTOM              = 0x1 << 0;
    static const U32 PRIM_FLAG_VOLUME_DETECT        = 0x1 << 1;
    static const U32 PRIM_FLAG_DYNAMIC              = 0x1 << 2;
    static const U32 PRIM_FLAG_AVATAR               = 0x1 << 3;
    static const U32 PRIM_FLAG_SCULPT               = 0x1 << 4;
    // not used yet, but soon
    static const U32 PRIM_FLAG_COLLISION_CALLBACK   = 0x1 << 5;
    static const U32 PRIM_FLAG_CONVEX               = 0x1 << 6;
    static const U32 PRIM_FLAG_DEFAULT_VOLUME       = 0x1 << 7;
    static const U32 PRIM_FLAG_SITTING              = 0x1 << 8;
    static const U32 PRIM_FLAG_SITTING_ON_GROUND    = 0x1 << 9;     // Set along with PRIM_FLAG_SITTING

    LLPrimitive();
    virtual ~LLPrimitive();

    void clearTextureList();

    static LLPrimitive *createPrimitive(LLPCode p_code);
    void init_primitive(LLPCode p_code);

    void setPCode(const LLPCode pcode);
    const LLVolume *getVolumeConst() const { return mVolumep; }     // HACK for Windoze confusion about ostream operator in LLVolume
    LLVolume *getVolume() const { return mVolumep; }
    virtual bool setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);

    // Modify texture entry properties
    inline bool validTE(const U8 te_num) const;
    LLTextureEntry* getTE(const U8 te_num) const;

    virtual void setNumTEs(const U8 num_tes);
    virtual void setAllTESelected(bool sel);
    virtual void setAllTETextures(const LLUUID &tex_id);
    virtual void setTE(const U8 index, const LLTextureEntry& te);
    virtual S32 setTEColor(const U8 te, const LLColor4 &color);
    virtual S32 setTEColor(const U8 te, const LLColor3 &color);
    virtual S32 setTEAlpha(const U8 te, const F32 alpha);
    virtual S32 setTETexture(const U8 te, const LLUUID &tex_id);
    virtual S32 setTEScale (const U8 te, const F32 s, const F32 t);
    virtual S32 setTEScaleS(const U8 te, const F32 s);
    virtual S32 setTEScaleT(const U8 te, const F32 t);
    virtual S32 setTEOffset (const U8 te, const F32 s, const F32 t);
    virtual S32 setTEOffsetS(const U8 te, const F32 s);
    virtual S32 setTEOffsetT(const U8 te, const F32 t);
    virtual S32 setTERotation(const U8 te, const F32 r);
    virtual S32 setTEBumpShinyFullbright(const U8 te, const U8 bump);
    virtual S32 setTEBumpShiny(const U8 te, const U8 bump);
    virtual S32 setTEMediaTexGen(const U8 te, const U8 media);
    virtual S32 setTEBumpmap(const U8 te, const U8 bump);
    virtual S32 setTEAlphaGamma(const U8 te, const U8 alphagamma);
    virtual S32 setTETexGen(const U8 te, const U8 texgen);
    virtual S32 setTEShiny(const U8 te, const U8 shiny);
    virtual S32 setTEFullbright(const U8 te, const U8 fullbright);
    virtual S32 setTEMediaFlags(const U8 te, const U8 flags);
    virtual S32 setTEGlow(const U8 te, const F32 glow);
    virtual S32 setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID);
    virtual S32 setTEMaterialParams(const U8 index, const LLMaterialPtr pMaterialParams);
    virtual bool setMaterial(const U8 material); // returns true if material changed
    virtual void setTESelected(const U8 te, bool sel);

    LLMaterialPtr getTEMaterialParams(const U8 index);

    void copyTEs(const LLPrimitive *primitive);
    S32 packTEField(U8 *cur_ptr, U8 *data_ptr, U8 data_size, U8 last_face_index, EMsgVariableType type) const;

    S32  packTEMessageBuffer(U8* packed_buffer) const;
    bool packTEMessage(LLMessageSystem *mesgsys) const;

    S32 unpackTEMessage(LLMessageSystem* mesgsys, char const* block_name, const S32 block_num); // Variable num of blocks
    S32 unpackTEMessage(LLDataPacker &dp);
    S32 parseTEMessage(LLMessageSystem* mesgsys, char const* block_name, const S32 block_num, LLTEContents& tec);
    static S32 parseTEMessage(U8* packed_buffer, U32 data_size, LLTEContents& tec);
    S32 applyParsedTEMessage(const LLTEContents& tec);

#ifdef CHECK_FOR_FINITE
    inline void setPosition(const LLVector3& pos);
    inline void setPosition(const F32 x, const F32 y, const F32 z);
    inline void addPosition(const LLVector3& pos);

    inline void setAngularVelocity(const LLVector3& avel);
    inline void setAngularVelocity(const F32 x, const F32 y, const F32 z);
    inline void setVelocity(const LLVector3& vel);
    inline void setVelocity(const F32 x, const F32 y, const F32 z);
    inline void setVelocityX(const F32 x);
    inline void setVelocityY(const F32 y);
    inline void setVelocityZ(const F32 z);
    inline void addVelocity(const LLVector3& vel);
    inline void setAcceleration(const LLVector3& accel);
    inline void setAcceleration(const F32 x, const F32 y, const F32 z);
#else
    // Don't override the base LLXForm operators.
    // Special case for setPosition.  If not check-for-finite, fall through to LLXform method.
    // void     setPosition(F32 x, F32 y, F32 z)
    // void     setPosition(LLVector3)

    void        setAngularVelocity(const LLVector3& avel)   { mAngularVelocity = avel; }
    void        setAngularVelocity(const F32 x, const F32 y, const F32 z)   { mAngularVelocity.setVec(x,y,z); }
    void        setVelocity(const LLVector3& vel)           { mVelocity = vel; }
    void        setVelocity(const F32 x, const F32 y, const F32 z)          { mVelocity.setVec(x,y,z); }
    void        setVelocityX(const F32 x)                   { mVelocity.mV[VX] = x; }
    void        setVelocityY(const F32 y)                   { mVelocity.mV[VY] = y; }
    void        setVelocityZ(const F32 z)                   { mVelocity.mV[VZ] = z; }
    void        addVelocity(const LLVector3& vel)           { mVelocity += vel; }
    void        setAcceleration(const LLVector3& accel)     { mAcceleration = accel; }
    void        setAcceleration(const F32 x, const F32 y, const F32 z)      { mAcceleration.setVec(x,y,z); }
#endif

    LLPCode             getPCode() const            { return mPrimitiveCode; }
    std::string         getPCodeString() const      { return pCodeToString(mPrimitiveCode); }
    const LLVector3&    getAngularVelocity() const  { return mAngularVelocity; }
    const LLVector3&    getVelocity() const         { return mVelocity; }
    const LLVector3&    getAcceleration() const     { return mAcceleration; }
    U8                  getNumTEs() const           { return mTextureList.size(); }
    U8                  getExpectedNumTEs() const;

    U8                  getMaterial() const         { return mMaterial; }

    void                setVolumeType(const U8 code);
    U8                  getVolumeType();

    // clears existing textures
    // copies the contents of other_list into mEntryList
    void copyTextureList(const LLPrimTextureList& other_list);

    // clears existing textures
    // takes the contents of other_list and clears other_list
    void takeTextureList(LLPrimTextureList& other_list);

    inline bool isAvatar() const;
    inline bool isSittingAvatar() const;
    inline bool isSittingAvatarOnGround() const;
    inline bool hasBumpmap() const  { return mNumBumpmapTEs > 0;}

    void setFlags(U32 flags) { mMiscFlags = flags; }
    void addFlags(U32 flags) { mMiscFlags |= flags; }
    void removeFlags(U32 flags) { mMiscFlags &= ~flags; }
    U32 getFlags() const { return mMiscFlags; }
    bool checkFlags(U32 flags) const { return (mMiscFlags & flags) != 0; }

    static std::string pCodeToString(const LLPCode pcode);
    static LLPCode legacyToPCode(const U8 legacy);
    static U8 pCodeToLegacy(const LLPCode pcode);
    static bool getTESTAxes(const U8 face, U32* s_axis, U32* t_axis);

    bool hasRenderMaterialParams() const;

    inline static bool isPrimitive(const LLPCode pcode);
    inline static bool isApp(const LLPCode pcode);

private:
    void updateNumBumpmap(const U8 index, const U8 bump);

protected:
    LLPCode             mPrimitiveCode;     // Primitive code
    LLVector3           mVelocity;          // how fast are we moving?
    LLVector3           mAcceleration;      // are we under constant acceleration?
    LLVector3           mAngularVelocity;   // angular velocity
    LLPointer<LLVolume> mVolumep;
    LLPrimTextureList   mTextureList;       // list of texture GUIDs, scales, offsets
    U8                  mMaterial;          // Material code
    U8                  mNumTEs;            // # of faces on the primitve
    U8                  mNumBumpmapTEs;     // number of bumpmap TEs.
    U32                 mMiscFlags;         // home for misc bools

public:
    static LLVolumeMgr* sVolumeManager;

    enum
    {
        NO_LOD = -1
    };
};

inline bool LLPrimitive::isAvatar() const
{
    return LL_PCODE_LEGACY_AVATAR == mPrimitiveCode;
}

inline bool LLPrimitive::isSittingAvatar() const
{
    // this is only used server-side
    return isAvatar() && checkFlags(PRIM_FLAG_SITTING | PRIM_FLAG_SITTING_ON_GROUND);
}

inline bool LLPrimitive::isSittingAvatarOnGround() const
{
    // this is only used server-side
    return isAvatar() && checkFlags(PRIM_FLAG_SITTING_ON_GROUND);
}

// static
inline bool LLPrimitive::isPrimitive(const LLPCode pcode)
{
    LLPCode base_type = pcode & LL_PCODE_BASE_MASK;

    if (base_type && (base_type < LL_PCODE_APP))
    {
        return true;
    }
    return false;
}

// static
inline bool LLPrimitive::isApp(const LLPCode pcode)
{
    LLPCode base_type = pcode & LL_PCODE_BASE_MASK;

    return (base_type == LL_PCODE_APP);
}


#ifdef CHECK_FOR_FINITE
// Special case for setPosition.  If not check-for-finite, fall through to LLXform method.
void LLPrimitive::setPosition(const F32 x, const F32 y, const F32 z)
{
    if (llfinite(x) && llfinite(y) && llfinite(z))
    {
        LLXform::setPosition(x, y, z);
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setPosition(x,y,z) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}

// Special case for setPosition.  If not check-for-finite, fall through to LLXform method.
void LLPrimitive::setPosition(const LLVector3& pos)
{
    if (pos.isFinite())
    {
        LLXform::setPosition(pos);
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setPosition(LLVector3) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}

void LLPrimitive::setAngularVelocity(const LLVector3& avel)
{
    if (avel.isFinite())
    {
        mAngularVelocity = avel;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setAngularVelocity" << LL_ENDL;
    }
}

void LLPrimitive::setAngularVelocity(const F32 x, const F32 y, const F32 z)
{
    if (llfinite(x) && llfinite(y) && llfinite(z))
    {
        mAngularVelocity.setVec(x,y,z);
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setAngularVelocity" << LL_ENDL;
    }
}

void LLPrimitive::setVelocity(const LLVector3& vel)
{
    if (vel.isFinite())
    {
        mVelocity = vel;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setVelocity(LLVector3) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}

void LLPrimitive::setVelocity(const F32 x, const F32 y, const F32 z)
{
    if (llfinite(x) && llfinite(y) && llfinite(z))
    {
        mVelocity.setVec(x,y,z);
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setVelocity(F32,F32,F32) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}

void LLPrimitive::setVelocityX(const F32 x)
{
    if (llfinite(x))
    {
        mVelocity.mV[VX] = x;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setVelocityX" << LL_ENDL;
    }
}

void LLPrimitive::setVelocityY(const F32 y)
{
    if (llfinite(y))
    {
        mVelocity.mV[VY] = y;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setVelocityY" << LL_ENDL;
    }
}

void LLPrimitive::setVelocityZ(const F32 z)
{
    if (llfinite(z))
    {
        mVelocity.mV[VZ] = z;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setVelocityZ" << LL_ENDL;
    }
}

void LLPrimitive::addVelocity(const LLVector3& vel)
{
    if (vel.isFinite())
    {
        mVelocity += vel;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::addVelocity" << LL_ENDL;
    }
}

void LLPrimitive::setAcceleration(const LLVector3& accel)
{
    if (accel.isFinite())
    {
        mAcceleration = accel;
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setAcceleration(LLVector3) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}

void LLPrimitive::setAcceleration(const F32 x, const F32 y, const F32 z)
{
    if (llfinite(x) && llfinite(y) && llfinite(z))
    {
        mAcceleration.setVec(x,y,z);
    }
    else
    {
        LL_ERRS() << "Non Finite in LLPrimitive::setAcceleration(F32,F32,F32) for " << pCodeToString(mPrimitiveCode) << LL_ENDL;
    }
}
#endif // CHECK_FOR_FINITE

inline bool LLPrimitive::validTE(const U8 te_num) const
{
    return (mNumTEs && te_num < mNumTEs);
}

#endif

