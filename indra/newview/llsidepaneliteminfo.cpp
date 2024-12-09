/**
 * @file llsidepaneliteminfo.cpp
 * @brief A floater which shows an inventory item's properties.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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
#include "llsidepaneliteminfo.h"

#include "roles_constants.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llbutton.h"
#include "llcallbacklist.h"
#include "llcombobox.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "lliconctrl.h"
#include "llinventorydefines.h"
#include "llinventoryicon.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "lllineeditor.h"
#include "llradiogroup.h"
#include "llslurl.h"
#include "lltexteditor.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llexperiencecache.h"
#include "lltrans.h"
#include "llviewerregion.h"


class PropertiesChangedCallback : public LLInventoryCallback
{
public:
    PropertiesChangedCallback(LLHandle<LLPanel> sidepanel_handle, LLUUID &item_id, S32 id)
        : mHandle(sidepanel_handle), mItemId(item_id), mId(id)
    {}

    void fire(const LLUUID &inv_item)
    {
        // inv_item can be null for some reason
        LLSidepanelItemInfo* sidepanel = dynamic_cast<LLSidepanelItemInfo*>(mHandle.get());
        if (sidepanel)
        {
            // sidepanel waits only for most recent update
            sidepanel->onUpdateCallback(mItemId, mId);
        }
    }
private:
    LLHandle<LLPanel> mHandle;
    LLUUID mItemId;
    S32 mId;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLObjectInventoryObserver
//
// Helper class to watch for changes in an object inventory.
// Used to update item properties in LLSidepanelItemInfo.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLObjectInventoryObserver : public LLVOInventoryListener
{
public:
    LLObjectInventoryObserver(LLSidepanelItemInfo* floater, LLViewerObject* object)
        : mFloater(floater)
    {
        registerVOInventoryListener(object, NULL);
    }
    virtual ~LLObjectInventoryObserver()
    {
        removeVOInventoryListener();
    }
    /*virtual*/ void inventoryChanged(LLViewerObject* object,
                                      LLInventoryObject::object_list_t* inventory,
                                      S32 serial_num,
                                      void* user_data);
private:
    LLSidepanelItemInfo* mFloater;  // Not a handle because LLSidepanelItemInfo is managing LLObjectInventoryObserver
};

/*virtual*/
void LLObjectInventoryObserver::inventoryChanged(LLViewerObject* object,
                                                 LLInventoryObject::object_list_t* inventory,
                                                 S32 serial_num,
                                                 void* user_data)
{
    mFloater->dirty();
}

///----------------------------------------------------------------------------
/// Class LLSidepanelItemInfo
///----------------------------------------------------------------------------

static LLPanelInjector<LLSidepanelItemInfo> t_item_info("sidepanel_item_info");

// Default constructor
LLSidepanelItemInfo::LLSidepanelItemInfo(const LLPanel::Params& p)
    : LLPanel(p)
    , mItemID(LLUUID::null)
    , mObjectInventoryObserver(NULL)
    , mUpdatePendingId(-1)
    , mIsDirty(false) /*Not ready*/
    , mParentFloater(NULL)
{
    gInventory.addObserver(this);
    gIdleCallbacks.addFunction(&LLSidepanelItemInfo::onIdle, (void*)this);
}

// Destroys the object
LLSidepanelItemInfo::~LLSidepanelItemInfo()
{
    gInventory.removeObserver(this);
    gIdleCallbacks.deleteFunction(&LLSidepanelItemInfo::onIdle, (void*)this);

    stopObjectInventoryObserver();

    if (mOwnerCacheConnection.connected())
    {
        mOwnerCacheConnection.disconnect();
    }
    if (mCreatorCacheConnection.connected())
    {
        mCreatorCacheConnection.disconnect();
    }
}

// virtual
bool LLSidepanelItemInfo::postBuild()
{
    mChangeThumbnailBtn = getChild<LLUICtrl>("change_thumbnail_btn");
    mItemTypeIcon = getChild<LLIconCtrl>("item_type_icon");
    mLabelOwnerName = getChild<LLTextBox>("LabelOwnerName");
    mLabelCreatorName = getChild<LLTextBox>("LabelCreatorName");

    getChild<LLLineEditor>("LabelItemName")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
    getChild<LLUICtrl>("LabelItemName")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitName,this));
    getChild<LLUICtrl>("LabelItemDesc")->setCommitCallback(boost::bind(&LLSidepanelItemInfo:: onCommitDescription, this));
    // Thumnail edition
    mChangeThumbnailBtn->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onEditThumbnail, this));
    // acquired date
    // owner permissions
    // Permissions debug text
    // group permissions
    getChild<LLUICtrl>("CheckShareWithGroup")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitPermissions, this, _1));
    // everyone permissions
    getChild<LLUICtrl>("CheckEveryoneCopy")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitPermissions, this, _1));
    // next owner permissions
    getChild<LLUICtrl>("CheckNextOwnerModify")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitPermissions, this, _1));
    getChild<LLUICtrl>("CheckNextOwnerCopy")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitPermissions, this, _1));
    getChild<LLUICtrl>("CheckNextOwnerTransfer")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitPermissions, this, _1));
    // Mark for sale or not, and sale info
    getChild<LLUICtrl>("CheckPurchase")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitSaleInfo, this, _1));
    // Change sale type, and sale info
    getChild<LLUICtrl>("ComboBoxSaleType")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitSaleInfo, this, _1));
    // "Price" label for edit
    getChild<LLUICtrl>("Edit Cost")->setCommitCallback(boost::bind(&LLSidepanelItemInfo::onCommitSaleInfo, this, _1));
    refresh();
    return true;
}

void LLSidepanelItemInfo::setObjectID(const LLUUID& object_id)
{
    mObjectID = object_id;

    // Start monitoring changes in the object inventory to update
    // selected inventory item properties in Item Profile panel. See STORM-148.
    startObjectInventoryObserver();
    mUpdatePendingId = -1;
}

void LLSidepanelItemInfo::setItemID(const LLUUID& item_id)
{
    if (mItemID != item_id)
    {
        mItemID = item_id;
        mUpdatePendingId = -1;
    }
    dirty();
}

void LLSidepanelItemInfo::setParentFloater(LLFloater* parent)
{
    mParentFloater = parent;
}

const LLUUID& LLSidepanelItemInfo::getObjectID() const
{
    return mObjectID;
}

const LLUUID& LLSidepanelItemInfo::getItemID() const
{
    return mItemID;
}

void LLSidepanelItemInfo::onUpdateCallback(const LLUUID& item_id, S32 received_update_id)
{
    if (mItemID == item_id && mUpdatePendingId == received_update_id)
    {
        mUpdatePendingId = -1;
        refresh();
    }
}

void LLSidepanelItemInfo::reset()
{
    mObjectID = LLUUID::null;
    mItemID = LLUUID::null;

    stopObjectInventoryObserver();
    dirty();
}

void LLSidepanelItemInfo::refresh()
{
    LLViewerInventoryItem* item = findItem();
    if(item)
    {
        const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
        bool in_trash = (item->getUUID() == trash_id) || gInventory.isObjectDescendentOf(item->getUUID(), trash_id);
        if (in_trash && mParentFloater)
        {
            // Close properties when moving to trash
            // Aren't supposed to view properties from trash
            mParentFloater->closeFloater();
        }
        else
        {
            refreshFromItem(item);
        }
        return;
    }

    if (mObjectID.notNull())
    {
        LLViewerObject* object = gObjectList.findObject(mObjectID);
        if (object)
        {
            // Object exists, but object's content is not nessesary
            // loaded, so assume item exists as well
            return;
        }
    }

    if (mParentFloater)
    {
        // if we failed to get item, it likely no longer exists
        mParentFloater->closeFloater();
    }
}

void LLSidepanelItemInfo::refreshFromItem(LLViewerInventoryItem* item)
{
    ////////////////////////
    // PERMISSIONS LOOKUP //
    ////////////////////////

    llassert(item);
    if (!item) return;

    if (mUpdatePendingId != -1)
    {
        return;
    }

    // do not enable the UI for incomplete items.
    bool is_complete = item->isFinished();
    const bool cannot_restrict_permissions = LLInventoryType::cannotRestrictPermissions(item->getInventoryType());
    const bool is_calling_card = (item->getInventoryType() == LLInventoryType::IT_CALLINGCARD);
    const bool is_settings = (item->getInventoryType() == LLInventoryType::IT_SETTINGS);
    const LLPermissions& perm = item->getPermissions();
    const bool can_agent_manipulate = gAgent.allowOperation(PERM_OWNER, perm,
                                GP_OBJECT_MANIPULATE);
    const bool can_agent_sell = gAgent.allowOperation(PERM_OWNER, perm,
                              GP_OBJECT_SET_SALE) &&
        !cannot_restrict_permissions;
    const bool is_link = item->getIsLinkType();

    const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
    bool not_in_trash = (item->getUUID() != trash_id) && !gInventory.isObjectDescendentOf(item->getUUID(), trash_id);

    // You need permission to modify the object to modify an inventory
    // item in it.
    LLViewerObject* object = NULL;
    if(!mObjectID.isNull()) object = gObjectList.findObject(mObjectID);
    bool is_obj_modify = true;
    if(object)
    {
        is_obj_modify = object->permOwnerModify();
    }

    if(item->getInventoryType() == LLInventoryType::IT_LSL)
    {
        getChildView("LabelItemExperienceTitle")->setVisible(true);
        LLTextBox* tb = getChild<LLTextBox>("LabelItemExperience");
        tb->setText(getString("loading_experience"));
        tb->setVisible(true);
        std::string url = std::string();
        if(object && object->getRegion())
        {
            url = object->getRegion()->getCapability("GetMetadata");
        }
        LLExperienceCache::instance().fetchAssociatedExperience(item->getParentUUID(), item->getUUID(), url,
                boost::bind(&LLSidepanelItemInfo::setAssociatedExperience, getDerivedHandle<LLSidepanelItemInfo>(), _1));
    }

    //////////////////////
    // ITEM NAME & DESC //
    //////////////////////
    bool is_modifiable = gAgent.allowOperation(PERM_MODIFY, perm,
                                               GP_OBJECT_MANIPULATE)
        && is_obj_modify && is_complete && not_in_trash;

    getChildView("LabelItemNameTitle")->setEnabled(true);
    getChildView("LabelItemName")->setEnabled(is_modifiable && !is_calling_card); // for now, don't allow rename of calling cards
    getChild<LLUICtrl>("LabelItemName")->setValue(item->getName());
    getChildView("LabelItemDescTitle")->setEnabled(true);
    getChildView("LabelItemDesc")->setEnabled(is_modifiable);
    getChild<LLUICtrl>("LabelItemDesc")->setValue(item->getDescription());
    getChild<LLUICtrl>("item_thumbnail")->setValue(item->getThumbnailUUID());

    LLUIImagePtr icon_img = LLInventoryIcon::getIcon(item->getType(), item->getInventoryType(), item->getFlags(), false);
    mItemTypeIcon->setImage(icon_img);

    // Style for creator and owner links
    LLStyle::Params style_params;
    LLUIColor link_color = LLUIColorTable::instance().getColor("HTMLLinkColor");
    style_params.color = link_color;
    style_params.readonly_color = link_color;
    style_params.is_link = true; // link will be added later
    const LLFontGL* fontp = mLabelCreatorName->getFont();
    style_params.font.name = LLFontGL::nameFromFont(fontp);
    style_params.font.size = LLFontGL::sizeFromFont(fontp);
    style_params.font.style = "UNDERLINE";

    //////////////////
    // CREATOR NAME //
    //////////////////
    if(!gCacheName) return;
    if(!gAgent.getRegion()) return;

    if (item->getCreatorUUID().notNull())
    {
        LLUUID creator_id = item->getCreatorUUID();
        std::string slurl =
            LLSLURL("agent", creator_id, "inspect").getSLURLString();

        style_params.link_href = slurl;

        LLAvatarName av_name;
        if (LLAvatarNameCache::get(creator_id, &av_name))
        {
            updateCreatorName(creator_id, av_name, style_params);
        }
        else
        {
            if (mCreatorCacheConnection.connected())
            {
                mCreatorCacheConnection.disconnect();
            }
            mLabelCreatorName->setText(LLTrans::getString("None"));
            mCreatorCacheConnection = LLAvatarNameCache::get(creator_id, boost::bind(&LLSidepanelItemInfo::updateCreatorName, this, _1, _2, style_params));
        }

        getChildView("LabelCreatorTitle")->setEnabled(true);
        mLabelCreatorName->setEnabled(true);
    }
    else
    {
        getChildView("LabelCreatorTitle")->setEnabled(false);
        mLabelCreatorName->setEnabled(false);
        mLabelCreatorName->setValue(getString("unknown_multiple"));
    }

    ////////////////
    // OWNER NAME //
    ////////////////
    if(perm.isOwned())
    {
        std::string slurl;
        if (perm.isGroupOwned())
        {
            LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(perm.getGroup());

            slurl = LLSLURL("group", perm.getGroup(), "inspect").getSLURLString();
            style_params.link_href = slurl;
            if (group_data && group_data->isGroupPropertiesDataComplete())
            {
                mLabelOwnerName->setText(group_data->mName, style_params);
            }
            else
            {
                // Triggers refresh
                LLGroupMgr::getInstance()->sendGroupPropertiesRequest(perm.getGroup());

                std::string name;
                gCacheName->getGroupName(perm.getGroup(), name);
                mLabelOwnerName->setText(name, style_params);
            }
        }
        else
        {
            LLUUID owner_id = perm.getOwner();
            slurl = LLSLURL("agent", owner_id, "inspect").getSLURLString();

            style_params.link_href = slurl;
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(owner_id, &av_name))
            {
                updateOwnerName(owner_id, av_name, style_params);
            }
            else
            {
                if (mOwnerCacheConnection.connected())
                {
                    mOwnerCacheConnection.disconnect();
                }
                mLabelOwnerName->setText(LLTrans::getString("None"));
                mOwnerCacheConnection = LLAvatarNameCache::get(owner_id, boost::bind(&LLSidepanelItemInfo::updateOwnerName, this, _1, _2, style_params));
            }
        }
        getChildView("LabelOwnerTitle")->setEnabled(true);
        mLabelOwnerName->setEnabled(true);
    }
    else
    {
        getChildView("LabelOwnerTitle")->setEnabled(false);
        mLabelOwnerName->setEnabled(false);
        mLabelOwnerName->setValue(getString("public"));
    }

    // Not yet supported for task inventories
    mChangeThumbnailBtn->setEnabled(mObjectID.isNull() && ALEXANDRIA_LINDEN_ID != perm.getOwner());

    ////////////
    // ORIGIN //
    ////////////

    if (object)
    {
        getChild<LLUICtrl>("origin")->setValue(getString("origin_inworld"));
    }
    else
    {
        getChild<LLUICtrl>("origin")->setValue(getString("origin_inventory"));
    }

    //////////////////
    // ACQUIRE DATE //
    //////////////////

    time_t time_utc = item->getCreationDate();
    if (0 == time_utc)
    {
        getChild<LLUICtrl>("LabelAcquiredDate")->setValue(getString("unknown"));
    }
    else
    {
        static bool use_24h = gSavedSettings.getBOOL("Use24HourClock");
        std::string timeStr = use_24h ? getString("acquiredDate") : getString("acquiredDateAMPM");
        LLSD substitution;
        substitution["datetime"] = (S32) time_utc;
        LLStringUtil::format (timeStr, substitution);
        getChild<LLUICtrl>("LabelAcquiredDate")->setValue(timeStr);
    }

    //////////////////////////////////////
    // PERMISSIONS AND SALE ITEM HIDING //
    //////////////////////////////////////

    const std::string perm_and_sale_items[]={
        "perms_inv",
        "perm_modify",
        "CheckOwnerModify",
        "CheckOwnerCopy",
        "CheckOwnerTransfer",
        "GroupLabel",
        "CheckShareWithGroup",
        "AnyoneLabel",
        "CheckEveryoneCopy",
        "NextOwnerLabel",
        "CheckNextOwnerModify",
        "CheckNextOwnerCopy",
        "CheckNextOwnerTransfer",
        "CheckPurchase",
        "ComboBoxSaleType",
        "Edit Cost"
    };

    const std::string debug_items[]={
        "BaseMaskDebug",
        "OwnerMaskDebug",
        "GroupMaskDebug",
        "EveryoneMaskDebug",
        "NextMaskDebug"
    };

    // Hide permissions checkboxes and labels and for sale info if in the trash
    // or ui elements don't apply to these objects and return from function
    if (!not_in_trash || cannot_restrict_permissions)
    {
        for(size_t t=0; t<LL_ARRAY_SIZE(perm_and_sale_items); ++t)
        {
            getChildView(perm_and_sale_items[t])->setVisible(false);
        }

        for(size_t t=0; t<LL_ARRAY_SIZE(debug_items); ++t)
        {
            getChildView(debug_items[t])->setVisible(false);
        }
        return;
    }
    else // Make sure perms and sale ui elements are visible
    {
        for(size_t t=0; t<LL_ARRAY_SIZE(perm_and_sale_items); ++t)
        {
            getChildView(perm_and_sale_items[t])->setVisible(true);
        }
    }

    ///////////////////////
    // OWNER PERMISSIONS //
    ///////////////////////

    U32 base_mask       = perm.getMaskBase();
    U32 owner_mask      = perm.getMaskOwner();
    U32 group_mask      = perm.getMaskGroup();
    U32 everyone_mask   = perm.getMaskEveryone();
    U32 next_owner_mask = perm.getMaskNextOwner();

    getChildView("CheckOwnerModify")->setEnabled(false);
    getChild<LLUICtrl>("CheckOwnerModify")->setValue(LLSD((bool)(owner_mask & PERM_MODIFY)));
    getChildView("CheckOwnerCopy")->setEnabled(false);
    getChild<LLUICtrl>("CheckOwnerCopy")->setValue(LLSD((bool)(owner_mask & PERM_COPY)));
    getChildView("CheckOwnerTransfer")->setEnabled(false);
    getChild<LLUICtrl>("CheckOwnerTransfer")->setValue(LLSD((bool)(owner_mask & PERM_TRANSFER)));

    ///////////////////////
    // DEBUG PERMISSIONS //
    ///////////////////////

    if( gSavedSettings.getBOOL("DebugPermissions") )
    {
        childSetVisible("layout_debug_permissions", true);

        bool slam_perm          = false;
        bool overwrite_group    = false;
        bool overwrite_everyone = false;

        if (item->getType() == LLAssetType::AT_OBJECT)
        {
            U32 flags = item->getFlags();
            slam_perm           = flags & LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_PERM;
            overwrite_everyone  = flags & LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
            overwrite_group     = flags & LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
        }

        std::string perm_string;

        perm_string = "B: ";
        perm_string += mask_to_string(base_mask);
        getChild<LLUICtrl>("BaseMaskDebug")->setValue(perm_string);

        perm_string = "O: ";
        perm_string += mask_to_string(owner_mask);
        getChild<LLUICtrl>("OwnerMaskDebug")->setValue(perm_string);

        perm_string = "G";
        perm_string += overwrite_group ? "*: " : ": ";
        perm_string += mask_to_string(group_mask);
        getChild<LLUICtrl>("GroupMaskDebug")->setValue(perm_string);

        perm_string = "E";
        perm_string += overwrite_everyone ? "*: " : ": ";
        perm_string += mask_to_string(everyone_mask);
        getChild<LLUICtrl>("EveryoneMaskDebug")->setValue(perm_string);

        perm_string = "N";
        perm_string += slam_perm ? "*: " : ": ";
        perm_string += mask_to_string(next_owner_mask);
        getChild<LLUICtrl>("NextMaskDebug")->setValue(perm_string);
    }
    else
    {
        childSetVisible("layout_debug_permissions", false);
    }

    /////////////
    // SHARING //
    /////////////

    // Check for ability to change values.
    if (is_link || cannot_restrict_permissions)
    {
        getChildView("CheckShareWithGroup")->setEnabled(false);
        getChildView("CheckEveryoneCopy")->setEnabled(false);
    }
    else if (is_obj_modify && can_agent_manipulate)
    {
        getChildView("CheckShareWithGroup")->setEnabled(true);
        getChildView("CheckEveryoneCopy")->setEnabled((owner_mask & PERM_COPY) && (owner_mask & PERM_TRANSFER));
    }
    else
    {
        getChildView("CheckShareWithGroup")->setEnabled(false);
        getChildView("CheckEveryoneCopy")->setEnabled(false);
    }

    // Set values.
    bool is_group_copy = group_mask & PERM_COPY;
    bool is_group_modify = group_mask & PERM_MODIFY;
    bool is_group_move = group_mask & PERM_MOVE;

    if (is_group_copy && is_group_modify && is_group_move)
    {
        getChild<LLUICtrl>("CheckShareWithGroup")->setValue(LLSD((bool)true));
        if (LLCheckBoxCtrl* ctl = getChild<LLCheckBoxCtrl>("CheckShareWithGroup"))
        {
            ctl->setTentative(false);
        }
    }
    else if (!is_group_copy && !is_group_modify && !is_group_move)
    {
        getChild<LLUICtrl>("CheckShareWithGroup")->setValue(LLSD((bool)false));
        if (LLCheckBoxCtrl* ctl = getChild<LLCheckBoxCtrl>("CheckShareWithGroup"))
        {
            ctl->setTentative(false);
        }
    }
    else
    {
        if (LLCheckBoxCtrl* ctl = getChild<LLCheckBoxCtrl>("CheckShareWithGroup"))
        {
            ctl->setTentative(!ctl->getEnabled());
            ctl->set(true);
        }
    }

    getChild<LLUICtrl>("CheckEveryoneCopy")->setValue(LLSD((bool)(everyone_mask & PERM_COPY)));

    ///////////////
    // SALE INFO //
    ///////////////

    const LLSaleInfo& sale_info = item->getSaleInfo();
    bool is_for_sale = sale_info.isForSale();
    LLComboBox* combo_sale_type = getChild<LLComboBox>("ComboBoxSaleType");
    LLUICtrl* edit_cost = getChild<LLUICtrl>("Edit Cost");

    // Check for ability to change values.
    if (is_obj_modify && can_agent_sell
        && gAgent.allowOperation(PERM_TRANSFER, perm, GP_OBJECT_MANIPULATE))
    {
        getChildView("CheckPurchase")->setEnabled(is_complete);

        getChildView("NextOwnerLabel")->setEnabled(true);
        getChildView("CheckNextOwnerModify")->setEnabled((base_mask & PERM_MODIFY) && !cannot_restrict_permissions);
        getChildView("CheckNextOwnerCopy")->setEnabled((base_mask & PERM_COPY) && !cannot_restrict_permissions && !is_settings);
        getChildView("CheckNextOwnerTransfer")->setEnabled((next_owner_mask & PERM_COPY) && !cannot_restrict_permissions);

        combo_sale_type->setEnabled(is_complete && is_for_sale);
        edit_cost->setEnabled(is_complete && is_for_sale);
    }
    else
    {
        getChildView("CheckPurchase")->setEnabled(false);

        getChildView("NextOwnerLabel")->setEnabled(false);
        getChildView("CheckNextOwnerModify")->setEnabled(false);
        getChildView("CheckNextOwnerCopy")->setEnabled(false);
        getChildView("CheckNextOwnerTransfer")->setEnabled(false);

        combo_sale_type->setEnabled(false);
        edit_cost->setEnabled(false);
    }

    // Hide any properties that are not relevant to settings
    if (is_settings)
    {
        getChild<LLUICtrl>("GroupLabel")->setEnabled(false);
        getChild<LLUICtrl>("GroupLabel")->setVisible(false);
        getChild<LLUICtrl>("CheckShareWithGroup")->setEnabled(false);
        getChild<LLUICtrl>("CheckShareWithGroup")->setVisible(false);
        getChild<LLUICtrl>("AnyoneLabel")->setEnabled(false);
        getChild<LLUICtrl>("AnyoneLabel")->setVisible(false);
        getChild<LLUICtrl>("CheckEveryoneCopy")->setEnabled(false);
        getChild<LLUICtrl>("CheckEveryoneCopy")->setVisible(false);
        getChild<LLUICtrl>("CheckPurchase")->setEnabled(false);
        getChild<LLUICtrl>("CheckPurchase")->setVisible(false);
        getChild<LLUICtrl>("ComboBoxSaleType")->setEnabled(false);
        getChild<LLUICtrl>("ComboBoxSaleType")->setVisible(false);
        getChild<LLUICtrl>("Edit Cost")->setEnabled(false);
        getChild<LLUICtrl>("Edit Cost")->setVisible(false);
    }

    // Set values.
    getChild<LLUICtrl>("CheckPurchase")->setValue(is_for_sale);
    getChild<LLUICtrl>("CheckNextOwnerModify")->setValue(LLSD(bool(next_owner_mask & PERM_MODIFY)));
    getChild<LLUICtrl>("CheckNextOwnerCopy")->setValue(LLSD(bool(next_owner_mask & PERM_COPY)));
    getChild<LLUICtrl>("CheckNextOwnerTransfer")->setValue(LLSD(bool(next_owner_mask & PERM_TRANSFER)));

    if (is_for_sale)
    {
        S32 numerical_price;
        numerical_price = sale_info.getSalePrice();
        edit_cost->setValue(llformat("%d",numerical_price));
        combo_sale_type->setValue(sale_info.getSaleType());
    }
    else
    {
        edit_cost->setValue(llformat("%d",0));
        combo_sale_type->setValue(LLSaleInfo::FS_COPY);
    }
}

void LLSidepanelItemInfo::updateCreatorName(const LLUUID& creator_id, const LLAvatarName& creator_name, const LLStyle::Params& style_params)
{
    if (mCreatorCacheConnection.connected())
    {
        mCreatorCacheConnection.disconnect();
    }
    std::string name = creator_name.getCompleteName();
    mLabelCreatorName->setText(name, style_params);
}

void LLSidepanelItemInfo::updateOwnerName(const LLUUID& owner_id, const LLAvatarName& owner_name, const LLStyle::Params& style_params)
{
    if (mOwnerCacheConnection.connected())
    {
        mOwnerCacheConnection.disconnect();
    }
    std::string name = owner_name.getCompleteName();
    mLabelOwnerName->setText(name, style_params);
}

void LLSidepanelItemInfo::changed(U32 mask)
{
    const LLUUID& item_id = getItemID();
    if (getObjectID().notNull() || item_id.isNull())
    {
        // Task inventory or not set up yet
        return;
    }

    const std::set<LLUUID>& mChangedItemIDs = gInventory.getChangedIDs();
    std::set<LLUUID>::const_iterator it;

    for (it = mChangedItemIDs.begin(); it != mChangedItemIDs.end(); it++)
    {
        // set dirty for 'item profile panel' only if changed item is the item for which 'item profile panel' is shown (STORM-288)
        if (*it == item_id)
        {
            // if there's a change we're interested in.
            if((mask & (LLInventoryObserver::LABEL | LLInventoryObserver::INTERNAL | LLInventoryObserver::REMOVE)) != 0)
            {
                dirty();
            }
        }
    }
}

void LLSidepanelItemInfo::dirty()
{
    mIsDirty = true;
}

// static
void LLSidepanelItemInfo::onIdle( void* user_data )
{
    LLSidepanelItemInfo* self = reinterpret_cast<LLSidepanelItemInfo*>(user_data);

    if( self->mIsDirty )
    {
        self->refresh();
        self->mIsDirty = false;
    }
}

void LLSidepanelItemInfo::setAssociatedExperience( LLHandle<LLSidepanelItemInfo> hInfo, const LLSD& experience )
{
    LLSidepanelItemInfo* info = hInfo.get();
    if(info)
    {
        LLUUID id;
        if(experience.has(LLExperienceCache::EXPERIENCE_ID))
        {
            id=experience[LLExperienceCache::EXPERIENCE_ID].asUUID();
        }
        if(id.notNull())
        {
            info->getChild<LLTextBox>("LabelItemExperience")->setText(LLSLURL("experience", id, "profile").getSLURLString());
        }
        else
        {
            info->getChild<LLTextBox>("LabelItemExperience")->setText(LLTrans::getString("ExperienceNameNull"));
        }
    }
}


void LLSidepanelItemInfo::startObjectInventoryObserver()
{
    if (!mObjectInventoryObserver)
    {
        stopObjectInventoryObserver();

        // Previous object observer should be removed before starting to observe a new object.
        llassert(mObjectInventoryObserver == NULL);
    }

    if (mObjectID.isNull())
    {
        LL_WARNS() << "Empty object id passed to inventory observer" << LL_ENDL;
        return;
    }

    LLViewerObject* object = gObjectList.findObject(mObjectID);

    mObjectInventoryObserver = new LLObjectInventoryObserver(this, object);
}

void LLSidepanelItemInfo::stopObjectInventoryObserver()
{
    delete mObjectInventoryObserver;
    mObjectInventoryObserver = NULL;
}

void LLSidepanelItemInfo::setPropertiesFieldsEnabled(bool enabled)
{
    const std::string fields[] = {
        "CheckOwnerModify",
        "CheckOwnerCopy",
        "CheckOwnerTransfer",
        "CheckShareWithGroup",
        "CheckEveryoneCopy",
        "CheckNextOwnerModify",
        "CheckNextOwnerCopy",
        "CheckNextOwnerTransfer",
        "CheckPurchase",
        "Edit Cost"
    };
    for (size_t t = 0; t<LL_ARRAY_SIZE(fields); ++t)
    {
        getChildView(fields[t])->setEnabled(false);
    }
}

void LLSidepanelItemInfo::onClickCreator()
{
    LLViewerInventoryItem* item = findItem();
    if(!item) return;
    if(!item->getCreatorUUID().isNull())
    {
        LLAvatarActions::showProfile(item->getCreatorUUID());
    }
}

// static
void LLSidepanelItemInfo::onClickOwner()
{
    LLViewerInventoryItem* item = findItem();
    if(!item) return;
    if(item->getPermissions().isGroupOwned())
    {
        LLGroupActions::show(item->getPermissions().getGroup());
    }
    else
    {
        LLAvatarActions::showProfile(item->getPermissions().getOwner());
    }
}

// static
void LLSidepanelItemInfo::onCommitName()
{
    //LL_INFOS() << "LLSidepanelItemInfo::onCommitName()" << LL_ENDL;
    LLViewerInventoryItem* item = findItem();
    if(!item)
    {
        return;
    }
    LLLineEditor* labelItemName = getChild<LLLineEditor>("LabelItemName");

    if(labelItemName&&
       (item->getName() != labelItemName->getText()) &&
       (gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE)) )
    {
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
        new_item->rename(labelItemName->getText());
        onCommitChanges(new_item);
    }
}

void LLSidepanelItemInfo::onCommitDescription()
{
    //LL_INFOS() << "LLSidepanelItemInfo::onCommitDescription()" << LL_ENDL;
    LLViewerInventoryItem* item = findItem();
    if(!item) return;

    LLTextEditor* labelItemDesc = getChild<LLTextEditor>("LabelItemDesc");
    if(!labelItemDesc)
    {
        return;
    }
    if((item->getDescription() != labelItemDesc->getText()) &&
       (gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE)))
    {
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);

        new_item->setDescription(labelItemDesc->getText());
        onCommitChanges(new_item);
    }
}

void LLSidepanelItemInfo::onCommitPermissions(LLUICtrl* ctrl)
{
    if (ctrl)
    {
        // will be enabled by response from server
        ctrl->setEnabled(false);
    }
    updatePermissions();
}

void LLSidepanelItemInfo::updatePermissions()
{
    LLViewerInventoryItem* item = findItem();
    if(!item) return;

    bool is_group_owned;
    LLUUID owner_id;
    LLUUID group_id;
    LLPermissions perm(item->getPermissions());
    perm.getOwnership(owner_id, is_group_owned);

    if (is_group_owned && gAgent.hasPowerInGroup(owner_id, GP_OBJECT_MANIPULATE))
    {
        group_id = owner_id;
    }

    LLCheckBoxCtrl* CheckShareWithGroup = getChild<LLCheckBoxCtrl>("CheckShareWithGroup");

    if(CheckShareWithGroup)
    {
        perm.setGroupBits(gAgent.getID(), group_id,
                        CheckShareWithGroup->get(),
                        PERM_MODIFY | PERM_MOVE | PERM_COPY);
    }
    LLCheckBoxCtrl* CheckEveryoneCopy = getChild<LLCheckBoxCtrl>("CheckEveryoneCopy");
    if(CheckEveryoneCopy)
    {
        perm.setEveryoneBits(gAgent.getID(), group_id,
                         CheckEveryoneCopy->get(), PERM_COPY);
    }

    LLCheckBoxCtrl* CheckNextOwnerModify = getChild<LLCheckBoxCtrl>("CheckNextOwnerModify");
    if(CheckNextOwnerModify)
    {
        perm.setNextOwnerBits(gAgent.getID(), group_id,
                            CheckNextOwnerModify->get(), PERM_MODIFY);
    }
    LLCheckBoxCtrl* CheckNextOwnerCopy = getChild<LLCheckBoxCtrl>("CheckNextOwnerCopy");
    if(CheckNextOwnerCopy)
    {
        perm.setNextOwnerBits(gAgent.getID(), group_id,
                            CheckNextOwnerCopy->get(), PERM_COPY);
    }
    LLCheckBoxCtrl* CheckNextOwnerTransfer = getChild<LLCheckBoxCtrl>("CheckNextOwnerTransfer");
    if(CheckNextOwnerTransfer)
    {
        perm.setNextOwnerBits(gAgent.getID(), group_id,
                            CheckNextOwnerTransfer->get(), PERM_TRANSFER);
    }
    if(perm != item->getPermissions()
        && item->isFinished())
    {
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
        new_item->setPermissions(perm);
        U32 flags = new_item->getFlags();
        // If next owner permissions have changed (and this is an object)
        // then set the slam permissions flag so that they are applied on rez.
        if((perm.getMaskNextOwner()!=item->getPermissions().getMaskNextOwner())
           && (item->getType() == LLAssetType::AT_OBJECT))
        {
            flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_PERM;
        }
        // If everyone permissions have changed (and this is an object)
        // then set the overwrite everyone permissions flag so they
        // are applied on rez.
        if ((perm.getMaskEveryone()!=item->getPermissions().getMaskEveryone())
            && (item->getType() == LLAssetType::AT_OBJECT))
        {
            flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE;
        }
        // If group permissions have changed (and this is an object)
        // then set the overwrite group permissions flag so they
        // are applied on rez.
        if ((perm.getMaskGroup()!=item->getPermissions().getMaskGroup())
            && (item->getType() == LLAssetType::AT_OBJECT))
        {
            flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP;
        }
        new_item->setFlags(flags);
        onCommitChanges(new_item);
    }
    else
    {
        // need to make sure we don't just follow the click
        refresh();
    }
}

void LLSidepanelItemInfo::onEditThumbnail()
{
    LLSD data;
    data["task_id"] = mObjectID;
    data["item_id"] = mItemID;
    LLFloaterReg::showInstance("change_item_thumbnail", data);
}

void LLSidepanelItemInfo::onCommitSaleInfo(LLUICtrl* ctrl)
{
    if (ctrl)
    {
        // will be enabled by response from server
        ctrl->setEnabled(false);
    }
    //LL_INFOS() << "LLSidepanelItemInfo::onCommitSaleInfo()" << LL_ENDL;
    updateSaleInfo();
}

void LLSidepanelItemInfo::updateSaleInfo()
{
    LLViewerInventoryItem* item = findItem();
    if(!item) return;
    LLSaleInfo sale_info(item->getSaleInfo());
    if(!gAgent.allowOperation(PERM_TRANSFER, item->getPermissions(), GP_OBJECT_SET_SALE))
    {
        getChild<LLUICtrl>("CheckPurchase")->setValue(LLSD((bool)false));
    }

    if((bool)getChild<LLUICtrl>("CheckPurchase")->getValue())
    {
        // turn on sale info
        LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_COPY;

        LLComboBox* combo_sale_type = getChild<LLComboBox>("ComboBoxSaleType");
        if (combo_sale_type)
        {
            sale_type = static_cast<LLSaleInfo::EForSale>(combo_sale_type->getValue().asInteger());
        }

        if (sale_type == LLSaleInfo::FS_COPY
            && !gAgent.allowOperation(PERM_COPY, item->getPermissions(),
                                      GP_OBJECT_SET_SALE))
        {
            sale_type = LLSaleInfo::FS_ORIGINAL;
        }



        S32 price = -1;
        price =  getChild<LLUICtrl>("Edit Cost")->getValue().asInteger();;

        // Invalid data - turn off the sale
        if (price < 0)
        {
            sale_type = LLSaleInfo::FS_NOT;
            price = 0;
        }

        sale_info.setSaleType(sale_type);
        sale_info.setSalePrice(price);
    }
    else
    {
        sale_info.setSaleType(LLSaleInfo::FS_NOT);
    }
    if(sale_info != item->getSaleInfo()
        && item->isFinished())
    {
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);

        // Force an update on the sale price at rez
        if (item->getType() == LLAssetType::AT_OBJECT)
        {
            U32 flags = new_item->getFlags();
            flags |= LLInventoryItemFlags::II_FLAGS_OBJECT_SLAM_SALE;
            new_item->setFlags(flags);
        }

        new_item->setSaleInfo(sale_info);
        onCommitChanges(new_item);
    }
    else
    {
        // need to make sure we don't just follow the click
        refresh();
    }
}

void LLSidepanelItemInfo::onCommitChanges(LLPointer<LLViewerInventoryItem> item)
{
    if (item.isNull())
    {
        return;
    }

    if (mObjectID.isNull())
    {
        // This is in the agent's inventory.
        // Mark update as pending and wait only for most recent one in case user requested for couple
        // Once update arrives or any of ids change drop pending id.
        mUpdatePendingId++;
        LLPointer<LLInventoryCallback> callback = new PropertiesChangedCallback(getHandle(), mItemID, mUpdatePendingId);
        update_inventory_item(item.get(), callback);
        //item->updateServer(false);
        gInventory.updateItem(item);
        gInventory.notifyObservers();
    }
    else
    {
        // This is in an object's contents.
        LLViewerObject* object = gObjectList.findObject(mObjectID);
        if (object)
        {
            object->updateInventory(
                item,
                TASK_INVENTORY_ITEM_KEY,
                false);

            if (object->isSelected())
            {
                // Since object is selected (build floater is open) object will
                // receive properties update, detect serial mismatch, dirty and
                // reload inventory, meanwhile some other updates will refresh it.
                // So mark dirty early, this will prevent unnecessary changes
                // and download will be triggered by LLPanelObjectInventory - it
                // prevents flashing in content tab and some duplicated request.
                object->dirtyInventory();
            }
            setPropertiesFieldsEnabled(false);
        }
    }
}

LLViewerInventoryItem* LLSidepanelItemInfo::findItem() const
{
    LLViewerInventoryItem* item = NULL;
    if(mObjectID.isNull())
    {
        // it is in agent inventory
        item = gInventory.getItem(mItemID);
    }
    else
    {
        LLViewerObject* object = gObjectList.findObject(mObjectID);
        if(object)
        {
            item = static_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemID));
        }
    }
    return item;
}

// virtual
void LLSidepanelItemInfo::save()
{
    onCommitName();
    onCommitDescription();
    updatePermissions();
    updateSaleInfo();
}
