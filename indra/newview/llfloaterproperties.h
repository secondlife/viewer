/** 
 * @file llfloaterproperties.h
 * @brief A floater which shows an inventory item's properties.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFLOATERPROPERTIES_H
#define LL_LLFLOATERPROPERTIES_H

#include <map>
#include "llfloater.h"
#include "lliconctrl.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterProperties
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLButton;
class LLCheckBoxCtrl;
class LLInventoryItem;
class LLLineEditor;
class LLRadioGroup;
class LLTextBox;

class LLFloaterProperties : public LLFloater
{
public:
	static LLFloaterProperties* find(const LLUUID& item_id,
									 const LLUUID& object_id);
	static LLFloaterProperties* show(const LLUUID& item_id,
									 const LLUUID& object_id);
	static void dirtyAll();

	static void closeByID(const LLUUID& item_id, const LLUUID& object_id);

	LLFloaterProperties(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_id, const LLUUID& object_id);
	virtual ~LLFloaterProperties();

	// do everything necessary
	void dirty() { mDirty = TRUE; }
	void refresh();

protected:
	// ui callbacks
	static void onClickCreator(void* data);
	static void onClickOwner(void* data);
	static void onCommitName(LLUICtrl* ctrl, void* data);
	static void onCommitDescription(LLUICtrl* ctrl, void* data);
	static void onCommitPermissions(LLUICtrl* ctrl, void* data);
	static void onCommitSaleInfo(LLUICtrl* ctrl, void* data);
	static void onCommitSaleType(LLUICtrl* ctrl, void* data);
	void updateSaleInfo();

	LLInventoryItem* findItem() const;

	void refreshFromItem(LLInventoryItem* item);
	virtual void draw();

protected:
	// The item id of the inventory item in question.
	LLUUID mItemID;

	// mObjectID will have a value if it is associated with a task in
	// the world, and will be == LLUUID::null if it's in the agent
	// inventory.
	LLUUID mObjectID;

	BOOL	mDirty;

	typedef std::map<LLUUID, LLFloaterProperties*, lluuid_less> instance_map;
	static instance_map sInstances;
};

class LLMultiProperties : public LLMultiFloater
{
public:
	LLMultiProperties(const LLRect& rect);
};

#endif // LL_LLFLOATERPROPERTIES_H
