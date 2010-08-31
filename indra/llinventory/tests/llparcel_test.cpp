/** 
 * @file llinventoryparcel_tut.cpp
 * @author Moss
 * @date 2007-04-17
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include <string>

#include "linden_common.h"

#include "../llparcel.h"

#include "../test/lltut.h"

namespace tut
{
	struct llinventoryparcel_data
	{
	};
	typedef test_group<llinventoryparcel_data> llinventoryparcel_test;
	typedef llinventoryparcel_test::object llinventoryparcel_object;
	tut::llinventoryparcel_test llinventoryparcel("llinventoryparcel");

	template<> template<>
	void llinventoryparcel_object::test<1>()
	{
		for (S32 i=0; i<LLParcel::C_COUNT; ++i)
		{
			const std::string& catstring =  LLParcel::getCategoryString(LLParcel::ECategory(i));
			ensure("LLParcel::getCategoryString(i)",
			       !catstring.empty());

			const std::string& catuistring =  LLParcel::getCategoryUIString(LLParcel::ECategory(i));
			ensure("LLParcel::getCategoryUIString(i)",
			       !catuistring.empty());

			ensure_equals("LLParcel::ECategory mapping of string back to enum", LLParcel::getCategoryFromString(catstring), i);
			ensure_equals("LLParcel::ECategory mapping of uistring back to enum", LLParcel::getCategoryFromUIString(catuistring), i);
		}

		// test the C_ANY case, which has to work for UI strings
		const std::string& catuistring =  LLParcel::getCategoryUIString(LLParcel::C_ANY);
		ensure("LLParcel::getCategoryUIString(C_ANY)",
		       !catuistring.empty());

		ensure_equals("LLParcel::ECategory mapping of uistring back to enum", LLParcel::getCategoryFromUIString(catuistring), LLParcel::C_ANY);
	}
}
