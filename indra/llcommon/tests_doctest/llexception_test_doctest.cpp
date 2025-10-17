// ---------------------------------------------------------------------------
// Auto-generated from llexception_test.cpp at 2025-10-16T18:47:17Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "llexception.h"
#include <typeinfo>
#include <boost/throw_exception.hpp>

TUT_SUITE("llcommon")
{
    TUT_CASE("llexception_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert llexception_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("throwing exceptions");

        //         // For each kind of exception, try both kinds of throw against all
        //         // three catch sequences
        //         std::size_t margin = 72;
        //         std::cout << center("FromStd", '=', margin) << std::endl;
        //         catch_both_several<FromStd>("FromStd");

        //         std::cout << center("FromBoth", '=', margin) << std::endl;
        //         catch_both_several<FromBoth>("FromBoth");

        //         std::cout << center("FromBoost", '=', margin) << std::endl;
        //         // can't throw with BOOST_THROW_EXCEPTION(), just use catch_several()
        //         catch_several(plain_throw<FromBoost>, "plain_throw<FromBoost>");

        //         std::cout << center("FromNeither", '=', margin) << std::endl;
        //         // can't throw this with BOOST_THROW_EXCEPTION() either
        //         catch_several(plain_throw<FromNeither>, "plain_throw<FromNeither>");

        //         std::cout << center("const char*", '=', margin) << std::endl;
        //         // We don't expect BOOST_THROW_EXCEPTION() to throw anything so daft
        //         // as a const char* or an int, so don't bother with
        //         // catch_both_several() -- just catch_several().
        //         catch_several(throw_char_ptr, "throw_char_ptr");

        //         std::cout << center("int", '=', margin) << std::endl;
        //         catch_several(throw_int, "throw_int");
        //     }
    }

    TUT_CASE("llexception_test::object_test_2")
    {
        DOCTEST_FAIL("TODO: convert llexception_test.cpp::object::test<2> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<2>()
        //     {
        //         set_test_name("reporting exceptions");

        //         try
        //         {
        //             LLTHROW(LLException("badness"));
        //         }
        //         catch (...)
        //         {
        //             LOG_UNHANDLED_EXCEPTION("llexception test<2>()");
        //         }
        //     }
    }

}

