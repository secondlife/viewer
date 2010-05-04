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
	LLProcessorInfoWindowsImpl()
	{
		getCPUIDInfo();
		setInfo(eFrequency, calculate_cpu_frequency(50));
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
		setConfig(eMaxID, (S32)ids);

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
				setInfo(eBrandID, cpu_info[1] & 0xff);

				setInfo(eFamilyName, compute_CPUFamilyName(cpu_vendor, family, ext_family));

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
				
				if(cpu_info[2] & 0x10)
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
		for(unsigned int i=0x80000000; i<=ext_ids; ++i)
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
		uint64_t frequency = getSysctlInt64("hw.cpufrequency");
		setInfo(eFrequency, (F64)frequency);
	}

	virtual ~LLProcessorInfoDarwinImpl() {}

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

#elif LL_LINUX

class LLProcessorInfoLinuxImpl : public LLProcessorInfoImpl
{
public:
	LLProcessorInfoLinuxImpl() 
	{
		std::map< std::string, std::string > cpuinfo;
		LLFILE* cpuinfo_fp = LLFile::fopen(CPUINFO_FILE, "rb");
		if(cpuinfo_fp)
		{
			char line[MAX_STRING];
			memset(line, 0, MAX_STRING);
			while(fgets(line, MAX_STRING, cpuinfo_fp))
			{
				// /proc/cpuinfo on Linux looks like:
				// name\t*: value\n
				char* tabspot = strchr( line, '\t' );
				if (tabspot == NULL)
					continue;
				char* colspot = strchr( tabspot, ':' );
				if (colspot == NULL)
					continue;
				char* spacespot = strchr( colspot, ' ' );
				if (spacespot == NULL)
					continue;
				char* nlspot = strchr( line, '\n' );
				if (nlspot == NULL)
					nlspot = line + strlen( line ); // Fallback to terminating NUL
				std::string linename( line, tabspot );
				std::string llinename(linename);
				LLStringUtil::toLower(llinename);
				std::string lineval( spacespot + 1, nlspot );
				cpuinfo[ llinename ] = lineval;
			}
			fclose(cpuinfo_fp);
		}
# if LL_X86
		std::string flags = " " + cpuinfo["flags"] + " ";
		LLStringUtil::toLower(flags);

		if( flags.find( " sse " ) != std::string::npos )
		{
			setExtension(eSSE_Ext); 
		}

		if( flags.find( " sse2 " ) != std::string::npos )
		{
			setExtension(eSSE2_Ext);
		}
	
		F64 mhz;
		if (LLStringUtil::convertToF64(cpuinfo["cpu mhz"], mhz)
			&& 200.0 < mhz && mhz < 10000.0)
		{
		    setInfo(eFrequency,(F64)(mhz));
		}

		if (!cpuinfo["model name"].empty())
			mCPUString = cpuinfo["model name"];
# endif // LL_X86
	}

	virtual ~LLProcessorInfoLinuxImpl() {}
private:

	void getCPUIDInfo()
	{
		std::map< std::string, std::string > cpuinfo;
		LLFILE* cpuinfo_fp = LLFile::fopen(CPUINFO_FILE, "rb");
		if(cpuinfo_fp)
		{
			char line[MAX_STRING];
			memset(line, 0, MAX_STRING);
			while(fgets(line, MAX_STRING, cpuinfo_fp))
			{
				// /proc/cpuinfo on Linux looks like:
				// name\t*: value\n
				char* tabspot = strchr( line, '\t' );
				if (tabspot == NULL)
					continue;
				char* colspot = strchr( tabspot, ':' );
				if (colspot == NULL)
					continue;
				char* spacespot = strchr( colspot, ' ' );
				if (spacespot == NULL)
					continue;
				char* nlspot = strchr( line, '\n' );
				if (nlspot == NULL)
					nlspot = line + strlen( line ); // Fallback to terminating NUL
				std::string linename( line, tabspot );
				std::string llinename(linename);
				LLStringUtil::toLower(llinename);
				std::string lineval( spacespot + 1, nlspot );
				cpuinfo[ llinename ] = lineval;
			}
			fclose(cpuinfo_fp);
		}
# if LL_X86

// *NOTE:Mani - eww, macros! srry.
#define LLPI_SET_INFO_STRING(llpi_id, cpuinfo_id) \
		if (!cpuinfo[cpuinfo_id].empty()) { setInfo(llpi_id, cpuinfo[cpuinfo_id]);}

#define LLPI_SET_INFO_INT(llpi_id, cpuinfo_id) \
		if (!cpuinfo[cpuinfo_id].empty()) { setInfo(llpi_id, LLStringUtil::convertToS32(cpuinfo[cpuinfo_id]));}

		F64 mhz;
		if (LLStringUtil::convertToF64(cpuinfo["cpu mhz"], mhz)
			&& 200.0 < mhz && mhz < 10000.0)
		{
		    setInfo(eFrequency,(F64)(mhz));
		}

		LLPI_SET_INFO_STRING(eBrandName, "model name");		
		LLPI_SET_INFO_STRING(eVendor, "vendor_id");

		LLPI_SET_INFO_INT(eStepping, "stepping");
		LLPI_SET_INFO_INT(eModel, "model");
		int family = LLStringUtil::convertTos32getSysctlInt("machdep.cpu.family");
		int ext_family = getSysctlInt("machdep.cpu.extfamily");
		LLPI_SET_INFO_INT(eFamily, "cpu family");
		//LLPI_SET_INFO_INT(eExtendedFamily, ext_family);
		// setInfo(eFamilyName, compute_CPUFamilyName(cpu_vendor, family, ext_family));
		// setInfo(eExtendedModel, getSysctlInt("machdep.cpu.extmodel"));
		// setInfo(eBrandID, getSysctlInt("machdep.cpu.brand"));
		// setInfo(eType, 0); // ? where to find this?

		//setConfig(eCLFLUSHCacheLineSize, ((cpu_info[1] >> 8) & 0xff) * 8);
		//setConfig(eAPICPhysicalID, (cpu_info[1] >> 24) & 0xff);
		//setConfig(eCacheLineSize, getSysctlInt("machdep.cpu.cache.linesize"));
		//setConfig(eL2Associativity, getSysctlInt("machdep.cpu.cache.L2_associativity"));
		//setConfig(eCacheSizeK, getSysctlInt("machdep.cpu.cache.size"));
		
		// Read extensions
		std::string flags = " " + cpuinfo["flags"] + " ";
		LLStringUtil::toLower(flags);

		if( flags.find( " sse " ) != std::string::npos )
		{
			setExtension(eSSE_Ext); 
		}

		if( flags.find( " sse2 " ) != std::string::npos )
		{
			setExtension(eSSE2_Ext);
		}
	
# endif // LL_X86
	}
};


#endif // LL_MSVC elif LL_DARWIN elif LL_LINUX

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

