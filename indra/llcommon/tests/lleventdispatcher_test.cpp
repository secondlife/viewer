/**
 * @file   lleventdispatcher_test.cpp
 * @author Nat Goodspeed
 * @date   2011-01-20
 * @brief  Test for lleventdispatcher.
 * 
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lleventdispatcher.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llsd.h"
#include "llsdutil.h"
#include "stringize.h"
#include "tests/wrapllerrs.h"

// http://www.boost.org/doc/libs/1_45_0/libs/function_types/example/interpreter.hpp
// downloaded 2011-01-20 by NRG and adapted with example usage
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// This example implements a simple batch-style dispatcher that is capable of
// calling functions previously registered with it. The parameter types of the
// functions are used to control the parsing of the input.
//
// Implementation description
// ==========================
//
// When a function is registered, an 'invoker' template is instantiated with
// the function's type. The 'invoker' fetches a value from the 'arg_source'
// for each parameter of the function into a tuple and finally invokes the the
// function with these values as arguments. The invoker's entrypoint, which
// is a function of the callable builtin that describes the function to call and
// a reference to the 'arg_source', is partially bound to the registered
// function and put into a map so it can be found by name during parsing.

#include <map>
#include <string>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/range.hpp>

#include <boost/lambda/lambda.hpp>

#include <iostream>

using boost::lambda::constant;
using boost::lambda::constant_ref;
using boost::lambda::var;

/*****************************************************************************
*   Output control
*****************************************************************************/
#ifdef DEBUG_ON
using std::cout;
#else
static std::ostringstream cout;
#endif

/*****************************************************************************
*   Example data, functions, classes
*****************************************************************************/
// sensing globals
static std::string gs;
static float gf;
static int gi;
static LLSD gl;

void clear()
{
    gs.clear();
    gf = 0;
    gi = 0;
    gl = LLSD();
}

void abc(const std::string& message)
{
    cout << "abc('" << message << "')\n";
    gs = message;
}

void def(float value, std::string desc)
{
    cout << "def(" << value << ", '" << desc << "')\n";
    gf = value;
    gs = desc;
}

void ghi(const std::string& foo, int bar)
{
    cout << "ghi('" << foo << "', " << bar << ")\n";
    gs = foo;
    gi = bar;
}

void jkl(const char* message)
{
    cout << "jkl('" << message << "')\n";
    gs = message;
}

void somefunc(const LLSD& value)
{
    cout << "somefunc(" << value << ")\n";
    gl = value;
}

class Dummy
{
public:
    Dummy(): _id("Dummy") {}

    void mno(const std::string& message)
    {
        cout << _id << "::mno('" << message << "')\n";
        s = message;
    }

    void pqr(float value, std::string desc)
    {
        cout << _id << "::pqr(" << value << ", '" << desc << "')\n";
        f = value;
        s = desc;
    }

    void stu(const std::string& foo, int bar)
    {
        cout << _id << "::stu('" << foo << "', " << bar << ")\n";
        s = foo;
        i = bar;
    }

    void vwx(const char* message)
    {
        cout << _id << "::vwx('" << message << "')\n";
        s = message;
    }

    static void yz1(const std::string& message)
    {
        cout << "Dummy::yz1('" << message << "')\n";
        // can't access sensing members...
        gs = message;
    }

    // sensing members
    std::string s;
    float f;
    int i;

private:
    std::string _id;
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lleventdispatcher_data
    {
        WrapLL_ERRS redirect;
        LLEventDispatcher work;
        Dummy dummy;

        lleventdispatcher_data():
            work("test dispatcher", "op")
        {
            // This object is reconstructed for every test<n> method. But
            // clear global variables every time too.
            ::clear();

            work.add("abc", "abc", abc, LLSDArray("message"));
            work.add("def", "def", def);
            work.add("ghi", "ghi", ghi);
            work.add("jkl", "jkl", jkl);
            work.add("yz1", "yz1", &Dummy::yz1);
            work.add("mno", "mno", &Dummy::mno, var(dummy),
                     LLSDArray("message"), LLSDArray("default message"));
            work.add("mnoptr", "mno", &Dummy::mno, constant(&dummy));
            work.add("pqr", "pqr", &Dummy::pqr, var(dummy),
                     LLSDArray("value")("desc"));
            work.add("stu", "stu", &Dummy::stu, var(dummy),
                     LLSDArray("foo")("bar"), LLSDArray(-1));
            work.add("vwx", "vwx", &Dummy::vwx, var(dummy),
                     LLSDArray("message"));
        }

        void ensure_has(const std::string& outer, const std::string& inner)
        {
            ensure(STRINGIZE("'" << outer << "' does not contain '" << inner << "'").c_str(),
                   outer.find(inner) != std::string::npos);
        }

        void call_exc(const std::string& func, const LLSD& args, const std::string& exc_frag)
        {
            std::string threw;
            try
            {
                work(func, args);
            }
            catch (const std::runtime_error& e)
            {
                cout << "*** " << e.what() << '\n';
                threw = e.what();
            }
            ensure_has(threw, exc_frag);
        }
    };
    typedef test_group<lleventdispatcher_data> lleventdispatcher_group;
    typedef lleventdispatcher_group::object object;
    lleventdispatcher_group lleventdispatchergrp("lleventdispatcher");

    template<> template<>
    void object::test<1>()
    {
        LLSD hello("Hello test!");
//        cout << std::string(hello) << "\n";
        clear();
        jkl(LLSDParam<const char*>(hello));
        ensure_equals(gs, hello.asString());
    }

    template<> template<>
    void object::test<2>()
    {
        somefunc("abc");
        ensure_equals(gl.asString(), "abc");

        somefunc(17);
        ensure_equals(gl.asInteger(), 17);

        somefunc(3.14);
        // 7 bits is 128, just enough to express two decimal places.
        ensure_approximately_equals(gl.asReal(), 3.14, 7);

        somefunc(LLSD().with(0, "abc").with(1, 17).with(2, 3.14));
        ensure(gl.isArray());
        ensure_equals(gl.size(), 4); // !!! bug in LLSD::with(Integer, const LLSD&) !!!
        ensure_equals(gl[1].asInteger(), 17);

        somefunc(LLSD().with("alpha", "abc").with("random", 17).with("pi", 3.14));
        ensure(gl.isMap());
        ensure_equals(gl.size(), 3);
        ensure_equals(gl["random"].asInteger(), 17);

        somefunc(LLSDArray("abc")(17)(3.14));
        ensure(gl.isArray());
        ensure_equals(gl.size(), 3);
        ensure_equals(gl[0].asString(), "abc");

        somefunc(LLSDMap("alpha", "abc")("random", 17)("pi", 3.14));
        ensure(gl.isMap());
        ensure_equals(gl.size(), 3);
        ensure_equals(gl["alpha"].asString(), "abc");
    }

    template<> template<>
    void object::test<3>()
    {
        call_exc("gofish", LLSDArray(1), "not found");
    }

    template<> template<>
    void object::test<4>()
    {
        call_exc("abc", LLSD(), "missing required");
    }

    template<> template<>
    void object::test<5>()
    {
        work("abc", LLSDMap("message", "something"));
        ensure_equals(gs, "something");
    }

    template<> template<>
    void object::test<6>()
    {
        work("abc", LLSDMap("message", "something")("plus", "more"));
        ensure_equals(gs, "something");
    }

    template<> template<>
    void object::test<7>()
    {
        call_exc("def", LLSDMap("value", 20)("desc", "questions"), "needs an args array");
    }

    template<> template<>
    void object::test<8>()
    {
        work("def", LLSDArray(20)("questions"));
        ensure_equals(gf, 20);
        ensure_equals(gs, "questions");
    }

    template<> template<>
    void object::test<9>()
    {
        work("def", LLSDArray(3.14)("pies"));
        ensure_approximately_equals(gf, 3.14, 7);
        ensure_equals(gs, "pies");
    }

    template<> template<>
    void object::test<10>()
    {
        work("ghi", LLSDArray("answer")(17));
        ensure_equals(gs, "answer");
        ensure_equals(gi, 17);
    }

    template<> template<>
    void object::test<11>()
    {
        work("ghi", LLSDArray("answer")(3.14));
        ensure_equals(gs, "answer");
        ensure_equals(gi, 3);
    }

    template<> template<>
    void object::test<12>()
    {
        work("jkl", LLSDArray("sample message"));
        ensure_equals(gs, "sample message");
    }

    template<> template<>
    void object::test<13>()
    {
        work("yz1", LLSDArray("w00t"));
        ensure_equals(gs, "w00t");
    }

    template<> template<>
    void object::test<14>()
    {
        std::string msg("nonstatic member function");
        work("mno", LLSDMap("message", msg));
        ensure_equals(dummy.s, msg);
    }

    template<> template<>
    void object::test<15>()
    {
        std::string msg("nonstatic member function reached by ptr");
        work("mnoptr", LLSDArray(msg));
        ensure_equals(dummy.s, msg);
    }

    template<> template<>
    void object::test<16>()
    {
        work("mno", LLSD());
        ensure_equals(dummy.s, "default message");
    }

    template<> template<>
    void object::test<17>()
    {
        work("pqr", LLSDMap("value", 3.14)("desc", "pies"));
        ensure_approximately_equals(dummy.f, 3.14, 7);
        ensure_equals(dummy.s, "pies");
    }

    template<> template<>
    void object::test<18>()
    {
        call_exc("pqr", LLSD(), "missing required");
    }

    template<> template<>
    void object::test<19>()
    {
        call_exc("pqr", LLSDMap("value", 3.14), "missing required");
    }

    template<> template<>
    void object::test<20>()
    {
        call_exc("pqr", LLSDMap("desc", "pies"), "missing required");
    }

    template<> template<>
    void object::test<21>()
    {
        work("stu", LLSDMap("bar", 3.14)("foo", "pies"));
        ensure_equals(dummy.s, "pies");
        ensure_equals(dummy.i, 3);
    }

    template<> template<>
    void object::test<22>()
    {
        call_exc("stu", LLSD(), "missing required");
    }

    template<> template<>
    void object::test<23>()
    {
        call_exc("stu", LLSDMap("bar", 3.14), "missing required");
    }

    template<> template<>
    void object::test<24>()
    {
        work("stu", LLSDMap("foo", "pies"));
        ensure_equals(dummy.s, "pies");
        ensure_equals(dummy.i, -1);
    }

    template<> template<>
    void object::test<25>()
    {
        std::string msg("nonstatic method(const char*)");
        work("vwx", LLSDMap("message", msg));
        ensure_equals(dummy.s, msg);
    }
} // namespace tut
