/**
 * @file   lltestapp.h
 * @author Nat Goodspeed
 * @date   2019-10-21
 * @brief  LLApp subclass useful for testing.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLTESTAPP_H)
#define LL_LLTESTAPP_H

#include "llapp.h"

/**
 * LLTestApp is a dummy LLApp that simply sets LLApp::isRunning() for anyone
 * who cares.
 */
class LLTestApp: public LLApp
{
public:
    LLTestApp()
    {
        setStatus(APP_STATUS_RUNNING);
    }

    bool init()    { return true; }
    bool cleanup() { return true; }
    bool frame()   { return true; }
};

#endif /* ! defined(LL_LLTESTAPP_H) */
