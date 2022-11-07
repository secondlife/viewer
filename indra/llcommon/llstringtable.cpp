/** 
 * @file llstringtable.cpp
 * @brief The LLStringTable class provides a _fast_ method for finding
 * unique copies of strings.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llstringtable.h"
#include "llstl.h"

LLStringTable gStringTable(32768);

LLStringTableEntry::LLStringTableEntry(const char *str)
: mString(NULL), mCount(1)
{
    // Copy string
    U32 length = (U32)strlen(str) + 1;   /*Flawfinder: ignore*/
    length = llmin(length, MAX_STRINGS_LENGTH);
    mString = new char[length];
    strncpy(mString, str, length);   /*Flawfinder: ignore*/
    mString[length - 1] = 0;
}

LLStringTableEntry::~LLStringTableEntry()
{
    delete [] mString;
    mCount = 0;
}

LLStringTable::LLStringTable(int tablesize)
: mUniqueEntries(0)
{
    S32 i;
    if (!tablesize)
        tablesize = 4096; // some arbitrary default
    // Make sure tablesize is power of 2
    for (i = 31; i>0; i--)
    {
        if (tablesize & (1<<i))
        {
            if (tablesize >= (3<<(i-1)))
                tablesize = (1<<(i+1));
            else
                tablesize = (1<<i);
            break;
        }
    }
    mMaxEntries = tablesize;

#if !STRING_TABLE_HASH_MAP
    // ALlocate strings
    mStringList = new string_list_ptr_t[mMaxEntries];
    // Clear strings
    for (i = 0; i < mMaxEntries; i++)
    {
        mStringList[i] = NULL;
    }
#endif
}

LLStringTable::~LLStringTable()
{
#if !STRING_TABLE_HASH_MAP
    if (mStringList)
    {
        for (S32 i = 0; i < mMaxEntries; i++)
        {
            if (mStringList[i])
            {
                string_list_t::iterator iter;
                for (iter = mStringList[i]->begin(); iter != mStringList[i]->end(); iter++)
                    delete *iter; // *iter = (LLStringTableEntry*)
            }
            delete mStringList[i];
        }
        delete [] mStringList;
        mStringList = NULL;
    }
#else
    // Need to clean up the string hash
    for_each(mStringHash.begin(), mStringHash.end(), DeletePairedPointer());
    mStringHash.clear();
#endif
}


static U32 hash_my_string(const char *str, int max_entries)
{
    U32 retval = 0;
#if 0
    while (*str)
    {
        retval <<= 1;
        retval += *str++;
    }
#else
    while (*str)
    {
        retval = (retval<<4) + *str;
        U32 x = (retval & 0xf0000000);
        if (x) retval = retval ^ (x>>24);
        retval = retval & (~x);
        str++;
    }
#endif
    return (retval & (max_entries-1)); // max_entries is gauranteed to be power of 2
}

char* LLStringTable::checkString(const std::string& str)
{
    return checkString(str.c_str());
}

char* LLStringTable::checkString(const char *str)
{
    LLStringTableEntry* entry = checkStringEntry(str);
    if (entry)
    {
    return entry->mString;
    }
    else
    {
    return NULL;
    }
}

LLStringTableEntry* LLStringTable::checkStringEntry(const std::string& str)
{
    return checkStringEntry(str.c_str());
}

LLStringTableEntry* LLStringTable::checkStringEntry(const char *str)
{
    if (str)
    {
        char *ret_val;
        LLStringTableEntry  *entry;
        U32                 hash_value = hash_my_string(str, mMaxEntries);
#if STRING_TABLE_HASH_MAP
#if 1 // Microsoft
        string_hash_t::iterator lower = mStringHash.lower_bound(hash_value);
        string_hash_t::iterator upper = mStringHash.upper_bound(hash_value);
#else // stlport
        std::pair<string_hash_t::iterator, string_hash_t::iterator> P = mStringHash.equal_range(hash_value);
        string_hash_t::iterator lower = P.first;
        string_hash_t::iterator upper = P.second;
#endif
        for (string_hash_t::iterator iter = lower; iter != upper; iter++)
        {
            entry = iter->second;
            ret_val = entry->mString;
            if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
            {
                return entry;
            }
        }
#else
        string_list_t       *strlist = mStringList[hash_value];
        if (strlist)
        {
            string_list_t::iterator iter;
            for (iter = strlist->begin(); iter != strlist->end(); iter++)
            {
                entry = *iter;
                ret_val = entry->mString;
                if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
                {
                    return entry;
                }
            }
        }
#endif
    }
    return NULL;
}

char* LLStringTable::addString(const std::string& str)
{
    //RN: safe to use temporary c_str since string is copied
    return addString(str.c_str());
}

char* LLStringTable::addString(const char *str)
{

    LLStringTableEntry* entry = addStringEntry(str);
    if (entry)
    {
    return entry->mString;
    }
    else
    {
    return NULL;
    }
}

LLStringTableEntry* LLStringTable::addStringEntry(const std::string& str)
{
    return addStringEntry(str.c_str());
}

LLStringTableEntry* LLStringTable::addStringEntry(const char *str)
{
    if (str)
    {
        char *ret_val = NULL;
        LLStringTableEntry  *entry;
        U32                 hash_value = hash_my_string(str, mMaxEntries);
#if STRING_TABLE_HASH_MAP
#if 1 // Microsoft
        string_hash_t::iterator lower = mStringHash.lower_bound(hash_value);
        string_hash_t::iterator upper = mStringHash.upper_bound(hash_value);
#else // stlport
        std::pair<string_hash_t::iterator, string_hash_t::iterator> P = mStringHash.equal_range(hash_value);
        string_hash_t::iterator lower = P.first;
        string_hash_t::iterator upper = P.second;
#endif
        for (string_hash_t::iterator iter = lower; iter != upper; iter++)
        {
            entry = iter->second;
            ret_val = entry->mString;
            if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
            {
                entry->incCount();
                return entry;
            }
        }

        // not found, so add!
        LLStringTableEntry* newentry = new LLStringTableEntry(str);
        ret_val = newentry->mString;
        mStringHash.insert(string_hash_t::value_type(hash_value, newentry));
#else
        string_list_t       *strlist = mStringList[hash_value];

        if (strlist)
        {
            string_list_t::iterator iter;
            for (iter = strlist->begin(); iter != strlist->end(); iter++)
            {
                entry = *iter;
                ret_val = entry->mString;
                if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
                {
                    entry->incCount();
                    return entry;
                }
            }
        }
        else
        {
            mStringList[hash_value] = new string_list_t;
            strlist = mStringList[hash_value];
        }

        // not found, so add!
        LLStringTableEntry *newentry = new LLStringTableEntry(str);
        //ret_val = newentry->mString;
        strlist->push_front(newentry);
#endif
        mUniqueEntries++;
        return newentry;
    }
    else
    {
        return NULL;
    }
}

void LLStringTable::removeString(const char *str)
{
    if (str)
    {
        char *ret_val;
        LLStringTableEntry  *entry;
        U32                 hash_value = hash_my_string(str, mMaxEntries);
#if STRING_TABLE_HASH_MAP
        {
#if 1 // Microsoft
            string_hash_t::iterator lower = mStringHash.lower_bound(hash_value);
            string_hash_t::iterator upper = mStringHash.upper_bound(hash_value);
#else // stlport
            std::pair<string_hash_t::iterator, string_hash_t::iterator> P = mStringHash.equal_range(hash_value);
            string_hash_t::iterator lower = P.first;
            string_hash_t::iterator upper = P.second;
#endif
            for (string_hash_t::iterator iter = lower; iter != upper; iter++)
            {
                entry = iter->second;
                ret_val = entry->mString;
                if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
                {
                    if (!entry->decCount())
                    {
                        mUniqueEntries--;
                        if (mUniqueEntries < 0)
                        {
                            LL_ERRS() << "LLStringTable:removeString trying to remove too many strings!" << LL_ENDL;
                        }
                        delete iter->second;
                        mStringHash.erase(iter);
                    }
                    return;
                }
            }
        }
#else
        string_list_t       *strlist = mStringList[hash_value];

        if (strlist)
        {
            string_list_t::iterator iter;
            for (iter = strlist->begin(); iter != strlist->end(); iter++)
            {
                entry = *iter;
                ret_val = entry->mString;
                if (!strncmp(ret_val, str, MAX_STRINGS_LENGTH))
                {
                    if (!entry->decCount())
                    {
                        mUniqueEntries--;
                        if (mUniqueEntries < 0)
                        {
                            LL_ERRS() << "LLStringTable:removeString trying to remove too many strings!" << LL_ENDL;
                        }
                        strlist->remove(entry);
                        delete entry;
                    }
                    return;
                }
            }
        }
#endif
    }
}

