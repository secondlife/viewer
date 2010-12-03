/** 
 * @file llhudtext.h
 * @brief LLHUDText class definition
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

#ifndef LL_LLHUDTEXT_H
#define LL_LLHUDTEXT_H

#include "llpointer.h"

#include "llhudobject.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "llrect.h"
#include "llfontgl.h"
#include <set>
#include <vector>

// Renders a 2D text billboard floating at the location specified.
class LLDrawable;
class LLHUDText;

struct lltextobject_further_away
{
	bool operator()(const LLPointer<LLHUDText>& lhs, const LLPointer<LLHUDText>& rhs) const;
};

class LLHUDText : public LLHUDObject
{
protected:
	class LLHUDTextSegment
	{
	public:
		LLHUDTextSegment(const LLWString& text, const LLFontGL::StyleFlags style, const LLColor4& color, const LLFontGL* font)
		:	mColor(color),
			mStyle(style),
			mText(text),
			mFont(font)
		{}
		F32 getWidth(const LLFontGL* font);
		const LLWString& getText() const { return mText; }
		void clearFontWidthMap() { mFontWidthMap.clear(); }
		
		LLColor4				mColor;
		LLFontGL::StyleFlags	mStyle;
		const LLFontGL*			mFont;
	private:
		LLWString				mText;
		std::map<const LLFontGL*, F32> mFontWidthMap;
	};

public:
	typedef enum e_text_alignment
	{
		ALIGN_TEXT_LEFT,
		ALIGN_TEXT_CENTER
	} ETextAlignment;

	typedef enum e_vert_alignment
	{
		ALIGN_VERT_TOP,
		ALIGN_VERT_CENTER
	} EVertAlignment;

public:
	// Set entire string, eliminating existing lines
	void setString(const std::string& text_utf8);

	void clearString();

	// Add text a line at a time, allowing custom formatting
	void addLine(const std::string &text_utf8, const LLColor4& color, const LLFontGL::StyleFlags style = LLFontGL::NORMAL, const LLFontGL* font = NULL);

	// Sets the default font for lines with no font specified
	void setFont(const LLFontGL* font);
	void setColor(const LLColor4 &color);
	void setAlpha(F32 alpha);
	void setZCompare(const BOOL zcompare);
	void setDoFade(const BOOL do_fade);
//	void setVisibleOffScreen(BOOL visible) { mVisibleOffScreen = visible; }
	
	// mMaxLines of -1 means unlimited lines.
	void setMaxLines(S32 max_lines) { mMaxLines = max_lines; }
	void setFadeDistance(F32 fade_distance, F32 fade_range) { mFadeDistance = fade_distance; mFadeRange = fade_range; }
	void updateVisibility();
	LLVector2 updateScreenPos(LLVector2 &offset_target);
	void updateSize();
	void setMass(F32 mass) { mMass = llmax(0.1f, mass); }
	void setTextAlignment(ETextAlignment alignment) { mTextAlignment = alignment; }
	void setVertAlignment(EVertAlignment alignment) { mVertAlignment = alignment; }
	/*virtual*/ void markDead();
	friend class LLHUDObject;
	/*virtual*/ F32 getDistance() const { return mLastDistance; }
	BOOL getVisible() { return mVisible; }
	BOOL getHidden() const { return mHidden; }
	void setHidden( BOOL hide ) { mHidden = hide; }
	void setOnHUDAttachment(BOOL on_hud) { mOnHUDAttachment = on_hud; }
	void shift(const LLVector3& offset);

	static void shiftAll(const LLVector3& offset);
	static void renderAllHUD();
	static void reshape();
	static void setDisplayText(BOOL flag) { sDisplayText = flag ; }

protected:
	LLHUDText(const U8 type);

	/*virtual*/ void render();
	void renderText();
	static void updateAll();
	S32 getMaxLines();

private:
	~LLHUDText();
	BOOL			mOnHUDAttachment;
	BOOL			mDoFade;
	F32				mFadeRange;
	F32				mFadeDistance;
	F32				mLastDistance;
	BOOL			mZCompare;
//	BOOL			mVisibleOffScreen;
	BOOL			mOffscreen;
	LLColor4		mColor;
	LLVector3		mScale;
	F32				mWidth;
	F32				mHeight;
	LLColor4U		mPickColor;
	const LLFontGL*	mFontp;
	const LLFontGL*	mBoldFontp;
	LLRectf			mSoftScreenRect;
	LLVector3		mPositionAgent;
	LLVector2		mPositionOffset;
	LLVector2		mTargetPositionOffset;
	F32				mMass;
	S32				mMaxLines;
	S32				mOffsetY;
	F32				mRadius;
	std::vector<LLHUDTextSegment> mTextSegments;
	ETextAlignment	mTextAlignment;
	EVertAlignment	mVertAlignment;
	BOOL			mHidden;

	static BOOL    sDisplayText ;
	static std::set<LLPointer<LLHUDText> > sTextObjects;
	static std::vector<LLPointer<LLHUDText> > sVisibleTextObjects;
	static std::vector<LLPointer<LLHUDText> > sVisibleHUDTextObjects;
	typedef std::set<LLPointer<LLHUDText> >::iterator TextObjectIterator;
	typedef std::vector<LLPointer<LLHUDText> >::iterator VisibleTextObjectIterator;
};

#endif // LL_LLHUDTEXT_H
