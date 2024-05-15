/**
* @file lluiuisage.h
* @brief Header file for LLUIUsage
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

#ifndef LL_LLUIUSAGE_H
#define LL_LLUIUSAGE_H

#include <map>
#include "llsd.h"
#include "llsingleton.h"

// UIUsage tracking to see which operations and UI elements are most popular in a session
class LLUIUsage : public LLSingleton<LLUIUsage>
{
public:
    LLSINGLETON(LLUIUsage);
    ~LLUIUsage();
public:
    static std::string sanitized(const std::string& s);
    static void setLLSDPath(LLSD& sd, const std::string& path, S32 max_elts, const LLSD& val);
    static void setLLSDNested(LLSD& sd, const std::vector<std::string>& fields, const LLSD& val);
    void logCommand(const std::string& command);
    void logControl(const std::string& control);
    void logFloater(const std::string& floater);
    void logPanel(const std::string& p);
    LLSD asLLSD() const;
    void clear();
private:
    std::map<std::string,U32> mCommandCounts;
    std::map<std::string,U32> mControlCounts;
    std::map<std::string,U32> mFloaterCounts;
    std::map<std::string,U32> mPanelCounts;
};

#endif // LLUIUIUSAGE.h
