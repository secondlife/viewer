/** 
 * @file llclassifiedflags.cpp
 * @brief 
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
 
ClassifiedFlags pack_classified_flags(BOOL is_mature, BOOL auto_renew)
{
	U8 rv = 0;
	if(is_mature) rv |= CLASSIFIED_FLAG_MATURE;
	if(auto_renew) rv |= CLASSIFIED_FLAG_AUTO_RENEW;
	return rv;
}

bool is_cf_mature(ClassifiedFlags flags)
{
	return ((flags & CLASSIFIED_FLAG_MATURE) != 0);
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
