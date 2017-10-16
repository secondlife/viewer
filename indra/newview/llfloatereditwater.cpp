/** 
 * @file llfloatereditwater.cpp
 * @brief Floater to create or edit a water preset
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

#include "llfloatereditwater.h"

#include <boost/make_shared.hpp>

// libs
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
//#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llsliderctrl.h"
#include "lltexturectrl.h"

// newview
#include "llagent.h"
#include "llregioninfomodel.h"
#include "llviewerregion.h"

#include "llenvironment.h"
#include "llsettingswater.h"
#include "llenvadapters.h"

#include "v3colorutil.h"

#undef max // Fixes a Windows compiler error

LLFloaterEditWater::LLFloaterEditWater(const LLSD &key):	
    LLFloater(key),	
    mWaterPresetNameEditor(NULL),
    mWaterPresetCombo(NULL),
    mMakeDefaultCheckBox(NULL),
    mSaveButton(NULL),
    mWaterAdapter()
{
}

// virtual
BOOL LLFloaterEditWater::postBuild()
{
	mWaterPresetNameEditor = getChild<LLLineEditor>("water_preset_name");
	mWaterPresetCombo = getChild<LLComboBox>("water_preset_combo");
	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");
	mSaveButton = getChild<LLButton>("save");

    mWaterAdapter = boost::make_shared<LLWatterSettingsAdapter>();

	initCallbacks();
	refreshWaterPresetsList();
	syncControls();

	return TRUE;
}

// virtual
void LLFloaterEditWater::onOpen(const LLSD& key)
{
	bool new_preset = isNewPreset();
	std::string param = key.asString();
	std::string floater_title = getString(std::string("title_") + param);
	std::string hint = getString(std::string("hint_" + param));

	// Update floater title.
	setTitle(floater_title);

	// Update the hint at the top.
	getChild<LLUICtrl>("hint")->setValue(hint);

	// Hide the hint to the right of the combo if we're invoked to create a new preset.
	getChildView("note")->setVisible(!new_preset);

	// Switch between the water presets combobox and preset name input field.
	mWaterPresetCombo->setVisible(!new_preset);
	mWaterPresetNameEditor->setVisible(new_preset);

	reset();
}

// virtual
void LLFloaterEditWater::onClose(bool app_quitting)
{
	if (!app_quitting) // there's no point to change environment if we're quitting
	{
//		LLEnvManagerNew::instance().usePrefs(); // revert changes made to current environment
	}
}

// virtual
void LLFloaterEditWater::draw()
{
	syncControls();
	LLFloater::draw();
}

void LLFloaterEditWater::initCallbacks(void)
{
	mWaterPresetNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterEditWater::onWaterPresetNameEdited, this), NULL);
	mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterEditWater::onWaterPresetSelected, this));
	mWaterPresetCombo->setTextEntryCallback(boost::bind(&LLFloaterEditWater::onWaterPresetNameEdited, this));

	mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditWater::onBtnSave, this));
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditWater::onBtnCancel, this));

	// Connect to region info updates.
	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditWater::onRegionInfoUpdate, this));

	//-------------------------------------------------------------------------

    getChild<LLUICtrl>("WaterFogColor")->setCommitCallback(boost::bind(&LLFloaterEditWater::onColorControlMoved, this, _1, &mWaterAdapter->mFogColor));

	// fog density
    getChild<LLUICtrl>("WaterFogDensity")->setCommitCallback(boost::bind(&LLFloaterEditWater::onExpFloatControlMoved, this, _1, &mWaterAdapter->mFogDensity));
    getChild<LLUICtrl>("WaterUnderWaterFogMod")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mUnderWaterFogMod));

	// blue density
    getChild<LLUICtrl>("WaterNormalScaleX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlXMoved, this, _1, &mWaterAdapter->mNormalScale));
    getChild<LLUICtrl>("WaterNormalScaleY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlYMoved, this, _1, &mWaterAdapter->mNormalScale));
    getChild<LLUICtrl>("WaterNormalScaleZ")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlZMoved, this, _1, &mWaterAdapter->mNormalScale));

	// fresnel
    getChild<LLUICtrl>("WaterFresnelScale")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mFresnelScale));
    getChild<LLUICtrl>("WaterFresnelOffset")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mFresnelOffset));

	// scale above/below
    getChild<LLUICtrl>("WaterScaleAbove")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mScaleAbove));
    getChild<LLUICtrl>("WaterScaleBelow")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mScaleBelow));

	// blur mult
    getChild<LLUICtrl>("WaterBlurMult")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &mWaterAdapter->mBlurMultiplier));

	// wave direction
    getChild<LLUICtrl>("WaterWave1DirX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlXMoved, this, _1, &mWaterAdapter->mWave1Dir));
    getChild<LLUICtrl>("WaterWave1DirY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlYMoved, this, _1, &mWaterAdapter->mWave1Dir));
    getChild<LLUICtrl>("WaterWave2DirX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlXMoved, this, _1, &mWaterAdapter->mWave2Dir));
    getChild<LLUICtrl>("WaterWave2DirY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlYMoved, this, _1, &mWaterAdapter->mWave2Dir));

	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("WaterNormalMap");
	texture_ctrl->setDefaultImageAssetID(DEFAULT_WATER_NORMAL);
	texture_ctrl->setCommitCallback(boost::bind(&LLFloaterEditWater::onNormalMapPicked, this, _1));
}

//=============================================================================

void LLFloaterEditWater::syncControls()
{
	// *TODO: Eliminate slow getChild() calls.

    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();
    mEditSettings = pwater;

	//getChild<LLUICtrl>("WaterGlow")->setValue(col.mV[3]);
    getChild<LLColorSwatchCtrl>("WaterFogColor")->set(LLColor4(pwater->getFogColor()));

	// fog and wavelets
    mWaterAdapter->mFogDensity = pwater->getFogDensity();
    getChild<LLUICtrl>("WaterFogDensity")->setValue(mWaterAdapter->mFogDensity.getExp());

    mWaterAdapter->mUnderWaterFogMod = pwater->getFogMod();
	getChild<LLUICtrl>("WaterUnderWaterFogMod")->setValue(static_cast<F32>(mWaterAdapter->mUnderWaterFogMod));

    mWaterAdapter->mNormalScale = pwater->getNormalScale();
    getChild<LLUICtrl>("WaterNormalScaleX")->setValue(mWaterAdapter->mNormalScale.getX());
	getChild<LLUICtrl>("WaterNormalScaleY")->setValue(mWaterAdapter->mNormalScale.getY());
	getChild<LLUICtrl>("WaterNormalScaleZ")->setValue(mWaterAdapter->mNormalScale.getZ());

	// Fresnel
    mWaterAdapter->mFresnelScale = pwater->getFresnelScale();
	getChild<LLUICtrl>("WaterFresnelScale")->setValue(static_cast<F32>(mWaterAdapter->mFresnelScale));
    mWaterAdapter->mFresnelOffset = pwater->getFresnelOffset();
    getChild<LLUICtrl>("WaterFresnelOffset")->setValue(static_cast<F32>(mWaterAdapter->mFresnelOffset));

	// Scale Above/Below
    mWaterAdapter->mScaleAbove = pwater->getScaleAbove();
    getChild<LLUICtrl>("WaterScaleAbove")->setValue(static_cast<F32>(mWaterAdapter->mScaleAbove));
    mWaterAdapter->mScaleBelow = pwater->getScaleBelow();
    getChild<LLUICtrl>("WaterScaleBelow")->setValue(static_cast<F32>(mWaterAdapter->mScaleBelow));

	// blur mult
    mWaterAdapter->mBlurMultiplier = pwater->getBlurMultiplier();
    getChild<LLUICtrl>("WaterBlurMult")->setValue(static_cast<F32>(mWaterAdapter->mBlurMultiplier));

	// wave directions
    mWaterAdapter->mWave1Dir = pwater->getWave1Dir();
	getChild<LLUICtrl>("WaterWave1DirX")->setValue(mWaterAdapter->mWave1Dir.getU());
	getChild<LLUICtrl>("WaterWave1DirY")->setValue(mWaterAdapter->mWave1Dir.getV());

    mWaterAdapter->mWave2Dir = pwater->getWave2Dir();
	getChild<LLUICtrl>("WaterWave2DirX")->setValue(mWaterAdapter->mWave2Dir.getU());
    getChild<LLUICtrl>("WaterWave2DirY")->setValue(mWaterAdapter->mWave2Dir.getV());

	LLTextureCtrl* textCtrl = getChild<LLTextureCtrl>("WaterNormalMap");
	textCtrl->setImageAssetID(pwater->getNormalMapID());
}


// vector control callbacks
void LLFloaterEditWater::onVector3ControlXMoved(LLUICtrl* ctrl, WLVect3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->setX( sldr_ctrl->getValueF32() );
	vector_ctrl->update(mEditSettings);
}

// vector control callbacks
void LLFloaterEditWater::onVector3ControlYMoved(LLUICtrl* ctrl, WLVect3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    vector_ctrl->setY(sldr_ctrl->getValueF32());
    vector_ctrl->update(mEditSettings);
}

// vector control callbacks
void LLFloaterEditWater::onVector3ControlZMoved(LLUICtrl* ctrl, WLVect3Control* vector_ctrl)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    vector_ctrl->setZ(sldr_ctrl->getValueF32());
    vector_ctrl->update(mEditSettings);
}


// vector control callbacks
void LLFloaterEditWater::onVector2ControlXMoved(LLUICtrl* ctrl, WLVect2Control* vector_ctrl)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    vector_ctrl->setU(sldr_ctrl->getValueF32());
    vector_ctrl->update(mEditSettings);
}

// vector control callbacks
void LLFloaterEditWater::onVector2ControlYMoved(LLUICtrl* ctrl, WLVect2Control* vector_ctrl)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    vector_ctrl->setV(sldr_ctrl->getValueF32());
    vector_ctrl->update(mEditSettings);
}

void LLFloaterEditWater::onFloatControlMoved(LLUICtrl* ctrl, WLFloatControl* floatControl)
{
    LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    floatControl->setValue(sldr_ctrl->getValueF32());
    floatControl->update(mEditSettings);
}

void LLFloaterEditWater::onExpFloatControlMoved(LLUICtrl* ctrl, WLXFloatControl* expFloatControl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

    expFloatControl->setExp(sldr_ctrl->getValueF32());
    expFloatControl->update(mEditSettings);
}

void LLFloaterEditWater::onColorControlMoved(LLUICtrl* ctrl, WLColorControl* color_ctrl)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	color_ctrl->setColor4( swatch->get() );
	color_ctrl->update(mEditSettings);
}

void LLFloaterEditWater::onNormalMapPicked(LLUICtrl* ctrl)
{
	LLTextureCtrl* textCtrl = static_cast<LLTextureCtrl*>(ctrl);
	LLUUID textID = textCtrl->getImageAssetID();
    mEditSettings->setNormalMapID(textID);
}

//=============================================================================

void LLFloaterEditWater::reset()
{
	if (isNewPreset())
	{
		mWaterPresetNameEditor->setValue(LLSD());
		mSaveButton->setEnabled(FALSE); // will be enabled as soon as users enters a name
	}
	else
	{
		refreshWaterPresetsList();

		// Disable controls until a water preset to edit is selected.
		enableEditing(false);
	}
}

bool LLFloaterEditWater::isNewPreset() const
{
	return mKey.asString() == "new";
}

void LLFloaterEditWater::refreshWaterPresetsList()
{
#if 0
	mWaterPresetCombo->removeall();

#if 0 // *TODO: enable when we have a clear workflow to edit existing region environment
	// If the region already has water params, add them to the list.
	const LLEnvironmentSettings& region_settings = LLEnvManagerNew::instance().getRegionSettings();
	if (region_settings.getWaterParams().size() != 0)
	{
		const std::string& region_name = gAgent.getRegion()->getName();
		mWaterPresetCombo->add(region_name, LLSD().with(0, region_name).with(1, LLEnvKey::SCOPE_REGION));
		mWaterPresetCombo->addSeparator();
	}
#endif

	std::list<std::string> user_presets, system_presets;
	LLWaterParamManager::instance().getPresetNames(user_presets, system_presets);

	// Add local user presets first.
	for (std::list<std::string>::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		const std::string& name = *it;
		mWaterPresetCombo->add(name, LLSD().with(0, name).with(1, LLEnvKey::SCOPE_LOCAL)); // [<name>, <scope>]
	}

	if (user_presets.size() > 0)
	{
		mWaterPresetCombo->addSeparator();
	}

	// Add local system presets.
	for (std::list<std::string>::const_iterator it = system_presets.begin(); it != system_presets.end(); ++it)
	{
		const std::string& name = *it;
		mWaterPresetCombo->add(name, LLSD().with(0, name).with(1, LLEnvKey::SCOPE_LOCAL)); // [<name>, <scope>]
	}

	mWaterPresetCombo->setLabel(getString("combo_label"));
#endif
}

void LLFloaterEditWater::enableEditing(bool enable)
{
	// Enable/disable water controls.
	getChild<LLPanel>("panel_water_preset")->setCtrlsEnabled(enable);

	// Enable/disable saving.
	mSaveButton->setEnabled(enable);
	mMakeDefaultCheckBox->setEnabled(enable);
}

void LLFloaterEditWater::saveRegionWater()
{
#if 0
	llassert(getCurrentScope() == LLEnvKey::SCOPE_REGION); // make sure we're editing region water

	LL_DEBUGS("Windlight") << "Saving region water preset" << LL_ENDL;

	//LLWaterParamSet region_water = water_mgr.mCurParams;

	// *TODO: save to cached region settings.
	LL_WARNS("Windlight") << "Saving region water is not fully implemented yet" << LL_ENDL;
#endif
}

#if 0
std::string LLFloaterEditWater::getCurrentPresetName() const
{
	std::string name;
	LLEnvKey::EScope scope;
	getSelectedPreset(name, scope);
	return name;
}
#endif

#if 0
LLEnvKey::EScope LLFloaterEditWater::getCurrentScope() const
{
	std::string name;
	LLEnvKey::EScope scope;
	getSelectedPreset(name, scope);
	return scope;
}
#endif

#if 0
void LLFloaterEditWater::getSelectedPreset(std::string& name, LLEnvKey::EScope& scope) const
{

	if (mWaterPresetNameEditor->getVisible())
	{
		name = mWaterPresetNameEditor->getText();
		scope = LLEnvKey::SCOPE_LOCAL;
	}
	else
	{
		LLSD combo_val = mWaterPresetCombo->getValue();

		if (!combo_val.isArray()) // manually typed text
		{
			name = combo_val.asString();
			scope = LLEnvKey::SCOPE_LOCAL;
		}
		else
		{
			name = combo_val[0].asString();
			scope = (LLEnvKey::EScope) combo_val[1].asInteger();
		}
	}

}
#endif

void LLFloaterEditWater::onWaterPresetNameEdited()
{
#if 0
	// Disable saving a water preset having empty name.
	mSaveButton->setEnabled(!getCurrentPresetName().empty());
#endif
}

void LLFloaterEditWater::onWaterPresetSelected()
{
#if 0
	LLWaterParamSet water_params;
	std::string name;
	LLEnvKey::EScope scope;

	getSelectedPreset(name, scope);

	// Display selected preset.
	if (scope == LLEnvKey::SCOPE_REGION)
	{
		water_params.setAll(LLEnvManagerNew::instance().getRegionSettings().getWaterParams());
	}
	else // local preset selected
	{
		if (!LLWaterParamManager::instance().getParamSet(name, water_params))
		{
			// Manually entered string?
			LL_WARNS("Windlight") << "No water preset named " << name << LL_ENDL;
			return;
		}
	}

	LLEnvManagerNew::instance().useWaterParams(water_params.getAll());

	bool can_edit = (scope == LLEnvKey::SCOPE_LOCAL || LLEnvManagerNew::canEditRegionSettings());
	enableEditing(can_edit);

	mMakeDefaultCheckBox->setEnabled(scope == LLEnvKey::SCOPE_LOCAL);
#endif
}

bool LLFloaterEditWater::onSaveAnswer(const LLSD& notification, const LLSD& response)
{
#if 0
  	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// If they choose save, do it.  Otherwise, don't do anything
	if (option == 0)
	{
		onSaveConfirmed();
	}
#endif
	return false;
}

void LLFloaterEditWater::onSaveConfirmed()
{
#if 0
	// Save currently displayed water params to the selected preset.
	std::string name = getCurrentPresetName();

	LL_DEBUGS("Windlight") << "Saving sky preset " << name << LL_ENDL;
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	if (water_mgr.hasParamSet(name))
	{
		water_mgr.setParamSet(name, water_mgr.mCurParams);
	}
	else
	{
		water_mgr.addParamSet(name, water_mgr.mCurParams);
	}

	water_mgr.savePreset(name);

	// Change preference if requested.
	if (mMakeDefaultCheckBox->getEnabled() && mMakeDefaultCheckBox->getValue())
	{
		LL_DEBUGS("Windlight") << name << " is now the new preferred water preset" << LL_ENDL;
		LLEnvManagerNew::instance().setUseWaterPreset(name);
	}

	closeFloater();
#endif
}

void LLFloaterEditWater::onBtnSave()
{
#if 0
	LLEnvKey::EScope scope;
	std::string name;
	getSelectedPreset(name, scope);

	if (scope == LLEnvKey::SCOPE_REGION)
	{
		saveRegionWater();
		closeFloater();
		return;
	}

	if (name.empty())
	{
		// *TODO: show an alert
		LL_WARNS() << "Empty water preset name" << LL_ENDL;
		return;
	}

	// Don't allow overwriting system presets.
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	if (water_mgr.isSystemPreset(name))
	{
		LLNotificationsUtil::add("WLNoEditDefault");
		return;
	}

	// Save, ask for confirmation for overwriting an existing preset.
	if (water_mgr.hasParamSet(name))
	{
		LLNotificationsUtil::add("WLSavePresetAlert", LLSD(), LLSD(), boost::bind(&LLFloaterEditWater::onSaveAnswer, this, _1, _2));
	}
	else
	{
		// new preset, hence no confirmation needed
		onSaveConfirmed();
	}
#endif
}

void LLFloaterEditWater::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditWater::onWaterPresetListChange()
{
#if 0
	std::string name;
	LLEnvKey::EScope scope;
	getSelectedPreset(name, scope); // preset being edited

	if (scope == LLEnvKey::SCOPE_LOCAL && !LLWaterParamManager::instance().hasParamSet(name))
	{
		// Preset we've been editing doesn't exist anymore. Close the floater.
		closeFloater(false);
	}
	else
	{
		// A new preset has been added.
		// Refresh the presets list, though it may not make sense as the floater is about to be closed.
		refreshWaterPresetsList();
	}
#endif
}

void LLFloaterEditWater::onRegionSettingsChange()
{
#if 0
	// If creating a new preset, don't bother.
	if (isNewPreset())
	{
		return;
	}

	if (getCurrentScope() == LLEnvKey::SCOPE_REGION) // if editing region water
	{
		// reset the floater to its initial state
		reset();

		// *TODO: Notify user?
	}
	else // editing a local preset
	{
		refreshWaterPresetsList();
	}
#endif
}

void LLFloaterEditWater::onRegionInfoUpdate()
{
#if 0
	bool can_edit = true;

	// If we've selected the region water for editing.
	if (getCurrentScope() == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
#endif
}
