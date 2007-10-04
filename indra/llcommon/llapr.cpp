/** 
 * @file llapr.cpp
 * @author Phoenix
 * @date 2004-11-28
 * @brief Helper functions for using the apache portable runtime library.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#include "linden_common.h"
#include "llapr.h"

apr_pool_t *gAPRPoolp = NULL; // Global APR memory pool
apr_thread_mutex_t *gLogMutexp = NULL;


void ll_init_apr()
{
	if (!gAPRPoolp)
	{
		// Initialize APR and create the global pool
		apr_initialize();
		apr_pool_create(&gAPRPoolp, NULL);

		// Initialize the logging mutex
		apr_thread_mutex_create(&gLogMutexp, APR_THREAD_MUTEX_UNNESTED, gAPRPoolp);
	}
}


void ll_cleanup_apr()
{
	llinfos << "Cleaning up APR" << llendl;

	if (gLogMutexp)
	{
		// Clean up the logging mutex

		// All other threads NEED to be done before we clean up APR, so this is okay.
		apr_thread_mutex_destroy(gLogMutexp);
		gLogMutexp = NULL;
	}
	if (gAPRPoolp)
	{
		apr_pool_destroy(gAPRPoolp);
		gAPRPoolp = NULL;
	}
	apr_terminate();
}

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

//
// Misc functions
//
bool ll_apr_warn_status(apr_status_t status)
{
	if(APR_SUCCESS == status) return false;
	char buf[MAX_STRING];	/* Flawfinder: ignore */
	llwarns << "APR: " << apr_strerror(status, buf, MAX_STRING) << llendl;
	return true;
}

void ll_apr_assert_status(apr_status_t status)
{
	llassert(ll_apr_warn_status(status) == false);
}

// File I/O
apr_file_t* ll_apr_file_open(const LLString& filename, apr_int32_t flags, S32* sizep, apr_pool_t* pool)
{
	apr_file_t* apr_file;
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_open(&apr_file, filename.c_str(), flags, APR_OS_DEFAULT, pool);
	if (s != APR_SUCCESS)
	{
		if (sizep)
		{
			*sizep = 0;
		}
		return NULL;
	}

	if (sizep)
	{
		S32 file_size = 0;
		apr_off_t offset = 0;
		if (apr_file_seek(apr_file, APR_END, &offset) == APR_SUCCESS)
		{
			llassert_always(offset <= 0x7fffffff);
			file_size = (S32)offset;
			offset = 0;
			apr_file_seek(apr_file, APR_SET, &offset);
		}
		*sizep = file_size;
	}

	return apr_file;
}
apr_file_t* ll_apr_file_open(const LLString& filename, apr_int32_t flags, S32* sizep)
{
	return ll_apr_file_open(filename, flags, sizep, NULL);
}
apr_file_t* ll_apr_file_open(const LLString& filename, apr_int32_t flags, apr_pool_t* pool)
{
	return ll_apr_file_open(filename, flags, NULL, pool);
}
apr_file_t* ll_apr_file_open(const LLString& filename, apr_int32_t flags)
{
	return ll_apr_file_open(filename, flags, NULL, NULL);
}

S32 ll_apr_file_read(apr_file_t* apr_file, void *buf, S32 nbytes)
{
	apr_size_t sz = nbytes;
	apr_status_t s = apr_file_read(apr_file, buf, &sz);
	if (s != APR_SUCCESS)
	{
		return 0;
	}
	else
	{
		llassert_always(sz <= 0x7fffffff);
		return (S32)sz;
	}
}

S32 ll_apr_file_read_ex(const LLString& filename, apr_pool_t* pool, void *buf, S32 offset, S32 nbytes)
{
	if (pool == NULL) pool = gAPRPoolp;
	apr_file_t* filep = ll_apr_file_open(filename, APR_READ|APR_BINARY, pool);
	if (!filep)
	{
		return 0;
	}
	S32 off;
	if (offset < 0)
		off = ll_apr_file_seek(filep, APR_END, 0);
	else
		off = ll_apr_file_seek(filep, APR_SET, offset);
	S32 bytes_read;
	if (off < 0)
	{
		bytes_read = 0;
	}
	else
	{
		bytes_read = ll_apr_file_read(filep, buf, nbytes );
	}
	apr_file_close(filep);

	return bytes_read;
}

S32 ll_apr_file_write(apr_file_t* apr_file, const void *buf, S32 nbytes)
{
	apr_size_t sz = nbytes;
	apr_status_t s = apr_file_write(apr_file, buf, &sz);
	if (s != APR_SUCCESS)
	{
		return 0;
	}
	else
	{
		llassert_always(sz <= 0x7fffffff);
		return (S32)sz;
	}
}

S32 ll_apr_file_write_ex(const LLString& filename, apr_pool_t* pool, void *buf, S32 offset, S32 nbytes)
{
	if (pool == NULL) pool = gAPRPoolp;
	apr_int32_t flags = APR_CREATE|APR_WRITE|APR_BINARY;
	if (offset < 0)
	{
		flags |= APR_APPEND;
		offset = 0;
	}
	apr_file_t* filep = ll_apr_file_open(filename, flags, pool);
	if (!filep)
	{
		return 0;
	}
	if (offset > 0)
	{
		offset = ll_apr_file_seek(filep, APR_SET, offset);
	}
	S32 bytes_written;
	if (offset < 0)
	{
		bytes_written = 0;
	}
	else
	{
		bytes_written = ll_apr_file_write(filep, buf, nbytes );
	}
	apr_file_close(filep);

	return bytes_written;
}

S32 ll_apr_file_seek(apr_file_t* apr_file, apr_seek_where_t where, S32 offset)
{
	apr_status_t s;
	apr_off_t apr_offset;
	if (offset >= 0)
	{
		apr_offset = (apr_off_t)offset;
		s = apr_file_seek(apr_file, where, &apr_offset);
	}
	else
	{
		apr_offset = 0;
		s = apr_file_seek(apr_file, APR_END, &apr_offset);
	}
	if (s != APR_SUCCESS)
	{
		return -1;
	}
	else
	{
		llassert_always(apr_offset <= 0x7fffffff);
		return (S32)apr_offset;
	}
}

bool ll_apr_file_remove(const LLString& filename, apr_pool_t* pool)
{
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_remove(filename.c_str(), pool);
	if (s != APR_SUCCESS)
	{
		llwarns << "ll_apr_file_remove failed on file: " << filename << llendl;
		return false;
	}
	return true;
}

bool ll_apr_file_rename(const LLString& filename, const LLString& newname, apr_pool_t* pool)
{
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_rename(filename.c_str(), newname.c_str(), pool);
	if (s != APR_SUCCESS)
	{
		llwarns << "ll_apr_file_rename failed on file: " << filename << llendl;
		return false;
	}
	return true;
}

bool ll_apr_file_exists(const LLString& filename, apr_pool_t* pool)
{
	apr_file_t* apr_file;
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_open(&apr_file, filename.c_str(), APR_READ, APR_OS_DEFAULT, pool);
	if (s != APR_SUCCESS || !apr_file)
	{
		return false;
	}
	else
	{
		apr_file_close(apr_file);
		return true;
	}
}

S32 ll_apr_file_size(const LLString& filename, apr_pool_t* pool)
{
	apr_file_t* apr_file;
	apr_finfo_t info;
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_open(&apr_file, filename.c_str(), APR_READ, APR_OS_DEFAULT, pool);
	if (s != APR_SUCCESS || !apr_file)
	{
		return 0;
	}
	else
	{
		apr_status_t s = apr_file_info_get(&info, APR_FINFO_SIZE, apr_file);
		apr_file_close(apr_file);
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

bool ll_apr_dir_make(const LLString& dirname, apr_pool_t* pool)
{
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_dir_make(dirname.c_str(), APR_FPROT_OS_DEFAULT, pool);
	if (s != APR_SUCCESS)
	{
		llwarns << "ll_apr_file_remove failed on file: " << dirname << llendl;
		return false;
	}
	return true;
}

bool ll_apr_dir_remove(const LLString& dirname, apr_pool_t* pool)
{
	apr_status_t s;
	if (pool == NULL) pool = gAPRPoolp;
	s = apr_file_remove(dirname.c_str(), pool);
	if (s != APR_SUCCESS)
	{
		llwarns << "ll_apr_file_remove failed on file: " << dirname << llendl;
		return false;
	}
	return true;
}

