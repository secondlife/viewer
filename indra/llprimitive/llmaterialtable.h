/** 
 * @file llmaterialtable.h
 * @brief Table of material information for the viewer UI
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMATERIALTABLE_H
#define LL_LLMATERIALTABLE_H

#include "lluuid.h"
#include "linked_lists.h"
#include "llstring.h"

const U32 LLMATERIAL_INFO_NAME_LENGTH = 256;

class LLMaterialInfo
{
public:
	U8		    mMCode;
	char		mName[LLMATERIAL_INFO_NAME_LENGTH];
	LLUUID		mDefaultTextureID;
	LLUUID		mShatterSoundID;
	F32         mDensity;           // kg/m^3
	F32         mFriction;
	F32         mRestitution;

	// damage and energy constants
	F32			mHPModifier;		// modifier on mass based HP total
	F32			mDamageModifier;	// modifier on KE based damage
	F32			mEPModifier;		// modifier on mass based EP total

	LLMaterialInfo(U8 mcode, char* name, const LLUUID &uuid)
	{
		init(mcode,name,uuid);
	};

	void init(U8 mcode, char* name, const LLUUID &uuid)
	{
		mName[0] = 0;
		mDensity = 1000.f;             // default to 1000.0 (water)
		mHPModifier = 1.f;
		mDamageModifier = 1.f;
		mEPModifier = 1.f;

		mMCode = mcode;
		if (name)
		{
			LLString::copy(mName,name,LLMATERIAL_INFO_NAME_LENGTH);
		}
		mDefaultTextureID = uuid;		
	};

	~LLMaterialInfo()
	{
	};

};

class LLMaterialTable
{
public:
	LLLinkedList<LLMaterialInfo>	mMaterialInfoList;
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

	BOOL add(U8 mcode, char* name, const LLUUID &uuid);	                 
	BOOL addCollisionSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addSlidingSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addRollingSound(U8 mcode, U8 mcode2, const LLUUID &uuid);
	BOOL addShatterSound(U8 mcode, const LLUUID &uuid);
	BOOL addDensity(U8 mcode, const F32 &density);
	BOOL addFriction(U8 mcode, const F32 &friction);
	BOOL addRestitution(U8 mcode, const F32 &restitution);
	BOOL addDamageAndEnergy(U8 mcode, const F32 &hp_mod, const F32 &damage_mod, const F32 &ep_mod);

	LLUUID getDefaultTextureID(char* name);					// LLUUID::null if not found
	LLUUID getDefaultTextureID(U8 mcode);					// LLUUID::null if not found
	U8     getMCode(const char* name);						// 0 if not found
	char*  getName(U8 mcode);

	F32 getDensity(U8 mcode);						        // kg/m^3, 0 if not found
	F32 getFriction(U8 mcode);						        // havok values
	F32 getRestitution(U8 mcode);						    // havok values
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

#endif

