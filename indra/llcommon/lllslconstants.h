/** 
 * @file lllslconstants.h
 * @author James Cook
 * @brief Constants used in lsl.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLLSLCONSTANTS_H
#define LL_LLLSLCONSTANTS_H

// LSL: Return flags for llGetAgentInfo
const U32 AGENT_FLYING		= 0x0001;
const U32 AGENT_ATTACHMENTS	= 0x0002;
const U32 AGENT_SCRIPTED	= 0x0004;
const U32 AGENT_MOUSELOOK	= 0x0008;
const U32 AGENT_SITTING		= 0x0010;
const U32 AGENT_ON_OBJECT	= 0x0020;
const U32 AGENT_AWAY		= 0x0040;
const U32 AGENT_WALKING		= 0x0080;
const U32 AGENT_IN_AIR		= 0x0100;
const U32 AGENT_TYPING		= 0x0200;
const U32 AGENT_CROUCHING	= 0x0400;
const U32 AGENT_BUSY		= 0x0800;
const U32 AGENT_ALWAYS_RUN	= 0x1000;
const U32 AGENT_AUTOPILOT	= 0x2000;

const S32 LSL_REMOTE_DATA_CHANNEL		= 1;
const S32 LSL_REMOTE_DATA_REQUEST		= 2;
const S32 LSL_REMOTE_DATA_REPLY			= 3;

// Constants used in extended LSL primitive setter and getters
const S32 LSL_PRIM_TYPE_LEGACY	= 1; // No longer supported.
const S32 LSL_PRIM_MATERIAL		= 2;
const S32 LSL_PRIM_PHYSICS		= 3;
const S32 LSL_PRIM_TEMP_ON_REZ	= 4;
const S32 LSL_PRIM_PHANTOM		= 5;
const S32 LSL_PRIM_POSITION		= 6;
const S32 LSL_PRIM_SIZE			= 7;
const S32 LSL_PRIM_ROTATION		= 8;
const S32 LSL_PRIM_TYPE			= 9; // Replacement for LSL_PRIM_TYPE_LEGACY
const S32 LSL_PRIM_TEXTURE		= 17;
const S32 LSL_PRIM_COLOR		= 18;
const S32 LSL_PRIM_BUMP_SHINY	= 19;
const S32 LSL_PRIM_FULLBRIGHT	= 20;
const S32 LSL_PRIM_FLEXIBLE		= 21;
const S32 LSL_PRIM_TEXGEN		= 22;
const S32 LSL_PRIM_POINT_LIGHT	= 23;
const S32 LSL_PRIM_CAST_SHADOWS	= 24;
const S32 LSL_PRIM_GLOW     	= 25;

const S32 LSL_PRIM_TYPE_BOX		= 0;
const S32 LSL_PRIM_TYPE_CYLINDER= 1;
const S32 LSL_PRIM_TYPE_PRISM	= 2;
const S32 LSL_PRIM_TYPE_SPHERE	= 3;
const S32 LSL_PRIM_TYPE_TORUS	= 4;
const S32 LSL_PRIM_TYPE_TUBE	= 5;
const S32 LSL_PRIM_TYPE_RING	= 6;
const S32 LSL_PRIM_TYPE_SCULPT  = 7;

const S32 LSL_PRIM_HOLE_DEFAULT = 0x00;
const S32 LSL_PRIM_HOLE_CIRCLE	= 0x10;
const S32 LSL_PRIM_HOLE_SQUARE  = 0x20;
const S32 LSL_PRIM_HOLE_TRIANGLE= 0x30;

const S32 LSL_PRIM_MATERIAL_STONE	= 0;
const S32 LSL_PRIM_MATERIAL_METAL	= 1;
const S32 LSL_PRIM_MATERIAL_GLASS	= 2;
const S32 LSL_PRIM_MATERIAL_WOOD	= 3;
const S32 LSL_PRIM_MATERIAL_FLESH	= 4;
const S32 LSL_PRIM_MATERIAL_PLASTIC	= 5;
const S32 LSL_PRIM_MATERIAL_RUBBER	= 6;
const S32 LSL_PRIM_MATERIAL_LIGHT	= 7;

const S32 LSL_PRIM_SHINY_NONE		= 0;
const S32 LSL_PRIM_SHINY_LOW		= 1;
const S32 LSL_PRIM_SHINY_MEDIUM		= 2;
const S32 LSL_PRIM_SHINY_HIGH		= 3;

const S32 LSL_PRIM_TEXGEN_DEFAULT	= 0;
const S32 LSL_PRIM_TEXGEN_PLANAR	= 1;

const S32 LSL_PRIM_BUMP_NONE		= 0;
const S32 LSL_PRIM_BUMP_BRIGHT		= 1;
const S32 LSL_PRIM_BUMP_DARK		= 2;
const S32 LSL_PRIM_BUMP_WOOD		= 3;
const S32 LSL_PRIM_BUMP_BARK		= 4;
const S32 LSL_PRIM_BUMP_BRICKS		= 5;
const S32 LSL_PRIM_BUMP_CHECKER		= 6;
const S32 LSL_PRIM_BUMP_CONCRETE	= 7;
const S32 LSL_PRIM_BUMP_TILE		= 8;
const S32 LSL_PRIM_BUMP_STONE		= 9;
const S32 LSL_PRIM_BUMP_DISKS		= 10;
const S32 LSL_PRIM_BUMP_GRAVEL		= 11;
const S32 LSL_PRIM_BUMP_BLOBS		= 12;
const S32 LSL_PRIM_BUMP_SIDING		= 13;
const S32 LSL_PRIM_BUMP_LARGETILE	= 14;
const S32 LSL_PRIM_BUMP_STUCCO		= 15;
const S32 LSL_PRIM_BUMP_SUCTION		= 16;
const S32 LSL_PRIM_BUMP_WEAVE		= 17;

const S32 LSL_PRIM_SCULPT_TYPE_SPHERE   = 1;
const S32 LSL_PRIM_SCULPT_TYPE_TORUS    = 2;
const S32 LSL_PRIM_SCULPT_TYPE_PLANE    = 3;
const S32 LSL_PRIM_SCULPT_TYPE_CYLINDER = 4;
const S32 LSL_PRIM_SCULPT_TYPE_MASK     = 7;
const S32 LSL_PRIM_SCULPT_FLAG_INVERT   = 64;
const S32 LSL_PRIM_SCULPT_FLAG_MIRROR   = 128;

const S32 LSL_ALL_SIDES				= -1;
const S32 LSL_LINK_ROOT				= 1;
const S32 LSL_LINK_FIRST_CHILD		= 2;
const S32 LSL_LINK_SET				= -1;
const S32 LSL_LINK_ALL_OTHERS		= -2;
const S32 LSL_LINK_ALL_CHILDREN		= -3;
const S32 LSL_LINK_THIS				= -4;

// LSL constants for llSetForSell
const S32 SELL_NOT			= 0;
const S32 SELL_ORIGINAL		= 1;
const S32 SELL_COPY			= 2;
const S32 SELL_CONTENTS		= 3;

// LSL constants for llSetPayPrice
const S32 PAY_PRICE_HIDE = -1;
const S32 PAY_PRICE_DEFAULT = -2;
const S32 MAX_PAY_BUTTONS = 4;
const S32 PAY_BUTTON_DEFAULT_0 = 1;
const S32 PAY_BUTTON_DEFAULT_1 = 5;
const S32 PAY_BUTTON_DEFAULT_2 = 10;
const S32 PAY_BUTTON_DEFAULT_3 = 20;

// lsl email registration.
const S32 EMAIL_REG_SUBSCRIBE_OBJECT	= 0x01;
const S32 EMAIL_REG_UNSUBSCRIBE_OBJECT	= 0x02;
const S32 EMAIL_REG_UNSUBSCRIBE_SIM		= 0x04;

const S32 LIST_STAT_RANGE = 0;
const S32 LIST_STAT_MIN		= 1;
const S32 LIST_STAT_MAX		= 2;
const S32 LIST_STAT_MEAN	= 3;
const S32 LIST_STAT_MEDIAN	= 4;
const S32 LIST_STAT_STD_DEV	= 5;
const S32 LIST_STAT_SUM = 6;
const S32 LIST_STAT_SUM_SQUARES = 7;
const S32 LIST_STAT_NUM_COUNT = 8;
const S32 LIST_STAT_GEO_MEAN = 9;

const S32 STRING_TRIM_HEAD = 0x01;
const S32 STRING_TRIM_TAIL = 0x02;
const S32 STRING_TRIM = STRING_TRIM_HEAD | STRING_TRIM_TAIL;

// llGetObjectDetails
const S32 OBJECT_UNKNOWN_DETAIL = -1;
const S32 OBJECT_NAME = 1;
const S32 OBJECT_DESC = 2;
const S32 OBJECT_POS = 3;
const S32 OBJECT_ROT = 4;
const S32 OBJECT_VELOCITY = 5;
const S32 OBJECT_OWNER = 6;
const S32 OBJECT_GROUP = 7;
const S32 OBJECT_CREATOR = 8;

// llTextBox() magic token string - yes this is a hack.  sue me.
char const* const TEXTBOX_MAGIC_TOKEN = "!!llTextBox!!";

// changed() event flags
const U32	CHANGED_NONE = 0x0;
const U32	CHANGED_INVENTORY = 0x1;
const U32	CHANGED_COLOR = 0x2;
const U32	CHANGED_SHAPE = 0x4;
const U32	CHANGED_SCALE = 0x8;
const U32	CHANGED_TEXTURE = 0x10;
const U32	CHANGED_LINK = 0x20;
const U32	CHANGED_ALLOWED_DROP = 0x40;
const U32	CHANGED_OWNER = 0x80;
const U32	CHANGED_REGION = 0x100;
const U32	CHANGED_TELEPORT = 0x200;
const U32	CHANGED_REGION_START = 0x400;
const U32   CHANGED_MEDIA = 0x800;

// Possible error results
const U32 LSL_STATUS_OK                 = 0;
const U32 LSL_STATUS_MALFORMED_PARAMS   = 1000;
const U32 LSL_STATUS_TYPE_MISMATCH      = 1001;
const U32 LSL_STATUS_BOUNDS_ERROR       = 1002;
const U32 LSL_STATUS_NOT_FOUND          = 1003;
const U32 LSL_STATUS_NOT_SUPPORTED      = 1004;
const U32 LSL_STATUS_INTERNAL_ERROR     = 1999;

// Start per-function errors below, starting at 2000:
const U32 LSL_STATUS_WHITELIST_FAILED   = 2001;

#endif
