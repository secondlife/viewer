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

#include "llsnapshotmodel.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"

class LLImageJPEG;

///----------------------------------------------------------------------------
/// Class LLSnapshotLivePreview 
///----------------------------------------------------------------------------
class LLSnapshotLivePreview : public LLView
{
	LOG_CLASS(LLSnapshotLivePreview);
public:

	static BOOL saveLocal(LLPointer<LLImageFormatted>);
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

    void setContainer(LLView* container) { mViewContainer = container; }

	/*virtual*/ void draw();
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);

	void setSize(S32 w, S32 h);
	void setWidth(S32 w) { mWidth[mCurImageIndex] = w; }
	void setHeight(S32 h) { mHeight[mCurImageIndex] = h; }
	void getSize(S32& w, S32& h) const;
	S32 getWidth() const { return mWidth[mCurImageIndex]; }
	S32 getHeight() const { return mHeight[mCurImageIndex]; }
    S32 getEncodedImageWidth() const;
    S32 getEncodedImageHeight() const;
    void estimateDataSize();
	S32 getDataSize() const { return mDataSize; }
	void setMaxImageSize(S32 size) ;
	S32  getMaxImageSize() {return mMaxImageSize ;}

    LLSnapshotModel::ESnapshotType getSnapshotType() const { return mSnapshotType; }
    LLSnapshotModel::ESnapshotFormat getSnapshotFormat() const { return mSnapshotFormat; }
	BOOL getSnapshotUpToDate() const { return mSnapshotUpToDate; }
	BOOL isSnapshotActive() { return mSnapshotActive; }
	LLViewerTexture* getThumbnailImage() const { return mThumbnailImage ; }
	S32  getThumbnailWidth() const { return mThumbnailWidth ; }
	S32  getThumbnailHeight() const { return mThumbnailHeight ; }
	BOOL getThumbnailLock() const { return mThumbnailUpdateLock ; }
	BOOL getThumbnailUpToDate() const { return mThumbnailUpToDate ;}
    void setThumbnailSubsampled(BOOL subsampled) { mThumbnailSubsampled = subsampled; }

	LLViewerTexture* getCurrentImage();
	F32 getImageAspect();
	const LLRect& getImageRect() const { return mImageRect[mCurImageIndex]; }
	BOOL isImageScaled() const { return mImageScaled[mCurImageIndex]; }
	void setImageScaled(BOOL scaled) { mImageScaled[mCurImageIndex] = scaled; }
	const LLVector3d& getPosTakenGlobal() const { return mPosTakenGlobal; }

    void setSnapshotType(LLSnapshotModel::ESnapshotType type) { mSnapshotType = type; }
    void setSnapshotFormat(LLSnapshotModel::ESnapshotFormat format);
	bool setSnapshotQuality(S32 quality, bool set_by_user = true);
	void setSnapshotBufferType(LLSnapshotModel::ESnapshotLayerType type) { mSnapshotBufferType = type; }
    void setAllowRenderUI(BOOL allow) { mAllowRenderUI = allow; }
    void setAllowFullScreenPreview(BOOL allow) { mAllowFullScreenPreview = allow; }
    void setFilter(std::string filter_name) { mFilterName = filter_name; }
    std::string  getFilter() const { return mFilterName; }
	void updateSnapshot(BOOL new_snapshot, BOOL new_thumbnail = FALSE, F32 delay = 0.f);
    void saveTexture(BOOL outfit_snapshot = FALSE, std::string name = "");
	BOOL saveLocal();

	LLPointer<LLImageFormatted>	getFormattedImage();
	LLPointer<LLImageRaw>		getEncodedImage();

	/// Sets size of preview thumbnail image and the surrounding rect.
	void setThumbnailPlaceholderRect(const LLRect& rect) {mThumbnailPlaceholderRect = rect; }
	BOOL setThumbnailImageSize() ;
	void generateThumbnailImage(BOOL force_update = FALSE) ;
	void resetThumbnailImage() { mThumbnailImage = NULL ; }
	void drawPreviewRect(S32 offset_x, S32 offset_y) ;
	void prepareFreezeFrame();
    
	LLViewerTexture* getBigThumbnailImage();
	S32  getBigThumbnailWidth() const { return mBigThumbnailWidth ; }
	S32  getBigThumbnailHeight() const { return mBigThumbnailHeight ; }

	// Returns TRUE when snapshot generated, FALSE otherwise.
	static BOOL onIdle( void* snapshot_preview );

private:
    LLView*                     mViewContainer;
    
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
    BOOL                        mThumbnailSubsampled; // TRUE if the thumbnail is a subsampled version of the mPreviewImage
    
	LLPointer<LLViewerTexture>	mBigThumbnailImage ;
    S32                         mBigThumbnailWidth;
    S32                         mBigThumbnailHeight;
    BOOL                        mBigThumbnailUpToDate;

	S32							mCurImageIndex;
    // The logic is mPreviewImage (raw frame) -> mFormattedImage (formatted / filtered) -> mPreviewImageEncoded (decoded back, to show artifacts)
	LLPointer<LLImageRaw>		mPreviewImage;
	LLPointer<LLImageRaw>		mPreviewImageEncoded;
	LLPointer<LLImageFormatted>	mFormattedImage;
    BOOL                        mAllowRenderUI;
    BOOL                        mAllowFullScreenPreview;
	LLFrameTimer				mSnapshotDelayTimer;
	S32							mShineCountdown;
	LLFrameTimer				mShineAnimTimer;
	F32							mFlashAlpha;
	BOOL						mNeedsFlash;
	LLVector3d					mPosTakenGlobal;
	S32							mSnapshotQuality;
	S32							mDataSize;
    LLSnapshotModel::ESnapshotType				mSnapshotType;
    LLSnapshotModel::ESnapshotFormat	mSnapshotFormat;
	BOOL						mSnapshotUpToDate;
	LLFrameTimer				mFallAnimTimer;
	LLVector3					mCameraPos;
	LLQuaternion				mCameraRot;
	BOOL						mSnapshotActive;
	LLSnapshotModel::ESnapshotLayerType mSnapshotBufferType;
    std::string                 mFilterName;

public:
	static std::set<LLSnapshotLivePreview*> sList;
	BOOL                        mKeepAspectRatio ;
	BOOL						mForceUpdateSnapshot;
};

#endif // LL_LLSNAPSHOTLIVEPREVIEW_H

