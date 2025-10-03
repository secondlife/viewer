/**
 * @file llviewerjointattachment.h
 * @brief Implementation of LLViewerJointAttachment class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVIEWERJOINTATTACHMENT_H
#define LL_LLVIEWERJOINTATTACHMENT_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerjoint.h"
#include "llstring.h"
#include "lluuid.h"

class LLDrawable;
class LLViewerObject;

//-----------------------------------------------------------------------------
// class LLViewerJointAttachment
//-----------------------------------------------------------------------------
class LLViewerJointAttachment :
    public LLViewerJoint
{
public:
    LLViewerJointAttachment();
    virtual ~LLViewerJointAttachment();

    // Returns true if this object is transparent.
    // This is used to determine in which order to draw objects.
    bool isTransparent() override;

    // Draws the shape attached to a joint.
    // Called by render().
    U32 drawShape( F32 pixelArea, bool first_pass, bool is_dummy ) override;

    bool updateLOD(F32 pixel_area, bool activate) override;

    //
    // accessors
    //

    void setPieSlice(S32 pie_slice) { mPieSlice = pie_slice; }
    void setVisibleInFirstPerson(bool visibility) { mVisibleInFirst = visibility; }
    bool getVisibleInFirstPerson() const { return mVisibleInFirst; }
    void setGroup(S32 group) { mGroup = group; }
    void setOriginalPosition(LLVector3 &position);
    void setAttachmentVisibility(bool visible);
    void setIsHUDAttachment(bool is_hud) { mIsHUDAttachment = is_hud; }
    bool getIsHUDAttachment() const { return mIsHUDAttachment; }

    bool isAnimatable() const override { return false; }

    S32 getGroup() const { return mGroup; }
    S32 getPieSlice() const { return mPieSlice; }
    S32 getNumObjects() const { return static_cast<S32>(mAttachedObjects.size()); }
    S32 getNumAnimatedObjects() const;

    void clampObjectPosition();

    //
    // unique methods
    //
    bool addObject(LLViewerObject* object);
    void removeObject(LLViewerObject *object);

    //
    // attachments operations
    //
    bool isObjectAttached(const LLViewerObject *viewer_object) const;
    const LLViewerObject *getAttachedObject(const LLUUID &object_id) const;
    LLViewerObject *getAttachedObject(const LLUUID &object_id);

    // list of attachments for this joint
    typedef std::vector<LLPointer<LLViewerObject> > attachedobjs_vec_t;
    attachedobjs_vec_t mAttachedObjects;

protected:
    void calcLOD();
    void setupDrawable(LLViewerObject *object);

private:
    bool            mVisibleInFirst;
    LLVector3       mOriginalPos;
    S32             mGroup;
    bool            mIsHUDAttachment;
    S32             mPieSlice;
};

#endif // LL_LLVIEWERJOINTATTACHMENT_H
