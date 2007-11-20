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

#include <tchar.h>
#include <tlhelp32.h>
#include <atlbase.h>
#include "llappviewer.h"
#include "llwindebug.h"
#include "llviewercontrol.h"
#include "lldir.h"
#include "llsd.h"
#include "llsdserialize.h"

#pragma warning(disable: 4200)	//nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable: 4100)	//unreferenced formal parameter

/*
LLSD Block for Windows Dump Information
<llsd>
  <map>
    <key>Platform</key>
    <string></string>
    <key>Process</key>
    <string></string>
    <key>Module</key>
    <string></string>
    <key>Date Modified</key>
    <string></string>
    <key>Exception Code</key>
    <string></string>
    <key>Exception Read/Write Address</key>
    <string></string>
    <key>Instruction</key>
    <string></string>
    <key>Registers</key>
    <map>
      <!-- Continued for all registers -->
      <key>EIP</key>
      <string>...</string>
      <!-- ... -->
    </map>
    <key>Call Stack</key>
    <array>
      <!-- One map per stack frame -->
      <map>
	<key>Module Name</key>
	<string></string>
	<key>Module Base Address</key>
	<string></string>
	<key>Module Offset Address</key>
	<string></string>
	<key>Parameters</key>
	<array>
	  <string></string>
	</array>
      </map>
      <!-- ... -->
    </array>
  </map>
</llsd>

*/

// From viewer.h
extern BOOL gInProductionGrid;

extern void (*gCrashCallback)(void);

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

MINIDUMPWRITEDUMP f_mdwp = NULL;

#undef UNICODE

HMODULE	hDbgHelp;

// Tool Help functions.
typedef	HANDLE (WINAPI * CREATE_TOOL_HELP32_SNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef	BOOL (WINAPI * MODULE32_FIRST)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
typedef	BOOL (WINAPI * MODULE32_NEST)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

CREATE_TOOL_HELP32_SNAPSHOT	CreateToolhelp32Snapshot_;
MODULE32_FIRST	Module32First_;
MODULE32_NEST	Module32Next_;

#define	DUMP_SIZE_MAX	8000	//max size of our dump
#define	CALL_TRACE_MAX	((DUMP_SIZE_MAX - 2000) / (MAX_PATH + 40))	//max number of traced calls
#define	NL				L"\r\n"	//new line

//****************************************************************************************
BOOL WINAPI Get_Module_By_Ret_Addr(PBYTE Ret_Addr, LPWSTR Module_Name, PBYTE & Module_Addr)
//****************************************************************************************
// Find module by Ret_Addr (address in the module).
// Return Module_Name (full path) and Module_Addr (start address).
// Return TRUE if found.
{
	MODULEENTRY32	M = {sizeof(M)};
	HANDLE	hSnapshot;

	bool found = false;
	
	if (CreateToolhelp32Snapshot_)
	{
		hSnapshot = CreateToolhelp32Snapshot_(TH32CS_SNAPMODULE, 0);
		
		if ((hSnapshot != INVALID_HANDLE_VALUE) &&
			Module32First_(hSnapshot, &M))
		{
			do
			{
				if (DWORD(Ret_Addr - M.modBaseAddr) < M.modBaseSize)
				{
					lstrcpyn(Module_Name, M.szExePath, MAX_PATH);
					Module_Addr = M.modBaseAddr;
					found = true;
					break;
				}
			} while (Module32Next_(hSnapshot, &M));
		}

		CloseHandle(hSnapshot);
	}

	return found;
} //Get_Module_By_Ret_Addr

//******************************************************************
void WINAPI Get_Call_Stack(PEXCEPTION_POINTERS pException, LLSD& info)
//******************************************************************
// Fill Str with call stack info.
// pException can be either GetExceptionInformation() or NULL.
// If pException = NULL - get current call stack.
{	

	USES_CONVERSION;

	LPWSTR	Module_Name = new WCHAR[MAX_PATH];
	PBYTE	Module_Addr = 0;
	
	typedef struct STACK
	{
		STACK *	Ebp;
		PBYTE	Ret_Addr;
		DWORD	Param[0];
	} STACK, * PSTACK;

	STACK	Stack = {0, 0};
	PSTACK	Ebp;

	if (pException)		//fake frame for exception address
	{
		Stack.Ebp = (PSTACK)pException->ContextRecord->Ebp;
		Stack.Ret_Addr = (PBYTE)pException->ExceptionRecord->ExceptionAddress;
		Ebp = &Stack;
	}
	else
	{
		Ebp = (PSTACK)&pException - 1;	//frame addr of Get_Call_Stack()

		// Skip frame of Get_Call_Stack().
		if (!IsBadReadPtr(Ebp, sizeof(PSTACK)))
			Ebp = Ebp->Ebp;		//caller ebp
	}

	// Trace CALL_TRACE_MAX calls maximum - not to exceed DUMP_SIZE_MAX.
	// Break trace on wrong stack frame.
	for (int Ret_Addr_I = 0, i = 0;
		(Ret_Addr_I < CALL_TRACE_MAX) && !IsBadReadPtr(Ebp, sizeof(PSTACK)) && !IsBadCodePtr(FARPROC(Ebp->Ret_Addr));
		Ret_Addr_I++, Ebp = Ebp->Ebp, ++i)
	{
		// If module with Ebp->Ret_Addr found.

		if (Get_Module_By_Ret_Addr(Ebp->Ret_Addr, Module_Name, Module_Addr))
		{
			// Save module's address and full path.
			info["Call Stack"][i]["Module Name"] = W2A(Module_Name);
			info["Call Stack"][i]["Module Address"] = (int)Module_Addr;
			info["Call Stack"][i]["Call Offset"] = (int)(Ebp->Ret_Addr - Module_Addr);

			LLSD params;
			// Save 5 params of the call. We don't know the real number of params.
			if (pException && !Ret_Addr_I)	//fake frame for exception address
				params[0] = "Exception Offset";
			else if (!IsBadReadPtr(Ebp, sizeof(PSTACK) + 5 * sizeof(DWORD)))
			{
				for(int j = 0; j < 5; ++j)
				{
					params[j] = (int)Ebp->Param[j];
				}
			}
			info["Call Stack"][i]["Parameters"] = params;
		}
		info["Call Stack"][i]["Return Address"] = (int)Ebp->Ret_Addr;			
	}
} //Get_Call_Stack

//***********************************
void WINAPI Get_Version_Str(LLSD& info)
//***********************************
// Fill Str with Windows version.
{
	OSVERSIONINFOEX	V = {sizeof(OSVERSIONINFOEX)};	//EX for NT 5.0 and later

	if (!GetVersionEx((POSVERSIONINFO)&V))
	{
		ZeroMemory(&V, sizeof(V));
		V.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx((POSVERSIONINFO)&V);
	}

	if (V.dwPlatformId != VER_PLATFORM_WIN32_NT)
		V.dwBuildNumber = LOWORD(V.dwBuildNumber);	//for 9x HIWORD(dwBuildNumber) = 0x04xx

	info["Platform"] = llformat("Windows:  %d.%d.%d, SP %d.%d, Product Type %d",	//SP - service pack, Product Type - VER_NT_WORKSTATION,...
		V.dwMajorVersion, V.dwMinorVersion, V.dwBuildNumber, V.wServicePackMajor, V.wServicePackMinor, V.wProductType);
} //Get_Version_Str

//*************************************************************
LLSD WINAPI Get_Exception_Info(PEXCEPTION_POINTERS pException)
//*************************************************************
// Allocate Str[DUMP_SIZE_MAX] and return Str with dump, if !pException - just return call stack in Str.
{
	USES_CONVERSION;

	LLSD info;
	LPWSTR		Str;
	int			Str_Len;
	int			i;
	LPWSTR		Module_Name = new WCHAR[MAX_PATH];
	PBYTE		Module_Addr;
	HANDLE		hFile;
	FILETIME	Last_Write_Time;
	FILETIME	Local_File_Time;
	SYSTEMTIME	T;
	
	Str = new WCHAR[DUMP_SIZE_MAX];
	Str_Len = 0;
	if (!Str)
		return NULL;
	
	Get_Version_Str(info);

	
	GetModuleFileName(NULL, Str, MAX_PATH);
	info["Process"] = W2A(Str);

	// If exception occurred.
	if (pException)
	{
		EXCEPTION_RECORD &	E = *pException->ExceptionRecord;
		CONTEXT &			C = *pException->ContextRecord;

		// If module with E.ExceptionAddress found - save its path and date.
		if (Get_Module_By_Ret_Addr((PBYTE)E.ExceptionAddress, Module_Name, Module_Addr))
		{
			info["Module"] = W2A(Module_Name);

			if ((hFile = CreateFile(Module_Name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
			{
				if (GetFileTime(hFile, NULL, NULL, &Last_Write_Time))
				{
					FileTimeToLocalFileTime(&Last_Write_Time, &Local_File_Time);
					FileTimeToSystemTime(&Local_File_Time, &T);

					info["Date Modified"] = llformat("%02d/%02d/%d", T.wMonth, T.wDay, T.wYear);
				}
				CloseHandle(hFile);
			}
		}
		else
		{
			info["Exception Addr"] = (int)E.ExceptionAddress;
		}
		
		info["Exception Code"] = (int)E.ExceptionCode;
		
		/*
		//TODO: Fix this
		if (E.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			// Access violation type - Write/Read.
			LLSD exception_info;
			exception_info["Type"] = E.ExceptionInformation[0] ? "Write" : "Read";
			exception_info["Address"] = llformat("%08x", E.ExceptionInformation[1]);
			info["Exception Information"] = exception_info;
		}
		*/

		
		// Save instruction that caused exception.
		Str_Len = 0;
		for (i = 0; i < 16; i++)
			Str_Len += wsprintf(Str + Str_Len, L" %02X", PBYTE(E.ExceptionAddress)[i]);
		info["Instruction"] = W2A(Str);

		LLSD registers;
		registers["EAX"] = (int)C.Eax;
		registers["EBX"] = (int)C.Ebx;
		registers["ECX"] = (int)C.Ecx;
		registers["EDX"] = (int)C.Edx;
		registers["ESI"] = (int)C.Esi;
		registers["EDI"] = (int)C.Edi;
		registers["ESP"] = (int)C.Esp;
		registers["EBP"] = (int)C.Ebp;
		registers["EIP"] = (int)C.Eip;
		registers["EFlags"] = (int)C.EFlags;
		info["Registers"] = registers;
	} //if (pException)
	
	// Save call stack info.
	Get_Call_Stack(pException, info);

	if (Str[0] == NL[0])
		lstrcpy(Str, Str + sizeof(NL) - 1);
	

	return info;
} //Get_Exception_Info

#define UNICODE


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

			//write_debug(msg.c_str());

			ok = FALSE;
		}
		else
		{
			f_mdwp = (MINIDUMPWRITEDUMP) GetProcAddress(hDll, "MiniDumpWriteDump");

			if (!f_mdwp)
			{
				//write_debug("No MiniDumpWriteDump!\n");
				FreeLibrary(hDll);
				hDll = NULL;
				ok = FALSE;
			}
		}

		gEmergencyMemoryReserve.reserve();
	}

	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(LLWinDebug::handleException);

	// Try to get Tool Help library functions.
	HMODULE hKernel32;
	hKernel32 = GetModuleHandle(_T("KERNEL32"));
	CreateToolhelp32Snapshot_ = (CREATE_TOOL_HELP32_SNAPSHOT)GetProcAddress(hKernel32, "CreateToolhelp32Snapshot");
	Module32First_ = (MODULE32_FIRST)GetProcAddress(hKernel32, "Module32FirstW");
	Module32Next_ = (MODULE32_NEST)GetProcAddress(hKernel32, "Module32NextW");

	if (s_first_run)
	{
		// We're fine, this is the first run.
		s_first_run = FALSE;
		return ok;
	}
	if (!prev_filter)
	{
		llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with NULL!" << llendl;
		ok = FALSE;
	}
	if (prev_filter != LLWinDebug::handleException)
	{
		llwarns << "Our exception handler (" << (void *)LLWinDebug::handleException << ") replaced with " << prev_filter << "!" << llendl;
		ok = FALSE;
	}

	return ok;
	// Internal builds don't mess with exception handling.
	//return TRUE;
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

	if (exception_infop)
	{

		std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
															   "SecondLifeException");

		std::string log_path = dump_path + ".log";

		LLSD info;
		info = Get_Exception_Info(exception_infop);
		if (info)
		{
			std::ofstream out_file(log_path.c_str());
			LLSDSerialize::toPrettyXML(info, out_file);
			out_file.close();
		}
	}
	else
	{
		// We're calling this due to a network error, not due to an actual exception.
		// It doesn't realy matter what we return.
		return EXCEPTION_CONTINUE_SEARCH;
	}

	//handle viewer crash must be called here since
	//we don't return handling of the application 
	//back to the process. 
	LLAppViewer::handleViewerCrash();

	//
	// At this point, we always want to exit the app.  There's no graceful
	// recovery for an unhandled exception.
	// 
	// Just kill the process.
	LONG retval = EXCEPTION_EXECUTE_HANDLER;
	
	return retval;
}

#endif
