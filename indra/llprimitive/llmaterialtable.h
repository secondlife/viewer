/** 
 * @file llmaterialtable.h
 * @brief Table of material information for the viewer UI
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

#ifndef LL_LLMATERIALTABLE_H
#define LL_LLMATERIALTABLE_H

#include "lluuid.h"
#include "llstring.h"

#include <list>

class LLMaterialInfo;

const U32 LLMATERIAL_INFO_NAME_LENGTH = 256;

// We've moved toward more reasonable mass values for the Havok4 engine.
// The LEGACY_DEFAULT_OBJECT_DENSITY is used to maintain support for
// legacy scripts code (llGetMass()) and script energy consumption.
const F32 DEFAULT_OBJECT_DENSITY = 1000.0f;			// per m^3
const F32 LEGACY_DEFAULT_OBJECT_DENSITY = 10.0f;

// Avatars density depends on the collision shape used.  The approximate
// legacy volumes of avatars are:
// Body_Length Body_Width Body_Fat Leg_Length  Volume(m^3)
// -------------------------------------------------------
//    min     |   min    |  min   |    min    |   0.123   |
//    max     |   max    |  max   |    max    |   0.208   |
//
// Either the avatar shape must be tweaked to match those volumes
// or the DEFAULT_AVATAR_DENSITY must be adjusted to achieve the 
// legacy mass.
//
// The current density appears to be low because the mass and
// inertia are computed as if the avatar were a cylinder which
// has more volume than the actual collision shape of the avatar.
// See the physics engine mass properties code for more info.
const F32 DEFAULT_AVATAR_DENSITY = 445.3f;		// was 444.24f;


class LLMaterialTable
{
public:
	static const F32 FRICTION_MIN;
	static const F32 FRICTION_GLASS;
	static const F32 FRICTION_LIGHT;
	static const F32 FRICTION_METAL;
	static const F32 FRICTION_PLASTIC;
	static const F32 FRICTION_WOOD;
	static const F32 FRICTION_LAND;
	static const F32 FRICTION_STONE;
	static const F32 FRICTION_FLESH;
	static const F32 FRICTION_RUBBER;
	static const F32 FRICTION_MAX;

	static const F32 RESTITUTION_MIN;
	static const F32 RESTITUTION_LAND;
	static const F32 RESTITUTION_FLESH;
	static const F32 RESTITUTION_STONE;
	static const F32 RESTITUTION_METAL;
	static const F32 RESTITUTION_WOOD;
	static const F32 RESTITUTION_GLASS;
	static const F32 RESTITUTION_PLASTIC;
	static const F32 RESTITUTION_LIGHT;
	static const F32 RESTITUTION_RUBBER;
	static const F32 RESTITUTION_MAX;

	typedef std::list<LLMaterialInfo*> info_list_t;
	info_list_t mMaterialInfoList;

	LLUUID *mCollisionSoundMatrix;
	LLUUID *mSlidingSoundMatrix;
	LLUUID *mRollingSoundMatrix;

	static const F32 DEFAULT_FRICTION;
	static const F32 DEFAULT_RESTITUTION;

public:
	LLMaterialTable();
	LLMaterialTable(U8);  // cheat with an overloaded constructor to build our basic table
	~LLMaterialTable();

	void initBasicTable();

	void initTableTransNames(std::map<std::string, std::string> namemap);
	
	BOOL add(U8 mcode, const std::string& name, const LLUUID &uuid);	                 
	BOOL addCollisionSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addSlidingSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addRollingSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addShatterSound(U8 mcode, const LLUUID &uuid);
	BOOL addDensity(U8 mcode, const F32 &density);
	BOOL addFriction(U8 mcode, const F32 &friction);
	BOOL addRestitution(U8 mcode, const F32 &restitution);
	BOOL addDamageAndEnergy(U8 mcode, const F32 &hp_mod, const F32 &damage_mod, const F32 &ep_mod);

	LLUUID getDefaultTextureID(const std::string& name);					// LLUUID::null if not found
	LLUUID getDefaultTextureID(U8 mcode);					// LLUUID::null if not found
	U8     getMCode(const std::string& name);						// 0 if not found
	std::string getName(U8 mcode);

	F32 getDensity(U8 mcode);						        // kg/m^3, 0 if not found
	F32 getFriction(U8 mcode);						        // physics values
	F32 getRestitution(U8 mcode);						    // physics values
	F32 getHPMod(U8 mcode);
	F32 getDamageMod(U8 mcode);
	F32 getEPMod(U8 mcode);

	LLUUID getCollisionSoundUUID(U8 mcode, U8 mcode2); 
	LLUUID getSlidingSoundUUID(U8 mcode, U8 mcode2); 
	LLUUID getRollingSoundUUID(U8 mcode, U8 mcode2); 
	LLUUID getShatterSoundUUID(U8 mcode);					// LLUUID::null if not found

	LLUUID getGroundCollisionSoundUUID(U8 mcode);
	LLUUID getGroundSlidingSoundUUID(U8 mcode);
	LLUUID getGroundRollingSoundUUID(U8 mcode);
	LLUUID getCollisionParticleUUID(U8 mcode, U8 mcode2);
	LLUUID getGroundCollisionParticleUUID(U8 mcode);

	static LLMaterialTable basic;
};


class LLMaterialInfo
{
public:
	U8		    mMCode;
	std::string	mName;
	LLUUID		mDefaultTextureID;
	LLUUID		mShatterSoundID;
	F32         mDensity;           // kg/m^3
	F32         mFriction;
	F32         mRestitution;

	// damage and energy constants
	F32			mHPModifier;		// modifier on mass based HP total
	F32			mDamageModifier;	// modifier on KE based damage
	F32			mEPModifier;		// modifier on mass based EP total

	LLMaterialInfo(U8 mcode, const std::string& name, const LLUUID &uuid)
	{
		init(mcode,name,uuid);
	};

	void init(U8 mcode, const std::string& name, const LLUUID &uuid)
	{
		mDensity = 1000.f;             // default to 1000.0 (water)
		mFriction = LLMaterialTable::DEFAULT_FRICTION;
		mRestitution = LLMaterialTable::DEFAULT_RESTITUTION;
		mHPModifier = 1.f;
		mDamageModifier = 1.f;
		mEPModifier = 1.f;

		mMCode = mcode;
		mName = name;
		mDefaultTextureID = uuid;		
	};

	~LLMaterialInfo()
	{
	};

};

#endif

