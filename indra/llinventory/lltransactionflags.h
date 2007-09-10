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

// stupid helper functions which should be replaced with some kind of
// internationalizeable message.
std::string build_transfer_message_to_source(
	S32 amount,
	const LLUUID& source_id,
	const LLUUID& dest_id,
	const std::string& dest_name,
	S32 transaction_type,
	const char* description);

std::string build_transfer_message_to_destination(
	S32 amount,
	const LLUUID& dest_id,
	const LLUUID& source_id,
	const std::string& source_name,
	S32 transaction_type,
	const char* description);

#endif // LL_LLTRANSACTIONFLAGS_H
