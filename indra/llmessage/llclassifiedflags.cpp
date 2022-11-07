/** 
 * @file llclassifiedflags.cpp
 * @brief 
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

//*****************************************************************************
// llclassifiedflags.cpp
//
// Some exported symbols and functions for dealing with classified flags.
//
// Copyright 2005, Linden Research, Inc
//*****************************************************************************

#include "linden_common.h"

#include "llclassifiedflags.h"

ClassifiedFlags pack_classified_flags_request(BOOL auto_renew, BOOL inc_pg, BOOL inc_mature, BOOL inc_adult)
{
    U8 rv = 0;
    if(inc_pg) rv |= CLASSIFIED_QUERY_INC_PG;
    if(inc_mature) rv |= CLASSIFIED_QUERY_INC_MATURE;
    if (inc_pg && !inc_mature) rv |= CLASSIFIED_FLAG_MATURE;
    if(inc_adult) rv |= CLASSIFIED_QUERY_INC_ADULT;
    if(auto_renew) rv |= CLASSIFIED_FLAG_AUTO_RENEW;
    return rv;
}

ClassifiedFlags pack_classified_flags(BOOL auto_renew, BOOL inc_pg, BOOL inc_mature, BOOL inc_adult)
{
    U8 rv = 0;
    if(inc_pg) rv |= CLASSIFIED_QUERY_INC_PG;
    if(inc_mature)
    {
        rv |= CLASSIFIED_QUERY_INC_MATURE;
        rv |= CLASSIFIED_FLAG_MATURE;
    }
    if(inc_adult) rv |= CLASSIFIED_QUERY_INC_ADULT;
    if(auto_renew) rv |= CLASSIFIED_FLAG_AUTO_RENEW;
    return rv;
}

bool is_cf_mature(ClassifiedFlags flags)
{
    return ((flags & CLASSIFIED_FLAG_MATURE) != 0) || ((flags & CLASSIFIED_QUERY_INC_MATURE) != 0);
}

// Deprecated, but leaving commented out because someday we might
// want to let users enable/disable classifieds. JC
//bool is_cf_enabled(ClassifiedFlags flags)
//{
//  return ((flags & CLASSIFIED_FLAG_ENABLED) == CLASSIFIED_FLAG_ENABLED);
//}

bool is_cf_update_time(ClassifiedFlags flags)
{
    return ((flags & CLASSIFIED_FLAG_UPDATE_TIME) != 0);
}

bool is_cf_auto_renew(ClassifiedFlags flags)
{
    return ((flags & CLASSIFIED_FLAG_AUTO_RENEW) != 0);
}
