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
	mPresetCombo->setCommitCallback(boost::bind(&LLFloaterDeleteEnvPreset::postPopulate, this));

	getChild<LLButton>("delete")->setCommitCallback(boost::bind(&LLFloaterDeleteEnvPreset::onBtnDelete, this));
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterDeleteEnvPreset::onBtnCancel, this));

	// Listen to presets addition/removal.
	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterDeleteEnvPreset::populateDayCyclesList, this));
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterDeleteEnvPreset::populateSkyPresetsList, this));
	LLWaterParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterDeleteEnvPreset::populateWaterPresetsList, this));

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
		// Don't allow deleting system presets.
		if (LLWaterParamManager::instance().isSystemPreset(preset_name))
		{
			LLNotificationsUtil::add("WLNoEditDefault");
			return;
		}

		confirm_cb = boost::bind(&LLFloaterDeleteEnvPreset::onDeleteWaterPresetConfirmation, this);
	}
	else if (param == "sky")
	{
		// Don't allow deleting presets referenced by local day cycles.
		if (LLDayCycleManager::instance().isSkyPresetReferenced(preset_name))
		{
			LLNotificationsUtil::add("GenericAlert", LLSD().with("MESSAGE", getString("msg_sky_is_referenced")));
			return;
		}

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
	if (mKey.asString() != "water") return;

	mPresetCombo->removeall();

	std::string cur_preset;
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	if (!env_mgr.getUseRegionSettings())
	{
		cur_preset = env_mgr.getWaterPresetName();
	}

	// *TODO: Reload the list when user preferences change.
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	const std::map<std::string, LLWaterParamSet> &water_params_map = water_mgr.mParamList;
	for (std::map<std::string, LLWaterParamSet>::const_iterator it = water_params_map.begin(); it != water_params_map.end(); it++)
	{
		std::string name = it->first;

		// list only user presets
		if (water_mgr.isSystemPreset(name))
		{
			continue;
		}

		bool enabled = (name != cur_preset); // don't allow deleting current preset
		mPresetCombo->add(name, ADD_BOTTOM, enabled);
	}

	postPopulate();
}

void LLFloaterDeleteEnvPreset::populateSkyPresetsList()
{
	if (mKey.asString() != "sky") return;

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

		// list only local user presets
		if (key.scope == LLEnvKey::SCOPE_REGION || sky_mgr.isSystemPreset(key.name))
		{
			continue;
		}

		bool enabled = (key.name != cur_preset);
		mPresetCombo->add(key.name, ADD_BOTTOM, enabled);
	}

	postPopulate();
}

void LLFloaterDeleteEnvPreset::populateDayCyclesList()
{
	if (mKey.asString() != "day_cycle") return;

	mPresetCombo->removeall();

	std::string cur_day;
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	if (!env_mgr.getUseRegionSettings() && env_mgr.getUseDayCycle())
	{
		cur_day = env_mgr.getDayCycleName();
	}

	LLDayCycleManager& day_mgr = LLDayCycleManager::instance();
	const LLDayCycleManager::dc_map_t& map = day_mgr.getPresets();
	for (LLDayCycleManager::dc_map_t::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		const std::string& name = it->first;

		// list only user presets
		if (day_mgr.isSystemPreset(name))
		{
			continue;
		}

		mPresetCombo->add(name, ADD_BOTTOM, name != cur_day);
	}

	postPopulate();
}

void LLFloaterDeleteEnvPreset::postPopulate()
{
	// Handle empty list and empty selection.
	bool has_selection = mPresetCombo->getItemCount() > 1 && mPresetCombo->getSelectedValue().isDefined();

	if (!has_selection)
	{
		mPresetCombo->setLabel(getString("combo_label"));
	}

	getChild<LLButton>("delete")->setEnabled(has_selection);
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

void LLFloaterDeleteEnvPreset::onDeleteWaterPresetConfirmation()
{
	LLWaterParamManager::instance().removeParamSet(mPresetCombo->getValue().asString(), true);
}
