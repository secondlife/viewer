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
#include "llnotificationsutil.h"

// newview
#include "lldaycyclemanager.h"
#include "llwaterparammanager.h"

static bool confirmation_callback(const LLSD& notification, const LLSD& response, boost::function<void()> cb)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		cb();
	}
	return false;

}

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

	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterDeleteEnvPreset::onDayCycleListChange, this));

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
	if (param == "water")
	{
		populateWaterPresetsList();
		getChild<LLButton>("delete")->setEnabled(FALSE); // not implemented yet
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
	std::string param = mKey.asString();
	std::string preset_name = mPresetCombo->getValue().asString();
	boost::function<void()> confirm_cb;

	if (param == "water")
	{
		llwarns << "Deleting water presets not implemented" << llendl;
		return;
	}
	else if (param == "sky")
	{
		LLWLParamManager& wl_mgr = LLWLParamManager::instance();

		// Don't allow deleting system presets.
		if (wl_mgr.isSystemPreset(preset_name))
		{
			LLNotificationsUtil::add("WLNoEditDefault");
			return;
		}

		confirm_cb = boost::bind(&LLFloaterDeleteEnvPreset::onDeleteSkyPresetConfirmation, this);
	}
	else if (param == "day_cycle")
	{
		LLDayCycleManager& day_mgr = LLDayCycleManager::instance();

		// Don't allow deleting system presets.
		if (day_mgr.isSystemPreset(preset_name))
		{
			LLNotificationsUtil::add("WLNoEditDefault");
			return;
		}

		confirm_cb = boost::bind(&LLFloaterDeleteEnvPreset::onDeleteDayCycleConfirmation, this);
	}
	else
	{
		llwarns << "Unrecognized key" << llendl;
	}

	LLSD args;
	args["MESSAGE"] = getString("msg_confirm_deletion");
	LLNotificationsUtil::add("GenericAlertYesCancel", args, LLSD(),
		boost::bind(&confirmation_callback, _1, _2, confirm_cb));
}

void LLFloaterDeleteEnvPreset::onBtnCancel()
{
	closeFloater();
}

void LLFloaterDeleteEnvPreset::populateWaterPresetsList()
{
	mPresetCombo->removeall();

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
	mPresetCombo->removeall();

	std::string cur_preset;
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	if (!env_mgr.getUseRegionSettings() && env_mgr.getUseFixedSky())
	{
		cur_preset = env_mgr.getSkyPresetName();
	}

	// *TODO: Reload the list when user preferences change.
	LLWLParamManager& sky_mgr = LLWLParamManager::instance();
	const std::map<LLWLParamKey, LLWLParamSet> &sky_params_map = sky_mgr.mParamList;
	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = sky_params_map.begin(); it != sky_params_map.end(); it++)
	{
		const LLWLParamKey& key = it->first;
		if (key.scope == LLEnvKey::SCOPE_REGION) continue; // list only local presets
		bool enabled = key.name != cur_preset && !sky_mgr.isSystemPreset(key.name);
		mPresetCombo->add(key.name, ADD_BOTTOM, enabled);
	}
}

void LLFloaterDeleteEnvPreset::populateDayCyclesList()
{
	mPresetCombo->removeall();

	// *TODO: Disable current day cycle.
	const LLDayCycleManager::dc_map_t& map = LLDayCycleManager::instance().getPresets();
	for (LLDayCycleManager::dc_map_t::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		mPresetCombo->add(it->first);
	}
}

void LLFloaterDeleteEnvPreset::onDeleteDayCycleConfirmation()
{
	LLDayCycleManager::instance().deletePreset(mPresetCombo->getValue().asString());
}

void LLFloaterDeleteEnvPreset::onDeleteSkyPresetConfirmation()
{
	LLWLParamKey key(mPresetCombo->getValue().asString(), LLEnvKey::SCOPE_LOCAL);
	LLWLParamManager::instance().removeParamSet(key, true);
}

void LLFloaterDeleteEnvPreset::onDayCycleListChange()
{
	populateDayCyclesList();
}
