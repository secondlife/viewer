/** 
 * @file sound_ids.h
 * @brief Temporary holder for sound IDs.
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

#ifndef LL_SOUND_IDS_H
#define LL_SOUND_IDS_H

// *NOTE: Do not put the actual IDs in this file - otherwise the symbols
// and values will be copied into every .o/.obj file and increase link time.

class LLUUID;

extern const LLUUID SND_NULL;
extern const LLUUID SND_RIDE;
extern const LLUUID SND_SHOT;
extern const LLUUID SND_MORTAR;
extern const LLUUID SND_HIT;
extern const LLUUID SND_EXPLOSION;
extern const LLUUID SND_BOING;
extern const LLUUID SND_OBJECT_CREATE;

//  Different bird sounds for different states 
extern const LLUUID SND_CHIRP;				//  Flying random chirp
extern const LLUUID SND_CHIRP2;			//  Spooked by user 
extern const LLUUID SND_CHIRP3;			//  Spooked by object
extern const LLUUID SND_CHIRP4;			//  Chasing other bird 
extern const LLUUID SND_CHIRP5;			//  Hopping random chirp
extern const LLUUID SND_CHIRPDEAD;			//  Hit by grenade - dead!


extern const LLUUID SND_MUNCH;
extern const LLUUID SND_PUNCH;
extern const LLUUID SND_SPLASH;
extern const LLUUID SND_CLICK;
extern const LLUUID SND_WHISTLE;
extern const LLUUID SND_TYPING;

extern const LLUUID SND_ARROW_SHOT;
extern const LLUUID SND_ARROW_THUD;
extern const LLUUID SND_LASER_SHOT;
extern const LLUUID SND_JET_THRUST;

extern const LLUUID SND_SILENCE;
extern const LLUUID SND_BUBBLES;
extern const LLUUID SND_WELCOME;
extern const LLUUID SND_SQUISH;
extern const LLUUID SND_SUBPOD;
extern const LLUUID SND_FOOTSTEPS;
extern const LLUUID SND_STEP_LEFT;
extern const LLUUID SND_STEP_RIGHT;

extern const LLUUID SND_BALL_COLLISION;

extern const LLUUID SND_OOOH_SCARE_ME;
extern const LLUUID SND_PAYBACK_TIME;
extern const LLUUID SND_READY_FOR_BATTLE;

extern const LLUUID SND_FLESH_FLESH;
extern const LLUUID SND_FLESH_PLASTIC;
extern const LLUUID SND_FLESH_RUBBER;
extern const LLUUID SND_GLASS_FLESH;
extern const LLUUID SND_GLASS_GLASS;
extern const LLUUID SND_GLASS_PLASTIC;
extern const LLUUID SND_GLASS_RUBBER;
extern const LLUUID SND_GLASS_WOOD;
extern const LLUUID SND_METAL_FLESH;
extern const LLUUID SND_METAL_GLASS;
extern const LLUUID SND_METAL_METAL;
extern const LLUUID SND_METAL_PLASTIC;
extern const LLUUID SND_METAL_RUBBER;
extern const LLUUID SND_METAL_WOOD;
extern const LLUUID SND_PLASTIC_PLASTIC;
extern const LLUUID SND_RUBBER_PLASTIC;
extern const LLUUID SND_RUBBER_RUBBER;
extern const LLUUID SND_STONE_FLESH;
extern const LLUUID SND_STONE_GLASS;
extern const LLUUID SND_STONE_METAL;
extern const LLUUID SND_STONE_PLASTIC;
extern const LLUUID SND_STONE_RUBBER;
extern const LLUUID SND_STONE_STONE;
extern const LLUUID SND_STONE_WOOD;
extern const LLUUID SND_WOOD_FLESH;
extern const LLUUID SND_WOOD_PLASTIC;
extern const LLUUID SND_WOOD_RUBBER;
extern const LLUUID SND_WOOD_WOOD;


extern const LLUUID SND_SLIDE_FLESH_FLESH;
extern const LLUUID SND_SLIDE_FLESH_PLASTIC;
extern const LLUUID SND_SLIDE_FLESH_RUBBER;
extern const LLUUID SND_SLIDE_FLESH_FABRIC;
extern const LLUUID SND_SLIDE_FLESH_GRAVEL;
extern const LLUUID SND_SLIDE_FLESH_GRAVEL_02;
extern const LLUUID SND_SLIDE_FLESH_GRAVEL_03;
extern const LLUUID SND_SLIDE_GLASS_GRAVEL;
extern const LLUUID SND_SLIDE_GLASS_GRAVEL_02;
extern const LLUUID SND_SLIDE_GLASS_GRAVEL_03;
extern const LLUUID SND_SLIDE_GLASS_FLESH;
extern const LLUUID SND_SLIDE_GLASS_GLASS;
extern const LLUUID SND_SLIDE_GLASS_PLASTIC;
extern const LLUUID SND_SLIDE_GLASS_RUBBER;
extern const LLUUID SND_SLIDE_GLASS_WOOD;
extern const LLUUID SND_SLIDE_METAL_FABRIC;
extern const LLUUID SND_SLIDE_METAL_FLESH;
extern const LLUUID SND_SLIDE_METAL_FLESH_02;
extern const LLUUID SND_SLIDE_METAL_GLASS;
extern const LLUUID SND_SLIDE_METAL_GLASS_02;
extern const LLUUID SND_SLIDE_METAL_GLASS_03;
extern const LLUUID SND_SLIDE_METAL_GLASS_04;
extern const LLUUID SND_SLIDE_METAL_GRAVEL;
extern const LLUUID SND_SLIDE_METAL_GRAVEL_02;
extern const LLUUID SND_SLIDE_METAL_METAL;
extern const LLUUID SND_SLIDE_METAL_METAL_02;
extern const LLUUID SND_SLIDE_METAL_METAL_03;
extern const LLUUID SND_SLIDE_METAL_METAL_04;
extern const LLUUID SND_SLIDE_METAL_METAL_05;
extern const LLUUID SND_SLIDE_METAL_METAL_06;
extern const LLUUID SND_SLIDE_METAL_PLASTIC;
extern const LLUUID SND_SLIDE_METAL_RUBBER;
extern const LLUUID SND_SLIDE_METAL_WOOD;
extern const LLUUID SND_SLIDE_METAL_WOOD_02;
extern const LLUUID SND_SLIDE_METAL_WOOD_03;
extern const LLUUID SND_SLIDE_METAL_WOOD_04;
extern const LLUUID SND_SLIDE_METAL_WOOD_05;
extern const LLUUID SND_SLIDE_METAL_WOOD_06;
extern const LLUUID SND_SLIDE_METAL_WOOD_07;
extern const LLUUID SND_SLIDE_METAL_WOOD_08;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL_02;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL_03;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL_04;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL_05;
extern const LLUUID SND_SLIDE_PLASTIC_GRAVEL_06;
extern const LLUUID SND_SLIDE_PLASTIC_PLASTIC;
extern const LLUUID SND_SLIDE_PLASTIC_PLASTIC_02;
extern const LLUUID SND_SLIDE_PLASTIC_PLASTIC_03;
extern const LLUUID SND_SLIDE_PLASTIC_FABRIC;
extern const LLUUID SND_SLIDE_PLASTIC_FABRIC_02;
extern const LLUUID SND_SLIDE_PLASTIC_FABRIC_03;
extern const LLUUID SND_SLIDE_PLASTIC_FABRIC_04;
extern const LLUUID SND_SLIDE_RUBBER_PLASTIC;
extern const LLUUID SND_SLIDE_RUBBER_PLASTIC_02;
extern const LLUUID SND_SLIDE_RUBBER_PLASTIC_03;
extern const LLUUID SND_SLIDE_RUBBER_RUBBER;
extern const LLUUID SND_SLIDE_STONE_FLESH;
extern const LLUUID SND_SLIDE_STONE_GLASS;
extern const LLUUID SND_SLIDE_STONE_METAL;
extern const LLUUID SND_SLIDE_STONE_PLASTIC;
extern const LLUUID SND_SLIDE_STONE_PLASTIC_02;
extern const LLUUID SND_SLIDE_STONE_PLASTIC_03;
extern const LLUUID SND_SLIDE_STONE_RUBBER;
extern const LLUUID SND_SLIDE_STONE_RUBBER_02;
extern const LLUUID SND_SLIDE_STONE_STONE;
extern const LLUUID SND_SLIDE_STONE_STONE_02;
extern const LLUUID SND_SLIDE_STONE_WOOD;
extern const LLUUID SND_SLIDE_STONE_WOOD_02;
extern const LLUUID SND_SLIDE_STONE_WOOD_03;
extern const LLUUID SND_SLIDE_STONE_WOOD_04;
extern const LLUUID SND_SLIDE_WOOD_FABRIC;
extern const LLUUID SND_SLIDE_WOOD_FABRIC_02;
extern const LLUUID SND_SLIDE_WOOD_FABRIC_03;
extern const LLUUID SND_SLIDE_WOOD_FABRIC_04;
extern const LLUUID SND_SLIDE_WOOD_FLESH;
extern const LLUUID SND_SLIDE_WOOD_FLESH_02;
extern const LLUUID SND_SLIDE_WOOD_FLESH_03;
extern const LLUUID SND_SLIDE_WOOD_FLESH_04;
extern const LLUUID SND_SLIDE_WOOD_GRAVEL;
extern const LLUUID SND_SLIDE_WOOD_GRAVEL_02;
extern const LLUUID SND_SLIDE_WOOD_GRAVEL_03;
extern const LLUUID SND_SLIDE_WOOD_GRAVEL_04;
extern const LLUUID SND_SLIDE_WOOD_PLASTIC;
extern const LLUUID SND_SLIDE_WOOD_PLASTIC_02;
extern const LLUUID SND_SLIDE_WOOD_PLASTIC_03;
extern const LLUUID SND_SLIDE_WOOD_RUBBER;
extern const LLUUID SND_SLIDE_WOOD_WOOD;
extern const LLUUID SND_SLIDE_WOOD_WOOD_02;
extern const LLUUID SND_SLIDE_WOOD_WOOD_03;
extern const LLUUID SND_SLIDE_WOOD_WOOD_04;
extern const LLUUID SND_SLIDE_WOOD_WOOD_05;
extern const LLUUID SND_SLIDE_WOOD_WOOD_06;
extern const LLUUID SND_SLIDE_WOOD_WOOD_07;
extern const LLUUID SND_SLIDE_WOOD_WOOD_08;


extern const LLUUID SND_ROLL_FLESH_FLESH;
extern const LLUUID SND_ROLL_FLESH_PLASTIC;
extern const LLUUID SND_ROLL_FLESH_PLASTIC_02;
extern const LLUUID SND_ROLL_FLESH_RUBBER;
extern const LLUUID SND_ROLL_GLASS_GRAVEL;
extern const LLUUID SND_ROLL_GLASS_GRAVEL_02;
extern const LLUUID SND_ROLL_GLASS_FLESH;
extern const LLUUID SND_ROLL_GLASS_GLASS;
extern const LLUUID SND_ROLL_GLASS_PLASTIC;
extern const LLUUID SND_ROLL_GLASS_RUBBER;
extern const LLUUID SND_ROLL_GLASS_WOOD;
extern const LLUUID SND_ROLL_GLASS_WOOD_02;
extern const LLUUID SND_ROLL_GRAVEL_GRAVEL;
extern const LLUUID SND_ROLL_GRAVEL_GRAVEL_02;
extern const LLUUID SND_ROLL_METAL_FABRIC;
extern const LLUUID SND_ROLL_METAL_FABRIC_02;
extern const LLUUID SND_ROLL_METAL_FLESH;
extern const LLUUID SND_ROLL_METAL_GLASS;
extern const LLUUID SND_ROLL_METAL_GLASS_02;
extern const LLUUID SND_ROLL_METAL_GLASS_03;
extern const LLUUID SND_ROLL_METAL_GRAVEL;
extern const LLUUID SND_ROLL_METAL_METAL;
extern const LLUUID SND_ROLL_METAL_METAL_02;
extern const LLUUID SND_ROLL_METAL_METAL_03;
extern const LLUUID SND_ROLL_METAL_METAL_04;
extern const LLUUID SND_ROLL_METAL_PLASTIC;
extern const LLUUID SND_ROLL_METAL_PLASTIC_01;
extern const LLUUID SND_ROLL_METAL_RUBBER;
extern const LLUUID SND_ROLL_METAL_WOOD;
extern const LLUUID SND_ROLL_METAL_WOOD_02;
extern const LLUUID SND_ROLL_METAL_WOOD_03;
extern const LLUUID SND_ROLL_METAL_WOOD_04;
extern const LLUUID SND_ROLL_METAL_WOOD_05;
extern const LLUUID SND_ROLL_PLASTIC_FABRIC;
extern const LLUUID SND_ROLL_PLASTIC_PLASTIC;
extern const LLUUID SND_ROLL_PLASTIC_PLASTIC_02;
extern const LLUUID SND_ROLL_RUBBER_PLASTIC;
extern const LLUUID SND_ROLL_RUBBER_RUBBER;
extern const LLUUID SND_ROLL_STONE_FLESH;
extern const LLUUID SND_ROLL_STONE_GLASS;
extern const LLUUID SND_ROLL_STONE_METAL;
extern const LLUUID SND_ROLL_STONE_PLASTIC;
extern const LLUUID SND_ROLL_STONE_RUBBER;
extern const LLUUID SND_ROLL_STONE_STONE;
extern const LLUUID SND_ROLL_STONE_STONE_02;
extern const LLUUID SND_ROLL_STONE_STONE_03;
extern const LLUUID SND_ROLL_STONE_STONE_04;
extern const LLUUID SND_ROLL_STONE_STONE_05;
extern const LLUUID SND_ROLL_STONE_WOOD;
extern const LLUUID SND_ROLL_STONE_WOOD_02;
extern const LLUUID SND_ROLL_STONE_WOOD_03;
extern const LLUUID SND_ROLL_STONE_WOOD_04;
extern const LLUUID SND_ROLL_WOOD_FLESH;
extern const LLUUID SND_ROLL_WOOD_FLESH_02;
extern const LLUUID SND_ROLL_WOOD_FLESH_03;
extern const LLUUID SND_ROLL_WOOD_FLESH_04;
extern const LLUUID SND_ROLL_WOOD_GRAVEL;
extern const LLUUID SND_ROLL_WOOD_GRAVEL_02;
extern const LLUUID SND_ROLL_WOOD_GRAVEL_03;
extern const LLUUID SND_ROLL_WOOD_PLASTIC;
extern const LLUUID SND_ROLL_WOOD_PLASTIC_02;
extern const LLUUID SND_ROLL_WOOD_RUBBER;
extern const LLUUID SND_ROLL_WOOD_WOOD;
extern const LLUUID SND_ROLL_WOOD_WOOD_02;
extern const LLUUID SND_ROLL_WOOD_WOOD_03;
extern const LLUUID SND_ROLL_WOOD_WOOD_04;
extern const LLUUID SND_ROLL_WOOD_WOOD_05;
extern const LLUUID SND_ROLL_WOOD_WOOD_06;
extern const LLUUID SND_ROLL_WOOD_WOOD_07;
extern const LLUUID SND_ROLL_WOOD_WOOD_08;
extern const LLUUID SND_ROLL_WOOD_WOOD_09;

extern const LLUUID SND_SLIDE_STONE_STONE_01;

extern const LLUUID SND_STONE_DIRT_01;
extern const LLUUID SND_STONE_DIRT_02;
extern const LLUUID SND_STONE_DIRT_03;
extern const LLUUID SND_STONE_DIRT_04;

extern const LLUUID SND_STONE_STONE_02;
extern const LLUUID SND_STONE_STONE_04;

#endif
