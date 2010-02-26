/** 
 * @file llhudtext.h
 * @brief LLHUDText class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLHUDTEXT_H
#define LL_LLHUDTEXT_H

#include "llpointer.h"
//#include "lldarrayptr.h"

#include "llhudobject.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v2math.h"
#include "llrect.h"
#include "llframetimer.h"
#include "llfontgl.h"
#include <set>
#include <vector>
//#include "lldarray.h"

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
	void setString(const std::string& text_utf8);
//	void setString(const LLWString &wstring);
	void clearString();
	void addLine(const std::string &text_utf8, const LLColor4& color, const LLFontGL::StyleFlags style = LLFontGL::NORMAL, const LLFontGL* font = NULL);
//	void addLine(const LLWString &wtext, const LLColor4& color, const LLFontGL::StyleFlags style = LLFontGL::NORMAL, const LLFontGL* font = NULL);
	void setLabel(const std::string& label_utf8);
//	void setLabel(const LLWString &label);
	void addLabel(const std::string& label_utf8);
	void setDropShadow(const BOOL do_shadow);
	void setFont(const LLFontGL* font);
	void setColor(const LLColor4 &color);
	void setAlpha(F32 alpha);
	void setZCompare(const BOOL zcompare);
	void setDoFade(const BOOL do_fade);
	void setVisibleOffScreen(BOOL visible) { mVisibleOffScreen = visible; }
	
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
	void setUseBubble(BOOL use_bubble) { mUseBubble = use_bubble; }
	S32  getLOD() { return mLOD; }
	BOOL getVisible() { return mVisible; }
	BOOL getHidden() const { return mHidden; }
	void setHidden( BOOL hide ) { mHidden = hide; }
	void setOnHUDAttachment(BOOL on_hud) { mOnHUDAttachment = on_hud; }
	void shift(const LLVector3& offset);

	BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, LLVector3& intersection, BOOL debug_render = FALSE);

	static void shiftAll(const LLVector3& offset);
	static void renderAllHUD();
	static void addPickable(std::set<LLViewerObject*> &pick_list);
	static void reshape();
	static void setDisplayText(BOOL flag) { sDisplayText = flag ; }
protected:
	LLHUDText(const U8 type);

	/*virtual*/ void render();
	/*virtual*/ void renderForSelect();
	void renderText(BOOL for_select);
	static void updateAll();
	void setLOD(S32 lod);
	S32 getMaxLines();

private:
	~LLHUDText();
	BOOL			mOnHUD;
	BOOL			mUseBubble;
	BOOL			mDropShadow;
	BOOL			mDoFade;
	F32				mFadeRange;
	F32				mFadeDistance;
	F32				mLastDistance;
	BOOL			mZCompare;
	BOOL			mVisibleOffScreen;
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
	std::vector<LLHUDTextSegment> mLabelSegments;
	LLFrameTimer	mResizeTimer;
	ETextAlignment	mTextAlignment;
	EVertAlignment	mVertAlignment;
	S32				mLOD;
	BOOL			mHidden;

	static BOOL    sDisplayText ;
	static std::set<LLPointer<LLHUDText> > sTextObjects;
	static std::vector<LLPointer<LLHUDText> > sVisibleTextObjects;
	static std::vector<LLPointer<LLHUDText> > sVisibleHUDTextObjects;
	typedef std::set<LLPointer<LLHUDText> >::iterator TextObjectIterator;
	typedef std::vector<LLPointer<LLHUDText> >::iterator VisibleTextObjectIterator;
};

#endif // LL_LLHUDTEXT_H
