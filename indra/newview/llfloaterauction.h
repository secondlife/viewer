/** 
 * @file llfloaterauction.h
 * @author James Cook, Ian Wilkes
 * @brief llfloaterauction class header file
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

#ifndef LL_LLFLOATERAUCTION_H
#define LL_LLFLOATERAUCTION_H

#include "llfloater.h"
#include "lluuid.h"
#include "llpointer.h"
#include "llviewertexture.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterAuction
//
// Class which holds the functionality to start auctions.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLParcelSelection;
class LLParcel;
class LLViewerRegion;

class LLFloaterAuction : public LLFloater
{
    friend class LLFloaterReg;
public:
    // LLFloater interface
    /*virtual*/ void onOpen(const LLSD& key);
    /*virtual*/ void draw();

private:
    
    LLFloaterAuction(const LLSD& key);
    ~LLFloaterAuction();
    
    void initialize();

    static void onClickSnapshot(void* data);
    static void onClickResetParcel(void* data);
    static void onClickSellToAnyone(void* data);        // Sell to anyone clicked
    bool onSellToAnyoneConfirmed(const LLSD& notification, const LLSD& response);   // Sell confirmation clicked
    static void onClickStartAuction(void* data);

    /*virtual*/ BOOL postBuild();

    void doResetParcel();
    void doSellToAnyone();
    void clearParcelAccessList( LLParcel* parcel, LLViewerRegion* region, U32 list);
    void cleanupAndClose();

private:

    LLTransactionID mTransactionID;
    LLAssetID mImageID;
    LLPointer<LLViewerTexture> mImage;
    LLSafeHandle<LLParcelSelection> mParcelp;
    S32 mParcelID;
    LLHost mParcelHost;

    std::string mParcelUpdateCapUrl;    // "ParcelPropertiesUpdate" capability
};


#endif // LL_LLFLOATERAUCTION_H
