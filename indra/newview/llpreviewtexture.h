/** 
 * @file llpreviewtexture.h
 * @brief LLPreviewTexture class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPREVIEWTEXTURE_H
#define LL_LLPREVIEWTEXTURE_H

#include "llpreview.h"
#include "llbutton.h"
#include "llframetimer.h"
#include "llviewerimage.h"

class LLImageRaw;

class LLPreviewTexture : public LLPreview
{
public:
	LLPreviewTexture(
		const std::string& name,
		const LLRect& rect,
		const std::string& title,
		const LLUUID& item_uuid,
		const LLUUID& object_id,
		BOOL show_keep_discard = FALSE);
	LLPreviewTexture(
		const std::string& name,
		const LLRect& rect,
		const std::string& title,
		const LLUUID& asset_id,
		BOOL copy_to_inv = FALSE);
	~LLPreviewTexture();

	virtual void		draw();

	virtual BOOL		canSaveAs();
	virtual void		saveAs();

	virtual void		loadAsset();
	virtual EAssetStatus	getAssetStatus();

	static void			saveToFile(void* userdata);
	static void			onFileLoadedForSave( 
							BOOL success,
							LLViewerImage *src_vi,
							LLImageRaw* src, 
							LLImageRaw* aux_src,
							S32 discard_level, 
							BOOL final,
							void* userdata );


protected:
	void				init();
	void				updateAspectRatio();

protected:
	LLUUID						mImageID;
	LLPointer<LLViewerImage>		mImage;
	BOOL				mLoadingFullImage;
	LLString			mSaveFileName;
	LLFrameTimer		mSavedFileTimer;
	BOOL                mShowKeepDiscard;
	BOOL                mCopyToInv;

	// This is stored off in a member variable, because the save-as
	// button and drag and drop functionality need to know.
	BOOL mIsCopyable;

	S32 mLastHeight;
	S32 mLastWidth;
};


#endif  // LL_LLPREVIEWTEXTURE_H
