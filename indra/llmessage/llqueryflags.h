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
const U32 DFQ_PEOPLE			= 0x0001;
const U32 DFQ_ONLINE			= 0x0002;
//const U32 DFQ_PLACES			= 0x0004;
const U32 DFQ_EVENTS			= 0x0008;
const U32 DFQ_GROUPS			= 0x0010;
const U32 DFQ_DATE_EVENTS		= 0x0020;

const U32 DFQ_AGENT_OWNED		= 0x0040;
const U32 DFQ_FOR_SALE			= 0x0080;
const U32 DFQ_GROUP_OWNED		= 0x0100;
//const U32 DFQ_AUCTION			= 0x0200;
const U32 DFQ_DWELL_SORT		= 0x0400;
const U32 DFQ_PG_SIMS_ONLY		= 0x0800;
const U32 DFQ_PICTURES_ONLY		= 0x1000;
const U32 DFQ_PG_EVENTS_ONLY	= 0x2000;
const U32 DFQ_MATURE_SIMS_ONLY  = 0x4000;

#endif
