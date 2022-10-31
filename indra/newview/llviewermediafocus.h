/** 
 * @file llpanelmsgs.h
 * @brief Message popup preferences panel
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_VIEWERMEDIAFOCUS_H
#define LL_VIEWERMEDIAFOCUS_H

// includes for LLViewerMediaFocus
#include "llfocusmgr.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"

class LLViewerMediaImpl;
class LLPanelPrimMediaControls;

class LLViewerMediaFocus : 
    public LLFocusableElement, 
    public LLSingleton<LLViewerMediaFocus>
{
    LLSINGLETON(LLViewerMediaFocus);
    ~LLViewerMediaFocus();

public:
    // Set/clear the face that has media focus (takes keyboard input and has the full set of controls)
    void setFocusFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
    void clearFocus();
    
    // Set/clear the face that has "media hover" (has the mimimal set of controls to zoom in or pop out into a media browser).
    // If a media face has focus, the media hover will be ignored.
    void setHoverFace(LLPointer<LLViewerObject> objectp, S32 face, viewer_media_t media_impl, LLVector3 pick_normal = LLVector3::zero);
    void clearHover();
    
    /*virtual*/ bool    getFocus();
    /*virtual*/ BOOL    handleKey(KEY key, MASK mask, BOOL called_from_parent);
    /*virtual*/ BOOL    handleKeyUp(KEY key, MASK mask, BOOL called_from_parent);
    /*virtual*/ BOOL    handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
    BOOL handleScrollWheel(const LLVector2& texture_coords, S32 clicks_x, S32 clicks_y);
    BOOL handleScrollWheel(S32 x, S32 y, S32 clicks_x, S32 clicks_y);

    void update();
    
    static LLVector3d setCameraZoom(LLViewerObject* object, LLVector3 normal, F32 padding_factor, bool zoom_in_only = false);
    static F32 getBBoxAspectRatio(const LLBBox& bbox, const LLVector3& normal, F32* height, F32* width, F32* depth);

    bool isFocusedOnFace(LLPointer<LLViewerObject> objectp, S32 face);
    bool isHoveringOverFace(LLPointer<LLViewerObject> objectp, S32 face);
    bool isHoveringOverFocused() { return mFocusedObjectID == mHoverObjectID && mFocusedObjectFace == mHoverObjectFace; };

    // These look up (by uuid) and return the values that were set with setFocusFace.  They will return null if the objects have been destroyed.
    LLViewerMediaImpl* getFocusedMediaImpl();
    LLViewerObject* getFocusedObject();
    S32 getFocusedFace() { return mFocusedObjectFace; }
    LLUUID getFocusedObjectID() { return mFocusedObjectID; }
    
    // These look up (by uuid) and return the values that were set with setHoverFace.  They will return null if the objects have been destroyed.
    LLViewerMediaImpl* getHoverMediaImpl();
    LLViewerObject* getHoverObject();
    S32 getHoverFace() { return mHoverObjectFace; }
    
    // Try to focus/zoom on the specified media (if it's on an object in world).
    void focusZoomOnMedia(LLUUID media_id);
    // Are we zoomed in?
    bool isZoomed() const;
    bool isZoomedOnMedia(LLUUID media_id);
    void unZoom();
    
    // Return the ID of the media instance the controls are currently attached to (either focus or hover).
    LLUUID getControlsMediaID();

    // The MoaP object wants keyup and keydown events.  Overridden to return true.
    virtual bool    wantsKeyUpKeyDown() const;
    virtual bool    wantsReturnKey() const;

protected:
    /*virtual*/ void    onFocusReceived();
    /*virtual*/ void    onFocusLost();

private:
    
    LLHandle<LLPanelPrimMediaControls> mMediaControls;
    LLObjectSelectionHandle mSelection;
    
    LLUUID mFocusedObjectID;
    S32 mFocusedObjectFace;
    LLUUID mFocusedImplID;
    LLUUID mPrevFocusedImplID;
    LLVector3 mFocusedObjectNormal;
    
    LLUUID mHoverObjectID;
    S32 mHoverObjectFace;
    LLUUID mHoverImplID;
    LLVector3 mHoverObjectNormal;
};


#endif // LL_VIEWERMEDIAFOCUS_H
