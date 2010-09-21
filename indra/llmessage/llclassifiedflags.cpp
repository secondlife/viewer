/** 
 * @file llclassifiedflags.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
//	return ((flags & CLASSIFIED_FLAG_ENABLED) == CLASSIFIED_FLAG_ENABLED);
//}

bool is_cf_update_time(ClassifiedFlags flags)
{
	return ((flags & CLASSIFIED_FLAG_UPDATE_TIME) != 0);
}

bool is_cf_auto_renew(ClassifiedFlags flags)
{
	return ((flags & CLASSIFIED_FLAG_AUTO_RENEW) != 0);
}
