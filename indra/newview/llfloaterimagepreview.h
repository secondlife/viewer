/** 
 * @file llfloaterimagepreview.h
 * @brief LLFloaterImagePreview class definition
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

#ifndef LL_LLFLOATERIMAGEPREVIEW_H
#define LL_LLFLOATERIMAGEPREVIEW_H

#include "llfloaternamedesc.h"
#include "lldynamictexture.h"
#include "llpointer.h"
#include "llquaternion.h"

class LLComboBox;
class LLJoint;
class LLViewerJointMesh;
class LLVOAvatar;
class LLTextBox;
class LLVertexBuffer;
class LLVolume;

class LLImagePreviewSculpted : public LLViewerDynamicTexture
{
protected:
    virtual ~LLImagePreviewSculpted();

 public:
    LLImagePreviewSculpted(S32 width, S32 height);  

    /*virtual*/ S8 getType() const ;

    void setPreviewTarget(LLImageRaw *imagep, F32 distance);
    void setTexture(U32 name) { mTextureName = name; }

    BOOL render();
    void refresh();
    void rotate(F32 yaw_radians, F32 pitch_radians);
    void zoom(F32 zoom_amt);
    void pan(F32 right, F32 up);
    virtual BOOL needsRender() { return mNeedsUpdate; }

 protected:
    BOOL        mNeedsUpdate;
    U32         mTextureName;
    F32         mCameraDistance;
    F32         mCameraYaw;
    F32         mCameraPitch;
    F32         mCameraZoom;
    LLVector3   mCameraOffset;
    LLPointer<LLVolume> mVolume;
    LLPointer<LLVertexBuffer> mVertexBuffer;
};


class LLImagePreviewAvatar : public LLViewerDynamicTexture
{
protected:
    virtual ~LLImagePreviewAvatar();

public:
    LLImagePreviewAvatar(S32 width, S32 height);    

    /*virtual*/ S8 getType() const ;

    void setPreviewTarget(const std::string& joint_name, const std::string& mesh_name, LLImageRaw* imagep, F32 distance, BOOL male);
    void setTexture(U32 name) { mTextureName = name; }
    void clearPreviewTexture(const std::string& mesh_name);

    BOOL    render();
    void    refresh();
    void    rotate(F32 yaw_radians, F32 pitch_radians);
    void    zoom(F32 zoom_amt);
    void    pan(F32 right, F32 up);
    virtual BOOL needsRender() { return mNeedsUpdate; }

protected:
    BOOL        mNeedsUpdate;
    LLJoint*    mTargetJoint;
    LLViewerJointMesh*  mTargetMesh;
    F32         mCameraDistance;
    F32         mCameraYaw;
    F32         mCameraPitch;
    F32         mCameraZoom;
    LLVector3   mCameraOffset;
    LLPointer<LLVOAvatar> mDummyAvatar;
    U32         mTextureName;
};

class LLFloaterImagePreview : public LLFloaterNameDesc
{
public:
    LLFloaterImagePreview(const std::string& filename);
    virtual ~LLFloaterImagePreview();

    virtual BOOL postBuild();
    
    BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    BOOL handleMouseUp(S32 x, S32 y, MASK mask);
    BOOL handleHover(S32 x, S32 y, MASK mask);
    BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 

    static void onMouseCaptureLostImagePreview(LLMouseHandler*);

    void clearAllPreviewTextures();

protected:
    static void     onPreviewTypeCommit(LLUICtrl*,void*);
    void            draw();
    bool            loadImage(const std::string& filename);

    LLPointer<LLImageRaw> mRawImagep;
    LLPointer<LLImagePreviewAvatar> mAvatarPreview;
    LLPointer<LLImagePreviewSculpted> mSculptedPreview;
    S32             mLastMouseX;
    S32             mLastMouseY;
    LLRect          mPreviewRect;
    LLRectf         mPreviewImageRect;
    LLPointer<LLViewerTexture> mImagep ;
    
    std::string mImageLoadError;
};

#endif  // LL_LLFLOATERIMAGEPREVIEW_H
