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
#include "../test/lldoctest.h"

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
TEST_SUITE("UnknownSuite") {

TEST_CASE("test_1")
{

        set_test_name("LLPounceableStatic out-of-order test");
        // LLPounceable<T, LLPounceableStatic>::callWhenReady() must work even
        // before LLPounceable's constructor runs. That's the whole point of
        // implementing it with an LLSingleton queue. This models (say)
        // LLPounceableStatic<LLMessageSystem*, LLPounceableStatic>.
        CHECK_MESSAGE(! static_check, "static_check should still be null");
        Data myData("test<1>");
        gForward = &myData;         // should run setter
        CHECK_MESSAGE(static_check == &myData, "static_check should be &myData");
    
}

TEST_CASE("test_2")
{

        set_test_name("LLPounceableQueue different queues");
        // We expect that LLPounceable<T, LLPounceableQueue> should have
        // different queues because that specialization stores the queue
        // directly in the LLPounceable instance.
        Data *aptr = 0, *bptr = 0;
        LLPounceable<Data*> a, b;
        a.callWhenReady(boost::bind(setter, &aptr, _1));
        b.callWhenReady(boost::bind(setter, &bptr, _1));
        CHECK_MESSAGE(! aptr, "aptr should be null");
        CHECK_MESSAGE(! bptr, "bptr should be null");
        Data adata("a"), bdata("b");
        a = &adata;
        CHECK_MESSAGE(aptr == &adata, "aptr should be &adata");
        // but we haven't yet set b
        CHECK_MESSAGE(!bptr, "bptr should still be null");
        b = &bdata;
        CHECK_MESSAGE(bptr == &bdata, "bptr should be &bdata");
    
}

TEST_CASE("test_3")
{

        set_test_name("LLPounceableStatic different queues");
        // LLPounceable<T, LLPounceableStatic> should also have a distinct
        // queue for each instance, but that engages an additional map lookup
        // because there's only one LLSingleton for each T.
        Data *aptr = 0, *bptr = 0;
        LLPounceable<Data*, LLPounceableStatic> a, b;
        a.callWhenReady(boost::bind(setter, &aptr, _1));
        b.callWhenReady(boost::bind(setter, &bptr, _1));
        CHECK_MESSAGE(! aptr, "aptr should be null");
        CHECK_MESSAGE(! bptr, "bptr should be null");
        Data adata("a"), bdata("b");
        a = &adata;
        CHECK_MESSAGE(aptr == &adata, "aptr should be &adata");
        // but we haven't yet set b
        CHECK_MESSAGE(!bptr, "bptr should still be null");
        b = &bdata;
        CHECK_MESSAGE(bptr == &bdata, "bptr should be &bdata");
    
}

TEST_CASE("test_4")
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
        CHECK_MESSAGE(aptr == &a, "aptr should be &a");
        // should be able to dereference with *
        ensure_equals("deref with *", (*pounceable).mData, "a");
        // should be able to dereference with ->
        CHECK_MESSAGE(pounceable->mData == "a", "deref with ->");
        // bool operations
        CHECK_MESSAGE(pounceable, "test with operator bool()");
        CHECK_MESSAGE(! (! pounceable, "test with operator !()"));
    
}

TEST_CASE("test_5")
{

        set_test_name("Multiple callWhenReady() queue items");
        Data *p1 = 0, *p2 = 0, *p3 = 0;
        Data a("a");
        LLPounceable<Data*> pounceable;
        // queue up a couple setter() calls for later
        pounceable.callWhenReady(boost::bind(setter, &p1, _1));
        pounceable.callWhenReady(boost::bind(setter, &p2, _1));
        // should still be pending
        CHECK_MESSAGE(!p1, "p1 should be null");
        CHECK_MESSAGE(!p2, "p2 should be null");
        CHECK_MESSAGE(!p3, "p3 should be null");
        pounceable = 0;
        // assigning a new empty value shouldn't flush the queue
        CHECK_MESSAGE(!p1, "p1 should still be null");
        CHECK_MESSAGE(!p2, "p2 should still be null");
        CHECK_MESSAGE(!p3, "p3 should still be null");
        // using whichever syntax
        pounceable.reset(0);
        // try to make ensure messages distinct... tough to pin down which
        // ensure() failed if multiple ensure() calls in the same test<n> have
        // the same message!
        CHECK_MESSAGE(!p1, "p1 should again be null");
        CHECK_MESSAGE(!p2, "p2 should again be null");
        CHECK_MESSAGE(!p3, "p3 should again be null");
        pounceable.reset(&a);       // should flush queue
        CHECK_MESSAGE(p1 == &a, "p1 should be &a");
        CHECK_MESSAGE(p2 == &a, "p2 should be &a");
        CHECK_MESSAGE(!p3, "p3 still not set");
        // immediate call
        pounceable.callWhenReady(boost::bind(setter, &p3, _1));
        CHECK_MESSAGE(p3 == &a, "p3 should be &a");
    
}

TEST_CASE("test_6")
{

        set_test_name("queue order");
        std::string data;
        LLPounceable<std::string*> pounceable;
        pounceable.callWhenReady(boost::bind(append, _1, "a"));
        pounceable.callWhenReady(boost::bind(append, _1, "b"));
        pounceable.callWhenReady(boost::bind(append, _1, "c"));
        pounceable = &data;
        CHECK_MESSAGE(data == "abc", "callWhenReady() must preserve chronological order");

        std::string data2;
        pounceable = NULL;
        pounceable.callWhenReady(boost::bind(append, _1, "d"));
        pounceable.callWhenReady(boost::bind(append, _1, "e"));
        pounceable.callWhenReady(boost::bind(append, _1, "f"));
        pounceable = &data2;
        CHECK_MESSAGE(data2 == "def", "LLPounceable must reset queue when fired");
    
}

TEST_CASE("test_7")
{

        set_test_name("compile-fail test, uncomment to check");
        // The following declaration should fail: only LLPounceableQueue and
        // LLPounceableStatic should work as tags.
//      LLPounceable<Data*, int> pounceable;
    
}

} // TEST_SUITE
