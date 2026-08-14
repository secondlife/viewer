/**
 * @file llfloaterjoin.cpp
 * @brief Modal floater for the Join page on the login screen
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterjoin.h"

#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llmediactrl.h"
#include "llui.h"
#include "llviewercontrol.h"

static const F32 OVERLAY_OPACITY = 0.8f;

// support for secondlife:///app/floaterjoin/{ACTION}/... SLapps
class LLFloaterJoinHandler : public LLCommandHandler
{
public:
    LLFloaterJoinHandler() : LLCommandHandler("floaterjoin", UNTRUSTED_THROTTLE) {}

    bool handle(const LLSD& params, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web)
    {
        if (params.size() < 1)
            return false;

        if (params[0].asString() == "close")
        {
            LLFloaterReg::hideInstance("join");
        }

        return true;
    }
};
LLFloaterJoinHandler gFloaterJoinHandler;

LLFloaterJoin::LLFloaterJoin(const LLSD& key)
    : LLModalDialog(key, /*modal=*/true)
{
}

bool LLFloaterJoin::postBuild()
{
    mWebBrowser = getChild<LLMediaCtrl>("join_browser");
    return true;
}

void LLFloaterJoin::draw()
{
    // darken everything behind the floater
    LLVector2 window_size = LLUI::getInstance()->getWindowSize();
    LLRect screen_rect = calcScreenRect();
    gl_rect_2d(-screen_rect.mLeft, ll_round(window_size.mV[VY]) - screen_rect.mBottom,
               ll_round(window_size.mV[VX]) - screen_rect.mLeft, -screen_rect.mBottom,
               LLColor4(0.f, 0.f, 0.f, OVERLAY_OPACITY));

    LLModalDialog::draw();
}

void LLFloaterJoin::onOpen(const LLSD& key)
{
    LLModalDialog::onOpen(key);

    std::string url = gSavedSettings.getString("JoinURL");
    if (key.has("url"))
    {
        url = key["url"].asString();
    }

    if (mWebBrowser && !url.empty())
    {
        mWebBrowser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
    }
}
