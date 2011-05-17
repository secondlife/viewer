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
#include "stdenums.h"
#include "lluistring.h"
#include "llcursortypes.h"
#include "lluictrlfactory.h"
#include "lltreeiterators.h"
#include "llfocusmgr.h"

#include <list>

class LLSD;

const U32	FOLLOWS_NONE	= 0x00;
const U32	FOLLOWS_LEFT	= 0x01;
const U32	FOLLOWS_RIGHT	= 0x02;
const U32	FOLLOWS_TOP		= 0x10;
const U32	FOLLOWS_BOTTOM	= 0x20;
const U32	FOLLOWS_ALL		= 0x33;

const BOOL	MOUSE_OPAQUE = TRUE;
const BOOL	NOT_MOUSE_OPAQUE = FALSE;

const U32 GL_NAME_UI_RESERVED = 2;


// maintains render state during traversal of UI tree
class LLViewDrawContext
{
public:
	F32 mAlpha;

	LLViewDrawContext(F32 alpha = 1.f)
	:	mAlpha(alpha)
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

class LLViewWidgetRegistry : public LLChildRegistry<LLViewWidgetRegistry>
{};

class LLView : public LLMouseHandler, public LLMortician, public LLFocusableElement
{
public:
	struct Follows : public LLInitParam::Choice<Follows>
	{
		Alternative<std::string>	string;
		Alternative<U32>			flags;

        Follows();
	};

	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<std::string>		name;

		Optional<bool>				enabled,
									visible,
									mouse_opaque,
									use_bounding_rect,
									from_xui,
									focus_root;

		Optional<S32>				tab_group,
									default_tab_group;
		Optional<std::string>		tool_tip;

		Optional<S32>				sound_flags;
		Optional<Follows>			follows;
		Optional<std::string>		hover_cursor;

		Optional<std::string>		layout;
		Optional<LLRect>			rect;

		// Historical bottom-left layout used bottom_delta and left_delta
		// for relative positioning.  New layout "topleft" prefers specifying
		// based on top edge.
		Optional<S32>				bottom_delta,	// from last bottom to my bottom
									top_pad,		// from last bottom to my top
									top_delta,		// from last top to my top
									left_pad,		// from last right to my left
									left_delta;		// from last left to my left

		//FIXME: get parent context involved in parsing traversal
		Ignored						needs_translate,	// cue for translation tools
									xmlns,				// xml namespace
									xmlns_xsi,			// xml namespace
									xsi_schemaLocation,	// xml schema
									xsi_type;			// xml schema type

		Params();
	};

	typedef LLViewWidgetRegistry child_registry_t;

	void initFromParams(const LLView::Params&);

protected:
	LLView(const LLView::Params&);
	friend class LLUICtrlFactory;

private:
	// widgets in general are not copyable
	LLView(const LLView& other) {};
public:
//#if LL_DEBUG
	static BOOL sIsDrawing;
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
	typedef child_list_t::iterator					child_list_iter_t;
	typedef child_list_t::const_iterator  			child_list_const_iter_t;
	typedef child_list_t::reverse_iterator 			child_list_reverse_iter_t;
	typedef child_list_t::const_reverse_iterator 	child_list_const_reverse_iter_t;

	typedef std::vector<class LLUICtrl *>				ctrl_list_t;

	typedef std::pair<S32, S32>							tab_order_t;
	typedef std::pair<LLUICtrl *, tab_order_t>			tab_order_pair_t;
	// this structure primarily sorts by the tab group, secondarily by the insertion ordinal (lastly by the value of the pointer)
	typedef std::map<const LLUICtrl*, tab_order_t>		child_tab_order_t;
	typedef child_tab_order_t::iterator					child_tab_order_iter_t;
	typedef child_tab_order_t::const_iterator			child_tab_order_const_iter_t;
	typedef child_tab_order_t::reverse_iterator			child_tab_order_reverse_iter_t;
	typedef child_tab_order_t::const_reverse_iterator	child_tab_order_const_reverse_iter_t;

	virtual ~LLView();

	// Some UI widgets need to be added as controls.  Others need to
	// be added as regular view children.  isCtrl should return TRUE
	// if a widget needs to be added as a ctrl
	virtual BOOL isCtrl() const;

	virtual BOOL isPanel() const;
	
	//
	// MANIPULATORS
	//
	void		setMouseOpaque( BOOL b )		{ mMouseOpaque = b; }
	BOOL		getMouseOpaque() const			{ return mMouseOpaque; }
	void		setToolTip( const LLStringExplicit& msg );
	BOOL		setToolTipArg( const LLStringExplicit& key, const LLStringExplicit& text );
	void		setToolTipArgs( const LLStringUtil::format_map_t& args );

	virtual void setRect(const LLRect &rect);
	void		setFollows(U32 flags)			{ mReshapeFlags = flags; }

	// deprecated, use setFollows() with FOLLOWS_LEFT | FOLLOWS_TOP, etc.
	void		setFollowsNone()				{ mReshapeFlags = FOLLOWS_NONE; }
	void		setFollowsLeft()				{ mReshapeFlags |= FOLLOWS_LEFT; }
	void		setFollowsTop()					{ mReshapeFlags |= FOLLOWS_TOP; }
	void		setFollowsRight()				{ mReshapeFlags |= FOLLOWS_RIGHT; }
	void		setFollowsBottom()				{ mReshapeFlags |= FOLLOWS_BOTTOM; }
	void		setFollowsAll()					{ mReshapeFlags |= FOLLOWS_ALL; }

	void        setSoundFlags(U8 flags)			{ mSoundFlags = flags; }
	void		setName(std::string name)			{ mName = name; }
	void		setUseBoundingRect( BOOL use_bounding_rect );
	BOOL		getUseBoundingRect() const;

	ECursorType	getHoverCursor() { return mHoverCursor; }

	const std::string& getToolTip() const			{ return mToolTipMsg.getString(); }

	void		sendChildToFront(LLView* child);
	void		sendChildToBack(LLView* child);
	void		moveChildToFrontOfTabGroup(LLUICtrl* child);
	void		moveChildToBackOfTabGroup(LLUICtrl* child);
	
	virtual bool addChild(LLView* view, S32 tab_group = 0);

	// implemented in terms of addChild()
	bool		addChildInBack(LLView* view,  S32 tab_group = 0);

	// remove the specified child from the view, and set it's parent to NULL.
	virtual void	removeChild(LLView* view);

	virtual BOOL	postBuild() { return TRUE; }

	const child_tab_order_t& getCtrlOrder() const		{ return mCtrlOrder; }
	ctrl_list_t getCtrlList() const;
	ctrl_list_t getCtrlListSorted() const;
	
	void setDefaultTabGroup(S32 d)				{ mDefaultTabGroup = d; }
	S32 getDefaultTabGroup() const				{ return mDefaultTabGroup; }
	S32 getLastTabGroup()						{ return mLastTabGroup; }

	BOOL		isInVisibleChain() const;
	BOOL		isInEnabledChain() const;

	void		setFocusRoot(BOOL b)			{ mIsFocusRoot = b; }
	BOOL		isFocusRoot() const				{ return mIsFocusRoot; }
	virtual BOOL canFocusChildren() const;

	BOOL focusNextRoot();
	BOOL focusPrevRoot();

	// Normally we want the app menus to get priority on accelerated keys
	// However, sometimes we want to give specific views a first chance
	// iat handling them. (eg. the script editor)
	virtual bool	hasAccelerators() const { return false; };

	// delete all children. Override this function if you need to
	// perform any extra clean up such as cached pointers to selected
	// children, etc.
	virtual void deleteAllChildren();

	void 	setAllChildrenEnabled(BOOL b);

	virtual void	setVisible(BOOL visible);
	const BOOL&		getVisible() const			{ return mVisible; }
	virtual void	setEnabled(BOOL enabled);
	BOOL			getEnabled() const			{ return mEnabled; }
	/// 'available' in this context means 'visible and enabled': in other
	/// words, can a user actually interact with this?
	virtual bool	isAvailable() const;
	/// The static isAvailable() tests an LLView* that could be NULL.
	static bool		isAvailable(const LLView* view);
	U8              getSoundFlags() const       { return mSoundFlags; }

	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );

	virtual void	handleVisibilityChange ( BOOL new_visibility );

	void			pushVisible(BOOL visible)	{ mLastVisible = mVisible; setVisible(visible); }
	void			popVisible()				{ setVisible(mLastVisible); }
	BOOL			getLastVisible()	const	{ return mLastVisible; }

	LLHandle<LLView>	getHandle()				{ mHandle.bind(this); return mHandle; }

	U32			getFollows() const				{ return mReshapeFlags; }
	BOOL		followsLeft() const				{ return mReshapeFlags & FOLLOWS_LEFT; }
	BOOL		followsRight() const			{ return mReshapeFlags & FOLLOWS_RIGHT; }
	BOOL		followsTop() const				{ return mReshapeFlags & FOLLOWS_TOP; }
	BOOL		followsBottom() const			{ return mReshapeFlags & FOLLOWS_BOTTOM; }
	BOOL		followsAll() const				{ return mReshapeFlags & FOLLOWS_ALL; }

	const LLRect&	getRect() const				{ return mRect; }
	const LLRect&	getBoundingRect() const		{ return mBoundingRect; }
	LLRect	getLocalBoundingRect() const;
	LLRect	calcScreenRect() const;
	LLRect	calcScreenBoundingRect() const;
	LLRect	getLocalRect() const;
	virtual LLRect getSnapRect() const;
	LLRect getLocalSnapRect() const;

	std::string getLayout() { return mLayout; }

	// Override and return required size for this object. 0 for width/height means don't care.
	virtual LLRect getRequiredRect();
	LLRect calcBoundingRect();
	void updateBoundingRect();

	LLView*		getRootView();
	LLView*		getParent() const				{ return mParentView; }
	LLView*		getFirstChild() const			{ return (mChildList.empty()) ? NULL : *(mChildList.begin()); }
	LLView*		findPrevSibling(LLView* child);
	LLView*		findNextSibling(LLView* child);
	S32			getChildCount()	const			{ return (S32)mChildList.size(); }
	template<class _Pr3> void sortChildren(_Pr3 _Pred) { mChildList.sort(_Pred); }
	BOOL		hasAncestor(const LLView* parentp) const;
	BOOL		hasChild(const std::string& childname, BOOL recurse = FALSE) const;
	BOOL 		childHasKeyboardFocus( const std::string& childname ) const;
	
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
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	translate( S32 x, S32 y );
	void			setOrigin( S32 x, S32 y )	{ mRect.translate( x - mRect.mLeft, y - mRect.mBottom ); }
	BOOL			translateIntoRect( const LLRect& constraint, BOOL allow_partial_outside );
	BOOL			translateIntoRectWithExclusion( const LLRect& inside, const LLRect& exclude, BOOL allow_partial_outside );
	void			centerWithin(const LLRect& bounds);

	void	setShape(const LLRect& new_rect, bool by_user = false);
	virtual LLView*	findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual BOOL	canSnapTo(const LLView* other_view);
	virtual void	setSnappedTo(const LLView* snap_view);

	// inherited from LLFocusableElement
	/* virtual */ BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/* virtual */ BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);

	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  std::string& tooltip_msg);

	virtual void	draw();

	void parseFollowsFlags(const LLView::Params& params);

	// Some widgets, like close box buttons, don't need to be saved
	BOOL getFromXUI() const { return mFromXUI; }
	void setFromXUI(BOOL b) { mFromXUI = b; }

	typedef enum e_hit_test_type
	{
		HIT_TEST_USE_BOUNDING_RECT,
		HIT_TEST_IGNORE_BOUNDING_RECT
	}EHitTestType;

	BOOL parentPointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL pointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL blockMouseEvent(S32 x, S32 y) const;

	// See LLMouseHandler virtuals for screenPointToLocal and localPointToScreen
	BOOL localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, const LLView* other_view) const;
	BOOL localRectToOtherView( const LLRect& local, LLRect* other, const LLView* other_view ) const;
	void screenRectToLocal( const LLRect& screen, LLRect* local ) const;
	void localRectToScreen( const LLRect& local, LLRect* screen ) const;
	
	LLControlVariable *findControl(const std::string& name);

	const child_list_t*	getChildList() const { return &mChildList; }
	child_list_const_iter_t	beginChild() const { return mChildList.begin(); }
	child_list_const_iter_t	endChild() const { return mChildList.end(); }

	// LLMouseHandler functions
	//  Default behavior is to pass events to children
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMiddleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);	
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, MASK mask);

	/*virtual*/ std::string	getName() const;
	/*virtual*/ void	onMouseCaptureLost();
	/*virtual*/ BOOL	hasMouseCapture();
	/*virtual*/ void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
	/*virtual*/ void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;

	virtual		LLView*	childFromPoint(S32 x, S32 y);

	// view-specific handlers 
	virtual void	onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void	onMouseLeave(S32 x, S32 y, MASK mask);


	template <class T> T* findChild(const std::string& name, BOOL recurse = TRUE) const
	{
		LLView* child = findChildView(name, recurse);
		T* result = dynamic_cast<T*>(child);
		return result;
	}

	template <class T> T* getChild(const std::string& name, BOOL recurse = TRUE) const;

	template <class T> T& getChildRef(const std::string& name, BOOL recurse = TRUE) const
	{
		return *getChild<T>(name, recurse);
	}

	virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE) const;
	virtual LLView* findChildView(const std::string& name, BOOL recurse = TRUE) const;

	template <class T> T* getDefaultWidget(const std::string& name) const
	{
		LLView* widgetp = getDefaultWidgetContainer().findChildView(name);
		return dynamic_cast<T*>(widgetp);
	}

	//////////////////////////////////////////////
	// statics
	//////////////////////////////////////////////
	//static LLFontGL::HAlign selectFontHAlign(LLXMLNodePtr node);
	
	// focuses the item in the list after the currently-focused item, wrapping if necessary
	static	BOOL focusNext(LLView::child_list_t & result);
	// focuses the item in the list before the currently-focused item, wrapping if necessary
	static	BOOL focusPrev(LLView::child_list_t & result);

	// returns query for iterating over controls in tab order	
	static const LLCtrlQuery & getTabOrderQuery();
	// return query for iterating over focus roots in tab order
	static const LLCtrlQuery & getFocusRootsQuery();

	static void deleteViewByHandle(LLHandle<LLView> handle);
	static LLWindow*	getWindow(void) { return LLUI::sWindow; }

	// Set up params after XML load before calling new(),
	// usually to adjust layout.
	static void applyXUILayout(Params& p, LLView* parent);

	// For re-export of floaters and panels, convert the coordinate system
	// to be top-left based.
	static void setupParamsForExport(Params& p, LLView* parent);
	
	//virtual BOOL	addChildFromParam(const LLInitParam::BaseBlock& params) { return TRUE; }
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

	virtual void	handleReshape(const LLRect& rect, bool by_user);
	virtual void	dirtyRect();

	//send custom notification to LLView parent
	virtual S32	notifyParent(const LLSD& info);

	//send custom notification to all view childrend
	// return true if _any_ children return true. otherwise false.
	virtual bool	notifyChildren(const LLSD& info);

	//send custom notification to current view
	virtual S32	notify(const LLSD& info) { return 0;};

	static const LLViewDrawContext& getDrawContext();

protected:
	void			drawDebugRect();
	void			drawChild(LLView* childp, S32 x_offset = 0, S32 y_offset = 0, BOOL force_draw = FALSE);
	void			drawChildren();

	LLView*	childrenHandleKey(KEY key, MASK mask);
	LLView* childrenHandleUnicodeChar(llwchar uni_char);
	LLView*	childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
											  BOOL drop,
											  EDragAndDropType type,
											  void* data,
											  EAcceptance* accept,
											  std::string& tooltip_msg);

	LLView*	childrenHandleHover(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMiddleMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMiddleMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleDoubleClick(S32 x, S32 y, MASK mask);
	LLView*	childrenHandleScrollWheel(S32 x, S32 y, S32 clicks);
	LLView* childrenHandleRightMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleRightMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleToolTip(S32 x, S32 y, MASK mask);

	ECursorType mHoverCursor;
	
private:

	LLView*		mParentView;
	child_list_t mChildList;

	// location in pixels, relative to surrounding structure, bottom,left=0,0
	BOOL		mVisible;
	LLRect		mRect;
	LLRect		mBoundingRect;
	
	std::string mLayout;
	std::string	mName;
	
	U32			mReshapeFlags;

	child_tab_order_t mCtrlOrder;
	S32			mDefaultTabGroup;
	S32			mLastTabGroup;

	BOOL		mEnabled;		// Enabled means "accepts input that has an effect on the state of the application."
								// A disabled view, for example, may still have a scrollbar that responds to mouse events.
	BOOL		mMouseOpaque;	// Opaque views handle all mouse events that are over their rect.
	LLUIString	mToolTipMsg;	// isNull() is true if none.

	U8          mSoundFlags;
	BOOL		mFromXUI;

	BOOL		mIsFocusRoot;
	BOOL		mUseBoundingRect; // hit test against bounding rectangle that includes all child elements

	LLRootHandle<LLView> mHandle;
	BOOL		mLastVisible;

	S32			mNextInsertionOrdinal;

	static LLWindow* sWindow;	// All root views must know about their window.

	typedef std::map<std::string, LLView*> default_widget_map_t;
	// allocate this map no demand, as it is rarely needed
	mutable LLView* mDefaultWidgets;

	LLView& getDefaultWidgetContainer() const;

public:
	// Depth in view hierarchy during rendering
	static S32	sDepth;

	// Draw debug rectangles around widgets to help with alignment and spacing
	static bool	sDebugRects;

	// Draw widget names and sizes when drawing debug rectangles, turning this
	// off is useful to make the rectangles themselves easier to see.
	static bool sDebugRectsShowNames;

	static bool sDebugKeys;
	static bool sDebugMouseHandling;
	static std::string sMouseHandlerMessage;
	static S32	sSelectID;
	static std::set<LLView*> sPreviewHighlightedElements;	// DEV-16869
	static BOOL sHighlightingDiffs;							// DEV-16869
	static LLView* sPreviewClickedElement;					// DEV-16869
	static BOOL sDrawPreviewHighlights;
	static S32 sLastLeftXML;
	static S32 sLastBottomXML;
	static BOOL sForceReshape;
};

class LLCompareByTabOrder
{
public:
	LLCompareByTabOrder(const LLView::child_tab_order_t& order) : mTabOrder(order) {}
	virtual ~LLCompareByTabOrder() {}
	bool operator() (const LLView* const a, const LLView* const b) const;
private:
	virtual bool compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const { return a < b; }
	// ok to store a reference, as this should only be allocated on stack during view query operations
	const LLView::child_tab_order_t& mTabOrder;
};

template <class T> T* LLView::getChild(const std::string& name, BOOL recurse) const
{
	LLView* child = findChildView(name, recurse);
	T* result = dynamic_cast<T*>(child);
	if (!result)
	{
		// did we find *something* with that name?
		if (child)
		{
			llwarns << "Found child named \"" << name << "\" but of wrong type " << typeid(*child).name() << ", expecting " << typeid(T*).name() << llendl;
		}
		result = getDefaultWidget<T>(name);
		if (!result)
		{
			result = LLUICtrlFactory::getDefaultWidget<T>(name);

			if (result)
			{
				// *NOTE: You cannot call mFoo = getChild<LLFoo>("bar")
				// in a floater or panel constructor.  The widgets will not
				// be ready.  Instead, put it in postBuild().
				llwarns << "Making dummy " << typeid(T).name() << " named \"" << name << "\" in " << getName() << llendl;
			}
			else
			{
				llwarns << "Failed to create dummy " << typeid(T).name() << llendl;
				return NULL;
			}

			getDefaultWidgetContainer().addChild(result);
		}
	}
	return result;
}

// Compiler optimization - don't generate these specializations inline,
// require explicit specialization.  See llbutton.cpp for an example.
#ifndef LLVIEW_CPP
extern template class LLView* LLView::getChild<class LLView>(
	const std::string& name, BOOL recurse) const;
#endif

#endif //LL_LLVIEW_H
