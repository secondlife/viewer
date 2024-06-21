/**
 * @file   llpbrterrainfeatures.h
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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


#pragma once

#include <string>

class LLViewerRegion;
class LLMessageSystem;
class LLModifyRegion;

// Queries/modifies PBR terrain repeats, possibly other features in the future
class LLPBRTerrainFeatures
{
public:
    static void queueQuery(LLViewerRegion& region, void(*done_callback)(LLUUID, bool, const LLModifyRegion&));
    static void queueModify(LLViewerRegion& region, const LLModifyRegion& composition);

private:
    static void queryRegionCoro(std::string cap_url, LLUUID region_id, void(*done_callback)(LLUUID, bool, const LLModifyRegion&) );
    static void modifyRegionCoro(std::string cap_url, LLSD updates, void(*done_callback)(bool) );
};

extern LLPBRTerrainFeatures gPBRTerrainFeatures;

