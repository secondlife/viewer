/**
 * @file   llurldispatcherlistener.h
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  LLEventAPI for LLURLDispatcher
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLURLDISPATCHERLISTENER_H)
#define LL_LLURLDISPATCHERLISTENER_H

#include "lleventapi.h"
class LLURLDispatcher;
class LLSD;

class LLURLDispatcherListener: public LLEventAPI
{
public:
    LLURLDispatcherListener(/* LLURLDispatcher* instance */); // all static members

private:
    void dispatch(const LLSD& params) const;
    void dispatchRightClick(const LLSD& params) const;
    void dispatchFromTextEditor(const LLSD& params) const;

    //LLURLDispatcher* mDispatcher;
};

#endif /* ! defined(LL_LLURLDISPATCHERLISTENER_H) */
