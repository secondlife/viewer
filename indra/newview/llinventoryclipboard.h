/** 
 * @file llinventoryclipboard.h
 * @brief LLInventoryClipboard class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	// this method gets the objects in the clipboard by copying them
	// into the array provided.
	void retrieve(LLDynamicArray<LLUUID>& inventory_objects) const;

	// this method empties out the clipboard
	void reset();

	// returns true if the clipboard has something pasteable in it.
	BOOL hasContents() const;

protected:
	static LLInventoryClipboard sInstance;

	LLDynamicArray<LLUUID> mObjects;

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
