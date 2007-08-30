/** 
 * @file llpanelpermissions.cpp
 * @brief LLPanelPermissions class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//*****************************************************************************
//
// This class represents the panel in the build view for
// viewing/editing object names, owners, permissions, etc.
//
//*****************************************************************************

#include "llviewerprecompiledheaders.h"

#include "llpanelpermissions.h"

#include "lluuid.h"
#include "llpermissions.h"
#include "llcategory.h"
#include "llclickaction.h"
#include "llfocusmgr.h"
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
#include "llradiogroup.h"
#include "llcombobox.h"
#include "llfloateravatarinfo.h"
#include "lluiconstants.h"
#include "lldbstrings.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroups.h"
#include "llnamebox.h"
#include "llviewercontrol.h"
#include "llvieweruictrlfactory.h"
#include "roles_constants.h"

///----------------------------------------------------------------------------
/// Class llpanelpermissions
///----------------------------------------------------------------------------

// Default constructor

BOOL	LLPanelPermissions::postBuild()
{
	this->childSetCommitCallback("Object Name",LLPanelPermissions::onCommitName,this);
	this->childSetPrevalidate("Object Name",LLLineEditor::prevalidatePrintableNotPipe);
	this->childSetCommitCallback("Object Description",LLPanelPermissions::onCommitDesc,this);
	this->childSetPrevalidate("Object Description",LLLineEditor::prevalidatePrintableNotPipe);

	
	this->childSetAction("button owner profile",LLPanelPermissions::onClickOwner,this);
	this->childSetAction("button creator profile",LLPanelPermissions::onClickCreator,this);

	this->childSetAction("button set group",LLPanelPermissions::onClickGroup,this);

	this->childSetCommitCallback("checkbox share with group",LLPanelPermissions::onCommitGroupShare,this);

	this->childSetAction("button deed",LLPanelPermissions::onClickDeedToGroup,this);

	this->childSetCommitCallback("checkbox allow everyone move",LLPanelPermissions::onCommitEveryoneMove,this);

	this->childSetCommitCallback("checkbox allow everyone copy",LLPanelPermissions::onCommitEveryoneCopy,this);
	
	this->childSetCommitCallback("checkbox for sale",LLPanelPermissions::onCommitSaleInfo,this);

	this->childSetCommitCallback("EdCost",LLPanelPermissions::onCommitSaleInfo,this);
	this->childSetPrevalidate("EdCost",LLLineEditor::prevalidateNonNegativeS32);

	this->childSetCommitCallback("sale type",LLPanelPermissions::onCommitSaleType,this);
	
	this->childSetCommitCallback("checkbox next owner can modify",LLPanelPermissions::onCommitNextOwnerModify,this);
	this->childSetCommitCallback("checkbox next owner can copy",LLPanelPermissions::onCommitNextOwnerCopy,this);
	this->childSetCommitCallback("checkbox next owner can transfer",LLPanelPermissions::onCommitNextOwnerTransfer,this);
	this->childSetCommitCallback("clickaction",LLPanelPermissions::onCommitClickAction,this);
	this->childSetCommitCallback("search_check",LLPanelPermissions::onCommitIncludeInSearch,this);
	

	LLTextBox*	LabelGroupNameRectProxy = gUICtrlFactory->getTextBoxByName(this,"Group Name Proxy");
	if(LabelGroupNameRectProxy )
	{
		mLabelGroupName = new LLNameBox("Group Name",LabelGroupNameRectProxy->getRect());
		addChild(mLabelGroupName);
	}else
		mLabelGroupName = NULL;

	return TRUE;
}

LLPanelPermissions::LLPanelPermissions(const std::string& title) :
	LLPanel(title)
{
	setMouseOpaque(FALSE);
}

// Destroys the object
LLPanelPermissions::~LLPanelPermissions()
{
	// base class will take care of everything
}


void LLPanelPermissions::refresh()
{
	
	LLButton*	BtnDeedToGroup = gUICtrlFactory->getButtonByName(this,"button deed");
	if(BtnDeedToGroup)
	{	
		LLString deedText;
		if (gSavedSettings.getWarning("DeedObject"))
		{
			deedText = this->childGetText("text deed continued");
		}
		else
		{
			deedText = this->childGetText("text deed");
		}
		BtnDeedToGroup->setLabelSelected(deedText);
		BtnDeedToGroup->setLabelUnselected(deedText);
	}
	BOOL root_selected = TRUE;
	LLSelectNode* nodep = gSelectMgr->getSelection()->getFirstRootNode();
	S32 object_count = gSelectMgr->getSelection()->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = gSelectMgr->getSelection()->getFirstNode();
		object_count = gSelectMgr->getSelection()->getObjectCount();
		root_selected = FALSE;
	}

	//BOOL attachment_selected = gSelectMgr->getSelection()->isAttachment();
	//attachment_selected = false;
	LLViewerObject* objectp = NULL;
	if(nodep) objectp = nodep->getObject();
	if(!nodep || !objectp)// || attachment_selected)
	{
		// ...nothing selected
		childSetEnabled("perm_modify",false);
		childSetText("perm_modify","");

		childSetEnabled("Creator:",false);
		childSetText("Creator Name","");
		childSetEnabled("Creator Name",false);
		childSetEnabled("button creator profile",false);

		childSetEnabled("Owner:",false);
		childSetText("Owner Name","");
		childSetEnabled("Owner Name",false);
		childSetEnabled("button owner profile",false);

		childSetEnabled("Group:",false);
		childSetText("Group Name","");
		childSetEnabled("Group Name",false);
		childSetEnabled("button set group",false);

		childSetText("Object Name","");
		childSetEnabled("Object Name",false);
		childSetEnabled("Name:",false);
		childSetText("Group Name","");
		childSetEnabled("Group Name",false);
		childSetEnabled("Description:",false);
		childSetText("Object Description","");
		childSetEnabled("Object Description",false);
 
		childSetText("prim info","");
		childSetEnabled("prim info",false);

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
		
		LLRadioGroup*	RadioSaleType = gUICtrlFactory->getRadioGroupByName(this,"sale type");
		if(RadioSaleType)
		{
			RadioSaleType->setSelectedIndex(-1);
			RadioSaleType->setEnabled(FALSE);
		}
		
		childSetEnabled("Price:  L$",false);
		childSetText("EdCost",false);
		childSetEnabled("EdCost",false);
		
		childSetEnabled("label click action",false);
		LLComboBox*	ComboClickAction = gUICtrlFactory->getComboBoxByName(this,"clickaction");
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
	BOOL is_perm_modify = (gSelectMgr->getSelection()->getFirstRootNode() 
							&& gSelectMgr->selectGetRootsModify()) 
							|| gSelectMgr->selectGetModify();
	const LLView* keyboard_focus_view = gFocusMgr.getKeyboardFocus();
	S32 string_index = 0;
	LLString MODIFY_INFO_STRINGS[] =
	{
		childGetText("text modify info 1"),
		childGetText("text modify info 2"),
		childGetText("text modify info 3"),
		childGetText("text modify info 4")
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
	LLString creator_name;
	creators_identical = gSelectMgr->selectGetCreator(mCreatorID,
													  creator_name);

	childSetText("Creator Name",creator_name);
	childSetEnabled("Creator Name",TRUE);
	childSetEnabled("button creator profile", creators_identical && mCreatorID.notNull() );

	// Update owner text field
	childSetEnabled("Owner:",true);

	BOOL owners_identical;
	LLString owner_name;
	owners_identical = gSelectMgr->selectGetOwner(mOwnerID, owner_name);

//	llinfos << "owners_identical " << (owners_identical ? "TRUE": "FALSE") << llendl;

	if (mOwnerID.isNull())
	{
		if(gSelectMgr->selectIsGroupOwned())
		{
			// Group owned already displayed by selectGetOwner
		}
		else
		{
			// Display last owner if public
			LLString last_owner_name;
			gSelectMgr->selectGetLastOwner(mLastOwnerID, last_owner_name);

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
	childSetEnabled("button owner profile",owners_identical && (mOwnerID.notNull() || gSelectMgr->selectIsGroupOwned()));

	// update group text field
	childSetEnabled("Group:",true);
	LLUUID group_id;
	BOOL groups_identical = gSelectMgr->selectGetGroup(group_id);
	if (groups_identical)
	{
		if(mLabelGroupName)
		{
			mLabelGroupName->setNameID(group_id, TRUE);
			mLabelGroupName->setEnabled(TRUE);
		}
	}
	childSetEnabled("button set group",owners_identical && (mOwnerID == gAgent.getID()));

	// figure out the contents of the name, description, & category
	BOOL edit_name_desc = FALSE;
	if(is_one_object && objectp->permModify())
	{
		edit_name_desc = TRUE;
	}
	if(is_one_object)
	{
		childSetEnabled("Name:",true);
		LLLineEditor* LineEditorObjectName = gUICtrlFactory->getLineEditorByName(this,"Object Name");
		if(keyboard_focus_view != LineEditorObjectName)
		{
			childSetText("Object Name",nodep->mName);
		}

		childSetEnabled("Description:",true);
		LLLineEditor*	LineEditorObjectDesc = gUICtrlFactory->getLineEditorByName(this,"Object Description");
		if(LineEditorObjectDesc)
		{
			if(keyboard_focus_view != LineEditorObjectDesc)
			{
				LineEditorObjectDesc->setText(nodep->mDescription);
			}
		}
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


	// Pre-compute object info string
	S32 prim_count = gSelectMgr->getSelection()->getObjectCount();
	S32 obj_count = gSelectMgr->getSelection()->getRootObjectCount();

	LLString object_info_string;
	if (1 == obj_count)
	{
		object_info_string.assign("1 Object, ");
	}
	else
	{
		char buffer[MAX_STRING];		/*Flawfinder: ignore*/
		snprintf(buffer, MAX_STRING, "%d Objects, ", obj_count);			/* Flawfinder: ignore */
		object_info_string.assign(buffer);
	}
	if (1 == prim_count)
	{
		object_info_string.append("1 Primitive");
	}
	else
	{
		char buffer[MAX_STRING];		/*Flawfinder: ignore*/
		snprintf(buffer, MAX_STRING, "%d Primitives", prim_count);			/* Flawfinder: ignore */
		object_info_string.append(buffer);
	}
	childSetText("prim info",object_info_string);
	childSetEnabled("prim info",true);

	S32 price;
	BOOL is_for_sale = gSelectMgr->selectIsForSale(price);
	if (!is_for_sale)
	{
		price = DEFAULT_PRICE;
	}

	BOOL self_owned = (gAgent.getID() == mOwnerID);
	BOOL group_owned = gSelectMgr->selectIsGroupOwned() ;
	BOOL public_owned = (mOwnerID.isNull() && !gSelectMgr->selectIsGroupOwned());
	BOOL can_transfer = gSelectMgr->selectGetRootsTransfer();
	BOOL can_copy = gSelectMgr->selectGetRootsCopy();

	if(!owners_identical)
	{
		childSetEnabled("Price:  L$",false);
		childSetText("EdCost","");
		childSetEnabled("EdCost",false);
	}
	else if(self_owned || (group_owned && gAgent.hasPowerInGroup(group_id,GP_OBJECT_SET_SALE)))
	{
		LLLineEditor*	EditPrice = gUICtrlFactory->getLineEditorByName(this,"EdCost");
		if(keyboard_focus_view != EditPrice)
		{
			childSetText("EdCost",llformat("%d",price));
		}
		if(is_for_sale && is_one_object && can_transfer)
		{
			childSetEnabled("Price:  L$",true);
			childSetEnabled("EdCost",true);
		}
		else
		{
			childSetEnabled("Price:  L$",false);
			childSetEnabled("EdCost",false);
		}
	}
	else if(!public_owned)
	{
		// ...someone, not you, owns it
		childSetEnabled("Price:  L$",false);
		childSetText("EdCost",llformat("%d",price));
		childSetEnabled("EdCost",false);
	}
	else
	{
		// ...public object
		childSetEnabled("Price:  L$",false);
		childSetText("EdCost","");
		childSetEnabled("EdCost",false);
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

	valid_base_perms = gSelectMgr->selectGetPerm(PERM_BASE,
									  &base_mask_on,
									  &base_mask_off);

	valid_owner_perms = gSelectMgr->selectGetPerm(PERM_OWNER,
									  &owner_mask_on,
									  &owner_mask_off);

	valid_group_perms = gSelectMgr->selectGetPerm(PERM_GROUP,
									  &group_mask_on,
									  &group_mask_off);
	
	valid_everyone_perms = gSelectMgr->selectGetPerm(PERM_EVERYONE,
									  &everyone_mask_on,
									  &everyone_mask_off);
	
	valid_next_perms = gSelectMgr->selectGetPerm(PERM_NEXT_OWNER,
									  &next_owner_mask_on,
									  &next_owner_mask_off);

	
	if( gSavedSettings.getBOOL("DebugPermissions") )
	{
		char perm_string[10];		/*Flawfinder: ignore*/
		if (valid_base_perms)
		{

			strcpy(perm_string, "B: ");	/*Flawfinder: ignore*/
			mask_to_string(base_mask_on, perm_string+3);
			childSetText("B:",perm_string);
			childSetVisible("B:",true);
			
			strcpy(perm_string, "O: ");	/*Flawfinder: ignore*/
			mask_to_string(owner_mask_on, perm_string+3);
			childSetText("O:",perm_string);
			childSetVisible("O:",true);
			
			strcpy(perm_string, "G: ");	/*Flawfinder: ignore*/
			mask_to_string(group_mask_on, perm_string+3);
			childSetText("G:",perm_string);
			childSetVisible("G:",true);
			
			strcpy(perm_string, "E: ");	/*Flawfinder: ignore*/
			mask_to_string(everyone_mask_on, perm_string+3);
			childSetText("E:",perm_string);
			childSetVisible("E:",true);
			
			strcpy(perm_string, "N: ");	/*Flawfinder: ignore*/
			mask_to_string(next_owner_mask_on, perm_string+3);
			childSetText("N:",perm_string);
			childSetVisible("N:",true);
		}
		strcpy(perm_string, "F: ");	/*Flawfinder: ignore*/
		U32 flag_mask = 0x0;
		if (objectp->permMove())
			flag_mask |= PERM_MOVE;
		if (objectp->permModify())
			flag_mask |= PERM_MODIFY;
		if (objectp->permCopy())
			flag_mask |= PERM_COPY;
		if (objectp->permTransfer())
			flag_mask |= PERM_TRANSFER;
		mask_to_string(flag_mask, perm_string+3);
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
		// XUI:translate
		// ...must select root to choose permissions
		childSetValue("perm_modify", "Must select entire object to set permissions.");
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
		childSetEnabled("checkbox for sale", can_transfer || (!can_transfer && is_for_sale));
		childSetEnabled("sale type",is_for_sale && can_transfer);

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
			childSetEnabled("button deed",gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned);
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
			childSetEnabled("button deed",gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned);
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
	BOOL valid_sale_info = gSelectMgr->selectGetSaleInfo(sale_info);
	LLSaleInfo::EForSale sale_type = sale_info.getSaleType();

	LLRadioGroup* RadioSaleType = gUICtrlFactory->getRadioGroupByName(this,"sale type");
	if(RadioSaleType)
	{
		if (valid_sale_info)
		{
			RadioSaleType->setSelectedIndex((S32)sale_type - 1);
		}
		else
		{
			// default option is sell copy, determined to be safest
			RadioSaleType->setSelectedIndex((S32)LLSaleInfo::FS_COPY - 1);
		}
	}

	if (is_for_sale)
	{
		childSetValue("checkbox for sale",TRUE);
		childSetTentative("checkbox for sale",!can_transfer || (!can_copy && sale_type == LLSaleInfo::FS_COPY));
	}
	else
	{
		childSetValue("checkbox for sale",FALSE);
		childSetTentative("checkbox for sale",false);
	}

	// Check search status of objects
	BOOL all_volume = gSelectMgr->selectionAllPCode( LL_PCODE_VOLUME );
	bool include_in_search;
	bool all_include_in_search = gSelectMgr->selectionGetIncludeInSearch(&include_in_search);
	childSetEnabled("search_check", is_perm_modify && all_volume);
	childSetValue("search_check", include_in_search);
	childSetTentative("search_check", ! all_include_in_search);

	// Click action (touch, sit, buy)
	U8 click_action = 0;
	if (gSelectMgr->selectionGetClickAction(&click_action))
	{
		LLComboBox*	ComboClickAction = gUICtrlFactory->getComboBoxByName(this,"clickaction");
		if(ComboClickAction)
		{
			ComboClickAction->setCurrentByIndex((S32)click_action);
		}
	}
	childSetEnabled("label click action",is_perm_modify && all_volume);
	childSetEnabled("clickaction",is_perm_modify && all_volume);
}


// static
void LLPanelPermissions::onClickClaim(void*)
{
	// try to claim ownership
	gSelectMgr->sendOwner(gAgent.getID(), gAgent.getGroupID());
}

// static
void LLPanelPermissions::onClickRelease(void*)
{
	// try to release ownership
	gSelectMgr->sendOwner(LLUUID::null, LLUUID::null);
}

// static
void LLPanelPermissions::onClickCreator(void *data)
{
	LLPanelPermissions *self = (LLPanelPermissions *)data;

	LLFloaterAvatarInfo::showFromObject(self->mCreatorID);
}

// static
void LLPanelPermissions::onClickOwner(void *data)
{
	LLPanelPermissions *self = (LLPanelPermissions *)data;

	if (gSelectMgr->selectIsGroupOwned())
	{
		LLUUID group_id;
		gSelectMgr->selectGetGroup(group_id);
		LLFloaterGroupInfo::showFromUUID(group_id);
	}
	else
	{
		LLFloaterAvatarInfo::showFromObject(self->mOwnerID);
	}
}

void LLPanelPermissions::onClickGroup(void* data)
{
	LLPanelPermissions* panelp = (LLPanelPermissions*)data;
	LLUUID owner_id;
	LLString name;
	BOOL owners_identical = gSelectMgr->selectGetOwner(owner_id, name);
	LLFloater* parent_floater = gFloaterView->getParentFloater(panelp);

	if(owners_identical && (owner_id == gAgent.getID()))
	{
		LLFloaterGroupPicker* fg;
		fg = LLFloaterGroupPicker::showInstance(LLSD(gAgent.getID()));
		fg->setSelectCallback( cbGroupID, data );

		if (parent_floater)
		{
			LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, fg);
			fg->setOrigin(new_rect.mLeft, new_rect.mBottom);
			parent_floater->addDependentFloater(fg);
		}
	}
}

// static
void LLPanelPermissions::cbGroupID(LLUUID group_id, void* userdata)
{
	LLPanelPermissions* self = (LLPanelPermissions*)userdata;
	if(self->mLabelGroupName)
	{
		self->mLabelGroupName->setNameID(group_id, TRUE);
	}
	gSelectMgr->sendGroup(group_id);
}

void callback_deed_to_group(S32 option, void*)
{
	if (0 == option)
	{
		LLUUID group_id;
		BOOL groups_identical = gSelectMgr->selectGetGroup(group_id);
		if(groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			gSelectMgr->sendOwner(LLUUID::null, group_id, FALSE);
//			gViewerStats->incStat(LLViewerStats::ST_RELEASE_COUNT);
		}
	}
}

void LLPanelPermissions::onClickDeedToGroup(void* data)
{
			gViewerWindow->alertXml( "DeedObjectToGroup",
			callback_deed_to_group, NULL);
}

///----------------------------------------------------------------------------
/// Permissions checkboxes
///----------------------------------------------------------------------------

// static
void LLPanelPermissions::onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm)
{
	LLViewerObject* object = gSelectMgr->getSelection()->getFirstRootObject();
	if(!object) return;

	// Checkbox will have toggled itself
	// LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	BOOL new_state = check->get();
	
	gSelectMgr->selectionSetObjectPermissions(field, new_state, perm);
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
	LLLineEditor*	tb = gUICtrlFactory->getLineEditorByName(self,"Object Name");
	if(tb)
	{
		gSelectMgr->selectionSetObjectName(tb->getText());
//		gSelectMgr->selectionSetObjectName(self->mLabelObjectName->getText());
	}
}


// static
void LLPanelPermissions::onCommitDesc(LLUICtrl*, void* data)
{
	//llinfos << "LLPanelPermissions::onCommitDesc()" << llendl;
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	le = gUICtrlFactory->getLineEditorByName(self,"Object Description");
	if(le)
	{
		gSelectMgr->selectionSetObjectDescription(le->getText());
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

	LLCheckBoxCtrl*	mCheckPurchase = gUICtrlFactory->getCheckBoxByName(this,"checkbox for sale");

	if(mCheckPurchase && mCheckPurchase->get())
	{
		LLRadioGroup* RadioSaleType = gUICtrlFactory->getRadioGroupByName(this,"sale type");
		if(RadioSaleType)
		{
			switch(RadioSaleType->getSelectedIndex())
			{
			case 0:
				sale_type = LLSaleInfo::FS_ORIGINAL;
				break;
			case 1:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			case 2:
				sale_type = LLSaleInfo::FS_CONTENTS;
				break;
			default:
				sale_type = LLSaleInfo::FS_COPY;
				break;
			}
		}
	}
	LLLineEditor*	mEditPrice = gUICtrlFactory->getLineEditorByName(this,"EdCost");

	S32 price = -1;
	if(mEditPrice)
	{
		price = atoi(mEditPrice->getText().c_str());
	}
	// Invalid data - turn off the sale
	if (price < 0)
	{
		sale_type = LLSaleInfo::FS_NOT;
		price = 0;
	}

	LLSaleInfo sale_info(sale_type, price);
	gSelectMgr->selectionSetObjectSaleInfo(sale_info);

	// If turned off for-sale, make sure click-action buy is turned
	// off as well
	if (sale_type == LLSaleInfo::FS_NOT)
	{
		U8 click_action = 0;
		gSelectMgr->selectionGetClickAction(&click_action);
		if (click_action == CLICK_ACTION_BUY)
		{
			gSelectMgr->selectionSetClickAction(CLICK_ACTION_TOUCH);
		}
	}
}

class LLSelectionPayable : public LLSelectedObjectFunctor
{
public:
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

	U8 click_action = (U8)box->getCurrentIndex();
	if (click_action == CLICK_ACTION_BUY)
	{
		LLSaleInfo sale_info;
		gSelectMgr->selectGetSaleInfo(sale_info);
		if (!sale_info.isForSale())
		{
			gViewerWindow->alertXml("CantSetBuyObject");

			// Set click action back to its old value
			U8 click_action = 0;
			gSelectMgr->selectionGetClickAction(&click_action);
			box->setCurrentByIndex((S32)click_action);

			return;
		}
	}
	else if (click_action == CLICK_ACTION_PAY)
	{
		// Verify object has script with money() handler
		LLSelectionPayable payable;
		bool can_pay = gSelectMgr->getSelection()->applyToObjects(&payable);
		if (!can_pay)
		{
			// Warn, but do it anyway.
			gViewerWindow->alertXml("ClickActionNotPayable");
		}
	}
	gSelectMgr->selectionSetClickAction(click_action);
}

// static
void LLPanelPermissions::onCommitIncludeInSearch(LLUICtrl* ctrl, void*)
{
	LLCheckBoxCtrl* box = (LLCheckBoxCtrl*)ctrl;
	llassert(box);

	gSelectMgr->selectionSetIncludeInSearch(box->get());
}

