/**
 * @file llfloatergesture.cpp
 * @brief LLFloaterMyEnvironment class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#include "llfloatermyenvironment.h"

#include "llinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"

#include "llagent.h"
#include "llclipboard.h"
#include "llcheckboxctrl.h"
#include "llviewerinventory.h"
#include "llenvironment.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "lltrans.h"
#include "llviewerparcelmgr.h"

//=========================================================================
namespace
{
    const std::string CHECK_DAYS("chk_days");
    const std::string CHECK_SKIES("chk_skies");
    const std::string CHECK_WATER("chk_water");
    const std::string FLT_SEARCH("flt_search");
    const std::string PANEL_SETTINGS("pnl_settings");
    const std::string CHECK_SHOWFOLDERS("chk_showfolders");
    const std::string BUTTON_NEWSETTINGS("btn_gear");
    const std::string BUTTON_GEAR("btn_newsettings");
    const std::string BUTTON_DELETE("btn_del");


    const std::string ACTION_DOCREATE("MyEnvironments.DoCreate");
    const std::string ACTION_DOEDIT("MyEnvironments.DoEdit");
    const std::string ACTION_DOAPPLY("MyEnvironments.DoApply");
    const std::string ACTION_COPYPASTE("MyEnvironments.CopyPaste");
    const std::string ENABLE_ACTION("MyEnvironments.EnableAction");
    const std::string ENABLE_CANAPPLY("MyEnvironments.CanApply");
    const std::string ENABLE_ENVIRONMENT("MyEnvironments.EnvironmentEnabled");

    const std::string PARAMETER_REGION("region");
    const std::string PARAMETER_PARCEL("parcel");
    const std::string PARAMETER_LOCAL("local");

    const std::string PARAMETER_EDIT("edit");
    const std::string PARAMETER_COPY("copy");
    const std::string PARAMETER_PASTE("paste");
    const std::string PARAMETER_COPYUUID("copy_uuid");
}

//=========================================================================
LLFloaterMyEnvironment::LLFloaterMyEnvironment(const LLSD& key) :
    LLFloater(key),
    mInventoryList(nullptr),
    mShowFolders(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS),
    mTypeFilter((0x01 << static_cast<U64>(LLSettingsType::ST_DAYCYCLE)) | (0x01 << static_cast<U64>(LLSettingsType::ST_SKY)) | (0x01 << static_cast<U64>(LLSettingsType::ST_WATER))),
    mSelectedAsset()
{
    mCommitCallbackRegistrar.add(ACTION_DOCREATE, [this](LLUICtrl *, const LLSD &userdata) { onDoCreate(userdata); });
    mCommitCallbackRegistrar.add(ACTION_DOEDIT, [this](LLUICtrl *, const LLSD &userdata) { mInventoryList->openSelected(); });
    mCommitCallbackRegistrar.add(ACTION_DOAPPLY, [this](LLUICtrl *, const LLSD &userdata) { onDoApply(userdata.asString()); });
    mCommitCallbackRegistrar.add(ACTION_COPYPASTE, [this](LLUICtrl *, const LLSD &userdata) { mInventoryList->doToSelected(userdata.asString()); });

    mEnableCallbackRegistrar.add(ENABLE_ACTION, [this](LLUICtrl *, const LLSD &userdata) { return canAction(userdata.asString()); });
    mEnableCallbackRegistrar.add(ENABLE_CANAPPLY, [this](LLUICtrl *, const LLSD &userdata) { return canApply(userdata.asString()); });
    mEnableCallbackRegistrar.add(ENABLE_ENVIRONMENT, [](LLUICtrl *, const LLSD &) { return LLEnvironment::instance().isInventoryEnabled(); });

}

LLFloaterMyEnvironment::~LLFloaterMyEnvironment()
{
}


bool LLFloaterMyEnvironment::postBuild()
{
    mInventoryList = getChild<LLInventoryPanel>(PANEL_SETTINGS);

    if (mInventoryList)
    {
        U32 filter_types = 0x0;
        filter_types |= 0x1 << LLInventoryType::IT_SETTINGS;

        mInventoryList->setFilterTypes(filter_types);

        mInventoryList->setSelectCallback([this](const std::deque<LLFolderViewItem*>&, bool) { onSelectionChange(); });
        mInventoryList->setShowFolderState(mShowFolders);
        mInventoryList->setFilterSettingsTypes(mTypeFilter);
    }

    childSetCommitCallback(CHECK_DAYS, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_SKIES, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_WATER, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_SHOWFOLDERS, [this](LLUICtrl*, void*) { onShowFoldersChange(); }, nullptr);

    mFilterEdit = getChild<LLFilterEditor>(FLT_SEARCH);
    mFilterEdit->setCommitCallback([this](LLUICtrl*, const LLSD& param){ onFilterEdit(param.asString()); });

    childSetCommitCallback(BUTTON_DELETE, [this](LLUICtrl *, void*) { onDeleteSelected(); }, nullptr);
    mSavedFolderState.setApply(false);
    return true;
}

void LLFloaterMyEnvironment::refresh()
{
    getChild<LLCheckBoxCtrl>(CHECK_SHOWFOLDERS)->setValue(LLSD::Boolean(mShowFolders == LLInventoryFilter::SHOW_ALL_FOLDERS));

    getChild<LLCheckBoxCtrl>(CHECK_DAYS)->setValue(LLSD::Boolean(mTypeFilter & (0x01 << static_cast<U64>(LLSettingsType::ST_DAYCYCLE))));
    getChild<LLCheckBoxCtrl>(CHECK_SKIES)->setValue(LLSD::Boolean(mTypeFilter & (0x01 << static_cast<U64>(LLSettingsType::ST_SKY))));
    getChild<LLCheckBoxCtrl>(CHECK_WATER)->setValue(LLSD::Boolean(mTypeFilter & (0x01 << static_cast<U64>(LLSettingsType::ST_WATER))));

    refreshButtonStates();

}

void LLFloaterMyEnvironment::onOpen(const LLSD& key)
{
    LLFloater::onOpen(key);

    if (key.has("asset_id") && mInventoryList)
    {
        mSelectedAsset = key["asset_id"].asUUID();

        if (!mSelectedAsset.isNull())
        {
            LLUUID obj_id = findItemByAssetId(mSelectedAsset, false, false);
            if (!obj_id.isNull())
                mInventoryList->setSelection(obj_id, false);
        }
    }
    else
    {
        mSelectedAsset.setNull();
    }

    refresh();
}

//-------------------------------------------------------------------------
void LLFloaterMyEnvironment::onShowFoldersChange()
{
    bool show_check(getChild<LLCheckBoxCtrl>(CHECK_SHOWFOLDERS)->getValue().asBoolean());

    mShowFolders = (show_check) ? LLInventoryFilter::SHOW_ALL_FOLDERS : LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS;

    if (mInventoryList)
        mInventoryList->setShowFolderState(mShowFolders);
}

void LLFloaterMyEnvironment::onFilterCheckChange()
{
    mTypeFilter = 0x0;

    if (getChild<LLCheckBoxCtrl>(CHECK_DAYS)->getValue().asBoolean())
        mTypeFilter |= 0x01 << static_cast<U64>(LLSettingsType::ST_DAYCYCLE);
    if (getChild<LLCheckBoxCtrl>(CHECK_SKIES)->getValue().asBoolean())
        mTypeFilter |= 0x01 << static_cast<U64>(LLSettingsType::ST_SKY);
    if (getChild<LLCheckBoxCtrl>(CHECK_WATER)->getValue().asBoolean())
        mTypeFilter |= 0x01 << static_cast<U64>(LLSettingsType::ST_WATER);

    if (mInventoryList)
        mInventoryList->setFilterSettingsTypes(mTypeFilter);
}

void LLFloaterMyEnvironment::onSelectionChange()
{
    refreshButtonStates();
}

void LLFloaterMyEnvironment::onFilterEdit(const std::string& search_string)
{
    std::string upper_case_search_string = search_string;
    LLStringUtil::toUpper(upper_case_search_string);

    if (upper_case_search_string.empty())
    {
        if (mInventoryList->getFilterSubString().empty())
        {
            // current filter and new filter empty, do nothing
            return;
        }

        mSavedFolderState.setApply(true);
        mInventoryList->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        // add folder with current item to list of previously opened folders
        LLOpenFoldersWithSelection opener;
        mInventoryList->getRootFolder()->applyFunctorRecursively(opener);
        mInventoryList->getRootFolder()->scrollToShowSelection();

    }
    else if (mInventoryList->getFilterSubString().empty())
    {
        // first letter in search term, save existing folder open state
        mSavedFolderState.setApply(false);
        mInventoryList->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
    }

    mInventoryList->setFilterSubString(search_string);
}

void LLFloaterMyEnvironment::onItemsRemovalConfirmation(const LLSD& notification, const LLSD& response, uuid_vec_t item_ids)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
{
    const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        for (const LLUUID& itemid : item_ids)
    {
        LLInventoryItem* inv_item = gInventory.getItem(itemid);

        if (inv_item && inv_item->getInventoryType() == LLInventoryType::IT_SETTINGS)
        {
            LLInventoryModel::update_list_t update;
            LLInventoryModel::LLCategoryUpdate old_folder(inv_item->getParentUUID(), -1);
            update.push_back(old_folder);
            LLInventoryModel::LLCategoryUpdate new_folder(trash_id, 1);
            update.push_back(new_folder);
            gInventory.accountForUpdate(update);

            LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(inv_item);
            new_item->setParent(trash_id);
            new_item->updateParentOnServer(false);
            gInventory.updateItem(new_item);
        }
    }
    gInventory.notifyObservers();
}
}

void LLFloaterMyEnvironment::onDeleteSelected()
{
    uuid_vec_t selected;

    getSelectedIds(selected);
    if (selected.empty())
        return;

    LLSD args;
    args["QUESTION"] = LLTrans::getString(selected.size() > 1 ? "DeleteItems" : "DeleteItem");
    LLNotificationsUtil::add(
        "DeleteItems",
        args,
        LLSD(),
        [this, selected](const LLSD& notification, const LLSD& response)
        {
            onItemsRemovalConfirmation(notification, response, selected);
        });
}


void LLFloaterMyEnvironment::onDoCreate(const LLSD &data)
{
    menu_create_inventory_item(mInventoryList, NULL, data);
}

void LLFloaterMyEnvironment::onDoApply(const std::string &context)
{
    uuid_vec_t selected;
    getSelectedIds(selected);

    if (selected.size() != 1) // Exactly one item selected.
        return;

    LLUUID item_id(selected.front());

    LLInventoryItem* itemp = gInventory.getItem(item_id);

    if (itemp && itemp->getInventoryType() == LLInventoryType::IT_SETTINGS)
    {
        LLUUID asset_id = itemp->getAssetUUID();
        std::string name = itemp->getName();

        U32 flags(0);

        if (!itemp->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID()))
            flags |= LLSettingsBase::FLAG_NOMOD;
        if (!itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
            flags |= LLSettingsBase::FLAG_NOTRANS;

        if (context == PARAMETER_REGION)
        {
            LLEnvironment::instance().updateRegion(asset_id, name, LLEnvironment::NO_TRACK, -1, -1, flags);
            LLEnvironment::instance().setSharedEnvironment();
        }
        else if (context == PARAMETER_PARCEL)
        {
            LLParcel *parcel(LLViewerParcelMgr::instance().getAgentOrSelectedParcel());
            if (!parcel)
            {
                LL_WARNS("ENVIRONMENT") << "Unable to determine parcel." << LL_ENDL;
                return;
            }
            LLEnvironment::instance().updateParcel(parcel->getLocalID(), asset_id, name, LLEnvironment::NO_TRACK, -1, -1, flags);
            LLEnvironment::instance().setSharedEnvironment();
        }
        else if (context == PARAMETER_LOCAL)
        {
            LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, asset_id);
            LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
        }
    }
}

bool LLFloaterMyEnvironment::canAction(const std::string &context)
{
    uuid_vec_t selected;
    getSelectedIds(selected);

    if (selected.empty())
        return false;

    if (context == PARAMETER_EDIT)
    {
        return (selected.size() == 1) && isSettingId(selected.front());
    }
    else if (context == PARAMETER_COPY)
    {
        for (std::vector<LLUUID>::iterator it = selected.begin(); it != selected.end(); it++)
        {
            if(!isSettingId(*it))
            {
                return false;
            }
        }
        return true;
    }
    else if (context == PARAMETER_PASTE)
    {
        if (!LLClipboard::instance().hasContents())
            return false;
        if (gInventory.isObjectDescendentOf(selected.front(), gInventory.getLibraryRootFolderID()))
            return false;

        std::vector<LLUUID> ids;
        LLClipboard::instance().pasteFromClipboard(ids);
        for (std::vector<LLUUID>::iterator it = ids.begin(); it != ids.end(); it++)
        {
            if (!isSettingId(*it))
            {
                return false;
            }
        }
        return (selected.size() == 1);
    }
    else if (context == PARAMETER_COPYUUID)
    {
        return (selected.size() == 1) && isSettingId(selected.front());
    }

    return false;
}

bool LLFloaterMyEnvironment::canApply(const std::string &context)
{
    uuid_vec_t selected;
    getSelectedIds(selected);

    if (selected.size() != 1) // Exactly one item selected.
        return false;

    if (context == PARAMETER_REGION)
    {
        return isSettingId(selected.front()) && LLEnvironment::instance().canAgentUpdateRegionEnvironment();
    }
    else if (context == PARAMETER_PARCEL)
    {
        return isSettingId(selected.front()) && LLEnvironment::instance().canAgentUpdateParcelEnvironment();
    }
    else if (context == PARAMETER_LOCAL)
    {
        return isSettingId(selected.front());
    }

    return false;
}

bool can_delete(const LLUUID& id)
{
    const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
    if (id == trash_id || gInventory.isObjectDescendentOf(id, trash_id))
    {
        return false;
    }

    LLViewerInventoryCategory* cat = gInventory.getCategory(id);
    if (cat)
    {
        if (!get_is_category_removable(&gInventory, id))
        {
            return false;
        }
    }
    else if (!get_is_item_removable(&gInventory, id, false))
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------
void LLFloaterMyEnvironment::refreshButtonStates()
{
    bool settings_ok = LLEnvironment::instance().isInventoryEnabled();

    uuid_vec_t selected;
    getSelectedIds(selected);

    getChild<LLUICtrl>(BUTTON_GEAR)->setEnabled(settings_ok);
    getChild<LLUICtrl>(BUTTON_NEWSETTINGS)->setEnabled(true);

    bool enable_delete = false;
    if(settings_ok && !selected.empty())
    {
        enable_delete = can_delete(selected.front());
    }

    getChild<LLUICtrl>(BUTTON_DELETE)->setEnabled(enable_delete);
}

//-------------------------------------------------------------------------
LLUUID LLFloaterMyEnvironment::findItemByAssetId(LLUUID asset_id, bool copyable_only, bool ignore_library)
{
    /*TODO: Rider: Move this to gInventory? */

    LLViewerInventoryCategory::cat_array_t cats;
    LLViewerInventoryItem::item_array_t items;
    LLAssetIDMatches asset_id_matches(asset_id);

    gInventory.collectDescendentsIf(LLUUID::null,
        cats,
        items,
        LLInventoryModel::INCLUDE_TRASH,
        asset_id_matches);

    if (!items.empty())
    {
        // search for copyable version first
        for (auto & item : items)
        {
            const LLPermissions& item_permissions = item->getPermissions();
            if (item_permissions.allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
            {
                if(!ignore_library || !gInventory.isObjectDescendentOf(item->getUUID(),gInventory.getLibraryRootFolderID()))
                {
                    return item->getUUID();
                }
            }
        }
        // otherwise just return first instance, unless copyable requested
        if (copyable_only)
        {
            return LLUUID::null;
        }
        else
        {
            if(!ignore_library || !gInventory.isObjectDescendentOf(items[0]->getUUID(),gInventory.getLibraryRootFolderID()))
            {
                return items[0]->getUUID();
            }
        }
    }

    return LLUUID::null;
}

bool LLFloaterMyEnvironment::isSettingId(const LLUUID& item_id)
{
    LLInventoryItem* itemp = gInventory.getItem(item_id);

    if (itemp && itemp->getInventoryType() == LLInventoryType::IT_SETTINGS)
    {
        return true;
    }
    return false;
}

void LLFloaterMyEnvironment::getSelectedIds(uuid_vec_t& ids) const
{
    LLInventoryPanel::selected_items_t items = mInventoryList->getSelectedItems();

    for (auto itemview : items)
    {
        LLFolderViewModelItemInventory* itemp = static_cast<LLFolderViewModelItemInventory*>(itemview->getViewModelItem());
        ids.push_back(itemp->getUUID());
    }
}
