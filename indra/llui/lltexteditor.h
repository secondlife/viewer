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
class TextCmd;
class LLUICtrlFactory;
class LLScrollContainer;

class LLTextEditor :
	public LLTextBase,
	protected LLPreeditor
{
public:
	struct Params : public LLInitParam::Block<Params, LLTextBase::Params>
	{
		Optional<std::string>	default_text;

		Optional<bool>			embedded_items,
								ignore_tab,
								handle_edit_keys_directly,
								show_line_numbers,
								commit_on_focus_lost,
								show_context_menu;

		//colors
		Optional<LLUIColor>		default_color;

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

	virtual ~LLTextEditor();

	typedef boost::signals2::signal<void (LLTextEditor* caller)> keystroke_signal_t;

	void	setKeystrokeCallback(const keystroke_signal_t::slot_type& callback);

	void	setParseHighlights(BOOL parsing) {mParseHighlights=parsing;}

	// mousehandler overrides
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask );
	virtual BOOL	handleMiddleMouseDown(S32 x,S32 y,MASK mask);

	virtual BOOL	handleKeyHere(KEY key, MASK mask );
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

	virtual void	onMouseCaptureLost();

	// view overrides
	virtual void	draw();
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	onCommit();
	virtual void	setEnabled(BOOL enabled);

	// uictrl overrides
	virtual void	clear();
	virtual void	setFocus( BOOL b );
	virtual BOOL	isDirty() const;

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

	void			selectNext(const std::string& search_text_in, BOOL case_insensitive, BOOL wrap = TRUE);
	BOOL			replaceText(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive, BOOL wrap = TRUE);
	void			replaceTextAll(const std::string& search_text, const std::string& replace_text, BOOL case_insensitive);
	
	// Undo/redo stack
	void			blockUndo();

	// Text editing
	virtual void	makePristine();
	BOOL			isPristine() const;
	BOOL			allowsEmbeddedItems() const { return mAllowEmbeddedItems; }

	//
	// Text manipulation
	//

	// inserts text at cursor
	void			insertText(const std::string &text);

	void			appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo);
	// Non-undoable
	void			setText(const LLStringExplicit &utf8str, const LLStyle::Params& input_params = LLStyle::Params());


	// Removes text from the end of document
	// Does not change highlight or cursor position.
	void 			removeTextFromEnd(S32 num_chars);

	BOOL			tryToRevertToPristineState();

	void			setCursorAndScrollToEnd();

	void			getCurrentLineAndColumn( S32* line, S32* col, BOOL include_wordwrap );

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

	const LLUUID&	getSourceID() const						{ return mSourceID; }

	const LLTextSegmentPtr	getPreviousSegment() const;
	void getSelectedSegments(segment_vec_t& segments) const;

	void			setShowContextMenu(bool show) { mShowContextMenu = show; }
	bool			getChowContextMenu() const { return mShowContextMenu; }

protected:
	void			showContextMenu(S32 x, S32 y);
	void			drawPreeditMarker();

	void 			assignEmbedded(const std::string &s);
	
	void			removeCharOrTab();

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
	void			deleteSelection(BOOL transient_operation);

	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;

	void			autoIndent();
	
	void			findEmbeddedItemSegments(S32 start, S32 end);
	void			getSegmentsInRange(segment_vec_t& segments, S32 start, S32 end, bool include_partial) const;

	virtual llwchar	pasteEmbeddedItem(llwchar ext_char) { return ext_char; }
	

	// Here's the method that takes and applies text commands.
	S32 			execute(TextCmd* cmd);

	// Undoable operations
	void			addChar(llwchar c); // at mCursorPos
	S32				addChar(S32 pos, llwchar wc);
	S32				overwriteChar(S32 pos, llwchar wc);
	void			removeChar();
	S32 			removeChar(S32 pos);
	S32				insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment);
	S32				remove(S32 pos, S32 length, bool group_with_next_op);
	
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
	virtual LLWString getPreeditString() const { return getWText(); }
	//
	// Protected data
	//
	// Probably deserves serious thought to hiding as many of these 
	// as possible behind protected accessor methods.
	//

	// Use these to determine if a click on an embedded item is a drag or not.
	S32				mMouseDownX;
	S32				mMouseDownY;
	
	LLWString			mPreeditWString;
	LLWString			mPreeditOverwrittenWString;
	std::vector<S32> 	mPreeditPositions;
	std::vector<BOOL> 	mPreeditStandouts;

protected:
	LLUIColor			mDefaultColor;

	BOOL				mShowLineNumbers;

	/*virtual*/ void	updateSegments();
	void				updateLinkSegments();

private:
	//
	// Methods
	//
	void	        pasteHelper(bool is_primary);

	void			drawLineNumbers();

	void			onKeyStroke();

	//
	// Data
	//
	LLKeywords		mKeywords;

	// Concrete TextCmd sub-classes used by the LLTextEditor base class
	class TextCmdInsert;
	class TextCmdAddChar;
	class TextCmdOverwriteChar;
	class TextCmdRemove;

	class LLViewBorder*	mBorder;

	BOOL			mBaseDocIsPristine;
	TextCmd*		mPristineCmd;

	TextCmd*		mLastCmd;

	typedef std::deque<TextCmd*> undo_stack_t;
	undo_stack_t	mUndoStack;

	BOOL			mTabsToNextField;		// if true, tab moves focus to next field, else inserts spaces
	BOOL			mCommitOnFocusLost;
	BOOL			mTakesFocus;

	BOOL			mAllowEmbeddedItems;
	bool			mShowContextMenu;

	LLUUID			mSourceID;

	// If true, the standard edit keys (Ctrl-X, Delete, etc,) are handled here 
	//instead of routed by the menu system
	BOOL			mHandleEditKeysDirectly;  

	LLCoordGL		mLastIMEPosition;		// Last position of the IME editor

	keystroke_signal_t mKeystrokeSignal;

	LLContextMenu* mContextMenu;
}; // end class LLTextEditor

// Build time optimization, generate once in .cpp file
#ifndef LLTEXTEDITOR_CPP
extern template class LLTextEditor* LLView::getChild<class LLTextEditor>(
	const std::string& name, BOOL recurse) const;
#endif

#endif  // LL_TEXTEDITOR_
