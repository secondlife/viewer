/** 
 * @file llprocessor.cpp
 * @brief Code to figure out the processor. Originally by Benjamin Jurke.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llprocessor.h"

#include "llerror.h"

//#include <memory>

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#   include <intrin.h>
#endif

#include "llsd.h"

class LLProcessorInfoImpl; // foward declaration for the mImpl;

namespace 
{
	enum cpu_info
	{
		eBrandName = 0,
		eFrequency,
		eVendor,
		eStepping,
		eFamily,
		eExtendedFamily,
		eModel,
		eExtendedModel,
		eType,
		eBrandID,
		eFamilyName
	};
		

	const char* cpu_info_names[] = 
	{
		"Processor Name",
		"Frequency",
		"Vendor",
		"Stepping",
		"Family",
		"Extended Family",
		"Model",
		"Extended Model",
		"Type",
		"Brand ID",
		"Family Name"
	};

	enum cpu_config
	{
		eMaxID,
		eMaxExtID,
		eCLFLUSHCacheLineSize,
		eAPICPhysicalID,
		eCacheLineSize,
		eL2Associativity,
		eCacheSizeK,
		eFeatureBits,
		eExtFeatureBits
	};

	const char* cpu_config_names[] =
	{
		"Max Supported CPUID level",
		"Max Supported Ext. CPUID level",
		"CLFLUSH cache line size",
		"APIC Physical ID",
		"Cache Line Size", 
		"L2 Associativity",
		"Cache Size",
		"Feature Bits",
		"Ext. Feature Bits"
	};



	// *NOTE:Mani - this contains the elements we reference directly and extensions beyond the first 32.
	// The rest of the names are referenced by bit maks returned from cpuid.
	enum cpu_features 
	{
		eSSE_Ext=25,
		eSSE2_Ext=26,

		eSSE3_Features=32,
		eMONTIOR_MWAIT=33,
		eCPLDebugStore=34,
		eThermalMonitor2=35,
		eAltivec=36
	};

	const char* cpu_feature_names[] =
	{
		"x87 FPU On Chip",
		"Virtual-8086 Mode Enhancement",
		"Debugging Extensions",
		"Page Size Extensions",
		"Time Stamp Counter",
		"RDMSR and WRMSR Support",
		"Physical Address Extensions",
		"Machine Check Exception",
		"CMPXCHG8B Instruction",
		"APIC On Chip",
		"Unknown1",
		"SYSENTER and SYSEXIT",
		"Memory Type Range Registers",
		"PTE Global Bit",
		"Machine Check Architecture",
		"Conditional Move/Compare Instruction",
		"Page Attribute Table",
		"Page Size Extension",
		"Processor Serial Number",
		"CFLUSH Extension",
		"Unknown2",
		"Debug Store",
		"Thermal Monitor and Clock Ctrl",
		"MMX Technology",
		"FXSAVE/FXRSTOR",
		"SSE Extensions",
		"SSE2 Extensions",
		"Self Snoop",
		"Hyper-threading Technology",
		"Thermal Monitor",
		"Unknown4",
		"Pend. Brk. EN.", // 31 End of FeatureInfo bits

		"SSE3 New Instructions", // 32
		"MONITOR/MWAIT", 
		"CPL Qualified Debug Store",
		"Thermal Monitor 2",

		"Altivec"
	};

	std::string compute_CPUFamilyName(const char* cpu_vendor, int family, int ext_family) 
	{
		const char* intel_string = "GenuineIntel";
		const char* amd_string = "AuthenticAMD";
		if(!strncmp(cpu_vendor, intel_string, strlen(intel_string)))
		{
			U32 composed_family = family + ext_family;
			switch(composed_family)
			{
			case 3: return "Intel i386";
			case 4: return "Intel i486";
			case 5: return "Intel Pentium";
			case 6: return "Intel Pentium Pro/2/3, Core";
			case 7: return "Intel Itanium (IA-64)";
			case 0xF: return "Intel Pentium 4";
			case 0x10: return "Intel Itanium 2 (IA-64)";
			default: return "Unknown";
			}
		}
		else if(!strncmp(cpu_vendor, amd_string, strlen(amd_string)))
		{
			U32 composed_family = (family == 0xF) 
				? family + ext_family
				: family;
			switch(composed_family)
			{
			case 4: return "AMD 80486/5x86";
			case 5: return "AMD K5/K6";
			case 6: return "AMD K7";
			case 0xF: return "AMD K8";
			case 0x10: return "AMD K8L";
			default: return "Unknown";
			}
		}
		return "Unknown";
	}
} // end unnamed namespace

// The base class for implementations.
// Each platform should override this class.
class LLProcessorInfoImpl
{
public:
	LLProcessorInfoImpl() 
	{
		mProcessorInfo["info"] = LLSD::emptyMap();
		mProcessorInfo["config"] = LLSD::emptyMap();
		mProcessorInfo["extension"] = LLSD::emptyMap();		
	}
	virtual ~LLProcessorInfoImpl() {}

	F64 getCPUFrequency() const 
	{ 
		return getInfo(eFrequency, 0).asReal(); 
	}

	bool hasSSE() const 
	{ 
		return hasExtension(cpu_feature_names[eSSE_Ext]);
	}

	bool hasSSE2() const
	{ 
		return hasExtension(cpu_feature_names[eSSE2_Ext]);
	}

	bool hasAltivec() const 
	{
		return hasExtension("Altivec"); 
	}

	std::string getCPUFamilyName() const { return getInfo(eFamilyName, "Unknown").asString(); }
	std::string getCPUBrandName() const { return getInfo(eBrandName, "Unknown").asString(); }

	std::string getCPUFeatureDescription() const 
	{
		std::ostringstream out;
		out << std::endl << std::endl;
		out << "// CPU General Information" << std::endl;
		out << "//////////////////////////" << std::endl;
		out << "Processor Name:   " << getCPUBrandName() << std::endl;
		out << "Frequency:        " << getCPUFrequency() / (F64)1000000 << " MHz" << std::endl;
		out << "Vendor:			  " << getInfo(eVendor, "Unknown").asString() << std::endl;
		out << "Family:           " << getCPUFamilyName() << " (" << getInfo(eFamily, 0) << ")" << std::endl;
		out << "Extended family:  " << getInfo(eExtendedFamily, 0) << std::endl;
		out << "Model:            " << getInfo(eModel, 0) << std::endl;
		out << "Extended model:   " << getInfo(eExtendedModel, 0) << std::endl;
		out << "Type:             " << getInfo(eType, 0) << std::endl;
		out << "Brand ID:         " << getInfo(eBrandID, 0) << std::endl;
		out << std::endl;
		out << "// CPU Configuration" << std::endl;
		out << "//////////////////////////" << std::endl;
		
		// Iterate through the dictionary of configuration options.
		LLSD configs = mProcessorInfo["config"];
		for(LLSD::map_const_iterator cfgItr = configs.beginMap(); cfgItr != configs.endMap(); ++cfgItr)
		{
			out << cfgItr->first << " = " << cfgItr->second << std::endl;
		}
		out << std::endl;
		
		out << "// CPU Extensions" << std::endl;
		out << "//////////////////////////" << std::endl;
		
		for(LLSD::map_const_iterator itr = mProcessorInfo["extension"].beginMap(); itr != mProcessorInfo["extension"].endMap(); ++itr)
		{
			out << "  " << itr->first << std::endl;			
		}
		return out.str(); 
	}

protected:
	void setInfo(cpu_info info_type, const LLSD& value) 
	{
		setInfo(cpu_info_names[info_type], value);
	}
    LLSD getInfo(cpu_info info_type, const LLSD& defaultVal) const
	{
		return getInfo(cpu_info_names[info_type], defaultVal);
	}

	void setConfig(cpu_config config_type, const LLSD& value) 
	{ 
		setConfig(cpu_config_names[config_type], value);
	}
	LLSD getConfig(cpu_config config_type, const LLSD& defaultVal) const
	{ 
		return getConfig(cpu_config_names[config_type], defaultVal);
	}

	void setExtension(const std::string& name) { mProcessorInfo["extension"][name] = "true"; }
	bool hasExtension(const std::string& name) const
	{ 
		return mProcessorInfo["extension"].has(name);
	}

private:
	void setInfo(const std::string& name, const LLSD& value) { mProcessorInfo["info"][name]=value; }
	LLSD getInfo(const std::string& name, const LLSD& defaultVal) const
	{ 
		LLSD r = mProcessorInfo["info"].get(name); 
		return r.isDefined() ? r : defaultVal;
	}
	void setConfig(const std::string& name, const LLSD& value) { mProcessorInfo["config"][name]=value; }
	LLSD getConfig(const std::string& name, const LLSD& defaultVal) const
	{ 
		LLSD r = mProcessorInfo["config"].get(name); 
		return r.isDefined() ? r : defaultVal;
	}

private:

	LLSD mProcessorInfo;
};


#ifdef LL_MSVC
// LL_MSVC and not LLWINDOWS because some of the following code 
// uses the MSVC compiler intrinsics __cpuid() and __rdtsc().

// Delays for the specified amount of milliseconds
static void _Delay(unsigned int ms)
{
	LARGE_INTEGER freq, c1, c2;
	__int64 x;

	// Get High-Res Timer frequency
	if (!QueryPerformanceFrequency(&freq))	
		return;

	// Convert ms to High-Res Timer value
	x = freq.QuadPart/1000*ms;		

	// Get first snapshot of High-Res Timer value
	QueryPerformanceCounter(&c1);		
	do
	{
		// Get second snapshot
		QueryPerformanceCounter(&c2);	
	}while(c2.QuadPart-c1.QuadPart < x);
	// Loop while (second-first < x)	
}

static F64 calculate_cpu_frequency(U32 measure_msecs)
{
	if(measure_msecs == 0)
	{
		return 0;
	}

	// After that we declare some vars and check the frequency of the high
	// resolution timer for the measure process.
	// If there"s no high-res timer, we exit.
	unsigned __int64 starttime, endtime, timedif, freq, start, end, dif;
	if (!QueryPerformanceFrequency((LARGE_INTEGER *) &freq))
	{
		return 0;
	}

	// Now we can init the measure process. We set the process and thread priority
	// to the highest available level (Realtime priority). Also we focus the
	// first processor in the multiprocessor system.
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	unsigned long dwCurPriorityClass = GetPriorityClass(hProcess);
	int iCurThreadPriority = GetThreadPriority(hThread);
	unsigned long dwProcessMask, dwSystemMask, dwNewMask = 1;
	GetProcessAffinityMask(hProcess, &dwProcessMask, &dwSystemMask);

	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	SetProcessAffinityMask(hProcess, dwNewMask);

	//// Now we call a CPUID to ensure, that all other prior called functions are
	//// completed now (serialization)
	//__asm cpuid
	int cpu_info[4] = {-1};
	__cpuid(cpu_info, 0);

	// We ask the high-res timer for the start time
	QueryPerformanceCounter((LARGE_INTEGER *) &starttime);

	// Then we get the current cpu clock and store it
	start = __rdtsc();

	// Now we wart for some msecs
	_Delay(measure_msecs);
	//	Sleep(uiMeasureMSecs);

	// We ask for the end time
	QueryPerformanceCounter((LARGE_INTEGER *) &endtime);

	// And also for the end cpu clock
	end = __rdtsc();

	// Now we can restore the default process and thread priorities
	SetProcessAffinityMask(hProcess, dwProcessMask);
	SetThreadPriority(hThread, iCurThreadPriority);
	SetPriorityClass(hProcess, dwCurPriorityClass);

	// Then we calculate the time and clock differences
	dif = end - start;
	timedif = endtime - starttime;

	// And finally the frequency is the clock difference divided by the time
	// difference. 
	F64 frequency = (F64)dif / (((F64)timedif) / freq);

	// At last we just return the frequency that is also stored in the call
	// member var uqwFrequency
	return frequency;
}

// Windows implementation
class LLProcessorInfoWindowsImpl : public LLProcessorInfoImpl
{
public:
	LLProcessorInfoWindowsImpl() :
	{
		getCPUIDInfo();
		AddInfoItem("Frequency", calculate_cpu_frequency(50));
	}

private:
	void getCPUIDInfo()
	{
		// http://msdn.microsoft.com/en-us/library/hskdteyh(VS.80).aspx

		// __cpuid with an InfoType argument of 0 returns the number of
		// valid Ids in cpu_info[0] and the CPU identification string in
		// the other three array elements. The CPU identification string is
		// not in linear order. The code below arranges the information 
		// in a human readable form.
		int cpu_info[4] = {-1};
		__cpuid(cpu_info, 0);
		unsigned int ids = (unsigned int)cpu_info[0];
		setInfo(eMaxID, ids);

		char cpu_vendor[0x20];
		memset(cpu_vendor, 0, sizeof(cpu_vendor));
		*((int*)cpu_vendor) = cpu_info[1];
		*((int*)(cpu_vendor+4)) = cpu_info[3];
		*((int*)(cpu_vendor+8)) = cpu_info[2];
		setInfo(eVendor, cpu_vendor);

		// Get the information associated with each valid Id
		for(unsigned int i=0; i<=ids; ++i)
		{
			__cpuid(cpu_info, i);

			// Interpret CPU feature information.
			if  (i == 1)
			{
				setInfo(eStepping, cpu_info[0] & 0xf);
				setInfo(eModel, (cpu_info[0] >> 4) & 0xf);
				int family = (cpu_info[0] >> 8) & 0xf;
				setInfo(eFamily, family);
				setInfo(eType, (cpu_info[0] >> 12) & 0x3);
				setInfo(eExtendedModel, (cpu_info[0] >> 16) & 0xf);
				int ext_family = (cpu_info[0] >> 20) & 0xff;
				setInfo(eExtendedFamily, ext_family);
				setInfo(eBrandIndex, cpu_info[1] & 0xff);

				setInfo(eFamilyName, compute_CPUFamilyName(family, ext_family));

				setConfig(eCLFLUSHCacheLineSize, ((cpu_info[1] >> 8) & 0xff) * 8);
				setConfig(eAPICPhysicalID, (cpu_info[1] >> 24) & 0xff);
				
				if(cpu_info[2] & 0x1)
				{
					setExtension(cpu_feature_names[eSSE3_Features]);
				}

				if(cpu_info[2] & 0x8)
				{
					setExtension(cpu_feature_names[eMONTIOR_MWAIT]);
				}
				
				if(cpu_info[2] & 0x10) || false;
				{
					setExtension(cpu_feature_names[eCPLDebugStore]);
				}
				
				if(cpu_info[2] & 0x100)
				{
					setExtension(cpu_feature_names[eThermalMonitor2]);
				}
						
				unsigned int feature_info = (unsigned int) cpu_info[3];
				for(unsigned int index = 0, bit = 1; index < eSSE3_Features; ++index, bit <<= 1)
				{
					if(feature_info & bit)
					{
						setExtension(cpu_feature_names[index]);
					}
				}
			}
		}

		// Calling __cpuid with 0x80000000 as the InfoType argument
		// gets the number of valid extended IDs.
		__cpuid(cpu_info, 0x80000000);
		unsigned int ext_ids = cpu_info[0];
		setConfig(eMaxExtID, 0);

		char cpu_brand_string[0x40];
		memset(cpu_brand_string, 0, sizeof(cpu_brand_string));

		// Get the information associated with each extended ID.
		for(unsigned int i=0x80000000; i<=mExtIds; ++i)
		{
			__cpuid(cpu_info, i);

			// Interpret CPU brand string and cache information.
			if  (i == 0x80000002)
				memcpy(cpu_brand_string, cpu_info, sizeof(cpu_info));
			else if  (i == 0x80000003)
				memcpy(cpu_brand_string + 16, cpu_info, sizeof(cpu_info));
			else if  (i == 0x80000004)
			{
				memcpy(cpu_brand_string + 32, cpu_info, sizeof(cpu_info));
				setInfo(eBrandName, cpu_brand_string);
			}
			else if  (i == 0x80000006)
			{
				setConfig(eCacheLineSize, cpu_info[2] & 0xff);
				setConfig(eL2Associativity, (cpu_info[2] >> 12) & 0xf);
				setConfig(eCacheSizeK, (cpu_info[2] >> 16) & 0xffff);
			}
		}
	}
};

#elif LL_DARWIN

#include <mach/machine.h>
#include <sys/sysctl.h>

class LLProcessorInfoDarwinImpl : public LLProcessorInfoImpl
{
public:
	LLProcessorInfoDarwinImpl() 
	{
		getCPUIDInfo();
	}

	virtual ~LLProcessorInfoDarwinImpl() {}

	virtual F64 getCPUFrequency() const { return 0; }
	virtual bool hasSSE() const { return false; }
	virtual bool hasSSE2() const { return false; }
	virtual bool hasAltivec() const { return false; }
	virtual std::string getCPUFamilyName() const { return "Unknown"; }
	virtual std::string getCPUBrandName() const { return "Unknown"; }
	virtual std::string getCPUFeatureDescription() const { return "Unknown"; }

private:
	int getSysctlInt(const char* name)
   	{
		int result = 0;
		size_t len = sizeof(int);
		int error = sysctlbyname(name, (void*)&result, &len, NULL, 0);
		return error == -1 ? 0 : result;
   	}

	uint64_t getSysctlInt64(const char* name)
   	{
		uint64_t value = 0;
		size_t size = sizeof(value);
		int result = sysctlbyname(name, (void*)&value, &size, NULL, 0);
		if ( result == 0 ) 
		{ 
			if ( size == sizeof( uint64_t ) ) 
				; 
			else if ( size == sizeof( uint32_t ) ) 
				value = (uint64_t)(( uint32_t *)&value); 
			else if ( size == sizeof( uint16_t ) ) 
				value =  (uint64_t)(( uint16_t *)&value); 
			else if ( size == sizeof( uint8_t ) ) 
				value =  (uint64_t)(( uint8_t *)&value); 
			else
			{
				LL_ERRS("Unknown type returned from sysctl!") << LL_ENDL;
			}
		}
				
		return result == -1 ? 0 : value;
   	}
	
	void getCPUIDInfo()
	{
		size_t len = 0;

		char cpu_brand_string[0x40];
		len = sizeof(cpu_brand_string);
		memset(cpu_brand_string, 0, len);
		sysctlbyname("machdep.cpu.brand_string", (void*)cpu_brand_string, &len, NULL, 0);
		cpu_brand_string[0x3f] = 0;
		setInfo(eBrandName, cpu_brand_string);
		
		char cpu_vendor[0x20];
		len = sizeof(cpu_vendor);
		memset(cpu_vendor, 0, len);
		sysctlbyname("machdep.cpu.vendor", (void*)cpu_vendor, &len, NULL, 0);
		cpu_vendor[0x1f] = 0;
		setInfo(eVendor, cpu_vendor);

		setInfo(eStepping, getSysctlInt("machdep.cpu.stepping"));
		setInfo(eModel, getSysctlInt("machdep.cpu.model"));
		int family = getSysctlInt("machdep.cpu.family");
		int ext_family = getSysctlInt("machdep.cpu.extfamily");
		setInfo(eFamily, family);
		setInfo(eExtendedFamily, ext_family);
		setInfo(eFamilyName, compute_CPUFamilyName(cpu_vendor, family, ext_family));
		setInfo(eExtendedModel, getSysctlInt("machdep.cpu.extmodel"));
		setInfo(eBrandID, getSysctlInt("machdep.cpu.brand"));
		setInfo(eType, 0); // ? where to find this?

		//setConfig(eCLFLUSHCacheLineSize, ((cpu_info[1] >> 8) & 0xff) * 8);
		//setConfig(eAPICPhysicalID, (cpu_info[1] >> 24) & 0xff);
		setConfig(eCacheLineSize, getSysctlInt("machdep.cpu.cache.linesize"));
		setConfig(eL2Associativity, getSysctlInt("machdep.cpu.cache.L2_associativity"));
		setConfig(eCacheSizeK, getSysctlInt("machdep.cpu.cache.size"));
		
		uint64_t feature_info = getSysctlInt64("machdep.cpu.feature_bits");
		S32 *feature_infos = (S32*)(&feature_info);
		
		setConfig(eFeatureBits, feature_infos[0]);

		for(unsigned int index = 0, bit = 1; index < eSSE3_Features; ++index, bit <<= 1)
		{
			if(feature_info & bit)
			{
				setExtension(cpu_feature_names[index]);
			}
		}

		// *NOTE:Mani - I didn't find any docs that assure me that machdep.cpu.feature_bits will always be
		// The feature bits I think it is. Here's a test:
#ifndef LL_RELEASE_FOR_DOWNLOAD
	#if defined(__i386__) && defined(__PIC__)
			/* %ebx may be the PIC register.  */
		#define __cpuid(level, a, b, c, d)			\
		__asm__ ("xchgl\t%%ebx, %1\n\t"			\
				"cpuid\n\t"					\
				"xchgl\t%%ebx, %1\n\t"			\
				: "=a" (a), "=r" (b), "=c" (c), "=d" (d)	\
				: "0" (level))
	#else
		#define __cpuid(level, a, b, c, d)			\
		__asm__ ("cpuid\n\t"					\
				 : "=a" (a), "=b" (b), "=c" (c), "=d" (d)	\
				 : "0" (level))
	#endif

		unsigned int eax, ebx, ecx, edx;
		__cpuid(0x1, eax, ebx, ecx, edx);
		if(feature_infos[0] != (S32)edx)
		{
			llerrs << "machdep.cpu.feature_bits doesn't match expected cpuid result!" << llendl;
		} 
#endif // LL_RELEASE_FOR_DOWNLOAD 	


		uint64_t ext_feature_info = getSysctlInt64("machdep.cpu.extfeature_bits");
		S32 *ext_feature_infos = (S32*)(&ext_feature_info);
		setConfig(eExtFeatureBits, ext_feature_infos[0]);
	}
};

#endif // LL_MSVC

//////////////////////////////////////////////////////
// Interface definition
LLProcessorInfo::LLProcessorInfo() : mImpl(NULL)
{
	// *NOTE:Mani - not thread safe.
	if(!mImpl)
	{
#ifdef LL_MSVC
		static LLProcessorInfoWindowsImpl the_impl; 
		mImpl = &the_impl;
#elif LL_DARWIN
		static LLProcessorInfoDarwinImpl the_impl; 
		mImpl = &the_impl;
#else
	#error "Unimplemented"
#endif // LL_MSVC
	}
}

LLProcessorInfo::~LLProcessorInfo() {}
F64 LLProcessorInfo::getCPUFrequency() const { return mImpl->getCPUFrequency(); }
bool LLProcessorInfo::hasSSE() const { return mImpl->hasSSE(); }
bool LLProcessorInfo::hasSSE2() const { return mImpl->hasSSE2(); }
bool LLProcessorInfo::hasAltivec() const { return mImpl->hasAltivec(); }
std::string LLProcessorInfo::getCPUFamilyName() const { return mImpl->getCPUFamilyName(); }
std::string LLProcessorInfo::getCPUBrandName() const { return mImpl->getCPUBrandName(); }
std::string LLProcessorInfo::getCPUFeatureDescription() const { return mImpl->getCPUFeatureDescription(); }

#if 0
// Filename: Processor.cpp
// =======================
// Author: Benjamin Jurke
// File history: 27.02.2002  - File created. Support for Intel and AMD processors
//               05.03.2002  - Fixed the CPUID bug: On Pre-Pentium CPUs the CPUID
//                             command is not available
//                           - The CProcessor::WriteInfoTextFile function do not 
//                             longer use Win32 file functions (-> os independend)
//                           - Optional include of the windows.h header which is
//                             still need for CProcessor::GetCPUFrequency.
//               06.03.2002  - My birthday (18th :-))
//                           - Replaced the "\r\n" line endings in function 
//                             CProcessor::cpu_infoToText by "\n"
//                           - Replaced unsigned __int64 by signed __int64 for
//                             solving some compiler conversion problems
//                           - Fixed a bug at family=6, model=6 (Celeron -> P2)
//////////////////////////////////////////////////////////////////////////////////



#include "processor.h"

#include <memory>

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>
#	include <windows.h>
#endif

#if !LL_DARWIN && !LL_SOLARIS

#ifdef PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
// We need the QueryPerformanceCounter and Sleep functions
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE 
#endif


// Some macros we often need
////////////////////////////
#define CheckBit(var, bit)   ((var & (1 << bit)) ? true : false)

#ifdef PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
// Delays for the specified amount of milliseconds
static	void	_Delay(unsigned int ms)
{
   LARGE_INTEGER	freq, c1, c2;
	__int64		x;

   // Get High-Res Timer frequency
	if (!QueryPerformanceFrequency(&freq))	
		return;
		
	// Convert ms to High-Res Timer value
	x = freq.QuadPart/1000*ms;		

   // Get first snapshot of High-Res Timer value
	QueryPerformanceCounter(&c1);		
	do
	{
            // Get second snapshot
	    QueryPerformanceCounter(&c2);	
	}while(c2.QuadPart-c1.QuadPart < x);
	// Loop while (second-first < x)	
}
#endif

// CProcessor::CProcessor
// ======================
// Class constructor:
/////////////////////////
CProcessor::CProcessor()
{
	uqwFrequency = 0;
	strCPUName[0] = 0;
	memset(&cpu_info, 0, sizeof(cpu_info));
}

// unsigned __int64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
// =========================================================================
// Function to measure the current CPU frequency
////////////////////////////////////////////////////////////////////////////
F64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
{
#ifndef PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
	return 0;
#else
	// If there are invalid measure time parameters, zero msecs for example,
	// we"ve to exit the function
	if (uiMeasureMSecs < 1)
	{
		// If theres already a measured frequency available, we return it
        if (uqwFrequency > 0)
			return uqwFrequency;
		else
			return 0;
	}

	// Now we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return 0;

	// First we get the CPUID standard level 0x00000001
	unsigned long reg;
	__asm
	{
		mov eax, 1
        cpuid
		mov reg, edx
	}

	// Then we check, if the RDTSC (Real Date Time Stamp Counter) is available.
	// This function is necessary for our measure process.
	if (!(reg & (1 << 4)))
		return 0;

	// After that we declare some vars and check the frequency of the high
	// resolution timer for the measure process.
	// If there"s no high-res timer, we exit.
	__int64 starttime, endtime, timedif, freq, start, end, dif;
	if (!QueryPerformanceFrequency((LARGE_INTEGER *) &freq))
		return 0;

	// Now we can init the measure process. We set the process and thread priority
	// to the highest available level (Realtime priority). Also we focus the
	// first processor in the multiprocessor system.
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	unsigned long dwCurPriorityClass = GetPriorityClass(hProcess);
	int iCurThreadPriority = GetThreadPriority(hThread);
	unsigned long dwProcessMask, dwSystemMask, dwNewMask = 1;
	GetProcessAffinityMask(hProcess, &dwProcessMask, &dwSystemMask);

	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	SetProcessAffinityMask(hProcess, dwNewMask);

	// Now we call a CPUID to ensure, that all other prior called functions are
	// completed now (serialization)
	__asm cpuid

	// We ask the high-res timer for the start time
	QueryPerformanceCounter((LARGE_INTEGER *) &starttime);

	// Then we get the current cpu clock and store it
	__asm 
	{
		rdtsc
		mov dword ptr [start+4], edx
		mov dword ptr [start], eax
	}

	// Now we wart for some msecs
	_Delay(uiMeasureMSecs);
//	Sleep(uiMeasureMSecs);

	// We ask for the end time
	QueryPerformanceCounter((LARGE_INTEGER *) &endtime);

	// And also for the end cpu clock
	__asm 
	{
		rdtsc
		mov dword ptr [end+4], edx
		mov dword ptr [end], eax
	}

	// Now we can restore the default process and thread priorities
	SetProcessAffinityMask(hProcess, dwProcessMask);
	SetThreadPriority(hThread, iCurThreadPriority);
	SetPriorityClass(hProcess, dwCurPriorityClass);

	// Then we calculate the time and clock differences
	dif = end - start;
	timedif = endtime - starttime;

	// And finally the frequency is the clock difference divided by the time
	// difference. 
	uqwFrequency = (F64)dif / (((F64)timedif) / freq);

	// At last we just return the frequency that is also stored in the call
	// member var uqwFrequency
	return uqwFrequency;
#endif
}

// bool CProcessor::AnalyzeIntelProcessor()
// ========================================
// Private class function for analyzing an Intel processor
//////////////////////////////////////////////////////////
bool CProcessor::AnalyzeIntelProcessor()
{
	// *NOTE:Mani -	http://www.intel.com/Assets/PDF/appnote/241618.pdf
	// According to the above doc, a lot of this what follows is wrong.
#if LL_WINDOWS
	unsigned long eaxreg, ebxreg, edxreg;

	// First we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return false;

	// Now we get the CPUID standard level 0x00000001
	__asm
	{
		mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then get the cpu model, family, type, stepping and brand id by masking
	// the eax and ebx register
	cpu_info.uiStepping = eaxreg & 0xF;
	cpu_info.uiModel    = (eaxreg >> 4) & 0xF;
	cpu_info.uiFamily   = (eaxreg >> 8) & 0xF;
	cpu_info.uiType     = (eaxreg >> 12) & 0x3;
	cpu_info.uiBrandID  = ebxreg & 0xF;

	// *NOTE:Mani - see http://www.intel.com/assets/pdf/appnote/241618.pdf
	// These values are composed according to section 2.1.2.2 of the above doc.
	cpu_info.uiExtendedFamily = ((eaxreg >> 20) & 0xFF) + cpu_info.uiFamily;
	cpu_info.uiExtendedModel = (((eaxreg >> 16) & 0xFF) << 4) + cpu_info.uiModel;

	// Getting the Brand ID string if supported.
	if (cpu_info.MaxSupportedExtendedLevel >= 0x80000004)
	{
		// If it supports the extended CPUID level 0x80000004 we read the data
		char tmp[52];		/* Flawfinder: ignore */
		memset(tmp, 0, sizeof(tmp));
        __asm
		{
			mov eax, 0x80000002
			cpuid
			mov dword ptr [tmp], eax
			mov dword ptr [tmp+4], ebx
			mov dword ptr [tmp+8], ecx
			mov dword ptr [tmp+12], edx
			mov eax, 0x80000003
			cpuid
			mov dword ptr [tmp+16], eax
			mov dword ptr [tmp+20], ebx
			mov dword ptr [tmp+24], ecx
			mov dword ptr [tmp+28], edx
			mov eax, 0x80000004
			cpuid
			mov dword ptr [tmp+32], eax
			mov dword ptr [tmp+36], ebx
			mov dword ptr [tmp+40], ecx
			mov dword ptr [tmp+44], edx
		}
		// And copy it to the brand id string
		strncpy(cpu_info.strBrandID, tmp,sizeof(cpu_info.strBrandID)-1);
		cpu_info.strBrandID[sizeof(cpu_info.strBrandID)-1]="\0";
	}
	else
	{
		static const char* INTEL_BRAND[] =
		{
			/* 0x00 */ "",
			/* 0x01 */ "0.18 micron Intel Celeron",
			/* 0x02 */ "0.18 micron Intel Pentium III",
			/* 0x03 */ "0.13 micron Intel Celeron",
			/* 0x04 */ "0.13 micron Intel Pentium III",
			/* 0x05 */ "",
			/* 0x06 */ "0.13 micron Intel Pentium III Mobile",
			/* 0x07 */ "0.13 micron Intel Celeron Mobile",
			/* 0x08 */ "0.18 micron Intel Pentium 4",
			/* 0x09 */ "0.13 micron Intel Pentium 4",
			/* 0x0A */ "0.13 micron Intel Celeron",
			/* 0x0B */ "0.13 micron Intel Pentium 4 Xeon",
			/* 0x0C */ "Intel Xeon MP",
			/* 0x0D */ "",
			/* 0x0E */ "0.18 micron Intel Pentium 4 Xeon",
			/* 0x0F */ "Mobile Intel Celeron",
			/* 0x10 */ "",
			/* 0x11 */ "Mobile Genuine Intel",
			/* 0x12 */ "Intel Celeron M",
			/* 0x13 */ "Mobile Intel Celeron",
			/* 0x14 */ "Intel Celeron",
			/* 0x15 */ "Mobile Genuine Intel",
			/* 0x16 */ "Intel Pentium M",
			/* 0x17 */ "Mobile Intel Celeron",
		};

		// Only override the brand if we have it in the lookup table.  We should
		// already have a string here from Getcpu_info().  JC
		if ( cpu_info.uiBrandID < LL_ARRAY_SIZE(INTEL_BRAND) )
		{
			strcpy(cpu_info.strBrandID, INTEL_BRAND[cpu_info.uiBrandID]);

			if (cpu_info.uiBrandID == 3 && cpu_info.uiModel == 6)
			{
				strcpy(cpu_info.strBrandID, "0.18 micron Intel Pentium III Xeon");
			}
		}
	}

	// Then we translate the cpu family
    switch (cpu_info.uiFamily)
	{
		case 3:			// Family = 3:  i386 (80386) processor family
			strcpy(cpu_info.strFamily, "Intel i386");	/* Flawfinder: ignore */	
			break;
		case 4:			// Family = 4:  i486 (80486) processor family
			strcpy(cpu_info.strFamily, "Intel i486");	/* Flawfinder: ignore */	
			break;
		case 5:			// Family = 5:  Pentium (80586) processor family
			strcpy(cpu_info.strFamily, "Intel Pentium");	/* Flawfinder: ignore */	
			break;
		case 6:			// Family = 6:  Pentium Pro (80686) processor family
			strcpy(cpu_info.strFamily, "Intel Pentium Pro/2/3, Core");	/* Flawfinder: ignore */	
			break;
		case 15:		// Family = 15:  Extended family specific
			// Masking the extended family
			cpu_info.uiExtendedFamily = (eaxreg >> 20) & 0xFF;
			switch (cpu_info.uiExtendedFamily)
			{
				case 0:			// Family = 15, Ext. Family = 0:  Pentium 4 (80786 ??) processor family
					strcpy(cpu_info.strFamily, "Intel Pentium 4");	/* Flawfinder: ignore */	
					break;
				case 1:			// Family = 15, Ext. Family = 1:  McKinley (64-bit) processor family
					strcpy(cpu_info.strFamily, "Intel McKinley (IA-64)");	/* Flawfinder: ignore */	
					break;
				default:		// Sure is sure
					strcpy(cpu_info.strFamily, "Unknown Intel Pentium 4+");	/* Flawfinder: ignore */	
					break;
			}
			break;
		default:		// Failsave
			strcpy(cpu_info.strFamily, "Unknown");	/* Flawfinder: ignore */
			break;
    }

	// Now we come to the big deal, the exact model name
	switch (cpu_info.uiFamily)
	{
		case 3:			// i386 (80386) processor family
			strcpy(cpu_info.strModel, "Unknown Intel i386");	/* Flawfinder: ignore */
			strncat(strCPUName, "Intel i386", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */		
			break;
		case 4:			// i486 (80486) processor family
			switch (cpu_info.uiModel)
			{
				case 0:			// Model = 0:  i486 DX-25/33 processor model
					strcpy(cpu_info.strModel, "Intel i486 DX-25/33");	/* Flawfinder: ignore */			
					strncat(strCPUName, "Intel i486 DX-25/33", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */		
					break;
				case 1:			// Model = 1:  i486 DX-50 processor model
					strcpy(cpu_info.strModel, "Intel i486 DX-50");	/* Flawfinder: ignore */		
					strncat(strCPUName, "Intel i486 DX-50", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */		
					break;
				case 2:			// Model = 2:  i486 SX processor model
					strcpy(cpu_info.strModel, "Intel i486 SX");	/* Flawfinder: ignore */		
					strncat(strCPUName, "Intel i486 SX", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */		
					break;
				case 3:			// Model = 3:  i486 DX2 (with i487 numeric coprocessor) processor model
					strcpy(cpu_info.strModel, "Intel i486 487/DX2");	/* Flawfinder: ignore */		
					strncat(strCPUName, "Intel i486 DX2 with i487 numeric coprocessor", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */		
					break;
				case 4:			// Model = 4:  i486 SL processor model (never heard ?!?)
					strcpy(cpu_info.strModel, "Intel i486 SL");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 SL", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 5:			// Model = 5:  i486 SX2 processor model
					strcpy(cpu_info.strModel, "Intel i486 SX2");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 SX2", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 7:			// Model = 7:  i486 write-back enhanced DX2 processor model
					strcpy(cpu_info.strModel, "Intel i486 write-back enhanced DX2");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 write-back enhanced DX2", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 8:			// Model = 8:  i486 DX4 processor model
					strcpy(cpu_info.strModel, "Intel i486 DX4");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 DX4", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 9:			// Model = 9:  i486 write-back enhanced DX4 processor model
					strcpy(cpu_info.strModel, "Intel i486 write-back enhanced DX4");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 DX4", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				default:		// ...
					strcpy(cpu_info.strModel, "Unknown Intel i486");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel i486 (Unknown model)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
			}
			break;
		case 5:			// Pentium (80586) processor family
			switch (cpu_info.uiModel)
			{
				case 0:			// Model = 0:  Pentium (P5 A-Step) processor model
					strcpy(cpu_info.strModel, "Intel Pentium (P5 A-Step)");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel Pentium (P5 A-Step core)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;		// Famous for the DIV bug, as far as I know
				case 1:			// Model = 1:  Pentium 60/66 processor model
					strcpy(cpu_info.strModel, "Intel Pentium 60/66 (P5)");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel Pentium 60/66 (P5 core)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 2:			// Model = 2:  Pentium 75-200 (P54C) processor model
					strcpy(cpu_info.strModel, "Intel Pentium 75-200 (P54C)");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel Pentium 75-200 (P54C core)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
					break;
				case 3:			// Model = 3:  Pentium overdrive for 486 systems processor model
					strcpy(cpu_info.strModel, "Intel Pentium for 486 system (P24T Overdrive)");	/* Flawfinder: ignore */	
					strncat(strCPUName, "Intel Pentium for 486 (P24T overdrive core)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 4:			// Model = 4:  Pentium MMX processor model
					strcpy(cpu_info.strModel, "Intel Pentium MMX (P55C)");	/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium MMX (P55C core)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 7:			// Model = 7:  Pentium processor model (don"t know difference to Model=2)
					strcpy(cpu_info.strModel, "Intel Pentium (P54C)");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium (P54C core)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 8:			// Model = 8:  Pentium MMX (0.25 micron) processor model
					strcpy(cpu_info.strModel, "Intel Pentium MMX (P55C), 0.25 micron");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium MMX (P55C core), 0.25 micron", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				default:		// ...
					strcpy(cpu_info.strModel, "Unknown Intel Pentium");	/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium (Unknown P5-model)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
			}
			break;
		case 6:			// Pentium Pro (80686) processor family
			switch (cpu_info.uiModel)
			{
				case 0:			// Model = 0:  Pentium Pro (P6 A-Step) processor model
					strcpy(cpu_info.strModel, "Intel Pentium Pro (P6 A-Step)");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium Pro (P6 A-Step core)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 1:			// Model = 1:  Pentium Pro
					strcpy(cpu_info.strModel, "Intel Pentium Pro (P6)");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium Pro (P6 core)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 3:			// Model = 3:  Pentium II (66 MHz FSB, I think) processor model
					strcpy(cpu_info.strModel, "Intel Pentium II Model 3, 0.28 micron");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium II (Model 3 core, 0.28 micron process)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 5:			// Model = 5:  Pentium II/Xeon/Celeron (0.25 micron) processor model
					strcpy(cpu_info.strModel, "Intel Pentium II Model 5/Xeon/Celeron, 0.25 micron");		/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium II/Xeon/Celeron (Model 5 core, 0.25 micron process)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 6:			// Model = 6:  Pentium II with internal L2 cache
					strcpy(cpu_info.strModel, "Intel Pentium II - internal L2 cache");	/*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium II with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 7:			// Model = 7:  Pentium III/Xeon (extern L2 cache) processor model
					strcpy(cpu_info.strModel, "Intel Pentium III/Pentium III Xeon - external L2 cache, 0.25 micron");		 /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium III/Pentium III Xeon (0.25 micron process) with external L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 8:			// Model = 8:  Pentium III/Xeon/Celeron (256 KB on-die L2 cache) processor model
					strcpy(cpu_info.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.18 micron");	 /*Flawfinder: ignore*/
					// We want to know it exactly:
					switch (cpu_info.uiBrandID)
					{
						case 1:			// Model = 8, Brand id = 1:  Celeron (on-die L2 cache) processor model
							strncat(strCPUName, "Intel Celeron (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
                        case 2:			// Model = 8, Brand id = 2:  Pentium III (on-die L2 cache) processor model (my current cpu :-))
							strncat(strCPUName, "Intel Pentium III (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						case 3:			// Model = 8, Brand id = 3:  Pentium III Xeon (on-die L2 cache) processor model
                            strncat(strCPUName, "Intel Pentium III Xeon (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						default:		// ...
							strncat(strCPUName, "Intel Pentium III core (unknown model, 0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
					}
					break;
				case 9:		// Model = 9:  Intel Pentium M processor, Intel Celeron M processor, model 9
					strcpy(cpu_info.strModel, "Intel Pentium M Series Processor");	 /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium M Series Processor", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 0xA:		// Model = 0xA:  Pentium III/Xeon/Celeron (1 or 2 MB on-die L2 cache) processor model
					strcpy(cpu_info.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.18 micron");	 /*Flawfinder: ignore*/
					// Exact detection:
					switch (cpu_info.uiBrandID)
					{
						case 1:			// Model = 0xA, Brand id = 1:  Celeron (1 or 2 MB on-die L2 cache (does it exist??)) processor model
							strncat(strCPUName, "Intel Celeron (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
                        case 2:			// Model = 0xA, Brand id = 2:  Pentium III (1 or 2 MB on-die L2 cache (never seen...)) processor model
							strncat(strCPUName, "Intel Pentium III (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						case 3:			// Model = 0xA, Brand id = 3:  Pentium III Xeon (1 or 2 MB on-die L2 cache) processor model
                            strncat(strCPUName, "Intel Pentium III Xeon (0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						default:		// Getting bored of this............
							strncat(strCPUName, "Intel Pentium III core (unknown model, 0.18 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
					}
					break;
				case 0xB:		// Model = 0xB: Pentium III/Xeon/Celeron (Tualatin core, on-die cache) processor model
					strcpy(cpu_info.strModel, "Intel Pentium III/Celeron/Pentium III Xeon - internal L2 cache, 0.13 micron");	 /*Flawfinder: ignore*/
					// Omniscient: ;-)
					switch (cpu_info.uiBrandID)
					{
						case 3:			// Model = 0xB, Brand id = 3:  Celeron (Tualatin core) processor model
							strncat(strCPUName, "Intel Celeron (Tualatin core, 0.13 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
                        case 4:			// Model = 0xB, Brand id = 4:  Pentium III (Tualatin core) processor model
							strncat(strCPUName, "Intel Pentium III (Tualatin core, 0.13 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						case 7:			// Model = 0xB, Brand id = 7:  Celeron mobile (Tualatin core) processor model
                            strncat(strCPUName, "Intel Celeron mobile (Tualatin core, 0.13 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
						default:		// *bored*
							strncat(strCPUName, "Intel Pentium III Tualatin core (unknown model, 0.13 micron process) with internal L2 cache", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
							break;
					}
					break;
				case 0xD:		// Model = 0xD:  Intel Pentium M processor, Intel Celeron M processor, model D
					strcpy(cpu_info.strModel, "Intel Pentium M Series Processor");	 /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium M Series Processor", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
				case 0xE:		// Model = 0xE:  Intel Core Duo processor, Intel Core Solo processor, model E
					strcpy(cpu_info.strModel, "Intel Core Series Processor");	 /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Core Series Processor", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;	
				case 0xF:		// Model = 0xF:  Intel Core 2 Duo processor, model F
					strcpy(cpu_info.strModel, "Intel Core 2 Series Processor");	 /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Core 2 Series Processor", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;	
				default:		// *more bored*
					strcpy(cpu_info.strModel, "Unknown Intel Pentium Pro/2/3, Core"); /*Flawfinder: ignore*/
					strncat(strCPUName, "Intel Pentium Pro/2/3, Core (Unknown model)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					break;
			}
			break;
		case 15:		// Extended processor family
			// Masking the extended model
			cpu_info.uiExtendedModel = (eaxreg >> 16) & 0xFF;
			switch (cpu_info.uiModel)
			{
				case 0:			// Model = 0:  Pentium 4 Willamette (A-Step) core
					if ((cpu_info.uiBrandID) == 8)	// Brand id = 8:  P4 Willamette
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Willamette (A-Step)"); /*Flawfinder: ignore*/
						strncat(strCPUName, "Intel Pentium 4 Willamette (A-Step)", sizeof(strCPUName)-(strlen(strCPUName)-1)); /*Flawfinder: ignore*/
					}
					else							// else Xeon
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Willamette Xeon (A-Step)");		/* Flawfinder: ignore */
						strncat(strCPUName, "Intel Pentium 4 Willamette Xeon (A-Step)", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */	
					}
					break;
				case 1:			// Model = 1:  Pentium 4 Willamette core
					if ((cpu_info.uiBrandID) == 8)	// Brand id = 8:  P4 Willamette
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Willamette");		/* Flawfinder: ignore */
						strncat(strCPUName, "Intel Pentium 4 Willamette", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
					}
					else							// else Xeon
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Willamette Xeon");		/* Flawfinder: ignore */
						strncat(strCPUName, "Intel Pentium 4 Willamette Xeon", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
					}
					break;
				case 2:			// Model = 2:  Pentium 4 Northwood core
					if (((cpu_info.uiBrandID) == 9) || ((cpu_info.uiBrandID) == 0xA))		// P4 Willamette
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Northwood");		/* Flawfinder: ignore */
						strncat(strCPUName, "Intel Pentium 4 Northwood", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
					}
					else							// Xeon
					{
						strcpy(cpu_info.strModel, "Intel Pentium 4 Northwood Xeon");		/* Flawfinder: ignore */
						strncat(strCPUName, "Intel Pentium 4 Northwood Xeon", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
					}
					break;
				default:		// Silly stupid never used failsave option
					strcpy(cpu_info.strModel, "Unknown Intel Pentium 4");		/* Flawfinder: ignore */
					strncat(strCPUName, "Intel Pentium 4 (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
					break;
			}
			break;
		default:		// *grmpf*
			strcpy(cpu_info.strModel, "Unknown Intel model");		/* Flawfinder: ignore */
			strncat(strCPUName, "Intel (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) - 1);		/* Flawfinder: ignore */
			break;
    }

	// After the long processor model block we now come to the processors serial
	// number.
	// First of all we check if the processor supports the serial number
	if (cpu_info.MaxSupportedLevel >= 3)
	{
		// If it supports the serial number CPUID level 0x00000003 we read the data
		unsigned long sig1, sig2, sig3;
		__asm
		{
			mov eax, 1
			cpuid
			mov sig1, eax
			mov eax, 3
			cpuid
			mov sig2, ecx
			mov sig3, edx
		}
		// Then we convert the data to a readable string
		snprintf(	/* Flawfinder: ignore */
			cpu_info.strProcessorSerial,
			sizeof(cpu_info.strProcessorSerial),
			"%04lX-%04lX-%04lX-%04lX-%04lX-%04lX",
			sig1 >> 16,
			sig1 & 0xFFFF,
			sig3 >> 16,
			sig3 & 0xFFFF,
			sig2 >> 16, sig2 & 0xFFFF);
	}
	else
	{
		// If there"s no serial number support we just put "No serial number"
		snprintf(	/* Flawfinder: ignore */
			cpu_info.strProcessorSerial,
			sizeof(cpu_info.strProcessorSerial),
			"No Processor Serial Number");	
	}

	// Now we get the standard processor extensions
	GetStandardProcessorExtensions();

	// And finally the processor configuration (caches, TLBs, ...) and translate
	// the data to readable strings
	GetStandardProcessorConfiguration();
	TranslateProcessorConfiguration();

	// At last...
	return true;
#else
	return FALSE;
#endif
}

// bool CProcessor::AnalyzeAMDProcessor()
// ======================================
// Private class function for analyzing an AMD processor
////////////////////////////////////////////////////////
bool CProcessor::AnalyzeAMDProcessor()
{
#if LL_WINDOWS
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;

	// First of all we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return 0;

	// Now we get the CPUID standard level 0x00000001
	__asm
	{
		mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then we mask the model, family, stepping and type (AMD does not support brand id)
	cpu_info.uiStepping = eaxreg & 0xF;
	cpu_info.uiModel    = (eaxreg >> 4) & 0xF;
	cpu_info.uiFamily   = (eaxreg >> 8) & 0xF;
	cpu_info.uiType     = (eaxreg >> 12) & 0x3;

	// Now we check if the processor supports the brand id string extended CPUID level
	if (cpu_info.MaxSupportedExtendedLevel >= 0x80000004)
	{
		// If it supports the extended CPUID level 0x80000004 we read the data
		char tmp[52];		/* Flawfinder: ignore */
		memset(tmp, 0, sizeof(tmp));
        __asm
		{
			mov eax, 0x80000002
			cpuid
			mov dword ptr [tmp], eax
			mov dword ptr [tmp+4], ebx
			mov dword ptr [tmp+8], ecx
			mov dword ptr [tmp+12], edx
			mov eax, 0x80000003
			cpuid
			mov dword ptr [tmp+16], eax
			mov dword ptr [tmp+20], ebx
			mov dword ptr [tmp+24], ecx
			mov dword ptr [tmp+28], edx
			mov eax, 0x80000004
			cpuid
			mov dword ptr [tmp+32], eax
			mov dword ptr [tmp+36], ebx
			mov dword ptr [tmp+40], ecx
			mov dword ptr [tmp+44], edx
		}
		// And copy it to the brand id string
		strncpy(cpu_info.strBrandID, tmp,sizeof(cpu_info.strBrandID)-1);
		cpu_info.strBrandID[sizeof(cpu_info.strBrandID)-1]="\0";
	}
	else
	{
		// Or just tell there is no brand id string support
		strcpy(cpu_info.strBrandID, "");		/* Flawfinder: ignore */
	}

	// After that we translate the processor family
    switch(cpu_info.uiFamily)
	{
		case 4:			// Family = 4:  486 (80486) or 5x86 (80486) processor family
			switch (cpu_info.uiModel)
			{
				case 3:			// Thanks to AMD for this nice form of family
				case 7:			// detection.... *grmpf*
				case 8:
				case 9:
					strcpy(cpu_info.strFamily, "AMD 80486");		/* Flawfinder: ignore */
					break;
				case 0xE:
				case 0xF:
					strcpy(cpu_info.strFamily, "AMD 5x86");		/* Flawfinder: ignore */
					break;
				default:
					strcpy(cpu_info.strFamily, "Unknown family");		/* Flawfinder: ignore */
					break;
			}
			break;
		case 5:			// Family = 5:  K5 or K6 processor family
			switch (cpu_info.uiModel)
			{
				case 0:
				case 1:
				case 2:
				case 3:
					strcpy(cpu_info.strFamily, "AMD K5");		/* Flawfinder: ignore */
					break;
				case 6:
				case 7:
				case 8:
				case 9:
					strcpy(cpu_info.strFamily, "AMD K6");		/* Flawfinder: ignore */
					break;
				default:
					strcpy(cpu_info.strFamily, "Unknown family");		/* Flawfinder: ignore */
					break;
			}
			break;
		case 6:			// Family = 6:  K7 (Athlon, ...) processor family
			strcpy(cpu_info.strFamily, "AMD K7");		/* Flawfinder: ignore */
			break;
		default:		// For security
			strcpy(cpu_info.strFamily, "Unknown family");		/* Flawfinder: ignore */
			break;
	}

	// After the family detection we come to the specific processor model
	// detection
	switch (cpu_info.uiFamily)
	{
		case 4:			// Family = 4:  486 (80486) or 5x85 (80486) processor family
			switch (cpu_info.uiModel)
			{
				case 3:			// Model = 3:  80486 DX2
					strcpy(cpu_info.strModel, "AMD 80486 DX2");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 80486 DX2", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 7:			// Model = 7:  80486 write-back enhanced DX2
					strcpy(cpu_info.strModel, "AMD 80486 write-back enhanced DX2");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 80486 write-back enhanced DX2", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 8:			// Model = 8:  80486 DX4
					strcpy(cpu_info.strModel, "AMD 80486 DX4");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 80486 DX4", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 9:			// Model = 9:  80486 write-back enhanced DX4
					strcpy(cpu_info.strModel, "AMD 80486 write-back enhanced DX4");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 80486 write-back enhanced DX4", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 0xE:		// Model = 0xE:  5x86
					strcpy(cpu_info.strModel, "AMD 5x86");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 5x86", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 0xF:		// Model = 0xF:  5x86 write-back enhanced (oh my god.....)
					strcpy(cpu_info.strModel, "AMD 5x86 write-back enhanced");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 5x86 write-back enhanced", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				default:		// ...
					strcpy(cpu_info.strModel, "Unknown AMD 80486 or 5x86 model");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD 80486 or 5x86 (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
			}
			break;
		case 5:			// Family = 5:  K5 / K6 processor family
			switch (cpu_info.uiModel)
			{
				case 0:			// Model = 0:  K5 SSA 5 (Pentium Rating *ggg* 75, 90 and 100 Mhz)
					strcpy(cpu_info.strModel, "AMD K5 SSA5 (PR75, PR90, PR100)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K5 SSA5 (PR75, PR90, PR100)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 1:			// Model = 1:  K5 5k86 (PR 120 and 133 MHz)
					strcpy(cpu_info.strModel, "AMD K5 5k86 (PR120, PR133)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K5 5k86 (PR120, PR133)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 2:			// Model = 2:  K5 5k86 (PR 166 MHz)
					strcpy(cpu_info.strModel, "AMD K5 5k86 (PR166)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K5 5k86 (PR166)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 3:			// Model = 3:  K5 5k86 (PR 200 MHz)
					strcpy(cpu_info.strModel, "AMD K5 5k86 (PR200)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K5 5k86 (PR200)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 6:			// Model = 6:  K6
					strcpy(cpu_info.strModel, "AMD K6 (0.30 micron)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K6 (0.30 micron)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 7:			// Model = 7:  K6 (0.25 micron)
					strcpy(cpu_info.strModel, "AMD K6 (0.25 micron)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K6 (0.25 micron)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 8:			// Model = 8:  K6-2
					strcpy(cpu_info.strModel, "AMD K6-2");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K6-2", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 9:			// Model = 9:  K6-III
					strcpy(cpu_info.strModel, "AMD K6-III");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K6-III", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 0xD:		// Model = 0xD:  K6-2+ / K6-III+
					strcpy(cpu_info.strModel, "AMD K6-2+ or K6-III+ (0.18 micron)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K6-2+ or K6-III+ (0.18 micron)", sizeof(strCPUName) - strlen(strCPUName) -1);	/* Flawfinder: ignore */
					break;
				default:		// ...
					strcpy(cpu_info.strModel, "Unknown AMD K5 or K6 model");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K5 or K6 (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
			}
			break;
		case 6:			// Family = 6:  K7 processor family (AMDs first good processors)
			switch (cpu_info.uiModel)
			{
				case 1:			// Athlon
					strcpy(cpu_info.strModel, "AMD Athlon (0.25 micron)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Athlon (0.25 micron)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 2:			// Athlon (0.18 micron)
					strcpy(cpu_info.strModel, "AMD Athlon (0.18 micron)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Athlon (0.18 micron)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 3:			// Duron (Spitfire core)
					strcpy(cpu_info.strModel, "AMD Duron (Spitfire)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Duron (Spitfire core)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 4:			// Athlon (Thunderbird core)
					strcpy(cpu_info.strModel, "AMD Athlon (Thunderbird)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Athlon (Thunderbird core)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 6:			// Athlon MP / Mobile Athlon (Palomino core)
					strcpy(cpu_info.strModel, "AMD Athlon MP/Mobile Athlon (Palomino)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Athlon MP/Mobile Athlon (Palomino core)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				case 7:			// Mobile Duron (Morgan core)
					strcpy(cpu_info.strModel, "AMD Mobile Duron (Morgan)");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD Mobile Duron (Morgan core)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
				default:		// ...
					strcpy(cpu_info.strModel, "Unknown AMD K7 model");		/* Flawfinder: ignore */
					strncat(strCPUName, "AMD K7 (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
					break;
			}
			break;
		default:		// ...
			strcpy(cpu_info.strModel, "Unknown AMD model");		/* Flawfinder: ignore */
			strncat(strCPUName, "AMD (Unknown model)", sizeof(strCPUName) - strlen(strCPUName) -1);		/* Flawfinder: ignore */
			break;
    }

	// Now we read the standard processor extension that are stored in the same
	// way the Intel standard extensions are
	GetStandardProcessorExtensions();

	// Then we check if theres an extended CPUID level support
	if (cpu_info.MaxSupportedExtendedLevel >= 0x80000001)
	{
		// If we can access the extended CPUID level 0x80000001 we get the
		// edx register
		__asm
		{
			mov eax, 0x80000001
			cpuid
			mov edxreg, edx
		}

		// Now we can mask some AMD specific cpu extensions
		cpu_info._Ext.EMMX_MultimediaExtensions					= CheckBit(edxreg, 22);
		cpu_info._Ext.AA64_AMD64BitArchitecture					= CheckBit(edxreg, 29);
		cpu_info._Ext._E3DNOW_InstructionExtensions				= CheckBit(edxreg, 30);
		cpu_info._Ext._3DNOW_InstructionExtensions				= CheckBit(edxreg, 31);
	}

	// After that we check if the processor supports the ext. CPUID level
	// 0x80000006
	if (cpu_info.MaxSupportedExtendedLevel >= 0x80000006)
	{
		// If it"s present, we read it out
        __asm
		{
            mov eax, 0x80000005
			cpuid
			mov eaxreg, eax
			mov ebxreg, ebx
			mov ecxreg, ecx
			mov edxreg, edx
		}

		// Then we mask the L1 Data TLB information
		if ((ebxreg >> 16) && (eaxreg >> 16))
		{
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB / 2 MB / 4MB"); 	/*Flawfinder: ignore*/
			cpu_info._Data.uiAssociativeWays = (eaxreg >> 24) & 0xFF;
			cpu_info._Data.uiEntries = (eaxreg >> 16) & 0xFF;
		}
		else if (eaxreg >> 16)
		{
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "2 MB / 4MB");		/*Flawfinder: ignore*/
			cpu_info._Data.uiAssociativeWays = (eaxreg >> 24) & 0xFF;
			cpu_info._Data.uiEntries = (eaxreg >> 16) & 0xFF;
		}
		else if (ebxreg >> 16)
		{
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB");		/*Flawfinder: ignore*/
			cpu_info._Data.uiAssociativeWays = (ebxreg >> 24) & 0xFF;
			cpu_info._Data.uiEntries = (ebxreg >> 16) & 0xFF;
		}
		if (cpu_info._Data.uiAssociativeWays == 0xFF)
			cpu_info._Data.uiAssociativeWays = (unsigned int) -1;

		// Now the L1 Instruction/Code TLB information
		if ((ebxreg & 0xFFFF) && (eaxreg & 0xFFFF))
		{
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB / 2 MB / 4MB");		/*Flawfinder: ignore*/
			cpu_info._Instruction.uiAssociativeWays = (eaxreg >> 8) & 0xFF;
			cpu_info._Instruction.uiEntries = eaxreg & 0xFF;
		}
		else if (eaxreg & 0xFFFF)
		{
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "2 MB / 4MB");		/*Flawfinder: ignore*/
			cpu_info._Instruction.uiAssociativeWays = (eaxreg >> 8) & 0xFF;
			cpu_info._Instruction.uiEntries = eaxreg & 0xFF;
		}
		else if (ebxreg & 0xFFFF)
		{
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB");	/*Flawfinder: ignore*/
			cpu_info._Instruction.uiAssociativeWays = (ebxreg >> 8) & 0xFF;
			cpu_info._Instruction.uiEntries = ebxreg & 0xFF;
		}
		if (cpu_info._Instruction.uiAssociativeWays == 0xFF)
			cpu_info._Instruction.uiAssociativeWays = (unsigned int) -1;
		
		// Then we read the L1 data cache information
		if ((ecxreg >> 24) > 0)
		{
			cpu_info._L1.Data.bPresent = true;
			snprintf(cpu_info._L1.Data.strSize, sizeof(cpu_info._L1.Data.strSize), "%d KB", ecxreg >> 24);		/* Flawfinder: ignore */
			cpu_info._L1.Data.uiAssociativeWays = (ecxreg >> 15) & 0xFF;
			cpu_info._L1.Data.uiLineSize = ecxreg & 0xFF;
		}
		// After that we read the L2 instruction/code cache information
		if ((edxreg >> 24) > 0)
		{
			cpu_info._L1.Instruction.bPresent = true;
			snprintf(cpu_info._L1.Instruction.strSize, sizeof(cpu_info._L1.Instruction.strSize), "%d KB", edxreg >> 24); 	/* Flawfinder: ignore */
			cpu_info._L1.Instruction.uiAssociativeWays = (edxreg >> 15) & 0xFF;
			cpu_info._L1.Instruction.uiLineSize = edxreg & 0xFF;
		}

		// Note: I"m not absolutely sure that the L1 page size code (the
		// "if/else if/else if" structs above) really detects the real page
		// size for the TLB. Somebody should check it....

		// Now we read the ext. CPUID level 0x80000006
        __asm
		{
			mov eax, 0x80000006
			cpuid
			mov eaxreg, eax
			mov ebxreg, ebx
			mov ecxreg, ecx
		}

		// We only mask the unified L2 cache masks (never heard of an
		// L2 cache that is divided in data and code parts)
		if (((ecxreg >> 12) & 0xF) > 0)
		{
			cpu_info._L2.bPresent = true;
			snprintf(cpu_info._L2.strSize, sizeof(cpu_info._L2.strSize), "%d KB", ecxreg >> 16);		/* Flawfinder: ignore */
			switch ((ecxreg >> 12) & 0xF)
			{
				case 1:
					cpu_info._L2.uiAssociativeWays = 1;
					break;
				case 2:
					cpu_info._L2.uiAssociativeWays = 2;
					break;
				case 4:
					cpu_info._L2.uiAssociativeWays = 4;
					break;
				case 6:
					cpu_info._L2.uiAssociativeWays = 8;
					break;
				case 8:
					cpu_info._L2.uiAssociativeWays = 16;
					break;
				case 0xF:
					cpu_info._L2.uiAssociativeWays = (unsigned int) -1;
					break;
				default:
					cpu_info._L2.uiAssociativeWays = 0;
					break;
			}
			cpu_info._L2.uiLineSize = ecxreg & 0xFF;
		}
	}
	else
	{
		// If we could not detect the ext. CPUID level 0x80000006 we
		// try to read the standard processor configuration.
		GetStandardProcessorConfiguration();
	}
	// After reading we translate the configuration to strings
	TranslateProcessorConfiguration();

	// And finally exit
	return true;
#else
	return FALSE;
#endif
}

// bool CProcessor::AnalyzeUnknownProcessor()
// ==========================================
// Private class function to analyze an unknown (No Intel or AMD) processor
///////////////////////////////////////////////////////////////////////////
bool CProcessor::AnalyzeUnknownProcessor()
{
#if LL_WINDOWS
	unsigned long eaxreg, ebxreg;

	// We check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return false;

	// First of all we read the standard CPUID level 0x00000001
	// This level should be available on every x86-processor clone
	__asm
	{
        mov eax, 1
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
	}
	// Then we mask the processor model, family, type and stepping
	cpu_info.uiStepping = eaxreg & 0xF;
	cpu_info.uiModel    = (eaxreg >> 4) & 0xF;
	cpu_info.uiFamily   = (eaxreg >> 8) & 0xF;
	cpu_info.uiType     = (eaxreg >> 12) & 0x3;

	// To have complete information we also mask the brand id
	cpu_info.uiBrandID  = ebxreg & 0xF;

	// Then we get the standard processor extensions
	GetStandardProcessorExtensions();

	// Now we mark everything we do not know as unknown
	strcpy(strCPUName, "Unknown");		/*Flawfinder: ignore*/

	strcpy(cpu_info._Data.strTLB, "Unknown");	/*Flawfinder: ignore*/
	strcpy(cpu_info._Instruction.strTLB, "Unknown");		/*Flawfinder: ignore*/
	
	strcpy(cpu_info._Trace.strCache, "Unknown");		/*Flawfinder: ignore*/
	strcpy(cpu_info._L1.Data.strCache, "Unknown");		/*Flawfinder: ignore*/
	strcpy(cpu_info._L1.Instruction.strCache, "Unknown");	/*Flawfinder: ignore*/
	strcpy(cpu_info._L2.strCache, "Unknown");		/*Flawfinder: ignore*/
	strcpy(cpu_info._L3.strCache, "Unknown");		/*Flawfinder: ignore*/

	strcpy(cpu_info.strProcessorSerial, "Unknown / Not supported");	/*Flawfinder: ignore*/

	// For the family, model and brand id we can only print the numeric value
	snprintf(cpu_info.strBrandID, sizeof(cpu_info.strBrandID), "Brand-ID number %d", cpu_info.uiBrandID);		/* Flawfinder: ignore */
	snprintf(cpu_info.strFamily, sizeof(cpu_info.strFamily), "Family number %d", cpu_info.uiFamily);		/* Flawfinder: ignore */
	snprintf(cpu_info.strModel, sizeof(cpu_info.strModel), "Model number %d", cpu_info.uiModel);		/* Flawfinder: ignore */

	// And thats it
	return true;
#else
	return FALSE;
#endif
}

// bool CProcessor::CheckCPUIDPresence()
// =====================================
// This function checks if the CPUID command is available on the current
// processor
////////////////////////////////////////////////////////////////////////
bool CProcessor::CheckCPUIDPresence()
{
#if LL_WINDOWS
	unsigned long BitChanged;
	
	// We"ve to check if we can toggle the flag register bit 21
	// If we can"t the processor does not support the CPUID command
	__asm
	{
		pushfd
		pop eax
		mov ebx, eax
		xor eax, 0x00200000 
		push eax
		popfd
		pushfd
		pop eax
		xor eax,ebx 
		mov BitChanged, eax
	}

	return ((BitChanged) ? true : false);
#else
	return FALSE;
#endif
}

// void CProcessor::DecodeProcessorConfiguration(unsigned int cfg)
// ===============================================================
// This function (or switch ?!) just translates a one-byte processor configuration
// byte to understandable values
//////////////////////////////////////////////////////////////////////////////////
void CProcessor::DecodeProcessorConfiguration(unsigned int cfg)
{
	// First we ensure that there"s only one single byte
	cfg &= 0xFF;

	// Then we do a big switch
	switch(cfg)
	{
		case 0:			// cfg = 0:  Unused
			break;
		case 0x1:		// cfg = 0x1:  code TLB present, 4 KB pages, 4 ways, 32 entries
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB");	/*Flawfinder: ignore*/
			cpu_info._Instruction.uiAssociativeWays = 4;
			cpu_info._Instruction.uiEntries = 32;
			break;
		case 0x2:		// cfg = 0x2:  code TLB present, 4 MB pages, fully associative, 2 entries
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 MB");	/*Flawfinder: ignore*/
			cpu_info._Instruction.uiAssociativeWays = 4;
			cpu_info._Instruction.uiEntries = 2;
			break;
		case 0x3:		// cfg = 0x3:  data TLB present, 4 KB pages, 4 ways, 64 entries
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB");		/*Flawfinder: ignore*/
			cpu_info._Data.uiAssociativeWays = 4;
			cpu_info._Data.uiEntries = 64;
			break;
		case 0x4:		// cfg = 0x4:  data TLB present, 4 MB pages, 4 ways, 8 entries
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 MB");	/*Flawfinder: ignore*/
			cpu_info._Data.uiAssociativeWays = 4;
			cpu_info._Data.uiEntries = 8;
			break;
		case 0x6:		// cfg = 0x6:  code L1 cache present, 8 KB, 4 ways, 32 byte lines
			cpu_info._L1.Instruction.bPresent = true;
			strcpy(cpu_info._L1.Instruction.strSize, "8 KB");	/*Flawfinder: ignore*/
			cpu_info._L1.Instruction.uiAssociativeWays = 4;
			cpu_info._L1.Instruction.uiLineSize = 32;
			break;
		case 0x8:		// cfg = 0x8:  code L1 cache present, 16 KB, 4 ways, 32 byte lines
			cpu_info._L1.Instruction.bPresent = true;
			strcpy(cpu_info._L1.Instruction.strSize, "16 KB");	/*Flawfinder: ignore*/
			cpu_info._L1.Instruction.uiAssociativeWays = 4;
			cpu_info._L1.Instruction.uiLineSize = 32;
			break;
		case 0xA:		// cfg = 0xA:  data L1 cache present, 8 KB, 2 ways, 32 byte lines
			cpu_info._L1.Data.bPresent = true;
			strcpy(cpu_info._L1.Data.strSize, "8 KB");	/*Flawfinder: ignore*/
			cpu_info._L1.Data.uiAssociativeWays = 2;
			cpu_info._L1.Data.uiLineSize = 32;
			break;
		case 0xC:		// cfg = 0xC:  data L1 cache present, 16 KB, 4 ways, 32 byte lines
			cpu_info._L1.Data.bPresent = true;
			strcpy(cpu_info._L1.Data.strSize, "16 KB");	/*Flawfinder: ignore*/
			cpu_info._L1.Data.uiAssociativeWays = 4;
			cpu_info._L1.Data.uiLineSize = 32;
			break;
		case 0x22:		// cfg = 0x22:  code and data L3 cache present, 512 KB, 4 ways, 64 byte lines, sectored
			cpu_info._L3.bPresent = true;
			strcpy(cpu_info._L3.strSize, "512 KB");	/*Flawfinder: ignore*/
			cpu_info._L3.uiAssociativeWays = 4;
			cpu_info._L3.uiLineSize = 64;
			cpu_info._L3.bSectored = true;
			break;
		case 0x23:		// cfg = 0x23:  code and data L3 cache present, 1024 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L3.bPresent = true;
			strcpy(cpu_info._L3.strSize, "1024 KB");	/*Flawfinder: ignore*/
			cpu_info._L3.uiAssociativeWays = 8;
			cpu_info._L3.uiLineSize = 64;
			cpu_info._L3.bSectored = true;
			break;
		case 0x25:		// cfg = 0x25:  code and data L3 cache present, 2048 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L3.bPresent = true;
			strcpy(cpu_info._L3.strSize, "2048 KB");	/*Flawfinder: ignore*/
			cpu_info._L3.uiAssociativeWays = 8;
			cpu_info._L3.uiLineSize = 64;
			cpu_info._L3.bSectored = true;
			break;
		case 0x29:		// cfg = 0x29:  code and data L3 cache present, 4096 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L3.bPresent = true;
			strcpy(cpu_info._L3.strSize, "4096 KB");	/*Flawfinder: ignore*/
			cpu_info._L3.uiAssociativeWays = 8;
			cpu_info._L3.uiLineSize = 64;
			cpu_info._L3.bSectored = true;
			break;
		case 0x40:		// cfg = 0x40:  no integrated L2 cache (P6 core) or L3 cache (P4 core)
			break;
		case 0x41:		// cfg = 0x41:  code and data L2 cache present, 128 KB, 4 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "128 KB");		/*Flawfinder: ignore*/
			cpu_info._L2.uiAssociativeWays = 4;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x42:		// cfg = 0x42:  code and data L2 cache present, 256 KB, 4 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "256 KB");		/*Flawfinder: ignore*/
			cpu_info._L2.uiAssociativeWays = 4;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x43:		// cfg = 0x43:  code and data L2 cache present, 512 KB, 4 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "512 KB");		/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 4;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x44:		// cfg = 0x44:  code and data L2 cache present, 1024 KB, 4 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "1 MB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 4;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x45:		// cfg = 0x45:  code and data L2 cache present, 2048 KB, 4 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "2 MB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 4;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x50:		// cfg = 0x50:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 64 entries
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Instruction.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Instruction.uiEntries = 64;
			break;
		case 0x51:		// cfg = 0x51:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 128 entries
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Instruction.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Instruction.uiEntries = 128;
			break;
		case 0x52:		// cfg = 0x52:  code TLB present, 4 KB / 4 MB / 2 MB pages, fully associative, 256 entries
			cpu_info._Instruction.bPresent = true;
			strcpy(cpu_info._Instruction.strPageSize, "4 KB / 2 MB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Instruction.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Instruction.uiEntries = 256;
			break;
		case 0x5B:		// cfg = 0x5B:  data TLB present, 4 KB / 4 MB pages, fully associative, 64 entries
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Data.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Data.uiEntries = 64;
			break;
		case 0x5C:		// cfg = 0x5C:  data TLB present, 4 KB / 4 MB pages, fully associative, 128 entries
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Data.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Data.uiEntries = 128;
			break;
		case 0x5d:		// cfg = 0x5D:  data TLB present, 4 KB / 4 MB pages, fully associative, 256 entries
			cpu_info._Data.bPresent = true;
			strcpy(cpu_info._Data.strPageSize, "4 KB / 4 MB");	/* Flawfinder: ignore */
			cpu_info._Data.uiAssociativeWays = (unsigned int) -1;
			cpu_info._Data.uiEntries = 256;
			break;
		case 0x66:		// cfg = 0x66:  data L1 cache present, 8 KB, 4 ways, 64 byte lines, sectored
			cpu_info._L1.Data.bPresent = true;
			strcpy(cpu_info._L1.Data.strSize, "8 KB");	/* Flawfinder: ignore */
			cpu_info._L1.Data.uiAssociativeWays = 4;
			cpu_info._L1.Data.uiLineSize = 64;
			break;
		case 0x67:		// cfg = 0x67:  data L1 cache present, 16 KB, 4 ways, 64 byte lines, sectored
			cpu_info._L1.Data.bPresent = true;
			strcpy(cpu_info._L1.Data.strSize, "16 KB");	/* Flawfinder: ignore */
			cpu_info._L1.Data.uiAssociativeWays = 4;
			cpu_info._L1.Data.uiLineSize = 64;
			break;
		case 0x68:		// cfg = 0x68:  data L1 cache present, 32 KB, 4 ways, 64 byte lines, sectored
			cpu_info._L1.Data.bPresent = true;
			strcpy(cpu_info._L1.Data.strSize, "32 KB");	/* Flawfinder: ignore */
			cpu_info._L1.Data.uiAssociativeWays = 4;
			cpu_info._L1.Data.uiLineSize = 64;
			break;
		case 0x70:		// cfg = 0x70:  trace L1 cache present, 12 KuOPs, 4 ways
			cpu_info._Trace.bPresent = true;
			strcpy(cpu_info._Trace.strSize, "12 K-micro-ops");	/* Flawfinder: ignore */
			cpu_info._Trace.uiAssociativeWays = 4;
			break;
		case 0x71:		// cfg = 0x71:  trace L1 cache present, 16 KuOPs, 4 ways
			cpu_info._Trace.bPresent = true;
			strcpy(cpu_info._Trace.strSize, "16 K-micro-ops");	/* Flawfinder: ignore */
			cpu_info._Trace.uiAssociativeWays = 4;
			break;
		case 0x72:		// cfg = 0x72:  trace L1 cache present, 32 KuOPs, 4 ways
			cpu_info._Trace.bPresent = true;
			strcpy(cpu_info._Trace.strSize, "32 K-micro-ops");	/* Flawfinder: ignore */
			cpu_info._Trace.uiAssociativeWays = 4;
			break;
		case 0x79:		// cfg = 0x79:  code and data L2 cache present, 128 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "128 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 64;
			cpu_info._L2.bSectored = true;
			break;
		case 0x7A:		// cfg = 0x7A:  code and data L2 cache present, 256 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "256 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 64;
			cpu_info._L2.bSectored = true;
			break;
		case 0x7B:		// cfg = 0x7B:  code and data L2 cache present, 512 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "512 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 64;
			cpu_info._L2.bSectored = true;
			break;
		case 0x7C:		// cfg = 0x7C:  code and data L2 cache present, 1024 KB, 8 ways, 64 byte lines, sectored
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "1 MB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 64;
			cpu_info._L2.bSectored = true;
			break;
		case 0x81:		// cfg = 0x81:  code and data L2 cache present, 128 KB, 8 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "128 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x82:		// cfg = 0x82:  code and data L2 cache present, 256 KB, 8 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "256 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x83:		// cfg = 0x83:  code and data L2 cache present, 512 KB, 8 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "512 KB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x84:		// cfg = 0x84:  code and data L2 cache present, 1024 KB, 8 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "1 MB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 32;
			break;
		case 0x85:		// cfg = 0x85:  code and data L2 cache present, 2048 KB, 8 ways, 32 byte lines
			cpu_info._L2.bPresent = true;
			strcpy(cpu_info._L2.strSize, "2 MB");	/* Flawfinder: ignore */
			cpu_info._L2.uiAssociativeWays = 8;
			cpu_info._L2.uiLineSize = 32;
			break;
	}
}

FORCEINLINE static char *TranslateAssociativeWays(unsigned int uiWays, char *buf)
{
	// We define 0xFFFFFFFF (= -1) as fully associative
    if (uiWays == ((unsigned int) -1))
		strcpy(buf, "fully associative");	/* Flawfinder: ignore */
	else
	{
		if (uiWays == 1)			// A one way associative cache is just direct mapped
			strcpy(buf, "direct mapped");	/* Flawfinder: ignore */
		else if (uiWays == 0)		// This should not happen...
			strcpy(buf, "unknown associative ways");	/* Flawfinder: ignore */
		else						// The x-way associative cache
			sprintf(buf, "%d ways associative", uiWays);	/* Flawfinder: ignore */
	}
	// To ease the function use we return the buffer
	return buf;
}
FORCEINLINE static void TranslateTLB(ProcessorTLB *tlb)
{
	char buf[64];	/* Flawfinder: ignore */

	// We just check if the TLB is present
	if (tlb->bPresent)
        snprintf(tlb->strTLB,sizeof(tlb->strTLB), "%s page size, %s, %d entries", tlb->strPageSize, TranslateAssociativeWays(tlb->uiAssociativeWays, buf), tlb->uiEntries);	/* Flawfinder: ignore */
	else
        strcpy(tlb->strTLB, "Not present");	/* Flawfinder: ignore */
}
FORCEINLINE static void TranslateCache(ProcessorCache *cache)
{
	char buf[64];	/* Flawfinder: ignore */

	// We just check if the cache is present
    if (cache->bPresent)
	{
		// If present we construct the string
		snprintf(cache->strCache, sizeof(cache->strCache), "%s cache size, %s, %d bytes line size", cache->strSize, TranslateAssociativeWays(cache->uiAssociativeWays, buf), cache->uiLineSize);	/* Flawfinder: ignore */
		if (cache->bSectored)
			strncat(cache->strCache, ", sectored", sizeof(cache->strCache)-strlen(cache->strCache)-1);	/* Flawfinder: ignore */
	}
	else
	{
		// Else we just say "Not present"
		strcpy(cache->strCache, "Not present");	/* Flawfinder: ignore */
	}
}

// void CProcessor::TranslateProcessorConfiguration()
// ==================================================
// Private class function to translate the processor configuration values
// to strings
/////////////////////////////////////////////////////////////////////////
void CProcessor::TranslateProcessorConfiguration()
{
	// We just call the small functions defined above
	TranslateTLB(&cpu_info._Data);
	TranslateTLB(&cpu_info._Instruction);

	TranslateCache(&cpu_info._Trace);

	TranslateCache(&cpu_info._L1.Instruction);
	TranslateCache(&cpu_info._L1.Data);
	TranslateCache(&cpu_info._L2);
	TranslateCache(&cpu_info._L3);
}

// void CProcessor::GetStandardProcessorConfiguration()
// ====================================================
// Private class function to read the standard processor configuration
//////////////////////////////////////////////////////////////////////
void CProcessor::GetStandardProcessorConfiguration()
{
#if LL_WINDOWS
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;

	// We check if the CPUID function is available
	if (!CheckCPUIDPresence())
		return;

	// First we check if the processor supports the standard
	// CPUID level 0x00000002
	if (cpu_info.MaxSupportedLevel >= 2)
	{
		// Now we go read the std. CPUID level 0x00000002 the first time
		unsigned long count, num = 255;
		for (count = 0; count < num; count++)
		{
			__asm
			{
				mov eax, 2
				cpuid
				mov eaxreg, eax
				mov ebxreg, ebx
				mov ecxreg, ecx
				mov edxreg, edx
			}
			// We have to repeat this reading for "num" times
			num = eaxreg & 0xFF;

			// Then we call the big decode switch function
			DecodeProcessorConfiguration(eaxreg >> 8);
			DecodeProcessorConfiguration(eaxreg >> 16);
			DecodeProcessorConfiguration(eaxreg >> 24);

			// If ebx contains additional data we also decode it
			if ((ebxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(ebxreg);
				DecodeProcessorConfiguration(ebxreg >> 8);
				DecodeProcessorConfiguration(ebxreg >> 16);
				DecodeProcessorConfiguration(ebxreg >> 24);
			}
			// And also the ecx register
			if ((ecxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(ecxreg);
				DecodeProcessorConfiguration(ecxreg >> 8);
				DecodeProcessorConfiguration(ecxreg >> 16);
				DecodeProcessorConfiguration(ecxreg >> 24);
			}
			// At last the edx processor register
			if ((edxreg & 0x80000000) == 0)
			{
				DecodeProcessorConfiguration(edxreg);
				DecodeProcessorConfiguration(edxreg >> 8);
				DecodeProcessorConfiguration(edxreg >> 16);
				DecodeProcessorConfiguration(edxreg >> 24);
			}
		}
	}
#endif
}

// void CProcessor::GetStandardProcessorExtensions()
// =================================================
// Private class function to read the standard processor extensions
///////////////////////////////////////////////////////////////////
void CProcessor::GetStandardProcessorExtensions()
{
#if LL_WINDOWS
	unsigned long ebxreg, edxreg;

	// We check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return;
	// We just get the standard CPUID level 0x00000001 which should be
	// available on every x86 processor
	__asm
	{
		mov eax, 1
		cpuid
		mov ebxreg, ebx
		mov edxreg, edx
	}
    
	// Then we mask some bits
	cpu_info._Ext.FPU_FloatingPointUnit							= CheckBit(edxreg, 0);
	cpu_info._Ext.VME_Virtual8086ModeEnhancements				= CheckBit(edxreg, 1);
	cpu_info._Ext.DE_DebuggingExtensions							= CheckBit(edxreg, 2);
	cpu_info._Ext.PSE_PageSizeExtensions							= CheckBit(edxreg, 3);
	cpu_info._Ext.TSC_TimeStampCounter							= CheckBit(edxreg, 4);
	cpu_info._Ext.MSR_ModelSpecificRegisters						= CheckBit(edxreg, 5);
	cpu_info._Ext.PAE_PhysicalAddressExtension					= CheckBit(edxreg, 6);
	cpu_info._Ext.MCE_MachineCheckException						= CheckBit(edxreg, 7);
	cpu_info._Ext.CX8_COMPXCHG8B_Instruction						= CheckBit(edxreg, 8);
	cpu_info._Ext.APIC_AdvancedProgrammableInterruptController	= CheckBit(edxreg, 9);
	cpu_info._Ext.APIC_ID = (ebxreg >> 24) & 0xFF;
	cpu_info._Ext.SEP_FastSystemCall								= CheckBit(edxreg, 11);
	cpu_info._Ext.MTRR_MemoryTypeRangeRegisters					= CheckBit(edxreg, 12);
	cpu_info._Ext.PGE_PTE_GlobalFlag								= CheckBit(edxreg, 13);
	cpu_info._Ext.MCA_MachineCheckArchitecture					= CheckBit(edxreg, 14);
	cpu_info._Ext.CMOV_ConditionalMoveAndCompareInstructions		= CheckBit(edxreg, 15);
	cpu_info._Ext.FGPAT_PageAttributeTable						= CheckBit(edxreg, 16);
	cpu_info._Ext.PSE36_36bitPageSizeExtension					= CheckBit(edxreg, 17);
	cpu_info._Ext.PN_ProcessorSerialNumber						= CheckBit(edxreg, 18);
	cpu_info._Ext.CLFSH_CFLUSH_Instruction						= CheckBit(edxreg, 19);
	cpu_info._Ext.CLFLUSH_InstructionCacheLineSize = (ebxreg >> 8) & 0xFF;
	cpu_info._Ext.DS_DebugStore									= CheckBit(edxreg, 21);
	cpu_info._Ext.ACPI_ThermalMonitorAndClockControl				= CheckBit(edxreg, 22);
	cpu_info._Ext.MMX_MultimediaExtensions						= CheckBit(edxreg, 23);
	cpu_info._Ext.FXSR_FastStreamingSIMD_ExtensionsSaveRestore	= CheckBit(edxreg, 24);
	cpu_info._Ext.SSE_StreamingSIMD_Extensions					= CheckBit(edxreg, 25);
	cpu_info._Ext.SSE2_StreamingSIMD2_Extensions					= CheckBit(edxreg, 26);
	cpu_info._Ext.Altivec_Extensions = false;
	cpu_info._Ext.SS_SelfSnoop									= CheckBit(edxreg, 27);
	cpu_info._Ext.HT_HyperThreading								= CheckBit(edxreg, 28);
	cpu_info._Ext.HT_HyterThreadingSiblings = (ebxreg >> 16) & 0xFF;
	cpu_info._Ext.TM_ThermalMonitor								= CheckBit(edxreg, 29);
	cpu_info._Ext.IA64_Intel64BitArchitecture					= CheckBit(edxreg, 30);
#endif
}

// const ProcessorInfo *CProcessor::Getcpu_info()
// =============================================
// Calls all the other detection function to create an detailed
// processor information
///////////////////////////////////////////////////////////////
const ProcessorInfo *CProcessor::Getcpu_info()
{
#if LL_WINDOWS
	unsigned long eaxreg, ebxreg, ecxreg, edxreg;
 
	// First of all we check if the CPUID command is available
	if (!CheckCPUIDPresence())
		return NULL;

	// We read the standard CPUID level 0x00000000 which should
	// be available on every x86 processor
	__asm
	{
		mov eax, 0
		cpuid
		mov eaxreg, eax
		mov ebxreg, ebx
		mov edxreg, edx
		mov ecxreg, ecx
	}
	// Then we connect the single register values to the vendor string
	*((unsigned long *) cpu_info.strVendor) = ebxreg;
	*((unsigned long *) (cpu_info.strVendor+4)) = edxreg;
	*((unsigned long *) (cpu_info.strVendor+8)) = ecxreg;
	// Null terminate for string comparisons below.
	cpu_info.strVendor[12] = 0;

	// We can also read the max. supported standard CPUID level
	cpu_info.MaxSupportedLevel = eaxreg & 0xFFFF;

	// Then we read the ext. CPUID level 0x80000000
	__asm
	{
        mov eax, 0x80000000
		cpuid
		mov eaxreg, eax
	}
	// ...to check the max. supportted extended CPUID level
	cpu_info.MaxSupportedExtendedLevel = eaxreg;

	// Then we switch to the specific processor vendors
	// See http://www.sandpile.org/ia32/cpuid.htm
	if (!strcmp(cpu_info.strVendor, "GenuineIntel"))
	{
		AnalyzeIntelProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "AuthenticAMD"))
	{
		AnalyzeAMDProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "UMC UMC UMC"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "CyrixInstead"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "NexGenDriven"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "CentaurHauls"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "RiseRiseRise"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "SiS SiS SiS"))
	{
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "GenuineTMx86"))
	{
		// Transmeta
		AnalyzeUnknownProcessor();
	}
	else if (!strcmp(cpu_info.strVendor, "Geode by NSC"))
	{
		AnalyzeUnknownProcessor();
	}
	else
	{
		AnalyzeUnknownProcessor();
	}
#endif
	// After all we return the class cpu_info member var
	return (&cpu_info);
}

#elif LL_SOLARIS
#include <kstat.h>

#if defined(__i386)
#include <sys/auxv.h>
#endif

// ======================
// Class constructor:
/////////////////////////
CProcessor::CProcessor()
{
	uqwFrequency = 0;
	strCPUName[0] = 0;
	memset(&cpu_info, 0, sizeof(cpu_info));
}

// unsigned __int64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
// =========================================================================
// Function to query the current CPU frequency
////////////////////////////////////////////////////////////////////////////
F64 CProcessor::GetCPUFrequency(unsigned int /*uiMeasureMSecs*/)
{
	if(uqwFrequency == 0){
		Getcpu_info();
	}

	return uqwFrequency;
}

// const ProcessorInfo *CProcessor::Getcpu_info()
// =============================================
// Calls all the other detection function to create an detailed
// processor information
///////////////////////////////////////////////////////////////
const ProcessorInfo *CProcessor::Getcpu_info()
{
					// In Solaris the CPU info is in the kstats
					// try "psrinfo" or "kstat cpu_info" to see all
					// that"s available
	int ncpus=0, i; 
	kstat_ctl_t	*kc;
	kstat_t 	*ks;
	kstat_named_t   *ksinfo, *ksi;
	kstat_t 	*CPU_stats_list;

	kc = kstat_open();

	if((int)kc == -1){
		llwarns << "kstat_open(0 failed!" << llendl;
		return (&cpu_info);
	}

	for (ks = kc->kc_chain; ks != NULL; ks = ks->ks_next) {
		if (strncmp(ks->ks_module, "cpu_info", 8) == 0 &&
			strncmp(ks->ks_name, "cpu_info", 8) == 0)
			ncpus++;
	}
	
	if(ncpus < 1){
		llwarns << "No cpus found in kstats!" << llendl;
		return (&cpu_info);
	}

	for (ks = kc->kc_chain; ks; ks = ks->ks_next) {
		if (strncmp(ks->ks_module, "cpu_info", 8) == 0 
		&&  strncmp(ks->ks_name, "cpu_info", 8) == 0 
		&&  kstat_read(kc, ks, NULL) != -1){     
			CPU_stats_list = ks;	// only looking at the first CPU
			
			break;
		}
	}

	if(ncpus > 1)
        	snprintf(strCPUName, sizeof(strCPUName), "%d x ", ncpus); 

	kstat_read(kc, CPU_stats_list, NULL);
	ksinfo = (kstat_named_t *)CPU_stats_list->ks_data;
	for(i=0; i < (int)(CPU_stats_list->ks_ndata); ++i){ // Walk the kstats for this cpu gathering what we need
		ksi = ksinfo++;
		if(!strcmp(ksi->name, "brand")){
			strncat(strCPUName, (char *)KSTAT_NAMED_STR_PTR(ksi),
				sizeof(strCPUName)-strlen(strCPUName)-1);
			strncat(cpu_info.strFamily, (char *)KSTAT_NAMED_STR_PTR(ksi),
				sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);
			strncpy(cpu_info.strBrandID, strCPUName,sizeof(cpu_info.strBrandID)-1);
			cpu_info.strBrandID[sizeof(cpu_info.strBrandID)-1]="\0";
			// DEBUG llinfos << "CPU brand: " << strCPUName << llendl;
			continue;
		}

		if(!strcmp(ksi->name, "clock_MHz")){
#if defined(__sparc)
			llinfos << "Raw kstat clock rate is: " << ksi->value.l << llendl;
			uqwFrequency = (F64)(ksi->value.l * 1000000);
#else
			uqwFrequency = (F64)(ksi->value.i64 * 1000000);
#endif
			//DEBUG llinfos << "CPU frequency: " << uqwFrequency << llendl;
			continue;
		}

#if defined(__i386)
		if(!strcmp(ksi->name, "vendor_id")){
			strncpy(cpu_info.strVendor, (char *)KSTAT_NAMED_STR_PTR(ksi), sizeof(cpu_info.strVendor)-1);
			// DEBUG llinfos << "CPU vendor: " << cpu_info.strVendor << llendl;
			continue;
		}
#endif
	}

	kstat_close(kc);

#if defined(__sparc)		// SPARC does not define a vendor string in kstat
	strncpy(cpu_info.strVendor, "Sun Microsystems, Inc.", sizeof(cpu_info.strVendor)-1);
#endif

	// DEBUG llinfo << "The system has " << ncpus << " CPUs with a clock rate of " <<  uqwFrequency << "MHz." << llendl;
	
#if defined (__i386)  //  we really don"t care about the CPU extensions on SPARC but on x86...

	// Now get cpu extensions

	uint_t ui;

	(void) getisax(&ui, 1);
	
	if(ui & AV_386_FPU)
		cpu_info._Ext.FPU_FloatingPointUnit = true;
	if(ui & AV_386_CX8)
		cpu_info._Ext.CX8_COMPXCHG8B_Instruction = true;
	if(ui & AV_386_MMX)
		cpu_info._Ext.MMX_MultimediaExtensions = true;
	if(ui & AV_386_AMD_MMX)
		cpu_info._Ext.MMX_MultimediaExtensions = true;
	if(ui & AV_386_FXSR)
		cpu_info._Ext.FXSR_FastStreamingSIMD_ExtensionsSaveRestore = true;
	if(ui & AV_386_SSE)
		 cpu_info._Ext.SSE_StreamingSIMD_Extensions = true;
	if(ui & AV_386_SSE2)
		cpu_info._Ext.SSE2_StreamingSIMD2_Extensions = true;
/* Left these here since they may get used later
	if(ui & AV_386_SSE3)
		cpu_info._Ext.... = true;
	if(ui & AV_386_AMD_3DNow)
		cpu_info._Ext.... = true;
	if(ui & AV_386_AMD_3DNowx)
		cpu_info._Ext.... = true;
*/
#endif
	return (&cpu_info);
}

#else
// LL_DARWIN

#include <mach/machine.h>
#include <sys/sysctl.h>

static char *TranslateAssociativeWays(unsigned int uiWays, char *buf)
{
	// We define 0xFFFFFFFF (= -1) as fully associative
    if (uiWays == ((unsigned int) -1))
		strcpy(buf, "fully associative");	/* Flawfinder: ignore */
	else
	{
		if (uiWays == 1)			// A one way associative cache is just direct mapped
			strcpy(buf, "direct mapped");	/* Flawfinder: ignore */
		else if (uiWays == 0)		// This should not happen...
			strcpy(buf, "unknown associative ways");	/* Flawfinder: ignore */
		else						// The x-way associative cache
			sprintf(buf, "%d ways associative", uiWays);	/* Flawfinder: ignore */
	}
	// To ease the function use we return the buffer
	return buf;
}
static void TranslateTLB(ProcessorTLB *tlb)
{
	char buf[64];	/* Flawfinder: ignore */

	// We just check if the TLB is present
	if (tlb->bPresent)
        snprintf(tlb->strTLB, sizeof(tlb->strTLB), "%s page size, %s, %d entries", tlb->strPageSize, TranslateAssociativeWays(tlb->uiAssociativeWays, buf), tlb->uiEntries);	/* Flawfinder: ignore */
	else
        strcpy(tlb->strTLB, "Not present");	/* Flawfinder: ignore */
}
static void TranslateCache(ProcessorCache *cache)
{
	char buf[64];	/* Flawfinder: ignore */

	// We just check if the cache is present
    if (cache->bPresent)
	{
		// If present we construct the string
		snprintf(cache->strCache,sizeof(cache->strCache), "%s cache size, %s, %d bytes line size", cache->strSize, TranslateAssociativeWays(cache->uiAssociativeWays, buf), cache->uiLineSize);	/* Flawfinder: ignore */
		if (cache->bSectored)
			strncat(cache->strCache, ", sectored", sizeof(cache->strCache)-strlen(cache->strCache)-1);	/* Flawfinder: ignore */
	}
	else
	{
		// Else we just say "Not present"
		strcpy(cache->strCache, "Not present");	/* Flawfinder: ignore */
	}
}

// void CProcessor::TranslateProcessorConfiguration()
// ==================================================
// Private class function to translate the processor configuration values
// to strings
/////////////////////////////////////////////////////////////////////////
void CProcessor::TranslateProcessorConfiguration()
{
	// We just call the small functions defined above
	TranslateTLB(&cpu_info._Data);
	TranslateTLB(&cpu_info._Instruction);

	TranslateCache(&cpu_info._Trace);

	TranslateCache(&cpu_info._L1.Instruction);
	TranslateCache(&cpu_info._L1.Data);
	TranslateCache(&cpu_info._L2);
	TranslateCache(&cpu_info._L3);
}

// CProcessor::CProcessor
// ======================
// Class constructor:
/////////////////////////
CProcessor::CProcessor()
{
	uqwFrequency = 0;
	strCPUName[0] = 0;
	memset(&cpu_info, 0, sizeof(cpu_info));
}

// unsigned __int64 CProcessor::GetCPUFrequency(unsigned int uiMeasureMSecs)
// =========================================================================
// Function to query the current CPU frequency
////////////////////////////////////////////////////////////////////////////
F64 CProcessor::GetCPUFrequency(unsigned int /*uiMeasureMSecs*/)
{
	U64 frequency = 0;
	size_t len = sizeof(frequency);
	
	if(sysctlbyname("hw.cpufrequency", &frequency, &len, NULL, 0) == 0)
	{
		uqwFrequency = (F64)frequency;
	}
	
	return uqwFrequency;
}

static bool hasFeature(const char *name)
{
	bool result = false;
	int val = 0;
	size_t len = sizeof(val);
	
	if(sysctlbyname(name, &val, &len, NULL, 0) == 0)
	{
		if(val != 0)
			result = true;
	}
	
	return result;
}

// const ProcessorInfo *CProcessor::Getcpu_info()
// =============================================
// Calls all the other detection function to create an detailed
// processor information
///////////////////////////////////////////////////////////////
const ProcessorInfo *CProcessor::Getcpu_info()
{
	int pagesize = 0;
	int cachelinesize = 0;
	int l1icachesize = 0;
	int l1dcachesize = 0;
	int l2settings = 0;
	int l2cachesize = 0;
	int l3settings = 0;
	int l3cachesize = 0;
	int ncpu = 0;
	int cpusubtype = 0;
	
	// sysctl knows all.
	int mib[2];
	size_t len;
	mib[0] = CTL_HW;
	
	mib[1] = HW_PAGESIZE;
	len = sizeof(pagesize);
	sysctl(mib, 2, &pagesize, &len, NULL, 0);

	mib[1] = HW_CACHELINE;
	len = sizeof(cachelinesize);
	sysctl(mib, 2, &cachelinesize, &len, NULL, 0);
	
	mib[1] = HW_L1ICACHESIZE;
	len = sizeof(l1icachesize);
	sysctl(mib, 2, &l1icachesize, &len, NULL, 0);
	
	mib[1] = HW_L1DCACHESIZE;
	len = sizeof(l1dcachesize);
	sysctl(mib, 2, &l1dcachesize, &len, NULL, 0);
	
	mib[1] = HW_L2SETTINGS;
	len = sizeof(l2settings);
	sysctl(mib, 2, &l2settings, &len, NULL, 0);
	
	mib[1] = HW_L2CACHESIZE;
	len = sizeof(l2cachesize);
	sysctl(mib, 2, &l2cachesize, &len, NULL, 0);
	
	mib[1] = HW_L3SETTINGS;
	len = sizeof(l3settings);
	sysctl(mib, 2, &l3settings, &len, NULL, 0);
	
	mib[1] = HW_L3CACHESIZE;
	len = sizeof(l3cachesize);
	sysctl(mib, 2, &l3cachesize, &len, NULL, 0);
	
	mib[1] = HW_NCPU;
	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);
	
	sysctlbyname("hw.cpusubtype", &cpusubtype, &len, NULL, 0);
	
	strCPUName[0] = 0;
	
	if((ncpu == 0) || (ncpu == 1))
	{
		// Uhhh...
	}
	else if(ncpu == 2)
	{
		strncat(strCPUName, "Dual ", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
	}
	else
	{
		snprintf(strCPUName, sizeof(strCPUName), "%d x ", ncpu);	/* Flawfinder: ignore */
	}
	
#if __ppc__
	switch(cpusubtype)
	{
		case CPU_SUBTYPE_POWERPC_601://         ((cpu_subtype_t) 1)
			strncat(strCPUName, "PowerPC 601", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
			
		break;
		case CPU_SUBTYPE_POWERPC_602://         ((cpu_subtype_t) 2)
			strncat(strCPUName, "PowerPC 602", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_603://         ((cpu_subtype_t) 3)
			strncat(strCPUName, "PowerPC 603", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_603e://        ((cpu_subtype_t) 4)
			strncat(strCPUName, "PowerPC 603e", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_603ev://       ((cpu_subtype_t) 5)
			strncat(strCPUName, "PowerPC 603ev", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_604://         ((cpu_subtype_t) 6)
			strncat(strCPUName, "PowerPC 604", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_604e://        ((cpu_subtype_t) 7)
			strncat(strCPUName, "PowerPC 604e", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_620://			((cpu_subtype_t) 8)
			strncat(strCPUName, "PowerPC 620", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_750://         ((cpu_subtype_t) 9)
			strncat(strCPUName, "PowerPC 750", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC G3", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_7400://        ((cpu_subtype_t) 10)
			strncat(strCPUName, "PowerPC 7400", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC G4", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_7450://        ((cpu_subtype_t) 11)
			strncat(strCPUName, "PowerPC 7450", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC G4", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;
		case CPU_SUBTYPE_POWERPC_970://         ((cpu_subtype_t) 100)
			strncat(strCPUName, "PowerPC 970", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
			strncat(cpu_info.strFamily, "PowerPC G5", sizeof(cpu_info.strFamily)-strlen(cpu_info.strFamily)-1);	/* Flawfinder: ignore */
		break;

		default:
			strncat(strCPUName, "PowerPC (Unknown)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */
		break;
	}

	cpu_info._Ext.EMMX_MultimediaExtensions = 
	cpu_info._Ext.MMX_MultimediaExtensions = 
	cpu_info._Ext.SSE_StreamingSIMD_Extensions =
	cpu_info._Ext.SSE2_StreamingSIMD2_Extensions = false;

	cpu_info._Ext.Altivec_Extensions = hasFeature("hw.optional.altivec");

#endif

#if __i386__
	// MBW -- XXX -- TODO -- make this call AnalyzeIntelProcessor()?
	switch(cpusubtype)
	{
		default:
			strncat(strCPUName, "i386 (Unknown)", sizeof(strCPUName)-strlen(strCPUName)-1);	/* Flawfinder: ignore */	
		break;
	}

	cpu_info._Ext.EMMX_MultimediaExtensions = hasFeature("hw.optional.mmx");  // MBW -- XXX -- this may be wrong...
	cpu_info._Ext.MMX_MultimediaExtensions = hasFeature("hw.optional.mmx");
	cpu_info._Ext.SSE_StreamingSIMD_Extensions = hasFeature("hw.optional.sse");
	cpu_info._Ext.SSE2_StreamingSIMD2_Extensions = hasFeature("hw.optional.sse2");
	cpu_info._Ext.Altivec_Extensions = false;
	cpu_info._Ext.AA64_AMD64BitArchitecture = hasFeature("hw.optional.x86_64");

#endif

	// Terse CPU info uses this string...
	strncpy(cpu_info.strBrandID, strCPUName,sizeof(cpu_info.strBrandID)-1);	/* Flawfinder: ignore */	
	cpu_info.strBrandID[sizeof(cpu_info.strBrandID)-1]="\0";
	
	// Fun cache config stuff...
	
	if(l1dcachesize != 0)
	{
		cpu_info._L1.Data.bPresent = true;
		snprintf(cpu_info._L1.Data.strSize, sizeof(cpu_info._L1.Data.strSize), "%d KB", l1dcachesize / 1024);	/* Flawfinder: ignore */
//		cpu_info._L1.Data.uiAssociativeWays = ???;
		cpu_info._L1.Data.uiLineSize = cachelinesize;
	}

	if(l1icachesize != 0)
	{
		cpu_info._L1.Instruction.bPresent = true;
		snprintf(cpu_info._L1.Instruction.strSize, sizeof(cpu_info._L1.Instruction.strSize), "%d KB", l1icachesize / 1024);	/* Flawfinder: ignore */
//		cpu_info._L1.Instruction.uiAssociativeWays = ???;
		cpu_info._L1.Instruction.uiLineSize = cachelinesize;
	}

	if(l2cachesize != 0)
	{
		cpu_info._L2.bPresent = true;
		snprintf(cpu_info._L2.strSize, sizeof(cpu_info._L2.strSize), "%d KB", l2cachesize / 1024);	/* Flawfinder: ignore */
//		cpu_info._L2.uiAssociativeWays = ???;
		cpu_info._L2.uiLineSize = cachelinesize;
	}

	if(l3cachesize != 0)
	{
		cpu_info._L2.bPresent = true;
		snprintf(cpu_info._L2.strSize, sizeof(cpu_info._L2.strSize), "%d KB", l3cachesize / 1024);	/* Flawfinder: ignore */
//		cpu_info._L2.uiAssociativeWays = ???;
		cpu_info._L2.uiLineSize = cachelinesize;
	}
	
	cpu_info._Ext.FPU_FloatingPointUnit = hasFeature("hw.optional.floatingpoint");

//	printf("pagesize = 0x%x\n", pagesize);
//	printf("cachelinesize = 0x%x\n", cachelinesize);
//	printf("l1icachesize = 0x%x\n", l1icachesize);
//	printf("l1dcachesize = 0x%x\n", l1dcachesize);
//	printf("l2settings = 0x%x\n", l2settings);
//	printf("l2cachesize = 0x%x\n", l2cachesize);
//	printf("l3settings = 0x%x\n", l3settings);
//	printf("l3cachesize = 0x%x\n", l3cachesize);
	
	// After reading we translate the configuration to strings
	TranslateProcessorConfiguration();

	// After all we return the class cpu_info member var
	return (&cpu_info);
}

#endif // LL_DARWIN

// bool CProcessor::cpu_infoToText(char *strBuffer, unsigned int uiMaxLen)
// ======================================================================
// Gets the frequency and processor information and writes it to a string
/////////////////////////////////////////////////////////////////////////
bool CProcessor::cpu_infoToText(char *strBuffer, unsigned int uiMaxLen)
{
#define LENCHECK                len = (unsigned int) strlen(buf); if (len >= uiMaxLen) return false; strcpy(strBuffer, buf); strBuffer += len;     /*Flawfinder: ignore*/
#define COPYADD(str)            strcpy(buf, str); LENCHECK;    /* Flawfinder: ignore */
#define FORMATADD(format, var)  sprintf(buf, format, var); LENCHECK;    /* Flawfinder: ignore */
#define BOOLADD(str, boolvar)   COPYADD(str); if (boolvar) { COPYADD(" Yes\n"); } else { COPYADD(" No\n"); }

	char buf[1024];	/* Flawfinder: ignore */	
	unsigned int len;

	// First we have to get the frequency
    GetCPUFrequency(50);

	// Then we get the processor information
	Getcpu_info();

    // Now we construct the string (see the macros at function beginning)
	strBuffer[0] = 0;

	COPYADD("// CPU General Information\n//////////////////////////\n");
	FORMATADD("Processor name:   %s\n", strCPUName);
	FORMATADD("Frequency:        %.2f MHz\n\n", (float) uqwFrequency / 1000000.0f);
	FORMATADD("Vendor:           %s\n", cpu_info.strVendor);
	FORMATADD("Family:           %s\n", cpu_info.strFamily);
	FORMATADD("Extended family:  %d\n", cpu_info.uiExtendedFamily);
	FORMATADD("Model:            %s\n", cpu_info.strModel);
	FORMATADD("Extended model:   %d\n", cpu_info.uiExtendedModel);
	FORMATADD("Type:             %s\n", cpu_info.strType);
	FORMATADD("Brand ID:         %s\n", cpu_info.strBrandID);
	if (cpu_info._Ext.PN_ProcessorSerialNumber)
	{
		FORMATADD("Processor Serial: %s\n", cpu_info.strProcessorSerial);
	}
	else
	{
	  COPYADD("Processor Serial: Disabled\n");
	}
#if !LL_SOLARIS		//  NOTE: Why bother printing all this when it"s irrelavent

	COPYADD("\n\n// CPU Configuration\n////////////////////\n");
	FORMATADD("L1 instruction cache:           %s\n", cpu_info._L1.Instruction.strCache);
	FORMATADD("L1 data cache:                  %s\n", cpu_info._L1.Data.strCache);
	FORMATADD("L2 cache:                       %s\n", cpu_info._L2.strCache);
	FORMATADD("L3 cache:                       %s\n", cpu_info._L3.strCache);
	FORMATADD("Trace cache:                    %s\n", cpu_info._Trace.strCache);
	FORMATADD("Instruction TLB:                %s\n", cpu_info._Instruction.strTLB);
	FORMATADD("Data TLB:                       %s\n", cpu_info._Data.strTLB);
	FORMATADD("Max Supported CPUID-Level:      0x%08lX\n", cpu_info.MaxSupportedLevel);
	FORMATADD("Max Supported Ext. CPUID-Level: 0x%08lX\n", cpu_info.MaxSupportedExtendedLevel);

	COPYADD("\n\n// CPU Extensions\n/////////////////\n");
	BOOLADD("AA64   AMD 64-bit Architecture:                    ", cpu_info._Ext.AA64_AMD64BitArchitecture);
	BOOLADD("ACPI   Thermal Monitor And Clock Control:          ", cpu_info._Ext.ACPI_ThermalMonitorAndClockControl);
	BOOLADD("APIC   Advanced Programmable Interrupt Controller: ", cpu_info._Ext.APIC_AdvancedProgrammableInterruptController);
	FORMATADD("       APIC-ID:                                     %d\n", cpu_info._Ext.APIC_ID);
	BOOLADD("CLFSH  CLFLUSH Instruction Presence:               ", cpu_info._Ext.CLFSH_CFLUSH_Instruction);
	FORMATADD("       CLFLUSH Instruction Cache Line Size:         %d\n", cpu_info._Ext.CLFLUSH_InstructionCacheLineSize);
	BOOLADD("CMOV   Conditional Move And Compare Instructions:  ", cpu_info._Ext.CMOV_ConditionalMoveAndCompareInstructions);
	BOOLADD("CX8    COMPXCHG8B Instruction:                     ", cpu_info._Ext.CX8_COMPXCHG8B_Instruction);
	BOOLADD("DE     Debugging Extensions:                       ", cpu_info._Ext.DE_DebuggingExtensions);
	BOOLADD("DS     Debug Store:                                ", cpu_info._Ext.DS_DebugStore);
	BOOLADD("FGPAT  Page Attribute Table:                       ", cpu_info._Ext.FGPAT_PageAttributeTable);
	BOOLADD("FPU    Floating Point Unit:                        ", cpu_info._Ext.FPU_FloatingPointUnit);
	BOOLADD("FXSR   Fast Streaming SIMD Extensions Save/Restore:", cpu_info._Ext.FXSR_FastStreamingSIMD_ExtensionsSaveRestore);
	BOOLADD("HT     Hyper Threading:                            ", cpu_info._Ext.HT_HyperThreading);
	BOOLADD("IA64   Intel 64-Bit Architecture:                  ", cpu_info._Ext.IA64_Intel64BitArchitecture);
	BOOLADD("MCA    Machine Check Architecture:                 ", cpu_info._Ext.MCA_MachineCheckArchitecture);
	BOOLADD("MCE    Machine Check Exception:                    ", cpu_info._Ext.MCE_MachineCheckException);
	BOOLADD("MMX    Multimedia Extensions:                      ", cpu_info._Ext.MMX_MultimediaExtensions);
	BOOLADD("MMX+   Multimedia Extensions:                      ", cpu_info._Ext.EMMX_MultimediaExtensions);
	BOOLADD("MSR    Model Specific Registers:                   ", cpu_info._Ext.MSR_ModelSpecificRegisters);
	BOOLADD("MTRR   Memory Type Range Registers:                ", cpu_info._Ext.MTRR_MemoryTypeRangeRegisters);
	BOOLADD("PAE    Physical Address Extension:                 ", cpu_info._Ext.PAE_PhysicalAddressExtension);
	BOOLADD("PGE    PTE Global Flag:                            ", cpu_info._Ext.PGE_PTE_GlobalFlag);
	if (cpu_info._Ext.PN_ProcessorSerialNumber)
	{
		FORMATADD("PN     Processor Serial Number:                     %s\n", cpu_info.strProcessorSerial);
	}
	else
	{
		COPYADD("PN     Processor Serial Number:                     Disabled\n");
	}
	BOOLADD("PSE    Page Size Extensions:                       ", cpu_info._Ext.PSE_PageSizeExtensions);
	BOOLADD("PSE36  36-bit Page Size Extension:                 ", cpu_info._Ext.PSE36_36bitPageSizeExtension);
	BOOLADD("SEP    Fast System Call:                           ", cpu_info._Ext.SEP_FastSystemCall);
	BOOLADD("SS     Self Snoop:                                 ", cpu_info._Ext.SS_SelfSnoop);
	BOOLADD("SSE    Streaming SIMD Extensions:                  ", cpu_info._Ext.SSE_StreamingSIMD_Extensions);
	BOOLADD("SSE2   Streaming SIMD 2 Extensions:                ", cpu_info._Ext.SSE2_StreamingSIMD2_Extensions);
	BOOLADD("ALTVEC Altivec Extensions:                         ", cpu_info._Ext.Altivec_Extensions);
	BOOLADD("TM     Thermal Monitor:                            ", cpu_info._Ext.TM_ThermalMonitor);
	BOOLADD("TSC    Time Stamp Counter:                         ", cpu_info._Ext.TSC_TimeStampCounter);
	BOOLADD("VME    Virtual 8086 Mode Enhancements:             ", cpu_info._Ext.VME_Virtual8086ModeEnhancements);
	BOOLADD("3DNow! Instructions:                               ", cpu_info._Ext._3DNOW_InstructionExtensions);
	BOOLADD("Enhanced 3DNow! Instructions:                      ", cpu_info._Ext._E3DNOW_InstructionExtensions);
#endif
	// Yippie!!!
	return true;
}

// bool CProcessor::WriteInfoTextFile(const std::string& strFilename)
// ===========================================================
// Takes use of CProcessor::cpu_infoToText and saves the string to a
// file
///////////////////////////////////////////////////////////////////
bool CProcessor::WriteInfoTextFile(const std::string& strFilename)
{
	char buf[16384];	/* Flawfinder: ignore */	

	// First we get the string
	if (!cpu_infoToText(buf, 16383))
		return false;

	// Then we create a new file (CREATE_ALWAYS)
	LLFILE *file = LLFile::fopen(strFilename, "w");	/* Flawfinder: ignore */	
	if (!file)
		return false;

	// After that we write the string to the file
	unsigned long dwBytesToWrite, dwBytesWritten;
	dwBytesToWrite = (unsigned long) strlen(buf);	 /*Flawfinder: ignore*/
	dwBytesWritten = (unsigned long) fwrite(buf, 1, dwBytesToWrite, file);
	fclose(file);
	if (dwBytesToWrite != dwBytesWritten)
		return false;

	// Done
	return true;
}
#endif
