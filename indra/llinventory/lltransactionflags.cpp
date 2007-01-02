/** 
 * @file lltransactionflags.cpp
 * @brief Some exported symbols and functions for dealing with
 * transaction flags.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lltransactionflags.h"
 
const U8 TRANSACTION_FLAGS_NONE = 0;
const U8 TRANSACTION_FLAG_SOURCE_GROUP = 1;
const U8 TRANSACTION_FLAG_DEST_GROUP = 2;
const U8 TRANSACTION_FLAG_OWNER_GROUP = 4;
const U8 TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION = 8;
const U8 TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION_REMOVAL = 16;

U8 pack_transaction_flags(BOOL is_source_group, BOOL is_dest_group)
{
	U8 rv = 0;
	if(is_source_group) rv |= TRANSACTION_FLAG_SOURCE_GROUP;
	if(is_dest_group) rv |= TRANSACTION_FLAG_DEST_GROUP;
	return rv;
}

BOOL is_tf_source_group(TransactionFlags flags)
{
	return ((flags & TRANSACTION_FLAG_SOURCE_GROUP) == TRANSACTION_FLAG_SOURCE_GROUP);
}

BOOL is_tf_dest_group(TransactionFlags flags)
{
	return ((flags & TRANSACTION_FLAG_DEST_GROUP) == TRANSACTION_FLAG_DEST_GROUP);
}

BOOL is_tf_owner_group(TransactionFlags flags)
{
	return ((flags & TRANSACTION_FLAG_OWNER_GROUP) == TRANSACTION_FLAG_OWNER_GROUP);
}

