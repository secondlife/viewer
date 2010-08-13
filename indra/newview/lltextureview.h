/** 
 * @file lltextureview.h
 * @brief LLTextureView class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLTEXTUREVIEW_H
#define LL_LLTEXTUREVIEW_H

#include "llcontainerview.h"

class LLViewerFetchedTexture;
class LLTextureBar;
class LLGLTexMemBar;
class LLAvatarTexBar;

class LLTextureView : public LLContainerView
{
	friend class LLTextureBar;
	friend class LLGLTexMemBar;
	friend class LLAvatarTexBar;
protected:
	LLTextureView(const Params&);
	friend class LLUICtrlFactory;
public:
	~LLTextureView();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);

	static void addDebugImage(LLViewerFetchedTexture* image) { sDebugImages.insert(image); }
	static void removeDebugImage(LLViewerFetchedTexture* image) { sDebugImages.insert(image); }
	static void clearDebugImages() { sDebugImages.clear(); }

private:
	BOOL addBar(LLViewerFetchedTexture *image, BOOL hilight = FALSE);
	void removeAllBars();

private:
	BOOL mFreezeView;
	BOOL mOrderFetch;
	BOOL mPrintList;
	
	LLTextBox *mInfoTextp;

	std::vector<LLTextureBar*> mTextureBars;
	U32 mNumTextureBars;

	LLGLTexMemBar* mGLTexMemBar;
	LLAvatarTexBar* mAvatarTexBar;
public:
	static std::set<LLViewerFetchedTexture*> sDebugImages;
};

class LLGLTexSizeBar;
class LLTextureSizeView : public LLContainerView
{
protected:
	LLTextureSizeView(const Params&);
	friend class LLUICtrlFactory;
public:	
	~LLTextureSizeView();

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask) ;
	
	void setType(S32 type) {mType = type ;}
	enum
	{
		TEXTURE_MEM_OVER_SIZE,
		TEXTURE_MEM_OVER_CATEGORY
	};
private:
	//draw background for TEXTURE_MEM_OVER_SIZE
	F32 drawTextureSizeDistributionGraph() ;
	//draw real-time texture mem bar over size
	void drawTextureSizeGraph();

	//draw background for TEXTURE_MEM_OVER_CATEGORY
	F32 drawTextureCategoryDistributionGraph() ;
	//draw real-time texture mem bar over category
	void drawTextureCategoryGraph();

private:
	std::vector<LLGLTexSizeBar*> mTextureSizeBar ;
	LLRect mTextureSizeBarRect ;
	S32    mTextureSizeBarWidth ;	
	S32    mType ;
};
extern LLTextureView *gTextureView;
extern LLTextureSizeView *gTextureSizeView;
extern LLTextureSizeView *gTextureCategoryView;
#endif // LL_TEXTURE_VIEW_H
