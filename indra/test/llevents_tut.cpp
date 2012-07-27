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
#include "lllistenerwrapper.h"
// STL headers
// std headers
#include <iostream>
#include <typeinfo>
// external library headers
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/assign/list_of.hpp>
// other Linden headers
#include "lltut.h"
#include "catch_and_store_what_in.h"
#include "stringize.h"
#include "tests/listener.h"

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
	// Now there's a static constructor in llevents.cpp that registers on
	// the "mainloop" pump to call LLEventPumps::flush().
	// Actually -- having to modify this to track the statically-
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
	std::string threw;
	try
	{
		// NOTE: boost::bind() saves its arguments by VALUE! If you pass
		// an object instance rather than a pointer, you'll end up binding
		// to an internal copy of that instance! Use boost::ref() to
		// capture a reference instead.
		per_frame.listen(listener0.getName(), // note bug, dup name
						 boost::bind(&Listener::call, boost::ref(listener1), _1));
	}
	CATCH_AND_STORE_WHAT_IN(threw, LLEventPump::DupListenerName)
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
	set_test_name("LLEventQueue delayed action");
	// This access is NOT legal usage: we can do it only because we're
	// hacking private for test purposes. Normally we'd either compile in
	// a particular name, or (later) edit a config file.
	pumps.mQueueNames.insert("login");
	LLEventPump& login(pumps.obtain("login"));
	// The "mainloop" pump is special: posting on that implicitly calls
	// LLEventPumps::flush(), which in turn should flush our "login"
	// LLEventQueue.
	LLEventPump& mainloop(pumps.obtain("mainloop"));
	ensure("LLEventQueue leaf class", dynamic_cast<LLEventQueue*> (&login));
	listener0.listenTo(login);
	listener0.reset(0);
	login.post(1);
	check_listener("waiting for queued event", listener0, 0);
	mainloop.post(LLSD());
	check_listener("got queued event", listener0, 1);
	login.stopListening(listener0.getName());
	// Verify that when an event handler posts a new event on the same
	// LLEventQueue, it doesn't get processed in the same flush() call --
	// it waits until the next flush() call.
	listener0.reset(17);
	login.listen("chainEvents", boost::bind(chainEvents, boost::ref(listener0), _1));
	login.post(1);
	check_listener("chainEvents(1) not yet called", listener0, 17);
	mainloop.post(LLSD());
	check_listener("chainEvents(1) called", listener0, 1);
	mainloop.post(LLSD());
	check_listener("chainEvents(0) called", listener0, 0);
	mainloop.post(LLSD());
	check_listener("chainEvents(-1) not called", listener0, 0);
	login.stopListening("chainEvents");
}

template<> template<>
void events_object::test<4>()
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
void events_object::test<5>()
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
void events_object::test<6>()
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
void events_object::test<7>()
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
	std::string threw;
	try
	{
		button.listen("spot",
					  boost::bind(&Collect::add, boost::ref(collector), "spot", _1),
					  // after "Mary" and "checked" -- whoops!
			 		  make<NameList>(list_of("Mary")("checked")));
	}
	CATCH_AND_STORE_WHAT_IN(threw, LLEventPump::Cycle)
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
	threw.clear();
	try
	{
		button.listen("of",
					  boost::bind(&Collect::add, boost::ref(collector), "of", _1),
					  make<NameList>(list_of("shoelaces")),
					  make<NameList>(list_of("yellow")));
	}
	CATCH_AND_STORE_WHAT_IN(threw, LLEventPump::OrderChange)
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
void events_object::test<8>()
{
	set_test_name("tweaked and untweaked LLEventPump instance names");
	{ 	// nested scope
		// Hand-instantiate an LLEventStream...
		LLEventStream bob("bob");
		std::string threw;
		try
		{
			// then another with a duplicate name.
			LLEventStream bob2("bob");
		}
		CATCH_AND_STORE_WHAT_IN(threw, LLEventPump::DupPumpName)
		ensure("Caught DupPumpName", !threw.empty());
	} 	// delete first 'bob'
	LLEventStream bob("bob"); 		// should work, previous one unregistered
	LLEventStream bob1("bob", true);// allowed to tweak name
	ensure_equals("tweaked LLEventStream name", bob1.getName(), "bob1");
	std::vector<boost::shared_ptr<LLEventStream> > streams;
	for (int i = 2; i <= 10; ++i)
	{
		streams.push_back(boost::shared_ptr<LLEventStream>(new LLEventStream("bob", true)));
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
void events_object::test<9>()
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
	std::string threw;
	try
	{
		LLListenerOrPumpName empty;
		empty(17);
	}
	CATCH_AND_STORE_WHAT_IN(threw, LLListenerOrPumpName::Empty)

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
void events_object::test<10>()
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

template<> template<>
void events_object::test<11>()
{
	set_test_name("listen(boost::bind(...weak_ptr...))");
	// listen() detecting weak_ptr<TempListener> in boost::bind() object
	bool live = false;
	LLEventPump& heaptest(pumps.obtain("heaptest"));
	LLBoundListener connection;
	ensure("default state", !connection.connected());
	{
		boost::shared_ptr<TempListener> newListener(new TempListener("heap", live));
		newListener->reset();
		ensure("TempListener constructed", live);
		connection = heaptest.listen(newListener->getName(),
									 boost::bind(&Listener::call, 
												 weaken(newListener), 
												 _1));
		ensure("new connection", connection.connected());
		heaptest.post(1);
		check_listener("received", *newListener, 1);
	} // presumably this will make newListener go away?
	// verify that
	ensure("TempListener destroyed", !live);
	ensure("implicit disconnect", !connection.connected());
	// now just make sure we don't blow up trying to access a freed object!
	heaptest.post(2);
}

template<> template<>
void events_object::test<12>()
{
	set_test_name("listen(boost::bind(...shared_ptr...))");
	/*==========================================================================*|
	// DISABLED because I've made this case produce a compile error.
	// Following the error leads the disappointed dev to a comment
	// instructing her to use the weaken() function to bind a weak_ptr<T>
	// instead of binding a shared_ptr<T>, and explaining why. I know of
	// no way to use TUT to code a repeatable test in which the expected
	// outcome is a compile error. The interested reader is invited to
	// uncomment this block and build to see for herself.

	// listen() detecting shared_ptr<TempListener> in boost::bind() object
	bool live = false;
	LLEventPump& heaptest(pumps.obtain("heaptest"));
	LLBoundListener connection;
	std::string listenerName("heap");
	ensure("default state", !connection.connected());
	{
		boost::shared_ptr<TempListener> newListener(new TempListener(listenerName, live));
		ensure_equals("use_count", newListener.use_count(), 1);
		newListener->reset();
		ensure("TempListener constructed", live);
		connection = heaptest.listen(newListener->getName(),
									 boost::bind(&Listener::call, newListener, _1));
		ensure("new connection", connection.connected());
		ensure_equals("use_count", newListener.use_count(), 2);
		heaptest.post(1);
		check_listener("received", *newListener, 1);
	} // this should make newListener go away...
	// Unfortunately, the fact that we've bound a shared_ptr by value into
	// our LLEventPump means that copy will keep the referenced object alive.
	ensure("TempListener still alive", live);
	ensure("still connected", connection.connected());
	// disconnecting explicitly should delete the TempListener...
	heaptest.stopListening(listenerName);
#if 0   // however, in my experience, it does not. I don't know why not.
	// Ah: on 2009-02-19, Frank Mori Hess, author of the Boost.Signals2
	// library, stated on the boost-users mailing list:
	// http://www.nabble.com/Re%3A--signals2--review--The-review-of-the-signals2-library-(formerly-thread_safe_signals)-begins-today%2C-Nov-1st-p22102367.html
	// "It will get destroyed eventually. The signal cleans up its slot
	// list little by little during connect/invoke. It doesn't immediately
	// remove disconnected slots from the slot list since other threads
	// might be using the same slot list concurrently. It might be
	// possible to make it immediately reset the shared_ptr owning the
	// slot though, leaving an empty shared_ptr in the slot list, since
	// that wouldn't invalidate any iterators."
	ensure("TempListener destroyed", ! live);
	ensure("implicit disconnect", ! connection.connected());
#endif  // 0
	// now just make sure we don't blow up trying to access a freed object!
	heaptest.post(2);
|*==========================================================================*/
}

class TempTrackableListener: public TempListener, public LLEventTrackable
{
public:
TempTrackableListener(const std::string& name, bool& liveFlag):
	TempListener(name, liveFlag)
{}
};

template<> template<>
void events_object::test<13>()
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
void events_object::test<14>()
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

template<> template<>
void events_object::test<15>()
{
// This test ensures that using an LLListenerWrapper subclass doesn't
// block Boost.Signals2 from recognizing a bound LLEventTrackable
// subclass.
set_test_name("listen(llwrap<LLLogListener>(boost::bind(...TempTrackableListener ref...)))");
bool live = false;
LLEventPump& heaptest(pumps.obtain("heaptest"));
LLBoundListener connection;
{
	TempTrackableListener tempListener("temp", live);
	ensure("TempTrackableListener constructed", live);
	connection = heaptest.listen(tempListener.getName(),
								 llwrap<LLLogListener>(
								 boost::bind(&TempTrackableListener::call,
											 boost::ref(tempListener), _1)));
	heaptest.post(1);
	check_listener("received", tempListener, 1);
} // presumably this will make tempListener go away?
// verify that
ensure("TempTrackableListener destroyed", ! live);
ensure("implicit disconnect", ! connection.connected());
// now just make sure we don't blow up trying to access a freed object!
heaptest.post(2);
}

class TempSharedListener: public TempListener,
public boost::enable_shared_from_this<TempSharedListener>
{
public:
TempSharedListener(const std::string& name, bool& liveFlag):
	TempListener(name, liveFlag)
{}
};

template<> template<>
void events_object::test<16>()
{
	set_test_name("listen(boost::bind(...TempSharedListener ref...))");
#if 0
bool live = false;
LLEventPump& heaptest(pumps.obtain("heaptest"));
LLBoundListener connection;
{
	// We MUST have at least one shared_ptr to an
	// enable_shared_from_this subclass object before
	// shared_from_this() can work.
	boost::shared_ptr<TempSharedListener>
		tempListener(new TempSharedListener("temp", live));
	ensure("TempSharedListener constructed", live);
	// However, we're not passing either the shared_ptr or its
	// corresponding weak_ptr -- instead, we're passing a reference to
	// the TempSharedListener.
/*==========================================================================*|
	 std::cout << "Capturing const ref" << std::endl;
	 const boost::enable_shared_from_this<TempSharedListener>& cref(*tempListener);
	 std::cout << "Capturing const ptr" << std::endl;
	 const boost::enable_shared_from_this<TempSharedListener>* cp(&cref);
	 std::cout << "Capturing non-const ptr" << std::endl;
	 boost::enable_shared_from_this<TempSharedListener>* p(const_cast<boost::enable_shared_from_this<TempSharedListener>*>(cp));
	 std::cout << "Capturing shared_from_this()" << std::endl;
	 boost::shared_ptr<TempSharedListener> sp(p->shared_from_this());
	 std::cout << "Capturing weak_ptr" << std::endl;
	 boost::weak_ptr<TempSharedListener> wp(weaken(sp));
	 std::cout << "Binding weak_ptr" << std::endl;
|*==========================================================================*/
	connection = heaptest.listen(tempListener->getName(),
								 boost::bind(&TempSharedListener::call, *tempListener, _1));
	heaptest.post(1);
	check_listener("received", *tempListener, 1);
} // presumably this will make tempListener go away?
// verify that
ensure("TempSharedListener destroyed", ! live);
ensure("implicit disconnect", ! connection.connected());
// now just make sure we don't blow up trying to access a freed object!
heaptest.post(2);
#endif // 0
}
} // namespace tut
