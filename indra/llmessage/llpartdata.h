/** 
 * @file llpartdata.h
 * @brief Particle system data packing
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLPARTDATA_H
#define LL_LLPARTDATA_H

#include "lluuid.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v2math.h"
#include "v4color.h"

class LLMessageSystem;
class LLDataPacker;

const S32 PS_CUR_VERSION = 18;

//
// These constants are used by the script code, not by the particle system itself
//

enum LLPSScriptFlags
{
	// Flags for the different parameters of individual particles
	LLPS_PART_FLAGS,
	LLPS_PART_START_COLOR,
	LLPS_PART_START_ALPHA,
	LLPS_PART_END_COLOR,
	LLPS_PART_END_ALPHA,
	LLPS_PART_START_SCALE,
	LLPS_PART_END_SCALE,
	LLPS_PART_MAX_AGE,

	// Flags for the different parameters of the particle source
	LLPS_SRC_ACCEL,
	LLPS_SRC_PATTERN,
	LLPS_SRC_INNERANGLE,
	LLPS_SRC_OUTERANGLE,
	LLPS_SRC_TEXTURE,
	LLPS_SRC_BURST_RATE,
	LLPS_SRC_BURST_DURATION,
	LLPS_SRC_BURST_PART_COUNT,
	LLPS_SRC_BURST_RADIUS,
	LLPS_SRC_BURST_SPEED_MIN,
	LLPS_SRC_BURST_SPEED_MAX,
	LLPS_SRC_MAX_AGE,
	LLPS_SRC_TARGET_UUID,
	LLPS_SRC_OMEGA,
	LLPS_SRC_ANGLE_BEGIN,
	LLPS_SRC_ANGLE_END
};


class LLPartData
{
public:
	LLPartData() :
		mFlags(0),
		mMaxAge(0.f),
		mParameter(0.f)
	{
	}
	BOOL unpack(LLDataPacker &dp);
	BOOL pack(LLDataPacker &dp);
	LLSD asLLSD() const;
	operator LLSD() const {return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	// Masks for the different particle flags
	enum
	{
		LL_PART_INTERP_COLOR_MASK =		0x01,
		LL_PART_INTERP_SCALE_MASK =		0x02,
		LL_PART_BOUNCE_MASK =			0x04,
		LL_PART_WIND_MASK =				0x08,
		LL_PART_FOLLOW_SRC_MASK =		0x10,		// Follows source, no rotation following (expensive!)
		LL_PART_FOLLOW_VELOCITY_MASK =	0x20,		// Particles orient themselves with velocity
		LL_PART_TARGET_POS_MASK =		0x40,
		LL_PART_TARGET_LINEAR_MASK =	0x80,		// Particle uses a direct linear interpolation
		LL_PART_EMISSIVE_MASK =			0x100,		// Particle is "emissive", instead of being lit
		LL_PART_BEAM_MASK =				0x200,		// Particle is a "beam" connecting source and target

		// Not implemented yet!
		//LL_PART_RANDOM_ACCEL_MASK =		0x100,		// Particles have random acceleration
		//LL_PART_RANDOM_VEL_MASK =		0x200,		// Particles have random velocity shifts"
		//LL_PART_TRAIL_MASK =			0x400,		// Particles have historical "trails"

		// Viewer side use only!
		LL_PART_DEAD_MASK =				0x80000000,
	};

	void setFlags(const U32 flags);
	void setMaxAge(const F32 max_age);
	void setStartScale(const F32 xs, F32 ys);
	void setEndScale(const F32 xs, F32 ys);
	void setStartColor(const LLVector3 &rgb);
	void setEndColor(const LLVector3 &rgb);
	void setStartAlpha(const F32 alpha);
	void setEndAlpha(const F32 alpha);


	friend class LLPartSysData;
	friend class LLViewerPartSourceScript;

	// These are public because I'm really lazy...
public:
	U32					mFlags;						// Particle state/interpolators in effect
	F32					mMaxAge;					// Maximum age of the particle
	LLColor4			mStartColor;				// Start color
	LLColor4			mEndColor;					// End color
	LLVector2			mStartScale;				// Start scale
	LLVector2			mEndScale;					// End scale

	LLVector3			mPosOffset;					// Offset from source if using FOLLOW_SOURCE
	F32					mParameter;					// A single floating point parameter
};


class LLPartSysData
{
public:
	LLPartSysData();

	BOOL unpack(LLDataPacker &dp);
	BOOL pack(LLDataPacker &dp);

	
	BOOL unpackBlock(const S32 block_num);
	BOOL packBlock();

	static BOOL packNull();
	static BOOL isNullPS(const S32 block_num); // Returns FALSE if this is a "NULL" particle system (i.e. no system)

	// Different masks for effects on the source
	enum
	{
		LL_PART_SRC_OBJ_REL_MASK		=	0x01,		// Accel and velocity for particles relative object rotation
		LL_PART_USE_NEW_ANGLE			=	0x02,		// Particles uses new 'correct' angle parameters.
	};

	// The different patterns for how particles are created
	enum
	{
		LL_PART_SRC_PATTERN_DROP =				0x01,
		LL_PART_SRC_PATTERN_EXPLODE =			0x02,
		// Not implemented fully yet
		LL_PART_SRC_PATTERN_ANGLE =				0x04,
		LL_PART_SRC_PATTERN_ANGLE_CONE =		0x08,
		LL_PART_SRC_PATTERN_ANGLE_CONE_EMPTY =	0x10,
	};


	void setBurstSpeedMin(const F32 spd) {	mBurstSpeedMin = llclamp(spd, -100.f, 100.f); }
	void setBurstSpeedMax(const F32 spd) {	mBurstSpeedMax = llclamp(spd, -100.f, 100.f); }
	void setBurstRadius(const F32 rad)	 {	mBurstRadius = llclamp(rad, 0.f, 50.f); }
	void setPartAccel(const LLVector3 &accel);
	void setUseNewAngle()	{ mFlags |=  LL_PART_USE_NEW_ANGLE; }
	void unsetUseNewAngle()	{ mFlags &= ~LL_PART_USE_NEW_ANGLE; }

	// Since the actual particle creation rate is
	// a combination of multiple parameters, we
	// need to clamp it using a separate method instead of an accessor.
	void clampSourceParticleRate();
	
	friend std::ostream&	 operator<<(std::ostream& s, const LLPartSysData &data);		// Stream a
	
public:
	// Public because I'm lazy....

	//
	// There are two kinds of data for the particle system
	// 1. Parameters which specify parameters of the source (mSource*)
	// 2. Parameters which specify parameters of the particles generated by the source (mPart*)
	//

	U32		mCRC;
	U32		mFlags;

	U8		mPattern;						// Pattern for particle velocity/output
	F32		mInnerAngle;					// Inner angle for PATTERN_ANGLE
	F32		mOuterAngle;					// Outer angle for PATTERN_ANGLE
	LLVector3 mAngularVelocity;				// Angular velocity for emission axis (for PATTERN_ANGLE)

	F32		mBurstRate;						// How often to do a burst of particles
	U8		mBurstPartCount;				// How many particles in a burst
	F32		mBurstRadius;
	F32		mBurstSpeedMin;					// Minimum particle velocity
	F32		mBurstSpeedMax;					// Maximum particle velocity

	F32		mMaxAge;						// Maximum lifetime of this particle source

	LLUUID	mTargetUUID;					// Target UUID for the particle system

	F32		mStartAge;						// Age at which to start the particle system (for an update after the
											// particle system has started)


	//
	// These are actually particle properties, but can be mutated by the source,
	// so are stored here instead
	//
	LLVector3	mPartAccel;
	LLUUID		mPartImageID;

	//
	// The "template" partdata where we actually store the non-mutable particle parameters
	//
	LLPartData	mPartData;

protected:
	S32		mNumParticles;					// Number of particles generated
};

#endif // LL_LLPARTDATA_H
