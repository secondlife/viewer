/**
 * @file   classic_callback_test.cpp
 * @author Nat Goodspeed
 * @date   2021-09-22
 * @brief  Test ClassicCallback and HeapClassicCallback.
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "classic_callback.h"
// STL headers
#include <iostream>
#include <string>
// std headers
// external library headers
// other Linden headers
#include "../test/lltut.h"

/*****************************************************************************
*   example callback
*****************************************************************************/
// callback_t is part of the specification of someAPI()
typedef void (*callback_t)(const char*, void*);
void someAPI(callback_t callback, void* userdata)
{
    callback("called", userdata);
}

// C++ callable I want as the actual callback
struct MyCallback
{
    void operator()(const char* msg, void*)
    {
        mMsg = msg;
    }

    void callback_with_extra(const std::string& extra, const char* msg)
    {
        mMsg = extra + ' ' + msg;
    }

    std::string mMsg;
};

/*****************************************************************************
*   example callback accepting several params, and void* userdata isn't first
*****************************************************************************/
typedef std::string (*complex_callback)(int, const char*, void*, double);
std::string otherAPI(complex_callback callback, void* userdata)
{
    return callback(17, "hello world", userdata, 3.0);
}

// struct into which we can capture complex_callback params
static struct Data
{
    void set(int i, const char* s, double f)
    {
        mi = i;
        ms = s;
        mf = f;
    }

    void clear() { set(0, "", 0.0); }

    int mi;
    std::string ms;
    double mf;
} sData;

// C++ callable I want to pass
struct OtherCallback
{
    std::string operator()(int num, const char* str, void*, double approx)
    {
        sData.set(num, str, approx);
        return "hello back!";
    }
};

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct classic_callback_data
    {
    };
    typedef test_group<classic_callback_data> classic_callback_group;
    typedef classic_callback_group::object object;
    classic_callback_group classic_callbackgrp("classic_callback");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("ClassicCallback");
        // engage someAPI(MyCallback())
        auto ccb{ makeClassicCallback<callback_t>(MyCallback()) };
        someAPI(ccb.get_callback(), ccb.get_userdata());
        // Unfortunately, with the side effect confined to the bound
        // MyCallback instance, that call was invisible. Bind a reference to a
        // named instance by specifying a ref type.
        MyCallback mcb;
        ClassicCallback<callback_t, void*, MyCallback&> ccb2(mcb);
        someAPI(ccb2.get_callback(), ccb2.get_userdata());
        ensure_equals("failed to call through ClassicCallback", mcb.mMsg, "called");

        // try with HeapClassicCallback
        mcb.mMsg.clear();
        auto hcbp{ makeHeapClassicCallback<callback_t>(mcb) };
        someAPI(hcbp->get_callback(), hcbp->get_userdata());
        ensure_equals("failed to call through HeapClassicCallback", mcb.mMsg, "called");

        // lambda
        // The tricky thing here is that a lambda is an unspecified type, so
        // you can't declare a ClassicCallback<signature, void*, that type>.
        mcb.mMsg.clear();
        auto xcb(
            makeClassicCallback<callback_t>(
                [&mcb](const char* msg, void*)
                { mcb.callback_with_extra("extra", msg); }));
        someAPI(xcb.get_callback(), xcb.get_userdata());
        ensure_equals("failed to call lambda", mcb.mMsg, "extra called");

        // engage otherAPI(OtherCallback())
        OtherCallback ocb;
        // Instead of specifying a reference type for the bound CALLBACK, as
        // with ccb2 above, you can alternatively move the callable object
        // into the ClassicCallback (of course AFTER any other reference).
        // That's why OtherCallback uses external data for its observable side
        // effect.
        auto occb{ makeClassicCallback<complex_callback>(std::move(ocb)) };
        std::string result{ otherAPI(occb.get_callback(), occb.get_userdata()) };
        ensure_equals("failed to return callback result", result, "hello back!");
        ensure_equals("failed to set int", sData.mi, 17);
        ensure_equals("failed to set string", sData.ms, "hello world");
        ensure_equals("failed to set double", sData.mf, 3.0);
    }
} // namespace tut
