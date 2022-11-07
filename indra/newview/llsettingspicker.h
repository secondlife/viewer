/** 
 * @file llsettingspicker.h
 * @author Rider Linden
 * @brief LLSettingsPicker class header file including related functions
 *
 * $LicenseInfo:firstyear=2018&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2018, Linden Research, Inc.
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

#ifndef LL_SETTINGSPICKER_H
#define LL_SETTINGSPICKER_H

#include "llinventorysettings.h"
#include "llfloater.h"
#include "llpermissionsflags.h"
#include "llfolderview.h"
#include "llinventory.h"
#include "llsettingsdaycycle.h"

#include <boost/signals2.hpp>

//=========================================================================
class LLFilterEditor;
class LLInventoryPanel;

//=========================================================================
class LLFloaterSettingsPicker : public LLFloater
{
public:
    enum ETrackMode
    {
        TRACK_NONE,
        TRACK_WATER,
        TRACK_SKY
    };
    typedef std::function<void()>                           close_callback_t;
    typedef std::function<void(const LLUUID& item_id)>     id_changed_callback_t;

    LLFloaterSettingsPicker(LLView * owner, LLUUID setting_item_id, const LLSD &params = LLSD());

    virtual                 ~LLFloaterSettingsPicker() override;

    void                    setActive(bool active);

    virtual BOOL            postBuild() override;
    virtual void            onClose(bool app_quitting) override;
    virtual void            draw() override;

    void                    setSettingsItemId(const LLUUID &settings_id, bool set_selection = true);
    LLUUID                  getSettingsItemId() const              { return mSettingItemID; }

    void                    setSettingsFilter(LLSettingsType::type_e type);
    LLSettingsType::type_e  getSettingsFilter() const { return mSettingsType; }

    // Only for day cycle
    void                    setTrackMode(ETrackMode mode);
    void                    setTrackWater() { mTrackMode = TRACK_WATER; }
    void                    setTrackSky() { mTrackMode = TRACK_SKY; }

    // Takes a UUID, wraps get/setImageAssetID
    virtual void            setValue(const LLSD& value) override;
    virtual LLSD            getValue() const override;

    static LLUUID                findItemID(const LLUUID& asset_id, bool copyable_only, bool ignore_library = false) 
    {
        LLInventoryItem *pitem = findItem(asset_id, copyable_only, ignore_library);
        if (pitem)
            return pitem->getUUID();
        return LLUUID::null;
    }

    static std::string           findItemName(const LLUUID& asset_id, bool copyable_only, bool ignore_library = false)
    {
        LLInventoryItem *pitem = findItem(asset_id, copyable_only, ignore_library);
        if (pitem)
            return pitem->getName();
        return std::string();
    }

    static LLInventoryItem *     findItem(const LLUUID& asset_id, bool copyable_only, bool ignore_library);


private:
    typedef std::deque<LLFolderViewItem *>  itemlist_t;

    void                    onFilterEdit(const std::string& search_string);
    void                    onSelectionChange(const itemlist_t &items, bool user_action);
    static void             onAssetLoadedCb(LLHandle<LLFloater> handle, LLUUID item_id, LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status);
    void                    onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings);
    void                    onButtonCancel();
    void                    onButtonSelect();
    virtual BOOL            handleDoubleClick(S32 x, S32 y, MASK mask) override;
    BOOL                    handleKeyHere(KEY key, MASK mask) override;
    void                    onFocusLost() override;


    LLHandle<LLView>        mOwnerHandle;
    LLUUID                  mSettingItemID;
    LLUUID                  mSettingAssetID;
    ETrackMode              mTrackMode;

    LLFilterEditor *        mFilterEdit;
    LLInventoryPanel *      mInventoryPanel;
    LLSettingsType::type_e  mSettingsType;

    F32                     mContextConeOpacity;
    PermissionMask          mImmediateFilterPermMask;

    bool                    mActive;
    bool                    mNoCopySettingsSelected;

    LLSaveFolderState       mSavedFolderState;

//     boost::signals2::signal<void(LLUUID id)>                mCommitSignal;
    boost::signals2::signal<void()>                         mCloseSignal;
    boost::signals2::signal<void(const LLUUID& item_id)>   mChangeIDSignal;
};

#endif  // LL_LLTEXTURECTRL_H
