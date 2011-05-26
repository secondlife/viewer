/** 
 * @file llfloaterdeleteenvpreset.cpp
 * @brief Floater to delete a water / sky / day cycle preset.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterdeleteenvpreset.h"

// libs
#include "llbutton.h"
#include "llcombobox.h"

// newview
#include "lldaycyclemanager.h"
#include "llwaterparammanager.h"

LLFloaterDeleteEnvPreset::LLFloaterDeleteEnvPreset(const LLSD &key)
:	LLFloater(key)
,	mPresetCombo(NULL)
{
}

// virtual
BOOL LLFloaterDeleteEnvPreset::postBuild()
{
	mPresetCombo = getChild<LLComboBox>("preset_combo");

	getChild<LLButton>("delete")->setCommitCallback(boost::bind(&LLFloaterDeleteEnvPreset::onBtnDelete, this));
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterDeleteEnvPreset::onBtnCancel, this));

	// Deletion is not implemented yet, so disable the button for now.
	getChild<LLButton>("delete")->setEnabled(FALSE);

	return TRUE;
}

// virtual
void LLFloaterDeleteEnvPreset::onOpen(const LLSD& key)
{
	std::string param = key.asString();
	std::string floater_title = getString(std::string("title_") + param);
	std::string combo_label = getString(std::string("label_" + param));

	// Update floater title.
	setTitle(floater_title);

	// Update the combobox label.
	getChild<LLUICtrl>("label")->setValue(combo_label);

	// Populate the combobox.
	mPresetCombo->removeall();
	if (param == "water")
	{
		populateWaterPresetsList();
	}
	else if (param == "sky")
	{
		populateSkyPresetsList();
	}
	else if (param == "day_cycle")
	{
		populateDayCyclesList();
	}
	else
	{
		llwarns << "Unrecognized key" << llendl;
	}
}

void LLFloaterDeleteEnvPreset::onBtnDelete()
{
	closeFloater();
}

void LLFloaterDeleteEnvPreset::onBtnCancel()
{
	closeFloater();
}

void LLFloaterDeleteEnvPreset::populateWaterPresetsList()
{
	// *TODO: Reload the list when user preferences change.
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	LL_DEBUGS("Windlight") << "Current water preset: " << water_mgr.mCurParams.mName << LL_ENDL;

	const std::map<std::string, LLWaterParamSet> &water_params_map = water_mgr.mParamList;
	for (std::map<std::string, LLWaterParamSet>::const_iterator it = water_params_map.begin(); it != water_params_map.end(); it++)
	{
		std::string name = it->first;
		bool enabled = (name != water_mgr.mCurParams.mName); // don't allow deleting current preset
		mPresetCombo->add(name, ADD_BOTTOM, enabled);
	}
}

void LLFloaterDeleteEnvPreset::populateSkyPresetsList()
{
	LLWLParamManager& sky_mgr = LLWLParamManager::instance();
	LL_DEBUGS("Windlight") << "Current sky preset: " << sky_mgr.mCurParams.mName << LL_ENDL;

	const std::map<LLWLParamKey, LLWLParamSet> &sky_params_map = sky_mgr.mParamList;
	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = sky_params_map.begin(); it != sky_params_map.end(); it++)
	{
		if (it->first.scope == LLEnvKey::SCOPE_REGION) continue; // list only local presets
		bool enabled = (it->first.name != sky_mgr.mCurParams.mName);
		mPresetCombo->add(it->first.name, ADD_BOTTOM, enabled);
	}
}

void LLFloaterDeleteEnvPreset::populateDayCyclesList()
{
	// *TODO: Disable current day cycle.
	const LLDayCycleManager::dc_map_t& map = LLDayCycleManager::instance().getPresets();
	for (LLDayCycleManager::dc_map_t::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		mPresetCombo->add(it->first);
	}
}
