/** 
 * @file llvoinventorylistener.cpp
 * @brief Interface for classes that wish to receive updates about viewer object inventory
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvoinventorylistener.h"
#include "llviewerobject.h"

void LLVOInventoryListener::removeVOInventoryListener()
{
	if (mListenerVObject != NULL)
	{
		mListenerVObject->removeInventoryListener(this);
		mListenerVObject = NULL;
	}
}

void LLVOInventoryListener::registerVOInventoryListener(LLViewerObject* object, void* user_data)
{
	removeVOInventoryListener();
	if (object != NULL)
	{
		mListenerVObject = object;
		object->registerInventoryListener(this,user_data);
	}
}

void LLVOInventoryListener::requestVOInventory()
{
	if (mListenerVObject != NULL)
	{
		mListenerVObject->requestInventory();
	}
}

// This assumes mListenerVObject is clearing it's own lists
void LLVOInventoryListener::clearVOInventoryListener()
{
	mListenerVObject = NULL;
}
