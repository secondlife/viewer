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

#include "llfiltereditor.h"
#include "llfolderviewmodel.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"

#include "lldraghandle.h"
#include "llviewercontrol.h"
#include "llagent.h"

//=========================================================================
namespace
{
    const std::string FLOATER_DEFINITION_XML("floater_settings_picker.xml");

    const std::string FLT_INVENTORY_SEARCH("flt_inventory_search");
    const std::string PNL_INVENTORY("pnl_inventory");
    const std::string CHK_SHOWFOLDERS("chk_showfolders");
    const std::string BTN_SELECT("btn_select");
    const std::string BTN_CANCEL("btn_cancel");

    const F32 CONTEXT_CONE_IN_ALPHA(0.0f);
    const F32 CONTEXT_CONE_OUT_ALPHA(1.0f);
    const F32 CONTEXT_FADE_TIME(0.08f);
}
//=========================================================================

LLFloaterSettingsPicker::LLFloaterSettingsPicker(LLView * owner, LLUUID initial_asset_id, const std::string &label, const LLSD &params):
    LLFloater(params),
    mOwnerHandle(),
    mLabel(label),
    mActive(true),
    mContextConeOpacity(0.0f),
    mSettingAssetID(initial_asset_id),
    mImmediateFilterPermMask(PERM_NONE)
{
    mOwnerHandle = owner->getHandle();

    buildFromFile(FLOATER_DEFINITION_XML);
    setCanMinimize(FALSE);
}


LLFloaterSettingsPicker::~LLFloaterSettingsPicker() 
{

}

//-------------------------------------------------------------------------
BOOL LLFloaterSettingsPicker::postBuild()
{
    if (!LLFloater::postBuild())
        return FALSE;

    if (!mLabel.empty())
    {
        std::string pick = getString("pick title");

        setTitle(pick + mLabel);
    }

//    childSetCommitCallback(CHK_SHOWFOLDERS, onShowFolders, this);
    getChildView(CHK_SHOWFOLDERS)->setVisible(FALSE);

    mFilterEdit = getChild<LLFilterEditor>(FLT_INVENTORY_SEARCH);
    mFilterEdit->setCommitCallback([this](LLUICtrl*, const LLSD& param){ onFilterEdit(param.asString()); });
    
    mInventoryPanel = getChild<LLInventoryPanel>(PNL_INVENTORY);
    if (mInventoryPanel)
    {
        U32 filter_types = 0x0;
        filter_types |= 0x1 << LLInventoryType::IT_SETTINGS;

        mInventoryPanel->setFilterTypes(filter_types);
        mInventoryPanel->setFilterPermMask(mImmediateFilterPermMask);

        mInventoryPanel->setSelectCallback([this](const LLFloaterSettingsPicker::itemlist_t &items, bool useraction){ onSelectionChange(items, useraction); });
        mInventoryPanel->setShowFolderState(LLInventoryFilter::SHOW_NON_EMPTY_FOLDERS);

        // Disable auto selecting first filtered item because it takes away
        // selection from the item set by LLTextureCtrl owning this floater.
        mInventoryPanel->getRootFolder()->setAutoSelectOverride(TRUE);

        // don't put keyboard focus on selected item, because the selection callback
        // will assume that this was user input
        if (!mSettingAssetID.isNull())
        {
            mInventoryPanel->setSelection(findItemID(mSettingAssetID, false), TAKE_FOCUS_NO);
        }
    }

    mNoCopySettingsSelected = FALSE;

    childSetAction(BTN_CANCEL, [this](LLUICtrl*, const LLSD& param){ onButtonCancel(); });
    childSetAction(BTN_SELECT, [this](LLUICtrl*, const LLSD& param){ onButtonSelect(); });

    // update permission filter once UI is fully initialized
    mSavedFolderState.setApply(FALSE);

    return TRUE;
}

void LLFloaterSettingsPicker::onClose(bool app_quitting)
{
    if (app_quitting)
        return;

    mCloseSignal();
    LLView *owner = mOwnerHandle.get();
    if (owner)
    {
        owner->setFocus(TRUE);
    }
}

void LLFloaterSettingsPicker::setValue(const LLSD& value)
{
    mSettingAssetID = value.asUUID();
}

LLSD LLFloaterSettingsPicker::getValue() const 
{
    return LLSD(mSettingAssetID);
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

void LLFloaterSettingsPicker::draw()
{
    LLView *owner = mOwnerHandle.get();
    if (owner)
    {
        // draw cone of context pointing back to texture swatch	
        LLRect owner_rect;
        owner->localRectToOtherView(owner->getLocalRect(), &owner_rect, this);
        LLRect local_rect = getLocalRect();
        if (gFocusMgr.childHasKeyboardFocus(this) && owner->isInVisibleChain() && mContextConeOpacity > 0.001f)
        {
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            LLGLEnable(GL_CULL_FACE);
            gGL.begin(LLRender::QUADS);
            {
                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
                gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);
                gGL.vertex2i(owner_rect.mRight, owner_rect.mTop);
                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
                gGL.vertex2i(local_rect.mRight, local_rect.mTop);
                gGL.vertex2i(local_rect.mLeft, local_rect.mTop);

                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
                gGL.vertex2i(local_rect.mLeft, local_rect.mTop);
                gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
                gGL.vertex2i(owner_rect.mLeft, owner_rect.mBottom);
                gGL.vertex2i(owner_rect.mLeft, owner_rect.mTop);

                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
                gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
                gGL.vertex2i(local_rect.mRight, local_rect.mTop);
                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
                gGL.vertex2i(owner_rect.mRight, owner_rect.mTop);
                gGL.vertex2i(owner_rect.mRight, owner_rect.mBottom);


                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
                gGL.vertex2i(local_rect.mLeft, local_rect.mBottom);
                gGL.vertex2i(local_rect.mRight, local_rect.mBottom);
                gGL.color4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
                gGL.vertex2i(owner_rect.mRight, owner_rect.mBottom);
                gGL.vertex2i(owner_rect.mLeft, owner_rect.mBottom);
            }
            gGL.end();
        }
    }

    if (gFocusMgr.childHasMouseCapture(getDragHandle()))
    {
        mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
    }
    else
    {
        mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLSmoothInterpolation::getInterpolant(CONTEXT_FADE_TIME));
    }

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

        mSavedFolderState.setApply(TRUE);
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
            mSavedFolderState.setApply(FALSE);
            mInventoryPanel->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        }
    }

    mInventoryPanel->setFilterSubString(search_string);
}

void LLFloaterSettingsPicker::onSelectionChange(const LLFloaterSettingsPicker::itemlist_t &items, bool user_action)
{
    if (items.size())
    {
        LLFolderViewItem* first_item = items.front();
        LLInventoryItem* itemp = gInventory.getItem(static_cast<LLFolderViewModelItemInventory*>(first_item->getViewModelItem())->getUUID());
        mNoCopySettingsSelected = false;
        if (itemp)
        {
//             if (!mChangeIDSignal.empty())
//             {
//                 mChangeIDSignal(itemp);
//             }
            if (!itemp->getPermissions().allowCopyBy(gAgent.getID()))
            {
                mNoCopySettingsSelected = true;
            }
            setSettingsAssetId(itemp->getAssetUUID(), false);
            mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?

            if (user_action)
            {
                mChangeIDSignal(mSettingAssetID);
            }
        }
    }
}

void LLFloaterSettingsPicker::onButtonCancel()
{
    closeFloater();
}

void LLFloaterSettingsPicker::onButtonSelect()
{
    if (mCommitSignal)
        (*mCommitSignal)(this, LLSD(mSettingAssetID));
    closeFloater();
}

BOOL LLFloaterSettingsPicker::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if (mSettingAssetID.notNull()
        && mInventoryPanel)
    {
        LLUUID item_id = findItemID(mSettingAssetID, FALSE);
        S32 inventory_x = x - mInventoryPanel->getRect().mLeft;
        S32 inventory_y = y - mInventoryPanel->getRect().mBottom;
        if (item_id.notNull()
            && mInventoryPanel->parentPointInView(inventory_x, inventory_y))
        {
            // make sure item (not folder) is selected
            LLFolderViewItem* item_viewp = mInventoryPanel->getItemByID(item_id);
            if (item_viewp && item_viewp->isSelected())
            {
                LLRect target_rect;
                item_viewp->localRectToOtherView(item_viewp->getLocalRect(), &target_rect, this);
                if (target_rect.pointInRect(x, y))
                {
                    // Quick-apply
                    if (mCommitSignal)
                        (*mCommitSignal)(this, LLSD(mSettingAssetID));
                    closeFloater();
                    return TRUE;
                }
            }
        }
    }
    return LLFloater::handleDoubleClick(x, y, mask);
}

//=========================================================================
void LLFloaterSettingsPicker::setActive(bool active)
{
    mActive = active;
}

void LLFloaterSettingsPicker::setSettingsAssetId(const LLUUID &settings_id, bool set_selection)
{
    if (mSettingAssetID != settings_id && mActive)
    {
        mNoCopySettingsSelected = false;
        mViewModel->setDirty(); // *TODO: shouldn't we be using setValue() here?
        mSettingAssetID = settings_id;
        LLUUID item_id = findItemID(mSettingAssetID, FALSE);
        if (item_id.isNull())
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
