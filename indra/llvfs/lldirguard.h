/** 
 * @file lldirguard.h
 * @brief Protect working directory from being changed in scope.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
			llinfos << "Resetting working dir from " << mFinalDirUtf8 << " to " << mOrigDirUtf8 << llendl;
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
