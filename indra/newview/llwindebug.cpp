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

#include <tchar.h>
#include <tlhelp32.h>
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
    <key>DateModified</key>
    <string></string>
    <key>ExceptionCode</key>
    <string></string>
    <key>ExceptionRead/WriteAddress</key>
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
	<key>ModuleName</key>
	<string></string>
	<key>ModuleBaseAddress</key>
	<string></string>
	<key>ModuleOffsetAddress</key>
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


extern void (*gCrashCallback)(void);

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

MINIDUMPWRITEDUMP f_mdwp = NULL;

#undef UNICODE

static LPTOP_LEVEL_EXCEPTION_FILTER gFilterFunc = NULL;

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

BOOL WINAPI Get_Module_By_Ret_Addr(PBYTE Ret_Addr, LPWSTR Module_Name, PBYTE & Module_Addr);


void printError( CHAR* msg )
{
  DWORD eNum;
  TCHAR sysMsg[256];
  TCHAR* p;

  eNum = GetLastError( );
  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, eNum,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         sysMsg, 256, NULL );

  // Trim the end of the line and terminate it with a null
  p = sysMsg;
  while( ( *p > 31 ) || ( *p == 9 ) )
    ++p;
  do { *p-- = 0; } while( ( p >= sysMsg ) &&
                          ( ( *p == '.' ) || ( *p < 33 ) ) );

  // Display the message
  printf( "\n  WARNING: %s failed with error %d (%s)", msg, eNum, sysMsg );
}

BOOL GetProcessThreadIDs(DWORD process_id, std::vector<DWORD>& thread_ids) 
{ 
  HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
  THREADENTRY32 te32; 
 
  // Take a snapshot of all running threads  
  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
  if( hThreadSnap == INVALID_HANDLE_VALUE ) 
    return( FALSE ); 
 
  // Fill in the size of the structure before using it. 
  te32.dwSize = sizeof(THREADENTRY32 ); 
 
  // Retrieve information about the first thread,
  // and exit if unsuccessful
  if( !Thread32First( hThreadSnap, &te32 ) ) 
  {
    printError( "Thread32First" );  // Show cause of failure
    CloseHandle( hThreadSnap );     // Must clean up the snapshot object!
    return( FALSE );
  }

  // Now walk the thread list of the system,
  // and display information about each thread
  // associated with the specified process
  do 
  { 
    if( te32.th32OwnerProcessID == process_id )
    {
      thread_ids.push_back(te32.th32ThreadID); 
    }
  } while( Thread32Next(hThreadSnap, &te32 ) ); 

//  Don't forget to clean up the snapshot object.
  CloseHandle( hThreadSnap );
  return( TRUE );
}

void WINAPI GetCallStackData(const CONTEXT* context_struct, LLSD& info)
{	
    // Fill Str with call stack info.
    // pException can be either GetExceptionInformation() or NULL.
    // If pException = NULL - get current call stack.

    LPWSTR	Module_Name = new WCHAR[MAX_PATH];
	PBYTE	Module_Addr = 0;
	
	typedef struct STACK
	{
		STACK *	Ebp;
		PBYTE	Ret_Addr;
		DWORD	Param[0];
	} STACK, * PSTACK;

	PSTACK	Ebp;

    if(context_struct)
    {
        Ebp = (PSTACK)context_struct->Ebp;
    }
    else
    {
        // The context struct is NULL, 
        // so we will use the current stack.
        Ebp = (PSTACK)&context_struct - 1;

        // Skip frame of GetCallStackData().
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
			info["CallStack"][i]["ModuleName"] = ll_convert_wide_to_string(Module_Name);
			info["CallStack"][i]["ModuleAddress"] = (int)Module_Addr;
			info["CallStack"][i]["CallOffset"] = (int)(Ebp->Ret_Addr - Module_Addr);

			LLSD params;
			// Save 5 params of the call. We don't know the real number of params.
			if (!IsBadReadPtr(Ebp, sizeof(PSTACK) + 5 * sizeof(DWORD)))
			{
				for(int j = 0; j < 5; ++j)
				{
					params[j] = (int)Ebp->Param[j];
				}
			}
			info["CallStack"][i]["Parameters"] = params;
		}
		info["CallStack"][i]["ReturnAddress"] = (int)Ebp->Ret_Addr;			
	}
}

BOOL GetThreadCallStack(DWORD thread_id, LLSD& info)
{
    if(GetCurrentThreadId() == thread_id)
    {
        // Early exit for the current thread.
        // Suspending the current thread would be a bad idea.
        // Plus you can't retrieve a valid current thread context.
        return false;
    }

    HANDLE thread_handle = INVALID_HANDLE_VALUE; 
    thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, thread_id);
    if(INVALID_HANDLE_VALUE == thread_handle)
    {
        return FALSE;
    }

    BOOL result = false;
    if(-1 != SuspendThread(thread_handle))
    {
        CONTEXT context_struct;
        context_struct.ContextFlags = CONTEXT_FULL;
        if(GetThreadContext(thread_handle, &context_struct))
        {
            GetCallStackData(&context_struct, info);
            result = true;
        }
        ResumeThread(thread_handle);
    }
    else
    {
        // Couldn't suspend thread.
    }

    CloseHandle(thread_handle);
    return result;
}


//Windows Call Stack Construction idea from 
//http://www.codeproject.com/tools/minidump.asp

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
			info["CallStack"][i]["ModuleName"] = ll_convert_wide_to_string(Module_Name);
			info["CallStack"][i]["ModuleAddress"] = (int)Module_Addr;
			info["CallStack"][i]["CallOffset"] = (int)(Ebp->Ret_Addr - Module_Addr);

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
			info["CallStack"][i]["Parameters"] = params;
		}
		info["CallStack"][i]["ReturnAddress"] = (int)Ebp->Ret_Addr;			
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
	LLSD info;
	LPWSTR		Str;
	int			Str_Len;
//	int			i;
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
	info["Process"] = ll_convert_wide_to_string(Str);

	// If exception occurred.
	if (pException)
	{
		EXCEPTION_RECORD &	E = *pException->ExceptionRecord;
		CONTEXT &			C = *pException->ContextRecord;

		// If module with E.ExceptionAddress found - save its path and date.
		if (Get_Module_By_Ret_Addr((PBYTE)E.ExceptionAddress, Module_Name, Module_Addr))
		{
			info["Module"] = ll_convert_wide_to_string(Module_Name);

			if ((hFile = CreateFile(Module_Name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
			{
				if (GetFileTime(hFile, NULL, NULL, &Last_Write_Time))
				{
					FileTimeToLocalFileTime(&Last_Write_Time, &Local_File_Time);
					FileTimeToSystemTime(&Local_File_Time, &T);

					info["DateModified"] = llformat("%02d/%02d/%d", T.wMonth, T.wDay, T.wYear);
				}
				CloseHandle(hFile);
			}
		}
		else
		{
			info["ExceptionAddr"] = (int)E.ExceptionAddress;
		}
		
		info["ExceptionCode"] = (int)E.ExceptionCode;
		
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
		/*
		std::string str;
		for (i = 0; i < 16; i++)
			str += llformat(" %02X", PBYTE(E.ExceptionAddress)[i]);
		info["Instruction"] = str;
		*/
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
void  LLWinDebug::initExceptionHandler(LPTOP_LEVEL_EXCEPTION_FILTER filter_func)
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
	}

	// Try to get Tool Help library functions.
	HMODULE hKernel32;
	hKernel32 = GetModuleHandle(_T("KERNEL32"));
	CreateToolhelp32Snapshot_ = (CREATE_TOOL_HELP32_SNAPSHOT)GetProcAddress(hKernel32, "CreateToolhelp32Snapshot");
	Module32First_ = (MODULE32_FIRST)GetProcAddress(hKernel32, "Module32FirstW");
	Module32Next_ = (MODULE32_NEST)GetProcAddress(hKernel32, "Module32NextW");

    LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(filter_func);

	if(prev_filter != gFilterFunc)
	{
		LL_WARNS("AppInit") 
			<< "Replacing unknown exception (" << (void *)prev_filter << ") with (" << (void *)filter_func << ") !" << LL_ENDL;
	}
	
	gFilterFunc = filter_func;
}

bool LLWinDebug::checkExceptionHandler()
{
	bool ok = true;
	LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	prev_filter = SetUnhandledExceptionFilter(gFilterFunc);

	if (prev_filter != gFilterFunc)
	{
		LL_WARNS("AppInit") << "Our exception handler (" << (void *)gFilterFunc << ") replaced with " << prev_filter << "!" << LL_ENDL;
		ok = false;
	}

	if (prev_filter == NULL)
	{
		ok = FALSE;
		if (gFilterFunc == NULL)
		{
			LL_WARNS("AppInit") << "Exception handler uninitialized." << LL_ENDL;
		}
		else
		{
			LL_WARNS("AppInit") << "Our exception handler (" << (void *)gFilterFunc << ") replaced with NULL!" << LL_ENDL;
		}
	}

	return ok;
}

void LLWinDebug::writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const std::string& filename)
{
	if(f_mdwp == NULL || gDirUtilp == NULL) 
	{
		return;
		//write_debug("No way to generate a minidump, no MiniDumpWriteDump function!\n");
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
void LLWinDebug::generateCrashStacks(struct _EXCEPTION_POINTERS *exception_infop)
{
	// *NOTE:Mani - This method is no longer the exception handler.
	// Its called from viewer_windows_exception_handler() and other places.

	// 
	// Let go of a bunch of reserved memory to give library calls etc
	// a chance to execute normally in the case that we ran out of
	// memory.
	//
	LLSD info;
	std::string dump_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,
												"SecondLifeException");
	std::string log_path = dump_path + ".log";

	if (exception_infop)
	{
		// Since there is exception info... Release the hounds.
		gEmergencyMemoryReserve.release();

		if(gSavedSettings.getControl("SaveMinidump").notNull() && gSavedSettings.getBOOL("SaveMinidump"))
		{
			_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

			ExInfo.ThreadId = ::GetCurrentThreadId();
			ExInfo.ExceptionPointers = exception_infop;
			ExInfo.ClientPointers = NULL;

			writeDumpToFile(MiniDumpNormal, &ExInfo, "SecondLife.dmp");
			writeDumpToFile((MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory), &ExInfo, "SecondLifePlus.dmp");
		}

		info = Get_Exception_Info(exception_infop);
	}

	LLSD threads;
    std::vector<DWORD> thread_ids;
    GetProcessThreadIDs(GetCurrentProcessId(), thread_ids);

    for(std::vector<DWORD>::iterator th_itr = thread_ids.begin(); 
                th_itr != thread_ids.end();
                ++th_itr)
    {
        LLSD thread_info;
        if(*th_itr != GetCurrentThreadId())
        {
            GetThreadCallStack(*th_itr, thread_info);
        }

        if(thread_info)
        {
            threads[llformat("ID %d", *th_itr)] = thread_info;
        }
    }

    info["Threads"] = threads;

	llofstream out_file(log_path);
	LLSDSerialize::toPrettyXML(info, out_file);
	out_file.close();
}
