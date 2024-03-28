/**
 * @file llfloaterclassified.cpp
 * @brief LLFloaterClassified for displaying classifieds.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#include "llfloaterclassified.h"

LLFloaterClassified::LLFloaterClassified(const LLSD& key)
 : LLFloater(key)
{
}

LLFloaterClassified::~LLFloaterClassified()
{
}

void LLFloaterClassified::onOpen(const LLSD& key)
{
    LLPanel* panel = findChild<LLPanel>("main_panel", true);
    if (panel)
    {
        panel->onOpen(key);
    }
    if (key.has("classified_name"))
    {
        setTitle(key["classified_name"].asString());
    }
    LLFloater::onOpen(key);
}

bool LLFloaterClassified::postBuild()
{
    return true;
}


bool LLFloaterClassified::matchesKey(const LLSD& key)
{
    bool is_mkey_valid = mKey.has("classified_id");
    bool is_key_valid = key.has("classified_id");
    if (is_mkey_valid && is_key_valid)
    {
        return key["classified_id"].asUUID() == mKey["classified_id"].asUUID();
    }
    return is_mkey_valid == is_key_valid;
}

// eof
