/**
 * @file   llbboxlocal_test.cpp
 * @author Martin Reddy
 * @date   2009-06-25
 * @brief  Test for llbboxlocal.cpp.
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
#include "../test/lltut.h"
#include "../llbboxlocal.h"

namespace tut
{
    struct LLBBoxLocalData
    {
    };

    typedef test_group<LLBBoxLocalData> factory;
    typedef factory::object object;
}

namespace
{
    tut::factory llbboxlocal_test_factory("LLBBoxLocal");
}

namespace tut
{
    template<> template<>
    void object::test<1>()
    {   
        //
        // test the default constructor
        //
        
        LLBBoxLocal bbox1;
        
        ensure_equals("Default bbox min", bbox1.getMin(), LLVector3(0.0f, 0.0f, 0.0f));
        ensure_equals("Default bbox max", bbox1.getMax(), LLVector3(0.0f, 0.0f, 0.0f));
    }

    template<> template<>
    void object::test<2>()
    {   
        //
        // test the non-default constructor
        //
        
        LLBBoxLocal bbox2(LLVector3(-1.0f, -2.0f, 0.0f), LLVector3(1.0f, 2.0f, 3.0f));
        
        ensure_equals("Custom bbox min", bbox2.getMin(), LLVector3(-1.0f, -2.0f, 0.0f));
        ensure_equals("Custom bbox max", bbox2.getMax(), LLVector3(1.0f, 2.0f, 3.0f));      
    }
    
    template<> template<>
    void object::test<3>()
    {   
        //
        // test the setMin()
        //
        // N.B. no validation is currently performed to ensure that the min
        // and max vectors are actually the min/max values.
        //
        
        LLBBoxLocal bbox2;
        bbox2.setMin(LLVector3(1.0f, 2.0f, 3.0f));
        
        ensure_equals("Custom bbox min (2)", bbox2.getMin(), LLVector3(1.0f, 2.0f, 3.0f));
    }
    
    template<> template<>
    void object::test<4>()
    {   
        //
        // test the setMax()
        //
        // N.B. no validation is currently performed to ensure that the min
        // and max vectors are actually the min/max values.
        //
        
        LLBBoxLocal bbox2;
        bbox2.setMax(LLVector3(10.0f, 20.0f, 30.0f));
        
        ensure_equals("Custom bbox max (2)", bbox2.getMax(), LLVector3(10.0f, 20.0f, 30.0f));
    }
    
    template<> template<>
    void object::test<5>()
    {   
        //
        // test the getCenter() method
        //
        
        ensure_equals("Default bbox center", LLBBoxLocal().getCenter(), LLVector3(0.0f, 0.0f, 0.0f));
        
        LLBBoxLocal bbox1(LLVector3(-1.0f, -1.0f, -1.0f), LLVector3(0.0f, 0.0f, 0.0f));
        
        ensure_equals("Custom bbox center", bbox1.getCenter(), LLVector3(-0.5f, -0.5f, -0.5f));
        
        LLBBoxLocal bbox2(LLVector3(0.0f, 0.0f, 0.0f), LLVector3(-1.0f, -1.0f, -1.0f));
        
        ensure_equals("Invalid bbox center", bbox2.getCenter(), LLVector3(-0.5f, -0.5f, -0.5f));
    }

    template<> template<>
    void object::test<6>()
    {   
        //
        // test the getExtent() method
        //
        
        LLBBoxLocal bbox2(LLVector3(0.0f, 0.0f, 0.0f), LLVector3(-1.0f, -1.0f, -1.0f));
        
        ensure_equals("Default bbox extent", LLBBoxLocal().getExtent(), LLVector3(0.0f, 0.0f, 0.0f));
        
        LLBBoxLocal bbox3(LLVector3(-1.0f, -1.0f, -1.0f), LLVector3(1.0f, 2.0f, 0.0f));
        
        ensure_equals("Custom bbox extent", bbox3.getExtent(), LLVector3(2.0f, 3.0f, 1.0f));
    }
    
    template<> template<>
    void object::test<7>()
    {
        //
        // test the addPoint() method
        //
        // N.B. if you create an empty bbox and then add points,
        // the vector (0, 0, 0) will always be part of the bbox.
        // (Fixing this would require adding a bool to the class size).
        //
        
        LLBBoxLocal bbox1;
        bbox1.addPoint(LLVector3(-1.0f, -2.0f, -3.0f));
        bbox1.addPoint(LLVector3(3.0f, 4.0f, 5.0f));
        
        ensure_equals("Custom BBox center (1)", bbox1.getCenter(), LLVector3(1.0f, 1.0f, 1.0f));
        ensure_equals("Custom BBox min (1)", bbox1.getMin(), LLVector3(-1.0f, -2.0f, -3.0f));
        ensure_equals("Custom BBox max (1)", bbox1.getMax(), LLVector3(3.0f, 4.0f, 5.0f));
        
        bbox1.addPoint(LLVector3(0.0f, 0.0f, 0.0f));
        bbox1.addPoint(LLVector3(1.0f, 2.0f, 3.0f));
        bbox1.addPoint(LLVector3(2.0f, 2.0f, 2.0f));
        
        ensure_equals("Custom BBox center (2)", bbox1.getCenter(), LLVector3(1.0f, 1.0f, 1.0f));
        ensure_equals("Custom BBox min (2)", bbox1.getMin(), LLVector3(-1.0f, -2.0f, -3.0f));
        ensure_equals("Custom BBox max (2)", bbox1.getMax(), LLVector3(3.0f, 4.0f, 5.0f));
        
        bbox1.addPoint(LLVector3(5.0f, 5.0f, 5.0f));
        
        ensure_equals("Custom BBox center (3)", bbox1.getCenter(), LLVector3(2.0f, 1.5f, 1.0f));
        ensure_equals("Custom BBox min (3)", bbox1.getMin(), LLVector3(-1.0f, -2.0f, -3.0f));
        ensure_equals("Custom BBox max (3)", bbox1.getMax(), LLVector3(5.0f, 5.0f, 5.0f));
    }
        
    template<> template<>
    void object::test<8>()
    {
        //
        // test the addBBox() methods
        //
        // N.B. if you create an empty bbox and then add points,
        // the vector (0, 0, 0) will always be part of the bbox.
        // (Fixing this would require adding a bool to the class size).
        //
        
        LLBBoxLocal bbox2(LLVector3(1.0f, 1.0f, 1.0f), LLVector3(2.0f, 2.0f, 2.0f));
        bbox2.addBBox(LLBBoxLocal(LLVector3(1.5f, 1.5f, 1.5f), LLVector3(3.0f, 3.0f, 3.0f)));
        
        ensure_equals("Custom BBox center (4)", bbox2.getCenter(), LLVector3(2.0f, 2.0f, 2.0f));
        ensure_equals("Custom BBox min (4)", bbox2.getMin(), LLVector3(1.0f, 1.0f, 1.0f));
        ensure_equals("Custom BBox max (4)", bbox2.getMax(), LLVector3(3.0f, 3.0f, 3.0f));
        
        bbox2.addBBox(LLBBoxLocal(LLVector3(-1.0f, -1.0f, -1.0f), LLVector3(0.0f, 0.0f, 0.0f)));
        
        ensure_equals("Custom BBox center (5)", bbox2.getCenter(), LLVector3(1.0f, 1.0f, 1.0f));
        ensure_equals("Custom BBox min (5)", bbox2.getMin(), LLVector3(-1.0f, -1.0f, -1.0f));
        ensure_equals("Custom BBox max (5)", bbox2.getMax(), LLVector3(3.0f, 3.0f, 3.0f));
    }
    
    template<> template<>
    void object::test<9>()
    {
        //
        // test the expand() method
        //
        
        LLBBoxLocal bbox1;
        bbox1.expand(0.0f);

        ensure_equals("Zero-expanded Default BBox center", bbox1.getCenter(), LLVector3(0.0f, 0.0f, 0.0f));

        LLBBoxLocal bbox2(LLVector3(1.0f, 2.0f, 3.0f), LLVector3(3.0f, 4.0f, 5.0f));
        bbox2.expand(0.0f);
        
        ensure_equals("Zero-expanded BBox center", bbox2.getCenter(), LLVector3(2.0f, 3.0f, 4.0f));
        ensure_equals("Zero-expanded BBox min", bbox2.getMin(), LLVector3(1.0f, 2.0f, 3.0f));
        ensure_equals("Zero-expanded BBox max", bbox2.getMax(), LLVector3(3.0f, 4.0f, 5.0f));

        bbox2.expand(0.5f);

        ensure_equals("Positive-expanded BBox center", bbox2.getCenter(), LLVector3(2.0f, 3.0f, 4.0f));
        ensure_equals("Positive-expanded BBox min", bbox2.getMin(), LLVector3(0.5f, 1.5f, 2.5f));
        ensure_equals("Positive-expanded BBox max", bbox2.getMax(), LLVector3(3.5f, 4.5f, 5.5f));

        bbox2.expand(-1.0f);
        
        ensure_equals("Negative-expanded BBox center", bbox2.getCenter(), LLVector3(2.0f, 3.0f, 4.0f));
        ensure_equals("Negative-expanded BBox min", bbox2.getMin(), LLVector3(1.5f, 2.5f, 3.5f));
        ensure_equals("Negative-expanded BBox max", bbox2.getMax(), LLVector3(2.5f, 3.5f, 4.5f));       
    }
}
