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
#include "llflyoutcombobtn.h"
#include "llinventory.h"

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
    static const std::string    KEY_INVENTORY_ID;

                            LLFloaterFixedEnvironment(const LLSD &key);
                            ~LLFloaterFixedEnvironment();

    virtual BOOL	        postBuild()                 override;
    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

    virtual void            onFocusReceived()           override;
    virtual void            onFocusLost()               override;

    void                    setEditSettings(const LLSettingsBase::ptr_t &settings)  { mSettings = settings; syncronizeTabs(); refresh(); }
    LLSettingsBase::ptr_t   getEditSettings()   const                           { return mSettings; }

protected:
    virtual void            updateEditEnvironment() = 0;
    virtual void            refresh()                   override;
    virtual void            syncronizeTabs();

    void                    loadInventoryItem(const LLUUID  &inventoryId);

    LLTabContainer *        mTab;
    LLLineEditor *          mTxtName;

    LLSettingsBase::ptr_t   mSettings;

    virtual void            doImportFromDisk() = 0;
    virtual void            doApplyCreateNewInventory();
    virtual void            doApplyUpdateInventory();
    virtual void            doApplyEnvironment(const std::string &where);

    bool                    canUseInventory() const;
    bool                    canApplyRegion() const;
    bool                    canApplyParcel() const;

    LLFlyoutComboBtnCtrl *      mFlyoutControl;

    LLUUID                  mInventoryId;
    LLInventoryItem *       mInventoryItem;

    void                    onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results);
    void                    onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results);

private:
    void                    onNameChanged(const std::string &name);

    void                    onButtonImport();
    void                    onButtonApply(LLUICtrl *ctrl, const LLSD &data);
    void                    onButtonCancel();

    void                    onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settins, S32 status);
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

    virtual void            doImportFromDisk()          override;

private:
};

class LLFloaterFixedEnvironmentSky : public LLFloaterFixedEnvironment
{
    LOG_CLASS(LLFloaterFixedEnvironmentSky);

public:
    LLFloaterFixedEnvironmentSky(const LLSD &key);

    BOOL	                postBuild()                 override;

    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

protected:
    virtual void            updateEditEnvironment()     override;

    virtual void            doImportFromDisk()          override;

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
