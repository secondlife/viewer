/** 
* @file llcrashloggerwindows.h
* @brief Windows crash logger definition
*
* $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLCRASHLOGGERWINDOWS_H
#define LLCRASHLOGGERWINDOWS_H

#include "llcrashlogger.h"
#include "windows.h"
#include "llstring.h"

class LLSD;

namespace google_breakpad {
	class CrashGenerationServer; 
	class ClientInfo;
}

class LLCrashLoggerWindows : public LLCrashLogger
{
public:
	LLCrashLoggerWindows(void);
	~LLCrashLoggerWindows(void);
	static LLCrashLoggerWindows* sInstance; 

	virtual bool init();
	virtual bool frame();
	virtual void updateApplication(const std::string& message = LLStringUtil::null);
	virtual bool cleanup();
	virtual void gatherPlatformSpecificFiles();
	void setHandle(HINSTANCE hInst) { mhInst = hInst; }
    int clients_connected() const {
        return mClientsConnected;
    }
	bool getMessageWithTimeout(MSG *msg, UINT to);
    
    // Starts the processing loop. This function does not return unless the
    // user is logging off or the user closes the crash service window. The
    // return value is a good number to pass in ExitProcess().
    int processingLoop();
private:
	void ProcessDlgItemText(HWND hWnd, int nIDDlgItem);
	void ProcessCaption(HWND hWnd);
	bool initCrashServer();
	google_breakpad::CrashGenerationServer* mCrashHandler;
	static void OnClientConnected(void* context,
 					const google_breakpad::ClientInfo* client_info);
 	
 	static void OnClientDumpRequest(
 					void* context,
 					const google_breakpad::ClientInfo* client_info,
 					const std::wstring* file_path);
 	
 	static void OnClientExited(void* context,
		 			const google_breakpad::ClientInfo* client_info);
    int mClientsConnected;
	int mPID;
	std::string mProcName;
    
	HINSTANCE mhInst;

};

#endif
