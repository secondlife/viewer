/**
 * @file llfloaterbuycurrencyhtml.cpp
 * @brief buy currency implemented in HTML floater - uses embedded media browser control
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llfloaterbuycurrencyhtml.h"
#include "llmediactrl.h"
#include "llviewercontrol.h"
#include "llweb.h"


LLFloaterBuyCurrencyHTML::LLFloaterBuyCurrencyHTML(const LLSD& key)
    :   LLFloater(key),
        mBrowser(nullptr)
{
}

LLFloaterBuyCurrencyHTML::~LLFloaterBuyCurrencyHTML()
{
}

bool LLFloaterBuyCurrencyHTML::postBuild()
{
    mBrowser = getChild<LLMediaCtrl>("browser");
    mBrowser->setErrorPageURL(gSavedSettings.getString("GenericErrorPageURL"));
    LLViewerMedia::getInstance()->getOpenIDCookie(mBrowser);
    return true;
}

void LLFloaterBuyCurrencyHTML::navigateToFinalURL()
{
    std::string buy_currency_url = gSavedSettings.getString("BuyCurrencyPacksURL");

    LLStringUtil::format_map_t replace;
    replace["[LANGUAGE]"] = LLUI::getLanguage();
    if (mShortfall > 0)
    {
        replace["[SHORTFALL]"] = std::to_string(mShortfall);
    }

    LLStringUtil::format(buy_currency_url, replace);

    if (mShortfall <= 0)
    {
        LLStringUtil::replaceString(buy_currency_url, "&shortfall=[SHORTFALL]", "");
    }

    mBrowser->navigateTo(buy_currency_url, HTTP_CONTENT_TEXT_HTML);
}
