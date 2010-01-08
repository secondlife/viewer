/** 
 * @file llprocessor.h
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


#ifndef LLPROCESSOR_H
#define LLPROCESSOR_H

class LLProcessorInfo
{
public:
	LLProcessorInfo(); 
 	~LLProcessorInfo();

	F64 getCPUFrequency() const;
	bool hasSSE() const;
	bool hasSSE2() const;
	bool hasAltivec() const;
	std::string getCPUFamilyName() const;
	std::string getCPUBrandName() const;
	std::string getCPUFeatureDescription() const;
};

# if 0
// Author: Benjamin Jurke
// File history: 27.02.2002   File created.
///////////////////////////////////////////

// Options:
///////////
#if LL_WINDOWS
#define PROCESSOR_FREQUENCY_MEASURE_AVAILABLE
#endif

#if LL_MSVC && _M_X64
#      define LL_X86_64 1
#      define LL_X86 1
#elif LL_MSVC && _M_IX86
#      define LL_X86 1
#elif LL_GNUC && ( defined(__amd64__) || defined(__x86_64__) )
#      define LL_X86_64 1
#      define LL_X86 1
#elif LL_GNUC && ( defined(__i386__) )
#      define LL_X86 1
#elif LL_GNUC && ( defined(__powerpc__) || defined(__ppc__) )
#      define LL_PPC 1
#endif


struct ProcessorExtensions
{
	bool FPU_FloatingPointUnit;
	bool VME_Virtual8086ModeEnhancements;
	bool DE_DebuggingExtensions;
	bool PSE_PageSizeExtensions;
	bool TSC_TimeStampCounter;
	bool MSR_ModelSpecificRegisters;
	bool PAE_PhysicalAddressExtension;
	bool MCE_MachineCheckException;
	bool CX8_COMPXCHG8B_Instruction;
	bool APIC_AdvancedProgrammableInterruptController;
	unsigned int APIC_ID;
	bool SEP_FastSystemCall;
	bool MTRR_MemoryTypeRangeRegisters;
	bool PGE_PTE_GlobalFlag;
	bool MCA_MachineCheckArchitecture;
	bool CMOV_ConditionalMoveAndCompareInstructions;
	bool FGPAT_PageAttributeTable;
	bool PSE36_36bitPageSizeExtension;
	bool PN_ProcessorSerialNumber;
	bool CLFSH_CFLUSH_Instruction;
	unsigned int CLFLUSH_InstructionCacheLineSize;
	bool DS_DebugStore;
	bool ACPI_ThermalMonitorAndClockControl;
	bool EMMX_MultimediaExtensions;
	bool MMX_MultimediaExtensions;
	bool FXSR_FastStreamingSIMD_ExtensionsSaveRestore;
	bool SSE_StreamingSIMD_Extensions;
	bool SSE2_StreamingSIMD2_Extensions;
	bool Altivec_Extensions;
	bool SS_SelfSnoop;
	bool HT_HyperThreading;
	unsigned int HT_HyterThreadingSiblings;
	bool TM_ThermalMonitor;
	bool IA64_Intel64BitArchitecture;
	bool _3DNOW_InstructionExtensions;
	bool _E3DNOW_InstructionExtensions;
	bool AA64_AMD64BitArchitecture;
};

struct ProcessorCache
{
	bool bPresent;
	char strSize[32];	/* Flawfinder: ignore */	
	unsigned int uiAssociativeWays;
	unsigned int uiLineSize;
	bool bSectored;
	char strCache[128];	/* Flawfinder: ignore */	
};

struct ProcessorL1Cache
{
    ProcessorCache Instruction;
	ProcessorCache Data;
};

struct ProcessorTLB
{
	bool bPresent;
	char strPageSize[32];	/* Flawfinder: ignore */	
	unsigned int uiAssociativeWays;
	unsigned int uiEntries;
	char strTLB[128];	/* Flawfinder: ignore */	
};

struct ProcessorInfo
{
	char strVendor[16];	/* Flawfinder: ignore */	
	unsigned int uiFamily;
	unsigned int uiExtendedFamily;
	char strFamily[64];	/* Flawfinder: ignore */	
	unsigned int uiModel;
	unsigned int uiExtendedModel;
	char strModel[128];	/* Flawfinder: ignore */	
	unsigned int uiStepping;
	unsigned int uiType;
	char strType[64];	/* Flawfinder: ignore */	
	unsigned int uiBrandID;
	char strBrandID[64];	/* Flawfinder: ignore */	
	char strProcessorSerial[64];	/* Flawfinder: ignore */	
	unsigned long MaxSupportedLevel;
	unsigned long MaxSupportedExtendedLevel;
	ProcessorExtensions _Ext;
	ProcessorL1Cache _L1;
	ProcessorCache _L2;
	ProcessorCache _L3;
	ProcessorCache _Trace;
	ProcessorTLB _Instruction;
	ProcessorTLB _Data;
};


// CProcessor
// ==========
// Class for detecting the processor name, type and available
// extensions as long as it's speed.
/////////////////////////////////////////////////////////////
class CProcessor
{
// Constructor / Destructor:
////////////////////////////
public:
	CProcessor();

// Private vars:
////////////////
private:
	F64 uqwFrequency;
	char strCPUName[128];	/* Flawfinder: ignore */	
	ProcessorInfo CPUInfo;

// Private functions:
/////////////////////
private:
	bool AnalyzeIntelProcessor();
	bool AnalyzeAMDProcessor();
	bool AnalyzeUnknownProcessor();
	bool CheckCPUIDPresence();
	void DecodeProcessorConfiguration(unsigned int cfg);
	void TranslateProcessorConfiguration();
	void GetStandardProcessorConfiguration();
	void GetStandardProcessorExtensions();

// Public functions:
////////////////////
public:
	F64 GetCPUFrequency(unsigned int uiMeasureMSecs);
	const ProcessorInfo *GetCPUInfo();
	bool CPUInfoToText(char *strBuffer, unsigned int uiMaxLen);
	bool WriteInfoTextFile(const std::string& strFilename);
};

#endif // 0

#endif // LLPROCESSOR_H
