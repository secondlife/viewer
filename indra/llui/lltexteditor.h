/** 
 * @file lltexteditor.h
 * @brief LLTextEditor base class
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

// Text editor widget to let users enter a a multi-line ASCII document//

#ifndef LL_LLTEXTEDITOR_H
#define LL_LLTEXTEDITOR_H

#include "llrect.h"
#include "llkeywords.h"
#include "lluictrl.h"
#include "llframetimer.h"
#include "lldarray.h"
#include "llstyle.h"
#include "lleditmenuhandler.h"
#include "lldarray.h"
#include "llviewborder.h" // for params
#include "lltextbase.h"

#include "llpreeditor.h"
#include "llcontrol.h"

class LLFontGL;
class LLScrollbar;
class LLKeywordToken;
class LLTextCmd;
class LLUICtrlFactory;
class LLScrollContainer;

class LLInlineViewSegment : public LLTextSegment
{
public:
	LLInlineViewSegment(LLView* widget, S32 start, S32 end);
	~LLInlineViewSegment();
	/*virtual*/ S32			getWidth(S32 first_char, S32 num_chars) const;
	/*virtual*/ S32			getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const;
	/*virtual*/ void		updateLayout(const class LLTextBase& editor);
	/*virtual*/ F32			draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect);
	/*virtuaL*/ S32			getMaxHeight() const;
	/*virtual*/ bool		canEdit() const { return false; }
	/*virtual*/ void		unlinkFromDocument(class LLTextBase* editor);
	/*virtual*/ void		linkToDocument(class LLTextBase* editor);

private:
	LLView* mView;
};

class LLTextEditor :
	public LLTextBase,
	public LLUICtrl,
	private LLEditMenuHandler,
	protected LLPreeditor
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<std::string>	default_text;
		Optional<S32>			max_text_length;

		Optional<bool>			read_only,
								embedded_items,
								word_wrap,
								ignore_tab,
								hide_border,
								track_bottom,
								handle_edit_keys_directly,
								show_line_numbers,
								commit_on_focus_lost;

		//colors
		Optional<LLUIColor>		cursor_color,
								default_color,
								text_color,
								text_readonly_color,
								bg_readonly_color,
								bg_writeable_color,
								bg_focus_color,
								link_color;

		Optional<LLViewBorder::Params> border;

		Ignored					type,
								length,
								is_unicode,
								hide_scrollbar;

		Params();
	};

	void initFromParams(const Params&);
protected:
	LLTextEditor(const Params&);
	friend class LLUICtrlFactory;
public:
	//
	// Constants
	//
	static const llwchar FIRST_EMBEDDED_CHAR = 0x100000;
	static const llwchar LAST_EMBEDDED_CHAR =  0x10ffff;
	static const S32 MAX_EMBEDDED_ITEMS = LAST_EMBEDDED_CHAR - FIRST_EMBEDDED_CHAR + 1;


	struct compare_segment_end
	{
		bool operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const
		{
			return a->getEnd() < b->getEnd();
		}
	};

	virtual ~LLTextEditor();

	void	setParseHighlights(BOOL parsing) {mParseHighlights=parsing;}

	// mousehandler overrides
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask );
	virtual BOOL	handleMiddleMouseDown(S32 x,S32 y,MASK mask);

	virtual BOOL	handleKeyHere(KEY key, MASK mask );
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept, std::string& tooltip_msg);
	virtual void	onMouseCaptureLost();

	// view overrides
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	draw();
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	onCommit();
	virtual void	setEnabled(BOOL enabled);

	// uictrl overrides
	virtual void	clear();
	virtual void	setFocus( BOOL b );
	virtual BOOL	acceptsTextInput() const;
	virtual BOOL	isDirty() const { return isPristine(); }
	virtual void 	setValue(const LLSD& value);

	// LLEditMenuHandler interface
	virtual void	undo();
	virtual BOOL	canUndo() const;
	virtual void	redo();
	virtual BOOL	canRedo() const;

	virtual void	cut();
	virtual BOOL	canCut() const;
	virtual void	copy();
	virtual BOOL	canCopy() const;
	virtual void	paste();
	virtual BOOL	canPaste() const;
 
	virtual void	updatePrimary();
	virtual void	copyPrimary();
	virtual void	pastePrimary();
	virtual BOOL	canPastePrimary() const;

	virtual void	doDelete();
	virtual BOOL	canDoDelete() const;
	virtual void	selectAll();
	virtual BOOL	canSelectAll()	const;
	virtual void	deselect();
	virtual BOOL	canDeselect() const;

	virtual void	onValueChange(S32 start, S32 end);

	void			selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap = TRUE);
	BOOL			replaceText(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive, BOOL wrap = TRUE);
	void			replaceTextAll(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive);
	BOOL			hasSelection() const		{ return (mSelectionStart !=mSelectionEnd); }
	void			replaceUrlLabel(const std::string &url, const std::string &label);
	
	// Undo/redo stack
	void			blockUndo();

	// Text editing
	virtual void	makePristine();
	BOOL			isPristine() const;
	BOOL			allowsEmbeddedItems() const { return mAllowEmbeddedItems; }
	S32				getLength() const { return getWText().length(); }
	void			setReadOnly(bool read_only) { mReadOnly = read_only; }
	bool			getReadOnly() { return mReadOnly; }

	//
	// Text manipulation
	//

	// inserts text at cursor
	void			insertText(const std::string &text);
	// appends text at end
	void 			appendText(const std::string &wtext, bool allow_undo, bool prepend_newline,
								const LLStyle::Params& style = LLStyle::Params());

	void 			appendColoredText(const std::string &wtext, bool allow_undo, 
									  bool prepend_newline,
									  const LLColor4 &color,
									  const std::string& font_name = LLStringUtil::null);
	// if styled text starts a line, you need to prepend a newline.
	void 			appendStyledText(const std::string &new_text, bool allow_undo, 
									 bool prepend_newline,
									 const LLStyle::Params& style);
	void			appendHighlightedText(const std::string &new_text,  bool allow_undo, 
										  bool prepend_newline,	 S32  highlight_part,
										  const LLStyle::Params& style);
	void			appendWidget(LLView* widget, const std::string &widget_text, bool allow_undo, bool prepend_newline);
	// Non-undoable
	void			setText(const LLStringExplicit &utf8str);
	void			setWText(const LLWString &wtext);


	// Removes text from the end of document
	// Does not change highlight or cursor position.
	void 			removeTextFromEnd(S32 num_chars);

	BOOL			tryToRevertToPristineState();

	bool			setCursor(S32 row, S32 column);
	bool			setCursorPos(S32 offset, bool keep_cursor_offset = false);
	void			setCursorAndScrollToEnd();

	void			getLineAndColumnForPosition( S32 position,  S32* line, S32* col, BOOL include_wordwrap );
	void			getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap );
	S32				getLineForPosition(S32 position);
	S32				getCurrentLine();

	void			loadKeywords(const std::string& filename,
								 const std::vector<std::string>& funcs,
								 const std::vector<std::string>& tooltips,
								 const LLColor3& func_color);
	LLKeywords::keyword_iterator_t keywordsBegin()	{ return mKeywords.begin(); }
	LLKeywords::keyword_iterator_t keywordsEnd()	{ return mKeywords.end(); }

	// Hacky methods to make it into a word-wrapping, potentially scrolling,
	// read-only text box.
	void			setCommitOnFocusLost(BOOL b)			{ mCommitOnFocusLost = b; }

	// Hack to handle Notecards
	virtual BOOL	importBuffer(const char* buffer, S32 length );
	virtual BOOL	exportBuffer(std::string& buffer );

	const class DocumentPanel*	getDocumentPanel() const { return mDocumentPanel; }

	const LLUUID&	getSourceID() const						{ return mSourceID; }

	// Callbacks
 	std::string     getText() const;
	
	// Callback for when a Url has been resolved by the server
	void            onUrlLabelUpdated(const std::string &url, const std::string &label);

	// Getters
	LLWString       getWText() const;
	llwchar			getWChar(S32 pos) const { return getWText()[pos]; }
	LLWString		getWSubString(S32 pos, S32 len) const { return getWText().substr(pos, len); }
	
	typedef std::vector<LLTextSegmentPtr> segment_vec_t;

	const LLTextSegmentPtr	getPreviousSegment() const;
	void getSelectedSegments(segment_vec_t& segments) const;

	void getSegmentsInRange(segment_vec_t& segments, S32 start, S32 end, bool include_partial) const;
	LLRect			getLocalRectFromDocIndex(S32 index) const; 

	void			addDocumentChild(LLView* view);
	void			removeDocumentChild(LLView* view);

protected:
	// Change cursor
	void			startOfLine();
	void			endOfLine();
	void			startOfDoc();
	void			endOfDoc();

	void			drawPreeditMarker();

	void			needsReflow() { mReflowNeeded = TRUE; }
	void			needsScroll() { mScrollNeeded = TRUE; }
	void			updateCursorXPos();

	void			updateScrollFromCursor();
	void			updateTextRect();
	const LLRect&	getTextRect() const { return mTextRect; }

	void 			assignEmbedded(const std::string &s);
	BOOL 			truncate();				// Returns true if truncation occurs
	
	void			removeCharOrTab();
	void			setCursorAtLocalPos(S32 x, S32 y, bool round, bool keep_cursor_offset = false);
	/*virtual*/ S32 getDocIndexFromLocalCoord( S32 local_x, S32 local_y, BOOL round ) const;

	void			indentSelectedLines( S32 spaces );
	S32				indentLine( S32 pos, S32 spaces );
	void			unindentLineBeforeCloseBrace();

	void			reportBadKeystroke() { make_ui_sound("UISndBadKeystroke"); }

	BOOL			handleNavigationKey(const KEY key, const MASK mask);
	BOOL			handleSpecialKey(const KEY key, const MASK mask, BOOL* return_key_hit);
	BOOL			handleSelectionKey(const KEY key, const MASK mask);
	BOOL			handleControlKey(const KEY key, const MASK mask);
	BOOL			handleEditKey(const KEY key, const MASK mask);

	BOOL			selectionContainsLineBreaks();
	void			startSelection();
	void			endSelection();
	void			deleteSelection(BOOL transient_operation);

	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;

	S32 			getLineCount() const { return mLineInfoList.size(); }
	S32 			getLineStart( S32 line ) const;
	S32 			getLineHeight( S32 line ) const;
	void			getLineAndOffset(S32 pos, S32* linep, S32* offsetp, bool include_wordwrap = true) const;
	S32				getPos(S32 line, S32 offset);

	void			changePage(S32 delta);
	void			changeLine(S32 delta);

	void			autoIndent();
	
	void			findEmbeddedItemSegments(S32 start, S32 end);
	void			insertSegment(LLTextSegmentPtr segment_to_insert);
	
	virtual llwchar	pasteEmbeddedItem(llwchar ext_char) { return ext_char; }
	
	// Abstract inner base class representing an undoable editor command.
	// Concrete sub-classes can be defined for operations such as insert, remove, etc.
	// Used as arguments to the execute() method below.
	class LLTextCmd
	{
	public:
		LLTextCmd( S32 pos, BOOL group_with_next, LLTextSegmentPtr segment = LLTextSegmentPtr() ) 
		:	mPos(pos), 
			mGroupWithNext(group_with_next)
		{
			if (segment.notNull())
			{
				mSegments.push_back(segment);
			}
		}
		virtual			~LLTextCmd() {}
		virtual BOOL	execute(LLTextEditor* editor, S32* delta) = 0;
		virtual S32		undo(LLTextEditor* editor) = 0;
		virtual S32		redo(LLTextEditor* editor) = 0;
		virtual BOOL	canExtend(S32 pos) const { return FALSE; }
		virtual void	blockExtensions() {}
		virtual BOOL	extendAndExecute( LLTextEditor* editor, S32 pos, llwchar c, S32* delta ) { llassert(0); return 0; }
		virtual BOOL	hasExtCharValue( llwchar value ) const { return FALSE; }

		// Defined here so they can access protected LLTextEditor editing methods
		S32				insert(LLTextEditor* editor, S32 pos, const LLWString &wstr) { return editor->insertStringNoUndo( pos, wstr, &mSegments ); }
		S32 			remove(LLTextEditor* editor, S32 pos, S32 length) { return editor->removeStringNoUndo( pos, length ); }
		S32				overwrite(LLTextEditor* editor, S32 pos, llwchar wc) { return editor->overwriteCharNoUndo(pos, wc); }
		
		S32				getPosition() const { return mPos; }
		BOOL			groupWithNext() const { return mGroupWithNext; }
		
	protected:
		const S32			mPos;
		BOOL				mGroupWithNext;
		segment_vec_t		mSegments;
	};
	// Here's the method that takes and applies text commands.
	S32 			execute(LLTextCmd* cmd);

	// Undoable operations
	void			addChar(llwchar c); // at mCursorPos
	S32				addChar(S32 pos, llwchar wc);
	S32				overwriteChar(S32 pos, llwchar wc);
	void			removeChar();
	S32 			removeChar(S32 pos);
	S32				insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment);
	S32				remove(S32 pos, S32 length, bool group_with_next_op);
	S32				append(const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment);
	
	// Direct operations
	S32				insertStringNoUndo(S32 pos, const LLWString &wstr, segment_vec_t* segments = NULL); // returns num of chars actually inserted
	S32 			removeStringNoUndo(S32 pos, S32 length);
	S32				overwriteCharNoUndo(S32 pos, llwchar wc);

	void			resetKeystrokeTimer() { mKeystrokeTimer.reset(); }

	void			updateAllowingLanguageInput();
	BOOL			hasPreeditString() const;

	// Overrides LLPreeditor
	virtual void	resetPreedit();
	virtual void	updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position);
	virtual void	markAsPreedit(S32 position, S32 length);
	virtual void	getPreeditRange(S32 *position, S32 *length) const;
	virtual void	getSelectionRange(S32 *position, S32 *length) const;
	virtual BOOL	getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const;
	virtual S32		getPreeditFontSize() const;
	//
	// Protected data
	//
	// Probably deserves serious thought to hiding as many of these 
	// as possible behind protected accessor methods.
	//

	// I-beam is just after the mCursorPos-th character.
	S32				mCursorPos;

	// Use these to determine if a click on an embedded item is a drag or not.
	S32				mMouseDownX;
	S32				mMouseDownY;
	
	// Are we in the middle of a drag-select?  To figure out if there is a current
	// selection, call hasSelection().
	BOOL			mIsSelecting;
	S32				mSelectionStart;
	S32				mSelectionEnd;
	S32				mLastSelectionX;
	S32				mLastSelectionY;

	BOOL			mParseHighlights;

	// Scrollbar data
	class DocumentPanel*	mDocumentPanel;
	LLScrollContainer*	mScroller;

	void			*mOnScrollEndData;

	LLWString			mPreeditWString;
	LLWString			mPreeditOverwrittenWString;
	std::vector<S32> 	mPreeditPositions;
	std::vector<BOOL> 	mPreeditStandouts;

	S32			mScrollIndex; // index into document that controls default scroll position

protected:
	LLUIColor		mCursorColor;
	LLUIColor		mFgColor;
	LLUIColor		mDefaultColor;
	LLUIColor		mReadOnlyFgColor;
	LLUIColor		mWriteableBgColor;
	LLUIColor		mReadOnlyBgColor;
	LLUIColor		mFocusBgColor;
	LLUIColor		mLinkColor;

	BOOL			mReadOnly;
	BOOL			mShowLineNumbers;

	void			updateSegments();
	void			updateLinkSegments();

private:

	//
	// Methods
	//
	void	        pasteHelper(bool is_primary);

	virtual 		LLTextViewModel* getViewModel() const;
	void			reflow(S32 startpos = 0);

	void			createDefaultSegment();
	LLStyleSP		getDefaultStyle();
	S32				getEditableIndex(S32 index, bool increasing_direction);

	void			drawBackground();
	void			drawSelectionBackground();
	void			drawCursor();
	void			drawText();
	void			drawLineNumbers();

	S32				getFirstVisibleLine() const;

	//
	// Data
	//
	LLKeywords		mKeywords;

	// Concrete LLTextCmd sub-classes used by the LLTextEditor base class
	class LLTextCmdInsert;
	class LLTextCmdAddChar;
	class LLTextCmdOverwriteChar;
	class LLTextCmdRemove;

	S32				mMaxTextByteLength;		// Maximum length mText is allowed to be in bytes

	class LLViewBorder*	mBorder;

	BOOL			mBaseDocIsPristine;
	LLTextCmd*		mPristineCmd;

	LLTextCmd*		mLastCmd;

	typedef std::deque<LLTextCmd*> undo_stack_t;
	undo_stack_t	mUndoStack;

	S32				mDesiredXPixel;			// X pixel position where the user wants the cursor to be
	LLRect			mTextRect;				// The rect in which text is drawn.  Excludes borders.
	// List of offsets and segment index of the start of each line.  Always has at least one node (0).
	struct line_info
	{
		line_info(S32 index_start, S32 index_end, S32 top, S32 bottom, S32 line_num) 
		:	mDocIndexStart(index_start), 
			mDocIndexEnd(index_end),
			mTop(top),
			mBottom(bottom),
			mLineNum(line_num)
		{}
		S32 mDocIndexStart;
		S32 mDocIndexEnd;
		S32 mTop;
		S32 mBottom;
		S32 mLineNum; // actual line count (ignoring soft newlines due to word wrap)
	};
	struct compare_bottom;
	struct compare_top;
	struct line_end_compare;
	typedef std::vector<line_info> line_list_t;
	line_list_t		mLineInfoList;
	BOOL			mReflowNeeded;
	BOOL			mScrollNeeded;

	LLFrameTimer	mKeystrokeTimer;

	BOOL			mTabsToNextField;		// if true, tab moves focus to next field, else inserts spaces
	BOOL			mCommitOnFocusLost;
	BOOL			mTakesFocus;
	BOOL			mTrackBottom;			// if true, keeps scroll position at bottom during resize

	BOOL			mAllowEmbeddedItems;

	LLUUID			mSourceID;

	// If true, the standard edit keys (Ctrl-X, Delete, etc,) are handled here 
	//instead of routed by the menu system
	BOOL			mHandleEditKeysDirectly;  

	LLCoordGL		mLastIMEPosition;		// Last position of the IME editor
}; // end class LLTextEditor


#endif  // LL_TEXTEDITOR_
