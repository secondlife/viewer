/**
 * @file   llevents_tut.cpp
 * @author Nat Goodspeed
 * @date   2008-09-12
 * @brief  Test of llevents.h
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#if LL_WINDOWS
#pragma warning (disable : 4675) // "resolved by ADL" -- just as I want!
#endif

// Precompiled header
#include "linden_common.h"
// associated header
// UGLY HACK! We want to verify state internal to the classes without
// providing public accessors.
#define testable public
#include "llevents.h"
#undef testable
// STL headers
// std headers
#include <iostream>
#include <typeinfo>
// external library headers
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/assign/list_of.hpp>
// other Linden headers
#include "tests/listener.h"             // must PRECEDE lltut.h
#include "lltut.h"
#include "catch_and_store_what_in.h"
#include "stringize.h"

using boost::assign::list_of;

template<typename T>
T make(const T& value)
{
    return value;
}

/*****************************************************************************
 *   tut test group
 *****************************************************************************/
namespace tut
{
struct events_data
{
    events_data() :
        pumps(LLEventPumps::instance()),
        listener0("first"),
        listener1("second")
    {
    }
    LLEventPumps& pumps;
    Listener listener0;
    Listener listener1;

    void check_listener(const std::string& desc, const Listener& listener, LLSD::Integer got)
    {
        ensure_equals(STRINGIZE(listener << ' ' << desc),
                      listener.getLastEvent().asInteger(), got);
    }
};
typedef test_group<events_data> events_group;
typedef events_group::object events_object;
tut::events_group evgr("events");

template<> template<>
void events_object::test<1>()
{
    set_test_name("basic operations");
    // Having to modify this to track the statically-
    // constructed pumps in other TUT modules in this giant monolithic test
    // executable isn't such a hot idea.
    // ensure_equals("initial pump", pumps.mPumpMap.size(), 1);
    size_t initial_pumps(pumps.mPumpMap.size());
    LLEventPump& per_frame(pumps.obtain("per-frame"));
    ensure_equals("first explicit pump", pumps.mPumpMap.size(), initial_pumps + 1);
    // Verify that per_frame was instantiated as an LLEventStream.
    ensure("LLEventStream leaf class", dynamic_cast<LLEventStream*> (&per_frame));
    ensure("enabled", per_frame.enabled());
    // Trivial test, but posting an event to an EventPump with no
    // listeners should not blow up. The test is relevant because defining
    // a boost::signal with a non-void return signature, using the default
    // combiner, blows up if there are no listeners. This is because the
    // default combiner is defined to return the value returned by the
    // last listener, which is meaningless if there were no listeners.
    per_frame.post(0);
    LLBoundListener connection = listener0.listenTo(per_frame);
    ensure("connected", connection.connected());
    ensure("not blocked", !connection.blocked());
    per_frame.post(1);
    check_listener("received", listener0, 1);
    { // block the connection
        LLEventPump::Blocker block(connection);
        ensure("blocked", connection.blocked());
        per_frame.post(2);
        check_listener("not updated", listener0, 1);
    } // unblock
    ensure("unblocked", !connection.blocked());
    per_frame.post(3);
    check_listener("unblocked", listener0, 3);
    LLBoundListener sameConnection = per_frame.getListener(listener0.getName());
    ensure("still connected", sameConnection.connected());
    ensure("still not blocked", !sameConnection.blocked());
    { // block it again
        LLEventPump::Blocker block(sameConnection);
        ensure("re-blocked", sameConnection.blocked());
        per_frame.post(4);
        check_listener("re-blocked", listener0, 3);
    } // unblock
    std::string threw = catch_what<LLEventPump::DupListenerName>(
        [&per_frame, this](){
            // NOTE: boost::bind() saves its arguments by VALUE! If you pass
            // an object instance rather than a pointer, you'll end up binding
            // to an internal copy of that instance! Use boost::ref() to
            // capture a reference instead.
            per_frame.listen(listener0.getName(), // note bug, dup name
                             boost::bind(&Listener::call, boost::ref(listener1), _1));
        });
    ensure_equals(threw,
                  std::string("DupListenerName: "
                              "Attempt to register duplicate listener name '") +
                              listener0.getName() + "' on " + typeid(per_frame).name() +
                              " '" + per_frame.getName() + "'");
    // do it right this time
    listener1.listenTo(per_frame);
    per_frame.post(5);
    check_listener("got", listener0, 5);
    check_listener("got", listener1, 5);
    per_frame.enable(false);
    per_frame.post(6);
    check_listener("didn't get", listener0, 5);
    check_listener("didn't get", listener1, 5);
    per_frame.enable();
    per_frame.post(7);
    check_listener("got", listener0, 7);
    check_listener("got", listener1, 7);
    per_frame.stopListening(listener0.getName());
    ensure("disconnected 0", ! connection.connected());
    ensure("disconnected 1", ! sameConnection.connected());
    per_frame.post(8);
    check_listener("disconnected", listener0, 7);
    check_listener("still connected", listener1, 8);
    per_frame.stopListening(listener1.getName());
    per_frame.post(9);
    check_listener("disconnected", listener1, 8);
}

template<> template<>
void events_object::test<2>()
{
    set_test_name("callstop() returning true");
    LLEventPump& per_frame(pumps.obtain("per-frame"));
    listener0.reset(0);
    listener1.reset(0);
    LLBoundListener bound0 = listener0.listenTo(per_frame, &Listener::callstop);
    LLBoundListener bound1 = listener1.listenTo(per_frame, &Listener::call,
                                                // after listener0
                                                make<LLEventPump::NameList>(list_of(listener0.getName())));
    ensure("enabled", per_frame.enabled());
    ensure("connected 0", bound0.connected());
    ensure("unblocked 0", !bound0.blocked());
    ensure("connected 1", bound1.connected());
    ensure("unblocked 1", !bound1.blocked());
    per_frame.post(1);
    check_listener("got", listener0, 1);
    // Because listener0.callstop() returns true, control never reaches listener1.call().
    check_listener("got", listener1, 0);
}

bool chainEvents(Listener& someListener, const LLSD& event)
{
    // Make this call so we can watch for side effects for test purposes.
    someListener.call(event);
    // This function represents a recursive event chain -- or some other
    // scenario in which an event handler raises additional events.
    int value = event.asInteger();
    if (value)
    {
        LLEventPumps::instance().obtain("login").post(value - 1);
    }
    return false;
}

template<> template<>
void events_object::test<3>()
{
    set_test_name("explicitly-instantiated LLEventStream");
    // Explicitly instantiate an LLEventStream, and verify that it
    // self-registers with LLEventPumps
    size_t registered = pumps.mPumpMap.size();
    size_t owned = pumps.mOurPumps.size();
    LLEventPump* localInstance;
    {
        LLEventStream myEventStream("stream");
        localInstance = &myEventStream;
        LLEventPump& stream(pumps.obtain("stream"));
        ensure("found named LLEventStream instance", &stream == localInstance);
        ensure_equals("registered new instance", pumps.mPumpMap.size(), registered + 1);
        ensure_equals("explicit instance not owned", pumps.mOurPumps.size(), owned);
    } // destroy myEventStream -- should unregister
    ensure_equals("destroyed instance unregistered", pumps.mPumpMap.size(), registered);
    ensure_equals("destroyed instance not owned", pumps.mOurPumps.size(), owned);
    LLEventPump& stream(pumps.obtain("stream"));
    ensure("new LLEventStream instance", &stream != localInstance);
    ensure_equals("obtain()ed instance registered", pumps.mPumpMap.size(), registered + 1);
    ensure_equals("obtain()ed instance owned", pumps.mOurPumps.size(), owned + 1);
}

template<> template<>
void events_object::test<4>()
{
    set_test_name("stopListening()");
    LLEventPump& login(pumps.obtain("login"));
    listener0.listenTo(login);
    login.stopListening(listener0.getName());
    // should not throw because stopListening() should have removed name
    listener0.listenTo(login, &Listener::callstop);
    LLBoundListener wrong = login.getListener("bogus");
    ensure("bogus connection disconnected", !wrong.connected());
    ensure("bogus connection blocked", wrong.blocked());
}

template<> template<>
void events_object::test<5>()
{
    set_test_name("chaining LLEventPump instances");
    LLEventPump& upstream(pumps.obtain("upstream"));
    // One potentially-useful construct is to chain LLEventPumps together.
    // Among other things, this allows you to turn subsets of listeners on
    // and off in groups.
    LLEventPump& filter0(pumps.obtain("filter0"));
    LLEventPump& filter1(pumps.obtain("filter1"));
    upstream.listen(filter0.getName(), boost::bind(&LLEventPump::post, boost::ref(filter0), _1));
    upstream.listen(filter1.getName(), boost::bind(&LLEventPump::post, boost::ref(filter1), _1));
    listener0.listenTo(filter0);
    listener1.listenTo(filter1);
    listener0.reset(0);
    listener1.reset(0);
    upstream.post(1);
    check_listener("got unfiltered", listener0, 1);
    check_listener("got unfiltered", listener1, 1);
    filter0.enable(false);
    upstream.post(2);
    check_listener("didn't get filtered", listener0, 1);
    check_listener("got filtered", listener1, 2);
}

template<> template<>
void events_object::test<6>()
{
    set_test_name("listener dependency order");
    typedef LLEventPump::NameList NameList;
    LLEventPump& button(pumps.obtain("button"));
    Collect collector;
    button.listen("Mary",
                  boost::bind(&Collect::add, boost::ref(collector), "Mary", _1),
                  // state that "Mary" must come after "checked"
                  make<NameList> (list_of("checked")));
    button.listen("checked",
                  boost::bind(&Collect::add, boost::ref(collector), "checked", _1),
                  // "checked" must come after "spot"
                  make<NameList> (list_of("spot")));
    button.listen("spot",
                  boost::bind(&Collect::add, boost::ref(collector), "spot", _1));
    button.post(1);
    ensure_equals(collector.result, make<StringVec>(list_of("spot")("checked")("Mary")));
    collector.clear();
    button.stopListening("Mary");
    button.listen("Mary",
            boost::bind(&Collect::add, boost::ref(collector), "Mary", _1),
            LLEventPump::empty, // no after dependencies
            // now "Mary" must come before "spot"
            make<NameList>(list_of("spot")));
    button.post(2);
    ensure_equals(collector.result, make<StringVec>(list_of("Mary")("spot")("checked")));
    collector.clear();
    button.stopListening("spot");
    std::string threw = catch_what<LLEventPump::Cycle>(
        [&button, &collector](){
            button.listen("spot",
                          boost::bind(&Collect::add, boost::ref(collector), "spot", _1),
                          // after "Mary" and "checked" -- whoops!
                          make<NameList>(list_of("Mary")("checked")));
        });
    // Obviously the specific wording of the exception text can
    // change; go ahead and change the test to match.
    // Establish that it contains:
    // - the name and runtime type of the LLEventPump
    ensure_contains("LLEventPump type", threw, typeid(button).name());
    ensure_contains("LLEventPump name", threw, "'button'");
    // - the name of the new listener that caused the problem
    ensure_contains("new listener name", threw, "'spot'");
    // - a synopsis of the problematic dependencies.
    ensure_contains("cyclic dependencies", threw,
                    "\"Mary\" -> before (\"spot\")");
    ensure_contains("cyclic dependencies", threw,
                    "after (\"spot\") -> \"checked\"");
    ensure_contains("cyclic dependencies", threw,
                    "after (\"Mary\", \"checked\") -> \"spot\"");
    button.listen("yellow",
                  boost::bind(&Collect::add, boost::ref(collector), "yellow", _1),
                  make<NameList>(list_of("checked")));
    button.listen("shoelaces",
                  boost::bind(&Collect::add, boost::ref(collector), "shoelaces", _1),
                  make<NameList>(list_of("checked")));
    button.post(3);
    ensure_equals(collector.result, make<StringVec>(list_of("Mary")("checked")("yellow")("shoelaces")));
    collector.clear();
    threw = catch_what<LLEventPump::OrderChange>(
        [&button, &collector](){
            button.listen("of",
                          boost::bind(&Collect::add, boost::ref(collector), "of", _1),
                          make<NameList>(list_of("shoelaces")),
                          make<NameList>(list_of("yellow")));
        });
    // Same remarks about the specific wording of the exception. Just
    // ensure that it contains enough information to clarify the
    // problem and what must be done to resolve it.
    ensure_contains("LLEventPump type", threw, typeid(button).name());
    ensure_contains("LLEventPump name", threw, "'button'");
    ensure_contains("new listener name", threw, "'of'");
    ensure_contains("prev listener name", threw, "'yellow'");
    // std::cout << "Thrown Exception: " << threw << std::endl;
    ensure_contains("old order", threw, "was: Mary, checked, yellow, shoelaces");
    ensure_contains("new order", threw, "now: Mary, checked, shoelaces, of, yellow");
    button.post(4);
    ensure_equals(collector.result, make<StringVec>(list_of("Mary")("checked")("yellow")("shoelaces")));
}

template<> template<>
void events_object::test<7>()
{
    set_test_name("tweaked and untweaked LLEventPump instance names");
    {   // nested scope
        // Hand-instantiate an LLEventStream...
        LLEventStream bob("bob");
        std::string threw = catch_what<LLEventPump::DupPumpName>(
            [](){
                // then another with a duplicate name.
                LLEventStream bob2("bob");
            });
        ensure("Caught DupPumpName", !threw.empty());
    }   // delete first 'bob'
    LLEventStream bob("bob");       // should work, previous one unregistered
    LLEventStream bob1("bob", true);// allowed to tweak name
    ensure_equals("tweaked LLEventStream name", bob1.getName(), "bob1");
    std::vector<std::shared_ptr<LLEventStream> > streams;
    for (int i = 2; i <= 10; ++i)
    {
        streams.push_back(std::shared_ptr<LLEventStream>(new LLEventStream("bob", true)));
    }
    ensure_equals("last tweaked LLEventStream name", streams.back()->getName(), "bob10");
}

// Define a function that accepts an LLListenerOrPumpName
void eventSource(const LLListenerOrPumpName& listener)
{
    // Pretend that some time has elapsed. Call listener immediately.
    listener(17);
}

template<> template<>
void events_object::test<8>()
{
    set_test_name("LLListenerOrPumpName");
    // Passing a boost::bind() expression to LLListenerOrPumpName
    listener0.reset(0);
    eventSource(boost::bind(&Listener::call, boost::ref(listener0), _1));
    check_listener("got by listener", listener0, 17);
    // Passing a string LLEventPump name to LLListenerOrPumpName
    listener0.reset(0);
    LLEventStream random("random");
    listener0.listenTo(random);
    eventSource("random");
    check_listener("got by pump name", listener0, 17);
    std::string threw = catch_what<LLListenerOrPumpName::Empty>(
        [](){
            LLListenerOrPumpName empty;
            empty(17);
        });

    ensure("threw Empty", !threw.empty());
}

class TempListener: public Listener
{
public:
    TempListener(const std::string& name, bool& liveFlag) :
        Listener(name), mLiveFlag(liveFlag)
    {
        mLiveFlag = true;
    }

    virtual ~TempListener()
    {
        mLiveFlag = false;
    }

private:
    bool& mLiveFlag;
};

template<> template<>
void events_object::test<9>()
{
    set_test_name("listen(boost::bind(...TempListener...))");
    // listen() can't do anything about a plain TempListener instance:
    // it's not managed with shared_ptr, nor is it an LLEventTrackable subclass
    bool live = false;
    LLEventPump& heaptest(pumps.obtain("heaptest"));
    LLBoundListener connection;
    {
        TempListener tempListener("temp", live);
        ensure("TempListener constructed", live);
        connection = heaptest.listen(tempListener.getName(),
                                     boost::bind(&Listener::call,
                                                 boost::ref(tempListener),
                                                 _1));
        heaptest.post(1);
        check_listener("received", tempListener, 1);
    } // presumably this will make newListener go away?
    // verify that
    ensure("TempListener destroyed", !live);
    // This is the case against which we can't defend. Don't even try to
    // post to heaptest -- that would engage Undefined Behavior.
    // Cautiously inspect connection...
    ensure("misleadingly connected", connection.connected());
    // then disconnect by hand.
    heaptest.stopListening("temp");
}

class TempTrackableListener: public TempListener, public LLEventTrackable
{
public:
    TempTrackableListener(const std::string& name, bool& liveFlag):
        TempListener(name, liveFlag)
    {}
};

template<> template<>
void events_object::test<10>()
{
    set_test_name("listen(boost::bind(...TempTrackableListener ref...))");
    bool live = false;
    LLEventPump& heaptest(pumps.obtain("heaptest"));
    LLBoundListener connection;
    {
        TempTrackableListener tempListener("temp", live);
        ensure("TempTrackableListener constructed", live);
        connection = heaptest.listen(tempListener.getName(),
                                     boost::bind(&TempTrackableListener::call,
                                                 boost::ref(tempListener), _1));
        heaptest.post(1);
        check_listener("received", tempListener, 1);
    } // presumably this will make tempListener go away?
    // verify that
    ensure("TempTrackableListener destroyed", ! live);
    ensure("implicit disconnect", ! connection.connected());
    // now just make sure we don't blow up trying to access a freed object!
    heaptest.post(2);
}

template<> template<>
void events_object::test<11>()
{
    set_test_name("listen(boost::bind(...TempTrackableListener pointer...))");
    bool live = false;
    LLEventPump& heaptest(pumps.obtain("heaptest"));
    LLBoundListener connection;
    {
        TempTrackableListener* newListener(new TempTrackableListener("temp", live));
        ensure("TempTrackableListener constructed", live);
        connection = heaptest.listen(newListener->getName(),
                                     boost::bind(&TempTrackableListener::call,
                                                 newListener, _1));
        heaptest.post(1);
        check_listener("received", *newListener, 1);
        // explicitly destroy newListener
        delete newListener;
    }
    // verify that
    ensure("TempTrackableListener destroyed", ! live);
    ensure("implicit disconnect", ! connection.connected());
    // now just make sure we don't blow up trying to access a freed object!
    heaptest.post(2);
}

} // namespace tut
