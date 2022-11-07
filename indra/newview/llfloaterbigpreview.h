/** 
* @file   llfloaterbigpreview.h
* @brief  Display of extended (big) preview for snapshots
* @author merov@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_LLFLOATERBIGPREVIEW_H
#define LL_LLFLOATERBIGPREVIEW_H

#include "llfloater.h"

class LLFloaterBigPreview : public LLFloater
{
public:
    LLFloaterBigPreview(const LLSD& key);
    ~LLFloaterBigPreview();
    
    BOOL postBuild();
    void draw();
    void onCancel();

    void setPreview(LLView* previewp) { mPreviewHandle = previewp->getHandle(); }
    void setFloaterOwner(LLFloater* floaterp) { mFloaterOwner = floaterp; }
    bool isFloaterOwner(LLFloater* floaterp) const { return (mFloaterOwner == floaterp); }
    void closeOnFloaterOwnerClosing(LLFloater* floaterp);
    
private:
    LLHandle<LLView> mPreviewHandle;
    LLUICtrl*  mPreviewPlaceholder;
    LLFloater* mFloaterOwner;
};

#endif // LL_LLFLOATERBIGPREVIEW_H

