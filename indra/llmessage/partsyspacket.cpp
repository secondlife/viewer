/** 
 * @file partsyspacket.cpp
 * @brief Object for packing particle system initialization parameters
 * before sending them over the network.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "partsyspacket.h"
#include "imageids.h"

// this function is global
void gSetInitDataDefaults(LLPartInitData *setMe)
{
	U32 i;

	//for(i = 0; i < 18; i++) 
	//{
	//	setMe->k[i] = 0.0f;
	//}

	//setMe->kill_p[0] = setMe->kill_p[1] = setMe->kill_p[2] = 0.0f;
	//setMe->kill_p[3] = -0.2f; // time parameter, die when t= 5.0f
	//setMe->kill_p[4] = 1.0f;
	//setMe->kill_p[5] = -0.5f; // or radius == 2 (contracting)
	
	//setMe->bounce_p[0] = setMe->bounce_p[1] = 
	//	setMe->bounce_p[2] = setMe->bounce_p[3] = 0.0f;
	//setMe->bounce_p[4] = 1.0f;
	
	setMe->bounce_b = 1.0f;
	// i just changed the meaning of bounce_b
	// its now the attenuation from revlecting your velocity across the normal
	// set by bounce_p
	
	//setMe->pos_ranges[0] = setMe->pos_ranges[2] = setMe->pos_ranges[4] = -1.0f;
	//setMe->pos_ranges[1] = setMe->pos_ranges[3] = setMe->pos_ranges[5] =  1.0f;

	//setMe->vel_ranges[0] = setMe->vel_ranges[2] = setMe->vel_ranges[4] = -1.0f;
	//setMe->vel_ranges[1] = setMe->vel_ranges[3] = setMe->vel_ranges[5] =  1.0f;

	for(i = 0; i < 3; i++) 
	{
		setMe->diffEqAlpha[i] = 0.0f;
		setMe->diffEqScale[i] = 0.0f;
	}

	setMe->scale_range[0] = 1.00f;
	setMe->scale_range[1] = 5.00f;
	setMe->scale_range[2] = setMe->scale_range[3] = 0.0f;

	setMe->alpha_range[0] = setMe->alpha_range[1] = 1.0f;
	setMe->alpha_range[2] = setMe->alpha_range[3] = 0.0f;

	setMe->vel_offset[0] = 0.0f; 
	setMe->vel_offset[1] = 0.0f;
	setMe->vel_offset[2] = 0.0f; 

	// start dropping particles when I'm more then one sim away
	setMe->mDistBeginFadeout = 256.0f;
	setMe->mDistEndFadeout = 1.414f * 512.0f; 
	// stop displaying particles when I'm more then two sim diagonals away

	setMe->mImageUuid = IMG_SHOT;

	for(i = 0; i < 8; i++)
	{
		setMe->mFlags[i] = 0x00;
	}

	setMe->createMe = TRUE;

	setMe->maxParticles = 25;
	setMe->initialParticles = 25;

	//These defaults are for an explosion - a short lived set of debris affected by gravity.
		//Action flags default to PART_SYS_AFFECTED_BY_WIND + PART_SYS_AFFECTED_BY_GRAVITY + PART_SYS_DISTANCE_DEATH 
	setMe->mFlags[PART_SYS_ACTION_BYTE] = PART_SYS_AFFECTED_BY_WIND | PART_SYS_AFFECTED_BY_GRAVITY | PART_SYS_DISTANCE_DEATH;
	setMe->mFlags[PART_SYS_KILL_BYTE] = PART_SYS_DISTANCE_DEATH + PART_SYS_TIME_DEATH;

	setMe->killPlaneNormal[0] = 0.0f;setMe->killPlaneNormal[1] = 0.0f;setMe->killPlaneNormal[2] = 1.0f;		//Straight up
	setMe->killPlaneZ = 0.0f;	//get local ground z as an approximation if turn on PART_SYS_KILL_PLANE
	setMe->bouncePlaneNormal[0] = 0.0f;setMe->bouncePlaneNormal[1] = 0.0f;setMe->bouncePlaneNormal[2] = 1.0f;	//Straight up
	setMe->bouncePlaneZ = 0.0f;	//get local ground z as an approximation if turn on PART_SYS_BOUNCE
	setMe->spawnRange = 1.0f;
	setMe->spawnFrequency = 0.0f;	//Create the instant one dies
	setMe->spawnFreqencyRange = 0.0f;
	setMe->spawnDirection[0] = 0.0f;setMe->spawnDirection[1] = 0.0f;setMe->spawnDirection[2] = 1.0f;		//Straight up
	setMe->spawnDirectionRange = 1.0f;	//global scattering
	setMe->spawnVelocity = 0.75f;
	setMe->spawnVelocityRange = 0.25f;	//velocity +/- 0.25
	setMe->speedLimit = 1.0f;

	setMe->windWeight = 0.5f;	//0.0f means looks like a heavy object (if gravity is on), 1.0f means light and fluffy
	setMe->currentGravity[0] = 0.0f;setMe->currentGravity[1] = 0.0f;setMe->currentGravity[2] = -9.81f;
		//This has to be constant to allow for compression
		
	setMe->gravityWeight = 0.5f;	//0.0f means boyed by air, 1.0f means it's a lead weight
	setMe->globalLifetime = 0.0f;	//Arbitrary, but default is no global die, so doesn't matter
	setMe->individualLifetime = 5.0f;
	setMe->individualLifetimeRange = 1.0f;	//Particles last 5 secs +/- 1
	setMe->alphaDecay = 1.0f;	//normal alpha fadeout
	setMe->scaleDecay = 0.0f;	//no scale decay
	setMe->distanceDeath = 10.0f;	//die if hit unit radius
	setMe->dampMotionFactor = 0.0f;

	setMe->windDiffusionFactor[0] = 0.0f; 
	setMe->windDiffusionFactor[1] = 0.0f;
	setMe->windDiffusionFactor[2] = 0.0f; 
}

LLPartSysCompressedPacket::LLPartSysCompressedPacket()
{
	// default constructor for mDefaults called implicitly/automatically here
	for(int i = 0; i < MAX_PART_SYS_PACKET_SIZE; i++) 
	{
		mData[i] = '\0';
	}

	mNumBytes = 0;

	gSetInitDataDefaults(&mDefaults);
}

LLPartSysCompressedPacket::~LLPartSysCompressedPacket()
{
	// no dynamic data is stored by this class, do nothing.
}

void LLPartSysCompressedPacket::writeFlagByte(LLPartInitData *in)
{
		mData[0] =  mData[1] = mData[2] = '\0';

	U32 i;
	//for(i = 1; i < 18; i++) {
	//	if(in->k[i] != mDefaults.k[i])
	//	{
	//		mData[0] |= PART_SYS_K_MASK;
	//		break;
	//	}
	//}

	if(in->killPlaneZ != mDefaults.killPlaneZ ||
		in->killPlaneNormal[0] != mDefaults.killPlaneNormal[0] || 
		in->killPlaneNormal[1] != mDefaults.killPlaneNormal[1] ||
		in->killPlaneNormal[2] != mDefaults.killPlaneNormal[2] ||
		in->distanceDeath != mDefaults.distanceDeath)
	{
		mData[0] |= PART_SYS_KILL_P_MASK;
	}

	
	
	if(in->bouncePlaneZ != mDefaults.bouncePlaneZ ||
		in->bouncePlaneNormal[0] != mDefaults.bouncePlaneNormal[0] || 
		in->bouncePlaneNormal[1] != mDefaults.bouncePlaneNormal[1] ||
		in->bouncePlaneNormal[2] != mDefaults.bouncePlaneNormal[2])
	{
		mData[0] |= PART_SYS_BOUNCE_P_MASK;
	}
	
	if(in->bounce_b != mDefaults.bounce_b)
	{
		mData[0] |= PART_SYS_BOUNCE_B_MASK;
	}

	
	//if(in->pos_ranges[0] != mDefaults.pos_ranges[0] || in->pos_ranges[1] != mDefaults.pos_ranges[1] ||
	//	in->pos_ranges[2] != mDefaults.pos_ranges[2] || in->pos_ranges[3] != mDefaults.pos_ranges[3] ||
	//	in->pos_ranges[4] != mDefaults.pos_ranges[4] || in->pos_ranges[5] != mDefaults.pos_ranges[5])
	//{
	//	mData[0] |= PART_SYS_POS_RANGES_MASK;
	//}
	
	//if(in->vel_ranges[0] != mDefaults.vel_ranges[0] || in->vel_ranges[1] != mDefaults.vel_ranges[1] ||
	//	in->vel_ranges[2] != mDefaults.vel_ranges[2] || in->vel_ranges[3] != mDefaults.vel_ranges[3] ||
	//	in->vel_ranges[4] != mDefaults.vel_ranges[4] || in->vel_ranges[5] != mDefaults.vel_ranges[5])
	//{
//		mData[0] |= PART_SYS_VEL_RANGES_MASK;
	//}


	if(in->diffEqAlpha[0] != mDefaults.diffEqAlpha[0] || 
		in->diffEqAlpha[1] != mDefaults.diffEqAlpha[1] ||
		in->diffEqAlpha[2] != mDefaults.diffEqAlpha[2] ||
		in->diffEqScale[0] != mDefaults.diffEqScale[0] || 
		in->diffEqScale[1] != mDefaults.diffEqScale[1] ||
		in->diffEqScale[2] != mDefaults.diffEqScale[2])
	{
		mData[0] |= PART_SYS_ALPHA_SCALE_DIFF_MASK;
	}


	if(in->scale_range[0] != mDefaults.scale_range[0] || 
		in->scale_range[1] != mDefaults.scale_range[1] ||
		in->scale_range[2] != mDefaults.scale_range[2] ||
		in->scale_range[3] != mDefaults.scale_range[3])
	{
		mData[0] |= PART_SYS_SCALE_RANGE_MASK;
	}

	
	if(in->alpha_range[0] != mDefaults.alpha_range[0] || 
		in->alpha_range[1] != mDefaults.alpha_range[1] ||
		in->alpha_range[2] != mDefaults.alpha_range[2] ||
		in->alpha_range[3] != mDefaults.alpha_range[3])
	{
		mData[2] |= PART_SYS_BYTE_3_ALPHA_MASK;
	}

	if(in->vel_offset[0] != mDefaults.vel_offset[0] || 
		in->vel_offset[1] != mDefaults.vel_offset[1] ||
		in->vel_offset[2] != mDefaults.vel_offset[2])
	{
		mData[0] |= PART_SYS_VEL_OFFSET_MASK;
	}


	if(in->mImageUuid != mDefaults.mImageUuid)
	{
		mData[0] |= PART_SYS_M_IMAGE_UUID_MASK;
	}

	for( i = 0; i < 8; i++)
	{
		if(in->mFlags[i])
		{
			mData[1] |= 1<<i;
//			llprintline("Flag \"%x\" gets byte \"%x\"\n", i<<i, in->mFlags[i]);
		}
	}


	if(in->spawnRange != mDefaults.spawnRange ||
		in->spawnFrequency != mDefaults.spawnFrequency ||
		in->spawnFreqencyRange != mDefaults.spawnFreqencyRange ||
		in->spawnDirection[0] != mDefaults.spawnDirection[0] || 
		in->spawnDirection[1] != mDefaults.spawnDirection[1] ||
		in->spawnDirection[2] != mDefaults.spawnDirection[2] ||
		in->spawnDirectionRange != mDefaults.spawnDirectionRange ||
		in->spawnVelocity != mDefaults.spawnVelocity ||
		in->spawnVelocityRange != mDefaults.spawnVelocityRange)
	{
		mData[3] |= PART_SYS_BYTE_SPAWN_MASK;
	}


	if(in->windWeight != mDefaults.windWeight ||
		in->currentGravity[0] != mDefaults.currentGravity[0] || 
		in->currentGravity[1] != mDefaults.currentGravity[1] ||
		in->currentGravity[2] != mDefaults.currentGravity[2] ||
		in->gravityWeight != mDefaults.gravityWeight)
	{
		mData[3] |= PART_SYS_BYTE_ENVIRONMENT_MASK;
	}

	
	if(in->globalLifetime != mDefaults.globalLifetime ||
		in->individualLifetime != mDefaults.individualLifetime ||
		in->individualLifetimeRange != mDefaults.individualLifetimeRange)
	{
		mData[3] |= PART_SYS_BYTE_LIFESPAN_MASK;
	}

	
	if(in->speedLimit != mDefaults.speedLimit ||
		in->alphaDecay != mDefaults.alphaDecay ||
		in->scaleDecay != mDefaults.scaleDecay ||
		in->dampMotionFactor != mDefaults.dampMotionFactor)
	{
		mData[3] |= PART_SYS_BYTE_DECAY_DAMP_MASK;
	}

	if(in->windDiffusionFactor[0] != mDefaults.windDiffusionFactor[0] || 
		in->windDiffusionFactor[1] != mDefaults.windDiffusionFactor[1] ||
		in->windDiffusionFactor[2] != mDefaults.windDiffusionFactor[2])
	{
		mData[3] |= PART_SYS_BYTE_WIND_DIFF_MASK;
	}
}

F32 floatFromTwoBytes(S8 bMant, S8 bExp)
{
	F32 result = bMant;
	while(bExp > 0) 
	{
		result *= 2.0f;
		bExp--;
	}
	while(bExp < 0) 
	{
		result *= 0.5f;
		bExp++;
	}
	return result;
}

void twoBytesFromFloat(F32 fIn, S8 &bMant, S8 &bExp)
{
	bExp = 0;
	if(fIn > 127.0f)
	{
		fIn = 127.0f;
	}
	if(fIn < -127.0f)
	{
		fIn = -127.0f;
	}
	while(fIn < 64 && fIn > -64 && bExp > -127) 
	{
		fIn *= 2.0f;
		bExp--;
	}
	while((fIn > 128 || fIn < -128) && bExp < 127)
	{
		fIn *= 0.5f;
		bExp++;
	}
	bMant = (S8)fIn;
}

	

/*
U32 LLPartSysCompressedPacket::writeK(LLPartInitData *in, U32 startByte)
{
	U32 i, kFlag, i_mod_eight;
	S8 bMant, bExp;

	kFlag = startByte;

	startByte += 3; // 3 bytes contain enough room for 18 flag bits
	mData[kFlag] = 0x00;
//	llprintline("In the writeK\n");

	i_mod_eight = 0;
	for(i = 0; i < 18; i++) 
	{
		if(in->k[i] != mDefaults.k[i]) 
		{

			mData[kFlag] |= 1<<i_mod_eight;
			twoBytesFromFloat(in->k[i], bMant, bExp);

			mData[startByte++] = bMant;
			mData[startByte++] = bExp;
		}
		i_mod_eight++;
		while(i_mod_eight >= 8) 
		{
			kFlag++;
			i_mod_eight -= 8;
		}
	}
	
	return startByte;
}*/

U32 LLPartSysCompressedPacket::writeKill_p(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;

	twoBytesFromFloat(in->killPlaneNormal[0], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->killPlaneNormal[1], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->killPlaneNormal[2], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	
	twoBytesFromFloat(in->killPlaneZ, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->distanceDeath, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}

U32 LLPartSysCompressedPacket::writeBounce_p(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;

	twoBytesFromFloat(in->bouncePlaneNormal[0], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->bouncePlaneNormal[1], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->bouncePlaneNormal[2], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	

	twoBytesFromFloat(in->bouncePlaneZ, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}

U32 LLPartSysCompressedPacket::writeBounce_b(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;
	twoBytesFromFloat(in->bounce_b, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	return startByte;
}

//U32 LLPartSysCompressedPacket::writePos_ranges(LLPartInitData *in, U32 startByte)
//{
//	S8 tmp;
//	int i;
//	for(i = 0; i < 6; i++)
//	{
//		tmp = (S8) in->pos_ranges[i]; // float to int conversion (keep the sign)
//		mData[startByte++] = (U8)tmp; // signed to unsigned typecast
//	}
//	return startByte;
//}

//U32 LLPartSysCompressedPacket::writeVel_ranges(LLPartInitData *in, U32 startByte)
//{
//	S8 tmp;
//	int i;
//	for(i = 0; i < 6; i++)
//	{
//		tmp = (S8) in->vel_ranges[i]; // float to int conversion (keep the sign)
//		mData[startByte++] = (U8)tmp; // signed to unsigned typecast
//	}
//	return startByte;
//}

U32 LLPartSysCompressedPacket::writeAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;
	for(i = 0; i < 3; i++)
	{
		twoBytesFromFloat(in->diffEqAlpha[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}
	for(i = 0; i < 3; i++)
	{
		twoBytesFromFloat(in->diffEqScale[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::writeScale_range(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;
	for(i = 0; i < 4; i++)
	{
		twoBytesFromFloat(in->scale_range[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}
	return startByte;
}


U32 LLPartSysCompressedPacket::writeAlpha_range(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;
	for(i = 0; i < 4; i++)
	{
		twoBytesFromFloat(in->alpha_range[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::writeVelocityOffset(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;
	for(i = 0; i < 3; i++)
	{
		twoBytesFromFloat(in->vel_offset[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::writeUUID(LLPartInitData *in, U32 startByte)
{
	U8 * bufPtr = mData + startByte;
	if(in->mImageUuid == IMG_SHOT) {
		mData[startByte++] = 0x01;
		return startByte;
	}

	if(in->mImageUuid == IMG_SPARK) {
		mData[startByte++] = 0x02;
		return startByte;
	}

	
	if(in->mImageUuid == IMG_BIG_EXPLOSION_1) {
		mData[startByte++] = 0x03;
		return startByte;
	}

	if(in->mImageUuid == IMG_BIG_EXPLOSION_2) {
		mData[startByte++] = 0x04;
		return startByte;
	}


	if(in->mImageUuid == IMG_SMOKE_POOF) {
		mData[startByte++] = 0x05;
		return startByte;
	}

	if(in->mImageUuid == IMG_FIRE) {
		mData[startByte++] = 0x06;
		return startByte;
	}

	
	if(in->mImageUuid == IMG_EXPLOSION) {
		mData[startByte++] = 0x07;
		return startByte;
	}

	if(in->mImageUuid == IMG_EXPLOSION_2) {
		mData[startByte++] = 0x08;
		return startByte;
	}

	
	if(in->mImageUuid == IMG_EXPLOSION_3) {
		mData[startByte++] = 0x09;
		return startByte;
	}

	if(in->mImageUuid == IMG_EXPLOSION_4) {
		mData[startByte++] = 0x0A;
		return startByte;
	}

	mData[startByte++] = 0x00; // flag for "read whole UUID"

	memcpy(bufPtr, in->mImageUuid.mData, 16);		/* Flawfinder: ignore */
	return (startByte+16);
}

U32 LLPartSysCompressedPacket::writeSpawn(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;

	twoBytesFromFloat(in->spawnRange, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->spawnFrequency, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->spawnFreqencyRange, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	
	
	for(i = 0; i < 3; i++)
	{
		twoBytesFromFloat(in->spawnDirection[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}

	twoBytesFromFloat(in->spawnDirectionRange, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->spawnVelocity, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	twoBytesFromFloat(in->spawnVelocityRange, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}

U32 LLPartSysCompressedPacket::writeEnvironment(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;
	int i;

	twoBytesFromFloat(in->windWeight, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	for(i = 0; i < 3; i++)
	{
		twoBytesFromFloat(in->currentGravity[i], bMant, bExp);
		mData[startByte++] = bMant;
		mData[startByte++] = bExp;
	}

	twoBytesFromFloat(in->gravityWeight, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;
	return startByte;
}	
	
U32 LLPartSysCompressedPacket::writeLifespan(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;

	twoBytesFromFloat(in->globalLifetime, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->individualLifetime, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->individualLifetimeRange, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}
	

U32 LLPartSysCompressedPacket::writeDecayDamp(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;

	twoBytesFromFloat(in->speedLimit, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->alphaDecay, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->scaleDecay, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->dampMotionFactor, bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}

U32 LLPartSysCompressedPacket::writeWindDiffusionFactor(LLPartInitData *in, U32 startByte)
{
	S8 bExp, bMant;

	twoBytesFromFloat(in->windDiffusionFactor[0], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->windDiffusionFactor[1], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	twoBytesFromFloat(in->windDiffusionFactor[2], bMant, bExp);
	mData[startByte++] = bMant;
	mData[startByte++] = bExp;

	return startByte;
}





	
/*
U32 LLPartSysCompressedPacket::readK(LLPartInitData *in, U32 startByte)
{
	U32 i, i_mod_eight, kFlag;
	S8 bMant, bExp; // 1 bytes mantissa and exponent for a float
	kFlag = startByte;
	startByte += 3; // 3 bytes has enough room for 18 bits

	i_mod_eight = 0;
	for(i = 0; i < 18; i++) 
	{
		if(mData[kFlag]&(1<<i_mod_eight))  
		{

			
		
			bMant = mData[startByte++];
			bExp = mData[startByte++];
		
			
			in->k[i] = floatFromTwoBytes(bMant, bExp); // much tighter platform-independent
			//                                            way to ship floats

		}
		i_mod_eight++;
		if(i_mod_eight >= 8)
		{
			i_mod_eight -= 8;
			kFlag++;
		}
	}
	
	return startByte;
}
*/

U32 LLPartSysCompressedPacket::readKill_p(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->killPlaneNormal[0] = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->killPlaneNormal[1] = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->killPlaneNormal[2] = floatFromTwoBytes(bMant, bExp);

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->killPlaneZ = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->distanceDeath = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readBounce_p(LLPartInitData *in, U32 startByte)
{

	S8 bMant, bExp;

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->bouncePlaneNormal[0] = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->bouncePlaneNormal[1] = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->bouncePlaneNormal[2] = floatFromTwoBytes(bMant, bExp);

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->bouncePlaneZ = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readBounce_b(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->bounce_b = floatFromTwoBytes(bMant, bExp);
	return startByte;
}


//U32 LLPartSysCompressedPacket::readPos_ranges(LLPartInitData *in, U32 startByte)
//{
//	S8 tmp;
//	int i;
//	for(i = 0; i < 6; i++)
//	{
//		tmp = (S8)mData[startByte++];
//		in->pos_ranges[i] = tmp;
//	}
//	return startByte;
//}

//U32 LLPartSysCompressedPacket::readVel_ranges(LLPartInitData *in, U32 startByte)
//{
//	S8 tmp;
//	int i;
//	for(i = 0; i < 6; i++)
//	{
//		tmp = (S8)mData[startByte++];
//		in->vel_ranges[i] = tmp;
//	}
//	return startByte;
//}



U32 LLPartSysCompressedPacket::readAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte)
{
	int i;
	S8 bMant, bExp;
	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->diffEqAlpha[i] = floatFromTwoBytes(bMant, bExp);
	}
	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->diffEqScale[i] = floatFromTwoBytes(bMant, bExp);
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::readAlpha_range(LLPartInitData *in, U32 startByte)
{
	int i;
	S8 bMant, bExp;
	for(i = 0; i < 4; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->alpha_range[i] = floatFromTwoBytes(bMant, bExp);
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::readScale_range(LLPartInitData *in, U32 startByte)
{
	int i;
	S8 bMant, bExp;
	for(i = 0; i < 4; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->scale_range[i] = floatFromTwoBytes(bMant, bExp);
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::readVelocityOffset(LLPartInitData *in, U32 startByte)
{
	int i;
	S8 bMant, bExp;
	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->vel_offset[i] = floatFromTwoBytes(bMant, bExp);
	}
	return startByte;
}

U32 LLPartSysCompressedPacket::readUUID(LLPartInitData *in, U32 startByte)
{
	U8 * bufPtr = mData + startByte;
	
	if(mData[startByte] == 0x01)
	{
		in->mImageUuid = IMG_SHOT;
		return startByte+1;
	}
	if(mData[startByte] == 0x02)
	{
		in->mImageUuid = IMG_SPARK;
		return startByte+1;
	}
	if(mData[startByte] == 0x03)
	{
		in->mImageUuid = IMG_BIG_EXPLOSION_1;
		return startByte+1;
	}
	if(mData[startByte] == 0x04)
	{
		in->mImageUuid = IMG_BIG_EXPLOSION_2;
		return startByte+1;
	}
	if(mData[startByte] == 0x05)
	{
		in->mImageUuid = IMG_SMOKE_POOF;
		return startByte+1;
	}
	if(mData[startByte] == 0x06)
	{
		in->mImageUuid = IMG_FIRE;
		return startByte+1;
	}
	if(mData[startByte] == 0x07)
	{
		in->mImageUuid = IMG_EXPLOSION;
		return startByte+1;
	}
	if(mData[startByte] == 0x08)
	{
		in->mImageUuid = IMG_EXPLOSION_2;
		return startByte+1;
	}
	if(mData[startByte] == 0x09)
	{
		in->mImageUuid = IMG_EXPLOSION_3;
		return startByte+1;
	}
	if(mData[startByte] == 0x0A)
	{
		in->mImageUuid = IMG_EXPLOSION_4;
		return startByte+1;
	}

	startByte++; // cause we actually have to read the UUID now.
	memcpy(in->mImageUuid.mData, bufPtr, 16);		/* Flawfinder: ignore */
	return (startByte+16);
}




U32 LLPartSysCompressedPacket::readSpawn(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;
	U32	i;

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnRange = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnFrequency = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnFreqencyRange = floatFromTwoBytes(bMant, bExp);

	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->spawnDirection[i] = floatFromTwoBytes(bMant, bExp);
	}

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnDirectionRange = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnVelocity = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->spawnVelocityRange = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readEnvironment(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;
	U32	i;


	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->windWeight = floatFromTwoBytes(bMant, bExp);

	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->currentGravity[i] = floatFromTwoBytes(bMant, bExp);
	}

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->gravityWeight = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readLifespan(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->globalLifetime = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->individualLifetime = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->individualLifetimeRange = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readDecayDamp(LLPartInitData *in, U32 startByte)
{
	S8 bMant, bExp;

	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->speedLimit = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->alphaDecay = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->scaleDecay = floatFromTwoBytes(bMant, bExp);
	bMant = mData[startByte++];
	bExp = mData[startByte++];
	in->dampMotionFactor = floatFromTwoBytes(bMant, bExp);

	return startByte;
}

U32 LLPartSysCompressedPacket::readWindDiffusionFactor(LLPartInitData *in, U32 startByte)
{
	int i;
	S8 bMant, bExp;
	for(i = 0; i < 3; i++)
	{
		bMant = mData[startByte++];
		bExp = mData[startByte++];
		in->windDiffusionFactor[i] = floatFromTwoBytes(bMant, bExp);
	}
	return startByte;
}

BOOL LLPartSysCompressedPacket::fromLLPartInitData(LLPartInitData *in, U32 &bytesUsed)
{

	writeFlagByte(in);
	U32 currByte = 4;
	
//	llprintline("calling \"fromLLPartInitData\"\n");

	//if(mData[0] & PART_SYS_K_MASK)
	//{
	//	currByte = writeK(in, 3); // first 3 bytes are reserved for header data
	//}
	


	if(mData[0] & PART_SYS_KILL_P_MASK)
	{
		currByte = writeKill_p(in, currByte);
	}
	
	if(mData[0] & PART_SYS_BOUNCE_P_MASK)
	{
		currByte = writeBounce_p(in, currByte);
	}
	
	if(mData[0] & PART_SYS_BOUNCE_B_MASK)
	{
		currByte = writeBounce_b(in, currByte);
	}
	
	//if(mData[0] & PART_SYS_POS_RANGES_MASK)
	//{
	//	currByte = writePos_ranges(in, currByte);
	//}
	
	//if(mData[0] & PART_SYS_VEL_RANGES_MASK)
	//{
	//	currByte = writeVel_ranges(in, currByte);
	//}

	if(mData[0] & PART_SYS_ALPHA_SCALE_DIFF_MASK)
	{
		currByte = writeAlphaScaleDiffEqn_range(in, currByte);
	}
	
	if(mData[0] & PART_SYS_SCALE_RANGE_MASK)
	{
		currByte = writeScale_range(in, currByte);
	}
	
	if(mData[0] & PART_SYS_VEL_OFFSET_MASK)
	{
		currByte = writeVelocityOffset(in, currByte);
	}

	if(mData[0] & PART_SYS_M_IMAGE_UUID_MASK)
	{
		currByte = writeUUID(in, currByte);
	}
	

	if(mData[3] & PART_SYS_BYTE_SPAWN_MASK)
	{
		currByte = writeSpawn(in, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_ENVIRONMENT_MASK)
	{
		currByte = writeEnvironment(in, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_LIFESPAN_MASK)
	{
		currByte = writeLifespan(in, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_DECAY_DAMP_MASK)
	{
		currByte = writeDecayDamp(in, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_WIND_DIFF_MASK)
	{
		currByte = writeWindDiffusionFactor(in, currByte);
	}
	

	if(mData[2] & PART_SYS_BYTE_3_ALPHA_MASK)
	{
		currByte = writeAlpha_range(in, currByte);
	}

	mData[currByte++] = (U8)in->maxParticles;
	mData[currByte++] = (U8)in->initialParticles;


	U32 flagFlag = 1; // flag indicating which flag bytes are non-zero
	//                   yeah, I know, the name sounds funny
	for(U32 i = 0; i < 8; i++)
	{
	
//		llprintline("Flag \"%x\" gets byte \"%x\"\n", flagFlag, in->mFlags[i]);
		if(mData[1] & flagFlag)
		{
			mData[currByte++] = in->mFlags[i];
//			llprintline("and is valid...\n");
		}
		flagFlag <<= 1;
	}

	bytesUsed = mNumBytes = currByte;
	
	
	
//	llprintline("returning from \"fromLLPartInitData\" with %d bytes\n", bytesUsed);
	
	return TRUE;
}

BOOL LLPartSysCompressedPacket::toLLPartInitData(LLPartInitData *out, U32 *bytesUsed)
{
	U32 currByte = 4;

	gSetInitDataDefaults(out);

	if(mData[0] & PART_SYS_KILL_P_MASK)
	{
		currByte = readKill_p(out, currByte);
	}
	
	if(mData[0] & PART_SYS_BOUNCE_P_MASK)
	{
		currByte = readBounce_p(out, currByte);
	}
	
	if(mData[0] & PART_SYS_BOUNCE_B_MASK)
	{
		currByte = readBounce_b(out, currByte);
	}
	
	if(mData[0] & PART_SYS_ALPHA_SCALE_DIFF_MASK)
	{
		currByte = readAlphaScaleDiffEqn_range(out, currByte);
	}
	
	if(mData[0] & PART_SYS_SCALE_RANGE_MASK)
	{
		currByte = readScale_range(out, currByte);
	}
	
	if(mData[0] & PART_SYS_VEL_OFFSET_MASK)
	{
		currByte = readVelocityOffset(out, currByte);
	}
	
	if(mData[0] & PART_SYS_M_IMAGE_UUID_MASK)
	{
		currByte = readUUID(out, currByte);
	}
	

	if(mData[3] & PART_SYS_BYTE_SPAWN_MASK)
	{
		currByte = readSpawn(out, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_ENVIRONMENT_MASK)
	{
		currByte = readEnvironment(out, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_LIFESPAN_MASK)
	{
		currByte = readLifespan(out, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_DECAY_DAMP_MASK)
	{
		currByte = readDecayDamp(out, currByte);
	}
	
	if(mData[3] & PART_SYS_BYTE_WIND_DIFF_MASK)
	{
		currByte = readWindDiffusionFactor(out, currByte);
	}
	
	if(mData[2] & PART_SYS_BYTE_3_ALPHA_MASK)
	{
		currByte = readAlpha_range(out, currByte);
	}

	out->maxParticles = mData[currByte++];
	out->initialParticles = mData[currByte++];

	U32 flagFlag = 1; // flag indicating which flag bytes are non-zero
	//                   yeah, I know, the name sounds funny
	for(U32 i = 0; i < 8; i++)
	{
		flagFlag = 1<<i;
	
		if((mData[1] & flagFlag))
		{
			out->mFlags[i] = mData[currByte++];
		}
	}

	*bytesUsed = currByte;
	return TRUE;
}

BOOL LLPartSysCompressedPacket::fromUnsignedBytes(U8 *in, U32 bytesUsed)
{
	if ((in != NULL) && (bytesUsed <= sizeof(mData)))
	{
		memcpy(mData, in, bytesUsed);	/* Flawfinder: ignore */
		mNumBytes = bytesUsed;
		return TRUE;
	}
	else
	{
		llerrs << "NULL input data or number of bytes exceed mData size" << llendl;
		return FALSE;
	}
}		
	

U32 LLPartSysCompressedPacket::bufferSize()
{
	return mNumBytes;
}

BOOL LLPartSysCompressedPacket::toUnsignedBytes(U8 *out)
{
	memcpy(out, mData, mNumBytes);		/* Flawfinder: ignore */
	return TRUE;
}

U8 * LLPartSysCompressedPacket::getBytePtr()
{
	return mData;
}


