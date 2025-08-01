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
#include <iomanip>
// external library headers
// other Linden headers
#include "llsd.h"
#include "llsdutil.h"
#include <array>
#include <string>
#include <vector>

// for ensure_equals
std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& stringvec)
{
    const char* delim = "[";
    for (const auto& str : stringvec)
    {
        out << delim << std::quoted(str);
        delim = ", ";
    }
    return out << ']';
}

// the above must be declared BEFORE ensure_equals(std::vector<std::string>)
#include "../test/lldoctest.h"

/*****************************************************************************
*   TUT
*****************************************************************************/
TEST_SUITE("UnknownSuite") {

struct apply_data
{

        apply_data()
        {
            // reset called before each test
            statics::called = false;
            statics::collected.clear();
        
};

TEST_CASE_FIXTURE(apply_data, "test_1")
{

        set_test_name("apply(tuple)");
        LL::apply(statics::various,
                  std::make_tuple(statics::b, statics::i, statics::f, statics::s,
                                  statics::uu, statics::dt, statics::uri, statics::bin));
        CHECK_MESSAGE(statics::called, "apply(tuple) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_2")
{

        set_test_name("apply(array)");
        LL::apply(statics::ints, statics::fibs);
        CHECK_MESSAGE(statics::called, "apply(array) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_3")
{

        set_test_name("apply(vector)");
        LL::apply(statics::strings, statics::quick);
        CHECK_MESSAGE(statics::called, "apply(vector) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_4")
{

        set_test_name("apply(LLSD())");
        LL::apply(statics::voidfunc, LLSD());
        CHECK_MESSAGE(statics::called, "apply(LLSD()) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_5")
{

        set_test_name("apply(fn(int), LLSD scalar)");
        LL::apply(statics::intfunc, LLSD(statics::i));
        CHECK_MESSAGE(statics::called, "apply(fn(int), LLSD scalar) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_6")
{

        set_test_name("apply(fn(LLSD), LLSD scalar)");
        // This test verifies that LLSDParam<LLSD> doesn't send the compiler
        // into infinite recursion when the target is itself LLSD.
        LL::apply(statics::sdfunc, LLSD(statics::i));
        CHECK_MESSAGE(statics::called, "apply(fn(LLSD), LLSD scalar) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_7")
{

        set_test_name("apply(LLSD array)");
        LL::apply(statics::various,
                  llsd::array(statics::b, statics::i, statics::f, statics::s,
                              statics::uu, statics::dt, statics::uri, statics::bin));
        CHECK_MESSAGE(statics::called, "apply(LLSD array) failed");
    
}

TEST_CASE_FIXTURE(apply_data, "test_8")
{

        set_test_name("VAPPLY()");
        // Make a std::array<std::string> from statics::quick. We can't call a
        // variadic function with a data structure of dynamic length.
        std::array<std::string, 5> strray;
        for (size_t i = 0; i < strray.size(); ++i)
            strray[i] = statics::quick[i];
        // This doesn't work: the compiler doesn't know which overload of
        // collect() to pass to LL::apply().
        // LL::apply(statics::collect, strray);
        // That's what VAPPLY() is for.
        VAPPLY(statics::collect, strray);
        CHECK_MESSAGE(statics::called, "VAPPLY() failed");
        CHECK_MESSAGE(statics::collected == statics::quick, "collected mismatch");
    
}

} // TEST_SUITE
