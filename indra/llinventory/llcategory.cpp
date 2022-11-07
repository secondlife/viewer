/** 
 * @file llcategory.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llcategory.h"

#include "message.h"

const LLCategory LLCategory::none;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

// This is the storage of the category names. It's loosely based on a
// heap-like structure with indices into it for faster searching and
// so that we don't have to maintain a balanced heap. It's *VITALLY*
// important that the CATEGORY_INDEX and CATEGORY_NAME tables are kept
// in synch.

// CATEGORY_INDEX indexes into CATEGORY_NAME at the first occurance of
// a child. Thus, the first child of root is "Object" which is located
// in CATEGORY_NAME[1].
const S32 CATEGORY_INDEX[] =
{
    1,  // ROOT
    6,  // object
    7,  // clothing
    7,  // texture
    7,  // sound
    7,  // landmark
    7,  // object|component
    7,  // off the end (required for child count calculations)
};

// The heap of names
const char* CATEGORY_NAME[] =
{
    "(none)",
    "Object",       // (none)
    "Clothing",
    "Texture",
    "Sound",
    "Landmark",
    "Component",    // object
    NULL
};

///----------------------------------------------------------------------------
/// Class llcategory
///----------------------------------------------------------------------------

LLCategory::LLCategory()
{
    // this is used as a simple compile time assertion. If this code
    // fails to compile, the depth has been changed, and we need to
    // clean up some of the code that relies on the depth, such as the
    // default constructor. If CATEGORY_DEPTH != 4, this code will
    // attempt to construct a zero length array - which the compiler
    // should balk at.
//  static const char CATEGORY_DEPTH_CHECK[(CATEGORY_DEPTH == 4)?1:0] = {' '};  // unused

    // actually initialize the object.
    mData[0] = 0;
    mData[1] = 0;
    mData[2] = 0;
    mData[3] = 0;
}

void LLCategory::init(U32 value)
{
    U8 v;
    for(S32 i = 0; i < CATEGORY_DEPTH; i++)
    {
        v = (U8)((0x000000ff) & value);
        mData[CATEGORY_DEPTH - 1 - i] = v;
        value >>= 8;
    }
}

U32 LLCategory::getU32() const
{
    U32 rv = 0;
    rv |= mData[0];
    rv <<= 8;
    rv |= mData[1];
    rv <<= 8;
    rv |= mData[2];
    rv <<= 8;
    rv |= mData[3];
    return rv;
}

S32 LLCategory::getSubCategoryCount() const
{
    S32 rv = CATEGORY_INDEX[mData[0] + 1] - CATEGORY_INDEX[mData[0]];
    return rv;
}

// This method will return a category that is the nth subcategory. If
// you're already at the bottom of the hierarchy, then the method will
// return a copy of this.
LLCategory LLCategory::getSubCategory(U8 n) const
{
    LLCategory rv(*this);
    for(S32 i = 0; i < (CATEGORY_DEPTH - 1); i++)
    {
        if(rv.mData[i] == 0)
        {
            rv.mData[i] = n + 1;
            break;
        }
    }
    return rv;
}

// This method will return the name of the leaf category type
const char* LLCategory::lookupName() const
{
    S32 i = 0;
    S32 index = mData[i++];
    while((i < CATEGORY_DEPTH) && (mData[i] != 0))
    {
        index = CATEGORY_INDEX[index];
        ++i;
    }
    return CATEGORY_NAME[index];
}

// message serialization
void LLCategory::packMessage(LLMessageSystem* msg) const
{
    U32 data = getU32();
    msg->addU32Fast(_PREHASH_Category, data);
}

// message serialization
void LLCategory::unpackMessage(LLMessageSystem* msg, const char* block)
{
    U32 data;
    msg->getU32Fast(block, _PREHASH_Category, data);
    init(data);
}

// message serialization
void LLCategory::unpackMultiMessage(LLMessageSystem* msg, const char* block,
                                    S32 block_num)
{
    U32 data;
    msg->getU32Fast(block, _PREHASH_Category, data, block_num);
    init(data);
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
