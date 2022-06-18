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
// external library headers
// other Linden headers
#include "llevents.h"

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
