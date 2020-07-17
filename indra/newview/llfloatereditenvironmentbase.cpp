/** 
 * @file llfloatereditenvironmentbase.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatereditenvironmentbase.h"

#include <boost/make_shared.hpp>

// libs
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llfilepicker.h"
#include "llsettingspicker.h"
#include "llviewerparcelmgr.h"

// newview
#include "llsettingssky.h"
#include "llsettingswater.h"

#include "llenvironment.h"
#include "llagent.h"
#include "llparcel.h"

#include "llsettingsvo.h"
#include "llinventorymodel.h"

namespace
{
    const std::string ACTION_APPLY_LOCAL("apply_local");
    const std::string ACTION_APPLY_PARCEL("apply_parcel");
    const std::string ACTION_APPLY_REGION("apply_region");
}

//=========================================================================
const std::string LLFloaterEditEnvironmentBase::KEY_INVENTORY_ID("inventory_id");


//=========================================================================

class LLFixedSettingCopiedCallback : public LLInventoryCallback
{
public:
    LLFixedSettingCopiedCallback(LLHandle<LLFloater> handle) : mHandle(handle) {}

    virtual void fire(const LLUUID& inv_item_id)
    {
        if (!mHandle.isDead())
        {
            LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);
            if (item)
            {
                LLFloaterEditEnvironmentBase* floater = (LLFloaterEditEnvironmentBase*)mHandle.get();
                floater->onInventoryCreated(item->getAssetUUID(), inv_item_id);
            }
        }
    }

private:
    LLHandle<LLFloater> mHandle;
};

//=========================================================================
LLFloaterEditEnvironmentBase::LLFloaterEditEnvironmentBase(const LLSD &key) :
    LLFloater(key),
    mInventoryId(),
    mInventoryItem(nullptr),
    mIsDirty(false),
    mCanCopy(false),
    mCanMod(false),
    mCanTrans(false),
    mCanSave(false)
{
}

LLFloaterEditEnvironmentBase::~LLFloaterEditEnvironmentBase()
{
}

void LLFloaterEditEnvironmentBase::onFocusReceived()
{
    if (isInVisibleChain())
    {
        updateEditEnvironment();
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
    }
}

void LLFloaterEditEnvironmentBase::onFocusLost()
{
}

void LLFloaterEditEnvironmentBase::loadInventoryItem(const LLUUID  &inventoryId, bool can_trans)
{
    if (inventoryId.isNull())
    {
        mInventoryItem = nullptr;
        mInventoryId.setNull();
        mCanMod = true;
        mCanCopy = true;
        mCanTrans = true;
        return;
    }

    mInventoryId = inventoryId;
    LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
    mInventoryItem = gInventory.getItem(mInventoryId);

    if (!mInventoryItem)
    {
        LL_WARNS("SETTINGS") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;
        LLNotificationsUtil::add("CantFindInvItem");
        closeFloater();

        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    if (mInventoryItem->getAssetUUID().isNull())
    {
        LL_WARNS("ENVIRONMENT") << "Asset ID in inventory item is NULL (" << mInventoryId << ")" << LL_ENDL;
        LLNotificationsUtil::add("UnableEditItem");
        closeFloater();

        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    mCanSave = true;
    mCanCopy = mInventoryItem->getPermissions().allowCopyBy(gAgent.getID());
    mCanMod = mInventoryItem->getPermissions().allowModifyBy(gAgent.getID());
    mCanTrans = can_trans && mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());

    mExpectingAssetId = mInventoryItem->getAssetUUID();
    LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}


void LLFloaterEditEnvironmentBase::checkAndConfirmSettingsLoss(LLFloaterEditEnvironmentBase::on_confirm_fn cb)
{
    if (isDirty())
    {
        LLSD args(LLSDMap("TYPE", getEditSettings()->getSettingsType())
            ("NAME", getEditSettings()->getName()));

        // create and show confirmation textbox
        LLNotificationsUtil::add("SettingsConfirmLoss", args, LLSD(),
            [cb](const LLSD&notif, const LLSD&resp)
            {
                S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
                if (opt == 0)
                    cb();
            });
    }
    else if (cb)
    {
        cb();
    }
}

void LLFloaterEditEnvironmentBase::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (asset_id != mExpectingAssetId)
    {
        LL_WARNS("ENVDAYEDIT") << "Expecting {" << mExpectingAssetId << "} got {" << asset_id << "} - throwing away." << LL_ENDL;
        return;
    }
    mExpectingAssetId.setNull();
    clearDirtyFlag();

    if (!settings || status)
    {
        LLSD args;
        args["NAME"] = (mInventoryItem) ? mInventoryItem->getName() : "Unknown";
        LLNotificationsUtil::add("FailedToFindSettings", args);
        closeFloater();
        return;
    }

    if (settings->getFlag(LLSettingsBase::FLAG_NOSAVE))
    {
        mCanSave = false;
        mCanCopy = false;
        mCanMod = false;
        mCanTrans = false;
    }
    else
    {
        if (mInventoryItem)
            settings->setName(mInventoryItem->getName());

        if (mCanCopy)
            settings->clearFlag(LLSettingsBase::FLAG_NOCOPY);
        else
            settings->setFlag(LLSettingsBase::FLAG_NOCOPY);

        if (mCanMod)
            settings->clearFlag(LLSettingsBase::FLAG_NOMOD);
        else
            settings->setFlag(LLSettingsBase::FLAG_NOMOD);

        if (mCanTrans)
            settings->clearFlag(LLSettingsBase::FLAG_NOTRANS);
        else
            settings->setFlag(LLSettingsBase::FLAG_NOTRANS);
    }

    setEditSettingsAndUpdate(settings);
}

void LLFloaterEditEnvironmentBase::onButtonImport()
{
    checkAndConfirmSettingsLoss([this](){ doImportFromDisk(); });
}

void LLFloaterEditEnvironmentBase::onSaveAsCommit(const LLSD& notification, const LLSD& response, const LLSettingsBase::ptr_t &settings)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (0 == option)
    {
        std::string settings_name = response["message"].asString();

        LLInventoryObject::correctInventoryName(settings_name);
        if (settings_name.empty())
        {
            // Ideally notification should disable 'OK' button if name won't fit our requirements,
            // for now either display notification, or use some default name
            settings_name = "Unnamed";
        }

        if (mCanMod)
        {
            doApplyCreateNewInventory(settings_name, settings);
        }
        else if (mInventoryItem)
        {
            const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
            LLUUID parent_id = mInventoryItem->getParentUUID();
            if (marketplacelistings_id == parent_id)
            {
                parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
            }

            LLPointer<LLInventoryCallback> cb = new LLFixedSettingCopiedCallback(getHandle());
            copy_inventory_item(
                gAgent.getID(),
                mInventoryItem->getPermissions().getOwner(),
                mInventoryItem->getUUID(),
                parent_id,
                settings_name,
                cb);
        }
        else
        {
            LL_WARNS() << "Failed to copy fixed env setting" << LL_ENDL;
        }
    }
}

void LLFloaterEditEnvironmentBase::onClickCloseBtn(bool app_quitting)
{
    if (!app_quitting)
        checkAndConfirmSettingsLoss([this](){ closeFloater(); clearDirtyFlag(); });
    else
        closeFloater();
}

void LLFloaterEditEnvironmentBase::doApplyCreateNewInventory(std::string settings_name, const LLSettingsBase::ptr_t &settings)
{
    if (mInventoryItem)
    {
        LLUUID parent_id = mInventoryItem->getParentUUID();
        U32 next_owner_perm = mInventoryItem->getPermissions().getMaskNextOwner();
        LLSettingsVOBase::createInventoryItem(settings, next_owner_perm, parent_id, settings_name,
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    }
    else
    {
        LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);
        // This method knows what sort of settings object to create.
        LLSettingsVOBase::createInventoryItem(settings, parent_id, settings_name,
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    }
}

void LLFloaterEditEnvironmentBase::doApplyUpdateInventory(const LLSettingsBase::ptr_t &settings)
{
    LL_DEBUGS("ENVEDIT") << "Update inventory for " << mInventoryId << LL_ENDL;
    if (mInventoryId.isNull())
    {
        LLSettingsVOBase::createInventoryItem(settings, gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS), std::string(),
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    }
    else
    {
        LLSettingsVOBase::updateInventoryItem(settings, mInventoryId,
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
    }
}

void LLFloaterEditEnvironmentBase::doApplyEnvironment(const std::string &where, const LLSettingsBase::ptr_t &settings)
{
    U32 flags(0);

    if (mInventoryItem)
    {
        if (!mInventoryItem->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID()))
            flags |= LLSettingsBase::FLAG_NOMOD;
        if (!mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
            flags |= LLSettingsBase::FLAG_NOTRANS;
    }

    flags |= settings->getFlags();
    settings->setFlag(flags);

    if (where == ACTION_APPLY_LOCAL)
    {
        settings->setName("Local"); // To distinguish and make sure there is a name. Safe, because this is a copy.
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, settings);
    }
    else if (where == ACTION_APPLY_PARCEL)
    {
        LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());

        if ((!parcel) || (parcel->getLocalID() == INVALID_PARCEL_ID))
        {
            LL_WARNS("ENVIRONMENT") << "Can not identify parcel. Not applying." << LL_ENDL;
            LLNotificationsUtil::add("WLParcelApplyFail");
            return;
        }

        if (mInventoryItem && !isDirty())
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), mInventoryItem->getAssetUUID(), mInventoryItem->getName(), LLEnvironment::NO_TRACK, -1, -1, flags);
        }
        else if (settings->getSettingsType() == "sky")
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsSky>(settings), -1, -1);
        }
        else if (settings->getSettingsType() == "water")
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsWater>(settings), -1, -1);
        }
        else if (settings->getSettingsType() == "day")
        {
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), std::static_pointer_cast<LLSettingsDay>(settings), -1, -1);
        }
    }
    else if (where == ACTION_APPLY_REGION)
    {
        if (mInventoryItem && !isDirty())
        {
            LLEnvironment::instance().updateRegion(mInventoryItem->getAssetUUID(), mInventoryItem->getName(), LLEnvironment::NO_TRACK, -1, -1, flags);
        }
        else if (settings->getSettingsType() == "sky")
        {
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsSky>(settings), -1, -1);
        }
        else if (settings->getSettingsType() == "water")
        {
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsWater>(settings), -1, -1);
        }
        else if (settings->getSettingsType() == "day")
        {
            LLEnvironment::instance().updateRegion(std::static_pointer_cast<LLSettingsDay>(settings), -1, -1);
        }
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

}

void LLFloaterEditEnvironmentBase::doCloseInventoryFloater(bool quitting)
{
    LLFloater* floaterp = mInventoryFloater.get();

    if (floaterp)
    {
        floaterp->closeFloater(quitting);
    }
}

void LLFloaterEditEnvironmentBase::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

    if (inventory_id.isNull() || !results["success"].asBoolean())
    {
        LLNotificationsUtil::add("CantCreateInventory");
        return;
    }
    onInventoryCreated(asset_id, inventory_id);
}

void LLFloaterEditEnvironmentBase::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id)
{
    bool can_trans = true;
    if (mInventoryItem)
    {
        LLPermissions perms = mInventoryItem->getPermissions();

        LLInventoryItem *created_item = gInventory.getItem(mInventoryId);

        if (created_item)
        {
            can_trans = perms.allowOperationBy(PERM_TRANSFER, gAgent.getID());
            created_item->setPermissions(perms);
            created_item->updateServer(false);
        }
    }

    clearDirtyFlag();
    setFocus(TRUE);                 // Call back the focus...
    loadInventoryItem(inventory_id, can_trans);
}

void LLFloaterEditEnvironmentBase::onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been updated with asset " << asset_id << " results are:" << results << LL_ENDL;

    clearDirtyFlag();
    if (inventory_id != mInventoryId)
    {
        loadInventoryItem(inventory_id);
    }
}

void LLFloaterEditEnvironmentBase::onPanelDirtyFlagChanged(bool value)
{
    if (value)
        setDirtyFlag();
}

//-------------------------------------------------------------------------
bool LLFloaterEditEnvironmentBase::canUseInventory() const
{
    return LLEnvironment::instance().isInventoryEnabled();
}

bool LLFloaterEditEnvironmentBase::canApplyRegion() const
{
    return gAgent.canManageEstate();
}

bool LLFloaterEditEnvironmentBase::canApplyParcel() const
{
    return LLEnvironment::instance().canAgentUpdateParcelEnvironment();
}

//=========================================================================
