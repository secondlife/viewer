/**
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

#include "llviewerprecompiledheaders.h"

#include "llsettingspicker.h"

#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfolderviewmodel.h"
#include "llinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llsettingsvo.h"

#include "lldraghandle.h"
#include "llviewercontrol.h"
#include "llagent.h"

//=========================================================================
namespace
{
    const std::string FLOATER_DEFINITION_XML("floater_settings_picker.xml");

    const std::string FLT_INVENTORY_SEARCH("flt_inventory_search");
    const std::string CMB_TRACK_SELECTION("track_selection");
    const std::string PNL_INVENTORY("pnl_inventory");
    const std::string PNL_COMBO("pnl_combo");
    const std::string BTN_SELECT("btn_select");
    const std::string BTN_CANCEL("btn_cancel");

    // strings in xml

    const std::string STR_TITLE_PREFIX = "pick title";
    const std::string STR_TITLE_TRACK = "pick_track";
    const std::string STR_TITLE_SETTINGS = "pick_settings";
    const std::string STR_TRACK_WATER = "track_water";
    const std::string STR_TRACK_GROUND = "track_ground";
    const std::string STR_TRACK_SKY = "track_sky";
}
//=========================================================================

LLFloaterSettingsPicker::LLFloaterSettingsPicker(LLView * owner, LLUUID initial_item_id, const LLSD &params):
    LLFloater(params),
    mOwnerHandle(),
    mActive(true),
    mContextConeOpacity(0.0f),
    mSettingItemID(initial_item_id),
    mTrackMode(TRACK_NONE),
    mImmediateFilterPermMask(PERM_NONE)
{
    mOwnerHandle = owner->getHandle();

    buildFromFile(FLOATER_DEFINITION_XML);
    setCanMinimize(false);
}


LLFloaterSettingsPicker::~LLFloaterSettingsPicker()
{

}

//-------------------------------------------------------------------------
bool LLFloaterSettingsPicker::postBuild()
{
    if (!LLFloater::postBuild())
        return false;

    std::string prefix = getString(STR_TITLE_PREFIX);
    std::string label = getString(STR_TITLE_SETTINGS);
    setTitle(prefix + " " + label);

    mFilterEdit = getChild<LLFilterEditor>(FLT_INVENTORY_SEARCH);
    mFilterEdit->setCommitCallback([this](LLUICtrl*, const LLSD& param) { onFilterEdit(param.asString()); });

    mInventoryPanel = getChild<LLInventoryPanel>(PNL_INVENTORY);
    if (mInventoryPanel)
    {
        U32 filter_types = 0x0;
        filter_types |= 0x1 << LLInventoryType::IT_SETTINGS;

        mInventoryPanel->setFilterTypes(filter_types);
        mInventoryPanel->setFilterPermMask(mImmediateFilterPermMask);

        mInventoryPanel->setSelectCallback([this](const LLFloaterSettingsPicker::itemlist_t &items, bool useraction){ onSelectionChange(items, useraction); });
        mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);
        mInventoryPanel->setSuppressOpenItemAction(true);

        // Disable auto selecting first filtered item because it takes away
        // selection from the item set by LLTextureCtrl owning this floater.
        mInventoryPanel->getRootFolder()->setAutoSelectOverride(true);

        // don't put keyboard focus on selected item, because the selection callback
        // will assume that this was user input
        if (!mSettingItemID.isNull())
        {
            //todo: this is bad idea
            mInventoryPanel->setSelection(mSettingItemID, TAKE_FOCUS_NO);
        }
        getChild<LLView>(BTN_SELECT)->setEnabled(mSettingItemID.notNull());
    }

    mNoCopySettingsSelected = false;

    childSetAction(BTN_CANCEL, [this](LLUICtrl*, const LLSD& param){ onButtonCancel(); });
    childSetAction(BTN_SELECT, [this](LLUICtrl*, const LLSD& param){ onButtonSelect(); });

    getChild<LLPanel>(PNL_COMBO)->setVisible(mTrackMode != TRACK_NONE);

    // update permission filter once UI is fully initialized
    mSavedFolderState.setApply(false);

    return true;
}

void LLFloaterSettingsPicker::onClose(bool app_quitting)
{
    if (app_quitting)
        return;

    mCloseSignal();
    LLView *owner = mOwnerHandle.get();
    if (owner)
    {
        owner->setFocus(true);
    }
    mSettingItemID.setNull();
    mInventoryPanel->getRootFolder()->clearSelection();
}

void LLFloaterSettingsPicker::setValue(const LLSD& value)
{
    mSettingItemID = value.asUUID();
}

LLSD LLFloaterSettingsPicker::getValue() const
{
    return LLSD(mSettingItemID);
}

void LLFloaterSettingsPicker::setSettingsFilter(LLSettingsType::type_e type)
{
    U64 filter = 0xFFFFFFFFFFFFFFFF;
    if (type != LLSettingsType::ST_NONE)
    {
        filter = static_cast<S64>(0x1) << static_cast<S64>(type);
    }

    mInventoryPanel->setFilterSettingsTypes(filter);
}

void LLFloaterSettingsPicker::setTrackMode(ETrackMode mode)
{
    mTrackMode = mode;
    getChild<LLPanel>(PNL_COMBO)->setVisible(mode != TRACK_NONE);

    std::string prefix = getString(STR_TITLE_PREFIX);
    std::string label;
    if (mode != TRACK_NONE)
    {
        label = getString(STR_TITLE_TRACK);
    }
    else
    {
        label = getString(STR_TITLE_SETTINGS);
    }
    setTitle(prefix + " " + label);
}

void LLFloaterSettingsPicker::draw()
{
    LLView *owner = mOwnerHandle.get();
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, owner);

    LLFloater::draw();
}

//=========================================================================
void LLFloaterSettingsPicker::onFilterEdit(const std::string& search_string)
{
    std::string upper_case_search_string = search_string;
    LLStringUtil::toUpper(upper_case_search_string);

    if (upper_case_search_string.empty())
    {
        if (mInventoryPanel->getFilterSubString().empty())
        {
            // current filter and new filter empty, do nothing
            return;
        }

        mSavedFolderState.setApply(true);
        mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        // add folder with current item to list of previously opened folders
        LLOpenFoldersWithSelection opener;
        mInventoryPanel->getRootFolder()->applyFunctorRecursively(opener);
        mInventoryPanel->getRootFolder()->scrollToShowSelection();
    }
    else if (mInventoryPanel->getFilterSubString().empty())
    {
        // first letter in search term, save existing folder open state
        if (!mInventoryPanel->getFilter().isNotDefault())
        {
            mSavedFolderState.setApply(false);
            mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        }
    }

    mInventoryPanel->setFilterSubString(search_string);
}

void LLFloaterSettingsPicker::onSelectionChange(const LLFloaterSettingsPicker::itemlist_t &items, bool user_action)
{
    bool is_item = false;
    LLUUID asset_id;
    if (items.size())
    {
        LLFolderViewItem* first_item = items.front();

        mNoCopySettingsSelected = false;
        if (first_item)
        {
            LLItemBridge *bridge_model = dynamic_cast<LLItemBridge *>(first_item->getViewModelItem());
            if (bridge_model && bridge_model->getItem())
            {
                if (!bridge_model->isItemCopyable())
                {
                    mNoCopySettingsSelected = true;
                }
                setSettingsItemId(bridge_model->getItem()->getUUID(), false);
                asset_id = bridge_model->getItem()->getAssetUUID();
                mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
                is_item = true;

                if (user_action)
                {
                    mChangeIDSignal(mSettingItemID);
                }
            }
        }
    }

    bool track_picker_enabled = mTrackMode != TRACK_NONE;

    getChild<LLView>(CMB_TRACK_SELECTION)->setEnabled(is_item && track_picker_enabled && mSettingAssetID == asset_id);
    getChild<LLView>(BTN_SELECT)->setEnabled(is_item && (!track_picker_enabled || mSettingAssetID == asset_id));
    if (track_picker_enabled && asset_id.notNull() && mSettingAssetID != asset_id)
    {
        LLUUID item_id = mSettingItemID;
        LLHandle<LLFloater> handle = getHandle();
        LLSettingsVOBase::getSettingsAsset(asset_id,
            [item_id, handle](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { LLFloaterSettingsPicker::onAssetLoadedCb(handle, item_id, asset_id, settings, status); });
    }
}

void LLFloaterSettingsPicker::onAssetLoadedCb(LLHandle<LLFloater> handle, LLUUID item_id, LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (handle.isDead() || status)
    {
        return;
    }

    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(handle.get());

    if (picker->mSettingItemID != item_id)
    {
        return;
    }

    picker->onAssetLoaded(asset_id, settings);
}

void LLFloaterSettingsPicker::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings)
{
    LLComboBox* track_selection = getChild<LLComboBox>(CMB_TRACK_SELECTION);
    track_selection->clear();
    track_selection->removeall();

    if (!settings)
    {
        LL_WARNS() << "Failed to load asset " << asset_id << LL_ENDL;
        return;
    }

    LLSettingsDay::ptr_t pday = std::dynamic_pointer_cast<LLSettingsDay>(settings);
    if (!pday)
    {
        LL_WARNS() << "Wrong asset type received by id " << asset_id << LL_ENDL;
        return;
    }

    if (mTrackMode == TRACK_WATER)
    {
        track_selection->add(getString(STR_TRACK_WATER), LLSD::Integer(LLSettingsDay::TRACK_WATER), ADD_TOP, true);
    }
    else if (mTrackMode == TRACK_SKY)
    {
        // track 1 always present
        track_selection->add(getString(STR_TRACK_GROUND), LLSD::Integer(LLSettingsDay::TRACK_GROUND_LEVEL), ADD_TOP, true);
        LLUIString formatted_label = getString(STR_TRACK_SKY);
        for (U32 i = 2; i < LLSettingsDay::TRACK_MAX; i++)
        {
            if (!pday->isTrackEmpty(i))
            {
                formatted_label.setArg("[NUM]", llformat("%d", i));
                track_selection->add(formatted_label.getString(), LLSD::Integer(i), ADD_TOP, true);
            }
        }
    }

    mSettingAssetID = asset_id;
    track_selection->setEnabled(true);
    track_selection->selectFirstItem();
    getChild<LLView>(BTN_SELECT)->setEnabled(true);
}

void LLFloaterSettingsPicker::onButtonCancel()
{
    closeFloater();
}

void LLFloaterSettingsPicker::onButtonSelect()
{
    applySelectedItemAndCloseFloater();
}

void LLFloaterSettingsPicker::applySelectedItemAndCloseFloater()
{
    if (mCommitSignal)
    {
        LLSD res;
        res["ItemId"] = mSettingItemID;
        res["Track"] = getChild<LLComboBox>(CMB_TRACK_SELECTION)->getValue();
        (*mCommitSignal)(this, res);
    }
    closeFloater();
}

bool LLFloaterSettingsPicker::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    bool result = false;
    if (mSettingItemID.notNull()
        && mInventoryPanel)
    {
        S32 inventory_x = x - mInventoryPanel->getRect().mLeft;
        S32 inventory_y = y - mInventoryPanel->getRect().mBottom;
        if (mInventoryPanel->parentPointInView(inventory_x, inventory_y))
        {
            // make sure item is selected and visible
            LLFolderViewItem* item_viewp = mInventoryPanel->getItemByID(mSettingItemID);
            if (item_viewp && item_viewp->getIsCurSelection() && item_viewp->getVisible())
            {
                LLRect target_rect;
                item_viewp->localRectToOtherView(item_viewp->getLocalRect(), &target_rect, this);
                if (target_rect.pointInRect(x, y))
                {
                    // Quick-apply
                    applySelectedItemAndCloseFloater();
                    // hit inside panel on selected item, double click should do nothing
                    result = true;
                }
            }
        }
    }

    if (!result)
    {
        result = LLFloater::handleDoubleClick(x, y, mask);
    }
    return result;
}

bool LLFloaterSettingsPicker::handleKeyHere(KEY key, MASK mask)
{
    if ((key == KEY_RETURN) && (mask == MASK_NONE))
    {
        LLFolderViewItem* item_viewp = mInventoryPanel->getItemByID(mSettingItemID);
        if (item_viewp && item_viewp->getIsCurSelection() && item_viewp->getVisible())
        {
            // Quick-apply
            applySelectedItemAndCloseFloater();
            return true;
        }
    }

    return LLFloater::handleKeyHere(key, mask);
}

void LLFloaterSettingsPicker::onFocusLost()
{
    if (isInVisibleChain())
    {
        closeFloater();
    }
}

//=========================================================================
void LLFloaterSettingsPicker::setActive(bool active)
{
    mActive = active;
}

void LLFloaterSettingsPicker::setSettingsItemId(const LLUUID &settings_id, bool set_selection)
{
    if (mSettingItemID != settings_id && mActive)
    {
        mNoCopySettingsSelected = false;
        mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
        mSettingItemID = settings_id;
        if (mSettingItemID.isNull())
        {
            mInventoryPanel->getRootFolder()->clearSelection();
        }
        else
        {
            LLInventoryItem* itemp = gInventory.getItem(settings_id);
            if (itemp && !itemp->getPermissions().allowCopyBy(gAgent.getID()))
            {
                mNoCopySettingsSelected = true;
            }
        }

        if (set_selection)
        {
            mInventoryPanel->setSelection(settings_id, TAKE_FOCUS_NO);
        }
    }
}

LLInventoryItem* LLFloaterSettingsPicker::findItem(const LLUUID& asset_id, bool copyable_only, bool ignore_library)
{
    if (asset_id.isNull())
        return nullptr;

    LLViewerInventoryCategory::cat_array_t cats;
    LLViewerInventoryItem::item_array_t items;
    LLAssetIDMatches asset_id_matches(asset_id);

    gInventory.collectDescendentsIf(LLUUID::null,
        cats,
        items,
        LLInventoryModel::INCLUDE_TRASH,
        asset_id_matches);

    if (items.size())
    {
        // search for copyable version first
        for (S32 i = 0; i < items.size(); i++)
        {
            LLInventoryItem* itemp = items[i];
            LLPermissions item_permissions = itemp->getPermissions();
            if (item_permissions.allowCopyBy(gAgent.getID(), gAgent.getGroupID()))
            {
                if(!ignore_library || !gInventory.isObjectDescendentOf(itemp->getUUID(),gInventory.getLibraryRootFolderID()))
                {
                    return itemp;
                }
            }
        }
        // otherwise just return first instance, unless copyable requested
        if (copyable_only)
        {
            return nullptr;
        }
        else
        {
            if(!ignore_library || !gInventory.isObjectDescendentOf(items[0]->getUUID(),gInventory.getLibraryRootFolderID()))
            {
                return items[0];
            }
        }
    }

    return nullptr;
}
