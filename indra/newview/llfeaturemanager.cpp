/** 
 * @file llfeaturemanager.cpp
 * @brief LLFeatureManager class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llhttpclient.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "llworld.h"
#include "lldrawpoolterrain.h"
#include "llviewertexturelist.h"
#include "llversioninfo.h"
#include "llwindow.h"
#include "llui.h"
#include "llcontrol.h"
#include "llboost.h"
#include "llweb.h"
#include "llviewershadermgr.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#define LL_EXPORT_GPU_TABLE 0

#if LL_DARWIN
const char FEATURE_TABLE_FILENAME[] = "featuretable_mac.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable_mac.%s.txt";
#elif LL_LINUX
const char FEATURE_TABLE_FILENAME[] = "featuretable_linux.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable_linux.%s.txt";
#elif LL_SOLARIS
const char FEATURE_TABLE_FILENAME[] = "featuretable_solaris.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable_solaris.%s.txt";
#else
const char FEATURE_TABLE_FILENAME[] = "featuretable%s.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable%s.%s.txt";
#endif

const char GPU_TABLE_FILENAME[] = "gpu_table.txt";
const char GPU_TABLE_VER_FILENAME[] = "gpu_table.%s.txt";

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

	LL_WARNS_ONCE("RenderInit") << "Feature " << name << " not on feature list!" << LL_ENDL;
	
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

	LL_WARNS_ONCE("RenderInit") << "Feature " << name << " not on feature list or not available!" << LL_ENDL;
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
	LL_INFOS("RenderInit") << "Applying GPU Feature list: " << name << LL_ENDL;
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

	// first table is install with app
	std::string app_path = gDirUtilp->getAppRODataDir();
	app_path += gDirUtilp->getDirDelimiter();

	std::string filename;
	std::string http_filename; 
#if LL_WINDOWS
	std::string os_string = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	if (os_string.find("Microsoft Windows XP") == 0)
	{
		filename = llformat(FEATURE_TABLE_FILENAME, "_xp");
		http_filename = llformat(FEATURE_TABLE_VER_FILENAME, "_xp", LLVersionInfo::getVersion().c_str());
	}
	else
	{
		filename = llformat(FEATURE_TABLE_FILENAME, "");
		http_filename = llformat(FEATURE_TABLE_VER_FILENAME, "", LLVersionInfo::getVersion().c_str());
	}
#else
	filename = FEATURE_TABLE_FILENAME;
	http_filename = llformat(FEATURE_TABLE_VER_FILENAME, LLVersionInfo::getVersion().c_str());
#endif

	app_path += filename;

	
	// second table is downloaded with HTTP
	std::string http_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, http_filename);

	// use HTTP table if it exists
	std::string path;
	if (gDirUtilp->fileExists(http_path))
	{
		path = http_path;
	}
	else
	{
		path = app_path;
	}

	
	return parseFeatureTable(path);
}


BOOL LLFeatureManager::parseFeatureTable(std::string filename)
{
	llinfos << "Looking for feature table in " << filename << llendl;

	llifstream file;
	std::string name;
	U32		version;
	
	file.open(filename); 	 /*Flawfinder: ignore*/

	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open feature table " << filename << LL_ENDL;
		return FALSE;
	}

	// Check file version
	file >> name;
	file >> version;
	if (name != "version")
	{
		LL_WARNS("RenderInit") << filename << " does not appear to be a valid feature table!" << LL_ENDL;
		return FALSE;
	}

	mTableVersion = version;

	LLFeatureList *flp = NULL;
	while (file >> name)
	{
		char buffer[MAX_STRING];		 /*Flawfinder: ignore*/
		
		if (name.substr(0,2) == "//")
		{
			// This is a comment.
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
	// defaults
	mGPUClass = GPU_CLASS_UNKNOWN;
	mGPUString = gGLManager.getRawGLString();
	mGPUSupported = FALSE;

	// first table is in the app dir
	std::string app_path = gDirUtilp->getAppRODataDir();
	app_path += gDirUtilp->getDirDelimiter();
	app_path += GPU_TABLE_FILENAME;
	
	// second table is downloaded with HTTP
	std::string http_filename = llformat(GPU_TABLE_VER_FILENAME, LLVersionInfo::getVersion().c_str());
	std::string http_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, http_filename);

	// use HTTP table if it exists
	std::string path;
	if (gDirUtilp->fileExists(http_path))
	{
		path = http_path;
	}
	else
	{
		path = app_path;
	}

	parseGPUTable(path);
}

	
void LLFeatureManager::parseGPUTable(std::string filename)
{
	llifstream file;
		
	file.open(filename);

	if (!file)
	{
		LL_WARNS("RenderInit") << "Unable to open GPU table: " << filename << "!" << LL_ENDL;
		return;
	}

	std::string rawRenderer = gGLManager.getRawGLString();
	std::string renderer = rawRenderer;
	for (std::string::iterator i = renderer.begin(); i != renderer.end(); ++i)
	{
		*i = tolower(*i);
	}

#if LL_EXPORT_GPU_TABLE
	llofstream json;
	json.open("gpu_table.json");

	json << "var gpu_table = [" << std::endl;
#endif

	bool gpuFound;
	U32 lineNumber;
	for (gpuFound = false, lineNumber = 0; !gpuFound && !file.eof(); lineNumber++)
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
		std::string cls, label, expr, supported, stats_based, expected_gl_version;
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
		if (token_iter != tokens.end())
		{
			stats_based = *token_iter++;
		}
		if (token_iter != tokens.end())
		{
			expected_gl_version = *token_iter++;
		}

		if (label.empty() || expr.empty() || cls.empty() || supported.empty())
		{
			LL_WARNS("RenderInit") << "invald gpu_table.txt:" << lineNumber << ": '" << buffer << "'" << LL_ENDL;
			continue;
		}
#if LL_EXPORT_GPU_TABLE
		json << "{'label' : '" << label << "',\n" << 
			"'regexp' : '" << expr << "',\n" <<
			"'class' : '" << cls << "',\n" <<
			"'supported' : '" << supported << "',\n" <<
			"'stats_based' : " << stats_based <<  ",\n" <<
			"'gl_version' : " << expected_gl_version << "\n},\n";
#endif

		for (U32 i = 0; i < expr.length(); i++)	 /*Flawfinder: ignore*/
		{
			expr[i] = tolower(expr[i]);
		}

		// run the regular expression against the renderer
		boost::regex re(expr.c_str());
		if(boost::regex_search(renderer, re))
		{
			// if we found it, stop!
#if !LL_EXPORT_GPU_TABLE
			gpuFound = true;
#endif
			mGPUString = label;
			mGPUClass = (EGPUClass) strtol(cls.c_str(), NULL, 10);
			mGPUSupported = (BOOL) strtol(supported.c_str(), NULL, 10);
		}
	}
#if LL_EXPORT_GPU_TABLE
	json << "];\n\n";
	json.close();
#endif
	file.close();

	if ( gpuFound )
	{
		LL_INFOS("RenderInit") << "GPU '" << rawRenderer << "' recognized as '" << mGPUString << "'" << LL_ENDL;
		if (!mGPUSupported)
		{
			LL_INFOS("RenderInit") << "GPU '" << mGPUString << "' is not supported." << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS("RenderInit") << "GPU '" << rawRenderer << "' not recognized" << LL_ENDL;
	}
}

// responder saves table into file
class LLHTTPFeatureTableResponder : public LLHTTPClient::Responder
{
public:

	LLHTTPFeatureTableResponder(std::string filename) :
		mFilename(filename)
	{
	}

	
	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
	{
		if (isGoodStatus(status))
		{
			// write to file

			llinfos << "writing feature table to " << mFilename << llendl;
			
			S32 file_size = buffer->countAfter(channels.in(), NULL);
			if (file_size > 0)
			{
				// read from buffer
				U8* copy_buffer = new U8[file_size];
				buffer->readAfter(channels.in(), NULL, copy_buffer, file_size);

				// write to file
				LLAPRFile out(mFilename, LL_APR_WB);
				out.write(copy_buffer, file_size);
				out.close();
			}
		}
		
	}
	
private:
	std::string mFilename;
};

void fetch_feature_table(std::string table)
{
	const std::string base       = gSavedSettings.getString("FeatureManagerHTTPTable");

#if LL_WINDOWS
	std::string os_string = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	std::string filename;
	if (os_string.find("Microsoft Windows XP") == 0)
	{
		filename = llformat(table.c_str(), "_xp", LLVersionInfo::getVersion().c_str());
	}
	else
	{
		filename = llformat(table.c_str(), "", LLVersionInfo::getVersion().c_str());
	}
#else
	const std::string filename   = llformat(table.c_str(), LLVersionInfo::getVersion().c_str());
#endif

	const std::string url        = base + "/" + filename;

	const std::string path       = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename);

	llinfos << "LLFeatureManager fetching " << url << " into " << path << llendl;
	
	LLHTTPClient::get(url, new LLHTTPFeatureTableResponder(path));
}

void fetch_gpu_table(std::string table)
{
	const std::string base       = gSavedSettings.getString("FeatureManagerHTTPTable");

	const std::string filename   = llformat(table.c_str(), LLVersionInfo::getVersion().c_str());

	const std::string url        = base + "/" + filename;

	const std::string path       = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename);

	llinfos << "LLFeatureManager fetching " << url << " into " << path << llendl;
	
	LLHTTPClient::get(url, new LLHTTPFeatureTableResponder(path));
}

// fetch table(s) from a website (S3)
void LLFeatureManager::fetchHTTPTables()
{
	fetch_feature_table(FEATURE_TABLE_VER_FILENAME);
	fetch_gpu_table(GPU_TABLE_VER_FILENAME);
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
	S32 level = llmax(GPU_CLASS_0, llmin(mGPUClass, GPU_CLASS_5));

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
	LLViewerShaderMgr::sSkipReload = true;

	applyBaseMasks();
	
	switch (level)
	{
		case 0:
			if (gGLManager.mGLVersion < 3.f || gGLManager.mIsIntel)
			{ //only use fixed function by default if GL version < 3.0 or this is an intel graphics chip
				maskFeatures("LowFixedFunction");			
			}
			else
			{ //same as low, but with "Basic Shaders" enabled
				maskFeatures("Low");
			}
			maskFeatures("Class0");
			break;
		case 1:
			maskFeatures("Mid");
			maskFeatures("Class1");
			break;
		case 2:
			maskFeatures("High");
			maskFeatures("Class2");
			break;
		case 3:
			maskFeatures("High");
			maskFeatures("Class3");
			break;
		case 4:
			maskFeatures("High");
			maskFeatures("Class4");
			break;
		case 5:
			maskFeatures("High");
			maskFeatures("Class5");
			break;
		case 6:
			maskFeatures("Ultra");
			break;
		default:
			maskFeatures("Low");
			maskFeatures("Class0");
			break;
	}

	applyFeatures(skipFeatures);

	LLViewerShaderMgr::sSkipReload = false;
	LLViewerShaderMgr::instance()->setShaders();
	gPipeline.refreshCachedSettings();
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
	if (mGPUClass >= 0 && mGPUClass < 6)
	{
		const char* class_table[] =
		{
			"Class0",
			"Class1",
			"Class2",
			"Class3",
			"Class4",
			"Class5",
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
	if (!gGLManager.mHasVertexShader || !mGPUSupported)
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
	if (gGLManager.mHasATIMemInfo && gGLManager.mVRAM < 256)
	{
		maskFeatures("ATIVramLT256");
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
	if (gGLManager.mGLVersion < 3.f)
	{
		maskFeatures("OpenGLPre30");
	}
	if (gGLManager.mNumTextureImageUnits <= 8)
	{
		maskFeatures("TexUnit8orLess");
	}
	if (gGLManager.mHasMapBufferRange)
	{
		maskFeatures("MapBufferRange");
	}
	if (gGLManager.mVRAM > 512)
	{
		maskFeatures("VRAMGT512");
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
