/** 
 * @file llfloaterimagepreview.h
 * @brief LLFloaterImagePreview class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	F32			mCameraDistance;
	F32			mCameraYaw;
	F32			mCameraPitch;
	F32			mCameraZoom;
	LLVector3	mCameraOffset;
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

	BOOL	render();
	void	refresh();
	void	rotate(F32 yaw_radians, F32 pitch_radians);
	void	zoom(F32 zoom_amt);
	void	pan(F32 right, F32 up);
	virtual BOOL needsRender() { return mNeedsUpdate; }

protected:
	BOOL		mNeedsUpdate;
	LLJoint*	mTargetJoint;
	LLViewerJointMesh*	mTargetMesh;
	F32			mCameraDistance;
	F32			mCameraYaw;
	F32			mCameraPitch;
	F32			mCameraZoom;
	LLVector3	mCameraOffset;
	LLPointer<LLVOAvatar> mDummyAvatar;
	U32			mTextureName;
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
	static void		onPreviewTypeCommit(LLUICtrl*,void*);
	void			draw();
	bool			loadImage(const std::string& filename);

	LLPointer<LLImageRaw> mRawImagep;
	LLPointer<LLImagePreviewAvatar> mAvatarPreview;
	LLPointer<LLImagePreviewSculpted> mSculptedPreview;
	S32				mLastMouseX;
	S32				mLastMouseY;
	LLRect			mPreviewRect;
	LLRectf			mPreviewImageRect;
	LLPointer<LLViewerTexture> mImagep ;
	
	std::string mImageLoadError;
};

#endif  // LL_LLFLOATERIMAGEPREVIEW_H
