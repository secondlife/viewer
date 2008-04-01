/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include <iostream>
#include <fstream>

#include "llviewerprecompiledheaders.h"

#include <boost/regex.hpp>

#include "llfeaturemanager.h"
#include "lldir.h"

#include "llsys.h"
#include "llgl.h"
#include "llsecondlifeurls.h"

#include "llviewercontrol.h"
#include "llworld.h"
#include "lldrawpoolterrain.h"
#include "llviewerimagelist.h"
#include "llwindow.h"
#include "llui.h"
#include "llcontrol.h"
#include "llboost.h"
#include "llweb.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

//
// externs
//
extern LLMemoryInfo gSysMemory;
extern LLCPUInfo gSysCPU;

#if LL_DARWIN
const char FEATURE_TABLE_FILENAME[] = "featuretable_mac.txt";
#elif LL_LINUX
const char FEATURE_TABLE_FILENAME[] = "featuretable_linux.txt";
#elif LL_SOLARIS
const char FEATURE_TABLE_FILENAME[] = "featuretable_solaris.txt";
#else
const char FEATURE_TABLE_FILENAME[] = "featuretable.txt";
#endif

const char GPU_TABLE_FILENAME[] = "gpu_table.txt";

LLFeatureInfo::LLFeatureInfo(const char *name, const BOOL available, const F32 level) : mValid(TRUE)
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

void LLFeatureList::addFeature(const char *name, const BOOL available, const F32 level)
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
	
	// changing this to TRUE so you have to explicitly disable 
	// something for it to be disabled
	return TRUE;
}

F32 LLFeatureList::getRecommendedValue(const char *name)
{
	if (mFeatures.count(name) && isFeatureAvailable(name))
	{
		return mFeatures[name].mRecommendedLevel;
	}

	llwarns << "Feature " << name << " not on feature list or not available!" << llendl;
	return 0;
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
	llinfos << "After applying mask " << mask.mName << llendl;
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
// 		llwarns << "Unknown feature mask " << name << llendl;
		return FALSE;
	}
	llinfos << "Applying Feature Mask: " << name << llendl;
	return maskList(*maskp);
}

BOOL LLFeatureManager::loadFeatureTables()
{
	// *TODO - if I or anyone else adds something else to the skipped list
	// make this data driven.  Put it in the feature table and parse it
	// correctly
	mSkippedFeatures.insert("RenderAnisotropic");
	mSkippedFeatures.insert("RenderGamma");
	mSkippedFeatures.insert("RenderVBOEnable");
	mSkippedFeatures.insert("RenderFogRatio");

	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += FEATURE_TABLE_FILENAME;


	char	name[MAX_STRING+1];	 /*Flawfinder: ignore*/

	llifstream file;
	U32		version;
	
	file.open(data_path.c_str()); 	 /*Flawfinder: ignore*/

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
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
		name[0] = 0;

		file >> name;
		
		if (strlen(name) >= 2 && 	 /*Flawfinder: ignore*/
			name[0] == '/' && 
			name[1] == '/')
		{
			// This is a comment.
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (strlen(name) == 0)		 /*Flawfinder: ignore*/
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

			flp = new LLFeatureList(name);
			mMaskList[name] = flp;
		}
		else
		{
			if (!flp)
			{
				llerrs << "Specified parameter before <list> keyword!" << llendl;
			}
			S32 available;
			F32 recommended;
			file >> available >> recommended;
			flp->addFeature(name, available, recommended);
		}
	}
	file.close();

	return TRUE;
}

void LLFeatureManager::loadGPUClass()
{
	std::string data_path = gDirUtilp->getAppRODataDir();

	data_path += gDirUtilp->getDirDelimiter();

	data_path += GPU_TABLE_FILENAME;

	// defaults
	mGPUClass = GPU_CLASS_UNKNOWN;
	mGPUString = gGLManager.getRawGLString();
	mGPUSupported = FALSE;

	llifstream file;
		
	file.open(data_path.c_str()); 		 /*Flawfinder: ignore*/

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
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
		buffer[0] = 0;

		file.getline(buffer, MAX_STRING);
		
		if (strlen(buffer) >= 2 && 	 /*Flawfinder: ignore*/
			buffer[0] == '/' && 
			buffer[1] == '/')
		{
			// This is a comment.
			continue;
		}

		if (strlen(buffer) == 0)	 /*Flawfinder: ignore*/
		{
			// This is a blank line
			continue;
		}

		// setup the tokenizer
		std::string buf(buffer);
		std::string cls, label, expr, supported;
		boost_tokenizer tokens(buf, boost::char_separator<char>("\t\n"));
		boost_tokenizer::iterator token_iter = tokens.begin();

		// grab the label, pseudo regular expression, and class
		if(token_iter != tokens.end())
		{
			label = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			expr = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			cls = *token_iter++;
		}
		if(token_iter != tokens.end())
		{
			supported = *token_iter++;
		}

		if (label.empty() || expr.empty() || cls.empty() || supported.empty())
		{
			continue;
		}
	
		for (U32 i = 0; i < expr.length(); i++)	 /*Flawfinder: ignore*/
		{
			expr[i] = tolower(expr[i]);
		}

		// run the regular expression against the renderer
		boost::regex re(expr.c_str());
		if(boost::regex_search(renderer, re))
		{
			// if we found it, stop!
			file.close();
			llinfos << "GPU is " << label << llendl;
			mGPUString = label;
			mGPUClass = (EGPUClass) strtol(cls.c_str(), NULL, 10);
			mGPUSupported = (BOOL) strtol(supported.c_str(), NULL, 10);
			file.close();
			return;
		}
	}
	file.close();

	llwarns << "Couldn't match GPU to a class: " << gGLManager.getRawGLString() << llendl;
}

void LLFeatureManager::cleanupFeatureTables()
{
	std::for_each(mMaskList.begin(), mMaskList.end(), DeletePairedPointer());
	mMaskList.clear();
}

void LLFeatureManager::init()
{
	// load the tables
	loadFeatureTables();

	// get the gpu class
	loadGPUClass();

	// apply the base masks, so we know if anything is disabled
	applyBaseMasks();
}

void LLFeatureManager::applyRecommendedSettings()
{
	// apply saved settings
	// cap the level at 2 (high)
	S32 level = llmax(GPU_CLASS_0, llmin(mGPUClass, GPU_CLASS_2));

	llinfos << "Applying Recommended Features" << llendl;

	setGraphicsLevel(level, false);
	gSavedSettings.setU32("RenderQualityPerformance", level);
	gSavedSettings.setBOOL("RenderCustomSettings", FALSE);

	// now apply the tweaks to draw distance
	// these are double negatives, because feature masks only work by
	// downgrading values, so i needed to make a true value go to false
	// for certain cards, thus the awkward name, "Disregard..."
	if(!gSavedSettings.getBOOL("Disregard96DefaultDrawDistance"))
	{
		gSavedSettings.setF32("RenderFarClip", 96.0f);
	}
	else if(!gSavedSettings.getBOOL("Disregard128DefaultDrawDistance"))
	{
		gSavedSettings.setF32("RenderFarClip", 128.0f);
	}


}

void LLFeatureManager::applyFeatures(bool skipFeatures)
{
	// see featuretable.txt / featuretable_linux.txt / featuretable_mac.txt

#ifndef LL_RELEASE_FOR_DOWNLOAD
	dump();
#endif

	// scroll through all of these and set their corresponding control value
	for(feature_map_t::iterator mIt = mFeatures.begin(); 
		mIt != mFeatures.end(); 
		++mIt)
	{
		// skip features you want to skip
		// do this for when you don't want to change certain settings
		if(skipFeatures)
		{
			if(mSkippedFeatures.find(mIt->first) != mSkippedFeatures.end())
			{
				continue;
			}
		}

		// get the control setting
		LLControlVariable* ctrl = gSavedSettings.getControl(mIt->first);
		if(ctrl == NULL)
		{
			llwarns << "AHHH! Control setting " << mIt->first << " does not exist!" << llendl;
			continue;
		}

		// handle all the different types
		if(ctrl->isType(TYPE_BOOLEAN))
		{
			gSavedSettings.setBOOL(mIt->first, (BOOL)getRecommendedValue(mIt->first.c_str()));
		}
		else if (ctrl->isType(TYPE_S32))
		{
			gSavedSettings.setS32(mIt->first, (S32)getRecommendedValue(mIt->first.c_str()));
		}
		else if (ctrl->isType(TYPE_U32))
		{
			gSavedSettings.setU32(mIt->first, (U32)getRecommendedValue(mIt->first.c_str()));
		}
		else if (ctrl->isType(TYPE_F32))
		{
			gSavedSettings.setF32(mIt->first, (F32)getRecommendedValue(mIt->first.c_str()));
		}
		else
		{
			llwarns << "AHHH! Control variable is not a numeric type!" << llendl;
		}
	}
}

void LLFeatureManager::setGraphicsLevel(S32 level, bool skipFeatures)
{
	applyBaseMasks();

	switch (level)
	{
		case 0:
			maskFeatures("Low");			
			break;
		case 1:
			maskFeatures("Mid");
			break;
		case 2:
			maskFeatures("High");
			break;
		case 3:
			maskFeatures("Ultra");
			break;
		default:
			maskFeatures("Low");
			break;
	}

	applyFeatures(skipFeatures);
}

void LLFeatureManager::applyBaseMasks()
{
	// reapply masks
	mFeatures.clear();

	LLFeatureList* maskp = findMask("all");
	if(maskp == NULL)
	{
		llwarns << "AHH! No \"all\" in feature table!" << llendl;
		return;
	}

	mFeatures = maskp->getFeatures();

	// mask class
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
	else
	{
		llinfos << "Setting GPU Class to Unknown" << llendl;
		maskFeatures("Unknown");
	}

	// now all those wacky ones
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
	if (gGLManager.mIsGFFX)
	{
		maskFeatures("GeForceFX");
	}
	if (gGLManager.mIsIntel)
	{
		maskFeatures("Intel");
	}
	if (gGLManager.mGLVersion < 1.5f)
	{
		maskFeatures("OpenGLPre15");
	}

	// now mask by gpu string
	// Replaces ' ' with '_' in mGPUString to deal with inability for parser to handle spaces
	std::string gpustr = mGPUString;
	for (std::string::iterator iter = gpustr.begin(); iter != gpustr.end(); ++iter)
	{
		if (*iter == ' ')
		{
			*iter = '_';
		}
	}

	//llinfos << "Masking features from gpu table match: " << gpustr << llendl;
	maskFeatures(gpustr.c_str());

	// now mask cpu type ones
	if (gSysMemory.getPhysicalMemoryClamped() <= 256*1024*1024)
	{
		maskFeatures("RAM256MB");
	}
	
#if LL_SOLARIS && defined(__sparc) 	//  even low MHz SPARCs are fast
#error The 800 is hinky. Would something like a LL_MIN_MHZ make more sense here?
	if (gSysCPU.getMhz() < 800)
#else
	if (gSysCPU.getMhz() < 1100)
#endif
	{
		maskFeatures("CPUSlow");
	}

	if (isSafe())
	{
		maskFeatures("safe");
	}
}
