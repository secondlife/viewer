/** 
* @file llcrashloggerwindows.h
* @brief Windows crash logger definition
*
* $LicenseInfo:firstyear=2003&license=viewergpl$
* 
* Copyright (c) 2003-2007, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlife.com/developers/opensource/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LLCRASHLOGGERWINDOWS_H
#define LLCRASHLOGGERWINDOWS_H

#include "llcrashlogger.h"
#include "windows.h"
#include "llstring.h"

class LLCrashLoggerWindows : public LLCrashLogger
{
public:
	LLCrashLoggerWindows(void);
	~LLCrashLoggerWindows(void);
	virtual bool init();
	virtual bool mainLoop();
	virtual void updateApplication(const std::string& message = LLStringUtil::null);
	virtual bool cleanup();
	virtual void gatherPlatformSpecificFiles();
	//void annotateCallStack();
	void setHandle(HINSTANCE hInst) { mhInst = hInst; }
private:
	void ProcessDlgItemText(HWND hWnd, int nIDDlgItem);
	void ProcessCaption(HWND hWnd);
	HINSTANCE mhInst;

};

#endif
