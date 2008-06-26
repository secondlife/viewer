/** 
 * @file lltransactionflags.h
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
	const std::string& description);

std::string build_transfer_message_to_destination(
	S32 amount,
	const LLUUID& dest_id,
	const LLUUID& source_id,
	const std::string& source_name,
	S32 transaction_type,
	const std::string& description);

#endif // LL_LLTRANSACTIONFLAGS_H
