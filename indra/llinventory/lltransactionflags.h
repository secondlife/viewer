/** 
 * @file lltransactionflags.h
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLTRANSACTIONFLAGS_H
#define LL_LLTRANSACTIONFLAGS_H

class LLUUID;

typedef U8 TransactionFlags;

// defined in common/llinventory/lltransactionflags.cpp
extern const TransactionFlags TRANSACTION_FLAGS_NONE;
extern const TransactionFlags TRANSACTION_FLAG_SOURCE_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_DEST_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_OWNER_GROUP;
extern const TransactionFlags TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION;
extern const TransactionFlags TRANSACTION_FLAG_SIMULTANEOUS_CONTRIBUTION_REMOVAL;

// very simple helper functions
TransactionFlags pack_transaction_flags(bool is_source_group, bool is_dest_group);
bool is_tf_source_group(TransactionFlags flags);
bool is_tf_dest_group(TransactionFlags flags);
bool is_tf_owner_group(TransactionFlags flags);

// stupid helper functions which should be replaced with some kind of
// internationalizeable message.
std::string build_transfer_message_to_source(
	S32 amount,
	const LLUUID& source_id,
	const LLUUID& dest_id,
	const std::string& dest_name,
	S32 transaction_type,
	const std::string& description);

std::string build_transfer_message_to_destination(
	S32 amount,
	const LLUUID& dest_id,
	const LLUUID& source_id,
	const std::string& source_name,
	S32 transaction_type,
	const std::string& description);

#endif // LL_LLTRANSACTIONFLAGS_H
