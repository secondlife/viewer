/** 
 * @file llpluginsharedmemory.cpp
 * LLPluginSharedMemory manages a shared memory segment for use by the LLPlugin API.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */

#include "linden_common.h"

#include "llpluginsharedmemory.h"

// on Mac and Linux, we use the native shm_open/mmap interface by using
//	#define USE_SHM_OPEN_SHARED_MEMORY 1
// in the appropriate sections below.

// For Windows, use:
//	#define USE_WIN32_SHARED_MEMORY 1

// If we ever want to fall back to the apr implementation for a platform, use:
//	#define USE_APR_SHARED_MEMORY 1

#if LL_WINDOWS
//	#define USE_APR_SHARED_MEMORY 1
	#define USE_WIN32_SHARED_MEMORY 1
#elif LL_DARWIN
	#define USE_SHM_OPEN_SHARED_MEMORY 1
#elif LL_LINUX
	#define USE_SHM_OPEN_SHARED_MEMORY 1
#endif


// FIXME: This path thing is evil and unacceptable.
#if LL_WINDOWS
	#define APR_SHARED_MEMORY_PREFIX_STRING "C:\\LLPlugin_"
	// Apparnently using the "Global\\" prefix here only works from administrative accounts under Vista.
	// Other options I've seen referenced are "Local\\" and "Session\\".
	#define WIN32_SHARED_MEMORY_PREFIX_STRING "Local\\LL_"
#else
	// mac and linux
	#define APR_SHARED_MEMORY_PREFIX_STRING "/tmp/LLPlugin_"
	#define SHM_OPEN_SHARED_MEMORY_PREFIX_STRING "/LL"
#endif

#if USE_APR_SHARED_MEMORY 
	#include "llapr.h"
	#include "apr_shm.h"
#elif USE_SHM_OPEN_SHARED_MEMORY
	#include <sys/fcntl.h>
	#include <sys/mman.h>
	#include <errno.h>
#elif USE_WIN32_SHARED_MEMORY
#include <windows.h>
#endif // USE_APR_SHARED_MEMORY


int LLPluginSharedMemory::sSegmentNumber = 0;

std::string LLPluginSharedMemory::createName(void)
{
	std::stringstream newname;

#if LL_WINDOWS
	newname << GetCurrentProcessId();
#else // LL_WINDOWS
	newname << getpid();
#endif // LL_WINDOWS
		
	newname << "_" << sSegmentNumber++;
	
	return newname.str();
}

/**
 * @brief LLPluginSharedMemoryImpl is the platform-dependent implementation of LLPluginSharedMemory. TODO:DOC is this necessary/sufficient? kinda obvious.
 *
 */
class LLPluginSharedMemoryPlatformImpl
{
public:
	LLPluginSharedMemoryPlatformImpl();
	~LLPluginSharedMemoryPlatformImpl();
	
#if USE_APR_SHARED_MEMORY
	apr_shm_t* mAprSharedMemory;	
#elif USE_SHM_OPEN_SHARED_MEMORY
	int mSharedMemoryFD;
#elif USE_WIN32_SHARED_MEMORY
	HANDLE mMapFile;
#endif

};

/**
 * Constructor. Creates a shared memory segment.
 */
LLPluginSharedMemory::LLPluginSharedMemory()
{
	mSize = 0;
	mMappedAddress = NULL;
	mNeedsDestroy = false;

	mImpl = new LLPluginSharedMemoryPlatformImpl;
}

/**
 * Destructor. Uses destroy() and detach() to ensure shared memory segment is cleaned up.
 */
LLPluginSharedMemory::~LLPluginSharedMemory()
{
	if(mNeedsDestroy)
		destroy();
	else
		detach();
		
	unlink();
	
	delete mImpl;
}

#if USE_APR_SHARED_MEMORY
// MARK: apr implementation

LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mAprSharedMemory = NULL;
}

LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
	
}

bool LLPluginSharedMemory::map(void)
{
	mMappedAddress = apr_shm_baseaddr_get(mImpl->mAprSharedMemory);
	if(mMappedAddress == NULL)
	{
		return false;
	}
	
	return true;
}

bool LLPluginSharedMemory::unmap(void)
{
	// This is a no-op under apr.
	return true;
}

bool LLPluginSharedMemory::close(void)
{
	// This is a no-op under apr.
	return true;
}

bool LLPluginSharedMemory::unlink(void)
{
	// This is a no-op under apr.
	return true;
}


bool LLPluginSharedMemory::create(size_t size)
{
	mName = APR_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;
	
	apr_status_t status = apr_shm_create( &(mImpl->mAprSharedMemory), mSize, mName.c_str(), gAPRPoolp );
	
	if(ll_apr_warn_status(status))
	{
		return false;
	}

	mNeedsDestroy = true;
	
	return map();
}

bool LLPluginSharedMemory::destroy(void)
{
	if(mImpl->mAprSharedMemory)
	{
		apr_status_t status = apr_shm_destroy(mImpl->mAprSharedMemory);
		if(ll_apr_warn_status(status))
		{
			// TODO: Is this a fatal error?  I think not...
		}
		mImpl->mAprSharedMemory = NULL;
	}
	
	return true;
}

bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;
	
	apr_status_t status = apr_shm_attach( &(mImpl->mAprSharedMemory), mName.c_str(), gAPRPoolp );
	
	if(ll_apr_warn_status(status))
	{
		return false;
	}

	return map();
}


bool LLPluginSharedMemory::detach(void)
{
	if(mImpl->mAprSharedMemory)
	{
		apr_status_t status = apr_shm_detach(mImpl->mAprSharedMemory);
		if(ll_apr_warn_status(status))
		{
			// TODO: Is this a fatal error?  I think not...
		}
		mImpl->mAprSharedMemory = NULL;
	}
	
	return true;
}


#elif USE_SHM_OPEN_SHARED_MEMORY
// MARK: shm_open/mmap implementation

LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mSharedMemoryFD = -1;
}

LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
}

bool LLPluginSharedMemory::map(void)
{
	mMappedAddress = ::mmap(NULL, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mImpl->mSharedMemoryFD, 0);
	if(mMappedAddress == NULL)
	{
		return false;
	}
	
	LL_DEBUGS("Plugin") << "memory mapped at " << mMappedAddress << LL_ENDL;

	return true;
}

bool LLPluginSharedMemory::unmap(void)
{
	if(mMappedAddress != NULL)
	{
		LL_DEBUGS("Plugin") << "calling munmap(" << mMappedAddress << ", " << mSize << ")" << LL_ENDL;
		if(::munmap(mMappedAddress, mSize) == -1)	
		{
			// TODO: Is this a fatal error?  I think not...
		}
		
		mMappedAddress = NULL;
	}

	return true;
}

bool LLPluginSharedMemory::close(void)
{
	if(mImpl->mSharedMemoryFD != -1)
	{
		LL_DEBUGS("Plugin") << "calling close(" << mImpl->mSharedMemoryFD << ")" << LL_ENDL;
		if(::close(mImpl->mSharedMemoryFD) == -1)
		{
			// TODO: Is this a fatal error?  I think not...
		}
		
		mImpl->mSharedMemoryFD = -1;
	}
	return true;
}

bool LLPluginSharedMemory::unlink(void)
{
	if(!mName.empty())
	{
		if(::shm_unlink(mName.c_str()) == -1)
		{
			return false;
		}
	}
		
	return true;
}


bool LLPluginSharedMemory::create(size_t size)
{
	mName = SHM_OPEN_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;
	
	// Preemptive unlink, just in case something didn't get cleaned up.
	unlink();

	mImpl->mSharedMemoryFD = ::shm_open(mName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if(mImpl->mSharedMemoryFD == -1)
	{
		return false;
	}
	
	mNeedsDestroy = true;
	
	if(::ftruncate(mImpl->mSharedMemoryFD, mSize) == -1)
	{
		return false;
	}
	
	
	return map();
}

bool LLPluginSharedMemory::destroy(void)
{
	unmap();
	close();	
	
	return true;
}


bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;

	mImpl->mSharedMemoryFD = ::shm_open(mName.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
	if(mImpl->mSharedMemoryFD == -1)
	{
		return false;
	}
	
	// unlink here so the segment will be cleaned up automatically after the last close.
	unlink();
	
	return map();
}

bool LLPluginSharedMemory::detach(void)
{
	unmap();
	close();	
	return true;
}

#elif USE_WIN32_SHARED_MEMORY
// MARK: Win32 CreateFileMapping-based implementation

// Reference: http://msdn.microsoft.com/en-us/library/aa366551(VS.85).aspx

LLPluginSharedMemoryPlatformImpl::LLPluginSharedMemoryPlatformImpl()
{
	mMapFile = NULL;
}

LLPluginSharedMemoryPlatformImpl::~LLPluginSharedMemoryPlatformImpl()
{
	
}

bool LLPluginSharedMemory::map(void)
{
	mMappedAddress = MapViewOfFile(
		mImpl->mMapFile,			// handle to map object
		FILE_MAP_ALL_ACCESS,		// read/write permission
		0,                   
		0,                   
		mSize);
		
	if(mMappedAddress == NULL)
	{
		LL_WARNS("Plugin") << "MapViewOfFile failed: " << GetLastError() << LL_ENDL;
		return false;
	}
	
	LL_DEBUGS("Plugin") << "memory mapped at " << mMappedAddress << LL_ENDL;

	return true;
}

bool LLPluginSharedMemory::unmap(void)
{
	if(mMappedAddress != NULL)
	{
		UnmapViewOfFile(mMappedAddress);	
		mMappedAddress = NULL;
	}

	return true;
}

bool LLPluginSharedMemory::close(void)
{
	if(mImpl->mMapFile != NULL)
	{
		CloseHandle(mImpl->mMapFile);
		mImpl->mMapFile = NULL;
	}
	
	return true;
}

bool LLPluginSharedMemory::unlink(void)
{
	// This is a no-op on Windows.
	return true;
}


bool LLPluginSharedMemory::create(size_t size)
{
	mName = WIN32_SHARED_MEMORY_PREFIX_STRING;
	mName += createName();
	mSize = size;

	mImpl->mMapFile = CreateFileMappingA(
                 INVALID_HANDLE_VALUE,		// use paging file
                 NULL,						// default security 
                 PAGE_READWRITE,			// read/write access
                 0,							// max. object size 
                 mSize,						// buffer size  
                 mName.c_str());			// name of mapping object

	if(mImpl->mMapFile == NULL)
	{
		LL_WARNS("Plugin") << "CreateFileMapping failed: " << GetLastError() << LL_ENDL;
		return false;
	}

	mNeedsDestroy = true;
		
	return map();
}

bool LLPluginSharedMemory::destroy(void)
{
	unmap();
	close();
	return true;
}

bool LLPluginSharedMemory::attach(const std::string &name, size_t size)
{
	mName = name;
	mSize = size;

	mImpl->mMapFile = OpenFileMappingA(
				FILE_MAP_ALL_ACCESS,		// read/write access
				FALSE,						// do not inherit the name
				mName.c_str());				// name of mapping object
	
	if(mImpl->mMapFile == NULL)
	{
		LL_WARNS("Plugin") << "OpenFileMapping failed: " << GetLastError() << LL_ENDL;
		return false;
	}
		
	return map();
}

bool LLPluginSharedMemory::detach(void)
{
	unmap();
	close();
	return true;
}



#endif
