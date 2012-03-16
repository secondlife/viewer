/** 
 * @file llinventoryclipboard.h
 * @brief LLInventoryClipboard class header file
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

#ifndef LL_LLINVENTORYCLIPBOARD_H
#define LL_LLINVENTORYCLIPBOARD_H

#include "lldarray.h"
#include "lluuid.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryClipboard
//
// This class is used to cut/copy/paste inventory items around the
// world. This class is accessed through a singleton (only one
// inventory clipboard for now) which can be referenced using the
// instance() method.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLInventoryClipboard
{
public:
	// calling this before main() is undefined
	static LLInventoryClipboard& instance() { return sInstance; }

	// this method adds to the current list.
	void add(const LLUUID& object);

	// this stores a single inventory object
	void store(const LLUUID& object);

	// this method stores an array of objects
	void store(const LLDynamicArray<LLUUID>& inventory_objects);

	void cut(const LLUUID& object);
	// this method gets the objects in the clipboard by copying them
	// into the array provided.
	void retrieve(LLDynamicArray<LLUUID>& inventory_objects) const;

	// this method empties out the clipboard
	void reset();

	// returns true if the clipboard has something pasteable in it.
	BOOL hasContents() const;
	bool isCutMode() const { return mCutMode; }

protected:
	static LLInventoryClipboard sInstance;

	LLDynamicArray<LLUUID> mObjects;
	bool mCutMode;

public:
	// please don't actually call these
	LLInventoryClipboard();
	~LLInventoryClipboard();
private:
	// please don't implement these
	LLInventoryClipboard(const LLInventoryClipboard&);
	LLInventoryClipboard& operator=(const LLInventoryClipboard&);
};


#endif // LL_LLINVENTORYCLIPBOARD_H
