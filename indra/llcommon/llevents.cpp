/**
 * @file   llevents.cpp
 * @author Nat Goodspeed
 * @date   2008-09-12
 * @brief  Implementation for llevents.
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

// Precompiled header
#include "linden_common.h"

#if LL_WINDOWS
#pragma warning (disable : 4675) // "resolved by ADL" -- just as I want!
#endif

// associated header
#include "llevents.h"
// STL headers
#include <set>
#include <sstream>
#include <algorithm>
// std headers
#include <typeinfo>
#include <cmath>
#include <cctype>
// external library headers
#include <boost/range/iterator_range.hpp>
#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4701) // compiler thinks might use uninitialized var, but no
#endif
#include <boost/lexical_cast.hpp>
#if LL_WINDOWS
#pragma warning (pop)
#endif
// other Linden headers
#include "stringize.h"
#include "llerror.h"
#include "llsdutil.h"
#include "llexception.h"
#if LL_MSVC
#pragma warning (disable : 4702)
#endif

/*****************************************************************************
*   LLEventPumps
*****************************************************************************/
LLEventPumps::LLEventPumps():
    mFactories
    {
        { "LLEventStream",   [](const std::string& name, bool tweak, const std::string& /*type*/)
                             { return new LLEventStream(name, tweak); } },
        { "LLEventMailDrop", [](const std::string& name, bool tweak, const std::string& /*type*/)
                             { return new LLEventMailDrop(name, tweak); } }
    },
    mTypes
    {
//      { "placeholder", "LLEventStream" }
    }
{}

bool LLEventPumps::registerTypeFactory(const std::string& type, const TypeFactory& factory)
{
    auto found = mFactories.find(type);
    // can't re-register a TypeFactory for a type name that's already registered
    if (found != mFactories.end())
        return false;
    // doesn't already exist, go ahead and register
    mFactories[type] = factory;
    return true;
}

void LLEventPumps::unregisterTypeFactory(const std::string& type)
{
    auto found = mFactories.find(type);
    if (found != mFactories.end())
        mFactories.erase(found);
}

bool LLEventPumps::registerPumpFactory(const std::string& name, const PumpFactory& factory)
{
    // Do we already have a pump by this name?
    if (mPumpMap.find(name) != mPumpMap.end())
        return false;
    // Do we already have an override for this pump name?
    if (mTypes.find(name) != mTypes.end())
        return false;
    // Leverage the two-level lookup implemented by mTypes (pump name -> type
    // name) and mFactories (type name -> factory). We could instead create a
    // whole separate (pump name -> factory) map, and look in both; or we
    // could change mTypes to (pump name -> factory) and, for typical type-
    // based lookups, use a "factory" that looks up the real factory in
    // mFactories. But this works, and we don't expect many calls to make() -
    // either explicit or implicit via obtain().
    // Create a bogus type name extremely unlikely to collide with an actual type.
    static std::string nul(1, '\0');
    std::string type_name{ nul + name };
    mTypes[name] = type_name;
    // TypeFactory is called with (name, tweak, type), whereas PumpFactory
    // accepts only name. We could adapt with std::bind(), but this lambda
    // does the trick.
    mFactories[type_name] =
        [factory]
        (const std::string& name, bool /*tweak*/, const std::string& /*type*/)
        { return factory(name); };
    return true;
}

void LLEventPumps::unregisterPumpFactory(const std::string& name)
{
    auto tfound = mTypes.find(name);
    if (tfound != mTypes.end())
    {
        auto ffound = mFactories.find(tfound->second);
        if (ffound != mFactories.end())
        {
            mFactories.erase(ffound);
        }
        mTypes.erase(tfound);
    }
}

LLEventPump& LLEventPumps::obtain(const std::string& name)
{
    PumpMap::iterator found = mPumpMap.find(name);
    if (found != mPumpMap.end())
    {
        // Here we already have an LLEventPump instance with the requested
        // name.
        return *found->second;
    }

    // Here we must instantiate an LLEventPump subclass. Is there a
    // preregistered class name override for this specific instance name?
    auto nfound = mTypes.find(name);
    std::string type;
    if (nfound != mTypes.end())
    {
        type = nfound->second;
    }
    // pass tweak=false: we already know there's no existing instance with
    // this name
    return make(name, false, type);
}

LLEventPump& LLEventPumps::make(const std::string& name, bool tweak,
                                const std::string& type)
{
    // find the relevant factory for this (or default) type
    auto found = mFactories.find(type.empty()? "LLEventStream" : type);
    if (found == mFactories.end())
    {
        // Passing an unrecognized type name is a no-no
        LLTHROW(BadType(type));
    }
    auto newInstance = (found->second)(name, tweak, type);
    // LLEventPump's constructor implicitly registers each new instance in
    // mPumpMap. But remember that we instantiated it (in mOurPumps) so we'll
    // delete it later.
    mOurPumps.insert(newInstance);
    return *newInstance;
}

bool LLEventPumps::post(const std::string&name, const LLSD&message)
{
    PumpMap::iterator found = mPumpMap.find(name);

    if (found == mPumpMap.end())
        return false;

    return (*found).second->post(message);
}

void LLEventPumps::flush()
{
    // Flush every known LLEventPump instance. Leave it up to each instance to
    // decide what to do with the flush() call.
    for (PumpMap::value_type& pair : mPumpMap)
    {
        pair.second->flush();
    }
}

void LLEventPumps::clear()
{
    // Clear every known LLEventPump instance. Leave it up to each instance to
    // decide what to do with the clear() call.
    for (PumpMap::value_type& pair : mPumpMap)
    {
        pair.second->clear();
    }
}

void LLEventPumps::reset()
{
    // Reset every known LLEventPump instance. Leave it up to each instance to
    // decide what to do with the reset() call.
    for (PumpMap::value_type& pair : mPumpMap)
    {
        pair.second->reset();
    }
}

std::string LLEventPumps::registerNew(const LLEventPump& pump, const std::string& name, bool tweak)
{
    std::pair<PumpMap::iterator, bool> inserted =
        mPumpMap.insert(PumpMap::value_type(name, const_cast<LLEventPump*>(&pump)));
    // If the insert worked, then the name is unique; return that.
    if (inserted.second)
        return name;
    // Here the new entry was NOT inserted, and therefore name isn't unique.
    // Unless we're permitted to tweak it, that's Bad.
    if (! tweak)
    {
        LLTHROW(LLEventPump::DupPumpName("Duplicate LLEventPump name '" + name + "'"));
    }
    // The passed name isn't unique, but we're permitted to tweak it. Find the
    // first decimal-integer suffix not already taken. The insert() attempt
    // above will have set inserted.first to the iterator of the existing
    // entry by that name. Starting there, walk forward until we reach an
    // entry that doesn't start with 'name'. For each entry consisting of name
    // + integer suffix, capture the integer suffix in a set. Use a set
    // because we're going to encounter string suffixes in the order: name1,
    // name10, name11, name2, ... Walking those possibilities in that order
    // isn't convenient to detect the first available "hole."
    std::set<int> suffixes;
    PumpMap::iterator pmi(inserted.first), pmend(mPumpMap.end());
    // We already know inserted.first references the existing entry with
    // 'name' as the key; skip that one and start with the next.
    while (++pmi != pmend)
    {
        if (pmi->first.substr(0, name.length()) != name)
        {
            // Found the first entry beyond the entries starting with 'name':
            // stop looping.
            break;
        }
        // Here we're looking at an entry that starts with 'name'. Is the rest
        // of it an integer?
        // Dubious (?) assumption: in the local character set, decimal digits
        // are in increasing order such that '9' is the last of them. This
        // test deals with 'name' values such as 'a', where there might be a
        // very large number of entries starting with 'a' whose suffixes
        // aren't integers. A secondary assumption is that digit characters
        // precede most common name characters (true in ASCII, false in
        // EBCDIC). The test below is correct either way, but it's worth more
        // if the assumption holds.
        if (pmi->first[name.length()] > '9')
            break;
        // It should be cheaper to detect that we're not looking at a digit
        // character -- and therefore the suffix can't possibly be an integer
        // -- than to attempt the lexical_cast and catch the exception.
        if (! std::isdigit(pmi->first[name.length()]))
            continue;
        // Okay, the first character of the suffix is a digit, it's worth at
        // least attempting to convert to int.
        try
        {
            suffixes.insert(boost::lexical_cast<int>(pmi->first.substr(name.length())));
        }
        catch (const boost::bad_lexical_cast&)
        {
            // If the rest of pmi->first isn't an int, just ignore it.
        }
    }
    // Here we've accumulated in 'suffixes' all existing int suffixes of the
    // entries starting with 'name'. Find the first unused one.
    int suffix = 1;
    for ( ; suffixes.find(suffix) != suffixes.end(); ++suffix)
        ;
    // Here 'suffix' is not in 'suffixes'. Construct a new name based on that
    // suffix, insert it and return it.
    std::ostringstream out;
    out << name << suffix;
    return registerNew(pump, out.str(), tweak);
}

void LLEventPumps::unregister(const LLEventPump& pump)
{
    // Remove this instance from mPumpMap
    PumpMap::iterator found = mPumpMap.find(pump.getName());
    if (found != mPumpMap.end())
    {
        mPumpMap.erase(found);
    }
    // If this instance is one we created, also remove it from mOurPumps so we
    // won't try again to delete it later!
    PumpSet::iterator psfound = mOurPumps.find(const_cast<LLEventPump*>(&pump));
    if (psfound != mOurPumps.end())
    {
        mOurPumps.erase(psfound);
    }
}

LLEventPumps::~LLEventPumps()
{
    // On destruction, delete every LLEventPump we instantiated (via
    // obtain()). CAREFUL: deleting an LLEventPump calls its destructor, which
    // calls unregister(), which removes that LLEventPump instance from
    // mOurPumps. So an iterator loop over mOurPumps to delete contained
    // LLEventPump instances is dangerous! Instead, delete them one at a time
    // until mOurPumps is empty.
    while (! mOurPumps.empty())
    {
        delete *mOurPumps.begin();
    }
    // Reset every remaining registered LLEventPump subclass instance: those
    // we DIDN'T instantiate using either make() or obtain().
    reset();
}

/*****************************************************************************
*   LLEventPump
*****************************************************************************/
#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif
const std::string LLEventPump::ANONYMOUS = std::string();


LLEventPump::LLEventPump(const std::string& name, bool tweak):
    // Register every new instance with LLEventPumps
    mRegistry(LLEventPumps::instance().getHandle()),
    mName(mRegistry.get()->registerNew(*this, name, tweak)),
    mSignal(std::make_shared<LLStandardSignal>()),
    mEnabled(true)
{}

#if LL_WINDOWS
#pragma warning (pop)
#endif

LLEventPump::~LLEventPump()
{
    // Unregister this doomed instance from LLEventPumps -- but only if
    // LLEventPumps is still around!
    LLEventPumps* registry = mRegistry.get();
    if (registry)
    {
        registry->unregister(*this);
    }
}

// static data member
const LLEventPump::NameList LLEventPump::empty;

std::string LLEventPump::inventName(const std::string& pfx)
{
    static long suffix = 0;
    return STRINGIZE(pfx << suffix++);
}

void LLEventPump::clear()
{
    // Destroy the original LLStandardSignal instance, replacing it with a
    // whole new one.
    mSignal = std::make_shared<LLStandardSignal>();
    mConnections.clear();
}

void LLEventPump::reset()
{
    // Resetting mSignal is supposed to disconnect everything on its own
    // But due to crash on 'reset' added explicit cleanup to get more data
    ConnectionMap::const_iterator iter = mConnections.begin();
    ConnectionMap::const_iterator end = mConnections.end();
    while (iter!=end)
    {
        iter->second.disconnect();
        iter++;
    }
    mConnections.clear();

    mSignal.reset();
    //mDeps.clear();
}

LLBoundListener LLEventPump::listen_impl(const std::string& name, const LLEventListener& listener,
                                         const NameList& after,
                                         const NameList& before)
{
    if (!mSignal)
    {
        LL_WARNS() << "Can't connect listener" << LL_ENDL;
        // connect will fail, return dummy
        return LLBoundListener();
    }

    float nodePosition = 1.0;

    // if the supplied name is empty we are not interested in the ordering mechanism 
    // and can bypass attempting to find the optimal location to insert the new 
    // listener.  We'll just tack it on to the end.
    if (!name.empty()) // should be the same as testing against ANONYMOUS
    {
        // Check for duplicate name before connecting listener to mSignal
        ConnectionMap::const_iterator found = mConnections.find(name);
        // In some cases the user might disconnect a connection explicitly -- or
        // might use LLEventTrackable to disconnect implicitly. Either way, we can
        // end up retaining in mConnections a zombie connection object that's
        // already been disconnected. Such a connection object can't be
        // reconnected -- nor, in the case of LLEventTrackable, would we want to
        // try, since disconnection happens with the destruction of the listener
        // object. That means it's safe to overwrite a disconnected connection
        // object with the new one we're attempting. The case we want to prevent
        // is only when the existing connection object is still connected.
        if (found != mConnections.end() && found->second.connected())
        {
        LLTHROW(DupListenerName("Attempt to register duplicate listener name '" + name +
                                "' on " + typeid(*this).name() + " '" + getName() + "'"));
        }
        // Okay, name is unique, try to reconcile its dependencies. Specify a new
        // "node" value that we never use for an mSignal placement; we'll fix it
        // later.
        DependencyMap::node_type& newNode = mDeps.add(name, -1.0, after, before);
        // What if this listener has been added, removed and re-added? In that
        // case newNode already has a non-negative value because we never remove a
        // listener from mDeps. But keep processing uniformly anyway in case the
        // listener was added back with different dependencies. Then mDeps.sort()
        // would put it in a different position, and the old newNode placement
        // value would be wrong, so we'd have to reassign it anyway. Trust that
        // re-adding a listener with the same dependencies is the trivial case for
        // mDeps.sort(): it can just replay its cache.
        DependencyMap::sorted_range sorted_range;
        try
        {
            // Can we pick an order that works including this new entry?
            sorted_range = mDeps.sort();
        }
        catch (const DependencyMap::Cycle& e)
        {
            // No: the new node's after/before dependencies have made mDeps
            // unsortable. If we leave the new node in mDeps, it will continue
            // to screw up all future attempts to sort()! Pull it out.
            mDeps.remove(name);
        LLTHROW(Cycle("New listener '" + name + "' on " + typeid(*this).name() +
                      " '" + getName() + "' would cause cycle: " + e.what()));
        }
        // Walk the list to verify that we haven't changed the order.
        float previous = 0.0, myprev = 0.0;
        DependencyMap::sorted_iterator mydmi = sorted_range.end(); // need this visible after loop
        for (DependencyMap::sorted_iterator dmi = sorted_range.begin();
            dmi != sorted_range.end(); ++dmi)
        {
            // Since we've added the new entry with an invalid placement,
            // recognize it and skip it.
            if (dmi->first == name)
            {
                // Remember the iterator belonging to our new node, and which
                // placement value was 'previous' at that point.
                mydmi = dmi;
                myprev = previous;
                continue;
            }
            // If the new node has rearranged the existing nodes, we'll find
            // that their placement values are no longer in increasing order.
            if (dmi->second < previous)
            {
                // This is another scenario in which we'd better back out the
                // newly-added node from mDeps -- but don't do it yet, we want to
                // traverse the existing mDeps to report on it!
                // Describe the change to the order of our listeners. Copy
                // everything but the newest listener to a vector we can sort to
                // obtain the old order.
                typedef std::vector< std::pair<float, std::string> > SortNameList;
                SortNameList sortnames;
                for (DependencyMap::sorted_iterator cdmi(sorted_range.begin()), cdmend(sorted_range.end());
                    cdmi != cdmend; ++cdmi)
                {
                    if (cdmi->first != name)
                    {
                        sortnames.push_back(SortNameList::value_type(cdmi->second, cdmi->first));
                    }
                }
                std::sort(sortnames.begin(), sortnames.end());
                std::ostringstream out;
                out << "New listener '" << name << "' on " << typeid(*this).name() << " '" << getName()
                    << "' would move previous listener '" << dmi->first << "'\nwas: ";
                SortNameList::const_iterator sni(sortnames.begin()), snend(sortnames.end());
                if (sni != snend)
                {
                    out << sni->second;
                    while (++sni != snend)
                    {
                        out << ", " << sni->second;
                    }
                }
                out << "\nnow: ";
                DependencyMap::sorted_iterator ddmi(sorted_range.begin()), ddmend(sorted_range.end());
                if (ddmi != ddmend)
                {
                    out << ddmi->first;
                    while (++ddmi != ddmend)
                    {
                        out << ", " << ddmi->first;
                    }
                }
                // NOW remove the offending listener node.
                mDeps.remove(name);
                // Having constructed a description of the order change, inform caller.
            LLTHROW(OrderChange(out.str()));
            }
            // This node becomes the previous one.
            previous = dmi->second;
        }
        // We just got done with a successful mDeps.add(name, ...) call. We'd
        // better have found 'name' somewhere in that sorted list!
        assert(mydmi != sorted_range.end());
        // Four cases:
        // 0. name is the only entry: placement 1.0
        // 1. name is the first of several entries: placement (next placement)/2
        // 2. name is between two other entries: placement (myprev + (next placement))/2
        // 3. name is the last entry: placement ceil(myprev) + 1.0
        // Since we've cleverly arranged for myprev to be 0.0 if name is the
        // first entry, this folds down to two cases. Case 1 is subsumed by
        // case 2, and case 0 is subsumed by case 3. So we need only handle
        // cases 2 and 3, which means we need only detect whether name is the
        // last entry. Increment mydmi to see if there's anything beyond.
        if (++mydmi != sorted_range.end())
        {
            // The new node isn't last. Place it between the previous node and
            // the successor.
            newNode = (myprev + mydmi->second) / 2.f;
        }
        else
        {
            // The new node is last. Bump myprev up to the next integer, add
            // 1.0 and use that.
            newNode = std::ceil(myprev) + 1.f;
        }

        nodePosition = newNode;
    }
    // Now that newNode has a value that places it appropriately in mSignal,
    // connect it.
    LLBoundListener bound = mSignal->connect(nodePosition, listener);
    
    if (!name.empty())
    {   // note that we are not tracking anonymous listeners here either.
        // This means that it is the caller's responsibility to either assign 
        // to a TempBoundListerer (scoped_connection) or manually disconnect 
        // when done. 
        mConnections[name] = bound;
    }
    return bound;
}

LLBoundListener LLEventPump::getListener(const std::string& name) const
{
    ConnectionMap::const_iterator found = mConnections.find(name);
    if (found != mConnections.end())
    {
        return found->second;
    }
    // not found, return dummy LLBoundListener
    return LLBoundListener();
}

void LLEventPump::stopListening(const std::string& name)
{
    ConnectionMap::iterator found = mConnections.find(name);
    if (found != mConnections.end())
    {
        found->second.disconnect();
        mConnections.erase(found);
    }
    // We intentionally do NOT remove this name from mDeps. It may happen that
    // the same listener with the same name and dependencies will jump on and
    // off this LLEventPump repeatedly. Keeping a cache of dependencies will
    // avoid a new dependency sort in such cases.
}

/*****************************************************************************
*   LLEventStream
*****************************************************************************/
bool LLEventStream::post(const LLSD& event)
{
    if (! mEnabled || !mSignal)
    {
        return false;
    }
    // NOTE NOTE NOTE: Any new access to member data beyond this point should
    // cause us to move our LLStandardSignal object to a pimpl class along
    // with said member data. Then the local shared_ptr will preserve both.

    // DEV-43463: capture a local copy of mSignal. We've turned up a
    // cross-coroutine scenario (described in the Jira) in which this post()
    // call could end up destroying 'this', the LLEventPump subclass instance
    // containing mSignal, during the call through *mSignal. So -- capture a
    // *stack* instance of the shared_ptr, ensuring that our heap
    // LLStandardSignal object will live at least until post() returns, even
    // if 'this' gets destroyed during the call.
    std::shared_ptr<LLStandardSignal> signal(mSignal);
    // Let caller know if any one listener handled the event. This is mostly
    // useful when using LLEventStream as a listener for an upstream
    // LLEventPump.
    return (*signal)(event);
}

/*****************************************************************************
 *   LLEventMailDrop
 *****************************************************************************/
bool LLEventMailDrop::post(const LLSD& event)
{
    // forward the call to our base class
    bool posted = LLEventStream::post(event);
    
    if (!posted)
    {   // if the event was not handled we will save it for later so that it can 
        // be posted to any future listeners when they attach.
        mEventHistory.push_back(event);
    }

    return posted;
}

LLBoundListener LLEventMailDrop::listen_impl(const std::string& name,
                                    const LLEventListener& listener,
                                    const NameList& after,
                                    const NameList& before)
{
    // Before actually connecting this listener for subsequent post() calls,
    // first feed each of the saved events, in order, to the new listener.
    // Remove any that this listener consumes -- Effective STL, Item 9.
    for (auto hi(mEventHistory.begin()), hend(mEventHistory.end()); hi != hend; )
    {
        if (listener(*hi))
        {
            // new listener consumed this event, erase it
            hi = mEventHistory.erase(hi);
        }
        else
        {
            // listener did not consume this event, just move along
            ++hi;
        }
    }

    // let base class perform the actual connection
    return LLEventStream::listen_impl(name, listener, after, before);
}

void LLEventMailDrop::discard()
{
    mEventHistory.clear();
}

/*****************************************************************************
*   LLListenerOrPumpName
*****************************************************************************/
LLListenerOrPumpName::LLListenerOrPumpName(const std::string& pumpname):
    // Look up the specified pumpname, and bind its post() method as our listener
    mListener(boost::bind(&LLEventPump::post,
                          boost::ref(LLEventPumps::instance().obtain(pumpname)),
                          _1))
{
}

LLListenerOrPumpName::LLListenerOrPumpName(const char* pumpname):
    // Look up the specified pumpname, and bind its post() method as our listener
    mListener(boost::bind(&LLEventPump::post,
                          boost::ref(LLEventPumps::instance().obtain(pumpname)),
                          _1))
{
}

bool LLListenerOrPumpName::operator()(const LLSD& event) const
{
    if (! mListener)
    {
        LLTHROW(Empty("attempting to call uninitialized"));
    }
    return (*mListener)(event);
}

void LLReqID::stamp(LLSD& response) const
{
    if (! (response.isUndefined() || response.isMap()))
    {
        // If 'response' was previously completely empty, it's okay to
        // turn it into a map. If it was already a map, then it should be
        // okay to add a key. But if it was anything else (e.g. a scalar),
        // assigning a ["reqid"] key will DISCARD the previous value,
        // replacing it with a map. That would be Bad.
        LL_INFOS("LLReqID") << "stamp(" << mReqid << ") leaving non-map response unmodified: "
                            << response << LL_ENDL;
        return;
    }
    LLSD oldReqid(response["reqid"]);
    if (! (oldReqid.isUndefined() || llsd_equals(oldReqid, mReqid)))
    {
        LL_INFOS("LLReqID") << "stamp(" << mReqid << ") preserving existing [\"reqid\"] value "
                            << oldReqid << " in response: " << response << LL_ENDL;
        return;
    }
    response["reqid"] = mReqid;
}

bool sendReply(const LLSD& reply, const LLSD& request, const std::string& replyKey)
{
    // If the original request has no value for replyKey, it's pointless to
    // construct or send a reply event: on which LLEventPump should we send
    // it? Allow that to be optional: if the caller wants to require replyKey,
    // it can so specify when registering the operation method.
    if (! request.has(replyKey))
    {
        return false;
    }

    // Here the request definitely contains replyKey; reasonable to proceed.

    // Copy 'reply' to modify it.
    LLSD newreply(reply);
    // Get the ["reqid"] element from request
    LLReqID reqID(request);
    // and copy it to 'newreply'.
    reqID.stamp(newreply);
    // Send reply on LLEventPump named in request[replyKey]. Don't forget to
    // send the modified 'newreply' instead of the original 'reply'.
    return LLEventPumps::instance().obtain(request[replyKey]).post(newreply);
}
