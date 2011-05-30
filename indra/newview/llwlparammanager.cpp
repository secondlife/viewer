/**
 * @file llwlparammanager.cpp
 * @brief Implementation for the LLWLParamManager class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llwlparammanager.h"

#include "pipeline.h"
#include "llsky.h"

#include "lldiriterator.h"
#include "llfloaterreg.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llsdserialize.h"

#include "v4math.h"
#include "llviewerdisplay.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "lldrawpoolwater.h"
#include "llagent.h"
#include "llviewerregion.h"

#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llwlparamset.h"
#include "llpostprocess.h"

#include "llfloaterwindlight.h"
#include "llfloaterdaycycle.h"
#include "llfloaterenvsettings.h"
#include "llviewershadermgr.h"
#include "llglslshader.h"

#include "curl/curl.h"
#include "llstreamtools.h"

LLWLParamManager::LLWLParamManager() :

	//set the defaults for the controls
	// index is from sWLUniforms in pipeline.cpp line 979

	/// Sun Delta Terrain tweak variables.
	mSunDeltaYaw(180.0f),
	mSceneLightStrength(2.0f),
	mWLGamma(1.0f, "gamma"),

	mBlueHorizon(0.25f, 0.25f, 1.0f, 1.0f, "blue_horizon", "WLBlueHorizon"),
	mHazeDensity(1.0f, 1.0f, 1.0f, 0.5f, "haze_density"),
	mBlueDensity(0.25f, 0.25f, 0.25f, 1.0f, "blue_density", "WLBlueDensity"),
	mDensityMult(1.0f, "density_multiplier", 1000),
	mHazeHorizon(1.0f, 1.0f, 1.0f, 0.5f, "haze_horizon"),
	mMaxAlt(4000.0f, "max_y"),

	// Lighting
	mLightnorm(0.f, 0.707f, -0.707f, 1.f, "lightnorm"),
	mSunlight(0.5f, 0.5f, 0.5f, 1.0f, "sunlight_color", "WLSunlight"),
	mAmbient(0.5f, 0.75f, 1.0f, 1.19f, "ambient", "WLAmbient"),
	mGlow(18.0f, 0.0f, -0.01f, 1.0f, "glow"),

	// Clouds
	mCloudColor(0.5f, 0.5f, 0.5f, 1.0f, "cloud_color", "WLCloudColor"),
	mCloudMain(0.5f, 0.5f, 0.125f, 1.0f, "cloud_pos_density1"),
	mCloudCoverage(0.0f, "cloud_shadow"),
	mCloudDetail(0.0f, 0.0f, 0.0f, 1.0f, "cloud_pos_density2"),
	mDistanceMult(1.0f, "distance_multiplier"),
	mCloudScale(0.42f, "cloud_scale"),

	// sky dome
	mDomeOffset(0.96f),
	mDomeRadius(15000.f)
{
}

LLWLParamManager::~LLWLParamManager()
{
}

void LLWLParamManager::clearParamSetsOfScope(LLWLParamKey::EScope scope)
{
	if (LLWLParamKey::SCOPE_LOCAL == scope)
	{
		LL_WARNS("Windlight") << "Tried to clear windlight sky presets from local system!  This shouldn't be called..." << LL_ENDL;
		return;
	}

	std::set<LLWLParamKey> to_remove;
	for(std::map<LLWLParamKey, LLWLParamSet>::iterator iter = mParamList.begin(); iter != mParamList.end(); ++iter)
	{
		if(iter->first.scope == scope)
		{
			to_remove.insert(iter->first);
		}
	}

	for(std::set<LLWLParamKey>::iterator iter = to_remove.begin(); iter != to_remove.end(); ++iter)
	{
		mParamList.erase(*iter);
	}
}

// returns all skies referenced by the day cycle, with their final names
// side effect: applies changes to all internal structures!
std::map<LLWLParamKey, LLWLParamSet> LLWLParamManager::finalizeFromDayCycle(LLWLParamKey::EScope scope)
{
	lldebugs << "mDay before finalizing:" << llendl;
	{
		for (std::map<F32, LLWLParamKey>::iterator iter = mDay.mTimeMap.begin(); iter != mDay.mTimeMap.end(); ++iter)
		{
			LLWLParamKey& key = iter->second;
			lldebugs << iter->first << "->" << key.name << llendl;
		}
	}

	std::map<LLWLParamKey, LLWLParamSet> final_references;

	// Move all referenced to desired scope, renaming if necessary
	// First, save skies referenced
	std::map<LLWLParamKey, LLWLParamSet> current_references; // all skies referenced by the day cycle, with their current names
	// guard against skies with same name and different scopes
	std::set<std::string> inserted_names;
	std::map<std::string, unsigned int> conflicted_names; // integer later used as a count, for uniquely renaming conflicts

	LLWLDayCycle& cycle = mDay;
	for(std::map<F32, LLWLParamKey>::iterator iter = cycle.mTimeMap.begin();
		iter != cycle.mTimeMap.end();
		++iter)
	{
		LLWLParamKey& key = iter->second;
		std::string desired_name = key.name;
		replace_newlines_with_whitespace(desired_name); // already shouldn't have newlines, but just in case
		if(inserted_names.find(desired_name) == inserted_names.end())
		{
			inserted_names.insert(desired_name);
		}
		else
		{
			// make exist in map
			conflicted_names[desired_name] = 0;
		}
		current_references[key] = mParamList[key];
	}

	// forget all old skies in target scope, and rebuild, renaming as needed
	clearParamSetsOfScope(scope);
	for(std::map<LLWLParamKey, LLWLParamSet>::iterator iter = current_references.begin(); iter != current_references.end(); ++iter)
	{
		const LLWLParamKey& old_key = iter->first;

		std::string desired_name(old_key.name);
		replace_newlines_with_whitespace(desired_name);

		LLWLParamKey new_key(desired_name, scope); // name will be replaced later if necessary

		// if this sky is one with a non-unique name, rename via appending a number
		// an existing preset of the target scope gets to keep its name
		if (scope != old_key.scope && conflicted_names.find(desired_name) != conflicted_names.end())
		{
			std::string& new_name = new_key.name;

			do
			{
				// if this executes more than once, this is an absurdly pathological case
				// (e.g. "x" repeated twice, but "x 1" already exists, so need to use "x 2")
				std::stringstream temp;
				temp << desired_name << " " << (++conflicted_names[desired_name]);
				new_name = temp.str();
			} while (inserted_names.find(new_name) != inserted_names.end());

			// yay, found one that works
			inserted_names.insert(new_name); // track names we consume here; shouldn't be necessary due to ++int? but just in case

			// *TODO factor out below into a rename()?

			LL_INFOS("Windlight") << "Renamed " << old_key.name << " (scope" << old_key.scope << ") to "
				<< new_key.name << " (scope " << new_key.scope << ")" << LL_ENDL;

			// update name in sky
			iter->second.mName = new_name;

			// update keys in day cycle
			for(std::map<F32, LLWLParamKey>::iterator frame = cycle.mTimeMap.begin(); frame != cycle.mTimeMap.end(); ++frame)
			{
				if (frame->second == old_key)
				{
					frame->second = new_key;
				}
			}

			// add to master sky map
			mParamList[new_key] = iter->second;
		}

		final_references[new_key] = iter->second;
	}

	lldebugs << "mDay after finalizing:" << llendl;
	{
		for (std::map<F32, LLWLParamKey>::iterator iter = mDay.mTimeMap.begin(); iter != mDay.mTimeMap.end(); ++iter)
		{
			LLWLParamKey& key = iter->second;
			lldebugs << iter->first << "->" << key.name << llendl;
		}
	}

	return final_references;
}

// static
LLSD LLWLParamManager::createSkyMap(std::map<LLWLParamKey, LLWLParamSet> refs)
{
	LLSD skies = LLSD::emptyMap();
	for(std::map<LLWLParamKey, LLWLParamSet>::iterator iter = refs.begin(); iter != refs.end(); ++iter)
	{
		skies.insert(iter->first.name, iter->second.getAll());
	}
	return skies;
}

void LLWLParamManager::addAllSkies(const LLWLParamKey::EScope scope, const LLSD& sky_presets)
{
	for(LLSD::map_const_iterator iter = sky_presets.beginMap(); iter != sky_presets.endMap(); ++iter)
	{
		LLWLParamSet set;
		set.setAll(iter->second);
		mParamList[LLWLParamKey(iter->first, scope)] = set;
	}
}

void LLWLParamManager::refreshRegionPresets()
{
	// Remove all region sky presets because they may belong to a previously visited region.
	clearParamSetsOfScope(LLEnvKey::SCOPE_REGION);

	// Add all sky presets belonging to the current region.
	addAllSkies(LLEnvKey::SCOPE_REGION, LLEnvManagerNew::instance().getRegionSettings().getSkyMap());
}

void LLWLParamManager::loadPresets(const std::string& file_name)
{
	std::string path_name(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/skies", ""));
	LL_INFOS2("AppInit", "Shaders") << "Loading Default WindLight settings from " << path_name << LL_ENDL;
	
	bool found = true;
	LLDirIterator app_settings_iter(path_name, "*.xml");
	while(found) 
	{
		std::string name;
		found = app_settings_iter.next(name);
		if(found)
		{
			name=name.erase(name.length()-4);

			// bugfix for SL-46920: preventing filenames that break stuff.
			char * curl_str = curl_unescape(name.c_str(), name.size());
			std::string unescaped_name(curl_str);
			curl_free(curl_str);
			curl_str = NULL;

			LL_DEBUGS2("AppInit", "Shaders") << "name: " << name << LL_ENDL;
			loadPreset(LLWLParamKey(unescaped_name, LLWLParamKey::SCOPE_LOCAL),FALSE);
		}
	}

	// And repeat for user presets, note the user presets will modify any system presets already loaded

	std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "windlight/skies", ""));
	LL_INFOS2("AppInit", "Shaders") << "Loading User WindLight settings from " << path_name2 << LL_ENDL;
	
	found = true;
	LLDirIterator user_settings_iter(path_name2, "*.xml");
	while(found) 
	{
		std::string name;
		found = user_settings_iter.next(name);
		if(found)
		{
			name=name.erase(name.length()-4);

			// bugfix for SL-46920: preventing filenames that break stuff.
			char * curl_str = curl_unescape(name.c_str(), name.size());
			std::string unescaped_name(curl_str);
			curl_free(curl_str);
			curl_str = NULL;

			LL_DEBUGS2("AppInit", "Shaders") << "name: " << name << LL_ENDL;
			loadPreset(LLWLParamKey(unescaped_name,LLWLParamKey::SCOPE_LOCAL),FALSE);
		}
	}

}

// untested and unmaintained!  sanity-check me before using
/*
void LLWLParamManager::savePresets(const std::string & fileName)
{
	//Nobody currently calls me, but if they did, then its reasonable to write the data out to the user's folder
	//and not over the RO system wide version.

	LLSD paramsData(LLSD::emptyMap());
	
	std::string pathName(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "windlight", fileName));

	for(std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = mParamList.begin();
		mIt != mParamList.end();
		++mIt) 
	{
		paramsData[mIt->first.name] = mIt->second.getAll();
	}

	llofstream presetsXML(pathName);

	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();

	formatter->format(paramsData, presetsXML, LLSDFormatter::OPTIONS_PRETTY);

	presetsXML.close();
}
*/

void LLWLParamManager::loadPreset(const LLWLParamKey key, bool propagate)
{
	if(mParamList.find(key) == mParamList.end())			// key does not already exist in mapping
	{
		if(key.scope == LLWLParamKey::SCOPE_LOCAL)			// local scope, so try to load from file
		{
			// bugfix for SL-46920: preventing filenames that break stuff.
			char * curl_str = curl_escape(key.name.c_str(), key.name.size());
			std::string escaped_filename(curl_str);
			curl_free(curl_str);
			curl_str = NULL;

			escaped_filename += ".xml";

			std::string pathName(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/skies", escaped_filename));
			llinfos << "Loading WindLight sky setting from " << pathName << llendl;

			llifstream presetsXML;
			presetsXML.open(pathName.c_str());

			// That failed, try loading from the users area instead.
			if(!presetsXML)
			{
				pathName=gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "windlight/skies", escaped_filename);
				llinfos << "Loading User WindLight sky setting from " << pathName << llendl;
				presetsXML.open(pathName.c_str());
			}

			if (presetsXML)
			{
				loadPresetFromXML(key, presetsXML);
				presetsXML.close();
			} 
			else 
			{
				llwarns << "Could not load local WindLight sky setting " << key.toString() << llendl;
				return;
			}
		}
		else
		{
			llwarns << "Attempted to load non-local WindLight sky settings " << key.toString() << "; not found in parameter mapping." << llendl;
			return;
		}		
	}

	if(propagate)
	{
		getParamSet(key, mCurParams);
		propagateParameters();
	}
}

void LLWLParamManager::loadPresetFromXML(LLWLParamKey key, std::istream & presetsXML)
{
	LLSD paramsData(LLSD::emptyMap());
	LLPointer<LLSDParser> parser = new LLSDXMLParser();

	parser->parse(presetsXML, paramsData, LLSDSerialize::SIZE_UNLIMITED);

	std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = mParamList.find(key);

	if(mIt == mParamList.end()) addParamSet(key, paramsData);
	else setParamSet(key, paramsData);
}

void LLWLParamManager::savePreset(LLWLParamKey key)
{
	// bugfix for SL-46920: preventing filenames that break stuff.
	char * curl_str = curl_escape(key.name.c_str(), key.name.size());
	std::string escaped_filename(curl_str);
	curl_free(curl_str);
	curl_str = NULL;

	escaped_filename += ".xml";

	// make an empty llsd
	LLSD paramsData(LLSD::emptyMap());
	std::string pathName(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "windlight/skies", escaped_filename));

	// fill it with LLSD windlight params
	paramsData = mParamList[key].getAll();

	// write to file
	llofstream presetsXML(pathName);
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(paramsData, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
	presetsXML.close();

	propagateParameters();
}

void LLWLParamManager::updateShaderUniforms(LLGLSLShader * shader)
{
	if (gPipeline.canUseWindLightShaders())
	{
		mCurParams.update(shader);
	}

	if (shader->mShaderGroup == LLGLSLShader::SG_DEFAULT)
	{
		shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, mRotatedLightDir.mV);
		shader->uniform3fv("camPosLocal", 1, LLViewerCamera::getInstance()->getOrigin().mV);
	} 

	else if (shader->mShaderGroup == LLGLSLShader::SG_SKY)
	{
		shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, mClampedLightDir.mV);
	}

	shader->uniform1f("scene_light_strength", mSceneLightStrength);
	
}

static LLFastTimer::DeclareTimer FTM_UPDATE_WLPARAM("Update Windlight Params");

void LLWLParamManager::propagateParameters(void)
{
	LLFastTimer ftm(FTM_UPDATE_WLPARAM);
	
	LLVector4 sunDir;
	LLVector4 moonDir;

	// set the sun direction from SunAngle and EastAngle
	F32 sinTheta = sin(mCurParams.getEastAngle());
	F32 cosTheta = cos(mCurParams.getEastAngle());

	F32 sinPhi = sin(mCurParams.getSunAngle());
	F32 cosPhi = cos(mCurParams.getSunAngle());

	sunDir.mV[0] = -sinTheta * cosPhi;
	sunDir.mV[1] = sinPhi;
	sunDir.mV[2] = cosTheta * cosPhi;
	sunDir.mV[3] = 0;

	moonDir = -sunDir;

	// is the normal from the sun or the moon
	if(sunDir.mV[1] >= 0)
	{
		mLightDir = sunDir;
	}
	else if(sunDir.mV[1] < 0 && sunDir.mV[1] > LLSky::NIGHTTIME_ELEVATION_COS)
	{
		// clamp v1 to 0 so sun never points up and causes weirdness on some machines
		LLVector3 vec(sunDir.mV[0], sunDir.mV[1], sunDir.mV[2]);
		vec.mV[1] = 0;
		vec.normVec();
		mLightDir = LLVector4(vec, 0.f);
	}
	else
	{
		mLightDir = moonDir;
	}

	// calculate the clamp lightnorm for sky (to prevent ugly banding in sky
	// when haze goes below the horizon
	mClampedLightDir = sunDir;

	if (mClampedLightDir.mV[1] < -0.1f)
	{
		mClampedLightDir.mV[1] = -0.1f;
	}

	mCurParams.set("lightnorm", mLightDir);

	// bind the variables for all shaders only if we're using WindLight
	LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
	end_shaders = LLViewerShaderMgr::instance()->endShaders();
	for(shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter) 
	{
		if (shaders_iter->mProgramObject != 0
			&& (gPipeline.canUseWindLightShaders()
				|| shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER))
		{
			shaders_iter->mUniformsDirty = TRUE;
		}
	}

	// get the cfr version of the sun's direction
	LLVector3 cfrSunDir(sunDir.mV[2], sunDir.mV[0], sunDir.mV[1]);

	// set direction and don't allow overriding
	gSky.setSunDirection(cfrSunDir, LLVector3(0,0,0));
	gSky.setOverrideSun(TRUE);
}

void LLWLParamManager::update(LLViewerCamera * cam)
{
	LLFastTimer ftm(FTM_UPDATE_WLPARAM);
	
	// update clouds, sun, and general
	mCurParams.updateCloudScrolling();
	
	// update only if running
	if(mAnimator.getIsRunning()) 
	{
		mAnimator.update(mCurParams);
	}

	// update the shaders and the menu
	propagateParameters();
	
	// sync menus if they exist
	LLFloaterWindLight* wlfloater = LLFloaterReg::findTypedInstance<LLFloaterWindLight>("env_windlight");
	if (wlfloater)
	{
		wlfloater->syncMenu();
	}
	LLFloaterDayCycle* dlfloater = LLFloaterReg::findTypedInstance<LLFloaterDayCycle>("env_day_cycle");
	if (dlfloater)
	{
		dlfloater->syncMenu();
	}
	LLFloaterEnvSettings* envfloater = LLFloaterReg::findTypedInstance<LLFloaterEnvSettings>("old_env_settings");
	if (envfloater)
	{
		envfloater->syncMenu();
	}

	F32 camYaw = cam->getYaw();

	stop_glerror();

	// *TODO: potential optimization - this block may only need to be
	// executed some of the time.  For example for water shaders only.
	{
		F32 camYawDelta = mSunDeltaYaw * DEG_TO_RAD;
		
		LLVector3 lightNorm3(mLightDir);
		lightNorm3 *= LLQuaternion(-(camYaw + camYawDelta), LLVector3(0.f, 1.f, 0.f));
		mRotatedLightDir = LLVector4(lightNorm3, 0.f);

		LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
		end_shaders = LLViewerShaderMgr::instance()->endShaders();
		for(shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter)
		{
			if (shaders_iter->mProgramObject != 0
				&& (gPipeline.canUseWindLightShaders()
				|| shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER))
			{
				shaders_iter->mUniformsDirty = TRUE;
			}
		}
	}
}

void LLWLParamManager::applyUserPrefs(bool interpolate)
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();

	if (env_mgr.getUseRegionSettings()) // apply region-wide settings
	{
		if (env_mgr.getRegionSettings().getSkyMap().size() == 0)
		{
			applyDefaults();
		}
		else
		{
			// *TODO: Support fixed sky from region.
			LL_DEBUGS("Windlight") << "Applying region sky" << LL_ENDL;

			// Apply region day cycle.
			const LLEnvironmentSettings& region_settings = env_mgr.getRegionSettings();
			applyDayCycleParams(
				region_settings.getWLDayCycle(),
				LLEnvKey::SCOPE_REGION,
				region_settings.getDayTime());
		}
	}
	else // apply user-specified settings
	{
		if (env_mgr.getUseDayCycle())
		{
			if (!env_mgr.useDayCycle(env_mgr.getDayCycleName(), LLEnvKey::SCOPE_LOCAL))
			{
				// *TODO: fix user prefs
				applyDefaults();
			}
		}
		else
		{
			LLWLParamSet param_set;
			std::string sky = env_mgr.getSkyPresetName();

			if (!getParamSet(LLWLParamKey(sky, LLWLParamKey::SCOPE_LOCAL), param_set))
			{
				llwarns << "No sky named " << sky << llendl;
			}
			else
			{
				LL_DEBUGS("Windlight") << "Loading fixed sky " << sky << LL_ENDL;
				applySkyParams(param_set.getAll());
			}
		}
	}
}

void LLWLParamManager::applyDefaults()
{
	LLEnvManagerNew::instance().useDayCycle("Default", LLEnvKey::SCOPE_LOCAL);
}

bool LLWLParamManager::applyDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time)
{
	mDay.loadDayCycle(params, scope);
	resetAnimator(time, true); // set to specified time and start animator
	return true;
}

bool LLWLParamManager::applySkyParams(const LLSD& params)
{
	mAnimator.deactivate();
	mCurParams.setAll(params);
	return true;
}

void LLWLParamManager::resetAnimator(F32 curTime, bool run)
{
	mAnimator.setTrack(mDay.mTimeMap, mDay.mDayRate, 
		curTime, run);

	return;
}

bool LLWLParamManager::addParamSet(const LLWLParamKey& key, LLWLParamSet& param)
{
	// add a new one if not one there already
	std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = mParamList.find(key);
	if(mIt == mParamList.end()) 
	{	
		mParamList[key] = param;
		return true;
	}

	return false;
}

BOOL LLWLParamManager::addParamSet(const LLWLParamKey& key, LLSD const & param)
{
	// add a new one if not one there already
	std::map<LLWLParamKey, LLWLParamSet>::const_iterator finder = mParamList.find(key);
	if(finder == mParamList.end())
	{
		mParamList[key].setAll(param);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool LLWLParamManager::getParamSet(const LLWLParamKey& key, LLWLParamSet& param)
{
	// find it and set it
	std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = mParamList.find(key);
	if(mIt != mParamList.end()) 
	{
		param = mParamList[key];
		param.mName = key.name;
		return true;
	}

	return false;
}

bool LLWLParamManager::setParamSet(const LLWLParamKey& key, LLWLParamSet& param)
{
	mParamList[key] = param;

	return true;
}

bool LLWLParamManager::setParamSet(const LLWLParamKey& key, const LLSD & param)
{
	// quick, non robust (we won't be working with files, but assets) check
	// this might not actually be true anymore....
	if(!param.isMap()) 
	{
		return false;
	}
	
	mParamList[key].setAll(param);

	return true;
}

void LLWLParamManager::removeParamSet(const LLWLParamKey& key, bool delete_from_disk)
{
	// remove from param list
	std::map<LLWLParamKey, LLWLParamSet>::iterator mIt = mParamList.find(key);
	if(mIt != mParamList.end()) 
	{
		mParamList.erase(mIt);
	}
	else
	{
		LL_WARNS("WindLight") << "Unable to delete key " << key.toString() << "; not found." << LL_ENDL;
	}

	mDay.removeReferencesTo(key);

	if(delete_from_disk && key.scope == LLWLParamKey::SCOPE_LOCAL)
	{
		std::string path_name(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "windlight/skies", ""));
		
		// use full curl escaped name
		char * curl_str = curl_escape(key.name.c_str(), key.name.size());
		std::string escaped_name(curl_str);
		curl_free(curl_str);
		curl_str = NULL;
		
		if(gDirUtilp->deleteFilesInDir(path_name, escaped_name + ".xml") < 1)
		{
			LL_WARNS("WindLight") << "Unable to delete key " << key.toString() << " from disk; not found." << LL_ENDL;
		}
	}
}


// virtual static
void LLWLParamManager::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing sky" << LL_ENDL;

	loadPresets(LLStringUtil::null);

	// load the day
	std::string preferred_day = LLEnvManagerNew::instance().getDayCycleName();
	if (!LLDayCycleManager::instance().getPreset(preferred_day, mDay))
	{
		// Fall back to default.
		llwarns << "No day cycle named " << preferred_day << ", falling back to defaults" << llendl;
		mDay.loadDayCycleFromFile("Default.xml");

		// *TODO: Fix user preferences accordingly.
	}

	// *HACK - sets cloud scrolling to what we want... fix this better in the future
	std::string sky = LLEnvManagerNew::instance().getSkyPresetName();
	if (!getParamSet(LLWLParamKey(sky, LLWLParamKey::SCOPE_LOCAL), mCurParams))
	{
		llwarns << "No sky preset named " << sky << ", falling back to defaults" << llendl;
		getParamSet(LLWLParamKey("Default", LLWLParamKey::SCOPE_LOCAL), mCurParams);

		// *TODO: Fix user preferences accordingly.
	}

	// set it to noon
	resetAnimator(0.5, LLEnvManagerNew::instance().getUseDayCycle());

	// but use linden time sets it to what the estate is
	mAnimator.setTimeType(LLWLAnimator::TIME_LINDEN);

	applyUserPrefs(false);
}
