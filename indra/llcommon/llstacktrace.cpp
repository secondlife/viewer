/** 
 * @file llstacktrace.cpp
 * @brief stack tracing functionality
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llstacktrace.h"

#ifdef LL_WINDOWS

#include <iostream>
#include <sstream>

#include "windows.h"
#include "Dbghelp.h"

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

#else

bool ll_get_stack_trace(std::vector<std::string>& lines)
{
	return false;
}

#endif

