/**
 * @file   llpanelloginlistener.h
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  LLEventAPI for LLPanelLogin
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLPANELLOGINLISTENER_H)
#define LL_LLPANELLOGINLISTENER_H

#include "lleventapi.h"
class LLPanelLogin;
class LLSD;

class LLPanelLoginListener: public LLEventAPI
{
public:
    LLPanelLoginListener(LLPanelLogin* instance);

private:
    void onClickConnect(const LLSD&) const;

    LLPanelLogin* mPanel;
};

#endif /* ! defined(LL_LLPANELLOGINLISTENER_H) */
