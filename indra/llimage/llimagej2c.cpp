/** 
 * @file llimagej2c.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"

#include "lldir.h"
#include "llimagej2c.h"
#include "llmemtype.h"
#include "lltimer.h"
#include "llmath.h"
#include "llmemory.h"

typedef LLImageJ2CImpl* (*CreateLLImageJ2CFunction)();
typedef void (*DestroyLLImageJ2CFunction)(LLImageJ2CImpl*);
typedef const char* (*EngineInfoLLImageJ2CFunction)();

// Declare the prototype for theses functions here. Their functionality
// will be implemented in other files which define a derived LLImageJ2CImpl
// but only ONE static library which has the implementation for these
// functions should ever be included.
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl();
void fallbackDestroyLLImageJ2CImpl(LLImageJ2CImpl* impl);
const char* fallbackEngineInfoLLImageJ2CImpl();

// Test data gathering handle
LLImageCompressionTester* LLImageJ2C::sTesterp = NULL ;
const std::string sTesterName("ImageCompressionTester");

//static
std::string LLImageJ2C::getEngineInfo()
{
    return fallbackEngineInfoLLImageJ2CImpl();
}

LLImageJ2C::LLImageJ2C() : 	LLImageFormatted(IMG_CODEC_J2C),
							mMaxBytes(0),
							mRawDiscardLevel(-1),
							mRate(DEFAULT_COMPRESSION_RATE),
							mReversible(FALSE),
							mAreaUsedForDataSizeCalcs(0)
{
	mImpl = fallbackCreateLLImageJ2CImpl();

	// Clear data size table
	for( S32 i = 0; i <= MAX_DISCARD_LEVEL; i++)
	{	// Array size is MAX_DISCARD_LEVEL+1
		mDataSizes[i] = 0;
	}

	// If that test log has ben requested but not yet created, create it
	if (LLMetricPerformanceTesterBasic::isMetricLogRequested(sTesterName) && !LLMetricPerformanceTesterBasic::getTester(sTesterName))
	{
		sTesterp = new LLImageCompressionTester() ;
		if (!sTesterp->isValid())
		{
			delete sTesterp;
			sTesterp = NULL;
		}
	}
}

// virtual
LLImageJ2C::~LLImageJ2C()
{
	if ( mImpl )
	{
        fallbackDestroyLLImageJ2CImpl(mImpl);
	}
}

// virtual
void LLImageJ2C::resetLastError()
{
	mLastError.clear();
}

//virtual
void LLImageJ2C::setLastError(const std::string& message, const std::string& filename)
{
	mLastError = message;
	if (!filename.empty())
		mLastError += std::string(" FILE: ") + filename;
}

// virtual
S8  LLImageJ2C::getRawDiscardLevel()
{
	return mRawDiscardLevel;
}

BOOL LLImageJ2C::updateData()
{
	BOOL res = TRUE;
	resetLastError();

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		res = FALSE;
	}
	else 
	{
		res = mImpl->getMetadata(*this);
	}

	if (res)
	{
		// SJB: override discard based on mMaxBytes elsewhere
		S32 max_bytes = getDataSize(); // mMaxBytes ? mMaxBytes : getDataSize();
		S32 discard = calcDiscardLevelBytes(max_bytes);
		setDiscardLevel(discard);
	}

	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}

BOOL LLImageJ2C::initDecode(LLImageRaw &raw_image, int discard_level, int* region)
{
	setDiscardLevel(discard_level != -1 ? discard_level : 0);
	return mImpl->initDecode(*this,raw_image,discard_level,region);
}

BOOL LLImageJ2C::initEncode(LLImageRaw &raw_image, int blocks_size, int precincts_size, int levels)
{
	return mImpl->initEncode(*this,raw_image,blocks_size,precincts_size,levels);
}

BOOL LLImageJ2C::decode(LLImageRaw *raw_imagep, F32 decode_time)
{
	return decodeChannels(raw_imagep, decode_time, 0, 4);
}


// Returns TRUE to mean done, whether successful or not.
BOOL LLImageJ2C::decodeChannels(LLImageRaw *raw_imagep, F32 decode_time, S32 first_channel, S32 max_channel_count )
{
	LLTimer elapsed;
	LLMemType mt1(mMemType);

	BOOL res = TRUE;
	
	resetLastError();

	// Check to make sure that this instance has been initialized with data
	if (!getData() || (getDataSize() < 16))
	{
		setLastError("LLImageJ2C uninitialized");
		res = TRUE; // done
	}
	else
	{
		// Update the raw discard level
		updateRawDiscardLevel();
		mDecoding = TRUE;
		res = mImpl->decodeImpl(*this, *raw_imagep, decode_time, first_channel, max_channel_count);
	}
	
	if (res)
	{
		if (!mDecoding)
		{
			// Failed
			raw_imagep->deleteData();
		}
		else
		{
			mDecoding = FALSE;
		}
	}

	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	
	LLImageCompressionTester* tester = (LLImageCompressionTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		// Decompression stat gathering
		// Note that we *do not* take into account the decompression failures data so we might overestimate the time spent processing

		// Always add the decompression time to the stat
		tester->updateDecompressionStats(elapsed.getElapsedTimeF32()) ;
		if (res)
		{
			// The whole data stream is finally decompressed when res is returned as TRUE
			tester->updateDecompressionStats(this->getDataSize(), raw_imagep->getDataSize()) ;
		}
	}

	return res;
}


BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, F32 encode_time)
{
	return encode(raw_imagep, NULL, encode_time);
}


BOOL LLImageJ2C::encode(const LLImageRaw *raw_imagep, const char* comment_text, F32 encode_time)
{
	LLTimer elapsed;
	LLMemType mt1(mMemType);
	resetLastError();
	BOOL res = mImpl->encodeImpl(*this, *raw_imagep, comment_text, encode_time, mReversible);
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}

	LLImageCompressionTester* tester = (LLImageCompressionTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
	if (tester)
	{
		// Compression stat gathering
		// Note that we *do not* take into account the compression failures cases so we night overestimate the time spent processing

		// Always add the compression time to the stat
		tester->updateCompressionStats(elapsed.getElapsedTimeF32()) ;
		if (res)
		{
			// The whole data stream is finally compressed when res is returned as TRUE
			tester->updateCompressionStats(this->getDataSize(), raw_imagep->getDataSize()) ;
		}
	}

	return res;
}

//static
S32 LLImageJ2C::calcHeaderSizeJ2C()
{
	return FIRST_PACKET_SIZE; // Hack. just needs to be >= actual header size...
}

//static
S32 LLImageJ2C::calcDataSizeJ2C(S32 w, S32 h, S32 comp, S32 discard_level, F32 rate)
{
	// Note: This provides an estimation for the first to last quality layer of a given discard level
	// This is however an efficient approximation, as the true discard level boundary would be
	// in general too big for fast fetching.
	// For details about the equation used here, see https://wiki.lindenlab.com/wiki/THX1138_KDU_Improvements#Byte_Range_Study

	// Estimate the number of layers. This is consistent with what's done for j2c encoding in LLImageJ2CKDU::encodeImpl().
	S32 nb_layers = 1;
	S32 surface = w*h;
	S32 s = 64*64;
	while (surface > s)
	{
		nb_layers++;
		s *= 4;
	}
	F32 layer_factor =  3.0f * (7 - llclamp(nb_layers,1,6));
	
	// Compute w/pow(2,discard_level) and h/pow(2,discard_level)
	w >>= discard_level;
	h >>= discard_level;
	w = llmax(w, 1);
	h = llmax(h, 1);

	// Temporary: compute both new and old range and pick one according to the settings TextureNewByteRange 
	// *TODO: Take the old code out once we have enough tests done
	S32 bytes;
	S32 new_bytes = (S32) (sqrt((F32)(w*h))*(F32)(comp)*rate*1000.f/layer_factor);
	S32 old_bytes = (S32)((F32)(w*h*comp)*rate);
	bytes = (LLImage::useNewByteRange() && (new_bytes < old_bytes) ? new_bytes : old_bytes);
	bytes = llmax(bytes, calcHeaderSizeJ2C());
	return bytes;
}

S32 LLImageJ2C::calcHeaderSize()
{
	return calcHeaderSizeJ2C();
}

// calcDataSize() returns how many bytes to read to load discard_level (including header)
S32 LLImageJ2C::calcDataSize(S32 discard_level)
{
	discard_level = llclamp(discard_level, 0, MAX_DISCARD_LEVEL);
	if ( mAreaUsedForDataSizeCalcs != (getHeight() * getWidth()) 
		|| (mDataSizes[0] == 0))
	{
		mAreaUsedForDataSizeCalcs = getHeight() * getWidth();
		
		S32 level = MAX_DISCARD_LEVEL;	// Start at the highest discard
		while ( level >= 0 )
		{
			mDataSizes[level] = calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), level, mRate);
			level--;
		}
	}
	return mDataSizes[discard_level];
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
		S32 bytes_needed = calcDataSize(discard_level);
		// Use TextureReverseByteRange percent (see settings.xml) of the optimal size to qualify as correct rendering for the given discard level
		if (bytes >= (bytes_needed*LLImage::getReverseByteRangePercent()/100))
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

void LLImageJ2C::setMaxBytes(S32 max_bytes)
{
	mMaxBytes = max_bytes;
}

void LLImageJ2C::setReversible(const BOOL reversible)
{
 	mReversible = reversible;
}


BOOL LLImageJ2C::loadAndValidate(const std::string &filename)
{
	BOOL res = TRUE;
	
	resetLastError();

	S32 file_size = 0;
	LLAPRFile infile ;
	infile.open(filename, LL_APR_RB, NULL, &file_size);
	apr_file_t* apr_file = infile.getFileHandle() ;
	if (!apr_file)
	{
		setLastError("Unable to open file for reading", filename);
		res = FALSE;
	}
	else if (file_size == 0)
	{
		setLastError("File is empty",filename);
		res = FALSE;
	}
	else
	{
		U8 *data = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), file_size);
		apr_size_t bytes_read = file_size;
		apr_status_t s = apr_file_read(apr_file, data, &bytes_read); // modifies bytes_read	
		infile.close() ;

		if (s != APR_SUCCESS || (S32)bytes_read != file_size)
		{
			FREE_MEM(LLImageBase::getPrivatePool(), data);
			setLastError("Unable to read entire file");
			res = FALSE;
		}
		else
		{
			res = validate(data, file_size);
		}
	}
	
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	
	return res;
}


BOOL LLImageJ2C::validate(U8 *data, U32 file_size)
{
	LLMemType mt1(mMemType);

	resetLastError();
	
	setData(data, file_size);

	BOOL res = updateData();
	if ( res )
	{
		// Check to make sure that this instance has been initialized with data
		if (!getData() || (0 == getDataSize()))
		{
			setLastError("LLImageJ2C uninitialized");
			res = FALSE;
		}
		else
		{
			res = mImpl->getMetadata(*this);
		}
	}
	
	if (!mLastError.empty())
	{
		LLImage::setLastError(mLastError);
	}
	return res;
}

void LLImageJ2C::decodeFailed()
{
	mDecoding = FALSE;
}

void LLImageJ2C::updateRawDiscardLevel()
{
	mRawDiscardLevel = mMaxBytes ? calcDiscardLevelBytes(mMaxBytes) : mDiscardLevel;
}

LLImageJ2CImpl::~LLImageJ2CImpl()
{
}

//----------------------------------------------------------------------------------------------
// Start of LLImageCompressionTester
//----------------------------------------------------------------------------------------------
LLImageCompressionTester::LLImageCompressionTester() : LLMetricPerformanceTesterBasic(sTesterName) 
{
	addMetric("Time Decompression (s)");
	addMetric("Volume In Decompression (kB)");
	addMetric("Volume Out Decompression (kB)");
	addMetric("Decompression Ratio (x:1)");
	addMetric("Perf Decompression (kB/s)");

	addMetric("Time Compression (s)");
	addMetric("Volume In Compression (kB)");
	addMetric("Volume Out Compression (kB)");
	addMetric("Compression Ratio (x:1)");
	addMetric("Perf Compression (kB/s)");

	mRunBytesInDecompression = 0;
	mRunBytesOutDecompression = 0;
	mRunBytesInCompression = 0;

	mTotalBytesInDecompression = 0;
	mTotalBytesOutDecompression = 0;
	mTotalBytesInCompression = 0;
	mTotalBytesOutCompression = 0;

	mTotalTimeDecompression = 0.0f;
	mTotalTimeCompression = 0.0f;
	mRunTimeDecompression = 0.0f;
}

LLImageCompressionTester::~LLImageCompressionTester()
{
	outputTestResults();
	LLImageJ2C::sTesterp = NULL;
}

//virtual 
void LLImageCompressionTester::outputTestRecord(LLSD *sd) 
{	
	std::string currentLabel = getCurrentLabelName();

	F32 decompressionPerf = 0.0f;
	F32 compressionPerf   = 0.0f;
	F32 decompressionRate = 0.0f;
	F32 compressionRate   = 0.0f;

	F32 totalkBInDecompression  = (F32)(mTotalBytesInDecompression)  / 1000.f;
	F32 totalkBOutDecompression = (F32)(mTotalBytesOutDecompression) / 1000.f;
	F32 totalkBInCompression    = (F32)(mTotalBytesInCompression)    / 1000.f;
	F32 totalkBOutCompression   = (F32)(mTotalBytesOutCompression)   / 1000.f;
	
	if (!is_approx_zero(mTotalTimeDecompression))
	{
		decompressionPerf = totalkBInDecompression / mTotalTimeDecompression;
	}
	if (!is_approx_zero(totalkBInDecompression))
	{
		decompressionRate = totalkBOutDecompression / totalkBInDecompression;
	}
	if (!is_approx_zero(mTotalTimeCompression))
	{
		compressionPerf = totalkBInCompression / mTotalTimeCompression;
	}
	if (!is_approx_zero(totalkBOutCompression))
	{
		compressionRate = totalkBInCompression / totalkBOutCompression;
	}

	(*sd)[currentLabel]["Time Decompression (s)"]		= (LLSD::Real)mTotalTimeDecompression;
	(*sd)[currentLabel]["Volume In Decompression (kB)"]	= (LLSD::Real)totalkBInDecompression;
	(*sd)[currentLabel]["Volume Out Decompression (kB)"]= (LLSD::Real)totalkBOutDecompression;
	(*sd)[currentLabel]["Decompression Ratio (x:1)"]	= (LLSD::Real)decompressionRate;
	(*sd)[currentLabel]["Perf Decompression (kB/s)"]	= (LLSD::Real)decompressionPerf;

	(*sd)[currentLabel]["Time Compression (s)"]			= (LLSD::Real)mTotalTimeCompression;
	(*sd)[currentLabel]["Volume In Compression (kB)"]	= (LLSD::Real)totalkBInCompression;
	(*sd)[currentLabel]["Volume Out Compression (kB)"]	= (LLSD::Real)totalkBOutCompression;
	(*sd)[currentLabel]["Compression Ratio (x:1)"]		= (LLSD::Real)compressionRate;
	(*sd)[currentLabel]["Perf Compression (kB/s)"]		= (LLSD::Real)compressionPerf;
}

void LLImageCompressionTester::updateCompressionStats(const F32 deltaTime) 
{
	mTotalTimeCompression += deltaTime;
}

void LLImageCompressionTester::updateCompressionStats(const S32 bytesCompress, const S32 bytesRaw) 
{
	mTotalBytesInCompression += bytesRaw;
	mRunBytesInCompression += bytesRaw;
	mTotalBytesOutCompression += bytesCompress;
	if (mRunBytesInCompression > (1000000))
	{
		// Output everything
		outputTestResults();
		// Reset the compression data of the run
		mRunBytesInCompression = 0;
	}
}

void LLImageCompressionTester::updateDecompressionStats(const F32 deltaTime) 
{
	mTotalTimeDecompression += deltaTime;
}

void LLImageCompressionTester::updateDecompressionStats(const S32 bytesIn, const S32 bytesOut) 
{
	mTotalBytesInDecompression += bytesIn;
	mRunBytesInDecompression += bytesIn;
	mTotalBytesOutDecompression += bytesOut;
	mRunBytesOutDecompression += bytesOut;
	//if (mRunBytesInDecompression > (1000000))
	if (mRunBytesOutDecompression > (10000000))
	//if ((mTotalTimeDecompression - mRunTimeDecompression) >= (5.0f))
	{
		// Output everything
		outputTestResults();
		// Reset the decompression data of the run
		mRunBytesInDecompression = 0;
		mRunBytesOutDecompression = 0;
		mRunTimeDecompression = mTotalTimeDecompression;
	}
}

//----------------------------------------------------------------------------------------------
// End of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------

