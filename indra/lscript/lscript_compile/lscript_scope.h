/** 
 * @file lscript_scope.h
 * @brief builds nametable and checks scope
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

#ifndef LL_LSCRIPT_SCOPE_H
#define LL_LSCRIPT_SCOPE_H

#include "string_table.h"
#include "llmap.h"
#include "lscript_byteformat.h"

typedef enum e_lscript_identifier_type
{
	LIT_INVALID,
	LIT_GLOBAL,
	LIT_VARIABLE,
	LIT_FUNCTION,
	LIT_LABEL,
	LIT_STATE,
	LIT_HANDLER,
	LIT_LIBRARY_FUNCTION,
	LIT_EOF
} LSCRIPTIdentifierType;

const char LSCRIPTFunctionTypeStrings[LST_EOF] =	 	/*Flawfinder: ignore*/
{
	'0',
	'i',
	'f',
	's',
	'k',
	'v',
	'q',
	'l',
	'0'
};

const char * const LSCRIPTListDescription[LST_EOF] =	/*Flawfinder: ignore*/
{
   "PUSHARGB 0",
   "PUSHARGB 1",
   "PUSHARGB 2",
   "PUSHARGB 3",
   "PUSHARGB 4",
   "PUSHARGB 5",
   "PUSHARGB 6",
   "PUSHARGB 7",
   "PUSHARGB 0"
};

const char * const LSCRIPTTypePush[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"PUSHE",
	"PUSHE",
	"PUSHE",
	"PUSHE",
	"PUSHEV",
	"PUSHEQ",
	"PUSHE",
	"undefined"
};

const char * const LSCRIPTTypeReturn[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"LOADP -12",
	"LOADP -12",
	"STORES -12\nPOP",
	"STORES -12\nPOP",
	"LOADVP -20",
	"LOADQP -24",
	"LOADLP -12",
	"undefined"
};

const char * const LSCRIPTTypePop[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"POP",
	"POP",
	"POPS",
	"POPS",
	"POPV",
	"POPQ",
	"POPL",
	"undefined"
};

const char * const LSCRIPTTypeDuplicate[LST_EOF] = 	 	/*Flawfinder: ignore*/
{
	"INVALID",
	"DUP",
	"DUP",
	"DUPS",
	"DUPS",
	"DUPV",
	"DUPQ",
	"DUPL",
	"undefined"
};

const char * const LSCRIPTTypeLocalStore[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"STORE ",
	"STORE ",
	"STORES ",
	"STORES ",
	"STOREV ",
	"STOREQ ",
	"STOREL ",
	"undefined"
};

const char * const LSCRIPTTypeLocalDeclaration[LST_EOF] = 	 	/*Flawfinder: ignore*/
{
	"INVALID",
	"STOREP ",
	"STOREP ",
	"STORESP ",
	"STORESP ",
	"STOREVP ",
	"STOREQP ",
	"STORELP ",
	"undefined"
};

const char * const LSCRIPTTypeGlobalStore[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"STOREG ",
	"STOREG ",
	"STORESG ",
	"STORESG ",
	"STOREGV ",
	"STOREGQ ",
	"STORELG ",
	"undefined"
};

const char * const LSCRIPTTypeLocalPush[LST_EOF] = 	 	/*Flawfinder: ignore*/
{
	"INVALID",
	"PUSH ",
	"PUSH ",
	"PUSHS ",
	"PUSHS ",
	"PUSHV ",
	"PUSHQ ",
	"PUSHL ",
	"undefined"
};

const char * const LSCRIPTTypeLocalPush1[LST_EOF] = 	 	/*Flawfinder: ignore*/
{
	"INVALID",
	"PUSHARGI 1",
	"PUSHARGF 1",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"undefined",
	"undefined"
};

const char * const LSCRIPTTypeGlobalPush[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"INVALID",
	"PUSHG ",
	"PUSHG ",
	"PUSHGS ",
	"PUSHGS ",
	"PUSHGV ",
	"PUSHGQ ",
	"PUSHGL ",
	"undefined"
};

class LLScriptSimpleAssignable;

class LLScriptArgString
{
public:
	LLScriptArgString() : mString(NULL) {}
	~LLScriptArgString() { delete [] mString; }

	LSCRIPTType getType(S32 count)
	{
		if (!mString)
			return LST_NULL;
		S32 length = (S32)strlen(mString);	 	/*Flawfinder: ignore*/
		if (count >= length)
		{
			return LST_NULL;
		}
		switch(mString[count])
		{
		case 'i':
			return LST_INTEGER;
		case 'f':
			return LST_FLOATINGPOINT;
		case 's':
			return LST_STRING;
		case 'k':
			return LST_KEY;
		case 'v':
			return LST_VECTOR;
		case 'q':
			return LST_QUATERNION;
		case 'l':
			return LST_LIST;
		default:
			return LST_NULL;
		}
	}

	void addType(LSCRIPTType type)
	{
		S32 count = 0;
		if (mString)
		{
			count = (S32)strlen(mString);	 	/*Flawfinder: ignore*/
			char *temp = new char[count + 2];
			memcpy(temp, mString, count);	 	/*Flawfinder: ignore*/
			delete [] mString;
			mString = temp;
			mString[count + 1] = 0;
		}
		else
		{
			mString = new char[count + 2];
			mString[count + 1] = 0;
		}
		mString[count++] = LSCRIPTFunctionTypeStrings[type];
	}

	S32 getNumber()
	{
		if (mString)
			return (S32)strlen(mString);	 	/*Flawfinder: ignore*/
		else
			return 0;
	}

	char *mString;
};

class LLScriptScopeEntry
{
public:
	LLScriptScopeEntry(char *identifier, LSCRIPTIdentifierType idtype, LSCRIPTType type, S32 count = 0)
		: mIdentifier(identifier), mIDType(idtype), mType(type), mOffset(0), mSize(0), mAssignable(NULL), mCount(count), mLibraryNumber(0)
	{
	}

	~LLScriptScopeEntry() {}

	char						*mIdentifier;
	LSCRIPTIdentifierType		mIDType;
	LSCRIPTType					mType;
	S32							mOffset;
	S32							mSize;
	LLScriptSimpleAssignable	*mAssignable;
	S32							mCount; // NOTE: Index for locals in CIL.
	U16							mLibraryNumber;
	LLScriptArgString			mFunctionArgs;
	LLScriptArgString			mLocals;
};

class LLScriptScope
{
public:
	LLScriptScope(LLStringTable *stable)
		: mParentScope(NULL), mSTable(stable), mFunctionCount(0), mStateCount(0)
	{ 
	}

	~LLScriptScope()	
	{
		mEntryMap.deleteAllData();
	}

	LLScriptScopeEntry *addEntry(char *identifier, LSCRIPTIdentifierType idtype, LSCRIPTType type)
	{
		char *name = mSTable->addString(identifier);
		if (!mEntryMap.checkData(name))
		{
			if (idtype == LIT_FUNCTION)
				mEntryMap[name] = new LLScriptScopeEntry(name, idtype, type, mFunctionCount++);
			else if (idtype == LIT_STATE)
				mEntryMap[name] = new LLScriptScopeEntry(name, idtype, type, mStateCount++);
			else
				mEntryMap[name] = new LLScriptScopeEntry(name, idtype, type);
			return mEntryMap[name];
		}
		else
		{
			// identifier already exists at this scope
			return NULL;
		}
	}

	BOOL checkEntry(char *identifier)
	{
		char *name = mSTable->addString(identifier);
		if (mEntryMap.checkData(name))
		{
			return TRUE;
		}
		else
		{
			// identifier already exists at this scope
			return FALSE;
		}
	}

	LLScriptScopeEntry *findEntry(char *identifier)
	{
		char			*name = mSTable->addString(identifier);
		LLScriptScope	*scope = this;

		while (scope)
		{
			if (scope->mEntryMap.checkData(name))
			{
				// cool, we found it at this scope
				return scope->mEntryMap[name];
			}
			scope = scope->mParentScope;
		}
		return NULL;
	}

	LLScriptScopeEntry *findEntryTyped(char *identifier, LSCRIPTIdentifierType idtype)
	{
		char			*name = mSTable->addString(identifier);
		LLScriptScope	*scope = this;

		while (scope)
		{
			if (scope->mEntryMap.checkData(name))
			{
				// need to check type, and if type is function we need to check both types
				if (idtype == LIT_FUNCTION)
				{
					if (scope->mEntryMap[name]->mIDType == LIT_FUNCTION)
					{
						return scope->mEntryMap[name];
					}
					else if (scope->mEntryMap[name]->mIDType == LIT_LIBRARY_FUNCTION)
					{
						return scope->mEntryMap[name];
					}
				}
				else if (scope->mEntryMap[name]->mIDType == idtype)
				{
					// cool, we found it at this scope
					return scope->mEntryMap[name];
				}
			}
			scope = scope->mParentScope;
		}
		return NULL;
	}

	void addParentScope(LLScriptScope *scope)
	{
		mParentScope = scope;
	}

	LLMap<char *, LLScriptScopeEntry *>	mEntryMap;
	LLScriptScope						*mParentScope;
	LLStringTable						*mSTable;
	S32									mFunctionCount;
	S32									mStateCount;
};

extern LLStringTable *gScopeStringTable;



#endif
