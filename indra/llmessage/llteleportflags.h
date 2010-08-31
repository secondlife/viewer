/** 
 * @file llteleportflags.h
 * @brief Teleport flags
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLTELEPORTFLAGS_H
#define LL_LLTELEPORTFLAGS_H

const U32 TELEPORT_FLAGS_DEFAULT 			= 0;
const U32 TELEPORT_FLAGS_SET_HOME_TO_TARGET	= 1 << 0;	// newbie leaving prelude
const U32 TELEPORT_FLAGS_SET_LAST_TO_TARGET	= 1 << 1;
const U32 TELEPORT_FLAGS_VIA_LURE 			= 1 << 2;
const U32 TELEPORT_FLAGS_VIA_LANDMARK 		= 1 << 3;
const U32 TELEPORT_FLAGS_VIA_LOCATION		= 1 << 4;
const U32 TELEPORT_FLAGS_VIA_HOME			= 1 << 5;
const U32 TELEPORT_FLAGS_VIA_TELEHUB		= 1 << 6;
const U32 TELEPORT_FLAGS_VIA_LOGIN			= 1 << 7;
const U32 TELEPORT_FLAGS_VIA_GODLIKE_LURE	= 1 << 8;
const U32 TELEPORT_FLAGS_GODLIKE 			= 1 << 9;
const U32 TELEPORT_FLAGS_911 				= 1 << 10;
const U32 TELEPORT_FLAGS_DISABLE_CANCEL		= 1 << 11;	// Used for llTeleportAgentHome()
const U32 TELEPORT_FLAGS_VIA_REGION_ID  	= 1 << 12;
const U32 TELEPORT_FLAGS_IS_FLYING			= 1 << 13;
const U32 TELEPORT_FLAGS_SHOW_RESET_HOME	= 1 << 14;
const U32 TELEPORT_FLAGS_FORCE_REDIRECT		= 1 << 15; // used to force a redirect to some random location - used when kicking someone from land.

const U32 TELEPORT_FLAGS_MASK_VIA =   TELEPORT_FLAGS_VIA_LURE 
									| TELEPORT_FLAGS_VIA_LANDMARK
									| TELEPORT_FLAGS_VIA_LOCATION
									| TELEPORT_FLAGS_VIA_HOME
									| TELEPORT_FLAGS_VIA_TELEHUB
									| TELEPORT_FLAGS_VIA_LOGIN
									| TELEPORT_FLAGS_VIA_REGION_ID;
	



const U32 LURE_FLAG_NORMAL_LURE  	= 1 << 0;
const U32 LURE_FLAG_GODLIKE_LURE 	= 1 << 1;
const U32 LURE_FLAG_GODLIKE_PURSUIT = 1 << 2;

#endif
