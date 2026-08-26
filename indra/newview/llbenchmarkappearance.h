/**
 * @file llbenchmarkappearance.h
 * @brief Privacy-safe appearance classification for renderer benchmarks.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#ifndef LL_LLBENCHMARKAPPEARANCE_H
#define LL_LLBENCHMARKAPPEARANCE_H

#include <array>

namespace LLBenchmarkAppearance
{
using RequiredParts = std::array<bool, 4>;

struct Facts
{
    bool avatar_valid = false;
    bool cof_present = false;
    bool cof_complete = false;
    bool cof_change_in_progress = false;
    RequiredParts required_links_resolved{};
    RequiredParts required_wearables_delivered{};
    bool avatar_loaded = false;
};

inline bool allRequiredParts(const RequiredParts& parts)
{
    for (bool present : parts)
    {
        if (!present)
        {
            return false;
        }
    }
    return true;
}

inline const char* classify(const Facts& facts)
{
    if (!facts.avatar_valid)
    {
        return "avatar-unavailable";
    }
    if (!facts.cof_present || !facts.cof_complete)
    {
        return "cof-incomplete";
    }
    if (!allRequiredParts(facts.required_links_resolved))
    {
        return "required-link-missing-or-unresolved";
    }
    if (!allRequiredParts(facts.required_wearables_delivered))
    {
        return "wearable-delivery-pending-or-failed";
    }
    if (!facts.avatar_loaded)
    {
        return "avatar-later-blocker";
    }
    return "ready";
}
}

#endif
