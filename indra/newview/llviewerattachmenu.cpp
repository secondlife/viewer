/** 
 * @file llviewerattachmenu.cpp
 * @brief "Attach to" / "Attach to HUD" submenus.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "llviewerattachmenu.h"

// project includes
#include "llagent.h"
#include "llinventorybridge.h" // for rez_attachment()
#include "llinventorymodel.h"
#include "llviewerinventory.h"
#include "llviewermenu.h" // for gMenuHolder
#include "llvoavatarself.h"

// linden libraries
#include "llmenugl.h"
#include "lltrans.h"

// static
void LLViewerAttachMenu::populateMenus(const std::string& attach_to_menu_name, const std::string& attach_to_hud_menu_name)
{
	// *TODO: share this code with other similar menus
	// (inventory panel context menu, in-world object menu).

	if (attach_to_menu_name.empty() || attach_to_hud_menu_name.empty() || !isAgentAvatarValid()) return;

	LLContextMenu* attach_menu = gMenuHolder->getChild<LLContextMenu>(attach_to_menu_name);
	LLContextMenu* attach_hud_menu = gMenuHolder->getChild<LLContextMenu>(attach_to_hud_menu_name);

	if (!attach_menu || attach_menu->getChildCount() != 0 ||
		!attach_hud_menu || attach_hud_menu->getChildCount() != 0)
	{
		return;
	}

	// Populate "Attach to..." / "Attach to HUD..." submenus.
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		LLMenuItemCallGL::Params p;
		std::string submenu_name = attachment->getName();
		std::string translated_submenu_name;

		if (LLTrans::findString(translated_submenu_name, submenu_name))
		{
			p.name = (" ") + translated_submenu_name + " ";
		}
		else
		{
			p.name = submenu_name;
		}

		LLSD cbparams;
		cbparams["index"] = curiter->first;
		cbparams["label"] = attachment->getName();
		p.on_click.function_name = "Object.Attach";
		p.on_click.parameter = LLSD(attachment->getName());
		p.on_enable.function_name = "Attachment.Label";
		p.on_enable.parameter = cbparams;

		LLMenuItemCallGL* item = LLUICtrlFactory::create<LLMenuItemCallGL>(p);
		LLView* parent_menu = attachment->getIsHUDAttachment() ? attach_hud_menu : attach_menu;
		parent_menu->addChild(item);
	}
}

// static
void LLViewerAttachMenu::attachObjects(const uuid_vec_t& items, const std::string& joint_name)
{
	LLViewerJointAttachment* attachmentp = NULL;
	for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
		 iter != gAgentAvatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getName() == joint_name)
		{
			attachmentp = attachment;
			break;
		}
	}
	if (attachmentp == NULL)
	{
		return;
	}

	for (uuid_vec_t::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		const LLUUID &id = *it;
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getLinkedItem(id);
		if(item && gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID()))
		{
			rez_attachment(item, attachmentp);
		}
		else if(item && item->isFinished())
		{
			// must be in library. copy it to our inventory and put it on.
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(attachmentp);
			copy_inventory_item(gAgent.getID(),
								item->getPermissions().getOwner(),
								item->getUUID(),
								LLUUID::null,
								std::string(),
								cb);
		}
	}
}
