/** 
 * @file indra_constants.h
 * @brief some useful short term constants for Indra
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_INDRA_CONSTANTS_H
#define LL_INDRA_CONSTANTS_H

#include "stdtypes.h"
#include "lluuid.h"

// At 45 Hz collisions seem stable and objects seem
// to settle down at a reasonable rate.
// JC 3/18/2003
const F32 HAVOK_TIMESTEP = 1.f / 45.f;

const F32 COLLISION_TOLERANCE = 0.1f;

// Time constants
const U32 HOURS_PER_LINDEN_DAY		= 4;	
const U32 DAYS_PER_LINDEN_YEAR		= 11;

const U32 SEC_PER_LINDEN_DAY		= HOURS_PER_LINDEN_DAY * 60 * 60;
const U32 SEC_PER_LINDEN_YEAR		= DAYS_PER_LINDEN_YEAR * SEC_PER_LINDEN_DAY;

const F32 REGION_WIDTH_METERS = 256.f;
const S32 REGION_WIDTH_UNITS = 256;
const U32 REGION_WIDTH_U32 = 256;

// Bits for simulator performance query flags
enum LAND_STAT_FLAGS
{
	STAT_FILTER_BY_PARCEL	= 0x00000001,
	STAT_FILTER_BY_OWNER	= 0x00000002,
	STAT_FILTER_BY_OBJECT	= 0x00000004,
	STAT_REQUEST_LAST_ENTRY	= 0x80000000,
};

enum LAND_STAT_REPORT_TYPE
{
	STAT_REPORT_TOP_SCRIPTS = 0,
	STAT_REPORT_TOP_COLLIDERS
};

const U32 STAT_FILTER_MASK	= 0x1FFFFFFF;

// Default maximum number of tasks/prims per region.
const U32 MAX_TASKS_PER_REGION = 15000;

const 	F32 	MIN_AGENT_DEPTH			= 0.30f;
const 	F32 	DEFAULT_AGENT_DEPTH 	= 0.45f;
const 	F32 	MAX_AGENT_DEPTH			= 0.60f;

const 	F32 	MIN_AGENT_WIDTH 		= 0.40f;
const 	F32 	DEFAULT_AGENT_WIDTH 	= 0.60f;
const 	F32 	MAX_AGENT_WIDTH 		= 0.80f;

const 	F32 	MIN_AGENT_HEIGHT		= 1.3f - 2.0f * COLLISION_TOLERANCE;
const 	F32 	DEFAULT_AGENT_HEIGHT	= 1.9f;
const 	F32 	MAX_AGENT_HEIGHT		= 2.65f - 2.0f * COLLISION_TOLERANCE;

// For linked sets
const S32 MAX_CHILDREN_PER_TASK = 255;
const S32 MAX_CHILDREN_PER_PHYSICAL_TASK = 31;

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
const U8 SIM_ACCESS_MIN 	= 0;
const U8 SIM_ACCESS_TRIAL	= 7;
const U8 SIM_ACCESS_PG		= 13;
const U8 SIM_ACCESS_MATURE	= 21;
const U8 SIM_ACCESS_DOWN	= 254;
const U8 SIM_ACCESS_MAX 	= SIM_ACCESS_MATURE;

// group constants
const S32 MAX_AGENT_GROUPS = 25;

// god levels
const U8 GOD_MAINTENANCE = 250;
const U8 GOD_FULL = 200;
const U8 GOD_LIAISON = 150;
const U8 GOD_CUSTOMER_SERVICE = 100;
const U8 GOD_LIKE = 1;
const U8 GOD_NOT = 0;

// "agent id" for things that should be done to ALL agents
const LLUUID LL_UUID_ALL_AGENTS("44e87126-e794-4ded-05b3-7c42da3d5cdb");

// Governor Linden's agent id.
const LLUUID GOVERNOR_LINDEN_ID("3d6181b0-6a4b-97ef-18d8-722652995cf1");
const LLUUID REALESTATE_LINDEN_ID("3d6181b0-6a4b-97ef-18d8-722652995cf1");
// Maintenance's group id.
const LLUUID MAINTENANCE_GROUP_ID("dc7b21cd-3c89-fcaa-31c8-25f9ffd224cd");

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

// radius within which a chat message is fully audible
const F32 CHAT_WHISPER_RADIUS = 10.f;
const F32 CHAT_NORMAL_RADIUS = 20.f;
const F32 CHAT_SHOUT_RADIUS = 100.f;
const F32 CHAT_MAX_RADIUS = CHAT_SHOUT_RADIUS;
const F32 CHAT_MAX_RADIUS_BY_TWO = CHAT_MAX_RADIUS / 2.f;

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

// map item types
const U32 MAP_ITEM_TELEHUB = 0x01;
const U32 MAP_ITEM_PG_EVENT = 0x02;
const U32 MAP_ITEM_MATURE_EVENT = 0x03;
const U32 MAP_ITEM_POPULAR = 0x04;
//const U32 MAP_ITEM_AGENT_COUNT = 0x05;
const U32 MAP_ITEM_AGENT_LOCATIONS = 0x06;
const U32 MAP_ITEM_LAND_FOR_SALE = 0x07;
const U32 MAP_ITEM_CLASSIFIED = 0x08;

// Crash reporter behavior
const char* const CRASH_SETTINGS_FILE = "crash_settings.xml";
const char* const CRASH_BEHAVIOR_SETTING = "CrashBehavior";
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
