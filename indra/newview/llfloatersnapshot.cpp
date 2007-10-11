/** 
 * @file llfloatersnapshot.cpp
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "v3dmath.h"
#include "lldir.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llcriticaldamp.h"
#include "llui.h"
#include "llviewertexteditor.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"
#include "llviewerstats.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llfloaterpostcard.h"
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llworld.h"

#include "llgl.h"
#include "llglheaders.h"
#include "llimagejpeg.h"
#include "llimagej2c.h"
#include "llvfile.h"
#include "llvfs.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLSnapshotFloaterView* gSnapshotFloaterView = NULL;

LLFloaterSnapshot* LLFloaterSnapshot::sInstance = NULL;
const F32 SNAPSHOT_TIME_DELAY = 1.f;

F32 SHINE_TIME = 0.5f;
F32 SHINE_WIDTH = 0.6f;
F32 SHINE_OPACITY = 0.3f;
F32 FALL_TIME = 0.6f;
S32 BORDER_WIDTH = 6;

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte

///----------------------------------------------------------------------------
/// Class LLSnapshotLivePreview 
///----------------------------------------------------------------------------
class LLSnapshotLivePreview : public LLView
{
public:
	enum ESnapshotType
	{
		SNAPSHOT_POSTCARD,
		SNAPSHOT_TEXTURE,
		SNAPSHOT_BITMAP
	};

	LLSnapshotLivePreview(const LLRect& rect);
	~LLSnapshotLivePreview();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	
	void setSize(S32 w, S32 h);
	void getSize(S32& w, S32& h) const;
	S32 getDataSize() const { return mDataSize; }
	
	ESnapshotType getSnapshotType() const { return mSnapshotType; }
	BOOL getSnapshotUpToDate() const { return mSnapshotUpToDate; }
	BOOL isSnapshotActive() { return mSnapshotActive; }
	LLImageGL* getCurrentImage();
	F32 getImageAspect();
	LLRect getImageRect();
	BOOL isImageScaled();
	
	void setSnapshotType(ESnapshotType type) { mSnapshotType = type; }
	void setSnapshotQuality(S32 quality);
	void setSnapshotBufferType(LLViewerWindow::ESnapshotType type) { mSnapshotBufferType = type; }
	void updateSnapshot(BOOL new_snapshot);
	LLFloaterPostcard* savePostcard();
	void saveTexture();
	BOOL saveLocal();

	static void onIdle( void* snapshot_preview );

protected:
	LLColor4					mColor;
	LLPointer<LLImageGL>		mViewerImage[2];
	LLRect						mImageRect[2];
	S32							mWidth[2];
	S32							mHeight[2];
	BOOL						mImageScaled[2];

	S32							mCurImageIndex;
	LLPointer<LLImageRaw>		mRawImage;
	LLPointer<LLImageRaw>		mRawImageEncoded;
	LLPointer<LLImageJPEG>		mJPEGImage;
	LLFrameTimer				mSnapshotDelayTimer;
	S32							mShineCountdown;
	LLFrameTimer				mShineAnimTimer;
	F32							mFlashAlpha;
	BOOL						mNeedsFlash;
	LLVector3d					mPosTakenGlobal;
	S32							mSnapshotQuality;
	S32							mDataSize;
	ESnapshotType				mSnapshotType;
	BOOL						mSnapshotUpToDate;
	LLFrameTimer				mFallAnimTimer;
	LLVector3					mCameraPos;
	LLQuaternion				mCameraRot;
	BOOL						mSnapshotActive;
	LLViewerWindow::ESnapshotType mSnapshotBufferType;

public:
	static std::set<LLSnapshotLivePreview*> sList;
};

std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;

LLSnapshotLivePreview::LLSnapshotLivePreview (const LLRect& rect) : 
	LLView("snapshot_live_preview", rect, FALSE), 
	mColor(1.f, 0.f, 0.f, 0.5f), 
	mCurImageIndex(0),
	mRawImage(NULL),
	mRawImageEncoded(NULL),
	mJPEGImage(NULL),
	mShineCountdown(0),
	mFlashAlpha(0.f),
	mNeedsFlash(TRUE),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mDataSize(0),
	mSnapshotType(SNAPSHOT_POSTCARD),
	mSnapshotUpToDate(FALSE),
	mCameraPos(gCamera->getOrigin()),
	mCameraRot(gCamera->getQuaternion()),
	mSnapshotActive(FALSE),
	mSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR)
{
	mSnapshotDelayTimer.setTimerExpirySec(0.0f);
	mSnapshotDelayTimer.start();
// 	gIdleCallbacks.addFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.insert(this);
	setFollowsAll();
	mWidth[0] = gViewerWindow->getWindowDisplayWidth();
	mWidth[1] = gViewerWindow->getWindowDisplayWidth();
	mHeight[0] = gViewerWindow->getWindowDisplayHeight();
	mHeight[1] = gViewerWindow->getWindowDisplayHeight();
	mImageScaled[0] = FALSE;
	mImageScaled[1] = FALSE;
}

LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
	// delete images
	mRawImage = NULL;
	mRawImageEncoded = NULL;
	mJPEGImage = NULL;

// 	gIdleCallbacks.deleteFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.erase(this);
}

LLImageGL* LLSnapshotLivePreview::getCurrentImage()
{
	return mViewerImage[mCurImageIndex];
}

F32 LLSnapshotLivePreview::getImageAspect()
{
	if (!mViewerImage[mCurImageIndex])
	{
		return 0.f;
	}

	F32 image_aspect_ratio = ((F32)mWidth[mCurImageIndex]) / ((F32)mHeight[mCurImageIndex]);
	F32 window_aspect_ratio = ((F32)mRect.getWidth()) / ((F32)mRect.getHeight());

	if (gSavedSettings.getBOOL("KeepAspectForSnapshot"))
	{
		return image_aspect_ratio;
	}
	else
	{
		return window_aspect_ratio;
	}
}

LLRect LLSnapshotLivePreview::getImageRect()
{
	return mImageRect[mCurImageIndex];
}

BOOL LLSnapshotLivePreview::isImageScaled()
{
	return mImageScaled[mCurImageIndex];
}

void LLSnapshotLivePreview::updateSnapshot(BOOL new_snapshot) 
{ 
	if (mSnapshotUpToDate)
	{
		S32 old_image_index = mCurImageIndex;
		mCurImageIndex = (mCurImageIndex + 1) % 2; 
		mWidth[mCurImageIndex] = mWidth[old_image_index];
		mHeight[mCurImageIndex] = mHeight[old_image_index];
		mFallAnimTimer.start();
	}
	mSnapshotUpToDate = FALSE; 
	mShineAnimTimer.stop();
	if (new_snapshot)
	{
		mSnapshotDelayTimer.start();
		mSnapshotDelayTimer.setTimerExpirySec(SNAPSHOT_TIME_DELAY);
	}

	LLRect& rect = mImageRect[mCurImageIndex];
	rect.set(0, mRect.getHeight(), mRect.getWidth(), 0);

	F32 image_aspect_ratio = ((F32)mWidth[mCurImageIndex]) / ((F32)mHeight[mCurImageIndex]);
	F32 window_aspect_ratio = ((F32)mRect.getWidth()) / ((F32)mRect.getHeight());

	if (gSavedSettings.getBOOL("KeepAspectForSnapshot"))
	{
		if (image_aspect_ratio > window_aspect_ratio)
		{
			// trim off top and bottom
			S32 new_height = llround((F32)mRect.getWidth() / image_aspect_ratio); 
			rect.mBottom += (mRect.getHeight() - new_height) / 2;
			rect.mTop -= (mRect.getHeight() - new_height) / 2;
		}
		else if (image_aspect_ratio < window_aspect_ratio)
		{
			// trim off left and right
			S32 new_width = llround((F32)mRect.getHeight() * image_aspect_ratio); 
			rect.mLeft += (mRect.getWidth() - new_width) / 2;
			rect.mRight -= (mRect.getWidth() - new_width) / 2;
		}
	}
}

void LLSnapshotLivePreview::setSnapshotQuality(S32 quality)
{
	if (quality != mSnapshotQuality)
	{
		mSnapshotQuality = quality;
		gSavedSettings.setS32("SnapshotQuality", quality);
	}
}

EWidgetType LLSnapshotLivePreview::getWidgetType() const
{
	return WIDGET_TYPE_SNAPSHOT_LIVE_PREVIEW;
}

LLString LLSnapshotLivePreview::getWidgetTag() const
{
	return LL_SNAPSHOT_LIVE_PREVIEW_TAG;
}

void LLSnapshotLivePreview::draw()
{
	if(getVisible()) 
	{
		if (mViewerImage[mCurImageIndex].notNull() &&
		    mRawImageEncoded.notNull() &&
		    mSnapshotUpToDate)
		{
			LLColor4 bg_color(0.f, 0.f, 0.3f, 0.4f);
			gl_rect_2d(mRect, bg_color);
			LLRect &rect = mImageRect[mCurImageIndex];
			LLRect shadow_rect = mImageRect[mCurImageIndex];
			shadow_rect.stretch(BORDER_WIDTH);
			gl_drop_shadow(shadow_rect.mLeft, shadow_rect.mTop, shadow_rect.mRight, shadow_rect.mBottom, LLColor4(0.f, 0.f, 0.f, mNeedsFlash ? 0.f :0.5f), 10);

			LLGLSTexture set_texture;
			LLColor4 image_color(1.f, 1.f, 1.f, 1.f);
			glColor4fv(image_color.mV);
			LLViewerImage::bindTexture(mViewerImage[mCurImageIndex]);
			// calculate UV scale
			F32 uv_width = mImageScaled[mCurImageIndex] ? 1.f : llmin((F32)mWidth[mCurImageIndex] / (F32)mViewerImage[mCurImageIndex]->getWidth(), 1.f);
			F32 uv_height = mImageScaled[mCurImageIndex] ? 1.f : llmin((F32)mHeight[mCurImageIndex] / (F32)mViewerImage[mCurImageIndex]->getHeight(), 1.f);
			glPushMatrix();
			{
				glTranslatef((F32)rect.mLeft, (F32)rect.mBottom, 0.f);
				glBegin(GL_QUADS);
				{
					glTexCoord2f(uv_width, uv_height);
					glVertex2i(rect.getWidth(), rect.getHeight() );

					glTexCoord2f(0.f, uv_height);
					glVertex2i(0, rect.getHeight() );

					glTexCoord2f(0.f, 0.f);
					glVertex2i(0, 0);

					glTexCoord2f(uv_width, 0.f);
					glVertex2i(rect.getWidth(), 0);
				}
				glEnd();
			}
			glPopMatrix();

			glColor4f(1.f, 1.f, 1.f, mFlashAlpha);
			gl_rect_2d(mRect);
			if (mNeedsFlash)
			{
				if (mFlashAlpha < 1.f)
				{
					mFlashAlpha = lerp(mFlashAlpha, 1.f, LLCriticalDamp::getInterpolant(0.02f));
				}
				else
				{
					mNeedsFlash = FALSE;
				}
			}
			else
			{
				mFlashAlpha = lerp(mFlashAlpha, 0.f, LLCriticalDamp::getInterpolant(0.15f));
			}

			if (mShineCountdown > 0)
			{
				mShineCountdown--;
				if (mShineCountdown == 0)
				{
					mShineAnimTimer.start();
				}
			}
			else if (mShineAnimTimer.getStarted())
			{
				//LLDebugVarMessageBox::show("Shine time", &SHINE_TIME, 10.f, 0.1f);
				//LLDebugVarMessageBox::show("Shine width", &SHINE_WIDTH, 2.f, 0.05f);
				//LLDebugVarMessageBox::show("Shine opacity", &SHINE_OPACITY, 1.f, 0.05f);

				F32 shine_interp = llmin(1.f, mShineAnimTimer.getElapsedTimeF32() / SHINE_TIME);
				
				// draw "shine" effect
				LLLocalClipRect clip(getLocalRect());
				{
					// draw diagonal stripe with gradient that passes over screen
					S32 x1 = gViewerWindow->getWindowWidth() * llround((clamp_rescale(shine_interp, 0.f, 1.f, -1.f - SHINE_WIDTH, 1.f)));
					S32 x2 = x1 + llround(gViewerWindow->getWindowWidth() * SHINE_WIDTH);
					S32 x3 = x2 + llround(gViewerWindow->getWindowWidth() * SHINE_WIDTH);
					S32 y1 = 0;
					S32 y2 = gViewerWindow->getWindowHeight();

					LLGLSNoTexture no_texture;
					glBegin(GL_QUADS);
					{
						glColor4f(1.f, 1.f, 1.f, 0.f);
						glVertex2i(x1, y1);
						glVertex2i(x1 + gViewerWindow->getWindowWidth(), y2);
						glColor4f(1.f, 1.f, 1.f, SHINE_OPACITY);
						glVertex2i(x2 + gViewerWindow->getWindowWidth(), y2);
						glVertex2i(x2, y1);

						glColor4f(1.f, 1.f, 1.f, SHINE_OPACITY);
						glVertex2i(x2, y1);
						glVertex2i(x2 + gViewerWindow->getWindowWidth(), y2);
						glColor4f(1.f, 1.f, 1.f, 0.f);
						glVertex2i(x3 + gViewerWindow->getWindowWidth(), y2);
						glVertex2i(x3, y1);
					}
					glEnd();
				}

				if (mShineAnimTimer.getElapsedTimeF32() > SHINE_TIME)
				{
					mShineAnimTimer.stop();
				}
			}
		}

		// draw framing rectangle
		{
			LLGLSNoTexture no_texture;
			glColor4f(1.f, 1.f, 1.f, 1.f);
			LLRect outline_rect = mImageRect[mCurImageIndex];
			glBegin(GL_QUADS);
			{
				glVertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
				glVertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
				glVertex2i(outline_rect.mRight, outline_rect.mTop);
				glVertex2i(outline_rect.mLeft, outline_rect.mTop);

				glVertex2i(outline_rect.mLeft, outline_rect.mBottom);
				glVertex2i(outline_rect.mRight, outline_rect.mBottom);
				glVertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
				glVertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);

				glVertex2i(outline_rect.mLeft, outline_rect.mTop);
				glVertex2i(outline_rect.mLeft, outline_rect.mBottom);
				glVertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
				glVertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);

				glVertex2i(outline_rect.mRight, outline_rect.mBottom);
				glVertex2i(outline_rect.mRight, outline_rect.mTop);
				glVertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
				glVertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			}
			glEnd();
		}

		// draw old image dropping away
		if (mFallAnimTimer.getStarted())
		{
			S32 old_image_index = (mCurImageIndex + 1) % 2;
			if (mViewerImage[old_image_index].notNull() && mFallAnimTimer.getElapsedTimeF32() < FALL_TIME)
			{
				LLGLSTexture texture_set;

				F32 fall_interp = mFallAnimTimer.getElapsedTimeF32() / FALL_TIME;
				F32 alpha = clamp_rescale(fall_interp, 0.f, 1.f, 0.8f, 0.4f);
				LLColor4 image_color(1.f, 1.f, 1.f, alpha);
				glColor4fv(image_color.mV);
				LLViewerImage::bindTexture(mViewerImage[old_image_index]);
				// calculate UV scale
				// *FIX get this to work with old image
				BOOL rescale = !mImageScaled[old_image_index] && mViewerImage[mCurImageIndex].notNull();
				F32 uv_width = rescale ? llmin((F32)mWidth[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getWidth(), 1.f) : 1.f;
				F32 uv_height = rescale ? llmin((F32)mHeight[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getHeight(), 1.f) : 1.f;
				glPushMatrix();
				{
					LLRect& rect = mImageRect[old_image_index];
					glTranslatef((F32)rect.mLeft, (F32)rect.mBottom - llround(mRect.getHeight() * 2.f * (fall_interp * fall_interp)), 0.f);
					glRotatef(-45.f * fall_interp, 0.f, 0.f, 1.f);
					glBegin(GL_QUADS);
					{
						glTexCoord2f(uv_width, uv_height);
						glVertex2i(rect.getWidth(), rect.getHeight() );

						glTexCoord2f(0.f, uv_height);
						glVertex2i(0, rect.getHeight() );

						glTexCoord2f(0.f, 0.f);
						glVertex2i(0, 0);

						glTexCoord2f(uv_width, 0.f);
						glVertex2i(rect.getWidth(), 0);
					}
					glEnd();
				}
				glPopMatrix();
			}
		}
	}
}

/*virtual*/ 
void LLSnapshotLivePreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect old_rect = mRect;
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		updateSnapshot(getSnapshotUpToDate());
	}
}

//static 
void LLSnapshotLivePreview::onIdle( void* snapshot_preview )
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)snapshot_preview;

	LLVector3 new_camera_pos = gCamera->getOrigin();
	LLQuaternion new_camera_rot = gCamera->getQuaternion();
	if (gSavedSettings.getBOOL("FreezeTime") && 
		(new_camera_pos != previewp->mCameraPos || dot(new_camera_rot, previewp->mCameraRot) < 0.995f))
	{
		previewp->mCameraPos = new_camera_pos;
		previewp->mCameraRot = new_camera_rot;
		// request a new snapshot whenever the camera moves, with a time delay
 		previewp->updateSnapshot(gSavedSettings.getBOOL("AutoSnapshot"));
	}

	previewp->mSnapshotActive = (previewp->mSnapshotDelayTimer.getStarted() &&	
								 previewp->mSnapshotDelayTimer.hasExpired());

	// don't take snapshots while ALT-zoom active
	if (gToolCamera->hasMouseCapture())
	{
		previewp->mSnapshotActive = FALSE;
	}

	if (previewp->mSnapshotActive)
	{
		if (!previewp->mRawImage)
		{
			previewp->mRawImage = new LLImageRaw;
		}

		if (!previewp->mRawImageEncoded)
		{
			previewp->mRawImageEncoded = new LLImageRaw;
		}

		previewp->setVisible(FALSE);
		previewp->setEnabled(FALSE);
		
		previewp->getWindow()->incBusyCount();
		previewp->mImageScaled[previewp->mCurImageIndex] = FALSE;

		// do update
		if(gViewerWindow->rawSnapshot(previewp->mRawImage,
								previewp->mWidth[previewp->mCurImageIndex],
								previewp->mHeight[previewp->mCurImageIndex],
								!gSavedSettings.getBOOL("KeepAspectForSnapshot"),
								gSavedSettings.getBOOL("RenderUIInSnapshot"),
								FALSE,
								previewp->mSnapshotBufferType))
		{
			previewp->mRawImageEncoded->resize(previewp->mRawImage->getWidth(), previewp->mRawImage->getHeight(), previewp->mRawImage->getComponents());

			if (previewp->getSnapshotType() == SNAPSHOT_POSTCARD)
			{
				// *FIX: just resize and reuse existing jpeg?
				previewp->mJPEGImage = NULL; // deletes image
				previewp->mJPEGImage = new LLImageJPEG();
				previewp->mJPEGImage->setEncodeQuality(llclamp(previewp->mSnapshotQuality, 0, 100));
				if (previewp->mJPEGImage->encode(previewp->mRawImage))
				{
					previewp->mDataSize = previewp->mJPEGImage->getDataSize();
					previewp->mJPEGImage->decode(previewp->mRawImageEncoded);
				}
			}
			else if (previewp->getSnapshotType() == SNAPSHOT_TEXTURE)
			{
				LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
				LLPointer<LLImageRaw> scaled = new LLImageRaw(previewp->mRawImage->getData(),
															  previewp->mRawImage->getWidth(),
															  previewp->mRawImage->getHeight(),
															  previewp->mRawImage->getComponents());
			
				scaled->biasedScaleToPowerOfTwo(512);
				previewp->mImageScaled[previewp->mCurImageIndex] = TRUE;
				if (formatted->encode(scaled))
				{
					previewp->mDataSize = formatted->getDataSize();
					formatted->decode(previewp->mRawImageEncoded);
				}
			}
			else
			{
				previewp->mRawImageEncoded->copy(previewp->mRawImage);
				previewp->mDataSize = previewp->mRawImage->getDataSize();
			}

			LLPointer<LLImageRaw> scaled = new LLImageRaw(previewp->mRawImageEncoded->getData(),
														  previewp->mRawImageEncoded->getWidth(),
														  previewp->mRawImageEncoded->getHeight(),
														  previewp->mRawImageEncoded->getComponents());
			
			// leave original image dimensions, just scale up texture buffer
			if (previewp->mRawImageEncoded->getWidth() > 1024 || previewp->mRawImageEncoded->getHeight() > 1024)
			{
				// go ahead and shrink image to appropriate power of 2 for display
				scaled->biasedScaleToPowerOfTwo(1024);
				previewp->mImageScaled[previewp->mCurImageIndex] = TRUE;
			}
			else
			{
				// expand image but keep original image data intact
				scaled->expandToPowerOfTwo(1024, FALSE);
			}

			previewp->mViewerImage[previewp->mCurImageIndex] = new LLImageGL(scaled, FALSE);
			previewp->mViewerImage[previewp->mCurImageIndex]->setMipFilterNearest(previewp->getSnapshotType() != SNAPSHOT_TEXTURE);
			LLViewerImage::bindTexture(previewp->mViewerImage[previewp->mCurImageIndex]);
			previewp->mViewerImage[previewp->mCurImageIndex]->setClamp(TRUE, TRUE);

			previewp->mSnapshotUpToDate = TRUE;

			previewp->mPosTakenGlobal = gAgent.getCameraPositionGlobal();
			previewp->mShineCountdown = 4; // wait a few frames to avoid animation glitch due to readback this frame
		}
		previewp->getWindow()->decBusyCount();
		// only show fullscreen preview when in freeze frame mode
		previewp->setVisible(gSavedSettings.getBOOL("UseFreezeFrame"));
		previewp->mSnapshotDelayTimer.stop();
		previewp->mSnapshotActive = FALSE;
	}
}

void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
	mWidth[mCurImageIndex] = w;
	mHeight[mCurImageIndex] = h;
}

void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
	w = mWidth[mCurImageIndex];
	h = mHeight[mCurImageIndex];
}

LLFloaterPostcard* LLSnapshotLivePreview::savePostcard()
{
	// calculate and pass in image scale in case image data only use portion
	// of viewerimage buffer
	LLVector2 image_scale(1.f, 1.f);
	if (!isImageScaled())
	{
		image_scale.setVec(llmin(1.f, (F32)mWidth[mCurImageIndex] / (F32)getCurrentImage()->getWidth()), llmin(1.f, (F32)mHeight[mCurImageIndex] / (F32)getCurrentImage()->getHeight()));
	}


	LLFloaterPostcard* floater = LLFloaterPostcard::showFromSnapshot(mJPEGImage, mViewerImage[mCurImageIndex], image_scale, mPosTakenGlobal);
	// relinquish lifetime of viewerimage and jpeg image to postcard floater
	mViewerImage[mCurImageIndex] = NULL;
	mJPEGImage = NULL;

	return floater;
}

void LLSnapshotLivePreview::saveTexture()
{
	// gen a new uuid for this asset
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
		
	LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
	LLPointer<LLImageRaw> scaled = new LLImageRaw(mRawImage->getData(),
												  mRawImage->getWidth(),
												  mRawImage->getHeight(),
												  mRawImage->getComponents());
	
	scaled->biasedScaleToPowerOfTwo(512);
			
	if (formatted->encode(scaled))
	{
		LLVFile::writeFile(formatted->getData(), formatted->getDataSize(), gVFS, new_asset_id, LLAssetType::AT_TEXTURE);
		std::string pos_string;
		gAgent.buildLocationString(pos_string);
		std::string who_took_it;
		gAgent.buildFullname(who_took_it);
		upload_new_resource(tid,	// tid
							LLAssetType::AT_TEXTURE,
							"Snapshot : " + pos_string,
							"Taken by " + who_took_it + " at " + pos_string,
							0,
							LLAssetType::AT_SNAPSHOT_CATEGORY,
							LLInventoryType::IT_SNAPSHOT,
							PERM_ALL,
							"Snapshot : " + pos_string);
	}
	else
	{
		gViewerWindow->alertXml("ErrorEncodingSnapshot");
		llwarns << "Error encoding snapshot" << llendl;
	}

	gViewerStats->incStat(LLViewerStats::ST_SNAPSHOT_COUNT );	
}

BOOL LLSnapshotLivePreview::saveLocal()
{
	return gViewerWindow->saveImageNumbered(mRawImage);
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSnapshot::Impl
{
public:
	Impl() : mLastToolset(NULL)
	{
	}
	~Impl()
	{
		//unpause avatars
		mAvatarPauseHandles.clear();

	}
	static void onClickDiscard(void* data);
	static void onClickKeep(void* data);
	static void onClickNewSnapshot(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void onClickKeepOpenCheck(LLUICtrl *ctrl, void* data);
	static void onClickKeepAspectCheck(LLUICtrl *ctrl, void* data);
	static void onCommitQuality(LLUICtrl* ctrl, void* data);
	static void onCommitResolution(LLUICtrl* ctrl, void* data);
	static void onCommitFreezeFrame(LLUICtrl* ctrl, void* data);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	static void onCommitSnapshotType(LLUICtrl* ctrl, void* data);
	static void onCommitCustomResolution(LLUICtrl *ctrl, void* data);

	static LLSnapshotLivePreview* getPreviewView(LLFloaterSnapshot *floater);
	static void setResolution(LLFloaterSnapshot* floater, const std::string& comboname);
	static void updateControls(LLFloaterSnapshot* floater);
	static void updateLayout(LLFloaterSnapshot* floater);

	static LLViewHandle sPreviewHandle;
	
private:
	static LLSnapshotLivePreview::ESnapshotType getTypeIndex(LLFloaterSnapshot* floater);
	static LLViewerWindow::ESnapshotType getLayerType(LLFloaterSnapshot* floater);
	static void comboSetCustom(LLFloaterSnapshot *floater, const std::string& comboname);
	static void checkAutoSnapshot(LLSnapshotLivePreview* floater);

public:
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;

	LLToolset*	mLastToolset;
};

// static
LLViewHandle LLFloaterSnapshot::Impl::sPreviewHandle;

// static
LLSnapshotLivePreview* LLFloaterSnapshot::Impl::getPreviewView(LLFloaterSnapshot *floater)
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)LLView::getViewByHandle(sPreviewHandle);
	return previewp;
}

// static
LLSnapshotLivePreview::ESnapshotType LLFloaterSnapshot::Impl::getTypeIndex(LLFloaterSnapshot* floater)
{
	LLSnapshotLivePreview::ESnapshotType index = LLSnapshotLivePreview::SNAPSHOT_POSTCARD;
	LLSD value = floater->childGetValue("snapshot_type_radio");
	const std::string id = value.asString();
	if (id == "postcard")
		index = LLSnapshotLivePreview::SNAPSHOT_POSTCARD;
	else if (id == "texture")
		index = LLSnapshotLivePreview::SNAPSHOT_TEXTURE;
	else if (id == "local")
		index = LLSnapshotLivePreview::SNAPSHOT_BITMAP;
	return index;
}

// static
LLViewerWindow::ESnapshotType LLFloaterSnapshot::Impl::getLayerType(LLFloaterSnapshot* floater)
{
	LLViewerWindow::ESnapshotType type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
	LLSD value = floater->childGetValue("layer_types");
	const std::string id = value.asString();
	if (id == "colors")
		type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
	else if (id == "depth")
		type = LLViewerWindow::SNAPSHOT_TYPE_DEPTH;
	else if (id == "objects")
		type = LLViewerWindow::SNAPSHOT_TYPE_OBJECT_ID;
	return type;
}

// static
void LLFloaterSnapshot::Impl::setResolution(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(floater, comboname);
	if (combo)
	{
		combo->setVisible(TRUE);
		onCommitResolution(combo, floater);
	}
}

//static 
void LLFloaterSnapshot::Impl::updateLayout(LLFloaterSnapshot* floaterp)
{
	LLSnapshotLivePreview* previewp = getPreviewView(floaterp);
	if (floaterp->childGetValue("freeze_frame_check").asBoolean())
	{
		// stop all mouse events at fullscreen preview layer
		floaterp->getParent()->setMouseOpaque(TRUE);
		
		// shrink to smaller layout
		floaterp->reshape(floaterp->mRect.getWidth(), 410);

		// can see and interact with fullscreen preview now
		if (previewp)
		{
			previewp->setVisible(TRUE);
			previewp->setEnabled(TRUE);
		}

		//RN: freeze all avatars
		LLCharacter* avatarp;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			iter != LLCharacter::sInstances.end(); ++iter)
		{
			avatarp = *iter;
			sInstance->impl.mAvatarPauseHandles.push_back(avatarp->requestPause());
		}

		// freeze everything else
		gSavedSettings.setBOOL("FreezeTime", TRUE);

		if (gToolMgr->getCurrentToolset() != gCameraToolset)
		{
			sInstance->impl.mLastToolset = gToolMgr->getCurrentToolset();
			if (gToolMgr)
			{
				gToolMgr->setCurrentToolset(gCameraToolset);
			}
		}
	}
	else // turning off freeze frame mode
	{
		floaterp->getParent()->setMouseOpaque(FALSE);
		floaterp->reshape(floaterp->mRect.getWidth(), 510);
		if (previewp)
		{
			previewp->setVisible(FALSE);
			previewp->setEnabled(FALSE);
		}

		//RN: thaw all avatars
		sInstance->impl.mAvatarPauseHandles.clear();

		// thaw everything else
		gSavedSettings.setBOOL("FreezeTime", FALSE);

		// restore last tool (e.g. pie menu, etc)
		if (sInstance->impl.mLastToolset)
		{
			if (gToolMgr)
			{
				gToolMgr->setCurrentToolset(sInstance->impl.mLastToolset);
			}
		}
	}
}


// static
void LLFloaterSnapshot::Impl::updateControls(LLFloaterSnapshot* floater)
{
	LLRadioGroup* snapshot_type_radio = LLUICtrlFactory::getRadioGroupByName(floater, "snapshot_type_radio");
	snapshot_type_radio->setSelectedIndex(gSavedSettings.getS32("LastSnapshotType"));
	LLSnapshotLivePreview::ESnapshotType shot_type = getTypeIndex(floater);
	LLViewerWindow::ESnapshotType layer_type = getLayerType(floater);

	floater->childSetVisible("postcard_size_combo", FALSE);
	floater->childSetVisible("texture_size_combo", FALSE);
	floater->childSetVisible("local_size_combo", FALSE);

	LLComboBox* combo;
	combo = LLUICtrlFactory::getComboBoxByName(floater, "postcard_size_combo");
	if (combo) combo->selectNthItem(gSavedSettings.getS32("SnapshotPostcardLastResolution"));
	combo = LLUICtrlFactory::getComboBoxByName(floater, "texture_size_combo");
	if (combo) combo->selectNthItem(gSavedSettings.getS32("SnapshotTextureLastResolution"));
	combo = LLUICtrlFactory::getComboBoxByName(floater, "local_size_combo");
	if (combo) combo->selectNthItem(gSavedSettings.getS32("SnapshotLocalLastResolution"));


	floater->childSetVisible("upload_btn", FALSE);
	floater->childSetVisible("send_btn", FALSE);
	floater->childSetVisible("save_btn", FALSE);
	
	switch(shot_type)
	{
	  case LLSnapshotLivePreview::SNAPSHOT_POSTCARD:
		layer_type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		floater->childSetValue("layer_types", "colors");
		floater->childSetEnabled("layer_types", FALSE);
		floater->childSetEnabled("image_quality_slider", TRUE);
		setResolution(floater, "postcard_size_combo");
		floater->childSetVisible("send_btn", TRUE);
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:
		layer_type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		floater->childSetValue("layer_types", "colors");
		floater->childSetEnabled("layer_types", FALSE);
		floater->childSetEnabled("image_quality_slider", FALSE);
		setResolution(floater, "texture_size_combo");
		floater->childSetVisible("upload_btn", TRUE);
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_BITMAP:
		floater->childSetEnabled("layer_types", TRUE);
		floater->childSetEnabled("image_quality_slider", FALSE);
		setResolution(floater, "local_size_combo");
		floater->childSetVisible("save_btn", TRUE);
		break;
	  default:
		break;
	}
	LLSnapshotLivePreview* previewp = getPreviewView(floater);
	if (previewp)
	{
		previewp->setSnapshotType(shot_type);
		previewp->setSnapshotBufferType(layer_type);
	}
}

// static
void LLFloaterSnapshot::Impl::checkAutoSnapshot(LLSnapshotLivePreview* previewp)
{
	if (previewp)
	{
		previewp->updateSnapshot(gSavedSettings.getBOOL("AutoSnapshot"));
	}
}

// static
void LLFloaterSnapshot::Impl::onClickDiscard(void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		view->close();
	}
}

// static
void LLFloaterSnapshot::Impl::onClickKeep(void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	
	if (previewp)
	{
		BOOL succeeded = TRUE; // Only used for saveLocal for now

		if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_POSTCARD)
		{
			LLFloaterPostcard* floater = previewp->savePostcard();
			// if still in snapshot mode, put postcard floater in snapshot floaterview
			// and link it to snapshot floater
			if (!gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
			{
				gFloaterView->removeChild(floater);
				gSnapshotFloaterView->addChild(floater);
				view->addDependentFloater(floater, FALSE);
			}
		}
		else if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE)
		{
			previewp->saveTexture();
		}
		else
		{
			succeeded = previewp->saveLocal();
		}

		if (gSavedSettings.getBOOL("CloseSnapshotOnKeep"))
		{
			view->close();
			// only plays sound and anim when keeping a snapshot, and closing the snapshot UI,
			// and only if the save succeeded (i.e. was not canceled)
			if (succeeded)
			{
				gViewerWindow->playSnapshotAnimAndSound();
			}
		}
		else
		{
			checkAutoSnapshot(previewp);
		}
	}
}

// static
void LLFloaterSnapshot::Impl::onClickNewSnapshot(void* data)
{
	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterSnapshot *)data);
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (previewp && view)
	{
		previewp->updateSnapshot(TRUE);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickAutoSnap(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "AutoSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
	}
}

// static
void LLFloaterSnapshot::Impl::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderUIInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
	}
}

// static
void LLFloaterSnapshot::Impl::onClickHUDCheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderHUDInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
	}
}

// static
void LLFloaterSnapshot::Impl::onClickKeepOpenCheck(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;

	gSavedSettings.setBOOL( "CloseSnapshotOnKeep", !check->get() );
}

// static
void LLFloaterSnapshot::Impl::onClickKeepAspectCheck(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "KeepAspectForSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
	}
}

// static
void LLFloaterSnapshot::Impl::onCommitQuality(LLUICtrl* ctrl, void* data)
{
	LLSliderCtrl* slider = (LLSliderCtrl*)ctrl;
	S32 quality_val = llfloor((F32)slider->getValue().asReal());

	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterSnapshot *)data);
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
	checkAutoSnapshot(previewp);
}

// static
void LLFloaterSnapshot::Impl::onCommitFreezeFrame(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !check_box)
	{
		return;
	}

	gSavedSettings.setBOOL("UseFreezeFrame", check_box->get());

	updateLayout(view);
}

// static
void LLFloaterSnapshot::Impl::onCommitResolution(LLUICtrl* ctrl, void* data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !combobox)
	{
		return;
	}

	// save off all selected resolution values
	LLComboBox* combo;
	combo = LLUICtrlFactory::getComboBoxByName(view, "postcard_size_combo");
	gSavedSettings.setS32("SnapshotPostcardLastResolution", combo->getCurrentIndex());
	combo = LLUICtrlFactory::getComboBoxByName(view, "texture_size_combo");
	gSavedSettings.setS32("SnapshotTextureLastResolution", combo->getCurrentIndex());
	combo = LLUICtrlFactory::getComboBoxByName(view, "local_size_combo");
	gSavedSettings.setS32("SnapshotLocalLastResolution", combo->getCurrentIndex());

	std::string sdstring = combobox->getSimpleSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream);
		
	S32 width = sdres[0];
	S32 height = sdres[1];
	
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		if (width == 0 || height == 0)
		{
			previewp->setSize(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		}
		else if (width == -1 || height == -1)
		{
			// load last custom value
			previewp->setSize(gSavedSettings.getS32("LastSnapshotWidth"), gSavedSettings.getS32("LastSnapshotHeight"));
		}
		else
		{
			previewp->setSize(width, height);
		}

		previewp->getSize(width, height);
		view->childSetValue("snapshot_width", width);
		view->childSetValue("snapshot_height", height);
		// hide old preview as the aspect ratio could be wrong
		checkAutoSnapshot(previewp);
	}
}

// static
void LLFloaterSnapshot::Impl::onCommitLayerTypes(LLUICtrl* ctrl, void*data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;

	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (view)
	{
		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if (previewp)
		{
			previewp->setSnapshotBufferType((LLViewerWindow::ESnapshotType)combobox->getCurrentIndex());
		}
		checkAutoSnapshot(previewp);
	}
}

//static 
void LLFloaterSnapshot::Impl::onCommitSnapshotType(LLUICtrl* ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		gSavedSettings.setS32("LastSnapshotType", getTypeIndex(view));
		getPreviewView(view)->updateSnapshot(TRUE);
		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::comboSetCustom(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(floater, comboname);
	if (combo)
	{
		combo->setCurrentByIndex(combo->getItemCount() - 1);
	}
}

//static
void LLFloaterSnapshot::Impl::onCommitCustomResolution(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;		
	if (view)
	{
		S32 w = llfloor((F32)view->childGetValue("snapshot_width").asReal());
		S32 h = llfloor((F32)view->childGetValue("snapshot_height").asReal());

		gSavedSettings.setS32("LastSnapshotWidth", w);
		gSavedSettings.setS32("LastSnapshotHeight", h);

		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if (previewp)
		{
			S32 curw,curh;
			previewp->getSize(curw, curh);
			
			if (w != curw || h != curh)
			{
				previewp->setSize(w,h);
				checkAutoSnapshot(previewp);
				comboSetCustom(view, "postcard_size_combo");
				comboSetCustom(view, "texture_size_combo");
				comboSetCustom(view, "local_size_combo");
			}
		}
	}
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot()
	: LLFloater("Snapshot Floater"),
	  impl (*(new Impl))
{
}

// Destroys the object
LLFloaterSnapshot::~LLFloaterSnapshot()
{
	if (sInstance == this)
	{
		delete LLView::getViewByHandle(Impl::sPreviewHandle);
		Impl::sPreviewHandle = LLViewHandle::sDeadHandle;
		sInstance = NULL;
	}

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl.mLastToolset)
	{
		if (gToolMgr)
		{
			gToolMgr->setCurrentToolset(impl.mLastToolset);
		}
	}

	delete &impl;
}

BOOL LLFloaterSnapshot::postBuild()
{
	childSetCommitCallback("snapshot_type_radio", Impl::onCommitSnapshotType, this);
	
	childSetAction("new_snapshot_btn", Impl::onClickNewSnapshot, this);

	childSetValue("auto_snapshot_check", gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", Impl::onClickAutoSnap, this);
	
	childSetAction("upload_btn", Impl::onClickKeep, this);
	childSetAction("send_btn", Impl::onClickKeep, this);
	childSetAction("save_btn", Impl::onClickKeep, this);
	childSetAction("discard_btn", Impl::onClickDiscard, this);

	childSetCommitCallback("image_quality_slider", Impl::onCommitQuality, this);
	childSetValue("image_quality_slider", gSavedSettings.getS32("SnapshotQuality"));

	childSetCommitCallback("snapshot_width", Impl::onCommitCustomResolution, this);

	childSetCommitCallback("snapshot_height", Impl::onCommitCustomResolution, this);

	childSetCommitCallback("ui_check", Impl::onClickUICheck, this);

	childSetCommitCallback("hud_check", Impl::onClickHUDCheck, this);
	childSetValue("hud_check", gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	childSetCommitCallback("keep_open_check", Impl::onClickKeepOpenCheck, this);
	childSetValue("keep_open_check", !gSavedSettings.getBOOL("CloseSnapshotOnKeep"));

	childSetCommitCallback("keep_aspect_check", Impl::onClickKeepAspectCheck, this);
	childSetValue("keep_aspect_check", gSavedSettings.getBOOL("KeepAspectForSnapshot"));

	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	childSetValue("layer_types", "colors");
	childSetEnabled("layer_types", FALSE);

	childSetValue("snapshot_width", gSavedSettings.getS32("LastSnapshotWidth"));
	childSetValue("snapshot_height", gSavedSettings.getS32("LastSnapshotHeight"));

	childSetValue("freeze_frame_check", gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", Impl::onCommitFreezeFrame, this);

	childSetCommitCallback("postcard_size_combo", Impl::onCommitResolution, this);
	childSetCommitCallback("texture_size_combo", Impl::onCommitResolution, this);
	childSetCommitCallback("local_size_combo", Impl::onCommitResolution, this);

	// create preview window
	LLRect full_screen_rect = sInstance->getRootView()->getRect();
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(full_screen_rect);
	sInstance->getRootView()->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	sInstance->getRootView()->addChild(previewp);
	sInstance->getRootView()->addChild(gSnapshotFloaterView);

	Impl::sPreviewHandle = previewp->mViewHandle;

	impl.updateControls(this);
	
    return TRUE;
}

void LLFloaterSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = impl.getPreviewView(this);

	if (previewp && previewp->isSnapshotActive())
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	if(getVisible() && !mMinimized)
	{
		if (previewp && previewp->getDataSize() > 0)
		{
			LLLocale locale(LLLocale::USER_LOCALE);

			LLString bytes_string;
			if (previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_POSTCARD && 
				previewp->getDataSize() > MAX_POSTCARD_DATASIZE)
			{
				childSetColor("file_size_label", LLColor4::red);
				childSetEnabled("send_btn", FALSE);
			}
			else
			{
				childSetColor("file_size_label", gColors.getColor( "LabelTextColor" ));
				childSetEnabled("send_btn", previewp->getSnapshotUpToDate());
			}
			
			if (previewp->getSnapshotUpToDate())
			{
				LLString bytes_string;
				gResMgr->getIntegerString(bytes_string, previewp->getDataSize());
				childSetTextArg("file_size_label", "[SIZE]", bytes_string);
			}
			else
			{
				childSetTextArg("file_size_label", "[SIZE]", childGetText("unknwon"));
				childSetColor("file_size_label", gColors.getColor( "LabelTextColor" ));
			}
			childSetEnabled("upload_btn", previewp->getSnapshotUpToDate());
			childSetEnabled("save_btn", previewp->getSnapshotUpToDate());

		}
		else
		{
			childSetTextArg("file_size_label", "[SIZE]", LLString("???"));
			childSetEnabled("upload_btn", FALSE);
			childSetEnabled("send_btn", FALSE);
			childSetEnabled("save_btn", FALSE);
		}

		BOOL ui_in_snapshot = gSavedSettings.getBOOL("RenderUIInSnapshot");
		childSetValue("ui_check", ui_in_snapshot);
		childSetToolTip("ui_check", "If selected shows the UI in the snapshot");
	}

	LLFloater::draw();

	// draw snapshot thumbnail if not in fullscreen preview mode
	if (!gSavedSettings.getBOOL("UseFreezeFrame") && previewp && previewp->getCurrentImage() && previewp->getSnapshotUpToDate())
	{
		F32 aspect = previewp->getImageAspect();
		// UI size for thumbnail
		S32 max_width = mRect.getWidth() - 20;
		S32 max_height = 90;

		S32 img_render_width = 0;
		S32 img_render_height = 0;
		if (aspect > max_width / max_height)
		{
			// image too wide, shrink to width
			img_render_width = max_width;
			img_render_height = llround((F32)max_width / aspect);
		}
		else
		{
			// image too tall, shrink to height
			img_render_height = max_height;
			img_render_width = llround((F32)max_height * aspect);
		}
		S32 image_width, image_height;
		previewp->getSize(image_width, image_height);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		{
			// handle case where image is only a portion of image buffer
			if (!previewp->isImageScaled())
			{
				glScalef(llmin(1.f, (F32)image_width / (F32)previewp->getCurrentImage()->getWidth()), llmin(1.f, (F32)image_height / (F32)previewp->getCurrentImage()->getHeight()), 1.f);
			}
			glMatrixMode(GL_MODELVIEW);
			gl_draw_scaled_image((mRect.getWidth() - img_render_width) / 2, 35 + (max_height - img_render_height) / 2, img_render_width, img_render_height, previewp->getCurrentImage(), LLColor4::white);
		}
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
}

void LLFloaterSnapshot::onClose(bool app_quitting)
{
	gSnapshotFloaterView->setEnabled(FALSE);
	destroy();
}

// static
void LLFloaterSnapshot::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterSnapshot();

		gUICtrlFactory->buildFloater(sInstance, "floater_snapshot.xml", NULL, FALSE);
		//move snapshot floater to special purpose snapshotfloaterview
		gFloaterView->removeChild(sInstance);
		gSnapshotFloaterView->addChild(sInstance);

		sInstance->impl.updateLayout(sInstance);
	}
	
	sInstance->open();		/* Flawfinder: ignore */
	sInstance->focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(sInstance, FALSE);
}

void LLFloaterSnapshot::hide(void*)
{
	if (sInstance && !sInstance->isDead())
	{
		sInstance->close();
	}
}

//static 
void LLFloaterSnapshot::update()
{
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		 iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		LLSnapshotLivePreview::onIdle(*iter);
	}
}

//============================================================================

LLSnapshotFloaterView::LLSnapshotFloaterView( const LLString& name, const LLRect& rect ) : LLFloaterView(name, rect)
{
	setMouseOpaque(TRUE);
	setEnabled(FALSE);
}

LLSnapshotFloaterView::~LLSnapshotFloaterView()
{
}

BOOL LLSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleKey(key, mask, called_from_parent);
	}

	if (!getEnabled())
	{
		return FALSE;
	}
	else
	{
		if (called_from_parent)
		{
			// pass all keystrokes down
			LLFloaterView::handleKey(key, mask, called_from_parent);
		}
		else
		{
			// bounce keystrokes back down
			LLFloaterView::handleKey(key, mask, TRUE);
		}
		return TRUE;
	}
}

BOOL LLSnapshotFloaterView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleMouseDown(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseDown(x, y, mask) == NULL)
	{
		gToolMgr->getCurrentTool()->handleMouseDown( x, y, mask );
	}
	return TRUE;
}

BOOL LLSnapshotFloaterView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleMouseUp(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseUp(x, y, mask) == NULL)
	{
		gToolMgr->getCurrentTool()->handleMouseUp( x, y, mask );
	}
	return TRUE;
}

BOOL LLSnapshotFloaterView::handleHover(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleHover(x, y, mask);
	}	
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleHover(x, y, mask) == NULL)
	{
		gToolMgr->getCurrentTool()->handleHover( x, y, mask );
	}
	return TRUE;
}
