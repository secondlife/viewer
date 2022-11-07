/** 
 * @file llfloatermyscripts.h
 * @brief LLFloaterMyScripts class definition.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLFLOATERMYSCRIPTS_H
#define LL_LLFLOATERMYSCRIPTS_H

#include "llfloater.h"
#include "llpanel.h"

class LLFloaterMyScripts : public LLFloater
{
public:
    LLFloaterMyScripts(const LLSD& seed);

    BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);
    void setAttachmentDetails(LLSD content);
    void setAttachmentSummary(LLSD content);
    bool requestAttachmentDetails();
    void clearList();

private:
    void getAttachmentLimitsCoro(std::string url);

    bool mGotAttachmentMemoryUsed;
    bool mAttachmentDetailsRequested;
    S32 mAttachmentMemoryMax;
    S32 mAttachmentMemoryUsed;

    bool mGotAttachmentURLsUsed;
    S32 mAttachmentURLsMax;
    S32 mAttachmentURLsUsed;

protected:
    
    static void onClickRefresh(void* userdata);
};

#endif
