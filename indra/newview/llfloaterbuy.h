/** 
 * @file llfloaterbuy.h
 * @author James Cook
 * @brief LLFloaterBuy class definition
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
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */ 

#ifndef LL_LLFLOATERBUY_H
#define LL_LLFLOATERBUY_H

#include "llfloater.h"
#include "llvoinventorylistener.h"
#include "llsaleinfo.h"
#include "llinventory.h"

class LLViewerObject;
class LLSaleInfo;
class LLObjectSelection;

class LLFloaterBuy
: public LLFloater, public LLVOInventoryListener
{
public:
    LLFloaterBuy(const LLSD& key);
    ~LLFloaterBuy();
    
    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onClose(bool app_quitting);
    
    static void show(const LLSaleInfo& sale_info);

protected:
    void reset();

    void requestObjectInventories();
    /*virtual*/ void inventoryChanged(LLViewerObject* obj,
                                 LLInventoryObject::object_list_t* inv,
                                 S32 serial_num,
                                 void* data);

    void onSelectionChanged();
    void showViews(bool show);

    void onClickBuy();
    void onClickCancel();

private:
    LLSafeHandle<LLObjectSelection> mObjectSelection;
    LLSaleInfo mSaleInfo;

    boost::signals2::connection mSelectionUpdateSlot;
};

#endif
