/**
 * @file lltexteditor.h
 * @brief LLTextEditor base class
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

// Text editor widget to let users enter a a multi-line ASCII document//

#ifndef LL_LLTEXTEDITOR_H
#define LL_LLTEXTEDITOR_H

#include "llrect.h"
#include "llframetimer.h"
#include "llstyle.h"
#include "lleditmenuhandler.h"
#include "llviewborder.h" // for params
#include "llstring.h"
#include "lltextbase.h"
#include "lltextvalidate.h"

#include "llpreeditor.h"
#include "llcontrol.h"

class LLFontGL;
class LLScrollbar;
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
        Optional<std::string>   default_text;
        Optional<LLTextValidate::Validator, LLTextValidate::Validators> prevalidator;

        Optional<bool>          embedded_items,
                                ignore_tab,
                                commit_on_focus_lost,
                                show_context_menu,
                                show_emoji_helper,
                                enable_tooltip_paste,
                                auto_indent;

        //colors
        Optional<LLUIColor>     default_color;

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

    void    setKeystrokeCallback(const keystroke_signal_t::slot_type& callback);

    void    setParseHighlights(bool parsing) {mParseHighlights=parsing;}

    static S32      spacesPerTab();

    void    insertEmoji(llwchar emoji);
    void    handleEmojiCommit(llwchar emoji);

    // mousehandler overrides
    virtual bool    handleMouseDown(S32 x, S32 y, MASK mask);
    virtual bool    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual bool    handleRightMouseDown(S32 x, S32 y, MASK mask);
    virtual bool    handleHover(S32 x, S32 y, MASK mask);
    virtual bool    handleDoubleClick(S32 x, S32 y, MASK mask );
    virtual bool    handleMiddleMouseDown(S32 x,S32 y,MASK mask);

    virtual bool    handleKeyHere(KEY key, MASK mask );
    virtual bool    handleUnicodeCharHere(llwchar uni_char);

    virtual void    onMouseCaptureLost();

    // view overrides
    virtual void    draw();
    virtual void    onFocusReceived();
    virtual void    onFocusLost();
    virtual void    onCommit();
    virtual void    setEnabled(bool enabled);

    // uictrl overrides
    virtual void    clear();
    virtual void    setFocus( bool b );
    virtual bool    isDirty() const;

    // LLEditMenuHandler interface
    virtual void    undo();
    virtual bool    canUndo() const;
    virtual void    redo();
    virtual bool    canRedo() const;

    virtual void    cut();
    virtual bool    canCut() const;
    virtual void    copy();
    virtual bool    canCopy() const;
    virtual void    paste();
    virtual bool    canPaste() const;

    virtual void    updatePrimary();
    virtual void    copyPrimary();
    virtual void    pastePrimary();
    virtual bool    canPastePrimary() const;

    virtual void    doDelete();
    virtual bool    canDoDelete() const;
    virtual void    selectAll();
    virtual bool    canSelectAll()  const;

    void            selectByCursorPosition(S32 prev_cursor_pos, S32 next_cursor_pos);

    virtual bool    canLoadOrSaveToFile();

    void            selectNext(const std::string& search_text_in, bool case_insensitive, bool wrap = true);
    bool            replaceText(const std::string& search_text, const std::string& replace_text, bool case_insensitive, bool wrap = true);
    void            replaceTextAll(const std::string& search_text, const std::string& replace_text, bool case_insensitive);

    // Undo/redo stack
    void            blockUndo();

    // Text editing
    virtual void    makePristine();
    bool            isPristine() const;
    bool            allowsEmbeddedItems() const { return mAllowEmbeddedItems; }

    // Autoreplace (formerly part of LLLineEditor)
    typedef boost::function<void(S32&, S32&, LLWString&, S32&, const LLWString&)> autoreplace_callback_t;
    autoreplace_callback_t mAutoreplaceCallback;
    void            setAutoreplaceCallback(autoreplace_callback_t cb) { mAutoreplaceCallback = cb; }

    /*virtual*/ void    onSpellCheckPerformed();

    //
    // Text manipulation
    //

    // inserts text at cursor
    void            insertText(const std::string &text);
    void            insertText(LLWString &text);

    void            appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo);
    // Non-undoable
    void            setText(const LLStringExplicit &utf8str, const LLStyle::Params& input_params = LLStyle::Params());


    // Removes text from the end of document
    // Does not change highlight or cursor position.
    void            removeTextFromEnd(S32 num_chars);

    bool            tryToRevertToPristineState();

    void            setCursorAndScrollToEnd();

    void            getCurrentLineAndColumn( S32* line, S32* col, bool include_wordwrap );

    // Hacky methods to make it into a word-wrapping, potentially scrolling,
    // read-only text box.
    void            setCommitOnFocusLost(bool b)            { mCommitOnFocusLost = b; }

    // Hack to handle Notecards
    virtual bool    importBuffer(const char* buffer, S32 length );
    virtual bool    exportBuffer(std::string& buffer );

    const LLUUID&   getSourceID() const                     { return mSourceID; }

    const LLTextSegmentPtr  getPreviousSegment() const;
    void            getSelectedSegments(segment_vec_t& segments) const;

    void            setShowContextMenu(bool show) { mShowContextMenu = show; }
    bool            getShowContextMenu() const { return mShowContextMenu; }

    void            showEmojiHelper();
    void            hideEmojiHelper();
    void            setShowEmojiHelper(bool show);
    bool            getShowEmojiHelper() const { return mShowEmojiHelper; }

    void            setPassDelete(bool b) { mPassDelete = b; }

protected:
    void            showContextMenu(S32 x, S32 y);
    void            drawPreeditMarker();

    void            removeCharOrTab();

    void            indentSelectedLines( S32 spaces );
    S32             indentLine( S32 pos, S32 spaces );
    void            unindentLineBeforeCloseBrace();

    virtual bool    handleSpecialKey(const KEY key, const MASK mask);
    bool            handleNavigationKey(const KEY key, const MASK mask);
    bool            handleSelectionKey(const KEY key, const MASK mask);
    bool            handleControlKey(const KEY key, const MASK mask);

    bool            selectionContainsLineBreaks();
    void            deleteSelection(bool transient_operation);

    S32             prevWordPos(S32 cursorPos) const;
    S32             nextWordPos(S32 cursorPos) const;

    void            autoIndent();

    void            getSegmentsInRange(segment_vec_t& segments, S32 start, S32 end, bool include_partial) const;

    virtual llwchar pasteEmbeddedItem(llwchar ext_char) { return ext_char; }


    // Here's the method that takes and applies text commands.
    S32             execute(TextCmd* cmd);

    // Undoable operations
    void            addChar(llwchar c); // at mCursorPos
    S32             addChar(S32 pos, llwchar wc);
public:
    void            addLineBreakChar(bool group_together = false);
protected:
    S32             overwriteChar(S32 pos, llwchar wc);
    void            removeChar();
    S32             removeChar(S32 pos);
    S32             insert(S32 pos, const LLWString &wstr, bool group_with_next_op, LLTextSegmentPtr segment);
    S32             remove(S32 pos, S32 length, bool group_with_next_op);

    void            tryToShowEmojiHelper();
    void            focusLostHelper();
    void            updateAllowingLanguageInput();
    bool            hasPreeditString() const;

    // Overrides LLPreeditor
    virtual void    resetPreedit();
    virtual void    updatePreedit(const LLWString &preedit_string,
                        const segment_lengths_t &preedit_segment_lengths, const standouts_t &preedit_standouts, S32 caret_position);
    virtual void    markAsPreedit(S32 position, S32 length);
    virtual void    getPreeditRange(S32 *position, S32 *length) const;
    virtual void    getSelectionRange(S32 *position, S32 *length) const;
    virtual bool    getPreeditLocation(S32 query_offset, LLCoordGL *coord, LLRect *bounds, LLRect *control) const;
    virtual S32     getPreeditFontSize() const;
    virtual LLWString getPreeditString() const { return getWText(); }

    virtual bool    useFontBuffers() const { return getReadOnly(); }
    //
    // Protected data
    //
    // Probably deserves serious thought to hiding as many of these
    // as possible behind protected accessor methods.
    //

    // Use these to determine if a click on an embedded item is a drag or not.
    S32             mMouseDownX;
    S32             mMouseDownY;

    LLWString           mPreeditWString;
    LLWString           mPreeditOverwrittenWString;
    std::vector<S32>    mPreeditPositions;
    LLPreeditor::standouts_t mPreeditStandouts;

protected:
    LLUIColor           mDefaultColor;

    bool                mAutoIndent;
    bool                mParseOnTheFly;

    void                updateLinkSegments();
    void                keepSelectionOnReturn(bool keep) { mKeepSelectionOnReturn = keep; }
    class LLViewBorder* mBorder;

private:
    //
    // Methods
    //
    void            pasteHelper(bool is_primary);
    void            cleanStringForPaste(LLWString& clean_string);

public:
    template <typename STRINGTYPE>
    void            pasteTextWithLinebreaks(const STRINGTYPE& clean_string, bool reset_cursor = false)
    {
        pasteTextWithLinebreaksImpl(ll_convert(clean_string), reset_cursor);
    }
    void            pasteTextWithLinebreaksImpl(const LLWString& clean_string, bool reset_cursor = false);

private:
    void            onKeyStroke();

    // Concrete TextCmd sub-classes used by the LLTextEditor base class
    class TextCmdInsert;
    class TextCmdAddChar;
    class TextCmdOverwriteChar;
    class TextCmdRemove;

    bool            mBaseDocIsPristine;
    TextCmd*        mPristineCmd;

    TextCmd*        mLastCmd;

    typedef std::deque<TextCmd*> undo_stack_t;
    undo_stack_t    mUndoStack;

    bool            mTabsToNextField;       // if true, tab moves focus to next field, else inserts spaces
    bool            mCommitOnFocusLost;
    bool            mTakesFocus;

    bool            mAllowEmbeddedItems;
    bool            mShowContextMenu;
    bool            mShowEmojiHelper;
    bool            mEnableTooltipPaste;
    bool            mPassDelete;
    bool            mKeepSelectionOnReturn; // disabling of removing selected text after pressing of Enter

    LLUUID          mSourceID;

    LLCoordGL       mLastIMEPosition;       // Last position of the IME editor

    keystroke_signal_t mKeystrokeSignal;
    LLTextValidate::Validator mPrevalidator;

    LLHandle<LLContextMenu> mContextMenuHandle;
}; // end class LLTextEditor

// Build time optimization, generate once in .cpp file
#ifndef LLTEXTEDITOR_CPP
extern template class LLTextEditor* LLView::getChild<class LLTextEditor>(
    std::string_view name, bool recurse) const;
#endif

#endif  // LL_TEXTEDITOR_H
