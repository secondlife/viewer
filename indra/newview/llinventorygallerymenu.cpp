/**
 * @file llinventorygallerymenu.cpp
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#include "llinventorygallery.h"
#include "llinventorygallerymenu.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llavataractions.h"
#include "llclipboard.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "lllandmarkactions.h"
#include "llmarketplacefunctions.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpreviewtexture.h"
#include "lltrans.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"

LLContextMenu* LLInventoryGalleryContextMenu::createMenu()
{
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

    registrar.add("Inventory.DoToSelected", boost::bind(&LLInventoryGalleryContextMenu::doToSelected, this, _2));
    registrar.add("Inventory.FileUploadLocation", boost::bind(&LLInventoryGalleryContextMenu::fileUploadLocation, this, _2));
    registrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH));
    registrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND));

    std::set<LLUUID> uuids(mUUIDs.begin(), mUUIDs.end());
    registrar.add("Inventory.Share", boost::bind(&LLAvatarActions::shareWithAvatars, uuids, gFloaterView->getParentFloater(mGallery)));

    enable_registrar.add("Inventory.CanSetUploadLocation", boost::bind(&LLInventoryGalleryContextMenu::canSetUploadLocation, this, _2));
    
    LLContextMenu* menu = createFromFile("menu_gallery_inventory.xml");

    updateMenuItemsVisibility(menu);

    return menu;
}

void LLInventoryGalleryContextMenu::doToSelected(const LLSD& userdata)
{
    std::string action = userdata.asString();
    LLInventoryObject* obj = gInventory.getObject(mUUIDs.front());
    if(!obj) return;

    if ("open_selected_folder" == action)
    {
        mGallery->setRootFolder(mUUIDs.front());
    }
    else if ("open_in_new_window" == action)
    {
        new_folder_window(mUUIDs.front());
    }
    else if ("properties" == action)
    {
        show_item_profile(mUUIDs.front());
    }
    else if ("restore" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLViewerInventoryCategory* cat = gInventory.getCategory(selected_id);
            if (cat)
            {
                const LLUUID new_parent = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cat->getType()));
                // do not restamp children on restore
                gInventory.changeCategoryParent(cat, new_parent, false);
            }
            else
            {
                LLViewerInventoryItem* item = gInventory.getItem(selected_id);
                if (item)
                {
                    bool is_snapshot = (item->getInventoryType() == LLInventoryType::IT_SNAPSHOT);

                    const LLUUID new_parent = gInventory.findCategoryUUIDForType(is_snapshot ? LLFolderType::FT_SNAPSHOT_CATEGORY : LLFolderType::assetTypeToFolderType(item->getType()));
                    // do not restamp children on restore
                    gInventory.changeItemParent(item, new_parent, false);
                }
            }
        }
    }
    else if ("copy_uuid" == action)
    {
        LLViewerInventoryItem* item = gInventory.getItem(mUUIDs.front());
        if(item)
        {
            LLUUID asset_id = item->getProtectedAssetUUID();
            std::string buffer;
            asset_id.toString(buffer);

            gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(buffer));
        }
    }
    else if ("purge" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            remove_inventory_object(selected_id, NULL);
        }
    }
    else if ("goto" == action)
    {
        show_item_original(mUUIDs.front());
    }
    else if ("thumbnail" == action)
    {
        LLSD data(mUUIDs.front());
        LLFloaterReg::showInstance("change_item_thumbnail", data);
    }
    else if ("cut" == action)
    {
        if (mGallery->canCut())
        {
            mGallery->cut();
        }
    }
    else if ("paste" == action)
    {
        if (mGallery->canPaste())
        {
            mGallery->paste();
        }
    }
    else if ("delete" == action)
    {
        mGallery->deleteSelection();
    }
    else if ("copy" == action)
    {
        if (mGallery->canCopy())
        {
            mGallery->copy();
        }
    }
    else if ("paste_link" == action)
    {
        mGallery->pasteAsLink();
    }
    else if ("rename" == action)
    {
        rename(mUUIDs.front());
    }
    else if ("open" == action || "open_original" == action)
    {
        LLViewerInventoryItem* item = gInventory.getItem(mUUIDs.front());
        if (item)
        {
            LLInvFVBridgeAction::doAction(item->getType(), mUUIDs.front(), &gInventory);
        }
    }
    else if ("ungroup_folder_items" == action)
    {
        ungroup_folder_items(mUUIDs.front());
    }
    else if ("take_off" == action || "detach" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLAppearanceMgr::instance().removeItemFromAvatar(selected_id);
        }
    }
    else if ("wear_add" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLAppearanceMgr::instance().wearItemOnAvatar(selected_id, true, false); // Don't replace if adding.
        }
    }
    else if ("wear" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLAppearanceMgr::instance().wearItemOnAvatar(selected_id, true, true);
        }
    }
    else if ("activate" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLGestureMgr::instance().activateGesture(selected_id);

            LLViewerInventoryItem* item = gInventory.getItem(selected_id);
            if (!item) return;

            gInventory.updateItem(item);
        }
        gInventory.notifyObservers();
    }
    else if ("deactivate" == action)
    {
        for (LLUUID& selected_id : mUUIDs)
        {
            LLGestureMgr::instance().deactivateGesture(selected_id);

            LLViewerInventoryItem* item = gInventory.getItem(selected_id);
            if (!item) return;

            gInventory.updateItem(item);
        }
        gInventory.notifyObservers();
    }
    else if ("replace_links" == action)
    {
        LLFloaterReg::showInstance("linkreplace", LLSD(mUUIDs.front()));
    }
    else if ("copy_slurl" == action)
    {
        boost::function<void(LLLandmark*)> copy_slurl_cb = [](LLLandmark* landmark)
        {
            LLVector3d global_pos;
            landmark->getGlobalPos(global_pos);
            boost::function<void(std::string& slurl)> copy_slurl_to_clipboard_cb = [](const std::string& slurl)
            {
               gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
               LLSD args;
               args["SLURL"] = slurl;
               LLNotificationsUtil::add("CopySLURL", args);
            };
            LLLandmarkActions::getSLURLfromPosGlobal(global_pos, copy_slurl_to_clipboard_cb, true);
        };
        LLLandmark* landmark = LLLandmarkActions::getLandmark(mUUIDs.front(), copy_slurl_cb);
        if (landmark)
        {
            copy_slurl_cb(landmark);
        }
    }
    else if ("about" == action)
    {
        LLSD key;
        key["type"] = "landmark";
        key["id"] = mUUIDs.front();
        LLFloaterSidePanelContainer::showPanel("places", key);
    }
    else if ("show_on_map" == action)
    {
        boost::function<void(LLLandmark*)> show_on_map_cb = [](LLLandmark* landmark)
        {
            LLVector3d landmark_global_pos;
            if (landmark->getGlobalPos(landmark_global_pos))
            {
                LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
                if (!landmark_global_pos.isExactlyZero() && worldmap_instance)
                {
                    worldmap_instance->trackLocation(landmark_global_pos);
                    LLFloaterReg::showInstance("world_map", "center");
                }
            }
        };
        LLLandmark* landmark = LLLandmarkActions::getLandmark(mUUIDs.front(), show_on_map_cb);
        if(landmark)
        {
            show_on_map_cb(landmark);
        }
    }
    else if ("save_as" == action)
    {
        LLPreviewTexture* preview_texture = LLFloaterReg::getTypedInstance<LLPreviewTexture>("preview_texture", mUUIDs.front());
        if (preview_texture)
        {
            preview_texture->openToSave();
            preview_texture->saveAs();
        }
    }
}

void LLInventoryGalleryContextMenu::rename(const LLUUID& item_id)
{
    LLInventoryObject* obj = gInventory.getObject(item_id);
    if (!obj) return;

    LLSD args;
    args["NAME"] = obj->getName();

    LLSD payload;
    payload["id"] = mUUIDs.front();

    LLNotificationsUtil::add("RenameItem", args, payload, boost::bind(onRename, _1, _2));
}

void LLInventoryGalleryContextMenu::onRename(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return; // canceled

    std::string new_name = response["new_name"].asString();
    LLStringUtil::trim(new_name);
    if (!new_name.empty())
    {
        LLUUID id = notification["payload"]["id"].asUUID();
        
        LLViewerInventoryCategory* cat = gInventory.getCategory(id);
        if(cat && (cat->getName() != new_name))
        {
            LLSD updates;
            updates["name"] = new_name;
            update_inventory_category(cat->getUUID(),updates, NULL);
            return;
        }
        
        LLViewerInventoryItem* item = gInventory.getItem(id);
        if(item && (item->getName() != new_name))
        {
            LLSD updates;
            updates["name"] = new_name;
            update_inventory_item(item->getUUID(),updates, NULL);
        }
    }
}

void LLInventoryGalleryContextMenu::fileUploadLocation(const LLSD& userdata)
{
    const std::string param = userdata.asString();
    if (param == "model")
    {
        gSavedPerAccountSettings.setString("ModelUploadFolder", mUUIDs.front().asString());
    }
    else if (param == "texture")
    {
        gSavedPerAccountSettings.setString("TextureUploadFolder", mUUIDs.front().asString());
    }
    else if (param == "sound")
    {
        gSavedPerAccountSettings.setString("SoundUploadFolder", mUUIDs.front().asString());
    }
    else if (param == "animation")
    {
        gSavedPerAccountSettings.setString("AnimationUploadFolder", mUUIDs.front().asString());
    }
}

bool LLInventoryGalleryContextMenu::canSetUploadLocation(const LLSD& userdata)
{
    if (mUUIDs.size() != 1)
    {
        return false;
    }
    LLInventoryCategory* cat = gInventory.getCategory(mUUIDs.front());
    if (!cat)
    {
        return false;
    }
    return true;
}

bool is_inbox_folder(LLUUID item_id)
{
    const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX);
    
    if (inbox_id.isNull())
    {
        return false;
    }
    
    return gInventory.isObjectDescendentOf(item_id, inbox_id);
}

void LLInventoryGalleryContextMenu::updateMenuItemsVisibility(LLContextMenu* menu)
{
    LLUUID selected_id = mUUIDs.front();
    LLInventoryObject* obj = gInventory.getObject(selected_id);
    if (!obj)
    {
        return;
    }

    std::vector<std::string> items;
    std::vector<std::string> disabled_items;

    bool is_agent_inventory = gInventory.isObjectDescendentOf(selected_id, gInventory.getRootFolderID());
    bool is_link = obj->getIsLinkType();
    bool is_folder = (obj->getType() == LLAssetType::AT_CATEGORY);
    bool is_cof = LLAppearanceMgr::instance().getIsInCOF(selected_id);
    bool is_inbox = is_inbox_folder(selected_id);
    bool is_trash = (selected_id == gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH));
    bool is_in_trash = gInventory.isObjectDescendentOf(selected_id, gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH));
    bool is_lost_and_found = (selected_id == gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
    bool is_outfits= (selected_id == gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS));
    //bool is_favorites= (selected_id == gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE));

    bool is_system_folder = false;
    LLFolderType::EType folder_type(LLFolderType::FT_NONE);
    bool has_children = false;
    bool is_full_perm_item = false;
    bool is_copyable = false;
    LLViewerInventoryItem* selected_item = gInventory.getItem(selected_id);

    if(is_folder)
    {
        LLInventoryCategory* category = gInventory.getCategory(selected_id);
        if (category)
        {
            folder_type = category->getPreferredType();
            is_system_folder = LLFolderType::lookupIsProtectedType(folder_type);
            has_children = (gInventory.categoryHasChildren(selected_id) != LLInventoryModel::CHILDREN_NO);
        }
    }
    else
    {
        if (selected_item)
        {
            is_full_perm_item = selected_item->getIsFullPerm();
            is_copyable = selected_item->getPermissions().allowCopyBy(gAgent.getID());
        }
    }

    if(!is_link)
    {
        items.push_back(std::string("thumbnail"));
        if (!is_agent_inventory || (is_in_trash && !is_trash))
        {
            disabled_items.push_back(std::string("thumbnail"));
        }
    }

    if (is_folder)
    {
        if(!isRootFolder())
        {
            items.push_back(std::string("Copy Separator"));

            items.push_back(std::string("open_in_current_window"));
            items.push_back(std::string("open_in_new_window"));
            items.push_back(std::string("Open Folder Separator"));
        }
    }
    else
    {
        if (is_agent_inventory && (obj->getType() != LLAssetType::AT_LINK_FOLDER))
        {
            items.push_back(std::string("Replace Links"));
        }
        if (obj->getType() == LLAssetType::AT_LANDMARK)
        {
            items.push_back(std::string("Landmark Separator"));
            items.push_back(std::string("url_copy"));
            items.push_back(std::string("About Landmark"));
            items.push_back(std::string("show_on_map"));
        }
    }

    if(is_trash)
    {
        items.push_back(std::string("Empty Trash"));

        LLInventoryModel::cat_array_t* cat_array;
        LLInventoryModel::item_array_t* item_array;
        gInventory.getDirectDescendentsOf(selected_id, cat_array, item_array);
        if (0 == cat_array->size() && 0 == item_array->size())
        {
            disabled_items.push_back(std::string("Empty Trash"));
        }
    }
    else if(is_in_trash)
    {
        if (is_link)
        {
            items.push_back(std::string("Find Original"));
            if (LLAssetType::lookupIsLinkType(obj->getType()))
            {
                disabled_items.push_back(std::string("Find Original"));
            }
        }
        items.push_back(std::string("Purge Item"));
        if (is_folder && !get_is_category_and_children_removable(&gInventory, selected_id, true))
        {
            disabled_items.push_back(std::string("Purge Item"));
        }
        items.push_back(std::string("Restore Item"));
    }
    else
    {
        if(can_share_item(selected_id))
        {
            items.push_back(std::string("Share"));
        }
        if (LLClipboard::instance().hasContents() && is_agent_inventory && !is_cof && !is_inbox_folder(selected_id))
        {
            items.push_back(std::string("Paste"));

            static LLCachedControl<bool> inventory_linking(gSavedSettings, "InventoryLinking", true);
            if (inventory_linking)
            {
                items.push_back(std::string("Paste As Link"));
            }
        }
        if (is_folder && is_agent_inventory)
        {
            if (!is_cof && (folder_type != LLFolderType::FT_OUTFIT) && !is_outfits && !is_inbox_folder(selected_id))
            {
                if (!gInventory.isObjectDescendentOf(selected_id, gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD)) && !isRootFolder())
                {
                    items.push_back(std::string("New Folder"));
                }
                items.push_back(std::string("upload_def"));
            }

            if(is_outfits && !isRootFolder())
            {
                items.push_back(std::string("New Outfit"));
            }

            items.push_back(std::string("Subfolder Separator"));
            if (!is_system_folder && !isRootFolder())
            {
                if(has_children && (folder_type != LLFolderType::FT_OUTFIT))
                {
                    items.push_back(std::string("Ungroup folder items"));
                }
                items.push_back(std::string("Cut"));
                items.push_back(std::string("Delete"));

                if(!get_is_category_and_children_removable(&gInventory, selected_id, false))
                {
                    disabled_items.push_back(std::string("Delete"));
                    disabled_items.push_back(std::string("Cut"));
                }
                else if (!get_is_category_and_children_removable(&gInventory, selected_id, true))
                {
                    disabled_items.push_back(std::string("Cut"));
                }

                if(!is_inbox)
                {
                    items.push_back(std::string("Rename"));
                }
            }
            if(!is_system_folder)
            {
                items.push_back(std::string("Copy"));
            }
        }
        else if(!is_folder)
        {
            items.push_back(std::string("Properties"));
            items.push_back(std::string("Copy Asset UUID"));
            items.push_back(std::string("Copy Separator"));

            bool is_asset_knowable = is_asset_knowable = LLAssetType::lookupIsAssetIDKnowable(obj->getType());
            if ( !is_asset_knowable // disable menu item for Inventory items with unknown asset. EXT-5308
                 || (! ( is_full_perm_item || gAgent.isGodlike())))
            {
                disabled_items.push_back(std::string("Copy Asset UUID"));
            }
            if(is_agent_inventory)
            {
                items.push_back(std::string("Cut"));
                if (!is_link || !is_cof || !get_is_item_worn(selected_id))
                {
                    items.push_back(std::string("Delete"));
                }
                if (!get_is_item_removable(&gInventory, selected_id, false))
                {
                    disabled_items.push_back(std::string("Delete"));
                    disabled_items.push_back(std::string("Cut"));
                }
                else if(!get_is_item_removable(&gInventory, selected_id, true))
                {
                    disabled_items.push_back(std::string("Cut"));
                }

                if (selected_item && (selected_item->getInventoryType() != LLInventoryType::IT_CALLINGCARD) && !is_inbox && selected_item->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID()))
                {
                    items.push_back(std::string("Rename"));
                }
            }
            items.push_back(std::string("Copy"));
            if (!is_copyable)
            {
                disabled_items.push_back(std::string("Copy"));
            }
        }
        if((obj->getType() == LLAssetType::AT_SETTINGS)
           || ((obj->getType() <= LLAssetType::AT_GESTURE)
               && obj->getType() != LLAssetType::AT_OBJECT
               && obj->getType() != LLAssetType::AT_CLOTHING
               && obj->getType() != LLAssetType::AT_CATEGORY
               && obj->getType() != LLAssetType::AT_LANDMARK
               && obj->getType() != LLAssetType::AT_BODYPART))
        {
            bool can_open = !LLAssetType::lookupIsLinkType(obj->getType());

            if (can_open)
            {
                if (is_link)
                    items.push_back(std::string("Open Original"));
                else
                    items.push_back(std::string("Open"));
            }
            else
            {
                disabled_items.push_back(std::string("Open"));
                disabled_items.push_back(std::string("Open Original"));
            }
            
            if(LLAssetType::AT_GESTURE == obj->getType())
            {
                items.push_back(std::string("Gesture Separator"));
                if(!LLGestureMgr::instance().isGestureActive(selected_id))
                {
                    items.push_back(std::string("Activate"));
                }
                else
                {
                    items.push_back(std::string("Deactivate"));
                }
            }
        }
        else if(LLAssetType::AT_LANDMARK == obj->getType())
        {
            items.push_back(std::string("Landmark Open"));
        }
        else if (obj->getType() == LLAssetType::AT_OBJECT || obj->getType() == LLAssetType::AT_CLOTHING || obj->getType() == LLAssetType::AT_BODYPART)
        {
            items.push_back(std::string("Wearable And Object Separator"));
            if(obj->getType() == LLAssetType::AT_CLOTHING)
            {
                items.push_back(std::string("Take Off"));
            }
            if(get_is_item_worn(selected_id))
            {
                if(obj->getType() == LLAssetType::AT_OBJECT)
                {
                    items.push_back(std::string("Detach From Yourself"));
                }
                disabled_items.push_back(std::string("Wearable And Object Wear"));
                disabled_items.push_back(std::string("Wearable Add"));
            }
            else
            {
                if(obj->getType() == LLAssetType::AT_OBJECT)
                {
                    items.push_back(std::string("Wearable Add"));
                }
                items.push_back(std::string("Wearable And Object Wear"));
                disabled_items.push_back(std::string("Take Off"));
            }

            if (!gAgentAvatarp->canAttachMoreObjects() && (obj->getType() == LLAssetType::AT_OBJECT))
            {
                disabled_items.push_back(std::string("Wearable And Object Wear"));
                disabled_items.push_back(std::string("Wearable Add"));
            }
            if (selected_item && (obj->getType() != LLAssetType::AT_OBJECT) && LLWearableType::getInstance()->getAllowMultiwear(selected_item->getWearableType()))
            {
                items.push_back(std::string("Wearable Add"));
                if (!gAgentWearables.canAddWearable(selected_item->getWearableType()))
                {
                    disabled_items.push_back(std::string("Wearable Add"));
                }
            }
        }
        if(obj->getType() == LLAssetType::AT_TEXTURE)
        {
            items.push_back(std::string("Save As"));
            bool can_copy = selected_item && selected_item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
            if (!can_copy)
            {
                disabled_items.push_back(std::string("Save As"));
            }
        }
        if (is_link)
        {
            items.push_back(std::string("Find Original"));
            if (LLAssetType::lookupIsLinkType(obj->getType()))
            {
                disabled_items.push_back(std::string("Find Original"));
            }
        }
        if (is_lost_and_found)
        {
            items.push_back(std::string("Empty Lost And Found"));

            LLInventoryModel::cat_array_t* cat_array;
            LLInventoryModel::item_array_t* item_array;
            gInventory.getDirectDescendentsOf(selected_id, cat_array, item_array);
            // Enable Empty menu item only when there is something to act upon.
            if (0 == cat_array->size() && 0 == item_array->size())
            {
                disabled_items.push_back(std::string("Empty Lost And Found"));
            }

            disabled_items.push_back(std::string("New Folder"));
            disabled_items.push_back(std::string("upload_def"));
        }
    }

    hide_context_entries(*menu, items, disabled_items);
}

