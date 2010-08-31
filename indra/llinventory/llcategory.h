/** 
 * @file llcategory.h
 * @brief LLCategory class header file.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
//	S32 count = LLCategory::none.getSubCategoryCount();
//	for(S32 i = 0; i < count; i++)
//	{
//		llinfos << none.getSubCategory(i).lookupNmae() << llendl;
//	}
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
