/**
 * @file lllineeditor.h
 * @brief Text editor widget to let users enter/edit a single line.
 *
 * Features:
 *      Text entry of a single line (text, delete, left and right arrow, insert, return).
 *      Callbacks either on every keystroke or just on the return key.
 *      Focus (allow multiple text entry widgets)
 *      Clipboard (cut, copy, and paste)
 *      Horizontal scrolling to allow strings longer than widget size allows
 *      Pre-validation (limit which keys can be used)
 *      Optional line history so previous entries can be recalled by CTRL UP/DOWN
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLLINEEDITOR_H
#define LL_LLLINEEDITOR_H

#include "v4color.h"
#include "llframetimer.h"

#include "lleditmenuhandler.h"
#include "llfontvertexbuffer.h"
#include "llspellcheckmenuhandler.h"
#include "lluictrl.h"
#include "lluiimage.h"
#include "lluistring.h"
#include "llviewborder.h"

#include "llpreeditor.h"
#include "lltextvalidate.h"

class LLFontGL;
class LLLineEditorRollback;
class LLButton;
class LLContextMenu;

class LLLineEditor
: public LLUICtrl, public LLEditMenuHandler, protected LLPreeditor, public LLSpellCheckMenuHandler
{
public:

    typedef boost::function<void (LLLineEditor* caller)> keystroke_callback_t;

    struct MaxLength : public LLInitParam::ChoiceBlock<MaxLength>
    {
        Alternative<S32> bytes, chars;

        MaxLength() : bytes("max_length_bytes", 254),
                      chars("max_length_chars", 0)
        {}
    };

    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<std::string>           default_text;
        Optional<MaxLength>             max_length;
        Optional<keystroke_callback_t>  keystroke_callback;

        Optional<LLTextValidate::Validator, LLTextValidate::Validators> prevalidator;
        Optional<LLTextValidate::Validator, LLTextValidate::Validators> input_prevalidator;

        Optional<LLViewBorder::Params>  border;

        Optional<LLUIImage*>            background_image,
                                        background_image_disabled,
                                        background_image_focused;

        Optional<bool>                  select_on_focus,
                                        revert_on_esc,
                                        spellcheck,
                                        commit_on_focus_lost,
                                        ignore_tab,
                                        bg_image_always_focused,
                                        show_label_focused,
                                        is_password,
                                        allow_emoji,
                                        use_bg_color;

        // colors
        Optional<LLUIColor>             cursor_color,
                                        bg_color,
                                        text_color,
                                        text_readonly_color,
                                        text_tentative_color,
                                        highlight_color,
                                        preedit_bg_color;

        Optional<S32>                   text_pad_left,
                                        text_pad_right;

        Ignored                         bg_visible;

        Params();
    };

    void initFromParams(const LLLineEditor::Params& params);

protected:
    LLLineEditor(const Params&);
    friend class LLUICtrlFactory;
    friend class LLFloaterEditUI;
    void showContextMenu(S32 x, S32 y);

public:
    virtual ~LLLineEditor();

    // mousehandler overrides
    /*virtual*/ bool    handleMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool    handleMouseUp(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool    handleHover(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool    handleDoubleClick(S32 x,S32 y,MASK mask) override;
    /*virtual*/ bool    handleMiddleMouseDown(S32 x,S32 y,MASK mask) override;
    /*virtual*/ bool    handleRightMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool    handleKeyHere(KEY key, MASK mask) override;
    /*virtual*/ bool    handleUnicodeCharHere(llwchar uni_char) override;
    /*virtual*/ void    onMouseCaptureLost() override;

    // LLEditMenuHandler overrides
    /*virtual*/ void    cut() override;
    /*virtual*/ bool    canCut() const override;
    /*virtual*/ void    copy() override;
    /*virtual*/ bool    canCopy() const override;
    /*virtual*/ void    paste() override;
    /*virtual*/ bool    canPaste() const override;

    virtual void    updatePrimary();
    virtual void    copyPrimary();
    virtual void    pastePrimary();
    virtual bool    canPastePrimary() const;

    /*virtual*/ void    doDelete() override;
    /*virtual*/ bool    canDoDelete() const override;

    /*virtual*/ void    selectAll() override;
    /*virtual*/ bool    canSelectAll() const override;

    /*virtual*/ void    deselect() override;
    /*virtual*/ bool    canDeselect() const override;

    // LLSpellCheckMenuHandler overrides
    /*virtual*/ bool    getSpellCheck() const override;

    /*virtual*/ const std::string& getSuggestion(U32 index) const override;
    /*virtual*/ U32     getSuggestionCount() const override;
    /*virtual*/ void    replaceWithSuggestion(U32 index) override;

    /*virtual*/ void    addToDictionary() override;
    /*virtual*/ bool    canAddToDictionary() const override;

    /*virtual*/ void    addToIgnore() override;
    /*virtual*/ bool    canAddToIgnore() const override;

    // Spell checking helper functions
    std::string         getMisspelledWord(U32 pos) const;
    bool                isMisspelledWord(U32 pos) const;
    void                onSpellCheckSettingsChange();

    // view overrides
    /*virtual*/ void    draw() override;
    /*virtual*/ void    reshape(S32 width, S32 height, bool called_from_parent = true) override;
    /*virtual*/ void    onFocusReceived() override;
    /*virtual*/ void    onFocusLost() override;
    /*virtual*/ void    setEnabled(bool enabled) override;

    // UI control overrides
    /*virtual*/ void    clear() override;
    /*virtual*/ void    onTabInto() override;
    /*virtual*/ void    setFocus(bool b) override;
    /*virtual*/ void    setRect(const LLRect& rect) override;
    /*virtual*/ bool    acceptsTextInput() const override;
    /*virtual*/ void    onCommit() override;
    /*virtual*/ bool    isDirty() const override;   // Returns true if user changed value at all
    /*virtual*/ void    resetDirty() override;      // Clear dirty state

    // assumes UTF8 text
    /*virtual*/ void    setValue(const LLSD& value) override;
    /*virtual*/ LLSD    getValue() const override;
    /*virtual*/ bool    setTextArg(const std::string& key, const LLStringExplicit& text) override;
    /*virtual*/ bool    setLabelArg(const std::string& key, const LLStringExplicit& text) override;

    void            setLabel(const LLStringExplicit &new_label) { mLabel = new_label; }
    const std::string&  getLabel()  { return mLabel.getString(); }

    void            setDefaultText() { setText(mDefaultText); }
    void            setText(const LLStringExplicit &new_text);

    const std::string& getText() const override { return mText.getString(); }
    const LLWString&   getWText() const    { return mText.getWString(); }
    LLWString getConvertedText() const; // trimmed text with paragraphs converted to newlines

    S32             getLength() const   { return mText.length(); }

    S32             getCursor() const   { return mCursorPos; }
    void            setCursor( S32 pos );
    void            setCursorToEnd();

    // set scroll to earliest position it can reasonable set
    void            resetScrollPosition();

    // Selects characters 'start' to 'end'.
    void            setSelection(S32 start, S32 end);
    /*virtual*/ void    getSelectionRange(S32 *position, S32 *length) const override;

    void            setCommitOnFocusLost( bool b )  { mCommitOnFocusLost = b; }
    void            setRevertOnEsc( bool b )        { mRevertOnEsc = b; }
    void            setKeystrokeOnEsc(bool b)       { mKeystrokeOnEsc = b; }

    void setCursorColor(const LLUIColor& c)          { mCursorColor = c; }
    const LLColor4& getCursorColor() const          { return mCursorColor.get(); }

    void setFgColor( const LLUIColor& c )            { mFgColor = c; }
    void setReadOnlyFgColor( const LLUIColor& c )    { mReadOnlyFgColor = c; }
    void setTentativeFgColor(const LLUIColor& c)     { mTentativeFgColor = c; }

    const LLColor4& getFgColor() const          { return mFgColor.get(); }
    const LLColor4& getReadOnlyFgColor() const  { return mReadOnlyFgColor.get(); }
    const LLColor4& getTentativeFgColor() const { return mTentativeFgColor.get(); }

    const LLFontGL* getFont() const override { return mGLFont; }
    void setFont(const LLFontGL* font);

    void            setIgnoreArrowKeys(bool b)      { mIgnoreArrowKeys = b; }
    void            setIgnoreTab(bool b)            { mIgnoreTab = b; }
    void            setPassDelete(bool b)           { mPassDelete = b; }
    void            setAllowEmoji(bool b)           { mAllowEmoji = b; }
    void            setDrawAsterixes(bool b);

    // get the cursor position of the beginning/end of the prev/next word in the text
    S32             prevWordPos(S32 cursorPos) const;
    S32             nextWordPos(S32 cursorPos) const;

    bool            hasSelection() const { return (mSelectionStart != mSelectionEnd); }
    void            startSelection();
    void            endSelection();
    void            extendSelection(S32 new_cursor_pos);
    void            deleteSelection();

    void            setSelectAllonFocusReceived(bool b);
    void            setSelectAllonCommit(bool b) { mSelectAllonCommit = b; }

    void            onKeystroke();
    typedef boost::function<void (LLLineEditor* caller, void* user_data)> callback_t;
    void            setKeystrokeCallback(callback_t callback, void* user_data);

    void            setMaxTextLength(S32 max_text_length);
    void            setMaxTextChars(S32 max_text_chars);
    // Manipulate left and right padding for text
    void getTextPadding(S32 *left, S32 *right);
    void setTextPadding(S32 left, S32 right);

    // Prevalidation controls which keystrokes can affect the editor
    void            setPrevalidate(LLTextValidate::Validator validator);
    // This method sets callback that prevents from:
    // - deleting, selecting, typing, cutting, pasting characters that are not valid.
    // Also callback that this method sets differs from setPrevalidate in a way that it validates just inputed
    // symbols, before existing text is modified, but setPrevalidate validates line after it was modified.
    void            setPrevalidateInput(LLTextValidate::Validator validator);
    static bool     postvalidateFloat(const std::string &str);

    bool            prevalidateInput(const LLWString& wstr);
    bool            evaluateFloat();

    // line history support:
    void            setEnableLineHistory( bool enabled ) { mHaveHistory = enabled; } // switches line history on or off
    void            updateHistory(); // stores current line in history

    void            setReplaceNewlinesWithSpaces(bool replace);

    void            resetContextMenu() { setContextMenu(NULL); };

    void            setBgImage(LLPointer<LLUIImage> image) { mBgImage = image; }
    void            setBgImageFocused(LLPointer<LLUIImage> image) { mBgImageFocused = image; }

    void setShowContextMenu(bool show) { mShowContextMenu = show; }
    bool getShowContextMenu() const { return mShowContextMenu; }

  private:
    // private helper methods

    void                    pasteHelper(bool is_primary);

    void            removeChar();
    void            addChar(const llwchar c);
    void            setCursorAtLocalPos(S32 local_mouse_x);
    S32             findPixelNearestPos(S32 cursor_offset = 0) const;
    S32             calcCursorPos(S32 mouse_x);
    bool            handleSpecialKey(KEY key, MASK mask);
    bool            handleSelectionKey(KEY key, MASK mask);
    void            updateTextPadding();

    // Draw the background image depending on enabled/focused state.
    void            drawBackground();

    //
    // private data members
    //
    void            updateAllowingLanguageInput();
    bool            hasPreeditString() const;
    // Implementation (overrides) of LLPreeditor
    /*virtual*/ void    resetPreedit() override;
    /*virtual*/ void    updatePreedit(const LLWString &preedit_string,
                        const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position) override;
    /*virtual*/ void    markAsPreedit(S32 position, S32 length) override;
    /*virtual*/ void    getPreeditRange(S32 *position, S32 *length) const override;
    /*virtual*/ bool    getPreeditLocation(S32 query_position, LLCoordGL *coord, LLRect *bounds, LLRect *control) const override;
    /*virtual*/ S32     getPreeditFontSize() const override;
    /*virtual*/ LLWString getPreeditString() const override { return getWText(); }

    void            setText(const LLStringExplicit &new_text, bool use_size_limit);

    void            setContextMenu(LLContextMenu* new_context_menu);

protected:
    LLUIString      mText;                  // The string being edited.
    std::string     mPrevText;              // Saved string for 'ESC' revert
    LLUIString      mLabel;                 // text label that is visible when no user text provided

    // line history support:
    bool        mHaveHistory;               // flag for enabled line history
    typedef std::vector<std::string>    line_history_t;
    line_history_t  mLineHistory;           // line history storage
    line_history_t::iterator    mCurrentHistoryLine;    // currently browsed history line

    LLViewBorder* mBorder;
    const LLFontGL* mGLFont;
    LLFontVertexBuffer mFontBufferPreSelection;
    LLFontVertexBuffer mFontBufferSelection;
    LLFontVertexBuffer mFontBufferPostSelection;
    LLFontVertexBuffer mFontBufferLabel;
    std::string mDefaultText;
    S32         mMaxLengthBytes;            // Max length of the UTF8 string in bytes
    S32         mMaxLengthChars;            // Maximum number of characters in the string
    S32         mCursorPos;                 // I-beam is just after the mCursorPos-th character.
    S32         mScrollHPos;                // Horizontal offset from the start of mText.  Used for scrolling.
    LLFrameTimer mScrollTimer;
    S32         mTextPadLeft;               // Used to reserve space before the beginning of the text for children.
    S32         mTextPadRight;              // Used to reserve space after the end of the text for children.
    S32         mTextLeftEdge;              // Pixels, cached left edge of text based on left padding and width
    S32         mTextRightEdge;             // Pixels, cached right edge of text based on right padding and width

    bool        mCommitOnFocusLost;
    bool        mRevertOnEsc;
    bool        mKeystrokeOnEsc;

    keystroke_callback_t mKeystrokeCallback;

    bool        mIsSelecting;               // Selection for clipboard operations
    S32         mSelectionStart;
    S32         mSelectionEnd;
    S32         mLastSelectionX;
    S32         mLastSelectionY;
    S32         mLastSelectionStart;
    S32         mLastSelectionEnd;

    bool        mSpellCheck;
    S32         mSpellCheckStart;
    S32         mSpellCheckEnd;
    LLTimer     mSpellCheckTimer;
    std::list<std::pair<U32, U32> > mMisspellRanges;
    std::vector<std::string>        mSuggestionList;

    LLTextValidate::Validator mPrevalidator;
    LLTextValidate::Validator mInputPrevalidator;

    LLFrameTimer mKeystrokeTimer;
    LLTimer     mTripleClickTimer;

    LLUIColor   mCursorColor;
    LLUIColor   mBgColor;
    LLUIColor   mFgColor;
    LLUIColor   mReadOnlyFgColor;
    LLUIColor   mTentativeFgColor;
    LLUIColor   mHighlightColor;        // background for selected text
    LLUIColor   mPreeditBgColor;        // preedit marker background color

    S32         mBorderThickness;

    bool        mIgnoreArrowKeys;
    bool        mIgnoreTab;
    bool        mDrawAsterixes;

    bool        mSelectAllonFocusReceived;
    bool        mSelectAllonCommit;
    bool        mPassDelete;

    bool        mReadOnly;

    bool        mShowImageFocused;
    bool        mShowLabelFocused;

    bool        mAllowEmoji;
    bool        mUseBgColor;

    LLWString   mPreeditWString;
    LLWString   mPreeditOverwrittenWString;
    std::vector<S32> mPreeditPositions;
    LLPreeditor::standouts_t mPreeditStandouts;

    LLHandle<LLContextMenu> mContextMenuHandle;

    bool mShowContextMenu;

private:
    // Instances that by default point to the statics but can be overidden in XML.
    LLPointer<LLUIImage> mBgImage;
    LLPointer<LLUIImage> mBgImageDisabled;
    LLPointer<LLUIImage> mBgImageFocused;

    bool        mReplaceNewlinesWithSpaces; // if false, will replace pasted newlines with paragraph symbol.

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

        void doRollback(LLLineEditor* ed) const
        {
            ed->mCursorPos = mCursorPos;
            ed->mScrollHPos = mScrollHPos;
            ed->mIsSelecting = mIsSelecting;
            ed->mSelectionStart = mSelectionStart;
            ed->mSelectionEnd = mSelectionEnd;
            ed->mText = mText;
            ed->mPrevText = mText;
        }

        std::string getText() const { return mText; }

    private:
        std::string mText;
        S32     mCursorPos;
        S32     mScrollHPos;
        bool    mIsSelecting;
        S32     mSelectionStart;
        S32     mSelectionEnd;
    }; // end class LLLineEditorRollback

}; // end class LLLineEditor

// Build time optimization, generate once in .cpp file
#ifndef LLLINEEDITOR_CPP
extern template class LLLineEditor* LLView::getChild<class LLLineEditor>(
    std::string_view name, bool recurse) const;
#endif

#endif  // LL_LINEEDITOR_
