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
#include <boost/assign/list_of.hpp>

#include "llfeaturemanager.h"
#include "lldir.h"

#include "llsys.h"
#include "llgl.h"

#include "llappviewer.h"
#include "llbufferstream.h"
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
#include "llstring.h"
#include "stringize.h"
#include "llcorehttputil.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

#if LL_DARWIN
const char FEATURE_TABLE_FILENAME[] = "featuretable_mac.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable_mac.%s.txt";
#elif LL_LINUX
const char FEATURE_TABLE_FILENAME[] = "featuretable_linux.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable_linux.%s.txt";
#else
const char FEATURE_TABLE_FILENAME[] = "featuretable.txt";
const char FEATURE_TABLE_VER_FILENAME[] = "featuretable.%s.txt";
#endif

#if 0                               // consuming code in #if 0 below
#endif
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
    LL_DEBUGS_ONCE("RenderInit") << "Feature '" << name << "' "
                                 << (available ? "" : "not " ) << "available"
                                 << " at " << level
                                 << LL_ENDL;
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
        LL_DEBUGS_ONCE("RenderInit") << "Setting '" << name << "' to recommended value " <<  mFeatures[name].mRecommendedLevel << LL_ENDL;
		return mFeatures[name].mRecommendedLevel;
	}

	LL_WARNS_ONCE("RenderInit") << "Feature " << name << " not on feature list or not available!" << LL_ENDL;
	return 0;
}

BOOL LLFeatureList::maskList(LLFeatureList &mask)
{
	LL_DEBUGS_ONCE() << "Masking with " << mask.mName << LL_ENDL;
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

	LLFeatureInfo fi;
	feature_map_t::iterator feature_it;
	for (feature_it = mFeatures.begin(); feature_it != mFeatures.end(); ++feature_it)
	{
		fi = feature_it->second;
		LL_DEBUGS("RenderInit") << "With " << mName << " feature " << fi.mName << " " << fi.mAvailable << ":" << fi.mRecommendedLevel << LL_ENDL;
	}
}

static const std::vector<std::string> sGraphicsLevelNames = boost::assign::list_of
	("Low")
	("LowMid")
	("Mid")
	("MidHigh")
	("High")
	("HighUltra")
	("Ultra")
;

U32 LLFeatureManager::getMaxGraphicsLevel() const
{
	return sGraphicsLevelNames.size() - 1;
}

bool LLFeatureManager::isValidGraphicsLevel(U32 level) const
{
	return (level <= getMaxGraphicsLevel());
}

std::string LLFeatureManager::getNameForGraphicsLevel(U32 level) const
{
	if (isValidGraphicsLevel(level))
	{
		return sGraphicsLevelNames[level];
	}
	return STRINGIZE("Invalid graphics level " << level << ", valid are 0 .. "
					 << getMaxGraphicsLevel());
}

S32 LLFeatureManager::getGraphicsLevelForName(const std::string& name) const
{
	const std::string FixedFunction("FixedFunction");
	std::string rname(name);
	if (LLStringUtil::endsWith(rname, FixedFunction))
	{
		// chop off any "FixedFunction" suffix
		rname = rname.substr(0, rname.length() - FixedFunction.length());
	}
	for (S32 i(0), iend(getMaxGraphicsLevel()); i <= iend; ++i)
	{
		if (sGraphicsLevelNames[i] == rname)
		{
			return i;
		}
	}
	return -1;
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

bool LLFeatureManager::loadFeatureTables()
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
	filename = FEATURE_TABLE_FILENAME;
	http_filename = llformat(FEATURE_TABLE_VER_FILENAME, LLVersionInfo::getVersion().c_str());

	app_path += filename;

	// second table is downloaded with HTTP - note that this will only be used on the run _after_ it is downloaded
	std::string http_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, http_filename);

	// use HTTP table if it exists
	std::string path;
	bool parse_ok = false;
	if (gDirUtilp->fileExists(http_path))
	{
		parse_ok = parseFeatureTable(http_path);
		if (!parse_ok)
		{
			// the HTTP table failed to parse, so delete it
			LLFile::remove(http_path);
			LL_WARNS("RenderInit") << "Removed invalid feature table '" << http_path << "'" << LL_ENDL;
		}
	}

	if (!parse_ok)
	{
		parse_ok = parseFeatureTable(app_path);
	}

	return parse_ok;
}


bool LLFeatureManager::parseFeatureTable(std::string filename)
{
	LL_INFOS("RenderInit") << "Attempting to parse feature table from " << filename << LL_ENDL;

	llifstream file;
	std::string name;
	U32		version;
	
	cleanupFeatureTables(); // in case an earlier attempt left partial results
	file.open(filename.c_str()); 	 /*Flawfinder: ignore*/

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
		return false;
	}

	mTableVersion = version;

	LLFeatureList *flp = NULL;
	bool parse_ok = true;
	while (file >> name && parse_ok)
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
			LL_DEBUGS("RenderInit") << "Before new list" << std::endl;
			if (flp)
			{
				flp->dump();
			}
			else
			{
				LL_CONT << "No current list";
			}
			LL_CONT << LL_ENDL;
			
			// It's a new mask, create it.
			file >> name;
			if (!mMaskList.count(name))
			{
                flp = new LLFeatureList(name);
                mMaskList[name] = flp;
            }
            else
            {
				LL_WARNS("RenderInit") << "Overriding mask " << name << ", this is invalid!" << LL_ENDL;
				parse_ok = false;
			}
		}
		else
		{
			if (flp)
			{
                S32 available;
                F32 recommended;
                file >> available >> recommended;
                flp->addFeature(name, available, recommended);
            }
			else
			{
				LL_WARNS("RenderInit") << "Specified parameter before <list> keyword!" << LL_ENDL;
				parse_ok = false;
			}
		}
	}
	file.close();

	if (!parse_ok)
	{
		LL_WARNS("RenderInit") << "Discarding feature table data from " << filename << LL_ENDL;
		cleanupFeatureTables();
	}
	
	return parse_ok;
}

F32 gpu_benchmark();

bool LLFeatureManager::loadGPUClass()
{
	//get memory bandwidth from benchmark
	F32 gbps = gpu_benchmark();

	if (gbps < 0.f)
	{ //couldn't bench, use GLVersion
#if LL_DARWIN
        //GLVersion is misleading on OSX, just default to class 3 if we can't bench
		LL_WARNS() << "Unable to get an accurate benchmark; defaulting to class 3" << LL_ENDL;
        mGPUClass = GPU_CLASS_3;
#else
		if (gGLManager.mGLVersion < 2.f)
		{
			mGPUClass = GPU_CLASS_0;
		}
		else if (gGLManager.mGLVersion < 3.f)
		{
			mGPUClass = GPU_CLASS_1;
		}
		else if (gGLManager.mGLVersion < 3.3f)
		{
			mGPUClass = GPU_CLASS_2;
		}
		else if (gGLManager.mGLVersion < 4.f)
		{
			mGPUClass = GPU_CLASS_3;
		}
		else 
		{
			mGPUClass = GPU_CLASS_4;
		}
#endif
	}
	else if (gGLManager.mGLVersion <= 2.f)
	{
		mGPUClass = GPU_CLASS_0;
	}
	else if (gGLManager.mGLVersion <= 3.f)
	{
		mGPUClass = GPU_CLASS_1;
	}
	else if (gbps <= 5.f)
	{
		mGPUClass = GPU_CLASS_0;
	}
	else if (gbps <= 8.f)
	{
		mGPUClass = GPU_CLASS_1;
	}
	else if (gbps <= 16.f)
	{
		mGPUClass = GPU_CLASS_2;
	}
	else if (gbps <= 40.f)
	{
		mGPUClass = GPU_CLASS_3;
	}
	else if (gbps <= 80.f)
	{
		mGPUClass = GPU_CLASS_4;
	}
	else 
	{
		mGPUClass = GPU_CLASS_5;
	}

	// defaults
	mGPUString = gGLManager.getRawGLString();
	mGPUSupported = TRUE;

	return true; // indicates that a gpu value was established
}

void LLFeatureManager::fetchFeatureTableCoro(std::string tableName)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FeatureManagerHTTPTable", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    const std::string base = gSavedSettings.getString("FeatureManagerHTTPTable");


#if LL_WINDOWS
    std::string os_string = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
    std::string filename;

    if (os_string.find("Microsoft Windows XP") == 0)
    {
        filename = llformat(tableName.c_str(), "_xp", LLVersionInfo::getVersion().c_str());
    }
    else
    {
        filename = llformat(tableName.c_str(), "", LLVersionInfo::getVersion().c_str());
    }
#else
    const std::string filename   = llformat(tableName.c_str(), LLVersionInfo::getVersion().c_str());
#endif

    std::string url        = base + "/" + filename;
    // testing url below
    //url = "http://viewer-settings.secondlife.com/featuretable.2.1.1.208406.txt";
    const std::string path       = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename);


    LL_INFOS() << "LLFeatureManager fetching " << url << " into " << path << LL_ENDL;

    LLSD result = httpAdapter->getRawAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {   // There was a newer feature table on the server. We've grabbed it and now should write it.
        // write to file
        const LLSD::Binary &raw = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW].asBinary();

        LL_INFOS() << "writing feature table to " << path << LL_ENDL;

        S32 size = raw.size();
        if (size > 0)
        {
            // write to file
            LLAPRFile out(path, LL_APR_WB);
            out.write(raw.data(), size);
            out.close();
        }
    }
}

// fetch table(s) from a website (S3)
void LLFeatureManager::fetchHTTPTables()
{
    LLCoros::instance().launch("LLFeatureManager::fetchFeatureTableCoro",
        boost::bind(&LLFeatureManager::fetchFeatureTableCoro, this, FEATURE_TABLE_VER_FILENAME));
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
	loadGPUClass();
	// apply saved settings
	// cap the level at 2 (high)
	U32 level = llmax(GPU_CLASS_0, llmin(mGPUClass, GPU_CLASS_5));

	LL_INFOS("RenderInit") << "Applying Recommended Features for level " << level << LL_ENDL;

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
			LL_WARNS() << "AHHH! Control setting " << mIt->first << " does not exist!" << LL_ENDL;
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
			LL_WARNS() << "AHHH! Control variable is not a numeric type!" << LL_ENDL;
		}
	}
}

void LLFeatureManager::setGraphicsLevel(U32 level, bool skipFeatures)
{
	LLViewerShaderMgr::sSkipReload = true;

	applyBaseMasks();

	// if we're passed an invalid level, default to "Low"
	std::string features(isValidGraphicsLevel(level)? getNameForGraphicsLevel(level) : "Low");
	if (features == "Low")
	{
#if LL_DARWIN
		// This Mac-specific change is to insure that we force 'Basic Shaders' for all Mac
		// systems which support them instead of falling back to fixed-function unnecessarily
		// MAINT-2157
		if (gGLManager.mGLVersion < 2.1f)
#else
		// only use fixed function by default if GL version < 3.0 or this is an intel graphics chip
		if (gGLManager.mGLVersion < 3.f || gGLManager.mIsIntel)
#endif
		{
            // same as Low, but with "Basic Shaders" disabled
			features = "LowFixedFunction";
		}
	}

	maskFeatures(features);

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

#if LL_DARWIN
	const LLOSInfo& osInfo = LLAppViewer::instance()->getOSInfo();
	if (osInfo.mMajorVer == 10 && osInfo.mMinorVer < 7)
	{
		maskFeatures("OSX_10_6_8");
	}
#endif

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

	//LL_INFOS() << "Masking features from gpu table match: " << gpustr << LL_ENDL;
	maskFeatures(gpustr);

	// now mask cpu type ones
	if (gSysMemory.getPhysicalMemoryKB() <= U32Megabytes(256))
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

LLSD LLFeatureManager::getRecommendedSettingsMap()
{
	// Create the map and fill it with the hardware recommended settings.
	// It's needed to create an initial Default graphics preset (MAINT-6435).
	// The process is similar to the one LLFeatureManager::applyRecommendedSettings() does.

	LLSD map(LLSD::emptyMap());

	loadGPUClass();
	U32 level = llmax(GPU_CLASS_0, llmin(mGPUClass, GPU_CLASS_5));
	LL_INFOS("RenderInit") << "Getting the map of recommended settings for level " << level << LL_ENDL;

	applyBaseMasks();
	std::string features(isValidGraphicsLevel(level) ? getNameForGraphicsLevel(level) : "Low");

	maskFeatures(features);

	LLControlVariable* ctrl = gSavedSettings.getControl("RenderQualityPerformance"); // include the quality value for correct preset loading   
	map["RenderQualityPerformance"]["Value"] = (LLSD::Integer)level;
	map["RenderQualityPerformance"]["Comment"] = ctrl->getComment();;
	map["RenderQualityPerformance"]["Persist"] = 1;
	map["RenderQualityPerformance"]["Type"] = LLControlGroup::typeEnumToString(ctrl->type());



	for (feature_map_t::iterator mIt = mFeatures.begin(); mIt != mFeatures.end(); ++mIt)
	{
		LLControlVariable* ctrl = gSavedSettings.getControl(mIt->first);
		if (ctrl == NULL)
		{
			LL_WARNS() << "AHHH! Control setting " << mIt->first << " does not exist!" << LL_ENDL;
			continue;
		}

		if (ctrl->isType(TYPE_BOOLEAN))
		{
			map[mIt->first]["Value"] = (LLSD::Boolean)getRecommendedValue(mIt->first);
		}
		else if (ctrl->isType(TYPE_S32) || ctrl->isType(TYPE_U32))
		{
			map[mIt->first]["Value"] = (LLSD::Integer)getRecommendedValue(mIt->first);
		}
		else if (ctrl->isType(TYPE_F32))
		{
			map[mIt->first]["Value"] = (LLSD::Real)getRecommendedValue(mIt->first);
		}
		else
		{
			LL_WARNS() << "AHHH! Control variable is not a numeric type!" << LL_ENDL;
			continue;
		}
		map[mIt->first]["Comment"] = ctrl->getComment();;
		map[mIt->first]["Persist"] = 1;
		map[mIt->first]["Type"] = LLControlGroup::typeEnumToString(ctrl->type());
	}
	
	return map;
}
