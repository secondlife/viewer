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
#include "llwaterparammanager.h"

#undef max // Fixes a Windows compiler error

LLFloaterEditWater::LLFloaterEditWater(const LLSD &key)
:	LLFloater(key)
,	mWaterPresetNameEditor(NULL)
,	mWaterPresetCombo(NULL)
,	mMakeDefaultCheckBox(NULL)
,	mSaveButton(NULL)
{
}

// virtual
BOOL LLFloaterEditWater::postBuild()
{
	mWaterPresetNameEditor = getChild<LLLineEditor>("water_preset_name");
	mWaterPresetCombo = getChild<LLComboBox>("water_preset_combo");
	mMakeDefaultCheckBox = getChild<LLCheckBoxCtrl>("make_default_cb");
	mSaveButton = getChild<LLButton>("save");

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
		LLEnvManagerNew::instance().usePrefs(); // revert changes made to current environment
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

	LLEnvManagerNew::instance().setRegionSettingsChangeCallback(boost::bind(&LLFloaterEditWater::onRegionSettingsChange, this));
	LLWaterParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEditWater::onWaterPresetListChange, this));

	// Connect to region info updates.
	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditWater::onRegionInfoUpdate, this));

	//-------------------------------------------------------------------------

	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();

	getChild<LLUICtrl>("WaterFogColor")->setCommitCallback(boost::bind(&LLFloaterEditWater::onWaterFogColorMoved, this, _1, &water_mgr.mFogColor));
	//getChild<LLUICtrl>("WaterGlow")->setCommitCallback(boost::bind(&LLFloaterEditWater::onColorControlAMoved, this, _1, &water_mgr.mFogColor));

	// fog density
	getChild<LLUICtrl>("WaterFogDensity")->setCommitCallback(boost::bind(&LLFloaterEditWater::onExpFloatControlMoved, this, _1, &water_mgr.mFogDensity));
	getChild<LLUICtrl>("WaterUnderWaterFogMod")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mUnderWaterFogMod));

	// blue density
	getChild<LLUICtrl>("WaterNormalScaleX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlXMoved, this, _1, &water_mgr.mNormalScale));
	getChild<LLUICtrl>("WaterNormalScaleY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlYMoved, this, _1, &water_mgr.mNormalScale));
	getChild<LLUICtrl>("WaterNormalScaleZ")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector3ControlZMoved, this, _1, &water_mgr.mNormalScale));

	// fresnel
	getChild<LLUICtrl>("WaterFresnelScale")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mFresnelScale));
	getChild<LLUICtrl>("WaterFresnelOffset")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mFresnelOffset));

	// scale above/below
	getChild<LLUICtrl>("WaterScaleAbove")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mScaleAbove));
	getChild<LLUICtrl>("WaterScaleBelow")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mScaleBelow));

	// blur mult
	getChild<LLUICtrl>("WaterBlurMult")->setCommitCallback(boost::bind(&LLFloaterEditWater::onFloatControlMoved, this, _1, &water_mgr.mBlurMultiplier));

	// wave direction
	getChild<LLUICtrl>("WaterWave1DirX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlXMoved, this, _1, &water_mgr.mWave1Dir));
	getChild<LLUICtrl>("WaterWave1DirY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlYMoved, this, _1, &water_mgr.mWave1Dir));
	getChild<LLUICtrl>("WaterWave2DirX")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlXMoved, this, _1, &water_mgr.mWave2Dir));
	getChild<LLUICtrl>("WaterWave2DirY")->setCommitCallback(boost::bind(&LLFloaterEditWater::onVector2ControlYMoved, this, _1, &water_mgr.mWave2Dir));

	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("WaterNormalMap");
	texture_ctrl->setDefaultImageAssetID(DEFAULT_WATER_NORMAL);
	texture_ctrl->setCommitCallback(boost::bind(&LLFloaterEditWater::onNormalMapPicked, this, _1));
}

//=============================================================================

void LLFloaterEditWater::syncControls()
{
	// *TODO: Eliminate slow getChild() calls.

	bool err;

	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();

	LLWaterParamSet& current_params = water_mgr.mCurParams;

	// blue horizon
	water_mgr.mFogColor = current_params.getVector4(water_mgr.mFogColor.mName, err);

	LLColor4 col = water_mgr.getFogColor();
	//getChild<LLUICtrl>("WaterGlow")->setValue(col.mV[3]);
	col.mV[3] = 1.0f;
	getChild<LLColorSwatchCtrl>("WaterFogColor")->set(col);

	// fog and wavelets
	water_mgr.mFogDensity.mExp =
		log(current_params.getFloat(water_mgr.mFogDensity.mName, err)) /
		log(water_mgr.mFogDensity.mBase);
	water_mgr.setDensitySliderValue(water_mgr.mFogDensity.mExp);
	getChild<LLUICtrl>("WaterFogDensity")->setValue(water_mgr.mFogDensity.mExp);

	water_mgr.mUnderWaterFogMod.mX =
		current_params.getFloat(water_mgr.mUnderWaterFogMod.mName, err);
	getChild<LLUICtrl>("WaterUnderWaterFogMod")->setValue(water_mgr.mUnderWaterFogMod.mX);

	water_mgr.mNormalScale = current_params.getVector3(water_mgr.mNormalScale.mName, err);
	getChild<LLUICtrl>("WaterNormalScaleX")->setValue(water_mgr.mNormalScale.mX);
	getChild<LLUICtrl>("WaterNormalScaleY")->setValue(water_mgr.mNormalScale.mY);
	getChild<LLUICtrl>("WaterNormalScaleZ")->setValue(water_mgr.mNormalScale.mZ);

	// Fresnel
	water_mgr.mFresnelScale.mX = current_params.getFloat(water_mgr.mFresnelScale.mName, err);
	getChild<LLUICtrl>("WaterFresnelScale")->setValue(water_mgr.mFresnelScale.mX);
	water_mgr.mFresnelOffset.mX = current_params.getFloat(water_mgr.mFresnelOffset.mName, err);
	getChild<LLUICtrl>("WaterFresnelOffset")->setValue(water_mgr.mFresnelOffset.mX);

	// Scale Above/Below
	water_mgr.mScaleAbove.mX = current_params.getFloat(water_mgr.mScaleAbove.mName, err);
	getChild<LLUICtrl>("WaterScaleAbove")->setValue(water_mgr.mScaleAbove.mX);
	water_mgr.mScaleBelow.mX = current_params.getFloat(water_mgr.mScaleBelow.mName, err);
	getChild<LLUICtrl>("WaterScaleBelow")->setValue(water_mgr.mScaleBelow.mX);

	// blur mult
	water_mgr.mBlurMultiplier.mX = current_params.getFloat(water_mgr.mBlurMultiplier.mName, err);
	getChild<LLUICtrl>("WaterBlurMult")->setValue(water_mgr.mBlurMultiplier.mX);

	// wave directions
	water_mgr.mWave1Dir = current_params.getVector2(water_mgr.mWave1Dir.mName, err);
	getChild<LLUICtrl>("WaterWave1DirX")->setValue(water_mgr.mWave1Dir.mX);
	getChild<LLUICtrl>("WaterWave1DirY")->setValue(water_mgr.mWave1Dir.mY);

	water_mgr.mWave2Dir = current_params.getVector2(water_mgr.mWave2Dir.mName, err);
	getChild<LLUICtrl>("WaterWave2DirX")->setValue(water_mgr.mWave2Dir.mX);
	getChild<LLUICtrl>("WaterWave2DirY")->setValue(water_mgr.mWave2Dir.mY);

	LLTextureCtrl* textCtrl = getChild<LLTextureCtrl>("WaterNormalMap");
	textCtrl->setImageAssetID(water_mgr.getNormalMapID());
}

// color control callbacks
void LLFloaterEditWater::onColorControlRMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	color_ctrl->mR = sldr_ctrl->getValueF32();

	// move i if it's the max
	if (color_ctrl->mR >= color_ctrl->mG
		&& color_ctrl->mR >= color_ctrl->mB
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mR;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		getChild<LLUICtrl>(name)->setValue(color_ctrl->mR);
	}

	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onColorControlGMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	color_ctrl->mG = sldr_ctrl->getValueF32();

	// move i if it's the max
	if (color_ctrl->mG >= color_ctrl->mR
		&& color_ctrl->mG >= color_ctrl->mB
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mG;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		getChild<LLUICtrl>(name)->setValue(color_ctrl->mG);

	}

	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onColorControlBMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	color_ctrl->mB = sldr_ctrl->getValueF32();

	// move i if it's the max
	if (color_ctrl->mB >= color_ctrl->mR
		&& color_ctrl->mB >= color_ctrl->mG
		&& color_ctrl->mHasSliderName)
	{
		color_ctrl->mI = color_ctrl->mB;
		std::string name = color_ctrl->mSliderName;
		name.append("I");

		getChild<LLUICtrl>(name)->setValue(color_ctrl->mB);
	}

	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onColorControlAMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	color_ctrl->mA = sldr_ctrl->getValueF32();

	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}


void LLFloaterEditWater::onColorControlIMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	color_ctrl->mI = sldr_ctrl->getValueF32();

	// only for sliders where we pass a name
	if (color_ctrl->mHasSliderName)
	{
		// set it to the top
		F32 maxVal = std::max(std::max(color_ctrl->mR, color_ctrl->mG), color_ctrl->mB);
		F32 iVal;

		iVal = color_ctrl->mI;

		// get the names of the other sliders
		std::string rName = color_ctrl->mSliderName;
		rName.append("R");
		std::string gName = color_ctrl->mSliderName;
		gName.append("G");
		std::string bName = color_ctrl->mSliderName;
		bName.append("B");

		// handle if at 0
		if (iVal == 0)
		{
			color_ctrl->mR = 0;
			color_ctrl->mG = 0;
			color_ctrl->mB = 0;

		// if all at the start
		// set them all to the intensity
		}
		else if (maxVal == 0)
		{
			color_ctrl->mR = iVal;
			color_ctrl->mG = iVal;
			color_ctrl->mB = iVal;
		}
		else
		{
			// add delta amounts to each
			F32 delta = (iVal - maxVal) / maxVal;
			color_ctrl->mR *= (1.0f + delta);
			color_ctrl->mG *= (1.0f + delta);
			color_ctrl->mB *= (1.0f + delta);
		}

		// set the sliders to the new vals
		getChild<LLUICtrl>(rName)->setValue(color_ctrl->mR);
		getChild<LLUICtrl>(gName)->setValue(color_ctrl->mG);
		getChild<LLUICtrl>(bName)->setValue(color_ctrl->mB);
	}

	// now update the current parameters and send them to shaders
	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}

// vector control callbacks
void LLFloaterEditWater::onVector3ControlXMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->mX = sldr_ctrl->getValueF32();

	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

// vector control callbacks
void LLFloaterEditWater::onVector3ControlYMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->mY = sldr_ctrl->getValueF32();

	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

// vector control callbacks
void LLFloaterEditWater::onVector3ControlZMoved(LLUICtrl* ctrl, WaterVector3Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->mZ = sldr_ctrl->getValueF32();

	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}


// vector control callbacks
void LLFloaterEditWater::onVector2ControlXMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->mX = sldr_ctrl->getValueF32();

	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

// vector control callbacks
void LLFloaterEditWater::onVector2ControlYMoved(LLUICtrl* ctrl, WaterVector2Control* vector_ctrl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	vector_ctrl->mY = sldr_ctrl->getValueF32();

	vector_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);

	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onFloatControlMoved(LLUICtrl* ctrl, WaterFloatControl* floatControl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	floatControl->mX = sldr_ctrl->getValueF32() / floatControl->mMult;

	floatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onExpFloatControlMoved(LLUICtrl* ctrl, WaterExpFloatControl* expFloatControl)
{
	LLSliderCtrl* sldr_ctrl = static_cast<LLSliderCtrl*>(ctrl);

	F32 val = sldr_ctrl->getValueF32();
	expFloatControl->mExp = val;
	LLWaterParamManager::getInstance()->setDensitySliderValue(val);

	expFloatControl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onWaterFogColorMoved(LLUICtrl* ctrl, WaterColorControl* color_ctrl)
{
	LLColorSwatchCtrl* swatch = static_cast<LLColorSwatchCtrl*>(ctrl);
	*color_ctrl = swatch->get();

	color_ctrl->update(LLWaterParamManager::getInstance()->mCurParams);
	LLWaterParamManager::getInstance()->propagateParameters();
}

void LLFloaterEditWater::onNormalMapPicked(LLUICtrl* ctrl)
{
	LLTextureCtrl* textCtrl = static_cast<LLTextureCtrl*>(ctrl);
	LLUUID textID = textCtrl->getImageAssetID();
	LLWaterParamManager::getInstance()->setNormalMapID(textID);
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
	llassert(getCurrentScope() == LLEnvKey::SCOPE_REGION); // make sure we're editing region water

	LL_DEBUGS("Windlight") << "Saving region water preset" << llendl;

	//LLWaterParamSet region_water = water_mgr.mCurParams;

	// *TODO: save to cached region settings.
	LL_WARNS("Windlight") << "Saving region water is not fully implemented yet" << LL_ENDL;
}

std::string LLFloaterEditWater::getCurrentPresetName() const
{
	std::string name;
	LLEnvKey::EScope scope;
	getSelectedPreset(name, scope);
	return name;
}

LLEnvKey::EScope LLFloaterEditWater::getCurrentScope() const
{
	std::string name;
	LLEnvKey::EScope scope;
	getSelectedPreset(name, scope);
	return scope;
}

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

void LLFloaterEditWater::onWaterPresetNameEdited()
{
	// Disable saving a water preset having empty name.
	mSaveButton->setEnabled(!getCurrentPresetName().empty());
}

void LLFloaterEditWater::onWaterPresetSelected()
{
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
}

bool LLFloaterEditWater::onSaveAnswer(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	// If they choose save, do it.  Otherwise, don't do anything
	if (option == 0)
	{
		onSaveConfirmed();
	}

	return false;
}

void LLFloaterEditWater::onSaveConfirmed()
{
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
		LL_DEBUGS("Windlight") << name << " is now the new preferred water preset" << llendl;
		LLEnvManagerNew::instance().setUseWaterPreset(name);
	}

	closeFloater();
}

void LLFloaterEditWater::onBtnSave()
{
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
		llwarns << "Empty water preset name" << llendl;
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
}

void LLFloaterEditWater::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditWater::onWaterPresetListChange()
{
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
}

void LLFloaterEditWater::onRegionSettingsChange()
{
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
}

void LLFloaterEditWater::onRegionInfoUpdate()
{
	bool can_edit = true;

	// If we've selected the region water for editing.
	if (getCurrentScope() == LLEnvKey::SCOPE_REGION)
	{
		// check whether we have the access
		can_edit = LLEnvManagerNew::canEditRegionSettings();
	}

	enableEditing(can_edit);
}
