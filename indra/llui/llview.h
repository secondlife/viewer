/**
 * @file llview.h
 * @brief Container for other views, anything that draws.
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

#ifndef LL_LLVIEW_H
#define LL_LLVIEW_H

// A view is an area in a window that can draw.  It might represent
// the HUD or a dialog box or a button.  It can also contain sub-views
// and child widgets

#include "stdtypes.h"
#include "llcoord.h"
#include "llfontgl.h"
#include "llhandle.h"
#include "llmortician.h"
#include "llmousehandler.h"
#include "llstring.h"
#include "llrect.h"
#include "llui.h"
#include "lluistring.h"
#include "llviewquery.h"
#include "lluistring.h"
#include "llcursortypes.h"
#include "lluictrlfactory.h"
#include "lltreeiterators.h"
#include "llfocusmgr.h"

#include <list>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class LLSD;

constexpr U32   FOLLOWS_NONE    = 0x00;
constexpr U32   FOLLOWS_LEFT    = 0x01;
constexpr U32   FOLLOWS_RIGHT   = 0x02;
constexpr U32   FOLLOWS_TOP     = 0x10;
constexpr U32   FOLLOWS_BOTTOM  = 0x20;
constexpr U32   FOLLOWS_ALL     = 0x33;

constexpr bool  MOUSE_OPAQUE = true;
constexpr bool  NOT_MOUSE_OPAQUE = false;

constexpr U32 GL_NAME_UI_RESERVED = 2;


// maintains render state during traversal of UI tree
class LLViewDrawContext
{
public:
    F32 mAlpha;

    LLViewDrawContext(F32 alpha = 1.f)
    :   mAlpha(alpha)
    {
        if (!sDrawContextStack.empty())
        {
            LLViewDrawContext* context_top = sDrawContextStack.back();
            // merge with top of stack
            mAlpha *= context_top->mAlpha;
        }
        sDrawContextStack.push_back(this);
    }

    ~LLViewDrawContext()
    {
        sDrawContextStack.pop_back();
    }

    static const LLViewDrawContext& getCurrentContext();

private:
    static std::vector<LLViewDrawContext*> sDrawContextStack;
};

class LLView
:   public LLMouseHandler,          // handles mouse events
    public LLFocusableElement,      // handles keyboard events
    public LLMortician,             // lazy deletion
    public LLHandleProvider<LLView>     // passes out weak references to self
{
public:

    enum EOrientation { HORIZONTAL, VERTICAL, ORIENTATION_COUNT };

    struct Follows : public LLInitParam::ChoiceBlock<Follows>
    {
        Alternative<std::string>    string;
        Alternative<U32>            flags;

        Follows();
    };

    struct Params : public LLInitParam::Block<Params>
    {
        Mandatory<std::string>      name;

        Optional<bool>              enabled,
                                    visible,
                                    mouse_opaque,
                                    use_bounding_rect,
                                    from_xui,
                                    focus_root;

        Optional<S32>               tab_group,
                                    default_tab_group;
        Optional<std::string>       tool_tip;

        Optional<S32>               sound_flags;
        Optional<Follows>           follows;
        Optional<std::string>       hover_cursor;

        Optional<std::string>       layout;
        Optional<LLRect>            rect;

        // Historical bottom-left layout used bottom_delta and left_delta
        // for relative positioning.  New layout "topleft" prefers specifying
        // based on top edge.
        Optional<S32>               bottom_delta,   // from last bottom to my bottom
                                    top_pad,        // from last bottom to my top
                                    top_delta,      // from last top to my top
                                    left_pad,       // from last right to my left
                                    left_delta;     // from last left to my left

        //FIXME: get parent context involved in parsing traversal
        Ignored                     needs_translate,    // cue for translation tools
                                    xmlns,              // xml namespace
                                    xmlns_xsi,          // xml namespace
                                    xsi_schemaLocation, // xml schema
                                    xsi_type;           // xml schema type

        Params();
    };

    // most widgets are valid children of LLView
    typedef LLDefaultChildRegistry child_registry_t;

    void initFromParams(const LLView::Params&);

protected:
    LLView(const LLView::Params&);
    friend class LLUICtrlFactory;

private:
    // widgets in general are not copyable
    LLView(const LLView& other);
public:
//#if LL_DEBUG
    static bool sIsDrawing;
//#endif
    enum ESoundFlags
    {
        SILENT = 0,
        MOUSE_DOWN = 1,
        MOUSE_UP = 2
    };

    enum ESnapType
    {
        SNAP_PARENT,
        SNAP_SIBLINGS,
        SNAP_PARENT_AND_SIBLINGS
    };

    enum ESnapEdge
    {
        SNAP_LEFT,
        SNAP_TOP,
        SNAP_RIGHT,
        SNAP_BOTTOM
    };

    typedef std::list<LLView*> child_list_t;
    typedef child_list_t::iterator                  child_list_iter_t;
    typedef child_list_t::const_iterator            child_list_const_iter_t;
    typedef child_list_t::reverse_iterator          child_list_reverse_iter_t;
    typedef child_list_t::const_reverse_iterator    child_list_const_reverse_iter_t;

    typedef std::pair<LLView *, S32>                tab_order_pair_t;
    // this structure primarily sorts by the tab group, secondarily by the insertion ordinal (lastly by the value of the pointer)
    typedef std::map<const LLView*, S32>        child_tab_order_t;
    typedef child_tab_order_t::iterator                 child_tab_order_iter_t;
    typedef child_tab_order_t::const_iterator           child_tab_order_const_iter_t;
    typedef child_tab_order_t::reverse_iterator         child_tab_order_reverse_iter_t;
    typedef child_tab_order_t::const_reverse_iterator   child_tab_order_const_reverse_iter_t;

    virtual ~LLView();

    // Some UI widgets need to be added as controls.  Others need to
    // be added as regular view children.  isCtrl should return true
    // if a widget needs to be added as a ctrl
    virtual bool isCtrl() const;

    virtual bool isPanel() const;

    //
    // MANIPULATORS
    //
    void        setMouseOpaque( bool b )        { mMouseOpaque = b; }
    bool        getMouseOpaque() const          { return mMouseOpaque; }
    void        setToolTip( const LLStringExplicit& msg );
    bool        setToolTipArg( const LLStringExplicit& key, const LLStringExplicit& text );
    void        setToolTipArgs( const LLStringUtil::format_map_t& args );

    virtual void setRect(const LLRect &rect);
    void        setFollows(U32 flags)           { mReshapeFlags = flags; }

    // deprecated, use setFollows() with FOLLOWS_LEFT | FOLLOWS_TOP, etc.
    void        setFollowsNone()                { mReshapeFlags = FOLLOWS_NONE; }
    void        setFollowsLeft()                { mReshapeFlags |= FOLLOWS_LEFT; }
    void        setFollowsTop()                 { mReshapeFlags |= FOLLOWS_TOP; }
    void        setFollowsRight()               { mReshapeFlags |= FOLLOWS_RIGHT; }
    void        setFollowsBottom()              { mReshapeFlags |= FOLLOWS_BOTTOM; }
    void        setFollowsAll()                 { mReshapeFlags |= FOLLOWS_ALL; }

    void        setSoundFlags(U8 flags)         { mSoundFlags = flags; }
    void        setName(std::string name)           { mName = name; }
    void        setUseBoundingRect( bool use_bounding_rect );
    bool        getUseBoundingRect() const;

    ECursorType getHoverCursor() const { return mHoverCursor; }

    static F32 getTooltipTimeout();
    virtual const std::string getToolTip() const;
    virtual const std::string& getText() const { return LLStringUtil::null; }
    virtual const LLFontGL* getFont() const { return nullptr; }

    void        sendChildToFront(LLView* child);
    void        sendChildToBack(LLView* child);

    virtual bool addChild(LLView* view, S32 tab_group = 0);

    // implemented in terms of addChild()
    bool        addChildInBack(LLView* view,  S32 tab_group = 0);

    // remove the specified child from the view, and set it's parent to NULL.
    virtual void    removeChild(LLView* view);

    virtual bool    postBuild() { return true; }

    const child_tab_order_t& getTabOrder() const        { return mTabOrder; }

    void setDefaultTabGroup(S32 d)              { mDefaultTabGroup = d; }
    S32 getDefaultTabGroup() const              { return mDefaultTabGroup; }
    S32 getLastTabGroup() const                 { return mLastTabGroup; }

    bool        isInVisibleChain() const;
    bool        isInEnabledChain() const;

    void        setFocusRoot(bool b)            { mIsFocusRoot = b; }
    bool        isFocusRoot() const             { return mIsFocusRoot; }
    virtual bool canFocusChildren() const;

    bool focusNextRoot();
    bool focusPrevRoot();

    // Normally we want the app menus to get priority on accelerated keys
    // However, sometimes we want to give specific views a first chance
    // iat handling them. (eg. the script editor)
    virtual bool    hasAccelerators() const { return false; };

    // delete all children. Override this function if you need to
    // perform any extra clean up such as cached pointers to selected
    // children, etc.
    virtual void deleteAllChildren();

    void    setAllChildrenEnabled(bool b, bool recursive = false);

    virtual void    setVisible(bool visible);
    void            setVisibleDirect(bool visible) { mVisible = visible; }
    const bool&     getVisible() const          { return mVisible; }
    virtual void    setEnabled(bool enabled);
    bool            getEnabled() const          { return mEnabled; }
    /// 'available' in this context means 'visible and enabled': in other
    /// words, can a user actually interact with this?
    virtual bool    isAvailable() const;
    /// The static isAvailable() tests an LLView* that could be NULL.
    static bool     isAvailable(const LLView* view);
    U8              getSoundFlags() const       { return mSoundFlags; }

    virtual bool    setLabelArg( const std::string& key, const LLStringExplicit& text );

    virtual void    onVisibilityChange ( bool new_visibility );
    virtual void    onUpdateScrollToChild(const LLUICtrl * cntrl);

    void            pushVisible(bool visible)   { mLastVisible = mVisible; setVisible(visible); }
    void            popVisible()                { setVisible(mLastVisible); }
    bool            getLastVisible()    const   { return mLastVisible; }

    U32         getFollows() const              { return mReshapeFlags; }
    bool        followsLeft() const             { return mReshapeFlags & FOLLOWS_LEFT; }
    bool        followsRight() const            { return mReshapeFlags & FOLLOWS_RIGHT; }
    bool        followsTop() const              { return mReshapeFlags & FOLLOWS_TOP; }
    bool        followsBottom() const           { return mReshapeFlags & FOLLOWS_BOTTOM; }
    bool        followsAll() const              { return mReshapeFlags & FOLLOWS_ALL; }

    const LLRect&   getRect() const             { return mRect; }
    const LLRect&   getBoundingRect() const     { return mBoundingRect; }
    LLRect  getLocalBoundingRect() const;
    LLRect  calcScreenRect() const;
    LLRect  calcScreenBoundingRect() const;
    LLRect  getLocalRect() const;
    virtual LLRect getSnapRect() const;
    LLRect getLocalSnapRect() const;

    std::string getLayout() { return mLayout; }

    // Override and return required size for this object. 0 for width/height means don't care.
    virtual LLRect getRequiredRect();
    LLRect calcBoundingRect();
    void updateBoundingRect();

    LLView*     getRootView();
    LLView*     getParent() const               { return mParentView; }
    LLView*     getFirstChild() const           { return (mChildList.empty()) ? NULL : *(mChildList.begin()); }
    LLView*     findPrevSibling(LLView* child);
    LLView*     findNextSibling(LLView* child);
    S32         getChildCount() const           { return (S32)mChildList.size(); }
    template<class _Pr3> void sortChildren(_Pr3 _Pred) { mChildList.sort(_Pred); }
    bool        hasAncestor(const LLView* parentp) const;
    bool        hasChild(std::string_view childname, bool recurse = false) const;
    bool        childHasKeyboardFocus( std::string_view childname ) const;

    // these iterators are used for collapsing various tree traversals into for loops
    typedef LLTreeDFSIter<LLView, child_list_const_iter_t> tree_iterator_t;
    tree_iterator_t beginTreeDFS();
    tree_iterator_t endTreeDFS();

    typedef LLTreeDFSPostIter<LLView, child_list_const_iter_t> tree_post_iterator_t;
    tree_post_iterator_t beginTreeDFSPost();
    tree_post_iterator_t endTreeDFSPost();

    typedef LLTreeBFSIter<LLView, child_list_const_iter_t> bfs_tree_iterator_t;
    bfs_tree_iterator_t beginTreeBFS();
    bfs_tree_iterator_t endTreeBFS();


    typedef LLTreeDownIter<LLView> root_to_view_iterator_t;
    root_to_view_iterator_t beginRootToView();
    root_to_view_iterator_t endRootToView();

    //
    // UTILITIES
    //

    // Default behavior is to use reshape flags to resize child views
    virtual void    reshape(S32 width, S32 height, bool called_from_parent = true);
    virtual void    translate( S32 x, S32 y );
    void            setOrigin( S32 x, S32 y )   { mRect.translate( x - mRect.mLeft, y - mRect.mBottom ); }
    bool            translateIntoRect( const LLRect& constraint, S32 min_overlap_pixels = S32_MAX);
    bool            translateRectIntoRect( const LLRect& rect, const LLRect& constraint, S32 min_overlap_pixels = S32_MAX);
    bool            translateIntoRectWithExclusion( const LLRect& inside, const LLRect& exclude, S32 min_overlap_pixels = S32_MAX);
    void            centerWithin(const LLRect& bounds);

    void    setShape(const LLRect& new_rect, bool by_user = false);
    virtual LLView* findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
    virtual LLView* findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);
    virtual bool    canSnapTo(const LLView* other_view);
    virtual void    setSnappedTo(const LLView* snap_view);

    // inherited from LLFocusableElement
    /* virtual */ bool  handleKey(KEY key, MASK mask, bool called_from_parent);
    /* virtual */ bool  handleKeyUp(KEY key, MASK mask, bool called_from_parent);
    /* virtual */ bool  handleUnicodeChar(llwchar uni_char, bool called_from_parent);

    virtual bool    handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                      EDragAndDropType cargo_type,
                                      void* cargo_data,
                                      EAcceptance* accept,
                                      std::string& tooltip_msg);

    virtual void    draw();

    void parseFollowsFlags(const LLView::Params& params);

    // Some widgets, like close box buttons, don't need to be saved
    bool getFromXUI() const { return mFromXUI; }
    void setFromXUI(bool b) { mFromXUI = b; }

    typedef enum e_hit_test_type
    {
        HIT_TEST_USE_BOUNDING_RECT,
        HIT_TEST_IGNORE_BOUNDING_RECT
    }EHitTestType;

    bool parentPointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
    bool pointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
    bool blockMouseEvent(S32 x, S32 y) const;

    // See LLMouseHandler virtuals for screenPointToLocal and localPointToScreen
    bool localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, const LLView* other_view) const;
    bool localRectToOtherView( const LLRect& local, LLRect* other, const LLView* other_view ) const;
    void screenRectToLocal( const LLRect& screen, LLRect* local ) const;
    void localRectToScreen( const LLRect& local, LLRect* screen ) const;

    LLControlVariable *findControl(std::string_view name);

    const child_list_t* getChildList() const { return &mChildList; }
    child_list_const_iter_t beginChild() const { return mChildList.begin(); }
    child_list_const_iter_t endChild() const { return mChildList.end(); }

    // LLMouseHandler functions
    //  Default behavior is to pass events to children
    /*virtual*/ bool    handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleMiddleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleMiddleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool    handleScrollHWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ bool    handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleRightMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ bool    handleToolTip(S32 x, S32 y, MASK mask);

    /*virtual*/ const std::string& getName() const;
    /*virtual*/ void    onMouseCaptureLost();
    /*virtual*/ bool    hasMouseCapture();
    /*virtual*/ void    screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
    /*virtual*/ void    localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;

    virtual     LLView* childFromPoint(S32 x, S32 y, bool recur=false);

    // view-specific handlers
    virtual void    onMouseEnter(S32 x, S32 y, MASK mask);
    virtual void    onMouseLeave(S32 x, S32 y, MASK mask);

    std::string getPathname() const;
    // static method handles NULL pointer too
    static std::string getPathname(const LLView*);

    template <class T> T* findChild(std::string_view name, bool recurse = true) const
    {
        LLView* child = findChildView(name, recurse);
        T* result = dynamic_cast<T*>(child);
        return result;
    }

    template <class T> T* getChild(std::string_view name, bool recurse = true) const;

    template <class T> T& getChildRef(std::string_view name, bool recurse = true) const
    {
        return *getChild<T>(name, recurse);
    }

    virtual LLView* getChildView(std::string_view name, bool recurse = true) const;
    virtual LLView* findChildView(std::string_view name, bool recurse = true) const;

    template <class T> T* getDefaultWidget(std::string_view name) const
    {
        LLView* widgetp = getDefaultWidgetContainer().findChildView(name);
        return dynamic_cast<T*>(widgetp);
    }

    template <class T> T* getParentByType() const
    {
        LLView* parent = getParent();
        while(parent)
        {
            if (dynamic_cast<T*>(parent))
            {
                return static_cast<T*>(parent);
            }
            parent = parent->getParent();
        }
        return NULL;
    }

    //////////////////////////////////////////////
    // statics
    //////////////////////////////////////////////
    //static LLFontGL::HAlign selectFontHAlign(LLXMLNodePtr node);

    // focuses the item in the list after the currently-focused item, wrapping if necessary
    static  bool focusNext(LLView::child_list_t & result);
    // focuses the item in the list before the currently-focused item, wrapping if necessary
    static  bool focusPrev(LLView::child_list_t & result);

    // returns query for iterating over controls in tab order
    static const LLViewQuery & getTabOrderQuery();
    // return query for iterating over focus roots in tab order
    static const LLViewQuery & getFocusRootsQuery();

    static LLWindow*    getWindow(void) { return LLUI::getInstance()->mWindow; }

    // Set up params after XML load before calling new(),
    // usually to adjust layout.
    static void applyXUILayout(Params& p, LLView* parent, LLRect layout_rect = LLRect());

    // For re-export of floaters and panels, convert the coordinate system
    // to be top-left based.
    static void setupParamsForExport(Params& p, LLView* parent);

    //virtual bool  addChildFromParam(const LLInitParam::BaseBlock& params) { return true; }
    virtual bool    handleKeyHere(KEY key, MASK mask);
    virtual bool    handleKeyUpHere(KEY key, MASK mask);
    virtual bool    handleUnicodeCharHere(llwchar uni_char);

    virtual void    handleReshape(const LLRect& rect, bool by_user);
    virtual void    dirtyRect();

    //send custom notification to LLView parent
    virtual S32 notifyParent(const LLSD& info);

    //send custom notification to all view childrend
    // return true if _any_ children return true. otherwise false.
    virtual bool    notifyChildren(const LLSD& info);

    //send custom notification to current view
    virtual S32 notify(const LLSD& info) { return 0;};

    static const LLViewDrawContext& getDrawContext();

    // Returns useful information about this ui widget.
    LLSD getInfo(void);

protected:
    void            drawDebugRect();
    void            drawChild(LLView* childp, S32 x_offset = 0, S32 y_offset = 0, bool force_draw = false);
    void            drawChildren();
    bool            visibleAndContains(S32 local_x, S32 local_Y);
    bool            visibleEnabledAndContains(S32 local_x, S32 local_y);
    void            logMouseEvent();

    LLView* childrenHandleKey(KEY key, MASK mask);
    LLView* childrenHandleKeyUp(KEY key, MASK mask);
    LLView* childrenHandleUnicodeChar(llwchar uni_char);
    LLView* childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
                                              bool drop,
                                              EDragAndDropType type,
                                              void* data,
                                              EAcceptance* accept,
                                              std::string& tooltip_msg);

    LLView* childrenHandleHover(S32 x, S32 y, MASK mask);
    LLView* childrenHandleMouseUp(S32 x, S32 y, MASK mask);
    LLView* childrenHandleMouseDown(S32 x, S32 y, MASK mask);
    LLView* childrenHandleMiddleMouseUp(S32 x, S32 y, MASK mask);
    LLView* childrenHandleMiddleMouseDown(S32 x, S32 y, MASK mask);
    LLView* childrenHandleDoubleClick(S32 x, S32 y, MASK mask);
    LLView* childrenHandleScrollWheel(S32 x, S32 y, S32 clicks);
    LLView* childrenHandleScrollHWheel(S32 x, S32 y, S32 clicks);
    LLView* childrenHandleRightMouseDown(S32 x, S32 y, MASK mask);
    LLView* childrenHandleRightMouseUp(S32 x, S32 y, MASK mask);
    LLView* childrenHandleToolTip(S32 x, S32 y, MASK mask);

    ECursorType mHoverCursor;

    virtual void addInfo(LLSD & info);
private:

    template <typename METHOD, typename XDATA>
    LLView* childrenHandleMouseEvent(const METHOD& method, S32 x, S32 y, XDATA extra, bool allow_mouse_block = true);

    template <typename METHOD, typename CHARTYPE>
    LLView* childrenHandleCharEvent(std::string_view desc, const METHOD& method,
                                    CHARTYPE c, MASK mask);

    // adapter to blur distinction between handleKey() and handleUnicodeChar()
    // for childrenHandleCharEvent()
    bool    handleUnicodeCharWithDummyMask(llwchar uni_char, MASK /* dummy */, bool from_parent)
    {
        return handleUnicodeChar(uni_char, from_parent);
    }

    LLView*     mParentView;
    child_list_t mChildList;

    // location in pixels, relative to surrounding structure, bottom,left=0,0
    bool        mVisible;
    LLRect      mRect;
    LLRect      mBoundingRect;

    std::string mLayout;
    std::string mName;

    U32         mReshapeFlags;

    child_tab_order_t mTabOrder;
    S32         mDefaultTabGroup;
    S32         mLastTabGroup;

    bool        mEnabled;       // Enabled means "accepts input that has an effect on the state of the application."
                                // A disabled view, for example, may still have a scrollbar that responds to mouse events.
    bool        mMouseOpaque;   // Opaque views handle all mouse events that are over their rect.
    LLUIString  mToolTipMsg;    // isNull() is true if none.

    U8          mSoundFlags;
    bool        mFromXUI;

    bool        mIsFocusRoot;
    bool        mUseBoundingRect; // hit test against bounding rectangle that includes all child elements

    bool        mLastVisible;

    bool        mInDraw;

    static LLWindow* sWindow;   // All root views must know about their window.

    typedef std::map<std::string, LLView*> default_widget_map_t;
    // allocate this map no demand, as it is rarely needed
    mutable LLView* mDefaultWidgets;

    LLView& getDefaultWidgetContainer() const;

    // This allows special mouse-event targeting logic for testing.
    typedef boost::function<bool(const LLView*, S32 x, S32 y)> DrilldownFunc;
    static DrilldownFunc sDrilldown;

public:
    // This is the only public accessor to alter sDrilldown. This is not
    // an accident. The intended usage pattern is like:
    // {
    //     LLView::TemporaryDrilldownFunc scoped_func(myfunctor);
    //     // ... test with myfunctor ...
    // } // exiting block restores original LLView::sDrilldown
    class TemporaryDrilldownFunc: public boost::noncopyable
    {
    public:
        TemporaryDrilldownFunc(const DrilldownFunc& func):
            mOldDrilldown(sDrilldown)
        {
            sDrilldown = func;
        }

        ~TemporaryDrilldownFunc()
        {
            sDrilldown = mOldDrilldown;
        }

    private:
        DrilldownFunc mOldDrilldown;
    };

    // Depth in view hierarchy during rendering
    static S32  sDepth;

    // Draw debug rectangles around widgets to help with alignment and spacing
    static bool sDebugRects;

    // Show hexadecimal byte values of unicode symbols in a tooltip
    static bool sDebugUnicode;

    // Show camera position and direction in Camera Controls floater
    static bool sDebugCamera;

    static bool sIsRectDirty;
    static LLRect sDirtyRect;

    // Draw widget names and sizes when drawing debug rectangles, turning this
    // off is useful to make the rectangles themselves easier to see.
    static bool sDebugRectsShowNames;

    static bool sDebugKeys;
    static bool sDebugMouseHandling;
    static std::string sMouseHandlerMessage;
    static S32  sSelectID;
    static std::set<LLView*> sPreviewHighlightedElements;   // DEV-16869
    static bool sHighlightingDiffs;                         // DEV-16869
    static LLView* sPreviewClickedElement;                  // DEV-16869
    static bool sDrawPreviewHighlights;
    static S32 sLastLeftXML;
    static S32 sLastBottomXML;
    static bool sForceReshape;
};

namespace LLInitParam
{
template<>
struct TypeValues<LLView::EOrientation> : public LLInitParam::TypeValuesHelper<LLView::EOrientation>
{
    static void declareValues();
};
}

template <class T> T* LLView::getChild(std::string_view name, bool recurse) const
{
    LLView* child = findChildView(name, recurse);
    T* result = dynamic_cast<T*>(child);
    if (!result)
    {
        // did we find *something* with that name?
        if (child)
        {
            LL_WARNS() << "Found child named \"" << name << "\" but of wrong type " << typeid(*child).name() << ", expecting " << typeid(T*).name() << LL_ENDL;
        }
        result = getDefaultWidget<T>(name);
        if (!result)
        {
            result = LLUICtrlFactory::getDefaultWidget<T>(name);
            if (!result)
            {
                LL_ERRS() << "Failed to create dummy " << typeid(T).name() << LL_ENDL;
            }

            // *NOTE: You cannot call mFoo = getChild<LLFoo>("bar")
            // in a floater or panel constructor.  The widgets will not
            // be ready.  Instead, put it in postBuild().
            LL_WARNS() << "Making dummy " << typeid(T).name() << " named \"" << name << "\" in " << getName() << LL_ENDL;

            getDefaultWidgetContainer().addChild(result);
        }
    }
    return result;
}

// Compiler optimization - don't generate these specializations inline,
// require explicit specialization.  See llbutton.cpp for an example.
#ifndef LLVIEW_CPP
extern template class LLView* LLView::getChild<class LLView>(
    std::string_view name, bool recurse) const;
#endif

#endif //LL_LLVIEW_H
