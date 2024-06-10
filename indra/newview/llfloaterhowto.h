/**
 * @file llfloaterhowto.h
 * @brief A variant of web floater meant to open guidebook
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#ifndef LL_LLFLOATERHOWTO_H
#define LL_LLFLOATERHOWTO_H

#include "llfloaterwebcontent.h"

class LLMediaCtrl;


class LLFloaterHowTo :
    public LLFloaterWebContent
{
public:
    LOG_CLASS(LLFloaterHowTo);

    typedef LLFloaterWebContent::Params Params;

    LLFloaterHowTo(const Params& key);

    void onOpen(const LLSD& key) override;

    bool handleKeyHere(KEY key, MASK mask) override;

    static LLFloaterHowTo* getInstance();

    bool matchesKey(const LLSD& key) override { return true; /*single instance*/ };

private:
    bool postBuild() override;
};

#endif  // LL_LLFLOATERHOWTO_H

