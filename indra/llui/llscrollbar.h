/** 
 * @file llscrollbar.h
 * @brief Scrollbar UI widget
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_SCROLLBAR_H
#define LL_SCROLLBAR_H

#include "stdtypes.h"
#include "lluictrl.h"
#include "v4color.h"
#include "llbutton.h"

//
// Classes
//
class LLScrollbar
: public LLUICtrl
{
public:

	enum ORIENTATION { HORIZONTAL, VERTICAL };
	
	typedef boost::function<void (S32, LLScrollbar*)> callback_t;
	struct Params 
	:	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Mandatory<ORIENTATION>			orientation;
		Mandatory<S32>					doc_size;
		Mandatory<S32>					doc_pos;
		Mandatory<S32>					page_size;

		Optional<callback_t> 			change_callback;
		Optional<S32>					step_size;
		Optional<S32>					thickness;

		Optional<LLUIImage*>			thumb_image_vertical,
										thumb_image_horizontal,
										track_image_horizontal,
										track_image_vertical;

		Optional<bool>					bg_visible;

		Optional<LLUIColor>				track_color,
										thumb_color,
										bg_color;

		Optional<LLButton::Params>		up_button;
		Optional<LLButton::Params>		down_button;
		Optional<LLButton::Params>		left_button;
		Optional<LLButton::Params>		right_button;

		Params();
	};

protected:
	LLScrollbar (const Params & p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLScrollbar();

	virtual void setValue(const LLSD& value);

	// Overrides from LLView
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
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
	void				setDocPos( S32 pos, BOOL update_thumb = TRUE );
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

	void				onLineUpBtnPressed(const LLSD& data);
	void				onLineDownBtnPressed(const LLSD& data);

private:
	void				updateThumbRect();
	void				changeLine(S32 delta, BOOL update_thumb );

	callback_t			mChangeCallback;

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

	LLUIColor			mTrackColor;
	LLUIColor			mThumbColor;
	LLUIColor			mBGColor;

	bool				mBGVisible;

	LLUIImagePtr		mThumbImageV;
	LLUIImagePtr		mThumbImageH;
	LLUIImagePtr		mTrackImageV;
	LLUIImagePtr		mTrackImageH;

	S32					mThickness;
};


#endif  // LL_SCROLLBAR_H
