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

#include "apr_pools.h"
#include "apr_dso.h"

#include "lldir.h"
#include "llimagej2c.h"
#include "llmemtype.h"
#include "lltimer.h"
#include "llmath.h"

typedef LLImageJ2CImpl* (*CreateLLImageJ2CFunction)();
typedef void (*DestroyLLImageJ2CFunction)(LLImageJ2CImpl*);
typedef const char* (*EngineInfoLLImageJ2CFunction)();

//some "private static" variables so we only attempt to load
//dynamic libaries once
CreateLLImageJ2CFunction j2cimpl_create_func;
DestroyLLImageJ2CFunction j2cimpl_destroy_func;
EngineInfoLLImageJ2CFunction j2cimpl_engineinfo_func;
apr_pool_t *j2cimpl_dso_memory_pool;
apr_dso_handle_t *j2cimpl_dso_handle;

//Declare the prototype for theses functions here, their functionality
//will be implemented in other files which define a derived LLImageJ2CImpl
//but only ONE static library which has the implementation for this
//function should ever be included
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl();
void fallbackDestroyLLImageJ2CImpl(LLImageJ2CImpl* impl);
const char* fallbackEngineInfoLLImageJ2CImpl();

// Test data gathering handle
LLImageCompressionTester* LLImageJ2C::sTesterp = NULL ;
const std::string sTesterName("ImageCompressionTester");

//static
//Loads the required "create", "destroy" and "engineinfo" functions needed
void LLImageJ2C::openDSO()
{
	//attempt to load a DSO and get some functions from it
	std::string dso_name;
	std::string dso_path;

	bool all_functions_loaded = false;
	apr_status_t rv;

#if LL_WINDOWS
	dso_name = "llkdu.dll";
#elif LL_DARWIN
	dso_name = "libllkdu.dylib";
#else
	dso_name = "libllkdu.so";
#endif

	dso_path = gDirUtilp->findFile(dso_name,
								   gDirUtilp->getAppRODataDir(),
								   gDirUtilp->getExecutableDir());

	j2cimpl_dso_handle      = NULL;
	j2cimpl_dso_memory_pool = NULL;

	//attempt to load the shared library
	apr_pool_create(&j2cimpl_dso_memory_pool, NULL);
	rv = apr_dso_load(&j2cimpl_dso_handle,
					  dso_path.c_str(),
					  j2cimpl_dso_memory_pool);

	//now, check for success
	if ( rv == APR_SUCCESS )
	{
		//found the dynamic library
		//now we want to load the functions we're interested in
		CreateLLImageJ2CFunction  create_func = NULL;
		DestroyLLImageJ2CFunction dest_func = NULL;
		EngineInfoLLImageJ2CFunction engineinfo_func = NULL;

		rv = apr_dso_sym((apr_dso_handle_sym_t*)&create_func,
						 j2cimpl_dso_handle,
						 "createLLImageJ2CKDU");
		if ( rv == APR_SUCCESS )
		{
			//we've loaded the create function ok
			//we need to delete via the DSO too
			//so lets check for a destruction function
			rv = apr_dso_sym((apr_dso_handle_sym_t*)&dest_func,
							 j2cimpl_dso_handle,
							 "destroyLLImageJ2CKDU");
			if ( rv == APR_SUCCESS )
			{
				//we've loaded the destroy function ok
				rv = apr_dso_sym((apr_dso_handle_sym_t*)&engineinfo_func,
						 j2cimpl_dso_handle,
						 "engineInfoLLImageJ2CKDU");
				if ( rv == APR_SUCCESS )
				{
					//ok, everything is loaded alright
					j2cimpl_create_func  = create_func;
					j2cimpl_destroy_func = dest_func;
					j2cimpl_engineinfo_func = engineinfo_func;
					all_functions_loaded = true;
				}
			}
		}
	}

	if ( !all_functions_loaded )
	{
		//something went wrong with the DSO or function loading..
		//fall back onto our satefy impl creation function

#if 0
		// precious verbose debugging, sadly we can't use our
		// 'llinfos' stream etc. this early in the initialisation seq.
		char errbuf[256];
		fprintf(stderr, "failed to load syms from DSO %s (%s)\n",
			dso_name.c_str(), dso_path.c_str());
		apr_strerror(rv, errbuf, sizeof(errbuf));
		fprintf(stderr, "error: %d, %s\n", rv, errbuf);
		apr_dso_error(j2cimpl_dso_handle, errbuf, sizeof(errbuf));
		fprintf(stderr, "dso-error: %d, %s\n", rv, errbuf);
#endif

		if ( j2cimpl_dso_handle )
		{
			apr_dso_unload(j2cimpl_dso_handle);
			j2cimpl_dso_handle = NULL;
		}

		if ( j2cimpl_dso_memory_pool )
		{
			apr_pool_destroy(j2cimpl_dso_memory_pool);
			j2cimpl_dso_memory_pool = NULL;
		}
	}
}

//static
void LLImageJ2C::closeDSO()
{
	if ( j2cimpl_dso_handle ) apr_dso_unload(j2cimpl_dso_handle);
	if (j2cimpl_dso_memory_pool) apr_pool_destroy(j2cimpl_dso_memory_pool);
}

//static
std::string LLImageJ2C::getEngineInfo()
{
	if (!j2cimpl_engineinfo_func)
		j2cimpl_engineinfo_func = fallbackEngineInfoLLImageJ2CImpl;

	return j2cimpl_engineinfo_func();
}

LLImageJ2C::LLImageJ2C() : 	LLImageFormatted(IMG_CODEC_J2C),
							mMaxBytes(0),
							mRawDiscardLevel(-1),
							mRate(0.0f),
							mReversible(FALSE),
							mAreaUsedForDataSizeCalcs(0)
{
	//We assume here that if we wanted to create via
	//a dynamic library that the approriate open calls were made
	//before any calls to this constructor.

	//Therefore, a NULL creation function pointer here means
	//we either did not want to create using functions from the dynamic
	//library or there were issues loading it, either way
	//use our fall back
	if ( !j2cimpl_create_func )
	{
		j2cimpl_create_func = fallbackCreateLLImageJ2CImpl;
	}

	mImpl = j2cimpl_create_func();

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
	//We assume here that if we wanted to destroy via
	//a dynamic library that the approriate open calls were made
	//before any calls to this destructor.

	//Therefore, a NULL creation function pointer here means
	//we either did not want to destroy using functions from the dynamic
	//library or there were issues loading it, either way
	//use our fall back
	if ( !j2cimpl_destroy_func )
	{
		j2cimpl_destroy_func = fallbackDestroyLLImageJ2CImpl;
	}

	if ( mImpl )
	{
		j2cimpl_destroy_func(mImpl);
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


// calcDataSize() returns how many bytes to read 
// to load discard_level (including header and higher discard levels)
S32 LLImageJ2C::calcDataSize(S32 discard_level)
{
	discard_level = llclamp(discard_level, 0, MAX_DISCARD_LEVEL);

	if ( mAreaUsedForDataSizeCalcs != (getHeight() * getWidth()) 
		|| mDataSizes[0] == 0)
	{
		mAreaUsedForDataSizeCalcs = getHeight() * getWidth();
		
		S32 level = MAX_DISCARD_LEVEL;	// Start at the highest discard
		while ( level >= 0 )
		{
			mDataSizes[level] = calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), level, mRate);
			level--;
		}

		/* This is technically a more correct way to calculate the size required
		   for each discard level, since they should include the size needed for
		   lower levels.   Unfortunately, this doesn't work well and will lead to 
		   download stalls.  The true correct way is to parse the header.  This will
		   all go away with http textures at some point.

		// Calculate the size for each discard level.   Lower levels (higher quality)
		// contain the cumulative size of higher levels		
		S32 total_size = calcHeaderSizeJ2C();

		S32 level = MAX_DISCARD_LEVEL;	// Start at the highest discard
		while ( level >= 0 )
		{	// Add in this discard level and all before it
			total_size += calcDataSizeJ2C(getWidth(), getHeight(), getComponents(), level, mRate);
			mDataSizes[level] = total_size;
			level--;
		}
		*/
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
		U8 *data = new U8[file_size];
		apr_size_t bytes_read = file_size;
		apr_status_t s = apr_file_read(apr_file, data, &bytes_read); // modifies bytes_read	
		infile.close() ;

		if (s != APR_SUCCESS || (S32)bytes_read != file_size)
		{
			delete[] data;
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
	mRunBytesInCompression = 0;

	mTotalBytesInDecompression = 0;
	mTotalBytesOutDecompression = 0;
	mTotalBytesInCompression = 0;
	mTotalBytesOutCompression = 0;

	mTotalTimeDecompression = 0.0f;
	mTotalTimeCompression = 0.0f;
}

LLImageCompressionTester::~LLImageCompressionTester()
{
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

	F32 totalkBInDecompression  = (F32)(mTotalBytesInDecompression)  / 1000.0;
	F32 totalkBOutDecompression = (F32)(mTotalBytesOutDecompression) / 1000.0;
	F32 totalkBInCompression    = (F32)(mTotalBytesInCompression)    / 1000.0;
	F32 totalkBOutCompression   = (F32)(mTotalBytesOutCompression)   / 1000.0;
	
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
	if (mRunBytesInDecompression > (1000000))
	{
		// Output everything
		outputTestResults();
		// Reset the decompression data of the run
		mRunBytesInDecompression = 0;
	}
}

//----------------------------------------------------------------------------------------------
// End of LLTexturePipelineTester
//----------------------------------------------------------------------------------------------

