/**
 * @file   lazyeventapi.cpp
 * @author Nat Goodspeed
 * @date   2022-06-17
 * @brief  Implementation for lazyeventapi.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lazyeventapi.h"
// STL headers
// std headers
#include <algorithm>                // std::find_if
// external library headers
// other Linden headers
#include "llevents.h"
#include "llsdutil.h"

LL::LazyEventAPIBase::LazyEventAPIBase(
    const std::string& name, const std::string& desc, const std::string& field)
{
    // populate embedded LazyEventAPIParams instance
    mParams.name = name;
    mParams.desc = desc;
    mParams.field = field;
    // mParams.init and mOperations are populated by subsequent add() calls.

    // Our raison d'etre: register as an LLEventPumps::PumpFactory
    // so obtain() will notice any request for this name and call us.
    // Of course, our subclass constructor must finish running (making add()
    // calls) before mParams will be fully populated, but we expect that to
    // happen well before the first LLEventPumps::obtain(name) call.
    mRegistered = LLEventPumps::instance().registerPumpFactory(
        name,
        [this](const std::string& name){ return construct(name); });
}

LL::LazyEventAPIBase::~LazyEventAPIBase()
{
    // If our constructor's registerPumpFactory() call was unsuccessful, that
    // probably means somebody else claimed the name first. If that's the
    // case, do NOT unregister their name out from under them!
    // If this is a static instance being destroyed at process shutdown,
    // LLEventPumps will probably have been cleaned up already.
    if (mRegistered && ! LLEventPumps::wasDeleted())
    {
        // unregister the callback to this doomed instance
        LLEventPumps::instance().unregisterPumpFactory(mParams.name);
    }
}

LLSD LL::LazyEventAPIBase::getMetadata(const std::string& name) const
{
    // Since mOperations is a vector rather than a map, just search.
    auto found = std::find_if(mOperations.begin(), mOperations.end(),
                              [&name](const auto& namedesc)
                              { return (namedesc.first == name); });
    if (found == mOperations.end())
        return {};

    // LLEventDispatcher() supplements the returned metadata in different
    // ways, depending on metadata provided to the specific add() method.
    // Don't try to emulate all that. At some point we might consider more
    // closely unifying LLEventDispatcher machinery with LazyEventAPI, but for
    // now this will have to do.
    return llsd::map("name", found->first, "desc", found->second);
}
