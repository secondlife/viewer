/** 
 * @file llstacktrace.cpp
 * @brief stack tracing functionality
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"
#include "llstacktrace.h"

#ifdef LL_WINDOWS

#include <iostream>
#include <sstream>

#include "llwin32headerslean.h"
#pragma warning (push)
#pragma warning (disable:4091) // a microsoft header has warnings. Very nice.
#include <dbghelp.h>
#pragma warning (pop)

typedef USHORT NTAPI RtlCaptureStackBackTrace_Function(
    IN ULONG frames_to_skip,
    IN ULONG frames_to_capture,
    OUT PVOID *backtrace,
    OUT PULONG backtrace_hash);

static RtlCaptureStackBackTrace_Function* const RtlCaptureStackBackTrace_fn =
   (RtlCaptureStackBackTrace_Function*)
   GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlCaptureStackBackTrace");

bool ll_get_stack_trace(std::vector<std::string>& lines)
{
    const S32 MAX_STACK_DEPTH = 32;
    const S32 STRING_NAME_LENGTH = 200;
    const S32 FRAME_SKIP = 2;
    static BOOL symbolsLoaded = false;
    static BOOL firstCall = true;

    HANDLE hProc = GetCurrentProcess();

    // load the symbols if they're not loaded
    if(!symbolsLoaded && firstCall)
    {
        symbolsLoaded = SymInitialize(hProc, NULL, true);
        firstCall = false;
    }

    // if loaded, get the call stack
    if(symbolsLoaded)
    {
        // create the frames to hold the addresses
        void* frames[MAX_STACK_DEPTH];
        memset(frames, 0, sizeof(void*)*MAX_STACK_DEPTH);
        S32 depth = 0;

        // get the addresses
        depth = RtlCaptureStackBackTrace_fn(FRAME_SKIP, MAX_STACK_DEPTH, frames, NULL);

        IMAGEHLP_LINE64 line;
        memset(&line, 0, sizeof(IMAGEHLP_LINE64));
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        // create something to hold address info
        PIMAGEHLP_SYMBOL64 pSym;
        pSym = (PIMAGEHLP_SYMBOL64)malloc(sizeof(IMAGEHLP_SYMBOL64) + STRING_NAME_LENGTH);
        memset(pSym, 0, sizeof(IMAGEHLP_SYMBOL64) + STRING_NAME_LENGTH);
        pSym->MaxNameLength = STRING_NAME_LENGTH;
        pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);

        // get address info for each address frame
        // and store
        for(S32 i=0; i < depth; i++)
        {
            std::stringstream stack_line;
            BOOL ret;

            DWORD64 addr = (DWORD64)frames[i];
            ret = SymGetSymFromAddr64(hProc, addr, 0, pSym);
            if(ret)
            {
                stack_line << pSym->Name << " ";
            }

            DWORD dummy;
            ret = SymGetLineFromAddr64(hProc, addr, &dummy, &line);
            if(ret)
            {
                std::string file_name = line.FileName;
                std::string::size_type index = file_name.rfind("\\");
                stack_line << file_name.substr(index + 1, file_name.size()) << ":" << line.LineNumber; 
            }

            lines.push_back(stack_line.str());
        }
        
        free(pSym);

        // TODO: figure out a way to cleanup symbol loading
        // Not hugely necessary, however.
        //SymCleanup(hProc);
        return true;
    }
    else
    {
        lines.push_back("Stack Trace Failed.  PDB symbol info not loaded");
    }

    return false;
}

void ll_get_stack_trace_internal(std::vector<std::string>& lines)
{
    const S32 MAX_STACK_DEPTH = 100;
    const S32 STRING_NAME_LENGTH = 256;

    HANDLE process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );

    void *stack[MAX_STACK_DEPTH];

    unsigned short frames = RtlCaptureStackBackTrace_fn( 0, MAX_STACK_DEPTH, stack, NULL );
    SYMBOL_INFO *symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + STRING_NAME_LENGTH * sizeof(char), 1);
    symbol->MaxNameLen = STRING_NAME_LENGTH-1;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for(unsigned int i = 0; i < frames; i++) 
    {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        lines.push_back(symbol->Name);
    }

    free( symbol );
}

#else

bool ll_get_stack_trace(std::vector<std::string>& lines)
{
    return false;
}

void ll_get_stack_trace_internal(std::vector<std::string>& lines)
{

}

#endif

