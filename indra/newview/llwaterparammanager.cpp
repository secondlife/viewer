/**
 * @file llwaterparammanager.cpp
 * @brief Implementation for the LLWaterParamManager class.
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

#include "llwaterparammanager.h"

#include "llrender.h"

#include "pipeline.h"
#include "llsky.h"

#include "lldiriterator.h"
#include "llfloaterreg.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llcheckboxctrl.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llviewercamera.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llsdserialize.h"

#include "v4math.h"
#include "llviewercontrol.h"
#include "lldrawpoolwater.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llviewerregion.h"

#include "llwlparammanager.h"
#include "llwaterparamset.h"

#include "curl/curl.h"

LLWaterParamManager::LLWaterParamManager() :
	mFogColor(22.f/255.f, 43.f/255.f, 54.f/255.f, 0.0f, 0.0f, "waterFogColor", "WaterFogColor"),
	mFogDensity(4, "waterFogDensity", 2),
	mUnderWaterFogMod(0.25, "underWaterFogMod"),
	mNormalScale(2.f, 2.f, 2.f, "normScale"),
	mFresnelScale(0.5f, "fresnelScale"),
	mFresnelOffset(0.4f, "fresnelOffset"),
	mScaleAbove(0.025f, "scaleAbove"),
	mScaleBelow(0.2f, "scaleBelow"),
	mBlurMultiplier(0.1f, "blurMultiplier"),
	mWave1Dir(.5f, .5f, "wave1Dir"),
	mWave2Dir(.5f, .5f, "wave2Dir"),
	mDensitySliderValue(1.0f),
	mWaterFogKS(1.0f)
{
}

LLWaterParamManager::~LLWaterParamManager()
{
}

void LLWaterParamManager::loadAllPresets()
{
	// First, load system (coming out of the box) water presets.
	loadPresetsFromDir(getSysDir());

	// Then load user presets. Note that user day presets will modify any system ones already loaded.
	loadPresetsFromDir(getUserDir());
}

void LLWaterParamManager::loadPresetsFromDir(const std::string& dir)
{
	LL_INFOS2("AppInit", "Shaders") << "Loading water presets from " << dir << LL_ENDL;

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
			llwarns << "Error loading water preset from " << path << llendl;
		}
	}
}

bool LLWaterParamManager::loadPreset(const std::string& path)
{
	llifstream xml_file;
	std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));

	xml_file.open(path.c_str());
	if (!xml_file)
	{
		return false;
	}

	LL_DEBUGS2("AppInit", "Shaders") << "Loading water " << name << LL_ENDL;

	LLSD params_data;
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	parser->parse(xml_file, params_data, LLSDSerialize::SIZE_UNLIMITED);
	xml_file.close();

	if (hasParamSet(name))
	{
		setParamSet(name, params_data);
	}
	else
	{
		addParamSet(name, params_data);
	}

	return true;
}

void LLWaterParamManager::savePreset(const std::string & name)
{
	llassert(!name.empty());

	// make an empty llsd
	LLSD paramsData(LLSD::emptyMap());
	std::string pathName(getUserDir() + LLURI::escape(name) + ".xml");

	// fill it with LLSD windlight params
	paramsData = mParamList[name].getAll();

	// write to file
	llofstream presetsXML(pathName);
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(paramsData, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
	presetsXML.close();

	propagateParameters();
}

void LLWaterParamManager::propagateParameters(void)
{
	// bind the variables only if we're using shaders
	if(gPipeline.canUseVertexShaders())
	{
		LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
		end_shaders = LLViewerShaderMgr::instance()->endShaders();
		for(shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter)
		{
			if (shaders_iter->mProgramObject != 0
				&& shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER)
			{
				shaders_iter->mUniformsDirty = TRUE;
			}
		}
	}

	bool err;
	F32 fog_density_slider = 
		log(mCurParams.getFloat(mFogDensity.mName, err)) / 
		log(mFogDensity.mBase);

	setDensitySliderValue(fog_density_slider);
}

void LLWaterParamManager::updateShaderUniforms(LLGLSLShader * shader)
{
	if (shader->mShaderGroup == LLGLSLShader::SG_WATER)
	{
		shader->uniform4fv(LLViewerShaderMgr::LIGHTNORM, 1, LLWLParamManager::getInstance()->getRotatedLightDir().mV);
		shader->uniform3fv("camPosLocal", 1, LLViewerCamera::getInstance()->getOrigin().mV);
		shader->uniform4fv("waterFogColor", 1, LLDrawPoolWater::sWaterFogColor.mV);
		shader->uniform1f("waterFogEnd", LLDrawPoolWater::sWaterFogEnd);
		shader->uniform4fv("waterPlane", 1, mWaterPlane.mV);
		shader->uniform1f("waterFogDensity", getFogDensity());
		shader->uniform1f("waterFogKS", mWaterFogKS);
		shader->uniform1f("distance_multiplier", 0);
	}
}

void LLWaterParamManager::applyParams(const LLSD& params, bool interpolate)
{
	if (params.size() == 0)
	{
		llwarns << "Undefined water params" << llendl;
		return;
	}

	if (interpolate)
	{
		LLWLParamManager::getInstance()->mAnimator.startInterpolation(params);
	}
	else
	{
		mCurParams.setAll(params);
	}
}

static LLFastTimer::DeclareTimer FTM_UPDATE_WATERPARAM("Update Water Params");

void LLWaterParamManager::update(LLViewerCamera * cam)
{
	LLFastTimer ftm(FTM_UPDATE_WATERPARAM);
	
	// update the shaders and the menu
	propagateParameters();
	
	// only do this if we're dealing with shaders
	if(gPipeline.canUseVertexShaders()) 
	{
		//transform water plane to eye space
		glh::vec3f norm(0.f, 0.f, 1.f);
		glh::vec3f p(0.f, 0.f, gAgent.getRegion()->getWaterHeight()+0.1f);
		
		F32 modelView[16];
		for (U32 i = 0; i < 16; i++)
		{
			modelView[i] = (F32) gGLModelView[i];
		}

		glh::matrix4f mat(modelView);
		glh::matrix4f invtrans = mat.inverse().transpose();
		glh::vec3f enorm;
		glh::vec3f ep;
		invtrans.mult_matrix_vec(norm, enorm);
		enorm.normalize();
		mat.mult_matrix_vec(p, ep);

		mWaterPlane = LLVector4(enorm.v[0], enorm.v[1], enorm.v[2], -ep.dot(enorm));

		LLVector3 sunMoonDir;
		if (gSky.getSunDirection().mV[2] > LLSky::NIGHTTIME_ELEVATION_COS) 	 
		{ 	 
			sunMoonDir = gSky.getSunDirection(); 	 
		} 	 
		else  	 
		{ 	 
			sunMoonDir = gSky.getMoonDirection(); 	 
		}
		sunMoonDir.normVec();
		mWaterFogKS = 1.f/llmax(sunMoonDir.mV[2], WATER_FOG_LIGHT_CLAMP);

		LLViewerShaderMgr::shader_iter shaders_iter, end_shaders;
		end_shaders = LLViewerShaderMgr::instance()->endShaders();
		for(shaders_iter = LLViewerShaderMgr::instance()->beginShaders(); shaders_iter != end_shaders; ++shaders_iter)
		{
			if (shaders_iter->mProgramObject != 0
				&& shaders_iter->mShaderGroup == LLGLSLShader::SG_WATER)
			{
				shaders_iter->mUniformsDirty = TRUE;
			}
		}
	}
}

bool LLWaterParamManager::addParamSet(const std::string& name, LLWaterParamSet& param)
{
	// add a new one if not one there already
	preset_map_t::iterator mIt = mParamList.find(name);
	if(mIt == mParamList.end()) 
	{	
		mParamList[name] = param;
		mPresetListChangeSignal();
		return true;
	}

	return false;
}

BOOL LLWaterParamManager::addParamSet(const std::string& name, LLSD const & param)
{
	LLWaterParamSet param_set;
	param_set.setAll(param);
	return addParamSet(name, param_set);
}

bool LLWaterParamManager::getParamSet(const std::string& name, LLWaterParamSet& param)
{
	// find it and set it
	preset_map_t::iterator mIt = mParamList.find(name);
	if(mIt != mParamList.end()) 
	{
		param = mParamList[name];
		param.mName = name;
		return true;
	}

	return false;
}

bool LLWaterParamManager::hasParamSet(const std::string& name)
{
	LLWaterParamSet dummy;
	return getParamSet(name, dummy);
}

bool LLWaterParamManager::setParamSet(const std::string& name, LLWaterParamSet& param)
{
	mParamList[name] = param;

	return true;
}

bool LLWaterParamManager::setParamSet(const std::string& name, const LLSD & param)
{
	// quick, non robust (we won't be working with files, but assets) check
	if(!param.isMap()) 
	{
		return false;
	}
	
	mParamList[name].setAll(param);

	return true;
}

bool LLWaterParamManager::removeParamSet(const std::string& name, bool delete_from_disk)
{
	// remove from param list
	preset_map_t::iterator it = mParamList.find(name);
	if (it == mParamList.end())
	{
		LL_WARNS("WindLight") << "No water preset named " << name << LL_ENDL;
		return false;
	}

	mParamList.erase(it);

	// remove from file system if requested
	if (delete_from_disk)
	{
		if (gDirUtilp->deleteFilesInDir(getUserDir(), LLURI::escape(name) + ".xml") < 1)
		{
			LL_WARNS("WindLight") << "Error removing water preset " << name << " from disk" << LL_ENDL;
		}
	}

	// signal interested parties
	mPresetListChangeSignal();
	return true;
}

bool LLWaterParamManager::isSystemPreset(const std::string& preset_name) const
{
	// *TODO: file system access is excessive here.
	return gDirUtilp->fileExists(getSysDir() + LLURI::escape(preset_name) + ".xml");
}

void LLWaterParamManager::getPresetNames(preset_name_list_t& presets) const
{
	presets.clear();

	for (preset_map_t::const_iterator it = mParamList.begin(); it != mParamList.end(); ++it)
	{
		presets.push_back(it->first);
	}
}

void LLWaterParamManager::getPresetNames(preset_name_list_t& user_presets, preset_name_list_t& system_presets) const
{
	user_presets.clear();
	system_presets.clear();

	for (preset_map_t::const_iterator it = mParamList.begin(); it != mParamList.end(); ++it)
	{
		if (isSystemPreset(it->first))
		{
			system_presets.push_back(it->first);
		}
		else
		{
			user_presets.push_back(it->first);
		}
	}
}

void LLWaterParamManager::getUserPresetNames(preset_name_list_t& user_presets) const
{
	preset_name_list_t dummy;
	getPresetNames(user_presets, dummy);
}

boost::signals2::connection LLWaterParamManager::setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb)
{
	return mPresetListChangeSignal.connect(cb);
}

F32 LLWaterParamManager::getFogDensity(void)
{
	bool err;

	F32 fogDensity = mCurParams.getFloat("waterFogDensity", err);
	
	// modify if we're underwater
	const F32 water_height = gAgent.getRegion() ? gAgent.getRegion()->getWaterHeight() : 0.f;
	F32 camera_height = gAgentCamera.getCameraPositionAgent().mV[2];
	if(camera_height <= water_height)
	{
		// raise it to the underwater fog density modifier
		fogDensity = pow(fogDensity, mCurParams.getFloat("underWaterFogMod", err));
	}

	return fogDensity;
}

// virtual static
void LLWaterParamManager::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing water" << LL_ENDL;
	loadAllPresets();
	LLEnvManagerNew::instance().usePrefs();
}

// static
std::string LLWaterParamManager::getSysDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/water", "");
}

// static
std::string LLWaterParamManager::getUserDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS , "windlight/water", "");
}
