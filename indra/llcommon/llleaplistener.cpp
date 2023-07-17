/**
 * @file   llleaplistener.cpp
 * @author Nat Goodspeed
 * @date   2012-03-16
 * @brief  Implementation for llleaplistener.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "llleaplistener.h"
// STL headers
#include <algorithm>                // std::find_if
#include <functional>
#include <map>
#include <set>
// std headers
// external library headers
// other Linden headers
#include "lazyeventapi.h"
#include "llsdutil.h"
#include "lluuid.h"
#include "stringize.h"

/*****************************************************************************
*   LEAP FEATURE STRINGS
*****************************************************************************/
/**
 * Implement "getFeatures" command. The LLSD map thus obtained is intended to
 * be machine-readable (read: easily-parsed, if parsing be necessary) and to
 * highlight the differences between this version of the LEAP protocol and
 * the baseline version. A client may thus determine whether or not the
 * running viewer supports some recent feature of interest.
 *
 * This method is defined at the top of this implementation file so it's easy
 * to find, easy to spot, easy to update as we enhance the LEAP protocol.
 */
/*static*/ LLSD LLLeapListener::getFeatures()
{
    static LLSD features;
    if (features.isUndefined())
    {
        features = LLSD::emptyMap();

        // This initial implementation IS the baseline LEAP protocol; thus the
        // set of differences is empty; thus features is initially empty.
//      features["featurename"] = "value";
    }

    return features;
}

LLLeapListener::LLLeapListener(const ConnectFunc& connect):
    // Each LEAP plugin has an instance of this listener. Make the command
    // pump name difficult for other such plugins to guess.
    LLEventAPI(LLUUID::generateNewID().asString(),
               "Operations relating to the LLSD Event API Plugin (LEAP) protocol"),
    mConnect(connect)
{
    LLSD need_name(LLSDMap("name", LLSD()));
    add("newpump",
        "Instantiate a new LLEventPump named like [\"name\"] and listen to it.\n"
        "[\"type\"] == \"LLEventStream\", \"LLEventMailDrop\" et al.\n"
        "Events sent through new LLEventPump will be decorated with [\"pump\"]=name.\n"
        "Returns actual name in [\"name\"] (may be different if collision).",
        &LLLeapListener::newpump,
        need_name);
    LLSD need_source_listener(LLSDMap("source", LLSD())("listener", LLSD()));
    add("listen",
        "Listen to an existing LLEventPump named [\"source\"], with listener name\n"
        "[\"listener\"].\n"
        "By default, send events on [\"source\"] to the plugin, decorated\n"
        "with [\"pump\"]=[\"source\"].\n"
        "If [\"dest\"] specified, send undecorated events on [\"source\"] to the\n"
        "LLEventPump named [\"dest\"].\n"
        "Returns [\"status\"] boolean indicating whether the connection was made.",
        &LLLeapListener::listen,
        need_source_listener);
    add("stoplistening",
        "Disconnect a connection previously established by \"listen\".\n"
        "Pass same [\"source\"] and [\"listener\"] arguments.\n"
        "Returns [\"status\"] boolean indicating whether such a listener existed.",
        &LLLeapListener::stoplistening,
        need_source_listener);
    add("ping",
        "No arguments, just a round-trip sanity check.",
        &LLLeapListener::ping);
    add("getAPIs",
        "Enumerate all LLEventAPI instances by name and description.",
        &LLLeapListener::getAPIs);
    add("getAPI",
        "Get name, description, dispatch key and operations for LLEventAPI [\"api\"].",
        &LLLeapListener::getAPI,
        LLSD().with("api", LLSD()));
    add("getFeatures",
        "Return an LLSD map of feature strings (deltas from baseline LEAP protocol)",
        static_cast<void (LLLeapListener::*)(const LLSD&) const>(&LLLeapListener::getFeatures));
    add("getFeature",
        "Return the feature value with key [\"feature\"]",
        &LLLeapListener::getFeature,
        LLSD().with("feature", LLSD()));
}

LLLeapListener::~LLLeapListener()
{
    // We'd have stored a map of LLTempBoundListener instances, save that the
    // operation of inserting into a std::map necessarily copies the
    // value_type, and Bad Things would happen if you copied an
    // LLTempBoundListener. (Destruction of the original would disconnect the
    // listener, invalidating every stored connection.)
    for (ListenersMap::value_type& pair : mListeners)
    {
        pair.second.disconnect();
    }
}

void LLLeapListener::newpump(const LLSD& request)
{
    Response reply(LLSD(), request);

    std::string name = request["name"];
    std::string type = request["type"];

    try
    {
        // tweak name for uniqueness
        LLEventPump& new_pump(LLEventPumps::instance().make(name, true, type));
        name = new_pump.getName();
        reply["name"] = name;

        // Now listen on this new pump with our plugin listener
        std::string myname("llleap");
        saveListener(name, myname, mConnect(new_pump, myname));
    }
    catch (const LLEventPumps::BadType& error)
    {
        reply.error(error.what());
    }
}

void LLLeapListener::listen(const LLSD& request)
{
    Response reply(LLSD(), request);

    std::string source_name = request["source"];
    std::string dest_name = request["dest"];
    std::string listener_name = request["listener"];

    LLEventPump & source = LLEventPumps::instance().obtain(source_name);

    reply["status"] = false;
    if (mListeners.find(ListenersMap::key_type(source_name, listener_name)) == mListeners.end())
    {
        try
        {
            if (request["dest"].isDefined())
            {
                // If we're asked to connect the "source" pump to a
                // specific "dest" pump, find dest pump and connect it.
                LLEventPump & dest = LLEventPumps::instance().obtain(dest_name);
                saveListener(source_name, listener_name,
                             source.listen(listener_name,
                                           boost::bind(&LLEventPump::post, &dest, _1)));
            }
            else
            {
                // "dest" unspecified means to direct events on "source"
                // to our plugin listener.
                saveListener(source_name, listener_name, mConnect(source, listener_name));
            }
            reply["status"] = true;
        }
        catch (const LLEventPump::DupListenerName &)
        {
            // pass - status already set to false
        }
    }
}

void LLLeapListener::stoplistening(const LLSD& request)
{
    Response reply(LLSD(), request);

    std::string source_name = request["source"];
    std::string listener_name = request["listener"];

    ListenersMap::iterator finder =
        mListeners.find(ListenersMap::key_type(source_name, listener_name));

    reply["status"] = false;
    if(finder != mListeners.end())
    {
        reply["status"] = true;
        finder->second.disconnect();
        mListeners.erase(finder);
    }
}

void LLLeapListener::ping(const LLSD& request) const
{
    // do nothing, default reply suffices
    Response(LLSD(), request);
}

void LLLeapListener::getAPIs(const LLSD& request) const
{
    Response reply(LLSD(), request);

    // first, traverse existing LLEventAPI instances
    std::set<std::string> instances;
    for (auto& ea : LLEventAPI::instance_snapshot())
    {
        // remember which APIs are actually instantiated
        instances.insert(ea.getName());
        reply[ea.getName()] = llsd::map("desc", ea.getDesc());
    }
    // supplement that with *potential* instances: that is, instances of
    // LazyEventAPI that can each instantiate an LLEventAPI on demand
    for (const auto& lea : LL::LazyEventAPIBase::instance_snapshot())
    {
        // skip any LazyEventAPI that's already instantiated its LLEventAPI
        if (instances.find(lea.getName()) == instances.end())
        {
            reply[lea.getName()] = llsd::map("desc", lea.getDesc());
        }
    }
}

// Because LazyEventAPI deliberately mimics LLEventAPI's query API, this
// function can be passed either -- even though they're unrelated types.
template <typename API>
void reportAPI(LLEventAPI::Response& reply, const API& api)
{
    reply["name"] = api.getName();
    reply["desc"] = api.getDesc();
    reply["key"]  = api.getDispatchKey();
    LLSD ops;
    for (const auto& namedesc : api)
    {
        ops.append(api.getMetadata(namedesc.first));
    }
    reply["ops"] = ops;
}

void LLLeapListener::getAPI(const LLSD& request) const
{
    Response reply(LLSD(), request);

    // check first among existing LLEventAPI instances
    auto foundea = LLEventAPI::getInstance(request["api"]);
    if (foundea)
    {
        reportAPI(reply, *foundea);
    }
    else
    {
        // Here the requested LLEventAPI doesn't yet exist, but do we have a
        // registered LazyEventAPI for it?
        LL::LazyEventAPIBase::instance_snapshot snap;
        auto foundlea = std::find_if(snap.begin(), snap.end(),
                                     [api = request["api"].asString()]
                                     (const auto& lea)
                                     { return (lea.getName() == api); });
        if (foundlea != snap.end())
        {
            reportAPI(reply, *foundlea);
        }
    }
}

void LLLeapListener::getFeatures(const LLSD& request) const
{
    // Merely constructing and destroying a Response object suffices here.
    // Giving it a name would only produce fatal 'unreferenced variable'
    // warnings.
    Response(getFeatures(), request);
}

void LLLeapListener::getFeature(const LLSD& request) const
{
    Response reply(LLSD(), request);

    LLSD::String feature_name(request["feature"]);
    LLSD features(getFeatures());
    if (features[feature_name].isDefined())
    {
        reply["feature"] = features[feature_name];
    }
}

void LLLeapListener::saveListener(const std::string& pump_name,
                                  const std::string& listener_name,
                                  const LLBoundListener& listener)
{
    mListeners.insert(ListenersMap::value_type(ListenersMap::key_type(pump_name, listener_name),
                                               listener));
}
