/** 
 * @file llinventoryfunctions.h
 * @brief Miscellaneous inventory-related functions and classes
 * class definition
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

#ifndef LL_LLINVENTORYFUNCTIONS_H
#define LL_LLINVENTORYFUNCTIONS_H

#include "llassetstorage.h"
#include "lldarray.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventoryfilter.h"
#include "llfolderview.h"
#include "llinventorymodel.h"
#include "lluictrlfactory.h"
#include <set>


class LLFolderViewItem;
class LLInventoryFilter;
class LLInventoryModel;
class LLInventoryPanel;
class LLInvFVBridge;
class LLInventoryFVBridgeBuilder;
class LLMenuBarGL;
class LLCheckBoxCtrl;
class LLSpinCtrl;
class LLScrollContainer;
class LLTextBox;
class LLIconCtrl;
class LLSaveFolderState;
class LLFilterEditor;
class LLTabContainer;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// This is a collection of miscellaneous functions and classes
// that don't fit cleanly into any other class header.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryState
{
public:
	// HACK: Until we can route this info through the instant message hierarchy
	static BOOL sWearNewClothing;
	static LLUUID sWearNewClothingTransactionID;	// wear all clothing in this transaction	
};

class LLSelectFirstFilteredItem : public LLFolderViewFunctor
{
public:
	LLSelectFirstFilteredItem() : mItemSelected(FALSE) {}
	virtual ~LLSelectFirstFilteredItem() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);
	BOOL wasItemSelected() { return mItemSelected; }
protected:
	BOOL	mItemSelected;
};

class LLOpenFilteredFolders : public LLFolderViewFunctor
{
public:
	LLOpenFilteredFolders()  {}
	virtual ~LLOpenFilteredFolders() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);
};

class LLSaveFolderState : public LLFolderViewFunctor
{
public:
	LLSaveFolderState() : mApply(FALSE) {}
	virtual ~LLSaveFolderState() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item) {}
	void setApply(BOOL apply);
	void clearOpenFolders() { mOpenFolders.clear(); }
protected:
	std::set<LLUUID> mOpenFolders;
	BOOL mApply;
};

class LLOpenFoldersWithSelection : public LLFolderViewFunctor
{
public:
	LLOpenFoldersWithSelection() {}
	virtual ~LLOpenFoldersWithSelection() {}
	virtual void doFolder(LLFolderViewFolder* folder);
	virtual void doItem(LLFolderViewItem* item);
};

const std::string& get_item_icon_name(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point, 
							 BOOL item_is_multi );

LLUIImagePtr get_item_icon(LLAssetType::EType asset_type,
							 LLInventoryType::EType inventory_type,
							 U32 attachment_point, 
							 BOOL item_is_multi );

#endif // LL_LLINVENTORYFUNCTIONS_H



