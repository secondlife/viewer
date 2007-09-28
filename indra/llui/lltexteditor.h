/** 
 * @file lltexteditor.h
 * @brief LLTextEditor base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

class LLFontGL;
class LLScrollbar;
class LLViewBorder;
class LLKeywordToken;
class LLTextCmd;
class LLUICtrlFactory;

//
// Constants
//

const llwchar FIRST_EMBEDDED_CHAR = 0x100000;
const llwchar LAST_EMBEDDED_CHAR =  0x10ffff;
const S32 MAX_EMBEDDED_ITEMS = LAST_EMBEDDED_CHAR - FIRST_EMBEDDED_CHAR + 1;

//
// Classes
//
class LLTextSegment;
class LLTextCmd;

class LLTextEditor : public LLUICtrl, LLEditMenuHandler
{
	friend class LLTextCmd;
public:
	LLTextEditor(const LLString& name,
				 const LLRect& rect,
				 S32 max_length,
				 const LLString &default_text, 
				 const LLFontGL* glfont = NULL,
				 BOOL allow_embedded_items = FALSE);

	virtual ~LLTextEditor();

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_TEXT_EDITOR; }
	virtual LLString getWidgetTag() const;

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void    setTextEditorParameters(LLXMLNodePtr node);
	void	setParseHTML(BOOL parsing) {mParseHTML=parsing;}

	// mousehandler overrides
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask );
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent );
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);

	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept, LLString& tooltip_msg);
	virtual void	onMouseCaptureLost();


	// view overrides
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent);
	virtual void	draw();
	virtual void	onFocusLost();
	virtual void	setEnabled(BOOL enabled);

	// uictrl overrides
	virtual void	onTabInto();
	virtual void	clear();
	virtual void	setFocus( BOOL b );
	virtual BOOL	acceptsTextInput() const;
	virtual BOOL	isDirty() const;

	// LLEditMenuHandler interface
	virtual void	undo();
	virtual BOOL	canUndo();

	virtual void	redo();
	virtual BOOL	canRedo();

	virtual void	cut();
	virtual BOOL	canCut();

	virtual void	copy();
	virtual BOOL	canCopy();

	virtual void	paste();
	virtual BOOL	canPaste();
	
	virtual void	doDelete();
	virtual BOOL	canDoDelete();

	virtual void	selectAll();
	virtual BOOL	canSelectAll();

	virtual void	deselect();
	virtual BOOL	canDeselect();

	void			selectNext(const LLString& search_text_in, BOOL case_insensitive, BOOL wrap = TRUE);
	BOOL			replaceText(const LLString& search_text, const LLString& replace_text, BOOL case_insensitive, BOOL wrap = TRUE);
	void			replaceTextAll(const LLString& search_text, const LLString& replace_text, BOOL case_insensitive);
	
	// Undo/redo stack
	void			blockUndo();

	// Text editing
	virtual void	makePristine();
	BOOL			isPristine() const;

	// inserts text at cursor
	void			insertText(const LLString &text);
	// appends text at end
	void 			appendText(const LLString &wtext, bool allow_undo, bool prepend_newline,
							   const LLStyle* segment_style = NULL);

	void 			appendColoredText(const LLString &wtext, bool allow_undo, 
									  bool prepend_newline,
									  const LLColor4 &color,
									  const LLString& font_name = LLString::null);
	// if styled text starts a line, you need to prepend a newline.
	void 			appendStyledText(const LLString &new_text, bool allow_undo, 
									 bool prepend_newline,
									 const LLStyle* style);

	// Removes text from the end of document
	// Does not change highlight or cursor position.
	void 			removeTextFromEnd(S32 num_chars);

	BOOL			tryToRevertToPristineState();

	void			setCursor(S32 row, S32 column);
	void			setCursorPos(S32 offset);
	void			setCursorAndScrollToEnd();

	void			getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap );

	void			loadKeywords(const LLString& filename,
								 const LLDynamicArray<const char*>& funcs,
								 const LLDynamicArray<const char*>& tooltips,
								 const LLColor3& func_color);

	void 			setCursorColor(const LLColor4& c)			{ mCursorColor = c; }
	void 			setFgColor( const LLColor4& c )				{ mFgColor = c; }
	void 			setReadOnlyFgColor( const LLColor4& c )		{ mReadOnlyFgColor = c; }
	void 			setWriteableBgColor( const LLColor4& c )	{ mWriteableBgColor = c; }
	void 			setReadOnlyBgColor( const LLColor4& c )		{ mReadOnlyBgColor = c; }

	void			setTrackColor( const LLColor4& color );
	void			setThumbColor( const LLColor4& color );
	void			setHighlightColor( const LLColor4& color );
	void			setShadowColor( const LLColor4& color );

	// Hacky methods to make it into a word-wrapping, potentially scrolling,
	// read-only text box.
	void			setBorderVisible(BOOL b);
	void			setTakesNonScrollClicks(BOOL b);
	void			setHideScrollbarForShortDocs(BOOL b);

	void			setWordWrap( BOOL b );
	void			setTabToNextField(BOOL b)				{ mTabToNextField = b; }
	void			setCommitOnFocusLost(BOOL b)			{ mCommitOnFocusLost = b; }

	// If takes focus, will take keyboard focus on click.
	void			setTakesFocus(BOOL b)					{ mTakesFocus = b; }

	// Hack to handle Notecards
	virtual BOOL	importBuffer(const LLString& buffer );
	virtual BOOL	exportBuffer(LLString& buffer );

	void			setSourceID(const LLUUID& id) 			{ mSourceID = id; }
	void 			setAcceptCallingCardNames(BOOL enable)	{ mAcceptCallingCardNames = enable; }

	void			setHandleEditKeysDirectly( BOOL b ) 	{ mHandleEditKeysDirectly = b; }

	// Callbacks
	static void		setLinkColor(LLColor4 color) { mLinkColor = color; }
	static void		setURLCallbacks(	void (*callback1) (const char* url), 
										BOOL (*callback2) (LLString url)      ) 
										{ mURLcallback = callback1; mSecondlifeURLcallback = callback2;}

	void			setOnScrollEndCallback(void (*callback)(void*), void* userdata);

	// new methods
	void 			setValue(const LLSD& value);
	LLSD 			getValue() const;

 	const LLString&	getText() const;
	
	// Non-undoable
	void			setText(const LLString &utf8str);
	void			setWText(const LLWString &wtext);
	
	S32				getMaxLength() const 			{ return mMaxTextLength; }

	// Change cursor
	void			startOfLine();
	void			endOfLine();
	void			endOfDoc();
	
	// Getters
	const LLWString& getWText() const;
	llwchar			getWChar(S32 pos);
	LLWString		getWSubString(S32 pos, S32 len);
	
	LLTextSegment*	getCurrentSegment();
	LLTextSegment*  getPreviousSegment();
	void getSelectedSegments(std::vector<LLTextSegment*>& segments);

protected:
 	S32				getLength() const;
	void			getSegmentAndOffset( S32 startpos, S32* segidxp, S32* offsetp );

	void			drawBackground();
	void			drawSelectionBackground();
	void			drawCursor();
	void			drawText();
	void			drawClippedSegment(const LLWString &wtext, S32 seg_start, S32 seg_end, F32 x, F32 y, S32 selection_left, S32 selection_right, const LLStyle& color, F32* right_x);

	void			updateLineStartList(S32 startpos = 0);
	void			updateScrollFromCursor();
	void			updateTextRect();
	void			updateSegments();
	void			pruneSegments();

	void 			assignEmbedded(const LLString &s);
	void 			truncate();
	
	static BOOL		isPartOfWord(llwchar c);

	void			removeCharOrTab();
	void			setCursorAtLocalPos(S32 x, S32 y, BOOL round);
	S32				getCursorPosFromLocalCoord( S32 local_x, S32 local_y, BOOL round );

	void			indentSelectedLines( S32 spaces );
	S32				indentLine( S32 pos, S32 spaces );
	void			unindentLineBeforeCloseBrace();

	S32				getSegmentIdxAtOffset(S32 offset);
	LLTextSegment*	getSegmentAtLocalPos(S32 x, S32 y);
	LLTextSegment*	getSegmentAtOffset(S32 offset);

	void			reportBadKeystroke();

	BOOL			handleNavigationKey(const KEY key, const MASK mask);
	BOOL			handleSpecialKey(const KEY key, const MASK mask, BOOL* return_key_hit);
	BOOL			handleSelectionKey(const KEY key, const MASK mask);
	BOOL			handleControlKey(const KEY key, const MASK mask);
	BOOL			handleEditKey(const KEY key, const MASK mask);

	BOOL			hasSelection() { return (mSelectionStart !=mSelectionEnd); }
	BOOL			selectionContainsLineBreaks();
	void			startSelection();
	void			endSelection();
	void			deleteSelection(BOOL transient_operation);

	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;

	S32 			getLineCount();
	S32 			getLineStart( S32 line );
	void			getLineAndOffset(S32 pos, S32* linep, S32* offsetp);
	S32				getPos(S32 line, S32 offset);

	void			changePage(S32 delta);
	void			changeLine(S32 delta);

	void			autoIndent();

	S32 			execute(LLTextCmd* cmd);
	
	void			findEmbeddedItemSegments();
	
	virtual BOOL	handleMouseUpOverSegment(S32 x, S32 y, MASK mask);
	virtual llwchar	pasteEmbeddedItem(llwchar ext_char);
	virtual void	bindEmbeddedChars(const LLFontGL* font);
	virtual void	unbindEmbeddedChars(const LLFontGL* font);
	
	S32				findHTMLToken(const LLString &line, S32 pos, BOOL reverse);
	BOOL			findHTML(const LLString &line, S32 *begin, S32 *end);

protected:
	// Undoable operations
	void			addChar(llwchar c); // at mCursorPos
	S32				addChar(S32 pos, llwchar wc);
	S32				overwriteChar(S32 pos, llwchar wc);
	void			removeChar();
	S32 			removeChar(S32 pos);
	S32				insert(const S32 pos, const LLWString &wstr, const BOOL group_with_next_op);
	S32				remove(const S32 pos, const S32 length, const BOOL group_with_next_op);
	S32				append(const LLWString &wstr, const BOOL group_with_next_op);
	
	// direct operations
	S32				insertStringNoUndo(S32 pos, const LLWString &wstr); // returns num of chars actually inserted
	S32 			removeStringNoUndo(S32 pos, S32 length);
	S32				overwriteCharNoUndo(S32 pos, llwchar wc);
	
public:
	LLKeywords		mKeywords;
	static LLColor4 mLinkColor;
	static void			(*mURLcallback) (const char* url);
	static BOOL			(*mSecondlifeURLcallback) (LLString url);
protected:
	LLWString		mWText;
	mutable LLString mUTF8Text;
	mutable BOOL	mTextIsUpToDate;
	
	S32				mMaxTextLength;		// Maximum length mText is allowed to be

	const LLFontGL*	mGLFont;

	LLScrollbar*	mScrollbar;
	LLViewBorder*	mBorder;

	BOOL			mBaseDocIsPristine;
	LLTextCmd*		mPristineCmd;

	LLTextCmd*		mLastCmd;

	typedef std::deque<LLTextCmd*> undo_stack_t;
	undo_stack_t	mUndoStack;

	S32				mCursorPos;				// I-beam is just after the mCursorPos-th character.
	S32				mDesiredXPixel;			// X pixel position where the user wants the cursor to be
	LLRect			mTextRect;				// The rect in which text is drawn.  Excludes borders.
	// List of offsets and segment index of the start of each line.  Always has at least one node (0).
	struct line_info
	{
		line_info(S32 segment, S32 offset) : mSegment(segment), mOffset(offset) {}
		S32 mSegment;
		S32 mOffset;
	};
	struct line_info_compare
	{
		bool operator()(const line_info& a, const line_info& b) const
		{
			if (a.mSegment < b.mSegment)
				return true;
			else if (a.mSegment > b.mSegment)
				return false;
			else
				return a.mOffset < b.mOffset;
		}
	};
	typedef std::vector<line_info> line_list_t;
	line_list_t mLineStartList;
	
	// Are we in the middle of a drag-select?  To figure out if there is a current
	// selection, call hasSelection().
	BOOL			mIsSelecting;

	S32				mSelectionStart;
	S32				mSelectionEnd;

	void			(*mOnScrollEndCallback)(void*);
	void			*mOnScrollEndData;

	typedef std::vector<LLTextSegment *> segment_list_t;
	segment_list_t mSegments;
	LLTextSegment*	mHoverSegment;
	LLFrameTimer	mKeystrokeTimer;

	LLColor4		mCursorColor;

	LLColor4		mFgColor;
	LLColor4		mReadOnlyFgColor;
	LLColor4		mWriteableBgColor;
	LLColor4		mReadOnlyBgColor;
	LLColor4		mFocusBgColor;

	BOOL			mReadOnly;
	BOOL			mWordWrap;

	BOOL			mTabToNextField;		// if true, tab moves focus to next field, else inserts spaces
	BOOL			mCommitOnFocusLost;
	BOOL			mTakesFocus;
	BOOL			mHideScrollbarForShortDocs;
	BOOL			mTakesNonScrollClicks;

	BOOL			mAllowEmbeddedItems;

	BOOL 			mAcceptCallingCardNames;

	LLUUID			mSourceID;

	BOOL			mHandleEditKeysDirectly;  // If true, the standard edit keys (Ctrl-X, Delete, etc,) are handled here instead of routed by the menu system

	// Use these to determine if a click on an embedded item is a drag 
	// or not.
	S32				mMouseDownX;
	S32				mMouseDownY;

	S32				mLastSelectionX;
	S32				mLastSelectionY;

	BOOL			mParseHTML;
	LLString		mHTML;

	LLCoordGL		mLastIMEPosition;		// Last position of the IME editor
};

class LLTextSegment
{
public:
	// for creating a compare value
	LLTextSegment(S32 start);
	LLTextSegment( const LLStyle& style, S32 start, S32 end );
	LLTextSegment( const LLColor4& color, S32 start, S32 end, BOOL is_visible);
	LLTextSegment( const LLColor4& color, S32 start, S32 end );
	LLTextSegment( const LLColor3& color, S32 start, S32 end );

	S32					getStart()							{ return mStart; }
	S32					getEnd()							{ return mEnd; }
	void				setEnd( S32 end )					{ mEnd = end; }
	const LLColor4&		getColor()							{ return mStyle.getColor(); }
	void 				setColor(const LLColor4 &color)		{ mStyle.setColor(color); }
	const LLStyle&		getStyle()							{ return mStyle; }
	void 				setStyle(const LLStyle &style)		{ mStyle = style; }
	void 				setIsDefault(BOOL b)   				{ mIsDefault = b; }
	BOOL 				getIsDefault()		   				{ return mIsDefault; }

	void				setToken( LLKeywordToken* token )	{ mToken = token; }
	LLKeywordToken*		getToken()							{ return mToken; }
	BOOL				getToolTip( LLString& msg );

	void				dump();

	struct compare
	{
		bool operator()(const LLTextSegment* a, const LLTextSegment* b) const
		{
			return a->mStart < b->mStart;
		}
	};
	
private:
	LLStyle     mStyle;
	S32			mStart;
	S32			mEnd;
	LLKeywordToken* mToken;
	BOOL		mIsDefault;
};

class LLTextCmd
{
public:
	LLTextCmd( S32 pos, BOOL group_with_next )
		: mPos(pos),
		  mGroupWithNext(group_with_next)
	{
	}
	virtual			~LLTextCmd() {}
	virtual BOOL	execute(LLTextEditor* editor, S32* delta) = 0;
	virtual S32		undo(LLTextEditor* editor) = 0;
	virtual S32		redo(LLTextEditor* editor) = 0;
	virtual BOOL	canExtend(S32 pos);
	virtual void	blockExtensions();
	virtual BOOL	extendAndExecute( LLTextEditor* editor, S32 pos, llwchar c, S32* delta );
	virtual BOOL	hasExtCharValue( llwchar value );

	// Define these here so they can access LLTextEditor through the friend relationship
	S32				insert(LLTextEditor* editor, S32 pos, const LLWString &wstr);
	S32 			remove(LLTextEditor* editor, S32 pos, S32 length);
	S32				overwrite(LLTextEditor* editor, S32 pos, llwchar wc);
	
	BOOL			groupWithNext() { return mGroupWithNext; }
	
protected:
	S32			mPos;
	BOOL		mGroupWithNext;
};


#endif  // LL_TEXTEDITOR_
