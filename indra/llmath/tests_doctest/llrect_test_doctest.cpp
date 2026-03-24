// doctest translation of llrect_test.cpp
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"
#include "indra/test/tut_compat_doctest.h"
#include "linden_common.h"
#include "llsdutil.h"
#include "../llrect.h"

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

namespace tut
{
    using tut_compat::ensure;
    using tut_compat::ensure_equals;
    using tut_compat::ensure_not;
    using tut_compat::ensure_throws;

    struct LLRectData
    {
    };
} // namespace tut

TUT_SUITE("llrect_test")
{
    TUT_CASE("llrect_test::object_test_1")
    {
        using namespace tut;

        LLSD zero;
        zero.append(0); zero.append(0); zero.append(0); zero.append(0);

        LLRect rect1;
        TUT_ENSURE("Empty rect", llsd_equals(rect1.getValue(), zero));
        TUT_ENSURE_EQ("Empty rect left", rect1.mLeft, 0);
        TUT_ENSURE_EQ("Empty rect top", rect1.mTop, 0);
        TUT_ENSURE_EQ("Empty rect right", rect1.mRight, 0);
        TUT_ENSURE_EQ("Empty rect bottom", rect1.mBottom, 0);
        TUT_ENSURE_EQ("Empty rect width", rect1.getWidth(), 0);
        TUT_ENSURE_EQ("Empty rect height", rect1.getHeight(), 0);
        TUT_ENSURE_EQ("Empty rect centerx", rect1.getCenterX(), 0);
        TUT_ENSURE_EQ("Empty rect centery", rect1.getCenterY(), 0);
    }

    TUT_CASE("llrect_test::object_test_2")
    {
        using namespace tut;

        LLSD zerof;
        zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f); zerof.append(0.0f);

        LLRectf rect2;
        TUT_ENSURE("Empty rectf", llsd_equals(rect2.getValue(), zerof));
        TUT_ENSURE_EQ("Empty rectf left", rect2.mLeft, 0.0f);
        TUT_ENSURE_EQ("Empty rectf top", rect2.mTop, 0.0f);
        TUT_ENSURE_EQ("Empty rectf right", rect2.mRight, 0.0f);
        TUT_ENSURE_EQ("Empty rectf bottom", rect2.mBottom, 0.0f);
        TUT_ENSURE_EQ("Empty rectf width", rect2.getWidth(), 0.0f);
        TUT_ENSURE_EQ("Empty rectf height", rect2.getHeight(), 0.0f);
        TUT_ENSURE_EQ("Empty rectf centerx", rect2.getCenterX(), 0.0f);
        TUT_ENSURE_EQ("Empty rectf centery", rect2.getCenterY(), 0.0f);
    }

    TUT_CASE("llrect_test::object_test_3")
    {
        using namespace tut;

        LLRect rect3(LLRect(1, 6, 7, 2));
        TUT_ENSURE_EQ("Default rect left", rect3.mLeft, 1);
        TUT_ENSURE_EQ("Default rect top", rect3.mTop, 6);
        TUT_ENSURE_EQ("Default rect right", rect3.mRight, 7);
        TUT_ENSURE_EQ("Default rect bottom", rect3.mBottom, 2);
        TUT_ENSURE_EQ("Default rect width", rect3.getWidth(), 6);
        TUT_ENSURE_EQ("Default rect height", rect3.getHeight(), 4);
        TUT_ENSURE_EQ("Default rect centerx", rect3.getCenterX(), 4);
        TUT_ENSURE_EQ("Default rect centery", rect3.getCenterY(), 4);
    }

    TUT_CASE("llrect_test::object_test_4")
    {
        using namespace tut;

        LLRectf rect4(1.0f, 5.0f, 6.0f, 2.0f);
        TUT_ENSURE_EQ("Default rectf left", rect4.mLeft, 1.0f);
        TUT_ENSURE_EQ("Default rectf top", rect4.mTop, 5.0f);
        TUT_ENSURE_EQ("Default rectf right", rect4.mRight, 6.0f);
        TUT_ENSURE_EQ("Default rectf bottom", rect4.mBottom, 2.0f);
        TUT_ENSURE_EQ("Default rectf width", rect4.getWidth(), 5.0f);
        TUT_ENSURE_EQ("Default rectf height", rect4.getHeight(), 3.0f);
        TUT_ENSURE_EQ("Default rectf centerx", rect4.getCenterX(), 3.5f);
        TUT_ENSURE_EQ("Default rectf centery", rect4.getCenterY(), 3.5f);
    }

    TUT_CASE("llrect_test::object_test_5")
    {
        using namespace tut;

        LLSD array;
        array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
        LLRectf rect5(array);
        TUT_ENSURE_EQ("LLSD rectf left", rect5.mLeft, -1.0f);
        TUT_ENSURE_EQ("LLSD rectf top", rect5.mTop, 0.0f);
        TUT_ENSURE_EQ("LLSD rectf right", rect5.mRight, 0.0f);
        TUT_ENSURE_EQ("LLSD rectf bottom", rect5.mBottom, -1.0f);
        TUT_ENSURE_EQ("LLSD rectf width", rect5.getWidth(), 1.0f);
        TUT_ENSURE_EQ("LLSD rectf height", rect5.getHeight(), 1.0f);
        TUT_ENSURE_EQ("LLSD rectf centerx", rect5.getCenterX(), -0.5f);
        TUT_ENSURE_EQ("LLSD rectf centery", rect5.getCenterY(), -0.5f);
    }

    TUT_CASE("llrect_test::object_test_6")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.mLeft = -1.0f;
        rectf.mTop = 1.0f;
        rectf.mRight = 1.0f;
        rectf.mBottom = -1.0f;
        TUT_ENSURE_EQ("Member-set rectf left", rectf.mLeft, -1.0f);
        TUT_ENSURE_EQ("Member-set rectf top", rectf.mTop, 1.0f);
        TUT_ENSURE_EQ("Member-set rectf right", rectf.mRight, 1.0f);
        TUT_ENSURE_EQ("Member-set rectf bottom", rectf.mBottom, -1.0f);
        TUT_ENSURE_EQ("Member-set rectf width", rectf.getWidth(), 2.0f);
        TUT_ENSURE_EQ("Member-set rectf height", rectf.getHeight(), 2.0f);
        TUT_ENSURE_EQ("Member-set rectf centerx", rectf.getCenterX(), 0.0f);
        TUT_ENSURE_EQ("Member-set rectf centery", rectf.getCenterY(), 0.0f);
    }

    TUT_CASE("llrect_test::object_test_7")
    {
        using namespace tut;

        LLRectf rectf;

        LLSD array;
        array.append(-1.0f); array.append(0.0f); array.append(0.0f); array.append(-1.0f);
        rectf.setValue(array);
        TUT_ENSURE_EQ("setValue() rectf left", rectf.mLeft, -1.0f);
        TUT_ENSURE_EQ("setValue() rectf top", rectf.mTop, 0.0f);
        TUT_ENSURE_EQ("setValue() rectf right", rectf.mRight, 0.0f);
        TUT_ENSURE_EQ("setValue() rectf bottom", rectf.mBottom, -1.0f);
        TUT_ENSURE_EQ("setValue() rectf width", rectf.getWidth(), 1.0f);
        TUT_ENSURE_EQ("setValue() rectf height", rectf.getHeight(), 1.0f);
        TUT_ENSURE_EQ("setValue() rectf centerx", rectf.getCenterX(), -0.5f);
        TUT_ENSURE_EQ("setValue() rectf centery", rectf.getCenterY(), -0.5f);
    }

    TUT_CASE("llrect_test::object_test_8")
    {
        using namespace tut;

        LLRect rect;

        rect.set(10, 90, 70, 10);
        TUT_ENSURE_EQ("set() rectf left", rect.mLeft, 10);
        TUT_ENSURE_EQ("set() rectf top", rect.mTop, 90);
        TUT_ENSURE_EQ("set() rectf right", rect.mRight, 70);
        TUT_ENSURE_EQ("set() rectf bottom", rect.mBottom, 10);
        TUT_ENSURE_EQ("set() rectf width", rect.getWidth(), 60);
        TUT_ENSURE_EQ("set() rectf height", rect.getHeight(), 80);
        TUT_ENSURE_EQ("set() rectf centerx", rect.getCenterX(), 40);
        TUT_ENSURE_EQ("set() rectf centery", rect.getCenterY(), 50);
    }

    TUT_CASE("llrect_test::object_test_9")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.setOriginAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf left", rectf.mLeft, 0.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf top", rectf.mTop, 1.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf right", rectf.mRight, 2.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf bottom", rectf.mBottom, 0.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf width", rectf.getWidth(), 2.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf height", rectf.getHeight(), 1.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
        TUT_ENSURE_EQ("setOriginAndSize() rectf centery", rectf.getCenterY(), 0.5f);
    }

    TUT_CASE("llrect_test::object_test_10")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.setLeftTopAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf left", rectf.mLeft, 0.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf top", rectf.mTop, 0.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf right", rectf.mRight, 2.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf bottom", rectf.mBottom, -1.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf width", rectf.getWidth(), 2.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf height", rectf.getHeight(), 1.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf centerx", rectf.getCenterX(), 1.0f);
        TUT_ENSURE_EQ("setLeftTopAndSize() rectf centery", rectf.getCenterY(), -0.5f);
    }

    TUT_CASE("llrect_test::object_test_11")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.setCenterAndSize(0.0f, 0.0f, 2.0f, 1.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf left", rectf.mLeft, -1.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf top", rectf.mTop, 0.5f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf right", rectf.mRight, 1.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf bottom", rectf.mBottom, -0.5f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf width", rectf.getWidth(), 2.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf height", rectf.getHeight(), 1.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf centerx", rectf.getCenterX(), 0.0f);
        TUT_ENSURE_EQ("setCenterAndSize() rectf centery", rectf.getCenterY(), 0.0f);
    }

    TUT_CASE("llrect_test::object_test_12")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
        TUT_ENSURE("BBox is valid", rectf.isValid());

        rectf.mLeft = 2.0f;
        TUT_ENSURE("BBox is not valid", ! rectf.isValid());

        rectf.makeValid();
        TUT_ENSURE("BBox forced valid", rectf.isValid());

        rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
        TUT_ENSURE("BBox(0,0,0,0) is valid", rectf.isValid());
    }

    TUT_CASE("llrect_test::object_test_13")
    {
        using namespace tut;

        LLRectf rectf;

        rectf.set(-1.0f, 1.0f, 1.0f, -1.0f);
        TUT_ENSURE("BBox is not Null", ! rectf.isEmpty());
        TUT_ENSURE("BBox notNull", rectf.notEmpty());

        rectf.mLeft = 2.0f;
        rectf.makeValid();
        TUT_ENSURE("BBox is now Null", rectf.isEmpty());

        rectf.set(-1.0f, -1.0f, -1.0f, -1.0f);
        TUT_ENSURE("BBox(0,0,0,0) is Null", rectf.isEmpty());
    }

    TUT_CASE("llrect_test::object_test_14")
    {
        using namespace tut;

        LLRectf rect1, rect2;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 0.9f, 1.0f, -1.0f);

        TUT_ENSURE("rect1 == rect2 (false)", ! (rect1 == rect2));
        TUT_ENSURE("rect1 != rect2 (true)", rect1 != rect2);

        TUT_ENSURE("rect1 == rect1 (true)", rect1 == rect1);
        TUT_ENSURE("rect1 != rect1 (false)", ! (rect1 != rect1));
    }

    TUT_CASE("llrect_test::object_test_15")
    {
        using namespace tut;

        LLRectf rect1, rect2(rect1);

        TUT_ENSURE("rect1 == rect2 (true)", rect1 == rect2);
        TUT_ENSURE("rect1 != rect2 (false)", ! (rect1 != rect2));
    }

    TUT_CASE("llrect_test::object_test_16")
    {
        using namespace tut;

        LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
        LLRectf rect2(rect1);

        rect1.translate(0.0f, 0.0f);

        TUT_ENSURE("translate(0, 0)", rect1 == rect2);

        rect1.translate(100.0f, 100.0f);
        rect1.translate(-100.0f, -100.0f);

        TUT_ENSURE("translate(100, 100) + translate(-100, -100)", rect1 == rect2);

        rect1.translate(10.0f, 0.0f);
        rect2.set(9.0f, 1.0f, 11.0f, -1.0f);
        TUT_ENSURE("translate(10, 0)", rect1 == rect2);

        rect1.translate(0.0f, 10.0f);
        rect2.set(9.0f, 11.0f, 11.0f, 9.0f);
        TUT_ENSURE("translate(0, 10)", rect1 == rect2);

        rect1.translate(-10.0f, -10.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        TUT_ENSURE("translate(-10, -10)", rect1 == rect2);
    }

    TUT_CASE("llrect_test::object_test_17")
    {
        using namespace tut;

        LLRectf rect1(-1.0f, 1.0f, 1.0f, -1.0f);
        LLRectf rect2(rect1);

        rect1.stretch(0.0f);
        TUT_ENSURE("stretch(0)", rect1 == rect2);

        rect1.stretch(0.0f, 0.0f);
        TUT_ENSURE("stretch(0, 0)", rect1 == rect2);

        rect1.stretch(10.0f);
        rect1.stretch(-10.0f);
        TUT_ENSURE("stretch(10) + stretch(-10)", rect1 == rect2);

        rect1.stretch(2.0f, 1.0f);
        rect2.set(-3.0f, 2.0f, 3.0f, -2.0f);
        TUT_ENSURE("stretch(2, 1)", rect1 == rect2);
    }

    TUT_CASE("llrect_test::object_test_18")
    {
        using namespace tut;

        LLRectf rect1, rect2, rect3;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        TUT_ENSURE_EQ("union with self", rect3, rect1);

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        TUT_ENSURE_EQ("union - overlap", rect3, LLRectf(-2.0f, 2.0f, 1.0f, -1.0f));

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
        rect3 = rect1;
        rect3.unionWith(rect2);
        TUT_ENSURE_EQ("union - no overlap", rect3, LLRectf(-1.0f, 10.0f, 10.0f, -1.0f));
    }

    TUT_CASE("llrect_test::object_test_19")
    {
        using namespace tut;

        LLRectf rect1, rect2, rect3;

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        TUT_ENSURE_EQ("intersect with self", rect3, rect1);

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(-2.0f, 2.0f, 0.0f, 0.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        TUT_ENSURE_EQ("intersect - overlap", rect3, LLRectf(-1.0f, 1.0f, 0.0f, 0.0f));

        rect1.set(-1.0f, 1.0f, 1.0f, -1.0f);
        rect2.set(5.0f, 10.0f, 10.0f, 5.0f);
        rect3 = rect1;
        rect3.intersectWith(rect2);
        TUT_ENSURE("intersect - no overlap", rect3.isEmpty());
    }

    TUT_CASE("llrect_test::object_test_20")
    {
        using namespace tut;

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);

        TUT_ENSURE("(0,0) not in rect", rect.pointInRect(0.0f, 0.0f) == false);
        TUT_ENSURE("(2,2) in rect", rect.pointInRect(2.0f, 2.0f) == true);
        TUT_ENSURE("(1,1) in rect", rect.pointInRect(1.0f, 1.0f) == true);
        TUT_ENSURE("(3,3) not in rect", rect.pointInRect(3.0f, 3.0f) == false);
        TUT_ENSURE("(2.999,2.999) in rect", rect.pointInRect(2.999f, 2.999f) == true);
        TUT_ENSURE("(2.999,3.0) not in rect", rect.pointInRect(2.999f, 3.0f) == false);
        TUT_ENSURE("(3.0,2.999) not in rect", rect.pointInRect(3.0f, 2.999f) == false);
    }

    TUT_CASE("llrect_test::object_test_21")
    {
        using namespace tut;

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);

        TUT_ENSURE("(0,0) in local rect", rect.localPointInRect(0.0f, 0.0f) == true);
        TUT_ENSURE("(-0.0001,-0.0001) not in local rect", rect.localPointInRect(-0.0001f, -0.001f) == false);
        TUT_ENSURE("(1,1) in local rect", rect.localPointInRect(1.0f, 1.0f) == true);
        TUT_ENSURE("(2,2) not in local rect", rect.localPointInRect(2.0f, 2.0f) == false);
        TUT_ENSURE("(1.999,1.999) in local rect", rect.localPointInRect(1.999f, 1.999f) == true);
        TUT_ENSURE("(1.999,2.0) not in local rect", rect.localPointInRect(1.999f, 2.0f) == false);
        TUT_ENSURE("(2.0,1.999) not in local rect", rect.localPointInRect(2.0f, 1.999f) == false);
    }

    TUT_CASE("llrect_test::object_test_22")
    {
        using namespace tut;

        LLRectf rect(1.0f, 3.0f, 3.0f, 1.0f);
        F32 x, y;

        x = 2.0f; y = 2.0f;
        rect.clampPointToRect(x, y);
        TUT_ENSURE_EQ("clamp x-coord within rect", x, 2.0f);
        TUT_ENSURE_EQ("clamp y-coord within rect", y, 2.0f);

        x = -100.0f; y = 100.0f;
        rect.clampPointToRect(x, y);
        TUT_ENSURE_EQ("clamp x-coord outside rect", x, 1.0f);
        TUT_ENSURE_EQ("clamp y-coord outside rect", y, 3.0f);

        x = 3.0f; y = 1.0f;
        rect.clampPointToRect(x, y);
        TUT_ENSURE_EQ("clamp x-coord edge rect", x, 3.0f);
        TUT_ENSURE_EQ("clamp y-coord edge rect", y, 1.0f);
    }
}
