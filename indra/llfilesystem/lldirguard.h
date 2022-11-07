/** 
 * @file lldirguard.h
 * @brief Protect working directory from being changed in scope.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_DIRGUARD_H
#define LL_DIRGUARD_H

#include "linden_common.h"
#include "llerror.h"

#if LL_WINDOWS
class LLDirectoryGuard
{
public:
    LLDirectoryGuard()
    {
        mOrigDirLen = GetCurrentDirectory(MAX_PATH, mOrigDir);
    }

    ~LLDirectoryGuard()
    {
        mFinalDirLen = GetCurrentDirectory(MAX_PATH, mFinalDir);
        if ((mOrigDirLen!=mFinalDirLen) ||
            (wcsncmp(mOrigDir,mFinalDir,mOrigDirLen)!=0))
        {
            // Dir has changed
            std::string mOrigDirUtf8 = utf16str_to_utf8str(llutf16string(mOrigDir));
            std::string mFinalDirUtf8 = utf16str_to_utf8str(llutf16string(mFinalDir));
            LL_INFOS() << "Resetting working dir from " << mFinalDirUtf8 << " to " << mOrigDirUtf8 << LL_ENDL;
            SetCurrentDirectory(mOrigDir);
        }
    }

private:
    TCHAR mOrigDir[MAX_PATH];
    DWORD mOrigDirLen;
    TCHAR mFinalDir[MAX_PATH];
    DWORD mFinalDirLen;
};
#else // No-op outside Windows.
class LLDirectoryGuard
{
public:
    LLDirectoryGuard() {}
    ~LLDirectoryGuard() {}
};
#endif 


#endif
