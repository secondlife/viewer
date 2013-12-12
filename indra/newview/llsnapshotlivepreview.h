/** 
* @file   llsnapshotlivepreview.h
* @brief  Header file for llsnapshotlivepreview
* @author Gilbert@lindenlab.com
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
#ifndef LL_LLSNAPSHOTLIVEPREVIEW_H
#define LL_LLSNAPSHOTLIVEPREVIEW_H

#include "llpanelsnapshot.h"
#include "llviewerwindow.h"

class LLImageJPEG;

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
	bool setSnapshotQuality(S32 quality, bool set_by_user = true);
	void setSnapshotBufferType(LLViewerWindow::ESnapshotType type) { mSnapshotBufferType = type; }
	void updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail = FALSE, F32 delay = 0.f);
	void saveWeb();
	void saveTexture();
	BOOL saveLocal();

	LLPointer<LLImageFormatted>	getFormattedImage() const { return mFormattedImage; }
	LLPointer<LLImageRaw>		getEncodedImage() const { return mPreviewImageEncoded; }

	/// Sets size of preview thumbnail image and thhe surrounding rect.
	void setThumbnailPlaceholderRect(const LLRect& rect) {mThumbnailPlaceholderRect = rect; }
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
	LLRect                      mThumbnailPlaceholderRect;

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

#endif // LL_LLSNAPSHOTLIVEPREVIEW_H

