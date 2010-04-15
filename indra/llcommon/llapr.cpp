/** 
 * @file llapr.cpp
 * @author Phoenix
 * @date 2004-11-28
 * @brief Helper functions for using the apache portable runtime library.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llapr.h"

apr_pool_t *gAPRPoolp = NULL; // Global APR memory pool
LLVolatileAPRPool *LLAPRFile::sAPRFilePoolp = NULL ; //global volatile APR memory pool.
apr_thread_mutex_t *gLogMutexp = NULL;
apr_thread_mutex_t *gCallStacksLogMutexp = NULL;

const S32 FULL_VOLATILE_APR_POOL = 1024 ; //number of references to LLVolatileAPRPool

void ll_init_apr()
{
	if (!gAPRPoolp)
	{
		// Initialize APR and create the global pool
		apr_initialize();
		apr_pool_create(&gAPRPoolp, NULL);
		
		// Initialize the logging mutex
		apr_thread_mutex_create(&gLogMutexp, APR_THREAD_MUTEX_UNNESTED, gAPRPoolp);
		apr_thread_mutex_create(&gCallStacksLogMutexp, APR_THREAD_MUTEX_UNNESTED, gAPRPoolp);
	}

	if(!LLAPRFile::sAPRFilePoolp)
	{
		LLAPRFile::sAPRFilePoolp = new LLVolatileAPRPool(FALSE) ;
	}
}


void ll_cleanup_apr()
{
	LL_INFOS("APR") << "Cleaning up APR" << LL_ENDL;

	if (gLogMutexp)
	{
		// Clean up the logging mutex

		// All other threads NEED to be done before we clean up APR, so this is okay.
		apr_thread_mutex_destroy(gLogMutexp);
		gLogMutexp = NULL;
	}
	if (gCallStacksLogMutexp)
	{
		// Clean up the logging mutex

		// All other threads NEED to be done before we clean up APR, so this is okay.
		apr_thread_mutex_destroy(gCallStacksLogMutexp);
		gCallStacksLogMutexp = NULL;
	}
	if (gAPRPoolp)
	{
		apr_pool_destroy(gAPRPoolp);
		gAPRPoolp = NULL;
	}
	if (LLAPRFile::sAPRFilePoolp)
	{
		delete LLAPRFile::sAPRFilePoolp ;
		LLAPRFile::sAPRFilePoolp = NULL ;
	}
	apr_terminate();
}

//
//
//LLAPRPool
//
LLAPRPool::LLAPRPool(apr_pool_t *parent, apr_size_t size, BOOL releasePoolFlag) 	
	: mParent(parent),
	mReleasePoolFlag(releasePoolFlag),
	mMaxSize(size),
	mPool(NULL)
{	
	createAPRPool() ;
}

LLAPRPool::~LLAPRPool() 
{
	releaseAPRPool() ;
}

void LLAPRPool::createAPRPool()
{
	if(mPool)
	{
		return ;
	}

	mStatus = apr_pool_create(&mPool, mParent);
	ll_apr_warn_status(mStatus) ;

	if(mMaxSize > 0) //size is the number of blocks (which is usually 4K), NOT bytes.
	{
		apr_allocator_t *allocator = apr_pool_allocator_get(mPool); 
		if (allocator) 
		{ 
			apr_allocator_max_free_set(allocator, mMaxSize) ;
		}
	}
}

void LLAPRPool::releaseAPRPool()
{
	if(!mPool)
	{
		return ;
	}

	if(!mParent || mReleasePoolFlag)
	{
		apr_pool_destroy(mPool) ;
		mPool = NULL ;
	}
}

//virtual
apr_pool_t* LLAPRPool::getAPRPool() 
{	
	return mPool ; 
}

LLVolatileAPRPool::LLVolatileAPRPool(BOOL is_local, apr_pool_t *parent, apr_size_t size, BOOL releasePoolFlag) 
				  : LLAPRPool(parent, size, releasePoolFlag),
				  mNumActiveRef(0),
				  mNumTotalRef(0),
				  mMutexPool(NULL),
				  mMutexp(NULL)
{
	//create mutex
	if(!is_local) //not a local apr_pool, that is: shared by multiple threads.
	{
		apr_pool_create(&mMutexPool, NULL); // Create a pool for mutex
		apr_thread_mutex_create(&mMutexp, APR_THREAD_MUTEX_UNNESTED, mMutexPool);
	}
}

LLVolatileAPRPool::~LLVolatileAPRPool()
{
	//delete mutex
	if(mMutexp)
	{
		apr_thread_mutex_destroy(mMutexp);
		apr_pool_destroy(mMutexPool);
	}
}

//
//define this virtual function to avoid any mistakenly calling LLAPRPool::getAPRPool().
//
//virtual 
apr_pool_t* LLVolatileAPRPool::getAPRPool() 
{
	return LLVolatileAPRPool::getVolatileAPRPool() ;
}

apr_pool_t* LLVolatileAPRPool::getVolatileAPRPool() 
{	
	LLScopedLock lock(mMutexp) ;

	mNumTotalRef++ ;
	mNumActiveRef++ ;

	if(!mPool)
	{
		createAPRPool() ;
	}
	
	return mPool ;
}

void LLVolatileAPRPool::clearVolatileAPRPool() 
{
	LLScopedLock lock(mMutexp) ;

	if(mNumActiveRef > 0)
	{
		mNumActiveRef--;
		if(mNumActiveRef < 1)
		{
			if(isFull()) 
			{
				mNumTotalRef = 0 ;

				//destroy the apr_pool.
				releaseAPRPool() ;
			}
			else 
			{
				//This does not actually free the memory, 
				//it just allows the pool to re-use this memory for the next allocation. 
				apr_pool_clear(mPool) ;
			}
		}
	}
	else
	{
		llassert_always(mNumActiveRef > 0) ;
	}

	//paranoia check if the pool is jammed.
	//will remove the check before going to release.
	llassert_always(mNumTotalRef < (FULL_VOLATILE_APR_POOL << 2)) ;
}

BOOL LLVolatileAPRPool::isFull()
{
	return mNumTotalRef > FULL_VOLATILE_APR_POOL ;
}
//---------------------------------------------------------------------
//
// LLScopedLock
//
LLScopedLock::LLScopedLock(apr_thread_mutex_t* mutex) : mMutex(mutex)
{
	if(mutex)
	{
		if(ll_apr_warn_status(apr_thread_mutex_lock(mMutex)))
		{
			mLocked = false;
		}
		else
		{
			mLocked = true;
		}
	}
	else
	{
		mLocked = false;
	}
}

LLScopedLock::~LLScopedLock()
{
	unlock();
}

void LLScopedLock::unlock()
{
	if(mLocked)
	{
		if(!ll_apr_warn_status(apr_thread_mutex_unlock(mMutex)))
		{
			mLocked = false;
		}
	}
}

//---------------------------------------------------------------------

bool ll_apr_warn_status(apr_status_t status)
{
	if(APR_SUCCESS == status) return false;
	char buf[MAX_STRING];	/* Flawfinder: ignore */
	apr_strerror(status, buf, MAX_STRING);
	LL_WARNS("APR") << "APR: " << buf << LL_ENDL;
	return true;
}

void ll_apr_assert_status(apr_status_t status)
{
	llassert(ll_apr_warn_status(status) == false);
}

//---------------------------------------------------------------------
//
// LLAPRFile functions
//
LLAPRFile::LLAPRFile()
	: mFile(NULL),
	  mCurrentFilePoolp(NULL)
{
}

LLAPRFile::LLAPRFile(const std::string& filename, apr_int32_t flags, LLVolatileAPRPool* pool)
	: mFile(NULL),
	  mCurrentFilePoolp(NULL)
{
	open(filename, flags, pool);
}

LLAPRFile::~LLAPRFile()
{
	close() ;
}

apr_status_t LLAPRFile::close() 
{
	apr_status_t ret = APR_SUCCESS ;
	if(mFile)
	{
		ret = apr_file_close(mFile);
		mFile = NULL ;
	}

	if(mCurrentFilePoolp)
	{
		mCurrentFilePoolp->clearVolatileAPRPool() ;
		mCurrentFilePoolp = NULL ;
	}

	return ret ;
}

apr_status_t LLAPRFile::open(const std::string& filename, apr_int32_t flags, LLVolatileAPRPool* pool, S32* sizep)
{
	apr_status_t s ;

	//check if already open some file
	llassert_always(!mFile) ;
	llassert_always(!mCurrentFilePoolp) ;
	
	apr_pool_t* apr_pool = pool ? pool->getVolatileAPRPool() : NULL ;
	s = apr_file_open(&mFile, filename.c_str(), flags, APR_OS_DEFAULT, getAPRFilePool(apr_pool));

	if (s != APR_SUCCESS || !mFile)
	{
		mFile = NULL ;
		
		if (sizep)
		{
			*sizep = 0;
		}
	}
	else if (sizep)
	{
		S32 file_size = 0;
		apr_off_t offset = 0;
		if (apr_file_seek(mFile, APR_END, &offset) == APR_SUCCESS)
		{
			llassert_always(offset <= 0x7fffffff);
			file_size = (S32)offset;
			offset = 0;
			apr_file_seek(mFile, APR_SET, &offset);
		}
		*sizep = file_size;
	}

	if(!mCurrentFilePoolp)
	{
		mCurrentFilePoolp = pool ;

		if(!mFile)
		{
			close() ;
		}
	}

	return s ;
}

//use gAPRPoolp.
apr_status_t LLAPRFile::open(const std::string& filename, apr_int32_t flags, BOOL use_global_pool)
{
	apr_status_t s;

	//check if already open some file
	llassert_always(!mFile) ;
	llassert_always(!mCurrentFilePoolp) ;
	llassert_always(use_global_pool) ; //be aware of using gAPRPoolp.
	
	s = apr_file_open(&mFile, filename.c_str(), flags, APR_OS_DEFAULT, gAPRPoolp);
	if (s != APR_SUCCESS || !mFile)
	{
		mFile = NULL ;
		close() ;
		return s;
	}

	return s;
}

apr_pool_t* LLAPRFile::getAPRFilePool(apr_pool_t* pool)
{	
	if(!pool)
	{
		mCurrentFilePoolp = sAPRFilePoolp ;
		return mCurrentFilePoolp->getVolatileAPRPool() ;
	}

	return pool ;
}

// File I/O
S32 LLAPRFile::read(void *buf, S32 nbytes)
{
	llassert_always(mFile) ;
	
	apr_size_t sz = nbytes;
	apr_status_t s = apr_file_read(mFile, buf, &sz);
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		return 0;
	}
	else
	{
		llassert_always(sz <= 0x7fffffff);
		return (S32)sz;
	}
}

S32 LLAPRFile::write(const void *buf, S32 nbytes)
{
	llassert_always(mFile) ;
	
	apr_size_t sz = nbytes;
	apr_status_t s = apr_file_write(mFile, buf, &sz);
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		return 0;
	}
	else
	{
		llassert_always(sz <= 0x7fffffff);
		return (S32)sz;
	}
}

S32 LLAPRFile::seek(apr_seek_where_t where, S32 offset)
{
	return LLAPRFile::seek(mFile, where, offset) ;
}

//
//*******************************************************************************************************************************
//static components of LLAPRFile
//

//static
apr_status_t LLAPRFile::close(apr_file_t* file_handle, LLVolatileAPRPool* pool) 
{
	apr_status_t ret = APR_SUCCESS ;
	if(file_handle)
	{
		ret = apr_file_close(file_handle);
		file_handle = NULL ;
	}

	if(pool)
	{
		pool->clearVolatileAPRPool() ;
	}

	return ret ;
}

//static
apr_file_t* LLAPRFile::open(const std::string& filename, LLVolatileAPRPool* pool, apr_int32_t flags)
{
	apr_status_t s;
	apr_file_t* file_handle ;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;

	s = apr_file_open(&file_handle, filename.c_str(), flags, APR_OS_DEFAULT, pool->getVolatileAPRPool());
	if (s != APR_SUCCESS || !file_handle)
	{
		ll_apr_warn_status(s);
		LL_WARNS("APR") << " Attempting to open filename: " << filename << LL_ENDL;
		file_handle = NULL ;
		close(file_handle, pool) ;
		return NULL;
	}

	return file_handle ;
}

//static
S32 LLAPRFile::seek(apr_file_t* file_handle, apr_seek_where_t where, S32 offset)
{
	if(!file_handle)
	{
		return -1 ;
	}

	apr_status_t s;
	apr_off_t apr_offset;
	if (offset >= 0)
	{
		apr_offset = (apr_off_t)offset;
		s = apr_file_seek(file_handle, where, &apr_offset);
	}
	else
	{
		apr_offset = 0;
		s = apr_file_seek(file_handle, APR_END, &apr_offset);
	}
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		return -1;
	}
	else
	{
		llassert_always(apr_offset <= 0x7fffffff);
		return (S32)apr_offset;
	}
}

//static
S32 LLAPRFile::readEx(const std::string& filename, void *buf, S32 offset, S32 nbytes, LLVolatileAPRPool* pool)
{
	if (offset < 0)
	{
		return 0; // do nothing, negative offsets don't make sense for reads
	}

	//*****************************************
	apr_file_t* file_handle = open(filename, pool, APR_READ|APR_BINARY); 
	//*****************************************	
	if (!file_handle)
	{
		return 0;
	}

	if (offset > 0)
	{
		offset = LLAPRFile::seek(file_handle, APR_SET, offset);
	}
	
	apr_size_t bytes_read;
	if (offset < 0)
	{
		bytes_read = 0;
	}
	else
	{
		bytes_read = nbytes ;		
		apr_status_t s = apr_file_read(file_handle, buf, &bytes_read);
		if (s != APR_SUCCESS)
		{
			LL_WARNS("APR") << " Attempting to read filename: " << filename << LL_ENDL;
			ll_apr_warn_status(s);
			bytes_read = 0;
		}
		else
		{
			llassert_always(bytes_read <= 0x7fffffff);		
		}
	}
	
	//*****************************************
	close(file_handle, pool) ; 
	//*****************************************
	return (S32)bytes_read;
}

//static
S32 LLAPRFile::writeEx(const std::string& filename, void *buf, S32 offset, S32 nbytes, LLVolatileAPRPool* pool)
{
	apr_int32_t flags = APR_CREATE|APR_WRITE|APR_BINARY;
	if (offset < 0)
	{
		flags |= APR_APPEND;
		offset = 0;
	}
	
	//*****************************************
	apr_file_t* file_handle = open(filename, pool, flags);
	//*****************************************
	if (!file_handle)
	{
		return 0;
	}

	if (offset > 0)
	{
		offset = LLAPRFile::seek(file_handle, APR_SET, offset);
	}
	
	apr_size_t bytes_written;
	if (offset < 0)
	{
		bytes_written = 0;
	}
	else
	{
		bytes_written = nbytes ;		
		apr_status_t s = apr_file_write(file_handle, buf, &bytes_written);
		if (s != APR_SUCCESS)
		{
			LL_WARNS("APR") << " Attempting to write filename: " << filename << LL_ENDL;
			ll_apr_warn_status(s);
			bytes_written = 0;
		}
		else
		{
			llassert_always(bytes_written <= 0x7fffffff);
		}
	}

	//*****************************************
	LLAPRFile::close(file_handle, pool);
	//*****************************************

	return (S32)bytes_written;
}

//static
bool LLAPRFile::remove(const std::string& filename, LLVolatileAPRPool* pool)
{
	apr_status_t s;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_file_remove(filename.c_str(), pool->getVolatileAPRPool());
	pool->clearVolatileAPRPool() ;

	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		LL_WARNS("APR") << " Attempting to remove filename: " << filename << LL_ENDL;
		return false;
	}
	return true;
}

//static
bool LLAPRFile::rename(const std::string& filename, const std::string& newname, LLVolatileAPRPool* pool)
{
	apr_status_t s;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_file_rename(filename.c_str(), newname.c_str(), pool->getVolatileAPRPool());
	pool->clearVolatileAPRPool() ;
	
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		LL_WARNS("APR") << " Attempting to rename filename: " << filename << LL_ENDL;
		return false;
	}
	return true;
}

//static
bool LLAPRFile::isExist(const std::string& filename, LLVolatileAPRPool* pool, apr_int32_t flags)
{
	apr_file_t* apr_file;
	apr_status_t s;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_file_open(&apr_file, filename.c_str(), flags, APR_OS_DEFAULT, pool->getVolatileAPRPool());	

	if (s != APR_SUCCESS || !apr_file)
	{
		pool->clearVolatileAPRPool() ;
		return false;
	}
	else
	{
		apr_file_close(apr_file) ;
		pool->clearVolatileAPRPool() ;
		return true;
	}
}

//static
S32 LLAPRFile::size(const std::string& filename, LLVolatileAPRPool* pool)
{
	apr_file_t* apr_file;
	apr_finfo_t info;
	apr_status_t s;
	
	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_file_open(&apr_file, filename.c_str(), APR_READ, APR_OS_DEFAULT, pool->getVolatileAPRPool());
	
	if (s != APR_SUCCESS || !apr_file)
	{		
		pool->clearVolatileAPRPool() ;
		
		return 0;
	}
	else
	{
		apr_status_t s = apr_file_info_get(&info, APR_FINFO_SIZE, apr_file);		

		apr_file_close(apr_file) ;
		pool->clearVolatileAPRPool() ;
		
		if (s == APR_SUCCESS)
		{
			return (S32)info.size;
		}
		else
		{
			return 0;
		}
	}
}

//static
bool LLAPRFile::makeDir(const std::string& dirname, LLVolatileAPRPool* pool)
{
	apr_status_t s;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_dir_make(dirname.c_str(), APR_FPROT_OS_DEFAULT, pool->getVolatileAPRPool());
	pool->clearVolatileAPRPool() ;
		
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		LL_WARNS("APR") << " Attempting to make directory: " << dirname << LL_ENDL;
		return false;
	}
	return true;
}

//static
bool LLAPRFile::removeDir(const std::string& dirname, LLVolatileAPRPool* pool)
{
	apr_status_t s;

	pool = pool ? pool : LLAPRFile::sAPRFilePoolp ;
	s = apr_file_remove(dirname.c_str(), pool->getVolatileAPRPool());
	pool->clearVolatileAPRPool() ;
	
	if (s != APR_SUCCESS)
	{
		ll_apr_warn_status(s);
		LL_WARNS("APR") << " Attempting to remove directory: " << dirname << LL_ENDL;
		return false;
	}
	return true;
}
//
//end of static components of LLAPRFile
//*******************************************************************************************************************************
//
