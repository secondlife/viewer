/**
 * @file   lazyeventapi.h
 * @author Nat Goodspeed
 * @date   2022-06-16
 * @brief  Declaring a static module-scope LazyEventAPI registers a specific
 *         LLEventAPI for future on-demand instantiation.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LAZYEVENTAPI_H)
#define LL_LAZYEVENTAPI_H

#include "apply.h"
#include "lleventapi.h"
#include "llinstancetracker.h"
#include <boost/signals2/signal.hpp>
#include <string>
#include <tuple>
#include <utility>                  // std::pair
#include <vector>

namespace LL
{
    /**
     * Bundle params we want to pass to LLEventAPI's protected constructor. We
     * package them this way so a subclass constructor can simply forward an
     * opaque reference to the LLEventAPI constructor.
     */
    // This is a class instead of a plain struct mostly so when we forward-
    // declare it we don't have to remember the distinction.
    class LazyEventAPIParams
    {
    public:
        // package the parameters used by the normal LLEventAPI constructor
        std::string name, desc, field;
        // bundle LLEventAPI::add() calls collected by LazyEventAPI::add(), so
        // the special LLEventAPI constructor we engage can "play back" those
        // add() calls
        boost::signals2::signal<void(LLEventAPI*)> init;
    };

    /**
     * LazyEventAPIBase implements most of the functionality of LazyEventAPI
     * (q.v.), but we need the LazyEventAPI template subclass so we can accept
     * the specific LLEventAPI subclass type.
     */
    // No LLInstanceTracker key: we don't need to find a specific instance,
    // LLLeapListener just needs to be able to enumerate all instances.
    class LazyEventAPIBase: public LLInstanceTracker<LazyEventAPIBase>
    {
    public:
        LazyEventAPIBase(const std::string& name, const std::string& desc,
                         const std::string& field);
        virtual ~LazyEventAPIBase();

        // Do not copy or move: once constructed, LazyEventAPIBase must stay
        // put: we bind its instance pointer into a callback.
        LazyEventAPIBase(const LazyEventAPIBase&) = delete;
        LazyEventAPIBase(LazyEventAPIBase&&) = delete;
        LazyEventAPIBase& operator=(const LazyEventAPIBase&) = delete;
        LazyEventAPIBase& operator=(LazyEventAPIBase&&) = delete;

        // capture add() calls we want to play back on LLEventAPI construction
        template <typename... ARGS>
        void add(const std::string& name, const std::string& desc, ARGS&&... rest)
        {
            // capture the metadata separately
            mOperations.push_back(std::make_pair(name, desc));
            // Use connect_extended() so the lambda is passed its own
            // connection.

            // apply() can't accept a template per se; it needs a particular
            // specialization. Specialize out here to work around a clang bug:
            // https://github.com/llvm/llvm-project/issues/41999
            auto func{ &LazyEventAPIBase::add_trampoline
                       <const std::string&, const std::string&, ARGS...> };
            // We can't bind an unexpanded parameter pack into a lambda --
            // shame really. Instead, capture all our args as a std::tuple and
            // then, in the lambda, use apply() to pass to add_trampoline().
            auto args{ std::make_tuple(name, desc, std::forward<ARGS>(rest)...) };

            mParams.init.connect_extended(
                [func, args]
                (const boost::signals2::connection& conn, LLEventAPI* instance)
                {
                    // we only need this connection once
                    conn.disconnect();
                    // apply() expects a tuple specifying ALL the arguments,
                    // so prepend instance.
                    apply(func, std::tuple_cat(std::make_tuple(instance), args));
                });
        }

        // The following queries mimic the LLEventAPI / LLEventDispatcher
        // query API.

        // Get the string name of the subject LLEventAPI
        std::string getName() const { return mParams.name; }
        // Get the documentation string
        std::string getDesc() const { return mParams.desc; }
        // Retrieve the LLSD key we use for dispatching
        std::string getDispatchKey() const { return mParams.field; }

        // operations
        using NameDesc = std::pair<std::string, std::string>;

    private:
        // metadata that might be queried by LLLeapListener
        std::vector<NameDesc> mOperations;

    public:
        using const_iterator = decltype(mOperations)::const_iterator;
        const_iterator begin() const { return mOperations.begin(); }
        const_iterator end()   const { return mOperations.end(); }
        LLSD getMetadata(const std::string& name) const;

    protected:
        // Params with which to instantiate the companion LLEventAPI subclass
        LazyEventAPIParams mParams;

    private:
        // true if we successfully registered our LLEventAPI on construction
        bool mRegistered;

        // actually instantiate the companion LLEventAPI subclass
        virtual LLEventPump* construct(const std::string& name) = 0;

        // Passing an overloaded function to any function that accepts an
        // arbitrary callable is a PITB because you have to specify the
        // correct overload. What we want is for the compiler to select the
        // correct overload, based on the carefully-wrought enable_ifs in
        // LLEventDispatcher. This (one and only) add_trampoline() method
        // exists solely to pass to LL::apply(). Once add_trampoline() is
        // called with the expanded arguments, we hope the compiler will Do
        // The Right Thing in selecting the correct LLEventAPI::add()
        // overload.
        template <typename... ARGS>
        static
        void add_trampoline(LLEventAPI* instance, ARGS&&... args)
        {
            instance->add(std::forward<ARGS>(args)...);
        }
    };

    /**
     * LazyEventAPI provides a way to register a particular LLEventAPI to be
     * instantiated on demand, that is, when its name is passed to
     * LLEventPumps::obtain().
     *
     * Derive your listener from LLEventAPI as usual, with its various
     * operation methods, but code your constructor to accept
     * <tt>(const LL::LazyEventAPIParams& params)</tt>
     * and forward that reference to (the protected)
     * <tt>LLEventAPI(const LL::LazyEventAPIParams&)</tt> constructor.
     *
     * Then derive your listener registrar from
     * <tt>LazyEventAPI<your LLEventAPI subclass></tt>. The constructor should
     * look very like a traditional LLEventAPI constructor:
     *
     * * pass (name, desc [, field]) to LazyEventAPI's constructor
     * * in the body, make a series of add() calls referencing your LLEventAPI
     *   subclass methods.
     *
     * You may use any LLEventAPI::add() methods, that is, any
     * LLEventDispatcher::add() methods. But the target methods you pass to
     * add() must belong to your LLEventAPI subclass, not the LazyEventAPI
     * subclass.
     *
     * Declare a static instance of your LazyEventAPI listener registrar
     * class. When it's constructed at static initialization time, it will
     * register your LLEventAPI subclass with LLEventPumps. It will also
     * collect metadata for the LLEventAPI and its operations to provide to
     * LLLeapListener's introspection queries.
     *
     * When someone later calls LLEventPumps::obtain() to post an event to
     * your LLEventAPI subclass, obtain() will instantiate it using
     * LazyEventAPI's name, desc, field and add() calls.
     */
    template <class EVENTAPI>
    class LazyEventAPI: public LazyEventAPIBase
    {
    public:
        // for subclass constructor to reference handler methods
        using listener = EVENTAPI;

        LazyEventAPI(const std::string& name, const std::string& desc,
                     const std::string& field="op"):
            // Forward ctor params to LazyEventAPIBase
            LazyEventAPIBase(name, desc, field)
        {}

    private:
        LLEventPump* construct(const std::string& /*name*/) override
        {
            // base class has carefully assembled LazyEventAPIParams embedded
            // in this instance, just pass to LLEventAPI subclass constructor
            return new EVENTAPI(mParams);
        }
    };
} // namespace LL

#endif /* ! defined(LL_LAZYEVENTAPI_H) */
