/** 
 * @file llqueryflags.h
 * @brief Flags for directory queries
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLQUERYFLAGS_H
#define LL_LLQUERYFLAGS_H

// Binary flags used for Find queries, shared between viewer and dataserver.

// DirFindQuery flags
const U32 DFQ_PEOPLE			= 0x1 << 0;
const U32 DFQ_ONLINE			= 0x1 << 1;
//const U32 DFQ_PLACES			= 0x1 << 2;
const U32 DFQ_EVENTS			= 0x1 << 3;
const U32 DFQ_GROUPS			= 0x1 << 4;
const U32 DFQ_DATE_EVENTS		= 0x1 << 5;

const U32 DFQ_AGENT_OWNED		= 0x1 << 6;
const U32 DFQ_FOR_SALE			= 0x1 << 7;
const U32 DFQ_GROUP_OWNED		= 0x1 << 8;
//const U32 DFQ_AUCTION			= 0x1 << 9;
const U32 DFQ_DWELL_SORT		= 0x1 << 10;
const U32 DFQ_PG_SIMS_ONLY		= 0x1 << 11;
const U32 DFQ_PICTURES_ONLY		= 0x1 << 12;
const U32 DFQ_PG_EVENTS_ONLY	= 0x1 << 13;
const U32 DFQ_MATURE_SIMS_ONLY  = 0x1 << 14;

const U32 DFQ_SORT_ASC			= 0x1 << 15;
const U32 DFQ_PRICE_SORT		= 0x1 << 16;
const U32 DFQ_PER_METER_SORT	= 0x1 << 17;
const U32 DFQ_AREA_SORT			= 0x1 << 18;
const U32 DFQ_NAME_SORT			= 0x1 << 19;

const U32 DFQ_LIMIT_BY_PRICE	= 0x1 << 20;
const U32 DFQ_LIMIT_BY_AREA		= 0x1 << 21;

// Sell Type flags
const U32 ST_AUCTION	= 0x1 << 1;
const U32 ST_NEWBIE		= 0x1 << 2;
const U32 ST_MAINLAND	= 0x1 << 3;
const U32 ST_ESTATE		= 0x1 << 4;

const U32 ST_ALL		= 0xFFFFFFFF;

#endif
