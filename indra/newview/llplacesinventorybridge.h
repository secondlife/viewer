/** 
 * @file llplacesinventorybridge.h
 * @brief Declaration of the Inventory-Folder-View-Bridge classes for Places Panel.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPLACESINVENTORYBRIDGE_H
#define LL_LLPLACESINVENTORYBRIDGE_H

#include "llinventorybridge.h"

class LLFolderViewFolder;

/**
 * Overridden version of the Inventory-Folder-View-Bridge for Places Panel (Landmarks Tab)
 */
class LLPlacesLandmarkBridge : public LLLandmarkBridge
{
	friend class LLPlacesInventoryBridgeBuilder;

public:
	/*virtual*/ void buildContextMenu(LLMenuGL& menu, U32 flags);

protected:
	LLPlacesLandmarkBridge(LLInventoryType::EType type, 
						   LLInventoryPanel* inventory,
						   LLFolderView* root,
						   const LLUUID& uuid, 
						   U32 flags = 0x00) :
		LLLandmarkBridge(inventory, root, uuid, flags)
	{
		mInvType = type;
	}
};

/**
 * Overridden version of the Inventory-Folder-View-Bridge for Folders
 */
class LLPlacesFolderBridge : public LLFolderBridge
{
	friend class LLPlacesInventoryBridgeBuilder;

public:
	/*virtual*/ void buildContextMenu(LLMenuGL& menu, U32 flags);
	/*virtual*/ void performAction(LLInventoryModel* model, std::string action);

protected:
	LLPlacesFolderBridge(LLInventoryType::EType type, 
						 LLInventoryPanel* inventory,
						 LLFolderView* root,						 
						 const LLUUID& uuid) :
		LLFolderBridge(inventory, root, uuid)
	{
		mInvType = type;
	}
	LLFolderViewFolder* getFolder();
};


/**
 * This class intended to override default InventoryBridgeBuilder for Inventory Panel.
 *
 * It builds Bridges for Landmarks and Folders in Places Landmarks Panel
 */
class LLPlacesInventoryBridgeBuilder : public LLInventoryFVBridgeBuilder
{
public:
	/*virtual*/ LLInvFVBridge* createBridge(LLAssetType::EType asset_type,
											LLAssetType::EType actual_asset_type,
											LLInventoryType::EType inv_type,
											LLInventoryPanel* inventory,
											LLFolderViewModelInventory* view_model,
											LLFolderView* root,
											const LLUUID& uuid,
											U32 flags = 0x00) const;
};

#endif // LL_LLPLACESINVENTORYBRIDGE_H
