/** 
 * @file indra_constants.h
 * @brief some useful short term constants for Indra
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

#ifndef LL_INDRA_CONSTANTS_H
#define LL_INDRA_CONSTANTS_H

#include "stdtypes.h"

class LLUUID;

static const F32 REGION_WIDTH_METERS = 256.f;
static const S32 REGION_WIDTH_UNITS = 256;
static const U32 REGION_WIDTH_U32 = 256;

const F32 REGION_HEIGHT_METERS = 4096.f;

const 	F32 	DEFAULT_AGENT_DEPTH 	= 0.45f;
const 	F32 	DEFAULT_AGENT_WIDTH 	= 0.60f;
const 	F32 	DEFAULT_AGENT_HEIGHT	= 1.9f;

enum ETerrainBrushType
{
	// the valid brush numbers cannot be reordered, because they 
	// are used in the binary LSL format as arguments to llModifyLand()
	E_LANDBRUSH_LEVEL	= 0,
	E_LANDBRUSH_RAISE	= 1,
	E_LANDBRUSH_LOWER	= 2,
	E_LANDBRUSH_SMOOTH 	= 3,
	E_LANDBRUSH_NOISE	= 4,
	E_LANDBRUSH_REVERT 	= 5,
	E_LANDBRUSH_INVALID = 6
};

// keys
// Bit masks for various keyboard modifier keys.
const MASK MASK_NONE =			0x0000;
const MASK MASK_CONTROL =		0x0001;		// Mapped to cmd on Macs
const MASK MASK_ALT =			0x0002;
const MASK MASK_SHIFT =			0x0004;
const MASK MASK_NORMALKEYS =    0x0007;     // A real mask - only get the bits for normal modifier keys
const MASK MASK_MAC_CONTROL =	0x0008;		// Un-mapped Ctrl key on Macs, not used on Windows
const MASK MASK_MODIFIERS =		MASK_CONTROL|MASK_ALT|MASK_SHIFT|MASK_MAC_CONTROL;

// Special keys go into >128
const KEY KEY_SPECIAL = 0x80;	// special keys start here
const KEY KEY_RETURN =	0x81;
const KEY KEY_LEFT =	0x82;
const KEY KEY_RIGHT =	0x83;
const KEY KEY_UP =		0x84;
const KEY KEY_DOWN =	0x85;
const KEY KEY_ESCAPE =	0x86;
const KEY KEY_BACKSPACE =0x87;
const KEY KEY_DELETE =	0x88;
const KEY KEY_SHIFT =	0x89;
const KEY KEY_CONTROL =	0x8A;
const KEY KEY_ALT =		0x8B;
const KEY KEY_HOME =	0x8C;
const KEY KEY_END =		0x8D;
const KEY KEY_PAGE_UP = 0x8E;
const KEY KEY_PAGE_DOWN = 0x8F;
const KEY KEY_HYPHEN = 0x90;
const KEY KEY_EQUALS = 0x91;
const KEY KEY_INSERT = 0x92;
const KEY KEY_CAPSLOCK = 0x93;
const KEY KEY_TAB =		0x94;
const KEY KEY_ADD = 	0x95;
const KEY KEY_SUBTRACT =0x96;
const KEY KEY_MULTIPLY =0x97;
const KEY KEY_DIVIDE =	0x98;
const KEY KEY_F1		= 0xA1;
const KEY KEY_F2		= 0xA2;
const KEY KEY_F3		= 0xA3;
const KEY KEY_F4		= 0xA4;
const KEY KEY_F5		= 0xA5;
const KEY KEY_F6		= 0xA6;
const KEY KEY_F7		= 0xA7;
const KEY KEY_F8		= 0xA8;
const KEY KEY_F9		= 0xA9;
const KEY KEY_F10		= 0xAA;
const KEY KEY_F11		= 0xAB;
const KEY KEY_F12		= 0xAC;

const KEY KEY_PAD_UP		= 0xC0;
const KEY KEY_PAD_DOWN		= 0xC1;
const KEY KEY_PAD_LEFT		= 0xC2;
const KEY KEY_PAD_RIGHT		= 0xC3;
const KEY KEY_PAD_HOME		= 0xC4;
const KEY KEY_PAD_END		= 0xC5;
const KEY KEY_PAD_PGUP		= 0xC6;
const KEY KEY_PAD_PGDN		= 0xC7;
const KEY KEY_PAD_CENTER	= 0xC8; // the 5 in the middle
const KEY KEY_PAD_INS		= 0xC9;
const KEY KEY_PAD_DEL		= 0xCA;
const KEY KEY_PAD_RETURN	= 0xCB;
const KEY KEY_PAD_ADD		= 0xCC; // not used
const KEY KEY_PAD_SUBTRACT	= 0xCD; // not used
const KEY KEY_PAD_MULTIPLY  = 0xCE; // not used
const KEY KEY_PAD_DIVIDE	= 0xCF; // not used

const KEY KEY_BUTTON0	= 0xD0;
const KEY KEY_BUTTON1	= 0xD1;
const KEY KEY_BUTTON2	= 0xD2;
const KEY KEY_BUTTON3	= 0xD3;
const KEY KEY_BUTTON4	= 0xD4;
const KEY KEY_BUTTON5	= 0xD5;
const KEY KEY_BUTTON6	= 0xD6;
const KEY KEY_BUTTON7	= 0xD7;
const KEY KEY_BUTTON8	= 0xD8;
const KEY KEY_BUTTON9	= 0xD9;
const KEY KEY_BUTTON10	= 0xDA;
const KEY KEY_BUTTON11	= 0xDB;
const KEY KEY_BUTTON12	= 0xDC;
const KEY KEY_BUTTON13	= 0xDD;
const KEY KEY_BUTTON14	= 0xDE;
const KEY KEY_BUTTON15	= 0xDF;

const KEY KEY_NONE =	0xFF; // not sent from keyboard.  For internal use only.

const S32 KEY_COUNT = 256;


const F32 DEFAULT_WATER_HEIGHT 	= 20.0f;

// Maturity ratings for simulators
const U8 SIM_ACCESS_MIN 	= 0;		// Treated as 'unknown', usually ends up being SIM_ACCESS_PG
const U8 SIM_ACCESS_PG		= 13;
const U8 SIM_ACCESS_MATURE	= 21;
const U8 SIM_ACCESS_ADULT	= 42;		// Seriously Adult Only
const U8 SIM_ACCESS_DOWN	= 254;
const U8 SIM_ACCESS_MAX 	= SIM_ACCESS_ADULT;

// attachment constants
const S32 MAX_AGENT_ATTACHMENTS = 38;
const U8  ATTACHMENT_ADD = 0x80;

// god levels
const U8 GOD_MAINTENANCE = 250;
const U8 GOD_FULL = 200;
const U8 GOD_LIAISON = 150;
const U8 GOD_CUSTOMER_SERVICE = 100;
const U8 GOD_LIKE = 1;
const U8 GOD_NOT = 0;

// "agent id" for things that should be done to ALL agents
LL_COMMON_API extern const LLUUID LL_UUID_ALL_AGENTS;

// inventory library owner
LL_COMMON_API extern const LLUUID ALEXANDRIA_LINDEN_ID;

LL_COMMON_API extern const LLUUID GOVERNOR_LINDEN_ID;
// Maintenance's group id.
LL_COMMON_API extern const LLUUID MAINTENANCE_GROUP_ID;

// image ids
LL_COMMON_API extern const LLUUID IMG_SMOKE;

LL_COMMON_API extern const LLUUID IMG_DEFAULT;

LL_COMMON_API extern const LLUUID IMG_SUN;
LL_COMMON_API extern const LLUUID IMG_MOON;
LL_COMMON_API extern const LLUUID IMG_SHOT;
LL_COMMON_API extern const LLUUID IMG_SPARK;
LL_COMMON_API extern const LLUUID IMG_FIRE;
LL_COMMON_API extern const LLUUID IMG_FACE_SELECT;
LL_COMMON_API extern const LLUUID IMG_DEFAULT_AVATAR;
LL_COMMON_API extern const LLUUID IMG_INVISIBLE;

LL_COMMON_API extern const LLUUID IMG_EXPLOSION;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_2;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_3;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_4;
LL_COMMON_API extern const LLUUID IMG_SMOKE_POOF;

LL_COMMON_API extern const LLUUID IMG_BIG_EXPLOSION_1;
LL_COMMON_API extern const LLUUID IMG_BIG_EXPLOSION_2;

LL_COMMON_API extern const LLUUID IMG_BLOOM1;
LL_COMMON_API extern const LLUUID TERRAIN_DIRT_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_GRASS_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_MOUNTAIN_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_ROCK_DETAIL;

LL_COMMON_API extern const LLUUID DEFAULT_WATER_NORMAL;


// radius within which a chat message is fully audible
const F32 CHAT_NORMAL_RADIUS = 20.f;

// media commands
const U32 PARCEL_MEDIA_COMMAND_STOP  = 0;
const U32 PARCEL_MEDIA_COMMAND_PAUSE = 1;
const U32 PARCEL_MEDIA_COMMAND_PLAY  = 2;
const U32 PARCEL_MEDIA_COMMAND_LOOP  = 3;
const U32 PARCEL_MEDIA_COMMAND_TEXTURE = 4;
const U32 PARCEL_MEDIA_COMMAND_URL = 5;
const U32 PARCEL_MEDIA_COMMAND_TIME = 6;
const U32 PARCEL_MEDIA_COMMAND_AGENT = 7;
const U32 PARCEL_MEDIA_COMMAND_UNLOAD = 8;
const U32 PARCEL_MEDIA_COMMAND_AUTO_ALIGN = 9;
const U32 PARCEL_MEDIA_COMMAND_TYPE = 10;
const U32 PARCEL_MEDIA_COMMAND_SIZE = 11;
const U32 PARCEL_MEDIA_COMMAND_DESC = 12;
const U32 PARCEL_MEDIA_COMMAND_LOOP_SET = 13;

const S32 CHAT_CHANNEL_DEBUG = S32_MAX;

// agent constants
const U32 CONTROL_AT_POS_INDEX				= 0;
const U32 CONTROL_AT_NEG_INDEX				= 1;
const U32 CONTROL_LEFT_POS_INDEX			= 2;
const U32 CONTROL_LEFT_NEG_INDEX			= 3;
const U32 CONTROL_UP_POS_INDEX				= 4;
const U32 CONTROL_UP_NEG_INDEX				= 5;
const U32 CONTROL_PITCH_POS_INDEX			= 6;
const U32 CONTROL_PITCH_NEG_INDEX			= 7;
const U32 CONTROL_YAW_POS_INDEX				= 8;
const U32 CONTROL_YAW_NEG_INDEX				= 9;
const U32 CONTROL_FAST_AT_INDEX				= 10;
const U32 CONTROL_FAST_LEFT_INDEX			= 11;
const U32 CONTROL_FAST_UP_INDEX				= 12;
const U32 CONTROL_FLY_INDEX					= 13;
const U32 CONTROL_STOP_INDEX				= 14;
const U32 CONTROL_FINISH_ANIM_INDEX			= 15;
const U32 CONTROL_STAND_UP_INDEX			= 16;
const U32 CONTROL_SIT_ON_GROUND_INDEX		= 17;
const U32 CONTROL_MOUSELOOK_INDEX			= 18;
const U32 CONTROL_NUDGE_AT_POS_INDEX		= 19;
const U32 CONTROL_NUDGE_AT_NEG_INDEX		= 20;
const U32 CONTROL_NUDGE_LEFT_POS_INDEX		= 21;
const U32 CONTROL_NUDGE_LEFT_NEG_INDEX		= 22;
const U32 CONTROL_NUDGE_UP_POS_INDEX		= 23;
const U32 CONTROL_NUDGE_UP_NEG_INDEX		= 24;
const U32 CONTROL_TURN_LEFT_INDEX			= 25;
const U32 CONTROL_TURN_RIGHT_INDEX			= 26;
const U32 CONTROL_AWAY_INDEX				= 27;
const U32 CONTROL_LBUTTON_DOWN_INDEX		= 28;
const U32 CONTROL_LBUTTON_UP_INDEX			= 29;
const U32 CONTROL_ML_LBUTTON_DOWN_INDEX		= 30;
const U32 CONTROL_ML_LBUTTON_UP_INDEX		= 31;
const U32 TOTAL_CONTROLS					= 32;

const U32 AGENT_CONTROL_AT_POS              = 0x1 << CONTROL_AT_POS_INDEX;			// 0x00000001
const U32 AGENT_CONTROL_AT_NEG              = 0x1 << CONTROL_AT_NEG_INDEX;			// 0x00000002
const U32 AGENT_CONTROL_LEFT_POS            = 0x1 << CONTROL_LEFT_POS_INDEX;		// 0x00000004
const U32 AGENT_CONTROL_LEFT_NEG            = 0x1 << CONTROL_LEFT_NEG_INDEX;		// 0x00000008
const U32 AGENT_CONTROL_UP_POS              = 0x1 << CONTROL_UP_POS_INDEX;			// 0x00000010
const U32 AGENT_CONTROL_UP_NEG              = 0x1 << CONTROL_UP_NEG_INDEX;			// 0x00000020
const U32 AGENT_CONTROL_PITCH_POS           = 0x1 << CONTROL_PITCH_POS_INDEX;		// 0x00000040
const U32 AGENT_CONTROL_PITCH_NEG           = 0x1 << CONTROL_PITCH_NEG_INDEX;		// 0x00000080
const U32 AGENT_CONTROL_YAW_POS             = 0x1 << CONTROL_YAW_POS_INDEX;			// 0x00000100
const U32 AGENT_CONTROL_YAW_NEG             = 0x1 << CONTROL_YAW_NEG_INDEX;			// 0x00000200

const U32 AGENT_CONTROL_FAST_AT             = 0x1 << CONTROL_FAST_AT_INDEX;			// 0x00000400
const U32 AGENT_CONTROL_FAST_LEFT           = 0x1 << CONTROL_FAST_LEFT_INDEX;		// 0x00000800
const U32 AGENT_CONTROL_FAST_UP             = 0x1 << CONTROL_FAST_UP_INDEX;			// 0x00001000

const U32 AGENT_CONTROL_FLY					= 0x1 << CONTROL_FLY_INDEX;				// 0x00002000
const U32 AGENT_CONTROL_STOP				= 0x1 << CONTROL_STOP_INDEX;			// 0x00004000
const U32 AGENT_CONTROL_FINISH_ANIM			= 0x1 << CONTROL_FINISH_ANIM_INDEX;		// 0x00008000
const U32 AGENT_CONTROL_STAND_UP			= 0x1 << CONTROL_STAND_UP_INDEX;		// 0x00010000
const U32 AGENT_CONTROL_SIT_ON_GROUND		= 0x1 << CONTROL_SIT_ON_GROUND_INDEX;	// 0x00020000
const U32 AGENT_CONTROL_MOUSELOOK			= 0x1 << CONTROL_MOUSELOOK_INDEX;		// 0x00040000

const U32 AGENT_CONTROL_NUDGE_AT_POS        = 0x1 << CONTROL_NUDGE_AT_POS_INDEX;	// 0x00080000
const U32 AGENT_CONTROL_NUDGE_AT_NEG        = 0x1 << CONTROL_NUDGE_AT_NEG_INDEX;	// 0x00100000
const U32 AGENT_CONTROL_NUDGE_LEFT_POS      = 0x1 << CONTROL_NUDGE_LEFT_POS_INDEX;	// 0x00200000
const U32 AGENT_CONTROL_NUDGE_LEFT_NEG      = 0x1 << CONTROL_NUDGE_LEFT_NEG_INDEX;	// 0x00400000
const U32 AGENT_CONTROL_NUDGE_UP_POS        = 0x1 << CONTROL_NUDGE_UP_POS_INDEX;	// 0x00800000
const U32 AGENT_CONTROL_NUDGE_UP_NEG        = 0x1 << CONTROL_NUDGE_UP_NEG_INDEX;	// 0x01000000
const U32 AGENT_CONTROL_TURN_LEFT	        = 0x1 << CONTROL_TURN_LEFT_INDEX;		// 0x02000000
const U32 AGENT_CONTROL_TURN_RIGHT	        = 0x1 << CONTROL_TURN_RIGHT_INDEX;		// 0x04000000

const U32 AGENT_CONTROL_AWAY				= 0x1 << CONTROL_AWAY_INDEX;			// 0x08000000

const U32 AGENT_CONTROL_LBUTTON_DOWN		= 0x1 << CONTROL_LBUTTON_DOWN_INDEX;	// 0x10000000
const U32 AGENT_CONTROL_LBUTTON_UP			= 0x1 << CONTROL_LBUTTON_UP_INDEX;		// 0x20000000
const U32 AGENT_CONTROL_ML_LBUTTON_DOWN		= 0x1 << CONTROL_ML_LBUTTON_DOWN_INDEX;	// 0x40000000
const U32 AGENT_CONTROL_ML_LBUTTON_UP		= ((U32)0x1) << CONTROL_ML_LBUTTON_UP_INDEX;	// 0x80000000

// move these up so that we can hide them in "State" for object updates 
// (for now)
const U32 AGENT_ATTACH_OFFSET				= 4;
const U32 AGENT_ATTACH_MASK					= 0xf << AGENT_ATTACH_OFFSET;

// RN: this method swaps the upper and lower nibbles to maintain backward 
// compatibility with old objects that only used the upper nibble
#define ATTACHMENT_ID_FROM_STATE(state) ((S32)((((U8)state & AGENT_ATTACH_MASK) >> 4) | (((U8)state & ~AGENT_ATTACH_MASK) << 4)))

// DO NOT CHANGE THE SEQUENCE OF THIS LIST!!
const U8 CLICK_ACTION_NONE = 0;
const U8 CLICK_ACTION_TOUCH = 0;
const U8 CLICK_ACTION_SIT = 1;
const U8 CLICK_ACTION_BUY = 2;
const U8 CLICK_ACTION_PAY = 3;
const U8 CLICK_ACTION_OPEN = 4;
const U8 CLICK_ACTION_PLAY = 5;
const U8 CLICK_ACTION_OPEN_MEDIA = 6;
const U8 CLICK_ACTION_ZOOM = 7;
// DO NOT CHANGE THE SEQUENCE OF THIS LIST!!


#endif
