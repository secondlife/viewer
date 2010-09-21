/** 
 * @file llnamevalue.cpp
 * @brief class for defining name value pairs.
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

// Examples:
// AvatarCharacter STRING RW DSV male1

#include "linden_common.h"

#include "llnamevalue.h"

#include "u64.h"
#include "llstring.h"
#include "string_table.h"

// Anonymous enumeration to provide constants in this file.
// *NOTE: These values may be used in sscanf statements below as their
// value-1, so search for '2047' if you cange NV_BUFFER_LEN or '63' if
// you change U64_BUFFER_LEN.
enum
{
	NV_BUFFER_LEN = 2048,
	U64_BUFFER_LEN = 64
};

LLStringTable	gNVNameTable(256);

char NameValueTypeStrings[NVT_EOF][NAME_VALUE_TYPE_STRING_LENGTH] = /*Flawfinder: Ignore*/
{
	"NULL",
	"STRING",
	"F32",
	"S32",
	"VEC3",
	"U32",
	"CAMERA", // Deprecated, but leaving in case removing completely would cause problems
	"ASSET",
	"U64"
};		

char NameValueClassStrings[NVC_EOF][NAME_VALUE_CLASS_STRING_LENGTH] = /*Flawfinder: Ignore*/
{
	"NULL",
	"R",			// read only
	"RW"			// read write
};		

char NameValueSendtoStrings[NVS_EOF][NAME_VALUE_SENDTO_STRING_LENGTH] = /*Flawfinder: Ignore*/
{
	"NULL",
	"S",	// "Sim", formerly SIM
	"DS",	// "Data Sim" formerly SIM_SPACE
	"SV",	// "Sim Viewer" formerly SIM_VIEWER
	"DSV"	// "Data Sim Viewer", formerly SIM_SPACE_VIEWER
};		/*Flawfinder: Ignore*/


//
// Class
//

LLNameValue::LLNameValue()
{
	baseInit();
}

void LLNameValue::baseInit()
{
	mNVNameTable = &gNVNameTable;

	mName = NULL;
	mNameValueReference.string = NULL;

	mType = NVT_NULL;
	mStringType = NameValueTypeStrings[NVT_NULL];
	
	mClass = NVC_NULL;
	mStringClass = NameValueClassStrings[NVC_NULL];
	
	mSendto = NVS_NULL;
	mStringSendto = NameValueSendtoStrings[NVS_NULL];
}

void LLNameValue::init(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto)
{
	mNVNameTable = &gNVNameTable;

	mName = mNVNameTable->addString(name);
	
	// Nota Bene: Whatever global structure manages this should have these in the name table already!
	mStringType = mNVNameTable->addString(type);
	if (!strcmp(mStringType, "STRING"))
	{
		S32 string_length = (S32)strlen(data);		/*Flawfinder: Ignore*/
		mType = NVT_STRING;

		delete[] mNameValueReference.string;
		
		// two options here. . .  data can either look like foo or "foo"
		// WRONG! - this is a poorly implemented and incomplete escape
		// mechanism. For example, using this scheme, there is no way
		// to tell an intentional double quotes from a zero length
		// string. This needs to excised. Phoenix
		//if (strchr(data, '\"'))
		//{
		//	string_length -= 2;
		//	mNameValueReference.string = new char[string_length + 1];;
		//	strncpy(mNameValueReference.string, data + 1, string_length);
		//}
		//else
		//{
		mNameValueReference.string = new char[string_length + 1];;
		strncpy(mNameValueReference.string, data, string_length);		/*Flawfinder: Ignore*/
		//}
		mNameValueReference.string[string_length] = 0;
	}
	else if (!strcmp(mStringType, "F32"))
	{
		mType = NVT_F32;
		mNameValueReference.f32 = new F32((F32)atof(data));
	}
	else if (!strcmp(mStringType, "S32"))
	{
		mType = NVT_S32;
		mNameValueReference.s32 = new S32(atoi(data));
	}
	else if (!strcmp(mStringType, "U64"))
	{
		mType = NVT_U64;
		mNameValueReference.u64 = new U64(str_to_U64(ll_safe_string(data)));
	}
	else if (!strcmp(mStringType, "VEC3"))
	{
		mType = NVT_VEC3;
		F32 t1, t2, t3;

		// two options here. . .  data can either look like 0, 1, 2 or <0, 1, 2>

		if (strchr(data, '<'))
		{
			sscanf(data, "<%f, %f, %f>", &t1, &t2, &t3);
		}
		else
		{
			sscanf(data, "%f, %f, %f", &t1, &t2, &t3);
		}

		// finite checks
		if (!llfinite(t1) || !llfinite(t2) || !llfinite(t3))
		{
			t1 = 0.f;
			t2 = 0.f;
			t3 = 0.f;
		}

		mNameValueReference.vec3 = new LLVector3(t1, t2, t3);
	}
	else if (!strcmp(mStringType, "U32"))
	{
		mType = NVT_U32;
		mNameValueReference.u32 = new U32(atoi(data));
	}
	else if(!strcmp(mStringType, (const char*)NameValueTypeStrings[NVT_ASSET]))
	{
		// assets are treated like strings, except that the name has
		// meaning to an LLAssetInfo object
		S32 string_length = (S32)strlen(data);		/*Flawfinder: Ignore*/
		mType = NVT_ASSET;

		// two options here. . .  data can either look like foo or "foo"
		// WRONG! - this is a poorly implemented and incomplete escape
		// mechanism. For example, using this scheme, there is no way
		// to tell an intentional double quotes from a zero length
		// string. This needs to excised. Phoenix
		//if (strchr(data, '\"'))
		//{
		//	string_length -= 2;
		//	mNameValueReference.string = new char[string_length + 1];;
		//	strncpy(mNameValueReference.string, data + 1, string_length);
		//}
		//else
		//{
		mNameValueReference.string = new char[string_length + 1];;
		strncpy(mNameValueReference.string, data, string_length);		/*Flawfinder: Ignore*/
		//}
		mNameValueReference.string[string_length] = 0;
	}
	else
	{
		llwarns << "Unknown name value type string " << mStringType << " for " << mName << llendl;
		mType = NVT_NULL;
	}


	// Nota Bene: Whatever global structure manages this should have these in the name table already!
	if (!strcmp(nvclass, "R") ||
		!strcmp(nvclass, "READ_ONLY"))			// legacy
	{
		mClass = NVC_READ_ONLY;
		mStringClass = mNVNameTable->addString("R");
	}
	else if (!strcmp(nvclass, "RW") ||
			!strcmp(nvclass, "READ_WRITE"))	// legacy
	{
		mClass = NVC_READ_WRITE;
		mStringClass = mNVNameTable->addString("RW");
	}
	else
	{
		// assume it's bad
		mClass = NVC_NULL;
		mStringClass = mNVNameTable->addString(nvclass);
	}

	// Initialize the sendto variable
	if (!strcmp(nvsendto, "S") ||
		!strcmp(nvsendto, "SIM"))			// legacy
	{
		mSendto = NVS_SIM;
		mStringSendto = mNVNameTable->addString("S");
	}
	else if (!strcmp(nvsendto, "DS") ||
		!strcmp(nvsendto, "SIM_SPACE"))	// legacy
	{
		mSendto = NVS_DATA_SIM;
		mStringSendto = mNVNameTable->addString("DS");
	}
	else if (!strcmp(nvsendto, "SV") ||
			!strcmp(nvsendto, "SIM_VIEWER"))	// legacy
	{
		mSendto = NVS_SIM_VIEWER;
		mStringSendto = mNVNameTable->addString("SV");
	}
	else if (!strcmp(nvsendto, "DSV") ||
			!strcmp(nvsendto, "SIM_SPACE_VIEWER"))	// legacy
	{
		mSendto = NVS_DATA_SIM_VIEWER;
		mStringSendto = mNVNameTable->addString("DSV");
	}
	else
	{
		llwarns << "LLNameValue::init() - unknown sendto field " 
				<< nvsendto << " for NV " << mName << llendl;
		mSendto = NVS_NULL;
		mStringSendto = mNVNameTable->addString("S");
	}

}


LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass)
{
	baseInit();
	// if not specified, send to simulator only
	init(name, data, type, nvclass, "SIM");
}


LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto)
{
	baseInit();
	init(name, data, type, nvclass, nvsendto);
}



// Initialize without any initial data.
LLNameValue::LLNameValue(const char *name, const char *type, const char *nvclass)
{
	baseInit();
	mName = mNVNameTable->addString(name);
	
	// Nota Bene: Whatever global structure manages this should have these in the name table already!
	mStringType = mNVNameTable->addString(type);
	if (!strcmp(mStringType, "STRING"))
	{
		mType = NVT_STRING;
		mNameValueReference.string = NULL;
	}
	else if (!strcmp(mStringType, "F32"))
	{
		mType = NVT_F32;
		mNameValueReference.f32 = NULL;
	}
	else if (!strcmp(mStringType, "S32"))
	{
		mType = NVT_S32;
		mNameValueReference.s32 = NULL;
	}
	else if (!strcmp(mStringType, "VEC3"))
	{
		mType = NVT_VEC3;
		mNameValueReference.vec3 = NULL;
	}
	else if (!strcmp(mStringType, "U32"))
	{
		mType = NVT_U32;
		mNameValueReference.u32 = NULL;
	}
	else if (!strcmp(mStringType, "U64"))
	{
		mType = NVT_U64;
		mNameValueReference.u64 = NULL;
	}
	else if(!strcmp(mStringType, (const char*)NameValueTypeStrings[NVT_ASSET]))
	{
		mType = NVT_ASSET;
		mNameValueReference.string = NULL;
	}
	else
	{
		mType = NVT_NULL;
		llinfos << "Unknown name-value type " << mStringType << llendl;
	}

	// Nota Bene: Whatever global structure manages this should have these in the name table already!
	mStringClass = mNVNameTable->addString(nvclass);
	if (!strcmp(mStringClass, "READ_ONLY"))
	{
		mClass = NVC_READ_ONLY;
	}
	else if (!strcmp(mStringClass, "READ_WRITE"))
	{
		mClass = NVC_READ_WRITE;
	}
	else
	{
		mClass = NVC_NULL;
	}

	// Initialize the sendto variable
	mStringSendto = mNVNameTable->addString("SIM");
	mSendto = NVS_SIM;
}


// data is in the format:
// "NameValueName	Type	Class	Data"
LLNameValue::LLNameValue(const char *data)
{
	baseInit();
	static char name[NV_BUFFER_LEN];	/*Flawfinder: ignore*/
	static char type[NV_BUFFER_LEN];	/*Flawfinder: ignore*/
	static char nvclass[NV_BUFFER_LEN];	/*Flawfinder: ignore*/
	static char nvsendto[NV_BUFFER_LEN];	/*Flawfinder: ignore*/
	static char nvdata[NV_BUFFER_LEN];	/*Flawfinder: ignore*/

	S32 i;

	S32	character_count = 0;
	S32	length = 0;

	// go to first non-whitespace character
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}

	// read in the name
	sscanf((data + character_count), "%2047s", name);	/*Flawfinder: ignore*/

	// bump past it and add null terminator
	length = (S32)strlen(name);			/* Flawfinder: ignore */
	name[length] = 0;
	character_count += length;

	// go to the next non-whitespace character
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}

	// read in the type
	sscanf((data + character_count), "%2047s", type);	/*Flawfinder: ignore*/

	// bump past it and add null terminator
	length = (S32)strlen(type);		/* Flawfinder: ignore */
	type[length] = 0;
	character_count += length;

	// go to the next non-whitespace character
	while (1)
	{
		if (  (*(data + character_count) == ' ')
			||(*(data + character_count) == '\n')
			||(*(data + character_count) == '\t')
			||(*(data + character_count) == '\r'))
		{
			character_count++;
		}
		else
		{
			break;
		}
	}

	// do we have a type argument?
	for (i = NVC_READ_ONLY; i < NVC_EOF; i++)
	{
		if (!strncmp(NameValueClassStrings[i], data + character_count, strlen(NameValueClassStrings[i])))		/* Flawfinder: ignore */
		{
			break;
		}
	}

	if (i != NVC_EOF)
	{
		// yes we do!
		// read in the class
		sscanf((data + character_count), "%2047s", nvclass);	/*Flawfinder: ignore*/

		// bump past it and add null terminator
		length = (S32)strlen(nvclass);		/* Flawfinder: ignore */
		nvclass[length] = 0;
		character_count += length;

		// go to the next non-whitespace character
		while (1)
		{
			if (  (*(data + character_count) == ' ')
				||(*(data + character_count) == '\n')
				||(*(data + character_count) == '\t')
				||(*(data + character_count) == '\r'))
			{
				character_count++;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		// no type argument given, default to read-write
		strncpy(nvclass, "READ_WRITE", sizeof(nvclass) -1);		/* Flawfinder: ignore */
		nvclass[sizeof(nvclass) -1] = '\0';
	}

	// Do we have a sendto argument?
	for (i = NVS_SIM; i < NVS_EOF; i++)
	{
		if (!strncmp(NameValueSendtoStrings[i], data + character_count, strlen(NameValueSendtoStrings[i])))		/* Flawfinder: ignore */
		{
			break;
		}
	}

	if (i != NVS_EOF)
	{
		// found a sendto argument
		sscanf((data + character_count), "%2047s", nvsendto);	/*Flawfinder: ignore*/

		// add null terminator
		length = (S32)strlen(nvsendto);		/* Flawfinder: ignore */
		nvsendto[length] = 0;
		character_count += length;

		// seek to next non-whitespace characer
		while (1)
		{
			if (  (*(data + character_count) == ' ')
				||(*(data + character_count) == '\n')
				||(*(data + character_count) == '\t')
				||(*(data + character_count) == '\r'))
			{
				character_count++;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		// no sendto argument given, default to sim only
		strncpy(nvsendto, "SIM", sizeof(nvsendto) -1);		/* Flawfinder: ignore */
		nvsendto[sizeof(nvsendto) -1] ='\0';
	}


	// copy the rest character by character into data
	length = 0;

	while ( (*(nvdata + length++) = *(data + character_count++)) )
		;

	init(name, nvdata, type, nvclass, nvsendto);
}


LLNameValue::~LLNameValue()
{
	mNVNameTable->removeString(mName);
	mName = NULL;
	
	switch(mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		delete [] mNameValueReference.string;
		mNameValueReference.string = NULL;
		break;
	case NVT_F32:
		delete mNameValueReference.f32;
		mNameValueReference.string = NULL;
		break;
	case NVT_S32:
		delete mNameValueReference.s32;
		mNameValueReference.string = NULL;
		break;
	case NVT_VEC3:
		delete mNameValueReference.vec3;
		mNameValueReference.string = NULL;
		break;
	case NVT_U32:
		delete mNameValueReference.u32;
		mNameValueReference.u32 = NULL;
		break;
	case NVT_U64:
		delete mNameValueReference.u64;
		mNameValueReference.u64 = NULL;
		break;
	default:
		break;
	}

	delete[] mNameValueReference.string;
	mNameValueReference.string = NULL;
}

char	*LLNameValue::getString()
{
	if (mType == NVT_STRING)
	{
		return mNameValueReference.string;
	}
	else
	{
		llerrs << mName << " not a string!" << llendl;
		return NULL;
	}
}

const char *LLNameValue::getAsset() const
{
	if (mType == NVT_ASSET)
	{
		return mNameValueReference.string;
	}
	else
	{
		llerrs << mName << " not an asset!" << llendl;
		return NULL;
	}
}

F32		*LLNameValue::getF32()
{
	if (mType == NVT_F32)
	{
		return mNameValueReference.f32;
	}
	else
	{
		llerrs << mName << " not a F32!" << llendl;
		return NULL;
	}
}

S32		*LLNameValue::getS32()
{
	if (mType == NVT_S32)
	{
		return mNameValueReference.s32;
	}
	else
	{
		llerrs << mName << " not a S32!" << llendl;
		return NULL;
	}
}

U32		*LLNameValue::getU32()
{
	if (mType == NVT_U32)
	{
		return mNameValueReference.u32;
	}
	else
	{
		llerrs << mName << " not a U32!" << llendl;
		return NULL;
	}
}

U64		*LLNameValue::getU64()
{
	if (mType == NVT_U64)
	{
		return mNameValueReference.u64;
	}
	else
	{
		llerrs << mName << " not a U64!" << llendl;
		return NULL;
	}
}

void	LLNameValue::getVec3(LLVector3 &vec)
{
	if (mType == NVT_VEC3)
	{
		vec = *mNameValueReference.vec3;
	}
	else
	{
		llerrs << mName << " not a Vec3!" << llendl;
	}
}

LLVector3	*LLNameValue::getVec3()
{
	if (mType == NVT_VEC3)
	{
		 return (mNameValueReference.vec3);
	}
	else
	{
		llerrs << mName << " not a Vec3!" << llendl;
		return NULL;
	}
}


BOOL LLNameValue::sendToData() const
{
	return (mSendto == NVS_DATA_SIM || mSendto == NVS_DATA_SIM_VIEWER);
}


BOOL LLNameValue::sendToViewer() const
{
	return (mSendto == NVS_SIM_VIEWER || mSendto == NVS_DATA_SIM_VIEWER);
}


LLNameValue &LLNameValue::operator=(const LLNameValue &a)
{
	if (mType != a.mType)
	{
		return *this;
	}
	if (mClass == NVC_READ_ONLY)
		return *this;

	switch(a.mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		if (mNameValueReference.string)
			delete [] mNameValueReference.string;

		mNameValueReference.string = new char [strlen(a.mNameValueReference.string) + 1];		/* Flawfinder: ignore */
		if(mNameValueReference.string != NULL)
		{
			strcpy(mNameValueReference.string, a.mNameValueReference.string);		/* Flawfinder: ignore */
		}
		break;
	case NVT_F32:
		*mNameValueReference.f32 = *a.mNameValueReference.f32;
		break;
	case NVT_S32:
		*mNameValueReference.s32 = *a.mNameValueReference.s32;
		break;
	case NVT_VEC3:
		*mNameValueReference.vec3 = *a.mNameValueReference.vec3;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = *a.mNameValueReference.u32;
		break;
	case NVT_U64:
		*mNameValueReference.u64 = *a.mNameValueReference.u64;
		break;
	default:
		llerrs << "Unknown Name value type " << (U32)a.mType << llendl;
		break;
	}

	return *this;
}

void LLNameValue::setString(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_STRING:
		if (a)
		{
			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}

			mNameValueReference.string = new char [strlen(a) + 1];		/* Flawfinder: ignore */
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);		/* Flawfinder: ignore */
			}
		}
		else
		{
			if (mNameValueReference.string)
				delete [] mNameValueReference.string;

			mNameValueReference.string = new char [1];
			mNameValueReference.string[0] = 0;
		}
		break;
	default:
		break;
	}

	return;
}


void LLNameValue::setAsset(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_ASSET:
		if (a)
		{
			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}
			mNameValueReference.string = new char [strlen(a) + 1];			/* Flawfinder: ignore */
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);		/* Flawfinder: ignore */
			}
		}
		else
		{
			if (mNameValueReference.string)
				delete [] mNameValueReference.string;

			mNameValueReference.string = new char [1];
			mNameValueReference.string[0] = 0;
		}
		break;
	default:
		break;
	}
}


void LLNameValue::setF32(const F32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_F32:
		*mNameValueReference.f32 = a;
		break;
	default:
		break;
	}

	return;
}


void LLNameValue::setS32(const S32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_S32:
		*mNameValueReference.s32 = a;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = a;
		break;
	case NVT_F32:
		*mNameValueReference.f32 = (F32)a;
		break;
	default:
		break;
	}

	return;
}


void LLNameValue::setU32(const U32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_S32:
		*mNameValueReference.s32 = a;
		break;
	case NVT_U32:
		*mNameValueReference.u32 = a;
		break;
	case NVT_F32:
		*mNameValueReference.f32 = (F32)a;
		break;
	default:
		llerrs << "NameValue: Trying to set U32 into a " << mStringType << ", unknown conversion" << llendl;
		break;
	}
	return;
}


void LLNameValue::setVec3(const LLVector3 &a)
{
	if (mClass == NVC_READ_ONLY)
		return;

	switch(mType)
	{
	case NVT_VEC3:
		*mNameValueReference.vec3 = a;
		break;
	default:
		llerrs << "NameValue: Trying to set LLVector3 into a " << mStringType << ", unknown conversion" << llendl;
		break;
	}
	return;
}


std::string LLNameValue::printNameValue() const
{
	std::string buffer;
	buffer = llformat("%s %s %s %s ", mName, mStringType, mStringClass, mStringSendto);
	buffer += printData();
//	llinfos << "Name Value Length: " << buffer.size() + 1 << llendl;
	return buffer;
}

std::string LLNameValue::printData() const
{
	std::string buffer;
	switch(mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		buffer = mNameValueReference.string;
		break;
	case NVT_F32:
		buffer = llformat("%f", *mNameValueReference.f32);
		break;
	case NVT_S32:
		buffer = llformat("%d", *mNameValueReference.s32);
		break;
	case NVT_U32:
	  	buffer = llformat("%u", *mNameValueReference.u32);
		break;
	case NVT_U64:
		{
			char u64_string[U64_BUFFER_LEN];	/* Flawfinder: ignore */
			U64_to_str(*mNameValueReference.u64, u64_string, sizeof(u64_string));
			buffer = u64_string;
		}
		break;
	case NVT_VEC3:
	  	buffer = llformat( "%f, %f, %f", mNameValueReference.vec3->mV[VX], mNameValueReference.vec3->mV[VY], mNameValueReference.vec3->mV[VZ]);
		break;
	default:
		llerrs << "Trying to print unknown NameValue type " << mStringType << llendl;
		break;
	}
	return buffer;
}

std::ostream&		operator<<(std::ostream& s, const LLNameValue &a)
{
	switch(a.mType)
	{
	case NVT_STRING:
	case NVT_ASSET:
		s << a.mNameValueReference.string;
		break;
	case NVT_F32:
		s << (*a.mNameValueReference.f32);
		break;
	case NVT_S32:
		s << *(a.mNameValueReference.s32);
		break;
	case NVT_U32:
		s << *(a.mNameValueReference.u32);
		break;
	case NVT_U64:
		{
			char u64_string[U64_BUFFER_LEN];	/* Flawfinder: ignore */
			U64_to_str(*a.mNameValueReference.u64, u64_string, sizeof(u64_string));
			s << u64_string;
		}
		break;
	case NVT_VEC3:
		s << *(a.mNameValueReference.vec3);
		break;
	default:
		llerrs << "Trying to print unknown NameValue type " << a.mStringType << llendl;
		break;
	}
	return s;
}

