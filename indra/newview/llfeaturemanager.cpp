/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include <iostream>
#include <fstream>

#include "llviewerprecompiledheaders.h"

#include "llfeaturemanager.h"
#include "lldir.h"

#include "llsys.h"
#include "llgl.h"
#include "llsecondlifeurls.h"

#include "llviewercontrol.h"
#include "llworld.h"
#include "pipeline.h"
#include "lldrawpoolterrain.h"
#include "llviewerimagelist.h"
#include "llwindow.h"
#include "llui.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

//
// externs
//
extern LLMemoryInfo gSysMemory;
extern LLCPUInfo gSysCPU;
extern void write_debug(const char *str);
extern void write_debug(const std::string& str);

#if LL_DARWIN
const char FEATURE_TABLE_FILENAME[] = "featuretable_mac.txt";
#else
const char FEATURE_TABLE_FILENAME[] = "featuretable.txt";
#endif

const char GPU_TABLE_FILENAME[] = "gpu_table.txt";

LLFeatureManager *gFeatureManagerp = NULL;

LLFeatureInfo::LLFeatureInfo(const char *name, const BOOL available, const S32 level) : mValid(TRUE)
{
	mName = name;
	mAvailable = available;
	mRecommendedLevel = level;
}

LLFeatureList::LLFeatureList(const char *name)
{
	mName = name;
}

LLFeatureList::~LLFeatureList()
{
}

void LLFeatureList::addFeature(const char *name, const BOOL available, const S32 level)
{
	if (mFeatures.count(name))
	{
		llwarns << "LLFeatureList::Attempting to add preexisting feature " << name << llendl;
	}

	LLFeatureInfo fi(name, available, level);
	mFeatures[name] = fi;
}

BOOL LLFeatureList::isFeatureAvailable(const char *name)
{
	if (mFeatures.count(name))
	{
		return mFeatures[name].mAvailable;
	}

	llwarns << "Feature " << name << " not on feature list!" << llendl;
	return FALSE;
}

S32 LLFeatureList::getRecommendedLevel(const char *name)
{
	if (mFeatures.count(name))
	{
		return mFeatures[name].mRecommendedLevel;
	}

	llwarns << "Feature " << name << " not on feature list!" << llendl;
	return -1;
}

BOOL LLFeatureList::maskList(LLFeatureList &mask)
{
	//llinfos << "Masking with " << mask.mName << llendl;
	//
	// Lookup the specified feature mask, and overlay it on top of the
	// current feature mask.
	//

	LLFeatureInfo mask_fi;

	feature_map_t::iterator feature_it;
	for (feature_it = mask.mFeatures.begin(); feature_it != mask.mFeatures.end(); ++feature_it)
	{
		mask_fi = feature_it->second;
		//
		// Look for the corresponding feature
		//
		if (!mFeatures.count(mask_fi.mName))
		{
			llwarns << "Feature " << mask_fi.mName << " in mask not in top level!" << llendl;
			continue;
		}

		LLFeatureInfo &cur_fi = mFeatures[mask_fi.mName];
		if (mask_fi.mAvailable && !cur_fi.mAvailable)
		{
			llwarns << "Mask attempting to reenabling disabled feature, ignoring " << cur_fi.mName << llendl;
			continue;
		}
		cur_fi.mAvailable = mask_fi.mAvailable;
		cur_fi.mRecommendedLevel = llmin(cur_fi.mRecommendedLevel, mask_fi.mRecommendedLevel);
#ifndef LL_RELEASE_FOR_DOWNLOAD
		llinfos << "Feature mask " << mask.mName
				<< " Feature " << mask_fi.mName
				<< " Mask: " << mask_fi.mRecommendedLevel
				<< " Now: " << cur_fi.mRecommendedLevel << llendl;
#endif
	}

#if 0 && !LL_RELEASE_FOR_DOWNLOAD
	llinfos << "After appling mask " << mask.mName << llendl;
	dump();
#endif
	return TRUE;
}

void LLFeatureList::dump()
{
	llinfos << "Feature list: " << mName << llendl;
	llinfos << "--------------" << llendl;

	LLFeatureInfo fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mFeatures.begin(); feature_it != mFeatures.end(); ++feature_it)
	{
		fi = feature_it->second;
		llinfos << fi.mName << "\t\t" << fi.mAvailable << ":" << fi.mRecommendedLevel << llendl;
	}
	llinfos << llendl;
}

LLFeatureList *LLFeatureManager::findMask(const char *name)
{
	if (mMaskList.count(name))
	{
		return mMaskList[name];
	}

	return NULL;
}

BOOL LLFeatureManager::maskFeatures(const char *name)
{
	LLFeatureList *maskp = findMask(name);
	if (!maskp)
	{
		llwarns << "Unknown feature mask " << name << llendl;
		return FALSE;
	}
	llinfos << "Applying Feature Mask: " << name << llendl;
	return maskList(*maskp);
}

BOOL LLFeatureManager::loadFeatureTables()
{
	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += FEATURE_TABLE_FILENAME;


	char	name[MAX_STRING+1];

	llifstream file;
	U32		version;
	
	file.open(data_path.c_str()); 

	if (!file)
	{
		llwarns << "Unable to open feature table!" << llendl;
		return FALSE;
	}

	// Check file version
	file >> name;
	file >> version;
	if (strcmp(name, "version"))
	{
		llwarns << data_path << " does not appear to be a valid feature table!" << llendl;
		return FALSE;
	}

	mTableVersion = version;

	LLFeatureList *flp = NULL;
	while (!file.eof())
	{
		char buffer[MAX_STRING];
		name[0] = 0;

		file >> name;
		
		if (strlen(name) >= 2 && 
			name[0] == '/' && 
			name[1] == '/')
		{
			// This is a comment.
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (strlen(name) == 0)
		{
			// This is a blank line
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (!strcmp(name, "list"))
		{
			if (flp)
			{
				//flp->dump();
			}
			// It's a new mask, create it.
			file >> name;
			if (mMaskList.count(name))
			{
				llerrs << "Overriding mask " << name << ", this is invalid!" << llendl;
			}

			if (!flp)
			{
				//
				// The first one is always the default
				//
				flp = this;
			}
			else
			{
				flp = new LLFeatureList(name);
				mMaskList[name] = flp;
			}

		}
		else
		{
			if (!flp)
			{
				llerrs << "Specified parameter before <list> keyword!" << llendl;
			}
			S32 available, recommended;
			file >> available >> recommended;
			flp->addFeature(name, available, recommended);
		}
	}
	file.close();
	//flp->dump();

	return TRUE;
}

void LLFeatureManager::loadGPUClass()
{
	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += GPU_TABLE_FILENAME;

	// defaults
	mGPUClass = 0;
	mGPUString = gGLManager.getRawGLString();
	
	llifstream file;
		
	file.open(data_path.c_str()); 

	if (!file)
	{
		llwarns << "Unable to open GPU table: " << data_path << "!" << llendl;
		return;
	}

	std::string renderer = gGLManager.getRawGLString();
	for (std::string::iterator i = renderer.begin(); i != renderer.end(); ++i)
	{
		*i = tolower(*i);
	}
	
	while (!file.eof())
	{
		char buffer[MAX_STRING];
		buffer[0] = 0;

		file.getline(buffer, MAX_STRING);
		
		if (strlen(buffer) >= 2 && 
			buffer[0] == '/' && 
			buffer[1] == '/')
		{
			// This is a comment.
			continue;
		}

		if (strlen(buffer) == 0)
		{
			// This is a blank line
			continue;
		}

		char* cls, *label, *expr;
		
		label = strtok(buffer, "\t");
		expr = strtok(NULL, "\t");
		cls = strtok(NULL, "\t");

		if (label == NULL || expr == NULL || cls == NULL)
		{
			continue;
		}
	
		for (U32 i = 0; i < strlen(expr); i++)
		{
			expr[i] = tolower(expr[i]);
		}
		
		char* ex = strtok(expr, ".*");
		char* rnd = (char*) renderer.c_str();
		
		while (ex != NULL && rnd != NULL)
		{
			rnd = strstr(rnd, ex);
			ex = strtok(NULL, ".*");
		}
		
		if (rnd != NULL)
		{
			file.close();
			llinfos << "GPU is " << label << llendl;
			mGPUString = label;
			mGPUClass = (S32) strtol(cls, NULL, 10);	
		}
	}
	file.close();
	//flp->dump();

	llwarns << "Couldn't match GPU to a class: " << gGLManager.getRawGLString() << llendl;
}

void LLFeatureManager::cleanupFeatureTables()
{
	std::for_each(mMaskList.begin(), mMaskList.end(), DeletePairedPointer());
	mMaskList.clear();
}


void LLFeatureManager::initCPUFeatureMasks()
{
	if (gSysMemory.getPhysicalMemory() <= 256*1024*1024)
	{
		maskFeatures("RAM256MB");
	}
	else if (gSysMemory.getPhysicalMemory() <= 512*1024*1024)
	{
		//maskFeatures("RAM512MB");
	}

	if (gSysCPU.getMhz() < 1100)
	{
		maskFeatures("CPUSlow");
	}
	if (isSafe())
	{
		maskFeatures("safe");
	}
}

void LLFeatureManager::initGraphicsFeatureMasks()
{
	loadGPUClass();
	
	if (mGPUClass >= 0 && mGPUClass < 4)
	{
		const char* class_table[] =
		{
			"Class0",
			"Class1",
			"Class2",
			"Class3"
		};

		llinfos << "Setting GPU Class to " << class_table[mGPUClass] << llendl;
		maskFeatures(class_table[mGPUClass]);
	}
	
	if (!gGLManager.mHasFragmentShader)
	{
		maskFeatures("NoPixelShaders");
	}
	if (!gGLManager.mHasVertexShader)
	{
		maskFeatures("NoVertexShaders");
	}
	if (gGLManager.mIsNVIDIA)
	{
		maskFeatures("NVIDIA");
	}
	if (gGLManager.mIsGF2or4MX)
	{
		maskFeatures("GeForce2");
	}
	if (gGLManager.mIsATI)
	{
		maskFeatures("ATI");
	}
	if (gGLManager.mIsRadeon8500)
	{
		maskFeatures("Radeon8500");
	}
	if (gGLManager.mIsRadeon9700)
	{
		maskFeatures("Radeon9700");
	}
	if (gGLManager.mIsGFFX)
	{
		maskFeatures("GeForceFX");
	}
	if (gGLManager.mIsIntel)
	{
		maskFeatures("Brookdale");
	}

	if (gGLManager.mIsMobilityRadeon9000)
	{
		maskFeatures("MobilityRadeon9000");
	}
	if (isSafe())
	{
		maskFeatures("safe");
	}
}

extern LLOSInfo gSysOS;


BOOL bad_hardware_dialog(const LLString &info_str, const LLString &url)
{
	if (!gSavedSettings.getWarning("AboutBadPCI"))
	{
		return FALSE;
	}

	// XUI:translate
	std::string msg = llformat(
			"[SECOND_LIFE] has detected that there may be a problem with.\n"
			"hardware or drivers on your computer.  Often resolving these\n"
			"issues can result in enhanced stability and performance.\n"
			" \n"
			"%s\n"
			" \n"
			"Would you like to view a web page with more detailed\n"
			"information on this problem?\n", info_str.c_str());

	// Warn them that runnin without DirectX 9 will
	// not allow us to tell them about driver issues
	S32 button = OSMessageBox(msg.c_str(),
							  "Warning",
							  OSMB_YESNO);
	if (OSBTN_YES== button)
	{
		llinfos << "User quitting after detecting bad drivers" << llendl;
		spawn_web_browser(url.c_str());
		return TRUE;
	}
	else
	{
		// Don't warn about bad PCI stuff again, they've clicked past it.
		gSavedSettings.setWarning("AboutBadPCI", FALSE);
	}
	return FALSE;
}

BOOL LLFeatureManager::initPCIFeatureMasks()
{
#if LL_WINDOWS
	BOOL exit_after_bad = FALSE;

	BOOL is_2000 = FALSE;
	BOOL is_xp = FALSE;

	if (gSysOS.mMajorVer != 5)
	{
		// Unknown windows version number, exit!"
		llwarns << "Unknown Windows major version " << gSysOS.mMajorVer << ", aborting detection!" << llendl;
		return FALSE;
	}
	if (gSysOS.mMinorVer == 0)
	{
		is_2000 = TRUE;
	}
	else if (gSysOS.mMinorVer == 1)
	{
		is_xp = TRUE;
	}
	else
	{
		llwarns << "Unknown Windows minor version " << gSysOS.mMinorVer << ", aborting detection!" << llendl;
		return FALSE;
	}

	// This only works on Win32, as it relies on DX9 hardware detection
	// The PCI masks are actually the inverse of the normal masks
	// We actually look through the masks,and see if any hardware matches it.
	// This is because the masks encode logic about

	// Check for the broken AMD AGP controllers (751, 761, 762)

	// Horrible cruddy fixed lookup table.
	// Figure out what OS we're on, the version numbers are different.  Sigh...

	LLDXDriverFile *dfilep = NULL;
	LLDXDevice *devp = NULL;

	// AMD(1022) AGP controllers
	// 7007		AMD-751 AGP Controller
	// 700F		AMD-761 AGP Controller
	// 700D		AMD-762 AGP Controller
	devp = gDXHardware.findDevice("VEN_1022", "DEV_7007|DEV_700F|DEV_700D");
	if (devp)
	{
		// We're just pretty much screwed here, there are big problems with this hardware
		// We've got trouble with the most recent nVidia drivers.  Check for this and warn.

		// Note:  Need to detect that we're running with older nVidia hardware, probably
		exit_after_bad |= bad_hardware_dialog("AMD AGP Controller",
											  AMD_AGP_URL);
	}

	// VIA(1106) AGP Controllers
	// These need upgrading on both Win2K and WinXP
	//
	// 8305		VT8363/8365			CPU to AGP - Apollo KT133/KM133
	// 8598		VT82C598MVP/694X	CPU to AGP - Apollo MVP3/Pro133A
	// 8605		VT8605				CPU to AGP - Apollo PM133
	// B091		VT8633				CPU to AGP - Apollo Pro 266
	// B099		VT8366/A/T			CPU to AGP - Apollo KT266/A/333
	// B168		VT8235				CPU to AGP (AGP 2.0/3.0) - ProSavageDDR P4X333 chipset
	// B188		VT8237				CPU to AGP (AGP 2.0/3.0) - K8T800
	// B198		VT8237				CPU to AGP (AGP 2.0/3.0) - ProSavageDDR P4X600 chipset

	devp = gDXHardware.findDevice("VEN_1106",
								  "DEV_8305|DEV_8598|DEV_8605|DEV_B091|"
								  "DEV_B099|DEV_B168|DEV_B188|DEV_B198");
	if (devp)
	{
		BOOL out_of_date = FALSE;
		// Wanted driver: VIAAGP1.SYS
		// Version Format: M.mm.0000.vvvv
		//		M.mm - Major/minor OS version (5.0 for Win2000, 5.1 for WinXP)
		//		vvvv - driver version number
		//
		// Notes:
		// 3442 is most recent as of 2/25/04, probably want at least 3430 (seems to be a common version)

		// These are DELIBERATE assignments inside if statements, blech.
		if (dfilep = devp->findDriver("pci.sys"))
		{
			// Old driver: pci.sys
			// Version: 5.01.2600.xxxx
			//
			// Notes:
			// Default WinXP driver for B168, B198?

			// Old driver: pci.sys
			// Version: 5.01.2195.xxxx
			//
			// Notes:
			// Default Win2K driver for 8305?

			llwarns << "Detected pci.sys" << llendl;
			write_debug("Old driver (pci.sys) for VIA detected!");
			out_of_date = TRUE;
		}
		else if (dfilep = devp->findDriver("VIAAGP.SYS"))
		{
			// Old driver: VIAAGP.SYS
			// Version: 5.01.2600.xxxx
			//
			// Notes:
			// Default WinXP driver for B09x?

			llwarns << "Detected VIAAGP.SYS" << llendl;
			write_debug("Old driver (VIAAGP.SYS) for VIA detected!");
			out_of_date = TRUE;
		}
		else if (dfilep = devp->findDriver("VIAAGP1.SYS"))
		{
			if (dfilep->mVersion.getField(3) < 3430)
			{
				// They're using a pretty old version of the VIA AGP drivers
				// Maybe they want to upgrade?
			    llwarns << "Detected VIAAGP1.SYS" << llendl;
			    write_debug("Old driver (VIAAGP1.SYS) for VIA detected!");
				out_of_date = TRUE;
			}
		}
		if (out_of_date)
		{
			exit_after_bad |= bad_hardware_dialog("Out of date VIA AGP chipset driver",
												  VIA_URL);
		}
	}

	// Intel(8086) AGP controllers (Win2K)
	// These particular controllers only may need drivers on Win2K
	//
	// 1A31		82845[MP|MZ] Processor to AGP Controller
	// 2532		82850/860 Processor to AGP Controller
	if (is_2000)
	{
		devp = gDXHardware.findDevice("VEN_8086",
									  "DEV_1A31");
		if (devp)
		{
			if (dfilep = devp->findDriver("pci.sys"))
			{
				// Old driver: pci.sys
				// Version 5.01.21[9|6]5.xxxx
				//
				// Notes:
				// Default driver for Win2K?  Not sure what the "correct" driver is -
				// maybe some variant of AGP440.SYS?
				llwarns << "Detected pci.sys" << llendl;
				write_debug("Old driver (pci.sys) for Intel 82845/850 on Win2K detected!");
				exit_after_bad |= bad_hardware_dialog("Out of date Intel chipset driver",
													  INTEL_CHIPSET_URL);
			}
		}
	}

	/* Removed 4/3/2006 by JC
	   After talking with Doug, we don't know what the proper driver
	   and/or version number should be for Intel 865.  Regardless, this
	   code would _always_ complain if you had that chipset.

	// Intel(8086) AGP controllers (All)
	// These particular controllers may need drivers on both Win2K and WinXP
	//
	// 2561		82845G/GL/GE/PE/GV Processor to AGP Controller
	// 2571		82865G/PE/P/GV/28248P Processor to AGP Controller
	devp = gDXHardware.findDevice("VEN_8086",
								  "DEV_2571");
	if (devp)
	{
		// Wanted driver: AGP440.SYS(?)
		//
		// Notes:
		// Not sure, need to verify with an actual 82865/75 (Dell 8300?)
		
		// Old driver: pci.sys
		// Version 5.01.21[9|6]5.xxxx
		//
		// Notes:
		// Default driver for Win2K?  Not sure what the "correct" driver is -
		// maybe some variant of AGP440.SYS?
		exit_after_bad |= bad_hardware_dialog("Out of date Intel chipset driver",
											  INTEL_CHIPSET_URL);
	}
	*/


	// SiS(1039) AGP controllers (All)
	// These particular controllers may need drivers on both Win2K and WinXP
	//
	// 0001		SiS 530
	// 0002		SiS SG86C202(???)
	// 0003		SiS 648FX
	devp = gDXHardware.findDevice("VEN_1039",
								  "DEV_0001|DEV_0002|DEV_0003");
	if (devp)
	{
		BOOL out_of_date = FALSE;
		// Wanted driver: SISAGPX.SYS
		//
		// Notes:
		// Not sure, need to verify with an actual 82865/75 (Dell 8300?)
		
		// Old driver: pci.sys
		// Version 5.01.21[9|6]5.xxxx
		//
		// Notes:
		// Default driver for Win2K?  Not sure what the "correct" driver is -
		// maybe some variant of AGP440.SYS?
		if (dfilep = devp->findDriver("pci.sys"))
		{
			// Old driver: pci.sys
			// Version 5.01.21[9|6]5.xxxx
			//
			llwarns << "Detected pci.sys" << llendl;
			write_debug("Old driver (pci.sys) for SiS detected!");
			out_of_date = TRUE;
		}

		if (dfilep = devp->findDriver("sisagp.sys"))
		{
			// Old driver: pci.sys
			// Version 5.01.21[9|6]5.xxxx
			//
			llwarns << "Detected sisagp.sys" << llendl;
			write_debug("Old driver (sisagp.sys) for SiS detected!");
			out_of_date = TRUE;
		}

		if (dfilep = devp->findDriver("sisagpx.sys"))
		{
			// Old driver: pci.sys
			// Version 7.02.0000.xxxx
			//
			// Notes:
			// Default driver for Win2K?  Not sure what the "correct" driver is -
			// maybe some variant of AGP440.SYS?
			if (dfilep->mVersion.getField(3) < 1160)
			{
				out_of_date = TRUE;
				llwarns << "Detected sisagpx.sys" << llendl;
				write_debug("Old driver (sisagpx.sys) for SiS detected!");
			}
		}
		if (out_of_date)
		{
			exit_after_bad |= bad_hardware_dialog("Out of date SiS chipset driver",
												  SIS_CHIPSET_URL);
		}
	}

	return exit_after_bad;
#else
	return TRUE;
#endif
}

void LLFeatureManager::applyRecommendedFeatures()
{
	// see featuretable.txt

	llinfos << "Applying Recommended Features" << llendl;
#ifndef LL_RELEASE_FOR_DOWNLOAD
	dump();
#endif
	
	// Enabling AGP
	if (getRecommendedLevel("RenderAGP"))
	{
		gSavedSettings.setBOOL("RenderUseAGP", TRUE);
	}
	else
	{
		gSavedSettings.setBOOL("RenderUseAGP", FALSE);
	}

	// Anisotropic rendering
	BOOL aniso = getRecommendedLevel("RenderAniso");
	LLImageGL::sGlobalUseAnisotropic	= aniso;
	gSavedSettings.setBOOL("RenderAnisotropic", LLImageGL::sGlobalUseAnisotropic);

	// Render Avatar Mode
	BOOL avatar_vp = getRecommendedLevel("RenderAvatarVP");
	S32 avatar_mode = getRecommendedLevel("RenderAvatarMode");
	if (avatar_vp == FALSE)
		avatar_mode = 0;
	gSavedSettings.setBOOL("RenderAvatarVP", avatar_vp);
	gSavedSettings.setS32("RenderAvatarMode", avatar_mode);
	
	// Render Distance
	S32 far_clip = getRecommendedLevel("RenderDistance");
	gSavedSettings.setF32("RenderFarClip", (F32)far_clip);

	// Lighting
	S32 lighting = getRecommendedLevel("RenderLighting");
	gSavedSettings.setS32("RenderLightingDetail", lighting);

	// ObjectBump
	BOOL bump = getRecommendedLevel("RenderObjectBump");
	gSavedSettings.setBOOL("RenderObjectBump", bump);
	
	// Particle Count
	S32 max_parts = getRecommendedLevel("RenderParticleCount");
	gSavedSettings.setS32("RenderMaxPartCount", max_parts);
	LLViewerPartSim::setMaxPartCount(max_parts);

	// RippleWater
	BOOL ripple = getRecommendedLevel("RenderRippleWater");
	gSavedSettings.setBOOL("RenderRippleWater", ripple);
	
	// Vertex Shaders
	S32 shaders = getRecommendedLevel("VertexShaderEnable");
	gSavedSettings.setBOOL("VertexShaderEnable", shaders);

	// Terrain
	S32 terrain = getRecommendedLevel("RenderTerrainDetail");
	gSavedSettings.setS32("RenderTerrainDetail", terrain);
	LLDrawPoolTerrain::sDetailMode = terrain;

	// Set the amount of VRAM we have available
	if (isSafe())
	{
		gSavedSettings.setS32("GraphicsCardMemorySetting", 1);  // 32 MB in 'safe' mode
	}
	else
	{
		S32 idx = gSavedSettings.getS32("GraphicsCardMemorySetting");
		// -1 indicates use default (max), don't change
		if (idx != -1)
		{
			idx = LLViewerImageList::getMaxVideoRamSetting(-2); // get max recommended setting
			gSavedSettings.setS32("GraphicsCardMemorySetting", idx);
		}
	}
}
