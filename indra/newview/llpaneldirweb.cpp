/**
 * @file llpaneldirweb.cpp
 * @brief Web panel in the legacy Search directory.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#include "llpaneldirweb.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloaterdirectory.h"
#include "lltextbox.h"
#include "llviewercontrol.h"
#include "llweb.h"

static LLPanelInjector<LLPanelDirWeb> t_panel_dir_web("panel_dir_web");

LLPanelDirWeb::LLPanelDirWeb()
:   LLPanel(),
    mFloaterDirectory(nullptr),
    mWebBrowser(nullptr)
{
}

bool LLPanelDirWeb::postBuild()
{
    childSetAction("home_btn", onClickHome, this);

    mBtnBack = getChild<LLButton>("back_btn");
    mBtnForward = getChild<LLButton>("forward_btn");
    mStatusBarText = getChild<LLTextBox>("statusbartext");

    mBtnBack->setClickedCallback([this](LLUICtrl*, const LLSD&) { mWebBrowser->navigateBack(); });
    mBtnForward->setClickedCallback([this](LLUICtrl*, const LLSD&) { mWebBrowser->navigateForward(); });

    mWebBrowser = findChild<LLMediaCtrl>("web_search");
    navigateToDefaultPage();
    mWebBrowser->addObserver(this);

    return true;
}

void LLPanelDirWeb::draw()
{
    // Asynchronous so we need to keep checking
    mBtnBack->setEnabled(mWebBrowser->canNavigateBack());
    mBtnForward->setEnabled(mWebBrowser->canNavigateForward());

    LLPanel::draw();
}

LLPanelDirWeb::~LLPanelDirWeb()
{
}

// When we show any browser-based view, we want to hide all
// the right-side XUI detail panels.
// virtual
void LLPanelDirWeb::onVisibilityChange(bool new_visibility)
{
    if (new_visibility && mFloaterDirectory)
    {
        mFloaterDirectory->hideAllDetailPanels();
    }
    LLPanel::onVisibilityChange(new_visibility);
}

void LLPanelDirWeb::navigateToDefaultPage()
{
    std::string url = gSavedSettings.getString("SearchURL");

    LLSD subs;
    subs["QUERY"] = "";
    subs["TYPE"] = "standard";
    // Default to PG
    std::string maturity = "g";
    if (gAgent.prefersAdult())
    {
        // PG,Mature,Adult
        maturity = "gma";
    }
    else if (gAgent.prefersMature())
    {
        // PG,Mature
        maturity = "gm";
    }
    subs["MATURITY"] = maturity;
    
    url = LLWeb::expandURLSubstitutions(url, subs);
    mWebBrowser->navigateTo(url, HTTP_CONTENT_TEXT_HTML);
}

// static
void LLPanelDirWeb::onClickHome( void* data )
{
    LLPanelDirWeb* self = (LLPanelDirWeb*)data;
    if (!self)
        return;
    self->navigateToDefaultPage();
}

void LLPanelDirWeb::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    if (event == MEDIA_EVENT_LOCATION_CHANGED)
    {
        const std::string url = self->getLocation();
        if (url.length())
            mStatusBarText->setText(url);
    }
    else if (event == MEDIA_EVENT_NAVIGATE_COMPLETE)
    {
        // we populate the status bar with URLs as they change so clear it now we're done
        const std::string end_str = "";
        mStatusBarText->setText(end_str);
    }
    else if (event == MEDIA_EVENT_STATUS_TEXT_CHANGED)
    {
        const std::string text = self->getStatusText();
        if (text.length())
            mStatusBarText->setText(text);
    }
    else if (event == MEDIA_EVENT_LINK_HOVERED)
    {
        const std::string link = self->getHoverLink();
        mStatusBarText->setText(link);
    }
}
