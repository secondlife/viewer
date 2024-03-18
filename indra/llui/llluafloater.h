/** 
 * @file llluafloater.h
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

#ifndef LL_LLLUAFLOATER_H
#define LL_LLLUAFLOATER_H

#include "llfloater.h"
#include "lleventdispatcher.h"
#include "llevents.h"

class LLLuaFloater : public LLFloater
{
public:
    LLLuaFloater(const LLSD &key);
    BOOL postBuild();
    virtual ~LLLuaFloater();

    void registerCallback(const std::string &ctrl_name, const std::string &event);
    void onClose(bool app_quitting);

    void post(const LLSD &data);
    static void showLuaFloater(const LLSD &data);
    static LLSD getEventsData();

private:
    LLEventDispatcher mDispatcher;
    LLTempBoundListener mBoundListener;

    std::string mListenerPumpName;
    std::string mCommandPumpName;
};
#endif
