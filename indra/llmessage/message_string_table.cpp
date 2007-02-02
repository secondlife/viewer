/** 
 * @file message_string_table.cpp
 * @brief static string table for message template
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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


LLMessageStringTable gMessageStringTable;


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

