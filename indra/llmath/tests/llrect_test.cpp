/**
 * @file   llrect_test.cpp
 * @author Martin Reddy
 * @date   2009-06-25
 * @brief  Test for llrect.cpp.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "../test/lldoctest.h"
#include "../llrect.h"

TEST_SUITE("LLRect") {

TEST_CASE("test_1")
{

        //
        // test the LLRect default constructor
        //

        LLSD zero;
        zero.append(0); zero.append(0); zero.append(0); zero.append(0);

        // default constructor
        LLRect rect1;
        ensure_equals("Empty rect", rect1.getValue(), zero);
        CHECK_MESSAGE(rect1.mLeft == 0, "Empty rect left");
        CHECK_MESSAGE(rect1.mTop == 0, "Empty rect top");
        CHECK_MESSAGE(rect1.mRight == 0, "Empty rect right");
        CHECK_MESSAGE(rect1.mBottom == 0, "Empty rect bottom");
        ensure_equals("Empty rect width", rect1.getWidth(), 0);
        ensure_equals("Empty rect height", rect1.getHeight(), 0);
        ensure_equals("Empty rect centerx", rect1.getCenterX(), 0);
        ensure_equals("Empty rect centery", rect1.getCenterY(), 0);
    
}

TEST_CASE("test_2")
{

        //
        // test the LLRectf default constructor
        //

        LLSD zerof;
        zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f);

        LLRectf rect2;
        ensure_equals("Empty rectf", rect2.getValue(), zerof);
        CHECK_MESSAGE(rect2.mLeft == 0.0f, "Empty rectf left");
        CHECK_MESSAGE(rect2.mTop == 0.0f, "Empty rectf top");
        CHECK_MESSAGE(rect2.mRight == 0.0f, "Empty rectf right");
        CHECK_MESSAGE(rect2.mBottom == 0.0f, "Empty rectf bottom");
        ensure_equals("Empty rectf width", rect2.getWidth(), 0.0f);
        ensure_equals("Empty rectf height", rect2.getHeight(), 0.0f);
        ensure_equals("Empty rectf centerx", rect2.getCenterX(), 0.0f);
        ensure_equals("Empty rectf centery", rect2.getCenterY(), 0.0f);
    
}

TEST_CASE("test_3")
{

        //
        // test the LLRect constructor from another LLRect
        //

        LLRect rect3(LLRect(1, 6, 7, 2));
        CHECK_MESSAGE(rect3.mLeft == 1, "Default rect left");
        CHECK_MESSAGE(rect3.mTop == 6, "Default rect top");
        CHECK_MESSAGE(rect3.mRight == 7, "Default rect right");
        CHECK_MESSAGE(rect3.mBottom == 2, "Default rect bottom");
        ensure_equals("Default rect width", rect3.getWidth(), 6);
        ensure_equals("Default rect height", rect3.getHeight(), 4);
        ensure_equals("Default rect centerx", rect3.getCenterX(), 4);
        ensure_equals("Default rect centery", rect3.getCenterY(), 4);
    
}

TEST_CASE("test_4")
{

        //
        // test the LLRectf four-float constructor
        //

        LLRectf rect4(1.0f, 5.0f, 6.0f, 2.0f);
        CHECK_MESSAGE(rect4.mLeft == 1.0f, "Default rectf left");
        CHECK_MESSAGE(rect4.mTop == 5.0f, "Default rectf top");
        CHECK_MESSAGE(rect4.mRight == 6.0f, "Default rectf right");
        CHECK_MESSAGE(rect4.mBottom == 2.0f, "Default rectf bottom");
        ensure_equals("Default rectf width", rect4.getWidth(), 5.0f);
        ensure_equals("Default rectf height", rect4.getHeight(), 3.0f);
        ensure_equals("Default rectf centerx", rect4.getCenterX(), 3.5f);
        ensure_equals("Default rectf centery", rect4.getCenterY(), 3.5f);
    
}

TEST_CASE("test_5")
{

        //
        // test the LLRectf LLSD constructor
        //

        LLSD array;
        array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
        LLRectf rect5(array);
        CHECK_MESSAGE(rect5.mLeft == -1.0f, "LLSD rectf left");
        CHECK_MESSAGE(rect5.mTop == 0.0f, "LLSD rectf top");
        CHECK_MESSAGE(rect5.mRight == 0.0f, "LLSD rectf right");
        CHECK_MESSAGE(rect5.mBottom == -1.0f, "LLSD rectf bottom");
        ensure_equals("LLSD rectf width", rect5.getWidth(), 1.0f);
        ensure_equals("LLSD rectf height", rect5.getHeight(), 1.0f);
        ensure_equals("LLSD rectf centerx", rect5.getCenterX(), -0.5f);
        ensure_equals("LLSD rectf centery", rect5.getCenterY(), -0.5f);
    
}

TEST_CASE("test_6")
{

        //
        // test directly setting the member variables for dimensions
        //

        LLRectf rectf;

        rectf.mLeft = -1.0f;
        rectf.mTop = 1.0f;
        rectf.mRight = 1.0f;
        rectf.mBottom = -1.0f;
        CHECK_MESSAGE(rectf.mLeft == -1.0f, "Member-set rectf left");
        CHECK_MESSAGE(rectf.mTop == 1.0f, "Member-set rectf top");
        CHECK_MESSAGE(rectf.mRight == 1.0f, "Member-set rectf right");
        CHECK_MESSAGE(rectf.mBottom == -1.0f, "Member-set rectf bottom");
        ensure_equals("Member-set rectf width", rectf.getWidth(), 2.0f);
        ensure_equals("Member-set rectf height", rectf.getHeight(), 2.0f);
        ensure_equals("Member-set rectf centerx", rectf.getCenterX(), 0.0f);
        ensure_equals("Member-set rectf centery", rectf.getCenterY(), 0.0f);
    
}

TEST_CASE("test_7")
{

        //
        // test the setValue() method
        //

        LLRectf rectf;

        LLSD array;
        array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
        rectf.setValue(array);
        CHECK_MESSAGE(rectf.mLeft == -1.0f, "setValue() rectf left");
        CHECK_MESSAGE(rectf.mTop == 0.0f, "setValue() rectf top");
        CHECK_MESSAGE(rectf.mRight == 0.0f, "setValue() rectf right");
        CHECK_MESSAGE(rectf.mBottom == -1.0f, "setValue() rectf bottom");
        ensure_equals("setValue() rectf width", rectf.getWidth(), 1.0f);
        ensure_equals("setValue() rectf height", rectf.getHeight(), 1.0f);
        ensure_equals("setValue() rectf centerx", rectf.getCenterX(), -0.5f);
        ensure_equals("setValue() rectf centery", rectf.getCenterY(), -0.5f);
    
}

TEST_CASE("test_8")
{

        //
        // test the set() method
        //

        LLRect rect;

        rect.set(10, 90, 70, 10);
        CHECK_MESSAGE(rect.mLeft == 10, "set() rectf left");
        CHECK_MESSAGE(rect.mTop == 90, "set() rectf top");
        CHECK_MESSAGE(rect.mRight == 70, "set() rectf right");
        CHECK_MESSAGE(rect.mBottom == 10, "set() rectf bottom");
        ensure_equals("set() rectf width", rect.getWidth(), 60);
        ensure_equals("set() rectf height", rect.getHeight(), 80);
        ensure_equals("set() rectf centerx", rect.getCenterX(), 40);
        ensure_equals("set() rectf centery", rect.getCenterY(), 50);
    
}

TEST_CASE("test_9")
{

        //
        // test the setOriginAndSize() method
        //

        LLRectf rectf;

        rectf.setOriginAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        CHECK_MESSAGE(rectf.mLeft == 0.0f, "setOriginAndSize() rectf left");
        CHECK_MESSAGE(rectf.mTop == 1.0f, "setOriginAndSize() rectf top");
        CHECK_MESSAGE(rectf.mRight == 2.0f, "setOriginAndSize() rectf right");
        CHECK_MESSAGE(rectf.mBottom == 0.0f, "setOriginAndSize() rectf bottom");
        ensure_equals("setOriginAndSize() rectf width", rectf.getWidth(), 2.0f);
        ensure_equals("setOriginAndSize() rectf height", rectf.getHeight(), 1.0f);
        ensure_equals("setOriginAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
        ensure_equals("setOriginAndSize() rectf centery", rectf.getCenterY(), 0.5f);
    
}

TEST_CASE("test_10")
{

        //
        // test the setLeftTopAndSize() method
        //

        LLRectf rectf;

        rectf.setLeftTopAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        CHECK_MESSAGE(rectf.mLeft == 0.0f, "setLeftTopAndSize() rectf left");
        CHECK_MESSAGE(rectf.mTop == 0.0f, "setLeftTopAndSize() rectf top");
        CHECK_MESSAGE(rectf.mRight == 2.0f, "setLeftTopAndSize() rectf right");
        CHECK_MESSAGE(rectf.mBottom == -1.0f, "setLeftTopAndSize() rectf bottom");
        ensure_equals("setLeftTopAndSize() rectf width", rectf.getWidth(), 2.0f);
        ensure_equals("setLeftTopAndSize() rectf height", rectf.getHeight(), 1.0f);
        ensure_equals("setLeftTopAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
        ensure_equals("setLeftTopAndSize() rectf centery", rectf.getCenterY(), -0.5f);
    
}

TEST_CASE("test_11")
{

        //
        // test the setCenterAndSize() method
        //

        LLRectf rectf;

        rectf.setCenterAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        CHECK_MESSAGE(rectf.mLeft == -1.0f, "setCenterAndSize() rectf left");
        CHECK_MESSAGE(rectf.mTop == 0.5f, "setCenterAndSize() rectf top");
        CHECK_MESSAGE(rectf.mRight == 1.0f, "setCenterAndSize() rectf right");
        CHECK_MESSAGE(rectf.mBottom == -0.5f, "setCenterAndSize() rectf bottom");
        ensure_equals("setCenterAndSize() rectf width", rectf.getWidth(), 2.0f);
        ensure_equals("setCenterAndSize() rectf height", rectf.getHeight(), 1.0f);
        ensure_equals("setCenterAndSize() rectf centerx", rectf.getCenterX(), 0.0f);
        ensure_equals("setCenterAndSize() rectf centery", rectf.getCenterY(), 0.0f);
    
}

TEST_CASE("test_12")
{

        //
        // test the validity checking method
        //

        LLRectf rectf;

        rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
        CHECK_MESSAGE(rectf.isValid(, "BBox is valid"));

        rectf.mLeft = 2.0f;
        CHECK_MESSAGE(! rectf.isValid(, "BBox is not valid"));

        rectf.makeValid();
        CHECK_MESSAGE(rectf.isValid(, "BBox forced valid"));

        rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
        CHECK_MESSAGE(rectf.isValid(, "BBox(0,0,0,0) is valid"));
    
}

TEST_CASE("test_13")
{

        //
        // test the null checking methods
        //

        LLRectf rectf;

        rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
        CHECK_MESSAGE(! rectf.isEmpty(, "BBox is not Null"));
        CHECK_MESSAGE(rectf.notEmpty(, "BBox notNull"));

        rectf.mLeft = 2.0f;
        rectf.makeValid();
        CHECK_MESSAGE(rectf.isEmpty(, "BBox is now Null"));

        rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
        CHECK_MESSAGE(rectf.isEmpty(, "BBox(0,0,0,0) is Null"));
    
}

TEST_CASE("test_14")
{

        //
        // test the (in)equality operators
        //

        LLRectf rect1, rect2;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 0.9f, 1.0f, -1.0f);

        CHECK_MESSAGE(! (rect1 == rect2, "rect1 == rect2 (false)"));
        CHECK_MESSAGE(rect1 != rect2, "rect1 != rect2 (true)");

        CHECK_MESSAGE(rect1 == rect1, "rect1 == rect1 (true)");
        CHECK_MESSAGE(! (rect1 != rect1, "rect1 != rect1 (false)"));
    
}

TEST_CASE("test_15")
{

        //
        // test the copy constructor
        //

        LLRectf rect1, rect2(rect1);

        CHECK_MESSAGE(rect1 == rect2, "rect1 == rect2 (true)");
        CHECK_MESSAGE(! (rect1 != rect2, "rect1 != rect2 (false)"));
    
}

TEST_CASE("test_16")
{

        //
        // test the translate() method
        //

        LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
        LLRectf rect2(rect1);

        rect1.translate(0.0f, 0.0f);

        CHECK_MESSAGE(rect1 == rect2, "translate(0, 0)");

        rect1.translate(100.0f, 100.0f);
        rect1.translate(-100.0f, -100.0f);

        CHECK_MESSAGE(rect1 == rect2, "translate(100, 100) + translate(-100, -100)");

        rect1.translate(10.0f, 0.0f);
        rect2.set(9.0f, 1.0f, 11.0f, -1.0f);
        CHECK_MESSAGE(rect1 == rect2, "translate(10, 0)");

        rect1.translate(0.0f, 10.0f);
        rect2.set(9.0f, 11.0f, 11.0f, 9.0f);
        CHECK_MESSAGE(rect1 == rect2, "translate(0, 10)");

        rect1.translate(-10.0f, -10.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        CHECK_MESSAGE(rect1 == rect2, "translate(-10, -10)");
    
}

TEST_CASE("test_17")
{

        //
        // test the stretch() method
        //

        LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
        LLRectf rect2(rect1);

        rect1.stretch(0.0f);
        CHECK_MESSAGE(rect1 == rect2, "stretch(0)");

        rect1.stretch(0.0f, 0.0f);
        CHECK_MESSAGE(rect1 == rect2, "stretch(0, 0)");

        rect1.stretch(10.0f);
        rect1.stretch(-10.0f);
        CHECK_MESSAGE(rect1 == rect2, "stretch(10) + stretch(-10)");

        rect1.stretch(2.0f, 1.0f);
        rect2.set(-3.0f, 2.0f, 3.0f, -2.0f);
        CHECK_MESSAGE(rect1 == rect2, "stretch(2, 1)");
    
}

TEST_CASE("test_18")
{

        //
        // test the unionWith() method
        //

        LLRectf rect1, rect2, rect3;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        CHECK_MESSAGE(rect3 == rect1, "union with self");

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        CHECK_MESSAGE(rect3 == LLRectf(-2.0f, 2.0f, 1.0f, -1.0f, "union - overlap"));

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        CHECK_MESSAGE(rect3 == LLRectf(-1.0f, 10.0f, 10.0f, -1.0f, "union - no overlap"));
    
}

TEST_CASE("test_19")
{

        //
        // test the intersectWith() methods
        //

        LLRectf rect1, rect2, rect3;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        CHECK_MESSAGE(rect3 == rect1, "intersect with self");

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        CHECK_MESSAGE(rect3 == LLRectf(-1.0f, 1.0f, 0.0f, 0.0f, "intersect - overlap"));

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        CHECK_MESSAGE(rect3.isEmpty(, "intersect - no overlap"));
    
}

TEST_CASE("test_20")
{

        //
        // test the pointInRect() method
        //

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);

        CHECK_MESSAGE(rect.pointInRect(0.0f, 0.0f, "(0,0) not in rect") == false);
        CHECK_MESSAGE(rect.pointInRect(2.0f, 2.0f, "(2,2) in rect") == true);
        CHECK_MESSAGE(rect.pointInRect(1.0f, 1.0f, "(1,1) in rect") == true);
        CHECK_MESSAGE(rect.pointInRect(3.0f, 3.0f, "(3,3) not in rect") == false);
        CHECK_MESSAGE(rect.pointInRect(2.999f, 2.999f, "(2.999,2.999) in rect") == true);
        CHECK_MESSAGE(rect.pointInRect(2.999f, 3.0f, "(2.999,3.0) not in rect") == false);
        CHECK_MESSAGE(rect.pointInRect(3.0f, 2.999f, "(3.0,2.999) not in rect") == false);
    
}

TEST_CASE("test_21")
{

        //
        // test the localPointInRect() method
        //

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);

        CHECK_MESSAGE(rect.localPointInRect(0.0f, 0.0f, "(0,0) in local rect") == true);
        CHECK_MESSAGE(rect.localPointInRect(-0.0001f, -0.001f, "(-0.0001,-0.0001) not in local rect") == false);
        CHECK_MESSAGE(rect.localPointInRect(1.0f, 1.0f, "(1,1) in local rect") == true);
        CHECK_MESSAGE(rect.localPointInRect(2.0f, 2.0f, "(2,2) not in local rect") == false);
        CHECK_MESSAGE(rect.localPointInRect(1.999f, 1.999f, "(1.999,1.999) in local rect") == true);
        CHECK_MESSAGE(rect.localPointInRect(1.999f, 2.0f, "(1.999,2.0) not in local rect") == false);
        CHECK_MESSAGE(rect.localPointInRect(2.0f, 1.999f, "(2.0,1.999) not in local rect") == false);
    
}

TEST_CASE("test_22")
{

        //
        // test the clampPointToRect() method
        //

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);
        F32 x, y;

        x = 2.0f; y = 2.0f;
        rect.clampPointToRect(x, y);
        CHECK_MESSAGE(x == 2.0f, "clamp x-coord within rect");
        CHECK_MESSAGE(y == 2.0f, "clamp y-coord within rect");

        x = -100.0f; y = 100.0f;
        rect.clampPointToRect(x, y);
        CHECK_MESSAGE(x == 1.0f, "clamp x-coord outside rect");
        CHECK_MESSAGE(y == 3.0f, "clamp y-coord outside rect");

        x = 3.0f; y = 1.0f;
        rect.clampPointToRect(x, y);
        CHECK_MESSAGE(x == 3.0f, "clamp x-coord edge rect");
        CHECK_MESSAGE(y == 1.0f, "clamp y-coord edge rect");
    
}

} // TEST_SUITE

