/**
 * @file   lluilistener.h
 * @author Nat Goodspeed
 * @date   2009-08-18
 * @brief  Engage named functions as specified by XUI
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLUILISTENER_H)
#define LL_LLUILISTENER_H

#include "lleventdispatcher.h"
#include <string>

class LLSD;

class LLUIListener: public LLDispatchListener
{
public:
    LLUIListener(const std::string& name);

private:
    void call(const LLSD& event) const;
};

#endif /* ! defined(LL_LLUILISTENER_H) */
