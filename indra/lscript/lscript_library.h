/** 
 * @file lscript_library.h
 * @brief External library interface
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LSCRIPT_LIBRARY_H
#define LL_LSCRIPT_LIBRARY_H

#include "lscript_byteformat.h"
#include "v3math.h"
#include "llquaternion.h"
#include "lluuid.h"
#include "lscript_byteconvert.h"
#include <stdio.h>

class LLScriptLibData;

class LLScriptLibraryFunction
{
public:
	LLScriptLibraryFunction(F32 eu, F32 st, void (*exec_func)(LLScriptLibData *, LLScriptLibData *, const LLUUID &), char *name, char *ret_type, char *args, char *desc, BOOL god_only = FALSE);
	~LLScriptLibraryFunction();

	F32  mEnergyUse;
	F32  mSleepTime;
	void (*mExecFunc)(LLScriptLibData *, LLScriptLibData *, const LLUUID &);
	char *mName;
	char *mReturnType;
	char *mArgs;
	char *mDesc;
	BOOL mGodOnly;
};

class LLScriptLibrary
{
public:
	LLScriptLibrary();
	~LLScriptLibrary();

	void init();

	void addFunction(LLScriptLibraryFunction *func);
	void assignExec(char *name, void (*exec_func)(LLScriptLibData *, LLScriptLibData *, const LLUUID &));

	S32						mNextNumber;
	LLScriptLibraryFunction	**mFunctions;
};

extern LLScriptLibrary gScriptLibrary;

class LLScriptLibData
{
public:
	// TODO: Change this to a union
	LSCRIPTType		mType;
	S32				mInteger;
	F32				mFP;
	char			*mKey;
	char			*mString;
	LLVector3		mVec;
	LLQuaternion	mQuat;
	LLScriptLibData *mListp;

	friend const bool operator<=(const LLScriptLibData &a, const LLScriptLibData &b)
	{
		if (a.mType == b.mType)
		{
			if (a.mType == LST_INTEGER)
			{
				return a.mInteger <= b.mInteger;
			}
			if (a.mType == LST_FLOATINGPOINT)
			{
				return a.mFP <= b.mFP;
			}
			if (a.mType == LST_STRING)
			{
				return strcmp(a.mString, b.mString) <= 0;
			}
			if (a.mType == LST_KEY)
			{
				return strcmp(a.mKey, b.mKey) <= 0;
			}
			if (a.mType == LST_VECTOR)
			{
				return a.mVec.magVecSquared() <= b.mVec.magVecSquared();
			}
		}
		return TRUE;
	}

	friend const bool operator==(const LLScriptLibData &a, const LLScriptLibData &b)
	{
		if (a.mType == b.mType)
		{
			if (a.mType == LST_INTEGER)
			{
				return a.mInteger == b.mInteger;
			}
			if (a.mType == LST_FLOATINGPOINT)
			{
				return a.mFP == b.mFP;
			}
			if (a.mType == LST_STRING)
			{
				return !strcmp(a.mString, b.mString);
			}
			if (a.mType == LST_KEY)
			{
				return !strcmp(a.mKey, b.mKey);
			}
			if (a.mType == LST_VECTOR)
			{
				return a.mVec == b.mVec;
			}
			if (a.mType == LST_QUATERNION)
			{
				return a.mQuat == b.mQuat;
			}
		}
		return FALSE;
	}

	S32 getListLength()
	{
		LLScriptLibData *data = this;
		S32 retval = 0;
		while (data->mListp)
		{
			retval++;
			data = data->mListp;
		}
		return retval;
	}

	BOOL checkForMultipleLists()
	{
		LLScriptLibData *data = this;
		while (data->mListp)
		{
			data = data->mListp;
			if (data->mType == LST_LIST)
				return TRUE;
		}
		return FALSE;
	}

	S32  getSavedSize()
	{
		S32 size = 0;
		// mType
		size += 4;

		switch(mType)
		{
		case LST_INTEGER:
			size += 4;
			break;
		case LST_FLOATINGPOINT:
			size += 4;
			break;
		case LST_KEY:
			size += (S32)strlen(mKey) + 1;	/*Flawfinder: ignore*/
			break;
		case LST_STRING:
			size += (S32)strlen(mString) + 1;	/*Flawfinder: ignore*/
			break;
		case LST_LIST:
			break;
		case LST_VECTOR:
			size += 12;
			break;
		case LST_QUATERNION:
			size += 16;
			break;
		default:
			break;
		}
		return size;
	}

	S32	 write2bytestream(U8 *dest)
	{
		S32 offset = 0;
		integer2bytestream(dest, offset, mType);
		switch(mType)
		{
		case LST_INTEGER:
			integer2bytestream(dest, offset, mInteger);
			break;
		case LST_FLOATINGPOINT:
			float2bytestream(dest, offset, mFP);
			break;
		case LST_KEY:
			char2bytestream(dest, offset, mKey);
			break;
		case LST_STRING:
			char2bytestream(dest, offset, mString);
			break;
		case LST_LIST:
			break;
		case LST_VECTOR:
			vector2bytestream(dest, offset, mVec);
			break;
		case LST_QUATERNION:
			quaternion2bytestream(dest, offset, mQuat);
			break;
		default:
			break;
		}
		return offset;
	}

	LLScriptLibData() : mType(LST_NULL), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
	}

	LLScriptLibData(const LLScriptLibData &data) : mType(data.mType), mInteger(data.mInteger), mFP(data.mFP), mKey(NULL), mString(NULL), mVec(data.mVec), mQuat(data.mQuat), mListp(NULL)
	{
		if (data.mKey)
		{
			mKey = new char[strlen(data.mKey) + 1];	/* Flawfinder: ignore */
			if (mKey == NULL)
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return;
			}
			strcpy(mKey, data.mKey);	/* Flawfinder: ignore */
		}
		if (data.mString)
		{
			mString = new char[strlen(data.mString) + 1];	/* Flawfinder: ignore */
			if (mString == NULL)
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return;
			}
			strcpy(mString, data.mString);	/* Flawfinder: ignore */
		}
	}

	LLScriptLibData(U8 *src, S32 &offset) : mListp(NULL)
	{
		static char temp[TOP_OF_MEMORY];	/* Flawfinder: ignore */
		mType = (LSCRIPTType)bytestream2integer(src, offset);
		switch(mType)
		{
		case LST_INTEGER:
			mInteger = bytestream2integer(src, offset);
			break;
		case LST_FLOATINGPOINT:
			mFP = bytestream2float(src, offset);
			break;
		case LST_KEY:
			{
				bytestream2char(temp, src, offset);
				mKey = new char[strlen(temp) + 1];	/* Flawfinder: ignore */
				if (mKey == NULL)
				{
					llerrs << "Memory Allocation Failed" << llendl;
					return;
				}
				strcpy(mKey, temp);	/* Flawfinder: ignore */
			}
			break;
		case LST_STRING:
			{
				bytestream2char(temp, src, offset);
				mString = new char[strlen(temp) + 1];	/* Flawfinder: ignore */
				if (mString == NULL)
				{
					llerrs << "Memory Allocation Failed" << llendl;
					return;
				}
				strcpy(mString, temp);	/* Flawfinder: ignore */
			}
			break;
		case LST_LIST:
			break;
		case LST_VECTOR:
			bytestream2vector(mVec, src, offset);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(mQuat, src, offset);
			break;
		default:
			break;
		}
	}

	void set(U8 *src, S32 &offset)
	{
		static char temp[TOP_OF_MEMORY];	/* Flawfinder: ignore */
		mType = (LSCRIPTType)bytestream2integer(src, offset);
		switch(mType)
		{
		case LST_INTEGER:
			mInteger = bytestream2integer(src, offset);
			break;
		case LST_FLOATINGPOINT:
			mFP = bytestream2float(src, offset);
			break;
		case LST_KEY:
			{
				bytestream2char(temp, src, offset);
				mKey = new char[strlen(temp) + 1];	/* Flawfinder: ignore */
				if (mKey == NULL)
				{
					llerrs << "Memory Allocation Failed" << llendl;
					return;
				}
				strcpy(mKey, temp);	/* Flawfinder: ignore */
			}
			break;
		case LST_STRING:
			{
				bytestream2char(temp, src, offset);
				mString = new char[strlen(temp) + 1];	/* Flawfinder: ignore */
				if (mString == NULL)
				{
					llerrs << "Memory Allocation Failed" << llendl;
					return;
				}
				strcpy(mString, temp);	/* Flawfinder: ignore */
			}
			break;
		case LST_LIST:
			break;
		case LST_VECTOR:
			bytestream2vector(mVec, src, offset);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(mQuat, src, offset);
			break;
		default:
			break;
		}
	}

	void print(std::ostream &s, BOOL b_prepend_comma);
	void print_separator(std::ostream& ostr, BOOL b_prepend_sep, char* sep);

	void setFromCSV(char *src)
	{
		mType = LST_STRING;
		mString = new char[strlen(src) + 1];	/* Flawfinder: ignore */
		if (mString == NULL)
		{
			llerrs << "Memory Allocation Failed" << llendl;
			return;
		}
		strcpy(mString, src);	/* Flawfinder: ignore */
	}

	LLScriptLibData(S32 integer) : mType(LST_INTEGER), mInteger(integer), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
	}

	LLScriptLibData(F32 fp) : mType(LST_FLOATINGPOINT), mInteger(0), mFP(fp), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
	}

	LLScriptLibData(const LLUUID &id) : mType(LST_KEY), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
		mKey = new char[UUID_STR_LENGTH];
		id.toString(mKey);
	}

	LLScriptLibData(char *string) : mType(LST_STRING), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
		if (!string)
		{
			mString = new char[1];
			mString[0] = 0;
		}
		else
		{
			mString = new char[strlen(string) + 1];	/* Flawfinder: ignore */
			if (mString == NULL)
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return;
			}
			strcpy(mString, string);	/* Flawfinder: ignore */
		}
	}

	LLScriptLibData(const char *string) : mType(LST_STRING), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(), mListp(NULL)
	{
		if (!string)
		{
			mString = new char[1];
			mString[0] = 0;
		}
		else
		{
			mString = new char[strlen(string) + 1];	/* Flawfinder: ignore */
			if (mString == NULL)
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return;
			}
			strcpy(mString, string);	/* Flawfinder: ignore */
		}
	}

	LLScriptLibData(LLVector3 &vec) : mType(LST_VECTOR), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(vec), mQuat(), mListp(NULL)
	{
	}

	LLScriptLibData(LLQuaternion &quat) : mType(LST_QUATERNION), mInteger(0), mFP(0.f), mKey(NULL), mString(NULL), mVec(), mQuat(quat), mListp(NULL)
	{
	}

	~LLScriptLibData()
	{
		delete mListp;
		delete [] mKey;
		delete [] mString;
	}

};

#endif
