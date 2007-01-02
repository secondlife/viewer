/** 
 * @file llimagej2c.cpp
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#ifndef LL_USE_KDU
#define LL_USE_KDU 1
#endif // LL_USE_KDU

#include "llimagej2c.h"
#include "llmemory.h"
#if LL_USE_KDU
#include "llimagej2ckdu.h"
#endif

#include "llimagej2coj.h"


LLImageJ2C::LLImageJ2C() : 	LLImageFormatted(IMG_CODEC_J2C),
							mMaxBytes(0),
							mRawDiscardLevel(-1),
							mRate(0.0f)
{
#if LL_USE_KDU
	mImpl = new LLImageJ2CKDU();
#else
	mImpl = new LLImageJ2COJ();
#endif
}

// virtual
LLImageJ2C::~LLImageJ2C()
{
	delete mImpl;
}

// virtual
S8  LLImageJ2C::getRawDiscardLevel()
{
	return mRawDiscardLevel;
}

BOOL LLImageJ2C::updateData()
{	
	resetLastError();

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		return FALSE;
	}

	if (!mImpl->getMetadata(*this))
	{
		return FALSE;
	}
	// SJB: override discard based on mMaxBytes elsewhere
	S32 max_bytes = getDataSize(); // mMaxBytes ? mMaxBytes : getDataSize();
	S32 discard = calcDiscardLevelBytes(max_bytes);
	setDiscardLevel(discard);

	return TRUE;
}


BOOL LLImageJ2C::decode(LLImageRaw *raw_imagep, F32 decode_time)
{
	return decode(raw_imagep, decode_time, 0, 4);
}


BOOL LLImageJ2C::decode(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count )
{
	LLMemType mt1((LLMemType::EMemType)mMemType);

	resetLastError();

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		return FALSE;
	}

	// Update the raw discard level
	updateRawDiscardLevel();

	return mImpl->decodeImpl(*this, *raw_imagep, decode_time, 0, 4);
}


BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, F32 encode_time)
{
	return encode(raw_imagep, NULL, encode_time);
}


BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time)
{
	LLMemType mt1((LLMemType::EMemType)mMemType);
	return mImpl->encodeImpl(*this, *raw_imagep, comment_text, encode_time);
}

//static
S32 LLImageJ2C::calcHeaderSizeJ2C()
{
	return 600; //2048; // ??? hack... just needs to be >= actual header size...
}

//static
S32 LLImageJ2C::calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate)
{
	if (rate <= 0.f) rate = .125f;
	while (discard_level > 0)
	{
		if (w < 1 || h < 1)
			break;
		w >>= 1;
		h >>= 1;
		discard_level--;
	}
	S32 bytes = (S32)((F32)(w*h*comp)*rate);
	bytes = llmax(bytes, calcHeaderSizeJ2C());
	return bytes;
}

S32 LLImageJ2C::calcHeaderSize()
{
	return calcHeaderSizeJ2C();
}

S32 LLImageJ2C::calcDataSize(S32 discard_level)
{
	return calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), discard_level, mRate);
}

S32 LLImageJ2C::calcDiscardLevelBytes(S32 bytes)
{
	llassert(bytes >= 0);
	S32 discard_level = 0;
	if (bytes == 0)
	{
		return MAX_DISCARD_LEVEL;
	}
	while (1)
	{
		S32 bytes_needed = calcDataSize(discard_level); // virtual
		if (bytes >= bytes_needed - (bytes_needed>>2)) // For J2c, up the res at 75% of the optimal number of bytes
		{
			break;
		}
		discard_level++;
		if (discard_level >= MAX_DISCARD_LEVEL)
		{
			break;
		}
	}
	return discard_level;
}

void LLImageJ2C::setRate(F32 rate)
{
	mRate = rate;
}

void LLImageJ2C::setMaxBytes(S32 max_bytes)
{
	mMaxBytes = max_bytes;
}
// NOT USED
// void LLImageJ2C::setReversible(const BOOL reversible)
// {
// 	mReversible = reversible;
// }


BOOL LLImageJ2C::loadAndValidate(const LLString &filename)
{
	resetLastError();

	S32 file_size = 0;
	apr_file_t* apr_file = ll_apr_file_open(filename, LL_APR_RB, &file_size);
	if (!apr_file)
	{
		setLastError("Unable to open file for reading", filename);
		return FALSE;
	}
	if (file_size == 0)
	{
		setLastError("File is empty",filename);
		apr_file_close(apr_file);
		return FALSE;
	}
	
	U8 *data = new U8[file_size];
	apr_size_t bytes_read = file_size;
	apr_status_t s = apr_file_read(apr_file, data, &bytes_read); // modifies bytes_read	
	if (s != APR_SUCCESS || bytes_read != file_size)
	{
		delete[] data;
		setLastError("Unable to read entire file");
		return FALSE;
	}
	apr_file_close(apr_file);

	return validate(data, file_size);
}


BOOL LLImageJ2C::validate(U8 *data, U32 file_size)
{
	LLMemType mt1((LLMemType::EMemType)mMemType);
	// Taken from setData()

	BOOL res = LLImageFormatted::setData(data, file_size);
	if ( !res )
	{
		return FALSE;
	}

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (0 == getDataSize()))
	{
		setLastError("LLImageJ2C uninitialized");
		return FALSE;
	}

	return mImpl->getMetadata(*this);
}

void LLImageJ2C::setDecodingDone(BOOL complete)
{
	mDecoding = FALSE;
	mDecoded = complete;
}

void LLImageJ2C::updateRawDiscardLevel()
{
	mRawDiscardLevel = mMaxBytes ? calcDiscardLevelBytes(mMaxBytes) : mDiscardLevel;
}

LLImageJ2CImpl::~LLImageJ2CImpl()
{
}
