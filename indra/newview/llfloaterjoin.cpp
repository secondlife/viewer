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
#include "llframetimer.h"
#include "llfloaterwebmodal.h"
#include "llmediactrl.h"
#include "llpanellogin.h"
#include "llui.h"
#include "llviewercontrol.h"

// support for secondlife:///app/floaterjoin/{ACTION}/... SLapps
class LLFloaterJoinHandler : public LLCommandHandler
{
public:
    LLFloaterJoinHandler() : LLCommandHandler("floaterjoin", UNTRUSTED_THROTTLE) {}

    // params/query_map can contain the join password, keep it out of the log
    bool isSensitive() const override { return true; }

    bool handle(const LLSD& params, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web) override
    {
        if (params.size() < 1)
            return false;

        const std::string action = params[0].asString();

        if (action == "close")
        {
            LLFloaterReg::hideInstance("join");
        }
        else if (action == "tos")
        {
            LLFloaterReg::showInstance("web_modal", LLSD().with("url", gSavedSettings.getString("TermsOfServiceURL")));
        }
        else if (action == "tc")
        {
            LLFloaterReg::showInstance("web_modal", LLSD().with("url", gSavedSettings.getString("TermsAndConditionsURL")));
        }
        else if (action == "join" && params.size() >= 3)
        {
            LLPanelLogin::setCredentialFields(LLURI::unescape(params[1].asString()),
                                              LLURI::unescape(params[2].asString()));
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
    mNativeRect = getRect();
    return true;
}

void LLFloaterJoin::updateFloaterSize()
{
    const LLRect& screen_rect = gFloaterView->getRect();
    S32 width = llmin(mNativeRect.getWidth(), screen_rect.getWidth());
    S32 height = llmin(mNativeRect.getHeight(), screen_rect.getHeight());

    // Shrink the floater to fit the screen, letting scroll container reveal the rest if
    // the screen is currently smaller than the floater's native size, else restore it
    if (width != getRect().getWidth() || height != getRect().getHeight())
    {
        reshape(width, height);
        center();
    }
}

void LLFloaterJoin::draw()
{
    static LLCachedControl<F32> overlay_opacity(gSavedSettings, "JoinFloaterOverlayOpacity");

    updateFloaterSize();

    // As a modal dialog this floater is drawn twice per frame (once by LLFloaterView and once
    // by LLPopupView), so paint the overlay only on the first pass of each frame
    U32 frame_count = LLFrameTimer::getFrameCount();
    if (mOverlayFrame != frame_count)
    {
        mOverlayFrame = frame_count;

        // darken everything behind the floater, in absolute window coordinates
        LLVector2 window_size = LLUI::getInstance()->getWindowSize();
        LLUI::pushMatrix();
        LLUI::loadIdentity();
        gl_rect_2d(0, ll_round(window_size.mV[VY]), ll_round(window_size.mV[VX]), 0,
                   LLColor4(0.f, 0.f, 0.f, llclamp((F32)overlay_opacity, 0.f, 1.f)));
        LLUI::popMatrix();
    }

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

    static LLCachedControl<bool> show_test_slapp(gSavedSettings, "JoinFloaterTestSLapp");
    getChild<LLUICtrl>("test_close_link")->setVisible(show_test_slapp);
    getChild<LLUICtrl>("test_join_link")->setVisible(show_test_slapp);
    getChild<LLUICtrl>("test_tos_link")->setVisible(show_test_slapp);
    getChild<LLUICtrl>("test_tc_link")->setVisible(show_test_slapp);
}
