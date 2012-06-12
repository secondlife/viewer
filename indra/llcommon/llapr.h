/** 
 * @file llapr.h
 * @author Phoenix
 * @date 2004-11-28
 * @brief Helper functions for using the apache portable runtime library.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLAPR_H
#define LL_LLAPR_H

#if LL_LINUX || LL_SOLARIS
#include <sys/param.h>  // Need PATH_MAX in APR headers...
#endif
#if LL_WINDOWS
	// Limit Windows API to small and manageable set.
	// If you get undefined symbols, find the appropriate
	// Windows header file and include that in your .cpp file.
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <windows.h>
#endif

#include <boost/noncopyable.hpp>

#include "apr_thread_proc.h"
#include "apr_thread_mutex.h"
#include "apr_getopt.h"
#include "apr_signal.h"
#include "apr_atomic.h"
#include "llstring.h"

extern LL_COMMON_API apr_thread_mutex_t* gLogMutexp;
extern apr_thread_mutex_t* gCallStacksLogMutexp;

struct apr_dso_handle_t;

/** 
 * @brief initialize the common apr constructs -- apr itself, the
 * global pool, and a mutex.
 */
void LL_COMMON_API ll_init_apr();

/** 
 * @brief Cleanup those common apr constructs.
 */
void LL_COMMON_API ll_cleanup_apr();

//
//LL apr_pool
//manage apr_pool_t, destroy allocated apr_pool in the destruction function.
//
class LL_COMMON_API LLAPRPool
{
public:
	LLAPRPool(apr_pool_t *parent = NULL, apr_size_t size = 0, BOOL releasePoolFlag = TRUE) ;
	virtual ~LLAPRPool() ;

	virtual apr_pool_t* getAPRPool() ;
	apr_status_t getStatus() {return mStatus ; }

protected:
	void releaseAPRPool() ;
	void createAPRPool() ;

protected:
	apr_pool_t*  mPool ;              //pointing to an apr_pool
	apr_pool_t*  mParent ;			  //parent pool
	apr_size_t   mMaxSize ;           //max size of mPool, mPool should return memory to system if allocated memory beyond this limit. However it seems not to work.
	apr_status_t mStatus ;            //status when creating the pool
	BOOL         mReleasePoolFlag ;   //if set, mPool is destroyed when LLAPRPool is deleted. default value is true.
};

//
//volatile LL apr_pool
//which clears memory automatically.
//so it can not hold static data or data after memory is cleared
//
class LL_COMMON_API LLVolatileAPRPool : public LLAPRPool
{
public:
	LLVolatileAPRPool(BOOL is_local = TRUE, apr_pool_t *parent = NULL, apr_size_t size = 0, BOOL releasePoolFlag = TRUE);
	virtual ~LLVolatileAPRPool();

	/*virtual*/ apr_pool_t* getAPRPool() ; //define this virtual function to avoid any mistakenly calling LLAPRPool::getAPRPool().
	apr_pool_t* getVolatileAPRPool() ;	
	void        clearVolatileAPRPool() ;

	BOOL        isFull() ;
	
private:
	S32 mNumActiveRef ; //number of active pointers pointing to the apr_pool.
	S32 mNumTotalRef ;  //number of total pointers pointing to the apr_pool since last creating.  

	apr_thread_mutex_t *mMutexp;
	apr_pool_t         *mMutexPool;
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
class LL_COMMON_API LLScopedLock : private boost::noncopyable
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
	Type operator --(int) { return apr_atomic_dec32(&mData); } // approximately --Type (0 if final is 0, non-zero otherwise)
	
private:
	apr_uint32_t mData;
};

typedef LLAtomic32<U32> LLAtomicU32;
typedef LLAtomic32<S32> LLAtomicS32;

// File IO convenience functions.
// Returns NULL if the file fails to open, sets *sizep to file size if not NULL
// abbreviated flags
#define LL_APR_R (APR_READ) // "r"
#define LL_APR_W (APR_CREATE|APR_TRUNCATE|APR_WRITE) // "w"
#define LL_APR_RB (APR_READ|APR_BINARY) // "rb"
#define LL_APR_WB (APR_CREATE|APR_TRUNCATE|APR_WRITE|APR_BINARY) // "wb"
#define LL_APR_RPB (APR_READ|APR_WRITE|APR_BINARY) // "r+b"
#define LL_APR_WPB (APR_CREATE|APR_TRUNCATE|APR_READ|APR_WRITE|APR_BINARY) // "w+b"

//
//apr_file manager
//which: 1)only keeps one file open;
//       2)closes the open file in the destruction function
//       3)informs the apr_pool to clean the memory when the file is closed.
//Note: please close an open file at the earliest convenience. 
//      especially do not put some time-costly operations between open() and close().
//      otherwise it might lock the APRFilePool.
//there are two different apr_pools the APRFile can use:
//      1, a temporary pool passed to an APRFile function, which is used within this function and only once.
//      2, a global pool.
//

class LL_COMMON_API LLAPRFile : boost::noncopyable
{
	// make this non copyable since a copy closes the file
private:
	apr_file_t* mFile ;
	LLVolatileAPRPool *mCurrentFilePoolp ; //currently in use apr_pool, could be one of them: sAPRFilePoolp, or a temp pool. 

public:
	LLAPRFile() ;
	LLAPRFile(const std::string& filename, apr_int32_t flags, LLVolatileAPRPool* pool = NULL);
	~LLAPRFile() ;
	
	apr_status_t open(const std::string& filename, apr_int32_t flags, LLVolatileAPRPool* pool = NULL, S32* sizep = NULL);
	apr_status_t open(const std::string& filename, apr_int32_t flags, BOOL use_global_pool); //use gAPRPoolp.
	apr_status_t close() ;

	// Returns actual offset, -1 if seek fails
	S32 seek(apr_seek_where_t where, S32 offset);
	apr_status_t eof() { return apr_file_eof(mFile);}

	// Returns bytes read/written, 0 if read/write fails:
	S32 read(void* buf, S32 nbytes);
	S32 write(const void* buf, S32 nbytes);
	
	apr_file_t* getFileHandle() {return mFile;}	

private:
	apr_pool_t* getAPRFilePool(apr_pool_t* pool) ;	
	
//
//*******************************************************************************************************************************
//static components
//
public:
	static LLVolatileAPRPool *sAPRFilePoolp ; //a global apr_pool for APRFile, which is used only when local pool does not exist.

private:
	static apr_file_t* open(const std::string& filename, LLVolatileAPRPool* pool, apr_int32_t flags);
	static apr_status_t close(apr_file_t* file, LLVolatileAPRPool* pool) ;
	static S32 seek(apr_file_t* file, apr_seek_where_t where, S32 offset);
public:
	// returns false if failure:
	static bool remove(const std::string& filename, LLVolatileAPRPool* pool = NULL);
	static bool rename(const std::string& filename, const std::string& newname, LLVolatileAPRPool* pool = NULL);
	static bool isExist(const std::string& filename, LLVolatileAPRPool* pool = NULL, apr_int32_t flags = APR_READ);
	static S32 size(const std::string& filename, LLVolatileAPRPool* pool = NULL);
	static bool makeDir(const std::string& dirname, LLVolatileAPRPool* pool = NULL);
	static bool removeDir(const std::string& dirname, LLVolatileAPRPool* pool = NULL);

	// Returns bytes read/written, 0 if read/write fails:
	static S32 readEx(const std::string& filename, void *buf, S32 offset, S32 nbytes, LLVolatileAPRPool* pool = NULL);	
	static S32 writeEx(const std::string& filename, void *buf, S32 offset, S32 nbytes, LLVolatileAPRPool* pool = NULL); // offset<0 means append
//*******************************************************************************************************************************
};

/**
 * @brief Function which appropriately logs error or remains quiet on
 * APR_SUCCESS.
 * @return Returns <code>true</code> if status is an error condition.
 */
bool LL_COMMON_API ll_apr_warn_status(apr_status_t status);
/// There's a whole other APR error-message function if you pass a DSO handle.
bool LL_COMMON_API ll_apr_warn_status(apr_status_t status, apr_dso_handle_t* handle);

void LL_COMMON_API ll_apr_assert_status(apr_status_t status);
void LL_COMMON_API ll_apr_assert_status(apr_status_t status, apr_dso_handle_t* handle);

extern "C" LL_COMMON_API apr_pool_t* gAPRPoolp; // Global APR memory pool

#endif // LL_LLAPR_H
