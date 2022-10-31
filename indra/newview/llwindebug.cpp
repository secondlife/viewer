/**
 * @file llwindebug.cpp
 * @brief Windows debugging functions
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
private:
    unsigned char *mReserve;
    static const size_t MEMORY_RESERVATION_SIZE;
};

LLMemoryReserve::LLMemoryReserve() :
    mReserve(NULL)
{
}

LLMemoryReserve::~LLMemoryReserve()
{
    release();
}

// I dunno - this just seemed like a pretty good value.
const size_t LLMemoryReserve::MEMORY_RESERVATION_SIZE = 5 * 1024 * 1024;

void LLMemoryReserve::reserve()
{
    if(NULL == mReserve)
    {
        mReserve = new unsigned char[MEMORY_RESERVATION_SIZE];
    }
}

void LLMemoryReserve::release()
{
    if (NULL != mReserve)
    {
        delete [] mReserve;
    }
    mReserve = NULL;
}

static LLMemoryReserve gEmergencyMemoryReserve;


LONG NTAPI vectoredHandler(PEXCEPTION_POINTERS exception_infop)
{
    LLWinDebug::instance().generateMinidump(exception_infop);
    return EXCEPTION_CONTINUE_SEARCH;
}

// static
void  LLWinDebug::initSingleton()
{
    static bool s_first_run = true;
    // Load the dbghelp dll now, instead of waiting for the crash.
    // Less potential for stack mangling

    // Don't install vectored exception handler if being debugged.
    if(IsDebuggerPresent()) return;

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
        AddVectoredExceptionHandler(0, &vectoredHandler);
    }
}

void LLWinDebug::cleanupSingleton()
{
    gEmergencyMemoryReserve.release();
}

void LLWinDebug::writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const std::string& filename)
{
    // Temporary fix to switch out the code that writes the DMP file.
    // Fix coming that doesn't write a mini dump file for regular C++ exceptions.
    const bool enable_write_dump_file = false;
    if ( enable_write_dump_file )
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
