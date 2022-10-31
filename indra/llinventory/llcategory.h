/** 
 * @file llcategory.h
 * @brief LLCategory class header file.
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

#ifndef LL_LLCATEGORY_H
#define LL_LLCATEGORY_H

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCategory
//
// An instance of the LLCategory class represents a particular
// category in a hierarchical classification system. For now, it is 4
// levels deep with 255 (minus 1) possible values at each level. If a
// non zero value is found at level 4, that is the leaf category,
// otherwise, it is the first level that has a 0 in the next depth
// level.
//
// To output the names of all top level categories, you could do the
// following:
//
//  S32 count = LLCategory::none.getSubCategoryCount();
//  for(S32 i = 0; i < count; i++)
//  {
//      LL_INFOS() << none.getSubCategory(i).lookupNmae() << LL_ENDL;
//  }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMessageSystem;

class LLCategory
{
public:
    // Nice default static const.
    static const LLCategory none;

    // construction. Since this is really a POD type, destruction,
    // copy, and assignment are handled by the compiler.
    LLCategory();
    explicit LLCategory(U32 value) { init(value); }

    // methods
    void init(U32 value);
    U32 getU32() const;
    S32 getSubCategoryCount() const;

    // This method will return a category that is the nth
    // subcategory. If you're already at the bottom of the hierarchy,
    // then the method will return a copy of this.
    LLCategory getSubCategory(U8 n) const;

    // This method will return the name of the leaf category type
    const char* lookupName() const;

    // This method will return the full hierarchy name in an easily
    // interpreted (TOP)|(SUB1)|(SUB2) format. *NOTE: not implemented
    // because we don't have anything but top level categories at the
    // moment.
    //const char* lookupFullName() const;

    // message serialization
    void packMessage(LLMessageSystem* msg) const;
    void unpackMessage(LLMessageSystem* msg, const char* block);
    void unpackMultiMessage(LLMessageSystem* msg, const char* block,
                            S32 block_num);
protected:
    enum
    {
        CATEGORY_TOP = 0,
        CATEGORY_DEPTH = 4,
    };

    U8 mData[CATEGORY_DEPTH];
};


#endif // LL_LLCATEGORY_H
