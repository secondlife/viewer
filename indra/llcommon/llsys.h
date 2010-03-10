/** 
 * @file llsys.h
 * @brief System information debugging classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_SYS_H
#define LL_SYS_H

//
// The LLOSInfo, LLCPUInfo, and LLMemoryInfo classes are essentially
// the same, but query different machine subsystems. Here's how you
// use an LLCPUInfo object:
//
//  LLCPUInfo info;
//  llinfos << info << llendl;
//

#include <iosfwd>
#include <string>

class LL_COMMON_API LLOSInfo
{
public:
	LLOSInfo();
	void stream(std::ostream& s) const;

	const std::string& getOSString() const;
	const std::string& getOSStringSimple() const;

	S32 mMajorVer;
	S32 mMinorVer;
	S32 mBuild;

#ifndef LL_WINDOWS
	static S32 getMaxOpenFiles();
#endif

	static U32 getProcessVirtualSizeKB();
	static U32 getProcessResidentSizeKB();
private:
	std::string mOSString;
	std::string mOSStringSimple;
};


class LL_COMMON_API LLCPUInfo
{
public:
	LLCPUInfo();	
	void stream(std::ostream& s) const;

	std::string getCPUString() const;

	bool hasAltivec() const;
	bool hasSSE() const;
	bool hasSSE2() const;
	S32	 getMhz() const;

	// Family is "AMD Duron" or "Intel Pentium Pro"
	const std::string& getFamily() const { return mFamily; }

private:
	bool mHasSSE;
	bool mHasSSE2;
	bool mHasAltivec;
	S32 mCPUMhz;
	std::string mFamily;
	std::string mCPUString;
};

//=============================================================================
//
//	CLASS		LLMemoryInfo

class LL_COMMON_API LLMemoryInfo

/*!	@brief		Class to query the memory subsystem

	@details
		Here's how you use an LLMemoryInfo:
		
		LLMemoryInfo info;
<br>	llinfos << info << llendl;
*/
{
public:
	LLMemoryInfo(); ///< Default constructor
	void stream(std::ostream& s) const;	///< output text info to s

	U32 getPhysicalMemoryKB() const; ///< Memory size in KiloBytes
	
	/*! Memory size in bytes, if total memory is >= 4GB then U32_MAX will
	**  be returned.
	*/
	U32 getPhysicalMemoryClamped() const; ///< Memory size in clamped bytes
};

LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLOSInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info);
LL_COMMON_API std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info);

// gunzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gunzip_file(const std::string& srcfile, const std::string& dstfile);
// gzip srcfile into dstfile.  Returns FALSE on error.
BOOL LL_COMMON_API gzip_file(const std::string& srcfile, const std::string& dstfile);

extern LL_COMMON_API LLCPUInfo gSysCPU;

#endif // LL_LLSYS_H
