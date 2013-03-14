/** 
 * @file llfloatersnapshot.cpp
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"

#include "llfloaterreg.h"

// Viewer includes
#include "llagent.h"
#include "llagentcamera.h"
#include "llcallbacklist.h"
#include "llcriticaldamp.h"
#include "llfloaterperms.h"
#include "llui.h"
#include "llfocusmgr.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "lleconomy.h"
#include "lllandmarkactions.h"
#include "llpanelsnapshot.h"
#include "llsidetraypanelcontainer.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerstats.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llcheckboxctrl.h"
#include "llslurl.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llwebsharing.h"
#include "llworld.h"
#include "llagentui.h"

// Linden library includes
#include "llfontgl.h"
#include "llsys.h"
#include "llrender.h"
#include "v3dmath.h"
#include "llmath.h"
#include "lldir.h"
#include "llsdserialize.h"
#include "llgl.h"
#include "llglheaders.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "lllocalcliprect.h"
#include "llnotificationsutil.h"
#include "llpostcard.h"
#include "llresmgr.h"		// LLLocale
#include "llvfile.h"
#include "llvfs.h"
#include "llwebprofile.h"
#include "llwindow.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
LLUICtrl* LLFloaterSnapshot::sThumbnailPlaceholder = NULL;
LLSnapshotFloaterView* gSnapshotFloaterView = NULL;

const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;

F32 SHINE_TIME = 0.5f;
F32 SHINE_WIDTH = 0.6f;
F32 SHINE_OPACITY = 0.3f;
F32 FALL_TIME = 0.6f;
S32 BORDER_WIDTH = 6;

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const S32 MAX_TEXTURE_SIZE = 512 ; //max upload texture size 512 * 512

static LLDefaultChildRegistry::Register<LLSnapshotFloaterView> r("snapshot_floater_view");

///----------------------------------------------------------------------------
/// Class LLSnapshotLivePreview 
///----------------------------------------------------------------------------
class LLSnapshotLivePreview : public LLView
{
	LOG_CLASS(LLSnapshotLivePreview);
public:
	enum ESnapshotType
	{
		SNAPSHOT_POSTCARD,
		SNAPSHOT_TEXTURE,
		SNAPSHOT_LOCAL,
		SNAPSHOT_WEB
	};


	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Params()
		{
			name = "snapshot_live_preview";
			mouse_opaque = false;
		}
	};


	LLSnapshotLivePreview(const LLSnapshotLivePreview::Params& p);
	~LLSnapshotLivePreview();

	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	
	void setSize(S32 w, S32 h);
	void setWidth(S32 w) { mWidth[mCurImageIndex] = w; }
	void setHeight(S32 h) { mHeight[mCurImageIndex] = h; }
	void getSize(S32& w, S32& h) const;
	S32 getWidth() const { return mWidth[mCurImageIndex]; }
	S32 getHeight() const { return mHeight[mCurImageIndex]; }
	S32 getDataSize() const { return mDataSize; }
	void setMaxImageSize(S32 size) ;
	S32  getMaxImageSize() {return mMaxImageSize ;}
	
	ESnapshotType getSnapshotType() const { return mSnapshotType; }
	LLFloaterSnapshot::ESnapshotFormat getSnapshotFormat() const { return mSnapshotFormat; }
	BOOL getSnapshotUpToDate() const { return mSnapshotUpToDate; }
	BOOL isSnapshotActive() { return mSnapshotActive; }
	LLViewerTexture* getThumbnailImage() const { return mThumbnailImage ; }
	S32  getThumbnailWidth() const { return mThumbnailWidth ; }
	S32  getThumbnailHeight() const { return mThumbnailHeight ; }
	BOOL getThumbnailLock() const { return mThumbnailUpdateLock ; }
	BOOL getThumbnailUpToDate() const { return mThumbnailUpToDate ;}
	LLViewerTexture* getCurrentImage();
	F32 getImageAspect();
	F32 getAspect() ;
	const LLRect& getImageRect() const { return mImageRect[mCurImageIndex]; }
	BOOL isImageScaled() const { return mImageScaled[mCurImageIndex]; }
	void setImageScaled(BOOL scaled) { mImageScaled[mCurImageIndex] = scaled; }
	const LLVector3d& getPosTakenGlobal() const { return mPosTakenGlobal; }
	
	void setSnapshotType(ESnapshotType type) { mSnapshotType = type; }
	void setSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat type) { mSnapshotFormat = type; }
	void setSnapshotQuality(S32 quality);
	void setSnapshotBufferType(LLViewerWindow::ESnapshotType type) { mSnapshotBufferType = type; }
	void updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail = FALSE, F32 delay = 0.f);
	void saveWeb();
	void saveTexture();
	BOOL saveLocal();

	LLPointer<LLImageFormatted>	getFormattedImage() const { return mFormattedImage; }
	LLPointer<LLImageRaw>		getEncodedImage() const { return mPreviewImageEncoded; }

	/// Sets size of preview thumbnail image and thhe surrounding rect.
	BOOL setThumbnailImageSize() ;
	void generateThumbnailImage(BOOL force_update = FALSE) ;
	void resetThumbnailImage() { mThumbnailImage = NULL ; }
	void drawPreviewRect(S32 offset_x, S32 offset_y) ;

	// Returns TRUE when snapshot generated, FALSE otherwise.
	static BOOL onIdle( void* snapshot_preview );

	// callback for region name resolve
	void regionNameCallback(LLImageJPEG* snapshot, LLSD& metadata, const std::string& name, S32 x, S32 y, S32 z);

private:
	LLColor4					mColor;
	LLPointer<LLViewerTexture>	mViewerImage[2]; //used to represent the scene when the frame is frozen.
	LLRect						mImageRect[2];
	S32							mWidth[2];
	S32							mHeight[2];
	BOOL						mImageScaled[2];
	S32                         mMaxImageSize ;
	
	//thumbnail image
	LLPointer<LLViewerTexture>	mThumbnailImage ;
	S32                         mThumbnailWidth ;
	S32                         mThumbnailHeight ;
	LLRect                      mPreviewRect ;
	BOOL                        mThumbnailUpdateLock ;
	BOOL                        mThumbnailUpToDate ;

	S32							mCurImageIndex;
	LLPointer<LLImageRaw>		mPreviewImage;
	LLPointer<LLImageRaw>		mPreviewImageEncoded;
	LLPointer<LLImageFormatted>	mFormattedImage;
	LLFrameTimer				mSnapshotDelayTimer;
	S32							mShineCountdown;
	LLFrameTimer				mShineAnimTimer;
	F32							mFlashAlpha;
	BOOL						mNeedsFlash;
	LLVector3d					mPosTakenGlobal;
	S32							mSnapshotQuality;
	S32							mDataSize;
	ESnapshotType				mSnapshotType;
	LLFloaterSnapshot::ESnapshotFormat	mSnapshotFormat;
	BOOL						mSnapshotUpToDate;
	LLFrameTimer				mFallAnimTimer;
	LLVector3					mCameraPos;
	LLQuaternion				mCameraRot;
	BOOL						mSnapshotActive;
	LLViewerWindow::ESnapshotType mSnapshotBufferType;

public:
	static std::set<LLSnapshotLivePreview*> sList;
	BOOL                        mKeepAspectRatio ;
};

std::set<LLSnapshotLivePreview*> LLSnapshotLivePreview::sList;

LLSnapshotLivePreview::LLSnapshotLivePreview (const LLSnapshotLivePreview::Params& p) 
:	LLView(p),
	mColor(1.f, 0.f, 0.f, 0.5f), 
	mCurImageIndex(0),
	mPreviewImage(NULL),
	mThumbnailImage(NULL) ,
	mThumbnailWidth(0),
	mThumbnailHeight(0),
	mPreviewImageEncoded(NULL),
	mFormattedImage(NULL),
	mShineCountdown(0),
	mFlashAlpha(0.f),
	mNeedsFlash(TRUE),
	mSnapshotQuality(gSavedSettings.getS32("SnapshotQuality")),
	mDataSize(0),
	mSnapshotType(SNAPSHOT_POSTCARD),
	mSnapshotFormat(LLFloaterSnapshot::ESnapshotFormat(gSavedSettings.getS32("SnapshotFormat"))),
	mSnapshotUpToDate(FALSE),
	mCameraPos(LLViewerCamera::getInstance()->getOrigin()),
	mCameraRot(LLViewerCamera::getInstance()->getQuaternion()),
	mSnapshotActive(FALSE),
	mSnapshotBufferType(LLViewerWindow::SNAPSHOT_TYPE_COLOR)
{
	setSnapshotQuality(gSavedSettings.getS32("SnapshotQuality"));
	mSnapshotDelayTimer.setTimerExpirySec(0.0f);
	mSnapshotDelayTimer.start();
// 	gIdleCallbacks.addFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.insert(this);
	setFollowsAll();
	mWidth[0] = gViewerWindow->getWindowWidthRaw();
	mWidth[1] = gViewerWindow->getWindowWidthRaw();
	mHeight[0] = gViewerWindow->getWindowHeightRaw();
	mHeight[1] = gViewerWindow->getWindowHeightRaw();
	mImageScaled[0] = FALSE;
	mImageScaled[1] = FALSE;

	mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;
	mThumbnailUpdateLock = FALSE ;
	mThumbnailUpToDate   = FALSE ;
}

LLSnapshotLivePreview::~LLSnapshotLivePreview()
{
	// delete images
	mPreviewImage = NULL;
	mPreviewImageEncoded = NULL;
	mFormattedImage = NULL;

// 	gIdleCallbacks.deleteFunction( &LLSnapshotLivePreview::onIdle, (void*)this );
	sList.erase(this);
}

void LLSnapshotLivePreview::setMaxImageSize(S32 size) 
{
	if(size < MAX_SNAPSHOT_IMAGE_SIZE)
	{
		mMaxImageSize = size;
	}
	else
	{
		mMaxImageSize = MAX_SNAPSHOT_IMAGE_SIZE ;
	}
}

LLViewerTexture* LLSnapshotLivePreview::getCurrentImage()
{
	return mViewerImage[mCurImageIndex];
}

F32 LLSnapshotLivePreview::getAspect()
{
	F32 image_aspect_ratio = ((F32)getWidth()) / ((F32)getHeight());
	F32 window_aspect_ratio = ((F32)getRect().getWidth()) / ((F32)getRect().getHeight());

	if (!mKeepAspectRatio)//gSavedSettings.getBOOL("KeepAspectForSnapshot"))
	{
		return image_aspect_ratio;
	}
	else
	{
		return window_aspect_ratio;
	}
}

F32 LLSnapshotLivePreview::getImageAspect()
{
	if (!getCurrentImage())
	{
		return 0.f;
	}

	return getAspect() ;	
}

void LLSnapshotLivePreview::updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail, F32 delay) 
{
	// Invalidate current image.
	lldebugs << "updateSnapshot: mSnapshotUpToDate = " << getSnapshotUpToDate() << llendl;
	if (getSnapshotUpToDate())
	{
		S32 old_image_index = mCurImageIndex;
		mCurImageIndex = (mCurImageIndex + 1) % 2; 
		setSize(mWidth[old_image_index], mHeight[old_image_index]);
		mFallAnimTimer.start();		
	}
	mSnapshotUpToDate = FALSE; 		

	// Update snapshot source rect depending on whether we keep the aspect ratio.
	LLRect& rect = mImageRect[mCurImageIndex];
	rect.set(0, getRect().getHeight(), getRect().getWidth(), 0);

	F32 image_aspect_ratio = ((F32)getWidth()) / ((F32)getHeight());
	F32 window_aspect_ratio = ((F32)getRect().getWidth()) / ((F32)getRect().getHeight());

	if (mKeepAspectRatio)//gSavedSettings.getBOOL("KeepAspectForSnapshot"))
	{
		if (image_aspect_ratio > window_aspect_ratio)
		{
			// trim off top and bottom
			S32 new_height = llround((F32)getRect().getWidth() / image_aspect_ratio); 
			rect.mBottom += (getRect().getHeight() - new_height) / 2;
			rect.mTop -= (getRect().getHeight() - new_height) / 2;
		}
		else if (image_aspect_ratio < window_aspect_ratio)
		{
			// trim off left and right
			S32 new_width = llround((F32)getRect().getHeight() * image_aspect_ratio); 
			rect.mLeft += (getRect().getWidth() - new_width) / 2;
			rect.mRight -= (getRect().getWidth() - new_width) / 2;
		}
	}

	// Stop shining animation.
	mShineAnimTimer.stop();

	// Update snapshot if requested.
	if (new_snapshot)
	{
		mSnapshotDelayTimer.start();
		mSnapshotDelayTimer.setTimerExpirySec(delay);
		LLFloaterSnapshot::preUpdate();
	}

	// Update thumbnail if requested.
	if(new_thumbnail)
	{
		mThumbnailUpToDate = FALSE ;
	}
}

void LLSnapshotLivePreview::setSnapshotQuality(S32 quality)
{
	llclamp(quality, 0, 100);
	if (quality != mSnapshotQuality)
	{
		mSnapshotQuality = quality;
		gSavedSettings.setS32("SnapshotQuality", quality);
		mSnapshotUpToDate = FALSE;
	}
}

void LLSnapshotLivePreview::drawPreviewRect(S32 offset_x, S32 offset_y)
{
	F32 line_width ; 
	glGetFloatv(GL_LINE_WIDTH, &line_width) ;
	glLineWidth(2.0f * line_width) ;
	LLColor4 color(0.0f, 0.0f, 0.0f, 1.0f) ;
	gl_rect_2d( mPreviewRect.mLeft + offset_x, mPreviewRect.mTop + offset_y,
				mPreviewRect.mRight + offset_x, mPreviewRect.mBottom + offset_y, color, FALSE ) ;
	glLineWidth(line_width) ;

	//draw four alpha rectangles to cover areas outside of the snapshot image
	if(!mKeepAspectRatio)
	{
		LLColor4 alpha_color(0.5f, 0.5f, 0.5f, 0.8f) ;
		S32 dwl = 0, dwr = 0 ;
		if(mThumbnailWidth > mPreviewRect.getWidth())
		{
			dwl = (mThumbnailWidth - mPreviewRect.getWidth()) >> 1 ;
			dwr = mThumbnailWidth - mPreviewRect.getWidth() - dwl ;

			gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y,
				mPreviewRect.mLeft + offset_x, mPreviewRect.mBottom + offset_y, alpha_color, TRUE ) ;
			gl_rect_2d( mPreviewRect.mRight + offset_x, mPreviewRect.mTop + offset_y,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y, alpha_color, TRUE ) ;
		}

		if(mThumbnailHeight > mPreviewRect.getHeight())
		{
			S32 dh = (mThumbnailHeight - mPreviewRect.getHeight()) >> 1 ;
			gl_rect_2d(mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mBottom + offset_y ,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mBottom + offset_y - dh, alpha_color, TRUE ) ;

			dh = mThumbnailHeight - mPreviewRect.getHeight() - dh ;
			gl_rect_2d( mPreviewRect.mLeft + offset_x - dwl, mPreviewRect.mTop + offset_y + dh,
				mPreviewRect.mRight + offset_x + dwr, mPreviewRect.mTop + offset_y, alpha_color, TRUE ) ;
		}
	}
}

//called when the frame is frozen.
void LLSnapshotLivePreview::draw()
{
	if (getCurrentImage() &&
	    mPreviewImageEncoded.notNull() &&
	    getSnapshotUpToDate())
	{
		LLColor4 bg_color(0.f, 0.f, 0.3f, 0.4f);
		gl_rect_2d(getRect(), bg_color);
		const LLRect& rect = getImageRect();
		LLRect shadow_rect = rect;
		shadow_rect.stretch(BORDER_WIDTH);
		gl_drop_shadow(shadow_rect.mLeft, shadow_rect.mTop, shadow_rect.mRight, shadow_rect.mBottom, LLColor4(0.f, 0.f, 0.f, mNeedsFlash ? 0.f :0.5f), 10);

		LLColor4 image_color(1.f, 1.f, 1.f, 1.f);
		gGL.color4fv(image_color.mV);
		gGL.getTexUnit(0)->bind(getCurrentImage());
		// calculate UV scale
		F32 uv_width = isImageScaled() ? 1.f : llmin((F32)getWidth() / (F32)getCurrentImage()->getWidth(), 1.f);
		F32 uv_height = isImageScaled() ? 1.f : llmin((F32)getHeight() / (F32)getCurrentImage()->getHeight(), 1.f);
		gGL.pushMatrix();
		{
			gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom, 0.f);
			gGL.begin(LLRender::QUADS);
			{
				gGL.texCoord2f(uv_width, uv_height);
				gGL.vertex2i(rect.getWidth(), rect.getHeight() );

				gGL.texCoord2f(0.f, uv_height);
				gGL.vertex2i(0, rect.getHeight() );

				gGL.texCoord2f(0.f, 0.f);
				gGL.vertex2i(0, 0);

				gGL.texCoord2f(uv_width, 0.f);
				gGL.vertex2i(rect.getWidth(), 0);
			}
			gGL.end();
		}
		gGL.popMatrix();

		gGL.color4f(1.f, 1.f, 1.f, mFlashAlpha);
		gl_rect_2d(getRect());
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

		// Draw shining animation if appropriate.
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
			lldebugs << "Drawing shining animation" << llendl;
			F32 shine_interp = llmin(1.f, mShineAnimTimer.getElapsedTimeF32() / SHINE_TIME);
			
			// draw "shine" effect
			LLLocalClipRect clip(getLocalRect());
			{
				// draw diagonal stripe with gradient that passes over screen
				S32 x1 = gViewerWindow->getWindowWidthScaled() * llround((clamp_rescale(shine_interp, 0.f, 1.f, -1.f - SHINE_WIDTH, 1.f)));
				S32 x2 = x1 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 x3 = x2 + llround(gViewerWindow->getWindowWidthScaled() * SHINE_WIDTH);
				S32 y1 = 0;
				S32 y2 = gViewerWindow->getWindowHeightScaled();

				gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
				gGL.begin(LLRender::QUADS);
				{
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x1, y1);
					gGL.vertex2i(x1 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x2, y1);

					gGL.color4f(1.f, 1.f, 1.f, SHINE_OPACITY);
					gGL.vertex2i(x2, y1);
					gGL.vertex2i(x2 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.color4f(1.f, 1.f, 1.f, 0.f);
					gGL.vertex2i(x3 + gViewerWindow->getWindowWidthScaled(), y2);
					gGL.vertex2i(x3, y1);
				}
				gGL.end();
			}

			// if we're at the end of the animation, stop
			if (shine_interp >= 1.f)
			{
				mShineAnimTimer.stop();
			}
		}
	}

	// draw framing rectangle
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gGL.color4f(1.f, 1.f, 1.f, 1.f);
		const LLRect& outline_rect = getImageRect();
		gGL.begin(LLRender::QUADS);
		{
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mLeft, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mLeft, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mLeft - BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);

			gGL.vertex2i(outline_rect.mRight, outline_rect.mBottom);
			gGL.vertex2i(outline_rect.mRight, outline_rect.mTop);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mTop + BORDER_WIDTH);
			gGL.vertex2i(outline_rect.mRight + BORDER_WIDTH, outline_rect.mBottom - BORDER_WIDTH);
		}
		gGL.end();
	}

	// draw old image dropping away
	if (mFallAnimTimer.getStarted())
	{
		S32 old_image_index = (mCurImageIndex + 1) % 2;
		if (mViewerImage[old_image_index].notNull() && mFallAnimTimer.getElapsedTimeF32() < FALL_TIME)
		{
			lldebugs << "Drawing fall animation" << llendl;
			F32 fall_interp = mFallAnimTimer.getElapsedTimeF32() / FALL_TIME;
			F32 alpha = clamp_rescale(fall_interp, 0.f, 1.f, 0.8f, 0.4f);
			LLColor4 image_color(1.f, 1.f, 1.f, alpha);
			gGL.color4fv(image_color.mV);
			gGL.getTexUnit(0)->bind(mViewerImage[old_image_index]);
			// calculate UV scale
			// *FIX get this to work with old image
			BOOL rescale = !mImageScaled[old_image_index] && mViewerImage[mCurImageIndex].notNull();
			F32 uv_width = rescale ? llmin((F32)mWidth[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getWidth(), 1.f) : 1.f;
			F32 uv_height = rescale ? llmin((F32)mHeight[old_image_index] / (F32)mViewerImage[mCurImageIndex]->getHeight(), 1.f) : 1.f;
			gGL.pushMatrix();
			{
				LLRect& rect = mImageRect[old_image_index];
				gGL.translatef((F32)rect.mLeft, (F32)rect.mBottom - llround(getRect().getHeight() * 2.f * (fall_interp * fall_interp)), 0.f);
				gGL.rotatef(-45.f * fall_interp, 0.f, 0.f, 1.f);
				gGL.begin(LLRender::QUADS);
				{
					gGL.texCoord2f(uv_width, uv_height);
					gGL.vertex2i(rect.getWidth(), rect.getHeight() );

					gGL.texCoord2f(0.f, uv_height);
					gGL.vertex2i(0, rect.getHeight() );

					gGL.texCoord2f(0.f, 0.f);
					gGL.vertex2i(0, 0);

					gGL.texCoord2f(uv_width, 0.f);
					gGL.vertex2i(rect.getWidth(), 0);
				}
				gGL.end();
			}
			gGL.popMatrix();
		}
	}
}

/*virtual*/ 
void LLSnapshotLivePreview::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLRect old_rect = getRect();
	LLView::reshape(width, height, called_from_parent);
	if (old_rect.getWidth() != width || old_rect.getHeight() != height)
	{
		lldebugs << "window reshaped, updating thumbnail" << llendl;
		updateSnapshot(FALSE, TRUE);
	}
}

BOOL LLSnapshotLivePreview::setThumbnailImageSize()
{
	if(getWidth() < 10 || getHeight() < 10)
	{
		return FALSE ;
	}
	S32 window_width = gViewerWindow->getWindowWidthRaw() ;
	S32 window_height = gViewerWindow->getWindowHeightRaw() ;

	F32 window_aspect_ratio = ((F32)window_width) / ((F32)window_height);

	// UI size for thumbnail
	// *FIXME: the rect does not change, so maybe there's no need to recalculate max w/h.
	const LLRect& thumbnail_rect = LLFloaterSnapshot::getThumbnailPlaceholderRect();
	S32 max_width = thumbnail_rect.getWidth();
	S32 max_height = thumbnail_rect.getHeight();

	if (window_aspect_ratio > (F32)max_width / max_height)
	{
		// image too wide, shrink to width
		mThumbnailWidth = max_width;
		mThumbnailHeight = llround((F32)max_width / window_aspect_ratio);
	}
	else
	{
		// image too tall, shrink to height
		mThumbnailHeight = max_height;
		mThumbnailWidth = llround((F32)max_height * window_aspect_ratio);
	}
	
	if(mThumbnailWidth > window_width || mThumbnailHeight > window_height)
	{
		return FALSE ;//if the window is too small, ignore thumbnail updating.
	}

	S32 left = 0 , top = mThumbnailHeight, right = mThumbnailWidth, bottom = 0 ;
	if(!mKeepAspectRatio)
	{
		F32 ratio_x = (F32)getWidth() / window_width ;
		F32 ratio_y = (F32)getHeight() / window_height ;

		//if(getWidth() > window_width ||
		//	getHeight() > window_height )
		{
			if(ratio_x > ratio_y)
			{
				top = (S32)(top * ratio_y / ratio_x) ;
			}
			else
			{
				right = (S32)(right * ratio_x / ratio_y) ;
			}			
		}
		//else
		//{
		//	right = (S32)(right * ratio_x) ;
		//	top = (S32)(top * ratio_y) ;
		//}
		left = (S32)((mThumbnailWidth - right) * 0.5f) ;
		bottom = (S32)((mThumbnailHeight - top) * 0.5f) ;
		top += bottom ;
		right += left ;
	}
	mPreviewRect.set(left - 1, top + 1, right + 1, bottom - 1) ;

	return TRUE ;
}

void LLSnapshotLivePreview::generateThumbnailImage(BOOL force_update)
{	
	if(mThumbnailUpdateLock) //in the process of updating
	{
		return ;
	}
	if(getThumbnailUpToDate() && !force_update)//already updated
	{
		return ;
	}
	if(getWidth() < 10 || getHeight() < 10)
	{
		return ;
	}

	////lock updating
	mThumbnailUpdateLock = TRUE ;

	if(!setThumbnailImageSize())
	{
		mThumbnailUpdateLock = FALSE ;
		mThumbnailUpToDate = TRUE ;
		return ;
	}

	if(mThumbnailImage)
	{
		resetThumbnailImage() ;
	}		

	LLPointer<LLImageRaw> raw = new LLImageRaw;
	if(!gViewerWindow->thumbnailSnapshot(raw,
							mThumbnailWidth, mThumbnailHeight,
							gSavedSettings.getBOOL("RenderUIInSnapshot"),
							FALSE,
							mSnapshotBufferType) )								
	{
		raw = NULL ;
	}

	if(raw)
	{
		raw->expandToPowerOfTwo();
		mThumbnailImage = LLViewerTextureManager::getLocalTexture(raw.get(), FALSE); 		
		mThumbnailUpToDate = TRUE ;
	}

	//unlock updating
	mThumbnailUpdateLock = FALSE ;		
}


// Called often. Checks whether it's time to grab a new snapshot and if so, does it.
// Returns TRUE if new snapshot generated, FALSE otherwise.
//static 
BOOL LLSnapshotLivePreview::onIdle( void* snapshot_preview )
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)snapshot_preview;
	if (previewp->getWidth() == 0 || previewp->getHeight() == 0)
	{
		llwarns << "Incorrect dimensions: " << previewp->getWidth() << "x" << previewp->getHeight() << llendl;
		return FALSE;
	}

	// If we're in freeze-frame mode and camera has moved, update snapshot.
	LLVector3 new_camera_pos = LLViewerCamera::getInstance()->getOrigin();
	LLQuaternion new_camera_rot = LLViewerCamera::getInstance()->getQuaternion();
	if (gSavedSettings.getBOOL("FreezeTime") && 
		(new_camera_pos != previewp->mCameraPos || dot(new_camera_rot, previewp->mCameraRot) < 0.995f))
	{
		previewp->mCameraPos = new_camera_pos;
		previewp->mCameraRot = new_camera_rot;
		// request a new snapshot whenever the camera moves, with a time delay
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		lldebugs << "camera moved, updating thumbnail" << llendl;
		previewp->updateSnapshot(
			autosnap, // whether a new snapshot is needed or merely invalidate the existing one
			FALSE, // or if 1st arg is false, whether to produce a new thumbnail image.
			autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f); // shutter delay if 1st arg is true.
	}

	// see if it's time yet to snap the shot and bomb out otherwise.
	previewp->mSnapshotActive = 
		(previewp->mSnapshotDelayTimer.getStarted() &&	previewp->mSnapshotDelayTimer.hasExpired())
		&& !LLToolCamera::getInstance()->hasMouseCapture(); // don't take snapshots while ALT-zoom active
	if ( ! previewp->mSnapshotActive)
	{
		return FALSE;
	}

	// time to produce a snapshot
	previewp->setThumbnailImageSize();

	lldebugs << "producing snapshot" << llendl;
	if (!previewp->mPreviewImage)
	{
		previewp->mPreviewImage = new LLImageRaw;
	}

	if (!previewp->mPreviewImageEncoded)
	{
		previewp->mPreviewImageEncoded = new LLImageRaw;
	}

	previewp->setVisible(FALSE);
	previewp->setEnabled(FALSE);
	
	previewp->getWindow()->incBusyCount();
	previewp->setImageScaled(FALSE);

	// grab the raw image and encode it into desired format
	if(gViewerWindow->rawSnapshot(
							previewp->mPreviewImage,
							previewp->getWidth(),
							previewp->getHeight(),
							previewp->mKeepAspectRatio,//gSavedSettings.getBOOL("KeepAspectForSnapshot"),
							previewp->getSnapshotType() == LLSnapshotLivePreview::SNAPSHOT_TEXTURE,
							gSavedSettings.getBOOL("RenderUIInSnapshot"),
							FALSE,
							previewp->mSnapshotBufferType,
							previewp->getMaxImageSize()))
	{
		previewp->mPreviewImageEncoded->resize(
			previewp->mPreviewImage->getWidth(), 
			previewp->mPreviewImage->getHeight(), 
			previewp->mPreviewImage->getComponents());

		if(previewp->getSnapshotType() == SNAPSHOT_TEXTURE)
		{
			lldebugs << "Encoding new image of format J2C" << llendl;
			LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
			LLPointer<LLImageRaw> scaled = new LLImageRaw(
				previewp->mPreviewImage->getData(),
				previewp->mPreviewImage->getWidth(),
				previewp->mPreviewImage->getHeight(),
				previewp->mPreviewImage->getComponents());
		
			scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
			previewp->setImageScaled(TRUE);
			if (formatted->encode(scaled, 0.f))
			{
				previewp->mDataSize = formatted->getDataSize();
				formatted->decode(previewp->mPreviewImageEncoded, 0);
			}
		}
		else
		{
			// delete any existing image
			previewp->mFormattedImage = NULL;
			// now create the new one of the appropriate format.
			LLFloaterSnapshot::ESnapshotFormat format = previewp->getSnapshotFormat();
			lldebugs << "Encoding new image of format " << format << llendl;

			switch(format)
			{
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG:
				previewp->mFormattedImage = new LLImagePNG(); 
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG:
				previewp->mFormattedImage = new LLImageJPEG(previewp->mSnapshotQuality); 
				break;
			case LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP:
				previewp->mFormattedImage = new LLImageBMP(); 
				break;
			}
			if (previewp->mFormattedImage->encode(previewp->mPreviewImage, 0))
			{
				previewp->mDataSize = previewp->mFormattedImage->getDataSize();
				// special case BMP to copy instead of decode otherwise decode will crash.
				if(format == LLFloaterSnapshot::SNAPSHOT_FORMAT_BMP)
				{
					previewp->mPreviewImageEncoded->copy(previewp->mPreviewImage);
				}
				else
				{
					previewp->mFormattedImage->decode(previewp->mPreviewImageEncoded, 0);
				}
			}
		}

		LLPointer<LLImageRaw> scaled = new LLImageRaw(
			previewp->mPreviewImageEncoded->getData(),
			previewp->mPreviewImageEncoded->getWidth(),
			previewp->mPreviewImageEncoded->getHeight(),
			previewp->mPreviewImageEncoded->getComponents());
		
		if(!scaled->isBufferInvalid())
		{
			// leave original image dimensions, just scale up texture buffer
			if (previewp->mPreviewImageEncoded->getWidth() > 1024 || previewp->mPreviewImageEncoded->getHeight() > 1024)
			{
				// go ahead and shrink image to appropriate power of 2 for display
				scaled->biasedScaleToPowerOfTwo(1024);
				previewp->setImageScaled(TRUE);
			}
			else
			{
				// expand image but keep original image data intact
				scaled->expandToPowerOfTwo(1024, FALSE);
			}

			previewp->mViewerImage[previewp->mCurImageIndex] = LLViewerTextureManager::getLocalTexture(scaled.get(), FALSE);
			LLPointer<LLViewerTexture> curr_preview_image = previewp->mViewerImage[previewp->mCurImageIndex];
			gGL.getTexUnit(0)->bind(curr_preview_image);
			if (previewp->getSnapshotType() != SNAPSHOT_TEXTURE)
			{
				curr_preview_image->setFilteringOption(LLTexUnit::TFO_POINT);
			}
			else
			{
				curr_preview_image->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
			}
			curr_preview_image->setAddressMode(LLTexUnit::TAM_CLAMP);

			previewp->mSnapshotUpToDate = TRUE;
			previewp->generateThumbnailImage(TRUE) ;

			previewp->mPosTakenGlobal = gAgentCamera.getCameraPositionGlobal();
			previewp->mShineCountdown = 4; // wait a few frames to avoid animation glitch due to readback this frame
		}
	}
	previewp->getWindow()->decBusyCount();
	// only show fullscreen preview when in freeze frame mode
	previewp->setVisible(gSavedSettings.getBOOL("UseFreezeFrame"));
	previewp->mSnapshotDelayTimer.stop();
	previewp->mSnapshotActive = FALSE;

	if(!previewp->getThumbnailUpToDate())
	{
		previewp->generateThumbnailImage() ;
	}
	lldebugs << "done creating snapshot" << llendl;
	LLFloaterSnapshot::postUpdate();

	return TRUE;
}

void LLSnapshotLivePreview::setSize(S32 w, S32 h)
{
	lldebugs << "setSize(" << w << ", " << h << ")" << llendl;
	setWidth(w);
	setHeight(h);
}

void LLSnapshotLivePreview::getSize(S32& w, S32& h) const
{
	w = getWidth();
	h = getHeight();
}

void LLSnapshotLivePreview::saveTexture()
{
	lldebugs << "saving texture: " << mPreviewImage->getWidth() << "x" << mPreviewImage->getHeight() << llendl;
	// gen a new uuid for this asset
	LLTransactionID tid;
	tid.generate();
	LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
		
	LLPointer<LLImageJ2C> formatted = new LLImageJ2C;
	LLPointer<LLImageRaw> scaled = new LLImageRaw(mPreviewImage->getData(),
												  mPreviewImage->getWidth(),
												  mPreviewImage->getHeight(),
												  mPreviewImage->getComponents());
	
	scaled->biasedScaleToPowerOfTwo(MAX_TEXTURE_SIZE);
	lldebugs << "scaled texture to " << scaled->getWidth() << "x" << scaled->getHeight() << llendl;

	if (formatted->encode(scaled, 0.0f))
	{
		LLVFile::writeFile(formatted->getData(), formatted->getDataSize(), gVFS, new_asset_id, LLAssetType::AT_TEXTURE);
		std::string pos_string;
		LLAgentUI::buildLocationString(pos_string, LLAgentUI::LOCATION_FORMAT_FULL);
		std::string who_took_it;
		LLAgentUI::buildFullname(who_took_it);
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
		void *userdata = NULL;
		upload_new_resource(tid,	// tid
				    LLAssetType::AT_TEXTURE,
				    "Snapshot : " + pos_string,
				    "Taken by " + who_took_it + " at " + pos_string,
				    0,
				    LLFolderType::FT_SNAPSHOT_CATEGORY,
				    LLInventoryType::IT_SNAPSHOT,
				    PERM_ALL,  // Note: Snapshots to inventory is a special case of content upload
				    LLFloaterPerms::getGroupPerms(), // that is more permissive than other uploads
				    LLFloaterPerms::getEveryonePerms(),
				    "Snapshot : " + pos_string,
				    callback, expected_upload_cost, userdata);
		gViewerWindow->playSnapshotAnimAndSound();
	}
	else
	{
		LLNotificationsUtil::add("ErrorEncodingSnapshot");
		llwarns << "Error encoding snapshot" << llendl;
	}

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_SNAPSHOT_COUNT );
	
	mDataSize = 0;
}

BOOL LLSnapshotLivePreview::saveLocal()
{
	BOOL success = gViewerWindow->saveImageNumbered(mFormattedImage);

	if(success)
	{
		gViewerWindow->playSnapshotAnimAndSound();
	}
	return success;
}

void LLSnapshotLivePreview::saveWeb()
{
	// *FIX: Will break if the window closes because of CloseSnapshotOnKeep!
	// Needs to pass on ownership of the image.
	LLImageJPEG* jpg = dynamic_cast<LLImageJPEG*>(mFormattedImage.get());
	if(!jpg)
	{
		llwarns << "Formatted image not a JPEG" << llendl;
		return;
	}

	LLSD metadata;
	metadata["description"] = getChild<LLLineEditor>("description")->getText();

	LLLandmarkActions::getRegionNameAndCoordsFromPosGlobal(gAgentCamera.getCameraPositionGlobal(),
		boost::bind(&LLSnapshotLivePreview::regionNameCallback, this, jpg, metadata, _1, _2, _3, _4));

	gViewerWindow->playSnapshotAnimAndSound();
}

void LLSnapshotLivePreview::regionNameCallback(LLImageJPEG* snapshot, LLSD& metadata, const std::string& name, S32 x, S32 y, S32 z)
{
	metadata["slurl"] = LLSLURL(name, LLVector3d(x, y, z)).getSLURLString();

	LLWebSharing::instance().shareSnapshot(snapshot, metadata);
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSnapshot::Impl
{
	LOG_CLASS(LLFloaterSnapshot::Impl);
public:
	typedef enum e_status
	{
		STATUS_READY,
		STATUS_WORKING,
		STATUS_FINISHED
	} EStatus;

	Impl()
	:	mAvatarPauseHandles(),
		mLastToolset(NULL),
		mAspectRatioCheckOff(false),
		mNeedRefresh(false),
		mStatus(STATUS_READY)
	{
	}
	~Impl()
	{
		//unpause avatars
		mAvatarPauseHandles.clear();

	}
	static void onClickNewSnapshot(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	//static void onClickAdvanceSnap(LLUICtrl *ctrl, void* data);
	static void onClickMore(void* data) ;
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void applyKeepAspectCheck(LLFloaterSnapshot* view, BOOL checked);
	static void updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update = TRUE);
	static void onCommitFreezeFrame(LLUICtrl* ctrl, void* data);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	static void onImageQualityChange(LLFloaterSnapshot* view, S32 quality_val);
	static void onImageFormatChange(LLFloaterSnapshot* view);
	static void applyCustomResolution(LLFloaterSnapshot* view, S32 w, S32 h);
	static void onSnapshotUploadFinished(bool status);
	static void onSendingPostcardFinished(bool status);
	static BOOL checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value);
	static void setImageSizeSpinnersValues(LLFloaterSnapshot *view, S32 width, S32 height) ;
	static void updateSpinners(LLFloaterSnapshot* view, LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed);

	static LLPanelSnapshot* getActivePanel(LLFloaterSnapshot* floater, bool ok_if_not_found = true);
	static LLSnapshotLivePreview::ESnapshotType getActiveSnapshotType(LLFloaterSnapshot* floater);
	static LLFloaterSnapshot::ESnapshotFormat getImageFormat(LLFloaterSnapshot* floater);
	static LLSpinCtrl* getWidthSpinner(LLFloaterSnapshot* floater);
	static LLSpinCtrl* getHeightSpinner(LLFloaterSnapshot* floater);
	static void enableAspectRatioCheckbox(LLFloaterSnapshot* floater, BOOL enable);
	static void setAspectRatioCheckboxValue(LLFloaterSnapshot* floater, BOOL checked);

	static LLSnapshotLivePreview* getPreviewView(LLFloaterSnapshot *floater);
	static void setResolution(LLFloaterSnapshot* floater, const std::string& comboname);
	static void updateControls(LLFloaterSnapshot* floater);
	static void updateLayout(LLFloaterSnapshot* floater);
	static void setStatus(EStatus status, bool ok = true, const std::string& msg = LLStringUtil::null);
	EStatus getStatus() const { return mStatus; }
	static void setNeedRefresh(LLFloaterSnapshot* floater, bool need);

private:
	static LLViewerWindow::ESnapshotType getLayerType(LLFloaterSnapshot* floater);
	static void comboSetCustom(LLFloaterSnapshot *floater, const std::string& comboname);
	static void checkAutoSnapshot(LLSnapshotLivePreview* floater, BOOL update_thumbnail = FALSE);
	static void checkAspectRatio(LLFloaterSnapshot *view, S32 index) ;
	static void setWorking(LLFloaterSnapshot* floater, bool working);
	static void setFinished(LLFloaterSnapshot* floater, bool finished, bool ok = true, const std::string& msg = LLStringUtil::null);


public:
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;

	LLToolset*	mLastToolset;
	LLHandle<LLView> mPreviewHandle;
	bool mAspectRatioCheckOff ;
	bool mNeedRefresh;
	EStatus mStatus;
};

// static
LLPanelSnapshot* LLFloaterSnapshot::Impl::getActivePanel(LLFloaterSnapshot* floater, bool ok_if_not_found)
{
	LLSideTrayPanelContainer* panel_container = floater->getChild<LLSideTrayPanelContainer>("panel_container");
	LLPanelSnapshot* active_panel = dynamic_cast<LLPanelSnapshot*>(panel_container->getCurrentPanel());
	if (!ok_if_not_found)
	{
		llassert_always(active_panel != NULL);
	}
	return active_panel;
}

// static
LLSnapshotLivePreview::ESnapshotType LLFloaterSnapshot::Impl::getActiveSnapshotType(LLFloaterSnapshot* floater)
{
	LLSnapshotLivePreview::ESnapshotType type = LLSnapshotLivePreview::SNAPSHOT_WEB;
	std::string name;
	LLPanelSnapshot* spanel = getActivePanel(floater);

	if (spanel)
	{
		name = spanel->getName();
	}

	if (name == "panel_snapshot_postcard")
	{
		type = LLSnapshotLivePreview::SNAPSHOT_POSTCARD;
	}
	else if (name == "panel_snapshot_inventory")
	{
		type = LLSnapshotLivePreview::SNAPSHOT_TEXTURE;
	}
	else if (name == "panel_snapshot_local")
	{
		type = LLSnapshotLivePreview::SNAPSHOT_LOCAL;
	}

	return type;
}

// static
LLFloaterSnapshot::ESnapshotFormat LLFloaterSnapshot::Impl::getImageFormat(LLFloaterSnapshot* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	// FIXME: if the default is not PNG, profile uploads may fail.
	return active_panel ? active_panel->getImageFormat() : LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG;
}

// static
LLSpinCtrl* LLFloaterSnapshot::Impl::getWidthSpinner(LLFloaterSnapshot* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	return active_panel ? active_panel->getWidthSpinner() : floater->getChild<LLSpinCtrl>("snapshot_width");
}

// static
LLSpinCtrl* LLFloaterSnapshot::Impl::getHeightSpinner(LLFloaterSnapshot* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	return active_panel ? active_panel->getHeightSpinner() : floater->getChild<LLSpinCtrl>("snapshot_height");
}

// static
void LLFloaterSnapshot::Impl::enableAspectRatioCheckbox(LLFloaterSnapshot* floater, BOOL enable)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->enableAspectRatioCheckbox(enable);
	}
}

// static
void LLFloaterSnapshot::Impl::setAspectRatioCheckboxValue(LLFloaterSnapshot* floater, BOOL checked)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->getChild<LLUICtrl>(active_panel->getAspectRatioCBName())->setValue(checked);
	}
}

// static
LLSnapshotLivePreview* LLFloaterSnapshot::Impl::getPreviewView(LLFloaterSnapshot *floater)
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)floater->impl.mPreviewHandle.get();
	return previewp;
}

// static
LLViewerWindow::ESnapshotType LLFloaterSnapshot::Impl::getLayerType(LLFloaterSnapshot* floater)
{
	LLViewerWindow::ESnapshotType type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
	LLSD value = floater->getChild<LLUICtrl>("layer_types")->getValue();
	const std::string id = value.asString();
	if (id == "colors")
		type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
	else if (id == "depth")
		type = LLViewerWindow::SNAPSHOT_TYPE_DEPTH;
	return type;
}

// static
void LLFloaterSnapshot::Impl::setResolution(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
		combo->setVisible(TRUE);
	updateResolution(combo, floater, FALSE); // to sync spinners with combo
}

//static 
void LLFloaterSnapshot::Impl::updateLayout(LLFloaterSnapshot* floaterp)
{
	LLSnapshotLivePreview* previewp = getPreviewView(floaterp);

	BOOL advanced = gSavedSettings.getBOOL("AdvanceSnapshot");

	// Show/hide advanced options.
	LLPanel* advanced_options_panel = floaterp->getChild<LLPanel>("advanced_options_panel");
	floaterp->getChild<LLButton>("advanced_options_btn")->setImageOverlay(advanced ? "TabIcon_Open_Off" : "TabIcon_Close_Off");
	if (advanced != advanced_options_panel->getVisible())
	{
		S32 panel_width = advanced_options_panel->getRect().getWidth();
		floaterp->getChild<LLPanel>("advanced_options_panel")->setVisible(advanced);
		S32 floater_width = floaterp->getRect().getWidth();
		floater_width += (advanced ? panel_width : -panel_width);
		floaterp->reshape(floater_width, floaterp->getRect().getHeight());
	}

	if(!advanced) //set to original window resolution
	{
		previewp->mKeepAspectRatio = TRUE;

		floaterp->getChild<LLComboBox>("profile_size_combo")->setCurrentByIndex(0);
		floaterp->getChild<LLComboBox>("postcard_size_combo")->setCurrentByIndex(0);
		floaterp->getChild<LLComboBox>("texture_size_combo")->setCurrentByIndex(0);
		floaterp->getChild<LLComboBox>("local_size_combo")->setCurrentByIndex(0);

		LLSnapshotLivePreview* previewp = getPreviewView(floaterp);
		previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
	}

	bool use_freeze_frame = floaterp->getChild<LLUICtrl>("freeze_frame_check")->getValue().asBoolean();

	if (use_freeze_frame)
	{
		// stop all mouse events at fullscreen preview layer
		floaterp->getParent()->setMouseOpaque(TRUE);
		
		// shrink to smaller layout
		// *TODO: unneeded?
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getRect().getHeight());

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
			floaterp->impl.mAvatarPauseHandles.push_back(avatarp->requestPause());
		}

		// freeze everything else
		gSavedSettings.setBOOL("FreezeTime", TRUE);

		if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
		{
			floaterp->impl.mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
			LLToolMgr::getInstance()->setCurrentToolset(gCameraToolset);
		}
	}
	else // turning off freeze frame mode
	{
		floaterp->getParent()->setMouseOpaque(FALSE);
		// *TODO: unneeded?
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getRect().getHeight());
		if (previewp)
		{
			previewp->setVisible(FALSE);
			previewp->setEnabled(FALSE);
		}

		//RN: thaw all avatars
		floaterp->impl.mAvatarPauseHandles.clear();

		// thaw everything else
		gSavedSettings.setBOOL("FreezeTime", FALSE);

		// restore last tool (e.g. pie menu, etc)
		if (floaterp->impl.mLastToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(floaterp->impl.mLastToolset);
		}
	}
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// FIXME: The above comment seems obsolete now.
// static
void LLFloaterSnapshot::Impl::updateControls(LLFloaterSnapshot* floater)
{
	LLSnapshotLivePreview::ESnapshotType shot_type = getActiveSnapshotType(floater);
	ESnapshotFormat shot_format = (ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
	LLViewerWindow::ESnapshotType layer_type = getLayerType(floater);

#if 0
	floater->getChildView("share_to_web")->setVisible( gSavedSettings.getBOOL("SnapshotSharingEnabled"));
#endif

	floater->getChild<LLComboBox>("local_format_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFormat"));
	enableAspectRatioCheckbox(floater, !floater->impl.mAspectRatioCheckOff);
	setAspectRatioCheckboxValue(floater, gSavedSettings.getBOOL("KeepAspectForSnapshot"));
	floater->getChildView("layer_types")->setEnabled(shot_type == LLSnapshotLivePreview::SNAPSHOT_LOCAL);

	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		LLSpinCtrl* width_ctrl = getWidthSpinner(floater);
		LLSpinCtrl* height_ctrl = getHeightSpinner(floater);

		// Initialize spinners.
		if (width_ctrl->getValue().asInteger() == 0)
		{
			S32 w = gViewerWindow->getWindowWidthRaw();
			lldebugs << "Initializing width spinner (" << width_ctrl->getName() << "): " << w << llendl;
			width_ctrl->setValue(w);
		}
		if (height_ctrl->getValue().asInteger() == 0)
		{
			S32 h = gViewerWindow->getWindowHeightRaw();
			lldebugs << "Initializing height spinner (" << height_ctrl->getName() << "): " << h << llendl;
			height_ctrl->setValue(h);
		}

		// Сlamp snapshot resolution to window size when showing UI or HUD in snapshot.
		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{
			S32 width = gViewerWindow->getWindowWidthRaw();
			S32 height = gViewerWindow->getWindowHeightRaw();

			width_ctrl->setMaxValue(width);

			height_ctrl->setMaxValue(height);

			if (width_ctrl->getValue().asInteger() > width)
			{
				width_ctrl->forceSetValue(width);
			}
			if (height_ctrl->getValue().asInteger() > height)
			{
				height_ctrl->forceSetValue(height);
			}
		}
		else
		{
			width_ctrl->setMaxValue(6016);
			height_ctrl->setMaxValue(6016);
		}
	}
		
	LLSnapshotLivePreview* previewp = getPreviewView(floater);
	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

	// *TODO: Separate maximum size for Web images from postcards
	lldebugs << "Is snapshot up-to-date? " << got_snap << llendl;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_snap)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}

	// Update displayed image resolution.
	LLTextBox* image_res_tb = floater->getChild<LLTextBox>("image_res_text");
	image_res_tb->setVisible(got_snap);
	if (got_snap)
	{
		LLPointer<LLImageRaw> img = previewp->getEncodedImage();
		image_res_tb->setTextArg("[WIDTH]", llformat("%d", img->getWidth()));
		image_res_tb->setTextArg("[HEIGHT]", llformat("%d", img->getHeight()));
	}

	floater->getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : floater->getString("unknown"));
	floater->getChild<LLUICtrl>("file_size_label")->setColor(
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD 
		&& got_bytes
		&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLUIColor(LLColor4::red) : LLUIColorTable::instance().getColor( "LabelTextColor" ));

	// Update the width and height spinners based on the corresponding resolution combos. (?)
	switch(shot_type)
	{
	  case LLSnapshotLivePreview::SNAPSHOT_WEB:
		layer_type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "profile_size_combo");
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_POSTCARD:
		layer_type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "postcard_size_combo");
		break;
	  case LLSnapshotLivePreview::SNAPSHOT_TEXTURE:
		layer_type = LLViewerWindow::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "texture_size_combo");
		break;
	  case  LLSnapshotLivePreview::SNAPSHOT_LOCAL:
		setResolution(floater, "local_size_combo");
		break;
	  default:
		break;
	}

	if (previewp)
	{
		previewp->setSnapshotType(shot_type);
		previewp->setSnapshotFormat(shot_format);
		previewp->setSnapshotBufferType(layer_type);
	}

	LLPanelSnapshot* current_panel = Impl::getActivePanel(floater);
	if (current_panel)
	{
		LLSD info;
		info["have-snapshot"] = got_snap;
		current_panel->updateControls(info);
	}
	lldebugs << "finished updating controls" << llendl;
}

// static
void LLFloaterSnapshot::Impl::setStatus(EStatus status, bool ok, const std::string& msg)
{
	LLFloaterSnapshot* floater = LLFloaterSnapshot::getInstance();
	switch (status)
	{
	case STATUS_READY:
		setWorking(floater, false);
		setFinished(floater, false);
		break;
	case STATUS_WORKING:
		setWorking(floater, true);
		setFinished(floater, false);
		break;
	case STATUS_FINISHED:
		setWorking(floater, false);
		setFinished(floater, true, ok, msg);
		break;
	}

	floater->impl.mStatus = status;
}

// static
void LLFloaterSnapshot::Impl::setNeedRefresh(LLFloaterSnapshot* floater, bool need)
{
	if (!floater) return;

	// Don't display the "Refresh to save" message if we're in auto-refresh mode.
	if (gSavedSettings.getBOOL("AutoSnapshot"))
	{
		need = false;
	}

	floater->mRefreshLabel->setVisible(need);
	floater->impl.mNeedRefresh = need;
}

// static
void LLFloaterSnapshot::Impl::checkAutoSnapshot(LLSnapshotLivePreview* previewp, BOOL update_thumbnail)
{
	if (previewp)
	{
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		lldebugs << "updating " << (autosnap ? "snapshot" : "thumbnail") << llendl;
		previewp->updateSnapshot(autosnap, update_thumbnail, autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
}

// static
void LLFloaterSnapshot::Impl::onClickNewSnapshot(void* data)
{
	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterSnapshot *)data);
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (previewp && view)
	{
		view->impl.setStatus(Impl::STATUS_READY);
		lldebugs << "updating snapshot" << llendl;
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
		updateControls(view);
	}
}

void LLFloaterSnapshot::Impl::onClickMore(void* data)
{
	BOOL visible = gSavedSettings.getBOOL("AdvanceSnapshot");
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		view->impl.setStatus(Impl::STATUS_READY);
		gSavedSettings.setBOOL("AdvanceSnapshot", !visible);
		updateControls(view) ;
		updateLayout(view) ;
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
		checkAutoSnapshot(getPreviewView(view), TRUE);
		updateControls(view);
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
		checkAutoSnapshot(getPreviewView(view), TRUE);
		updateControls(view);
	}
}

// static
void LLFloaterSnapshot::Impl::applyKeepAspectCheck(LLFloaterSnapshot* view, BOOL checked)
{
	gSavedSettings.setBOOL("KeepAspectForSnapshot", checked);

	if (view)
	{
		LLSnapshotLivePreview* previewp = getPreviewView(view) ;
		if(previewp)
		{
			previewp->mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;

			S32 w, h ;
			previewp->getSize(w, h) ;
			updateSpinners(view, previewp, w, h, TRUE); // may change w and h

			lldebugs << "updating thumbnail" << llendl;
			previewp->setSize(w, h) ;
			previewp->updateSnapshot(FALSE, TRUE);
			checkAutoSnapshot(previewp, TRUE);
		}
	}
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
void LLFloaterSnapshot::Impl::checkAspectRatio(LLFloaterSnapshot *view, S32 index)
{
	LLSnapshotLivePreview *previewp = getPreviewView(view) ;

	// Don't round texture sizes; textures are commonly stretched in world, profiles, etc and need to be "squashed" during upload, not cropped here
	if(LLSnapshotLivePreview::SNAPSHOT_TEXTURE == getActiveSnapshotType(view))
	{
		previewp->mKeepAspectRatio = FALSE ;
		return ;
	}

	BOOL keep_aspect = FALSE, enable_cb = FALSE;

	if (0 == index) // current window size
	{
		enable_cb = FALSE;
		keep_aspect = TRUE;
	}
	else if (-1 == index) // custom
	{
		enable_cb = TRUE;
		keep_aspect = gSavedSettings.getBOOL("KeepAspectForSnapshot");
	}
	else // predefined resolution
	{
		enable_cb = FALSE;
		keep_aspect = FALSE;
	}

	view->impl.mAspectRatioCheckOff = !enable_cb;
	enableAspectRatioCheckbox(view, enable_cb);
	if (previewp)
	{
		previewp->mKeepAspectRatio = keep_aspect;
	}
}

// Show/hide upload progress indicators.
// static
void LLFloaterSnapshot::Impl::setWorking(LLFloaterSnapshot* floater, bool working)
{
	LLUICtrl* working_lbl = floater->getChild<LLUICtrl>("working_lbl");
	working_lbl->setVisible(working);
	floater->getChild<LLUICtrl>("working_indicator")->setVisible(working);

	if (working)
	{
		const std::string panel_name = getActivePanel(floater, false)->getName();
		const std::string prefix = panel_name.substr(std::string("panel_snapshot_").size());
		std::string progress_text = floater->getString(prefix + "_" + "progress_str");
		working_lbl->setValue(progress_text);
	}

	// All controls should be disabled while posting.
	floater->setCtrlsEnabled(!working);
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->enableControls(!working);
	}
}

// Show/hide upload status message.
// static
void LLFloaterSnapshot::Impl::setFinished(LLFloaterSnapshot* floater, bool finished, bool ok, const std::string& msg)
{
	floater->mSucceessLblPanel->setVisible(finished && ok);
	floater->mFailureLblPanel->setVisible(finished && !ok);

	if (finished)
	{
		LLUICtrl* finished_lbl = floater->getChild<LLUICtrl>(ok ? "succeeded_lbl" : "failed_lbl");
		std::string result_text = floater->getString(msg + "_" + (ok ? "succeeded_str" : "failed_str"));
		finished_lbl->setValue(result_text);

		LLSideTrayPanelContainer* panel_container = floater->getChild<LLSideTrayPanelContainer>("panel_container");
		panel_container->openPreviousPanel();
		panel_container->getCurrentPanel()->onOpen(LLSD());
	}
}

// Apply a new resolution selected from the given combobox.
// static
void LLFloaterSnapshot::Impl::updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (!view || !combobox)
	{
		llassert(view && combobox);
		return;
	}

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());
		
	S32 width = sdres[0];
	S32 height = sdres[1];
	
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;
		
		if (width == 0 || height == 0)
		{
			// take resolution from current window size
			lldebugs << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << llendl;
			previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
		}
		else if (width == -1 || height == -1)
		{
			// load last custom value
			S32 new_width = 0, new_height = 0;
			LLPanelSnapshot* spanel = getActivePanel(view);
			if (spanel)
			{
				lldebugs << "Loading typed res from panel " << spanel->getName() << llendl;
				new_width = spanel->getTypedPreviewWidth();
				new_height = spanel->getTypedPreviewHeight();

				// Limit custom size for inventory snapshots to 512x512 px.
				if (getActiveSnapshotType(view) == LLSnapshotLivePreview::SNAPSHOT_TEXTURE)
				{
					new_width = llmin(new_width, MAX_TEXTURE_SIZE);
					new_height = llmin(new_height, MAX_TEXTURE_SIZE);
				}
			}
			else
			{
				lldebugs << "No custom res chosen, setting preview res from window: "
					<< gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << llendl;
				new_width = gViewerWindow->getWindowWidthRaw();
				new_height = gViewerWindow->getWindowHeightRaw();
			}

			llassert(new_width > 0 && new_height > 0);
			previewp->setSize(new_width, new_height);
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			lldebugs << "Setting preview res selected from combo: " << width << "x" << height << llendl;
			previewp->setSize(width, height);
		}

		checkAspectRatio(view, width) ;

		previewp->getSize(width, height);
	
		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
			width = llmin(width, gViewerWindow->getWindowWidthRaw());
			height = llmin(height, gViewerWindow->getWindowHeightRaw());
		}

		updateSpinners(view, previewp, width, height, TRUE); // may change width and height
		
		if(getWidthSpinner(view)->getValue().asInteger() != width || getHeightSpinner(view)->getValue().asInteger() != height)
		{
			getWidthSpinner(view)->setValue(width);
			getHeightSpinner(view)->setValue(height);
		}

		if(original_width != width || original_height != height)
		{
			previewp->setSize(width, height);

			// hide old preview as the aspect ratio could be wrong
			checkAutoSnapshot(previewp, FALSE);
			lldebugs << "updating thumbnail" << llendl;
			getPreviewView(view)->updateSnapshot(FALSE, TRUE);
			if(do_update)
			{
				lldebugs << "Will update controls" << llendl;
				updateControls(view);
				setNeedRefresh(view, true);
			}
		}
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
		checkAutoSnapshot(previewp, TRUE);
	}
}

// static
void LLFloaterSnapshot::Impl::onImageQualityChange(LLFloaterSnapshot* view, S32 quality_val)
{
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
	checkAutoSnapshot(previewp, TRUE);
}

// static
void LLFloaterSnapshot::Impl::onImageFormatChange(LLFloaterSnapshot* view)
{
	if (view)
	{
		gSavedSettings.setS32("SnapshotFormat", getImageFormat(view));
		lldebugs << "image format changed, updating snapshot" << llendl;
		getPreviewView(view)->updateSnapshot(TRUE);
		updateControls(view);
		setNeedRefresh(view, false); // we're refreshing
	}
}

// Sets the named size combo to "custom" mode.
// static
void LLFloaterSnapshot::Impl::comboSetCustom(LLFloaterSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setCurrentByIndex(combo->getItemCount() - 1); // "custom" is always the last index
	checkAspectRatio(floater, -1); // -1 means custom
}

// Update supplied width and height according to the constrain proportions flag; limit them by max_val.
//static
BOOL LLFloaterSnapshot::Impl::checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value)
{
	S32 w = width ;
	S32 h = height ;

	if(previewp && previewp->mKeepAspectRatio)
	{
		if(gViewerWindow->getWindowWidthRaw() < 1 || gViewerWindow->getWindowHeightRaw() < 1)
		{
			return FALSE ;
		}

		//aspect ratio of the current window
		F32 aspect_ratio = (F32)gViewerWindow->getWindowWidthRaw() / gViewerWindow->getWindowHeightRaw() ;

		//change another value proportionally
		if(isWidthChanged)
		{
			height = llround(width / aspect_ratio) ;
		}
		else
		{
			width = llround(height * aspect_ratio) ;
		}

		//bound w/h by the max_value
		if(width > max_value || height > max_value)
		{
			if(width > height)
			{
				width = max_value ;
				height = (S32)(width / aspect_ratio) ;
			}
			else
			{
				height = max_value ;
				width = (S32)(height * aspect_ratio) ;
			}
		}
	}

	return (w != width || h != height) ;
}

//static
void LLFloaterSnapshot::Impl::setImageSizeSpinnersValues(LLFloaterSnapshot *view, S32 width, S32 height)
{
	getWidthSpinner(view)->forceSetValue(width);
	getHeightSpinner(view)->forceSetValue(height);
}

// static
void LLFloaterSnapshot::Impl::updateSpinners(LLFloaterSnapshot* view, LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed)
{
	if (checkImageSize(previewp, width, height, is_width_changed, previewp->getMaxImageSize()))
	{
		setImageSizeSpinnersValues(view, width, height);
	}
}

// static
void LLFloaterSnapshot::Impl::applyCustomResolution(LLFloaterSnapshot* view, S32 w, S32 h)
{
	bool need_refresh = false;

	lldebugs << "applyCustomResolution(" << w << ", " << h << ")" << llendl;
	if (!view) return;

	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp)
	{
		S32 curw,curh;
		previewp->getSize(curw, curh);

		if (w != curw || h != curh)
		{
			//if to upload a snapshot, process spinner input in a special way.
			previewp->setMaxImageSize((S32) getWidthSpinner(view)->getMaxValue()) ;

			updateSpinners(view, previewp, w, h, w != curw); // may change w and h

			previewp->setSize(w,h);
			checkAutoSnapshot(previewp, FALSE);
			lldebugs << "applied custom resolution, updating thumbnail" << llendl;
			previewp->updateSnapshot(FALSE, TRUE);
			comboSetCustom(view, "profile_size_combo");
			comboSetCustom(view, "postcard_size_combo");
			comboSetCustom(view, "texture_size_combo");
			comboSetCustom(view, "local_size_combo");
			need_refresh = true;
		}
	}

	updateControls(view);
	if (need_refresh)
	{
		setNeedRefresh(view, true); // need to do this after updateControls()
	}
}

// static
void LLFloaterSnapshot::Impl::onSnapshotUploadFinished(bool status)
{
	setStatus(STATUS_FINISHED, status, "profile");
}


// static
void LLFloaterSnapshot::Impl::onSendingPostcardFinished(bool status)
{
	setStatus(STATUS_FINISHED, status, "postcard");
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot(const LLSD& key)
	: LLFloater(key),
	  mRefreshBtn(NULL),
	  mRefreshLabel(NULL),
	  mSucceessLblPanel(NULL),
	  mFailureLblPanel(NULL),
	  impl (*(new Impl))
{
}

// Destroys the object
LLFloaterSnapshot::~LLFloaterSnapshot()
{
	if (impl.mPreviewHandle.get()) impl.mPreviewHandle.get()->die();

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl.mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl.mLastToolset);
	}

	delete &impl;
}


BOOL LLFloaterSnapshot::postBuild()
{
	// Kick start Web Sharing, to fetch its config data if it needs to.
	if (gSavedSettings.getBOOL("SnapshotSharingEnabled"))
	{
		LLWebSharing::instance().init();
	}

	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", Impl::onClickNewSnapshot, this);
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");

	childSetAction("advanced_options_btn", Impl::onClickMore, this);

	childSetCommitCallback("ui_check", Impl::onClickUICheck, this);
	getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));

	childSetCommitCallback("hud_check", Impl::onClickHUDCheck, this);
	getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	impl.setAspectRatioCheckboxValue(this, gSavedSettings.getBOOL("KeepAspectForSnapshot"));

	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	getChild<LLUICtrl>("layer_types")->setValue("colors");
	getChildView("layer_types")->setEnabled(FALSE);

	getChild<LLUICtrl>("freeze_frame_check")->setValue(gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", Impl::onCommitFreezeFrame, this);

	getChild<LLUICtrl>("auto_snapshot_check")->setValue(gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", Impl::onClickAutoSnap, this);

	LLWebProfile::setImageUploadResultCallback(boost::bind(&LLFloaterSnapshot::Impl::onSnapshotUploadFinished, _1));
	LLPostCard::setPostResultCallback(boost::bind(&LLFloaterSnapshot::Impl::onSendingPostcardFinished, _1));

	sThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

	// create preview window
	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	LLView* parent_view = gSnapshotFloaterView->getParent();
	
	parent_view->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	parent_view->addChild(previewp);
	parent_view->addChild(gSnapshotFloaterView);
	
	//move snapshot floater to special purpose snapshotfloaterview
	gFloaterView->removeChild(this);
	gSnapshotFloaterView->addChild(this);

	// Pre-select "Current Window" resolution.
	getChild<LLComboBox>("profile_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("postcard_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("texture_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("local_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("local_format_combo")->selectNthItem(0);

	impl.mPreviewHandle = previewp->getHandle();
	impl.updateControls(this);
	impl.updateLayout(this);
	
	return TRUE;
}

void LLFloaterSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = impl.getPreviewView(this);

	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	LLFloater::draw();

	if (previewp && !isMinimized())
	{		
		if(previewp->getThumbnailImage())
		{
			bool working = impl.getStatus() == Impl::STATUS_WORKING;
			const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
			const S32 thumbnail_w = previewp->getThumbnailWidth();
			const S32 thumbnail_h = previewp->getThumbnailHeight();

			// calc preview offset within the preview rect
			const S32 local_offset_x = (thumbnail_rect.getWidth() - thumbnail_w) / 2 ;
			const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ; // preview y pos within the preview rect

			// calc preview offset within the floater rect
			S32 offset_x = thumbnail_rect.mLeft + local_offset_x;
			S32 offset_y = thumbnail_rect.mBottom + local_offset_y;

			gGL.matrixMode(LLRender::MM_MODELVIEW);
			// Apply floater transparency to the texture unless the floater is focused.
			F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
			LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
			gl_draw_scaled_image(offset_x, offset_y, 
					thumbnail_w, thumbnail_h,
					previewp->getThumbnailImage(), color % alpha);

			previewp->drawPreviewRect(offset_x, offset_y) ;

			// Draw some controls on top of the preview thumbnail.
			static const S32 PADDING = 5;
			static const S32 REFRESH_LBL_BG_HEIGHT = 32;

			// Reshape and position the posting result message panels at the top of the thumbnail.
			// Do this regardless of current posting status (finished or not) to avoid flicker
			// when the result message is displayed for the first time.
			// if (impl.getStatus() == Impl::STATUS_FINISHED)
			{
				LLRect result_lbl_rect = mSucceessLblPanel->getRect();
				const S32 result_lbl_h = result_lbl_rect.getHeight();
				result_lbl_rect.setLeftTopAndSize(local_offset_x, local_offset_y + thumbnail_h, thumbnail_w - 1, result_lbl_h);
				mSucceessLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
				mSucceessLblPanel->setRect(result_lbl_rect);
				mFailureLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
				mFailureLblPanel->setRect(result_lbl_rect);
			}

			// Position the refresh button in the bottom left corner of the thumbnail.
			mRefreshBtn->setOrigin(local_offset_x + PADDING, local_offset_y + PADDING);

			if (impl.mNeedRefresh)
			{
				// Place the refresh hint text to the right of the refresh button.
				const LLRect& refresh_btn_rect = mRefreshBtn->getRect();
				mRefreshLabel->setOrigin(refresh_btn_rect.mLeft + refresh_btn_rect.getWidth() + PADDING, refresh_btn_rect.mBottom);

				// Draw the refresh hint background.
				LLRect refresh_label_bg_rect(offset_x, offset_y + REFRESH_LBL_BG_HEIGHT, offset_x + thumbnail_w - 1, offset_y);
				gl_rect_2d(refresh_label_bg_rect, LLColor4::white % 0.9f, TRUE);
			}

			gGL.pushUIMatrix();
			LLUI::translate((F32) thumbnail_rect.mLeft, (F32) thumbnail_rect.mBottom);
			sThumbnailPlaceholder->draw();
			gGL.popUIMatrix();
		}
	}
}

void LLFloaterSnapshot::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = LLFloaterSnapshot::Impl::getPreviewView(this);
	if(preview)
	{
		lldebugs << "opened, updating snapshot" << llendl;
		preview->updateSnapshot(TRUE);
	}
	focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

	// Initialize default tab.
	getChild<LLSideTrayPanelContainer>("panel_container")->getCurrentPanel()->onOpen(LLSD());
}

void LLFloaterSnapshot::onClose(bool app_quitting)
{
	getParent()->setMouseOpaque(FALSE);
}

// virtual
S32 LLFloaterSnapshot::notify(const LLSD& info)
{
	// A child panel wants to change snapshot resolution.
	if (info.has("combo-res-change"))
	{
		std::string combo_name = info["combo-res-change"]["control-name"].asString();
		impl.updateResolution(getChild<LLUICtrl>(combo_name), this);
		return 1;
	}

	if (info.has("custom-res-change"))
	{
		LLSD res = info["custom-res-change"];
		impl.applyCustomResolution(this, res["w"].asInteger(), res["h"].asInteger());
		return 1;
	}

	if (info.has("keep-aspect-change"))
	{
		impl.applyKeepAspectCheck(this, info["keep-aspect-change"].asBoolean());
		return 1;
	}

	if (info.has("image-quality-change"))
	{
		impl.onImageQualityChange(this, info["image-quality-change"].asInteger());
		return 1;
	}

	if (info.has("image-format-change"))
	{
		impl.onImageFormatChange(this);
		return 1;
	}

	if (info.has("set-ready"))
	{
		impl.setStatus(Impl::STATUS_READY);
		return 1;
	}

	if (info.has("set-working"))
	{
		impl.setStatus(Impl::STATUS_WORKING);
		return 1;
	}

	if (info.has("set-finished"))
	{
		LLSD data = info["set-finished"];
		impl.setStatus(Impl::STATUS_FINISHED, data["ok"].asBoolean(), data["msg"].asString());
		return 1;
	}
	return 0;
}

//static 
void LLFloaterSnapshot::update()
{
	LLFloaterSnapshot* inst = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!inst)
		return;
	
	BOOL changed = FALSE;
	lldebugs << "npreviews: " << LLSnapshotLivePreview::sList.size() << llendl;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		 iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
	if(changed)
	{
		lldebugs << "changed" << llendl;
		inst->impl.updateControls(inst);
	}
}

// static
LLFloaterSnapshot* LLFloaterSnapshot::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterSnapshot>("snapshot");
}

// static
void LLFloaterSnapshot::saveTexture()
{
	lldebugs << "saveTexture" << llendl;

	// FIXME: duplicated code
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!instance)
	{
		llassert(instance != NULL);
		return;
	}
	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return;
	}

	previewp->saveTexture();
}

// static
BOOL LLFloaterSnapshot::saveLocal()
{
	lldebugs << "saveLocal" << llendl;
	// FIXME: duplicated code
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!instance)
	{
		llassert(instance != NULL);
		return FALSE;
	}
	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return FALSE;
	}

	return previewp->saveLocal();
}

// static
void LLFloaterSnapshot::preUpdate()
{
	// FIXME: duplicated code
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (instance)
	{
		// Disable the send/post/save buttons until snapshot is ready.
		Impl::updateControls(instance);

		// Force hiding the "Refresh to save" hint because we know we've just started refresh.
		Impl::setNeedRefresh(instance, false);
	}
}

// static
void LLFloaterSnapshot::postUpdate()
{
	// FIXME: duplicated code
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (instance)
	{
		// Enable the send/post/save buttons.
		Impl::updateControls(instance);

		// We've just done refresh.
		Impl::setNeedRefresh(instance, false);

		// The refresh button is initially hidden. We show it after the first update,
		// i.e. when preview appears.
		if (!instance->mRefreshBtn->getVisible())
		{
			instance->mRefreshBtn->setVisible(true);
		}
	}
}

// static
void LLFloaterSnapshot::postSave()
{
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!instance)
	{
		llassert(instance != NULL);
		return;
	}

	instance->impl.updateControls(instance);
	instance->impl.setStatus(Impl::STATUS_WORKING);
}

// static
void LLFloaterSnapshot::postPanelSwitch()
{
	LLFloaterSnapshot* instance = getInstance();
	instance->impl.updateControls(instance);

	// Remove the success/failure indicator whenever user presses a snapshot option button.
	instance->impl.setStatus(Impl::STATUS_READY);
}

// static
LLPointer<LLImageFormatted> LLFloaterSnapshot::getImageData()
{
	// FIXME: May not work for textures.

	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!instance)
	{
		llassert(instance != NULL);
		return NULL;
	}

	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return NULL;
	}

	LLPointer<LLImageFormatted> img = previewp->getFormattedImage();
	if (!img.get())
	{
		llwarns << "Empty snapshot image data" << llendl;
		llassert(img.get() != NULL);
	}

	return img;
}

// static
const LLVector3d& LLFloaterSnapshot::getPosTakenGlobal()
{
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (!instance)
	{
		llassert(instance != NULL);
		return LLVector3d::zero;
	}

	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return LLVector3d::zero;
	}

	return previewp->getPosTakenGlobal();
}

// static
void LLFloaterSnapshot::setAgentEmail(const std::string& email)
{
	LLFloaterSnapshot* instance = LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
	if (instance)
	{
		LLSideTrayPanelContainer* panel_container = instance->getChild<LLSideTrayPanelContainer>("panel_container");
		LLPanel* postcard_panel = panel_container->getPanelByName("panel_snapshot_postcard");
		postcard_panel->notify(LLSD().with("agent-email", email));
	}
}

///----------------------------------------------------------------------------
/// Class LLSnapshotFloaterView
///----------------------------------------------------------------------------

LLSnapshotFloaterView::LLSnapshotFloaterView (const Params& p) : LLFloaterView (p)
{
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
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseDown( x, y, mask );
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
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseUp( x, y, mask );
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
		LLToolMgr::getInstance()->getCurrentTool()->handleHover( x, y, mask );
	}
	return TRUE;
}
