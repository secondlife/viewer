/** 
 * @file lscript_error.h
 * @brief error reporting class and strings
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LSCRIPT_ERROR_H
#define LL_LSCRIPT_ERROR_H

#include "lscript_scope.h"

typedef enum e_lscript_compile_pass
{
	LSCP_INVALID,
	LSCP_PRETTY_PRINT,
	LSCP_PRUNE,
	LSCP_SCOPE_PASS1,
	LSCP_SCOPE_PASS2,
	LSCP_TYPE,
	LSCP_RESOURCE,
	LSCP_EMIT_ASSEMBLY,
	LSCP_EMIT_BYTE_CODE,
	LSCP_DETERMINE_HANDLERS,
	LSCP_LIST_BUILD_SIMPLE,
	LSCP_TO_STACK,
	LSCP_BUILD_FUNCTION_ARGS,
	LSCP_EMIT_CIL_ASSEMBLY,
	LSCP_EOF
} LSCRIPTCompilePass;

typedef enum e_lscript_prune_type
{
	LSPRUNE_INVALID,
	LSPRUNE_GLOBAL_VOIDS,
	LSPRUNE_GLOBAL_NON_VOIDS,
	LSPRUNE_EVENTS,
	LSPRUNE_DEAD_CODE,
	LSPRUNE_EOF
} LSCRIPTPruneType;

extern S32 gColumn;
extern S32 gLine;
extern S32 gInternalColumn;
extern S32 gInternalLine;


// used to describe where in the file this piece is
class LLScriptByteCodeChunk;

class LLScriptLibData;

class LLScriptFilePosition
{
public:
	LLScriptFilePosition(S32 line, S32 col)
		: mLineNumber(line), mColumnNumber(col), mByteOffset(0), mByteSize(0)
	{
	}

	virtual ~LLScriptFilePosition() {}

	virtual void recurse(LLFILE *fp, S32 tabs, S32 tabsize, 
						LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, 
						LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, 
						LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata) = 0;
	virtual S32 getSize() = 0;

	void fdotabs(LLFILE *fp, S32 tabs, S32 tabsize);

	S32 mLineNumber;
	S32 mColumnNumber;

	S32 mByteOffset;
	S32 mByteSize;
};

typedef enum e_lscript_warnings
{
	LSWARN_INVALID,
	LSWARN_DEAD_CODE,
	LSWARN_EOF
} LSCRIPTWarnings;

typedef enum e_lscript_errors
{
	LSERROR_INVALID,
	LSERROR_SYNTAX_ERROR,
	LSERROR_NO_RETURN,
	LSERROR_INVALID_VOID_RETURN,
	LSERROR_INVALID_RETURN,
	LSERROR_STATE_CHANGE_IN_GLOBAL,
	LSERROR_DUPLICATE_NAME,
	LSERROR_UNDEFINED_NAME,
	LSERROR_TYPE_MISMATCH,
	LSERROR_EXPRESSION_ON_LVALUE,
	LSERROR_ASSEMBLE_OUT_OF_MEMORY,
	LSERROR_FUNCTION_TYPE_ERROR,
	LSERROR_VECTOR_METHOD_ERROR,
	LSERROR_NO_LISTS_IN_LISTS,
	LSERROR_NO_UNITIALIZED_VARIABLES_IN_LISTS,
	LSERROR_NEED_NEW_SCOPE,
	LSERROR_EOF
} LSCRIPTErrors;

class LLScriptGenerateErrorText
{
public:
	LLScriptGenerateErrorText() { init(); }
	~LLScriptGenerateErrorText() {}

	void init() { mTotalErrors = 0; mTotalWarnings = 0; }

	void writeWarning(LLFILE *fp, LLScriptFilePosition *pos, LSCRIPTWarnings warning);
	void writeWarning(LLFILE *fp, S32 line, S32 col, LSCRIPTWarnings warning);
	void writeError(LLFILE *fp, LLScriptFilePosition *pos, LSCRIPTErrors error);
	void writeError(LLFILE *fp, S32 line, S32 col, LSCRIPTErrors error);

	BOOL getErrors() { return mTotalErrors; }
	BOOL getWarnings() { return mTotalWarnings; }

	S32 mTotalErrors;
	S32 mTotalWarnings;
};

extern LLScriptGenerateErrorText gErrorToText;

#endif
