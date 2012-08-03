/** 
 * @file llvolume.h
 * @brief LLVolume base class.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVOLUME_H
#define LL_LLVOLUME_H

#include <iostream>

class LLProfileParams;
class LLPathParams;
class LLVolumeParams;
class LLProfile;
class LLPath;

template <class T> class LLOctreeNode;

class LLVector4a;
class LLVolumeFace;
class LLVolume;
class LLVolumeTriangle;

#include "lldarray.h"
#include "lluuid.h"
#include "v4color.h"
//#include "vmath.h"
#include "v2math.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "llquaternion.h"
#include "llstrider.h"
#include "v4coloru.h"
#include "llrefcount.h"
#include "llpointer.h"
#include "llfile.h"

//============================================================================

const S32 MIN_DETAIL_FACES = 6;
const S32 MIN_LOD = 0;
const S32 MAX_LOD = 3;

// These are defined here but are not enforced at this level,
// rather they are here for the convenience of code that uses
// the LLVolume class.
const F32 MIN_VOLUME_PROFILE_WIDTH 	= 0.05f;
const F32 MIN_VOLUME_PATH_WIDTH 	= 0.05f;

const F32 CUT_QUANTA    = 0.00002f;
const F32 SCALE_QUANTA  = 0.01f;
const F32 SHEAR_QUANTA  = 0.01f;
const F32 TAPER_QUANTA  = 0.01f;
const F32 REV_QUANTA    = 0.015f;
const F32 HOLLOW_QUANTA = 0.00002f;

const S32 MAX_VOLUME_TRIANGLE_INDICES = 10000;

//============================================================================

// useful masks
const LLPCode LL_PCODE_HOLLOW_MASK 	= 0x80;		// has a thickness
const LLPCode LL_PCODE_SEGMENT_MASK = 0x40;		// segments (1 angle)
const LLPCode LL_PCODE_PATCH_MASK 	= 0x20;		// segmented segments (2 angles)
const LLPCode LL_PCODE_HEMI_MASK 	= 0x10;		// half-primitives get their own type per PR's dictum
const LLPCode LL_PCODE_BASE_MASK 	= 0x0F;

	// primitive shapes
const LLPCode	LL_PCODE_CUBE 			= 1;
const LLPCode	LL_PCODE_PRISM 			= 2;
const LLPCode	LL_PCODE_TETRAHEDRON 	= 3;
const LLPCode	LL_PCODE_PYRAMID 		= 4;
const LLPCode	LL_PCODE_CYLINDER 		= 5;
const LLPCode	LL_PCODE_CONE 			= 6;
const LLPCode	LL_PCODE_SPHERE 		= 7;
const LLPCode	LL_PCODE_TORUS 			= 8;
const LLPCode	LL_PCODE_VOLUME			= 9;

	// surfaces
//const LLPCode	LL_PCODE_SURFACE_TRIANGLE 	= 10;
//const LLPCode	LL_PCODE_SURFACE_SQUARE 	= 11;
//const LLPCode	LL_PCODE_SURFACE_DISC 		= 12;

const LLPCode	LL_PCODE_APP				= 14; // App specific pcode (for viewer/sim side only objects)
const LLPCode	LL_PCODE_LEGACY				= 15;

// Pcodes for legacy objects
//const LLPCode	LL_PCODE_LEGACY_ATOR =				0x10 | LL_PCODE_LEGACY; // ATOR
const LLPCode	LL_PCODE_LEGACY_AVATAR =			0x20 | LL_PCODE_LEGACY; // PLAYER
//const LLPCode	LL_PCODE_LEGACY_BIRD =				0x30 | LL_PCODE_LEGACY; // BIRD
//const LLPCode	LL_PCODE_LEGACY_DEMON =				0x40 | LL_PCODE_LEGACY; // DEMON
const LLPCode	LL_PCODE_LEGACY_GRASS =				0x50 | LL_PCODE_LEGACY; // GRASS
const LLPCode	LL_PCODE_TREE_NEW =					0x60 | LL_PCODE_LEGACY; // new trees
//const LLPCode	LL_PCODE_LEGACY_ORACLE =			0x70 | LL_PCODE_LEGACY; // ORACLE
const LLPCode	LL_PCODE_LEGACY_PART_SYS =			0x80 | LL_PCODE_LEGACY; // PART_SYS
const LLPCode	LL_PCODE_LEGACY_ROCK =				0x90 | LL_PCODE_LEGACY; // ROCK
//const LLPCode	LL_PCODE_LEGACY_SHOT =				0xA0 | LL_PCODE_LEGACY; // BASIC_SHOT
//const LLPCode	LL_PCODE_LEGACY_SHOT_BIG =			0xB0 | LL_PCODE_LEGACY;
//const LLPCode	LL_PCODE_LEGACY_SMOKE =				0xC0 | LL_PCODE_LEGACY; // SMOKE
//const LLPCode	LL_PCODE_LEGACY_SPARK =				0xD0 | LL_PCODE_LEGACY;// SPARK
const LLPCode	LL_PCODE_LEGACY_TEXT_BUBBLE =		0xE0 | LL_PCODE_LEGACY; // TEXTBUBBLE
const LLPCode	LL_PCODE_LEGACY_TREE =				0xF0 | LL_PCODE_LEGACY; // TREE

	// hemis
const LLPCode	LL_PCODE_CYLINDER_HEMI =		LL_PCODE_CYLINDER	| LL_PCODE_HEMI_MASK;
const LLPCode	LL_PCODE_CONE_HEMI =			LL_PCODE_CONE		| LL_PCODE_HEMI_MASK;
const LLPCode	LL_PCODE_SPHERE_HEMI =			LL_PCODE_SPHERE		| LL_PCODE_HEMI_MASK;
const LLPCode	LL_PCODE_TORUS_HEMI =			LL_PCODE_TORUS		| LL_PCODE_HEMI_MASK;


// Volumes consist of a profile at the base that is swept around
// a path to make a volume.
// The profile code
const U8	LL_PCODE_PROFILE_MASK		= 0x0f;
const U8	LL_PCODE_PROFILE_MIN		= 0x00;
const U8    LL_PCODE_PROFILE_CIRCLE		= 0x00;
const U8    LL_PCODE_PROFILE_SQUARE		= 0x01;
const U8	LL_PCODE_PROFILE_ISOTRI		= 0x02;
const U8    LL_PCODE_PROFILE_EQUALTRI	= 0x03;
const U8    LL_PCODE_PROFILE_RIGHTTRI	= 0x04;
const U8	LL_PCODE_PROFILE_CIRCLE_HALF = 0x05;
const U8	LL_PCODE_PROFILE_MAX		= 0x05;

// Stored in the profile byte
const U8	LL_PCODE_HOLE_MASK		= 0xf0;
const U8	LL_PCODE_HOLE_MIN		= 0x00;	  
const U8	LL_PCODE_HOLE_SAME		= 0x00;		// same as outside profile
const U8	LL_PCODE_HOLE_CIRCLE	= 0x10;
const U8	LL_PCODE_HOLE_SQUARE	= 0x20;
const U8	LL_PCODE_HOLE_TRIANGLE	= 0x30;
const U8	LL_PCODE_HOLE_MAX		= 0x03;		// min/max needs to be >> 4 of real min/max

const U8    LL_PCODE_PATH_IGNORE    = 0x00;
const U8	LL_PCODE_PATH_MIN		= 0x01;		// min/max needs to be >> 4 of real min/max
const U8    LL_PCODE_PATH_LINE      = 0x10;
const U8    LL_PCODE_PATH_CIRCLE    = 0x20;
const U8    LL_PCODE_PATH_CIRCLE2   = 0x30;
const U8    LL_PCODE_PATH_TEST      = 0x40;
const U8    LL_PCODE_PATH_FLEXIBLE  = 0x80;
const U8	LL_PCODE_PATH_MAX		= 0x08;

//============================================================================

// face identifiers
typedef U16 LLFaceID;

const LLFaceID	LL_FACE_PATH_BEGIN		= 0x1 << 0;
const LLFaceID	LL_FACE_PATH_END		= 0x1 << 1;
const LLFaceID	LL_FACE_INNER_SIDE		= 0x1 << 2;
const LLFaceID	LL_FACE_PROFILE_BEGIN	= 0x1 << 3;
const LLFaceID	LL_FACE_PROFILE_END		= 0x1 << 4;
const LLFaceID	LL_FACE_OUTER_SIDE_0	= 0x1 << 5;
const LLFaceID	LL_FACE_OUTER_SIDE_1	= 0x1 << 6;
const LLFaceID	LL_FACE_OUTER_SIDE_2	= 0x1 << 7;
const LLFaceID	LL_FACE_OUTER_SIDE_3	= 0x1 << 8;

//============================================================================

// sculpt types + flags

const U8 LL_SCULPT_TYPE_NONE      = 0;
const U8 LL_SCULPT_TYPE_SPHERE    = 1;
const U8 LL_SCULPT_TYPE_TORUS     = 2;
const U8 LL_SCULPT_TYPE_PLANE     = 3;
const U8 LL_SCULPT_TYPE_CYLINDER  = 4;
const U8 LL_SCULPT_TYPE_MESH      = 5;
const U8 LL_SCULPT_TYPE_MASK      = LL_SCULPT_TYPE_SPHERE | LL_SCULPT_TYPE_TORUS | LL_SCULPT_TYPE_PLANE |
	LL_SCULPT_TYPE_CYLINDER | LL_SCULPT_TYPE_MESH;

const U8 LL_SCULPT_FLAG_INVERT    = 64;
const U8 LL_SCULPT_FLAG_MIRROR    = 128;

const S32 LL_SCULPT_MESH_MAX_FACES = 8;

class LLProfileParams
{
public:
	LLProfileParams()
		: mCurveType(LL_PCODE_PROFILE_SQUARE),
		  mBegin(0.f),
		  mEnd(1.f),
		  mHollow(0.f),
		  mCRC(0)
	{
	}

	LLProfileParams(U8 curve, F32 begin, F32 end, F32 hollow)
		: mCurveType(curve),
		  mBegin(begin),
		  mEnd(end),
		  mHollow(hollow),
		  mCRC(0)
	{
	}

	LLProfileParams(U8 curve, U16 begin, U16 end, U16 hollow)
	{
		mCurveType = curve;
		F32 temp_f32 = begin * CUT_QUANTA;
		if (temp_f32 > 1.f)
		{
			temp_f32 = 1.f;
		}
		mBegin = temp_f32;
		temp_f32 = end * CUT_QUANTA;
		if (temp_f32 > 1.f)
		{
			temp_f32 = 1.f;
		}
		mEnd = 1.f - temp_f32;
		temp_f32 = hollow * HOLLOW_QUANTA;
		if (temp_f32 > 1.f)
		{
			temp_f32 = 1.f;
		}
		mHollow = temp_f32;
		mCRC = 0;
	}

	bool operator==(const LLProfileParams &params) const;
	bool operator!=(const LLProfileParams &params) const;
	bool operator<(const LLProfileParams &params) const;
	
	void copyParams(const LLProfileParams &params);

	BOOL importFile(LLFILE *fp);
	BOOL exportFile(LLFILE *fp) const;

	BOOL importLegacyStream(std::istream& input_stream);
	BOOL exportLegacyStream(std::ostream& output_stream) const;

	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	const F32&  getBegin () const				{ return mBegin; }
	const F32&  getEnd   () const				{ return mEnd;   }
	const F32&  getHollow() const				{ return mHollow; }
	const U8&   getCurveType () const			{ return mCurveType; }

	void setCurveType(const U32 type)			{ mCurveType = type;}
	void setBegin(const F32 begin)				{ mBegin = (begin >= 1.0f) ? 0.0f : ((int) (begin * 100000))/100000.0f;}
	void setEnd(const F32 end)					{ mEnd   = (end   <= 0.0f) ? 1.0f : ((int) (end * 100000))/100000.0f;}
	void setHollow(const F32 hollow)			{ mHollow = ((int) (hollow * 100000))/100000.0f;}

	friend std::ostream& operator<<(std::ostream &s, const LLProfileParams &profile_params);

protected:
	// Profile params
	U8			  mCurveType;
	F32           mBegin;
	F32           mEnd;
	F32			  mHollow;

	U32           mCRC;
};

inline bool LLProfileParams::operator==(const LLProfileParams &params) const
{
	return 
		(getCurveType() == params.getCurveType()) &&
		(getBegin() == params.getBegin()) &&
		(getEnd() == params.getEnd()) &&
		(getHollow() == params.getHollow());
}

inline bool LLProfileParams::operator!=(const LLProfileParams &params) const
{
	return 
		(getCurveType() != params.getCurveType()) ||
		(getBegin() != params.getBegin()) ||
		(getEnd() != params.getEnd()) ||
		(getHollow() != params.getHollow());
}


inline bool LLProfileParams::operator<(const LLProfileParams &params) const
{
	if (getCurveType() != params.getCurveType())
	{
		return getCurveType() < params.getCurveType();
	}
	else
	if (getBegin() != params.getBegin())
	{
		return getBegin() < params.getBegin();
	}
	else
	if (getEnd() != params.getEnd())
	{
		return getEnd() < params.getEnd();
	}
	else
	{
		return getHollow() < params.getHollow();
	}
}

#define U8_TO_F32(x) (F32)(*((S8 *)&x))

class LLPathParams
{
public:
	LLPathParams()
		:
		mCurveType(LL_PCODE_PATH_LINE),
		mBegin(0.f),
		mEnd(1.f),
		mScale(1.f,1.f),
		mShear(0.f,0.f),
		mTwistBegin(0.f),
		mTwistEnd(0.f),
		mRadiusOffset(0.f),
		mTaper(0.f,0.f),
		mRevolutions(1.f),
		mSkew(0.f),
		mCRC(0)
	{
	}

	LLPathParams(U8 curve, F32 begin, F32 end, F32 scx, F32 scy, F32 shx, F32 shy, F32 twistend, F32 twistbegin, F32 radiusoffset, F32 tx, F32 ty, F32 revolutions, F32 skew)
		: mCurveType(curve),
		  mBegin(begin),
		  mEnd(end),
		  mScale(scx,scy),
		  mShear(shx,shy),
		  mTwistBegin(twistbegin),
		  mTwistEnd(twistend), 
		  mRadiusOffset(radiusoffset),
		  mTaper(tx,ty),
		  mRevolutions(revolutions),
		  mSkew(skew),
		  mCRC(0)
	{
	}

	LLPathParams(U8 curve, U16 begin, U16 end, U8 scx, U8 scy, U8 shx, U8 shy, U8 twistend, U8 twistbegin, U8 radiusoffset, U8 tx, U8 ty, U8 revolutions, U8 skew)
	{
		mCurveType = curve;
		mBegin = (F32)(begin * CUT_QUANTA);
		mEnd = (F32)(100.f - end) * CUT_QUANTA;
		if (mEnd > 1.f)
			mEnd = 1.f;
		mScale.setVec((F32) (200 - scx) * SCALE_QUANTA,(F32) (200 - scy) * SCALE_QUANTA);
		mShear.setVec(U8_TO_F32(shx) * SHEAR_QUANTA,U8_TO_F32(shy) * SHEAR_QUANTA);
		mTwistBegin = U8_TO_F32(twistbegin) * SCALE_QUANTA;
		mTwistEnd = U8_TO_F32(twistend) * SCALE_QUANTA;
		mRadiusOffset = U8_TO_F32(radiusoffset) * SCALE_QUANTA;
		mTaper.setVec(U8_TO_F32(tx) * TAPER_QUANTA,U8_TO_F32(ty) * TAPER_QUANTA);
		mRevolutions = ((F32)revolutions) * REV_QUANTA + 1.0f;
		mSkew = U8_TO_F32(skew) * SCALE_QUANTA;

		mCRC = 0;
	}

	bool operator==(const LLPathParams &params) const;
	bool operator!=(const LLPathParams &params) const;
	bool operator<(const LLPathParams &params) const;

	void copyParams(const LLPathParams &params);

	BOOL importFile(LLFILE *fp);
	BOOL exportFile(LLFILE *fp) const;

	BOOL importLegacyStream(std::istream& input_stream);
	BOOL exportLegacyStream(std::ostream& output_stream) const;

	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	const F32& getBegin() const			{ return mBegin; }
	const F32& getEnd() const			{ return mEnd; }
	const LLVector2 &getScale() const	{ return mScale; }
	const F32& getScaleX() const		{ return mScale.mV[0]; }
	const F32& getScaleY() const		{ return mScale.mV[1]; }
	const LLVector2 getBeginScale() const;
	const LLVector2 getEndScale() const;
	const LLVector2 &getShear() const	{ return mShear; }
	const F32& getShearX() const		{ return mShear.mV[0]; }
	const F32& getShearY() const		{ return mShear.mV[1]; }
	const U8& getCurveType () const		{ return mCurveType; }

	const F32& getTwistBegin() const	{ return mTwistBegin;	}
	const F32& getTwistEnd() const		{ return mTwistEnd;	}
	const F32& getTwist() const			{ return mTwistEnd; }	// deprecated
	const F32& getRadiusOffset() const	{ return mRadiusOffset; }
	const LLVector2 &getTaper() const	{ return mTaper;		}
	const F32& getTaperX() const		{ return mTaper.mV[0];	}
	const F32& getTaperY() const		{ return mTaper.mV[1];	}
	const F32& getRevolutions() const	{ return mRevolutions;	}
	const F32& getSkew() const			{ return mSkew;			}

	void setCurveType(const U8 type)	{ mCurveType = type;	}
	void setBegin(const F32 begin)		{ mBegin     = begin;	}
	void setEnd(const F32 end)			{ mEnd       = end;	}

	void setScale(const F32 x, const F32 y)		{ mScale.setVec(x,y); }
	void setScaleX(const F32 v)					{ mScale.mV[VX] = v; }
	void setScaleY(const F32 v)					{ mScale.mV[VY] = v; }
	void setShear(const F32 x, const F32 y)		{ mShear.setVec(x,y); }
	void setShearX(const F32 v)					{ mShear.mV[VX] = v; }
	void setShearY(const F32 v)					{ mShear.mV[VY] = v; }

	void setTwistBegin(const F32 twist_begin)	{ mTwistBegin	= twist_begin;	}
	void setTwistEnd(const F32 twist_end)		{ mTwistEnd		= twist_end;	}
	void setTwist(const F32 twist)				{ setTwistEnd(twist); }	// deprecated
	void setRadiusOffset(const F32 radius_offset){ mRadiusOffset	= radius_offset; }
	void setTaper(const F32 x, const F32 y)		{ mTaper.setVec(x,y);			}
	void setTaperX(const F32 v)					{ mTaper.mV[VX]	= v;			}
	void setTaperY(const F32 v)					{ mTaper.mV[VY]	= v;			}
	void setRevolutions(const F32 revolutions)	{ mRevolutions	= revolutions;	}
	void setSkew(const F32 skew)				{ mSkew			= skew;			}

	friend std::ostream& operator<<(std::ostream &s, const LLPathParams &path_params);

protected:
	// Path params
	U8			  mCurveType;
	F32           mBegin;
	F32           mEnd;
	LLVector2	  mScale;
	LLVector2     mShear;

	F32			  mTwistBegin;
	F32			  mTwistEnd;
	F32			  mRadiusOffset;
	LLVector2	  mTaper;
	F32			  mRevolutions;
	F32			  mSkew;

	U32           mCRC;
};

inline bool LLPathParams::operator==(const LLPathParams &params) const
{
	return
		(getCurveType() == params.getCurveType()) && 
		(getScale() == params.getScale()) &&
		(getBegin() == params.getBegin()) && 
		(getEnd() == params.getEnd()) && 
		(getShear() == params.getShear()) &&
		(getTwist() == params.getTwist()) &&
		(getTwistBegin() == params.getTwistBegin()) &&
		(getRadiusOffset() == params.getRadiusOffset()) &&
		(getTaper() == params.getTaper()) &&
		(getRevolutions() == params.getRevolutions()) &&
		(getSkew() == params.getSkew());
}

inline bool LLPathParams::operator!=(const LLPathParams &params) const
{
	return
		(getCurveType() != params.getCurveType()) ||
		(getScale() != params.getScale()) ||
		(getBegin() != params.getBegin()) || 
		(getEnd() != params.getEnd()) || 
		(getShear() != params.getShear()) ||
		(getTwist() != params.getTwist()) ||
		(getTwistBegin() !=params.getTwistBegin()) ||
		(getRadiusOffset() != params.getRadiusOffset()) ||
		(getTaper() != params.getTaper()) ||
		(getRevolutions() != params.getRevolutions()) ||
		(getSkew() != params.getSkew());
}


inline bool LLPathParams::operator<(const LLPathParams &params) const
{
	if( getCurveType() != params.getCurveType()) 
	{
		return getCurveType() < params.getCurveType();
	}
	else
	if( getScale() != params.getScale()) 
	{
		return getScale() < params.getScale();
	}
	else
	if( getBegin() != params.getBegin()) 
	{
		return getBegin() < params.getBegin();
	}
	else
	if( getEnd() != params.getEnd()) 
	{
		return getEnd() < params.getEnd();
	}
	else
	if( getShear() != params.getShear()) 
	{
		return getShear() < params.getShear();
	}
	else
	if( getTwist() != params.getTwist())
	{
		return getTwist() < params.getTwist();
	}
	else
	if( getTwistBegin() != params.getTwistBegin())
	{
		return getTwistBegin() < params.getTwistBegin();
	}
	else
	if( getRadiusOffset() != params.getRadiusOffset())
	{
		return getRadiusOffset() < params.getRadiusOffset();
	}
	else
	if( getTaper() != params.getTaper())
	{
		return getTaper() < params.getTaper();
	}
	else
	if( getRevolutions() != params.getRevolutions())
	{
		return getRevolutions() < params.getRevolutions();
	}
	else
	{
		return getSkew() < params.getSkew();
	}
}

typedef LLVolumeParams* LLVolumeParamsPtr;
typedef const LLVolumeParams* const_LLVolumeParamsPtr;

class LLVolumeParams
{
public:
	LLVolumeParams()
		: mSculptType(LL_SCULPT_TYPE_NONE)
	{
	}

	LLVolumeParams(LLProfileParams &profile, LLPathParams &path,
				   LLUUID sculpt_id = LLUUID::null, U8 sculpt_type = LL_SCULPT_TYPE_NONE)
		: mProfileParams(profile), mPathParams(path), mSculptID(sculpt_id), mSculptType(sculpt_type)
	{
	}

	bool operator==(const LLVolumeParams &params) const;
	bool operator!=(const LLVolumeParams &params) const;
	bool operator<(const LLVolumeParams &params) const;


	void copyParams(const LLVolumeParams &params);
	
	const LLProfileParams &getProfileParams() const {return mProfileParams;}
	LLProfileParams &getProfileParams() {return mProfileParams;}
	const LLPathParams &getPathParams() const {return mPathParams;}
	LLPathParams &getPathParams() {return mPathParams;}

	BOOL importFile(LLFILE *fp);
	BOOL exportFile(LLFILE *fp) const;

	BOOL importLegacyStream(std::istream& input_stream);
	BOOL exportLegacyStream(std::ostream& output_stream) const;

	LLSD sculptAsLLSD() const;
	bool sculptFromLLSD(LLSD& sd);
	
	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	bool setType(U8 profile, U8 path);

	//void setBeginS(const F32 beginS)			{ mProfileParams.setBegin(beginS); }	// range 0 to 1
	//void setBeginT(const F32 beginT)			{ mPathParams.setBegin(beginT); }		// range 0 to 1
	//void setEndS(const F32 endS)				{ mProfileParams.setEnd(endS); }		// range 0 to 1, must be greater than begin
	//void setEndT(const F32 endT)				{ mPathParams.setEnd(endT); }			// range 0 to 1, must be greater than begin

	bool setBeginAndEndS(const F32 begin, const F32 end);			// both range from 0 to 1, begin must be less than end
	bool setBeginAndEndT(const F32 begin, const F32 end);			// both range from 0 to 1, begin must be less than end

	bool setHollow(const F32 hollow);	// range 0 to 1
	bool setRatio(const F32 x)					{ return setRatio(x,x); }			// 0 = point, 1 = same as base
	bool setShear(const F32 x)					{ return setShear(x,x); }			// 0 = no movement, 
	bool setRatio(const F32 x, const F32 y);			// 0 = point, 1 = same as base
	bool setShear(const F32 x, const F32 y);			// 0 = no movement

	bool setTwistBegin(const F32 twist_begin);	// range -1 to 1
	bool setTwistEnd(const F32 twist_end);		// range -1 to 1
	bool setTwist(const F32 twist)				{ return setTwistEnd(twist); }		// deprecated
	bool setTaper(const F32 x, const F32 y)		{ bool pass_x = setTaperX(x); bool pass_y = setTaperY(y); return pass_x && pass_y; }
	bool setTaperX(const F32 v);				// -1 to 1
	bool setTaperY(const F32 v);				// -1 to 1
	bool setRevolutions(const F32 revolutions);	// 1 to 4
	bool setRadiusOffset(const F32 radius_offset);
	bool setSkew(const F32 skew);
	bool setSculptID(const LLUUID sculpt_id, U8 sculpt_type);

	static bool validate(U8 prof_curve, F32 prof_begin, F32 prof_end, F32 hollow,
		U8 path_curve, F32 path_begin, F32 path_end,
		F32 scx, F32 scy, F32 shx, F32 shy,
		F32 twistend, F32 twistbegin, F32 radiusoffset,
		F32 tx, F32 ty, F32 revolutions, F32 skew);
	
	const F32&  getBeginS() 	const	{ return mProfileParams.getBegin(); }
 	const F32&  getBeginT() 	const	{ return mPathParams.getBegin(); }
 	const F32&  getEndS() 		const	{ return mProfileParams.getEnd(); }
 	const F32&  getEndT() 		const	{ return mPathParams.getEnd(); }
 
 	const F32&  getHollow() 	const   { return mProfileParams.getHollow(); }
 	const F32&  getTwist() 	const   	{ return mPathParams.getTwist(); }
 	const F32&  getRatio() 	const		{ return mPathParams.getScaleX(); }
 	const F32&  getRatioX() 	const   { return mPathParams.getScaleX(); }
 	const F32&  getRatioY() 	const   { return mPathParams.getScaleY(); }
 	const F32&  getShearX() 	const   { return mPathParams.getShearX(); }
 	const F32&  getShearY() 	const   { return mPathParams.getShearY(); }

	const F32&	getTwistBegin()const	{ return mPathParams.getTwistBegin();	}
	const F32&  getRadiusOffset() const	{ return mPathParams.getRadiusOffset();	}
	const F32&  getTaper() const		{ return mPathParams.getTaperX();		}
	const F32&	getTaperX() const		{ return mPathParams.getTaperX();		}
	const F32&  getTaperY() const		{ return mPathParams.getTaperY();		}
	const F32&  getRevolutions() const	{ return mPathParams.getRevolutions();	}
	const F32&  getSkew() const			{ return mPathParams.getSkew();			}
	const LLUUID& getSculptID() const	{ return mSculptID;						}
	const U8& getSculptType() const     { return mSculptType;                   }
	bool isSculpt() const;
	bool isMeshSculpt() const;
	BOOL isConvex() const;

	// 'begin' and 'end' should be in range [0, 1] (they will be clamped)
	// (begin, end) = (0, 1) will not change the volume
	// (begin, end) = (0, 0.5) will reduce the volume to the first half of its profile/path (S/T)
	void reduceS(F32 begin, F32 end);
	void reduceT(F32 begin, F32 end);

	struct compare
	{
		bool operator()( const const_LLVolumeParamsPtr& first, const const_LLVolumeParamsPtr& second) const
		{
			return (*first < *second);
		}
	};
	
	friend std::ostream& operator<<(std::ostream &s, const LLVolumeParams &volume_params);

	// debug helper functions
	void setCube();

protected:
	LLProfileParams mProfileParams;
	LLPathParams	mPathParams;
	LLUUID mSculptID;
	U8 mSculptType;
};


class LLProfile
{
public:
	LLProfile()
		: mOpen(FALSE),
		  mConcave(FALSE),
		  mDirty(TRUE),
		  mTotalOut(0),
		  mTotal(2)
	{
	}

	~LLProfile();

	S32	 getTotal() const								{ return mTotal; }
	S32	 getTotalOut() const							{ return mTotalOut; }	// Total number of outside points
	BOOL isFlat(S32 face) const							{ return (mFaces[face].mCount == 2); }
	BOOL isOpen() const									{ return mOpen; }
	void setDirty()										{ mDirty     = TRUE; }

	static S32 getNumPoints(const LLProfileParams& params, BOOL path_open, F32 detail = 1.0f, S32 split = 0,
				  BOOL is_sculpted = FALSE, S32 sculpt_size = 0);
	BOOL generate(const LLProfileParams& params, BOOL path_open, F32 detail = 1.0f, S32 split = 0,
				  BOOL is_sculpted = FALSE, S32 sculpt_size = 0);
	BOOL isConcave() const								{ return mConcave; }
public:
	struct Face
	{
		S32       mIndex;
		S32       mCount;
		F32       mScaleU;
		BOOL      mCap;
		BOOL      mFlat;
		LLFaceID  mFaceID;
	};
	
	std::vector<LLVector3> mProfile;	
	std::vector<LLVector2> mNormals;
	std::vector<Face>      mFaces;
	std::vector<LLVector3> mEdgeNormals;
	std::vector<LLVector3> mEdgeCenters;

	friend std::ostream& operator<<(std::ostream &s, const LLProfile &profile);

protected:
	void genNormals(const LLProfileParams& params);
	static S32 getNumNGonPoints(const LLProfileParams& params, S32 sides, F32 offset=0.0f, F32 bevel = 0.0f, F32 ang_scale = 1.f, S32 split = 0);
	void genNGon(const LLProfileParams& params, S32 sides, F32 offset=0.0f, F32 bevel = 0.0f, F32 ang_scale = 1.f, S32 split = 0);

	Face* addHole(const LLProfileParams& params, BOOL flat, F32 sides, F32 offset, F32 box_hollow, F32 ang_scale, S32 split = 0);
	Face* addCap (S16 faceID);
	Face* addFace(S32 index, S32 count, F32 scaleU, S16 faceID, BOOL flat);

protected:
	BOOL		  mOpen;
	BOOL		  mConcave;
	BOOL          mDirty;

	S32			  mTotalOut;
	S32			  mTotal;
};

//-------------------------------------------------------------------
// SWEEP/EXTRUDE PATHS
//-------------------------------------------------------------------

class LLPath
{
public:
	struct PathPt
	{
		LLVector3	 mPos;
		LLVector2    mScale;
		LLQuaternion mRot;
		F32			 mTexT;
		PathPt() { mPos.setVec(0,0,0); mTexT = 0; mScale.setVec(0,0); mRot.loadIdentity(); }
	};

public:
	LLPath()
		: mOpen(FALSE),
		  mTotal(0),
		  mDirty(TRUE),
		  mStep(1)
	{
	}

	virtual ~LLPath();

	static S32 getNumPoints(const LLPathParams& params, F32 detail);
	static S32 getNumNGonPoints(const LLPathParams& params, S32 sides, F32 offset=0.0f, F32 end_scale = 1.f, F32 twist_scale = 1.f);

	void genNGon(const LLPathParams& params, S32 sides, F32 offset=0.0f, F32 end_scale = 1.f, F32 twist_scale = 1.f);
	virtual BOOL generate(const LLPathParams& params, F32 detail=1.0f, S32 split = 0,
						  BOOL is_sculpted = FALSE, S32 sculpt_size = 0);

	BOOL isOpen() const						{ return mOpen; }
	F32 getStep() const						{ return mStep; }
	void setDirty()							{ mDirty     = TRUE; }

	S32 getPathLength() const				{ return (S32)mPath.size(); }

	void resizePath(S32 length) { mPath.resize(length); }

	friend std::ostream& operator<<(std::ostream &s, const LLPath &path);

public:
	std::vector<PathPt> mPath;

protected:
	BOOL		  mOpen;
	S32			  mTotal;
	BOOL          mDirty;
	F32           mStep;
};

class LLDynamicPath : public LLPath
{
public:
	LLDynamicPath() : LLPath() { }
	/*virtual*/ BOOL generate(const LLPathParams& params, F32 detail=1.0f, S32 split = 0,
							  BOOL is_sculpted = FALSE, S32 sculpt_size = 0);
};

// Yet another "face" class - caches volume-specific, but not instance-specific data for faces)
class LLVolumeFace
{
public:
	class VertexData
	{
		enum 
		{
			POSITION = 0,
			NORMAL = 1
		};

	private:
		void init();
	public:
		VertexData();
		VertexData(const VertexData& rhs);
		const VertexData& operator=(const VertexData& rhs);

		~VertexData();
		LLVector4a& getPosition();
		LLVector4a& getNormal();
		const LLVector4a& getPosition() const;
		const LLVector4a& getNormal() const;
		void setPosition(const LLVector4a& pos);
		void setNormal(const LLVector4a& norm);
		

		LLVector2 mTexCoord;

		bool operator<(const VertexData& rhs) const;
		bool operator==(const VertexData& rhs) const;
		bool compareNormal(const VertexData& rhs, F32 angle_cutoff) const;

	private:
		LLVector4a* mData;
	};

	LLVolumeFace();
	LLVolumeFace(const LLVolumeFace& src);
	LLVolumeFace& operator=(const LLVolumeFace& rhs);

	~LLVolumeFace();
private:
	void freeData();
public:

	BOOL create(LLVolume* volume, BOOL partial_build = FALSE);
	void createBinormals();
	
	void appendFace(const LLVolumeFace& face, LLMatrix4& transform, LLMatrix4& normal_tranform);

	void resizeVertices(S32 num_verts);
	void allocateBinormals(S32 num_verts);
	void allocateWeights(S32 num_verts);
	void resizeIndices(S32 num_indices);
	void fillFromLegacyData(std::vector<LLVolumeFace::VertexData>& v, std::vector<U16>& idx);

	void pushVertex(const VertexData& cv);
	void pushVertex(const LLVector4a& pos, const LLVector4a& norm, const LLVector2& tc);
	void pushIndex(const U16& idx);

	void swapData(LLVolumeFace& rhs);

	void getVertexData(U16 indx, LLVolumeFace::VertexData& cv);

	class VertexMapData : public LLVolumeFace::VertexData
	{
	public:
		U16 mIndex;

		bool operator==(const LLVolumeFace::VertexData& rhs) const;

		struct ComparePosition
		{
			bool operator()(const LLVector3& a, const LLVector3& b) const;
		};

		typedef std::map<LLVector3, std::vector<VertexMapData>, VertexMapData::ComparePosition > PointMap;
	};

	void optimize(F32 angle_cutoff = 2.f);
	void cacheOptimize();

	void createOctree(F32 scaler = 0.25f, const LLVector4a& center = LLVector4a(0,0,0), const LLVector4a& size = LLVector4a(0.5f,0.5f,0.5f));

	enum
	{
		SINGLE_MASK =	0x0001,
		CAP_MASK =		0x0002,
		END_MASK =		0x0004,
		SIDE_MASK =		0x0008,
		INNER_MASK =	0x0010,
		OUTER_MASK =	0x0020,
		HOLLOW_MASK =	0x0040,
		OPEN_MASK =		0x0080,
		FLAT_MASK =		0x0100,
		TOP_MASK =		0x0200,
		BOTTOM_MASK =	0x0400
	};
	
public:
	S32 mID;
	U32 mTypeMask;
	
	// Only used for INNER/OUTER faces
	S32 mBeginS;
	S32 mBeginT;
	S32 mNumS;
	S32 mNumT;

	LLVector4a* mExtents; //minimum and maximum point of face
	LLVector4a* mCenter;
	LLVector2   mTexCoordExtents[2]; //minimum and maximum of texture coordinates of the face.

	S32 mNumVertices;
	S32 mNumIndices;

	LLVector4a* mPositions;
	LLVector4a* mNormals;
	LLVector4a* mBinormals;
	LLVector2*  mTexCoords;
	U16* mIndices;

	//vertex buffer filled in by LLFace to cache this volume face geometry in vram 
	// (declared as a LLPointer to LLRefCount to avoid dependency on LLVertexBuffer)
	mutable LLPointer<LLRefCount> mVertexBuffer; 

	std::vector<S32>	mEdge;

	//list of skin weights for rigged volumes
	// format is mWeights[vertex_index].mV[influence] = <joint_index>.<weight>
	// mWeights.size() should be empty or match mVertices.size()  
	LLVector4a* mWeights;

	LLOctreeNode<LLVolumeTriangle>* mOctree;

private:
	BOOL createUnCutCubeCap(LLVolume* volume, BOOL partial_build = FALSE);
	BOOL createCap(LLVolume* volume, BOOL partial_build = FALSE);
	BOOL createSide(LLVolume* volume, BOOL partial_build = FALSE);
};

class LLVolume : public LLRefCount
{
	friend class LLVolumeLODGroup;

protected:
	~LLVolume(); // use unref

public:
	struct Point
	{
		LLVector3 mPos;
	};

	struct FaceParams
	{
		LLFaceID mFaceID;
		S32 mBeginS;
		S32 mCountS;
		S32 mBeginT;
		S32 mCountT;
	};

	LLVolume(const LLVolumeParams &params, const F32 detail, const BOOL generate_single_face = FALSE, const BOOL is_unique = FALSE);
	
	U8 getProfileType()	const								{ return mParams.getProfileParams().getCurveType(); }
	U8 getPathType() const									{ return mParams.getPathParams().getCurveType(); }
	S32	getNumFaces() const;
	S32 getNumVolumeFaces() const							{ return mVolumeFaces.size(); }
	F32 getDetail() const									{ return mDetail; }
	F32 getSurfaceArea() const								{ return mSurfaceArea; }
	const LLVolumeParams& getParams() const					{ return mParams; }
	LLVolumeParams getCopyOfParams() const					{ return mParams; }
	const LLProfile& getProfile() const						{ return *mProfilep; }
	LLPath& getPath() const									{ return *mPathp; }
	void resizePath(S32 length);
	const std::vector<Point>& getMesh() const				{ return mMesh; }
	const LLVector3& getMeshPt(const U32 i) const			{ return mMesh[i].mPos; }

	void setDirty() { mPathp->setDirty(); mProfilep->setDirty(); }

	void regen();
	void genBinormals(S32 face);

	BOOL isConvex() const;
	BOOL isCap(S32 face);
	BOOL isFlat(S32 face);
	BOOL isUnique() const									{ return mUnique; }

	S32 getSculptLevel() const                              { return mSculptLevel; }
	void setSculptLevel(S32 level)							{ mSculptLevel = level; }

	S32 *getTriangleIndices(U32 &num_indices) const;

	// returns number of triangle indeces required for path/profile mesh
	S32 getNumTriangleIndices() const;
	static void getLoDTriangleCounts(const LLVolumeParams& params, S32* counts);

	S32 getNumTriangles(S32* vcount = NULL) const;

	void generateSilhouetteVertices(std::vector<LLVector3> &vertices, 
									std::vector<LLVector3> &normals, 
									const LLVector3& view_vec,
									const LLMatrix4& mat,
									const LLMatrix3& norm_mat,
									S32 face_index);

	//get the face index of the face that intersects with the given line segment at the point 
	//closest to start.  Moves end to the point of intersection.  Returns -1 if no intersection.
	//Line segment must be in volume space.
	S32 lineSegmentIntersect(const LLVector3& start, const LLVector3& end,
							 S32 face = -1,                          // which face to check, -1 = ALL_SIDES
							 LLVector3* intersection = NULL,         // return the intersection point
							 LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
							 LLVector3* normal = NULL,               // return the surface normal at the intersection point
							 LLVector3* bi_normal = NULL             // return the surface bi-normal at the intersection point
		);

	S32 lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, 
								   S32 face = 1,
								   LLVector3* intersection = NULL,
								   LLVector2* tex_coord = NULL,
								   LLVector3* normal = NULL,
								   LLVector3* bi_normal = NULL);
	
	// The following cleans up vertices and triangles,
	// getting rid of degenerate triangles and duplicate vertices,
	// and allocates new arrays with the clean data.
	static BOOL cleanupTriangleData( const S32 num_input_vertices,
								const std::vector<Point> &input_vertices,
								const S32 num_input_triangles,
								S32 *input_triangles,
								S32 &num_output_vertices,
								LLVector3 **output_vertices,
								S32 &num_output_triangles,
								S32 **output_triangles);
	LLFaceID generateFaceMask();

	BOOL isFaceMaskValid(LLFaceID face_mask);
	static S32 sNumMeshPoints;

	friend std::ostream& operator<<(std::ostream &s, const LLVolume &volume);
	friend std::ostream& operator<<(std::ostream &s, const LLVolume *volumep);		// HACK to bypass Windoze confusion over 
																				// conversion if *(LLVolume*) to LLVolume&
	const LLVolumeFace &getVolumeFace(const S32 f) const {return mVolumeFaces[f];} // DO NOT DELETE VOLUME WHILE USING THIS REFERENCE, OR HOLD A POINTER TO THIS VOLUMEFACE
	
	U32					mFaceMask;			// bit array of which faces exist in this volume
	LLVector3			mLODScaleBias;		// vector for biasing LOD based on scale
	
	void sculpt(U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data, S32 sculpt_level);
	void copyVolumeFaces(const LLVolume* volume);
	void cacheOptimize();

private:
	void sculptGenerateMapVertices(U16 sculpt_width, U16 sculpt_height, S8 sculpt_components, const U8* sculpt_data, U8 sculpt_type);
	F32 sculptGetSurfaceArea();
	void sculptGeneratePlaceholder();
	void sculptCalcMeshResolution(U16 width, U16 height, U8 type, S32& s, S32& t);

	
protected:
	BOOL generate();
	void createVolumeFaces();
public:
	virtual bool unpackVolumeFaces(std::istream& is, S32 size);

	virtual void setMeshAssetLoaded(BOOL loaded);
	virtual BOOL isMeshAssetLoaded();

 protected:
	BOOL mUnique;
	F32 mDetail;
	S32 mSculptLevel;
	F32 mSurfaceArea; //unscaled surface area
	BOOL mIsMeshAssetLoaded;
	
	LLVolumeParams mParams;
	LLPath *mPathp;
	LLProfile *mProfilep;
	std::vector<Point> mMesh;
	
	BOOL mGenerateSingleFace;
	typedef std::vector<LLVolumeFace> face_list_t;
	face_list_t mVolumeFaces;

public:
	LLVector4a* mHullPoints;
	U16* mHullIndices;
	S32 mNumHullPoints;
	S32 mNumHullIndices;
};

std::ostream& operator<<(std::ostream &s, const LLVolumeParams &volume_params);

void calc_binormal_from_triangle(
		LLVector4a& binormal,
		const LLVector4a& pos0,
		const LLVector2& tex0,
		const LLVector4a& pos1,
		const LLVector2& tex1,
		const LLVector4a& pos2,
		const LLVector2& tex2);

BOOL LLLineSegmentBoxIntersect(const F32* start, const F32* end, const F32* center, const F32* size);
BOOL LLLineSegmentBoxIntersect(const LLVector3& start, const LLVector3& end, const LLVector3& center, const LLVector3& size);
BOOL LLLineSegmentBoxIntersect(const LLVector4a& start, const LLVector4a& end, const LLVector4a& center, const LLVector4a& size);

BOOL LLTriangleRayIntersect(const LLVector3& vert0, const LLVector3& vert1, const LLVector3& vert2, const LLVector3& orig, const LLVector3& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t, BOOL two_sided);

BOOL LLTriangleRayIntersect(const LLVector4a& vert0, const LLVector4a& vert1, const LLVector4a& vert2, const LLVector4a& orig, const LLVector4a& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t);
BOOL LLTriangleRayIntersectTwoSided(const LLVector4a& vert0, const LLVector4a& vert1, const LLVector4a& vert2, const LLVector4a& orig, const LLVector4a& dir,
							F32& intersection_a, F32& intersection_b, F32& intersection_t);
	
	

#endif
