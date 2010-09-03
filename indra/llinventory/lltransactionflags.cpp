/** 
 * @file lltransactionflags.cpp
 * @brief Some exported symbols and functions for dealing with
 * transaction flags.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "linden_common.h"

#include "lluuid.h"
#include "lltransactionflags.h"
#include "lltransactiontypes.h"
 
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
	const std::string& description)
{
	switch( transaction_type )
	{
	case TRANS_OBJECT_SALE:
	  ostr << " for " << (description.length() > 0 ? description : std::string("<unknown>"));
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
	const std::string& description)
{
	lldebugs << "build_transfer_message_to_source: " << amount << " "
		<< source_id << " " << dest_id << " " << dest_name << " "
		<< transaction_type << " "
		<< (description.empty() ? "(no desc)" : description)
		<< llendl;
	if(source_id.isNull())
	{
		return description;
	}
	if((0 == amount) && description.empty())
	{
		return description;
	}
	std::ostringstream ostr;
	if(dest_id.isNull())
	{
		// *NOTE: Do not change these strings!  The viewer matches
		// them in llviewermessage.cpp to perform localization.
		// If you need to make changes, add a new, localizable message. JC
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
	const std::string& description)
{
	lldebugs << "build_transfer_message_to_dest: " << amount << " "
		<< dest_id << " " << source_id << " " << source_name << " "
		<< transaction_type << " " << (description.empty() ? "(no desc)" : description)
		<< llendl;
	if(0 == amount)
	{
		return std::string();
	}
	if(dest_id.isNull())
	{
		return description;
	}
	std::ostringstream ostr;
	// *NOTE: Do not change these strings!  The viewer matches
	// them in llviewermessage.cpp to perform localization.
	// If you need to make changes, add a new, localizable message. JC
	ostr << source_name << " paid you L$" << amount;
	append_reason(ostr, transaction_type, description);
	ostr << ".";
	return ostr.str();
}
