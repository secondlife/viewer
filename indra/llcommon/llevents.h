/**
 * @file   llevents.h
 * @author Kent Quirk, Nat Goodspeed
 * @date   2008-09-11
 * @brief  This is an implementation of the event system described at
 *         https://wiki.lindenlab.com/wiki/Viewer:Messaging/Event_System,
 *         originally introduced in llnotifications.h. It has nothing
 *         whatsoever to do with the older system in llevent.h.
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

#if ! defined(LL_LLEVENTS_H)
#define LL_LLEVENTS_H

#include <string>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <stdexcept>
#if LL_WINDOWS
	#pragma warning (push)
	#pragma warning (disable : 4263) // boost::signals2::expired_slot::what() has const mismatch
	#pragma warning (disable : 4264) 
#endif
#include <boost/signals2.hpp>
#if LL_WINDOWS
	#pragma warning (pop)
#endif

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>        // noncopyable
#include <boost/optional/optional.hpp>
#include <boost/visit_each.hpp>
#include <boost/ref.hpp>            // reference_wrapper
#include <boost/type_traits/is_pointer.hpp>
#include <boost/function.hpp>
#include <boost/static_assert.hpp>
#include "llsd.h"
#include "llsingleton.h"
#include "lldependencies.h"
#include "ll_template_cast.h"

/*==========================================================================*|
// override this to allow binding free functions with more parameters
#ifndef LLEVENTS_LISTENER_ARITY
#define LLEVENTS_LISTENER_ARITY 10
#endif
|*==========================================================================*/

// hack for testing
#ifndef testable
#define testable private
#endif

/*****************************************************************************
*   Signal and handler declarations
*   Using a single handler signature means that we can have a common handler
*   type, rather than needing a distinct one for each different handler.
*****************************************************************************/

/**
 * A boost::signals Combiner that stops the first time a handler returns true
 * We need this because we want to have our handlers return bool, so that
 * we have the option to cause a handler to stop further processing. The
 * default handler fails when the signal returns a value but has no slots.
 */
struct LLStopWhenHandled
{
    typedef bool result_type;

    template<typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) const
    {
        for (InputIterator si = first; si != last; ++si)
		{
            if (*si)
			{
                return true;
			}
		}
        return false;
    }
};

/**
 * We want to have a standard signature for all signals; this way,
 * we can easily document a protocol for communicating across
 * dlls and into scripting languages someday.
 *
 * We want to return a bool to indicate whether the signal has been
 * handled and should NOT be passed on to other listeners.
 * Return true to stop further handling of the signal, and false
 * to continue.
 *
 * We take an LLSD because this way the contents of the signal
 * are independent of the API used to communicate it.
 * It is const ref because then there's low cost to pass it;
 * if you only need to inspect it, it's very cheap.
 *
 * @internal
 * The @c float template parameter indicates that we will internally use @c
 * float to indicate relative listener order on a given LLStandardSignal.
 * Don't worry, the @c float values are strictly internal! They are not part
 * of the interface, for the excellent reason that requiring the caller to
 * specify a numeric key to establish order means that the caller must know
 * the universe of possible values. We use LLDependencies for that instead.
 */
typedef boost::signals2::signal<bool(const LLSD&), LLStopWhenHandled, float>  LLStandardSignal;
/// Methods that forward listeners (e.g. constructed with
/// <tt>boost::bind()</tt>) should accept (const LLEventListener&)
typedef LLStandardSignal::slot_type LLEventListener;
/// Result of registering a listener, supports <tt>connected()</tt>,
/// <tt>disconnect()</tt> and <tt>blocked()</tt>
typedef boost::signals2::connection LLBoundListener;
/// Storing an LLBoundListener in LLTempBoundListener will disconnect the
/// referenced listener when the LLTempBoundListener instance is destroyed.
typedef boost::signals2::scoped_connection LLTempBoundListener;

/**
 * A common idiom for event-based code is to accept either a callable --
 * directly called on completion -- or the string name of an LLEventPump on
 * which to post the completion event. Specifying a parameter as <tt>const
 * LLListenerOrPumpName&</tt> allows either.
 *
 * Calling a validly-constructed LLListenerOrPumpName, passing the LLSD
 * 'event' object, either calls the callable or posts the event to the named
 * LLEventPump.
 *
 * A default-constructed LLListenerOrPumpName is 'empty'. (This is useful as
 * the default value of an optional method parameter.) Calling it throws
 * LLListenerOrPumpName::Empty. Test for this condition beforehand using
 * either <tt>if (param)</tt> or <tt>if (! param)</tt>.
 */
class LL_COMMON_API LLListenerOrPumpName
{
public:
    /// passing string name of LLEventPump
    LLListenerOrPumpName(const std::string& pumpname);
    /// passing string literal (overload so compiler isn't forced to infer
    /// double conversion)
    LLListenerOrPumpName(const char* pumpname);
    /// passing listener -- the "anything else" catch-all case. The type of an
    /// object constructed by boost::bind() isn't intended to be written out.
    /// Normally we'd just accept 'const LLEventListener&', but that would
    /// require double implicit conversion: boost::bind() object to
    /// LLEventListener, LLEventListener to LLListenerOrPumpName. So use a
    /// template to forward anything.
    template<typename T>
    LLListenerOrPumpName(const T& listener): mListener(listener) {}

    /// for omitted method parameter: uninitialized mListener
    LLListenerOrPumpName() {}

    /// test for validity
    operator bool() const { return bool(mListener); }
    bool operator! () const { return ! mListener; }

    /// explicit accessor
    const LLEventListener& getListener() const { return *mListener; }

    /// implicit conversion to LLEventListener
    operator LLEventListener() const { return *mListener; }

    /// allow calling directly
    bool operator()(const LLSD& event) const;

    /// exception if you try to call when empty
    struct Empty: public std::runtime_error
    {
        Empty(const std::string& what):
            std::runtime_error(std::string("LLListenerOrPumpName::Empty: ") + what) {}
    };

private:
    boost::optional<LLEventListener> mListener;
};

/*****************************************************************************
*   LLEventPumps
*****************************************************************************/
class LLEventPump;

/**
 * LLEventPumps is a Singleton manager through which one typically accesses
 * this subsystem.
 */
class LL_COMMON_API LLEventPumps: public LLSingleton<LLEventPumps>
{
    friend class LLSingleton<LLEventPumps>;
public:
    /**
     * Find or create an LLEventPump instance with a specific name. We return
     * a reference so there's no question about ownership. obtain() @em finds
     * an instance without conferring @em ownership.
     */
    LLEventPump& obtain(const std::string& name);
    /**
     * Flush all known LLEventPump instances
     */
    void flush();

    /**
     * Reset all known LLEventPump instances
     * workaround for DEV-35406 crash on shutdown
     */
    void reset();

private:
    friend class LLEventPump;
    /**
     * Register a new LLEventPump instance (internal)
     */
    std::string registerNew(const LLEventPump&, const std::string& name, bool tweak);
    /**
     * Unregister a doomed LLEventPump instance (internal)
     */
    void unregister(const LLEventPump&);

private:
    LLEventPumps();
    ~LLEventPumps();

testable:
    // Map of all known LLEventPump instances, whether or not we instantiated
    // them. We store a plain old LLEventPump* because this map doesn't claim
    // ownership of the instances. Though the common usage pattern is to
    // request an instance using obtain(), it's fair to instantiate an
    // LLEventPump subclass statically, as a class member, on the stack or on
    // the heap. In such cases, the instantiating party is responsible for its
    // lifespan.
    typedef std::map<std::string, LLEventPump*> PumpMap;
    PumpMap mPumpMap;
    // Set of all LLEventPumps we instantiated. Membership in this set means
    // we claim ownership, and will delete them when this LLEventPumps is
    // destroyed.
    typedef std::set<LLEventPump*> PumpSet;
    PumpSet mOurPumps;
    // LLEventPump names that should be instantiated as LLEventQueue rather
    // than as LLEventStream
    typedef std::set<std::string> PumpNames;
    PumpNames mQueueNames;
};

/*****************************************************************************
*   details
*****************************************************************************/
namespace LLEventDetail
{
    /// Any callable capable of connecting an LLEventListener to an
    /// LLStandardSignal to produce an LLBoundListener can be mapped to this
    /// signature.
    typedef boost::function<LLBoundListener(const LLEventListener&)> ConnectFunc;

    /// overload of visit_and_connect() when we have a string identifier available
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const std::string& name,
                                      const LISTENER& listener,
                                      const ConnectFunc& connect_func);
    /**
     * Utility template function to use Visitor appropriately
     *
     * @param listener Callable to connect, typically a boost::bind()
     * expression. This will be visited by Visitor using boost::visit_each().
     * @param connect_func Callable that will connect() @a listener to an
     * LLStandardSignal, returning LLBoundListener.
     */
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const LISTENER& listener,
                                      const ConnectFunc& connect_func)
    {
        return visit_and_connect("", listener, connect_func);
    }
} // namespace LLEventDetail

/*****************************************************************************
*   LLEventTrackable
*****************************************************************************/
/**
 * LLEventTrackable wraps boost::signals2::trackable, which resembles
 * boost::trackable. Derive your listener class from LLEventTrackable instead,
 * and use something like
 * <tt>LLEventPump::listen(boost::bind(&YourTrackableSubclass::method,
 * instance, _1))</tt>. This will implicitly disconnect when the object
 * referenced by @c instance is destroyed.
 *
 * @note
 * LLEventTrackable doesn't address a couple of cases:
 * * Object destroyed during call
 *   - You enter a slot call in thread A.
 *   - Thread B destroys the object, which of course disconnects it from any
 *     future slot calls.
 *   - Thread A's call uses 'this', which now refers to a defunct object.
 *     Undefined behavior results.
 * * Call during destruction
 *   - @c MySubclass is derived from LLEventTrackable.
 *   - @c MySubclass registers one of its own methods using
 *     <tt>LLEventPump::listen()</tt>.
 *   - The @c MySubclass object begins destruction. <tt>~MySubclass()</tt>
 *     runs, destroying state specific to the subclass. (For instance, a
 *     <tt>Foo*</tt> data member is <tt>delete</tt>d but not zeroed.)
 *   - The listening method will not be disconnected until
 *     <tt>~LLEventTrackable()</tt> runs.
 *   - Before we get there, another thread posts data to the @c LLEventPump
 *     instance, calling the @c MySubclass method.
 *   - The method in question relies on valid @c MySubclass state. (For
 *     instance, it attempts to dereference the <tt>Foo*</tt> pointer that was
 *     <tt>delete</tt>d but not zeroed.)
 *   - Undefined behavior results.
 * If you suspect you may encounter any such scenario, you're better off
 * managing the lifespan of your object with <tt>boost::shared_ptr</tt>.
 * Passing <tt>LLEventPump::listen()</tt> a <tt>boost::bind()</tt> expression
 * involving a <tt>boost::weak_ptr<Foo></tt> is recognized specially, engaging
 * thread-safe Boost.Signals2 machinery.
 */
typedef boost::signals2::trackable LLEventTrackable;

/*****************************************************************************
*   LLEventPump
*****************************************************************************/
/**
 * LLEventPump is the base class interface through which we access the
 * concrete subclasses LLEventStream and LLEventQueue.
 *
 * @NOTE
 * LLEventPump derives from LLEventTrackable so that when you "chain"
 * LLEventPump instances together, they will automatically disconnect on
 * destruction. Please see LLEventTrackable documentation for situations in
 * which this may be perilous across threads.
 */
class LL_COMMON_API LLEventPump: public LLEventTrackable
{
public:
    /**
     * Exception thrown by LLEventPump(). You are trying to instantiate an
     * LLEventPump (subclass) using the same name as some other instance, and
     * you didn't pass <tt>tweak=true</tt> to permit it to generate a unique
     * variant.
     */
    struct DupPumpName: public std::runtime_error
    {
        DupPumpName(const std::string& what):
            std::runtime_error(std::string("DupPumpName: ") + what) {}
    };

    /**
     * Instantiate an LLEventPump (subclass) with the string name by which it
     * can be found using LLEventPumps::obtain().
     *
     * If you pass (or default) @a tweak to @c false, then a duplicate name
     * will throw DupPumpName. This won't happen if LLEventPumps::obtain()
     * instantiates the LLEventPump, because obtain() uses find-or-create
     * logic. It can only happen if you instantiate an LLEventPump in your own
     * code -- and a collision with the name of some other LLEventPump is
     * likely to cause much more subtle problems!
     *
     * When you hand-instantiate an LLEventPump, consider passing @a tweak as
     * @c true. This directs LLEventPump() to append a suffix to the passed @a
     * name to make it unique. You can retrieve the adjusted name by calling
     * getName() on your new instance.
     */
    LLEventPump(const std::string& name, bool tweak=false);
    virtual ~LLEventPump();

    /// group exceptions thrown by listen(). We use exceptions because these
    /// particular errors are likely to be coding errors, found and fixed by
    /// the developer even before preliminary checkin.
    struct ListenError: public std::runtime_error
    {
        ListenError(const std::string& what): std::runtime_error(what) {}
    };
    /**
     * exception thrown by listen(). You are attempting to register a
     * listener on this LLEventPump using the same listener name as an
     * already-registered listener.
     */
    struct DupListenerName: public ListenError
    {
        DupListenerName(const std::string& what):
            ListenError(std::string("DupListenerName: ") + what)
        {}
    };
    /**
     * exception thrown by listen(). The order dependencies specified for your
     * listener are incompatible with existing listeners.
     *
     * Consider listener "a" which specifies before "b" and "b" which
     * specifies before "c". You are now attempting to register "c" before
     * "a". There is no order that can satisfy all constraints.
     */
    struct Cycle: public ListenError
    {
        Cycle(const std::string& what): ListenError(std::string("Cycle: ") + what) {}
    };
    /**
     * exception thrown by listen(). This one means that your new listener
     * would force a change to the order of previously-registered listeners,
     * and we don't have a good way to implement that.
     *
     * Consider listeners "some", "other" and "third". "some" and "other" are
     * registered earlier without specifying relative order, so "other"
     * happens to be first. Now you attempt to register "third" after "some"
     * and before "other". Whoops, that would require swapping "some" and
     * "other", which we can't do. Instead we throw this exception.
     *
     * It may not be possible to change the registration order so we already
     * know "third"s order requirement by the time we register the second of
     * "some" and "other". A solution would be to specify that "some" must
     * come before "other", or equivalently that "other" must come after
     * "some".
     */
    struct OrderChange: public ListenError
    {
        OrderChange(const std::string& what): ListenError(std::string("OrderChange: ") + what) {}
    };

    /// used by listen()
    typedef std::vector<std::string> NameList;
    /// convenience placeholder for when you explicitly want to pass an empty
    /// NameList
    const static NameList empty;

    /// Get this LLEventPump's name
    std::string getName() const { return mName; }

    /**
     * Register a new listener with a unique name. Specify an optional list
     * of other listener names after which this one must be called, likewise
     * an optional list of other listener names before which this one must be
     * called. The other listeners mentioned need not yet be registered
     * themselves. listen() can throw any ListenError; see ListenError
     * subclasses.
     *
     * The listener name must be unique among active listeners for this
     * LLEventPump, else you get DupListenerName. If you don't care to invent
     * a name yourself, use inventName(). (I was tempted to recognize e.g. ""
     * and internally generate a distinct name for that case. But that would
     * handle badly the scenario in which you want to add, remove, re-add,
     * etc. the same listener: each new listen() call would necessarily
     * perform a new dependency sort. Assuming you specify the same
     * after/before lists each time, using inventName() when you first
     * instantiate your listener, then passing the same name on each listen()
     * call, allows us to optimize away the second and subsequent dependency
     * sorts.
     *
     * If (as is typical) you pass a <tt>boost::bind()</tt> expression as @a
     * listener, listen() will inspect the components of that expression. If a
     * bound object matches any of several cases, the connection will
     * automatically be disconnected when that object is destroyed.
     *
     * * You bind a <tt>boost::weak_ptr</tt>.
     * * Binding a <tt>boost::shared_ptr</tt> that way would ensure that the
     *   referenced object would @em never be destroyed, since the @c
     *   shared_ptr stored in the LLEventPump would remain an outstanding
     *   reference. Use the weaken() function to convert your @c shared_ptr to
     *   @c weak_ptr. Because this is easy to forget, binding a @c shared_ptr
     *   will produce a compile error (@c BOOST_STATIC_ASSERT failure).
     * * You bind a simple pointer or reference to an object derived from
     *   <tt>boost::enable_shared_from_this</tt>. (UNDER CONSTRUCTION)
     * * You bind a simple pointer or reference to an object derived from
     *   LLEventTrackable. Unlike the cases described above, though, this is
     *   vulnerable to a couple of cross-thread race conditions, as described
     *   in the LLEventTrackable documentation.
     */
    template <typename LISTENER>
    LLBoundListener listen(const std::string& name, const LISTENER& listener,
                           const NameList& after=NameList(),
                           const NameList& before=NameList())
    {
        // Examine listener, using our listen_impl() method to make the
        // actual connection.
        // This is why listen() is a template. Conversion from boost::bind()
        // to LLEventListener performs type erasure, so it's important to look
        // at the boost::bind object itself before that happens.
        return LLEventDetail::visit_and_connect(name,
                                                listener,
                                                boost::bind(&LLEventPump::listen_impl,
                                                            this,
                                                            name,
                                                            _1,
                                                            after,
                                                            before));
    }

    /// Get the LLBoundListener associated with the passed name (dummy
    /// LLBoundListener if not found)
    virtual LLBoundListener getListener(const std::string& name) const;
    /**
     * Instantiate one of these to block an existing connection:
     * @code
     * { // in some local scope
     *     LLEventPump::Blocker block(someLLBoundListener);
     *     // code that needs the connection blocked
     * } // unblock the connection again
     * @endcode
     */
    typedef boost::signals2::shared_connection_block Blocker;
    /// Unregister a listener by name. Prefer this to
    /// <tt>getListener(name).disconnect()</tt> because stopListening() also
    /// forgets this name.
    virtual void stopListening(const std::string& name);
    /// Post an event to all listeners. The @c bool return is only meaningful
    /// if the underlying leaf class is LLEventStream -- beware of relying on
    /// it too much! Truthfully, we return @c bool mostly to permit chaining
    /// one LLEventPump as a listener on another.
    virtual bool post(const LLSD&) = 0;
    /// Enable/disable: while disabled, silently ignore all post() calls
    virtual void enable(bool enabled=true) { mEnabled = enabled; }
    /// query
    virtual bool enabled() const { return mEnabled; }

    /// Generate a distinct name for a listener -- see listen()
    static std::string inventName(const std::string& pfx="listener");

private:
    friend class LLEventPumps;
    /// flush queued events
    virtual void flush() {}

    virtual void reset();

private:
    virtual LLBoundListener listen_impl(const std::string& name, const LLEventListener&,
                                        const NameList& after,
                                        const NameList& before);
    std::string mName;

protected:
    /// implement the dispatching
    boost::shared_ptr<LLStandardSignal> mSignal;

    /// valve open?
    bool mEnabled;
    /// Map of named listeners. This tracks the listeners that actually exist
    /// at this moment. When we stopListening(), we discard the entry from
    /// this map.
    typedef std::map<std::string, boost::signals2::connection> ConnectionMap;
    ConnectionMap mConnections;
    typedef LLDependencies<std::string, float> DependencyMap;
    /// Dependencies between listeners. For each listener, track the float
    /// used to establish its place in mSignal's order. This caches all the
    /// listeners that have ever registered; stopListening() does not discard
    /// the entry from this map. This is to avoid a new dependency sort if the
    /// same listener with the same dependencies keeps hopping on and off this
    /// LLEventPump.
    DependencyMap mDeps;
};

/*****************************************************************************
*   LLEventStream
*****************************************************************************/
/**
 * LLEventStream is a thin wrapper around LLStandardSignal. Posting an
 * event immediately calls all registered listeners.
 */
class LL_COMMON_API LLEventStream: public LLEventPump
{
public:
    LLEventStream(const std::string& name, bool tweak=false): LLEventPump(name, tweak) {}
    virtual ~LLEventStream() {}

    /// Post an event to all listeners
    virtual bool post(const LLSD& event);
};

/*****************************************************************************
*   LLEventQueue
*****************************************************************************/
/**
 * LLEventQueue isa LLEventPump whose post() method defers calling registered
 * listeners until flush() is called.
 */
class LL_COMMON_API LLEventQueue: public LLEventPump
{
public:
    LLEventQueue(const std::string& name, bool tweak=false): LLEventPump(name, tweak) {}
    virtual ~LLEventQueue() {}

    /// Post an event to all listeners
    virtual bool post(const LLSD& event);

private:
    /// flush queued events
    virtual void flush();

private:
    typedef std::deque<LLSD> EventQueue;
    EventQueue mEventQueue;
};

/*****************************************************************************
*   LLReqID
*****************************************************************************/
/**
 * This class helps the implementer of a given event API to honor the
 * ["reqid"] convention. By this convention, each event API stamps into its
 * response LLSD a ["reqid"] key whose value echoes the ["reqid"] value, if
 * any, from the corresponding request.
 *
 * This supports an (atypical, but occasionally necessary) use case in which
 * two or more asynchronous requests are multiplexed onto the same ["reply"]
 * LLEventPump. Since the response events could arrive in arbitrary order, the
 * caller must be able to demux them. It does so by matching the ["reqid"]
 * value in each response with the ["reqid"] value in the corresponding
 * request.
 *
 * It is the caller's responsibility to ensure distinct ["reqid"] values for
 * that case. Though LLSD::UUID is guaranteed to work, it might be overkill:
 * the "namespace" of unique ["reqid"] values is simply the set of requests
 * specifying the same ["reply"] LLEventPump name.
 *
 * Making a given event API echo the request's ["reqid"] into the response is
 * nearly trivial. This helper is mostly for mnemonic purposes, to serve as a
 * place to put these comments. We hope that each time a coder implements a
 * new event API based on some existing one, s/he will say, "Huh, what's an
 * LLReqID?" and look up this material.
 *
 * The hardest part about the convention is deciding where to store the
 * ["reqid"] value. Ironically, LLReqID can't help with that: you must store
 * an LLReqID instance in whatever storage will persist until the reply is
 * sent. For example, if the request ultimately ends up using a Responder
 * subclass, storing an LLReqID instance in the Responder works.
 *
 * @note
 * The @em implementer of an event API must honor the ["reqid"] convention.
 * However, the @em caller of an event API need only use it if s/he is sharing
 * the same ["reply"] LLEventPump for two or more asynchronous event API
 * requests.
 *
 * In most cases, it's far easier for the caller to instantiate a local
 * LLEventStream and pass its name to the event API in question. Then it's
 * perfectly reasonable not to set a ["reqid"] key in the request, ignoring
 * the @c isUndefined() ["reqid"] value in the response.
 */
class LL_COMMON_API LLReqID
{
public:
    /**
     * If you have the request in hand at the time you instantiate the
     * LLReqID, pass that request to extract its ["reqid"].
 */
    LLReqID(const LLSD& request):
        mReqid(request["reqid"])
    {}
    /// If you don't yet have the request, use setFrom() later.
    LLReqID() {}

    /// Extract and store the ["reqid"] value from an incoming request.
    void setFrom(const LLSD& request)
    {
        mReqid = request["reqid"];
    }

    /// Set ["reqid"] key into a pending response LLSD object.
    void stamp(LLSD& response) const;

    /// Make a whole new response LLSD object with our ["reqid"].
    LLSD makeResponse() const
    {
        LLSD response;
        stamp(response);
        return response;
    }

    /// Not really sure of a use case for this accessor...
    LLSD getReqID() const { return mReqid; }

private:
    LLSD mReqid;
};

/**
 * Conventionally send a reply to a request event.
 *
 * @a reply is the LLSD reply event to send
 * @a request is the corresponding LLSD request event
 * @a replyKey is the key in the @a request event, conventionally ["reply"],
 * whose value is the name of the LLEventPump on which to send the reply.
 *
 * Before sending the reply event, sendReply() copies the ["reqid"] item from
 * the request to the reply.
 */
LL_COMMON_API bool sendReply(const LLSD& reply, const LLSD& request,
                             const std::string& replyKey="reply");

/**
 * Base class for LLListenerWrapper. See visit_and_connect() and llwrap(). We
 * provide virtual @c accept_xxx() methods, customization points allowing a
 * subclass access to certain data visible at LLEventPump::listen() time.
 * Example subclass usage:
 *
 * @code
 * myEventPump.listen("somename",
 *                    llwrap<MyListenerWrapper>(boost::bind(&MyClass::method, instance, _1)));
 * @endcode
 *
 * Because of the anticipated usage (note the anonymous temporary
 * MyListenerWrapper instance in the example above), the @c accept_xxx()
 * methods must be @c const.
 */
class LL_COMMON_API LLListenerWrapperBase
{
public:
    /// New instance. The accept_xxx() machinery makes it important to use
    /// shared_ptrs for our data. Many copies of this object are made before
    /// the instance that actually ends up in the signal, yet accept_xxx()
    /// will later be called on the @em original instance. All copies of the
    /// same original instance must share the same data.
    LLListenerWrapperBase():
        mName(new std::string),
        mConnection(new LLBoundListener)
    {
    }
	
    /// Copy constructor. Copy shared_ptrs to original instance data.
    LLListenerWrapperBase(const LLListenerWrapperBase& that):
        mName(that.mName),
        mConnection(that.mConnection)
    {
    }
	virtual ~LLListenerWrapperBase() {}

    /// Ask LLEventPump::listen() for the listener name
    virtual void accept_name(const std::string& name) const
    {
        *mName = name;
    }

    /// Ask LLEventPump::listen() for the new connection
    virtual void accept_connection(const LLBoundListener& connection) const
    {
        *mConnection = connection;
    }

protected:
    /// Listener name.
    boost::shared_ptr<std::string> mName;
    /// Connection.
    boost::shared_ptr<LLBoundListener> mConnection;
};

/*****************************************************************************
*   Underpinnings
*****************************************************************************/
/**
 * We originally provided a suite of overloaded
 * LLEventTrackable::listenTo(LLEventPump&, ...) methods that would call
 * LLEventPump::listen(...) and then pass the returned LLBoundListener to
 * LLEventTrackable::track(). This was workable but error-prone: the coder
 * must remember to call listenTo() rather than the more straightforward
 * listen() method.
 *
 * Now we publish only the single canonical listen() method, so there's a
 * uniform mechanism. Having a single way to do this is good, in that there's
 * no question in the coder's mind which of several alternatives to choose.
 *
 * To support automatic connection management, we use boost::visit_each
 * (http://www.boost.org/doc/libs/1_37_0/doc/html/boost/visit_each.html) to
 * inspect each argument of a boost::bind expression. (Although the visit_each
 * mechanism was first introduced with the original Boost.Signals library, it
 * was only later documented.)
 *
 * Cases:
 * * At least one of the function's arguments is a boost::weak_ptr<T>. Pass
 *   the corresponding shared_ptr to slot_type::track(). Ideally that would be
 *   the object whose method we want to call, but in fact we do the same for
 *   any weak_ptr we might find among the bound arguments. If we're passing
 *   our bound method a weak_ptr to some object, wouldn't the destruction of
 *   that object invalidate the call? So we disconnect automatically when any
 *   such object is destroyed. This is the mechanism preferred by boost::
 *   signals2.
 * * One of the functions's arguments is a boost::shared_ptr<T>. This produces
 *   a compile error: the bound copy of the shared_ptr stored in the
 *   boost_bind object stored in the signal object would make the referenced
 *   T object immortal. We provide a weaken() function. Pass
 *   weaken(your_shared_ptr) instead. (We can inspect, but not modify, the
 *   boost::bind object. Otherwise we'd replace the shared_ptr with weak_ptr
 *   implicitly and just proceed.)
 * * One of the function's arguments is a plain pointer/reference to an object
 *   derived from boost::enable_shared_from_this. We assume that this object
 *   is managed using boost::shared_ptr, so we implicitly extract a shared_ptr
 *   and track that. (UNDER CONSTRUCTION)
 * * One of the function's arguments is derived from LLEventTrackable. Pass
 *   the LLBoundListener to its LLEventTrackable::track(). This is vulnerable
 *   to a couple different race conditions, as described in LLEventTrackable
 *   documentation. (NOTE: Now that LLEventTrackable is a typedef for
 *   boost::signals2::trackable, the Signals2 library handles this itself, so
 *   our visitor needs no special logic for this case.)
 * * Any other argument type is irrelevant to automatic connection management.
 */

namespace LLEventDetail
{
    template <typename F>
    const F& unwrap(const F& f) { return f; }

    template <typename F>
    const F& unwrap(const boost::reference_wrapper<F>& f) { return f.get(); }

    // Most of the following is lifted from the Boost.Signals use of
    // visit_each.
    template<bool Cond> struct truth {};

    /**
     * boost::visit_each() Visitor, used on a template argument <tt>const F&
     * f</tt> as follows (see visit_and_connect()):
     * @code
     * LLEventListener listener(f);
     * Visitor visitor(listener); // bind listener so it can track() shared_ptrs
     * using boost::visit_each;   // allow unqualified visit_each() call for ADL
     * visit_each(visitor, unwrap(f));
     * @endcode
     */
    class Visitor
    {
    public:
        /**
         * Visitor binds a reference to LLEventListener so we can track() any
         * shared_ptrs we find in the argument list.
         */
        Visitor(LLEventListener& listener):
            mListener(listener)
        {
        }

        /**
         * boost::visit_each() calls this method for each component of a
         * boost::bind() expression.
         */
        template <typename T>
        void operator()(const T& t) const
        {
            decode(t, 0);
        }

    private:
        // decode() decides between a reference wrapper and anything else
        // boost::ref() variant
        template<typename T>
        void decode(const boost::reference_wrapper<T>& t, int) const
        {
//          add_if_trackable(t.get_pointer());
        }

        // decode() anything else
        template<typename T>
        void decode(const T& t, long) const
        {
            typedef truth<(boost::is_pointer<T>::value)> is_a_pointer;
            maybe_get_pointer(t, is_a_pointer());
        }

        // maybe_get_pointer() decides between a pointer and a non-pointer
        // plain pointer variant
        template<typename T>
        void maybe_get_pointer(const T& t, truth<true>) const
        {
//          add_if_trackable(t);
        }

        // shared_ptr variant
        template<typename T>
        void maybe_get_pointer(const boost::shared_ptr<T>& t, truth<false>) const
        {
            // If we have a shared_ptr to this object, it doesn't matter
            // whether the object is derived from LLEventTrackable, so no
            // further analysis of T is needed.
//          mListener.track(t);

            // Make this case illegal. Passing a bound shared_ptr to
            // slot_type::track() is useless, since the bound shared_ptr will
            // keep the object alive anyway! Force the coder to cast to weak_ptr.

            // Trivial as it is, make the BOOST_STATIC_ASSERT() condition
            // dependent on template param so the macro is only evaluated if
            // this method is in fact instantiated, as described here:
            // http://www.boost.org/doc/libs/1_34_1/doc/html/boost_staticassert.html

            // ATTENTION: Don't bind a shared_ptr<anything> using
            // LLEventPump::listen(boost::bind()). Doing so captures a copy of
            // the shared_ptr, making the referenced object effectively
            // immortal. Use the weaken() function, e.g.:
            // somepump.listen(boost::bind(...weaken(my_shared_ptr)...));
            // This lets us automatically disconnect when the referenced
            // object is destroyed.
            BOOST_STATIC_ASSERT(sizeof(T) == 0);
        }

        // weak_ptr variant
        template<typename T>
        void maybe_get_pointer(const boost::weak_ptr<T>& t, truth<false>) const
        {
            // If we have a weak_ptr to this object, it doesn't matter
            // whether the object is derived from LLEventTrackable, so no
            // further analysis of T is needed.
            mListener.track(t);
//          std::cout << "Found weak_ptr<" << typeid(T).name() << ">!\n";
        }

#if 0
        // reference to anything derived from boost::enable_shared_from_this
        template <typename T>
        inline void maybe_get_pointer(const boost::enable_shared_from_this<T>& ct,
                                      truth<false>) const
        {
            // Use the slot_type::track(shared_ptr) mechanism. Cast away
            // const-ness because (in our code base anyway) it's unusual
            // to find shared_ptr<const T>.
            boost::enable_shared_from_this<T>&
                t(const_cast<boost::enable_shared_from_this<T>&>(ct));
            std::cout << "Capturing shared_from_this()" << std::endl;
            boost::shared_ptr<T> sp(t.shared_from_this());
/*==========================================================================*|
            std::cout << "Capturing weak_ptr" << std::endl;
            boost::weak_ptr<T> wp(sp);
|*==========================================================================*/
            std::cout << "Tracking shared__ptr" << std::endl;
            mListener.track(sp);
        }
#endif

        // non-pointer variant
        template<typename T>
        void maybe_get_pointer(const T& t, truth<false>) const
        {
            // Take the address of this object, because the object itself may be
            // trackable
//          add_if_trackable(boost::addressof(t));
        }

/*==========================================================================*|
        // add_if_trackable() adds LLEventTrackable objects to mTrackables
        inline void add_if_trackable(const LLEventTrackable* t) const
        {
            if (t)
            {
            }
        }

        // pointer to anything not an LLEventTrackable subclass
        inline void add_if_trackable(const void*) const
        {
        }

        // pointer to free function
        // The following construct uses the preprocessor to generate
        // add_if_trackable() overloads accepting pointer-to-function taking
        // 0, 1, ..., LLEVENTS_LISTENER_ARITY parameters of arbitrary type.
#define BOOST_PP_LOCAL_MACRO(n)                                     \
        template <typename R                                        \
                  BOOST_PP_COMMA_IF(n)                              \
                  BOOST_PP_ENUM_PARAMS(n, typename T)>              \
        inline void                                                 \
        add_if_trackable(R (*)(BOOST_PP_ENUM_PARAMS(n, T))) const   \
        {                                                           \
        }
#define BOOST_PP_LOCAL_LIMITS (0, LLEVENTS_LISTENER_ARITY)
#include BOOST_PP_LOCAL_ITERATE()
#undef  BOOST_PP_LOCAL_MACRO
#undef  BOOST_PP_LOCAL_LIMITS
|*==========================================================================*/

        /// Bind a reference to the LLEventListener to call its track() method.
        LLEventListener& mListener;
    };

    /**
     * Utility template function to use Visitor appropriately
     *
     * @param raw_listener Callable to connect, typically a boost::bind()
     * expression. This will be visited by Visitor using boost::visit_each().
     * @param connect_funct Callable that will connect() @a raw_listener to an
     * LLStandardSignal, returning LLBoundListener.
     */
    template <typename LISTENER>
    LLBoundListener visit_and_connect(const std::string& name,
                                      const LISTENER& raw_listener,
                                      const ConnectFunc& connect_func)
    {
        // Capture the listener
        LLEventListener listener(raw_listener);
        // Define our Visitor, binding the listener so we can call
        // listener.track() if we discover any shared_ptr<Foo>.
        LLEventDetail::Visitor visitor(listener);
        // Allow unqualified visit_each() call for ADL
        using boost::visit_each;
        // Visit each component of a boost::bind() expression. Pass
        // 'raw_listener', our template argument, rather than 'listener' from
        // which type details have been erased. unwrap() comes from
        // Boost.Signals, in case we were passed a boost::ref().
        visit_each(visitor, LLEventDetail::unwrap(raw_listener));
        // Make the connection using passed function.
        LLBoundListener connection(connect_func(listener));
        // If the LISTENER is an LLListenerWrapperBase subclass, pass it the
        // desired information. It's important that we pass the raw_listener
        // so the compiler can make decisions based on its original type.
        const LLListenerWrapperBase* lwb =
            ll_template_cast<const LLListenerWrapperBase*>(&raw_listener);
        if (lwb)
        {
            lwb->accept_name(name);
            lwb->accept_connection(connection);
        }
        // In any case, show new connection to caller.
        return connection;
    }
} // namespace LLEventDetail

// Somewhat to my surprise, passing boost::bind(...boost::weak_ptr<T>...) to
// listen() fails in Boost code trying to instantiate LLEventListener (i.e.
// LLStandardSignal::slot_type) because the boost::get_pointer() utility function isn't
// specialized for boost::weak_ptr. This remedies that omission.
namespace boost
{
    template <typename T>
    T* get_pointer(const weak_ptr<T>& ptr) { return shared_ptr<T>(ptr).get(); }
}

/// Since we forbid use of listen(boost::bind(...shared_ptr<T>...)), provide an
/// easy way to cast to the corresponding weak_ptr.
template <typename T>
boost::weak_ptr<T> weaken(const boost::shared_ptr<T>& ptr)
{
    return boost::weak_ptr<T>(ptr);
}

#endif /* ! defined(LL_LLEVENTS_H) */
