/** 
 * @file llpluginsharedmemory.h
 * @brief LLPluginSharedMemory manages a shared memory segment for use by the LLPlugin API.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
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

#ifndef LL_LLPLUGINSHAREDMEMORY_H
#define LL_LLPLUGINSHAREDMEMORY_H

class LLPluginSharedMemoryPlatformImpl;

class LLPluginSharedMemory
{
	LOG_CLASS(LLPluginSharedMemory);
public:
	LLPluginSharedMemory();
	~LLPluginSharedMemory();
	
	// Parent will use create/destroy, child will use attach/detach.
	// Message transactions will ensure child attaches after parent creates and detaches before parent destroys.
	
	// create() implicitly creates a name for the segment which is guaranteed to be unique on the host at the current time.
	bool create(size_t size);
	bool destroy(void);
	
	bool attach(const std::string &name, size_t size);
	bool detach(void);

	bool isMapped(void) const { return (mMappedAddress != NULL); };
	void *getMappedAddress(void) const { return mMappedAddress; };
	size_t getSize(void) const { return mSize; };
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
