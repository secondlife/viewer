/**
 * @file   llcommanddispatcherlistener.h
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  LLEventAPI for LLCommandDispatcher
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCOMMANDDISPATCHERLISTENER_H)
#define LL_LLCOMMANDDISPATCHERLISTENER_H

#include "lleventapi.h"
class LLCommandDispatcher;
class LLSD;

class LLCommandDispatcherListener: public LLEventAPI
{
public:
    LLCommandDispatcherListener(/* LLCommandDispatcher* instance */); // all static members

private:
    void dispatch(const LLSD& params) const;
    void enumerate(const LLSD& params) const;

    //LLCommandDispatcher* mDispatcher;
};

#endif /* ! defined(LL_LLCOMMANDDISPATCHERLISTENER_H) */
