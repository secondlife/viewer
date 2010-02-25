/** 
 * @file llfloaterinventory.cpp
 * @brief Implementation of the inventory view and associated stuff.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include <utility> // for std::pair<>

#include "llinventoryfunctions.h"

// library includes
#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llfloaterreg.h"
#include "llsdserialize.h"
#include "llfiltereditor.h"
#include "llspinctrl.h"
#include "llui.h"
#include "message.h"

// newview includes
#include "llappearancemgr.h"
#include "llappviewer.h"
//#include "llfirstuse.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lluictrlfactory.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

BOOL LLInventoryState::sWearNewClothing = FALSE;
LLUUID LLInventoryState::sWearNewClothingTransactionID;

void LLSaveFolderState::setApply(BOOL apply)
{
	mApply = apply; 
	// before generating new list of open folders, clear the old one
	if(!apply) 
	{
		clearOpenFolders(); 
	}
}

void LLSaveFolderState::doFolder(LLFolderViewFolder* folder)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_DO_FOLDER);
	if(mApply)
	{
		// we're applying the open state
		LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
		if(!bridge) return;
		LLUUID id(bridge->getUUID());
		if(mOpenFolders.find(id) != mOpenFolders.end())
		{
			folder->setOpen(TRUE);
		}
		else
		{
			// keep selected filter in its current state, this is less jarring to user
			if (!folder->isSelected())
			{
				folder->setOpen(FALSE);
			}
		}
	}
	else
	{
		// we're recording state at this point
		if(folder->isOpen())
		{
			LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
			if(!bridge) return;
			mOpenFolders.insert(bridge->getUUID());
		}
	}
}

void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && folder->getParentFolder())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
	// if this folder didn't pass the filter, and none of its descendants did
	else if (!folder->getFiltered() && !folder->hasFilteredDescendants())
	{
		folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_NO);
	}
}

void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered() && !mItemSelected)
	{
		item->getRoot()->setSelection(item, FALSE, FALSE);
		if (item->getParentFolder())
		{
			item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		item->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}

void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && !mItemSelected)
	{
		folder->getRoot()->setSelection(folder, FALSE, FALSE);
		if (folder->getParentFolder())
		{
			folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		folder->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}

void LLOpenFoldersWithSelection::doItem(LLFolderViewItem *item)
{
	if (item->getParentFolder() && item->isSelected())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFoldersWithSelection::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getParentFolder() && folder->isSelected())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

const std::string& get_item_icon_name(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point,
							 BOOL item_is_multi )
{
	EInventoryIcon idx = OBJECT_ICON_NAME;
	if ( item_is_multi )
	{
		idx = OBJECT_MULTI_ICON_NAME;
	}
	
	switch(asset_type)
	{
	case LLAssetType::AT_TEXTURE:
		if(LLInventoryType::IT_SNAPSHOT == inventory_type)
		{
			idx = SNAPSHOT_ICON_NAME;
		}
		else
		{
			idx = TEXTURE_ICON_NAME;
		}
		break;

	case LLAssetType::AT_SOUND:
		idx = SOUND_ICON_NAME;
		break;
	case LLAssetType::AT_CALLINGCARD:
		if(attachment_point!= 0)
		{
			idx = CALLINGCARD_ONLINE_ICON_NAME;
		}
		else
		{
			idx = CALLINGCARD_OFFLINE_ICON_NAME;
		}
		break;
	case LLAssetType::AT_LANDMARK:
		if(attachment_point!= 0)
		{
			idx = LANDMARK_VISITED_ICON_NAME;
		}
		else
		{
			idx = LANDMARK_ICON_NAME;
		}
		break;
	case LLAssetType::AT_SCRIPT:
	case LLAssetType::AT_LSL_TEXT:
	case LLAssetType::AT_LSL_BYTECODE:
		idx = SCRIPT_ICON_NAME;
		break;
	case LLAssetType::AT_CLOTHING:
		idx = CLOTHING_ICON_NAME;
	case LLAssetType::AT_BODYPART :
		if(LLAssetType::AT_BODYPART == asset_type)
		{
			idx = BODYPART_ICON_NAME;
		}
		switch(LLInventoryItem::II_FLAGS_WEARABLES_MASK & attachment_point)
		{
		case WT_SHAPE:
			idx = BODYPART_SHAPE_ICON_NAME;
			break;
		case WT_SKIN:
			idx = BODYPART_SKIN_ICON_NAME;
			break;
		case WT_HAIR:
			idx = BODYPART_HAIR_ICON_NAME;
			break;
		case WT_EYES:
			idx = BODYPART_EYES_ICON_NAME;
			break;
		case WT_SHIRT:
			idx = CLOTHING_SHIRT_ICON_NAME;
			break;
		case WT_PANTS:
			idx = CLOTHING_PANTS_ICON_NAME;
			break;
		case WT_SHOES:
			idx = CLOTHING_SHOES_ICON_NAME;
			break;
		case WT_SOCKS:
			idx = CLOTHING_SOCKS_ICON_NAME;
			break;
		case WT_JACKET:
			idx = CLOTHING_JACKET_ICON_NAME;
			break;
		case WT_GLOVES:
			idx = CLOTHING_GLOVES_ICON_NAME;
			break;
		case WT_UNDERSHIRT:
			idx = CLOTHING_UNDERSHIRT_ICON_NAME;
			break;
		case WT_UNDERPANTS:
			idx = CLOTHING_UNDERPANTS_ICON_NAME;
			break;
		case WT_SKIRT:
			idx = CLOTHING_SKIRT_ICON_NAME;
			break;
		case WT_ALPHA:
			idx = CLOTHING_ALPHA_ICON_NAME;
			break;
		case WT_TATTOO:
			idx = CLOTHING_TATTOO_ICON_NAME;
			break;
		default:
			// no-op, go with choice above
			break;
		}
		break;
	case LLAssetType::AT_NOTECARD:
		idx = NOTECARD_ICON_NAME;
		break;
	case LLAssetType::AT_ANIMATION:
		idx = ANIMATION_ICON_NAME;
		break;
	case LLAssetType::AT_GESTURE:
		idx = GESTURE_ICON_NAME;
		break;
	case LLAssetType::AT_LINK:
		idx = LINKITEM_ICON_NAME;
		break;
	case LLAssetType::AT_LINK_FOLDER:
		idx = LINKFOLDER_ICON_NAME;
		break;
	default:
		break;
	}
	
	return ICON_NAME[idx];
}

LLUIImagePtr get_item_icon(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point,
							 BOOL item_is_multi)
{
	const std::string& icon_name = get_item_icon_name(asset_type, inventory_type, attachment_point, item_is_multi );
	return LLUI::getUIImage(icon_name);
}

BOOL get_is_item_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
		return FALSE;
	
	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			const LLVOAvatarSelf* my_avatar = gAgent.getAvatarObject();
			if(my_avatar && my_avatar->isWearingAttachment(item->getLinkedUUID()))
				return TRUE;
			break;
		}
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
				return TRUE;
			break;
		case LLAssetType::AT_GESTURE:
			if (LLGestureManager::instance().isGestureActive(item->getLinkedUUID()))
				return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}
