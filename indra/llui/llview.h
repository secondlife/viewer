/** 
 * @file llview.h
 * @brief Container for other views, anything that draws.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEW_H
#define LL_LLVIEW_H

// A view is an area in a window that can draw.  It might represent
// the HUD or a dialog box or a button.  It can also contain sub-views
// and child widgets

#include <iosfwd>
#include <list>

#include "lluixmltags.h"
#include "llrect.h"
#include "llmousehandler.h"
#include "stdenums.h"
#include "llsd.h"
#include "llstring.h"
#include "llnametable.h"
#include "llcoord.h"
#include "llmortician.h"
#include "llxmlnode.h"
#include "llfontgl.h"
#include "llviewquery.h"

#include "llui.h"

class LLColor4;
class LLWindow;
class LLUICtrl;
class LLScrollListItem;

const U32	FOLLOWS_NONE	= 0x00;
const U32	FOLLOWS_LEFT	= 0x01;
const U32	FOLLOWS_RIGHT	= 0x02;
const U32	FOLLOWS_TOP		= 0x10;
const U32	FOLLOWS_BOTTOM	= 0x20;
const U32	FOLLOWS_ALL		= 0x33;

const BOOL	MOUSE_OPAQUE = TRUE;
const BOOL	NOT_MOUSE_OPAQUE = FALSE;

const U32 GL_NAME_UI_RESERVED = 2;

class LLSimpleListener;
class LLEventDispatcher;

class LLViewHandle
{
public:
	LLViewHandle() { mID = 0; } 

	void init() { mID = ++sNextID; }
	void markDead() { mID = 0; }
	BOOL isDead() { return (mID == 0); }
	friend bool operator==(const LLViewHandle& lhs, const LLViewHandle& rhs);
	friend bool operator!=(const LLViewHandle& lhs, const LLViewHandle& rhs);
	friend bool	operator<(const LLViewHandle &a, const LLViewHandle &b);

public:
	static LLViewHandle sDeadHandle;

protected:
	S32		mID;

	static S32 sNextID;
};

class LLView : public LLMouseHandler, public LLMortician, public LLSimpleListenerObservable
{

public:
#if LL_DEBUG
	static BOOL sIsDrawing;
#endif
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

	typedef std::vector<LLUICtrl *>					ctrl_list_t;

	typedef std::pair<S32, S32>							tab_order_t;
	typedef std::pair<LLUICtrl *, tab_order_t>			tab_order_pair_t;
	// this structure primarily sorts by the tab group, secondarily by the insertion ordinal (lastly by the value of the pointer)
	typedef std::map<const LLUICtrl*, tab_order_t>		child_tab_order_t;
	typedef child_tab_order_t::iterator					child_tab_order_iter_t;
	typedef child_tab_order_t::const_iterator			child_tab_order_const_iter_t;
	typedef child_tab_order_t::reverse_iterator			child_tab_order_reverse_iter_t;
	typedef child_tab_order_t::const_reverse_iterator	child_tab_order_const_reverse_iter_t;

private:
	LLView*		mParentView;
	child_list_t mChildList;

protected:
	LLString	mName;
	// location in pixels, relative to surrounding structure, bottom,left=0,0
	LLRect		mRect;
	
	U32			mReshapeFlags;

	child_tab_order_t mCtrlOrder;
	S32			mDefaultTabGroup;

	BOOL		mEnabled;		// Enabled means "accepts input that has an effect on the state of the application."
								// A disabled view, for example, may still have a scrollbar that responds to mouse events.
	BOOL		mMouseOpaque;	// Opaque views handle all mouse events that are over their rect.
	LLString	mToolTipMsg;	// isNull() is true if none.

	U8          mSoundFlags;
	BOOL		mSaveToXML;

	BOOL		mIsFocusRoot;

public:
	LLViewHandle mViewHandle;
	BOOL		mLastVisible;
	BOOL		mSpanChildren;

private:
	BOOL		mVisible;
	BOOL		mHidden;		// Never show (generally for replacement text only)

	S32			mNextInsertionOrdinal;

protected:
	static LLWindow* sWindow;	// All root views must know about their window.

public:
	static BOOL	sDebugRects;	// Draw debug rects behind everything.
	static BOOL sDebugKeys;
	static S32	sDepth;
	static BOOL sDebugMouseHandling;
	static LLString sMouseHandlerMessage;
	static S32	sSelectID;
	static BOOL sEditingUI;
	static LLView* sEditingUIView;
	static S32 sLastLeftXML;
	static S32 sLastBottomXML;
	static std::map<LLViewHandle,LLView*> sViewHandleMap;
	static BOOL sForceReshape;

public:
	static LLView* getViewByHandle(LLViewHandle handle);
	static BOOL deleteViewByHandle(LLViewHandle handle);
	
public:
	LLView();
	LLView(const LLString& name, BOOL mouse_opaque);
	LLView(const LLString& name, const LLRect& rect, BOOL mouse_opaque, U32 follows=FOLLOWS_NONE);

	virtual ~LLView();

	// Hack to support LLFocusMgr
	virtual BOOL isView();

	// Some UI widgets need to be added as controls.  Others need to
	// be added as regular view children.  isCtrl should return TRUE
	// if a widget needs to be added as a ctrl
	virtual BOOL isCtrl() const;

	virtual BOOL isPanel();
	
	//
	// MANIPULATORS
	//
	void		setMouseOpaque( BOOL b );
	void		setToolTip( const LLString& msg );

	virtual void setRect(const LLRect &rect);
	void		setFollows(U32 flags);

	// deprecated, use setFollows() with FOLLOWS_LEFT | FOLLOWS_TOP, etc.
	void		setFollowsNone();
	void		setFollowsLeft();
	void		setFollowsTop();
	void		setFollowsRight();
	void		setFollowsBottom();
	void		setFollowsAll();

	void        setSoundFlags(U8 flags);
	void		setName(LLString name);
	void		setSpanChildren( BOOL span_children );

	const LLString& getToolTip();

	void		sendChildToFront(LLView* child);
	void		sendChildToBack(LLView* child);
	void		moveChildToFrontOfTabGroup(LLUICtrl* child);

	void		addChild(LLView* view, S32 tab_group = 0);
	void		addChildAtEnd(LLView* view,  S32 tab_group = 0);
	// remove the specified child from the view, and set it's parent to NULL.
	void		removeChild( LLView* view );

	virtual void	addCtrl( LLUICtrl* ctrl, S32 tab_group);
	virtual void	addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group);
	virtual void	removeCtrl( LLUICtrl* ctrl);

	child_tab_order_t getCtrlOrder() const { return mCtrlOrder; }
	ctrl_list_t getCtrlList() const;
	ctrl_list_t getCtrlListSorted() const;
	S32 getDefaultTabGroup() const;

	BOOL		isInVisibleChain() const;
	BOOL		isInEnabledChain() const;

	BOOL		isFocusRoot() const;
	LLView*		findRootMostFocusRoot();
	virtual BOOL canFocusChildren() const;

	class LLFocusRootsFilter : public LLQueryFilter, public LLSingleton<LLFocusRootsFilter>
	{
		/*virtual*/ filterResult_t operator() (const LLView* const view, const viewList_t & children) const 
		{
			return filterResult_t(view->isCtrl() && view->isFocusRoot(), !view->isFocusRoot());
		}
	};

	virtual  BOOL focusNextRoot();
	virtual  BOOL focusPrevRoot();

	virtual BOOL focusNextItem(BOOL text_entry_only);
	virtual BOOL focusPrevItem(BOOL text_entry_only);
	virtual BOOL focusFirstItem(BOOL prefer_text_fields = FALSE );
	virtual BOOL focusLastItem(BOOL prefer_text_fields = FALSE);

	// delete all children. Override this function if you need to
	// perform any extra clean up such as cached pointers to selected
	// children, etc.
	virtual void deleteAllChildren();

	// by default, does nothing
	virtual void	setTentative(BOOL b);
	// by default, returns false
	virtual BOOL	getTentative() const;
	virtual void 	setAllChildrenEnabled(BOOL b);

	virtual void	setEnabled(BOOL enabled);
	virtual void	setVisible(BOOL visible);
	virtual void	setHidden(BOOL hidden);		// Never show (replacement text)

	// by default, does nothing and returns false
	virtual BOOL	setLabelArg( const LLString& key, const LLString& text );

	virtual void	onVisibilityChange ( BOOL curVisibilityIn );

	void			pushVisible(BOOL visible)	{ mLastVisible = mVisible; setVisible(visible); }
	void			popVisible()				{ setVisible(mLastVisible); mLastVisible = TRUE; }

	//
	// ACCESSORS
	//
	BOOL		getMouseOpaque() const			{ return mMouseOpaque; }

	U32			getFollows() const				{ return mReshapeFlags; }
	BOOL		followsLeft() const				{ return mReshapeFlags & FOLLOWS_LEFT; }
	BOOL		followsRight() const			{ return mReshapeFlags & FOLLOWS_RIGHT; }
	BOOL		followsTop() const				{ return mReshapeFlags & FOLLOWS_TOP; }
	BOOL		followsBottom() const			{ return mReshapeFlags & FOLLOWS_BOTTOM; }
	BOOL		followsAll() const				{ return mReshapeFlags & FOLLOWS_ALL; }

	const LLRect&	getRect() const				{ return mRect; }
	const LLRect	getScreenRect() const;
	const LLRect	getLocalRect() const;
	virtual const LLRect getSnapRect() const	{ return mRect; }
	virtual const LLRect getLocalSnapRect() const;

	virtual LLRect getRequiredRect();		// Get required size for this object. 0 for width/height means don't care.
	virtual void updateRect();				// apply procedural updates to own rectangle

	LLView*		getRootView();
	LLView*		getParent() const				{ return mParentView; }
	LLView*		getFirstChild() 				{ return (mChildList.empty()) ? NULL : *(mChildList.begin()); }
	S32			getChildCount()	const			{ return (S32)mChildList.size(); }
	template<class _Pr3> void sortChildren(_Pr3 _Pred) { mChildList.sort(_Pred); }
	BOOL		hasAncestor(LLView* parentp);

	BOOL		hasChild(const LLString& childname, BOOL recurse = FALSE) const;

	BOOL 		childHasKeyboardFocus( const LLString& childname ) const;

	//
	// UTILITIES
	//

	// Default behavior is to use reshape flags to resize child views
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	translate( S32 x, S32 y );
	virtual void	setOrigin( S32 x, S32 y )	{ mRect.translate( x - mRect.mLeft, y - mRect.mBottom ); }
	BOOL			translateIntoRect( const LLRect& constraint, BOOL allow_partial_outside );

	virtual void	userSetShape(const LLRect& new_rect);
	virtual LLView*	findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);

	// Defaults to other_view->getVisible()
	virtual BOOL	canSnapTo(LLView* other_view);

	virtual void	snappedTo(LLView* snap_view);

	virtual BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  LLString& tooltip_msg);

	// LLMouseHandler functions
	// Default behavior is to pass events to children

	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ void	onMouseCaptureLost();
	/*virtual*/ BOOL	hasMouseCapture();

	// Default behavior is to pass the tooltip event to children,
	// then display mToolTipMsg if no child handled it.
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);

	LLString getShowNamesToolTip();

	virtual void	draw();

	void			drawDebugRect();
	void			drawChild(LLView* childp, S32 x_offset = 0, S32 y_offset = 0);

	virtual const LLString&	getName() const;

	virtual EWidgetType getWidgetType() const = 0;
	virtual LLString getWidgetTag() const = 0;
	virtual LLXMLNodePtr getXML(bool save_children = true) const;

	static U32 createRect(LLXMLNodePtr node, LLRect &rect, LLView* parent_view, const LLRect &required_rect = LLRect());
	virtual void initFromXML(LLXMLNodePtr node, LLView* parent);

	static LLFontGL* selectFont(LLXMLNodePtr node);
	static LLFontGL::HAlign selectFontHAlign(LLXMLNodePtr node);
	static LLFontGL::VAlign selectFontVAlign(LLXMLNodePtr node);
	static LLFontGL::StyleFlags selectFontStyle(LLXMLNodePtr node);

	// Some widgets, like close box buttons, don't need to be saved
	BOOL getSaveToXML() const { return mSaveToXML; }
	void setSaveToXML(BOOL b) { mSaveToXML = b; }

	// Only saves color if different from default setting.
	static void addColorXML(LLXMLNodePtr node, const LLColor4& color,
							const LLString& xml_name, const LLString& control_name);
	static void saveColorToXML(std::ostream& out, const LLColor4& color,
							   const LLString& xml_name, const LLString& control_name,
							   const LLString& indent); // DEPRECATED
	// Escapes " (quot) ' (apos) & (amp) < (lt) > (gt)
	//static LLString escapeXML(const LLString& xml);
	static LLWString escapeXML(const LLWString& xml);
	
	//same as above, but wraps multiple lines in quotes and prepends
	//indent as leading white space on each line
	static LLString escapeXML(const LLString& xml, LLString& indent);

	// focuses the item in the list after the currently-focused item, wrapping if necessary
	static	BOOL focusNext(LLView::child_list_t & result);
	// focuses the item in the list before the currently-focused item, wrapping if necessary
	static	BOOL focusPrev(LLView::child_list_t & result);

	// returns query for iterating over controls in tab order	
	static const LLCtrlQuery & getTabOrderQuery();
	// return query for iterating over focus roots in tab order
	static const LLCtrlQuery & getFocusRootsQuery();

	BOOL			getEnabled() const			{ return mEnabled; }
	BOOL			getVisible() const			{ return mVisible && !mHidden; }
	U8              getSoundFlags() const       { return mSoundFlags; }

	// Default to no action
	virtual void	onFocusLost();
	virtual void	onFocusReceived();

	BOOL			parentPointInView(S32 x, S32 y) const { return mRect.pointInRect( x, y ); }
	BOOL			pointInView(S32 x, S32 y) const { return mRect.localPointInRect( x, y ); }
	virtual void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
	virtual void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;
	virtual BOOL	localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, LLView* other_view);
	virtual void	screenRectToLocal( const LLRect& screen, LLRect* local ) const;
	virtual void	localRectToScreen( const LLRect& local, LLRect* screen ) const;
	virtual BOOL	localRectToOtherView( const LLRect& local, LLRect* other, LLView* other_view ) const;


	static LLWindow*	getWindow(void);

	// Listener dispatching functions (Dispatcher deletes pointers to listeners on deregistration or destruction)
	LLSimpleListener* getListenerByName(const LLString &callback_name);
	void registerEventListener(LLString name, LLSimpleListener* function);
	void deregisterEventListener(LLString name);
	LLString findEventListener(LLSimpleListener *listener) const;
	void addListenerToControl(LLEventDispatcher *observer, const LLString& name, LLSD filter, LLSD userdata);

	virtual LLView* getChildByName(const LLString& name, BOOL recurse = FALSE) const;

	void addBoolControl(LLString name, bool initial_value);
	LLControlBase *getControl(LLString name);
	virtual LLControlBase *findControl(LLString name);

	void			setControlValue(const LLSD& value);
	virtual void	setControlName(const LLString& control, LLView *context);
	virtual LLString getControlName() const;
	virtual bool	handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);
	virtual void	setValue(const LLSD& value);
	const child_list_t*	getChildList() const { return &mChildList; }

protected:
	virtual BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent);

	LLView*	childrenHandleKey(KEY key, MASK mask);
	LLView* childrenHandleUnicodeChar(llwchar uni_char);
	LLView*	childrenHandleDragAndDrop(S32 x, S32 y, MASK mask,
											  BOOL drop,
											  EDragAndDropType type,
											  void* data,
											  EAcceptance* accept,
											  LLString& tooltip_msg);

	LLView*	childrenHandleHover(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseUp(S32 x, S32 y, MASK mask);
	LLView* childrenHandleMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleDoubleClick(S32 x, S32 y, MASK mask);
	LLView*	childrenHandleScrollWheel(S32 x, S32 y, S32 clicks);
	LLView* childrenHandleRightMouseDown(S32 x, S32 y, MASK mask);
	LLView* childrenHandleRightMouseUp(S32 x, S32 y, MASK mask);

	typedef std::map<LLString, LLSimpleListener*> dispatch_list_t;
	dispatch_list_t mDispatchList;

protected:
	typedef std::map<LLString, LLControlBase*> control_map_t;
	control_map_t mFloaterControls;

	LLString		mControlName;
	friend class LLUICtrlFactory;
};




class LLCompareByTabOrder
{
public:
	LLCompareByTabOrder(LLView::child_tab_order_t order);
	virtual ~LLCompareByTabOrder() {}
	bool operator() (const LLView* const a, const LLView* const b) const;
protected:
	virtual bool compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const;
	LLView::child_tab_order_t mTabOrder;
};

#endif
