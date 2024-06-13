/**
 * @file llsidepaneltaskinfo.cpp
 * @brief LLSidepanelTaskInfo class implementation
 * This class represents the panel in the build view for
 * viewing/editing object names, owners, permissions, etc.
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

#include "llsidepaneltaskinfo.h"

#include "lluuid.h"
#include "llpermissions.h"
#include "llcategory.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llstring.h"

#include "llviewerwindow.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcallbacklist.h"
#include "llcheckboxctrl.h"
#include "llviewerobject.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llstatusbar.h"        // for getBalance()
#include "lllineeditor.h"
#include "llcombobox.h"
#include "lluiconstants.h"
#include "lldbstrings.h"
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llavataractions.h"
#include "llnamebox.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "roles_constants.h"
#include "llgroupactions.h"
#include "lltextbase.h"
#include "llstring.h"
#include "lltrans.h"

///----------------------------------------------------------------------------
/// Class llsidepaneltaskinfo
///----------------------------------------------------------------------------

LLSidepanelTaskInfo* LLSidepanelTaskInfo::sActivePanel = NULL;

static LLPanelInjector<LLSidepanelTaskInfo> t_task_info("sidepanel_task_info");

static std::string click_action_to_string_value(U8 click_action)
{
    switch (click_action)
    {
        case CLICK_ACTION_TOUCH:
            return "Touch";
        case CLICK_ACTION_SIT:
            return "Sit";
        case CLICK_ACTION_BUY:
            return "Buy";
        case CLICK_ACTION_PAY:
            return "Pay";
        case CLICK_ACTION_OPEN:
            return "Open";
        case CLICK_ACTION_ZOOM:
            return "Zoom";
        case CLICK_ACTION_DISABLED:
            return "None";
        case CLICK_ACTION_IGNORE:
            return "Ignore";
        default:
            return "Touch";
    }
}

// Default constructor
LLSidepanelTaskInfo::LLSidepanelTaskInfo()
    : mVisibleDebugPermissions(true) // space was allocated by default
{
    setMouseOpaque(false);
    mSelectionUpdateSlot = LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLSidepanelTaskInfo::refreshAll, this));
    gIdleCallbacks.addFunction(&LLSidepanelTaskInfo::onIdle, (void*)this);
}


LLSidepanelTaskInfo::~LLSidepanelTaskInfo()
{
    if (sActivePanel == this)
        sActivePanel = NULL;
    gIdleCallbacks.deleteFunction(&LLSidepanelTaskInfo::onIdle, (void*)this);

    if (mSelectionUpdateSlot.connected())
    {
        mSelectionUpdateSlot.disconnect();
    }
}

// virtual
bool LLSidepanelTaskInfo::postBuild()
{
    mOpenBtn = getChild<LLButton>("open_btn");
    mOpenBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onOpenButtonClicked, this));
    mPayBtn = getChild<LLButton>("pay_btn");
    mPayBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onPayButtonClicked, this));
    mBuyBtn = getChild<LLButton>("buy_btn");
    mBuyBtn->setClickedCallback(boost::bind(&handle_buy));
    mDetailsBtn = getChild<LLButton>("details_btn");
    mDetailsBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onDetailsButtonClicked, this));

    mDeedBtn = getChild<LLButton>("button deed");

    mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");

    childSetCommitCallback("Object Name",                       LLSidepanelTaskInfo::onCommitName,this);
    getChild<LLLineEditor>("Object Name")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
    childSetCommitCallback("Object Description",                LLSidepanelTaskInfo::onCommitDesc,this);
    getChild<LLLineEditor>("Object Description")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
    getChild<LLUICtrl>("button set group")->setCommitCallback(boost::bind(&LLSidepanelTaskInfo::onClickGroup,this));
    childSetCommitCallback("checkbox share with group",         &LLSidepanelTaskInfo::onCommitGroupShare,this);
    childSetAction("button deed",                               &LLSidepanelTaskInfo::onClickDeedToGroup,this);
    childSetCommitCallback("checkbox allow everyone move",      &LLSidepanelTaskInfo::onCommitEveryoneMove,this);
    childSetCommitCallback("checkbox allow everyone copy",      &LLSidepanelTaskInfo::onCommitEveryoneCopy,this);
    childSetCommitCallback("checkbox for sale",                 &LLSidepanelTaskInfo::onCommitSaleInfo,this);
    childSetCommitCallback("sale type",                         &LLSidepanelTaskInfo::onCommitSaleType,this);
    childSetCommitCallback("Edit Cost",                         &LLSidepanelTaskInfo::onCommitSaleInfo, this);
    childSetCommitCallback("checkbox next owner can modify",    &LLSidepanelTaskInfo::onCommitNextOwnerModify,this);
    childSetCommitCallback("checkbox next owner can copy",      &LLSidepanelTaskInfo::onCommitNextOwnerCopy,this);
    childSetCommitCallback("checkbox next owner can transfer",  &LLSidepanelTaskInfo::onCommitNextOwnerTransfer,this);
    childSetCommitCallback("clickaction",                       &LLSidepanelTaskInfo::onCommitClickAction,this);
    childSetCommitCallback("search_check",                      &LLSidepanelTaskInfo::onCommitIncludeInSearch,this);

    mDAPermModify = getChild<LLUICtrl>("perm_modify");
    mDACreatorName = getChild<LLUICtrl>("Creator Name");
    mDAOwner = getChildView("Owner:");
    mDAOwnerName = getChild<LLUICtrl>("Owner Name");
    mDAButtonSetGroup = getChildView("button set group");
    mDAObjectName = getChild<LLUICtrl>("Object Name");
    mDAName = getChildView("Name:");
    mDADescription = getChildView("Description:");
    mDAObjectDescription = getChild<LLUICtrl>("Object Description");
    mDACheckboxShareWithGroup = getChild<LLUICtrl>("checkbox share with group");
    mDAButtonDeed = getChildView("button deed");
    mDACheckboxAllowEveryoneMove = getChild<LLUICtrl>("checkbox allow everyone move");
    mDACheckboxAllowEveryoneCopy = getChild<LLUICtrl>("checkbox allow everyone copy");
    mDACheckboxNextOwnerCanModify = getChild<LLUICtrl>("checkbox next owner can modify");
    mDACheckboxNextOwnerCanCopy = getChild<LLUICtrl>("checkbox next owner can copy");
    mDACheckboxNextOwnerCanTransfer = getChild<LLUICtrl>("checkbox next owner can transfer");
    mDACheckboxForSale = getChild<LLUICtrl>("checkbox for sale");
    mDASearchCheck = getChild<LLUICtrl>("search_check");
    mDAComboSaleType = getChild<LLComboBox>("sale type");
    mDAEditCost = getChild<LLUICtrl>("Edit Cost");
    mDALabelClickAction = getChildView("label click action");
    mDAComboClickAction = getChild<LLComboBox>("clickaction");
    mDAPathfindingAttributes = getChild<LLTextBase>("pathfinding_attributes_value");
    mDAB = getChild<LLUICtrl>("B:");
    mDAO = getChild<LLUICtrl>("O:");
    mDAG = getChild<LLUICtrl>("G:");
    mDAE = getChild<LLUICtrl>("E:");
    mDAN = getChild<LLUICtrl>("N:");
    mDAF = getChild<LLUICtrl>("F:");

    return true;
}

/*virtual*/ void LLSidepanelTaskInfo::onVisibilityChange(bool visible)
{
    if (visible)
    {
        sActivePanel = this;
        mObject = getFirstSelectedObject();
    }
    else
    {
        sActivePanel = NULL;
        // drop selection reference
        mObjectSelection = NULL;
    }
}


void LLSidepanelTaskInfo::disableAll()
{
    mDACreatorName->setValue(LLStringUtil::null);
    mDACreatorName->setEnabled(false);

    mDAOwner->setEnabled(false);
    mDAOwnerName->setValue(LLStringUtil::null);
    mDAOwnerName->setEnabled(false);

    mDAObjectName->setValue(LLStringUtil::null);
    mDAObjectName->setEnabled(false);
    mDAName->setEnabled(false);
    mDADescription->setEnabled(false);
    mDAObjectDescription->setValue(LLStringUtil::null);
    mDAObjectDescription->setEnabled(false);

    mDAPathfindingAttributes->setEnabled(false);
    mDAPathfindingAttributes->setValue(LLStringUtil::null);

    mDAButtonSetGroup->setEnabled(false);
    mDAButtonDeed->setEnabled(false);

    mDAPermModify->setEnabled(false);
    mDAPermModify->setValue(LLStringUtil::null);
    mDAEditCost->setValue(LLStringUtil::null);
    mDAComboSaleType->setValue(LLSaleInfo::FS_COPY);

    disablePermissions();

    if (mVisibleDebugPermissions)
    {
        mDAB->setVisible(false);
        mDAO->setVisible(false);
        mDAG->setVisible(false);
        mDAE->setVisible(false);
        mDAN->setVisible(false);
        mDAF->setVisible(false);

        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        LLRect parent_rect = parent_floater->getRect();
        LLRect debug_rect = mDAB->getRect();
        // use double the debug rect for padding (since it isn't trivial to extract top_pad)
        parent_floater->reshape(parent_rect.getWidth(), parent_rect.getHeight() - (debug_rect.getHeight() * 2));
        mVisibleDebugPermissions = false;
    }

    mOpenBtn->setEnabled(false);
    mPayBtn->setEnabled(false);
    mBuyBtn->setEnabled(false);
}

void LLSidepanelTaskInfo::disablePermissions()
{
    mDACheckboxShareWithGroup->setValue(false);
    mDACheckboxShareWithGroup->setEnabled(false);

    mDACheckboxAllowEveryoneMove->setValue(false);
    mDACheckboxAllowEveryoneMove->setEnabled(false);
    mDACheckboxAllowEveryoneCopy->setValue(false);
    mDACheckboxAllowEveryoneCopy->setEnabled(false);

    //Next owner can:
    mDACheckboxNextOwnerCanModify->setValue(false);
    mDACheckboxNextOwnerCanModify->setEnabled(false);
    mDACheckboxNextOwnerCanCopy->setValue(false);
    mDACheckboxNextOwnerCanCopy->setEnabled(false);
    mDACheckboxNextOwnerCanTransfer->setValue(false);
    mDACheckboxNextOwnerCanTransfer->setEnabled(false);

    //checkbox for sale
    mDACheckboxForSale->setValue(false);
    mDACheckboxForSale->setEnabled(false);

    //checkbox include in search
    mDASearchCheck->setValue(false);
    mDASearchCheck->setEnabled(false);

    mDAComboSaleType->setEnabled(false);

    mDAEditCost->setEnabled(false);

    mDALabelClickAction->setEnabled(false);
    if (mDAComboClickAction)
    {
        mDAComboClickAction->setEnabled(false);
        mDAComboClickAction->clear();
    }
}

void LLSidepanelTaskInfo::refresh()
{
    mIsDirty = false;

    LLButton* btn_deed_to_group = mDeedBtn;
    if (btn_deed_to_group)
    {
        std::string deedText;
        if (gWarningSettings.getBOOL("DeedObject"))
        {
            deedText = getString("text deed continued");
        }
        else
        {
            deedText = getString("text deed");
        }
        btn_deed_to_group->setLabelSelected(deedText);
        btn_deed_to_group->setLabelUnselected(deedText);
    }

    bool root_selected = true;
    LLSelectNode* nodep = mObjectSelection->getFirstRootNode();
    S32 object_count = mObjectSelection->getRootObjectCount();
    if (!nodep || (object_count == 0))
    {
        nodep = mObjectSelection->getFirstNode();
        object_count = mObjectSelection->getObjectCount();
        root_selected = false;
    }

    LLViewerObject* objectp = NULL;
    if (nodep)
    {
        objectp = nodep->getObject();
    }

    // ...nothing selected
    if (!nodep || !objectp)
    {
        disableAll();
        return;
    }

    // figure out a few variables
    const bool is_one_object = (object_count == 1);

    // BUG: fails if a root and non-root are both single-selected.
    const bool is_perm_modify = (mObjectSelection->getFirstRootNode() && LLSelectMgr::getInstance()->selectGetRootsModify()) ||
        LLSelectMgr::getInstance()->selectGetModify();
    const bool is_nonpermanent_enforced = (mObjectSelection->getFirstRootNode() && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced()) ||
        LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();

    S32 string_index = 0;
    std::string MODIFY_INFO_STRINGS[] =
        {
            getString("text modify info 1"),
            getString("text modify info 2"),
            getString("text modify info 3"),
            getString("text modify info 4"),
            getString("text modify info 5"),
            getString("text modify info 6")
        };
    if (!is_perm_modify)
    {
        string_index += 2;
    }
    else if (!is_nonpermanent_enforced)
    {
        string_index += 4;
    }
    if (!is_one_object)
    {
        ++string_index;
    }
    getChildView("perm_modify")->setEnabled(true);
    getChild<LLUICtrl>("perm_modify")->setValue(MODIFY_INFO_STRINGS[string_index]);

    std::string pfAttrName;

    if ((mObjectSelection->getFirstRootNode()
        && LLSelectMgr::getInstance()->selectGetRootsNonPathfinding())
        || LLSelectMgr::getInstance()->selectGetNonPathfinding())
    {
        pfAttrName = "Pathfinding_Object_Attr_None";
    }
    else if ((mObjectSelection->getFirstRootNode()
        && LLSelectMgr::getInstance()->selectGetRootsPermanent())
        || LLSelectMgr::getInstance()->selectGetPermanent())
    {
        pfAttrName = "Pathfinding_Object_Attr_Permanent";
    }
    else if ((mObjectSelection->getFirstRootNode()
        && LLSelectMgr::getInstance()->selectGetRootsCharacter())
        || LLSelectMgr::getInstance()->selectGetCharacter())
    {
        pfAttrName = "Pathfinding_Object_Attr_Character";
    }
    else
    {
        pfAttrName = "Pathfinding_Object_Attr_MultiSelect";
    }

    mDAPathfindingAttributes->setEnabled(true);
    mDAPathfindingAttributes->setValue(LLTrans::getString(pfAttrName));

    // Update creator text field
    getChildView("Creator:")->setEnabled(true);

    std::string creator_name;
    LLUUID creator_id;
    LLSelectMgr::getInstance()->selectGetCreator(creator_id, creator_name);

    if(creator_id != mCreatorID )
    {
        mDACreatorName->setValue(creator_name);
        mCreatorID = creator_id;
    }
    if(mDACreatorName->getValue().asString() == LLStringUtil::null)
    {
        mDACreatorName->setValue(creator_name);
    }
    mDACreatorName->setEnabled(true);

    // Update owner text field
    getChildView("Owner:")->setEnabled(true);

    std::string owner_name;
    LLUUID owner_id;
    const bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
    if (owner_id.isNull())
    {
        if (LLSelectMgr::getInstance()->selectIsGroupOwned())
        {
            // Group owned already displayed by selectGetOwner
        }
        else
        {
            // Display last owner if public
            std::string last_owner_name;
            LLSelectMgr::getInstance()->selectGetLastOwner(mLastOwnerID, last_owner_name);

            // It should never happen that the last owner is null and the owner
            // is null, but it seems to be a bug in the simulator right now. JC
            if (!mLastOwnerID.isNull() && !last_owner_name.empty())
            {
                owner_name.append(", last ");
                owner_name.append(last_owner_name);
            }
        }
    }

    if(owner_id.isNull() || (owner_id != mOwnerID))
    {
        mDAOwnerName->setValue(owner_name);
        mOwnerID = owner_id;
    }
    if(mDAOwnerName->getValue().asString() == LLStringUtil::null)
    {
        mDAOwnerName->setValue(owner_name);
    }

    getChildView("Owner Name")->setEnabled(true);

    // update group text field
    getChildView("Group:")->setEnabled(true);
    getChild<LLUICtrl>("Group Name")->setValue(LLStringUtil::null);
    LLUUID group_id;
    bool groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
    if (groups_identical)
    {
        if (mLabelGroupName)
        {
            mLabelGroupName->setNameID(group_id,true);
            mLabelGroupName->setEnabled(true);
        }
    }
    else
    {
        if (mLabelGroupName)
        {
            mLabelGroupName->setNameID(LLUUID::null, true);
            mLabelGroupName->refresh(LLUUID::null, std::string(), true);
            mLabelGroupName->setEnabled(false);
        }
    }

    getChildView("button set group")->setEnabled(owners_identical && (mOwnerID == gAgent.getID()) && is_nonpermanent_enforced);

    getChildView("Name:")->setEnabled(true);
    LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
    getChildView("Description:")->setEnabled(true);
    LLLineEditor* LineEditorObjectDesc = getChild<LLLineEditor>("Object Description");

    if (is_one_object)
    {
        if (!LineEditorObjectName->hasFocus())
        {
            getChild<LLUICtrl>("Object Name")->setValue(nodep->mName);
        }

        if (LineEditorObjectDesc)
        {
            if (!LineEditorObjectDesc->hasFocus())
            {
                LineEditorObjectDesc->setText(nodep->mDescription);
            }
        }
    }
    else
    {
        getChild<LLUICtrl>("Object Name")->setValue(LLStringUtil::null);
        LineEditorObjectDesc->setText(LLStringUtil::null);
    }

    // figure out the contents of the name, description, & category
    bool edit_name_desc = false;
    if (is_one_object && objectp->permModify() && !objectp->isPermanentEnforced())
    {
        edit_name_desc = true;
    }
    if (edit_name_desc)
    {
        getChildView("Object Name")->setEnabled(true);
        getChildView("Object Description")->setEnabled(true);
    }
    else
    {
        getChildView("Object Name")->setEnabled(false);
        getChildView("Object Description")->setEnabled(false);
    }

    S32 total_sale_price = 0;
    S32 individual_sale_price = 0;
    bool is_for_sale_mixed = false;
    bool is_sale_price_mixed = false;
    U32 num_for_sale = false;
    LLSelectMgr::getInstance()->selectGetAggregateSaleInfo(num_for_sale,
                                                           is_for_sale_mixed,
                                                           is_sale_price_mixed,
                                                           total_sale_price,
                                                           individual_sale_price);

    const bool self_owned = (gAgent.getID() == mOwnerID);
    const bool group_owned = LLSelectMgr::getInstance()->selectIsGroupOwned() ;
    const bool public_owned = (mOwnerID.isNull() && !LLSelectMgr::getInstance()->selectIsGroupOwned());
    const bool can_transfer = LLSelectMgr::getInstance()->selectGetRootsTransfer();
    const bool can_copy = LLSelectMgr::getInstance()->selectGetRootsCopy();

    if (!owners_identical)
    {
        getChildView("Cost")->setEnabled(false);
        getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
        getChildView("Edit Cost")->setEnabled(false);
    }
    // You own these objects.
    else if (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
    {
        LLSpinCtrl *edit_price = getChild<LLSpinCtrl>("Edit Cost");

        // If there are multiple items for sale then set text to PRICE PER UNIT.
        if (num_for_sale > 1)
        {
            std::string label_text = is_sale_price_mixed? "Cost Mixed" :"Cost Per Unit";
            edit_price->setLabel(getString(label_text));
        }
        else
        {
            edit_price->setLabel(getString("Cost Default"));
        }

        if (!edit_price->hasFocus())
        {
            // If the sale price is mixed then set the cost to MIXED, otherwise
            // set to the actual cost.
            if ((num_for_sale > 0) && is_for_sale_mixed)
            {
                edit_price->setTentative(true);
            }
            else if ((num_for_sale > 0) && is_sale_price_mixed)
            {
                edit_price->setTentative(true);
            }
            else
            {
                edit_price->setValue(individual_sale_price);
            }
        }
        // The edit fields are only enabled if you can sell this object
        // and the sale price is not mixed.
        bool enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : false;
        getChildView("Cost")->setEnabled(enable_edit);
        getChildView("Edit Cost")->setEnabled(enable_edit);
    }
    // Someone, not you, owns these objects.
    else if (!public_owned)
    {
        getChildView("Cost")->setEnabled(false);
        getChildView("Edit Cost")->setEnabled(false);

        // Don't show a price if none of the items are for sale.
        if (num_for_sale)
            getChild<LLUICtrl>("Edit Cost")->setValue(llformat("%d",total_sale_price));
        else
            getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);

        // If multiple items are for sale, set text to TOTAL PRICE.
        if (num_for_sale > 1)
            getChild<LLSpinCtrl>("Edit Cost")->setLabel(getString("Cost Total"));
        else
            getChild<LLSpinCtrl>("Edit Cost")->setLabel(getString("Cost Default"));
    }
    // This is a public object.
    else
    {
        getChildView("Cost")->setEnabled(false);
        getChild<LLSpinCtrl>("Edit Cost")->setLabel(getString("Cost Default"));
        getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
        getChildView("Edit Cost")->setEnabled(false);
    }

    // Enable and disable the permissions checkboxes
    // based on who owns the object.
    // TODO: Creator permissions

    U32 base_mask_on            = 0;
    U32 base_mask_off           = 0;
    U32 owner_mask_off          = 0;
    U32 owner_mask_on           = 0;
    U32 group_mask_on           = 0;
    U32 group_mask_off          = 0;
    U32 everyone_mask_on        = 0;
    U32 everyone_mask_off       = 0;
    U32 next_owner_mask_on      = 0;
    U32 next_owner_mask_off     = 0;

    bool valid_base_perms       = LLSelectMgr::getInstance()->selectGetPerm(PERM_BASE,
                                                                            &base_mask_on,
                                                                            &base_mask_off);
    //bool valid_owner_perms =//
    LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
                                              &owner_mask_on,
                                              &owner_mask_off);
    bool valid_group_perms      = LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
                                                                            &group_mask_on,
                                                                            &group_mask_off);

    bool valid_everyone_perms   = LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
                                                                            &everyone_mask_on,
                                                                            &everyone_mask_off);

    bool valid_next_perms       = LLSelectMgr::getInstance()->selectGetPerm(PERM_NEXT_OWNER,
                                                                            &next_owner_mask_on,
                                                                            &next_owner_mask_off);


    if (gSavedSettings.getBOOL("DebugPermissions") )
    {
        if (valid_base_perms)
        {
            mDAB->setValue("B: " + mask_to_string(base_mask_on));
            mDAB->setVisible(                           true);

            mDAO->setValue("O: " + mask_to_string(owner_mask_on));
            mDAO->setVisible(                           true);

            mDAG->setValue("G: " + mask_to_string(group_mask_on));
            mDAG->setVisible(                           true);

            mDAE->setValue("E: " + mask_to_string(everyone_mask_on));
            mDAE->setVisible(                           true);

            mDAN->setValue("N: " + mask_to_string(next_owner_mask_on));
            mDAN->setVisible(                           true);
        }

        U32 flag_mask = 0x0;
        if (objectp->permMove())        flag_mask |= PERM_MOVE;
        if (objectp->permModify())      flag_mask |= PERM_MODIFY;
        if (objectp->permCopy())        flag_mask |= PERM_COPY;
        if (objectp->permTransfer())    flag_mask |= PERM_TRANSFER;

        mDAF->setValue("F:" + mask_to_string(flag_mask));
        mDAF->setVisible(true);

        if (!mVisibleDebugPermissions)
        {
            LLFloater* parent_floater = gFloaterView->getParentFloater(this);
            LLRect parent_rect = parent_floater->getRect();
            LLRect debug_rect = mDAB->getRect();
            // use double the debug rect for padding (since it isn't trivial to extract top_pad)
            parent_floater->reshape(parent_rect.getWidth(), parent_rect.getHeight() + (debug_rect.getHeight() * 2));
            mVisibleDebugPermissions = true;
        }
    }
    else if (mVisibleDebugPermissions)
    {
        mDAB->setVisible(false);
        mDAO->setVisible(false);
        mDAG->setVisible(false);
        mDAE->setVisible(false);
        mDAN->setVisible(false);
        mDAF->setVisible(false);

        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        LLRect parent_rect = parent_floater->getRect();
        LLRect debug_rect = mDAB->getRect();
        // use double the debug rect for padding (since it isn't trivial to extract top_pad)
        parent_floater->reshape(parent_rect.getWidth(), parent_rect.getHeight() - (debug_rect.getHeight() * 2));
        mVisibleDebugPermissions = false;
    }

    bool has_change_perm_ability = false;
    bool has_change_sale_ability = false;

    if (valid_base_perms && is_nonpermanent_enforced &&
        (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
    {
        has_change_perm_ability = true;
    }
    if (valid_base_perms && is_nonpermanent_enforced &&
       (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_SET_SALE))))
    {
        has_change_sale_ability = true;
    }

    if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
    {
        // ...must select root to choose permissions
        getChild<LLUICtrl>("perm_modify")->setValue(getString("text modify warning"));
    }

    if (has_change_perm_ability)
    {
        getChildView("checkbox share with group")->setEnabled(true);
        getChildView("checkbox allow everyone move")->setEnabled(owner_mask_on & PERM_MOVE);
        getChildView("checkbox allow everyone copy")->setEnabled(owner_mask_on & PERM_COPY && owner_mask_on & PERM_TRANSFER);
    }
    else
    {
        getChildView("checkbox share with group")->setEnabled(false);
        getChildView("checkbox allow everyone move")->setEnabled(false);
        getChildView("checkbox allow everyone copy")->setEnabled(false);
    }

    if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
    {
        getChildView("checkbox for sale")->setEnabled(can_transfer || (!can_transfer && num_for_sale));
        // Set the checkbox to tentative if the prices of each object selected
        // are not the same.
        getChild<LLUICtrl>("checkbox for sale")->setTentative(              is_for_sale_mixed);
        getChildView("sale type")->setEnabled(num_for_sale && can_transfer && !is_sale_price_mixed);

        getChildView("checkbox next owner can modify")->setEnabled(base_mask_on & PERM_MODIFY);
        getChildView("checkbox next owner can copy")->setEnabled(base_mask_on & PERM_COPY);
        getChildView("checkbox next owner can transfer")->setEnabled(next_owner_mask_on & PERM_COPY);
    }
    else
    {
        getChildView("checkbox for sale")->setEnabled(false);
        getChildView("sale type")->setEnabled(false);

        getChildView("checkbox next owner can modify")->setEnabled(false);
        getChildView("checkbox next owner can copy")->setEnabled(false);
        getChildView("checkbox next owner can transfer")->setEnabled(false);
    }

    if (valid_group_perms)
    {
        if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
        {
            getChild<LLUICtrl>("checkbox share with group")->setValue(true);
            getChild<LLUICtrl>("checkbox share with group")->setTentative(  false);
            getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
        }
        else if ((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
        {
            getChild<LLUICtrl>("checkbox share with group")->setValue(false);
            getChild<LLUICtrl>("checkbox share with group")->setTentative(  false);
            getChildView("button deed")->setEnabled(false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox share with group")->setValue(true);
            getChild<LLUICtrl>("checkbox share with group")->setTentative(  true);
            getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
        }
    }

    if (valid_everyone_perms)
    {
        // Move
        if (everyone_mask_on & PERM_MOVE)
        {
            getChild<LLUICtrl>("checkbox allow everyone move")->setValue(true);
            getChild<LLUICtrl>("checkbox allow everyone move")->setTentative(   false);
        }
        else if (everyone_mask_off & PERM_MOVE)
        {
            getChild<LLUICtrl>("checkbox allow everyone move")->setValue(false);
            getChild<LLUICtrl>("checkbox allow everyone move")->setTentative(   false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox allow everyone move")->setValue(true);
            getChild<LLUICtrl>("checkbox allow everyone move")->setTentative(   true);
        }

        // Copy == everyone can't copy
        if (everyone_mask_on & PERM_COPY)
        {
            getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(true);
            getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(   !can_copy || !can_transfer);
        }
        else if (everyone_mask_off & PERM_COPY)
        {
            getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(false);
            getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(   false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(true);
            getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(   true);
        }
    }

    if (valid_next_perms)
    {
        // Modify == next owner canot modify
        if (next_owner_mask_on & PERM_MODIFY)
        {
            getChild<LLUICtrl>("checkbox next owner can modify")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can modify")->setTentative( false);
        }
        else if (next_owner_mask_off & PERM_MODIFY)
        {
            getChild<LLUICtrl>("checkbox next owner can modify")->setValue(false);
            getChild<LLUICtrl>("checkbox next owner can modify")->setTentative( false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox next owner can modify")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can modify")->setTentative( true);
        }

        // Copy == next owner cannot copy
        if (next_owner_mask_on & PERM_COPY)
        {
            getChild<LLUICtrl>("checkbox next owner can copy")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(   !can_copy);
        }
        else if (next_owner_mask_off & PERM_COPY)
        {
            getChild<LLUICtrl>("checkbox next owner can copy")->setValue(false);
            getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(   false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox next owner can copy")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(   true);
        }

        // Transfer == next owner cannot transfer
        if (next_owner_mask_on & PERM_TRANSFER)
        {
            getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( !can_transfer);
        }
        else if (next_owner_mask_off & PERM_TRANSFER)
        {
            getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(false);
            getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( false);
        }
        else
        {
            getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(true);
            getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( true);
        }
    }

    // reflect sale information
    LLSaleInfo sale_info;
    bool valid_sale_info = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
    LLSaleInfo::EForSale sale_type = sale_info.getSaleType();

    LLComboBox* combo_sale_type = getChild<LLComboBox>("sale type");
    if (valid_sale_info)
    {
        combo_sale_type->setValue(                  sale_type == LLSaleInfo::FS_NOT ? LLSaleInfo::FS_COPY : sale_type);
        combo_sale_type->setTentative(              false); // unfortunately this doesn't do anything at the moment.
    }
    else
    {
        // default option is sell copy, determined to be safest
        combo_sale_type->setValue(                  LLSaleInfo::FS_COPY);
        combo_sale_type->setTentative(              true); // unfortunately this doesn't do anything at the moment.
    }

    getChild<LLUICtrl>("checkbox for sale")->setValue((num_for_sale != 0));

    // HACK: There are some old objects in world that are set for sale,
    // but are no-transfer.  We need to let users turn for-sale off, but only
    // if for-sale is set.
    bool cannot_actually_sell = !can_transfer || (!can_copy && sale_type == LLSaleInfo::FS_COPY);
    if (cannot_actually_sell)
    {
        if (num_for_sale && has_change_sale_ability)
        {
            getChildView("checkbox for sale")->setEnabled(true);
        }
    }

    // Check search status of objects
    const bool all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );
    bool include_in_search;
    const bool all_include_in_search = LLSelectMgr::getInstance()->selectionGetIncludeInSearch(&include_in_search);
    getChildView("search_check")->setEnabled(has_change_sale_ability && all_volume);
    getChild<LLUICtrl>("search_check")->setValue(include_in_search);
    getChild<LLUICtrl>("search_check")->setTentative(!all_include_in_search);

    // Click action (touch, sit, buy)
    U8 click_action = 0;
    if (LLSelectMgr::getInstance()->selectionGetClickAction(&click_action))
    {
        getChild<LLComboBox>("clickaction")->setValue(click_action_to_string_value(click_action));
    }
    getChildView("label click action")->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);
    getChildView("clickaction")->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);

    updateVerbs();
}


// static
void LLSidepanelTaskInfo::onClickClaim(void*)
{
    // try to claim ownership
    LLSelectMgr::getInstance()->sendOwner(gAgent.getID(), gAgent.getGroupID());
}

// static
void LLSidepanelTaskInfo::onClickRelease(void*)
{
    // try to release ownership
    LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null);
}

void LLSidepanelTaskInfo::onClickGroup()
{
    LLUUID owner_id;
    std::string name;
    bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
    LLFloater* parent_floater = gFloaterView->getParentFloater(this);

    if (owners_identical && (owner_id == gAgent.getID()))
    {
        LLFloaterGroupPicker* fg = LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
        if (fg)
        {
            fg->setSelectGroupCallback( boost::bind(&LLSidepanelTaskInfo::cbGroupID, this, _1) );
            if (parent_floater)
            {
                LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
                fg->setOrigin(new_rect.mLeft, new_rect.mBottom);
                parent_floater->addDependentFloater(fg);
            }
        }
    }
}

void LLSidepanelTaskInfo::cbGroupID(LLUUID group_id)
{
    if (mLabelGroupName)
    {
        mLabelGroupName->setNameID(group_id, true);
    }
    LLSelectMgr::getInstance()->sendGroup(group_id);
}

static bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
    const S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        LLUUID group_id;
        const bool groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
        if (group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
        {
            LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, false);
        }
    }
    return false;
}

void LLSidepanelTaskInfo::onClickDeedToGroup(void *data)
{
    LLNotificationsUtil::add("DeedObjectToGroup", LLSD(), LLSD(), callback_deed_to_group);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

// static
void LLSidepanelTaskInfo::onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm)
{
    LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
    if(!object) return;

    // Checkbox will have toggled itself
    // LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
    bool new_state = check->get();

    LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);

    LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    if (self)
    {
        self->disablePermissions();
    }
}

// static
void LLSidepanelTaskInfo::onCommitGroupShare(LLUICtrl *ctrl, void *data)
{
    onCommitPerm(ctrl, data, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}

// static
void LLSidepanelTaskInfo::onCommitEveryoneMove(LLUICtrl *ctrl, void *data)
{
    onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_MOVE);
}


// static
void LLSidepanelTaskInfo::onCommitEveryoneCopy(LLUICtrl *ctrl, void *data)
{
    onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_COPY);
}

// static
void LLSidepanelTaskInfo::onCommitNextOwnerModify(LLUICtrl* ctrl, void* data)
{
    //LL_INFOS() << "LLSidepanelTaskInfo::onCommitNextOwnerModify" << LL_ENDL;
    onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}

// static
void LLSidepanelTaskInfo::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
    //LL_INFOS() << "LLSidepanelTaskInfo::onCommitNextOwnerCopy" << LL_ENDL;
    onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}

// static
void LLSidepanelTaskInfo::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
    //LL_INFOS() << "LLSidepanelTaskInfo::onCommitNextOwnerTransfer" << LL_ENDL;
    onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}

// static
void LLSidepanelTaskInfo::onCommitName(LLUICtrl*, void* data)
{
    //LL_INFOS() << "LLSidepanelTaskInfo::onCommitName()" << LL_ENDL;
    LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    LLLineEditor*   tb = self->getChild<LLLineEditor>("Object Name");
    if(tb)
    {
        LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
//      LLSelectMgr::getInstance()->selectionSetObjectName(self->mLabelObjectName->getText());
    }
}


// static
void LLSidepanelTaskInfo::onCommitDesc(LLUICtrl*, void* data)
{
    //LL_INFOS() << "LLSidepanelTaskInfo::onCommitDesc()" << LL_ENDL;
    LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    LLLineEditor*   le = self->getChild<LLLineEditor>("Object Description");
    if(le)
    {
        LLSelectMgr::getInstance()->selectionSetObjectDescription(le->getText());
    }
}

// static
void LLSidepanelTaskInfo::onCommitSaleInfo(LLUICtrl*, void* data)
{
    LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    self->setAllSaleInfo();
}

// static
void LLSidepanelTaskInfo::onCommitSaleType(LLUICtrl*, void* data)
{
    LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
    self->setAllSaleInfo();
}


void LLSidepanelTaskInfo::setAllSaleInfo()
{
    LLSaleInfo::EForSale sale_type = LLSaleInfo::FS_NOT;

    LLCheckBoxCtrl *checkPurchase = getChild<LLCheckBoxCtrl>("checkbox for sale");

    // Set the sale type if the object(s) are for sale.
    if(checkPurchase && checkPurchase->get())
    {
        sale_type = static_cast<LLSaleInfo::EForSale>(getChild<LLComboBox>("sale type")->getValue().asInteger());
    }

    S32 price = -1;

    LLSpinCtrl *edit_price = getChild<LLSpinCtrl>("Edit Cost");
    price = (edit_price->getTentative()) ? DEFAULT_PRICE : edit_price->getValue().asInteger();

    // If somehow an invalid price, turn the sale off.
    if (price < 0)
        sale_type = LLSaleInfo::FS_NOT;

    LLSaleInfo old_sale_info;
    LLSelectMgr::getInstance()->selectGetSaleInfo(old_sale_info);

    LLSaleInfo new_sale_info(sale_type, price);
    LLSelectMgr::getInstance()->selectionSetObjectSaleInfo(new_sale_info);

    U8 old_click_action = 0;
    LLSelectMgr::getInstance()->selectionGetClickAction(&old_click_action);

    if (old_sale_info.isForSale()
        && !new_sale_info.isForSale()
        && old_click_action == CLICK_ACTION_BUY)
    {
        // If turned off for-sale, make sure click-action buy is turned
        // off as well
        LLSelectMgr::getInstance()->
            selectionSetClickAction(CLICK_ACTION_TOUCH);
    }
    else if (new_sale_info.isForSale()
        && !old_sale_info.isForSale()
        && old_click_action == CLICK_ACTION_TOUCH)
    {
        // If just turning on for-sale, preemptively turn on one-click buy
        // unless user have a different click action set
        LLSelectMgr::getInstance()->
            selectionSetClickAction(CLICK_ACTION_BUY);
    }
}

struct LLSelectionPayable : public LLSelectedObjectFunctor
{
    virtual bool apply(LLViewerObject* obj)
    {
        // can pay if you or your parent has money() event in script
        LLViewerObject* parent = (LLViewerObject*)obj->getParent();
        return (obj->flagTakesMoney() ||
                (parent && parent->flagTakesMoney()));
    }
};

static U8 string_value_to_click_action(std::string p_value)
{
    if (p_value == "Touch")
        return CLICK_ACTION_TOUCH;
    if (p_value == "Sit")
        return CLICK_ACTION_SIT;
    if (p_value == "Buy")
        return CLICK_ACTION_BUY;
    if (p_value == "Pay")
        return CLICK_ACTION_PAY;
    if (p_value == "Open")
        return CLICK_ACTION_OPEN;
    if (p_value == "Zoom")
        return CLICK_ACTION_ZOOM;
    if (p_value == "None")
        return CLICK_ACTION_DISABLED;
    if (p_value == "Ignore")
        return CLICK_ACTION_IGNORE;
    return CLICK_ACTION_TOUCH;
}

// static
void LLSidepanelTaskInfo::onCommitClickAction(LLUICtrl* ctrl, void*)
{
    LLComboBox* box = (LLComboBox*)ctrl;
    if (!box)
        return;
    std::string value = box->getValue().asString();
    U8 click_action = string_value_to_click_action(value);
    doClickAction(click_action);
}

// static
void LLSidepanelTaskInfo::doClickAction(U8 click_action)
{
    if (click_action == CLICK_ACTION_BUY)
    {
        LLSaleInfo sale_info;
        LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
        if (!sale_info.isForSale())
        {
            LLNotificationsUtil::add("CantSetBuyObject");

            // Set click action back to its old value
            U8 click_action = 0;
            LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
            return;
        }
    }
    else if (click_action == CLICK_ACTION_PAY)
    {
        // Verify object has script with money() handler
        LLSelectionPayable payable;
        bool can_pay = LLSelectMgr::getInstance()->getSelection()->applyToObjects(&payable);
        if (!can_pay)
        {
            // Warn, but do it anyway.
            LLNotificationsUtil::add("ClickActionNotPayable");
        }
        else
        {
            handle_give_money_dialog();
        }
    }
    LLSelectMgr::getInstance()->selectionSetClickAction(click_action);
}

// static
void LLSidepanelTaskInfo::onCommitIncludeInSearch(LLUICtrl* ctrl, void* data)
{
    LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
    llassert(box);
    LLSelectMgr::getInstance()->selectionSetIncludeInSearch(box->get());
}

// virtual
void LLSidepanelTaskInfo::updateVerbs()
{
    LLSafeHandle<LLObjectSelection> object_selection = LLSelectMgr::getInstance()->getSelection();
    const bool any_selected = (object_selection->getNumNodes() > 0);

    mOpenBtn->setVisible(true);
    mPayBtn->setVisible(true);
    mBuyBtn->setVisible(true);
    mDetailsBtn->setVisible(true);

    mOpenBtn->setEnabled(enable_object_open());
    mPayBtn->setEnabled(enable_pay_object());
    mBuyBtn->setEnabled(enable_buy_object());
    mDetailsBtn->setEnabled(any_selected);
}

void LLSidepanelTaskInfo::onOpenButtonClicked()
{
    if (enable_object_open())
    {
        handle_object_open();
    }
}

void LLSidepanelTaskInfo::onPayButtonClicked()
{
    doClickAction(CLICK_ACTION_PAY);
}

void LLSidepanelTaskInfo::onBuyButtonClicked()
{
    doClickAction(CLICK_ACTION_BUY);
}

void LLSidepanelTaskInfo::onDetailsButtonClicked()
{
    LLFloaterReg::showInstance("inspect", LLSD());
}

// virtual
void LLSidepanelTaskInfo::save()
{
    onCommitGroupShare(getChild<LLCheckBoxCtrl>("checkbox share with group"), this);
    onCommitEveryoneMove(getChild<LLCheckBoxCtrl>("checkbox allow everyone move"), this);
    onCommitEveryoneCopy(getChild<LLCheckBoxCtrl>("checkbox allow everyone copy"), this);
    onCommitNextOwnerModify(getChild<LLCheckBoxCtrl>("checkbox next owner can modify"), this);
    onCommitNextOwnerCopy(getChild<LLCheckBoxCtrl>("checkbox next owner can copy"), this);
    onCommitNextOwnerTransfer(getChild<LLCheckBoxCtrl>("checkbox next owner can transfer"), this);
    onCommitName(getChild<LLLineEditor>("Object Name"), this);
    onCommitDesc(getChild<LLLineEditor>("Object Description"), this);
    onCommitSaleInfo(NULL, this);
    onCommitSaleType(NULL, this);
    onCommitIncludeInSearch(getChild<LLCheckBoxCtrl>("search_check"), this);
}

// removes keyboard focus so that all fields can be updated
// and then restored focus
void LLSidepanelTaskInfo::refreshAll()
{
    // update UI as soon as we have an object
    // but remove keyboard focus first so fields are free to update
    LLFocusableElement* focus = NULL;
    if (hasFocus())
    {
        focus = gFocusMgr.getKeyboardFocus();
        setFocus(false);
    }
    refresh();
    if (focus)
    {
        focus->setFocus(true);
    }
}


void LLSidepanelTaskInfo::setObjectSelection(LLObjectSelectionHandle selection)
{
    mObjectSelection = selection;
    refreshAll();
}

LLSidepanelTaskInfo* LLSidepanelTaskInfo::getActivePanel()
{
    return sActivePanel;
}

void LLSidepanelTaskInfo::dirty()
{
    mIsDirty = true;
}

// static
void LLSidepanelTaskInfo::onIdle( void* user_data )
{
    LLSidepanelTaskInfo* self = reinterpret_cast<LLSidepanelTaskInfo*>(user_data);

    if( self->mIsDirty )
    {
        self->refresh();
        self->mIsDirty = false;
    }
}

LLViewerObject* LLSidepanelTaskInfo::getObject()
{
    if (!mObject->isDead())
        return mObject;
    return NULL;
}

LLViewerObject* LLSidepanelTaskInfo::getFirstSelectedObject()
{
    LLSelectNode *node = mObjectSelection->getFirstRootNode();
    if (node)
    {
        return node->getObject();
    }
    return NULL;
}

const LLUUID& LLSidepanelTaskInfo::getSelectedUUID()
{
    const LLViewerObject* obj = getFirstSelectedObject();
    if (obj)
    {
        return obj->getID();
    }
    return LLUUID::null;
}
