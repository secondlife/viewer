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

static constexpr F32 REGION_WIDTH_METERS = 256.f;
static constexpr S32 REGION_WIDTH_UNITS = 256;
static constexpr U32 REGION_WIDTH_U32 = 256;

constexpr F32 REGION_HEIGHT_METERS = 4096.f;

constexpr   F32     DEFAULT_AGENT_DEPTH     = 0.45f;
constexpr   F32     DEFAULT_AGENT_WIDTH     = 0.60f;
constexpr   F32     DEFAULT_AGENT_HEIGHT    = 1.9f;

enum ETerrainBrushType
{
    // the valid brush numbers cannot be reordered, because they
    // are used in the binary LSL format as arguments to llModifyLand()
    E_LANDBRUSH_LEVEL   = 0,
    E_LANDBRUSH_RAISE   = 1,
    E_LANDBRUSH_LOWER   = 2,
    E_LANDBRUSH_SMOOTH  = 3,
    E_LANDBRUSH_NOISE   = 4,
    E_LANDBRUSH_REVERT  = 5,
    E_LANDBRUSH_INVALID = 6
};

enum EMouseClickType{
    CLICK_NONE = -1,
    CLICK_LEFT = 0,
    CLICK_MIDDLE,
    CLICK_RIGHT,
    CLICK_BUTTON4,
    CLICK_BUTTON5,
    CLICK_DOUBLELEFT,
    CLICK_COUNT // 'size', CLICK_NONE does not counts
};

// keys
// Bit masks for various keyboard modifier keys.
constexpr MASK MASK_NONE =          0x0000;
constexpr MASK MASK_CONTROL =       0x0001;     // Mapped to cmd on Macs
constexpr MASK MASK_ALT =           0x0002;
constexpr MASK MASK_SHIFT =         0x0004;
constexpr MASK MASK_NORMALKEYS =    0x0007;     // A real mask - only get the bits for normal modifier keys
constexpr MASK MASK_MAC_CONTROL =   0x0008;     // Un-mapped Ctrl key on Macs, not used on Windows
constexpr MASK MASK_MODIFIERS =     MASK_CONTROL|MASK_ALT|MASK_SHIFT|MASK_MAC_CONTROL;

// Special keys go into >128
constexpr KEY KEY_SPECIAL = 0x80;   // special keys start here
constexpr KEY KEY_RETURN =  0x81;
constexpr KEY KEY_LEFT =    0x82;
constexpr KEY KEY_RIGHT =   0x83;
constexpr KEY KEY_UP =      0x84;
constexpr KEY KEY_DOWN =    0x85;
constexpr KEY KEY_ESCAPE =  0x86;
constexpr KEY KEY_BACKSPACE =0x87;
constexpr KEY KEY_DELETE =  0x88;
constexpr KEY KEY_SHIFT =   0x89;
constexpr KEY KEY_CONTROL = 0x8A;
constexpr KEY KEY_ALT =     0x8B;
constexpr KEY KEY_HOME =    0x8C;
constexpr KEY KEY_END =     0x8D;
constexpr KEY KEY_PAGE_UP = 0x8E;
constexpr KEY KEY_PAGE_DOWN = 0x8F;
constexpr KEY KEY_HYPHEN = 0x90;
constexpr KEY KEY_EQUALS = 0x91;
constexpr KEY KEY_INSERT = 0x92;
constexpr KEY KEY_CAPSLOCK = 0x93;
constexpr KEY KEY_TAB =     0x94;
constexpr KEY KEY_ADD =     0x95;
constexpr KEY KEY_SUBTRACT =0x96;
constexpr KEY KEY_MULTIPLY =0x97;
constexpr KEY KEY_DIVIDE =  0x98;
constexpr KEY KEY_F1        = 0xA1;
constexpr KEY KEY_F2        = 0xA2;
constexpr KEY KEY_F3        = 0xA3;
constexpr KEY KEY_F4        = 0xA4;
constexpr KEY KEY_F5        = 0xA5;
constexpr KEY KEY_F6        = 0xA6;
constexpr KEY KEY_F7        = 0xA7;
constexpr KEY KEY_F8        = 0xA8;
constexpr KEY KEY_F9        = 0xA9;
constexpr KEY KEY_F10       = 0xAA;
constexpr KEY KEY_F11       = 0xAB;
constexpr KEY KEY_F12       = 0xAC;

constexpr KEY KEY_PAD_UP        = 0xC0;
constexpr KEY KEY_PAD_DOWN      = 0xC1;
constexpr KEY KEY_PAD_LEFT      = 0xC2;
constexpr KEY KEY_PAD_RIGHT     = 0xC3;
constexpr KEY KEY_PAD_HOME      = 0xC4;
constexpr KEY KEY_PAD_END       = 0xC5;
constexpr KEY KEY_PAD_PGUP      = 0xC6;
constexpr KEY KEY_PAD_PGDN      = 0xC7;
constexpr KEY KEY_PAD_CENTER    = 0xC8; // the 5 in the middle
constexpr KEY KEY_PAD_INS       = 0xC9;
constexpr KEY KEY_PAD_DEL       = 0xCA;
constexpr KEY KEY_PAD_RETURN    = 0xCB;
constexpr KEY KEY_PAD_ADD       = 0xCC; // not used
constexpr KEY KEY_PAD_SUBTRACT  = 0xCD; // not used
constexpr KEY KEY_PAD_MULTIPLY  = 0xCE; // not used
constexpr KEY KEY_PAD_DIVIDE    = 0xCF; // not used

constexpr KEY KEY_BUTTON0   = 0xD0;
constexpr KEY KEY_BUTTON1   = 0xD1;
constexpr KEY KEY_BUTTON2   = 0xD2;
constexpr KEY KEY_BUTTON3   = 0xD3;
constexpr KEY KEY_BUTTON4   = 0xD4;
constexpr KEY KEY_BUTTON5   = 0xD5;
constexpr KEY KEY_BUTTON6   = 0xD6;
constexpr KEY KEY_BUTTON7   = 0xD7;
constexpr KEY KEY_BUTTON8   = 0xD8;
constexpr KEY KEY_BUTTON9   = 0xD9;
constexpr KEY KEY_BUTTON10  = 0xDA;
constexpr KEY KEY_BUTTON11  = 0xDB;
constexpr KEY KEY_BUTTON12  = 0xDC;
constexpr KEY KEY_BUTTON13  = 0xDD;
constexpr KEY KEY_BUTTON14  = 0xDE;
constexpr KEY KEY_BUTTON15  = 0xDF;

constexpr KEY KEY_NONE =    0xFF; // not sent from keyboard.  For internal use only.

constexpr S32 KEY_COUNT = 256;


constexpr F32 DEFAULT_WATER_HEIGHT  = 20.0f;

// Maturity ratings for simulators
constexpr U8 SIM_ACCESS_MIN     = 0;        // Treated as 'unknown', usually ends up being SIM_ACCESS_PG
constexpr U8 SIM_ACCESS_PG      = 13;
constexpr U8 SIM_ACCESS_MATURE  = 21;
constexpr U8 SIM_ACCESS_ADULT   = 42;       // Seriously Adult Only
constexpr U8 SIM_ACCESS_DOWN    = 254;
constexpr U8 SIM_ACCESS_MAX     = SIM_ACCESS_ADULT;

// attachment constants
constexpr U8  ATTACHMENT_ADD = 0x80;

// god levels
constexpr U8 GOD_MAINTENANCE = 250;
constexpr U8 GOD_FULL = 200;
constexpr U8 GOD_LIAISON = 150;
constexpr U8 GOD_CUSTOMER_SERVICE = 100;
constexpr U8 GOD_LIKE = 1;
constexpr U8 GOD_NOT = 0;

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
LL_COMMON_API extern const LLUUID IMG_WHITE;

LL_COMMON_API extern const LLUUID IMG_EXPLOSION;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_2;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_3;
LL_COMMON_API extern const LLUUID IMG_EXPLOSION_4;
LL_COMMON_API extern const LLUUID IMG_SMOKE_POOF;

LL_COMMON_API extern const LLUUID IMG_BIG_EXPLOSION_1;
LL_COMMON_API extern const LLUUID IMG_BIG_EXPLOSION_2;

LL_COMMON_API extern const LLUUID IMG_ALPHA_GRAD;
LL_COMMON_API extern const LLUUID IMG_ALPHA_GRAD_2D;
LL_COMMON_API extern const LLUUID IMG_TRANSPARENT;

LL_COMMON_API extern const LLUUID TERRAIN_DIRT_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_GRASS_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_MOUNTAIN_DETAIL;
LL_COMMON_API extern const LLUUID TERRAIN_ROCK_DETAIL;

LL_COMMON_API extern const LLUUID IMG_USE_BAKED_HEAD;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_UPPER;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_LOWER;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_EYES;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_SKIRT;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_HAIR;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_LEFTARM;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_LEFTLEG;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_AUX1;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_AUX2;
LL_COMMON_API extern const LLUUID IMG_USE_BAKED_AUX3;

LL_COMMON_API extern const LLUUID DEFAULT_WATER_NORMAL;

LL_COMMON_API extern const LLUUID DEFAULT_OBJECT_TEXTURE;
LL_COMMON_API extern const LLUUID DEFAULT_OBJECT_SPECULAR;
LL_COMMON_API extern const LLUUID DEFAULT_OBJECT_NORMAL;
LL_COMMON_API extern const LLUUID BLANK_OBJECT_NORMAL;

LL_COMMON_API extern const LLUUID BLANK_MATERIAL_ASSET_ID;

// radius within which a chat message is fully audible
constexpr F32 CHAT_NORMAL_RADIUS = 20.f;

// media commands
constexpr U32 PARCEL_MEDIA_COMMAND_STOP  = 0;
constexpr U32 PARCEL_MEDIA_COMMAND_PAUSE = 1;
constexpr U32 PARCEL_MEDIA_COMMAND_PLAY  = 2;
constexpr U32 PARCEL_MEDIA_COMMAND_LOOP  = 3;
constexpr U32 PARCEL_MEDIA_COMMAND_TEXTURE = 4;
constexpr U32 PARCEL_MEDIA_COMMAND_URL = 5;
constexpr U32 PARCEL_MEDIA_COMMAND_TIME = 6;
constexpr U32 PARCEL_MEDIA_COMMAND_AGENT = 7;
constexpr U32 PARCEL_MEDIA_COMMAND_UNLOAD = 8;
constexpr U32 PARCEL_MEDIA_COMMAND_AUTO_ALIGN = 9;
constexpr U32 PARCEL_MEDIA_COMMAND_TYPE = 10;
constexpr U32 PARCEL_MEDIA_COMMAND_SIZE = 11;
constexpr U32 PARCEL_MEDIA_COMMAND_DESC = 12;
constexpr U32 PARCEL_MEDIA_COMMAND_LOOP_SET = 13;

const S32 CHAT_CHANNEL_DEBUG = S32_MAX;

// agent constants
constexpr U32 CONTROL_AT_POS_INDEX              = 0;
constexpr U32 CONTROL_AT_NEG_INDEX              = 1;
constexpr U32 CONTROL_LEFT_POS_INDEX            = 2;
constexpr U32 CONTROL_LEFT_NEG_INDEX            = 3;
constexpr U32 CONTROL_UP_POS_INDEX              = 4;
constexpr U32 CONTROL_UP_NEG_INDEX              = 5;
constexpr U32 CONTROL_PITCH_POS_INDEX           = 6;
constexpr U32 CONTROL_PITCH_NEG_INDEX           = 7;
constexpr U32 CONTROL_YAW_POS_INDEX             = 8;
constexpr U32 CONTROL_YAW_NEG_INDEX             = 9;
constexpr U32 CONTROL_FAST_AT_INDEX             = 10;
constexpr U32 CONTROL_FAST_LEFT_INDEX           = 11;
constexpr U32 CONTROL_FAST_UP_INDEX             = 12;
constexpr U32 CONTROL_FLY_INDEX                 = 13;
constexpr U32 CONTROL_STOP_INDEX                = 14;
constexpr U32 CONTROL_FINISH_ANIM_INDEX         = 15;
constexpr U32 CONTROL_STAND_UP_INDEX            = 16;
constexpr U32 CONTROL_SIT_ON_GROUND_INDEX       = 17;
constexpr U32 CONTROL_MOUSELOOK_INDEX           = 18;
constexpr U32 CONTROL_NUDGE_AT_POS_INDEX        = 19;
constexpr U32 CONTROL_NUDGE_AT_NEG_INDEX        = 20;
constexpr U32 CONTROL_NUDGE_LEFT_POS_INDEX      = 21;
constexpr U32 CONTROL_NUDGE_LEFT_NEG_INDEX      = 22;
constexpr U32 CONTROL_NUDGE_UP_POS_INDEX        = 23;
constexpr U32 CONTROL_NUDGE_UP_NEG_INDEX        = 24;
constexpr U32 CONTROL_TURN_LEFT_INDEX           = 25;
constexpr U32 CONTROL_TURN_RIGHT_INDEX          = 26;
constexpr U32 CONTROL_AWAY_INDEX                = 27;
constexpr U32 CONTROL_LBUTTON_DOWN_INDEX        = 28;
constexpr U32 CONTROL_LBUTTON_UP_INDEX          = 29;
constexpr U32 CONTROL_ML_LBUTTON_DOWN_INDEX     = 30;
constexpr U32 CONTROL_ML_LBUTTON_UP_INDEX       = 31;
constexpr U32 TOTAL_CONTROLS                    = 32;

constexpr U32 AGENT_CONTROL_AT_POS              = 0x1 << CONTROL_AT_POS_INDEX;          // 0x00000001
constexpr U32 AGENT_CONTROL_AT_NEG              = 0x1 << CONTROL_AT_NEG_INDEX;          // 0x00000002
constexpr U32 AGENT_CONTROL_LEFT_POS            = 0x1 << CONTROL_LEFT_POS_INDEX;        // 0x00000004
constexpr U32 AGENT_CONTROL_LEFT_NEG            = 0x1 << CONTROL_LEFT_NEG_INDEX;        // 0x00000008
constexpr U32 AGENT_CONTROL_UP_POS              = 0x1 << CONTROL_UP_POS_INDEX;          // 0x00000010
constexpr U32 AGENT_CONTROL_UP_NEG              = 0x1 << CONTROL_UP_NEG_INDEX;          // 0x00000020
constexpr U32 AGENT_CONTROL_PITCH_POS           = 0x1 << CONTROL_PITCH_POS_INDEX;       // 0x00000040
constexpr U32 AGENT_CONTROL_PITCH_NEG           = 0x1 << CONTROL_PITCH_NEG_INDEX;       // 0x00000080
constexpr U32 AGENT_CONTROL_YAW_POS             = 0x1 << CONTROL_YAW_POS_INDEX;         // 0x00000100
constexpr U32 AGENT_CONTROL_YAW_NEG             = 0x1 << CONTROL_YAW_NEG_INDEX;         // 0x00000200

constexpr U32 AGENT_CONTROL_FAST_AT             = 0x1 << CONTROL_FAST_AT_INDEX;         // 0x00000400
constexpr U32 AGENT_CONTROL_FAST_LEFT           = 0x1 << CONTROL_FAST_LEFT_INDEX;       // 0x00000800
constexpr U32 AGENT_CONTROL_FAST_UP             = 0x1 << CONTROL_FAST_UP_INDEX;         // 0x00001000

constexpr U32 AGENT_CONTROL_FLY                 = 0x1 << CONTROL_FLY_INDEX;             // 0x00002000
constexpr U32 AGENT_CONTROL_STOP                = 0x1 << CONTROL_STOP_INDEX;            // 0x00004000
constexpr U32 AGENT_CONTROL_FINISH_ANIM         = 0x1 << CONTROL_FINISH_ANIM_INDEX;     // 0x00008000
constexpr U32 AGENT_CONTROL_STAND_UP            = 0x1 << CONTROL_STAND_UP_INDEX;        // 0x00010000
constexpr U32 AGENT_CONTROL_SIT_ON_GROUND       = 0x1 << CONTROL_SIT_ON_GROUND_INDEX;   // 0x00020000
constexpr U32 AGENT_CONTROL_MOUSELOOK           = 0x1 << CONTROL_MOUSELOOK_INDEX;       // 0x00040000

constexpr U32 AGENT_CONTROL_NUDGE_AT_POS        = 0x1 << CONTROL_NUDGE_AT_POS_INDEX;    // 0x00080000
constexpr U32 AGENT_CONTROL_NUDGE_AT_NEG        = 0x1 << CONTROL_NUDGE_AT_NEG_INDEX;    // 0x00100000
constexpr U32 AGENT_CONTROL_NUDGE_LEFT_POS      = 0x1 << CONTROL_NUDGE_LEFT_POS_INDEX;  // 0x00200000
constexpr U32 AGENT_CONTROL_NUDGE_LEFT_NEG      = 0x1 << CONTROL_NUDGE_LEFT_NEG_INDEX;  // 0x00400000
constexpr U32 AGENT_CONTROL_NUDGE_UP_POS        = 0x1 << CONTROL_NUDGE_UP_POS_INDEX;    // 0x00800000
constexpr U32 AGENT_CONTROL_NUDGE_UP_NEG        = 0x1 << CONTROL_NUDGE_UP_NEG_INDEX;    // 0x01000000
constexpr U32 AGENT_CONTROL_TURN_LEFT           = 0x1 << CONTROL_TURN_LEFT_INDEX;       // 0x02000000
constexpr U32 AGENT_CONTROL_TURN_RIGHT          = 0x1 << CONTROL_TURN_RIGHT_INDEX;      // 0x04000000

constexpr U32 AGENT_CONTROL_AWAY                = 0x1 << CONTROL_AWAY_INDEX;            // 0x08000000

constexpr U32 AGENT_CONTROL_LBUTTON_DOWN        = 0x1 << CONTROL_LBUTTON_DOWN_INDEX;    // 0x10000000
constexpr U32 AGENT_CONTROL_LBUTTON_UP          = 0x1 << CONTROL_LBUTTON_UP_INDEX;      // 0x20000000
constexpr U32 AGENT_CONTROL_ML_LBUTTON_DOWN     = 0x1 << CONTROL_ML_LBUTTON_DOWN_INDEX; // 0x40000000
constexpr U32 AGENT_CONTROL_ML_LBUTTON_UP       = ((U32)0x1) << CONTROL_ML_LBUTTON_UP_INDEX;    // 0x80000000

// move these up so that we can hide them in "State" for object updates
// (for now)
constexpr U32 AGENT_ATTACH_OFFSET               = 4;
constexpr U32 AGENT_ATTACH_MASK                 = 0xf << AGENT_ATTACH_OFFSET;

// RN: this method swaps the upper and lower nibbles to maintain backward
// compatibility with old objects that only used the upper nibble
#define ATTACHMENT_ID_FROM_STATE(state) ((S32)((((U8)state & AGENT_ATTACH_MASK) >> 4) | (((U8)state & ~AGENT_ATTACH_MASK) << 4)))

// DO NOT CHANGE THE SEQUENCE OF THIS LIST!!
constexpr U8 CLICK_ACTION_NONE = 0;
constexpr U8 CLICK_ACTION_TOUCH = 0;
constexpr U8 CLICK_ACTION_SIT = 1;
constexpr U8 CLICK_ACTION_BUY = 2;
constexpr U8 CLICK_ACTION_PAY = 3;
constexpr U8 CLICK_ACTION_OPEN = 4;
constexpr U8 CLICK_ACTION_PLAY = 5;
constexpr U8 CLICK_ACTION_OPEN_MEDIA = 6;
constexpr U8 CLICK_ACTION_ZOOM = 7;
constexpr U8 CLICK_ACTION_DISABLED = 8;
constexpr U8 CLICK_ACTION_IGNORE = 9;
// DO NOT CHANGE THE SEQUENCE OF THIS LIST!!

constexpr U32 BEACON_SHOW_MAP  = 0x0001;
constexpr U32 BEACON_FOCUS_MAP = 0x0002;

#endif
