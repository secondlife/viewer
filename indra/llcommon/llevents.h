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
#include <functional>

#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/utility.hpp>        // noncopyable
#include <boost/optional/optional.hpp>
#include <boost/visit_each.hpp>
#include <boost/ref.hpp>            // reference_wrapper
#include <boost/type_traits/is_pointer.hpp>
#include <boost/static_assert.hpp>
#include "llsd.h"
#include "llsingleton.h"
#include "lldependencies.h"
#include "llstl.h"
#include "llexception.h"
#include "llhandle.h"
#include "llcoros.h"

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
            try
            {
                if (*si)
                {
                    return true;
                }
            }
            catch (const LLContinueError&)
            {
                // We catch LLContinueError here because an LLContinueError-
                // based exception means the viewer as a whole should carry on
                // to the best of our ability. Therefore subsequent listeners
                // on the same LLEventPump should still receive this event.

                // The iterator passed to a boost::signals2 Combiner is very
                // clever, but provides no contextual information. We would
                // very much like to be able to log the name of the LLEventPump
                // plus the name of this particular listener, but alas.
                LOG_UNHANDLED_EXCEPTION("LLEventPump");
            }
            // We do NOT catch (...) here because we might as well let it
            // propagate out to the generic handler. If we were able to log
            // context information here, that would be great, but we can't, so
            // there's no point.
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
    struct Empty: public LLException
    {
        Empty(const std::string& what): LLException("LLListenerOrPumpName::Empty: " + what) {}
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
// LLEventPumps isa LLHandleProvider only for (hopefully rare) long-lived
// class objects that must refer to this class late in their lifespan, say in
// the destructor. Specifically, the case that matters is a possible reference
// after LLEventPumps::deleteSingleton(). (Lingering LLEventPump instances are
// capable of this.) In that case, instead of calling LLEventPumps::instance()
// again -- resurrecting the deleted LLSingleton -- store an
// LLHandle<LLEventPumps> and test it before use.
class LL_COMMON_API LLEventPumps: public LLSingleton<LLEventPumps>,
                                  public LLHandleProvider<LLEventPumps>
{
    LLSINGLETON(LLEventPumps);
public:
    /**
     * Find or create an LLEventPump instance with a specific name. We return
     * a reference so there's no question about ownership. obtain() @em finds
     * an instance without conferring @em ownership.
     */
    LLEventPump& obtain(const std::string& name);

    /// exception potentially thrown by make()
    struct BadType: public LLException
    {
        BadType(const std::string& what): LLException("BadType: " + what) {}
    };

    /**
     * Create an LLEventPump with suggested name (optionally of specified
     * LLEventPump subclass type). As with obtain(), LLEventPumps owns the new
     * instance.
     *
     * As with LLEventPump's constructor, make() could throw
     * LLEventPump::DupPumpName unless you pass tweak=true.
     *
     * As with a hand-constructed LLEventPump subclass, if you pass
     * tweak=true, the tweaked name can be obtained by LLEventPump::getName().
     *
     * Pass empty type to get the default LLEventStream.
     *
     * If you pass an unrecognized type string, make() throws BadType.
     */
    LLEventPump& make(const std::string& name, bool tweak=false,
                      const std::string& type=std::string());

    /// function passed to registerTypeFactory()
    typedef std::function<LLEventPump*(const std::string& name, bool tweak, const std::string& type)> TypeFactory;

    /**
     * Register a TypeFactory for use with make(). When make() is called with
     * the specified @a type string, call @a factory(name, tweak, type) to
     * instantiate it.
     *
     * Returns true if successfully registered, false if there already exists
     * a TypeFactory for the specified @a type name.
     */
    bool registerTypeFactory(const std::string& type, const TypeFactory& factory);
    void unregisterTypeFactory(const std::string& type);

    /// function passed to registerPumpFactory()
    typedef std::function<LLEventPump*(const std::string&)> PumpFactory;

    /**
     * Register a PumpFactory for use with obtain(). When obtain() is called
     * with the specified @a name string, if an LLEventPump with the specified
     * @a name doesn't already exist, call @a factory(name) to instantiate it.
     *
     * Returns true if successfully registered, false if there already exists
     * a factory override for the specified @a name.
     *
     * PumpFactory does not support @a tweak because it's only called when
     * <i>that particular</i> @a name is passed to obtain(). Bear in mind that
     * <tt>obtain(name)</tt> might still bypass the caller's PumpFactory for a
     * couple different reasons:
     *
     * * registerPumpFactory() returns false because there's already a factory
     *   override for the specified @name
     * * between a successful <tt>registerPumpFactory(name)</tt> call (returns
     *   true) and a call to <tt>obtain(name)</tt>, someone explicitly
     *   instantiated an LLEventPump(name), so obtain(name) returned that.
     */
    bool registerPumpFactory(const std::string& name, const PumpFactory& factory);
    void unregisterPumpFactory(const std::string& name);

    /**
     * Find the named LLEventPump instance. If it exists post the message to it.
     * If the pump does not exist, do nothing.
     *
     * returns the result of the LLEventPump::post. If no pump exists returns false.
     *
     * This is syntactically similar to LLEventPumps::instance().post(name, message),
     * however if the pump does not already exist it will not be created.
     */
    bool post(const std::string&, const LLSD&);

    /**
     * Flush all known LLEventPump instances
     */
    void flush();

    /**
     * Disconnect listeners from all known LLEventPump instances
     */
    void clear();

    /**
     * Reset all known LLEventPump instances
     * workaround for DEV-35406 crash on shutdown
     */
    void reset(bool log_pumps = false);

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
    // for make(), map string type name to LLEventPump subclass factory function
    typedef std::map<std::string, TypeFactory> TypeFactories;
    // Data used by make().
    // One might think mFactories and mTypes could reasonably be static. So
    // they could -- if not for the fact that make() or obtain() might be
    // called before this module's static variables have been initialized.
    // This is why we use singletons in the first place.
    TypeFactories mFactories;

    // for obtain(), map desired string instance name to string type when
    // obtain() must create the instance
    typedef std::map<std::string, std::string> InstanceTypes;
    InstanceTypes mTypes;
};

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
 */
typedef boost::signals2::trackable LLEventTrackable;

/*****************************************************************************
*   LLEventPump
*****************************************************************************/
/**
 * LLEventPump is the base class interface through which we access the
 * concrete subclasses such as LLEventStream.
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
    static const std::string ANONYMOUS; // constant for anonymous listeners.

    /**
     * Exception thrown by LLEventPump(). You are trying to instantiate an
     * LLEventPump (subclass) using the same name as some other instance, and
     * you didn't pass <tt>tweak=true</tt> to permit it to generate a unique
     * variant.
     */
    struct DupPumpName: public LLException
    {
        DupPumpName(const std::string& what): LLException("DupPumpName: " + what) {}
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
    struct ListenError: public LLException
    {
        ListenError(const std::string& what): LLException(what) {}
    };
    /**
     * exception thrown by listen(). You are attempting to register a
     * listener on this LLEventPump using the same listener name as an
     * already-registered listener.
     */
    struct DupListenerName: public ListenError
    {
        DupListenerName(const std::string& what): ListenError("DupListenerName: " + what) {}
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
        Cycle(const std::string& what): ListenError("Cycle: " + what) {}
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
        OrderChange(const std::string& what): ListenError("OrderChange: " + what) {}
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
     * If name is set to LLEventPump::ANONYMOUS listen will bypass the entire
     * dependency and ordering calculation. In this case, it is critical that
     * the result be assigned to a LLTempBoundListener or the listener is
     * manually disconnected when no longer needed since there will be no
     * way to later find and disconnect this listener manually.
     */
    LLBoundListener listen(const std::string& name,
                           const LLEventListener& listener,
                           const NameList& after=NameList(),
                           const NameList& before=NameList())
    {
        return listen_impl(name, listener, after, before);
    }

    /// Get the LLBoundListener associated with the passed name (dummy
    /// LLBoundListener if not found)
    virtual LLBoundListener getListener(const std::string& name);
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

    /// flush queued events
    virtual void flush() {}

private:
    friend class LLEventPumps;
    virtual void clear();
    virtual void reset();



private:
    // must precede mName; see LLEventPump::LLEventPump()
    LLHandle<LLEventPumps> mRegistry;

    std::string mName;
    LLCoros::Mutex mConnectionListMutex;

protected:
    virtual LLBoundListener listen_impl(const std::string& name, const LLEventListener&,
                                        const NameList& after,
                                        const NameList& before);

    /// implement the dispatching
    std::shared_ptr<LLStandardSignal> mSignal;

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
 *   LLEventMailDrop
 *****************************************************************************/
/**
 * LLEventMailDrop is a specialization of LLEventStream. Events are posted
 * normally, however if no listener returns that it has handled the event
 * (returns true), it is placed in a queue. Subsequent attaching listeners
 * will receive stored events from the queue until some listener indicates
 * that the event has been handled.
 *
 * LLEventMailDrop completely decouples the timing of post() calls from
 * listen() calls: every event posted to an LLEventMailDrop is eventually seen
 * by all listeners, until some listener consumes it. The caveat is that each
 * event *must* eventually reach a listener that will consume it, else the
 * queue will grow to arbitrary length.
 *
 * @NOTE: When using an LLEventMailDrop with an LLEventTimeout or
 * LLEventFilter attaching the filter downstream, using Timeout's constructor will
 * cause the MailDrop to discharge any of its stored events. The timeout should
 * instead be connected upstream using its listen() method.
 */
class LL_COMMON_API LLEventMailDrop : public LLEventStream
{
public:
    LLEventMailDrop(const std::string& name, bool tweak = false) : LLEventStream(name, tweak) {}
    virtual ~LLEventMailDrop() {}

    /// Post an event to all listeners
    virtual bool post(const LLSD& event) override;

    /// Remove any history stored in the mail drop.
    void discard();

protected:
    virtual LLBoundListener listen_impl(const std::string& name, const LLEventListener&,
                                        const NameList& after,
                                        const NameList& before) override;

private:
    typedef std::list<LLSD> EventList;
    EventList mEventHistory;
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

#endif /* ! defined(LL_LLEVENTS_H) */
