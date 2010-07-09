/**
 * @file llwindebug.cpp
 * @brief Windows debugging functions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"

#include "llwindebug.h"
#include "lldir.h"


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


LONG NTAPI vectoredHandler(PEXCEPTION_POINTERS exception_infop)
{
	LLWinDebug::instance().generateMinidump(exception_infop);
	return EXCEPTION_CONTINUE_SEARCH;
}

// static
void  LLWinDebug::init()
{
	static bool s_first_run = true;
	// Load the dbghelp dll now, instead of waiting for the crash.
	// Less potential for stack mangling

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
			LL_WARNS("AppInit") << "Couldn't find dbghelp.dll!" << LL_ENDL;
		}
		else
		{
			f_mdwp = (MINIDUMPWRITEDUMP) GetProcAddress(hDll, "MiniDumpWriteDump");

			if (!f_mdwp)
			{
				FreeLibrary(hDll);
				hDll = NULL;
			}
		}

		gEmergencyMemoryReserve.reserve();

		s_first_run = false;

		// Add this exeption hanlder to save windows style minidump.
		//AddVectoredExceptionHandler(0, &vectoredHandler);
	}
}

void LLWinDebug::writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const std::string& filename)
{
	if(f_mdwp == NULL || gDirUtilp == NULL)
	{
		return;
	}
	else
	{
		std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, filename);

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
void LLWinDebug::generateMinidump(struct _EXCEPTION_POINTERS *exception_infop)
{
	std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
												"SecondLifeException");
	if (exception_infop)
	{
		// Since there is exception info... Release the hounds.
		gEmergencyMemoryReserve.release();

		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ExceptionPointers = exception_infop;
		ExInfo.ClientPointers = NULL;
		writeDumpToFile((MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory), &ExInfo, "SecondLife.dmp");
	}
}
