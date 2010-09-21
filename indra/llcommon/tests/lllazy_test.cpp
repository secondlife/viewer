/**
 * @file   lllazy_test.cpp
 * @author Nat Goodspeed
 * @date   2009-01-28
 * @brief  Tests of lllazy.h.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lllazy.h"
// STL headers
#include <iostream>
// std headers
// external library headers
#include <boost/lambda/construct.hpp>
#include <boost/lambda/bind.hpp>
// other Linden headers
#include "../test/lltut.h"

namespace bll = boost::lambda;

/*****************************************************************************
*   Test classes
*****************************************************************************/

// Let's say that because of its many external dependencies, YuckyFoo is very
// hard to instantiate in a test harness.
class YuckyFoo
{
public:
    virtual ~YuckyFoo() {}
    virtual std::string whoami() const { return "YuckyFoo"; }
};

// Let's further suppose that YuckyBar is another hard-to-instantiate class.
class YuckyBar
{
public:
    YuckyBar(const std::string& which):
        mWhich(which)
    {}
    virtual ~YuckyBar() {}

    virtual std::string identity() const { return std::string("YuckyBar(") + mWhich + ")"; }

private:
    const std::string mWhich;
};

// Pretend that this class would be tough to test because, up until we started
// trying to test it, it contained instances of both YuckyFoo and YuckyBar.
// Now we've refactored so it contains LLLazy<YuckyFoo> and LLLazy<YuckyBar>.
// More than that, it contains them by virtue of deriving from
// LLLazyBase<YuckyFoo> and LLLazyBase<YuckyBar>.
// We postulate two different LLLazyBases because, with only one, you need not
// specify *which* get()/set() method you're talking about. That's a simpler
// case.
class NeedsTesting: public LLLazyBase<YuckyFoo>, public LLLazyBase<YuckyBar>
{
public:
    NeedsTesting():
        // mYuckyBar("RealYuckyBar")
        LLLazyBase<YuckyBar>(bll::bind(bll::new_ptr<YuckyBar>(), "RealYuckyBar"))
    {}
    virtual ~NeedsTesting() {}

    virtual std::string describe() const
    {
        return std::string("NeedsTesting(") + getLazy<YuckyFoo>(this).whoami() + ", " +
            getLazy<YuckyBar>(this).identity() + ")";
    }

private:
    // These instance members were moved to LLLazyBases:
    // YuckyFoo mYuckyFoo;
    // YuckyBar mYuckyBar;
};

// Fake up a test YuckyFoo class
class TestFoo: public YuckyFoo
{
public:
    virtual std::string whoami() const { return "TestFoo"; }
};

// and a test YuckyBar
class TestBar: public YuckyBar
{
public:
    TestBar(const std::string& which): YuckyBar(which) {}
    virtual std::string identity() const
    {
        return std::string("TestBar(") + YuckyBar::identity() + ")";
    }
};

// So here's a test subclass of NeedsTesting that uses TestFoo and TestBar
// instead of YuckyFoo and YuckyBar.
class TestNeedsTesting: public NeedsTesting
{
public:
    TestNeedsTesting()
    {
        // Exercise setLazy(T*)
        setLazy<YuckyFoo>(this, new TestFoo());
        // Exercise setLazy(Factory)
        setLazy<YuckyBar>(this, bll::bind(bll::new_ptr<TestBar>(), "TestYuckyBar"));
    }

    virtual std::string describe() const
    {
        return std::string("TestNeedsTesting(") + NeedsTesting::describe() + ")";
    }

    void toolate()
    {
        setLazy<YuckyFoo>(this, new TestFoo());
    }
};

// This class tests having an explicit LLLazy<T> instance as a named member,
// rather than deriving from LLLazyBase<T>.
class LazyMember
{
public:
    YuckyFoo& getYuckyFoo() { return *mYuckyFoo; }
    std::string whoisit() const { return mYuckyFoo->whoami(); }

protected:
    LLLazy<YuckyFoo> mYuckyFoo;
};

// This is a test subclass of the above, dynamically replacing the
// LLLazy<YuckyFoo> member.
class TestLazyMember: public LazyMember
{
public:
    // use factory setter
    TestLazyMember()
    {
        mYuckyFoo.set(bll::new_ptr<TestFoo>());
    }

    // use instance setter
    TestLazyMember(YuckyFoo* instance)
    {
        mYuckyFoo.set(instance);
    }
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lllazy_data
    {
    };
    typedef test_group<lllazy_data> lllazy_group;
    typedef lllazy_group::object lllazy_object;
    lllazy_group lllazygrp("lllazy");

    template<> template<>
    void lllazy_object::test<1>()
    {
        // Instantiate an official one, just because we can
        NeedsTesting nt;
        // and a test one
        TestNeedsTesting tnt;
//      std::cout << nt.describe() << '\n';
        ensure_equals(nt.describe(), "NeedsTesting(YuckyFoo, YuckyBar(RealYuckyBar))");
//      std::cout << tnt.describe() << '\n';
        ensure_equals(tnt.describe(),
                      "TestNeedsTesting(NeedsTesting(TestFoo, TestBar(YuckyBar(TestYuckyBar))))");
    }

    template<> template<>
    void lllazy_object::test<2>()
    {
        TestNeedsTesting tnt;
        std::string threw;
        try
        {
            tnt.toolate();
        }
        catch (const LLLazyCommon::InstanceChange& e)
        {
            threw = e.what();
        }
        ensure_contains("InstanceChange exception", threw, "replace LLLazy instance");
    }

    template<> template<>
    void lllazy_object::test<3>()
    {
        {
            LazyMember lm;
            // operator*() on-demand instantiation
            ensure_equals(lm.getYuckyFoo().whoami(), "YuckyFoo");
        }
        {
            LazyMember lm;
            // operator->() on-demand instantiation
            ensure_equals(lm.whoisit(), "YuckyFoo");
        }
    }

    template<> template<>
    void lllazy_object::test<4>()
    {
        {
            // factory setter
            TestLazyMember tlm;
            ensure_equals(tlm.whoisit(), "TestFoo");
        }
        {
            // instance setter
            TestLazyMember tlm(new TestFoo());
            ensure_equals(tlm.whoisit(), "TestFoo");
        }
    }
} // namespace tut
