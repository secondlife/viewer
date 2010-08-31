/**
 * @file   llstartuplistener.h
 * @author Nat Goodspeed
 * @date   2009-12-07
 * @brief  Event API to provide access to LLStartUp
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLSTARTUPLISTENER_H)
#define LL_LLSTARTUPLISTENER_H

#include "lleventapi.h"
class LLStartUp;
class LLSD;

class LLStartupListener: public LLEventAPI
{
public:
    LLStartupListener(/* LLStartUp* instance */); // all static members!

private:
    void postStartupState(const LLSD&) const;

    //LLStartup* mStartup;
};

#endif /* ! defined(LL_LLSTARTUPLISTENER_H) */
