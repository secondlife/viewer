/** 
 * @file llpluginsharedmemory.h
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

#ifndef LL_LLPLUGINSHAREDMEMORY_H
#define LL_LLPLUGINSHAREDMEMORY_H

class LLPluginSharedMemoryPlatformImpl;

/**
 * @brief LLPluginSharedMemory manages a shared memory segment for use by the LLPlugin API.
 *
 */
class LLPluginSharedMemory
{
	LOG_CLASS(LLPluginSharedMemory);
public:
	LLPluginSharedMemory();
	~LLPluginSharedMemory();
	
	// Parent will use create/destroy, child will use attach/detach.
	// Message transactions will ensure child attaches after parent creates and detaches before parent destroys.
	
   /** 
    * Creates a shared memory segment, with a name which is guaranteed to be unique on the host at the current time. Used by parent.
    * Message transactions will (? TODO:DOC - should? must?) ensure child attaches after parent creates and detaches before parent destroys.
    *
    * @param[in] size Shared memory size in TODO:DOC units = bytes?.
    *
    * @return False for failure, true for success.
    */
	bool create(size_t size);
   /** 
    * Destroys a shared memory segment. Used by parent.
    * Message transactions will (? TODO:DOC - should? must?) ensure child attaches after parent creates and detaches before parent destroys.
    *
    * @return True. TODO:DOC - always returns true. Is this the intended behavior?
    */
	bool destroy(void);
	
   /** 
    * Creates and attaches a name to a shared memory segment. TODO:DOC what's the difference between attach() and create()?
    *
    * @param[in] name Name to attach to memory segment
    * @param[in] size Size of memory segment TODO:DOC in bytes?
    *
    * @return False on failure, true otherwise.
    */
	bool attach(const std::string &name, size_t size);
   /** 
    * Detaches shared memory segment.
    *
    * @return False on failure, true otherwise.
    */
	bool detach(void);

   /** 
    * Checks if shared memory is mapped to a non-null address.
    *
    * @return True if memory address is non-null, false otherwise.
    */
	bool isMapped(void) const { return (mMappedAddress != NULL); };
   /** 
    * Get pointer to shared memory.
    *
    * @return Pointer to shared memory.
    */
	void *getMappedAddress(void) const { return mMappedAddress; };
   /** 
    * Get size of shared memory.
    *
    * @return Size of shared memory in bytes. TODO:DOC are bytes the correct unit?
    */
	size_t getSize(void) const { return mSize; };
   /** 
    * Get name of shared memory.
    *
    * @return Name of shared memory.
    */
	std::string getName() const { return mName; };
	
private:
	bool map(void);
	bool unmap(void);
	bool close(void);
	bool unlink(void);
	
	std::string mName;
	size_t mSize;
	void *mMappedAddress;
	bool mNeedsDestroy;
	
	LLPluginSharedMemoryPlatformImpl *mImpl;
	
	static int sSegmentNumber;
	static std::string createName();
	
};



#endif // LL_LLPLUGINSHAREDMEMORY_H
