/** 
 * @file lltransactionflags.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSACTIONFLAGS_H
#define LL_LLTRANSACTIONFLAGS_H

typedef U8 TransactionFlags;

// defined in common/llinventory/lltransactionflags.cpp
extern const TransactionFlags TRANSACTION_FLAGS_NONE;
extern const TransactionFlags TRANSACTION_FLAG_SOURCE_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_DEST_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_OWNER_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION;
extern const TransactionFlags TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION_REMOVAL;

// very simple helper functions
TransactionFlags pack_transaction_flags(BOOL is_source_group, BOOL is_dest_group);
BOOL is_tf_source_group(TransactionFlags flags);
BOOL is_tf_dest_group(TransactionFlags flags);
BOOL is_tf_owner_group(TransactionFlags flags);

#endif // LL_LLTRANSACTIONFLAGS_H
