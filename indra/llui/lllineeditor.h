/** 
 * @file lllineeditor.h
 * @brief Text editor widget to let users enter/edit a single line.
 *
 * Features: 
 *		Text entry of a single line (text, delete, left and right arrow, insert, return).
 *		Callbacks either on every keystroke or just on the return key.
 *		Focus (allow multiple text entry widgets)
 *		Clipboard (cut, copy, and paste)
 *		Horizontal scrolling to allow strings longer than widget size allows 
 *		Pre-validation (limit which keys can be used)
 *		Optional line history so previous entries can be recalled by CTRL UP/DOWN
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

#ifndef LL_LLLINEEDITOR_H
#define LL_LLLINEEDITOR_H

#include "v4color.h"
#include "llframetimer.h"

#include "lleditmenuhandler.h"
#include "lluictrl.h"
#include "lluistring.h"
#include "llviewborder.h"

#include "llpreeditor.h"

class LLFontGL;
class LLLineEditorRollback;
class LLButton;

typedef BOOL (*LLLinePrevalidateFunc)(const LLWString &wstr);


class LLLineEditor
: public LLUICtrl, public LLEditMenuHandler, protected LLPreeditor
{

public:
	LLLineEditor(const std::string& name, 
				 const LLRect& rect,
				 const std::string& default_text = LLStringUtil::null,
				 const LLFontGL* glfont = NULL,
				 S32 max_length_bytes = 254,
				 void (*commit_callback)(LLUICtrl* caller, void* user_data) = NULL,
				 void (*keystroke_callback)(LLLineEditor* caller, void* user_data) = NULL,
				 void (*focus_lost_callback)(LLFocusableElement* caller, void* user_data) = NULL,
				 void* userdata = NULL,
				 LLLinePrevalidateFunc prevalidate_func = NULL,
				 LLViewBorder::EBevel border_bevel = LLViewBorder::BEVEL_IN,
				 LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE,
				 S32 border_thickness = 1);

	virtual ~LLLineEditor();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	void setColorParameters(LLXMLNodePtr node);
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	static void cleanupLineEditor();

	// mousehandler overrides
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x,S32 y,MASK mask);
	/*virtual*/ BOOL	handleMiddleMouseDown(S32 x,S32 y,MASK mask);
	/*virtual*/ BOOL	handleKeyHere(KEY key, MASK mask );
	/*virtual*/ BOOL	handleUnicodeCharHere(llwchar uni_char);
	/*virtual*/ void	onMouseCaptureLost();

	// LLEditMenuHandler overrides
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
	virtual BOOL	canSelectAll() const;

	virtual void	deselect();
	virtual BOOL	canDeselect() const;

	// view overrides
	virtual void	draw();
	virtual void	reshape(S32 width,S32 height,BOOL called_from_parent=TRUE);
	virtual void	onFocusReceived();
	virtual void	onFocusLost();
	virtual void	setEnabled(BOOL enabled);

	// UI control overrides
	virtual void	clear();
	virtual void	onTabInto();
	virtual void	setFocus( BOOL b );
	virtual void 	setRect(const LLRect& rect);
	virtual BOOL	acceptsTextInput() const;
	virtual void	onCommit();
	virtual BOOL	isDirty() const { return mText.getString() != mPrevText; }	// Returns TRUE if user changed value at all
	virtual void	resetDirty() { mPrevText = mText.getString(); }		// Clear dirty state

	// assumes UTF8 text
	virtual void	setValue(const LLSD& value ) { setText(value.asString()); }
	virtual LLSD	getValue() const { return LLSD(getText()); }
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );

	void			setLabel(const LLStringExplicit &new_label) { mLabel = new_label; }
	void			setText(const LLStringExplicit &new_text);

	const std::string& getText() const		{ return mText.getString(); }
	const LLWString& getWText() const	{ return mText.getWString(); }
	LLWString getConvertedText() const; // trimmed text with paragraphs converted to newlines

	S32				getLength() const	{ return mText.length(); }

	S32				getCursor()	const	{ return mCursorPos; }
	void			setCursor( S32 pos );
	void			setCursorToEnd();

	// Selects characters 'start' to 'end'.
	void			setSelection(S32 start, S32 end);
	
	void			setCommitOnFocusLost( BOOL b )	{ mCommitOnFocusLost = b; }
	void			setRevertOnEsc( BOOL b )		{ mRevertOnEsc = b; }

	void setCursorColor(const LLColor4& c)			{ mCursorColor = c; }
	const LLColor4& getCursorColor() const			{ return mCursorColor; }

	void setFgColor( const LLColor4& c )			{ mFgColor = c; }
	void setReadOnlyFgColor( const LLColor4& c )	{ mReadOnlyFgColor = c; }
	void setTentativeFgColor(const LLColor4& c)		{ mTentativeFgColor = c; }
	void setWriteableBgColor( const LLColor4& c )	{ mWriteableBgColor = c; }
	void setReadOnlyBgColor( const LLColor4& c )	{ mReadOnlyBgColor = c; }
	void setFocusBgColor(const LLColor4& c)			{ mFocusBgColor = c; }

	const LLColor4& getFgColor() const			{ return mFgColor; }
	const LLColor4& getReadOnlyFgColor() const	{ return mReadOnlyFgColor; }
	const LLColor4& getTentativeFgColor() const { return mTentativeFgColor; }
	const LLColor4& getWriteableBgColor() const	{ return mWriteableBgColor; }
	const LLColor4& getReadOnlyBgColor() const	{ return mReadOnlyBgColor; }
	const LLColor4& getFocusBgColor() const		{ return mFocusBgColor; }

	void			setIgnoreArrowKeys(BOOL b)		{ mIgnoreArrowKeys = b; }
	void			setIgnoreTab(BOOL b)			{ mIgnoreTab = b; }
	void			setPassDelete(BOOL b)			{ mPassDelete = b; }
	void			setDrawAsterixes(BOOL b);

	// get the cursor position of the beginning/end of the prev/next word in the text
	S32				prevWordPos(S32 cursorPos) const;
	S32				nextWordPos(S32 cursorPos) const;

	BOOL			hasSelection() const { return (mSelectionStart != mSelectionEnd); }
	void			startSelection();
	void			endSelection();
	void			extendSelection(S32 new_cursor_pos);
	void			deleteSelection();

	void			setHandleEditKeysDirectly( BOOL b ) { mHandleEditKeysDirectly = b; }
	void			setSelectAllonFocusReceived(BOOL b);

	void			setKeystrokeCallback(void (*keystroke_callback)(LLLineEditor* caller, void* user_data));

	void			setMaxTextLength(S32 max_text_length);
	void			setTextPadding(S32 left, S32 right); // Used to specify room for children before or after text.

	// Prevalidation controls which keystrokes can affect the editor
	void			setPrevalidate( BOOL (*func)(const LLWString &) );
	static BOOL		prevalidateFloat(const LLWString &str );
	static BOOL		prevalidateInt(const LLWString &str );
	static BOOL		prevalidatePositiveS32(const LLWString &str);
	static BOOL		prevalidateNonNegativeS32(const LLWString &str);
	static BOOL		prevalidateAlphaNum(const LLWString &str );
	static BOOL		prevalidateAlphaNumSpace(const LLWString &str );
	static BOOL		prevalidatePrintableNotPipe(const LLWString &str); 
	static BOOL		prevalidatePrintableNoSpace(const LLWString &str);
	static BOOL		prevalidateASCII(const LLWString &str);

	static BOOL		postvalidateFloat(const std::string &str);

	// line history support:
	void			setEnableLineHistory( BOOL enabled ) { mHaveHistory = enabled; } // switches line history on or off 
	void			updateHistory(); // stores current line in history

	void			setReplaceNewlinesWithSpaces(BOOL replace);
	
private:
	// private helper methods

	void                    pasteHelper(bool is_primary);

	void			removeChar();
	void			addChar(const llwchar c);
	void			setCursorAtLocalPos(S32 local_mouse_x);
	S32				findPixelNearestPos(S32 cursor_offset = 0) const;
	void			reportBadKeystroke();
	BOOL			handleSpecialKey(KEY key, MASK mask);
	BOOL			handleSelectionKey(KEY key, MASK mask);
	BOOL			handleControlKey(KEY key, MASK mask);
	S32				handleCommitKey(KEY key, MASK mask);

	//
	// private data members
	//
	void			updateAllowingLanguageInput();
	BOOL			hasPreeditString() const;
	// Implementation (overrides) of LLPreeditor
	virtual void	resetPreedit();
	virtual void	updatePreedit(const LLWString &preedit_string,
						const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position);
	virtual void	markAsPreedit(S32 position, S32 length);
	virtual void	getPreeditRange(S32 *position, S32 *length) const;
	virtual void	getSelectionRange(S32 *position, S32 *length) const;
	virtual BOOL	getPreeditLocation(S32 query_position, LLCoordGL *coord, LLRect *bounds, LLRect *control) const;
	virtual S32		getPreeditFontSize() const;

protected:
	LLUIString		mText;					// The string being edited.
	std::string		mPrevText;				// Saved string for 'ESC' revert
	LLUIString		mLabel;					// text label that is visible when no user text provided

	// line history support:
	BOOL		mHaveHistory;				// flag for enabled line history
	std::vector<std::string> mLineHistory;		// line history storage
	U32			mCurrentHistoryLine;		// currently browsed history line

	LLViewBorder* mBorder;
	const LLFontGL*	mGLFont;
	S32			mMaxLengthBytes;			// Max length of the UTF8 string in bytes
	S32			mCursorPos;					// I-beam is just after the mCursorPos-th character.
	S32			mScrollHPos;				// Horizontal offset from the start of mText.  Used for scrolling.
	LLFrameTimer mScrollTimer;
	S32			mTextPadLeft;				// Used to reserve space before the beginning of the text for children.
	S32			mTextPadRight;				// Used to reserve space after the end of the text for children.
	S32			mMinHPixels;
	S32			mMaxHPixels;

	BOOL		mCommitOnFocusLost;
	BOOL		mRevertOnEsc;

	void		(*mKeystrokeCallback)( LLLineEditor* caller, void* userdata );

	BOOL		mIsSelecting;				// Selection for clipboard operations
	S32			mSelectionStart;
	S32			mSelectionEnd;
	S32			mLastSelectionX;
	S32			mLastSelectionY;
	S32			mLastSelectionStart;
	S32			mLastSelectionEnd;

	S32			(*mPrevalidateFunc)(const LLWString &str);

	LLFrameTimer mKeystrokeTimer;

	LLColor4	mCursorColor;

	LLColor4	mFgColor;
	LLColor4	mReadOnlyFgColor;
	LLColor4	mTentativeFgColor;
	LLColor4	mWriteableBgColor;
	LLColor4	mReadOnlyBgColor;
	LLColor4	mFocusBgColor;

	S32			mBorderThickness;

	BOOL		mIgnoreArrowKeys;
	BOOL		mIgnoreTab;
	BOOL		mDrawAsterixes;

	BOOL		mHandleEditKeysDirectly;  // If true, the standard edit keys (Ctrl-X, Delete, etc,) are handled here instead of routed by the menu system
	BOOL		mSelectAllonFocusReceived;
	BOOL		mPassDelete;

	BOOL		mReadOnly;

	LLWString	mPreeditWString;
	LLWString	mPreeditOverwrittenWString;
	std::vector<S32> mPreeditPositions;
	LLPreeditor::standouts_t mPreeditStandouts;

private:
	// Utility on top of LLUI::getUIImage, looks up a named image in a given XML node and returns it if possible
	// or returns a given default image if anything in the process fails.
	static LLPointer<LLUIImage> parseImage(std::string name, LLXMLNodePtr from, LLPointer<LLUIImage> def);
	// Global instance used as default for member instance below.
	static LLPointer<LLUIImage> sImage;
	// Instances that by default point to the statics but can be overidden in XML.
	LLPointer<LLUIImage> mImage;

	BOOL        mReplaceNewlinesWithSpaces; // if false, will replace pasted newlines with paragraph symbol.

	// private helper class
	class LLLineEditorRollback
	{
	public:
		LLLineEditorRollback( LLLineEditor* ed )
			:
			mCursorPos( ed->mCursorPos ),
			mScrollHPos( ed->mScrollHPos ),
			mIsSelecting( ed->mIsSelecting ),
			mSelectionStart( ed->mSelectionStart ),
			mSelectionEnd( ed->mSelectionEnd )
		{
			mText = ed->getText();
		}

		void doRollback( LLLineEditor* ed )
		{
			ed->mCursorPos = mCursorPos;
			ed->mScrollHPos = mScrollHPos;
			ed->mIsSelecting = mIsSelecting;
			ed->mSelectionStart = mSelectionStart;
			ed->mSelectionEnd = mSelectionEnd;
			ed->mText = mText;
			ed->mPrevText = mText;
		}

		std::string getText()   { return mText; }

	private:
		std::string mText;
		S32		mCursorPos;
		S32		mScrollHPos;
		BOOL	mIsSelecting;
		S32		mSelectionStart;
		S32		mSelectionEnd;
	}; // end class LLLineEditorRollback

}; // end class LLLineEditor



/*
 * @brief A line editor with a button to clear it and a callback to call on every edit event.
 */
class LLSearchEditor : public LLUICtrl
{
public:
	LLSearchEditor(const std::string& name, 
		const LLRect& rect,
		S32 max_length_bytes,
		void (*search_callback)(const std::string& search_string, void* user_data),
		void* userdata);

	virtual ~LLSearchEditor() {}

	/*virtual*/ void	draw();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	void setText(const LLStringExplicit &new_text) { mSearchEdit->setText(new_text); }

	void setSearchCallback(void (*search_callback)(const std::string& search_string, void* user_data), void* data) { mSearchCallback = search_callback; mCallbackUserData = data; }

	// LLUICtrl interface
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;
	virtual BOOL	setTextArg( const std::string& key, const LLStringExplicit& text );
	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );
	virtual void	clear();

private:
	static void onSearchEdit(LLLineEditor* caller, void* user_data );
	static void onClearSearch(void* user_data);

	LLLineEditor* mSearchEdit;
	class LLButton* mClearSearchButton;
	void (*mSearchCallback)(const std::string& search_string, void* user_data);

};

#endif  // LL_LINEEDITOR_
