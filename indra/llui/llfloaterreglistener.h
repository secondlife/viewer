/**
 * @file   llfloaterreglistener.h
 * @author Nat Goodspeed
 * @date   2009-08-12
 * @brief  Wrap (subset of) LLFloaterReg API with an event API
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLFLOATERREGLISTENER_H)
#define LL_LLFLOATERREGLISTENER_H

#include "lleventapi.h"
#include <string>

class LLSD;

/// Event API wrapper for LLFloaterReg
class LLFloaterRegListener: public LLEventAPI
{
public:
    /// As all public LLFloaterReg methods are static, there's no point in
    /// binding an LLFloaterReg instance.
    LLFloaterRegListener();

private:
    void getBuildMap(const LLSD& event) const;
    void showInstance(const LLSD& event) const;
    void hideInstance(const LLSD& event) const;
    void toggleInstance(const LLSD& event) const;
    void clickButton(const LLSD& event) const;
};

#endif /* ! defined(LL_LLFLOATERREGLISTENER_H) */
