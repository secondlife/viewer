/** 
 * @file lscript_error.cpp
 * @brief error reporting class and strings
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lscript_error.h"

S32 gColumn = 0;
S32 gLine = 0;
S32 gInternalColumn = 0;
S32 gInternalLine = 0;

LLScriptGenerateErrorText gErrorToText;

void LLScriptFilePosition::fdotabs(FILE *fp, S32 tabs, S32 tabsize)
{
	S32 i;
	for (i = 0; i < tabs * tabsize; i++)
	{
		fprintf(fp, " ");
	}
}

char *gWarningText[LSWARN_EOF] = 
{
	"INVALID",
	"Dead code found beyond return statement"
};

char *gErrorText[LSERROR_EOF] = 
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
	"Declaration requires a new scope -- use { and }"
};

void LLScriptGenerateErrorText::writeWarning(FILE *fp, LLScriptFilePosition *pos, LSCRIPTWarnings warning)
{
	fprintf(fp, "(%d, %d) : WARNING : %s\n", pos->mLineNumber, pos->mColumnNumber, gWarningText[warning]);
	mTotalWarnings++;
}

void LLScriptGenerateErrorText::writeWarning(FILE *fp, S32 line, S32 col, LSCRIPTWarnings warning)
{
	fprintf(fp, "(%d, %d) : WARNING : %s\n", line, col, gWarningText[warning]);
	mTotalWarnings++;
}

void LLScriptGenerateErrorText::writeError(FILE *fp, LLScriptFilePosition *pos, LSCRIPTErrors error)
{
	fprintf(fp, "(%d, %d) : ERROR : %s\n", pos->mLineNumber, pos->mColumnNumber, gErrorText[error]);
	mTotalErrors++;
}

void LLScriptGenerateErrorText::writeError(FILE *fp, S32 line, S32 col, LSCRIPTErrors error)
{
	fprintf(fp, "(%d, %d) : ERROR : %s\n", line, col, gErrorText[error]);
	mTotalErrors++;
}
