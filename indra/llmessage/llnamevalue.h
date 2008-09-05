/** 
 * @file llnamevalue.h
 * @brief class for defining name value pairs.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLNAMEVALUE_H
#define LL_LLNAMEVALUE_H

// As of January 2008, I believe we only use the following name-value
// pairs.  This is hard to prove because they are initialized from
// strings.  JC
//
// FirstName STRING
// LastName STRING
// AttachPt U32
// AttachmentItemId STRING
// Title STRING
// AttachmentOffset VEC3
// AttachmentOrientation VEC3
// SitObject STRING
// SitPosition VEC3

#include "string_table.h"
#include "llmath.h"
#include "v3math.h"
#include "lldbstrings.h"

class LLNameValue;
class LLStringTable;

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
	void init(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto );

	LLNameValue();
	LLNameValue(const char *data);
	LLNameValue(const char *name, const char *type, const char *nvclass );
	LLNameValue(const char *name, const char *data, const char *type, const char *nvclass );
	LLNameValue(const char *name, const char *data, const char *type, const char *nvclass, const char *nvsendto );

	~LLNameValue();

	char			*getString();
	const char		*getAsset() const;
	F32				*getF32();
	S32				*getS32();
	void			getVec3(LLVector3 &vec);
	LLVector3		*getVec3();
	U32				*getU32();
	U64				*getU64();

	const char		*getType() const		{ return mStringType; }
	const char		*getClass() const		{ return mStringClass; }
	const char		*getSendto() const		{ return mStringSendto; }

	BOOL			sendToData() const;
	BOOL			sendToViewer() const;

	void			callCallback();
	std::string		printNameValue() const;
	std::string		printData() const;
	
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

	friend std::ostream&		operator<<(std::ostream& s, const LLNameValue &a);

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
};

extern LLStringTable	gNVNameTable;


#endif
