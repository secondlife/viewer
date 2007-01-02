/** 
 * @file llfloaterimagepreview.h
 * @brief LLFloaterImagePreview class definition
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERIMAGEPREVIEW_H
#define LL_LLFLOATERIMAGEPREVIEW_H

#include "llfloaternamedesc.h"
#include "lldynamictexture.h"
#include "llquaternion.h"

class LLComboBox;
class LLJoint;
class LLViewerJointMesh;
class LLVOAvatar;
class LLTextBox;

class LLImagePreviewAvatar : public LLDynamicTexture
{
public:
	LLImagePreviewAvatar(S32 width, S32 height);
	virtual ~LLImagePreviewAvatar();

	void setPreviewTarget(const char* joint_name, const char *mesh_name, LLImageRaw* imagep, F32 distance, BOOL male);
	void setTexture(U32 name) { mTextureName = name; }

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
	LLFloaterImagePreview(const char* filename);
	virtual ~LLFloaterImagePreview();

	virtual BOOL postBuild();
	
	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 

	static void onMouseCaptureLost(LLMouseHandler*);
	static void setUploadAmount(S32 amount) { sUploadAmount = amount; }

protected:
	static void		onPreviewTypeCommit(LLUICtrl*,void*);
	void			draw();
	bool			loadImage(const char* filename);

	LLPointer<LLImageRaw> mRawImagep;
	LLImagePreviewAvatar* mAvatarPreview;
	S32				mLastMouseX;
	S32				mLastMouseY;
	LLRect			mPreviewRect;
	LLRectf			mPreviewImageRect;
	GLuint			mGLName;

	static S32		sUploadAmount;
};

#endif  // LL_LLFLOATERIMAGEPREVIEW_H
