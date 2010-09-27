/**
 * @file   llrect_test.cpp
 * @author Martin Reddy
 * @date   2009-06-25
 * @brief  Test for llrect.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009-2009, Linden Research, Inc.
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
#include "../test/lltut.h"
#include "../llrect.h"

namespace tut
{
	struct LLRectData
	{
	};

	typedef test_group<LLRectData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory llrect_test_factory("LLRect");
}

namespace tut
{
	template<> template<>
	void object::test<1>()
	{	
		//
		// test the LLRect default constructor
		//
		
		LLSD zero;
		zero.append(0);	zero.append(0); zero.append(0); zero.append(0);
		
		// default constructor
		LLRect rect1;
		ensure_equals("Empty rect", rect1.getValue(), zero);
		ensure_equals("Empty rect left", rect1.mLeft, 0);
		ensure_equals("Empty rect top", rect1.mTop, 0);
		ensure_equals("Empty rect right", rect1.mRight, 0);
		ensure_equals("Empty rect bottom", rect1.mBottom, 0);
		ensure_equals("Empty rect width", rect1.getWidth(), 0);
		ensure_equals("Empty rect height", rect1.getHeight(), 0);
		ensure_equals("Empty rect centerx", rect1.getCenterX(), 0);
		ensure_equals("Empty rect centery", rect1.getCenterY(), 0);
	}
	
	template<> template<>
	void object::test<2>()
	{	
		//
		// test the LLRectf default constructor
		//
		
		LLSD zerof;
		zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f);

		LLRectf rect2;
		ensure_equals("Empty rectf", rect2.getValue(), zerof);
		ensure_equals("Empty rectf left", rect2.mLeft, 0.0f);
		ensure_equals("Empty rectf top", rect2.mTop, 0.0f);
		ensure_equals("Empty rectf right", rect2.mRight, 0.0f);
		ensure_equals("Empty rectf bottom", rect2.mBottom, 0.0f);
		ensure_equals("Empty rectf width", rect2.getWidth(), 0.0f);
		ensure_equals("Empty rectf height", rect2.getHeight(), 0.0f);
		ensure_equals("Empty rectf centerx", rect2.getCenterX(), 0.0f);
		ensure_equals("Empty rectf centery", rect2.getCenterY(), 0.0f);
	}

	template<> template<>
	void object::test<3>()
	{	
		//
		// test the LLRect constructor from another LLRect
		//	
		
		LLRect rect3(LLRect(1, 6, 7, 2));
		ensure_equals("Default rect left", rect3.mLeft, 1);
		ensure_equals("Default rect top", rect3.mTop, 6);
		ensure_equals("Default rect right", rect3.mRight, 7);
		ensure_equals("Default rect bottom", rect3.mBottom, 2);
		ensure_equals("Default rect width", rect3.getWidth(), 6);
		ensure_equals("Default rect height", rect3.getHeight(), 4);
		ensure_equals("Default rect centerx", rect3.getCenterX(), 4);
		ensure_equals("Default rect centery", rect3.getCenterY(), 4);
	}

	template<> template<>
	void object::test<4>()
	{	
		//
		// test the LLRectf four-float constructor
		//	
		
		LLRectf rect4(1.0f, 5.0f, 6.0f, 2.0f);
		ensure_equals("Default rectf left", rect4.mLeft, 1.0f);
		ensure_equals("Default rectf top", rect4.mTop, 5.0f);
		ensure_equals("Default rectf right", rect4.mRight, 6.0f);
		ensure_equals("Default rectf bottom", rect4.mBottom, 2.0f);
		ensure_equals("Default rectf width", rect4.getWidth(), 5.0f);
		ensure_equals("Default rectf height", rect4.getHeight(), 3.0f);
		ensure_equals("Default rectf centerx", rect4.getCenterX(), 3.5f);
		ensure_equals("Default rectf centery", rect4.getCenterY(), 3.5f);
	}

	template<> template<>
	void object::test<5>()
	{	
		//
		// test the LLRectf LLSD constructor
		//

		LLSD array;
		array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
		LLRectf rect5(array);
		ensure_equals("LLSD rectf left", rect5.mLeft, -1.0f);
		ensure_equals("LLSD rectf top", rect5.mTop, 0.0f);
		ensure_equals("LLSD rectf right", rect5.mRight, 0.0f);
		ensure_equals("LLSD rectf bottom", rect5.mBottom, -1.0f);
		ensure_equals("LLSD rectf width", rect5.getWidth(), 1.0f);
		ensure_equals("LLSD rectf height", rect5.getHeight(), 1.0f);
		ensure_equals("LLSD rectf centerx", rect5.getCenterX(), -0.5f);
		ensure_equals("LLSD rectf centery", rect5.getCenterY(), -0.5f);		
	}
	
	template<> template<>
	void object::test<6>()
	{	
		//
		// test directly setting the member variables for dimensions
		//
		
		LLRectf rectf;

		rectf.mLeft = -1.0f;
		rectf.mTop = 1.0f;
		rectf.mRight = 1.0f;
		rectf.mBottom = -1.0f;
		ensure_equals("Member-set rectf left", rectf.mLeft, -1.0f);
		ensure_equals("Member-set rectf top", rectf.mTop, 1.0f);
		ensure_equals("Member-set rectf right", rectf.mRight, 1.0f);
		ensure_equals("Member-set rectf bottom", rectf.mBottom, -1.0f);
		ensure_equals("Member-set rectf width", rectf.getWidth(), 2.0f);
		ensure_equals("Member-set rectf height", rectf.getHeight(), 2.0f);
		ensure_equals("Member-set rectf centerx", rectf.getCenterX(), 0.0f);
		ensure_equals("Member-set rectf centery", rectf.getCenterY(), 0.0f);
	}
		
	template<> template<>
	void object::test<7>()
	{	
		//
		// test the setValue() method
		//
			
		LLRectf rectf;
		
		LLSD array;
		array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
		rectf.setValue(array);
		ensure_equals("setValue() rectf left", rectf.mLeft, -1.0f);
		ensure_equals("setValue() rectf top", rectf.mTop, 0.0f);
		ensure_equals("setValue() rectf right", rectf.mRight, 0.0f);
		ensure_equals("setValue() rectf bottom", rectf.mBottom, -1.0f);
		ensure_equals("setValue() rectf width", rectf.getWidth(), 1.0f);
		ensure_equals("setValue() rectf height", rectf.getHeight(), 1.0f);
		ensure_equals("setValue() rectf centerx", rectf.getCenterX(), -0.5f);
		ensure_equals("setValue() rectf centery", rectf.getCenterY(), -0.5f);
	}
		
	template<> template<>
	void object::test<8>()
	{	
		//
		// test the set() method
		//
		
		LLRect rect;
		
		rect.set(10, 90, 70, 10);
		ensure_equals("set() rectf left", rect.mLeft, 10);
		ensure_equals("set() rectf top", rect.mTop, 90);
		ensure_equals("set() rectf right", rect.mRight, 70);
		ensure_equals("set() rectf bottom", rect.mBottom, 10);
		ensure_equals("set() rectf width", rect.getWidth(), 60);
		ensure_equals("set() rectf height", rect.getHeight(), 80);
		ensure_equals("set() rectf centerx", rect.getCenterX(), 40);
		ensure_equals("set() rectf centery", rect.getCenterY(), 50);
	}

	template<> template<>
	void object::test<9>()
	{	
		//
		// test the setOriginAndSize() method
		//
		
		LLRectf rectf;
		
		rectf.setOriginAndSize(0.0f, 0.0f, 2.0f, 1.0f);
		ensure_equals("setOriginAndSize() rectf left", rectf.mLeft, 0.0f);
		ensure_equals("setOriginAndSize() rectf top", rectf.mTop, 1.0f);
		ensure_equals("setOriginAndSize() rectf right", rectf.mRight, 2.0f);
		ensure_equals("setOriginAndSize() rectf bottom", rectf.mBottom, 0.0f);
		ensure_equals("setOriginAndSize() rectf width", rectf.getWidth(), 2.0f);
		ensure_equals("setOriginAndSize() rectf height", rectf.getHeight(), 1.0f);
		ensure_equals("setOriginAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
		ensure_equals("setOriginAndSize() rectf centery", rectf.getCenterY(), 0.5f);	
	}
	
	template<> template<>
	void object::test<10>()
	{	
		//
		// test the setLeftTopAndSize() method
		//
		
		LLRectf rectf;

		rectf.setLeftTopAndSize(0.0f, 0.0f, 2.0f, 1.0f);
		ensure_equals("setLeftTopAndSize() rectf left", rectf.mLeft, 0.0f);
		ensure_equals("setLeftTopAndSize() rectf top", rectf.mTop, 0.0f);
		ensure_equals("setLeftTopAndSize() rectf right", rectf.mRight, 2.0f);
		ensure_equals("setLeftTopAndSize() rectf bottom", rectf.mBottom, -1.0f);
		ensure_equals("setLeftTopAndSize() rectf width", rectf.getWidth(), 2.0f);
		ensure_equals("setLeftTopAndSize() rectf height", rectf.getHeight(), 1.0f);
		ensure_equals("setLeftTopAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
		ensure_equals("setLeftTopAndSize() rectf centery", rectf.getCenterY(), -0.5f);
	}

	template<> template<>
	void object::test<11>()
	{	
		//
		// test the setCenterAndSize() method
		//
		
		LLRectf rectf;
		
		rectf.setCenterAndSize(0.0f, 0.0f, 2.0f, 1.0f);
		ensure_equals("setCenterAndSize() rectf left", rectf.mLeft, -1.0f);
		ensure_equals("setCenterAndSize() rectf top", rectf.mTop, 0.5f);
		ensure_equals("setCenterAndSize() rectf right", rectf.mRight, 1.0f);
		ensure_equals("setCenterAndSize() rectf bottom", rectf.mBottom, -0.5f);
		ensure_equals("setCenterAndSize() rectf width", rectf.getWidth(), 2.0f);
		ensure_equals("setCenterAndSize() rectf height", rectf.getHeight(), 1.0f);
		ensure_equals("setCenterAndSize() rectf centerx", rectf.getCenterX(), 0.0f);
		ensure_equals("setCenterAndSize() rectf centery", rectf.getCenterY(), 0.0f);
	}

	template<> template<>
	void object::test<12>()
	{	
		//
		// test the validity checking method
		//
		
		LLRectf rectf;

		rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
		ensure("BBox is valid", rectf.isValid());

		rectf.mLeft = 2.0f;
		ensure("BBox is not valid", ! rectf.isValid());

		rectf.makeValid();
		ensure("BBox forced valid", rectf.isValid());

		rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
		ensure("BBox(0,0,0,0) is valid", rectf.isValid());
	}

	template<> template<>
	void object::test<13>()
	{	
		//
		// test the null checking methods
		//
		
		LLRectf rectf;
		
		rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
		ensure("BBox is not Null", ! rectf.isEmpty());
		ensure("BBox notNull", rectf.notEmpty());
		
		rectf.mLeft = 2.0f;
		rectf.makeValid();
		ensure("BBox is now Null", rectf.isEmpty());
		
		rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
		ensure("BBox(0,0,0,0) is Null", rectf.isEmpty());
	}
		
	template<> template<>
	void object::test<14>()
	{	
		//
		// test the (in)equality operators
		//
		
		LLRectf rect1, rect2;

		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(-1.0f, 0.9f, 1.0f, -1.0f);

		ensure("rect1 == rect2 (false)", ! (rect1 == rect2));
		ensure("rect1 != rect2 (true)", rect1 != rect2);

		ensure("rect1 == rect1 (true)", rect1 == rect1);
		ensure("rect1 != rect1 (false)", ! (rect1 != rect1));
	}
	
	template<> template<>
	void object::test<15>()
	{	
		//
		// test the copy constructor
		//
		
		LLRectf rect1, rect2(rect1);

		ensure("rect1 == rect2 (true)", rect1 == rect2);
		ensure("rect1 != rect2 (false)", ! (rect1 != rect2));
	}

	template<> template<>
	void object::test<16>()
	{	
		//
		// test the translate() method
		//
		
		LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
		LLRectf rect2(rect1);

		rect1.translate(0.0f, 0.0f);

		ensure("translate(0, 0)", rect1 == rect2);

		rect1.translate(100.0f, 100.0f);
		rect1.translate(-100.0f, -100.0f);

		ensure("translate(100, 100) + translate(-100, -100)", rect1 == rect2);

		rect1.translate(10.0f, 0.0f);
		rect2.set(9.0f, 1.0f, 11.0f, -1.0f);
		ensure("translate(10, 0)", rect1 == rect2);

		rect1.translate(0.0f, 10.0f);
		rect2.set(9.0f, 11.0f, 11.0f, 9.0f);
		ensure("translate(0, 10)", rect1 == rect2);

		rect1.translate(-10.0f, -10.0f);
		rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
		ensure("translate(-10, -10)", rect1 == rect2);
	}

	template<> template<>
	void object::test<17>()
	{	
		//
		// test the stretch() method
		//
		
		LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
		LLRectf rect2(rect1);
		
		rect1.stretch(0.0f);
		ensure("stretch(0)", rect1 == rect2);
		
		rect1.stretch(0.0f, 0.0f);
		ensure("stretch(0, 0)", rect1 == rect2);
		
		rect1.stretch(10.0f);
		rect1.stretch(-10.0f);
		ensure("stretch(10) + stretch(-10)", rect1 == rect2);
		
		rect1.stretch(2.0f, 1.0f);
		rect2.set(-3.0f, 2.0f, 3.0f, -2.0f);
		ensure("stretch(2, 1)", rect1 == rect2);
	}
	
	
	template<> template<>
	void object::test<18>()
	{	
		//
		// test the unionWith() method
		//
		
		LLRectf rect1, rect2, rect3;

		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect3 = rect1;
		rect3.unionWith(rect2);
		ensure_equals("union with self", rect3, rect1);

		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
		rect3 = rect1;
		rect3.unionWith(rect2);
		ensure_equals("union - overlap", rect3, LLRectf(-2.0f, 2.0f, 1.0f, -1.0f));
		
		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
		rect3 = rect1;
		rect3.unionWith(rect2);
		ensure_equals("union - no overlap", rect3, LLRectf(-1.0f, 10.0f, 10.0f, -1.0f));
	}

	template<> template<>
	void object::test<19>()
	{	
		//
		// test the intersectWith() methods
		//
		
		LLRectf rect1, rect2, rect3;
		
		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect3 = rect1;
		rect3.intersectWith(rect2);
		ensure_equals("intersect with self", rect3, rect1);
		
		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
		rect3 = rect1;
		rect3.intersectWith(rect2);
		ensure_equals("intersect - overlap", rect3, LLRectf(-1.0f, 1.0f, 0.0f, 0.0f));
		
		rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
		rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
		rect3 = rect1;
		rect3.intersectWith(rect2);
		ensure("intersect - no overlap", rect3.isEmpty());
	}
		
	template<> template<>
	void object::test<20>()
	{
		//
		// test the pointInRect() method
		//
		
		LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);

		ensure("(0,0) not in rect", rect.pointInRect(0.0f, 0.0f) == FALSE);
		ensure("(2,2) in rect", rect.pointInRect(2.0f, 2.0f) == TRUE);
		ensure("(1,1) in rect", rect.pointInRect(1.0f, 1.0f) == TRUE);
		ensure("(3,3) not in rect", rect.pointInRect(3.0f, 3.0f) == FALSE);
		ensure("(2.999,2.999) in rect", rect.pointInRect(2.999f, 2.999f) == TRUE);
		ensure("(2.999,3.0) not in rect", rect.pointInRect(2.999f, 3.0f) == FALSE);
		ensure("(3.0,2.999) not in rect", rect.pointInRect(3.0f, 2.999f) == FALSE);
	}

	template<> template<>
	void object::test<21>()
	{
		//
		// test the localPointInRect() method
		//
		
		LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);
		
		ensure("(0,0) in local rect", rect.localPointInRect(0.0f, 0.0f) == TRUE);
		ensure("(-0.0001,-0.0001) not in local rect", rect.localPointInRect(-0.0001f, -0.001f) == FALSE);
		ensure("(1,1) in local rect", rect.localPointInRect(1.0f, 1.0f) == TRUE);
		ensure("(2,2) not in local rect", rect.localPointInRect(2.0f, 2.0f) == FALSE);
		ensure("(1.999,1.999) in local rect", rect.localPointInRect(1.999f, 1.999f) == TRUE);
		ensure("(1.999,2.0) not in local rect", rect.localPointInRect(1.999f, 2.0f) == FALSE);
		ensure("(2.0,1.999) not in local rect", rect.localPointInRect(2.0f, 1.999f) == FALSE);
	}
	
	template<> template<>
	void object::test<22>()
	{
		//
		// test the clampPointToRect() method
		//

		LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);
		F32 x, y;

		x = 2.0f; y = 2.0f;
		rect.clampPointToRect(x, y);
		ensure_equals("clamp x-coord within rect", x, 2.0f);
		ensure_equals("clamp y-coord within rect", y, 2.0f);

		x = -100.0f; y = 100.0f;
		rect.clampPointToRect(x, y);
		ensure_equals("clamp x-coord outside rect", x, 1.0f);
		ensure_equals("clamp y-coord outside rect", y, 3.0f);

		x = 3.0f; y = 1.0f;
		rect.clampPointToRect(x, y);
		ensure_equals("clamp x-coord edge rect", x, 3.0f);
		ensure_equals("clamp y-coord edge rect", y, 1.0f);
	}
}
