/**
 * @file llurlfloaterdispatchhandler.h
 * @brief Handles URLFloater generic message from server
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

#ifndef LL_LLURLFLOATERDISPATCHHANDLER_H
#define LL_LLURLFLOATERDISPATCHHANDLER_H

#include "lldispatcher.h"

class LLUrlFloaterDispatchHandler : public LLDispatchHandler
{
public:
    LOG_CLASS(LLUrlFloaterDispatchHandler);

    LLUrlFloaterDispatchHandler();
    virtual ~LLUrlFloaterDispatchHandler();

    virtual bool operator()(const LLDispatcher *, const std::string& key, const LLUUID& invoice, const sparam_t& strings) override;

    static void registerInDispatcher();

private:
    static LLUrlFloaterDispatchHandler sUrlDispatchhandler;
};

#endif  // LL_LLURLFLOATERDISPATCHHANDLER_H

