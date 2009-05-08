/** 
 * @file llfontbitmapcache.h
 * @brief Storage for previously rendered glyphs.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLFONTBITMAPCACHE_H
#define LL_LLFONTBITMAPCACHE_H

#include <vector>

// Maintain a collection of bitmaps containing rendered glyphs.
// Generalizes the single-bitmap logic from LLFont and LLFontGL.
class LLFontBitmapCache: public LLRefCount
{
public:
	LLFontBitmapCache();
	~LLFontBitmapCache();

	// Need to call this once, before caching any glyphs.
 	void init(S32 num_components,
			  S32 max_char_width,
			  S32 max_char_height);

	void reset();

	BOOL nextOpenPos(S32 width, S32 &posX, S32 &posY, S32 &bitmapNum);
	
	void destroyGL();
	
 	LLImageRaw *getImageRaw(U32 bitmapNum = 0) const;
 	LLImageGL *getImageGL(U32 bitmapNum = 0) const;
	
	S32 getMaxCharWidth() const { return mMaxCharWidth; }
	S32 getNumComponents() const { return mNumComponents; }
	S32 getBitmapWidth() const { return mBitmapWidth; }
	S32 getBitmapHeight() const { return mBitmapHeight; }

private:
	S32 mNumComponents;
	S32 mBitmapWidth;
	S32 mBitmapHeight;
	S32 mBitmapNum;
	S32 mMaxCharWidth;
	S32 mMaxCharHeight;
	S32 mCurrentOffsetX;
	S32 mCurrentOffsetY;
	S32 mCurrentBitmapNum;
	std::vector<LLPointer<LLImageRaw> >	mImageRawVec;
	std::vector<LLPointer<LLImageGL> > mImageGLVec;
};

#endif //LL_LLFONTBITMAPCACHE_H
