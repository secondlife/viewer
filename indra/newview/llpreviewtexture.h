/** 
 * @file llpreviewtexture.h
 * @brief LLPreviewTexture class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

	virtual const char *getTitleName() const { return "Texture"; }
	
private:
	void				updateDimensions();
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
