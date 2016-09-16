/**
 * @file   llpounceable_test.cpp
 * @author Nat Goodspeed
 * @date   2015-05-22
 * @brief  Test for llpounceable.
 * 
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Copyright (c) 2015, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llpounceable.h"
// STL headers
// std headers
// external library headers
#include <boost/bind.hpp>
// other Linden headers
#include "../test/lltut.h"

/*----------------------------- string testing -----------------------------*/
void append(std::string* dest, const std::string& src)
{
    dest->append(src);
}

/*-------------------------- Data-struct testing ---------------------------*/
struct Data
{
    Data(const std::string& data):
        mData(data)
    {}
    const std::string mData;
};

void setter(Data** dest, Data* ptr)
{
    *dest = ptr;
}

static Data* static_check = 0;

// Set up an extern pointer to an LLPounceableStatic so the linker will fill
// in the forward reference from below, before runtime.
extern LLPounceable<Data*, LLPounceableStatic> gForward;

struct EnqueueCall
{
    EnqueueCall()
    {
        // Intentionally use a forward reference to an LLPounceableStatic that
        // we believe is NOT YET CONSTRUCTED. This models the scenario in
        // which a constructor in another translation unit runs before
        // constructors in this one. We very specifically want callWhenReady()
        // to work even in that case: we need the LLPounceableQueueImpl to be
        // initialized even if the LLPounceable itself is not.
        gForward.callWhenReady(boost::bind(setter, &static_check, _1));
    }
} nqcall;
// When this declaration is processed, we should enqueue the
// setter(&static_check, _1) call for when gForward is set non-NULL. Needless
// to remark, we want this call not to crash.

// Now declare gForward. Its constructor should not run until after nqcall's.
LLPounceable<Data*, LLPounceableStatic> gForward;

/*****************************************************************************
*   TUT
*****************************************************************************/
namespace tut
{
    struct llpounceable_data
    {
    };
    typedef test_group<llpounceable_data> llpounceable_group;
    typedef llpounceable_group::object object;
    llpounceable_group llpounceablegrp("llpounceable");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("LLPounceableStatic out-of-order test");
        // LLPounceable<T, LLPounceableStatic>::callWhenReady() must work even
        // before LLPounceable's constructor runs. That's the whole point of
        // implementing it with an LLSingleton queue. This models (say)
        // LLPounceableStatic<LLMessageSystem*, LLPounceableStatic>.
        ensure("static_check should still be null", ! static_check);
        Data myData("test<1>");
        gForward = &myData;         // should run setter
        ensure_equals("static_check should be &myData", static_check, &myData);
    }

    template<> template<>
    void object::test<2>()
    {
        set_test_name("LLPounceableQueue different queues");
        // We expect that LLPounceable<T, LLPounceableQueue> should have
        // different queues because that specialization stores the queue
        // directly in the LLPounceable instance.
        Data *aptr = 0, *bptr = 0;
        LLPounceable<Data*> a, b;
        a.callWhenReady(boost::bind(setter, &aptr, _1));
        b.callWhenReady(boost::bind(setter, &bptr, _1));
        ensure("aptr should be null", ! aptr);
        ensure("bptr should be null", ! bptr);
        Data adata("a"), bdata("b");
        a = &adata;
        ensure_equals("aptr should be &adata", aptr, &adata);
        // but we haven't yet set b
        ensure("bptr should still be null", !bptr);
        b = &bdata;
        ensure_equals("bptr should be &bdata", bptr, &bdata);
    }

    template<> template<>
    void object::test<3>()
    {
        set_test_name("LLPounceableStatic different queues");
        // LLPounceable<T, LLPounceableStatic> should also have a distinct
        // queue for each instance, but that engages an additional map lookup
        // because there's only one LLSingleton for each T.
        Data *aptr = 0, *bptr = 0;
        LLPounceable<Data*, LLPounceableStatic> a, b;
        a.callWhenReady(boost::bind(setter, &aptr, _1));
        b.callWhenReady(boost::bind(setter, &bptr, _1));
        ensure("aptr should be null", ! aptr);
        ensure("bptr should be null", ! bptr);
        Data adata("a"), bdata("b");
        a = &adata;
        ensure_equals("aptr should be &adata", aptr, &adata);
        // but we haven't yet set b
        ensure("bptr should still be null", !bptr);
        b = &bdata;
        ensure_equals("bptr should be &bdata", bptr, &bdata);
    }

    template<> template<>
    void object::test<4>()
    {
        set_test_name("LLPounceable<T> looks like T");
        // We want LLPounceable<T, TAG> to be drop-in replaceable for a plain
        // T for read constructs. In particular, it should behave like a dumb
        // pointer -- and with zero abstraction cost for such usage.
        Data* aptr = 0;
        Data a("a");
        // should be able to initialize a pounceable (when its constructor
        // runs)
        LLPounceable<Data*> pounceable(&a);
        // should be able to pass LLPounceable<T> to function accepting T
        setter(&aptr, pounceable);
        ensure_equals("aptr should be &a", aptr, &a);
        // should be able to dereference with *
        ensure_equals("deref with *", (*pounceable).mData, "a");
        // should be able to dereference with ->
        ensure_equals("deref with ->", pounceable->mData, "a");
        // bool operations
        ensure("test with operator bool()", pounceable);
        ensure("test with operator !()", ! (! pounceable));
    }

    template<> template<>
    void object::test<5>()
    {
        set_test_name("Multiple callWhenReady() queue items");
        Data *p1 = 0, *p2 = 0, *p3 = 0;
        Data a("a");
        LLPounceable<Data*> pounceable;
        // queue up a couple setter() calls for later
        pounceable.callWhenReady(boost::bind(setter, &p1, _1));
        pounceable.callWhenReady(boost::bind(setter, &p2, _1));
        // should still be pending
        ensure("p1 should be null", !p1);
        ensure("p2 should be null", !p2);
        ensure("p3 should be null", !p3);
        pounceable = 0;
        // assigning a new empty value shouldn't flush the queue
        ensure("p1 should still be null", !p1);
        ensure("p2 should still be null", !p2);
        ensure("p3 should still be null", !p3);
        // using whichever syntax
        pounceable.reset(0);
        // try to make ensure messages distinct... tough to pin down which
        // ensure() failed if multiple ensure() calls in the same test<n> have
        // the same message!
        ensure("p1 should again be null", !p1);
        ensure("p2 should again be null", !p2);
        ensure("p3 should again be null", !p3);
        pounceable.reset(&a);       // should flush queue
        ensure_equals("p1 should be &a", p1, &a);
        ensure_equals("p2 should be &a", p2, &a);
        ensure("p3 still not set", !p3);
        // immediate call
        pounceable.callWhenReady(boost::bind(setter, &p3, _1));
        ensure_equals("p3 should be &a", p3, &a);
    }

    template<> template<>
    void object::test<6>()
    {
        set_test_name("queue order");
        std::string data;
        LLPounceable<std::string*> pounceable;
        pounceable.callWhenReady(boost::bind(append, _1, "a"));
        pounceable.callWhenReady(boost::bind(append, _1, "b"));
        pounceable.callWhenReady(boost::bind(append, _1, "c"));
        pounceable = &data;
        ensure_equals("callWhenReady() must preserve chronological order",
                      data, "abc");

        std::string data2;
        pounceable = NULL;
        pounceable.callWhenReady(boost::bind(append, _1, "d"));
        pounceable.callWhenReady(boost::bind(append, _1, "e"));
        pounceable.callWhenReady(boost::bind(append, _1, "f"));
        pounceable = &data2;
        ensure_equals("LLPounceable must reset queue when fired",
                      data2, "def");
    }

    template<> template<>
    void object::test<7>()
    {
        set_test_name("compile-fail test, uncomment to check");
        // The following declaration should fail: only LLPounceableQueue and
        // LLPounceableStatic should work as tags.
//      LLPounceable<Data*, int> pounceable;
    }
} // namespace tut
