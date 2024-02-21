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
#include "llavatariconctrl.h"
#include "llnamebox.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llspinctrl.h"
#include "roles_constants.h"
#include "llgroupactions.h"
#include "llgroupiconctrl.h"
#include "lltrans.h"
#include "llinventorymodel.h"

#include "llavatarnamecache.h"
#include "llcachename.h"


U8 string_value_to_click_action(std::string p_value);
std::string click_action_to_string_value( U8 action);

U8 string_value_to_click_action(std::string p_value)
{
	if (p_value == "Touch")
	{
		return CLICK_ACTION_TOUCH;
	}
	if (p_value == "Sit")
	{
		return CLICK_ACTION_SIT;
	}
	if (p_value == "Buy")
	{
		return CLICK_ACTION_BUY;
	}
	if (p_value == "Pay")
	{
		return CLICK_ACTION_PAY;
	}
	if (p_value == "Open")
	{
		return CLICK_ACTION_OPEN;
	}
	if (p_value == "Zoom")
	{
		return CLICK_ACTION_ZOOM;
	}
	if (p_value == "Ignore")
	{
		return CLICK_ACTION_IGNORE;
	}
	if (p_value == "None")
	{
		return CLICK_ACTION_DISABLED;
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
		case CLICK_ACTION_IGNORE:
			return "Ignore";
			break;
		case CLICK_ACTION_DISABLED:
			return "None";
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
	setMouseOpaque(false);
}

bool LLPanelPermissions::postBuild()
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
	mLabelOwnerName = getChild<LLTextBox>("Owner Name");
	mLabelCreatorName = getChild<LLTextBox>("Creator Name");

	return true;
}


LLPanelPermissions::~LLPanelPermissions()
{
	if (mOwnerCacheConnection.connected())
	{
		mOwnerCacheConnection.disconnect();
	}
	if (mCreatorCacheConnection.connected())
	{
		mCreatorCacheConnection.disconnect();
	}
	// base class will take care of everything
}


void LLPanelPermissions::disableAll()
{
	getChildView("perm_modify")->setEnabled(false);
	getChild<LLUICtrl>("perm_modify")->setValue(LLStringUtil::null);

	getChildView("pathfinding_attributes_value")->setEnabled(false);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLStringUtil::null);

	getChildView("Creator:")->setEnabled(false);
	getChild<LLUICtrl>("Creator Icon")->setVisible(false);
	mLabelCreatorName->setValue(LLStringUtil::null);
	mLabelCreatorName->setEnabled(false);

	getChildView("Owner:")->setEnabled(false);
	getChild<LLUICtrl>("Owner Icon")->setVisible(false);
	getChild<LLUICtrl>("Owner Group Icon")->setVisible(false);
	mLabelOwnerName->setValue(LLStringUtil::null);
	mLabelOwnerName->setEnabled(false);

	getChildView("Group:")->setEnabled(false);
	getChild<LLUICtrl>("Group Name Proxy")->setValue(LLStringUtil::null);
	getChildView("Group Name Proxy")->setEnabled(false);
	getChildView("button set group")->setEnabled(false);

	getChild<LLUICtrl>("Object Name")->setValue(LLStringUtil::null);
	getChildView("Object Name")->setEnabled(false);
	getChildView("Name:")->setEnabled(false);
	getChild<LLUICtrl>("Group Name")->setValue(LLStringUtil::null);
	getChildView("Group Name")->setEnabled(false);
	getChildView("Description:")->setEnabled(false);
	getChild<LLUICtrl>("Object Description")->setValue(LLStringUtil::null);
	getChildView("Object Description")->setEnabled(false);
		
	getChild<LLUICtrl>("checkbox share with group")->setValue(false);
	getChildView("checkbox share with group")->setEnabled(false);
	getChildView("button deed")->setEnabled(false);

	getChild<LLUICtrl>("checkbox allow everyone move")->setValue(false);
	getChildView("checkbox allow everyone move")->setEnabled(false);
	getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(false);
	getChildView("checkbox allow everyone copy")->setEnabled(false);

	//Next owner can:
	getChildView("Next owner can:")->setEnabled(false);
	getChild<LLUICtrl>("checkbox next owner can modify")->setValue(false);
	getChildView("checkbox next owner can modify")->setEnabled(false);
	getChild<LLUICtrl>("checkbox next owner can copy")->setValue(false);
	getChildView("checkbox next owner can copy")->setEnabled(false);
	getChild<LLUICtrl>("checkbox next owner can transfer")->setValue(false);
	getChildView("checkbox next owner can transfer")->setEnabled(false);

	//checkbox for sale
	getChild<LLUICtrl>("checkbox for sale")->setValue(false);
	getChildView("checkbox for sale")->setEnabled(false);

	//checkbox include in search
	getChild<LLUICtrl>("search_check")->setValue(false);
	getChildView("search_check")->setEnabled(false);
		
	LLComboBox*	combo_sale_type = getChild<LLComboBox>("sale type");
	combo_sale_type->setValue(LLSaleInfo::FS_COPY);
	combo_sale_type->setEnabled(false);
		
	getChildView("Cost")->setEnabled(false);
	getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
	getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
	getChildView("Edit Cost")->setEnabled(false);
		
	getChildView("label click action")->setEnabled(false);
	LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
	if (combo_click_action)
	{
		combo_click_action->setEnabled(false);
		combo_click_action->clear();
	}
	getChildView("B:")->setVisible(false);
	getChildView("O:")->setVisible(false);
	getChildView("G:")->setVisible(false);
	getChildView("E:")->setVisible(false);
	getChildView("N:")->setVisible(false);
	getChildView("F:")->setVisible(false);
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
	bool root_selected = true;
	LLSelectNode* nodep = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
	S32 object_count = LLSelectMgr::getInstance()->getSelection()->getRootObjectCount();
	if(!nodep || 0 == object_count)
	{
		nodep = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
		object_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		root_selected = false;
	}

	//bool attachment_selected = LLSelectMgr::getInstance()->getSelection()->isAttachment();
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
	const bool is_one_object = (object_count == 1);
	
	// BUG: fails if a root and non-root are both single-selected.
	bool is_perm_modify = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
						   && LLSelectMgr::getInstance()->selectGetRootsModify())
		|| LLSelectMgr::getInstance()->selectGetModify();
	bool is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode() 
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
	getChildView("perm_modify")->setEnabled(true);
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

	getChildView("pathfinding_attributes_value")->setEnabled(true);
	getChild<LLUICtrl>("pathfinding_attributes_value")->setValue(LLTrans::getString(pfAttrName));
	
	// Update creator text field
	getChildView("Creator:")->setEnabled(true);
	std::string creator_app_link;
	LLSelectMgr::getInstance()->selectGetCreator(mCreatorID, creator_app_link);

	// Style for creator and owner links (both group and agent)
	LLStyle::Params style_params;
	LLColor4 link_color = LLUIColorTable::instance().getColor("HTMLLinkColor");
	style_params.color = link_color;
	style_params.readonly_color = link_color;
	style_params.is_link = true; // link will be added later
	const LLFontGL* fontp = mLabelCreatorName->getFont();
	style_params.font.name = LLFontGL::nameFromFont(fontp);
	style_params.font.size = LLFontGL::sizeFromFont(fontp);
	style_params.font.style = "UNDERLINE";

	LLAvatarName av_name;
	style_params.link_href = creator_app_link;
	if (LLAvatarNameCache::get(mCreatorID, &av_name))
	{
		updateCreatorName(mCreatorID, av_name, style_params);
	}
	else
	{
		if (mCreatorCacheConnection.connected())
		{
			mCreatorCacheConnection.disconnect();
		}
		mLabelCreatorName->setText(LLTrans::getString("None"));
		mCreatorCacheConnection = LLAvatarNameCache::get(mCreatorID, boost::bind(&LLPanelPermissions::updateCreatorName, this, _1, _2, style_params));
	}
	getChild<LLAvatarIconCtrl>("Creator Icon")->setValue(mCreatorID);
	getChild<LLAvatarIconCtrl>("Creator Icon")->setVisible(true);
	mLabelCreatorName->setEnabled(true);

	// Update owner text field
	getChildView("Owner:")->setEnabled(true);

	std::string owner_app_link;
	const bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(mOwnerID, owner_app_link);


	if (LLSelectMgr::getInstance()->selectIsGroupOwned())
	{
		// Group owned already displayed by selectGetOwner
		LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mOwnerID);
		if (group_data && group_data->isGroupPropertiesDataComplete())
		{
			style_params.link_href = owner_app_link;
			mLabelOwnerName->setText(group_data->mName, style_params);
			getChild<LLGroupIconCtrl>("Owner Group Icon")->setIconId(group_data->mInsigniaID);
			getChild<LLGroupIconCtrl>("Owner Group Icon")->setVisible(true);
			getChild<LLUICtrl>("Owner Icon")->setVisible(false);
		}
		else
		{
			// Triggers refresh
			LLGroupMgr::getInstance()->sendGroupPropertiesRequest(mOwnerID);
		}
	}
	else
	{
		LLUUID owner_id = mOwnerID;
		if (owner_id.isNull())
		{
			// Display last owner if public
			std::string last_owner_app_link;
			LLSelectMgr::getInstance()->selectGetLastOwner(mLastOwnerID, last_owner_app_link);

			// It should never happen that the last owner is null and the owner
			// is null, but it seems to be a bug in the simulator right now. JC
			if (!mLastOwnerID.isNull() && !last_owner_app_link.empty())
			{
				owner_app_link.append(", last ");
				owner_app_link.append(last_owner_app_link);
			}
			owner_id = mLastOwnerID;
		}

		style_params.link_href = owner_app_link;
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
			mOwnerCacheConnection = LLAvatarNameCache::get(owner_id, boost::bind(&LLPanelPermissions::updateOwnerName, this, _1, _2, style_params));
		}

		getChild<LLAvatarIconCtrl>("Owner Icon")->setValue(owner_id);
		getChild<LLAvatarIconCtrl>("Owner Icon")->setVisible(true);
		getChild<LLUICtrl>("Owner Group Icon")->setVisible(false);
	}
	mLabelOwnerName->setEnabled(true);

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
	
	getChildView("button set group")->setEnabled(root_selected && owners_identical && (mOwnerID == gAgent.getID()) && is_nonpermanent_enforced);

	getChildView("Name:")->setEnabled(true);
	LLLineEditor* LineEditorObjectName = getChild<LLLineEditor>("Object Name");
	getChildView("Description:")->setEnabled(true);
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
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Total"));
		else
			getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
	}
	// This is a public object.
	else
	{
		getChildView("Cost")->setEnabled(false);
		getChild<LLUICtrl>("Cost")->setValue(getString("Cost Default"));
		
		getChild<LLUICtrl>("Edit Cost")->setValue(LLStringUtil::null);
		getChildView("Edit Cost")->setEnabled(false);
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

	bool valid_base_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_BASE,
																			&base_mask_on,
																			&base_mask_off);
	//bool valid_owner_perms =//
	LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
											  &owner_mask_on,
											  &owner_mask_off);
	bool valid_group_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
																			&group_mask_on,
																			&group_mask_off);
	
	bool valid_everyone_perms 	= LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
																			&everyone_mask_on,
																			&everyone_mask_off);
	
	bool valid_next_perms 		= LLSelectMgr::getInstance()->selectGetPerm(PERM_NEXT_OWNER,
																			&next_owner_mask_on,
																			&next_owner_mask_off);


	if (gSavedSettings.getBOOL("DebugPermissions") )
	{
		if (valid_base_perms)
		{
			getChild<LLUICtrl>("B:")->setValue("B: " + mask_to_string(base_mask_on));
			getChildView("B:")->setVisible(true);
			getChild<LLUICtrl>("O:")->setValue("O: " + mask_to_string(owner_mask_on));
			getChildView("O:")->setVisible(true);
			getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(group_mask_on));
			getChildView("G:")->setVisible(true);
			getChild<LLUICtrl>("E:")->setValue("E: " + mask_to_string(everyone_mask_on));
			getChildView("E:")->setVisible(true);
			getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(next_owner_mask_on));
			getChildView("N:")->setVisible(true);
		}
		else if(!root_selected)
		{
			if(object_count == 1)
			{
				LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
				if (node && node->mValid)
				{
					getChild<LLUICtrl>("B:")->setValue("B: " + mask_to_string( node->mPermissions->getMaskBase()));
					getChildView("B:")->setVisible(true);
					getChild<LLUICtrl>("O:")->setValue("O: " + mask_to_string(node->mPermissions->getMaskOwner()));
					getChildView("O:")->setVisible(true);
					getChild<LLUICtrl>("G:")->setValue("G: " + mask_to_string(node->mPermissions->getMaskGroup()));
					getChildView("G:")->setVisible(true);
					getChild<LLUICtrl>("E:")->setValue("E: " + mask_to_string(node->mPermissions->getMaskEveryone()));
					getChildView("E:")->setVisible(true);
					getChild<LLUICtrl>("N:")->setValue("N: " + mask_to_string(node->mPermissions->getMaskNextOwner()));
					getChildView("N:")->setVisible(true);
				}
			}
		}
		else
		{
		    getChildView("B:")->setVisible(false);
		    getChildView("O:")->setVisible(false);
		    getChildView("G:")->setVisible(false);
		    getChildView("E:")->setVisible(false);
		    getChildView("N:")->setVisible(false);
		}

		U32 flag_mask = 0x0;
		if (objectp->permMove()) 		flag_mask |= PERM_MOVE;
		if (objectp->permModify()) 		flag_mask |= PERM_MODIFY;
		if (objectp->permCopy()) 		flag_mask |= PERM_COPY;
		if (objectp->permTransfer()) 	flag_mask |= PERM_TRANSFER;

		getChild<LLUICtrl>("F:")->setValue("F:" + mask_to_string(flag_mask));
		getChildView("F:")->setVisible(								true);
	}
	else
	{
		getChildView("B:")->setVisible(								false);
		getChildView("O:")->setVisible(								false);
		getChildView("G:")->setVisible(								false);
		getChildView("E:")->setVisible(								false);
		getChildView("N:")->setVisible(								false);
		getChildView("F:")->setVisible(								false);
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
		getChild<LLUICtrl>("checkbox for sale")->setTentative( 				is_for_sale_mixed);
		getChildView("sale type")->setEnabled(num_for_sale && can_transfer && !is_sale_price_mixed);

		getChildView("Next owner can:")->setEnabled(true);
		getChildView("checkbox next owner can modify")->setEnabled(base_mask_on & PERM_MODIFY);
		getChildView("checkbox next owner can copy")->setEnabled(base_mask_on & PERM_COPY);
		getChildView("checkbox next owner can transfer")->setEnabled(next_owner_mask_on & PERM_COPY);
	}
	else 
	{
		getChildView("checkbox for sale")->setEnabled(false);
		getChildView("sale type")->setEnabled(false);

		getChildView("Next owner can:")->setEnabled(false);
		getChildView("checkbox next owner can modify")->setEnabled(false);
		getChildView("checkbox next owner can copy")->setEnabled(false);
		getChildView("checkbox next owner can transfer")->setEnabled(false);
	}

	if (valid_group_perms)
	{
		if ((group_mask_on & PERM_COPY) && (group_mask_on & PERM_MODIFY) && (group_mask_on & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(true);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	false);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
		else if ((group_mask_off & PERM_COPY) && (group_mask_off & PERM_MODIFY) && (group_mask_off & PERM_MOVE))
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(false);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(	false);
			getChildView("button deed")->setEnabled(false);
		}
		else
		{
			getChild<LLUICtrl>("checkbox share with group")->setValue(true);
			getChild<LLUICtrl>("checkbox share with group")->setTentative(!has_change_perm_ability);
			getChildView("button deed")->setEnabled(gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) && (group_mask_on & PERM_MOVE) && (owner_mask_on & PERM_TRANSFER) && !group_owned && can_transfer);
		}
	}			

	if (valid_everyone_perms)
	{
		// Move
		if (everyone_mask_on & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(true);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	false);
		}
		else if (everyone_mask_off & PERM_MOVE)
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(false);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	false);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone move")->setValue(true);
			getChild<LLUICtrl>("checkbox allow everyone move")->setTentative( 	true);
		}

		// Copy == everyone can't copy
		if (everyone_mask_on & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(true);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative( 	!can_copy || !can_transfer);
		}
		else if (everyone_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(false);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	false);
		}
		else
		{
			getChild<LLUICtrl>("checkbox allow everyone copy")->setValue(true);
			getChild<LLUICtrl>("checkbox allow everyone copy")->setTentative(	true);
		}
	}

	if (valid_next_perms)
	{
		// Modify == next owner canot modify
		if (next_owner_mask_on & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(true);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	false);
		}
		else if (next_owner_mask_off & PERM_MODIFY)
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(false);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	false);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can modify")->setValue(true);
			getChild<LLUICtrl>("checkbox next owner can modify")->setTentative(	true);
		}

		// Copy == next owner cannot copy
		if (next_owner_mask_on & PERM_COPY)
		{			
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(true);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	!can_copy);
		}
		else if (next_owner_mask_off & PERM_COPY)
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(false);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	false);
		}
		else
		{
			getChild<LLUICtrl>("checkbox next owner can copy")->setValue(true);
			getChild<LLUICtrl>("checkbox next owner can copy")->setTentative(	true);
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
		combo_sale_type->setValue(					sale_type == LLSaleInfo::FS_NOT ? LLSaleInfo::FS_COPY : sale_type);
		combo_sale_type->setTentative(				false); // unfortunately this doesn't do anything at the moment.
	}
	else
	{
		// default option is sell copy, determined to be safest
		combo_sale_type->setValue(					LLSaleInfo::FS_COPY);
		combo_sale_type->setTentative(				true); // unfortunately this doesn't do anything at the moment.
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
	getChild<LLUICtrl>("search_check")->setTentative( 				!all_include_in_search);

	// Click action (touch, sit, buy, pay, open, play, open media, zoom, ignore)
	U8 click_action = 0;
	if (LLSelectMgr::getInstance()->selectionGetClickAction(&click_action))
	{
		LLComboBox*	combo_click_action = getChild<LLComboBox>("clickaction");
		if (combo_click_action)
		{
			const std::string combo_value = click_action_to_string_value(click_action);
			combo_click_action->setValue(LLSD(combo_value));
		}
	}

	if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
	{
		getChildView("checkbox for sale")->setEnabled(false);
		getChildView("Edit Cost")->setEnabled(false);
		getChild<LLComboBox>("sale type")->setEnabled(false);
	}

	getChildView("label click action")->setEnabled(is_perm_modify && is_nonpermanent_enforced  && all_volume);
	getChildView("clickaction")->setEnabled(is_perm_modify && is_nonpermanent_enforced && all_volume);
}

// Shorten name if it doesn't fit into max_pixels of two lines
void shorten_name(std::string &name, const LLStyle::Params& style_params, S32 max_pixels)
{
    const LLFontGL* font = style_params.font();

    LLWString wline = utf8str_to_wstring(name);
    // panel supports two lines long names
    S32 segment_length = font->maxDrawableChars(wline.c_str(), max_pixels, wline.length(), LLFontGL::WORD_BOUNDARY_IF_POSSIBLE);
    if (segment_length == wline.length())
    {
        // no work needed
        return;
    }

    S32 first_line_length = segment_length;
    segment_length = font->maxDrawableChars(wline.substr(first_line_length).c_str(), max_pixels, wline.length(), LLFontGL::ANYWHERE);
    if (segment_length + first_line_length == wline.length())
    {
        // no work needed
        return;
    }

    // name does not fit, cut it, add ...
    const LLWString dots_pad(utf8str_to_wstring(std::string("....")));
    S32 elipses_width = font->getWidthF32(dots_pad.c_str());
    segment_length = font->maxDrawableChars(wline.substr(first_line_length).c_str(), max_pixels - elipses_width, wline.length(), LLFontGL::ANYWHERE);

    name = name.substr(0, segment_length + first_line_length) + std::string("...");
}

void LLPanelPermissions::updateOwnerName(const LLUUID& owner_id, const LLAvatarName& owner_name, const LLStyle::Params& style_params)
{
	if (mOwnerCacheConnection.connected())
	{
		mOwnerCacheConnection.disconnect();
	}
	std::string name = owner_name.getCompleteName();
	shorten_name(name, style_params, mLabelOwnerName->getLocalRect().getWidth());
	mLabelOwnerName->setText(name, style_params);
}

void LLPanelPermissions::updateCreatorName(const LLUUID& creator_id, const LLAvatarName& creator_name, const LLStyle::Params& style_params)
{
	if (mCreatorCacheConnection.connected())
	{
		mCreatorCacheConnection.disconnect();
	}
	std::string name = creator_name.getCompleteName();
	shorten_name(name, style_params, mLabelCreatorName->getLocalRect().getWidth());
	mLabelCreatorName->setText(name, style_params);
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
	bool owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, name);
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
		mLabelGroupName->setNameID(group_id, true);
	}
	LLSelectMgr::getInstance()->sendGroup(group_id);
}

bool callback_deed_to_group(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLUUID group_id;
		bool groups_identical = LLSelectMgr::getInstance()->selectGetGroup(group_id);
		if(group_id.notNull() && groups_identical && (gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED)))
		{
			LLSelectMgr::getInstance()->sendOwner(LLUUID::null, group_id, false);
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
	bool new_state = check->get();
	
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
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerModify" << LL_ENDL;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_MODIFY);
}

// static
void LLPanelPermissions::onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data)
{
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerCopy" << LL_ENDL;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_COPY);
}

// static
void LLPanelPermissions::onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data)
{
	//LL_INFOS() << "LLPanelPermissions::onCommitNextOwnerTransfer" << LL_ENDL;
	onCommitPerm(ctrl, data, PERM_NEXT_OWNER, PERM_TRANSFER);
}

// static
void LLPanelPermissions::onCommitName(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	tb = self->getChild<LLLineEditor>("Object Name");
	if (!tb)
	{
		return;
	}
	LLSelectMgr::getInstance()->selectionSetObjectName(tb->getText());
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->isAttachment() && (selection->getNumNodes() == 1) && !tb->getText().empty())
	{
		LLUUID object_id = selection->getFirstObject()->getAttachmentItemID();
		LLViewerInventoryItem* item = findItem(object_id);
		if (item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			new_item->rename(tb->getText());
			new_item->updateServer(false);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
	}
}


// static
void LLPanelPermissions::onCommitDesc(LLUICtrl*, void* data)
{
	LLPanelPermissions* self = (LLPanelPermissions*)data;
	LLLineEditor*	le = self->getChild<LLLineEditor>("Object Description");
	if (!le)
	{
		return;
	}
	LLSelectMgr::getInstance()->selectionSetObjectDescription(le->getText());
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->isAttachment() && (selection->getNumNodes() == 1))
	{
		LLUUID object_id = selection->getFirstObject()->getAttachmentItemID();
		LLViewerInventoryItem* item = findItem(object_id);
		if (item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			new_item->setDescription(le->getText());
			new_item->updateServer(false);
			gInventory.updateItem(new_item);
			gInventory.notifyObservers();
		}
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
	LL_INFOS() << "LLPanelPermissions::setAllSaleInfo()" << LL_ENDL;
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

    // Note: won't work right if a root and non-root are both single-selected (here and other places).
    bool is_perm_modify = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
                           && LLSelectMgr::getInstance()->selectGetRootsModify())
                          || LLSelectMgr::getInstance()->selectGetModify();
    bool is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
                                     && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
                                    || LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();

    if (is_perm_modify && is_nonpermanent_enforced)
    {
        struct f : public LLSelectedObjectFunctor
        {
            virtual bool apply(LLViewerObject* object)
            {
                return object->getClickAction() == CLICK_ACTION_BUY
                    || object->getClickAction() == CLICK_ACTION_TOUCH;
            }
        } check_actions;

        // Selection should only contain objects that are of target
        // action already or of action we are aiming to remove.
        bool default_actions = LLSelectMgr::getInstance()->getSelection()->applyToObjects(&check_actions);

        if (default_actions && old_sale_info.isForSale() != new_sale_info.isForSale())
        {
            U8 new_click_action = new_sale_info.isForSale() ? CLICK_ACTION_BUY : CLICK_ACTION_TOUCH;
            LLSelectMgr::getInstance()->selectionSetClickAction(new_click_action);
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


LLViewerInventoryItem* LLPanelPermissions::findItem(LLUUID &object_id)
{
	if (!object_id.isNull())
	{
		return gInventory.getItem(object_id);
	}
	return NULL;
}
