/** 
 * @file llclassifiedflags.h
 * @brief Flags used in the classifieds.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCLASSIFIEDFLAGS_H
#define LL_LLCLASSIFIEDFLAGS_H

typedef U8 ClassifiedFlags;

const U8 CLASSIFIED_FLAG_NONE   	= 1 << 0;
const U8 CLASSIFIED_FLAG_MATURE 	= 1 << 1;
//const U8 CLASSIFIED_FLAG_ENABLED	= 1 << 2; // see llclassifiedflags.cpp
//const U8 CLASSIFIED_FLAG_HAS_PRICE= 1 << 3; // deprecated
const U8 CLASSIFIED_FLAG_UPDATE_TIME= 1 << 4;
const U8 CLASSIFIED_FLAG_AUTO_RENEW = 1 << 5;

const U8 CLASSIFIED_QUERY_FILTER_MATURE		= 1 << 1;
const U8 CLASSIFIED_QUERY_FILTER_ENABLED	= 1 << 2;
const U8 CLASSIFIED_QUERY_FILTER_PRICE		= 1 << 3;

const S32 MAX_CLASSIFIEDS = 100;

ClassifiedFlags pack_classified_flags(BOOL is_mature, BOOL auto_renew);
bool is_cf_mature(ClassifiedFlags flags);
//bool is_cf_enabled(ClassifiedFlags flags);
bool is_cf_update_time(ClassifiedFlags flags);
bool is_cf_auto_renew(ClassifiedFlags flags);

#endif
