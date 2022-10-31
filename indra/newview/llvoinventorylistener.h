/** 
 * @file llvoinventorylistener.h
 * @brief Interface for classes that wish to receive updates about viewer object inventory
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// Description of LLVOInventoryListener class, which is an interface
// for windows that are interested in updates to a ViewerObject's inventory.

#ifndef LL_LLVOINVENTORYLISTENER_H
#define LL_LLVOINVENTORYLISTENER_H

#include "llinventory.h"

class LLViewerObject;

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

