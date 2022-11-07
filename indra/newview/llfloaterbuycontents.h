/** 
 * @file llfloaterbuycontents.h
 * @author James Cook
 * @brief LLFloaterBuyContents class header file
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

/**
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#ifndef LL_LLFLOATERBUYCONTENTS_H
#define LL_LLFLOATERBUYCONTENTS_H

#include "llfloater.h"
#include "llvoinventorylistener.h"
#include "llinventory.h"

class LLViewerObject;
class LLObjectSelection;

class LLFloaterBuyContents
: public LLFloater, public LLVOInventoryListener
{
public:
    static void show(const LLSaleInfo& sale_info);

    LLFloaterBuyContents(const LLSD& key);
    ~LLFloaterBuyContents();
    /*virtual*/ BOOL    postBuild();
    
protected:
    void requestObjectInventories();
    /*virtual*/ void inventoryChanged(LLViewerObject* obj,
                                 LLInventoryObject::object_list_t* inv,
                                 S32 serial_num,
                                 void* data);
    
    void onClickBuy();
    void onClickCancel();

protected:
    LLSafeHandle<LLObjectSelection> mObjectSelection;
    LLSaleInfo mSaleInfo;
};

#endif
