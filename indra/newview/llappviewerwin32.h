/**
 * @file llappviewerwin32.h
 * @brief The LLAppViewerWin32 class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLAPPVIEWERWIN32_H
#define LL_LLAPPVIEWERWIN32_H

#ifndef LL_LLAPPVIEWER_H
#include "llappviewer.h"
#endif

class LLAppViewerWin32 : public LLAppViewer
{
public:
    LLAppViewerWin32(const char* cmd_line);
    virtual ~LLAppViewerWin32();

    //
    // Main application logic
    //
    bool init() override; // Override to do application initialization
    bool cleanup() override;

    bool reportCrashToBugsplat(void* pExcepInfo) override;

protected:
    bool initWindow() override; // Override to initialize the viewer's window.
    void initLoggingAndGetLastDuration() override; // Override to clean stack_trace info.
    void initConsole() override; // Initialize OS level debugging console.
    bool initHardwareTest() override; // Win32 uses DX9 to test hardware.
    bool initParseCommandLine(LLCommandLineParser& clp) override;

    bool beingDebugged() override;
    bool restoreErrorTrap() override;

    bool sendURLToOtherInstance(const std::string& url) override;

    std::string generateSerialNumber();

    static const std::string sWindowClass;

private:
    void disableWinErrorReporting();

    std::string mCmdLine;
    bool mIsConsoleAllocated;
};

#endif // LL_LLAPPVIEWERWIN32_H
