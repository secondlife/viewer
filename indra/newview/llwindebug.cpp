/** 
 * @file llwindebug.cpp
 * @brief Windows debugging functions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#ifdef LL_WINDOWS

#include "llwindebug.h"
#include "llviewercontrol.h"
#include "lldir.h"

#include "llappviewer.h"


// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

MINIDUMPWRITEDUMP f_mdwp = NULL;



class LLMemoryReserve {
public:
	LLMemoryReserve();
	~LLMemoryReserve();
	void reserve();
	void release();
protected:
	unsigned char *mReserve;
	static const size_t MEMORY_RESERVATION_SIZE;
};

LLMemoryReserve::LLMemoryReserve() :
	mReserve(NULL)
{
};

LLMemoryReserve::~LLMemoryReserve()
{
	release();
}

// I dunno - this just seemed like a pretty good value.
const size_t LLMemoryReserve::MEMORY_RESERVATION_SIZE = 5 * 1024 * 1024;

void LLMemoryReserve::reserve()
{
	if(NULL == mReserve)
		mReserve = new unsigned char[MEMORY_RESERVATION_SIZE];
};

void LLMemoryReserve::release()
{
	delete [] mReserve;
	mReserve = NULL;
};

static LLMemoryReserve gEmergencyMemoryReserve;

// static
BOOL LLWinDebug::setupExceptionHandler()
{
#ifdef LL_RELEASE_FOR_DOWNLOAD

	static BOOL s_first_run = TRUE;
	// Load the dbghelp dll now, instead of waiting for the crash.
	// Less potential for stack mangling

	BOOL ok = TRUE;
	if (s_first_run)
	{
		// First, try loading from the directory that the app resides in.
		std::string local_dll_name = gDirUtilp->findFile("dbghelp.dll", gDirUtilp->getWorkingDir(), gDirUtilp->getExecutableDir());

		HMODULE hDll = NULL;
		hDll = LoadLibraryA(local_dll_name.c_str());
		if (!hDll)
		{
			hDll = LoadLibrary(L"dbghelp.dll");
		}

		if (!hDll)
		{
			llwarns << "Couldn't find dbghelp.dll!" << llendl;

			std::string msg = "Couldn't find dbghelp.dll at ";
			msg += local_dll_name;
			msg += "!\n";

			LLAppViewer::instance()->writeDebug(msg.c_str());

			ok = FALSE;
		}
		else
		{
			f_mdwp = (MINIDUMPWRITEDUMP) GetProcAddress(hDll, "MiniDumpWriteDump");

			if (!f_mdwp)
			{
				LLAppViewer::instance()->writeDebug("No MiniDumpWriteDump!\n");
				FreeLibrary(hDll);
				hDll = NULL;
				ok = FALSE;
			}
		}

		gEmergencyMemoryReserve.reserve();
	}

	// *REMOVE: LLApp now handles the exception handing. 
	//            LLAppViewerWin32 calls SetUnhandledExceptionFilter()

	//LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	//prev_filter = SetUnhandledExceptionFilter(LLWinDebug::handleException);

	//if (s_first_run)
	//{
	//	// We're fine, this is the first run.
	//	s_first_run = FALSE;
	//	return ok;
	//}
	//if (!prev_filter)
	//{
	//	llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with NULL!" << llendl;
	//	ok = FALSE;
	//}
	//if (prev_filter != LLWinDebug::handleException)
	//{
	//	llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with " << prev_filter << "!" << llendl;
	//	ok = FALSE;
	//}

	return ok;
#else
	// Internal builds don't mess with exception handling.
	return TRUE;
#endif
}

void LLWinDebug::writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const char *filename)
{
	if(f_mdwp == NULL) 
	{
		LLAppViewer::instance()->writeDebug("No way to generate a minidump, no MiniDumpWriteDump function!\n");
	}
	else if(gDirUtilp == NULL)
	{
		LLAppViewer::instance()->writeDebug("No way to generate a minidump, no gDirUtilp!\n");
	}
	else
	{
		std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
															   filename);

		HANDLE hFile = CreateFileA(dump_path.c_str(),
									GENERIC_WRITE,
									FILE_SHARE_WRITE,
									NULL,
									CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL,
									NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// Write the dump, ignoring the return value
			f_mdwp(GetCurrentProcess(),
					GetCurrentProcessId(),
					hFile,
					type,
					ExInfop,
					NULL,
					NULL);

			CloseHandle(hFile);
		}

	}
}

// static
LONG LLWinDebug::handleException(struct _EXCEPTION_POINTERS *exception_infop)
{
	// *NOTE:Mani - This method is no longer the initial exception handler.
	// It is called from viewer_windows_exception_handler() and other places.

	// 
	// Let go of a bunch of reserved memory to give library calls etc
	// a chance to execute normally in the case that we ran out of
	// memory.
	//
	gEmergencyMemoryReserve.release();

	BOOL userWantsMaxiDump =
		(stricmp(gSavedSettings.getString("LastName").c_str(), "linden") == 0)
		|| (stricmp(gSavedSettings.getString("LastName").c_str(), "tester") == 0);

	BOOL alsoSaveMaxiDump = userWantsMaxiDump && !gInProductionGrid;

	/* Calculate alsoSaveMaxiDump here */

	if (exception_infop)
	{
		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ExceptionPointers = exception_infop;
		ExInfo.ClientPointers = NULL;

		writeDumpToFile(MiniDumpNormal, &ExInfo, "SecondLife.dmp");

		if(alsoSaveMaxiDump)
			writeDumpToFile((MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory), &ExInfo, "SecondLifePlus.dmp");
	}
	else
	{
		writeDumpToFile(MiniDumpNormal, NULL, "SecondLife.dmp");

		if(alsoSaveMaxiDump)
			writeDumpToFile((MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory), NULL, "SecondLifePlus.dmp");
	}

	if (!exception_infop)
	{
		// We're calling this due to a network error, not due to an actual exception.
		// It doesn't realy matter what we return.
		return EXCEPTION_CONTINUE_SEARCH;
	}

	//
	// At this point, we always want to exit the app.  There's no graceful
	// recovery for an unhandled exception.
	// 
	// Just kill the process.
	LONG retval = EXCEPTION_EXECUTE_HANDLER;
	
	return retval;
}

#endif

