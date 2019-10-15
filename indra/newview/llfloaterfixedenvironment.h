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

#include "boost/signals2.hpp"

class LLTabContainer;
class LLButton;
class LLLineEditor;
class LLFloaterSettingsPicker;
class LLFixedSettingCopiedCallback;

/**
 * Floater container for creating and editing fixed environment settings.
 */
class LLFloaterFixedEnvironment : public LLFloater
{
    LOG_CLASS(LLFloaterFixedEnvironment);

    friend class LLFixedSettingCopiedCallback;

public:
    static const std::string    KEY_INVENTORY_ID;

                            LLFloaterFixedEnvironment(const LLSD &key);
                            ~LLFloaterFixedEnvironment();

    virtual BOOL	        postBuild()                 override;
    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

    virtual void            onFocusReceived()           override;
    virtual void            onFocusLost()               override;

    void                    setEditSettings(const LLSettingsBase::ptr_t &settings)  { mSettings = settings; clearDirtyFlag(); syncronizeTabs(); refresh(); }
    LLSettingsBase::ptr_t   getEditSettings()   const                           { return mSettings; }

    virtual BOOL            isDirty() const override            { return getIsDirty(); }

protected:
    typedef std::function<void()> on_confirm_fn;

    virtual void            updateEditEnvironment() = 0;
    virtual void            refresh()                   override;
    virtual void            syncronizeTabs();

    LLFloaterSettingsPicker *getSettingsPicker();

    void                    loadInventoryItem(const LLUUID  &inventoryId, bool can_trans = true);

    void                    checkAndConfirmSettingsLoss(on_confirm_fn cb);

    LLTabContainer *        mTab;
    LLLineEditor *          mTxtName;

    LLSettingsBase::ptr_t   mSettings;

    virtual void            doImportFromDisk() = 0;
    virtual void            doApplyCreateNewInventory(std::string settings_name, const LLSettingsBase::ptr_t &settings);
    virtual void            doApplyUpdateInventory(const LLSettingsBase::ptr_t &settings);
    virtual void            doApplyEnvironment(const std::string &where, const LLSettingsBase::ptr_t &settings);
    void                    doCloseInventoryFloater(bool quitting = false);

    bool                    canUseInventory() const;
    bool                    canApplyRegion() const;
    bool                    canApplyParcel() const;

    LLFlyoutComboBtnCtrl *      mFlyoutControl;

    LLUUID                  mInventoryId;
    LLInventoryItem *       mInventoryItem;
    LLHandle<LLFloater>     mInventoryFloater;
    bool                    mCanCopy;
    bool                    mCanMod;
    bool                    mCanTrans;

    void                    onInventoryCreated(LLUUID asset_id, LLUUID inventory_id);
    void                    onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results);
    void                    onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results);

    bool                    getIsDirty() const  { return mIsDirty; }
    void                    setDirtyFlag()      { mIsDirty = true; }
    virtual void            clearDirtyFlag();

    void                    doSelectFromInventory();
    void                    onPanelDirtyFlagChanged(bool);

    virtual void            onClickCloseBtn(bool app_quitting = false) override;
    void                    onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsBase::ptr_t &settings);

private:
    void                    onNameChanged(const std::string &name);

    void                    onButtonImport();
    void                    onButtonApply(LLUICtrl *ctrl, const LLSD &data);
    void                    onButtonLoad();

    void                    onPickerCommitSetting(LLUUID item_id);
    void                    onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settins, S32 status);

    bool                    mIsDirty;
};

class LLFloaterFixedEnvironmentWater : public LLFloaterFixedEnvironment
{
    LOG_CLASS(LLFloaterFixedEnvironmentWater);

public:
    LLFloaterFixedEnvironmentWater(const LLSD &key);

    BOOL	                postBuild()                 override;

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

    BOOL	                postBuild()                 override;

    virtual void            onOpen(const LLSD& key)     override;
    virtual void            onClose(bool app_quitting)  override;

protected:
    virtual void            updateEditEnvironment()     override;

    virtual void            doImportFromDisk()          override;
    void                    loadSkySettingFromFile(const std::vector<std::string>& filenames);

private:
};

class LLSettingsEditPanel : public LLPanel
{
public:
    virtual void setSettings(const LLSettingsBase::ptr_t &) = 0;

    typedef boost::signals2::signal<void(LLPanel *, bool)> on_dirty_charged_sg;
    typedef boost::signals2::connection connection_t;

    inline bool         getIsDirty() const      { return mIsDirty; }
    inline void         setIsDirty()            { mIsDirty = true; if (!mOnDirtyChanged.empty()) mOnDirtyChanged(this, mIsDirty); }
    inline void         clearIsDirty()          { mIsDirty = false; if (!mOnDirtyChanged.empty()) mOnDirtyChanged(this, mIsDirty); }

    inline bool        getCanChangeSettings() const    { return mCanEdit; }
    inline void        setCanChangeSettings(bool flag) { mCanEdit = flag; }

    inline connection_t setOnDirtyFlagChanged(on_dirty_charged_sg::slot_type cb)    { return mOnDirtyChanged.connect(cb); }


protected:
    LLSettingsEditPanel() :
        LLPanel(),
        mIsDirty(false),
        mOnDirtyChanged()
    {}

private:
    bool                mIsDirty;
    bool                mCanEdit;
    
    on_dirty_charged_sg mOnDirtyChanged;
};

#endif // LL_FLOATERFIXEDENVIRONMENT_H
