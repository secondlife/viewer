/**
 * @file   llsidetraylistener.h
 * @author Nat Goodspeed
 * @date   2011-02-15
 * @brief  
 * 
 * $LicenseInfo:firstyear=2011&license=lgpl$
 * Copyright (c) 2011, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLSIDETRAYLISTENER_H)
#define LL_LLSIDETRAYLISTENER_H

#include "lleventapi.h"
#include <boost/function.hpp>

class LLSideTray;
class LLSD;

class LLSideTrayListener: public LLEventAPI
{
    typedef boost::function<LLSideTray*()> Getter;

public:
    LLSideTrayListener(const Getter& getter);

private:
    void getCollapsed(const LLSD& event) const;
    void getTabs(const LLSD& event) const;
    void getPanels(const LLSD& event) const;

    Getter mGetter;
};

#endif /* ! defined(LL_LLSIDETRAYLISTENER_H) */
