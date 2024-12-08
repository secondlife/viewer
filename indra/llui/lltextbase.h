/**
 * @file lltextbase.h
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLTEXTBASE_H
#define LL_LLTEXTBASE_H

#include "v4color.h"
#include "lleditmenuhandler.h"
#include "llfontvertexbuffer.h"
#include "llspellcheckmenuhandler.h"
#include "llstyle.h"
#include "llkeywords.h"
#include "llpanel.h"
#include "lltextparser.h"

#include <string>
#include <vector>
#include <set>

#include <boost/signals2.hpp>

class LLScrollContainer;
class LLContextMenu;
class LLUrlMatch;
class LLTextBase;

///
/// A text segment is used to specify a subsection of a text string
/// that should be formatted differently, such as a hyperlink. It
/// includes a start/end offset from the start of the string, a
/// style to render with, an optional tooltip, etc.
///
class LLTextSegment
:   public LLRefCount,
    public LLMouseHandler
{
public:
    LLTextSegment(S32 start, S32 end)
    :   mStart(start),
        mEnd(end)
    {}
    virtual ~LLTextSegment();
    virtual LLTextSegmentPtr clone(LLTextBase& terget) const { return new LLTextSegment(mStart, mEnd); }

    bool                        getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const;

    virtual bool                getDimensionsF32(S32 first_char, S32 num_chars, F32& width, S32& height) const;
    virtual S32                 getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;

    /**
    * Get number of chars that fit into free part of current line.
    *
    * @param num_pixels - maximum width of rect
    * @param segment_offset - symbol in segment we start processing line from
    * @param line_offset - symbol in line after which segment starts
    * @param max_chars - limit of symbols that will fit in current line
    * @param line_ind - index of not word-wrapped string inside segment for multi-line segments.
    * Two string separated by word-wrap will have same index.
    * @return number of chars that will fit into current line
    */
    virtual S32                 getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars, S32 line_ind) const;
    virtual void                updateLayout(const class LLTextBase& editor);
    virtual F32                 draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);
    virtual bool                canEdit() const;
    virtual void                unlinkFromDocument(class LLTextBase* editor);
    virtual void                linkToDocument(class LLTextBase* editor);

    virtual const LLUIColor&     getColor() const;
    //virtual void              setColor(const LLUIColor &color);
    virtual LLStyleConstSP      getStyle() const;
    virtual void                setStyle(LLStyleConstSP style);
    virtual void                setToken( LLKeywordToken* token );
    virtual LLKeywordToken*     getToken() const;
    virtual void                setToolTip(const std::string& tooltip);
    virtual std::string         getToolTip() const;
    virtual void                dump() const;

    // LLMouseHandler interface
    /*virtual*/ bool            handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleMiddleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleMiddleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleRightMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ bool            handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool            handleScrollHWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool            handleToolTip(S32 x, S32 y, MASK mask);
    /*virtual*/ const std::string&  getName() const;
    /*virtual*/ void            onMouseCaptureLost();
    /*virtual*/ void            screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
    /*virtual*/ void            localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;
    /*virtual*/ bool            hasMouseCapture();

    S32                     getStart() const                    { return mStart; }
    void                    setStart(S32 start)                 { mStart = start; }
    S32                     getEnd() const                      { return mEnd; }
    void                    setEnd( S32 end )                   { mEnd = end; }

protected:
    S32             mStart;
    S32             mEnd;
};

class LLNormalTextSegment : public LLTextSegment
{
public:
    LLNormalTextSegment( LLStyleConstSP style, S32 start, S32 end, LLTextBase& editor );
    LLNormalTextSegment( const LLUIColor& color, S32 start, S32 end, LLTextBase& editor, bool is_visible = true);
    virtual ~LLNormalTextSegment();
    LLStyleConstSP cloneStyle(LLTextBase& target, const LLStyle* source) const;
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;

    /*virtual*/ bool                getDimensionsF32(S32 first_char, S32 num_chars, F32& width, S32& height) const;
    /*virtual*/ S32                 getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const;
    /*virtual*/ S32                 getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars, S32 line_ind) const;
    /*virtual*/ void                updateLayout(const class LLTextBase& editor);
    /*virtual*/ F32                 draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);
    /*virtual*/ bool                canEdit() const { return true; }
    /*virtual*/ const LLUIColor&     getColor() const                    { return mStyle->getColor(); }
    /*virtual*/ LLStyleConstSP      getStyle() const                    { return mStyle; }
    /*virtual*/ void                setStyle(LLStyleConstSP style)  { mStyle = style; }
    /*virtual*/ void                setToken( LLKeywordToken* token )   { mToken = token; }
    /*virtual*/ LLKeywordToken*     getToken() const                    { return mToken; }
    /*virtual*/ void                setToolTip(const std::string& tooltip);
    /*virtual*/ std::string         getToolTip() const { return mTooltip; }
    /*virtual*/ void                dump() const;

    /*virtual*/ bool                handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ bool                handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool                handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool                handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool                handleToolTip(S32 x, S32 y, MASK mask);

protected:
    virtual bool        useFontBuffers() const { return true; }
    F32                 drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, LLRectf rect);

    virtual     const LLWString&    getWText()  const;
    virtual     const S32           getLength() const;

protected:
    class LLTextBase&   mEditor;
    LLStyleConstSP      mStyle;
    S32                 mFontHeight;
    LLKeywordToken*     mToken;
    std::string         mTooltip;
    boost::signals2::connection mImageLoadedConnection;

    // font rendering
    LLFontVertexBuffer  mFontBufferPreSelection;
    LLFontVertexBuffer  mFontBufferSelection;
    LLFontVertexBuffer  mFontBufferPostSelection;
    S32                 mLastGeneration = -1;
};

// This text segment is the same as LLNormalTextSegment, the only difference
// is that LLNormalTextSegment draws value of LLTextBase (LLTextBase::getWText()),
// but LLLabelTextSegment draws label of the LLTextBase (LLTextBase::mLabel)
class LLLabelTextSegment : public LLNormalTextSegment
{
public:
    LLLabelTextSegment( LLStyleConstSP style, S32 start, S32 end, LLTextBase& editor );
    LLLabelTextSegment( const LLUIColor& color, S32 start, S32 end, LLTextBase& editor, bool is_visible = true);
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;

protected:

    /*virtual*/ const LLWString&    getWText()  const;
    /*virtual*/ const S32           getLength() const;
};

// Text segment that represents a single emoji character that has a different style (=font size) than the rest of
// the document it belongs to
class LLEmojiTextSegment : public LLNormalTextSegment
{
public:
    LLEmojiTextSegment(LLStyleConstSP style, S32 start, S32 end, LLTextBase& editor);
    LLEmojiTextSegment(const LLUIColor& color, S32 start, S32 end, LLTextBase& editor, bool is_visible = true);
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const override;

    F32 draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect) override;

    bool canEdit() const override { return false; }
    bool handleToolTip(S32 x, S32 y, MASK mask) override;
};

// Text segment that changes it's style depending of mouse pointer position ( is it inside or outside segment)
class LLOnHoverChangeableTextSegment : public LLNormalTextSegment
{
public:
    LLOnHoverChangeableTextSegment( LLStyleConstSP style, LLStyleConstSP normal_style, S32 start, S32 end, LLTextBase& editor );
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;
    /*virtual*/ F32 draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);
    /*virtual*/ bool handleHover(S32 x, S32 y, MASK mask);
protected:
    // Style used for text when mouse pointer is over segment
    LLStyleConstSP      mHoveredStyle;
    // Style used for text when mouse pointer is outside segment
    LLStyleConstSP      mNormalStyle;

};

class LLIndexSegment : public LLTextSegment
{
public:
    LLIndexSegment() : LLTextSegment(0, 0) {}
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const { return new LLIndexSegment(); }
};

class LLInlineViewSegment : public LLTextSegment
{
public:
    struct Params : public LLInitParam::Block<Params>
    {
        Mandatory<LLView*>      view;
        Optional<bool>          force_newline;
        Optional<S32>           left_pad,
                                right_pad,
                                bottom_pad,
                                top_pad;
    };

    LLInlineViewSegment(const Params& p, S32 start, S32 end);
    ~LLInlineViewSegment();
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;

    /*virtual*/ bool        getDimensionsF32(S32 first_char, S32 num_chars, F32& width, S32& height) const;
    /*virtual*/ S32         getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars, S32 line_ind) const;
    /*virtual*/ void        updateLayout(const class LLTextBase& editor);
    /*virtual*/ F32         draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);
    /*virtual*/ bool        canEdit() const { return false; }
    /*virtual*/ void        unlinkFromDocument(class LLTextBase* editor);
    /*virtual*/ void        linkToDocument(class LLTextBase* editor);

private:
    S32 mLeftPad;
    S32 mRightPad;
    S32 mTopPad;
    S32 mBottomPad;
    LLView* mView;
    bool    mForceNewLine;
};

class LLLineBreakTextSegment : public LLTextSegment
{
public:

    LLLineBreakTextSegment(LLStyleConstSP style, S32 pos);
    LLLineBreakTextSegment(S32 pos);
    ~LLLineBreakTextSegment();
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;
    /*virtual*/ bool        getDimensionsF32(S32 first_char, S32 num_chars, F32& width, S32& height) const;
    S32         getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars, S32 line_ind) const;
    F32         draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);

private:
    S32         mFontHeight;
};

class LLImageTextSegment : public LLTextSegment
{
public:
    LLImageTextSegment(LLStyleConstSP style, S32 pos,class LLTextBase& editor);
    ~LLImageTextSegment();
    /*virtual*/ LLTextSegmentPtr clone(LLTextBase& target) const;

    /*virtual*/ bool    getDimensionsF32(S32 first_char, S32 num_chars, F32& width, S32& height) const;
    /*virtual*/ S32     getNumChars(S32 num_pixels, S32 segment_offset, S32 char_offset, S32 max_chars, S32 line_ind) const;
    /*virtual*/ F32     draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRectf& draw_rect);

    /*virtual*/ bool    handleToolTip(S32 x, S32 y, MASK mask);
    /*virtual*/ void    setToolTip(const std::string& tooltip);

private:
    LLTextBase&     mEditor;
    LLStyleConstSP  mStyle;

protected:
    std::string     mTooltip;
};

typedef LLPointer<LLTextSegment> LLTextSegmentPtr;

///
/// The LLTextBase class provides a base class for all text fields, such
/// as LLTextEditor and LLTextBox. It implements shared functionality
/// such as Url highlighting and opening.
///
class LLTextBase
:   public LLUICtrl,
    protected LLEditMenuHandler,
    public LLSpellCheckMenuHandler,
    public ll::ui::SearchableControl
{
public:
    friend class LLTextSegment;
    friend class LLNormalTextSegment;
    friend class LLUICtrlFactory;

    typedef boost::signals2::signal<bool (const LLUUID& user_id)> is_friend_signal_t;
    typedef boost::signals2::signal<bool (const LLUUID& blocked_id, const std::string from)> is_blocked_signal_t;

    struct LineSpacingParams : public LLInitParam::ChoiceBlock<LineSpacingParams>
    {
        Alternative<F32>    multiple;
        Alternative<S32>    pixels;
        LineSpacingParams();
    };

    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLUIColor>     cursor_color,
                                text_color,
                                text_readonly_color,
                                text_tentative_color,
                                bg_readonly_color,
                                bg_writeable_color,
                                bg_focus_color,
                                text_selected_color,
                                bg_selected_color;

        Optional<bool>          bg_visible,
                                border_visible,
                                track_end,
                                read_only,
                                skip_link_underline,
                                spellcheck,
                                allow_scroll,
                                plain_text,
                                wrap,
                                use_ellipses,
                                use_emoji,
                                use_color,
                                parse_urls,
                                force_urls_external,
                                parse_highlights,
                                clip,
                                clip_partial,
                                trusted_content,
                                always_show_icons;

        Optional<S32>           v_pad,
                                h_pad;


        Optional<LineSpacingParams>
                                line_spacing;

        Optional<S32>           max_text_length;

        Optional<LLFontGL::ShadowType>  font_shadow;

        Optional<LLFontGL::VAlign> text_valign;

        Params();
    };

    // LLMouseHandler interface
    /*virtual*/ bool        handleMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleMouseUp(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleMiddleMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleMiddleMouseUp(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleRightMouseDown(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleRightMouseUp(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleDoubleClick(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleHover(S32 x, S32 y, MASK mask) override;
    /*virtual*/ bool        handleScrollWheel(S32 x, S32 y, S32 clicks) override;
    /*virtual*/ bool        handleToolTip(S32 x, S32 y, MASK mask) override;

    // LLView interface
    /*virtual*/ void        reshape(S32 width, S32 height, bool called_from_parent = true) override;
    /*virtual*/ void        draw() override;

    // LLUICtrl interface
    /*virtual*/ bool        acceptsTextInput() const override { return !mReadOnly; }
    /*virtual*/ void        setColor(const LLUIColor& c) override;
    virtual     void        setReadOnlyColor(const LLUIColor& c);
    /*virtual*/ void        onVisibilityChange(bool new_visibility) override;

    /*virtual*/ void        setValue(const LLSD& value) override;
    /*virtual*/ LLTextViewModel* getViewModel() const override;

    // LLEditMenuHandler interface
    /*virtual*/ bool        canDeselect() const override;
    /*virtual*/ void        deselect() override;

    virtual void    onFocusReceived() override;
    virtual void    onFocusLost() override;

    void        setParseHTML(bool parse_html) { mParseHTML = parse_html; }

    // LLSpellCheckMenuHandler overrides
    /*virtual*/ bool        getSpellCheck() const override;

    /*virtual*/ const std::string& getSuggestion(U32 index) const override;
    /*virtual*/ U32         getSuggestionCount() const override;
    /*virtual*/ void        replaceWithSuggestion(U32 index) override;

    /*virtual*/ void        addToDictionary() override;
    /*virtual*/ bool        canAddToDictionary() const override;

    /*virtual*/ void        addToIgnore() override;
    /*virtual*/ bool        canAddToIgnore() const override;

    // Spell checking helper functions
    std::string             getMisspelledWord(U32 pos) const;
    bool                    isMisspelledWord(U32 pos) const;
    void                    onSpellCheckSettingsChange();
    virtual void            onSpellCheckPerformed(){}

    // used by LLTextSegment layout code
    bool                    getWordWrap() const { return mWordWrap; }
    bool                    getUseEllipses() const { return mUseEllipses; }
    bool                    getUseEmoji() const { return mUseEmoji; }
    void                    setUseEmoji(bool value) { mUseEmoji = value; }
    bool                    getUseColor() const { return mUseColor; }
    void                    setUseColor(bool value) { mUseColor = value; }
    bool                    truncate(); // returns true of truncation occurred

    bool                    isContentTrusted() const { return mTrustedContent; }
    void                    setContentTrusted(bool trusted_content) { mTrustedContent = trusted_content; }

    // TODO: move into LLTextSegment?
    void                    createUrlContextMenu(S32 x, S32 y, const std::string &url, const std::string& text); // create a popup context menu for the given Url

    // Text accessors
    // TODO: add optional style parameter
    virtual void            setText(const LLStringExplicit &utf8str , const LLStyle::Params& input_params = LLStyle::Params()); // uses default style
    /*virtual*/ const std::string& getText() const override;
    void                    setMaxTextLength(S32 length) { mMaxTextByteLength = length; }
    S32                     getMaxTextLength() const { return mMaxTextByteLength; }

    // wide-char versions
    void                    setWText(const LLWString& text);
    const LLWString&        getWText() const;
    S32                     getTextGeneration() const;

    void                    appendText(const std::string &new_text, bool prepend_newline, const LLStyle::Params& input_params = LLStyle::Params());

    void                    setLabel(const LLStringExplicit& label);
    /*virtual*/ bool        setLabelArg(const std::string& key, const LLStringExplicit& text) override;

    const   std::string&    getLabel()  { return mLabel.getString(); }
    const   LLWString&      getWlabel() { return mLabel.getWString();}

    void                    setLastSegmentToolTip(const std::string &tooltip);

    /**
     * If label is set, draws text label (which is LLLabelTextSegment)
     * that is visible when no user text provided
     */
    void                    resetLabel();

    void                    setFont(const LLFontGL* font);

    // force reflow of text
    void                    needsReflow(S32 index = 0);

    S32                     getLength() const { return static_cast<S32>(getWText().length()); }
    S32                     getLineCount() const { return static_cast<S32>(mLineInfoList.size()); }
    S32                     removeFirstLine(); // returns removed length

    void                    addDocumentChild(LLView* view);
    void                    removeDocumentChild(LLView* view);
    const LLView*           getDocumentView() const { return mDocumentView; }
    LLRect                  getVisibleTextRect() const { return mVisibleTextRect; }
    LLRect                  getTextBoundingRect();
    LLRect                  getVisibleDocumentRect() const;

    S32                     getVPad() const { return mVPad; }
    S32                     getHPad() const { return mHPad; }
    F32                     getLineSpacingMult() const { return mLineSpacingMult; }
    S32                     getLineSpacingPixels() const { return mLineSpacingPixels; } // only for multiline

    S32                     getDocIndexFromLocalCoord( S32 local_x, S32 local_y, bool round, bool hit_past_end_of_line = true) const;
    LLRect                  getLocalRectFromDocIndex(S32 pos) const;
    LLRect                  getDocRectFromDocIndex(S32 pos) const;

    void                    setReadOnly(bool read_only) { mReadOnly = read_only; }
    bool                    getReadOnly() const { return mReadOnly; }

    void                    setSkipLinkUnderline(bool skip_link_underline) { mSkipLinkUnderline = skip_link_underline; }
    bool                    getSkipLinkUnderline() const { return mSkipLinkUnderline;  }

    void                    setParseURLs(bool parse_urls) { mParseHTML = parse_urls; }

    void                    setPlainText(bool value) { mPlainText = value;}
    bool                    getPlainText() const { return mPlainText; }

    // cursor manipulation
    bool                    setCursor(S32 row, S32 column);
    bool                    setCursorPos(S32 cursor_pos, bool keep_cursor_offset = false);
    void                    startOfLine();
    void                    endOfLine();
    void                    startOfDoc();
    void                    endOfDoc();
    void                    changePage(S32 delta);
    void                    changeLine(S32 delta);

    bool                    scrolledToStart();
    bool                    scrolledToEnd();

    const LLFontGL*         getFont() const override { return mFont; }

    virtual void            copyContents(const LLTextBase* source);
    virtual void            appendLineBreakSegment(const LLStyle::Params& style_params);
    virtual void            appendImageSegment(const LLStyle::Params& style_params);
    virtual void            appendWidget(const LLInlineViewSegment::Params& params, const std::string& text, bool allow_undo);
    boost::signals2::connection setURLClickedCallback(const commit_signal_t::slot_type& cb);
    boost::signals2::connection setIsFriendCallback(const is_friend_signal_t::slot_type& cb);
    boost::signals2::connection setIsObjectBlockedCallback(const is_blocked_signal_t::slot_type& cb);

    void                    setWordWrap(bool wrap);
    LLScrollContainer*      getScrollContainer() const { return mScroller; }

protected:
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

    // helper structs
    struct compare_bottom
    {
        bool operator()(const S32& a, const line_info& b) const;
        bool operator()(const line_info& a, const S32& b) const;
        bool operator()(const line_info& a, const line_info& b) const;
    };
    struct compare_top
    {
        bool operator()(const S32& a, const line_info& b) const;
        bool operator()(const line_info& a, const S32& b) const;
        bool operator()(const line_info& a, const line_info& b) const;
    };
    struct line_end_compare;
    typedef std::vector<LLTextSegmentPtr> segment_vec_t;

    // Abstract inner base class representing an undoable editor command.
    // Concrete sub-classes can be defined for operations such as insert, remove, etc.
    // Used as arguments to the execute() method below.
    class TextCmd
    {
    public:
        TextCmd( S32 pos, bool group_with_next, LLTextSegmentPtr segment = LLTextSegmentPtr() )
        :   mPos(pos),
            mGroupWithNext(group_with_next)
        {
            if (segment.notNull())
            {
                mSegments.push_back(segment);
            }
        }
        virtual         ~TextCmd() {}
        virtual bool    execute(LLTextBase* editor, S32* delta) = 0;
        virtual S32     undo(LLTextBase* editor) = 0;
        virtual S32     redo(LLTextBase* editor) = 0;
        virtual bool    canExtend(S32 pos) const { return false; }
        virtual void    blockExtensions() {}
        virtual bool    extendAndExecute( LLTextBase* editor, S32 pos, llwchar c, S32* delta ) { llassert(0); return 0; }
        virtual bool    hasExtCharValue( llwchar value ) const { return false; }

        // Defined here so they can access protected LLTextEditor editing methods
        S32             insert(LLTextBase* editor, S32 pos, const LLWString &wstr) { return editor->insertStringNoUndo( pos, wstr, &mSegments ); }
        S32             remove(LLTextBase* editor, S32 pos, S32 length) { return editor->removeStringNoUndo( pos, length ); }
        S32             overwrite(LLTextBase* editor, S32 pos, llwchar wc) { return editor->overwriteCharNoUndo(pos, wc); }

        S32             getPosition() const { return mPos; }
        bool            groupWithNext() const { return mGroupWithNext; }

    protected:
        const S32           mPos;
        bool                mGroupWithNext;
        segment_vec_t       mSegments;
    };

    struct compare_segment_end
    {
        bool operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const;
    };
    typedef std::multiset<LLTextSegmentPtr, compare_segment_end> segment_set_t;

    // member functions
    LLTextBase(const Params &p);
    virtual ~LLTextBase();
    void                            initFromParams(const Params& p);
    virtual void                    beforeValueChange();
    virtual void                    onValueChange(S32 start, S32 end);
    virtual bool                    useLabel() const;

    // draw methods
    virtual void                    drawSelectionBackground(); // draws the black box behind the selected text
    void                            drawCursor();
    void                            drawText();

    // modify contents
    S32                             insertStringNoUndo(S32 pos, const LLWString &wstr,
                                        segment_vec_t* segments = NULL); // returns num of chars actually inserted
    S32                             removeStringNoUndo(S32 pos, S32 length);
    S32                             overwriteCharNoUndo(S32 pos, llwchar wc);
    void                            appendAndHighlightText(const std::string &new_text,
                                        LLTextParser::EHighlightPosition highlight_part,
                                        const LLStyle::Params& stylep, bool underline_on_hover_only = false);


    // manage segments
    void                            getSegmentAndOffset(S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp) const;
    void                            getSegmentAndOffset(S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp);
    LLTextSegmentPtr                getSegmentAtLocalPos(S32 x, S32 y, bool hit_past_end_of_line = true);
    segment_set_t::iterator         getEditableSegIterContaining(S32 index);
    segment_set_t::const_iterator   getEditableSegIterContaining(S32 index) const;
    segment_set_t::iterator         getSegIterContaining(S32 index);
    segment_set_t::const_iterator   getSegIterContaining(S32 index) const;
    void                            clearSegments();
    void                            createDefaultSegment();
    virtual void                    updateSegments();
    void                            insertSegment(LLTextSegmentPtr segment_to_insert);
    const LLStyle::Params&          getStyleParams();

    //  manage lines
    S32                             getLineStart( S32 line ) const;
    S32                             getLineEnd( S32 line ) const;
    S32                             getLineNumFromDocIndex( S32 doc_index, bool include_wordwrap = true) const;
    S32                             getLineOffsetFromDocIndex( S32 doc_index, bool include_wordwrap = true) const;
    S32                             getFirstVisibleLine() const;
    std::pair<S32, S32>             getVisibleLines(bool fully_visible = false);
    S32                             getLeftOffset(S32 width);
    void                            reflow();

    // cursor
    void                            updateCursorXPos();
    void                            setCursorAtLocalPos( S32 local_x, S32 local_y, bool round, bool keep_cursor_offset=false );
    S32                             getEditableIndex(S32 index, bool increasing_direction); // constraint cursor to editable segments of document
    void                            resetCursorBlink() { mCursorBlinkTimer.reset(); }
    void                            updateScrollFromCursor();

    // text selection
    bool                            hasSelection() const { return (mSelectionStart !=mSelectionEnd); }
    void                            startSelection();
    void                            endSelection();

    // misc
    void                            updateRects();
    void                            needsScroll() { mScrollNeeded = true; }

    struct URLLabelCallback;
    // Replace a URL with a new icon and label, for example, when
    // avatar names are looked up.
    void replaceUrl(const std::string &url, const std::string &label, const std::string& icon);

    void appendTextImpl(const std::string &new_text, const LLStyle::Params& input_params = LLStyle::Params());
    void appendAndHighlightTextImpl(const std::string &new_text, LLTextParser::EHighlightPosition highlight_part,
        const LLStyle::Params& style_params, bool underline_on_hover_only, std::string tooltip = LLStringUtil::null);

protected:
    // virtual
    std::string _getSearchText() const override
    {
        return mLabel.getString() + getToolTip();
    }

    std::vector<LLRect> getSelectionRects();

protected:
    // text segmentation and flow
    segment_set_t               mSegments;
    line_list_t                 mLineInfoList;
    LLRect                      mVisibleTextRect;           // The rect in which text is drawn.  Excludes borders.
    LLRect                      mTextBoundingRect;

    // default text style
    LLStyle::Params             mStyle;
    bool                        mStyleDirty;
    const LLFontGL*             mFont;
    const LLFontGL::ShadowType  mFontShadow;

    // colors
    LLUIColor                   mCursorColor;
    LLUIColor                   mFgColor;
    LLUIColor                   mReadOnlyFgColor;
    LLUIColor                   mTentativeFgColor;
    LLUIColor                   mWriteableBgColor;
    LLUIColor                   mReadOnlyBgColor;
    LLUIColor                   mFocusBgColor;
    LLUIColor                   mTextSelectedColor;
    LLUIColor                   mSelectedBGColor;

    // cursor
    S32                         mCursorPos;         // I-beam is just after the mCursorPos-th character.
    S32                         mDesiredXPixel;     // X pixel position where the user wants the cursor to be
    LLFrameTimer                mCursorBlinkTimer;  // timer that controls cursor blinking

    // selection
    S32                         mSelectionStart;
    S32                         mSelectionEnd;
    LLTimer                     mTripleClickTimer;

    bool                        mIsSelecting;       // Are we in the middle of a drag-select?

    // spell checking
    bool                        mSpellCheck;
    S32                         mSpellCheckStart;
    S32                         mSpellCheckEnd;
    LLTimer                     mSpellCheckTimer;
    std::list<std::pair<U32, U32> > mMisspellRanges;
    std::vector<std::string>        mSuggestionList;

    // configuration
    S32                         mHPad;              // padding on left of text
    S32                         mVPad;              // padding above text
    LLFontGL::HAlign            mHAlign;            // horizontal alignment of the document in its entirety
    LLFontGL::VAlign            mVAlign;            // vertical alignment of the document in its entirety
    LLFontGL::VAlign            mTextVAlign;        // vertical alignment of a text segment within a single line of text
    F32                         mLineSpacingMult;   // multiple of line height used as space for a single line of text (e.g. 1.5 to get 50% padding)
    S32                         mLineSpacingPixels; // padding between lines
    bool                        mBorderVisible;
    bool                        mParseHTML;         // make URLs interactive
    bool                        mForceUrlsExternal; // URLs from this textbox will be opened in external browser
    bool                        mParseHighlights;   // highlight user-defined keywords
    bool                        mWordWrap;
    bool                        mUseEllipses;
    bool                        mUseEmoji;
    bool                        mUseColor;
    bool                        mTrackEnd;          // if true, keeps scroll position at end of document during resize
    bool                        mReadOnly;
    bool                        mBGVisible;         // render background?
    bool                        mClip;              // clip text to widget rect
    bool                        mClipPartial;       // false if we show lines that are partially inside bounding rect
    bool                        mTrustedContent;    // if false, does not allow to execute SURL links from this editor
    bool                        mPlainText;         // didn't use Image or Icon segments
    bool                        mAutoIndent;
    S32                         mMaxTextByteLength; // Maximum length mText is allowed to be in bytes
    bool                        mSkipTripleClick;
    bool                        mAlwaysShowIcons;

    bool                        mSkipLinkUnderline;

    // support widgets
    LLHandle<LLContextMenu>     mPopupMenuHandle;
    LLView*                     mDocumentView;
    LLScrollContainer*          mScroller;

    // transient state
    S32                         mReflowIndex;       // index at which to start reflow.  S32_MAX indicates no reflow needed.
    bool                        mScrollNeeded;      // need to change scroll region because of change to cursor position
    S32                         mScrollIndex;       // index of first character to keep visible in scroll region

    // Fired when a URL link is clicked
    commit_signal_t*            mURLClickSignal;

    // Used to check if user with given ID is avatar's friend
    is_friend_signal_t*         mIsFriendSignal;
    is_blocked_signal_t*        mIsObjectBlockedSignal;

    LLUIString                  mLabel; // text label that is visible when no user text provided
};

#endif
