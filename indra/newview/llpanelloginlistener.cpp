/**
 * @file   llpanelloginlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  Implementation for llpanelloginlistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llpanelloginlistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llpanellogin.h"

LLPanelLoginListener::LLPanelLoginListener(LLPanelLogin* instance):
    LLEventAPI("LLPanelLogin", "Access to LLPanelLogin methods"),
    mPanel(instance)
{
    add("onClickConnect",
        "Pretend user clicked the \"Log In\" button",
        &LLPanelLoginListener::onClickConnect);
}

void LLPanelLoginListener::onClickConnect(const LLSD&) const
{
    mPanel->onClickConnect(NULL);
}
