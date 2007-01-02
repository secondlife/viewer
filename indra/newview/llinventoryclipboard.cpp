/** 
 * @file llinventoryclipboard.cpp
 * @brief LLInventoryClipboard class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llinventoryclipboard.h"


LLInventoryClipboard LLInventoryClipboard::sInstance;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------


///----------------------------------------------------------------------------
/// Class LLInventoryClipboard
///----------------------------------------------------------------------------

LLInventoryClipboard::LLInventoryClipboard()
{
}

LLInventoryClipboard::~LLInventoryClipboard()
{
	reset();
}

void LLInventoryClipboard::add(const LLUUID& object)
{
	mObjects.put(object);
}

// this stores a single inventory object
void LLInventoryClipboard::store(const LLUUID& object)
{
	reset();
	mObjects.put(object);
}

void LLInventoryClipboard::store(const LLDynamicArray<LLUUID>& inv_objects)
{
	reset();
	S32 count = inv_objects.count();
	for(S32 i = 0; i < count; i++)
	{
		mObjects.put(inv_objects[i]);
	}
}

void LLInventoryClipboard::retrieve(LLDynamicArray<LLUUID>& inv_objects) const
{
	inv_objects.reset();
	S32 count = mObjects.count();
	for(S32 i = 0; i < count; i++)
	{
		inv_objects.put(mObjects[i]);
	}
}

void LLInventoryClipboard::reset()
{
	mObjects.reset();
}

// returns true if the clipboard has something pasteable in it.
BOOL LLInventoryClipboard::hasContents() const
{
	return (mObjects.count() > 0);
}


///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
