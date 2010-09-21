/** 
 * @file message_string_table.cpp
 * @brief static string table for message template
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llerror.h"
#include "message.h"

inline U32	message_hash_my_string(const char *str)
{
	U32 retval = 0;
	while (*str++)
	{
		retval += *str;
		retval <<= 1;
	}
	return (retval % MESSAGE_NUMBER_OF_HASH_BUCKETS);
}



LLMessageStringTable::LLMessageStringTable()
:	mUsed(0)
{
	for (U32 i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
	{
		mEmpty[i] = TRUE;
		mString[i][0] = 0;
	}
}


LLMessageStringTable::~LLMessageStringTable()
{ }


char* LLMessageStringTable::getString(const char *str)
{
	U32 hash_value = message_hash_my_string(str);
	while (!mEmpty[hash_value])
	{
		if (!strncmp(str, mString[hash_value], MESSAGE_MAX_STRINGS_LENGTH))
		{
			return mString[hash_value];
		}
		else
		{
			hash_value++;
			hash_value %= MESSAGE_NUMBER_OF_HASH_BUCKETS;
		}
	}
	// not found, so add!
	strncpy(mString[hash_value], str, MESSAGE_MAX_STRINGS_LENGTH);	/* Flawfinder: ignore */
	mString[hash_value][MESSAGE_MAX_STRINGS_LENGTH - 1] = 0;
	mEmpty[hash_value] = FALSE;
	mUsed++;
	if (mUsed >= MESSAGE_NUMBER_OF_HASH_BUCKETS - 1)
	{
		U32 i;
		llinfos << "Dumping string table before crashing on HashTable full!" << llendl;
		for (i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
		{
			llinfos << "Entry #" << i << ": " << mString[i] << llendl;
		}
	}
	return mString[hash_value];
}

