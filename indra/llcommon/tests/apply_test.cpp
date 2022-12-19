/**
 * @file   apply_test.cpp
 * @author Nat Goodspeed
 * @date   2022-12-19
 * @brief  Test for apply.
 * 
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "apply.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    namespace statics
    {
        /*------------------------------ data ------------------------------*/
        // Although we're using types from the LLSD namespace, we're not
        // constructing LLSD values, but rather instances of the C++ types
        // supported by LLSD.
        static LLSD::Boolean b{true};
        static LLSD::Integer i{17};
        static LLSD::Real    f{3.14};
        static LLSD::String  s{ "hello" };
        static LLSD::UUID    uu{ "baadf00d-dead-beef-baad-feedb0ef" };
        static LLSD::Date    dt{ "2022-12-19" };
        static LLSD::URI     uri{ "http://secondlife.com" };
        static LLSD::Binary  bin{ 0x01, 0x02, 0x03, 0x04, 0x05 };

        static std::vector<LLSD::String> quick
        {
            "The", "quick", "brown", "fox", "etc."
        };

        static std::array<int, 5> fibs
        {
            0, 1, 1, 2, 3
        };

        // ensure that apply() actually reaches the target method --
        // lack of ensure_equals() failure could be due to no-op apply()
        bool called{ false };

        /*------------------------- test functions -------------------------*/
        void various(LLSD::Boolean b, LLSD::Integer i, LLSD::Real f, const LLSD::String& s,
                     const LLSD::UUID& uu, const LLSD::Date& dt,
                     const LLSD::URI& uri, const LLSD::Binary& bin)
        {
            called = true;
            ensure_equals(  "b mismatch", b,   statics::b);
            ensure_equals(  "i mismatch", i,   statics::i);
            ensure_equals(  "f mismatch", f,   statics::f);
            ensure_equals(  "s mismatch", s,   statics::s);
            ensure_equals( "uu mismatch", uu,  statics::uu);
            ensure_equals( "dt mismatch", dt,  statics::dt);
            ensure_equals("uri mismatch", uri, statics::uri);
            ensure_equals("bin mismatch", bin, statics::bin);
        }

        void strings(std::string s0, std::string s1, std::string s2, std::string s3, std::string s4)
        {
            called = true;
            ensure_equals("s0 mismatch", s0, statics::quick[0]);
            ensure_equals("s1 mismatch", s1, statics::quick[1]);
            ensure_equals("s2 mismatch", s2, statics::quick[2]);
            ensure_equals("s3 mismatch", s3, statics::quick[3]);
            ensure_equals("s4 mismatch", s4, statics::quick[4]);
        }

        void ints(int i0, int i1, int i2, int i3, int i4)
        {
            called = true;
            ensure_equals("i0 mismatch", i0, statics::fibs[0]);
            ensure_equals("i1 mismatch", i1, statics::fibs[1]);
            ensure_equals("i2 mismatch", i2, statics::fibs[2]);
            ensure_equals("i3 mismatch", i3, statics::fibs[3]);
            ensure_equals("i4 mismatch", i4, statics::fibs[4]);
        }

        void intfunc(int i)
        {
            called = true;
            ensure_equals("i mismatch", i, statics::i);
        }

        void voidfunc()
        {
            called = true;
        }
    } // namespace statics

    struct apply_data
    {
        apply_data()
        {
            // reset called before each test
            statics::called = false;
        }
    };
    typedef test_group<apply_data> apply_group;
    typedef apply_group::object object;
    apply_group applygrp("apply");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("apply(tuple)");
        LL::apply(statics::various,
                  std::make_tuple(statics::b, statics::i, statics::f, statics::s,
                                  statics::uu, statics::dt, statics::uri, statics::bin));
        ensure("apply(tuple) failed", statics::called);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("apply(array)");
        LL::apply(statics::ints, statics::fibs);
        ensure("apply(array) failed", statics::called);
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("apply(vector)");
        LL::apply(statics::strings, statics::quick);
        ensure("apply(vector) failed", statics::called);
    }

    // The various apply(LLSD) tests exercise only the success cases because
    // the failure cases trigger assert() fail, which is hard to catch.
    template<> template<>
    void object::test<4>()
    {
        set_test_name("apply(LLSD())");
        LL::apply(statics::voidfunc, LLSD());
        ensure("apply(LLSD()) failed", statics::called);
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("apply(LLSD scalar)");
        LL::apply(statics::intfunc, LLSD(statics::i));
        ensure("apply(LLSD scalar) failed", statics::called);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("apply(LLSD array)");
        LL::apply(statics::various,
                  llsd::array(statics::b, statics::i, statics::f, statics::s,
                              statics::uu, statics::dt, statics::uri, statics::bin));
        ensure("apply(LLSD array) failed", statics::called);
    }
} // namespace tut
