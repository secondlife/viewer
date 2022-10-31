/** 
 * @file llfloaterbuyland.h
 * @brief LLFloaterBuyLand class definition
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLFLOATERBUYLAND_H
#define LL_LLFLOATERBUYLAND_H

class LLFloater;
class LLViewerRegion;
class LLParcelSelection;

class LLFloaterBuyLand
{
public:
    static void buyLand(LLViewerRegion* region,
                        LLSafeHandle<LLParcelSelection> parcel,
                        bool is_for_group);
    static void updateCovenantText(const std::string& string, const LLUUID& asset_id);
    static void updateEstateName(const std::string& name);
    static void updateLastModified(const std::string& text);
    static void updateEstateOwnerName(const std::string& name);
    
    static LLFloater* buildFloater(const LLSD& key);
};

#endif
