/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include <iostream>
#include <fstream>

#include <boost/regex.hpp>

#include "llfeaturemanager.h"
#include "lldir.h"

#include "llsys.h"
#include "llgl.h"
#include "llsecondlifeurls.h"

#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llworld.h"
#include "lldrawpoolterrain.h"
#include "llviewertexturelist.h"
#include "llwindow.h"
#include "llui.h"
#include "llcontrol.h"
#include "llboost.h"
#include "llweb.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif


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

LLFeatureInfo::LLFeatureInfo(const std::string& name, const BOOL available, const F32 level)
	: mValid(TRUE), mName(name), mAvailable(available), mRecommendedLevel(level)
{
}

LLFeatureList::LLFeatureList(const std::string& name)
	: mName(name)
{
}

LLFeatureList::~LLFeatureList()
{
}

void LLFeatureList::addFeature(const std::string& name, const BOOL available, const F32 level)
{
	if (mFeatures.count(name))
	{
		LL_WARNS("RenderInit") << "LLFeatureList::Attempting to add preexisting feature " << name << LL_ENDL;
	}

	LLFeatureInfo fi(name, available, level);
	mFeatures[name] = fi;
}

BOOL LLFeatureList::isFeatureAvailable(const std::string& name)
{
	if (mFeatures.count(name))
	{
		return mFeatures[name].mAvailable;
	}

	LL_WARNS("RenderInit") << "Feature " << name << " not on feature list!" << LL_ENDL;
	
	// changing this to TRUE so you have to explicitly disable 
	// something for it to be disabled
	return TRUE;
}

F32 LLFeatureList::getRecommendedValue(const std::string& name)
{
	if (mFeatures.count(name) && isFeatureAvailable(name))
	{
		return mFeatures[name].mRecommendedLevel;
	}

	LL_WARNS("RenderInit") << "Feature " << name << " not on feature list or not available!" << LL_ENDL;
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
			LL_WARNS("RenderInit") << "Feature " << mask_fi.mName << " in mask not in top level!" << LL_ENDL;
			continue;
		}

		LLFeatureInfo &cur_fi = mFeatures[mask_fi.mName];
		if (mask_fi.mAvailable && !cur_fi.mAvailable)
		{
			LL_WARNS("RenderInit") << "Mask attempting to reenabling disabled feature, ignoring " << cur_fi.mName << LL_ENDL;
			continue;
		}
		cur_fi.mAvailable = mask_fi.mAvailable;
		cur_fi.mRecommendedLevel = llmin(cur_fi.mRecommendedLevel, mask_fi.mRecommendedLevel);
		LL_DEBUGS("RenderInit") << "Feature mask " << mask.mName
				<< " Feature " << mask_fi.mName
				<< " Mask: " << mask_fi.mRecommendedLevel
				<< " Now: " << cur_fi.mRecommendedLevel << LL_ENDL;
	}

	LL_DEBUGS("RenderInit") << "After applying mask " << mask.mName << std::endl;
		// Will conditionally call dump only if the above message will be logged, thanks 
		// to it being wrapped by the LL_DEBUGS and LL_ENDL macros.
		dump();
	LL_CONT << LL_ENDL;

	return TRUE;
}

void LLFeatureList::dump()
{
	LL_DEBUGS("RenderInit") << "Feature list: " << mName << LL_ENDL;
	LL_DEBUGS("RenderInit") << "--------------" << LL_ENDL;

	LLFeatureInfo fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mFeatures.begin(); feature_it != mFeatures.end(); ++feature_it)
	{
		fi = feature_it->second;
		LL_DEBUGS("RenderInit") << fi.mName << "\t\t" << fi.mAvailable << ":" << fi.mRecommendedLevel << LL_ENDL;
	}
	LL_DEBUGS("RenderInit") << LL_ENDL;
}

LLFeatureList *LLFeatureManager::findMask(const std::string& name)
{
	if (mMaskList.count(name))
	{
		return mMaskList[name];
	}

	return NULL;
}

BOOL LLFeatureManager::maskFeatures(const std::string& name)
{
	LLFeatureList *maskp = findMask(name);
	if (!maskp)
	{
 		LL_DEBUGS("RenderInit") << "Unknown feature mask " << name << LL_ENDL;
		return FALSE;
	}
	LL_DEBUGS("RenderInit") << "Applying Feature Mask: " << name << LL_ENDL;
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
	lldebugs << "Looking for feature table in " << data_path << llendl;

	llifstream file;
	std::string name;
	U32		version;
	
	file.open(data_path); 	 /*Flawfinder: ignore*/

	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open feature table!" << LL_ENDL;
		return FALSE;
	}

	// Check file version
	file >> name;
	file >> version;
	if (name != "version")
	{
		LL_WARNS("RenderInit") << data_path << " does not appear to be a valid feature table!" << LL_ENDL;
		return FALSE;
	}

	mTableVersion = version;

	LLFeatureList *flp = NULL;
	while (!file.eof() && file.good())
	{
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/

		file >> name;
		
		if (name.substr(0,2) == "//")
		{
			// This is a comment.
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (name.empty())
		{
			// This is a blank line
			file.getline(buffer, MAX_STRING);
			continue;
		}

		if (name == "list")
		{
			if (flp)
			{
				//flp->dump();
			}
			// It's a new mask, create it.
			file >> name;
			if (mMaskList.count(name))
			{
				LL_ERRS("RenderInit") << "Overriding mask " << name << ", this is invalid!" << LL_ENDL;
			}

			flp = new LLFeatureList(name);
			mMaskList[name] = flp;
		}
		else
		{
			if (!flp)
			{
				LL_ERRS("RenderInit") << "Specified parameter before <list> keyword!" << LL_ENDL;
				return FALSE;
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
		
	file.open(data_path); 		 /*Flawfinder: ignore*/

	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open GPU table: " << data_path << "!" << LL_ENDL;
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
			LL_INFOS("RenderInit") << "GPU is " << label << llendl;
			mGPUString = label;
			mGPUClass = (EGPUClass) strtol(cls.c_str(), NULL, 10);
			mGPUSupported = (BOOL) strtol(supported.c_str(), NULL, 10);
			file.close();
			return;
		}
	}
	file.close();

	LL_WARNS("RenderInit") << "Couldn't match GPU to a class: " << gGLManager.getRawGLString() << LL_ENDL;
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
			gSavedSettings.setBOOL(mIt->first, (BOOL)getRecommendedValue(mIt->first));
		}
		else if (ctrl->isType(TYPE_S32))
		{
			gSavedSettings.setS32(mIt->first, (S32)getRecommendedValue(mIt->first));
		}
		else if (ctrl->isType(TYPE_U32))
		{
			gSavedSettings.setU32(mIt->first, (U32)getRecommendedValue(mIt->first));
		}
		else if (ctrl->isType(TYPE_F32))
		{
			gSavedSettings.setF32(mIt->first, (F32)getRecommendedValue(mIt->first));
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
		LL_WARNS("RenderInit") << "AHH! No \"all\" in feature table!" << LL_ENDL;
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

		LL_INFOS("RenderInit") << "Setting GPU Class to " << class_table[mGPUClass] << LL_ENDL;
		maskFeatures(class_table[mGPUClass]);
	}
	else
	{
		LL_INFOS("RenderInit") << "Setting GPU Class to Unknown" << LL_ENDL;
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
	if (gGLManager.mATIOldDriver)
	{
		maskFeatures("ATIOldDriver");
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
	maskFeatures(gpustr);

	// now mask cpu type ones
	if (gSysMemory.getPhysicalMemoryClamped() <= 256*1024*1024)
	{
		maskFeatures("RAM256MB");
	}
	
#if LL_SOLARIS && defined(__sparc) 	//  even low MHz SPARCs are fast
#error The 800 is hinky. Would something like a LL_MIN_MHZ make more sense here?
	if (gSysCPU.getMHz() < 800)
#else
	if (gSysCPU.getMHz() < 1100)
#endif
	{
		maskFeatures("CPUSlow");
	}

	if (isSafe())
	{
		maskFeatures("safe");
	}
}
