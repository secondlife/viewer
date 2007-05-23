/** 
 * @file llprimitive.h
 * @brief LLPrimitive base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPRIMITIVE_H
#define LL_LLPRIMITIVE_H

#include "lluuid.h"
#include "v3math.h"
#include "xform.h"
#include "message.h"
#include "llmemory.h"
#include "llvolume.h"
#include "lltextureentry.h"

// Moved to stdtypes.h --JC
// typedef U8 LLPCode;
class LLMessageSystem;
class LLVolumeParams;
class LLColor4;
class LLColor3;
class LLTextureEntry;
class LLDataPacker;

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

extern const char *SCULPT_DEFAULT_TEXTURE;

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
		PARAMS_SCULPT   = 0x30
	};
	
public:
	U16 mType;
	virtual ~LLNetworkData() {};
	virtual BOOL pack(LLDataPacker &dp) const = 0;
	virtual BOOL unpack(LLDataPacker &dp) = 0;
	virtual bool operator==(const LLNetworkData& data) const = 0;
	virtual void copy(const LLNetworkData& data) = 0;
	static BOOL isValid(U16 param_type, U32 size);
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
protected:
	LLColor4 mColor; // alpha = intensity
	F32 mRadius;
	F32 mFalloff;
	F32 mCutoff;

public:
	LLLightParams();
	/*virtual*/ BOOL pack(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpack(LLDataPacker &dp);
	/*virtual*/ bool operator==(const LLNetworkData& data) const;
	/*virtual*/ void copy(const LLNetworkData& data);

	void setColor(const LLColor4& color)	{ mColor = color; mColor.clamp(); }
	void setRadius(F32 radius)				{ mRadius = llclamp(radius, LIGHT_MIN_RADIUS, LIGHT_MAX_RADIUS); }
	void setFalloff(F32 falloff)			{ mFalloff = llclamp(falloff, LIGHT_MIN_FALLOFF, LIGHT_MAX_FALLOFF); }
	void setCutoff(F32 cutoff)				{ mCutoff = llclamp(cutoff, LIGHT_MIN_CUTOFF, LIGHT_MAX_CUTOFF); }

	LLColor4 getColor() const				{ return mColor; }
	F32 getRadius() const					{ return mRadius; }
	F32 getFalloff() const					{ return mFalloff; }
	F32 getCutoff() const					{ return mCutoff; }
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
extern const BOOL FLEXIBLE_OBJECT_DEFAULT_USING_COLLISION_SPHERE;
extern const BOOL FLEXIBLE_OBJECT_DEFAULT_RENDERING_COLLISION_SPHERE;


class LLFlexibleObjectData : public LLNetworkData
{
protected:
	S32			mSimulateLOD;		// 2^n = number of simulated sections
	F32			mGravity;
	F32			mAirFriction;		// higher is more stable, but too much looks like it's underwater
	F32			mWindSensitivity;	// interacts with tension, air friction, and gravity
	F32			mTension;			//interacts in complex ways with other parameters
	LLVector3	mUserForce;			// custom user-defined force vector
	//BOOL		mUsingCollisionSphere;
	//BOOL		mRenderingCollisionSphere;

public:
	void		setSimulateLOD(S32 lod)			{ mSimulateLOD = llclamp(lod, (S32)FLEXIBLE_OBJECT_MIN_SECTIONS, (S32)FLEXIBLE_OBJECT_MAX_SECTIONS); }
	void		setGravity(F32 gravity)			{ mGravity = llclamp(gravity, FLEXIBLE_OBJECT_MIN_GRAVITY, FLEXIBLE_OBJECT_MAX_GRAVITY); }
	void		setAirFriction(F32 friction)	{ mAirFriction = llclamp(friction, FLEXIBLE_OBJECT_MIN_AIR_FRICTION, FLEXIBLE_OBJECT_MAX_AIR_FRICTION); }
	void		setWindSensitivity(F32 wind)	{ mWindSensitivity = llclamp(wind, FLEXIBLE_OBJECT_MIN_WIND_SENSITIVITY, FLEXIBLE_OBJECT_MAX_WIND_SENSITIVITY); }
	void		setTension(F32 tension)			{ mTension = llclamp(tension, FLEXIBLE_OBJECT_MIN_TENSION, FLEXIBLE_OBJECT_MAX_TENSION); }
	void		setUserForce(LLVector3 &force)	{ mUserForce = force; }

	S32			getSimulateLOD() const			{ return mSimulateLOD; }
	F32			getGravity() const				{ return mGravity; }
	F32			getAirFriction() const			{ return mAirFriction; }
	F32			getWindSensitivity() const		{ return mWindSensitivity; }
	F32			getTension() const				{ return mTension; }
	LLVector3	getUserForce() const			{ return mUserForce; }

	//------ the constructor for the structure ------------
	LLFlexibleObjectData();
	BOOL pack(LLDataPacker &dp) const;
	BOOL unpack(LLDataPacker &dp);
	bool operator==(const LLNetworkData& data) const;
	void copy(const LLNetworkData& data);
};// end of attributes structure



class LLSculptParams : public LLNetworkData
{
protected:
	LLUUID mSculptTexture;
	U8 mSculptType;
	
public:
	LLSculptParams();
	/*virtual*/ BOOL pack(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpack(LLDataPacker &dp);
	/*virtual*/ bool operator==(const LLNetworkData& data) const;
	/*virtual*/ void copy(const LLNetworkData& data);

	void setSculptTexture(const LLUUID& id) { mSculptTexture = id; }
	LLUUID getSculptTexture()               { return mSculptTexture; }
	void setSculptType(U8 type)             { mSculptType = type; }
	U8 getSculptType()                      { return mSculptType; }
};



class LLPrimitive : public LLXform
{
public:
	LLPrimitive();
	virtual ~LLPrimitive();

	static LLPrimitive *createPrimitive(LLPCode p_code);
	void init(LLPCode p_code);

	void setPCode(const LLPCode pcode);
	const LLVolume *getVolumeConst() const { return mVolumep; }		// HACK for Windoze confusion about ostream operator in LLVolume
	LLVolume *getVolume() const { return mVolumep; }
	virtual BOOL setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);

	// Modify texture entry properties
	inline BOOL validTE(const U8 te_num) const;
	const LLTextureEntry *getTE(const U8 te_num) const;

	virtual void setNumTEs(const U8 num_tes);
	virtual void setAllTETextures(const LLUUID &tex_id);
	virtual void setTE(const U8 index, const LLTextureEntry &te);
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
	virtual S32 setTETexGen(const U8 te, const U8 texgen);
	virtual S32 setTEShiny(const U8 te, const U8 shiny);
	virtual S32 setTEFullbright(const U8 te, const U8 fullbright);
	virtual S32 setTEMediaFlags(const U8 te, const U8 flags);
	virtual S32 setTEGlow(const U8 te, const F32 glow);
	virtual BOOL setMaterial(const U8 material); // returns TRUE if material changed

	void setTEArrays(const U8 size,
					  const LLUUID* image_ids,
					  const F32* scale_s,
					  const F32* scale_t);
	void copyTEs(const LLPrimitive *primitive);
	S32 packTEField(U8 *cur_ptr, U8 *data_ptr, U8 data_size, U8 last_face_index, EMsgVariableType type) const;
	S32 unpackTEField(U8 *cur_ptr, U8 *buffer_end, U8 *data_ptr, U8 data_size, U8 face_count, EMsgVariableType type);
	BOOL packTEMessage(LLMessageSystem *mesgsys) const;
	BOOL packTEMessage(LLDataPacker &dp) const;
	S32 unpackTEMessage(LLMessageSystem *mesgsys, char *block_name);
	S32 unpackTEMessage(LLMessageSystem *mesgsys, char *block_name, const S32 block_num); // Variable num of blocks
	BOOL unpackTEMessage(LLDataPacker &dp);
	
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
	// void		setPosition(F32 x, F32 y, F32 z)
	// void		setPosition(LLVector3)

	void 		setAngularVelocity(const LLVector3& avel)	{ mAngularVelocity = avel; }
	void 		setAngularVelocity(const F32 x, const F32 y, const F32 z)	{ mAngularVelocity.setVec(x,y,z); }
	void 		setVelocity(const LLVector3& vel)			{ mVelocity = vel; }
	void 		setVelocity(const F32 x, const F32 y, const F32 z)			{ mVelocity.setVec(x,y,z); }
	void 		setVelocityX(const F32 x)					{ mVelocity.mV[VX] = x; }
	void 		setVelocityY(const F32 y)					{ mVelocity.mV[VY] = y; }
	void 		setVelocityZ(const F32 z)					{ mVelocity.mV[VZ] = z; }
	void 		addVelocity(const LLVector3& vel)			{ mVelocity += vel; }
	void 		setAcceleration(const LLVector3& accel)		{ mAcceleration = accel; }
	void 		setAcceleration(const F32 x, const F32 y, const F32 z)		{ mAcceleration.setVec(x,y,z); }
#endif
	
	const LLPCode		getPCode() const			{ return mPrimitiveCode; }
	const char *		getPCodeString() const		{ return pCodeToString(mPrimitiveCode); }
	const LLVector3&	getAngularVelocity() const	{ return mAngularVelocity; }
	const LLVector3&	getVelocity() const			{ return mVelocity; }
	const LLVector3&	getAcceleration() const		{ return mAcceleration; }
	const U8			getNumTEs() const			{ return mNumTEs; }

	const U8			getMaterial() const			{ return mMaterial; }
	
	void				setVolumeType(const U8 code);
	U8					getVolumeType();

	void setTextureList(LLTextureEntry *listp);

	inline BOOL			isAvatar() const;
	
	static const char *pCodeToString(const LLPCode pcode);
	static LLPCode legacyToPCode(const U8 legacy);
	static U8 pCodeToLegacy(const LLPCode pcode);

	inline static BOOL isPrimitive(const LLPCode pcode);
	inline static BOOL isApp(const LLPCode pcode);

protected:
	LLPCode				mPrimitiveCode;		// Primitive code
	LLVector3			mVelocity;			// how fast are we moving?
	LLVector3			mAcceleration;		// are we under constant acceleration?
	LLVector3			mAngularVelocity;	// angular velocity
	LLPointer<LLVolume> mVolumep;
	LLTextureEntry		*mTextureList;		// list of texture GUIDs, scales, offsets
	U8					mMaterial;			// Material code
	U8					mNumTEs;			// # of faces on the primitve	
};

inline BOOL LLPrimitive::isAvatar() const
{
	return mPrimitiveCode == LL_PCODE_LEGACY_AVATAR;
}

// static
inline BOOL LLPrimitive::isPrimitive(const LLPCode pcode)
{
	LLPCode base_type = pcode & LL_PCODE_BASE_MASK;

	if (base_type && (base_type < LL_PCODE_APP))
	{
		return TRUE;
	}
	return FALSE;
}

// static
inline BOOL LLPrimitive::isApp(const LLPCode pcode)
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
		llerrs << "Non Finite in LLPrimitive::setPosition(x,y,z) for " << pCodeToString(mPrimitiveCode) << llendl;
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
		llerrs << "Non Finite in LLPrimitive::setPosition(LLVector3) for " << pCodeToString(mPrimitiveCode) << llendl;
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
		llerror("Non Finite in LLPrimitive::setAngularVelocity", 0);
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
		llerror("Non Finite in LLPrimitive::setAngularVelocity", 0);
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
		llerrs << "Non Finite in LLPrimitive::setVelocity(LLVector3) for " << pCodeToString(mPrimitiveCode) << llendl;
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
		llerrs << "Non Finite in LLPrimitive::setVelocity(F32,F32,F32) for " << pCodeToString(mPrimitiveCode) << llendl;
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
		llerror("Non Finite in LLPrimitive::setVelocityX", 0);
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
		llerror("Non Finite in LLPrimitive::setVelocityY", 0);
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
		llerror("Non Finite in LLPrimitive::setVelocityZ", 0);
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
		llerror("Non Finite in LLPrimitive::addVelocity", 0);
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
		llerrs << "Non Finite in LLPrimitive::setAcceleration(LLVector3) for " << pCodeToString(mPrimitiveCode) << llendl;
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
		llerrs << "Non Finite in LLPrimitive::setAcceleration(F32,F32,F32) for " << pCodeToString(mPrimitiveCode) << llendl;
	}
}
#endif // CHECK_FOR_FINITE

inline BOOL LLPrimitive::validTE(const U8 te_num) const
{
	return (mNumTEs && te_num < mNumTEs);
}

#endif

