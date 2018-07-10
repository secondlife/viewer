/** 
 * @file llfloatergesture.cpp
 * @brief Read-only list of gestures from your inventory.
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

#include "llfloatermyenvironment.h"

#include "llinventory.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llclipboard.h"

#include "llagent.h"
#include "llclipboard.h"
#include "llkeyboard.h"
#include "llmenugl.h"
#include "llmultigesture.h"
#include "llscrolllistctrl.h"
#include "llcheckboxctrl.h"
#include "lltrans.h"
#include "llviewergesture.h"
#include "llviewermenu.h" 
#include "llviewerinventory.h"
#include "llviewercontrol.h"
#include "llfloaterperms.h"
#include "llenvironment.h"

//=========================================================================
namespace
{
    const std::string CHECK_DAYS("chk_days");
    const std::string CHECK_SKIES("chk_skies");
    const std::string CHECK_WATER("chk_water");
    const std::string PANEL_SETTINGS("pnl_settings");
    const std::string CHECK_SHOWFOLDERS("chk_showfolders");
    const std::string BUTTON_NEWSETTINGS("btn_gear");
    const std::string BUTTON_GEAR("btn_newsettings");
    const std::string BUTTON_DELETE("btn_del");

}

//=========================================================================
#if 0
BOOL item_name_precedes( LLInventoryItem* a, LLInventoryItem* b )
{
	return LLStringUtil::precedesDict( a->getName(), b->getName() );
}

class LLFloaterGestureObserver : public LLGestureManagerObserver
{
public:
	LLFloaterGestureObserver(LLFloaterGesture* floater) : mFloater(floater) {}
	virtual ~LLFloaterGestureObserver() {}
	virtual void changed() { mFloater->refreshAll(); }

private:
	LLFloaterGesture* mFloater;
};
//-----------------------------
// GestureCallback
//-----------------------------

class GestureShowCallback : public LLInventoryCallback
{
public:
	void fire(const LLUUID &inv_item)
	{
		LLPreviewGesture::show(inv_item, LLUUID::null);
		
		LLInventoryItem* item = gInventory.getItem(inv_item);
		if (item)
		{
			LLPermissions perm = item->getPermissions();
			perm.setMaskNext(LLFloaterPerms::getNextOwnerPerms("Gestures"));
			perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms("Gestures"));
			perm.setMaskGroup(LLFloaterPerms::getGroupPerms("Gestures"));
			item->setPermissions(perm);
			item->updateServer(FALSE);
		}
	}
};

class GestureCopiedCallback : public LLInventoryCallback
{
private:
	LLFloaterGesture* mFloater;
	
public:
	GestureCopiedCallback(LLFloaterGesture* floater): mFloater(floater)
	{}
	void fire(const LLUUID &inv_item)
	{
		if(mFloater)
		{
			mFloater->addGesture(inv_item,NULL,mFloater->getChild<LLScrollListCtrl>("gesture_list"));

			// EXP-1909 (Pasted gesture displayed twice)
			// The problem is that addGesture is called here for the second time for the same item (which is copied)
			// First time addGesture is called from LLFloaterGestureObserver::changed(), which is a callback for inventory
			// change. So we need to refresh the gesture list to avoid duplicates.
			mFloater->refreshAll();
		}
	}
};

#endif

//=========================================================================
LLFloaterMyEnvironment::LLFloaterMyEnvironment(const LLSD& key) :
    LLFloater(key),
    mInventoryList(nullptr),
    mShowFolders(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS),
    mTypeFilter((0x01 << static_cast<U64>(LLSettingsType::ST_DAYCYCLE)) | (0x01 << static_cast<U64>(LLSettingsType::ST_SKY)) | (0x01 << static_cast<U64>(LLSettingsType::ST_WATER))),
    mSelectedAsset()
{
    mCommitCallbackRegistrar.add("MyEnvironments.DoCreate", [this](LLUICtrl *, const LLSD &userdata) { onDoCreate(userdata); });

    mEnableCallbackRegistrar.add("MyEnvironments.EnvironmentEnabled", [](LLUICtrl *, const LLSD &) { return LLEnvironment::instance().isInventoryEnabled(); });

#if 0
	mObserver = new LLFloaterGestureObserver(this);
	LLGestureMgr::instance().addObserver(mObserver);

	mCommitCallbackRegistrar.add("Gesture.Action.ToogleActiveState", boost::bind(&LLFloaterGesture::onActivateBtnClick, this));
	mCommitCallbackRegistrar.add("Gesture.Action.ShowPreview", boost::bind(&LLFloaterGesture::onClickEdit, this));
	mCommitCallbackRegistrar.add("Gesture.Action.CopyPaste", boost::bind(&LLFloaterGesture::onCopyPasteAction, this, _2));
	mCommitCallbackRegistrar.add("Gesture.Action.SaveToCOF", boost::bind(&LLFloaterGesture::addToCurrentOutFit, this));

	mEnableCallbackRegistrar.add("Gesture.EnableAction", boost::bind(&LLFloaterGesture::isActionEnabled, this, _2));
#endif
}

LLFloaterMyEnvironment::~LLFloaterMyEnvironment()
{
    // 	LLGestureMgr::instance().removeObserver(mObserver);
    // 	delete mObserver;
    // 	mObserver = NULL;
    // 	gInventory.removeObserver(this);
}


BOOL LLFloaterMyEnvironment::postBuild()
{
    mInventoryList = getChild<LLInventoryPanel>(PANEL_SETTINGS);

    if (mInventoryList)
    {
        U32 filter_types = 0x0;
        filter_types |= 0x1 << LLInventoryType::IT_SETTINGS;

        mInventoryList->setFilterTypes(filter_types);

        mInventoryList->setSelectCallback([this](const std::deque<LLFolderViewItem*>&, BOOL) { onSelectionChange(); });
        mInventoryList->setShowFolderState(mShowFolders);
        mInventoryList->setFilterSettingsTypes(mTypeFilter);
    }

    childSetCommitCallback(CHECK_DAYS, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_SKIES, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_WATER, [this](LLUICtrl*, void*) { onFilterCheckChange(); }, nullptr);
    childSetCommitCallback(CHECK_SHOWFOLDERS, [this](LLUICtrl*, void*) { onShowFoldersChange(); }, nullptr);

    childSetCommitCallback(BUTTON_DELETE, [this](LLUICtrl *, void*) { onDeleteSelected(); }, nullptr);

    return TRUE;
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
    bool show_check (getChild<LLCheckBoxCtrl>(CHECK_SHOWFOLDERS)->getValue().asBoolean());

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

void LLFloaterMyEnvironment::onDeleteSelected()
{
    uuid_vec_t selected;

    getSelectedIds(selected);
    if (selected.empty())
        return;

    const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
    for (const LLUUID& itemid: selected)
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
            // no need to restamp it though it's a move into trash because
            // it's a brand new item already.
            new_item->updateParentOnServer(FALSE);
            gInventory.updateItem(new_item);
        }
    }
    gInventory.notifyObservers();
}


void LLFloaterMyEnvironment::onDoCreate(const LLSD &data)
{
    menu_create_inventory_item(mInventoryList, NULL, data);
}

//-------------------------------------------------------------------------
void LLFloaterMyEnvironment::refreshButtonStates()
{
    bool settings_ok = LLEnvironment::instance().isInventoryEnabled();

    uuid_vec_t selected;
    getSelectedIds(selected);

    getChild<LLUICtrl>(BUTTON_GEAR)->setEnabled(settings_ok);
    getChild<LLUICtrl>(BUTTON_NEWSETTINGS)->setEnabled(true);
    getChild<LLUICtrl>(BUTTON_DELETE)->setEnabled(settings_ok && !selected.empty());
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

void LLFloaterMyEnvironment::getSelectedIds(uuid_vec_t& ids) const
{
    LLInventoryPanel::selected_items_t items = mInventoryList->getSelectedItems();

    for (auto itemview : items)
    {
        LLFolderViewModelItemInventory* itemp = static_cast<LLFolderViewModelItemInventory*>(itemview->getViewModelItem());
        ids.push_back(itemp->getUUID());
    }
}
