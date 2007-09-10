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
#include "lltransactiontypes.h"
#include "lluuid.h"
 
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

void append_reason(
	std::ostream& ostr,
	S32 transaction_type,
	const char* description)
{
	switch( transaction_type )
	{
	case TRANS_OBJECT_SALE:
		ostr << " for " << (description ? description : "<unknown>");
		break;
	case TRANS_LAND_SALE:
		ostr << " for a parcel of land";
		break;
	case TRANS_LAND_PASS_SALE:
		ostr << " for a land access pass";
		break;
	case TRANS_GROUP_LAND_DEED:
		ostr << " for deeding land";
	default:
		break;
	}
}

std::string build_transfer_message_to_source(
	S32 amount,
	const LLUUID& source_id,
	const LLUUID& dest_id,
	const std::string& dest_name,
	S32 transaction_type,
	const char* description)
{
	lldebugs << "build_transfer_message_to_source: " << amount << " "
		<< source_id << " " << dest_id << " " << dest_name << " "
		<< (description?description:"(no desc)") << llendl;
	if((0 == amount) || source_id.isNull()) return ll_safe_string(description);
	std::ostringstream ostr;
	if(dest_id.isNull())
	{
		ostr << "You paid L$" << amount;
		switch(transaction_type)
		{
		case TRANS_GROUP_CREATE:
			ostr << " to create a group";
			break;
		case TRANS_GROUP_JOIN:
			ostr << " to join a group";
			break;
		case TRANS_UPLOAD_CHARGE:
			ostr << " to upload";
			break;
		default:
			break;
		}
	}
	else
	{
		ostr << "You paid " << dest_name << " L$" << amount;
		append_reason(ostr, transaction_type, description);
	}
	ostr << ".";
	return ostr.str();
}

std::string build_transfer_message_to_destination(
	S32 amount,
	const LLUUID& dest_id,
	const LLUUID& source_id,
	const std::string& source_name,
	S32 transaction_type,
	const char* description)
{
	lldebugs << "build_transfer_message_to_dest: " << amount << " "
		<< dest_id << " " << source_id << " " << source_name << " "
		<< (description?description:"(no desc)") << llendl;
	if(0 == amount) return std::string();
	if(dest_id.isNull()) return ll_safe_string(description);
	std::ostringstream ostr;
	ostr << source_name << " paid you L$" << amount;
	append_reason(ostr, transaction_type, description);
	ostr << ".";
	return ostr.str();
}
