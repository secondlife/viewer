// ---------------------------------------------------------------------------
// Auto-generated from classic_callback_test.cpp at 2025-10-16T18:47:16Z
// This file is a TODO stub produced by gen_tut_to_doctest.py.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"
#include "linden_common.h"
#include "classic_callback.h"
#include <iostream>
#include <string>

TUT_SUITE("llcommon")
{
    TUT_CASE("classic_callback_test::object_test_1")
    {
        DOCTEST_FAIL("TODO: convert classic_callback_test.cpp::object::test<1> from TUT to doctest");
        // Original snippet:
        // template<> template<>
        //     void object::test<1>()
        //     {
        //         set_test_name("ClassicCallback");
        //         // engage someAPI(MyCallback())
        //         auto ccb{ makeClassicCallback<callback_t>(MyCallback()) };
        //         someAPI(ccb.get_callback(), ccb.get_userdata());
        //         // Unfortunately, with the side effect confined to the bound
        //         // MyCallback instance, that call was invisible. Bind a reference to a
        //         // named instance by specifying a ref type.
        //         MyCallback mcb;
        //         ClassicCallback<callback_t, void*, MyCallback&> ccb2(mcb);
        //         someAPI(ccb2.get_callback(), ccb2.get_userdata());
        //         ensure_equals("failed to call through ClassicCallback", mcb.mMsg, "called");

        //         // try with HeapClassicCallback
        //         mcb.mMsg.clear();
        //         auto hcbp{ makeHeapClassicCallback<callback_t>(mcb) };
        //         someAPI(hcbp->get_callback(), hcbp->get_userdata());
        //         ensure_equals("failed to call through HeapClassicCallback", mcb.mMsg, "called");

        //         // lambda
        //         // The tricky thing here is that a lambda is an unspecified type, so
        //         // you can't declare a ClassicCallback<signature, void*, that type>.
        //         mcb.mMsg.clear();
        //         auto xcb(
        //             makeClassicCallback<callback_t>(
        //                 [&mcb](const char* msg, void*)
        //                 { mcb.callback_with_extra("extra", msg); }));
        //         someAPI(xcb.get_callback(), xcb.get_userdata());
        //         ensure_equals("failed to call lambda", mcb.mMsg, "extra called");

        //         // engage otherAPI(OtherCallback())
        //         OtherCallback ocb;
        //         // Instead of specifying a reference type for the bound CALLBACK, as
        //         // with ccb2 above, you can alternatively move the callable object
        //         // into the ClassicCallback (of course AFTER any other reference).
        //         // That's why OtherCallback uses external data for its observable side
        //         // effect.
        //         auto occb{ makeClassicCallback<complex_callback>(std::move(ocb)) };
        //         std::string result{ otherAPI(occb.get_callback(), occb.get_userdata()) };
        //         ensure_equals("failed to return callback result", result, "hello back!");
        //         ensure_equals("failed to set int", sData.mi, 17);
        //         ensure_equals("failed to set string", sData.ms, "hello world");
        //         ensure_equals("failed to set double", sData.mf, 3.0);
        //     }
    }

}

