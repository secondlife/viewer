/** 
 * @file llvoinventorylistener.h
 * @brief Interface for classes that wish to receive updates about viewer object inventory
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

// Description of LLVOInventoryListener class, which is an interface
// for windows that are interested in updates to a ViewerObject's inventory.

#ifndef LL_LLVOINVENTORYLISTENER_H
#define LL_LLVOINVENTORYLISTENER_H

#include "llviewerobject.h"

class LLVOInventoryListener
{
public:
	virtual void inventoryChanged(LLViewerObject* object,
								 LLInventoryObject::object_list_t* inventory,
								 S32 serial_num,
								 void* user_data) = 0;

	// Remove the listener from the object and clear this listener
	void removeVOInventoryListener();

	// Just clear this listener, don't worry about the object.
	void clearVOInventoryListener();

protected:
	LLVOInventoryListener() : mListenerVObject(NULL) { }
	virtual ~LLVOInventoryListener() { removeVOInventoryListener(); }

	void registerVOInventoryListener(LLViewerObject* object, void* user_data);
	void requestVOInventory();

private:
	// LLViewerObject is normally wrapped by an LLPointer, but not in
	// this case, because it's already sure to be kept alive by
	// LLPointers held by other objects that have longer lifetimes
	// than this one.  Plumbing correct LLPointer usage all the way
	// down here has been deemed too much work for now.
	LLViewerObject *mListenerVObject;
};

#endif

