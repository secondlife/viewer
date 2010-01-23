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
#include "lleditmenuhandler.h"
#include "llstyle.h"
#include "llkeywords.h"
#include "llpanel.h"

#include <string>
#include <set>

#include <boost/signals2.hpp>

class LLContextMenu;
class LLTextSegment;

typedef LLPointer<LLTextSegment> LLTextSegmentPtr;

///
/// The LLTextBase class provides a base class for all text fields, such
/// as LLTextEditor and LLTextBox. It implements shared functionality
/// such as Url highlighting and opening.
///
class LLTextBase 
:	public LLUICtrl,
	protected LLEditMenuHandler
{
public:
	struct LineSpacingParams : public LLInitParam::Choice<LineSpacingParams>
	{
		Alternative<F32>	multiple;
		Alternative<S32>	pixels;
		LineSpacingParams();
	};

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIColor>		cursor_color,
								text_color,
								text_readonly_color,
								bg_readonly_color,
								bg_writeable_color,
								bg_focus_color;

		Optional<bool>			bg_visible,
								border_visible,
								track_end,
								read_only,
								allow_scroll,
								wrap,
								use_ellipses,
								allow_html,
								parse_highlights,
								clip_partial;
								
		Optional<S32>			v_pad,
								h_pad;


		Optional<LineSpacingParams>
								line_spacing;

		Optional<S32>			max_text_length;

		Optional<LLFontGL::ShadowType>	font_shadow;

		Params();
	};

	// LLMouseHandler interface
	/*virtual*/ BOOL		handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleMiddleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleRightMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL		handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL		handleToolTip(S32 x, S32 y, MASK mask);

	// LLView interface
	/*virtual*/ void		reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void		draw();

	// LLUICtrl interface
	/*virtual*/ BOOL		acceptsTextInput() const { return !mReadOnly; }
	/*virtual*/ void		setColor( const LLColor4& c );
	virtual     void 		setReadOnlyColor(const LLColor4 &c);

	/*virtual*/ void		setValue(const LLSD& value );
	/*virtual*/ LLTextViewModel* getViewModel() const;

	// LLEditMenuHandler interface
	/*virtual*/ void		deselect();

	// used by LLTextSegment layout code
	bool					getWordWrap() { return mWordWrap; }
	bool					getUseEllipses() { return mUseEllipses; }
	bool					truncate(); // returns true of truncation occurred

	// TODO: move into LLTextSegment?
	void					createUrlContextMenu(S32 x, S32 y, const std::string &url); // create a popup context menu for the given Url

	// Text accessors
	// TODO: add optional style parameter
	virtual void			setText(const LLStringExplicit &utf8str , const LLStyle::Params& input_params = LLStyle::Params()); // uses default style
	virtual std::string		getText() const;
	void					setMaxTextLength(S32 length) { mMaxTextByteLength = length; }

	// wide-char versions
	void					setWText(const LLWString& text);
	LLWString       		getWText() const;

	void					appendText(const std::string &new_text, bool prepend_newline, const LLStyle::Params& input_params = LLStyle::Params());
	// force reflow of text
	void					needsReflow() { mReflowNeeded = TRUE; }

	S32						getLength() const { return getWText().length(); }
	S32						getLineCount() const { return mLineInfoList.size(); }

	void					addDocumentChild(LLView* view);
	void					removeDocumentChild(LLView* view);
	const LLView*			getDocumentView() const { return mDocumentView; }
	LLRect					getVisibleTextRect() { return mVisibleTextRect; }
	LLRect					getTextBoundingRect();
	LLRect					getVisibleDocumentRect() const;

	S32						getVPad() { return mVPad; }
	S32						getHPad() { return mHPad; }

	S32						getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const;
	LLRect					getLocalRectFromDocIndex(S32 pos) const;
	LLRect					getDocRectFromDocIndex(S32 pos) const;

	void					setReadOnly(bool read_only) { mReadOnly = read_only; }
	bool					getReadOnly() { return mReadOnly; }

	// cursor manipulation
	bool					setCursor(S32 row, S32 column);
	bool					setCursorPos(S32 cursor_pos, bool keep_cursor_offset = false);
	void					startOfLine();
	void					endOfLine();
	void					startOfDoc();
	void					endOfDoc();
	void					changePage( S32 delta );
	void					changeLine( S32 delta );

	bool					scrolledToStart();
	bool					scrolledToEnd();

	const LLFontGL*			getDefaultFont() const					{ return mDefaultFont; }

public:
	// Fired when a URL link is clicked
	commit_signal_t mURLClickSignal;

protected:
	// helper structs
	struct compare_bottom;
	struct compare_top;
	struct line_end_compare;
	typedef std::vector<LLTextSegmentPtr> segment_vec_t;

	// Abstract inner base class representing an undoable editor command.
	// Concrete sub-classes can be defined for operations such as insert, remove, etc.
	// Used as arguments to the execute() method below.
	class TextCmd
	{
	public:
		TextCmd( S32 pos, BOOL group_with_next, LLTextSegmentPtr segment = LLTextSegmentPtr() ) 
		:	mPos(pos), 
			mGroupWithNext(group_with_next)
		{
			if (segment.notNull())
			{
				mSegments.push_back(segment);
			}
		}
		virtual			~TextCmd() {}
		virtual BOOL	execute(LLTextBase* editor, S32* delta) = 0;
		virtual S32		undo(LLTextBase* editor) = 0;
		virtual S32		redo(LLTextBase* editor) = 0;
		virtual BOOL	canExtend(S32 pos) const { return FALSE; }
		virtual void	blockExtensions() {}
		virtual BOOL	extendAndExecute( LLTextBase* editor, S32 pos, llwchar c, S32* delta ) { llassert(0); return 0; }
		virtual BOOL	hasExtCharValue( llwchar value ) const { return FALSE; }

		// Defined here so they can access protected LLTextEditor editing methods
		S32				insert(LLTextBase* editor, S32 pos, const LLWString &wstr) { return editor->insertStringNoUndo( pos, wstr, &mSegments ); }
		S32 			remove(LLTextBase* editor, S32 pos, S32 length) { return editor->removeStringNoUndo( pos, length ); }
		S32				overwrite(LLTextBase* editor, S32 pos, llwchar wc) { return editor->overwriteCharNoUndo(pos, wc); }
		
		S32				getPosition() const { return mPos; }
		BOOL			groupWithNext() const { return mGroupWithNext; }
		
	protected:
		const S32			mPos;
		BOOL				mGroupWithNext;
		segment_vec_t		mSegments;
	};

	struct compare_segment_end
	{
		bool operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const;
	};
	typedef std::multiset<LLTextSegmentPtr, compare_segment_end> segment_set_t;

	// protected member variables
	// List of offsets and segment index of the start of each line.  Always has at least one node (0).
	struct line_info
	{
		line_info(S32 index_start, S32 index_end, LLRect rect, S32 line_num);
		S32 mDocIndexStart;
		S32 mDocIndexEnd;
		LLRect mRect;
		S32 mLineNum; // actual line count (ignoring soft newlines due to word wrap)
	};
	typedef std::vector<line_info> line_list_t;

	// member functions
	LLTextBase(const Params &p);
	virtual ~LLTextBase();
	void							initFromParams(const Params& p);
	virtual void					onValueChange(S32 start, S32 end);

	// draw methods
	void							drawSelectionBackground(); // draws the black box behind the selected text
	void							drawCursor();
	void							drawText();

	// modify contents
	S32								insertStringNoUndo(S32 pos, const LLWString &wstr, segment_vec_t* segments = NULL); // returns num of chars actually inserted
	S32 							removeStringNoUndo(S32 pos, S32 length);
	S32								overwriteCharNoUndo(S32 pos, llwchar wc);
	void							appendAndHighlightText(const std::string &new_text, bool prepend_newline, S32 highlight_part, const LLStyle::Params& stylep);


	// manage segments 
	void                			getSegmentAndOffset( S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp ) const;
	void                			getSegmentAndOffset( S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp );
	LLTextSegmentPtr    			getSegmentAtLocalPos( S32 x, S32 y );
	segment_set_t::iterator			getSegIterContaining(S32 index);
	segment_set_t::const_iterator	getSegIterContaining(S32 index) const;
	void                			clearSegments();
	void							createDefaultSegment();
	virtual void					updateSegments();
	void							insertSegment(LLTextSegmentPtr segment_to_insert);
	LLStyle::Params					getDefaultStyleParams();

	//  manage lines
	S32								getLineStart( S32 line ) const;
	S32								getLineEnd( S32 line ) const;
	S32								getLineNumFromDocIndex( S32 doc_index, bool include_wordwrap = true) const;
	S32								getLineOffsetFromDocIndex( S32 doc_index, bool include_wordwrap = true) const;
	S32								getFirstVisibleLine() const;
	std::pair<S32, S32>				getVisibleLines(bool fully_visible = false);
	S32								getLeftOffset(S32 width);
	void							reflow(S32 start_index = 0);

	// cursor
	void							updateCursorXPos();
	void							setCursorAtLocalPos( S32 local_x, S32 local_y, bool round, bool keep_cursor_offset=false );
	S32								getEditableIndex(S32 index, bool increasing_direction); // constraint cursor to editable segments of document
	void							resetCursorBlink() { mCursorBlinkTimer.reset(); }
	void							updateScrollFromCursor();

	// text selection
	bool							hasSelection() const { return (mSelectionStart !=mSelectionEnd); }
	void 							startSelection();
	void 							endSelection();

	// misc
	void							updateRects();
	void							needsScroll() { mScrollNeeded = TRUE; }
	void							replaceUrlLabel(const std::string &url, const std::string &label);

protected:
	// text segmentation and flow
	segment_set_t       		mSegments;
	line_list_t					mLineInfoList;
	LLRect						mVisibleTextRect;			// The rect in which text is drawn.  Excludes borders.
	LLRect						mTextBoundingRect;

	// colors
	LLUIColor					mCursorColor;
	LLUIColor					mFgColor;
	LLUIColor					mReadOnlyFgColor;
	LLUIColor					mWriteableBgColor;
	LLUIColor					mReadOnlyBgColor;
	LLUIColor					mFocusBgColor;

	// cursor
	S32							mCursorPos;			// I-beam is just after the mCursorPos-th character.
	S32							mDesiredXPixel;		// X pixel position where the user wants the cursor to be
	LLFrameTimer				mCursorBlinkTimer;  // timer that controls cursor blinking

	// selection
	S32							mSelectionStart;
	S32							mSelectionEnd;
	
	BOOL						mIsSelecting;		// Are we in the middle of a drag-select? 

	// configuration
	S32							mHPad;				// padding on left of text
	S32							mVPad;				// padding above text
	LLFontGL::HAlign			mHAlign;
	F32							mLineSpacingMult;	// multiple of line height used as space for a single line of text (e.g. 1.5 to get 50% padding)
	S32							mLineSpacingPixels;	// padding between lines
	const LLFontGL*				mDefaultFont;		// font that is used when none specified
	LLFontGL::ShadowType		mFontShadow;
	bool						mBorderVisible;
	bool                		mParseHTML;			// make URLs interactive
	bool						mParseHighlights;	// highlight user-defined keywords
	bool                		mWordWrap;
	bool						mUseEllipses;
	bool						mTrackEnd;			// if true, keeps scroll position at end of document during resize
	bool						mReadOnly;
	bool						mBGVisible;			// render background?
	bool						mClipPartial;		// false if we show lines that are partially inside bounding rect
	S32							mMaxTextByteLength;	// Maximum length mText is allowed to be in bytes

	// support widgets
	LLContextMenu*				mPopupMenu;
	LLView*						mDocumentView;
	class LLScrollContainer*	mScroller;

	// transient state
	bool						mReflowNeeded;		// need to reflow text because of change to text contents or display region
	bool						mScrollNeeded;		// need to change scroll region because of change to cursor position
	S32							mScrollIndex;		// index of first character to keep visible in scroll region

};

///
/// A text segment is used to specify a subsection of a text string
/// that should be formatted differently, such as a hyperlink. It
/// includes a start/end offset from the start of the string, a
/// style to render with, an optional tooltip, etc.
///
class LLTextSegment : public LLRefCount, public LLMouseHandler
{
public:
	LLTextSegment(S32 start, S32 end) : mStart(start), mEnd(end){};
	virtual ~LLTextSegment();

	virtual bool				getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const;
	virtual S32					getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;
	virtual S32					getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	virtual void				updateLayout(const class LLTextBase& editor);
	virtual F32					draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	virtual bool				canEdit() const;
	virtual void				unlinkFromDocument(class LLTextBase* editor);
	virtual void				linkToDocument(class LLTextBase* editor);

	virtual const LLColor4&		getColor() const;
	//virtual void 				setColor(const LLColor4 &color);
	virtual LLStyleConstSP		getStyle() const;
	virtual void 				setStyle(LLStyleConstSP &style);
	virtual void				setToken( LLKeywordToken* token );
	virtual LLKeywordToken*		getToken() const;
	virtual void				setToolTip(const std::string& tooltip);
	virtual void				dump() const;

	// LLMouseHandler interface
	/*virtual*/ BOOL			handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleMiddleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleRightMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL			handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL			handleToolTip(S32 x, S32 y, MASK mask);
	/*virtual*/ std::string		getName() const;
	/*virtual*/ void			onMouseCaptureLost();
	/*virtual*/ void			screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
	/*virtual*/ void			localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;
	/*virtual*/ BOOL			hasMouseCapture();

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
	LLNormalTextSegment( LLStyleConstSP& style, S32 start, S32 end, LLTextBase& editor );
	LLNormalTextSegment( const LLColor4& color, S32 start, S32 end, LLTextBase& editor, BOOL is_visible = TRUE);
	~LLNormalTextSegment();

	/*virtual*/ bool				getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const;
	/*virtual*/ S32					getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;
	/*virtual*/ S32					getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	/*virtual*/ F32					draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	/*virtual*/ bool				canEdit() const { return true; }
	/*virtual*/ const LLColor4&		getColor() const					{ return mStyle->getColor(); }
	/*virtual*/ LLStyleConstSP		getStyle() const					{ return mStyle; }
	/*virtual*/ void 				setStyle(LLStyleConstSP &style)	{ mStyle = style; }
	/*virtual*/ void				setToken( LLKeywordToken* token )	{ mToken = token; }
	/*virtual*/ LLKeywordToken*		getToken() const					{ return mToken; }
	/*virtual*/ BOOL				getToolTip( std::string& msg ) const;
	/*virtual*/ void				setToolTip(const std::string& tooltip);
	/*virtual*/ void				dump() const;

	/*virtual*/ BOOL				handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL				handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL				handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL				handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL				handleToolTip(S32 x, S32 y, MASK mask);

protected:
	F32					drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, LLRect rect);

protected:
	class LLTextBase&	mEditor;
	LLStyleConstSP		mStyle;
	S32					mFontHeight;
	LLKeywordToken* 	mToken;
	std::string     	mTooltip;
	boost::signals2::connection mImageLoadedConnection;
};

class LLIndexSegment : public LLTextSegment
{
public:
	LLIndexSegment(S32 pos) : LLTextSegment(pos, pos) {}
};

class LLInlineViewSegment : public LLTextSegment
{
public:
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<LLView*>		view;
		Optional<bool>			force_newline;
		Optional<S32>			left_pad,
								right_pad,
								bottom_pad,
								top_pad;
	};

	LLInlineViewSegment(const Params& p, S32 start, S32 end);
	~LLInlineViewSegment();
	/*virtual*/ bool		getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const;
	/*virtual*/ S32			getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	/*virtual*/ void		updateLayout(const class LLTextBase& editor);
	/*virtual*/ F32			draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	/*virtual*/ bool		canEdit() const { return false; }
	/*virtual*/ void		unlinkFromDocument(class LLTextBase* editor);
	/*virtual*/ void		linkToDocument(class LLTextBase* editor);

private:
	S32 mLeftPad;
	S32 mRightPad;
	S32 mTopPad;
	S32 mBottomPad;
	LLView* mView;
	bool	mForceNewLine;
};


#endif
