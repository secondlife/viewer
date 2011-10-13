/** 
 * @file llmaterialtable.cpp
 * @brief Table of material names and IDs for viewer
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

#include "llmaterialtable.h"
#include "llstl.h"
#include "material_codes.h"
#include "sound_ids.h"
#include "imageids.h"

LLMaterialTable LLMaterialTable::basic(1);

/* 
	Old Havok 1 constants

// these are the approximately correct friction values for various materials
// however Havok1's friction dynamics are not very correct, so the effective
// friction coefficients that result from these numbers are approximately
// 25-50% too low, more incorrect for the lower values.
F32 const LLMaterialTable::FRICTION_MIN 	= 0.2f; 	
F32 const LLMaterialTable::FRICTION_GLASS 	= 0.2f; 	// borosilicate glass
F32 const LLMaterialTable::FRICTION_LIGHT 	= 0.2f; 	//
F32 const LLMaterialTable::FRICTION_METAL 	= 0.3f; 	// steel
F32 const LLMaterialTable::FRICTION_PLASTIC	= 0.4f; 	// HDPE
F32 const LLMaterialTable::FRICTION_WOOD 	= 0.6f; 	// southern pine
F32 const LLMaterialTable::FRICTION_FLESH 	= 0.60f; 	// saltwater
F32 const LLMaterialTable::FRICTION_LAND 	= 0.78f; 	// dirt
F32 const LLMaterialTable::FRICTION_STONE 	= 0.8f; 	// concrete
F32 const LLMaterialTable::FRICTION_RUBBER 	= 0.9f; 	//
F32 const LLMaterialTable::FRICTION_MAX 	= 0.95f; 	//
*/

// #if LL_CURRENT_HAVOK_VERSION == LL_HAVOK_VERSION_460
// Havok4 has more correct friction dynamics, however here we have to use
// the "incorrect" equivalents for the legacy Havok1 behavior
F32 const LLMaterialTable::FRICTION_MIN 	= 0.15f; 	
F32 const LLMaterialTable::FRICTION_GLASS 	= 0.13f; 	// borosilicate glass
F32 const LLMaterialTable::FRICTION_LIGHT 	= 0.14f; 	//
F32 const LLMaterialTable::FRICTION_METAL 	= 0.22f; 	// steel
F32 const LLMaterialTable::FRICTION_PLASTIC	= 0.3f; 	// HDPE
F32 const LLMaterialTable::FRICTION_WOOD 	= 0.44f; 	// southern pine
F32 const LLMaterialTable::FRICTION_FLESH 	= 0.46f; 	// saltwater
F32 const LLMaterialTable::FRICTION_LAND 	= 0.58f; 	// dirt
F32 const LLMaterialTable::FRICTION_STONE 	= 0.6f; 	// concrete
F32 const LLMaterialTable::FRICTION_RUBBER 	= 0.67f; 	//
F32 const LLMaterialTable::FRICTION_MAX 	= 0.71f; 	//
// #endif

F32 const LLMaterialTable::RESTITUTION_MIN 		= 0.02f; 	
F32 const LLMaterialTable::RESTITUTION_LAND 	= LLMaterialTable::RESTITUTION_MIN;
F32 const LLMaterialTable::RESTITUTION_FLESH 	= 0.2f; 	// saltwater
F32 const LLMaterialTable::RESTITUTION_STONE 	= 0.4f; 	// concrete
F32 const LLMaterialTable::RESTITUTION_METAL 	= 0.4f; 	// steel
F32 const LLMaterialTable::RESTITUTION_WOOD 	= 0.5f; 	// southern pine
F32 const LLMaterialTable::RESTITUTION_GLASS 	= 0.7f; 	// borosilicate glass
F32 const LLMaterialTable::RESTITUTION_PLASTIC	= 0.7f; 	// HDPE
F32 const LLMaterialTable::RESTITUTION_LIGHT 	= 0.7f; 	//
F32 const LLMaterialTable::RESTITUTION_RUBBER 	= 0.9f; 	//
F32 const LLMaterialTable::RESTITUTION_MAX		= 0.95f;

F32 const LLMaterialTable::DEFAULT_FRICTION = 0.5f;
F32 const LLMaterialTable::DEFAULT_RESTITUTION = 0.4f;

LLMaterialTable::LLMaterialTable()
	: mCollisionSoundMatrix(NULL),
	  mSlidingSoundMatrix(NULL),
	  mRollingSoundMatrix(NULL)
{
}

LLMaterialTable::LLMaterialTable(U8 isBasic)
{
	initBasicTable();
}

LLMaterialTable::~LLMaterialTable()
{
	if (mCollisionSoundMatrix)
	{
		delete [] mCollisionSoundMatrix;
		mCollisionSoundMatrix = NULL;
	}

	if (mSlidingSoundMatrix)
	{
		delete [] mSlidingSoundMatrix;
		mSlidingSoundMatrix = NULL;
	}

	if (mRollingSoundMatrix)
	{
		delete [] mRollingSoundMatrix;
		mRollingSoundMatrix = NULL;
	}

	for_each(mMaterialInfoList.begin(), mMaterialInfoList.end(), DeletePointer());
	mMaterialInfoList.clear();
}

void LLMaterialTable::initTableTransNames(std::map<std::string, std::string> namemap)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		std::string name = infop->mName;
		infop->mName = namemap[name];
	}
}

void LLMaterialTable::initBasicTable()
{
	// *TODO: Translate
	add(LL_MCODE_STONE,std::string("Stone"), LL_DEFAULT_STONE_UUID);	
	add(LL_MCODE_METAL,std::string("Metal"), LL_DEFAULT_METAL_UUID);
	add(LL_MCODE_GLASS,std::string("Glass"), LL_DEFAULT_GLASS_UUID);
	add(LL_MCODE_WOOD,std::string("Wood"), LL_DEFAULT_WOOD_UUID);
	add(LL_MCODE_FLESH,std::string("Flesh"), LL_DEFAULT_FLESH_UUID);
	add(LL_MCODE_PLASTIC,std::string("Plastic"), LL_DEFAULT_PLASTIC_UUID);
	add(LL_MCODE_RUBBER,std::string("Rubber"), LL_DEFAULT_RUBBER_UUID);
	add(LL_MCODE_LIGHT,std::string("Light"), LL_DEFAULT_LIGHT_UUID);

	// specify densities for these materials. . . 
    // these were taken from http://www.mcelwee.net/html/densities_of_various_materials.html

	addDensity(LL_MCODE_STONE,30.f);
	addDensity(LL_MCODE_METAL,50.f);
	addDensity(LL_MCODE_GLASS,20.f);
	addDensity(LL_MCODE_WOOD,10.f); 
	addDensity(LL_MCODE_FLESH,10.f);
	addDensity(LL_MCODE_PLASTIC,5.f);
	addDensity(LL_MCODE_RUBBER,0.5f); //
	addDensity(LL_MCODE_LIGHT,20.f); //

	// add damage and energy values
	addDamageAndEnergy(LL_MCODE_STONE, 1.f, 1.f, 1.f);	// concrete
	addDamageAndEnergy(LL_MCODE_METAL, 1.f, 1.f, 1.f);  // steel
	addDamageAndEnergy(LL_MCODE_GLASS, 1.f, 1.f, 1.f);  // borosilicate glass
	addDamageAndEnergy(LL_MCODE_WOOD, 1.f, 1.f, 1.f);    // southern pine
	addDamageAndEnergy(LL_MCODE_FLESH, 1.f, 1.f, 1.f);  // saltwater
	addDamageAndEnergy(LL_MCODE_PLASTIC, 1.f, 1.f, 1.f); // HDPE
	addDamageAndEnergy(LL_MCODE_RUBBER, 1.f, 1.f, 1.f); //
	addDamageAndEnergy(LL_MCODE_LIGHT, 1.f, 1.f, 1.f); //

	addFriction(LL_MCODE_STONE,0.8f);	// concrete
	addFriction(LL_MCODE_METAL,0.3f);  // steel
	addFriction(LL_MCODE_GLASS,0.2f);  // borosilicate glass
	addFriction(LL_MCODE_WOOD,0.6f);    // southern pine
	addFriction(LL_MCODE_FLESH,0.9f);  // saltwater
	addFriction(LL_MCODE_PLASTIC,0.4f); // HDPE
	addFriction(LL_MCODE_RUBBER,0.9f); //
	addFriction(LL_MCODE_LIGHT,0.2f); //

	addRestitution(LL_MCODE_STONE,0.4f);	// concrete
	addRestitution(LL_MCODE_METAL,0.4f);  // steel
	addRestitution(LL_MCODE_GLASS,0.7f);  // borosilicate glass
	addRestitution(LL_MCODE_WOOD,0.5f);    // southern pine
	addRestitution(LL_MCODE_FLESH,0.3f);  // saltwater
	addRestitution(LL_MCODE_PLASTIC,0.7f); // HDPE
	addRestitution(LL_MCODE_RUBBER,0.9f); //
	addRestitution(LL_MCODE_LIGHT,0.7f); //

	addShatterSound(LL_MCODE_STONE,LLUUID("ea296329-0f09-4993-af1b-e6784bab1dc9"));
	addShatterSound(LL_MCODE_METAL,LLUUID("d1375446-1c4d-470b-9135-30132433b678"));
	addShatterSound(LL_MCODE_GLASS,LLUUID("85cda060-b393-48e6-81c8-2cfdfb275351"));
	addShatterSound(LL_MCODE_WOOD,LLUUID("6f00669f-15e0-4793-a63e-c03f62fee43a"));
	addShatterSound(LL_MCODE_FLESH,LLUUID("2d8c6f51-149e-4e23-8413-93a379b42b67"));
	addShatterSound(LL_MCODE_PLASTIC,LLUUID("d55c7f3c-e1c3-4ddc-9eff-9ef805d9190e"));
	addShatterSound(LL_MCODE_RUBBER,LLUUID("212b6d1e-8d9c-4986-b3aa-f3c6df8d987d"));
	addShatterSound(LL_MCODE_LIGHT,LLUUID("d55c7f3c-e1c3-4ddc-9eff-9ef805d9190e"));

	//  CollisionSounds
	mCollisionSoundMatrix = new LLUUID[LL_MCODE_END*LL_MCODE_END];
	if (mCollisionSoundMatrix)
	{
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_STONE, SND_STONE_STONE);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_METAL, SND_STONE_METAL);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_GLASS, SND_STONE_GLASS);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_WOOD, SND_STONE_WOOD);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_FLESH, SND_STONE_FLESH);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_PLASTIC, SND_STONE_PLASTIC);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_RUBBER, SND_STONE_RUBBER);
		addCollisionSound(LL_MCODE_STONE, LL_MCODE_LIGHT, SND_STONE_PLASTIC);

		addCollisionSound(LL_MCODE_METAL, LL_MCODE_METAL, SND_METAL_METAL);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_GLASS, SND_METAL_GLASS);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_WOOD, SND_METAL_WOOD);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_FLESH, SND_METAL_FLESH);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_PLASTIC, SND_METAL_PLASTIC);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_LIGHT, SND_METAL_PLASTIC);
		addCollisionSound(LL_MCODE_METAL, LL_MCODE_RUBBER, SND_METAL_RUBBER);

		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_GLASS, SND_GLASS_GLASS);
		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_WOOD, SND_GLASS_WOOD);
		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_FLESH, SND_GLASS_FLESH);
		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_PLASTIC, SND_GLASS_PLASTIC);
		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_RUBBER, SND_GLASS_RUBBER);
		addCollisionSound(LL_MCODE_GLASS, LL_MCODE_LIGHT, SND_GLASS_PLASTIC);

		addCollisionSound(LL_MCODE_WOOD, LL_MCODE_WOOD, SND_WOOD_WOOD);
		addCollisionSound(LL_MCODE_WOOD, LL_MCODE_FLESH, SND_WOOD_FLESH);
		addCollisionSound(LL_MCODE_WOOD, LL_MCODE_PLASTIC, SND_WOOD_PLASTIC);
		addCollisionSound(LL_MCODE_WOOD, LL_MCODE_RUBBER, SND_WOOD_RUBBER);
		addCollisionSound(LL_MCODE_WOOD, LL_MCODE_LIGHT, SND_WOOD_PLASTIC);

		addCollisionSound(LL_MCODE_FLESH, LL_MCODE_FLESH, SND_FLESH_FLESH);
		addCollisionSound(LL_MCODE_FLESH, LL_MCODE_PLASTIC, SND_FLESH_PLASTIC);
		addCollisionSound(LL_MCODE_FLESH, LL_MCODE_RUBBER, SND_FLESH_RUBBER);
		addCollisionSound(LL_MCODE_FLESH, LL_MCODE_LIGHT, SND_FLESH_PLASTIC);

		addCollisionSound(LL_MCODE_RUBBER, LL_MCODE_RUBBER, SND_RUBBER_RUBBER);
		addCollisionSound(LL_MCODE_RUBBER, LL_MCODE_PLASTIC, SND_RUBBER_PLASTIC);
		addCollisionSound(LL_MCODE_RUBBER, LL_MCODE_LIGHT, SND_RUBBER_PLASTIC);

		addCollisionSound(LL_MCODE_PLASTIC, LL_MCODE_PLASTIC, SND_PLASTIC_PLASTIC);
		addCollisionSound(LL_MCODE_PLASTIC, LL_MCODE_LIGHT, SND_PLASTIC_PLASTIC);

		addCollisionSound(LL_MCODE_LIGHT, LL_MCODE_LIGHT, SND_PLASTIC_PLASTIC);
	}
	
	//  Sliding Sounds
	mSlidingSoundMatrix = new LLUUID[LL_MCODE_END*LL_MCODE_END];
	if (mSlidingSoundMatrix)
	{
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_STONE, SND_SLIDE_STONE_STONE);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_METAL, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_GLASS, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_WOOD, SND_SLIDE_STONE_WOOD);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_PLASTIC, SND_SLIDE_STONE_PLASTIC);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_RUBBER, SND_SLIDE_STONE_RUBBER);
		addSlidingSound(LL_MCODE_STONE, LL_MCODE_LIGHT, SND_SLIDE_STONE_PLASTIC);

		addSlidingSound(LL_MCODE_METAL, LL_MCODE_METAL, SND_SLIDE_METAL_METAL);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_GLASS, SND_SLIDE_METAL_GLASS);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_WOOD, SND_SLIDE_METAL_WOOD);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_FLESH, SND_SLIDE_METAL_FLESH);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_RUBBER, SND_SLIDE_METAL_RUBBER);
		addSlidingSound(LL_MCODE_METAL, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_GLASS, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_WOOD, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_GLASS, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addSlidingSound(LL_MCODE_WOOD, LL_MCODE_WOOD, SND_SLIDE_WOOD_WOOD);
		addSlidingSound(LL_MCODE_WOOD, LL_MCODE_FLESH, SND_SLIDE_WOOD_FLESH);
		addSlidingSound(LL_MCODE_WOOD, LL_MCODE_PLASTIC, SND_SLIDE_WOOD_PLASTIC);
		addSlidingSound(LL_MCODE_WOOD, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_WOOD, LL_MCODE_LIGHT, SND_SLIDE_WOOD_PLASTIC);

		addSlidingSound(LL_MCODE_FLESH, LL_MCODE_FLESH, SND_SLIDE_FLESH_FLESH);
		addSlidingSound(LL_MCODE_FLESH, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_FLESH, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_FLESH, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addSlidingSound(LL_MCODE_RUBBER, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_RUBBER, LL_MCODE_PLASTIC, SND_SLIDE_RUBBER_PLASTIC);
		addSlidingSound(LL_MCODE_RUBBER, LL_MCODE_LIGHT, SND_SLIDE_RUBBER_PLASTIC);

		addSlidingSound(LL_MCODE_PLASTIC, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addSlidingSound(LL_MCODE_PLASTIC, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addSlidingSound(LL_MCODE_LIGHT, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);
	}

	//  Rolling Sounds
	mRollingSoundMatrix = new LLUUID[LL_MCODE_END*LL_MCODE_END];
	if (mRollingSoundMatrix)
	{
		addRollingSound(LL_MCODE_STONE, LL_MCODE_STONE, SND_ROLL_STONE_STONE);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_METAL, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_GLASS, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_WOOD, SND_ROLL_STONE_WOOD);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_PLASTIC, SND_ROLL_STONE_PLASTIC);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_STONE, LL_MCODE_LIGHT, SND_ROLL_STONE_PLASTIC);

		addRollingSound(LL_MCODE_METAL, LL_MCODE_METAL, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_GLASS, SND_ROLL_METAL_GLASS);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_WOOD, SND_ROLL_METAL_WOOD);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_PLASTIC, SND_ROLL_METAL_WOOD);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_METAL, LL_MCODE_LIGHT, SND_ROLL_METAL_WOOD);

		addRollingSound(LL_MCODE_GLASS, LL_MCODE_GLASS, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_GLASS, LL_MCODE_WOOD, SND_ROLL_GLASS_WOOD);
		addRollingSound(LL_MCODE_GLASS, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_GLASS, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_GLASS, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_GLASS, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addRollingSound(LL_MCODE_WOOD, LL_MCODE_WOOD, SND_ROLL_WOOD_WOOD);
		addRollingSound(LL_MCODE_WOOD, LL_MCODE_FLESH, SND_ROLL_WOOD_FLESH);
		addRollingSound(LL_MCODE_WOOD, LL_MCODE_PLASTIC, SND_ROLL_WOOD_PLASTIC);
		addRollingSound(LL_MCODE_WOOD, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_WOOD, LL_MCODE_LIGHT, SND_ROLL_WOOD_PLASTIC);

		addRollingSound(LL_MCODE_FLESH, LL_MCODE_FLESH, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_FLESH, LL_MCODE_PLASTIC, SND_ROLL_FLESH_PLASTIC);
		addRollingSound(LL_MCODE_FLESH, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_FLESH, LL_MCODE_LIGHT, SND_ROLL_FLESH_PLASTIC);

		addRollingSound(LL_MCODE_RUBBER, LL_MCODE_RUBBER, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_RUBBER, LL_MCODE_PLASTIC, SND_SLIDE_STONE_STONE_01);
		addRollingSound(LL_MCODE_RUBBER, LL_MCODE_LIGHT, SND_SLIDE_STONE_STONE_01);

		addRollingSound(LL_MCODE_PLASTIC, LL_MCODE_PLASTIC, SND_ROLL_PLASTIC_PLASTIC);
		addRollingSound(LL_MCODE_PLASTIC, LL_MCODE_LIGHT, SND_ROLL_PLASTIC_PLASTIC);

		addRollingSound(LL_MCODE_LIGHT, LL_MCODE_LIGHT, SND_ROLL_PLASTIC_PLASTIC);
	}
}

BOOL LLMaterialTable::add(U8 mcode, const std::string& name, const LLUUID &uuid)
{
	LLMaterialInfo *infop;

	infop = new LLMaterialInfo(mcode,name,uuid);
	if (!infop) return FALSE;

	// Add at the end so the order in menus matches the order in this
	// file.  JNC 11.30.01
	mMaterialInfoList.push_back(infop);

	return TRUE;
}

BOOL LLMaterialTable::addCollisionSound(U8 mcode, U8 mcode2, const LLUUID &uuid)
{
	if (mCollisionSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		mCollisionSoundMatrix[mcode * LL_MCODE_END + mcode2] = uuid;		
		if (mcode != mcode2)
		{
			mCollisionSoundMatrix[mcode2 * LL_MCODE_END + mcode] = uuid;		
		}
	}
	return TRUE;
}

BOOL LLMaterialTable::addSlidingSound(U8 mcode, U8 mcode2, const LLUUID &uuid)
{
	if (mSlidingSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		mSlidingSoundMatrix[mcode * LL_MCODE_END + mcode2] = uuid;		
		if (mcode != mcode2)
		{
			mSlidingSoundMatrix[mcode2 * LL_MCODE_END + mcode] = uuid;		
		}
	}
	return TRUE;
}

BOOL LLMaterialTable::addRollingSound(U8 mcode, U8 mcode2, const LLUUID &uuid)
{
	if (mRollingSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		mRollingSoundMatrix[mcode * LL_MCODE_END + mcode2] = uuid;		
		if (mcode != mcode2)
		{
			mRollingSoundMatrix[mcode2 * LL_MCODE_END + mcode] = uuid;		
		}
	}
	return TRUE;
}

BOOL LLMaterialTable::addShatterSound(U8 mcode, const LLUUID &uuid)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			infop->mShatterSoundID = uuid;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMaterialTable::addDensity(U8 mcode, const F32 &density)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			infop->mDensity = density;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMaterialTable::addRestitution(U8 mcode, const F32 &restitution)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			infop->mRestitution = restitution;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMaterialTable::addFriction(U8 mcode, const F32 &friction)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			infop->mFriction = friction;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLMaterialTable::addDamageAndEnergy(U8 mcode, const F32 &hp_mod, const F32 &damage_mod, const F32 &ep_mod)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			infop->mHPModifier = hp_mod;
			infop->mDamageModifier = damage_mod;
			infop->mEPModifier = ep_mod;
			return TRUE;
		}
	}

	return FALSE;
}

LLUUID LLMaterialTable::getDefaultTextureID(const std::string& name)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (name == infop->mName)
		{
			return infop->mDefaultTextureID;
		}
	}

	return LLUUID::null;
}


LLUUID LLMaterialTable::getDefaultTextureID(U8 mcode)
{
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mDefaultTextureID;
		}
	}

	return LLUUID::null;
}


U8 LLMaterialTable::getMCode(const std::string& name)
{
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (name == infop->mName)
		{
			return infop->mMCode;
		}
	}

	return 0;
}


std::string LLMaterialTable::getName(U8 mcode)
{
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mName;
		}
	}

	return std::string();
}


LLUUID LLMaterialTable::getCollisionSoundUUID(U8 mcode, U8 mcode2)
{
	mcode &= LL_MCODE_MASK;
	mcode2 &= LL_MCODE_MASK;
	
	//llinfos << "code 1: " << ((U32) mcode) << " code 2:" << ((U32) mcode2) << llendl;
	if (mCollisionSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		return(mCollisionSoundMatrix[mcode * LL_MCODE_END + mcode2]);
	}
	else
	{
		//llinfos << "Null Sound" << llendl;
		return(SND_NULL);
	}
}

LLUUID LLMaterialTable::getSlidingSoundUUID(U8 mcode, U8 mcode2)
{	
	mcode &= LL_MCODE_MASK;
	mcode2 &= LL_MCODE_MASK;
	
	if (mSlidingSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		return(mSlidingSoundMatrix[mcode * LL_MCODE_END + mcode2]);
	}
	else
	{
		return(SND_NULL);
	}
}

LLUUID LLMaterialTable::getRollingSoundUUID(U8 mcode, U8 mcode2)
{	
	mcode &= LL_MCODE_MASK;
	mcode2 &= LL_MCODE_MASK;
	
	if (mRollingSoundMatrix && (mcode < LL_MCODE_END) && (mcode2 < LL_MCODE_END))
	{
		return(mRollingSoundMatrix[mcode * LL_MCODE_END + mcode2]);
	}
	else
	{
		return(SND_NULL);
	}
}

LLUUID LLMaterialTable::getGroundCollisionSoundUUID(U8 mcode)
{	
	//  Create material appropriate sounds for collisions with the ground 
	//  For now, simply return a single sound for all materials 
	return SND_STONE_DIRT_02;
}

LLUUID LLMaterialTable::getGroundSlidingSoundUUID(U8 mcode)
{	
	//  Create material-specific sound for sliding on ground
	//  For now, just return a single sound 
	return SND_SLIDE_STONE_STONE_01;
}

LLUUID LLMaterialTable::getGroundRollingSoundUUID(U8 mcode)
{	
	//  Create material-specific sound for rolling on ground
	//  For now, just return a single sound 
	return SND_SLIDE_STONE_STONE_01;
}

LLUUID LLMaterialTable::getCollisionParticleUUID(U8 mcode, U8 mcode2)
{	
	//  Returns an appropriate UUID to use as sprite at collision betweeen objects  
	//  For now, just return a single image 
	return IMG_SHOT;
}

LLUUID LLMaterialTable::getGroundCollisionParticleUUID(U8 mcode)
{	
	//  Returns an appropriate 
	//  For now, just return a single sound 
	return IMG_SMOKE_POOF;
}


F32 LLMaterialTable::getDensity(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mDensity;
		}
	}

	return 0.f;
}

F32 LLMaterialTable::getRestitution(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mRestitution;
		}
	}

	return LLMaterialTable::DEFAULT_RESTITUTION;
}

F32 LLMaterialTable::getFriction(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mFriction;
		}
	}

	return LLMaterialTable::DEFAULT_FRICTION;
}

F32 LLMaterialTable::getHPMod(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mHPModifier;
		}
	}

	return 1.f;
}

F32 LLMaterialTable::getDamageMod(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mDamageModifier;
		}
	}

	return 1.f;
}

F32 LLMaterialTable::getEPMod(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mEPModifier;
		}
	}

	return 1.f;
}

LLUUID LLMaterialTable::getShatterSoundUUID(U8 mcode)
{	
	mcode &= LL_MCODE_MASK;
	for (info_list_t::iterator iter = mMaterialInfoList.begin();
		 iter != mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo *infop = *iter;
		if (mcode == infop->mMCode)
		{
			return infop->mShatterSoundID;
		}
	}

	return SND_NULL;
}
