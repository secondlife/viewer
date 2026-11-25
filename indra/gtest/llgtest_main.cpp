/**
 * @file llgtest_main.cpp
 * @brief Common main() for GoogleTest based unit tests.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "llgtest.h"

#include <gmock/gmock.h>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{

// Prints a symbolized stack trace of a crashing thread to stderr.
//
// GoogleTest only catches crashes on the thread running the test body, but
// libraries under test (llwebrtc, for one) crash on their own threads, where
// all a test run would otherwise report is the process dying.  This runs for
// a crash on any thread.  Symbols come from the PDBs beside the binaries.
LONG WINAPI printCrashStack(EXCEPTION_POINTERS* info)
{
    static volatile LONG sEntered = 0;
    if (InterlockedExchange(&sEntered, 1))
    {
        return EXCEPTION_CONTINUE_SEARCH; // a second crash while reporting
    }

    const testing::TestInfo* test = testing::UnitTest::GetInstance()->current_test_info();
    fprintf(stderr, "\n*** Unhandled exception 0x%08lX at %p on thread %lu, during %s%s%s\n",
            info->ExceptionRecord->ExceptionCode,
            info->ExceptionRecord->ExceptionAddress,
            GetCurrentThreadId(),
            test ? test->test_suite_name() : "(no test)",
            test ? "." : "",
            test ? test->name() : "");

    HANDLE process = GetCurrentProcess();
    HANDLE thread  = GetCurrentThread();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(process, nullptr, TRUE);

    CONTEXT context = *info->ContextRecord;
    STACKFRAME64 frame = {};
    DWORD machine;
#if defined(_M_X64)
    machine                = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_ARM64)
    machine                = IMAGE_FILE_MACHINE_ARM64;
    frame.AddrPC.Offset    = context.Pc;
    frame.AddrFrame.Offset = context.Fp;
    frame.AddrStack.Offset = context.Sp;
#else
    machine = 0;
#endif
    frame.AddrPC.Mode    = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    for (int depth = 0; machine && depth < 64; ++depth)
    {
        if (!StackWalk64(machine, process, thread, &frame, &context, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr) ||
            frame.AddrPC.Offset == 0)
        {
            break;
        }

        char symbol_buffer[sizeof(SYMBOL_INFO) + 512] = {};
        SYMBOL_INFO* symbol  = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = 511;
        DWORD64 displacement = 0;

        IMAGEHLP_MODULE64 module = {};
        module.SizeOfStruct      = sizeof(module);
        const char* module_name  = SymGetModuleInfo64(process, frame.AddrPC.Offset, &module) ? module.ModuleName : "?";

        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct    = sizeof(line);
        DWORD line_displacement = 0;

        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
        {
            if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_displacement, &line))
            {
                fprintf(stderr, "  #%-2d %s!%s  [%s:%lu]\n", depth, module_name, symbol->Name, line.FileName, line.LineNumber);
            }
            else
            {
                fprintf(stderr, "  #%-2d %s!%s+0x%llx\n", depth, module_name, symbol->Name, displacement);
            }
        }
        else
        {
            fprintf(stderr, "  #%-2d %s!0x%llx\n", depth, module_name, frame.AddrPC.Offset);
        }
    }
    fflush(stderr);

    return EXCEPTION_CONTINUE_SEARCH; // let the process die as it would have
}

} // namespace
#else
#include <signal.h>
#endif

int main(int argc, char** argv)
{
#if defined(_WIN32)
    // Never pop up modal assert/abort dialogs: a test run on a build machine
    // has nobody to click them, so report to stderr and fail instead.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    // Report crashes on any thread with a stack trace, and without the
    // Windows Error Reporting dialog.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(printCrashStack);
#else
    // Run with SIGPIPE ignored, as the viewer does (LLApp's signal setup).
    // webrtc suppresses SIGPIPE itself only on Linux (MSG_NOSIGNAL); on
    // macOS it leaves that to the embedding app.  Without this, a send on a
    // TCP connection whose other end has gone -- routine in the tests where
    // the server stops responding or is torn down -- kills the whole test
    // process instead of failing with EPIPE.
    signal(SIGPIPE, SIG_IGN);
#endif

    // Initializes both GoogleMock and GoogleTest, and strips the flags
    // they recognize from argv.
    testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
