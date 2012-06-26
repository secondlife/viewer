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
#include "llcheckboxctrl.h"
#include "llviewerobject.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llstatusbar.h"		// for getBalance()
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

///----------------------------------------------------------------------------
/// Class llsidepaneltaskinfo
///----------------------------------------------------------------------------

LLSidepanelTaskInfo* LLSidepanelTaskInfo::sActivePanel = NULL;

static LLRegisterPanelClassWrapper<LLSidepanelTaskInfo> t_task_info("sidepanel_task_info");

// Default constructor
LLSidepanelTaskInfo::LLSidepanelTaskInfo()
{
	setMouseOpaque(FALSE);
	LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLSidepanelTaskInfo::refreshAll, this));
}


LLSidepanelTaskInfo::~LLSidepanelTaskInfo()
{
	if (sActivePanel == this)
		sActivePanel = NULL;
}

// virtual
BOOL LLSidepanelTaskInfo::postBuild()
{
	LLSidepanelInventorySubpanel::postBuild();

	mOpenBtn = getChild<LLButton>("open_btn");
	mOpenBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onOpenButtonClicked, this));
	mPayBtn = getChild<LLButton>("pay_btn");
	mPayBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onPayButtonClicked, this));
	mBuyBtn = getChild<LLButton>("buy_btn");
	mBuyBtn->setClickedCallback(boost::bind(&handle_buy));
	mDetailsBtn = getChild<LLButton>("details_btn");
	mDetailsBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onDetailsButtonClicked, this));

	mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");

	childSetCommitCallback("Object Name",						LLSidepanelTaskInfo::onCommitName,this);
	getChild<LLLineEditor>("Object Name")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
	childSetCommitCallback("Object Description",				LLSidepanelTaskInfo::onCommitDesc,this);
	getChild<LLLineEditor>("Object Description")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
	getChild<LLUICtrl>("button set group")->setCommitCallback(boost::bind(&LLSidepanelTaskInfo::onClickGroup,this));
	childSetCommitCallback("checkbox share with group",			&LLSidepanelTaskInfo::onCommitGroupShare,this);
	childSetAction("button deed",								&LLSidepanelTaskInfo::onClickDeedToGroup,this);
	childSetCommitCallback("checkbox allow everyone move",		&LLSidepanelTaskInfo::onCommitEveryoneMove,this);
	childSetCommitCallback("checkbox allow everyone copy",		&LLSidepanelTaskInfo::onCommitEveryoneCopy,this);
	childSetCommitCallback("checkbox for sale",					&LLSidepanelTaskInfo::onCommitSaleInfo,this);
	childSetCommitCallback("sale type",							&LLSidepanelTaskInfo::onCommitSaleType,this);
	childSetCommitCallback("Edit Cost", 						&LLSidepanelTaskInfo::onCommitSaleInfo, this);
	childSetCommitCallback("checkbox next owner can modify",	&LLSidepanelTaskInfo::onCommitNextOwnerModify,this);
	childSetCommitCallback("checkbox next owner can copy",		&LLSidepanelTaskInfo::onCommitNextOwnerCopy,this);
	childSetCommitCallback("checkbox next owner can transfer",	&LLSidepanelTaskInfo::onCommitNextOwnerTransfer,this);
	childSetCommitCallback("clickaction",						&LLSidepanelTaskInfo::onCommitClickAction,this);
	childSetCommitCallback("search_check",						&LLSidepanelTaskInfo::onCommitIncludeInSearch,this);
	
	mDAPermModify = getChild<LLUICtrl>("perm_modify");
	mDACreator = getChildView("Creator:");
	mDACreatorName = getChild<LLUICtrl>("Creator Name");
	mDAOwner = getChildView("Owner:");
	mDAOwnerName = getChild<LLUICtrl>("Owner Name");
	mDAGroup = getChildView("Group:");
	mDAGroupName = getChild<LLUICtrl>("Group Name");
	mDAButtonSetGroup = getChildView("button set group");
	mDAObjectName = getChild<LLUICtrl>("Object Name");
	mDAName = getChildView("Name:");
	mDADescription = getChildView("Description:");
	mDAObjectDescription = getChild<LLUICtrl>("Object Description");
	mDAPermissions = getChildView("Permissions:");
	mDACheckboxShareWithGroup = getChild<LLUICtrl>("checkbox share with group");
	mDAButtonDeed = getChildView("button deed");
	mDACheckboxAllowEveryoneMove = getChild<LLUICtrl>("checkbox allow everyone move");
	mDACheckboxAllowEveryoneCopy = getChild<LLUICtrl>("checkbox allow everyone copy");
	mDANextOwnerCan = getChildView("Next owner can:");
	mDACheckboxNextOwnerCanModify = getChild<LLUICtrl>("checkbox next owner can modify");
	mDACheckboxNextOwnerCanCopy = getChild<LLUICtrl>("checkbox next owner can copy");
	mDACheckboxNextOwnerCanTransfer = getChild<LLUICtrl>("checkbox next owner can transfer");
	mDACheckboxForSale = getChild<LLUICtrl>("checkbox for sale");
	mDASearchCheck = getChild<LLUICtrl>("search_check");
	mDAComboSaleType = getChild<LLComboBox>("sale type");
	mDACost = getChild<LLUICtrl>("Cost");
	mDAEditCost = getChild<LLUICtrl>("Edit Cost");
	mDALabelClickAction = getChildView("label click action");
	mDAComboClickAction = getChild<LLComboBox>("clickaction");
	mDAB = getChildView("B:");
	mDAO = getChildView("O:");
	mDAG = getChildView("G:");
	mDAE = getChildView("E:");
	mDAN = getChildView("N:");
	mDAF = getChildView("F:");
	
	return TRUE;
}

/*virtual*/ void LLSidepanelTaskInfo::handleVisibilityChange ( BOOL visible )
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
	mDAPermModify->setEnabled(FALSE);
	mDAPermModify->setValue(LLStringUtil::null);

	mDACreator->setEnabled(FALSE);
	mDACreatorName->setValue(LLStringUtil::null);
	mDACreatorName->setEnabled(FALSE);

	mDAOwner->setEnabled(FALSE);
	mDAOwnerName->setValue(LLStringUtil::null);
	mDAOwnerName->setEnabled(FALSE);

	mDAGroup->setEnabled(FALSE);
	mDAGroupName->setValue(LLStringUtil::null);
	mDAGroupName->setEnabled(FALSE);
	mDAButtonSetGroup->setEnabled(FALSE);

	mDAObjectName->setValue(LLStringUtil::null);
	mDAObjectName->setEnabled(FALSE);
	mDAName->setEnabled(FALSE);
	mDAGroupName->setValue(LLStringUtil::null);
	mDAGroupName->setEnabled(FALSE);
	mDADescription->setEnabled(FALSE);
	mDAObjectDescription->setValue(LLStringUtil::null);
	mDAObjectDescription->setEnabled(FALSE);

	mDAPermissions->setEnabled(FALSE);
		
	mDACheckboxShareWithGroup->setValue(FALSE);
	mDACheckboxShareWithGroup->setEnabled(FALSE);
	mDAButtonDeed->setEnabled(FALSE);

	mDACheckboxAllowEveryoneMove->setValue(FALSE);
	mDACheckboxAllowEveryoneMove->setEnabled(FALSE);
	mDACheckboxAllowEveryoneCopy->setValue(FALSE);
	mDACheckboxAllowEveryoneCopy->setEnabled(FALSE);

	//Next owner can:
	mDANextOwnerCan->setEnabled(FALSE);
	mDACheckboxNextOwnerCanModify->setValue(FALSE);
	mDACheckboxNextOwnerCanModify->setEnabled(FALSE);
	mDACheckboxNextOwnerCanCopy->setValue(FALSE);
	mDACheckboxNextOwnerCanCopy->setEnabled(FALSE);
	mDACheckboxNextOwnerCanTransfer->setValue(FALSE);
	mDACheckboxNextOwnerCanTransfer->setEnabled(FALSE);

	//checkbox for sale
	mDACheckboxForSale->setValue(FALSE);
	mDACheckboxForSale->setEnabled(FALSE);

	//checkbox include in search
	mDASearchCheck->setValue(FALSE);
	mDASearchCheck->setEnabled(FALSE);
		
	mDAComboSaleType->setValue(LLSaleInfo::FS_COPY);
	mDAComboSaleType->setEnabled(FALSE);
		
	mDACost->setEnabled(FALSE);
	mDACost->setValue(getString("Cost Default"));
	mDAEditCost->setValue(LLStringUtil::null);
	mDAEditCost->setEnabled(FALSE);
		
	mDALabelClickAction->setEnabled(FALSE);
	if (mDAComboClickAction)
	{
		mDAComboClickAction->setEnabled(FALSE);
		mDAComboClickAction->clear();
	}

	mDAB->setVisible(FALSE);
	mDAO->setVisible(FALSE);
	mDAG->setVisible(FALSE);
	mDAE->setVisible(FALSE);
	mDAN->setVisible(FALSE);
	mDAF->setVisible(FALSE);
	
	mOpenBtn->setEnabled(FALSE);
	mPayBtn->setEnabled(FALSE);
	mBuyBtn->setEnabled(FALSE);
}

void LLSidepanelTaskInfo::refresh()
{
	LLButton* btn_deed_to_group = getChild<LLButton>("button deed");
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

	BOOL root_selected = TRUE;
	LLSelectNode* nodep = mObjectSelection->getFirstRootNode();
	S32 object_count = mObjectSelection->getRootObjectCount();
	if (!nodep || (object_count == 0))
	{
		nodep = mObjectSelection->getFirstNode();
		object_count = mObjectSelection->getObjectCount();
		root_selected = FALSE;
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
	const BOOL is_one_object = (object_count == 1);
	
	// BUG: fails if a root and non-root are both single-selected.
	const BOOL is_perm_modify = (mObjectSelection->getFirstRootNode() && LLSelectMgr::getInstance()->selectGetRootsModify()) ||
		LLSelectMgr::getInstance()->selectGetModify();

	S32 string_index = 0;
	std::string MODIFY_INFO_STRINGS[] =
		{
			getString("text modify info 1"),
			getString("text modify info 2"),
			getString("text modify info 3"),
			getString("text modify info 4")
		};
	if (!is_perm_modify)
	{
		string_index += 2;
	}
	if (!is_one_object)
	{
		++string_index;
	}
	getChildView("perm_modify")->setEnabled(TRUE);
	getChild<LLUICtrl>("perm_modify")->setValue(MODIFY_INFO_STRINGS[string_index]);

	getChildView("Permissions:")->setEnabled(TRUE);
	
	// Update creator text field
	getChildView("Creator:")->setEnabled(TRUE);
	BOOL creators_identical;
	std::string creator_name;
	creators_identical = LLSelectMgr::getInstance()->selectGetCreator(mCreatorID,
																	  creator_name);

	getChild<LLUICtrl>("Creator Name")->setValue(creator_name);
	getChildView("Creator Name")->setEnabled(TRUE);

	// Update owner text field
	getChildView("Owner:")->setEnabled(TRUE);

	std::string owner_name;
	const BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_name);
	if (mOwnerID.isNull())
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
	getChild<LLUICtrl>("Owner Name")->setValue(owner_name);
	getChildView("Owner Name")->setEnabled(TRUE);

	// update group text field
	getChildView("Group:")->setEnabled(TRUE);
	getChild<LLUICtrl>("Group Name")->setValue(LLStringUtil::null);
	LLUUID group_id;
	BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
	if (groups_identical)
	{
		if (mLabelGroupName)
		{
			mLabelGroupName->setNameID(group_id,TRUE);
			mLabelGroupName->setEnabled(TRUE);
		}
	}
	else
	{
		if (mLabelGroupName)
		{
			mLabelGroupName->setNameID(LLUUID::null, TRUE);
			mLabelGroupName->refresh(LLUUID::null, std::string(), true);
			mLabelGroupName->setEnabled(FALSE);
		}
	}
	
	getChildView("button set group")->setEnabled(owners_identical && (mOwnerID == gAgent.getID()));

	getChildView("Name:")->setEnabled(TRUE);
	LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
	getChildView("Description:")->setEnabled(TRUE);
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
	BOOL edit_name_desc = FALSE;
	if (is_one_object && objectp->permModify())
	{
		edit_name_desc = TRUE;
	}
	if (edit_name_desc)
	{
		getChildView("Object Name")->setEnabled(TRUE);
		getChildView("Object Description")->setEnabled(TRUE);
	}
	else
	{
		getChildView("Object Name")->setEnabled(FALSE);
		getChildView("Object Description")->setEnabled(FALSE);
	}

	S32 total_sale_price = 0;
	S32 individual_sale_price = 0;
	BOOL is_for_sale_mixed = FALSE;
	BOOL is_sale_price_mixed = FALSE;
	U32 num_for_sale = FALSE;
    LLSelectMgr::getInstance()->selectGetAggregateSaleInfo(num_for_sale,
														   is_for_sale_mixed,
														   is_sale_price_mixed,
														   total_sale_price,
														   individual_sale_price);

	const BOOL self_owned = (gAgent.getID() == mOwnerID);
	const BOOL group_owned = LLSelectMgr::getInstance()->selectIsGroupOwned() ;
	const BOOL public_owned = (mOwnerID.isNull() && !LLSelectMgr::getInstance()->selectIsGroupOwned());
	const BOOL can_transfer = LLSelectMgr::getInstance()->selectGetRootsTransfer();
	const BOOL can_copy = LLSelectMgr::getInstance()->selectGetRootsCopy();

	if (!owners_identical)
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		getChildView("Edit Cost")->setEnabled(FALSE);
	}
	// You own these objects.
	else if (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		// If there are multiple items for sale then set text to PRICE PER UNIT.
		if (num_for_sale > 1)
		{
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Per Unit"));
		}
		else
		{
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
		}
		
		LLSpinCtrl *edit_price = getChild<LLSpinCtrl>("Edit Cost");
		if (!edit_price->hasFocus())
		{
			// If the sale price is mixed then set the cost to MIXED, otherwise
			// set to the actual cost.
			if ((num_for_sale > 0) && is_for_sale_mixed)
			{
				edit_price->setTentative(TRUE);
			}
			else if ((num_for_sale > 0) && is_sale_price_mixed)
			{
				edit_price->setTentative(TRUE);
			}
			else 
			{
				edit_price->setValue(individual_sale_price);
			}
		}
		// The edit fields are only enabled if you can sell this object
		// and the sale price is not mixed.
		BOOL enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : FALSE;
		getChildView("Cost")->setEnabled(enable_edit);
		getChildView("Edit Cost")->setEnabled(enable_edit);
	}
	// Someone, not you, owns these objects.
	else if (!public_owned)
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChildView("Edit Cost")->setEnabled(FALSE);
		
		// Don't show a price if none of the items are for sale.
		if (num_for_sale)
			getChild<LLUICtrl>("Edit Cost")->setValue(llformat("%d",total_sale_price));
		else
			getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);

		// If multiple items are for sale, set text to TOTAL PRICE.
		if (num_for_sale > 1)
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Total"));
		else
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
	}
	// This is a public object.
	else
	{
		getChildView("Cost")->setEnabled(FALSE);
		getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
		
		getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		getChildView("Edit Cost")->setEnabled(FALSE);
	}

	// Enable and disable the permissions checkboxes
	// based on who owns the object.
	// TODO: Creator permissions

	U32 base_mask_on 			= 0;
	U32 base_mask_off		 	= 0;
	U32 owner_mask_off			= 0;
	U32 owner_mask_on 			= 0;
	U32 group_mask_on 			= 0;
	U32 group_mask_off 			= 0;
	U32 everyone_mask_on 		= 0;
	U32 everyone_mask_off 		= 0;
	U32 next_owner_mask_on 		= 0;
	U32 next_owner_mask_off		= 0;

	BOOL valid_base_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_BASE,
																			&base_mask_on,
																			&base_mask_off);
	//BOOL valid_owner_perms =//
	LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
											  &owner_mask_on,
											  &owner_mask_off);
	BOOL valid_group_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
																			&group_mask_on,
																			&group_mask_off);
	
	BOOL valid_everyone_perms 	= LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
																			&everyone_mask_on,
																			&everyone_mask_off);
	
	BOOL valid_next_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_NEXT_OWNER,
																			&next_owner_mask_on,
																			&next_owner_mask_off);

	
	if (gSavedSettings.getBOOL("DebugPermissions") )
	{
		if (valid_base_perms)
		{
			getChild<LLUICtrl>("B:")->setValue("B: " + mask_to_string(base_mask_on));
			getChildView("B:")->setVisible(							TRUE);
			
			getChild<LLUICtrl>("O:")->setValue("O: " + mask_to_string(owner_mask_on));
			getChildView("O:")->setVisible(							TRUE);
			
			getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(group_mask_on));
			getChildView("G:")->setVisible(							TRUE);
			
			getChild<LLUICtrl>("E:")->setValue("E: " + mask_to_string(everyone_mask_on));
			getChildView("E:")->setVisible(							TRUE);
			
			getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(next_owner_mask_on));
			getChildView("N:")->setVisible(							TRUE);
		}

		U32 flag_mask = 0x0;
		if (objectp->permMove()) 		flag_mask |= PERM_MOVE;
		if (objectp->permModify()) 		flag_mask |= PERM_MODIFY;
		if (objectp->permCopy()) 		flag_mask |= PERM_COPY;
		if (objectp->permTransfer()) 	flag_mask |= PERM_TRANSFER;

		getChild<LLUICtrl>("F:")->setValue("F:" + mask_to_string(flag_mask));
		getChildView("F:")->setVisible(								TRUE);
	}
	else
	{
		getChildView("B:")->setVisible(								FALSE);
		getChildView("O:")->setVisible(								FALSE);
		getChildView("G:")->setVisible(								FALSE);
		getChildView("E:")->setVisible(								FALSE);
		getChildView("N:")->setVisible(								FALSE);
		getChildView("F:")->setVisible(								FALSE);
	}

	BOOL has_change_perm_ability = FALSE;
	BOOL has_change_sale_ability = FALSE;

	if (valid_base_perms &&
		(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = TRUE;
	}
	if (valid_base_perms &&
	   (self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_SET_SALE))))
	{
		has_change_sale_ability = TRUE;
	}

	if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
	{
		// ...must select root to choose permissions
		getChild<LLUICtrl>("perm_modify")->setValue(getString("text modify warning"));
	}

	if (has_change_perm_ability)
	{
		getChildView("checkbox share with group")->setEnabled(TRUE);
		getChildView("checkbox allow everyone move")->setEnabled(owner_mask_on & PERM_MOVE);
		getChildView("checkbox allow everyone copy")->setEnabled(owner_mask_on & PERM_COPY && owner_mask_on & PERM_TRANSFER);
	}
	else
	{
		getChildView("checkbox share with group")->setEnabled(FALSE);
		getChildView("checkbox allow everyone move")->setEnabled(FALSE);
		getChildView("checkbox allow everyone copy")->setEnabled(FALSE);
	}

	if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
	{
		getChildView("checkbox for sale")->setEnabled(can_transfer || (!can_transfer && num_for_sale));
		// Set the checkbox to tentative if the prices of each object selected
		// are not the same.
		getChild<LLUICtrl>("checkbox for sale")->setTentative( 				is_for_sale_mixed);
		getChildView("sale type")->setEnabled(num_for_sale && can_transfer && !is_sale_price_mixed);

		getChildView("Next owner can:")->setEnabled(TRUE);
		getChildView("checkbox next owner can modify")->setEnabled(base_mask_on & PERM_MODIFY);
		getChildView("checkbox next owner can copy")->setEnabled(base_mask_on & PERM_COPY);
		getChildView("checkbox next owner can transfer")->setEnabled(next_owner_mask_on & PERM_COPY);
	}
	else 
	{
		getChildView("checkbox for sale")->setEnabled(FALSE);
		getChildView("sale type")->setEnabled(FALSE);

		getChildView("Next owner can:")->setEnabled(FALSE);
		getChildView("checkbox next owner can modify")->setEnabled(FALSE);
		getChildView("checkbox next owner can copy")->setEnabled(FALSE);
		getChildView("checkbox next owner can transfer")->setEnabled(FALSE);
	}

	if (valid_group_perms)
	{
		if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	FALSE);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
		else if ((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	FALSE);
			getChildView("button deed")->setEnabled(FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	TRUE);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
	}			

	if (valid_everyone_perms)
	{
		// Move
		if (everyone_mask_on & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	FALSE);
		}
		else if (everyone_mask_off & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	TRUE);
		}

		// Copy == everyone can't copy
		if (everyone_mask_on & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative( 	!can_copy || !can_transfer);
		}
		else if (everyone_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	TRUE);
		}
	}

	if (valid_next_perms)
	{
		// Modify == next owner canot modify
		if (next_owner_mask_on & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	FALSE);
		}
		else if (next_owner_mask_off & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	TRUE);
		}

		// Copy == next owner cannot copy
		if (next_owner_mask_on & PERM_COPY)
		{			
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	!can_copy);
		}
		else if (next_owner_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	TRUE);
		}

		// Transfer == next owner cannot transfer
		if (next_owner_mask_on & PERM_TRANSFER)
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( !can_transfer);
		}
		else if (next_owner_mask_off & PERM_TRANSFER)
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(FALSE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( FALSE);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(TRUE);
			getChild<LLUICtrl>("checkbox next owner can transfer")->setTentative( TRUE);
		}
	}

	// reflect sale information
	LLSaleInfo sale_info;
	BOOL valid_sale_info = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();

	LLComboBox* combo_sale_type = getChild<LLComboBox>("sale type");
	if (valid_sale_info)
	{
		combo_sale_type->setValue(					sale_type == LLSaleInfo::FS_NOT ? LLSaleInfo::FS_COPY : sale_type);
		combo_sale_type->setTentative(				FALSE); // unfortunately this doesn't do anything at the moment.
	}
	else
	{
		// default option is sell copy, determined to be safest
		combo_sale_type->setValue(					LLSaleInfo::FS_COPY);
		combo_sale_type->setTentative(				TRUE); // unfortunately this doesn't do anything at the moment.
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
	const BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );
	bool include_in_search;
	const BOOL all_include_in_search = LLSelectMgr::getInstance()->selectionGetIncludeInSearch(&include_in_search);
	getChildView("search_check")->setEnabled(has_change_sale_ability && all_volume);
	getChild<LLUICtrl>("search_check")->setValue(include_in_search);
	getChild<LLUICtrl>("search_check")->setTentative( 				!all_include_in_search);

	// Click action (touch, sit, buy)
	U8 click_action = 0;
	if (LLSelectMgr::getInstance()->selectionGetClickAction(&click_action))
	{
		LLComboBox*	ComboClickAction = getChild<LLComboBox>("clickaction");
		if (ComboClickAction)
		{
			ComboClickAction->setCurrentByIndex((S32)click_action);
		}
	}
	getChildView("label click action")->setEnabled(is_perm_modify && all_volume);
	getChildView("clickaction")->setEnabled(is_perm_modify && all_volume);

	if (!getIsEditing())
	{
		const std::string no_item_names[] = 
			{
				"Object Name",
				"Object Description",
				"button set group",
				"checkbox share with group",
				"button deed",
				"checkbox allow everyone move",
				"checkbox allow everyone copy",
				"checkbox for sale",
				"sale type",
				"Edit Cost",
				"checkbox next owner can modify",
				"checkbox next owner can copy",
				"checkbox next owner can transfer",
				"clickaction",
				"search_check",
				"perm_modify",
				"Group Name",
			};
		for (size_t t=0; t<LL_ARRAY_SIZE(no_item_names); ++t)
		{
			getChildView(no_item_names[t])->setEnabled(	FALSE);
		}
	}
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
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
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
		mLabelGroupName->setNameID(group_id, TRUE);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

static bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	const S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLUUID group_id;
		const BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if (group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
//			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_RELEASE_COUNT);
		}
	}
	return FALSE;
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
	BOOL new_state = check->get();
	
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);
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
	//llinfos << "LLSidepanelTaskInfo::onCommitNextOwnerModify" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}

// static
void LLSidepanelTaskInfo::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLSidepanelTaskInfo::onCommitNextOwnerCopy" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}

// static
void LLSidepanelTaskInfo::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLSidepanelTaskInfo::onCommitNextOwnerTransfer" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}

// static
void LLSidepanelTaskInfo::onCommitName(LLUICtrl*, void* data)
{
	//llinfos << "LLSidepanelTaskInfo::onCommitName()" << llendl;
	LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
	LLLineEditor*	tb = self->getChild<LLLineEditor>("Object Name");
	if(tb)
	{
		LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
//		LLSelectMgr::getInstance()->selectionSetObjectName(self->mLabelObjectName->getText());
	}
}


// static
void LLSidepanelTaskInfo::onCommitDesc(LLUICtrl*, void* data)
{
	//llinfos << "LLSidepanelTaskInfo::onCommitDesc()" << llendl;
	LLSidepanelTaskInfo* self = (LLSidepanelTaskInfo*)data;
	LLLineEditor*	le = self->getChild<LLLineEditor>("Object Description");
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
	return CLICK_ACTION_TOUCH;
}

// static
void LLSidepanelTaskInfo::onCommitClickAction(LLUICtrl* ctrl, void*)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if (!box) return;
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
	LLSidepanelInventorySubpanel::updateVerbs();

	/*
	mOpenBtn->setVisible(!getIsEditing());
	mPayBtn->setVisible(!getIsEditing());
	mBuyBtn->setVisible(!getIsEditing());
	//const LLViewerObject *obj = getFirstSelectedObject();
	//mEditBtn->setEnabled(obj && obj->permModify());
	*/

	LLSafeHandle<LLObjectSelection> object_selection = LLSelectMgr::getInstance()->getSelection();
	const BOOL any_selected = (object_selection->getNumNodes() > 0);

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
		setFocus(FALSE);
	}
	refresh();
	if (focus)
	{
		focus->setFocus(TRUE);
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
