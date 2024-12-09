/**
 * @file llfloatercreatelandmark.cpp
 * @brief LLFloaterCreateLandmark class implementation
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#include "llfloatercreatelandmark.h"

#include "llagent.h"
#include "llagentui.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "lllandmarkactions.h"
#include "llnotificationsutil.h"
#include "llpanellandmarkinfo.h"
#include "llparcel.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

typedef std::pair<LLUUID, std::string> folder_pair_t;

class LLLandmarksInventoryObserver : public LLInventoryObserver
{
public:
    LLLandmarksInventoryObserver(LLFloaterCreateLandmark* create_landmark_floater) :
        mFloater(create_landmark_floater)
    {}

    void changed(U32 mask) override
    {
        if (mFloater->getItem())
        {
            checkChanged(mask);
        }
        else
        {
            checkCreated(mask);
        }
    }

protected:
    void checkCreated(U32 mask)
    {
        if (gInventory.getAddedIDs().empty())
        {
            return;
        }

        if (!(mask & LLInventoryObserver::ADD) ||
            !(mask & LLInventoryObserver::CREATE) ||
            !(mask & LLInventoryObserver::UPDATE_CREATE))
        {
            return;
        }

        mFloater->setItem(gInventory.getAddedIDs());
    }

    void checkChanged(U32 mask)
    {
        if (gInventory.getChangedIDs().empty())
        {
            return;
        }

        if ((mask & LLInventoryObserver::LABEL) ||
            (mask & LLInventoryObserver::INTERNAL) ||
            (mask & LLInventoryObserver::REMOVE) ||
            (mask & LLInventoryObserver::STRUCTURE) ||
            (mask & LLInventoryObserver::REBUILD))
        {
            mFloater->updateItem(gInventory.getChangedIDs(), mask);
        }
    }

private:
    LLFloaterCreateLandmark* mFloater;
};

LLFloaterCreateLandmark::LLFloaterCreateLandmark(const LLSD& key)
    : LLFloater("add_landmark")
{
    mInventoryObserver = new LLLandmarksInventoryObserver(this);
}

LLFloaterCreateLandmark::~LLFloaterCreateLandmark()
{
    removeObserver();
}

bool LLFloaterCreateLandmark::postBuild()
{
    mFolderCombo = getChild<LLComboBox>("folder_combo");
    mLandmarkTitleEditor = getChild<LLLineEditor>("title_editor");
    mNotesEditor = getChild<LLTextEditor>("notes_editor");

    getChild<LLTextBox>("new_folder_textbox")->setURLClickedCallback(boost::bind(&LLFloaterCreateLandmark::onCreateFolderClicked, this));
    getChild<LLButton>("ok_btn")->setClickedCallback(boost::bind(&LLFloaterCreateLandmark::onSaveClicked, this));
    getChild<LLButton>("cancel_btn")->setClickedCallback(boost::bind(&LLFloaterCreateLandmark::onCancelClicked, this));

    mLandmarkTitleEditor->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTextChanges(); });
    mNotesEditor->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTextChanges(); });

    mLandmarksID = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);

    return true;
}

void LLFloaterCreateLandmark::removeObserver()
{
    gInventory.removeObserver(mInventoryObserver);
}

void LLFloaterCreateLandmark::onOpen(const LLSD& key)
{
    LLUUID dest_folder;
    if (key.has("dest_folder"))
    {
        dest_folder = key["dest_folder"].asUUID();
    }
    mItem = NULL;
    gInventory.addObserver(mInventoryObserver);
    if ((mHasCustomPosition = key.has("region")))
    {
        mLandmarkTitleEditor->setText(key["title"].asString());
    }
    else
    {
        // mLandmarkTitleEditor->setText() is called here
        setLandmarkInfo(dest_folder);
    }
    populateFoldersList(dest_folder);

    mLandmarkTitleEditor->setCursorToEnd();
}

void LLFloaterCreateLandmark::onClose(bool app_quitting)
{
    removeObserver();
}

void LLFloaterCreateLandmark::setLandmarkInfo(const LLUUID &folder_id)
{
    LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
    LLParcel* parcel = parcel_mgr->getAgentParcel();
    std::string name = parcel->getName();
    LLVector3 agent_pos = gAgent.getPositionAgent();

    if (name.empty())
    {
        S32 region_x = ll_round(agent_pos.mV[VX]);
        S32 region_y = ll_round(agent_pos.mV[VY]);
        S32 region_z = ll_round(agent_pos.mV[VZ]);

        std::string region_name;
        LLViewerRegion* region = parcel_mgr->getSelectionRegion();
        if (region)
        {
            region_name = region->getName();
        }
        else
        {
            std::string desc;
            LLAgentUI::buildLocationString(desc, LLAgentUI::LOCATION_FORMAT_NORMAL, agent_pos);
            region_name = desc;
        }

        name = llformat("%s (%d, %d, %d)", region_name.c_str(), region_x, region_y, region_z);
    }

    mLandmarkTitleEditor->setText(name);

    LLLandmarkActions::createLandmarkHere(name, "", folder_id.notNull() ? folder_id : gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE));
}

bool cmp_folders(const folder_pair_t& left, const folder_pair_t& right)
{
    return left.second < right.second;
}

void LLFloaterCreateLandmark::populateFoldersList(const LLUUID &folder_id)
{
    // Collect all folders that can contain landmarks.
    LLInventoryModel::cat_array_t cats;
    LLPanelLandmarkInfo::collectLandmarkFolders(cats);

    mFolderCombo->removeall();

    // Put the "My Favorites" folder first in list.
    LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
    LLViewerInventoryCategory* favorites_cat = gInventory.getCategory(favorites_id);
    if (!favorites_cat)
    {
        LL_WARNS() << "Cannot find the favorites folder" << LL_ENDL;
    }
    else
    {
        mFolderCombo->add(getString("favorites_bar"), favorites_cat->getUUID());
    }

    // Add the "Landmarks" category.
    const LLViewerInventoryCategory* lmcat = gInventory.getCategory(mLandmarksID);
    if (!lmcat)
    {
        LL_WARNS() << "Cannot find the landmarks folder" << LL_ENDL;
    }
    else
    {
        std::string cat_full_name = LLPanelLandmarkInfo::getFullFolderName(lmcat);
        mFolderCombo->add(cat_full_name, lmcat->getUUID());
    }

    typedef std::vector<folder_pair_t> folder_vec_t;
    folder_vec_t folders;
    // Sort the folders by their full name.
    for (S32 i = 0; i < cats.size(); i++)
    {
        const LLViewerInventoryCategory* cat = cats.at(i);
        std::string cat_full_name = LLPanelLandmarkInfo::getFullFolderName(cat);
        folders.push_back(folder_pair_t(cat->getUUID(), cat_full_name));
    }
    sort(folders.begin(), folders.end(), cmp_folders);

    // Finally, populate the combobox.
    for (folder_vec_t::const_iterator it = folders.begin(); it != folders.end(); it++)
        mFolderCombo->add(it->second, LLSD(it->first));

    if (folder_id.notNull())
    {
        mFolderCombo->setCurrentByID(folder_id);
    }
}

void LLFloaterCreateLandmark::onCommitTextChanges()
{
    if (mItem.isNull())
    {
        return;
    }

    std::string current_title_value = mLandmarkTitleEditor->getText();
    LLStringUtil::trim(current_title_value);
    if (current_title_value.empty())
    {
        return;
    }

    std::string current_notes_value = mNotesEditor->getText();
    LLStringUtil::trim(current_notes_value);

    std::string item_title_value = mItem->getName();
    std::string item_notes_value = mItem->getDescription();

    if (item_title_value != current_title_value || item_notes_value != current_notes_value)
    {
        LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(mItem);
        new_item->rename(current_title_value);
        new_item->setDescription(current_notes_value);
        LLPointer<LLInventoryCallback> cb;
        LLInventoryModel::LLCategoryUpdate up(mItem->getParentUUID(), 0);
        gInventory.accountForUpdate(up);
        update_inventory_item(new_item, cb);
    }
}

void LLFloaterCreateLandmark::onCreateFolderClicked()
{
    LLNotificationsUtil::add("CreateLandmarkFolder", LLSD(), LLSD(),
        [this](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            std::string folder_name = resp["message"].asString();
            if (!folder_name.empty())
            {
                gInventory.createNewCategory(mLandmarksID, LLFolderType::FT_NONE, folder_name, folderCreatedCallback);
                gInventory.notifyObservers();
            }
        }
    });
}

void LLFloaterCreateLandmark::folderCreatedCallback(LLUUID folder_id)
{
    LLFloaterCreateLandmark* floater = LLFloaterReg::findTypedInstance<LLFloaterCreateLandmark>("add_landmark");
    if (floater && !floater->isDead())
    {
        floater->populateFoldersList(folder_id);
    }
}

void LLFloaterCreateLandmark::onSaveClicked()
{
    if (mItem.isNull() && !mHasCustomPosition)
    {
        closeFloater();
        return;
    }

    std::string current_title_value = mLandmarkTitleEditor->getText();
    LLStringUtil::trim(current_title_value);
    if (current_title_value.empty())
    {
        return;
    }

    std::string current_notes_value = mNotesEditor->getText();
    LLStringUtil::trim(current_notes_value);

    LLUUID folder_id = mFolderCombo->getValue().asUUID();

    if (mItem.isNull() && mHasCustomPosition)
    {
        std::string region = mKey["region"];
        S32 x = mKey["x"].asInteger();
        S32 y = mKey["y"].asInteger();
        S32 z = mKey["z"].asInteger();
        create_inventory_landmark(folder_id, current_title_value, current_notes_value, region, x, y, z);
        return;
    }

    std::string item_title_value = mItem->getName();
    std::string item_notes_value = mItem->getDescription();
    bool change_parent = folder_id != mItem->getParentUUID();

    LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(mItem);

    if (item_title_value != current_title_value ||
        item_notes_value != current_notes_value)
    {
        new_item->rename(current_title_value);
        new_item->setDescription(current_notes_value);
        LLPointer<LLInventoryCallback> cb;
        if (change_parent)
        {
            cb = new LLUpdateLandmarkParent(new_item, folder_id);
        }
        LLInventoryModel::LLCategoryUpdate up(mItem->getParentUUID(), 0);
        gInventory.accountForUpdate(up);
        update_inventory_item(new_item, cb);
    }
    else if (change_parent)
    {
        LLInventoryModel::update_list_t update;
        LLInventoryModel::LLCategoryUpdate old_folder(mItem->getParentUUID(),-1);
        update.push_back(old_folder);
        LLInventoryModel::LLCategoryUpdate new_folder(folder_id, 1);
        update.push_back(new_folder);
        gInventory.accountForUpdate(update);

        new_item->setParent(folder_id);
        new_item->updateParentOnServer(false);
    }

    removeObserver();

    gInventory.updateItem(new_item);
    gInventory.notifyObservers();

    closeFloater();
}

void LLFloaterCreateLandmark::onCancelClicked()
{
    removeObserver();
    if (!mItem.isNull())
    {
        LLUUID item_id = mItem->getUUID();
        remove_inventory_item(item_id, NULL);
    }
    closeFloater();
}


void LLFloaterCreateLandmark::setItem(const uuid_set_t& items)
{
    for (uuid_set_t::const_iterator item_iter = items.begin();
        item_iter != items.end();
        ++item_iter)
    {
        const LLUUID& item_id = (*item_iter);
        if (!highlight_offered_object(item_id))
        {
            continue;
        }

        LLInventoryItem* item = gInventory.getItem(item_id);

        llassert(item);
        if (item && (LLAssetType::AT_LANDMARK == item->getType()) )
        {
            if (!getItem())
            {
                mItem = item;
                mAssetID = mItem->getAssetUUID();
                mParentID = mItem->getParentUUID();
                setVisibleAndFrontmost(true);
                break;
            }
        }
    }
}

void LLFloaterCreateLandmark::updateItem(const uuid_set_t& items, U32 mask)
{
    if (!getItem())
    {
        return;
    }

    if (mHasCustomPosition)
    {
        closeFloater();
        return;
    }

    LLUUID landmark_id = getItem()->getUUID();

    for (uuid_set_t::const_iterator item_iter = items.begin();
        item_iter != items.end();
        ++item_iter)
    {
        const LLUUID& item_id = (*item_iter);
        if (landmark_id == item_id)
        {
            if (getItem() != gInventory.getItem(item_id))
            {
                // item is obsolete or removed
                closeFloater();
            }

            if (mParentID != mItem->getParentUUID())
            {
                // user moved landmark in inventory,
                // assume that we are done all other changes should already be commited
                closeFloater();
            }

            if ((mask & LLInventoryObserver::INTERNAL) && mAssetID != mItem->getAssetUUID())
            {
                closeFloater();
            }

            if (mask & LLInventoryObserver::LABEL)
            {
                mLandmarkTitleEditor->setText(mItem->getName());
            }

            if (mask & LLInventoryObserver::INTERNAL)
            {
                mNotesEditor->setText(mItem->getDescription());
            }
        }
    }
}
