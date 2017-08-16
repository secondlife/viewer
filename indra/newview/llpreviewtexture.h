/** 
 * @file llpreviewtexture.h
 * @brief LLPreviewTexture class definition
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

#ifndef LL_LLPREVIEWTEXTURE_H
#define LL_LLPREVIEWTEXTURE_H

#include "llpreview.h"
#include "llbutton.h"
#include "llframetimer.h"
#include "llviewertexture.h"

class LLComboBox;
class LLImageRaw;

class LLPreviewTexture : public LLPreview
{
public:
	LLPreviewTexture(const LLSD& key);
	~LLPreviewTexture();

	virtual void		draw();

	virtual BOOL		canSaveAs() const;
	virtual void		saveAs();

	virtual void		loadAsset();
	virtual EAssetStatus	getAssetStatus();
	
	virtual void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void 		onFocusReceived();
	
	static void			onFileLoadedForSave( 
							BOOL success,
							LLViewerFetchedTexture *src_vi,
							LLImageRaw* src, 
							LLImageRaw* aux_src,
							S32 discard_level, 
							BOOL final,
							void* userdata );
	void 				openToSave();
	
	static void			onSaveAsBtn(void* data);

	/*virtual*/ void setObjectID(const LLUUID& object_id);
protected:
	void				init();
	void				populateRatioList();
	/* virtual */ BOOL	postBuild();
	bool				setAspectRatio(const F32 width, const F32 height);
	static void			onAspectRatioCommit(LLUICtrl*,void* userdata);
	void				adjustAspectRatio();
	
private:
	void				updateImageID(); // set what image is being uploaded.
	void				updateDimensions();
	LLUUID				mImageID;
	LLPointer<LLViewerFetchedTexture>		mImage;
	S32                 mImageOldBoostLevel;
	std::string			mSaveFileName;
	LLFrameTimer		mSavedFileTimer;
	BOOL				mLoadingFullImage;
	BOOL                mShowKeepDiscard;
	BOOL                mCopyToInv;

	// Save the image once it's loaded.
	BOOL                mPreviewToSave;

	// This is stored off in a member variable, because the save-as
	// button and drag and drop functionality need to know.
	BOOL mIsCopyable;
	BOOL mIsFullPerm;
	BOOL mUpdateDimensions;
	S32 mLastHeight;
	S32 mLastWidth;
	F32 mAspectRatio;	

	LLLoadedCallbackEntry::source_callback_list_t mCallbackTextureList ; 
	std::vector<std::string>		mRatiosList;
};
#endif  // LL_LLPREVIEWTEXTURE_H
