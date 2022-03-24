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

    LLEnvironment::instance().setWaterListChange(boost::bind(&LLFloaterEditWater::onWaterPresetListChange, this));

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
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
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

    std::string name = pwater->getName();
    mWaterPresetNameEditor->setText(name);
    mWaterPresetCombo->setValue(name);

	//getChild<LLUICtrl>("WaterGlow")->setValue(col.mV[3]);
    getChild<LLColorSwatchCtrl>("WaterFogColor")->set(LLColor4(pwater->getWaterFogColor()));

	// fog and wavelets
    mWaterAdapter->mFogDensity = pwater->getWaterFogDensity();
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
	mWaterPresetCombo->removeall();

    LLEnvironment::list_name_id_t list = LLEnvironment::instance().getWaterList();

    for (LLEnvironment::list_name_id_t::iterator it = list.begin(); it != list.end(); ++it)
    {
        mWaterPresetCombo->add((*it).first, LLSDArray((*it).first)((*it).second));
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

std::string LLFloaterEditWater::getSelectedPresetName() const
{
    std::string name;
	if (mWaterPresetNameEditor->getVisible())
	{
		name = mWaterPresetNameEditor->getText();
	}
	else
	{
		LLSD combo_val = mWaterPresetCombo->getValue();
		name = combo_val[0].asString();
	}

    return name;
}

void LLFloaterEditWater::onWaterPresetNameEdited()
{
    std::string name = mWaterPresetNameEditor->getText();
    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();

    pwater->setName(name);
#if 0
	// Disable saving a water preset having empty name.
	mSaveButton->setEnabled(!getCurrentPresetName().empty());
#endif
}

void LLFloaterEditWater::onWaterPresetSelected()
{
	std::string name;

	name = getSelectedPresetName();

    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().findWaterByName(name);

    if (!pwater)
    {
        LL_WARNS("WATEREDIT") << "Could not find water preset" << LL_ENDL;
        enableEditing(false);
        return;
    }

    pwater = pwater->buildClone();
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, pwater);
    mEditSettings = pwater;

    syncControls();
    enableEditing(true);
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
    std::string name = mEditSettings->getName();

	LL_DEBUGS("Windlight") << "Saving sky preset " << name << LL_ENDL;

    LLEnvironment::instance().addWater(mEditSettings);

	// Change preference if requested.
	if (mMakeDefaultCheckBox->getEnabled() && mMakeDefaultCheckBox->getValue())
	{
		LL_DEBUGS("Windlight") << name << " is now the new preferred water preset" << LL_ENDL;
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mEditSettings);
	}

	closeFloater();
}

void LLFloaterEditWater::onBtnSave()
{
    LLEnvironment::instance().addWater(mEditSettings);
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mEditSettings);

    closeFloater();
}

void LLFloaterEditWater::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditWater::onWaterPresetListChange()
{
    refreshWaterPresetsList();
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
