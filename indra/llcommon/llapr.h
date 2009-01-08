/** 
 * @file llapr.h
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

#ifndef LL_LLAPR_H
#define LL_LLAPR_H

#if LL_LINUX || LL_SOLARIS
#include <sys/param.h>  // Need PATH_MAX in APR headers...
#endif

#include <boost/noncopyable.hpp>

#include "apr_thread_proc.h"
#include "apr_thread_mutex.h"
#include "apr_getopt.h"
#include "apr_signal.h"
#include "apr_atomic.h"
#include "llstring.h"

extern apr_thread_mutex_t* gLogMutexp;

/** 
 * @brief initialize the common apr constructs -- apr itself, the
 * global pool, and a mutex.
 */
void ll_init_apr();

/** 
 * @brief Cleanup those common apr constructs.
 */
void ll_cleanup_apr();

class LLAPRPool
{
public:
	LLAPRPool(apr_pool_t *parent = NULL, apr_size_t size = 0) ;
	~LLAPRPool() ;

	apr_pool_t* getAPRPool() {return mPool ; }
	apr_status_t getStatus() {return mStatus ; }

private:
	apr_pool_t*  mPool ;
	apr_status_t mStatus ;
} ;

/** 
 * @class LLScopedLock
 * @brief Small class to help lock and unlock mutexes.
 *
 * This class is used to have a stack level lock once you already have
 * an apr mutex handy. The constructor handles the lock, and the
 * destructor handles the unlock. Instances of this class are
 * <b>not</b> thread safe.
 */
class LLScopedLock : private boost::noncopyable
{
public:
	/**
	 * @brief Constructor which accepts a mutex, and locks it.
	 *
	 * @param mutex An allocated APR mutex. If you pass in NULL,
	 * this wrapper will not lock.
	 */
	LLScopedLock(apr_thread_mutex_t* mutex);

	/**
	 * @brief Destructor which unlocks the mutex if still locked.
	 */
	~LLScopedLock();

	/** 
	 * @brief Check lock.
	 */
	bool isLocked() const { return mLocked; }

	/** 
	 * @brief This method unlocks the mutex.
	 */
	void unlock();

protected:
	bool mLocked;
	apr_thread_mutex_t* mMutex;
};

template <typename Type> class LLAtomic32
{
public:
	LLAtomic32<Type>() {};
	LLAtomic32<Type>(Type x) {apr_atomic_set32(&mData, apr_uint32_t(x)); };
	~LLAtomic32<Type>() {};

	operator const Type() { apr_uint32_t data = apr_atomic_read32(&mData); return Type(data); }
	Type operator =(const Type& x) { apr_atomic_set32(&mData, apr_uint32_t(x)); return Type(mData); }
	void operator -=(Type x) { apr_atomic_sub32(&mData, apr_uint32_t(x)); }
	void operator +=(Type x) { apr_atomic_add32(&mData, apr_uint32_t(x)); }
	Type operator ++(int) { return apr_atomic_inc32(&mData); } // Type++
	Type operator --(int) { return apr_atomic_dec32(&mData); } // Type--
	
private:
	apr_uint32_t mData;
};

typedef LLAtomic32<U32> LLAtomicU32;
typedef LLAtomic32<S32> LLAtomicS32;

// File IO convenience functions.
// Returns NULL if the file fails to openm sets *sizep to file size of not NULL
// abbreviated flags
#define LL_APR_R (APR_READ) // "r"
#define LL_APR_W (APR_CREATE|APR_TRUNCATE|APR_WRITE) // "w"
#define LL_APR_RB (APR_READ|APR_BINARY) // "rb"
#define LL_APR_WB (APR_CREATE|APR_TRUNCATE|APR_WRITE|APR_BINARY) // "wb"
#define LL_APR_RPB (APR_READ|APR_WRITE|APR_BINARY) // "r+b"
#define LL_APR_WPB (APR_CREATE|APR_TRUNCATE|APR_READ|APR_WRITE|APR_BINARY) // "w+b"
apr_file_t* ll_apr_file_open(const std::string& filename, apr_int32_t flags, S32* sizep, apr_pool_t* pool);
apr_file_t* ll_apr_file_open(const std::string& filename, apr_int32_t flags, S32* sizep);
apr_file_t* ll_apr_file_open(const std::string& filename, apr_int32_t flags, apr_pool_t* pool);
apr_file_t* ll_apr_file_open(const std::string& filename, apr_int32_t flags);
// Returns actual offset, -1 if seek fails
S32 ll_apr_file_seek(apr_file_t* apr_file, apr_seek_where_t where, S32 offset);
// Returns bytes read/written, 0 if read/write fails:
S32 ll_apr_file_read(apr_file_t* apr_file, void* buf, S32 nbytes);
S32 ll_apr_file_read_ex(const std::string& filename, apr_pool_t* pool, void *buf, S32 offset, S32 nbytes);
S32 ll_apr_file_write(apr_file_t* apr_file, const void* buf, S32 nbytes);
S32 ll_apr_file_write_ex(const std::string& filename, apr_pool_t* pool, void *buf, S32 offset, S32 nbytes);
// returns false if failure:
bool ll_apr_file_remove(const std::string& filename, apr_pool_t* pool = NULL);
bool ll_apr_file_rename(const std::string& filename, const std::string& newname, apr_pool_t* pool = NULL);
bool ll_apr_file_exists(const std::string& filename, apr_pool_t* pool = NULL);
S32 ll_apr_file_size(const std::string& filename, apr_pool_t* pool = NULL);
bool ll_apr_dir_make(const std::string& dirname, apr_pool_t* pool = NULL);
bool ll_apr_dir_remove(const std::string& dirname, apr_pool_t* pool = NULL);

/**
 * @brief Function which approprately logs error or remains quiet on
 * APR_SUCCESS.
 * @return Returns <code>true</code> if status is an error condition.
 */
bool ll_apr_warn_status(apr_status_t status);

void ll_apr_assert_status(apr_status_t status);

extern "C" apr_pool_t* gAPRPoolp; // Global APR memory pool

#endif // LL_LLAPR_H
