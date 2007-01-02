/** 
 * @file llfloaterproperties.h
 * @brief A floater which shows an inventory item's properties.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
