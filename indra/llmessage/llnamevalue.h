/** 
 * @file llnamevalue.h
 * @brief class for defining name value pairs.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLNAMEVALUE_H
#define LL_LLNAMEVALUE_H

#include "string_table.h"
#include "llskipmap.h"
#include "llmath.h"
#include "v3math.h"
#include "lldbstrings.h"

class LLNameValue;
typedef void (*TNameValueCallback)(LLNameValue *changed, void **user_data);

void add_use_callback(char *name, TNameValueCallback ucb, void **user_data);

typedef enum e_name_value_types
{
	NVT_NULL,
	NVT_STRING,
	NVT_F32,
	NVT_S32,
	NVT_VEC3,
	NVT_U32,
	NVT_CAMERA, // Deprecated, but leaving in case removing this will cause problems
	NVT_ASSET,
	NVT_U64,
	NVT_EOF
} ENameValueType;

typedef enum e_name_value_class
{
	NVC_NULL,
	NVC_READ_ONLY,
	NVC_READ_WRITE,
	NVC_CALLBACK,
	NVC_EOF
} ENameValueClass;

typedef enum e_name_value_sento
{
	NVS_NULL,
	NVS_SIM,
	NVS_DATA_SIM,
	NVS_SIM_VIEWER,
	NVS_DATA_SIM_VIEWER,
	NVS_EOF
} ENameValueSendto;


// NameValues can always be "printed" into a buffer of this length.
const U32 NAME_VALUE_BUF_SIZE = 1024;


const U32 NAME_VALUE_TYPE_STRING_LENGTH = 8;
const U32 NAME_VALUE_CLASS_STRING_LENGTH = 16;
const U32 NAME_VALUE_SENDTO_STRING_LENGTH = 18;
const U32 NAME_VALUE_DATA_SIZE = 
			NAME_VALUE_BUF_SIZE - 
			( DB_NV_NAME_BUF_SIZE +
			NAME_VALUE_TYPE_STRING_LENGTH +
			NAME_VALUE_CLASS_STRING_LENGTH + 
			NAME_VALUE_SENDTO_STRING_LENGTH );


extern char NameValueTypeStrings[NVT_EOF][NAME_VALUE_TYPE_STRING_LENGTH];			/* Flawfinder: Ignore */
extern char NameValueClassStrings[NVC_EOF][NAME_VALUE_CLASS_STRING_LENGTH];		/* Flawfinder: Ignore */
extern char NameValueSendtoStrings[NVS_EOF][NAME_VALUE_SENDTO_STRING_LENGTH];		/* Flawfinder: Ignore */

typedef union u_name_value_reference
{
	char		*string;
	F32			*f32;
	S32			*s32;
	LLVector3	*vec3;
	U32			*u32;
	U64			*u64;
} UNameValueReference;


class LLNameValue
{
public:
	void baseInit();
	void init(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto, 
				TNameValueCallback nvcb = NULL, void **user_data = NULL);

	LLNameValue();
	LLNameValue(const char *data);
	LLNameValue(const char *name, const char *type, const char *nvclass, 
				TNameValueCallback nvcb = NULL, void **user_data = NULL);
	LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, 
				TNameValueCallback nvcb = NULL, void **user_data = NULL);
	LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto, 
				TNameValueCallback nvcb = NULL, void **user_data = NULL);

	~LLNameValue();

	char			*getString();
	const char		*getAsset() const;
	F32				*getF32();
	S32				*getS32();
	void			getVec3(LLVector3 &vec);
	LLVector3		*getVec3();
	F32				magnitude();
	U32				*getU32();
	U64				*getU64();

	const char		*getType() const		{ return mStringType; }
	const char		*getClass() const		{ return mStringClass; }
	const char		*getSendto() const		{ return mStringSendto; }

	BOOL			sendToData() const;
	BOOL			sendToViewer() const;

	void			callCallback();
	std::string		printNameValue();
	std::string		printData();
	
	ENameValueType		getTypeEnum() const		{ return mType; }
	ENameValueClass		getClassEnum() const	{ return mClass; }
	ENameValueSendto	getSendtoEnum() const	{ return mSendto; }

	LLNameValue		&operator=(const LLNameValue &a);
	void			setString(const char *a);
	void			setAsset(const char *a);
	void			setF32(const F32 a);
	void			setS32(const S32 a);
	void			setVec3(const LLVector3 &a);
	void			setU32(const U32 a);

	BOOL			nonzero();

	friend std::ostream&		operator<<(std::ostream& s, const LLNameValue &a);

	friend LLNameValue	&operator+(const LLNameValue &a, const LLNameValue &b);
	friend LLNameValue	&operator-(const LLNameValue &a, const LLNameValue &b);
	friend LLNameValue	&operator*(const LLNameValue &a, const LLNameValue &b);
	friend LLNameValue	&operator/(const LLNameValue &a, const LLNameValue &b);
	friend LLNameValue	&operator%(const LLNameValue &a, const LLNameValue &b);
	friend LLNameValue	&operator*(const LLNameValue &a, F32 k);
	friend LLNameValue	&operator*(F32 k, const LLNameValue &a);

	friend bool			operator==(const LLNameValue &a, const LLNameValue &b);	
	friend bool			operator<=(const LLNameValue &a, const LLNameValue &b);	
	friend bool			operator>=(const LLNameValue &a, const LLNameValue &b);	
	friend bool			operator<(const LLNameValue &a, const LLNameValue &b);	
	friend bool			operator>(const LLNameValue &a, const LLNameValue &b);	
	friend bool			operator!=(const LLNameValue &a, const LLNameValue &b);	

	friend LLNameValue	&operator-(const LLNameValue &a);	

private:
	void			printNameValue(std::ostream& s);
	
public:
	char						*mName;

	char						*mStringType;
	ENameValueType				mType;
	char						*mStringClass;
	ENameValueClass				mClass;
	char						*mStringSendto;
	ENameValueSendto			mSendto;

	UNameValueReference			mNameValueReference;
	LLStringTable				*mNVNameTable;
	TNameValueCallback			mNameValueCB;
	void						**mUserData;
};

extern LLStringTable	gNVNameTable;


#endif
