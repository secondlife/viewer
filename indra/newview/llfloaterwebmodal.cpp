/**
 * @file llfloaterwebmodal.cpp
 * @brief Modal floater for displaying a web page above other modal floaters
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

#include "llfloaterwebmodal.h"

#include "llmediactrl.h"

LLFloaterWebModal::LLFloaterWebModal(const LLSD& key)
    // don't hide an existing modal floater (e.g. LLFloaterJoin) behind this one
    : LLModalDialog(key, /*modal=*/true, /*hide_others=*/false)
{
}

bool LLFloaterWebModal::postBuild()
{
    mWebBrowser = getChild<LLMediaCtrl>("web_modal_browser");
    mWebBrowser->addObserver(this);
    return true;
}

void LLFloaterWebModal::onOpen(const LLSD& key)
{
    LLModalDialog::onOpen(key);

    if (mWebBrowser && key.has("url"))
    {
        mWebBrowser->navigateTo(key["url"].asString(), HTTP_CONTENT_TEXT_HTML);
    }
}

void LLFloaterWebModal::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    if (event == MEDIA_EVENT_NAME_CHANGED)
    {
        const std::string page_title = self->getMediaName();
        setTitle(page_title.empty() ? self->getLocation() : page_title);
    }
}
