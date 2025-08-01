/**
 * @file   lazyeventapi_test.cpp
 * @author Nat Goodspeed
 * @date   2022-06-18
 * @brief  Test for lazyeventapi.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lazyeventapi.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"
#include "llevents.h"
#include "llsdutil.h"

// observable side effect, solely for testing
static LLSD data;

// LLEventAPI listener subclass
class MyListener: public LLEventAPI
{
public:
    // need this trivial forwarding constructor
    // (of course do any other initialization your subclass requires)
    MyListener(const LL::LazyEventAPIParams& params):
        LLEventAPI(params)
    {}

    // example operation, registered by LazyEventAPI subclass below
    void set_data(const LLSD& event)
    {
        data = event["data"];
    }
};

// LazyEventAPI registrar subclass
class MyRegistrar: public LL::LazyEventAPI<MyListener>
{
    using super = LL::LazyEventAPI<MyListener>;
    using super::listener;
public:
    // LazyEventAPI subclass initializes like a classic LLEventAPI subclass
    // constructor, with API name and desc plus add() calls for the defined
    // operations
    MyRegistrar():
        super("Test", "This is a test LLEventAPI")
    {
        add("set", "This is a set operation", &listener::set_data);
    }
};
// Normally we'd declare a static instance of MyRegistrar -- but because we
// want to test both with and without, defer declaration to individual test
// methods.

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lazyeventapi_data
    {
        lazyeventapi_data()
        {
            // before every test, reset 'data'
            data.clear();
        }
        ~lazyeventapi_data()
        {
            // after every test, reset LLEventPumps
            LLEventPumps::deleteSingleton();
        }
    };
    typedef test_group<lazyeventapi_data> lazyeventapi_group;
    typedef lazyeventapi_group::object object;
    lazyeventapi_group lazyeventapigrp("lazyeventapi");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("LazyEventAPI");
        // this is where the magic (should) happen
        // 'register' still a keyword until C++17
        MyRegistrar regster;
        LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "hey"));
        ensure_equals("failed to set data", data.asString(), "hey");
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("No LazyEventAPI");
        // Because the MyRegistrar declaration in test<1>() is local, because
        // it has been destroyed, we fully expect NOT to reach a MyListener
        // instance with this post.
        LLEventPumps::instance().obtain("Test").post(llsd::map("op", "set", "data", "moot"));
        ensure("accidentally set data", ! data.isDefined());
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("LazyEventAPI metadata");
        MyRegistrar regster;
        // Of course we have 'regster' in hand; we don't need to search for
        // it. But this next test verifies that we can find (all) LazyEventAPI
        // instances using LazyEventAPIBase::instance_snapshot. Normally we
        // wouldn't search; normally we'd just look at each instance in the
        // loop body.
        const MyRegistrar* found = nullptr;
        for (const auto& registrar : LL::LazyEventAPIBase::instance_snapshot())
            if ((found = dynamic_cast<const MyRegistrar*>(&registrar)))
                break;
        ensure("Failed to find MyRegistrar via LLInstanceTracker", found);

        ensure_equals("wrong API name", found->getName(), "Test");
        ensure_contains("wrong API desc", found->getDesc(), "test LLEventAPI");
        ensure_equals("wrong API field", found->getDispatchKey(), "op");
        // Normally we'd just iterate over *found. But for test purposes,
        // actually capture the range of NameDesc pairs in a vector.
        std::vector<LL::LazyEventAPIBase::NameDesc> ops{ found->begin(), found->end() };
        ensure_equals("failed to find operations", ops.size(), 1);
        ensure_equals("wrong operation name", ops[0].first, "set");
        ensure_contains("wrong operation desc", ops[0].second, "set operation");
        LLSD metadata{ found->getMetadata(ops[0].first) };
        ensure_equals("bad metadata name", metadata["name"].asString(), ops[0].first);
        ensure_equals("bad metadata desc", metadata["desc"].asString(), ops[0].second);
    }
} // namespace tut
