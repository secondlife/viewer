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

// At 45 Hz collisions seem stable and objects seem
// to settle down at a reasonable rate.
// JC 3/18/2003

// const F32 PHYSICS_TIMESTEP = 1.f / 45.f;
// This must be a #define due to anal retentive restrictions on const expressions
// CG 2008-06-05
#define PHYSICS_TIMESTEP (1.f / 45.f)

const F32 COLLISION_TOLERANCE = 0.1f;
const F32 HALF_COLLISION_TOLERANCE = 0.05f;

// Time constants
const U32 HOURS_PER_LINDEN_DAY		= 4;	
const U32 DAYS_PER_LINDEN_YEAR		= 11;

const U32 SEC_PER_LINDEN_DAY		= HOURS_PER_LINDEN_DAY * 60 * 60;
const U32 SEC_PER_LINDEN_YEAR		= DAYS_PER_LINDEN_YEAR * SEC_PER_LINDEN_DAY;

static const F32 REGION_WIDTH_METERS = 256.f;
static const S32 REGION_WIDTH_UNITS = 256;
static const U32 REGION_WIDTH_U32 = 256;

const F32 REGION_HEIGHT_METERS = 4096.f;

// Bits for simulator performance query flags
enum LAND_STAT_FLAGS
{
	STAT_FILTER_BY_PARCEL	= 0x00000001,
	STAT_FILTER_BY_OWNER	= 0x00000002,
	STAT_FILTER_BY_OBJECT	= 0x00000004,
	STAT_FILTER_BY_PARCEL_NAME	= 0x00000008,
	STAT_REQUEST_LAST_ENTRY	= 0x80000000,
};

enum LAND_STAT_REPORT_TYPE
{
	STAT_REPORT_TOP_SCRIPTS = 0,
	STAT_REPORT_TOP_COLLIDERS
};

const U32 STAT_FILTER_MASK	= 0x1FFFFFFF;

// Region absolute limits
static const S32		REGION_AGENT_COUNT_MIN = 1;
static const S32		REGION_AGENT_COUNT_MAX = 200;			// Must fit in U8 for the moment (RegionInfo msg)
static const S32		REGION_PRIM_COUNT_MIN = 0;
static const S32		REGION_PRIM_COUNT_MAX = 40000;
static const F32		REGION_PRIM_BONUS_MIN = 1.0;
static const F32		REGION_PRIM_BONUS_MAX = 10.0;

// Default maximum number of tasks/prims per region.
const U32 DEFAULT_MAX_REGION_WIDE_PRIM_COUNT = 15000;

const 	F32 	MIN_AGENT_DEPTH			= 0.30f;
const 	F32 	DEFAULT_AGENT_DEPTH 	= 0.45f;
const 	F32 	MAX_AGENT_DEPTH			= 0.60f;

const 	F32 	MIN_AGENT_WIDTH 		= 0.40f;
const 	F32 	DEFAULT_AGENT_WIDTH 	= 0.60f;
const 	F32 	MAX_AGENT_WIDTH 		= 0.80f;

const 	F32 	MIN_AGENT_HEIGHT		= 1.1f;
const 	F32 	DEFAULT_AGENT_HEIGHT	= 1.9f;
const 	F32 	MAX_AGENT_HEIGHT		= 2.45f;

// For linked sets
const S32 MAX_CHILDREN_PER_TASK = 255;
const S32 MAX_CHILDREN_PER_PHYSICAL_TASK = 32;

const S32 MAX_JOINTS_PER_OBJECT = 1;	// limiting to 1 until Havok 2.x

const	char* const	DEFAULT_DMZ_SPACE_SERVER	= "192.168.0.140";
const	char* const	DEFAULT_DMZ_USER_SERVER		= "192.168.0.140";
const	char* const	DEFAULT_DMZ_DATA_SERVER		= "192.168.0.140";
const	char* const	DEFAULT_DMZ_ASSET_SERVER	= "http://asset.dmz.lindenlab.com:80";

const	char* const	DEFAULT_AGNI_SPACE_SERVER	= "63.211.139.100";
const	char* const	DEFAULT_AGNI_USER_SERVER	= "63.211.139.100";
const	char* const	DEFAULT_AGNI_DATA_SERVER	= "63.211.139.100";
const	char* const	DEFAULT_AGNI_ASSET_SERVER	= "http://asset.agni.lindenlab.com:80";

// Information about what ports are for what services is in the wiki Name Space Ports page
// https://wiki.lindenlab.com/wiki/Name_Space_Ports
const	char* const DEFAULT_LOCAL_ASSET_SERVER	= "http://localhost:12041/asset/tmp";
const	char* const	LOCAL_ASSET_URL_FORMAT		= "http://%s:12041/asset";

const	U32		DEFAULT_LAUNCHER_PORT			= 12029;
//const	U32		DEFAULT_BIGBOARD_PORT			= 12030; // Deprecated
//const	U32		DEFAULT_QUERYSIM_PORT			= 12031; // Deprecated
const	U32		DEFAULT_DATA_SERVER_PORT		= 12032;
const	U32		DEFAULT_SPACE_SERVER_PORT		= 12033;
const	U32		DEFAULT_VIEWER_PORT				= 12034;
const	U32		DEFAULT_SIMULATOR_PORT			= 12035;
const	U32		DEFAULT_USER_SERVER_PORT		= 12036;
const	U32		DEFAULT_RPC_SERVER_PORT			= 12037;
const	U32		DEFAULT_LOG_DATA_SERVER_PORT	= 12039;
const	U32		DEFAULT_BACKBONE_PORT			= 12040;
const   U32		DEFAULT_LOCAL_ASSET_PORT		= 12041;
//const   U32		DEFAULT_BACKBONE_CAP_PORT		= 12042; // Deprecated
const   U32		DEFAULT_CAP_PROXY_PORT			= 12043;
const   U32		DEFAULT_INV_DATA_SERVER_PORT	= 12044;
const	U32		DEFAULT_CGI_SERVICES_PORT		= 12045;

// Mapserver uses ports 12124 - 12139 to allow multiple mapservers to run
// on a single host for map tile generation. JC
const	U32		DEFAULT_MAPSERVER_PORT			= 12124;

// For automatic port discovery when running multiple viewers on one host
const	U32		PORT_DISCOVERY_RANGE_MIN		= 13000;
const	U32		PORT_DISCOVERY_RANGE_MAX		= PORT_DISCOVERY_RANGE_MIN + 50;

const	char	LAND_LAYER_CODE					= 'L';
const	char	WATER_LAYER_CODE				= 'W';
const	char	WIND_LAYER_CODE					= '7';
const	char	CLOUD_LAYER_CODE				= '8';

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
LL_COMMON_API extern const LLUUID REALESTATE_LINDEN_ID;
// Maintenance's group id.
LL_COMMON_API extern const LLUUID MAINTENANCE_GROUP_ID;

// Flags for kick message
const U32 KICK_FLAGS_DEFAULT	= 0x0;
const U32 KICK_FLAGS_FREEZE		= 1 << 0;
const U32 KICK_FLAGS_UNFREEZE	= 1 << 1;

const U8 UPD_NONE      		= 0x00;
const U8 UPD_POSITION  		= 0x01;
const U8 UPD_ROTATION  		= 0x02;
const U8 UPD_SCALE     		= 0x04;
const U8 UPD_LINKED_SETS 	= 0x08;
const U8 UPD_UNIFORM 		= 0x10;	// used with UPD_SCALE

// Agent Update Flags (U8)
const U8 AU_FLAGS_NONE      		= 0x00;
const U8 AU_FLAGS_HIDETITLE      	= 0x01;
const U8 AU_FLAGS_CLIENT_AUTOPILOT	= 0x02;

// start location constants
const U32 START_LOCATION_ID_LAST 		= 0;
const U32 START_LOCATION_ID_HOME 		= 1;
const U32 START_LOCATION_ID_DIRECT	 	= 2;	// for direct teleport
const U32 START_LOCATION_ID_PARCEL	 	= 3;	// for teleports to a parcel
const U32 START_LOCATION_ID_TELEHUB 	= 4;	// for teleports to a spawnpoint
const U32 START_LOCATION_ID_URL			= 5;
const U32 START_LOCATION_ID_COUNT 		= 6;

// group constants
const U32 GROUP_MIN_SIZE = 2;

// gMaxAgentGroups is now sent by login.cgi, which
// looks it up from globals.xml.
//
// For now we need an old default value however,
// so the viewer can be deployed ahead of login.cgi.
//
const S32 DEFAULT_MAX_AGENT_GROUPS = 25;

// radius within which a chat message is fully audible
const F32 CHAT_WHISPER_RADIUS = 10.f;
const F32 CHAT_NORMAL_RADIUS = 20.f;
const F32 CHAT_SHOUT_RADIUS = 100.f;
const F32 CHAT_MAX_RADIUS = CHAT_SHOUT_RADIUS;
const F32 CHAT_MAX_RADIUS_BY_TWO = CHAT_MAX_RADIUS / 2.f;

// squared editions of the above for distance checks
const F32 CHAT_WHISPER_RADIUS_SQUARED = CHAT_WHISPER_RADIUS * CHAT_WHISPER_RADIUS;
const F32 CHAT_NORMAL_RADIUS_SQUARED = CHAT_NORMAL_RADIUS * CHAT_NORMAL_RADIUS;
const F32 CHAT_SHOUT_RADIUS_SQUARED = CHAT_SHOUT_RADIUS * CHAT_SHOUT_RADIUS;
const F32 CHAT_MAX_RADIUS_SQUARED = CHAT_SHOUT_RADIUS_SQUARED;
const F32 CHAT_MAX_RADIUS_BY_TWO_SQUARED = CHAT_MAX_RADIUS_BY_TWO * CHAT_MAX_RADIUS_BY_TWO;


// this times above gives barely audible radius
const F32 CHAT_BARELY_AUDIBLE_FACTOR = 2.0f;

// distance in front of speaking agent the sphere is centered
const F32 CHAT_WHISPER_OFFSET = 5.f;
const F32 CHAT_NORMAL_OFFSET = 10.f;
const F32 CHAT_SHOUT_OFFSET = 50.f;

// first clean starts at 3 AM
const S32 SANDBOX_FIRST_CLEAN_HOUR = 3;
// clean every <n> hours
const S32 SANDBOX_CLEAN_FREQ = 12;

const F32 WIND_SCALE_HACK		= 2.0f;	// hack to make wind speeds more realistic

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

// map item types
const U32 MAP_ITEM_TELEHUB = 0x01;
const U32 MAP_ITEM_PG_EVENT = 0x02;
const U32 MAP_ITEM_MATURE_EVENT = 0x03;
//const U32 MAP_ITEM_POPULAR = 0x04;		// No longer supported, 2009-03-02 KLW
//const U32 MAP_ITEM_AGENT_COUNT = 0x05;
const U32 MAP_ITEM_AGENT_LOCATIONS = 0x06;
const U32 MAP_ITEM_LAND_FOR_SALE = 0x07;
const U32 MAP_ITEM_CLASSIFIED = 0x08;
const U32 MAP_ITEM_ADULT_EVENT = 0x09;
const U32 MAP_ITEM_LAND_FOR_SALE_ADULT = 0x0a;

// Region map layer numbers
const S32 MAP_SIM_OBJECTS = 0;	
const S32 MAP_SIM_TERRAIN = 1;
const S32 MAP_SIM_LAND_FOR_SALE = 2;			// Transparent alpha overlay of land for sale
const S32 MAP_SIM_IMAGE_TYPES = 3;				// Number of map layers
const S32 MAP_SIM_INFO_MASK  		= 0x00FFFFFF;		// Agent access may be stuffed into upper byte
const S32 MAP_SIM_LAYER_MASK 		= 0x0000FFFF;		// Layer info is in lower 16 bits
const S32 MAP_SIM_RETURN_NULL_SIMS 	= 0x00010000;
const S32 MAP_SIM_PRELUDE 			= 0x00020000;

// Crash reporter behavior
const S32 CRASH_BEHAVIOR_ASK = 0;
const S32 CRASH_BEHAVIOR_ALWAYS_SEND = 1;
const S32 CRASH_BEHAVIOR_NEVER_SEND = 2;

// Export/Import return values
const S32 EXPORT_SUCCESS = 0;
const S32 EXPORT_ERROR_PERMISSIONS = -1;
const S32 EXPORT_ERROR_UNKNOWN = -2;

// This is how long the sim will try to teleport you before giving up.
const F32 TELEPORT_EXPIRY = 15.0f;
// Additional time (in seconds) to wait per attachment
const F32 TELEPORT_EXPIRY_PER_ATTACHMENT = 3.f;

// The maximum size of an object extra parameters binary (packed) block
#define MAX_OBJECT_PARAMS_SIZE 1024

const S32 CHAT_CHANNEL_DEBUG = S32_MAX;

// PLEASE don't add constants here.  Every dev will have to do
// a complete rebuild.  Try to find another shared header file,
// like llregionflags.h, lllslconstants.h, llagentconstants.h,
// or create a new one.  JC

#endif
