/** 
 * @file llpanelpermissions.cpp
 * @brief LLPanelPermissions class implementation
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

#include "llpanelpermissions.h"

// library includes
#include "lluuid.h"
#include "llpermissions.h"
#include "llcategory.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
#include "llnotificationsutil.h"
#include "llstring.h"

// project includes
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
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "roles_constants.h"
#include "llgroupactions.h"
#include "lltrans.h"


U8 string_value_to_click_action(std::string p_value);
std::string click_action_to_string_value( U8 action);

U8 string_value_to_click_action(std::string p_value)
{
	if(p_value == "Touch")
	{
		return CLICK_ACTION_TOUCH;
	}
	if(p_value == "Sit")
	{
		return CLICK_ACTION_SIT;
	}
	if(p_value == "Buy")
	{
		return CLICK_ACTION_BUY;
	}
	if(p_value == "Pay")
	{
		return CLICK_ACTION_PAY;
	}
	if(p_value == "Open")
	{
		return CLICK_ACTION_OPEN;
	}
	if(p_value == "Zoom")
	{
		return CLICK_ACTION_ZOOM;
	}
	return CLICK_ACTION_TOUCH;
}

std::string click_action_to_string_value( U8 action)
{
	switch (action) 
	{
		case CLICK_ACTION_TOUCH:
		default:	
			return "Touch";
			break;
		case CLICK_ACTION_SIT:
			return "Sit";
			break;
		case CLICK_ACTION_BUY:
			return "Buy";
			break;
		case CLICK_ACTION_PAY:
			return "Pay";
			break;
		case CLICK_ACTION_OPEN:
			return "Open";
			break;
		case CLICK_ACTION_ZOOM:
			return "Zoom";
			break;
	}
}

///----------------------------------------------------------------------------
/// Class llpanelpermissions
///----------------------------------------------------------------------------

// Default constructor
LLPanelPermissions::LLPanelPermissions() :
	LLPanel()
{
	setMouseOpaque(FALSE);
}

BOOL LLPanelPermissions::postBuild()
{
	childSetCommitCallback("Object Name",LLPanelPermissions::onCommitName,this);
	getChild<LLLineEditor>("Object Name")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);
	childSetCommitCallback("Object Description",LLPanelPermissions::onCommitDesc,this);
	getChild<LLLineEditor>("Object Description")->setPrevalidate(LLTextValidate::validateASCIIPrintableNoPipe);

	
	getChild<LLUICtrl>("button set group")->setCommitCallback(boost::bind(&LLPanelPermissions::onClickGroup,this));

	childSetCommitCallback("checkbox share with group",LLPanelPermissions::onCommitGroupShare,this);

	childSetAction("button deed",LLPanelPermissions::onClickDeedToGroup,this);

	childSetCommitCallback("checkbox allow everyone move",LLPanelPermissions::onCommitEveryoneMove,this);

	childSetCommitCallback("checkbox allow everyone copy",LLPanelPermissions::onCommitEveryoneCopy,this);
	
	childSetCommitCallback("checkbox for sale",LLPanelPermissions::onCommitSaleInfo,this);

	childSetCommitCallback("sale type",LLPanelPermissions::onCommitSaleType,this);

	childSetCommitCallback("Edit Cost", LLPanelPermissions::onCommitSaleInfo, this);
	
	childSetCommitCallback("checkbox next owner can modify",LLPanelPermissions::onCommitNextOwnerModify,this);
	childSetCommitCallback("checkbox next owner can copy",LLPanelPermissions::onCommitNextOwnerCopy,this);
	childSetCommitCallback("checkbox next owner can transfer",LLPanelPermissions::onCommitNextOwnerTransfer,this);
	childSetCommitCallback("clickaction",LLPanelPermissions::onCommitClickAction,this);
	childSetCommitCallback("search_check",LLPanelPermissions::onCommitIncludeInSearch,this);
	
	mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");

	return TRUE;
}


LLPanelPermissions::~LLPanelPermissions()
{
	// base class will take care of everything
}


void LLPanelPermissions::disableAll()
{
	getChildView("perm_modify")->setEnabled(FALSE);
	getChild<LLUICtrl>("perm_modify")->setValue(LLStringUtil::null);

	getChildView("pathfinding_attributes_value")->setEnabled(FALSE);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLStringUtil::null);

	getChildView("Creator:")->setEnabled(FALSE);
	getChild<LLUICtrl>("Creator Name")->setValue(LLStringUtil::null);
	getChildView("Creator Name")->setEnabled(FALSE);

	getChildView("Owner:")->setEnabled(FALSE);
	getChild<LLUICtrl>("Owner Name")->setValue(LLStringUtil::null);
	getChildView("Owner Name")->setEnabled(FALSE);

	getChildView("Group:")->setEnabled(FALSE);
	getChild<LLUICtrl>("Group Name Proxy")->setValue(LLStringUtil::null);
	getChildView("Group Name Proxy")->setEnabled(FALSE);
	getChildView("button set group")->setEnabled(FALSE);

	getChild<LLUICtrl>("Object Name")->setValue(LLStringUtil::null);
	getChildView("Object Name")->setEnabled(FALSE);
	getChildView("Name:")->setEnabled(FALSE);
	getChild<LLUICtrl>("Group Name")->setValue(LLStringUtil::null);
	getChildView("Group Name")->setEnabled(FALSE);
	getChildView("Description:")->setEnabled(FALSE);
	getChild<LLUICtrl>("Object Description")->setValue(LLStringUtil::null);
	getChildView("Object Description")->setEnabled(FALSE);

	getChildView("Permissions:")->setEnabled(FALSE);
		
	getChild<LLUICtrl>("checkbox share with group")->setValue(FALSE);
	getChildView("checkbox share with group")->setEnabled(FALSE);
	getChildView("button deed")->setEnabled(FALSE);

	getChild<LLUICtrl>("checkbox allow everyone move")->setValue(FALSE);
	getChildView("checkbox allow everyone move")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(FALSE);
	getChildView("checkbox allow everyone copy")->setEnabled(FALSE);

	//Next owner can:
	getChildView("Next owner can:")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can modify")->setValue(FALSE);
	getChildView("checkbox next owner can modify")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can copy")->setValue(FALSE);
	getChildView("checkbox next owner can copy")->setEnabled(FALSE);
	getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(FALSE);
	getChildView("checkbox next owner can transfer")->setEnabled(FALSE);

	//checkbox for sale
	getChild<LLUICtrl>("checkbox for sale")->setValue(FALSE);
	getChildView("checkbox for sale")->setEnabled(FALSE);

	//checkbox include in search
	getChild<LLUICtrl>("search_check")->setValue(FALSE);
	getChildView("search_check")->setEnabled(FALSE);
		
	LLComboBox*	combo_sale_type = getChild<LLComboBox>("sale type");
	combo_sale_type->setValue(LLSaleInfo::FS_COPY);
	combo_sale_type->setEnabled(FALSE);
		
	getChildView("Cost")->setEnabled(FALSE);
	getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
	getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
	getChildView("Edit Cost")->setEnabled(FALSE);
		
	getChildView("label click action")->setEnabled(FALSE);
	LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
	if (combo_click_action)
	{
		combo_click_action->setEnabled(FALSE);
		combo_click_action->clear();
	}
	getChildView("B:")->setVisible(								FALSE);
	getChildView("O:")->setVisible(								FALSE);
	getChildView("G:")->setVisible(								FALSE);
	getChildView("E:")->setVisible(								FALSE);
	getChildView("N:")->setVisible(								FALSE);
	getChildView("F:")->setVisible(								FALSE);
}

void LLPanelPermissions::refresh()
{
	LLButton*	BtnDeedToGroup = getChild<LLButton>("button deed");
	if(BtnDeedToGroup)
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
		BtnDeedToGroup->setLabelSelected(deedText);
		BtnDeedToGroup->setLabelUnselected(deedText);
	}
	BOOL root_selected = TRUE;
	LLSelectNode* nodep = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	S32 object_count = LLSelectMgr::getInstance()->getSelection()->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
		object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		root_selected = FALSE;
	}

	//BOOL attachment_selected = LLSelectMgr::getInstance()->getSelection()->isAttachment();
	//attachment_selected = false;
	LLViewerObject* objectp = NULL;
	if(nodep) objectp = nodep->getObject();
	if(!nodep || !objectp)// || attachment_selected)
	{
		// ...nothing selected
		disableAll();
		return;
	}

	// figure out a few variables
	const BOOL is_one_object = (object_count == 1);
	
	// BUG: fails if a root and non-root are both single-selected.
	BOOL is_perm_modify = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
						   && LLSelectMgr::getInstance()->selectGetRootsModify())
		|| LLSelectMgr::getInstance()->selectGetModify();
	BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
						   && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
		|| LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();
	const LLFocusableElement* keyboard_focus_view = gFocusMgr.getKeyboardFocus();

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
	getChildView("perm_modify")->setEnabled(TRUE);
	getChild<LLUICtrl>("perm_modify")->setValue(MODIFY_INFO_STRINGS[string_index]);

	std::string pfAttrName;

	if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
		&& LLSelectMgr::getInstance()->selectGetRootsNonPathfinding())
		|| LLSelectMgr::getInstance()->selectGetNonPathfinding())
	{
		pfAttrName = "Pathfinding_Object_Attr_None";
	}
	else if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
		&& LLSelectMgr::getInstance()->selectGetRootsPermanent())
		|| LLSelectMgr::getInstance()->selectGetPermanent())
	{
		pfAttrName = "Pathfinding_Object_Attr_Permanent";
	}
	else if ((LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
		&& LLSelectMgr::getInstance()->selectGetRootsCharacter())
		|| LLSelectMgr::getInstance()->selectGetCharacter())
	{
		pfAttrName = "Pathfinding_Object_Attr_Character";
	}
	else
	{
		pfAttrName = "Pathfinding_Object_Attr_MultiSelect";
	}

	getChildView("pathfinding_attributes_value")->setEnabled(TRUE);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLTrans::getString(pfAttrName));

	getChildView("Permissions:")->setEnabled(TRUE);
	
	// Update creator text field
	getChildView("Creator:")->setEnabled(TRUE);
	std::string creator_name;
	LLSelectMgr::getInstance()->selectGetCreator(mCreatorID, creator_name);

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
	
	getChildView("button set group")->setEnabled(owners_identical && (mOwnerID == gAgent.getID()) && is_nonpermanent_enforced);

	getChildView("Name:")->setEnabled(TRUE);
	LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
	getChildView("Description:")->setEnabled(TRUE);
	LLLineEditor* LineEditorObjectDesc = getChild<LLLineEditor>("Object Description");

	if (is_one_object)
	{
		if (keyboard_focus_view != LineEditorObjectName)
		{
			getChild<LLUICtrl>("Object Name")->setValue(nodep->mName);
		}

		if (LineEditorObjectDesc)
		{
			if (keyboard_focus_view != LineEditorObjectDesc)
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
	if (is_one_object && objectp->permModify() && !objectp->isPermanentEnforced())
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

	if (valid_base_perms && is_nonpermanent_enforced &&
		(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = TRUE;
	}
	if (valid_base_perms && is_nonpermanent_enforced &&
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
		LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
		if(combo_click_action)
		{
			const std::string combo_value = click_action_to_string_value(click_action);
			combo_click_action->setValue(LLSD(combo_value));
		}
	}
	getChildView("label click action")->setEnabled(is_perm_modify && is_nonpermanent_enforced  && all_volume);
	getChildView("clickaction")->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);
}


// static
void LLPanelPermissions::onClickClaim(void*)
{
	// try to claim ownership
	LLSelectMgr::getInstance()->sendOwner(gAgent.getID(), gAgent.getGroupID());
}

// static
void LLPanelPermissions::onClickRelease(void*)
{
	// try to release ownership
	LLSelectMgr::getInstance()->sendOwner(LLUUID::null, LLUUID::null);
}

void LLPanelPermissions::onClickGroup()
{
	LLUUID owner_id;
	std::string name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
	LLFloater* parent_floater = gFloaterView->getParentFloater(this);

	if(owners_identical && (owner_id == gAgent.getID()))
	{
		LLFloaterGroupPicker* fg = 	LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
		if (fg)
		{
			fg->setSelectGroupCallback( boost::bind(&LLPanelPermissions::cbGroupID, this, _1) );

			if (parent_floater)
			{
				LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
				fg->setOrigin(new_rect.mLeft, new_rect.mBottom);
				parent_floater->addDependentFloater(fg);
			}
		}
	}
}

void LLPanelPermissions::cbGroupID(LLUUID group_id)
{
	if(mLabelGroupName)
	{
		mLabelGroupName->setNameID(group_id, TRUE);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLUUID group_id;
		BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if(group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, FALSE);
//			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_RELEASE_COUNT);
		}
	}
	return false;
}

void LLPanelPermissions::onClickDeedToGroup(void* data)
{
	LLNotificationsUtil::add( "DeedObjectToGroup", LLSD(), LLSD(), callback_deed_to_group);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

// static
void LLPanelPermissions::onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm)
{
	LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	if(!object) return;

	// Checkbox will have toggled itself
	// LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	BOOL new_state = check->get();
	
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);
}

// static
void LLPanelPermissions::onCommitGroupShare(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}

// static
void LLPanelPermissions::onCommitEveryoneMove(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_MOVE);
}


// static
void LLPanelPermissions::onCommitEveryoneCopy(LLUICtrl *ctrl, void *data)
{
	onCommitPerm(ctrl, data, PERM_EVERYONE, PERM_COPY);
}

// static
void LLPanelPermissions::onCommitNextOwnerModify(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitNextOwnerModify" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}

// static
void LLPanelPermissions::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitNextOwnerCopy" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}

// static
void LLPanelPermissions::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitNextOwnerTransfer" << llendl;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}

// static
void LLPanelPermissions::onCommitName(LLUICtrl*, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitName()" << llendl;
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	tb = self->getChild<LLLineEditor>("Object Name");
	if(tb)
	{
		LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
//		LLSelectMgr::getInstance()->selectionSetObjectName(self->mLabelObjectName->getText());
	}
}


// static
void LLPanelPermissions::onCommitDesc(LLUICtrl*, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitDesc()" << llendl;
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	le = self->getChild<LLLineEditor>("Object Description");
	if(le)
	{
		LLSelectMgr::getInstance()->selectionSetObjectDescription(le->getText());
	}
}

// static
void LLPanelPermissions::onCommitSaleInfo(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	self->setAllSaleInfo();
}

// static
void LLPanelPermissions::onCommitSaleType(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	self->setAllSaleInfo();
}

void LLPanelPermissions::setAllSaleInfo()
{
	llinfos << "LLPanelPermissions::setAllSaleInfo()" << llendl;
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
		return (obj->flagTakesMoney() 
			   || (parent && parent->flagTakesMoney()));
	}
};

// static
void LLPanelPermissions::onCommitClickAction(LLUICtrl* ctrl, void*)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if (!box) return;
	std::string value = box->getValue().asString();
	U8 click_action = string_value_to_click_action(value);
	
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
			std::string item_value = click_action_to_string_value(click_action);
			box->setValue(LLSD(item_value));
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
void LLPanelPermissions::onCommitIncludeInSearch(LLUICtrl* ctrl, void*)
{
	LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
	llassert(box);

	LLSelectMgr::getInstance()->selectionSetIncludeInSearch(box->get());
}

