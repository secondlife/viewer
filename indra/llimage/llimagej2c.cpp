/** 
 * @file llimagej2c.cpp
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "linden_common.h"

#include <apr-1/apr_pools.h>
#include <apr-1/apr_dso.h>

#include "lldir.h"
#include "llimagej2c.h"
#include "llmemory.h"

typedef LLImageJ2CImpl* (*CreateLLImageJ2CFunction)();
typedef void (*DestroyLLImageJ2CFunction)(LLImageJ2CImpl*);

//some "private static" variables so we only attempt to load
//dynamic libaries once
CreateLLImageJ2CFunction j2cimpl_create_func;
DestroyLLImageJ2CFunction j2cimpl_destroy_func;
apr_pool_t *j2cimpl_dso_memory_pool;
apr_dso_handle_t *j2cimpl_dso_handle;

//Declare the prototype for theses functions here, their functionality
//will be implemented in other files which define a derived LLImageJ2CImpl
//but only ONE static library which has the implementation for this
//function should ever be included
LLImageJ2CImpl* fallbackCreateLLImageJ2CImpl();
void fallbackDestroyLLImageJ2CImpl(LLImageJ2CImpl* impl);

//static
//Loads the required "create" and "destroy" functions needed
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
				//k, everything is loaded alright
				j2cimpl_create_func  = create_func;
				j2cimpl_destroy_func = dest_func;
				all_functions_loaded = true;
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

LLImageJ2C::LLImageJ2C() : 	LLImageFormatted(IMG_CODEC_J2C),
							mMaxBytes(0),
							mRawDiscardLevel(-1),
							mRate(0.0f)
{
	//We assume here that if we wanted to destory via
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
}

// virtual
LLImageJ2C::~LLImageJ2C()
{
	//We assume here that if we wanted to destory via
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

	return mImpl->decodeImpl(*this, *raw_imagep, decode_time, first_channel, max_channel_count);
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
