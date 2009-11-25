/** 
 * @file llsidepaneltaskinfo.cpp
 * @brief LLSidepanelTaskInfo class implementation
 * This class represents the panel in the build view for
 * viewing/editing object names, owners, permissions, etc.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llsidepaneltaskinfo.h"

#include "lluuid.h"
#include "llpermissions.h"
#include "llcategory.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
#include "llnotifications.h"
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
	mBuyBtn->setClickedCallback(boost::bind(&LLSidepanelTaskInfo::onBuyButtonClicked, this));

	childSetPrevalidate("Object Name",LLLineEditor::prevalidateASCIIPrintableNoPipe);
	childSetPrevalidate("Object Description",LLLineEditor::prevalidateASCIIPrintableNoPipe);

//	getChild<LLUICtrl>("button set group")->setCommitCallback(boost::bind(&LLSidepanelTaskInfo::onClickGroup,this));
//	childSetAction("button deed",LLSidepanelTaskInfo::onClickDeedToGroup,this);

	mLabelGroupName = getChild<LLNameBox>("Group Name Proxy");

	return TRUE;
}

// virtual
void LLSidepanelTaskInfo::setVisible(BOOL visible)
{
	LLPanel::setVisible(visible);
	if (visible)
	{
		sActivePanel = this;
	}
	else
	{
		sActivePanel = NULL;
	}
}

void LLSidepanelTaskInfo::refresh()
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
	LLSelectNode* nodep = mObjectSelection->getFirstRootNode();
	S32 object_count = mObjectSelection->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = mObjectSelection->getFirstNode();
		object_count = mObjectSelection->getObjectCount();
		root_selected = FALSE;
	}

	//BOOL attachment_selected = mObjectSelection->isAttachment();
	//attachment_selected = false;
	LLViewerObject* objectp = NULL;
	if(nodep) objectp = nodep->getObject();
	if(!nodep || !objectp)// || attachment_selected)
	{
		// ...nothing selected
		childSetEnabled("perm_modify",false);
		childSetText("perm_modify",LLStringUtil::null);

		childSetEnabled("Creator:",false);
		childSetText("Creator Name",LLStringUtil::null);
		childSetEnabled("Creator Name",false);

		childSetEnabled("Owner:",false);
		childSetText("Owner Name",LLStringUtil::null);
		childSetEnabled("Owner Name",false);

		childSetEnabled("Group:",false);
		childSetText("Group Name",LLStringUtil::null);
		childSetEnabled("Group Name",false);
		childSetEnabled("button set group",false);

		childSetText("Object Name",LLStringUtil::null);
		childSetEnabled("Object Name",false);
		childSetEnabled("Name:",false);
		childSetText("Group Name",LLStringUtil::null);
		childSetEnabled("Group Name",false);
		childSetEnabled("Description:",false);
		childSetText("Object Description",LLStringUtil::null);
		childSetEnabled("Object Description",false);

		childSetEnabled("Permissions:",false);
		
		childSetValue("checkbox share with group",FALSE);
		childSetEnabled("checkbox share with group",false);
		childSetEnabled("button deed",false);

		childSetValue("checkbox allow everyone move",FALSE);
		childSetEnabled("checkbox allow everyone move",false);
		childSetValue("checkbox allow everyone copy",FALSE);
		childSetEnabled("checkbox allow everyone copy",false);

		//Next owner can:
		childSetEnabled("Next owner can:",false);
		childSetValue("checkbox next owner can modify",FALSE);
		childSetEnabled("checkbox next owner can modify",false);
		childSetValue("checkbox next owner can copy",FALSE);
		childSetEnabled("checkbox next owner can copy",false);
		childSetValue("checkbox next owner can transfer",FALSE);
		childSetEnabled("checkbox next owner can transfer",false);

		//checkbox for sale
		childSetValue("checkbox for sale",FALSE);
		childSetEnabled("checkbox for sale",false);

		//checkbox include in search
		childSetValue("search_check", FALSE);
		childSetEnabled("search_check", false);
		
		LLComboBox*	combo_sale_type = getChild<LLComboBox>("sale type");
		combo_sale_type->setValue(LLSaleInfo::FS_COPY);
		combo_sale_type->setEnabled(FALSE);
		
		childSetEnabled("Cost",false);
		childSetText("Cost",getString("Cost Default"));
		childSetText("Edit Cost",LLStringUtil::null);
		childSetEnabled("Edit Cost",false);
		
		childSetEnabled("label click action",false);
		LLComboBox*	ComboClickAction = getChild<LLComboBox>("clickaction");
		if(ComboClickAction)
		{
			ComboClickAction->setEnabled(FALSE);
			ComboClickAction->clear();
		}
		childSetVisible("B:",false);
		childSetVisible("O:",false);
		childSetVisible("G:",false);
		childSetVisible("E:",false);
		childSetVisible("N:",false);
		childSetVisible("F:",false);

		return;
	}

	// figure out a few variables
	BOOL is_one_object = (object_count == 1);

	// BUG: fails if a root and non-root are both single-selected.
	BOOL is_perm_modify = (mObjectSelection->getFirstRootNode() 
							&& LLSelectMgr::getInstance()->selectGetRootsModify()) 
							|| LLSelectMgr::getInstance()->selectGetModify();
	const LLFocusableElement* keyboard_focus_view = gFocusMgr.getKeyboardFocus();
	S32 string_index = 0;
	std::string MODIFY_INFO_STRINGS[] =
	{
		getString("text modify info 1"),
		getString("text modify info 2"),
		getString("text modify info 3"),
		getString("text modify info 4")
	};
	if(!is_perm_modify)
	{
		string_index += 2;
	}
	if(!is_one_object)
	{
		++string_index;
	}
	childSetEnabled("perm_modify",true);
	childSetText("perm_modify",MODIFY_INFO_STRINGS[string_index]);

	childSetEnabled("Permissions:",true);
	
	// Update creator text field
	childSetEnabled("Creator:",true);
	BOOL creators_identical;
	std::string creator_name;
	creators_identical = LLSelectMgr::getInstance()->selectGetCreator(mCreatorID,
													  creator_name);

	childSetText("Creator Name",creator_name);
	childSetEnabled("Creator Name",TRUE);

	// Update owner text field
	childSetEnabled("Owner:",true);

	BOOL owners_identical;
	std::string owner_name;
	owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_name);

//	llinfos << "owners_identical " << (owners_identical ? "TRUE": "FALSE") << llendl;

	if (mOwnerID.isNull())
	{
		if(LLSelectMgr::getInstance()->selectIsGroupOwned())
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
				owner_name.append( last_owner_name );
			}
		}
	}

	childSetText("Owner Name",owner_name);
	childSetEnabled("Owner Name",TRUE);

	// update group text field
	childSetEnabled("Group:",true);
	childSetText("Group Name",LLStringUtil::null);
	LLUUID group_id;
	BOOL groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
	if (groups_identical)
	{
		if(mLabelGroupName)
		{
			mLabelGroupName->setNameID(group_id, TRUE);
			mLabelGroupName->setEnabled(TRUE);
		}
	}
	else
	{
		if(mLabelGroupName)
		{
			mLabelGroupName->setNameID(LLUUID::null, TRUE);
			mLabelGroupName->refresh(LLUUID::null, LLStringUtil::null, LLStringUtil::null, TRUE);
			mLabelGroupName->setEnabled(FALSE);
		}
	}
	
	childSetEnabled("button set group",owners_identical && (mOwnerID == gAgent.getID()));

	// figure out the contents of the name, description, & category
	BOOL edit_name_desc = FALSE;
	if(is_one_object && objectp->permModify())
	{
		edit_name_desc = TRUE;
	}

	childSetEnabled("Name:",true);
	LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
	childSetEnabled("Description:",true);
	LLLineEditor*	LineEditorObjectDesc = getChild<LLLineEditor>("Object Description");

	if(is_one_object)
	{
		if(keyboard_focus_view != LineEditorObjectName)
		{
			childSetText("Object Name",nodep->mName);
		}

		if(LineEditorObjectDesc)
		{
			if(keyboard_focus_view != LineEditorObjectDesc)
			{
				LineEditorObjectDesc->setText(nodep->mDescription);
			}
		}
	}
	else
	{
		childSetText("Object Name",LLStringUtil::null);
		LineEditorObjectDesc->setText(LLStringUtil::null);
	}

	if(edit_name_desc)
	{
		childSetEnabled("Object Name",true);
		childSetEnabled("Object Description",true);
	}
	else
	{
		childSetEnabled("Object Name",false);
		childSetEnabled("Object Description",false);
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

	if(!owners_identical)
	{
		childSetEnabled("Cost",false);
		childSetText("Edit Cost",LLStringUtil::null);
		childSetEnabled("Edit Cost",false);
	}
	// You own these objects.
	else if(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		// If there are multiple items for sale then set text to PRICE PER UNIT.
		if (num_for_sale > 1)
		{
			childSetText("Cost",getString("Cost Per Unit"));
		}
		else
		{
			childSetText("Cost",getString("Cost Default"));
		}
		
		LLSpinCtrl *edit_price = getChild<LLSpinCtrl>("Edit Cost");
		if(!edit_price->hasFocus())
		{
			// If the sale price is mixed then set the cost to MIXED, otherwise
			// set to the actual cost.
			if (num_for_sale > 0 && is_for_sale_mixed)
			{
				edit_price->setTentative(TRUE);
			}
			else if (num_for_sale > 0 && is_sale_price_mixed)
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
		bool enable_edit = (num_for_sale && can_transfer) ? !is_for_sale_mixed : false;
		childSetEnabled("Cost",enable_edit);
		childSetEnabled("Edit Cost",enable_edit);
	}
	// Someone, not you, owns these objects.
	else if(!public_owned)
	{
		childSetEnabled("Cost",false);
		childSetEnabled("Edit Cost",false);
		
		// Don't show a price if none of the items are for sale.
		if (num_for_sale)
			childSetText("Edit Cost",llformat("%d",total_sale_price));
		else
			childSetText("Edit Cost",LLStringUtil::null);

		// If multiple items are for sale, set text to TOTAL PRICE.
		if (num_for_sale > 1)
			childSetText("Cost",getString("Cost Total"));
		else
			childSetText("Cost",getString("Cost Default"));
	}
	// This is a public object.
	else
	{
		childSetEnabled("Cost",false);
		childSetText("Cost",getString("Cost Default"));
		
		childSetText("Edit Cost",LLStringUtil::null);
		childSetEnabled("Edit Cost",false);
	}

	// Enable and disable the permissions checkboxes
	// based on who owns the object.
	// TODO: Creator permissions

	BOOL valid_base_perms		= FALSE;
	BOOL valid_owner_perms		= FALSE;
	BOOL valid_group_perms		= FALSE;
	BOOL valid_everyone_perms	= FALSE;
	BOOL valid_next_perms		= FALSE;

	U32 base_mask_on;
	U32 base_mask_off;
	U32 owner_mask_on;
	U32 owner_mask_off;
	U32 group_mask_on;
	U32 group_mask_off;
	U32 everyone_mask_on;
	U32 everyone_mask_off;
	U32 next_owner_mask_on = 0;
	U32 next_owner_mask_off = 0;

	valid_base_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_BASE,
									  &base_mask_on,
									  &base_mask_off);

	valid_owner_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
									  &owner_mask_on,
									  &owner_mask_off);

	valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
									  &group_mask_on,
									  &group_mask_off);
	
	valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
									  &everyone_mask_on,
									  &everyone_mask_off);
	
	valid_next_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_NEXT_OWNER,
									  &next_owner_mask_on,
									  &next_owner_mask_off);

	
	if( gSavedSettings.getBOOL("DebugPermissions") )
	{
		std::string perm_string;
		if (valid_base_perms)
		{
			perm_string = "B: ";
			perm_string += mask_to_string(base_mask_on);
			childSetText("B:",perm_string);
			childSetVisible("B:",true);
			
			perm_string = "O: ";
			perm_string += mask_to_string(owner_mask_on);
			childSetText("O:",perm_string);
			childSetVisible("O:",true);
			
			perm_string = "G: ";
			perm_string += mask_to_string(group_mask_on);
			childSetText("G:",perm_string);
			childSetVisible("G:",true);
			
			perm_string = "E: ";
			perm_string += mask_to_string(everyone_mask_on);
			childSetText("E:",perm_string);
			childSetVisible("E:",true);
			
			perm_string = "N: ";
			perm_string += mask_to_string(next_owner_mask_on);
			childSetText("N:",perm_string);
			childSetVisible("N:",true);
		}
		perm_string = "F: ";
		U32 flag_mask = 0x0;
		if (objectp->permMove())
			flag_mask |= PERM_MOVE;
		if (objectp->permModify())
			flag_mask |= PERM_MODIFY;
		if (objectp->permCopy())
			flag_mask |= PERM_COPY;
		if (objectp->permTransfer())
			flag_mask |= PERM_TRANSFER;
		perm_string += mask_to_string(flag_mask);
		childSetText("F:",perm_string);
		childSetVisible("F:",true);
	}
	else
	{
		childSetVisible("B:",false);
		childSetVisible("O:",false);
		childSetVisible("G:",false);
		childSetVisible("E:",false);
		childSetVisible("N:",false);
		childSetVisible("F:",false);
	}

	bool has_change_perm_ability = false;
	bool has_change_sale_ability = false;

	if(valid_base_perms 
	   && (self_owned 
		   || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_MANIPULATE))))
	{
		has_change_perm_ability = true;
	}
	if(valid_base_perms 
	   && (self_owned 
		   || (group_owned && gAgent.hasPowerInGroup(group_id, GP_OBJECT_SET_SALE))))
	{
		has_change_sale_ability = true;
	}

	if (!has_change_perm_ability && !has_change_sale_ability && !root_selected)
	{
		// ...must select root to choose permissions
		childSetValue("perm_modify", getString("text modify warning"));
	}

	if (has_change_perm_ability)
	{
		childSetEnabled("checkbox share with group",true);
		childSetEnabled("checkbox allow everyone move",owner_mask_on & PERM_MOVE);
		childSetEnabled("checkbox allow everyone copy",owner_mask_on & PERM_COPY && owner_mask_on & PERM_TRANSFER);
	}
	else
	{
		childSetEnabled("checkbox share with group", FALSE);
		childSetEnabled("checkbox allow everyone move", FALSE);
		childSetEnabled("checkbox allow everyone copy", FALSE);
	}

	if (has_change_sale_ability && (owner_mask_on & PERM_TRANSFER))
	{
		childSetEnabled("checkbox for sale", can_transfer || (!can_transfer && num_for_sale));
		// Set the checkbox to tentative if the prices of each object selected
		// are not the same.
		childSetTentative("checkbox for sale", is_for_sale_mixed);
		childSetEnabled("sale type",num_for_sale && can_transfer && !is_sale_price_mixed);

		childSetEnabled("Next owner can:", TRUE);
		childSetEnabled("checkbox next owner can modify",base_mask_on & PERM_MODIFY);
		childSetEnabled("checkbox next owner can copy",base_mask_on & PERM_COPY);
		childSetEnabled("checkbox next owner can transfer",next_owner_mask_on & PERM_COPY);
	}
	else 
	{
		childSetEnabled("checkbox for sale",FALSE);
		childSetEnabled("sale type",FALSE);

		childSetEnabled("Next owner can:",FALSE);
		childSetEnabled("checkbox next owner can modify",FALSE);
		childSetEnabled("checkbox next owner can copy",FALSE);
		childSetEnabled("checkbox next owner can transfer",FALSE);
	}

	if(valid_group_perms)
	{
		if((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
		{
			childSetValue("checkbox share with group",TRUE);
			childSetTentative("checkbox share with group",FALSE);
			childSetEnabled("button deed",gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
		else if((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
		{
			childSetValue("checkbox share with group",FALSE);
			childSetTentative("checkbox share with group",false);
			childSetEnabled("button deed",false);
		}
		else
		{
			childSetValue("checkbox share with group",TRUE);
			childSetTentative("checkbox share with group",true);
			childSetEnabled("button deed",gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
	}			

	if(valid_everyone_perms)
	{
		// Move
		if(everyone_mask_on & PERM_MOVE)
		{
			childSetValue("checkbox allow everyone move",TRUE);
			childSetTentative("checkbox allow everyone move",false);
		}
		else if(everyone_mask_off & PERM_MOVE)
		{
			childSetValue("checkbox allow everyone move",FALSE);
			childSetTentative("checkbox allow everyone move",false);
		}
		else
		{
			childSetValue("checkbox allow everyone move",TRUE);
			childSetTentative("checkbox allow everyone move",true);
		}

		// Copy == everyone can't copy
		if(everyone_mask_on & PERM_COPY)
		{
			childSetValue("checkbox allow everyone copy",TRUE);
			childSetTentative("checkbox allow everyone copy",!can_copy || !can_transfer);
		}
		else if(everyone_mask_off & PERM_COPY)
		{
			childSetValue("checkbox allow everyone copy",FALSE);
			childSetTentative("checkbox allow everyone copy",false);
		}
		else
		{
			childSetValue("checkbox allow everyone copy",TRUE);
			childSetTentative("checkbox allow everyone copy",true);
		}
	}

	if(valid_next_perms)
	{
		// Modify == next owner canot modify
		if(next_owner_mask_on & PERM_MODIFY)
		{
			childSetValue("checkbox next owner can modify",TRUE);
			childSetTentative("checkbox next owner can modify",false);
		}
		else if(next_owner_mask_off & PERM_MODIFY)
		{
			childSetValue("checkbox next owner can modify",FALSE);
			childSetTentative("checkbox next owner can modify",false);
		}
		else
		{
			childSetValue("checkbox next owner can modify",TRUE);
			childSetTentative("checkbox next owner can modify",true);
		}

		// Copy == next owner cannot copy
		if(next_owner_mask_on & PERM_COPY)
		{			
			childSetValue("checkbox next owner can copy",TRUE);
			childSetTentative("checkbox next owner can copy",!can_copy);
		}
		else if(next_owner_mask_off & PERM_COPY)
		{
			childSetValue("checkbox next owner can copy",FALSE);
			childSetTentative("checkbox next owner can copy",FALSE);
		}
		else
		{
			childSetValue("checkbox next owner can copy",TRUE);
			childSetTentative("checkbox next owner can copy",TRUE);
		}

		// Transfer == next owner cannot transfer
		if(next_owner_mask_on & PERM_TRANSFER)
		{
			childSetValue("checkbox next owner can transfer",TRUE);
			childSetTentative("checkbox next owner can transfer",!can_transfer);
		}
		else if(next_owner_mask_off & PERM_TRANSFER)
		{
			childSetValue("checkbox next owner can transfer",FALSE);
			childSetTentative("checkbox next owner can transfer",FALSE);
		}
		else
		{
			childSetValue("checkbox next owner can transfer",TRUE);
			childSetTentative("checkbox next owner can transfer",TRUE);
		}
	}

	// reflect sale information
	LLSaleInfo sale_info;
	BOOL valid_sale_info = LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();

	LLComboBox* combo_sale_type = getChild<LLComboBox>("sale type");
	if (valid_sale_info)
	{
		combo_sale_type->setValue(sale_type == LLSaleInfo::FS_NOT ? LLSaleInfo::FS_COPY : sale_type);
		combo_sale_type->setTentative(FALSE); // unfortunately this doesn't do anything at the moment.
	}
	else
	{
		// default option is sell copy, determined to be safest
		combo_sale_type->setValue(LLSaleInfo::FS_COPY);
		combo_sale_type->setTentative(TRUE); // unfortunately this doesn't do anything at the moment.
	}

	childSetValue("checkbox for sale", num_for_sale != 0);

	// HACK: There are some old objects in world that are set for sale,
	// but are no-transfer.  We need to let users turn for-sale off, but only
	// if for-sale is set.
	bool cannot_actually_sell = !can_transfer || (!can_copy && sale_type == LLSaleInfo::FS_COPY);
	if (num_for_sale && has_change_sale_ability && cannot_actually_sell)
	{
		childSetEnabled("checkbox for sale", true);
	}
		
	// Check search status of objects
	BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );
	bool include_in_search;
	bool all_include_in_search = LLSelectMgr::getInstance()->selectionGetIncludeInSearch(&include_in_search);
	childSetEnabled("search_check", has_change_sale_ability && all_volume);
	childSetValue("search_check", include_in_search);
	childSetTentative("search_check", ! all_include_in_search);

	// Click action (touch, sit, buy)
	U8 click_action = 0;
	if (LLSelectMgr::getInstance()->selectionGetClickAction(&click_action))
	{
		LLComboBox*	ComboClickAction = getChild<LLComboBox>("clickaction");
		if(ComboClickAction)
		{
			ComboClickAction->setCurrentByIndex((S32)click_action);
		}
	}
	childSetEnabled("label click action",is_perm_modify && all_volume);
	childSetEnabled("clickaction",is_perm_modify && all_volume);

	if (!getIsEditing())
	{
		const std::string no_item_names[]={
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
		for(size_t t=0; t<LL_ARRAY_SIZE(no_item_names); ++t)
		{
			childSetEnabled(no_item_names[t],false);
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

	if(owners_identical && (owner_id == gAgent.getID()))
	{
		LLFloaterGroupPicker* fg = 	LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
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
	if(mLabelGroupName)
	{
		mLabelGroupName->setNameID(group_id, TRUE);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

static bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
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

void LLSidepanelTaskInfo::onClickDeedToGroup()
{
	LLNotifications::instance().add( "DeedObjectToGroup", LLSD(), LLSD(), callback_deed_to_group);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

void LLSidepanelTaskInfo::onCommitPerm(LLCheckBoxCtrl *ctrl, U8 field, U32 perm)
{
	LLViewerObject* object = mObjectSelection->getFirstRootObject();
	if(!object) return;

	BOOL new_state = ctrl->get();
	LLSelectMgr::getInstance()->selectionSetObjectPermissions(field, new_state, perm);
}

void LLSidepanelTaskInfo::onCommitGroupShare()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox share with group");
	onCommitPerm(ctrl, PERM_GROUP, PERM_MODIFY | PERM_MOVE | PERM_COPY);
}

void LLSidepanelTaskInfo::onCommitEveryoneMove()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox allow everyone move");
	onCommitPerm(ctrl, PERM_EVERYONE, PERM_MOVE);
}


void LLSidepanelTaskInfo::onCommitEveryoneCopy()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox allow everyone copy");
	onCommitPerm(ctrl, PERM_EVERYONE, PERM_COPY);
}

void LLSidepanelTaskInfo::onCommitNextOwnerModify()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox next owner can modify");
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_MODIFY);
}

void LLSidepanelTaskInfo::onCommitNextOwnerCopy()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox next owner can copy");
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_COPY);
}

void LLSidepanelTaskInfo::onCommitNextOwnerTransfer()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("checkbox next owner can transfer");
	onCommitPerm(ctrl, PERM_NEXT_OWNER, PERM_TRANSFER);
}

void LLSidepanelTaskInfo::onCommitName()
{
	LLLineEditor* tb = getChild<LLLineEditor>("Object Name");
	LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
}

void LLSidepanelTaskInfo::onCommitDesc()
{
	LLLineEditor* le = getChild<LLLineEditor>("Object Description");
	LLSelectMgr::getInstance()->selectionSetObjectDescription(le->getText());
}

void LLSidepanelTaskInfo::onCommitSaleInfo()
{
	setAllSaleInfo();
}

void LLSidepanelTaskInfo::onCommitSaleType()
{
	setAllSaleInfo();
}

void LLSidepanelTaskInfo::setAllSaleInfo()
{
	llinfos << "LLSidepanelTaskInfo::setAllSaleInfo()" << llendl;
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

	LLSaleInfo sale_info(sale_type, price);
	LLSelectMgr::getInstance()->selectionSetObjectSaleInfo(sale_info);
	
	// If turned off for-sale, make sure click-action buy is turned
	// off as well
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		U8 click_action = 0;
		LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
		if (click_action == CLICK_ACTION_BUY)
		{
			LLSelectMgr::getInstance()->selectionSetClickAction(CLICK_ACTION_TOUCH);
		}
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
void LLSidepanelTaskInfo::onCommitClickAction(U8 click_action)
{
	if (click_action == CLICK_ACTION_BUY)
	{
		LLSaleInfo sale_info;
		LLSelectMgr::getInstance()->selectGetSaleInfo(sale_info);
		if (!sale_info.isForSale())
		{
			LLNotifications::instance().add("CantSetBuyObject");

			// Set click action back to its old value
			U8 click_action = 0;
			LLSelectMgr::getInstance()->selectionGetClickAction(&click_action);
//			box->setCurrentByIndex((S32)click_action);

			return;
		}
	}
	else if (click_action == CLICK_ACTION_PAY)
	{
		// Verify object has script with money() handler
		LLSelectionPayable payable;
		bool can_pay = mObjectSelection->applyToObjects(&payable);
		if (!can_pay)
		{
			// Warn, but do it anyway.
			LLNotifications::instance().add("ClickActionNotPayable");
		}
	}
	LLSelectMgr::getInstance()->selectionSetClickAction(click_action);
}

// static
void LLSidepanelTaskInfo::onCommitIncludeInSearch()
{
	LLCheckBoxCtrl *ctrl = getChild<LLCheckBoxCtrl>("search_check");
	LLSelectMgr::getInstance()->selectionSetIncludeInSearch(ctrl->get());
}

// virtual
void LLSidepanelTaskInfo::updateVerbs()
{
	LLSidepanelInventorySubpanel::updateVerbs();

	mOpenBtn->setVisible(!getIsEditing());
	mPayBtn->setVisible(!getIsEditing());
	mBuyBtn->setVisible(!getIsEditing());

	mOpenBtn->setEnabled(enable_object_open());
	const LLViewerObject *obj = getFirstSelectedObject();
	mEditBtn->setEnabled(obj && obj->permModify());
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
	onCommitClickAction(CLICK_ACTION_PAY);
}

void LLSidepanelTaskInfo::onBuyButtonClicked()
{
	onCommitClickAction(CLICK_ACTION_BUY);
}

// virtual
void LLSidepanelTaskInfo::save()
{
	onCommitGroupShare();
	onCommitEveryoneMove();
	onCommitEveryoneCopy();
	onCommitNextOwnerModify();
	onCommitNextOwnerCopy();
	onCommitNextOwnerTransfer();
	onCommitName();
	onCommitDesc();
	onCommitSaleInfo();
	onCommitSaleType();
	onCommitIncludeInSearch();
}

void LLSidepanelTaskInfo::setObjectSelection(LLObjectSelectionHandle selection)
{
	mObjectSelection = selection;
}

LLSidepanelTaskInfo* LLSidepanelTaskInfo::getActivePanel()
{
	return sActivePanel;
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
