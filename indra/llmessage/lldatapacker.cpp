/** 
 * @file lldatapacker.cpp
 * @brief Data packer implementation.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "lldatapacker.h"
#include "llerror.h"

#include "message.h"

#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "lluuid.h"

// *NOTE: there are functions below which use sscanf and rely on this
// particular value of DP_BUFSIZE. Search for '511' (DP_BUFSIZE - 1)
// to find them if you change this number.
const S32 DP_BUFSIZE = 512;

static char DUMMY_BUFFER[128]; /*Flawfinder: ignore*/

LLDataPacker::LLDataPacker() : mPassFlags(0), mWriteEnabled(FALSE)
{
}

BOOL LLDataPacker::packFixed(const F32 value, const char *name,
							 const BOOL is_signed, const U32 int_bits, const U32 frac_bits)
{
	BOOL success = TRUE;
	S32 unsigned_bits = int_bits + frac_bits;
	S32 total_bits = unsigned_bits;

	if (is_signed)
	{
		total_bits++;
	}

	S32 min_val;
	U32 max_val;
	if (is_signed)
	{
		min_val = 1 << int_bits;
		min_val *= -1;
	}
	else
	{
		min_val = 0;
	}
	max_val = 1 << int_bits;

	// Clamp to be within range
	F32 fixed_val = llclamp(value, (F32)min_val, (F32)max_val);
	if (is_signed)
	{
		fixed_val += max_val;
	}
	fixed_val *= 1 << frac_bits;

	if (total_bits <= 8)
	{
		packU8((U8)fixed_val, name);
	}
	else if (total_bits <= 16)
	{
		packU16((U16)fixed_val, name);
	}
	else if (total_bits <= 31)
	{
		packU32((U32)fixed_val, name);
	}
	else
	{
		llerrs << "Using fixed-point packing of " << total_bits << " bits, why?!" << llendl;
	}
	return success;
}

BOOL LLDataPacker::unpackFixed(F32 &value, const char *name,
							   const BOOL is_signed, const U32 int_bits, const U32 frac_bits)
{
	//BOOL success = TRUE;
	//llinfos << "unpackFixed:" << name << " int:" << int_bits << " frac:" << frac_bits << llendl;
	BOOL ok = FALSE;
	S32 unsigned_bits = int_bits + frac_bits;
	S32 total_bits = unsigned_bits;

	if (is_signed)
	{
		total_bits++;
	}

	S32 min_val;
	U32 max_val;
	if (is_signed)
	{
		min_val = 1 << int_bits;
		min_val *= -1;
	}
	max_val = 1 << int_bits;

	F32 fixed_val;
	if (total_bits <= 8)
	{
		U8 fixed_8;
		ok = unpackU8(fixed_8, name);
		fixed_val = (F32)fixed_8;
	}
	else if (total_bits <= 16)
	{
		U16 fixed_16;
		ok = unpackU16(fixed_16, name);
		fixed_val = (F32)fixed_16;
	}
	else if (total_bits <= 31)
	{
		U32 fixed_32;
		ok = unpackU32(fixed_32, name);
		fixed_val = (F32)fixed_32;
	}
	else
	{
		fixed_val = 0;
		llerrs << "Bad bit count: " << total_bits << llendl;
	}

	//llinfos << "Fixed_val:" << fixed_val << llendl;

	fixed_val /= (F32)(1 << frac_bits);
	if (is_signed)
	{
		fixed_val -= max_val;
	}
	value = fixed_val;
	//llinfos << "Value: " << value << llendl;
	return ok;
}

//---------------------------------------------------------------------------
// LLDataPackerBinaryBuffer implementation
//---------------------------------------------------------------------------

BOOL LLDataPackerBinaryBuffer::packString(const std::string& value, const char *name)
{
	BOOL success = TRUE;
	S32 length = value.length()+1;

	success &= verifyLength(length, name);

	if (mWriteEnabled) 
	{
		htonmemcpy(mCurBufferp, value.c_str(), MVT_VARIABLE, length);  
	}
	mCurBufferp += length;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackString(std::string& value, const char *name)
{
	BOOL success = TRUE;
	S32 length = (S32)strlen((char *)mCurBufferp) + 1; /*Flawfinder: ignore*/

	success &= verifyLength(length, name);

	value = std::string((char*)mCurBufferp); // We already assume NULL termination calling strlen()
	
	mCurBufferp += length;
	return success;
}

BOOL LLDataPackerBinaryBuffer::packBinaryData(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(size + 4, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &size, MVT_S32, 4);  
	}
	mCurBufferp += 4;
	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value, MVT_VARIABLE, size);  
	}
	mCurBufferp += size;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(4, name);
	htonmemcpy(&size, mCurBufferp, MVT_S32, 4);
	mCurBufferp += 4;
	success &= verifyLength(size, name);
	if (success)
	{
		htonmemcpy(value, mCurBufferp, MVT_VARIABLE, size);
		mCurBufferp += size;
	}
	else
	{
		llwarns << "LLDataPackerBinaryBuffer::unpackBinaryData would unpack invalid data, aborting!" << llendl;
		success = FALSE;
	}
	return success;
}


BOOL LLDataPackerBinaryBuffer::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(size, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value, MVT_VARIABLE, size);  
	}
	mCurBufferp += size;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(size, name);
	htonmemcpy(value, mCurBufferp, MVT_VARIABLE, size);
	mCurBufferp += size;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packU8(const U8 value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U8), name);

	if (mWriteEnabled) 
	{
		*mCurBufferp = value;
	}
	mCurBufferp++;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackU8(U8 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U8), name);

	value = *mCurBufferp;
	mCurBufferp++;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packU16(const U16 value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U16), name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &value, MVT_U16, 2);  
	}
	mCurBufferp += 2;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackU16(U16 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U16), name);

	htonmemcpy(&value, mCurBufferp, MVT_U16, 2);
	mCurBufferp += 2;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packU32(const U32 value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U32), name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &value, MVT_U32, 4);  
	}
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackU32(U32 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(U32), name);

	htonmemcpy(&value, mCurBufferp, MVT_U32, 4);
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packS32(const S32 value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(S32), name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &value, MVT_S32, 4); 
	}
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackS32(S32 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(S32), name);

	htonmemcpy(&value, mCurBufferp, MVT_S32, 4);
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packF32(const F32 value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(F32), name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &value, MVT_F32, 4); 
	}
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackF32(F32 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(sizeof(F32), name);

	htonmemcpy(&value, mCurBufferp, MVT_F32, 4);
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packColor4(const LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value.mV, MVT_LLVector4, 16); 
	}
	mCurBufferp += 16;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackColor4(LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	htonmemcpy(value.mV, mCurBufferp, MVT_LLVector4, 16);
	mCurBufferp += 16;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packColor4U(const LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(4, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value.mV, MVT_VARIABLE, 4);  
	}
	mCurBufferp += 4;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackColor4U(LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(4, name);

	htonmemcpy(value.mV, mCurBufferp, MVT_VARIABLE, 4);
	mCurBufferp += 4;
	return success;
}



BOOL LLDataPackerBinaryBuffer::packVector2(const LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(8, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, &value.mV[0], MVT_F32, 4);  
		htonmemcpy(mCurBufferp+4, &value.mV[1], MVT_F32, 4);  
	}
	mCurBufferp += 8;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackVector2(LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(8, name);

	htonmemcpy(&value.mV[0], mCurBufferp, MVT_F32, 4);
	htonmemcpy(&value.mV[1], mCurBufferp+4, MVT_F32, 4);
	mCurBufferp += 8;
	return success;
}


BOOL LLDataPackerBinaryBuffer::packVector3(const LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(12, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value.mV, MVT_LLVector3, 12);  
	}
	mCurBufferp += 12;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackVector3(LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(12, name);

	htonmemcpy(value.mV, mCurBufferp, MVT_LLVector3, 12);
	mCurBufferp += 12;
	return success;
}

BOOL LLDataPackerBinaryBuffer::packVector4(const LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value.mV, MVT_LLVector4, 16);  
	}
	mCurBufferp += 16;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackVector4(LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	htonmemcpy(value.mV, mCurBufferp, MVT_LLVector4, 16);
	mCurBufferp += 16;
	return success;
}

BOOL LLDataPackerBinaryBuffer::packUUID(const LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	if (mWriteEnabled) 
	{ 
		htonmemcpy(mCurBufferp, value.mData, MVT_LLUUID, 16);  
	}
	mCurBufferp += 16;
	return success;
}


BOOL LLDataPackerBinaryBuffer::unpackUUID(LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	success &= verifyLength(16, name);

	htonmemcpy(value.mData, mCurBufferp, MVT_LLUUID, 16);
	mCurBufferp += 16;
	return success;
}

const LLDataPackerBinaryBuffer&	LLDataPackerBinaryBuffer::operator=(const LLDataPackerBinaryBuffer &a)
{
	if (a.getBufferSize() > getBufferSize())
	{
		// We've got problems, ack!
		llerrs << "Trying to do an assignment with not enough room in the target." << llendl;
	}
	memcpy(mBufferp, a.mBufferp, a.getBufferSize());	/*Flawfinder: ignore*/
	return *this;
}

void LLDataPackerBinaryBuffer::dumpBufferToLog()
{
	llwarns << "Binary Buffer Dump, size: " << mBufferSize << llendl;
	char line_buffer[256]; /*Flawfinder: ignore*/
	S32 i;
	S32 cur_line_pos = 0;

	S32 cur_line = 0;
	for (i = 0; i < mBufferSize; i++)
	{
		snprintf(line_buffer + cur_line_pos*3, sizeof(line_buffer) - cur_line_pos*3, "%02x ", mBufferp[i]); 	/* Flawfinder: ignore */
		cur_line_pos++;
		if (cur_line_pos >= 16)
		{
			cur_line_pos = 0;
			llwarns << "Offset:" << std::hex << cur_line*16 << std::dec << " Data:" << line_buffer << llendl;
			cur_line++;
		}
	}
	if (cur_line_pos)
	{
		llwarns << "Offset:" << std::hex << cur_line*16 << std::dec << " Data:" << line_buffer << llendl;
	}
}

//---------------------------------------------------------------------------
// LLDataPackerAsciiBuffer implementation
//---------------------------------------------------------------------------
BOOL LLDataPackerAsciiBuffer::packString(const std::string& value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled) 
	{
		numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\n", value.c_str());		/* Flawfinder: ignore */
	}
	else
	{
		numCopied = value.length() + 1; /*Flawfinder: ignore*/
	}

	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		// *NOTE: I believe we need to mark a failure bit at this point.
	    numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packString: string truncated: " << value << llendl;
	}
	mCurBufferp += numCopied;
	return success;
}

BOOL LLDataPackerAsciiBuffer::unpackString(std::string& value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
	BOOL res = getValueStr(name, valuestr, DP_BUFSIZE); // NULL terminated
	if (!res) // 
	{
		return FALSE;
	}
	value = valuestr;
	return success;
}


BOOL LLDataPackerAsciiBuffer::packBinaryData(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	
	int numCopied = 0;
	if (mWriteEnabled)
	{
		numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%010d ", size);	/* Flawfinder: ignore */

		// snprintf returns number of bytes that would have been
		// written had the output not being truncated. In that case,
		// it will retuen >= passed in size value.  so a check needs
		// to be added to detect truncation, and if there is any, only
		// account for the actual number of bytes written..and not
		// what could have been written.
		if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
		{
			numCopied = getBufferSize()-getCurrentSize();
			llwarns << "LLDataPackerAsciiBuffer::packBinaryData: number truncated: " << size << llendl;
		}
		mCurBufferp += numCopied;


		S32 i;
		BOOL bBufferFull = FALSE;
		for (i = 0; i < size && !bBufferFull; i++)
		{
			numCopied = snprintf(mCurBufferp, getBufferSize()-getCurrentSize(), "%02x ", value[i]);	/* Flawfinder: ignore */
			if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
			{
				numCopied = getBufferSize()-getCurrentSize();
				llwarns << "LLDataPackerAsciiBuffer::packBinaryData: data truncated: " << llendl;
				bBufferFull = TRUE;
			}
			mCurBufferp += numCopied;
		}

		if (!bBufferFull)
		{
			numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(), "\n");	/* Flawfinder: ignore */
			if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
		    	{
				numCopied = getBufferSize()-getCurrentSize();
				llwarns << "LLDataPackerAsciiBuffer::packBinaryData: newline truncated: " << llendl;
		    	}
		    	mCurBufferp += numCopied;
		}
	}
	else
	{
		// why +10 ?? XXXCHECK
		numCopied = 10 + 1; // size plus newline
		numCopied += size;
		if (numCopied > getBufferSize()-getCurrentSize())
		{
			numCopied = getBufferSize()-getCurrentSize();
		}   
		mCurBufferp += numCopied;
	}
	
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char *cur_pos = &valuestr[0];
	sscanf(valuestr,"%010d", &size);
	cur_pos += 11;

	S32 i;
	for (i = 0; i < size; i++)
	{
		S32 val;
		sscanf(cur_pos,"%02x", &val);
		value[i] = val;
		cur_pos += 3;
	}
	return success;
}


BOOL LLDataPackerAsciiBuffer::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	
	if (mWriteEnabled)
	{
		S32 i;
		int numCopied = 0;
		BOOL bBufferFull = FALSE;
		for (i = 0; i < size && !bBufferFull; i++)
		{
			numCopied = snprintf(mCurBufferp, getBufferSize()-getCurrentSize(), "%02x ", value[i]);	/* Flawfinder: ignore */
			if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
			{
			    numCopied = getBufferSize()-getCurrentSize();
				llwarns << "LLDataPackerAsciiBuffer::packBinaryDataFixed: data truncated: " << llendl;
			    bBufferFull = TRUE;
			}
			mCurBufferp += numCopied;

		}
		if (!bBufferFull)
		{
			numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(), "\n");	/* Flawfinder: ignore */
			if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
			{
				numCopied = getBufferSize()-getCurrentSize();
				llwarns << "LLDataPackerAsciiBuffer::packBinaryDataFixed: newline truncated: " << llendl;
			}
			
			mCurBufferp += numCopied;
		}
	}
	else
	{
		int numCopied = 2 * size + 1; //hex bytes plus newline 
		if (numCopied > getBufferSize()-getCurrentSize())
		{
			numCopied = getBufferSize()-getCurrentSize();
		}   
		mCurBufferp += numCopied;
	}
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */		
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char *cur_pos = &valuestr[0];

	S32 i;
	for (i = 0; i < size; i++)
	{
		S32 val;
		sscanf(cur_pos,"%02x", &val);
		value[i] = val;
		cur_pos += 3;
	}
	return success;
}



BOOL LLDataPackerAsciiBuffer::packU8(const U8 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);	/* Flawfinder: ignore */
	}
	else
	{
		// just do the write to a temp buffer to get the length
		numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);	/* Flawfinder: ignore */
	}

	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packU8: val truncated: " << llendl;
	}

	mCurBufferp += numCopied;

	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackU8(U8 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 in_val;
	sscanf(valuestr,"%d", &in_val);
	value = in_val;
	return success;
}

BOOL LLDataPackerAsciiBuffer::packU16(const U16 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);	/* Flawfinder: ignore */
	}

	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packU16: val truncated: " << llendl;
	}

	mCurBufferp += numCopied;

	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackU16(U16 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 in_val;
	sscanf(valuestr,"%d", &in_val);
	value = in_val;
	return success;
}


BOOL LLDataPackerAsciiBuffer::packU32(const U32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%u\n", value);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%u\n", value);	/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packU32: val truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackU32(U32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%u", &value);
	return success;
}


BOOL LLDataPackerAsciiBuffer::packS32(const S32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d\n", value);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%d\n", value);		/* Flawfinder: ignore */	
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packS32: val truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackS32(S32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%d", &value);
	return success;
}


BOOL LLDataPackerAsciiBuffer::packF32(const F32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f\n", value);		/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER, sizeof(DUMMY_BUFFER), "%f\n", value);		/* Flawfinder: ignore */	
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packF32: val truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackF32(F32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];		/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f", &value);
	return success;
}


BOOL LLDataPackerAsciiBuffer::packColor4(const LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packColor4: truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackColor4(LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
	return success;
}

BOOL LLDataPackerAsciiBuffer::packColor4U(const LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
		numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packColor4U: truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackColor4U(LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	 /* Flawfinder: ignore */ 
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 r, g, b, a;

	sscanf(valuestr,"%d %d %d %d", &r, &g, &b, &a);
	value.mV[0] = r;
	value.mV[1] = g;
	value.mV[2] = b;
	value.mV[3] = a;
	return success;
}


BOOL LLDataPackerAsciiBuffer::packVector2(const LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f\n", value.mV[0], value.mV[1]);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f\n", value.mV[0], value.mV[1]);		/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
		numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packVector2: truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackVector2(LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	 /* Flawfinder: ignore */ 
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f", &value.mV[0], &value.mV[1]);
	return success;
}


BOOL LLDataPackerAsciiBuffer::packVector3(const LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]);	/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
	    numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packVector3: truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackVector3(LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	/* Flawfinder: ignore */ 
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f", &value.mV[0], &value.mV[1], &value.mV[2]);
	return success;
}

BOOL LLDataPackerAsciiBuffer::packVector4(const LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	int numCopied = 0;
	if (mWriteEnabled)
	{
	    	numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = snprintf(DUMMY_BUFFER,sizeof(DUMMY_BUFFER),"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	/* Flawfinder: ignore */
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
	    numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packVector4: truncated: " << llendl;
	}

	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackVector4(LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	/* Flawfinder: ignore */ 
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
	return success;
}


BOOL LLDataPackerAsciiBuffer::packUUID(const LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);

	int numCopied = 0;
	if (mWriteEnabled)
	{
		std::string tmp_str;
		value.toString(tmp_str);
		numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\n", tmp_str.c_str());	/* Flawfinder: ignore */
	}
	else
	{
		numCopied = 64 + 1; // UUID + newline
	}
	// snprintf returns number of bytes that would have been written
	// had the output not being truncated. In that case, it will
	// return either -1 or value >= passed in size value . So a check needs to be added
	// to detect truncation, and if there is any, only account for the
	// actual number of bytes written..and not what could have been
	// written.
	if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
	{
	    numCopied = getBufferSize()-getCurrentSize();
		llwarns << "LLDataPackerAsciiBuffer::packUUID: truncated: " << llendl;
		success = FALSE;
	}
	mCurBufferp += numCopied;
	return success;
}


BOOL LLDataPackerAsciiBuffer::unpackUUID(LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char tmp_str[64];	/* Flawfinder: ignore */
	sscanf(valuestr, "%63s", tmp_str);	/* Flawfinder: ignore */
	value.set(tmp_str);

	return success;
}

void LLDataPackerAsciiBuffer::dump()
{
	llinfos << "Buffer: " << mBufferp << llendl;
}

void LLDataPackerAsciiBuffer::writeIndentedName(const char *name)
{
	if (mIncludeNames)
	{
		int numCopied = 0;
		if (mWriteEnabled)
		{
			numCopied = snprintf(mCurBufferp,getBufferSize()-getCurrentSize(),"%s\t", name);	/* Flawfinder: ignore */
		}
		else
		{
			numCopied = (S32)strlen(name) + 1; 	/* Flawfinder: ignore */ //name + tab  	
		}

		// snprintf returns number of bytes that would have been written
		// had the output not being truncated. In that case, it will
		// return either -1 or value >= passed in size value . So a check needs to be added
		// to detect truncation, and if there is any, only account for the
		// actual number of bytes written..and not what could have been
		// written.
		if (numCopied < 0 || numCopied > getBufferSize()-getCurrentSize())
		{
			numCopied = getBufferSize()-getCurrentSize();
			llwarns << "LLDataPackerAsciiBuffer::writeIndentedName: truncated: " << llendl;
		}

		mCurBufferp += numCopied;
	}
}

BOOL LLDataPackerAsciiBuffer::getValueStr(const char *name, char *out_value, S32 value_len)
{
	BOOL success = TRUE;
	char buffer[DP_BUFSIZE];	/* Flawfinder: ignore */
	char keyword[DP_BUFSIZE];	/* Flawfinder: ignore */
	char value[DP_BUFSIZE];	/* Flawfinder: ignore */

	buffer[0] = '\0';
	keyword[0] = '\0';
	value[0] = '\0';

	if (mIncludeNames)
	{
		// Read both the name and the value, and validate the name.
		sscanf(mCurBufferp, "%511[^\n]", buffer);
		// Skip the \n
		mCurBufferp += (S32)strlen(buffer) + 1;	/* Flawfinder: ignore */

		sscanf(buffer, "%511s %511[^\n]", keyword, value);	/* Flawfinder: ignore */

		if (strcmp(keyword, name))
		{
			llwarns << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << llendl;
			return FALSE;
		}
	}
	else
	{
		// Just the value exists
		sscanf(mCurBufferp, "%511[^\n]", value);
		// Skip the \n
		mCurBufferp += (S32)strlen(value) + 1;	/* Flawfinder: ignore */
	}

	S32 in_value_len = (S32)strlen(value)+1;	/* Flawfinder: ignore */
	S32 min_len = llmin(in_value_len, value_len);
	memcpy(out_value, value, min_len);	/* Flawfinder: ignore */
	out_value[min_len-1] = 0;

	return success;
}

// helper function used by LLDataPackerAsciiFile
// to convert F32 into a string. This is to avoid
// << operator writing F32 value into a stream 
// since it does not seem to preserve the float value
std::string convertF32ToString(F32 val)
{
	std::string str;
	char  buf[20];
	snprintf(buf, 20, "%f", val);
	str = buf;
	return str;
}

//---------------------------------------------------------------------------
// LLDataPackerAsciiFile implementation
//---------------------------------------------------------------------------
BOOL LLDataPackerAsciiFile::packString(const std::string& value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%s\n", value.c_str());	
	}
	else if (mOutputStream)
	{
		*mOutputStream << value << "\n";
	}
	return success;
}

BOOL LLDataPackerAsciiFile::unpackString(std::string& value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE];	/* Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}
	value = valuestr;
	return success;
}


BOOL LLDataPackerAsciiFile::packBinaryData(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	
	if (mFP)
	{
		fprintf(mFP, "%010d ", size);

		S32 i;
		for (i = 0; i < size; i++)
		{
			fprintf(mFP, "%02x ", value[i]);
		}
		fprintf(mFP, "\n");
	}
	else if (mOutputStream)
	{
		char buffer[32];	/* Flawfinder: ignore */
		snprintf(buffer,sizeof(buffer), "%010d ", size);	/* Flawfinder: ignore */
		*mOutputStream << buffer;

		S32 i;
		for (i = 0; i < size; i++)
		{
			snprintf(buffer, sizeof(buffer), "%02x ", value[i]);	/* Flawfinder: ignore */
			*mOutputStream << buffer;
		}
		*mOutputStream << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackBinaryData(U8 *value, S32 &size, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char *cur_pos = &valuestr[0];
	sscanf(valuestr,"%010d", &size);
	cur_pos += 11;

	S32 i;
	for (i = 0; i < size; i++)
	{
		S32 val;
		sscanf(cur_pos,"%02x", &val);
		value[i] = val;
		cur_pos += 3;
	}
	return success;
}


BOOL LLDataPackerAsciiFile::packBinaryDataFixed(const U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	
	if (mFP)
	{
		S32 i;
		for (i = 0; i < size; i++)
		{
			fprintf(mFP, "%02x ", value[i]);
		}
		fprintf(mFP, "\n");
	}
	else if (mOutputStream)
	{
		char buffer[32]; /*Flawfinder: ignore*/
		S32 i;
		for (i = 0; i < size; i++)
		{
			snprintf(buffer, sizeof(buffer), "%02x ", value[i]);	/* Flawfinder: ignore */
			*mOutputStream << buffer;
		}
		*mOutputStream << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackBinaryDataFixed(U8 *value, S32 size, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore*/
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char *cur_pos = &valuestr[0];

	S32 i;
	for (i = 0; i < size; i++)
	{
		S32 val;
		sscanf(cur_pos,"%02x", &val);
		value[i] = val;
		cur_pos += 3;
	}
	return success;
}



BOOL LLDataPackerAsciiFile::packU8(const U8 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%d\n", value);	
	}
	else if (mOutputStream)
	{
		// We have to cast this to an integer because streams serialize
		// bytes as bytes - not as text.
		*mOutputStream << (S32)value << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackU8(U8 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 in_val;
	sscanf(valuestr,"%d", &in_val);
	value = in_val;
	return success;
}

BOOL LLDataPackerAsciiFile::packU16(const U16 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%d\n", value);	
	}
	else if (mOutputStream)
	{
		*mOutputStream <<"" << value << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackU16(U16 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 in_val;
	sscanf(valuestr,"%d", &in_val);
	value = in_val;
	return success;
}


BOOL LLDataPackerAsciiFile::packU32(const U32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%u\n", value);	
	}
	else if (mOutputStream)
	{
		*mOutputStream <<"" << value << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackU32(U32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%u", &value);
	return success;
}


BOOL LLDataPackerAsciiFile::packS32(const S32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%d\n", value);	
	}
	else if (mOutputStream)
	{
		*mOutputStream <<"" << value << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackS32(S32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%d", &value);
	return success;
}


BOOL LLDataPackerAsciiFile::packF32(const F32 value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%f\n", value);	
	}
	else if (mOutputStream)
	{
		*mOutputStream <<"" << convertF32ToString(value) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackF32(F32 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f", &value);
	return success;
}


BOOL LLDataPackerAsciiFile::packColor4(const LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	
	}
	else if (mOutputStream)
	{
		*mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << " " << convertF32ToString(value.mV[3]) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackColor4(LLColor4 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
	return success;
}

BOOL LLDataPackerAsciiFile::packColor4U(const LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%d %d %d %d\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	
	}
	else if (mOutputStream)
	{
		*mOutputStream << (S32)(value.mV[0]) << " " << (S32)(value.mV[1]) << " " << (S32)(value.mV[2]) << " " << (S32)(value.mV[3]) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackColor4U(LLColor4U &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	S32 r, g, b, a;

	sscanf(valuestr,"%d %d %d %d", &r, &g, &b, &a);
	value.mV[0] = r;
	value.mV[1] = g;
	value.mV[2] = b;
	value.mV[3] = a;
	return success;
}


BOOL LLDataPackerAsciiFile::packVector2(const LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%f %f\n", value.mV[0], value.mV[1]);	
	}
	else if (mOutputStream)
	{
		*mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackVector2(LLVector2 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f", &value.mV[0], &value.mV[1]);
	return success;
}


BOOL LLDataPackerAsciiFile::packVector3(const LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%f %f %f\n", value.mV[0], value.mV[1], value.mV[2]);	
	}
	else if (mOutputStream)
	{
		*mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackVector3(LLVector3 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f", &value.mV[0], &value.mV[1], &value.mV[2]);
	return success;
}

BOOL LLDataPackerAsciiFile::packVector4(const LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	if (mFP)
	{
		fprintf(mFP,"%f %f %f %f\n", value.mV[0], value.mV[1], value.mV[2], value.mV[3]);	
	}
	else if (mOutputStream)
	{
		*mOutputStream << convertF32ToString(value.mV[0]) << " " << convertF32ToString(value.mV[1]) << " " << convertF32ToString(value.mV[2]) << " " << convertF32ToString(value.mV[3]) << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackVector4(LLVector4 &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	sscanf(valuestr,"%f %f %f %f", &value.mV[0], &value.mV[1], &value.mV[2], &value.mV[3]);
	return success;
}


BOOL LLDataPackerAsciiFile::packUUID(const LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	writeIndentedName(name);
	std::string tmp_str;
	value.toString(tmp_str);
	if (mFP)
	{
		fprintf(mFP,"%s\n", tmp_str.c_str());
	}
	else if (mOutputStream)
	{
		*mOutputStream <<"" << tmp_str << "\n";
	}
	return success;
}


BOOL LLDataPackerAsciiFile::unpackUUID(LLUUID &value, const char *name)
{
	BOOL success = TRUE;
	char valuestr[DP_BUFSIZE]; /*Flawfinder: ignore */
	if (!getValueStr(name, valuestr, DP_BUFSIZE))
	{
		return FALSE;
	}

	char tmp_str[64]; /*Flawfinder: ignore */
	sscanf(valuestr,"%63s",tmp_str);	/* Flawfinder: ignore */
	value.set(tmp_str);

	return success;
}


void LLDataPackerAsciiFile::writeIndentedName(const char *name)
{
	std::string indent_buf;
	indent_buf.reserve(mIndent+1);

	S32 i;
	for(i = 0; i < mIndent; i++)
	{
		indent_buf[i] = '\t';
	}
	indent_buf[i] = 0;
	if (mFP)
	{
		fprintf(mFP,"%s%s\t",indent_buf.c_str(), name);		
	}
	else if (mOutputStream)
	{
		*mOutputStream << indent_buf << name << "\t";
	}
}

BOOL LLDataPackerAsciiFile::getValueStr(const char *name, char *out_value, S32 value_len)
{
	BOOL success = FALSE;
	char buffer[DP_BUFSIZE]; /*Flawfinder: ignore*/
	char keyword[DP_BUFSIZE]; /*Flawfinder: ignore*/
	char value[DP_BUFSIZE]; /*Flawfinder: ignore*/

	buffer[0] = '\0';
	keyword[0] = '\0';
	value[0] = '\0';

	if (mFP)
	{
		fpos_t last_pos;
		fgetpos(mFP, &last_pos);
		if (fgets(buffer, DP_BUFSIZE, mFP) == NULL)
		{
			buffer[0] = '\0';
		}
	
		sscanf(buffer, "%511s %511[^\n]", keyword, value);	/* Flawfinder: ignore */
	
		if (!keyword[0])
		{
			llwarns << "Data packer could not get the keyword!" << llendl;
			fsetpos(mFP, &last_pos);
			return FALSE;
		}
		if (strcmp(keyword, name))
		{
			llwarns << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << llendl;
			fsetpos(mFP, &last_pos);
			return FALSE;
		}

		S32 in_value_len = (S32)strlen(value)+1; /*Flawfinder: ignore*/
		S32 min_len = llmin(in_value_len, value_len);
		memcpy(out_value, value, min_len); /*Flawfinder: ignore*/
		out_value[min_len-1] = 0;
		success = TRUE;
	}
	else if (mInputStream)
	{
		mInputStream->getline(buffer, DP_BUFSIZE);
	
		sscanf(buffer, "%511s %511[^\n]", keyword, value);	/* Flawfinder: ignore */
		if (!keyword[0])
		{
			llwarns << "Data packer could not get the keyword!" << llendl;
			return FALSE;
		}
		if (strcmp(keyword, name))
		{
			llwarns << "Data packer expecting keyword of type " << name << ", got " << keyword << " instead!" << llendl;
			return FALSE;
		}

		S32 in_value_len = (S32)strlen(value)+1; /*Flawfinder: ignore*/
		S32 min_len = llmin(in_value_len, value_len);
		memcpy(out_value, value, min_len); /*Flawfinder: ignore*/
		out_value[min_len-1] = 0;
		success = TRUE;
	}

	return success;
}
