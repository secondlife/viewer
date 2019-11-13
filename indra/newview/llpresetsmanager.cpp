/**
 * @file llpresetsmanager.cpp
 * @brief Implementation for the LLPresetsManager class.
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

#include <boost/assign/list_of.hpp>

#include "llpresetsmanager.h"

#include "lldiriterator.h"
#include "llfloater.h"
#include "llsdserialize.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "llfeaturemanager.h"
#include "llagentcamera.h"
#include "llfile.h"

LLPresetsManager::LLPresetsManager()
{
}

LLPresetsManager::~LLPresetsManager()
{
	mCameraChangedSignal.disconnect();
}

void LLPresetsManager::triggerChangeCameraSignal()
{
	mPresetListChangeCameraSignal();
}

void LLPresetsManager::triggerChangeSignal()
{
	mPresetListChangeSignal();
}

void LLPresetsManager::createMissingDefault(const std::string& subdirectory)
{
	if(gDirUtilp->getLindenUserDir().empty())
	{
		return;
	}

	if (PRESETS_CAMERA == subdirectory)
	{
		createCameraDefaultPresets();
		return;
	}

	std::string default_file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR,
		subdirectory, PRESETS_DEFAULT + ".xml");
	if (!gDirUtilp->fileExists(default_file))
	{
		LL_INFOS() << "No default preset found -- creating one at " << default_file << LL_ENDL;

		// Write current settings as the default
		savePreset(subdirectory, PRESETS_DEFAULT, true);
	}
	else
	{
		LL_DEBUGS() << "default preset exists; no-op" << LL_ENDL;
	}
}

void LLPresetsManager::createCameraDefaultPresets()
{
	createDefaultCameraPreset(PRESETS_REAR_VIEW);
	createDefaultCameraPreset(PRESETS_FRONT_VIEW);
	createDefaultCameraPreset(PRESETS_SIDE_VIEW);
}

void LLPresetsManager::startWatching(const std::string& subdirectory)
{
	if (PRESETS_CAMERA == subdirectory)
	{
		std::vector<std::string> name_list;
		getControlNames(name_list);
		getOffsetControlNames(name_list);

		for (std::vector<std::string>::iterator it = name_list.begin(); it != name_list.end(); ++it)
		{
			std::string ctrl_name = *it;
			if (gSavedSettings.controlExists(ctrl_name))
			{
				LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl(ctrl_name);
				if (cntrl_ptr.isNull())
				{
					LL_WARNS("Init") << "Unable to set signal on global setting '" << ctrl_name
						<< "'" << LL_ENDL;
				}
				else
				{
					mCameraChangedSignal = cntrl_ptr->getCommitSignal()->connect(boost::bind(&settingChanged));
				}
			}
		}
	}
}

std::string LLPresetsManager::getPresetsDir(const std::string& subdirectory)
{
	std::string presets_path = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR);

	LLFile::mkdir(presets_path);

	std::string dest_path = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR, subdirectory);
	if (!gDirUtilp->fileExists(dest_path))
		LLFile::mkdir(dest_path);

	if (PRESETS_CAMERA == subdirectory)
	{
		std::string source_dir = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, PRESETS_CAMERA);
		LLDirIterator dir_iter(source_dir, "*.xml");
		bool found = true;
		while (found)
		{
			std::string file;
			found = dir_iter.next(file);

			if (found)
			{
				std::string source = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, PRESETS_CAMERA, file);
				file = LLURI::escape(file);
				std::string dest = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR, PRESETS_CAMERA, file);
				LLFile::copy(source, dest);
			}
		}
	}

	return dest_path;
}

void LLPresetsManager::loadPresetNamesFromDir(const std::string& dir, preset_name_list_t& presets, EDefaultOptions default_option)
{
	LL_INFOS("AppInit") << "Loading list of preset names from " << dir << LL_ENDL;

	mPresetNames.clear();

	LLDirIterator dir_iter(dir, "*.xml");
	bool found = true;
	while (found)
	{
		std::string file;
		found = dir_iter.next(file);

		if (found)
		{
			std::string path = gDirUtilp->add(dir, file);
			std::string name = LLURI::unescape(gDirUtilp->getBaseFileName(path, /*strip_exten = */ true));
			LL_DEBUGS() << "  Found preset '" << name << "'" << LL_ENDL;

			if (isTemplateCameraPreset(name))
			{
				continue;
			}
			if (default_option == DEFAULT_VIEWS_HIDE)
			{
				if (isDefaultCameraPreset(name))
				{
					continue;
				}
			}
			if (PRESETS_DEFAULT != name)
			{
				mPresetNames.push_back(name);
			}
			else
			{
				switch (default_option)
				{
					case DEFAULT_SHOW:
						mPresetNames.push_back(LLTrans::getString(PRESETS_DEFAULT));
						break;

					case DEFAULT_TOP:
						mPresetNames.push_front(LLTrans::getString(PRESETS_DEFAULT));
						break;

					case DEFAULT_HIDE:
					default:
						break;
				}
			}
		}
	}

	presets = mPresetNames;
}

bool LLPresetsManager::mCameraDirty = false;
bool LLPresetsManager::mIgnoreChangedSignal = false;

void LLPresetsManager::setCameraDirty(bool dirty)
{
	mCameraDirty = dirty;
}

bool LLPresetsManager::isCameraDirty()
{
	return mCameraDirty;
}

void LLPresetsManager::settingChanged()
{
	setCameraDirty(true);

	static LLCachedControl<std::string> preset_camera_active(gSavedSettings, "PresetCameraActive", "");
	std::string preset_name = preset_camera_active;
	if (!preset_name.empty() && !mIgnoreChangedSignal)
	{
		gSavedSettings.setString("PresetCameraActive", "");

		// Hack call because this is a static routine
		LLPresetsManager::getInstance()->triggerChangeCameraSignal();
	}
}

void LLPresetsManager::getControlNames(std::vector<std::string>& names)
{
	const std::vector<std::string> camera_controls = boost::assign::list_of
		// From panel_preferences_move.xml
		("CameraAngle")
		("CameraOffsetScale")
		("EditCameraMovement")
		("AppearanceCameraMovement")
		// From llagentcamera.cpp
		("CameraOffsetBuild")
		("CameraOffsetScale")
		("TrackFocusObject")
        ;
    names = camera_controls;
}

void LLPresetsManager::getOffsetControlNames(std::vector<std::string>& names)
{
	const std::vector<std::string> offset_controls = boost::assign::list_of
		("CameraOffsetRearView")
		("CameraOffsetFrontView")
		("CameraOffsetGroupView")
		("CameraOffsetCustomPreset")
		("FocusOffsetRearView")
		("FocusOffsetFrontView")
		("FocusOffsetGroupView")
		("FocusOffsetCustomPreset")
		;
	names.insert(std::end(names), std::begin(offset_controls), std::end(offset_controls));
}

bool LLPresetsManager::savePreset(const std::string& subdirectory, std::string name, bool createDefault)
{
	bool IS_CAMERA = (PRESETS_CAMERA == subdirectory);
	bool IS_GRAPHIC = (PRESETS_GRAPHIC == subdirectory);

	if (LLTrans::getString(PRESETS_DEFAULT) == name)
	{
		name = PRESETS_DEFAULT;
	}
	if (!createDefault && name == PRESETS_DEFAULT)
	{
		LL_WARNS() << "Should not overwrite default" << LL_ENDL;
		return false;
	}

	if (isTemplateCameraPreset(name))
	{
		LL_WARNS() << "Should not overwrite template presets" << LL_ENDL;
		return false;
	}

	bool saved = false;
	std::vector<std::string> name_list;

	if (IS_GRAPHIC)
	{
		LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
		if (instance && !createDefault)
		{
			gSavedSettings.setString("PresetGraphicActive", name);
			instance->getControlNames(name_list);
			LL_DEBUGS() << "saving preset '" << name << "'; " << name_list.size() << " names" << LL_ENDL;
			name_list.push_back("PresetGraphicActive");
		}
		else
        {
			LL_WARNS("Presets") << "preferences floater instance not found" << LL_ENDL;
		}
	}
	else if (IS_CAMERA)
	{
		name_list.clear();
		getControlNames(name_list);
		name_list.push_back("PresetCameraActive");
		name_list.push_back(gAgentCamera.getCameraOffsetCtrlName());
		name_list.push_back(gAgentCamera.getFocusOffsetCtrlName());
	}
	else
	{
		LL_ERRS() << "Invalid presets directory '" << subdirectory << "'" << LL_ENDL;
	}
 
	// make an empty llsd
	LLSD paramsData(LLSD::emptyMap());

	// Create a default graphics preset from hw recommended settings 
	if (IS_GRAPHIC && createDefault && name == PRESETS_DEFAULT)
	{
		paramsData = LLFeatureManager::getInstance()->getRecommendedSettingsMap();
		if (gSavedSettings.getU32("RenderAvatarMaxComplexity") == 0)
		{
			// use the recommended setting as an initial one (MAINT-6435)
			gSavedSettings.setU32("RenderAvatarMaxComplexity", paramsData["RenderAvatarMaxComplexity"]["Value"].asInteger());
		}
	}
	else
	{
		ECameraPreset new_camera_preset = (ECameraPreset)gSavedSettings.getU32("CameraPreset");
		bool new_camera_offsets = false;
		if (IS_CAMERA)
		{
			if (isDefaultCameraPreset(name))
			{
				if (PRESETS_REAR_VIEW == name)
				{
					new_camera_preset = CAMERA_PRESET_REAR_VIEW;
				}
				else if (PRESETS_SIDE_VIEW == name)
				{
					new_camera_preset = CAMERA_PRESET_GROUP_VIEW;
				}
				else if (PRESETS_FRONT_VIEW == name)
				{
					new_camera_preset = CAMERA_PRESET_FRONT_VIEW;
				}
			}
			else 
			{
				new_camera_preset = CAMERA_PRESET_CUSTOM;
			}
			new_camera_offsets = (!isDefaultCameraPreset(name) || (ECameraPreset)gSavedSettings.getU32("CameraPreset") != new_camera_preset);
		}
		for (std::vector<std::string>::iterator it = name_list.begin(); it != name_list.end(); ++it)
		{
			std::string ctrl_name = *it;
			std::string dest_ctrl_name = ctrl_name;
			if (IS_CAMERA && new_camera_offsets)
			{
				if (ctrl_name == gAgentCamera.getCameraOffsetCtrlName())
				{
					dest_ctrl_name = gAgentCamera.getCameraOffsetCtrlName(new_camera_preset);
				}
				if (ctrl_name == gAgentCamera.getFocusOffsetCtrlName())
				{
					dest_ctrl_name = gAgentCamera.getFocusOffsetCtrlName(new_camera_preset);
				}
			}

			LLControlVariable* ctrl = gSavedSettings.getControl(ctrl_name).get();
			if (ctrl)
			{
				std::string comment = ctrl->getComment();
				std::string type = LLControlGroup::typeEnumToString(ctrl->type());
				LLSD value = ctrl->getValue();

				paramsData[dest_ctrl_name]["Comment"] = comment;
				paramsData[dest_ctrl_name]["Persist"] = 1;
				paramsData[dest_ctrl_name]["Type"] = type;
				paramsData[dest_ctrl_name]["Value"] = value;
			}
		}
		if (IS_CAMERA)
		{
			gSavedSettings.setU32("CameraPreset", new_camera_preset);
		}
	}

	std::string pathName(getPresetsDir(subdirectory) + gDirUtilp->getDirDelimiter() + LLURI::escape(name) + ".xml");

 // If the active preset name is the only thing in the list, don't save the list
	if (paramsData.size() > 1)
	{
		// write to file
		llofstream presetsXML(pathName.c_str());
		if (presetsXML.is_open())
		{
			LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
			formatter->format(paramsData, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
			presetsXML.close();
			saved = true;
            
			LL_DEBUGS() << "saved preset '" << name << "'; " << paramsData.size() << " parameters" << LL_ENDL;

			if (IS_GRAPHIC)
			{
				gSavedSettings.setString("PresetGraphicActive", name);
				// signal interested parties
				triggerChangeSignal();
			}

			if (IS_CAMERA)
			{
				gSavedSettings.setString("PresetCameraActive", name);
				setCameraDirty(false);
				// signal interested parties
				triggerChangeCameraSignal();
			}
		}
		else
		{
			LL_WARNS("Presets") << "Cannot open for output preset file " << pathName << LL_ENDL;
		}
	}
    else
	{
		LL_INFOS() << "No settings available to be saved" << LL_ENDL;
	}
    
	return saved;
}

bool LLPresetsManager::setPresetNamesInComboBox(const std::string& subdirectory, LLComboBox* combo, EDefaultOptions default_option)
{
	bool sts = true;

	combo->clearRows();
	combo->setEnabled(TRUE);

	std::string presets_dir = getPresetsDir(subdirectory);

	if (!presets_dir.empty())
	{
		std::list<std::string> preset_names;
		loadPresetNamesFromDir(presets_dir, preset_names, default_option);

		std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");

		if (preset_names.begin() != preset_names.end())
		{
			for (std::list<std::string>::const_iterator it = preset_names.begin(); it != preset_names.end(); ++it)
			{
				const std::string& name = *it;
				combo->add(name, name);
			}
		}
		else
		{
			combo->setLabel(LLTrans::getString("preset_combo_label"));
			combo->setEnabled(FALSE);
			sts = false;
		}
	}
	return sts;
}

void LLPresetsManager::loadPreset(const std::string& subdirectory, std::string name)
{
	if (LLTrans::getString(PRESETS_DEFAULT) == name)
	{
		name = PRESETS_DEFAULT;
	}

	std::string full_path(getPresetsDir(subdirectory) + gDirUtilp->getDirDelimiter() + LLURI::escape(name) + ".xml");

    LL_DEBUGS() << "attempting to load preset '"<<name<<"' from '"<<full_path<<"'" << LL_ENDL;

	mIgnoreChangedSignal = true;
	if(gSavedSettings.loadFromFile(full_path, false, true) > 0)
	{
		mIgnoreChangedSignal = false;
		if(PRESETS_GRAPHIC == subdirectory)
		{
			gSavedSettings.setString("PresetGraphicActive", name);

			LLFloaterPreference* instance = LLFloaterReg::findTypedInstance<LLFloaterPreference>("preferences");
			if (instance)
			{
				instance->refreshEnabledGraphics();
			}
			triggerChangeSignal();
		}
		if(PRESETS_CAMERA == subdirectory)
		{
			gSavedSettings.setString("PresetCameraActive", name);
			triggerChangeCameraSignal();
		}
	}
	else
	{
		mIgnoreChangedSignal = false;
		LL_WARNS("Presets") << "failed to load preset '"<<name<<"' from '"<<full_path<<"'" << LL_ENDL;
	}
}

bool LLPresetsManager::deletePreset(const std::string& subdirectory, std::string name)
{
	if (LLTrans::getString(PRESETS_DEFAULT) == name)
	{
		name = PRESETS_DEFAULT;
	}

	bool sts = true;

	if (PRESETS_DEFAULT == name)
	{
		// This code should never execute
		LL_WARNS("Presets") << "You are not allowed to delete the default preset." << LL_ENDL;
		sts = false;
	}

	if (gDirUtilp->deleteFilesInDir(getPresetsDir(subdirectory), LLURI::escape(name) + ".xml") < 1)
	{
		LL_WARNS("Presets") << "Error removing preset " << name << " from disk" << LL_ENDL;
		sts = false;
	}

	// If you delete the preset that is currently marked as loaded then also indicate that no preset is loaded.
	if(PRESETS_GRAPHIC == subdirectory)
	{
		if (gSavedSettings.getString("PresetGraphicActive") == name)
		{
			gSavedSettings.setString("PresetGraphicActive", "");
		}
		// signal interested parties
		triggerChangeSignal();
	}

	if(PRESETS_CAMERA == subdirectory)
	{
		if (gSavedSettings.getString("PresetCameraActive") == name)
		{
			gSavedSettings.setString("PresetCameraActive", "");
		}
		// signal interested parties
		triggerChangeCameraSignal();
	}

	return sts;
}

bool LLPresetsManager::isDefaultCameraPreset(std::string preset_name)
{
	return (preset_name == PRESETS_REAR_VIEW || preset_name == PRESETS_SIDE_VIEW || preset_name == PRESETS_FRONT_VIEW);
}

bool LLPresetsManager::isTemplateCameraPreset(std::string preset_name)
{
	return (preset_name == PRESETS_REAR || preset_name == PRESETS_SIDE || preset_name == PRESETS_FRONT);
}

void LLPresetsManager::resetCameraPreset(std::string preset_name)
{
	if (isDefaultCameraPreset(preset_name))
	{
		createDefaultCameraPreset(preset_name, true);

		if (gSavedSettings.getString("PresetCameraActive") == preset_name)
		{
			loadPreset(PRESETS_CAMERA, preset_name);
		}
	}
}

void LLPresetsManager::createDefaultCameraPreset(std::string preset_name, bool force_reset)
{
	std::string preset_file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR,
		PRESETS_CAMERA, LLURI::escape(preset_name) + ".xml");
	if (!gDirUtilp->fileExists(preset_file) || force_reset)
	{
		std::string template_name = preset_name.substr(0, preset_name.size() - PRESETS_VIEW_SUFFIX.size());
		std::string default_template_file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, PRESETS_DIR,
			PRESETS_CAMERA, template_name + ".xml");
		LLFile::copy(default_template_file, preset_file);
	}
}

boost::signals2::connection LLPresetsManager::setPresetListChangeCameraCallback(const preset_list_signal_t::slot_type& cb)
{
	return mPresetListChangeCameraSignal.connect(cb);
}

boost::signals2::connection LLPresetsManager::setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb)
{
	return mPresetListChangeSignal.connect(cb);
}
