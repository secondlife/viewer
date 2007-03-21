/** 
 * @file lltextureview.h
 * @brief LLTextureView class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTEXTUREVIEW_H
#define LL_LLTEXTUREVIEW_H

#include "llcontainerview.h"
#include "linked_lists.h"

class LLViewerImage;
class LLTextureBar;
class LLGLTexMemBar;

class LLTextureView : public LLContainerView
{
	friend class LLTextureBar;
	friend class LLGLTexMemBar;
public:
	LLTextureView(const std::string& name, const LLRect& rect);
	~LLTextureView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);

	static void addDebugImage(LLViewerImage* image) { sDebugImages.insert(image); }
	static void removeDebugImage(LLViewerImage* image) { sDebugImages.insert(image); }
	static void clearDebugImages() { sDebugImages.clear(); }

private:
	BOOL addBar(LLViewerImage *image, BOOL hilight = FALSE);
	void removeAllBars();

private:
	BOOL mFreezeView;
	BOOL mOrderFetch;
	BOOL mPrintList;
	
	LLTextBox *mInfoTextp;

	std::vector<LLTextureBar*> mTextureBars;
	U32 mNumTextureBars;

	LLGLTexMemBar* mGLTexMemBar;
	
public:
	static std::set<LLViewerImage*> sDebugImages;
};

extern LLTextureView *gTextureView;
#endif // LL_TEXTURE_VIEW_H
