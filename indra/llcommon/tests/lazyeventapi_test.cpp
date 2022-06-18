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

// LLEventAPI listener subclass
class MyListener: public LLEventAPI
{
public:
    MyListener(const LL::LazyEventAPIParams& params):
        LLEventAPI(params)
    {}

    void get(const LLSD& event)
    {
        std::cout << "MyListener::get() got " << event << std::endl;
    }
};

// LazyEventAPI registrar subclass
class MyRegistrar: public LL::LazyEventAPI<MyListener>
{
    using super = LL::LazyEventAPI<MyListener>;
    using super::listener;
public:
    MyRegistrar():
        super("Test", "This is a test LLEventAPI")
    {
        add("get", "This is a get operation", &listener::get);            
    }
};
// Normally we'd declare a static instance of MyRegistrar -- but because we
// may want to test with and without, defer declaration to individual test
// methods.

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct lazyeventapi_data
    {
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
        LLEventPumps::instance().obtain("Test").post("hey");
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("No LazyEventAPI");
        // Because the MyRegistrar declaration in test<1>() is local, because
        // it has been destroyed, we fully expect NOT to reach a MyListener
        // instance with this post.
        LLEventPumps::instance().obtain("Test").post("moot");
    }
} // namespace tut
