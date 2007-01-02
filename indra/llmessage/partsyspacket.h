/** 
 * @file partsyspacket.h
 * @brief Object for packing particle system initialization parameters
 * before sending them over the network
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_PARTSYSPACKET_H
#define LL_PARTSYSPACKET_H

#include "lluuid.h"

// Particle system stuff

const U64 PART_SYS_MAX_TIME_IN_USEC = 1000000;  //  1 second, die not quite near instantaneously


// this struct is for particle system initialization parameters 
// I'm breaking some rules here, but I need a storage structure to hold initialization data
//	for these things.  Sorry guys, they're not simple enough (yet) to avoid this cleanly 
struct LLPartInitData {
	// please do not add functions to this class -- data only!
	//F32 k[18];     // first 9 --> x,y,z  last 9 --> scale, alpha, rot
	//F32 kill_p[6]; // last one is for particles that die when they reach a spherical bounding radius 
	//F32 kill_plane[3]; 
	//F32 bounce_p[5]; 
	F32 bounce_b; // recently changed
	// no need to store orientation and position here, as they're sent over seperately
	//F32 pos_ranges[6]; 
	//F32 vel_ranges[6];
	F32 scale_range[4];
	F32 alpha_range[4];
	F32 vel_offset[3];	//new - more understandable!

	F32 mDistBeginFadeout; // for fadeout LOD optimization 
	F32 mDistEndFadeout;
	
	LLUUID mImageUuid;
	//U8 n; // number of particles 
	U8 mFlags[8]; // for miscellaneous data --> its interpretation can change at my whim!
	U8 createMe; // do I need to be created? or has the work allready been done?
	//ActionFlag is now mFlags[PART_SYS_ACTION_BYTE]
	//Spawn point is initially object creation center

	F32 diffEqAlpha[3];
	F32 diffEqScale[3];

	U8	maxParticles;
		//How many particles exist at any time within the system?
	U8	initialParticles;
		//How many particles exist when the system is created?
	F32	killPlaneZ;
		//For simplicity assume the XY plane, so this sets an altitude at which to die
	F32 killPlaneNormal[3]; 
		//Normal if not planar XY
	F32	bouncePlaneZ;
		//For simplicity assume the XY plane, so this sets an altitude at which to bounce
	F32 bouncePlaneNormal[3]; 
		//Normal if not planar XY
	F32	spawnRange;
		//Range of emission points about the mSpawnPoint
	F32	spawnFrequency;
		//Required if the system is to spawn new particles.
		//This variable determines the time after a particle dies when it is respawned.
	F32	spawnFreqencyRange;
		//Determines the random range of time until a new particle is spawned.
	F32	spawnDirection[3];
		//Direction vector giving the mean direction in which particles are spawned
	F32	spawnDirectionRange;
		//Direction limiting the angular range of emissions about the mean direction. 1.0f means everywhere, 0.0f means uni-directional
	F32	spawnVelocity;
		//The mean speed at which particles are emitted
	F32	spawnVelocityRange;
		//The range of speeds about the mean at which particles are emitted.
	F32	speedLimit;
		//Used to constrain particle maximum velocity
	F32	windWeight;
		//How much of an effect does wind have
	F32	currentGravity[3];
		//Gravity direction used in update calculations
	F32	gravityWeight;
		//How much of an effect does gravity have
	F32	globalLifetime;
		//If particles re-spawn, a system can exist forever.
		//If (ActionFlags & PART_SYS_GLOBAL_DIE) is TRUE this variable is used to determine how long the system lasts.
	F32	individualLifetime;
		//How long does each particle last if nothing else happens to it
	F32	individualLifetimeRange;
		//Range of variation in individual lifetimes
	F32	alphaDecay;
		//By what factor does alpha decrease as the lifetime of a particle is approached.
	F32	scaleDecay;
		//By what factor does scale decrease as the lifetime of a particle is approached.
	F32	distanceDeath;
		//With the increased functionality, particle systems can expand to indefinite size
		//(e.g. wind can chaotically move particles into a wide spread).
		//To avoid particles exceeding normal object size constraints,
		//set the PART_SYS_DISTANCE_DEATH flag, and set a distance value here, representing a radius around the spawn point.
	F32	dampMotionFactor;
		//How much to damp motion
	F32 windDiffusionFactor[3];
		//Change the size and alpha of particles as wind speed increases (scale gets bigger, alpha smaller)
};

// constants for setting flag values
// BYTES are in range 0-8, bits are in range 2^0 - 2^8 and can only be powers of two
const int PART_SYS_NO_Z_BUFFER_BYTE = 0; // option to turn off z-buffer when rendering
const int PART_SYS_NO_Z_BUFFER_BIT = 2;  // particle systems -- 
// I advise against using this, as it looks bad in every case I've tried

const int PART_SYS_SLOW_ANIM_BYTE = 0; // slow animation down by a factor of 10
const int PART_SYS_SLOW_ANIM_BIT = 1;  // useful for tweaking anims during debugging

const int PART_SYS_FOLLOW_VEL_BYTE = 0; // indicates whether to orient sprites towards
const int PART_SYS_FOLLOW_VEL_BIT = 4;  // their velocity vector -- default is FALSE

const int PART_SYS_IS_LIGHT_BYTE = 0;   // indicates whether a particular particle system
const int PART_SYS_IS_LIGHT_BIT = 8;    // is also a light object -- for andrew
// should deprecate this once there is a general method for setting light properties of objects

const int PART_SYS_SPAWN_COPY_BYTE = 0;   // indicates whether to spawn baby particle systems on 
const int PART_SYS_SPAWN_COPY_BIT = 0x10; // particle death -- intended for smoke trails

const int PART_SYS_COPY_VEL_BYTE = 0; // indicates whether baby particle systems inherit parents vel
const int PART_SYS_COPY_VEL_BIT = 0x20; // (by default they don't)

const int PART_SYS_INVISIBLE_BYTE = 0; // optional -- turn off display, just simulate
const int PART_SYS_INVISIBLE_BIT = 0x40; // useful for smoke trails

const int PART_SYS_ADAPT_TO_FRAMERATE_BYTE = 0;    // drop sprites from render call proportionally
const int PART_SYS_ADAPT_TO_FRAMERATE_BIT = 0x80; // to how far we are below 60 fps


// 26 September 2001 - not even big enough to hold all changes, so should enlarge anyway
//const U16 MAX_PART_SYS_PACKET_SIZE = 180;
const U16 MAX_PART_SYS_PACKET_SIZE = 256;

//const U8 PART_SYS_K_MASK				= 0x01;
const U8 PART_SYS_KILL_P_MASK			= 0x02;
const U8 PART_SYS_BOUNCE_P_MASK			= 0x04;
const U8 PART_SYS_BOUNCE_B_MASK			= 0x08;
//const U8 PART_SYS_POS_RANGES_MASK		= 0x10;
//const U8 PART_SYS_VEL_RANGES_MASK		= 0x20;
const U8 PART_SYS_VEL_OFFSET_MASK		= 0x10;	//re-use one of the original slots now commented out
const U8 PART_SYS_ALPHA_SCALE_DIFF_MASK = 0x20;	//re-use one of the original slots now commented out
const U8 PART_SYS_SCALE_RANGE_MASK		= 0x40;
const U8 PART_SYS_M_IMAGE_UUID_MASK		= 0x80;
const U8 PART_SYS_BYTE_3_ALPHA_MASK		= 0x01; // wrapped around, didn't we?

const U8 PART_SYS_BYTE_SPAWN_MASK		= 0x01;
const U8 PART_SYS_BYTE_ENVIRONMENT_MASK	= 0x02;
const U8 PART_SYS_BYTE_LIFESPAN_MASK	= 0x04;
const U8 PART_SYS_BYTE_DECAY_DAMP_MASK	= 0x08;
const U8 PART_SYS_BYTE_WIND_DIFF_MASK	= 0x10;


// 26 September 2001 - new constants for  mActionFlags
const int PART_SYS_ACTION_BYTE = 1;
const U8 PART_SYS_SPAWN 						= 0x01;
const U8 PART_SYS_BOUNCE 						= 0x02;
const U8 PART_SYS_AFFECTED_BY_WIND 				= 0x04;
const U8 PART_SYS_AFFECTED_BY_GRAVITY			= 0x08;
const U8 PART_SYS_EVALUATE_WIND_PER_PARTICLE 	= 0x10; 
const U8 PART_SYS_DAMP_MOTION 					= 0x20;
const U8 PART_SYS_WIND_DIFFUSION 				= 0x40;

// 26 September 2001 - new constants for  mKillFlags
const int PART_SYS_KILL_BYTE = 2;
const U8 PART_SYS_KILL_PLANE					= 0x01;
const U8 PART_SYS_GLOBAL_DIE 					= 0x02;  
const U8 PART_SYS_DISTANCE_DEATH 				= 0x04;
const U8 PART_SYS_TIME_DEATH 					= 0x08;


// global, because the sim-side also calls it in the LLPartInitDataFactory


void gSetInitDataDefaults(LLPartInitData *setMe);

class LLPartSysCompressedPacket
{
public:
	LLPartSysCompressedPacket();
	~LLPartSysCompressedPacket();
	BOOL	fromLLPartInitData(LLPartInitData *in, U32 &bytesUsed);
	BOOL	toLLPartInitData(LLPartInitData *out, U32 *bytesUsed);
	BOOL	fromUnsignedBytes(U8 *in, U32 bytesUsed);
	BOOL	toUnsignedBytes(U8 *out);
	U32		bufferSize();
	U8		*getBytePtr();

protected:
	U8 mData[MAX_PART_SYS_PACKET_SIZE];
	U32 mNumBytes;
	LLPartInitData mDefaults; //  this is intended to hold default LLPartInitData values
	//                            please do not modify it
	LLPartInitData mWorkingCopy; // uncompressed data I'm working with

protected:
	// private functions (used only to break up code)
	void	writeFlagByte(LLPartInitData *in);
	//U32		writeK(LLPartInitData *in, U32 startByte);
	U32		writeKill_p(LLPartInitData *in, U32 startByte);
	U32		writeBounce_p(LLPartInitData *in, U32 startByte);
	U32		writeBounce_b(LLPartInitData *in, U32 startByte);
	//U32		writePos_ranges(LLPartInitData *in, U32 startByte);
	//U32		writeVel_ranges(LLPartInitData *in, U32 startByte);
	U32		writeAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte);
	U32		writeScale_range(LLPartInitData *in, U32 startByte);
	U32		writeAlpha_range(LLPartInitData *in, U32 startByte);
	U32		writeUUID(LLPartInitData *in, U32 startByte);

	U32		writeVelocityOffset(LLPartInitData *in, U32 startByte);
	U32		writeSpawn(LLPartInitData *in, U32 startByte);	//all spawn data
	U32		writeEnvironment(LLPartInitData *in, U32 startByte);	//wind and gravity
	U32		writeLifespan(LLPartInitData *in, U32 startByte);	//lifespan data - individual and global
	U32		writeDecayDamp(LLPartInitData *in, U32 startByte);	//alpha and scale, and motion damp
	U32		writeWindDiffusionFactor(LLPartInitData *in, U32 startByte);

	
	//U32		readK(LLPartInitData *in, U32 startByte);
	U32		readKill_p(LLPartInitData *in, U32 startByte);
	U32		readBounce_p(LLPartInitData *in, U32 startByte);
	U32		readBounce_b(LLPartInitData *in, U32 startByte);
	//U32		readPos_ranges(LLPartInitData *in, U32 startByte);
	//U32		readVel_ranges(LLPartInitData *in, U32 startByte);
	U32		readAlphaScaleDiffEqn_range(LLPartInitData *in, U32 startByte);
	U32		readScale_range(LLPartInitData *in, U32 startByte);
	U32		readAlpha_range(LLPartInitData *in, U32 startByte);
	U32		readUUID(LLPartInitData *in, U32 startByte);

	U32		readVelocityOffset(LLPartInitData *in, U32 startByte);
	U32		readSpawn(LLPartInitData *in, U32 startByte);	//all spawn data
	U32		readEnvironment(LLPartInitData *in, U32 startByte);	//wind and gravity
	U32		readLifespan(LLPartInitData *in, U32 startByte);	//lifespan data - individual and global
	U32		readDecayDamp(LLPartInitData *in, U32 startByte);	//alpha and scale, and motion damp
	U32		readWindDiffusionFactor(LLPartInitData *in, U32 startByte);
};

#endif

