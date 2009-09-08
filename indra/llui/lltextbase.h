/** 
 * @file lltextbase.h
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLTEXTBASE_H
#define LL_LLTEXTBASE_H

#include "v4color.h"
#include "llstyle.h"
#include "llkeywords.h"
#include "lluictrl.h"

#include <string>
#include <set>

class LLContextMenu;
class LLTextSegment;

typedef LLPointer<LLTextSegment> LLTextSegmentPtr;

///
/// The LLTextBase class provides a base class for all text fields, such
/// as LLTextEditor and LLTextBox. It implements shared functionality
/// such as Url highlighting and opening.
///
class LLTextBase
{
public:
	LLTextBase(const LLUICtrl::Params &p);
	virtual ~LLTextBase();

	/// specify the color to display Url hyperlinks in the text
	static void setLinkColor(LLColor4 color) { mLinkColor = color; }

	/// enable/disable the automatic hyperlinking of Urls in the text
	void        setParseHTML(BOOL parsing) { mParseHTML=parsing; }

	// public text editing virtual methods
	virtual LLWString getWText() const = 0;
	virtual BOOL      allowsEmbeddedItems() const { return FALSE; }
	virtual BOOL      getWordWrap() { return mWordWrap; }
	virtual S32       getLength() const = 0;

protected:
	struct compare_segment_end
	{
		bool operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const;
	};
	typedef std::multiset<LLTextSegmentPtr, compare_segment_end> segment_set_t;

	// routines to manage segments 
	void                getSegmentAndOffset( S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp ) const;
	void                getSegmentAndOffset( S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp );
	LLTextSegmentPtr    getSegmentAtLocalPos( S32 x, S32 y );
	segment_set_t::iterator			getSegIterContaining(S32 index);
	segment_set_t::const_iterator	getSegIterContaining(S32 index) const;
	void                clearSegments();
	void                setHoverSegment(LLTextSegmentPtr segment);

	// event handling for Urls within the text field
	BOOL                handleHoverOverUrl(S32 x, S32 y);
	BOOL                handleMouseUpOverUrl(S32 x, S32 y);
	BOOL                handleRightMouseDownOverUrl(LLView *view, S32 x, S32 y);
	BOOL                handleToolTipForUrl(LLView *view, S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen);

	// pure virtuals that have to be implemented by any subclasses
	virtual S32         getLineCount() const = 0;
	virtual S32         getLineStart( S32 line ) const = 0;
	virtual S32         getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const = 0;

	// protected member variables
	static LLUIColor    mLinkColor;
	const LLFontGL      *mDefaultFont;
	segment_set_t       mSegments;
	LLTextSegmentPtr    mHoverSegment;	
	BOOL                mParseHTML;
	BOOL                mWordWrap;

private:
	// create a popup context menu for the given Url
	static LLContextMenu *createUrlContextMenu(const std::string &url);

	LLContextMenu        *mPopupMenu;
};

///
/// A text segment is used to specify a subsection of a text string
/// that should be formatted differently, such as a hyperlink. It
/// includes a start/end offset from the start of the string, a
/// style to render with, an optional tooltip, etc.
///
class LLTextSegment : public LLRefCount
{
public:
	LLTextSegment(S32 start, S32 end) : mStart(start), mEnd(end){};
	virtual ~LLTextSegment();

	virtual S32					getWidth(S32 first_char, S32 num_chars) const;
	virtual S32					getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;
	virtual S32					getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	virtual void				updateLayout(const class LLTextBase& editor);
	virtual F32					draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	virtual S32					getMaxHeight() const;
	virtual bool				canEdit() const;
	virtual void				unlinkFromDocument(class LLTextBase* editor);
	virtual void				linkToDocument(class LLTextBase* editor);

	virtual void				setHasMouseHover(bool hover);
	virtual const LLColor4&		getColor() const;
	virtual void 				setColor(const LLColor4 &color);
	virtual const LLStyleSP		getStyle() const;
	virtual void 				setStyle(const LLStyleSP &style);
	virtual void				setToken( LLKeywordToken* token );
	virtual LLKeywordToken*		getToken() const;
	virtual BOOL				getToolTip( std::string& msg ) const;
	virtual void				setToolTip(const std::string& tooltip);
	virtual void				dump() const;

	S32							getStart() const 					{ return mStart; }
	void						setStart(S32 start)					{ mStart = start; }
	S32							getEnd() const						{ return mEnd; }
	void						setEnd( S32 end )					{ mEnd = end; }

protected:
	S32				mStart;
	S32				mEnd;
};

class LLNormalTextSegment : public LLTextSegment
{
public:
	LLNormalTextSegment( const LLStyleSP& style, S32 start, S32 end, LLTextBase& editor );
	LLNormalTextSegment( const LLColor4& color, S32 start, S32 end, LLTextBase& editor, BOOL is_visible = TRUE);

	/*virtual*/ S32					getWidth(S32 first_char, S32 num_chars) const;
	/*virtual*/ S32					getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;
	/*virtual*/ S32					getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	/*virtual*/ F32					draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	/*virtual*/ S32					getMaxHeight() const;
	/*virtual*/ bool				canEdit() const { return true; }
	/*virtual*/ void				setHasMouseHover(bool hover)		{ mHasMouseHover = hover; }
	/*virtual*/ const LLColor4&		getColor() const					{ return mStyle->getColor(); }
	/*virtual*/ void 				setColor(const LLColor4 &color)		{ mStyle->setColor(color); }
	/*virtual*/ const LLStyleSP		getStyle() const					{ return mStyle; }
	/*virtual*/ void 				setStyle(const LLStyleSP &style)	{ mStyle = style; }
	/*virtual*/ void				setToken( LLKeywordToken* token )	{ mToken = token; }
	/*virtual*/ LLKeywordToken*		getToken() const					{ return mToken; }
	/*virtual*/ BOOL				getToolTip( std::string& msg ) const;
	/*virtual*/ void				setToolTip(const std::string& tooltip);
	/*virtual*/ void				dump() const;

protected:
	F32				drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, F32 x, F32 y);

	class LLTextBase&	mEditor;
	LLStyleSP		mStyle;
	S32				mMaxHeight;
	LLKeywordToken* mToken;
	bool			mHasMouseHover;
	std::string     mTooltip;
};

class LLIndexSegment : public LLTextSegment
{
public:
	LLIndexSegment(S32 pos) : LLTextSegment(pos, pos) {}
};

#endif
