/** 
 * @file lscript_error.cpp
 * @brief error reporting class and strings
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lscript_error.h"

S32 gColumn = 0;
S32 gLine = 0;
S32 gInternalColumn = 0;
S32 gInternalLine = 0;

LLScriptGenerateErrorText gErrorToText;

void LLScriptFilePosition::fdotabs(LLFILE *fp, S32 tabs, S32 tabsize)
{
	S32 i;
	for (i = 0; i < tabs * tabsize; i++)
	{
		fprintf(fp, " ");
	}
}

const char* gWarningText[LSWARN_EOF] = 	 	/*Flawfinder: ignore*/
{
	"INVALID",
	"Dead code found beyond return statement"
};

const char* gErrorText[LSERROR_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"Syntax error",
	"Not all code paths return a value",
	"Function returns a value but return statement doesn't",
	"Return statement type doesn't match function return type",
	"Global functions can't change state",
	"Name previously declared within scope",
	"Name not defined within scope",
	"Type mismatch",
	"Expression must act on LValue",
	"Byte code assembly failed -- out of memory",
	"Function call mismatches type or number of arguments",
	"Use of vector or quaternion method on incorrect type",
	"Lists can't be included in lists",
	"Unitialized variables can't be included in lists",
	"Declaration requires a new scope -- use { and }",
	"CIL assembler failed",
	"Bytecode transformer failed",
	"Bytecode verification failed"
};

void LLScriptGenerateErrorText::writeWarning(LLFILE *fp, LLScriptFilePosition *pos, LSCRIPTWarnings warning)
{
	fprintf(fp, "(%d, %d) : WARNING : %s\n", pos->mLineNumber, pos->mColumnNumber, gWarningText[warning]);
	mTotalWarnings++;
}

void LLScriptGenerateErrorText::writeWarning(LLFILE *fp, S32 line, S32 col, LSCRIPTWarnings warning)
{
	fprintf(fp, "(%d, %d) : WARNING : %s\n", line, col, gWarningText[warning]);
	mTotalWarnings++;
}

void LLScriptGenerateErrorText::writeError(LLFILE *fp, LLScriptFilePosition *pos, LSCRIPTErrors error)
{
	fprintf(fp, "(%d, %d) : ERROR : %s\n", pos->mLineNumber, pos->mColumnNumber, gErrorText[error]);
	mTotalErrors++;
}

void LLScriptGenerateErrorText::writeError(LLFILE *fp, S32 line, S32 col, LSCRIPTErrors error)
{
	fprintf(fp, "(%d, %d) : ERROR : %s\n", line, col, gErrorText[error]);
	mTotalErrors++;
}

std::string getLScriptErrorString(LSCRIPTErrors error)
{
	return gErrorText[error];
}
