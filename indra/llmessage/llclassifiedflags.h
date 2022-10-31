/** 
 * @file llclassifiedflags.h
 * @brief Flags used in the classifieds.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLCLASSIFIEDFLAGS_H
#define LL_LLCLASSIFIEDFLAGS_H

typedef U8 ClassifiedFlags;

const U8 CLASSIFIED_FLAG_NONE       = 1 << 0;
const U8 CLASSIFIED_FLAG_MATURE     = 1 << 1;
//const U8 CLASSIFIED_FLAG_ENABLED  = 1 << 2; // see llclassifiedflags.cpp
//const U8 CLASSIFIED_FLAG_HAS_PRICE= 1 << 3; // deprecated
const U8 CLASSIFIED_FLAG_UPDATE_TIME= 1 << 4;
const U8 CLASSIFIED_FLAG_AUTO_RENEW = 1 << 5;

const U8 CLASSIFIED_QUERY_FILTER_MATURE     = 1 << 1;
//const U8 CLASSIFIED_QUERY_FILTER_ENABLED  = 1 << 2;
//const U8 CLASSIFIED_QUERY_FILTER_PRICE    = 1 << 3;

// These are new with Adult-enabled viewers (1.23 and later)
const U8 CLASSIFIED_QUERY_INC_PG            = 1 << 2;
const U8 CLASSIFIED_QUERY_INC_MATURE        = 1 << 3;
const U8 CLASSIFIED_QUERY_INC_ADULT         = 1 << 6;
const U8 CLASSIFIED_QUERY_INC_NEW_VIEWER    = (CLASSIFIED_QUERY_INC_PG | CLASSIFIED_QUERY_INC_MATURE | CLASSIFIED_QUERY_INC_ADULT);

const S32 MAX_CLASSIFIEDS = 100;

// This function is used in AO viewers to pack old query flags into the request 
// so that they can talk to old dataservers properly. When the AO servers are deployed on agni
// we can revert back to ClassifiedFlags pack_classified_flags and get rider of this one.
ClassifiedFlags pack_classified_flags_request(BOOL auto_renew, BOOL is_pg, BOOL is_mature, BOOL is_adult);

ClassifiedFlags pack_classified_flags(BOOL auto_renew, BOOL is_pg, BOOL is_mature, BOOL is_adult);
bool is_cf_mature(ClassifiedFlags flags);
//bool is_cf_enabled(ClassifiedFlags flags);
bool is_cf_update_time(ClassifiedFlags flags);
bool is_cf_auto_renew(ClassifiedFlags flags);

#endif
