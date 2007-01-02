/** 
 * @file llnamevalue.cpp
 * @brief class for defining name value pairs.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Examples:
// AvatarCharacter STRING RW DSV male1

#include "linden_common.h"

#include <map>

#include "llnamevalue.h"
#include "u64.h"
#include "llstring.h"
#include "llcamera.h"

// Anonymous enumeration to provide constants in this file.
// *NOTE: These values may be used in sscanf statements below as their
// value-1, so search for '2047' if you cange NV_BUFFER_LEN or '63' if
// you change U64_BUFFER_LEN.
enum
{
	NV_BUFFER_LEN = 2048,
	U64_BUFFER_LEN = 64
};

struct user_callback_t
{
	user_callback_t() {};
	user_callback_t(TNameValueCallback cb, void** data) : m_Callback(cb), m_Data(data) {}
	TNameValueCallback	m_Callback;
	void **				m_Data;
};
typedef std::map<char *, user_callback_t> user_callback_map_t;
user_callback_map_t gUserCallbackMap;

LLStringTable	gNVNameTable(16384);

char NameValueTypeStrings[NVT_EOF][NAME_VALUE_TYPE_STRING_LENGTH] =
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
};		/*Flawfinder: Ignore*/

char NameValueClassStrings[NVC_EOF][NAME_VALUE_CLASS_STRING_LENGTH] =
{
	"NULL",
	"R",			// read only
	"RW",			// read write
	"CB"			// callback
};		/*Flawfinder: Ignore*/

char NameValueSendtoStrings[NVS_EOF][NAME_VALUE_SENDTO_STRING_LENGTH] =
{
	"NULL",
	"S",	// "Sim", formerly SIM
	"DS",	// "Data Sim" formerly SIM_SPACE
	"SV",	// "Sim Viewer" formerly SIM_VIEWER
	"DSV"	// "Data Sim Viewer", formerly SIM_SPACE_VIEWER
};		/*Flawfinder: Ignore*/


void add_use_callback(char *name, TNameValueCallback ucb, void **user_data)
{
	char *temp = gNVNameTable.addString(name);
	gUserCallbackMap[temp] = user_callback_t(ucb,user_data);
}


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

void LLNameValue::init(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto, TNameValueCallback nvcb, void **user_data)
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
		mNameValueReference.u64 = new U64(str_to_U64(data));
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
	else if (!strcmp(nvclass, "CB") ||
			!strcmp(nvclass, "CALLBACK"))		// legacy
	{
		mClass = NVC_CALLBACK;
		mStringClass = mNVNameTable->addString("CB");
		mNameValueCB = nvcb;
		mUserData = user_data;
	}
	else
	{
		// assume it's bad
		mClass = NVC_NULL;
		mStringClass = mNVNameTable->addString(nvclass);
		mNameValueCB = NULL;
		mUserData = NULL;

		// are we a user-defined call back?
		for (user_callback_map_t::iterator iter = gUserCallbackMap.begin();
			 iter != gUserCallbackMap.end(); iter++)
		{
			char* tname = iter->first;
			if (tname == mStringClass)
			{
				mClass = NVC_CALLBACK;
				mNameValueCB = (iter->second).m_Callback;
				mUserData = (iter->second).m_Data;
			}
		}

		// Warn if we didn't find a callback
		if (mClass == NVC_NULL)
		{
			llwarns << "Unknown user callback in name value init() for " << mName << llendl;
		}
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


LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, TNameValueCallback nvcb, void **user_data)
{
	baseInit();
	// if not specified, send to simulator only
	init(name, data, type, nvclass, "SIM", nvcb, user_data);
}


LLNameValue::LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto, TNameValueCallback nvcb, void **user_data)
{
	baseInit();
	init(name, data, type, nvclass, nvsendto, nvcb, user_data);
}



// Initialize without any initial data.
LLNameValue::LLNameValue(const char *name, const char *type, const char *nvclass, TNameValueCallback nvcb, void **user_data)
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
	else if (!strcmp(mStringClass, "CALLBACK"))
	{
		mClass = NVC_READ_WRITE;
		mNameValueCB = nvcb;
		mUserData = user_data;
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
	static char name[NV_BUFFER_LEN];
	static char type[NV_BUFFER_LEN];
	static char nvclass[NV_BUFFER_LEN];
	static char nvsendto[NV_BUFFER_LEN];
	static char nvdata[NV_BUFFER_LEN];

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
	sscanf((data + character_count), "%2047s", name);

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
	sscanf((data + character_count), "%2047s", type);

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
		sscanf((data + character_count), "%2047s", nvclass);

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
		sscanf((data + character_count), "%2047s", nvsendto);

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


F32 LLNameValue::magnitude()
{
	switch(mType)
	{
	case NVT_STRING:
		return (F32)(strlen(mNameValueReference.string));		/* Flawfinder: ignore */
		break;
	case NVT_F32:
		return (fabsf(*mNameValueReference.f32));
		break;
	case NVT_S32:
		return (fabsf((F32)(*mNameValueReference.s32)));
		break;
	case NVT_VEC3:
		return (mNameValueReference.vec3->magVec());
		break;
	case NVT_U32:
		return (F32)(*mNameValueReference.u32);
		break;
	default:
		llerrs << "No magnitude operation for NV type " << mStringType << llendl;
		break;
	}
	return 0.f;
}


void LLNameValue::callCallback()
{
	if (mNameValueCB)
	{
		(*mNameValueCB)(this, mUserData);
	}
	else
	{
		llinfos << mName << " has no callback!" << llendl;
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

	BOOL b_changed = FALSE;
	if (  (mClass == NVC_CALLBACK)
		&&(*this != a))
	{
		b_changed = TRUE;
	}

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

	if (b_changed)
	{
		callCallback();
	}

	return *this;
}

void LLNameValue::setString(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_STRING:
		if (a)
		{
			if (  (mClass == NVC_CALLBACK)
				&&(strcmp(this->mNameValueReference.string,a)))
			{
				b_changed = TRUE;
			}

			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}

			mNameValueReference.string = new char [strlen(a) + 1];		/* Flawfinder: ignore */
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);		/* Flawfinder: ignore */
			}
			
			if (b_changed)
			{
				callCallback();
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

	if (b_changed)
	{
		callCallback();
	}

	return;
}


void LLNameValue::setAsset(const char *a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_ASSET:
		if (a)
		{
			if (  (mClass == NVC_CALLBACK)
				&&(strcmp(this->mNameValueReference.string,a)))
			{
				b_changed = TRUE;
			}

			if (mNameValueReference.string)
			{
				delete [] mNameValueReference.string;
			}
			mNameValueReference.string = new char [strlen(a) + 1];			/* Flawfinder: ignore */
			if(mNameValueReference.string != NULL)
			{
				strcpy(mNameValueReference.string,  a);		/* Flawfinder: ignore */
			}
			
			if (b_changed)
			{
				callCallback();
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
	if (b_changed)
	{
		callCallback();
	}
}


void LLNameValue::setF32(const F32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_F32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.f32 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.f32 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	default:
		break;
	}
	if (b_changed)
	{
		callCallback();
	}

	return;
}


void LLNameValue::setS32(const S32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_S32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.s32 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.s32 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	case NVT_U32:
		if (  (mClass == NVC_CALLBACK)
			&& ((S32) (*this->mNameValueReference.u32) != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.u32 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	case NVT_F32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.f32 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.f32 = (F32)a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	default:
		break;
	}
	if (b_changed)
	{
		callCallback();
	}

	return;
}


void LLNameValue::setU32(const U32 a)
{
	if (mClass == NVC_READ_ONLY)
		return;
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_S32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.s32 != (S32) a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.s32 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	case NVT_U32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.u32 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.u32 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	case NVT_F32:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.f32 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.f32 = (F32)a;
		if (b_changed)
		{
			callCallback();
		}
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
	BOOL b_changed = FALSE;

	switch(mType)
	{
	case NVT_VEC3:
		if (  (mClass == NVC_CALLBACK)
			&&(*this->mNameValueReference.vec3 != a))
		{
			b_changed = TRUE;
		}
		*mNameValueReference.vec3 = a;
		if (b_changed)
		{
			callCallback();
		}
		break;
	default:
		llerrs << "NameValue: Trying to set LLVector3 into a " << mStringType << ", unknown conversion" << llendl;
		break;
	}
	return;
}


BOOL LLNameValue::nonzero()
{
	switch(mType)
	{
	case NVT_STRING:
		if (!mNameValueReference.string)
			return 0;
		return (mNameValueReference.string[0] != 0);
	case NVT_F32:
		return (*mNameValueReference.f32 != 0.f);
	case NVT_S32:
		return (*mNameValueReference.s32 != 0);
	case NVT_U32:
		return (*mNameValueReference.u32 != 0);
	case NVT_VEC3:
		return (mNameValueReference.vec3->magVecSquared() != 0.f);
	default:
		llerrs << "NameValue: Trying to call nonzero on a " << mStringType << ", unknown conversion" << llendl;
		break;
	}
	return FALSE;
}

std::string LLNameValue::printNameValue()
{
	std::string buffer;
	buffer = llformat("%s %s %s %s ", mName, mStringType, mStringClass, mStringSendto);
	buffer += printData();
//	llinfos << "Name Value Length: " << buffer.size() + 1 << llendl;
	return buffer;
}

std::string LLNameValue::printData()
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
	case NVT_VEC3:
		s << *(a.mNameValueReference.vec3);
		break;
	default:
		llerrs << "Trying to print unknown NameValue type " << a.mStringType << llendl;
		break;
	}
	return s;
}


// nota bene: return values aren't static for now to prevent memory leaks

LLNameValue	&operator+(const LLNameValue &a, const LLNameValue &b)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		if (b.mType == NVT_STRING)
		{
			retval.mType = a.mType;
			retval.mStringType = NameValueTypeStrings[a.mType];

			S32 length1 = (S32)strlen(a.mNameValueReference.string);		/* Flawfinder: Ignore */
			S32 length2 = (S32)strlen(b.mNameValueReference.string);		/* Flawfinder: Ignore */
			delete [] retval.mNameValueReference.string;
			retval.mNameValueReference.string = new char[length1 + length2 + 1];
			if(retval.mNameValueReference.string != NULL)
			{
				strcpy(retval.mNameValueReference.string, a.mNameValueReference.string);	/* Flawfinder: Ignore */
				strcat(retval.mNameValueReference.string, b.mNameValueReference.string);		/* Flawfinder: Ignore */
			}
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 + *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 + *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 + *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 + *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 + *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 + *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 + *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.u32 + *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_U32;
			retval.mStringType = NameValueTypeStrings[NVT_U32];
			delete retval.mNameValueReference.u32;
			retval.mNameValueReference.u32 = new U32(*a.mNameValueReference.u32 + *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			retval.mType = a.mType;
			retval.mStringType = NameValueTypeStrings[a.mType];
			delete retval.mNameValueReference.vec3;
			retval.mNameValueReference.vec3 = new LLVector3(*a.mNameValueReference.vec3 + *b.mNameValueReference.vec3);
		}
		break;
	default:
		llerrs << "Unknown add of NV type " << a.mStringType << " to " << b.mStringType << llendl;
		break;
	}
	return retval;
}

LLNameValue	&operator-(const LLNameValue &a, const LLNameValue &b)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 - *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 - *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 - *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 - *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 - *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 - *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 - *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.u32 - *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_U32;
			retval.mStringType = NameValueTypeStrings[NVT_U32];
			delete retval.mNameValueReference.u32;
			retval.mNameValueReference.u32 = new U32(*a.mNameValueReference.u32 - *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			retval.mType = a.mType;
			retval.mStringType = NameValueTypeStrings[a.mType];
			delete retval.mNameValueReference.vec3;
			retval.mNameValueReference.vec3 = new LLVector3(*a.mNameValueReference.vec3 - *b.mNameValueReference.vec3);
		}
		break;
	default:
		llerrs << "Unknown subtract of NV type " << a.mStringType << " to " << b.mStringType << llendl;
		break;
	}
	return retval;
}

LLNameValue	&operator*(const LLNameValue &a, const LLNameValue &b)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 * *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 * *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 * *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 * *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 * *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 * *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 * *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.u32 * *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_U32;
			retval.mStringType = NameValueTypeStrings[NVT_U32];
			delete retval.mNameValueReference.u32;
			retval.mNameValueReference.u32 = new U32(*a.mNameValueReference.u32 * *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[a.mType];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32((*a.mNameValueReference.vec3) * (*b.mNameValueReference.vec3));
		}
		break;
	default:
		llerrs << "Unknown multiply of NV type " << a.mStringType << " to " << b.mStringType << llendl;
		break;
	}
	return retval;
}

LLNameValue	&operator/(const LLNameValue &a, const LLNameValue &b)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 / *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 / *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 / *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 / *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 / *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 / *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			retval.mType = NVT_F32;
			retval.mStringType = NameValueTypeStrings[NVT_F32];
			delete retval.mNameValueReference.f32;
			retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 / *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.u32 / *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_U32;
			retval.mStringType = NameValueTypeStrings[NVT_U32];
			delete retval.mNameValueReference.u32;
			retval.mNameValueReference.u32 = new U32(*a.mNameValueReference.u32 / *b.mNameValueReference.u32);
		}
		break;
	default:
		llerrs << "Unknown divide of NV type " << a.mStringType << " to " << b.mStringType << llendl;
		break;
	}
	return retval;
}

LLNameValue	&operator%(const LLNameValue &a, const LLNameValue &b)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		break;
	case NVT_S32:
		if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 % *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.s32 % *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_S32)
		{
			retval.mType = NVT_S32;
			retval.mStringType = NameValueTypeStrings[NVT_S32];
			delete retval.mNameValueReference.s32;
			retval.mNameValueReference.s32 = new S32(*a.mNameValueReference.u32 % *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			retval.mType = NVT_U32;
			retval.mStringType = NameValueTypeStrings[NVT_U32];
			delete retval.mNameValueReference.u32;
			retval.mNameValueReference.u32 = new U32(*a.mNameValueReference.u32 % *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			retval.mType = a.mType;
			retval.mStringType = NameValueTypeStrings[a.mType];
			delete retval.mNameValueReference.vec3;
			retval.mNameValueReference.vec3 = new LLVector3(*a.mNameValueReference.vec3 % *b.mNameValueReference.vec3);
		}
		break;
	default:
		llerrs << "Unknown % of NV type " << a.mStringType << " to " << b.mStringType << llendl;
		break;
	}
	return retval;
}


// Multiplying anything times a float gives you some floats
LLNameValue	&operator*(const LLNameValue &a, F32 k)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 * k);
		break;
	case NVT_S32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 * k);
		break;
	case NVT_U32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 * k);
		break;
	case NVT_VEC3:
		retval.mType = a.mType;
		retval.mStringType = NameValueTypeStrings[a.mType];
		delete retval.mNameValueReference.vec3;
		retval.mNameValueReference.vec3 = new LLVector3(*a.mNameValueReference.vec3 * k);
		break;
	default:
		llerrs << "Unknown multiply of NV type " << a.mStringType << " with F32" << llendl;
		break;
	}
	return retval;
}


LLNameValue	&operator*(F32 k, const LLNameValue &a)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.f32 * k);
		break;
	case NVT_S32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.s32 * k);
		break;
	case NVT_U32:
		retval.mType = NVT_F32;
		retval.mStringType = NameValueTypeStrings[NVT_F32];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(*a.mNameValueReference.u32 * k);
		break;
	case NVT_VEC3:
		retval.mType = a.mType;
		retval.mStringType = NameValueTypeStrings[a.mType];
		delete retval.mNameValueReference.vec3;
		retval.mNameValueReference.vec3 = new LLVector3(*a.mNameValueReference.vec3 * k);
		break;
	default:
		llerrs << "Unknown multiply of NV type " << a.mStringType << " with F32" << llendl;
		break;
	}
	return retval;
}


bool operator==(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (b.mType == NVT_STRING)
		{
			if (!a.mNameValueReference.string)
				return FALSE;
			if (!b.mNameValueReference.string)
				return FALSE;
			return (!strcmp(a.mNameValueReference.string, b.mNameValueReference.string));
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 == *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 == *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 == *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 == *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 == *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 == (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 == *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 == *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 == *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			return (*a.mNameValueReference.vec3 == *b.mNameValueReference.vec3);
		}
		break;
	default:
		llerrs << "Unknown == NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}

bool operator<=(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (b.mType == NVT_STRING)
		{
			S32 retval = strcmp(a.mNameValueReference.string, b.mNameValueReference.string);
			return (retval <= 0);
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 <= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 <= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 <= *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 <= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 <= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 <= (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 <= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 <= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 <= *b.mNameValueReference.u32);
		}
		break;
	default:
		llerrs << "Unknown <= NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}


bool			operator>=(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_STRING))
		{
			S32 retval = strcmp(a.mNameValueReference.string, b.mNameValueReference.string);
			return (retval >= 0);
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 >= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 >= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 >= *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 >= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 >= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 >= (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 >= *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 >= *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 >= *b.mNameValueReference.u32);
		}
		break;
	default:
		llerrs << "Unknown >= NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}


bool			operator<(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_STRING))
		{
			S32 retval = strcmp(a.mNameValueReference.string, b.mNameValueReference.string);
			return (retval < 0);
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 < *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 < *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 < *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 < *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 < *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 < (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 < *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 < *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 < *b.mNameValueReference.u32);
		}
		break;
	default:
		llerrs << "Unknown < NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}


bool			operator>(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_STRING))
		{
			S32 retval = strcmp(a.mNameValueReference.string, b.mNameValueReference.string);
			return (retval > 0);
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 > *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 > *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 > *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 > *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 > *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 > (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 > *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 > *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 > *b.mNameValueReference.u32);
		}
		break;
	default:
		llerrs << "Unknown > NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}

bool			operator!=(const LLNameValue &a, const LLNameValue &b)
{
	switch(a.mType)
	{
	case NVT_STRING:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_STRING))
		{
			return (strcmp(a.mNameValueReference.string, b.mNameValueReference.string)) ? true : false;
		}
		break;
	case NVT_F32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.f32 != *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.f32 != *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.f32 != *b.mNameValueReference.u32);
		}
		break;
	case NVT_S32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.s32 != *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return (*a.mNameValueReference.s32 != *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.s32 != (S32) *b.mNameValueReference.u32);
		}
		break;
	case NVT_U32:
		if (b.mType == NVT_F32)
		{
			return (*a.mNameValueReference.u32 != *b.mNameValueReference.f32);
		}
		else if (b.mType == NVT_S32)
		{
			return ((S32) *a.mNameValueReference.u32 != *b.mNameValueReference.s32);
		}
		else if (b.mType == NVT_U32)
		{
			return (*a.mNameValueReference.u32 != *b.mNameValueReference.u32);
		}
		break;
	case NVT_VEC3:
		if (  (a.mType == b.mType)
			&&(a.mType == NVT_VEC3))
		{
			return (*a.mNameValueReference.vec3 != *b.mNameValueReference.vec3);
		}
		break;
	default:
		llerrs << "Unknown != NV type " << a.mStringType << " with " << b.mStringType << llendl;
		break;
	}
	return FALSE;
}


LLNameValue	&operator-(const LLNameValue &a)
{
	static LLNameValue retval;

	switch(a.mType)
	{
	case NVT_STRING:
		break;
	case NVT_F32:
		retval.mType = a.mType;
		retval.mStringType = NameValueTypeStrings[a.mType];
		delete retval.mNameValueReference.f32;
		retval.mNameValueReference.f32 = new F32(-*a.mNameValueReference.f32);
		break;
	case NVT_S32:
		retval.mType = a.mType;
		retval.mStringType = NameValueTypeStrings[a.mType];
		delete retval.mNameValueReference.s32;
		retval.mNameValueReference.s32 = new S32(-*a.mNameValueReference.s32);
		break;
	case NVT_U32:
		retval.mType = NVT_S32;
		retval.mStringType = NameValueTypeStrings[NVT_S32];
		delete retval.mNameValueReference.s32;
		// Can't do unary minus on U32, doesn't work.
		retval.mNameValueReference.s32 = new S32(-S32(*a.mNameValueReference.u32));
		break;
	case NVT_VEC3:
		retval.mType = a.mType;
		retval.mStringType = NameValueTypeStrings[a.mType];
		delete retval.mNameValueReference.vec3;
		retval.mNameValueReference.vec3 = new LLVector3(-*a.mNameValueReference.vec3);
		break;
	default:
		llerrs << "Unknown - NV type " << a.mStringType << llendl;
		break;
	}
	return retval;
}
