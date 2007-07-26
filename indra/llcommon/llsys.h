/** 
 * @file llsys.h
 * @brief System information debugging classes.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

class LLOSInfo
{
public:
	LLOSInfo();
	void stream(std::ostream& s) const;

	const std::string& getOSString() const;

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
};


class LLCPUInfo
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

class LLMemoryInfo
{
public:
	LLMemoryInfo();
	void stream(std::ostream& s) const;

	U32 getPhysicalMemory() const;
};


std::ostream& operator<<(std::ostream& s, const LLOSInfo& info);
std::ostream& operator<<(std::ostream& s, const LLCPUInfo& info);
std::ostream& operator<<(std::ostream& s, const LLMemoryInfo& info);

// gunzip srcfile into dstfile.  Returns FALSE on error.
BOOL gunzip_file(const char *srcfile, const char *dstfile);
// gzip srcfile into dstfile.  Returns FALSE on error.
BOOL gzip_file(const char *srcfile, const char *dstfile);

extern LLCPUInfo gSysCPU;

#endif // LL_LLSYS_H
