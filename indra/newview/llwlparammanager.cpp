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

#include "llviewershadermgr.h"
#include "llglslshader.h"

#include "curl/curl.h"
#include "llstreamtools.h"

LLWLParamManager::LLWLParamManager() :

	//set the defaults for the controls

	/// Sun Delta Terrain tweak variables.
	mSunDeltaYaw(180.0f),
	mSceneLightStrength(2.0f),
	mWLGamma(1.0f, "gamma"),

	mBlueHorizon(0.25f, 0.25f, 1.0f, 1.0f, "blue_horizon", "WLBlueHorizon"),
	mHazeDensity(1.0f, "haze_density"),
	mBlueDensity(0.25f, 0.25f, 0.25f, 1.0f, "blue_density", "WLBlueDensity"),
	mDensityMult(1.0f, "density_multiplier", 1000),
	mHazeHorizon(1.0f, "haze_horizon"),
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

void LLWLParamManager::loadAllPresets()
{
	// First, load system (coming out of the box) sky presets.
	loadPresetsFromDir(getSysDir());

	// Then load user presets. Note that user day presets will modify any system ones already loaded.
	loadPresetsFromDir(getUserDir());
}

void LLWLParamManager::loadPresetsFromDir(const std::string& dir)
{
	LL_INFOS2("AppInit", "Shaders") << "Loading sky presets from " << dir << LL_ENDL;

	LLDirIterator dir_iter(dir, "*.xml");
	while (1)
	{
		std::string file;
		if (!dir_iter.next(file))
		{
			break; // no more files
		}

		std::string path = gDirUtilp->add(dir, file);
		if (!loadPreset(path))
		{
			llwarns << "Error loading sky preset from " << path << llendl;
		}
	}
}

bool LLWLParamManager::loadPreset(const std::string& path)
{
	llifstream xml_file;
	std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));

	xml_file.open(path.c_str());
	if (!xml_file)
	{
		return false;
	}

	LL_DEBUGS2("AppInit", "Shaders") << "Loading sky " << name << LL_ENDL;

	LLSD params_data;
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED);
	xml_file.close();

	LLWLParamKey key(name, LLEnvKey::SCOPE_LOCAL);
	if (hasParamSet(key))
	{
		setParamSet(key, params_data);
	}
	else
	{
		addParamSet(key, params_data);
	}

	return true;
}

void LLWLParamManager::savePreset(LLWLParamKey key)
{
	llassert(key.scope == LLEnvKey::SCOPE_LOCAL && !key.name.empty());

	// make an empty llsd
	LLSD paramsData(LLSD::emptyMap());
	std::string pathName(getUserDir() + escapeString(key.name) + ".xml");

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
		llassert(!key.name.empty());
		// *TODO: validate params
		mParamList[key] = param;
		mPresetListChangeSignal();
		return true;
	}

	return false;
}

BOOL LLWLParamManager::addParamSet(const LLWLParamKey& key, LLSD const & param)
{
	LLWLParamSet param_set;
	param_set.setAll(param);
	return addParamSet(key, param_set);
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

bool LLWLParamManager::hasParamSet(const LLWLParamKey& key)
{
	LLWLParamSet dummy;
	return getParamSet(key, dummy);
}

bool LLWLParamManager::setParamSet(const LLWLParamKey& key, LLWLParamSet& param)
{
	llassert(!key.name.empty());
	// *TODO: validate params
	mParamList[key] = param;

	return true;
}

bool LLWLParamManager::setParamSet(const LLWLParamKey& key, const LLSD & param)
{
	llassert(!key.name.empty());
	// *TODO: validate params

	// quick, non robust (we won't be working with files, but assets) check
	// this might not actually be true anymore....
	if(!param.isMap()) 
	{
		return false;
	}
	
	LLWLParamSet param_set;
	param_set.setAll(param);
	return setParamSet(key, param_set);
}

void LLWLParamManager::removeParamSet(const LLWLParamKey& key, bool delete_from_disk)
{
	// *NOTE: Removing a sky preset invalidates day cycles that refer to it.

	if (key.scope == LLEnvKey::SCOPE_REGION)
	{
		llwarns << "Removing region skies not supported" << llendl;
		llassert(key.scope == LLEnvKey::SCOPE_LOCAL);
		return;
	}

	// remove from param list
	std::map<LLWLParamKey, LLWLParamSet>::iterator it = mParamList.find(key);
	if (it == mParamList.end())
	{
		LL_WARNS("WindLight") << "No sky preset named " << key.name << LL_ENDL;
		return;
	}

	mParamList.erase(it);
	mDay.removeReferencesTo(key);

	// remove from file system if requested
	if (delete_from_disk)
	{
		std::string path_name(getUserDir());
		std::string escaped_name = escapeString(key.name);

		if(gDirUtilp->deleteFilesInDir(path_name, escaped_name + ".xml") < 1)
		{
			LL_WARNS("WindLight") << "Error removing sky preset " << key.name << " from disk" << LL_ENDL;
		}
	}

	// signal interested parties
	mPresetListChangeSignal();
}

bool LLWLParamManager::isSystemPreset(const std::string& preset_name) const
{
	// *TODO: file system access is excessive here.
	return gDirUtilp->fileExists(getSysDir() + escapeString(preset_name) + ".xml");
}

void LLWLParamManager::getPresetNames(preset_name_list_t& region, preset_name_list_t& user, preset_name_list_t& sys) const
{
	region.clear();
	user.clear();
	sys.clear();

	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = mParamList.begin(); it != mParamList.end(); it++)
	{
		const LLWLParamKey& key = it->first;
		const std::string& name = key.name;

		if (key.scope == LLEnvKey::SCOPE_REGION)
		{
			region.push_back(name);
		}
		else
		{
			if (isSystemPreset(name))
			{
				sys.push_back(name);
			}
			else
			{
				user.push_back(name);
			}
		}
	}
}

void LLWLParamManager::getUserPresetNames(preset_name_list_t& user) const
{
	preset_name_list_t region, sys; // unused
	getPresetNames(region, user, sys);
}

void LLWLParamManager::getPresetKeys(preset_key_list_t& keys) const
{
	keys.clear();

	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = mParamList.begin(); it != mParamList.end(); it++)
	{
		keys.push_back(it->first);
	}
}

boost::signals2::connection LLWLParamManager::setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb)
{
	return mPresetListChangeSignal.connect(cb);
}

// virtual static
void LLWLParamManager::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing sky" << LL_ENDL;

	loadAllPresets();

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

	LLEnvManagerNew::instance().usePrefs();
}

// static
std::string LLWLParamManager::getSysDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/skies", "");
}

// static
std::string LLWLParamManager::getUserDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS , "windlight/skies", "");
}

// static
std::string LLWLParamManager::escapeString(const std::string& str)
{
	// Don't use LLURI::escape() because it doesn't encode '-' characters
	// which may break handling of some system presets like "A-12AM".
	char* curl_str = curl_escape(str.c_str(), str.size());
	std::string escaped_str(curl_str);
	curl_free(curl_str);

	return escaped_str;
}
