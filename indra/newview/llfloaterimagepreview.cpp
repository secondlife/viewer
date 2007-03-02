/** 
 * @file llfloaterimagepreview.cpp
 * @brief LLFloaterImagePreview class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterimagepreview.h"

#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llface.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "pipeline.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"

//static
S32 LLFloaterImagePreview::sUploadAmount = 10;

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;

//-----------------------------------------------------------------------------
// LLFloaterImagePreview()
//-----------------------------------------------------------------------------
LLFloaterImagePreview::LLFloaterImagePreview(const char* filename) : 
	LLFloaterNameDesc(filename)
{
	mLastMouseX = 0;
	mLastMouseY = 0;
	mGLName = 0;
	loadImage(mFilenameAndPath.c_str());
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::postBuild()
{
	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}

	childSetLabelArg("ok_btn", "[AMOUNT]", llformat("%d",sUploadAmount));

	LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
	if (iface)
	{
		iface->selectFirstItem();
	}
	childSetCommitCallback("clothing_type_combo", onPreviewTypeCommit, this);

	mPreviewRect.set(PREVIEW_HPAD, 
		PREVIEW_TEXTURE_HEIGHT,
		getRect().getWidth() - PREVIEW_HPAD, 
		PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
	mPreviewImageRect.set(0.f, 1.f, 1.f, 0.f);

	childHide("bad_image_text");

	if (mRawImagep.notNull())
	{
		mAvatarPreview = new LLImagePreviewAvatar(256, 256);
		mAvatarPreview->setPreviewTarget("mPelvis", "mUpperBodyMesh0", mRawImagep, 2.f, FALSE);
	}
	else
	{
		mAvatarPreview = NULL;
		childShow("bad_image_text");
		childDisable("clothing_type_combo");
		childDisable("ok_btn");
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterImagePreview()
//-----------------------------------------------------------------------------
LLFloaterImagePreview::~LLFloaterImagePreview()
{
	mRawImagep = NULL;
	delete mAvatarPreview;
	if (mGLName)
	{
		glDeleteTextures(1, &mGLName );
	}
}

//static 
//-----------------------------------------------------------------------------
// onPreviewTypeCommit()
//-----------------------------------------------------------------------------
void	LLFloaterImagePreview::onPreviewTypeCommit(LLUICtrl* ctrl, void* userdata)
{
	LLFloaterImagePreview *fp =(LLFloaterImagePreview *)userdata;
	
	if (!fp->mAvatarPreview)
	{
		return;
	}

	S32 which_mode = 0;

	LLCtrlSelectionInterface* iface = fp->childGetSelectionInterface("clothing_type_combo");
	if (iface)
	{
		which_mode = iface->getFirstSelectedIndex();
	}

	switch(which_mode)
	{
	case 0:
		break;
	case 1:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHairMesh0", fp->mRawImagep, 0.4f, FALSE);
		break;
	case 2:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHeadMesh0", fp->mRawImagep, 0.4f, FALSE);
		break;
	case 3:
		fp->mAvatarPreview->setPreviewTarget("mChest", "mUpperBodyMesh0", fp->mRawImagep, 1.0f, FALSE);
		break;
	case 4:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mLowerBodyMesh0", fp->mRawImagep, 1.2f, FALSE);
		break;
	case 5:
		fp->mAvatarPreview->setPreviewTarget("mSkull", "mHeadMesh0", fp->mRawImagep, 0.4f, TRUE);
		break;
	case 6:
		fp->mAvatarPreview->setPreviewTarget("mChest", "mUpperBodyMesh0", fp->mRawImagep, 1.2f, TRUE);
		break;
	case 7:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mLowerBodyMesh0", fp->mRawImagep, 1.2f, TRUE);
		break;
	case 8:
		fp->mAvatarPreview->setPreviewTarget("mKneeLeft", "mSkirtMesh0", fp->mRawImagep, 1.3f, FALSE);
		break;
	default:
		break;
	}
	fp->mAvatarPreview->refresh();
}

//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterImagePreview::draw()
{
	LLFloater::draw();
	LLRect r = getRect();

	if (mRawImagep.notNull())
	{
		LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
		if (iface && iface->getFirstSelectedIndex() <= 0)
		{
			gl_rect_2d_checkerboard(mPreviewRect);
			LLGLDisable gls_alpha(GL_ALPHA_TEST);

			GLenum format_options[4] = { GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
			GLenum format = format_options[mRawImagep->getComponents()-1];

			GLenum internal_format_options[4] = { GL_LUMINANCE8, GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8 };
			GLenum internal_format = internal_format_options[mRawImagep->getComponents()-1];
		
			if (mGLName)
			{
				LLImageGL::bindExternalTexture( mGLName, 0, GL_TEXTURE_2D ); 
			}
			else
			{
				glGenTextures(1, &mGLName );
				stop_glerror();

				LLImageGL::bindExternalTexture( mGLName, 0, GL_TEXTURE_2D ); 
				stop_glerror();

				glTexImage2D(
					GL_TEXTURE_2D, 0, internal_format, 
					mRawImagep->getWidth(), mRawImagep->getHeight(),
					0, format, GL_UNSIGNED_BYTE, mRawImagep->getData());
				stop_glerror();

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (mAvatarPreview)
				{
					mAvatarPreview->setTexture(mGLName);
				}
			}

			glColor3f(1.f, 1.f, 1.f);
			glBegin( GL_QUADS );
			{
				glTexCoord2f(mPreviewImageRect.mLeft, mPreviewImageRect.mTop);
				glVertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
				glTexCoord2f(mPreviewImageRect.mLeft, mPreviewImageRect.mBottom);
				glVertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
				glTexCoord2f(mPreviewImageRect.mRight, mPreviewImageRect.mBottom);
				glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
				glTexCoord2f(mPreviewImageRect.mRight, mPreviewImageRect.mTop);
				glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
			}
			glEnd();

			LLImageGL::unbindTexture(0, GL_TEXTURE_2D);

			stop_glerror();
		}
		else
		{
			if (mAvatarPreview)
			{
				glColor3f(1.f, 1.f, 1.f);
				mAvatarPreview->bindTexture();

				glBegin( GL_QUADS );
				{
					glTexCoord2f(0.f, 1.f);
					glVertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
					glTexCoord2f(0.f, 0.f);
					glVertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
					glTexCoord2f(1.f, 0.f);
					glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
					glTexCoord2f(1.f, 1.f);
					glVertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
				}
				glEnd();

				mAvatarPreview->unbindTexture();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// loadImage()
//-----------------------------------------------------------------------------
bool LLFloaterImagePreview::loadImage(const char *src_filename)
{
	// U32 length = strlen(src_filename);
	const char* ext = strrchr(src_filename, '.');
	char error_message[MAX_STRING];
	error_message[0] = '\0';

	U32 codec = IMG_CODEC_INVALID;
	LLString temp_str;
	if( 0 == strnicmp(ext, ".bmp", 4) )
	{
		codec = IMG_CODEC_BMP;
	}
	else if( 0 == strnicmp(ext, ".tga", 4) )
	{
		codec = IMG_CODEC_TGA;
	}
	else if( 0 == strnicmp(ext, ".jpg", 4) || 0 == strnicmp(ext, ".jpeg", 5))
	{
		codec = IMG_CODEC_JPEG;
	}

	LLPointer<LLImageRaw> raw_image = new LLImageRaw;

	switch (codec)
	{
	case IMG_CODEC_BMP:
		{
			LLPointer<LLImageBMP> bmp_image = new LLImageBMP;

			if (!bmp_image->load(src_filename))
			{
				return false;
			}
			
			if (!bmp_image->decode(raw_image))
			{
				return false;
			}
		}
		break;
	case IMG_CODEC_TGA:
		{
			LLPointer<LLImageTGA> tga_image = new LLImageTGA;

			if (!tga_image->load(src_filename))
			{
				return false;
			}
			
			if (!tga_image->decode(raw_image))
			{
				return false;
			}

			if(	(tga_image->getComponents() != 3) &&
				(tga_image->getComponents() != 4) )
			{
				tga_image->setLastError( "Image files with less than 3 or more than 4 components are not supported." );
				return false;
			}
		}
		break;
	case IMG_CODEC_JPEG:
		{
			LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG;

			if (!jpeg_image->load(src_filename))
			{
				return false;
			}
			
			if (!jpeg_image->decode(raw_image))
			{
				return false;
			}
		}
		break;
	default:
		return false;
	}

	raw_image->biasedScaleToPowerOfTwo(1024);
	mRawImagep = raw_image;
	
	return true;
}

//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPreviewRect.pointInRect(x, y))
	{
		bringToFront( x, y );
		gViewerWindow->setMouseCapture(this, onMouseCaptureLost);
		gViewerWindow->hideCursor();
		mLastMouseX = x;
		mLastMouseY = y;
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleMouseUp()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gViewerWindow->setMouseCapture(FALSE, NULL);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleHover(S32 x, S32 y, MASK mask)
{
	MASK local_mask = mask & ~MASK_ALT;

	if (mAvatarPreview && gViewerWindow->hasMouseCapture(this))
	{
		if (local_mask == MASK_PAN)
		{
			// pan here
			LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
			if (iface && iface->getFirstSelectedIndex() <= 0)
			{
				mPreviewImageRect.translate((F32)(x - mLastMouseX) * -0.005f * mPreviewImageRect.getWidth(), 
					(F32)(y - mLastMouseY) * -0.005f * mPreviewImageRect.getHeight());
			}
			else
			{
				mAvatarPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
			}
		}
		else if (local_mask == MASK_ORBIT)
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 pitch_radians = (F32)(y - mLastMouseY) * 0.02f;
			
			mAvatarPreview->rotate(yaw_radians, pitch_radians);
		}
		else 
		{
			LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
			if (iface && iface->getFirstSelectedIndex() <= 0)
			{
				F32 zoom_amt = (F32)(y - mLastMouseY) * -0.002f;
				mPreviewImageRect.stretch(zoom_amt);
			}
			else
			{
				F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
				F32 zoom_amt = (F32)(y - mLastMouseY) * 0.02f;
				
				mAvatarPreview->rotate(yaw_radians, 0.f);
				mAvatarPreview->zoom(zoom_amt);
			}
		}

		LLCtrlSelectionInterface* iface = childGetSelectionInterface("clothing_type_combo");
		if (iface && iface->getFirstSelectedIndex() <= 0)
		{
			if (mPreviewImageRect.getWidth() > 1.f)
			{
				mPreviewImageRect.stretch((1.f - mPreviewImageRect.getWidth()) * 0.5f);
			}
			else if (mPreviewImageRect.getWidth() < 0.1f)
			{
				mPreviewImageRect.stretch((0.1f - mPreviewImageRect.getWidth()) * 0.5f);
			}

			if (mPreviewImageRect.getHeight() > 1.f)
			{
				mPreviewImageRect.stretch((1.f - mPreviewImageRect.getHeight()) * 0.5f);
			}
			else if (mPreviewImageRect.getHeight() < 0.1f)
			{
				mPreviewImageRect.stretch((0.1f - mPreviewImageRect.getHeight()) * 0.5f);
			}

			if (mPreviewImageRect.mLeft < 0.f)
			{
				mPreviewImageRect.translate(-mPreviewImageRect.mLeft, 0.f);
			}
			else if (mPreviewImageRect.mRight > 1.f)
			{
				mPreviewImageRect.translate(1.f - mPreviewImageRect.mRight, 0.f);
			}

			if (mPreviewImageRect.mBottom < 0.f)
			{
				mPreviewImageRect.translate(0.f, -mPreviewImageRect.mBottom);
			}
			else if (mPreviewImageRect.mTop > 1.f)
			{
				mPreviewImageRect.translate(0.f, 1.f - mPreviewImageRect.mTop);
			}
		}
		else
		{
			mAvatarPreview->refresh();
		}

		LLUI::setCursorPositionLocal(this, mLastMouseX, mLastMouseY);
	}

	if (!mPreviewRect.pointInRect(x, y) || !mAvatarPreview)
	{
		return LLFloater::handleHover(x, y, mask);
	}
	else if (local_mask == MASK_ORBIT)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (local_mask == MASK_PAN)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
BOOL LLFloaterImagePreview::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (mPreviewRect.pointInRect(x, y) && mAvatarPreview)
	{
		mAvatarPreview->zoom((F32)clicks * -0.2f);
		mAvatarPreview->refresh();
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// onMouseCaptureLost()
//-----------------------------------------------------------------------------
void LLFloaterImagePreview::onMouseCaptureLost(LLMouseHandler* handler)
{
	gViewerWindow->showCursor();
}


//-----------------------------------------------------------------------------
// LLImagePreviewAvatar
//-----------------------------------------------------------------------------
LLImagePreviewAvatar::LLImagePreviewAvatar(S32 width, S32 height) : LLDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
{
	mNeedsUpdate = TRUE;
	mTargetJoint = NULL;
	mTargetMesh = NULL;
	mCameraDistance = 0.f;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;

	mDummyAvatar = new LLVOAvatar(LLUUID::null, LL_PCODE_LEGACY_AVATAR, gAgent.getRegion());
	mDummyAvatar->createDrawable(&gPipeline);
	mDummyAvatar->mIsDummy = TRUE;
	mDummyAvatar->mSpecialRenderMode = 2;
	mDummyAvatar->setPositionAgent(LLVector3::zero);
	mDummyAvatar->slamPosition();
	mDummyAvatar->updateJointLODs();
	mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	gPipeline.markVisible(mDummyAvatar->mDrawable, *gCamera);

	mTextureName = 0;
}

LLImagePreviewAvatar::~LLImagePreviewAvatar()
{
	mDummyAvatar->markDead();
}


void LLImagePreviewAvatar::setPreviewTarget(const char* joint_name, const char* mesh_name, LLImageRaw* imagep, F32 distance, BOOL male) 
{ 
	mTargetJoint = mDummyAvatar->mRoot.findJoint(joint_name);
	// clear out existing test mesh
	if (mTargetMesh)
	{
		mTargetMesh->setTestTexture(0);
	}

	if (male)
	{
		mDummyAvatar->setVisualParamWeight( "male", 1.f );
		mDummyAvatar->updateVisualParams();
		mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	}
	else
	{
		mDummyAvatar->setVisualParamWeight( "male", 0.f );
		mDummyAvatar->updateVisualParams();
		mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	}
	mDummyAvatar->mRoot.setVisible(FALSE, TRUE);

	mTargetMesh = (LLViewerJointMesh*)mDummyAvatar->mRoot.findJoint(mesh_name);
	mTargetMesh->setTestTexture(mTextureName);
	mTargetMesh->setVisible(TRUE, FALSE);
	mCameraDistance = distance;
	mCameraZoom = 1.f;
	mCameraPitch = 0.f;
	mCameraYaw = 0.f;
	mCameraOffset.clearVec();
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
BOOL	LLImagePreviewAvatar::render()
{
	mNeedsUpdate = FALSE;
	LLVOAvatar* avatarp = mDummyAvatar;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0f, mWidth, 0.0f, mHeight, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	LLGLSUIDefault def;
	glColor4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mWidth, mHeight );

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	LLVector3 target_pos = mTargetJoint->getWorldPosition();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = avatarp->mPelvisp->getWorldRotation() * camera_rot;
	gCamera->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	stop_glerror();

	gCamera->setView(gCamera->getDefaultFOV() / mCameraZoom);
	gCamera->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mWidth, mHeight, FALSE);

	LLVertexBuffer::stopRender();
	avatarp->updateLOD();
	LLVertexBuffer::startRender();

	if (avatarp->mDrawable.notNull())
	{
		LLGLDepthTest gls_depth(GL_TRUE, GL_TRUE);
		// make sure alpha=0 shows avatar material color
		LLGLDisable no_blend(GL_BLEND);

		LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)avatarp->mDrawable->getFace(0)->getPool();
		
		avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::refresh()
{ 
	mNeedsUpdate = TRUE; 
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLImagePreviewAvatar::zoom(F32 zoom_amt)
{
	mCameraZoom	= llclamp(mCameraZoom + zoom_amt, 1.f, 10.f);
}

void LLImagePreviewAvatar::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}
