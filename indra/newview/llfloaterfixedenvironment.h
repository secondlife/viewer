/** 
 * @file llfloaterfixedenvironment.h
 * @brief Floaters to create and edit fixed settings for sky and water.
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

#ifndef LL_FLOATERFIXEDENVIRONMENT_H
#define LL_FLOATERFIXEDENVIRONMENT_H

#include "llfloater.h"
#include "llsettingsbase.h"

class LLTabContainer;
class LLButton;
class LLLineEditor;

/**
 * Floater container for creating and editing fixed environment settings.
 */
class LLFloaterFixedEnvironment : public LLFloater
{
    LOG_CLASS(LLFloaterFixedEnvironment);

public:
                            LLFloaterFixedEnvironment(const LLSD &key);

    virtual BOOL	        postBuild()         override;

    virtual void            onFocusReceived()   override;
    virtual void            onFocusLost()       override;

    void                    setEditSettings(const LLSettingsBase::ptr_t &settings)  { mSettings = settings; syncronizeTabs(); refresh(); }
    LLSettingsBase::ptr_t   getEditSettings()   const                           { return mSettings; }

protected:
    virtual void            updateEditEnvironment() = 0;
    virtual void            refresh()           override;

    virtual void            syncronizeTabs();

    LLTabContainer *        mTab;
    LLLineEditor *          mTxtName;

    LLSettingsBase::ptr_t   mSettings;

    virtual void            doLoadFromInventory() = 0;
    virtual void            doImportFromDisk() = 0;
    virtual void            doApplyFixedSettings() = 0;

    bool                    canUseInventory() const;

private:
    void                    onNameChanged(const std::string &name);

    void                    onButtonLoad();
    void                    onButtonImport();
    void                    onButtonApply();
    void                    onButtonCancel();

#if 0

	/*virtual*/	BOOL	postBuild();
	/*virtual*/ void	onOpen(const LLSD& key);
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ void	draw();


	//-- WL stuff begins ------------------------------------------------------

	void syncControls(); /// sync up sliders with parameters

	void setColorSwatch(const std::string& name, const WLColorControl& from_ctrl, F32 k);

	// general purpose callbacks for dealing with color controllers
	void onColorControlMoved(LLUICtrl* ctrl, WLColorControl* color_ctrl);
	void onColorControlRMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlGMoved(LLUICtrl* ctrl, void* userdata);
	void onColorControlBMoved(LLUICtrl* ctrl, void* userdata);
	void onFloatControlMoved(LLUICtrl* ctrl, void* userdata);

    void adjustIntensity(WLColorControl *ctrl, F32 color, F32 scale);

	// lighting callbacks for glow
	void onGlowRMoved(LLUICtrl* ctrl, void* userdata);
	void onGlowBMoved(LLUICtrl* ctrl, void* userdata);

	// lighting callbacks for sun
	void onSunMoved(LLUICtrl* ctrl, void* userdata);
	void onTimeChanged();

    void onSunRotationChanged();
    void onMoonRotationChanged();

	// for handling when the star slider is moved to adjust the alpha
	void onStarAlphaMoved(LLUICtrl* ctrl);

	// handle cloud scrolling
	void onCloudScrollXMoved(LLUICtrl* ctrl);
	void onCloudScrollYMoved(LLUICtrl* ctrl);

	//-- WL stuff ends --------------------------------------------------------

	void reset(); /// reset the floater to its initial state
	bool isNewPreset() const;
	void refreshSkyPresetsList();
	void enableEditing(bool enable);
	void saveRegionSky();
	std::string getSelectedPresetName() const;

	void onSkyPresetNameEdited();
	void onSkyPresetSelected();
	bool onSaveAnswer(const LLSD& notification, const LLSD& response);
	void onSaveConfirmed();

	void onBtnSave();
	void onBtnCancel();

	void onSkyPresetListChange();
	void onRegionSettingsChange();
	void onRegionInfoUpdate();

    LLSettingsSky::ptr_t mEditSettings;

	LLLineEditor*	mSkyPresetNameEditor;
	LLComboBox*		mSkyPresetCombo;
	LLCheckBoxCtrl*	mMakeDefaultCheckBox;
	LLButton*		mSaveButton;
    LLSkySettingsAdapterPtr mSkyAdapter;
#endif
};

class LLFloaterFixedEnvironmentWater : public LLFloaterFixedEnvironment
{
    LOG_CLASS(LLFloaterFixedEnvironmentWater);

public:
    LLFloaterFixedEnvironmentWater(const LLSD &key);

    BOOL	                postBuild()                 override;

    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

protected:
    virtual void            updateEditEnvironment()     override;

    virtual void            doLoadFromInventory()       override;
    virtual void            doImportFromDisk()          override;
    virtual void            doApplyFixedSettings()      override;

private:
};

class LLSettingsEditPanel : public LLPanel
{
public:
    virtual void setSettings(LLSettingsBase::ptr_t &) = 0;

//     virtual void refresh() override;

protected:
    LLSettingsEditPanel() :
        LLPanel()
    {}

};

#endif // LL_FLOATERFIXEDENVIRONMENT_H
