/** 
 * @file llfloateropenobject.h
 * @brief LLFloaterOpenObject class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

/*
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#ifndef LL_LLFLOATEROPENOBJECT_H
#define LL_LLFLOATEROPENOBJECT_H

#include "llfloater.h"

class LLObjectSelection;
class LLPanelObjectInventory;

class LLFloaterOpenObject
: public LLFloater
{
	friend class LLFloaterReg;
public:
	
	void dirty();
	
	class LLCategoryCreate
	{
		public:
			LLCategoryCreate(LLUUID object_id, bool wear) : mObjectID(object_id), mWear(wear) {}
		public:
			LLUUID mObjectID;
			bool mWear;
	};
	
	struct LLCatAndWear
	{
		LLUUID mCatID;
		bool mWear;
		bool mFolderResponded;
	};

protected:

	/*virtual*/	BOOL	postBuild();
	void refresh();
	void draw();
	virtual void onOpen(const LLSD& key);

	void moveToInventory(bool wear);

	void onClickMoveToInventory();
	void onClickMoveAndWear();
	static void callbackCreateInventoryCategory(const LLSD& result, void* data);
	static void callbackMoveInventory(S32 result, void* data);

private:
	
	LLFloaterOpenObject(const LLSD& key);
	~LLFloaterOpenObject();
	
protected:

	LLPanelObjectInventory*	mPanelInventoryObject;
	LLSafeHandle<LLObjectSelection> mObjectSelection;
	BOOL mDirty;
};

#endif
