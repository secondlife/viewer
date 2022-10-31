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

#include "llfloatereditenvironmentbase.h"
#include "llsettingsbase.h"
#include "llflyoutcombobtn.h"
#include "llinventory.h"

#include "boost/signals2.hpp"

class LLTabContainer;
class LLButton;
class LLLineEditor;
class LLFloaterSettingsPicker;
class LLFixedSettingCopiedCallback;

/**
 * Floater container for creating and editing fixed environment settings.
 */
class LLFloaterFixedEnvironment : public LLFloaterEditEnvironmentBase
{
    LOG_CLASS(LLFloaterFixedEnvironment);
public:
                            LLFloaterFixedEnvironment(const LLSD &key);
                            ~LLFloaterFixedEnvironment();

    virtual BOOL            postBuild()                 override;
    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

    void                    setEditSettings(const LLSettingsBase::ptr_t &settings)  { mSettings = settings; clearDirtyFlag(); syncronizeTabs(); refresh(); }
    virtual LLSettingsBase::ptr_t getEditSettings()   const override                { return mSettings; }

protected:
    typedef std::function<void()> on_confirm_fn;

    virtual void            refresh()                   override;
    void                    setEditSettingsAndUpdate(const LLSettingsBase::ptr_t &settings) override;
    virtual void            syncronizeTabs();

    virtual LLFloaterSettingsPicker *getSettingsPicker() override;

    LLTabContainer *        mTab;
    LLLineEditor *          mTxtName;

    LLSettingsBase::ptr_t   mSettings;

    LLFlyoutComboBtnCtrl *      mFlyoutControl;

    void                    onInventoryCreated(LLUUID asset_id, LLUUID inventory_id);
    void                    onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results);
    void                    onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results);

    virtual void            clearDirtyFlag() override;
    void                    updatePermissionFlags();

    void                    doSelectFromInventory();

    virtual void            onClickCloseBtn(bool app_quitting = false) override;

private:
    void                    onNameChanged(const std::string &name);

    void                    onButtonImport();
    void                    onButtonApply(LLUICtrl *ctrl, const LLSD &data);
    void                    onButtonLoad();

    void                    onPickerCommitSetting(LLUUID item_id);
};

class LLFloaterFixedEnvironmentWater : public LLFloaterFixedEnvironment
{
    LOG_CLASS(LLFloaterFixedEnvironmentWater);

public:
    LLFloaterFixedEnvironmentWater(const LLSD &key);

    BOOL                    postBuild()                 override;

    virtual void            onOpen(const LLSD& key)     override;

protected:
    virtual void            updateEditEnvironment()     override;

    virtual void            doImportFromDisk()          override;
    void                    loadWaterSettingFromFile(const std::vector<std::string>& filenames);

private:
};

class LLFloaterFixedEnvironmentSky : public LLFloaterFixedEnvironment
{
    LOG_CLASS(LLFloaterFixedEnvironmentSky);

public:
    LLFloaterFixedEnvironmentSky(const LLSD &key);

    BOOL                    postBuild()                 override;

    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

protected:
    virtual void            updateEditEnvironment()     override;

    virtual void            doImportFromDisk()          override;
    void                    loadSkySettingFromFile(const std::vector<std::string>& filenames);

private:
};

#endif // LL_FLOATERFIXEDENVIRONMENT_H
