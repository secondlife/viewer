/** 
* @file llfloaterbigpreview.cpp
* @brief Display of extended (big) preview for snapshots and SL Share
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterbigpreview.h"
#include "llsnapshotlivepreview.h"

///////////////////////
//LLFloaterBigPreview//
///////////////////////

LLFloaterBigPreview::LLFloaterBigPreview(const LLSD& key) : LLFloater(key),
    mPreviewPlaceholder(NULL),
    mFloaterOwner(NULL)
{
}

LLFloaterBigPreview::~LLFloaterBigPreview()
{
    if (mPreviewHandle.get())
    {
        mPreviewHandle.get()->die();
    }
}

void LLFloaterBigPreview::onCancel()
{
    closeFloater();
}

void LLFloaterBigPreview::closeOnFloaterOwnerClosing(LLFloater* floaterp)
{
    if (isFloaterOwner(floaterp))
    {
        closeFloater();
    }
}

BOOL LLFloaterBigPreview::postBuild()
{
    mPreviewPlaceholder = getChild<LLUICtrl>("big_preview_placeholder");
    return LLFloater::postBuild();
}

void LLFloaterBigPreview::draw()
{
    LLFloater::draw();

    LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());
    
    // Display the preview if one is available
    if (previewp && previewp->getBigThumbnailImage())
    {
        // Get the preview rect
        const LLRect& preview_rect = mPreviewPlaceholder->getRect();
        
        // Get the preview texture size
        S32 thumbnail_w = previewp->getBigThumbnailWidth();
        S32 thumbnail_h = previewp->getBigThumbnailHeight();
        
        // Compute the scaling ratio and the size of the final texture in the rect: we want to prevent anisotropic scaling (distorted in x and y)
        F32 ratio = llmax((F32)(thumbnail_w)/(F32)(preview_rect.getWidth()), (F32)(thumbnail_h)/(F32)(preview_rect.getHeight()));
        thumbnail_w = (S32)((F32)(thumbnail_w)/ratio);
        thumbnail_h = (S32)((F32)(thumbnail_h)/ratio);
        
        // Compute the preview offset within the preview rect: we want to center that preview in the available rect
        const S32 local_offset_x = (preview_rect.getWidth()  - thumbnail_w) / 2 ;
        const S32 local_offset_y = (preview_rect.getHeight() - thumbnail_h) / 2 ;
        
        // Compute preview offset within the floater rect
        S32 offset_x = preview_rect.mLeft   + local_offset_x;
        S32 offset_y = preview_rect.mBottom + local_offset_y;
                
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        // Apply floater transparency to the texture unless the floater is focused.
        F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
        LLColor4 color = LLColor4::white;
        
        // Draw the preview texture
        gl_draw_scaled_image(offset_x, offset_y,
                             thumbnail_w, thumbnail_h,
                             previewp->getBigThumbnailImage(), color % alpha);
    }
}

