/** 
 * @file llscrollbar.h
 * @brief Scrollbar UI widget
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_SCROLLBAR_H
#define LL_SCROLLBAR_H

#include "stdtypes.h"
#include "lluictrl.h"
#include "v4color.h"

//
// Constants
//
const S32 SCROLLBAR_SIZE = 16;


//
// Classes
//
class LLScrollbar
: public LLUICtrl
{
public:
	enum ORIENTATION { HORIZONTAL, VERTICAL };

	LLScrollbar(const std::string& name, LLRect rect,
		ORIENTATION orientation,
		S32 doc_size, S32 doc_pos, S32 page_size,
		void(*change_callback)( S32 new_pos, LLScrollbar* self, void* userdata ),
		void* callback_user_data = NULL,
		S32 step_size = 1);

	virtual ~LLScrollbar();

	virtual void setValue(const LLSD& value);

	// Overrides from LLView
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, 
		EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string &tooltip_msg);

	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual void	draw();

	// How long the "document" is.
	void				setDocSize( S32 size );
	S32					getDocSize() const		{ return mDocSize; }

	// How many "lines" the "document" has scrolled.
	// 0 <= DocPos <= DocSize - DocVisibile
	void				setDocPos( S32 pos );
	S32					getDocPos() const		{ return mDocPos; }

	BOOL				isAtBeginning();
	BOOL				isAtEnd();

	// Setting both at once.
	void				setDocParams( S32 size, S32 pos );

	// How many "lines" of the "document" is can appear on a page.
	void				setPageSize( S32 page_size );
	S32					getPageSize() const		{ return mPageSize; }
	
	// The farthest the document can be scrolled (top of the last page).
	S32					getDocPosMax() const	{ return llmax( 0, mDocSize - mPageSize); }

	void				pageUp(S32 overlap);
	void				pageDown(S32 overlap);

	static void			onLineUpBtnPressed(void* userdata);
	static void			onLineDownBtnPressed(void* userdata);

	void setTrackColor( const LLColor4& color ) { mTrackColor = color; }
	void setThumbColor( const LLColor4& color ) { mThumbColor = color; }
	void setHighlightColor( const LLColor4& color ) { mHighlightColor = color; }
	void setShadowColor( const LLColor4& color ) { mShadowColor = color; }

	void setOnScrollEndCallback(void (*callback)(void*), void* userdata) { mOnScrollEndCallback = callback; mOnScrollEndData = userdata;}

private:
	void				updateThumbRect();
	void				changeLine(S32 delta, BOOL update_thumb );

	void				(*mChangeCallback)( S32 new_pos, LLScrollbar* self, void* userdata );
	void*				mCallbackUserData;

	const ORIENTATION	mOrientation;	
	S32					mDocSize;		// Size of the document that the scrollbar is modeling.  Units depend on the user.  0 <= mDocSize.
	S32					mDocPos;		// Position within the doc that the scrollbar is modeling, in "lines" (user size)
	S32					mPageSize;		// Maximum number of lines that can be seen at one time.
	S32					mStepSize;
	BOOL				mDocChanged;

	LLRect				mThumbRect;
	S32					mDragStartX;
	S32					mDragStartY;
	F32					mHoverGlowStrength;
	F32					mCurGlowStrength;

	LLRect				mOrigRect;
	S32					mLastDelta;

	LLColor4			mTrackColor;
	LLColor4			mThumbColor;
	LLColor4			mFocusColor;
	LLColor4			mHighlightColor;
	LLColor4			mShadowColor;

	void			(*mOnScrollEndCallback)(void*);
	void			*mOnScrollEndData;
};



#endif  // LL_SCROLLBAR_H
