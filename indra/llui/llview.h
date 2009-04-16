/** 
 * @file llview.h
 * @brief Container for other views, anything that draws.
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

#ifndef LL_LLVIEW_H
#define LL_LLVIEW_H

// A view is an area in a window that can draw.  It might represent
// the HUD or a dialog box or a button.  It can also contain sub-views
// and child widgets

#include "llcoord.h"
#include "llfontgl.h"
#include "llmortician.h"
#include "llmousehandler.h"
#include "llnametable.h"
#include "llsd.h"
#include "llstring.h"
#include "llrect.h"
#include "llui.h"
#include "lluistring.h"
#include "lluixmltags.h"
#include "llviewquery.h"
#include "llxmlnode.h"
#include "stdenums.h"
#include "lluistring.h"
#include "llcursortypes.h"

const U32	FOLLOWS_NONE	= 0x00;
const U32	FOLLOWS_LEFT	= 0x01;
const U32	FOLLOWS_RIGHT	= 0x02;
const U32	FOLLOWS_TOP		= 0x10;
const U32	FOLLOWS_BOTTOM	= 0x20;
const U32	FOLLOWS_ALL		= 0x33;

const BOOL	MOUSE_OPAQUE = TRUE;
const BOOL	NOT_MOUSE_OPAQUE = FALSE;

const U32 GL_NAME_UI_RESERVED = 2;

/*
// virtual functions defined in LLView:

virtual BOOL isCtrl() const;
		LLUICtrl
virtual BOOL isPanel();
		LLPanel
virtual void setRect(const LLRect &rect);
		LLLineEditor
virtual void	addCtrl( LLUICtrl* ctrl, S32 tab_group);
virtual void	addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group);
virtual void	removeCtrl( LLUICtrl* ctrl);
		LLPanel
virtual BOOL canFocusChildren() const		{ return TRUE; }
		LLFolderView
virtual void deleteAllChildren();
		LLFolderView, LLPanelInventory
virtual void	setTentative(BOOL b)		{}
		LLUICtrl, LLSliderCtrl, LLSpinCtrl
virtual BOOL	getTentative() const		{ return FALSE; }
		LLUICtrl, LLCheckBoxCtrl
virtual void	setVisible(BOOL visible);
		LLFloater, LLAlertDialog, LLMenuItemGL, LLModalDialog
virtual void	setEnabled(BOOL enabled)	{ mEnabled = enabled; }
		LLCheckBoxCtrl, LLComboBox, LLLineEditor, LLMenuGL, LLRadioGroup, etc
virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text ) { return FALSE; }
		LLUICtrl, LLButton, LLCheckBoxCtrl, LLLineEditor, LLMenuGL, LLSliderCtrl
virtual void	onVisibilityChange ( BOOL curVisibilityIn );
		LLMenuGL
virtual LLRect getSnapRect() const	{ return mRect; } *TODO: Make non virtual
		LLFloater
virtual LLRect getRequiredRect()			{ return mRect; }
		LLScrolllistCtrl
virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
		LLUICtrl, et. al.
virtual void	translate( S32 x, S32 y );
		LLMenuGL		
virtual void	userSetShape(const LLRect& new_rect);
		LLFloater, LLScrollLIstVtrl
virtual LLView*	findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
virtual LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);
		LLScrollListCtrl
virtual BOOL	canSnapTo(const LLView* other_view) { return other_view != this && other_view->getVisible(); }
		LLFloater
virtual void	snappedTo(const LLView* snap_view) {}
		LLFloater
virtual BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
		*
virtual BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
		*
virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,EDragAndDropType cargo_type,void* cargo_data,EAcceptance* accept,std::string& tooltip_msg);
		*
virtual void	draw();
		*

		*
virtual LLXMLNodePtr getXML(bool save_children = true) const;
		*
virtual void initFromXML(LLXMLNodePtr node, LLView* parent);
		*
virtual void onFocusLost() {}
		LLUICtrl, LLScrollListCtrl, LLMenuGL, LLLineEditor, LLComboBox
virtual void onFocusReceived() {}
		LLUICtrl, LLTextEditor, LLScrollListVtrl, LLMenuGL, LLLineEditor
virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;
		LLTabContainer, LLPanel, LLMenuGL
virtual void	setControlName(const std::string& control, LLView *context);
		LLSliderCtrl, LLCheckBoxCtrl
virtual std::string getControlName() const { return mControlName; }
		LLSliderCtrl, LLCheckBoxCtrl
virtual bool	handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);
		LLMenuItem
virtual void	setValue(const LLSD& value);
		*

protected:
virtual BOOL	handleKeyHere(KEY key, MASK mask);
		*
virtual BOOL	handleUnicodeCharHere(llwchar uni_char);
		*
*/

class LLUICtrlFactory;

// maps xml strings to widget classes
class LLWidgetClassRegistry : public LLSingleton<LLWidgetClassRegistry>
{
	friend class LLSingleton<LLWidgetClassRegistry>;
public:
	typedef LLView* (*factory_func_t)(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	typedef std::map<std::string, factory_func_t> factory_map_t;

	void registerCtrl(const std::string& xml_tag, factory_func_t function);
	BOOL isTagRegistered(const std::string& xml_tag);
	factory_func_t getCreatorFunc(const std::string& xml_tag);

	// get (first) xml tag for a given class
	template <class T> std::string getTag()
	{
		factory_map_t::iterator it;
		for(it = mCreatorFunctions.begin(); it != mCreatorFunctions.end(); ++it)
		{
			if (it->second == T::fromXML)
			{
				return it->first;
			}
		}

		return "";
	}

private:
	LLWidgetClassRegistry();
	virtual ~LLWidgetClassRegistry() {};

	typedef std::set<std::string> ctrl_name_set_t;
	ctrl_name_set_t mUICtrlNames;

	// map of xml tags to widget creator functions
	factory_map_t mCreatorFunctions;
};

template<class T>
class LLRegisterWidget
{
public:
	LLRegisterWidget(const std::string& tag) 
	{
		LLWidgetClassRegistry* registry = LLWidgetClassRegistry::getInstance();
		if (registry->isTagRegistered(tag))
		{
			//error!
			llerrs << "Widget named " << tag << " already registered!" << llendl;
		}
		else
		{
			registry->registerCtrl(tag, T::fromXML);
		}
	}
};

class LLView : public LLMouseHandler, public LLMortician
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

	typedef std::vector<class LLUICtrl *>				ctrl_list_t;

	typedef std::pair<S32, S32>							tab_order_t;
	typedef std::pair<LLUICtrl *, tab_order_t>			tab_order_pair_t;
	// this structure primarily sorts by the tab group, secondarily by the insertion ordinal (lastly by the value of the pointer)
	typedef std::map<const LLUICtrl*, tab_order_t>		child_tab_order_t;
	typedef child_tab_order_t::iterator					child_tab_order_iter_t;
	typedef child_tab_order_t::const_iterator			child_tab_order_const_iter_t;
	typedef child_tab_order_t::reverse_iterator			child_tab_order_reverse_iter_t;
	typedef child_tab_order_t::const_reverse_iterator	child_tab_order_const_reverse_iter_t;

	LLView();
	LLView(const std::string& name, BOOL mouse_opaque);
	LLView(const std::string& name, const LLRect& rect, BOOL mouse_opaque, U32 follows=FOLLOWS_NONE);

	virtual ~LLView();

	// Hack to support LLFocusMgr (from LLMouseHandler)
	/*virtual*/ BOOL isView() const;

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
	BOOL		getUseBoundingRect();

	const std::string& getToolTip() const			{ return mToolTipMsg.getString(); }

	void		sendChildToFront(LLView* child);
	void		sendChildToBack(LLView* child);
	void		moveChildToFrontOfTabGroup(LLUICtrl* child);
	void		moveChildToBackOfTabGroup(LLUICtrl* child);

	void		addChild(LLView* view, S32 tab_group = 0);
	void		addChildAtEnd(LLView* view,  S32 tab_group = 0);
	// remove the specified child from the view, and set it's parent to NULL.
	void		removeChild(LLView* view, BOOL deleteIt = FALSE);

	virtual void	addCtrl( LLUICtrl* ctrl, S32 tab_group);
	virtual void	addCtrlAtEnd( LLUICtrl* ctrl, S32 tab_group);
	virtual void	removeCtrl( LLUICtrl* ctrl);

	child_tab_order_t getCtrlOrder() const		{ return mCtrlOrder; }
	ctrl_list_t getCtrlList() const;
	ctrl_list_t getCtrlListSorted() const;
	
	void setDefaultTabGroup(S32 d)				{ mDefaultTabGroup = d; }
	S32 getDefaultTabGroup() const				{ return mDefaultTabGroup; }

	BOOL		isInVisibleChain() const;
	BOOL		isInEnabledChain() const;

	void		setFocusRoot(BOOL b)			{ mIsFocusRoot = b; }
	BOOL		isFocusRoot() const				{ return mIsFocusRoot; }
	virtual BOOL canFocusChildren() const;

	BOOL focusNextRoot();
	BOOL focusPrevRoot();

	// delete all children. Override this function if you need to
	// perform any extra clean up such as cached pointers to selected
	// children, etc.
	virtual void deleteAllChildren();

	virtual void	setTentative(BOOL b);
	virtual BOOL	getTentative() const;
	void 	setAllChildrenEnabled(BOOL b);

	virtual void	setVisible(BOOL visible);
	BOOL			getVisible() const			{ return mVisible; }
	virtual void	setEnabled(BOOL enabled);
	BOOL			getEnabled() const			{ return mEnabled; }
	U8              getSoundFlags() const       { return mSoundFlags; }

	virtual BOOL	setLabelArg( const std::string& key, const LLStringExplicit& text );

	virtual void	onVisibilityChange ( BOOL curVisibilityIn );

	void			pushVisible(BOOL visible)	{ mLastVisible = mVisible; setVisible(visible); }
	void			popVisible()				{ setVisible(mLastVisible); mLastVisible = TRUE; }
	
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
	LLRect	getScreenRect() const;
	LLRect	getLocalRect() const;
	virtual LLRect getSnapRect() const;
	LLRect getLocalSnapRect() const;

	// Override and return required size for this object. 0 for width/height means don't care.
	virtual LLRect getRequiredRect();
	void updateBoundingRect();

	LLView*		getRootView();
	LLView*		getParent() const				{ return mParentView; }
	LLView*		getFirstChild() const			{ return (mChildList.empty()) ? NULL : *(mChildList.begin()); }
	S32			getChildCount()	const			{ return (S32)mChildList.size(); }
	template<class _Pr3> void sortChildren(_Pr3 _Pred) { mChildList.sort(_Pred); }
	BOOL		hasAncestor(const LLView* parentp) const;
	BOOL		hasChild(const std::string& childname, BOOL recurse = FALSE) const;
	BOOL 		childHasKeyboardFocus( const std::string& childname ) const;


	//
	// UTILITIES
	//

	// Default behavior is to use reshape flags to resize child views
	virtual void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	virtual void	translate( S32 x, S32 y );
	void			setOrigin( S32 x, S32 y )	{ mRect.translate( x - mRect.mLeft, y - mRect.mBottom ); }
	BOOL			translateIntoRect( const LLRect& constraint, BOOL allow_partial_outside );
	void			centerWithin(const LLRect& bounds);

	virtual void	userSetShape(const LLRect& new_rect);
	virtual LLView*	findSnapRect(LLRect& new_rect, const LLCoordGL& mouse_dir, LLView::ESnapType snap_type, S32 threshold, S32 padding = 0);
	virtual LLView*	findSnapEdge(S32& new_edge_val, const LLCoordGL& mouse_dir, ESnapEdge snap_edge, ESnapType snap_type, S32 threshold, S32 padding = 0);

	virtual BOOL	canSnapTo(const LLView* other_view);

	virtual void	snappedTo(const LLView* snap_view);

	virtual BOOL	handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL	handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data,
									  EAcceptance* accept,
									  std::string& tooltip_msg);

	std::string getShowNamesToolTip();

	virtual void	draw();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	//FIXME: make LLView non-instantiable from XML
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, class LLUICtrlFactory *factory);
	virtual void initFromXML(LLXMLNodePtr node, LLView* parent);
	void parseFollowsFlags(LLXMLNodePtr node);

	// Some widgets, like close box buttons, don't need to be saved
	BOOL getSaveToXML() const { return mSaveToXML; }
	void setSaveToXML(BOOL b) { mSaveToXML = b; }

	virtual void onFocusLost();
	virtual void onFocusReceived();

	typedef enum e_hit_test_type
	{
		HIT_TEST_USE_BOUNDING_RECT,
		HIT_TEST_IGNORE_BOUNDING_RECT
	}EHitTestType;

	BOOL parentPointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL pointInView(S32 x, S32 y, EHitTestType type = HIT_TEST_USE_BOUNDING_RECT) const;
	BOOL blockMouseEvent(S32 x, S32 y) const;

	// See LLMouseHandler virtuals for screenPointToLocal and localPointToScreen
	BOOL localPointToOtherView( S32 x, S32 y, S32 *other_x, S32 *other_y, LLView* other_view) const;
	BOOL localRectToOtherView( const LLRect& local, LLRect* other, LLView* other_view ) const;
	void screenRectToLocal( const LLRect& screen, LLRect* local ) const;
	void localRectToScreen( const LLRect& local, LLRect* screen ) const;
	
	// Listener dispatching functions (Dispatcher deletes pointers to listeners on deregistration or destruction)
	LLSimpleListener* getListenerByName(const std::string& callback_name);
	void registerEventListener(std::string name, LLSimpleListener* function);
	void deregisterEventListener(std::string name);
	std::string findEventListener(LLSimpleListener *listener) const;
	void addListenerToControl(LLEventDispatcher *observer, const std::string& name, LLSD filter, LLSD userdata);

	void addBoolControl(const std::string& name, bool initial_value);
	LLControlVariable *getControl(const std::string& name);
	LLControlVariable *findControl(const std::string& name);

	bool setControlValue(const LLSD& value);
	virtual void	setControlName(const std::string& control, LLView *context);
	virtual std::string getControlName() const { return mControlName; }
//	virtual bool	handleEvent(LLPointer<LLEvent> event, const LLSD& userdata);
	virtual void	setValue(const LLSD& value);
	virtual LLSD	getValue() const;

	const child_list_t*	getChildList() const { return &mChildList; }

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
	/*virtual*/ BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect); // Display mToolTipMsg if no child handles it.
	/*virtual*/ const std::string&	getName() const;
	/*virtual*/ void	onMouseCaptureLost();
	/*virtual*/ BOOL	hasMouseCapture();
	/*virtual*/ BOOL isView(); // Hack to support LLFocusMgr
	/*virtual*/ void	screenPointToLocal(S32 screen_x, S32 screen_y, S32* local_x, S32* local_y) const;
	/*virtual*/ void	localPointToScreen(S32 local_x, S32 local_y, S32* screen_x, S32* screen_y) const;

	template <class T> T* getChild(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const
	{
		LLView* child = getChildView(name, recurse, FALSE);
		T* result = dynamic_cast<T*>(child);
		if (!result)
		{
			// did we find *something* with that name?
			if (child)
			{
				llwarns << "Found child named " << name << " but of wrong type " << typeid(child).name() << ", expecting " << typeid(T).name() << llendl;
			}
			if (create_if_missing)
			{
				// create dummy widget instance here
				result = createDummyWidget<T>(name);
			}
		}
		return result;
	}

	template <class T> T& getChildRef(const std::string& name, BOOL recurse = TRUE) const
	{
		return *getChild<T>(name, recurse, TRUE);
	}

	virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE, BOOL create_if_missing = TRUE) const;

	template <class T> T* createDummyWidget(const std::string& name) const
	{
		T* widget = getDummyWidget<T>(name);
		if (!widget)
		{
			// get xml tag name corresponding to requested widget type (e.g. "button")
			std::string xml_tag = LLWidgetClassRegistry::getInstance()->getTag<T>();
			if (xml_tag.empty())
			{
				llwarns << "No xml tag registered for this class " << llendl;
				return NULL;
			}
			// create dummy xml node (<button name="foo"/>)
			LLXMLNodePtr new_node_ptr = new LLXMLNode(xml_tag.c_str(), FALSE);
			new_node_ptr->createChild("name", TRUE)->setStringValue(name);
			
			widget = dynamic_cast<T*>(createWidget(new_node_ptr));
			if (widget)
			{
				// need non-const to update private dummy widget cache
				llwarns << "Making dummy " << xml_tag << " named " << name << " in " << getName() << llendl;
				mDummyWidgets.insert(std::make_pair(name, widget));
			}
			else
			{
				// dynamic cast will fail if T::fromXML only registered for base class
				llwarns << "Failed to create dummy widget of requested type " << llendl;
				return NULL;
			}
		}
		return widget;
	}

	template <class T> T* getDummyWidget(const std::string& name) const
	{
		dummy_widget_map_t::const_iterator found_it = mDummyWidgets.find(name);
		if (found_it == mDummyWidgets.end())
		{
			return NULL;
		}
		return dynamic_cast<T*>(found_it->second);
	}

	LLView* createWidget(LLXMLNodePtr xml_node) const;


	// statics
	static U32 createRect(LLXMLNodePtr node, LLRect &rect, LLView* parent_view, const LLRect &required_rect = LLRect());
	
	static LLFontGL* selectFont(LLXMLNodePtr node);
	static LLFontGL::HAlign selectFontHAlign(LLXMLNodePtr node);
	static LLFontGL::VAlign selectFontVAlign(LLXMLNodePtr node);
	static LLFontGL::StyleFlags selectFontStyle(LLXMLNodePtr node);

	
	// Only saves color if different from default setting.
	static void addColorXML(LLXMLNodePtr node, const LLColor4& color,
							const char* xml_name, const char* control_name);
	// Escapes " (quot) ' (apos) & (amp) < (lt) > (gt)
	//static std::string escapeXML(const std::string& xml);
	static LLWString escapeXML(const LLWString& xml);
	
	//same as above, but wraps multiple lines in quotes and prepends
	//indent as leading white space on each line
	static std::string escapeXML(const std::string& xml, std::string& indent);

	// focuses the item in the list after the currently-focused item, wrapping if necessary
	static	BOOL focusNext(LLView::child_list_t & result);
	// focuses the item in the list before the currently-focused item, wrapping if necessary
	static	BOOL focusPrev(LLView::child_list_t & result);

	// returns query for iterating over controls in tab order	
	static const LLCtrlQuery & getTabOrderQuery();
	// return query for iterating over focus roots in tab order
	static const LLCtrlQuery & getFocusRootsQuery();

	static BOOL deleteViewByHandle(LLHandle<LLView> handle);
	static LLWindow*	getWindow(void) { return LLUI::sWindow; }

	
protected:
	virtual BOOL	handleKeyHere(KEY key, MASK mask);
	virtual BOOL	handleUnicodeCharHere(llwchar uni_char);

	void			drawDebugRect();
	void			drawChild(LLView* childp, S32 x_offset = 0, S32 y_offset = 0, BOOL force_draw = FALSE);

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

	static bool controlListener(const LLSD& newvalue, LLHandle<LLView> handle, std::string type);

	typedef std::map<std::string, LLControlVariable*> control_map_t;
	control_map_t mFloaterControls;

private:
	LLView*		mParentView;
	child_list_t mChildList;

	std::string	mName;
	// location in pixels, relative to surrounding structure, bottom,left=0,0
	LLRect		mRect;
	LLRect		mBoundingRect;
	
	U32			mReshapeFlags;

	child_tab_order_t mCtrlOrder;
	S32			mDefaultTabGroup;

	BOOL		mEnabled;		// Enabled means "accepts input that has an effect on the state of the application."
								// A disabled view, for example, may still have a scrollbar that responds to mouse events.
	BOOL		mMouseOpaque;	// Opaque views handle all mouse events that are over their rect.
	LLUIString	mToolTipMsg;	// isNull() is true if none.

	U8          mSoundFlags;
	BOOL		mSaveToXML;

	BOOL		mIsFocusRoot;
	BOOL		mUseBoundingRect; // hit test against bounding rectangle that includes all child elements

	LLRootHandle<LLView> mHandle;
	BOOL		mLastVisible;

	BOOL		mVisible;

	S32			mNextInsertionOrdinal;

	static LLWindow* sWindow;	// All root views must know about their window.

	typedef std::map<std::string, LLPointer<LLSimpleListener> > dispatch_list_t;
	dispatch_list_t mDispatchList;

	std::string		mControlName;

	typedef std::map<std::string, LLView*> dummy_widget_map_t;
	mutable dummy_widget_map_t mDummyWidgets;

	boost::signals::connection mControlConnection;

	ECursorType mHoverCursor;
	
public:
	static BOOL	sDebugRects;	// Draw debug rects behind everything.
	static BOOL sDebugKeys;
	static S32	sDepth;
	static BOOL sDebugMouseHandling;
	static std::string sMouseHandlerMessage;
	static S32	sSelectID;
	static BOOL sEditingUI;
	static LLView* sEditingUIView;
	static S32 sLastLeftXML;
	static S32 sLastBottomXML;
	static BOOL sForceReshape;
};

class LLCompareByTabOrder
{
public:
	LLCompareByTabOrder(LLView::child_tab_order_t order) : mTabOrder(order) {}
	virtual ~LLCompareByTabOrder() {}
	bool operator() (const LLView* const a, const LLView* const b) const;
private:
	virtual bool compareTabOrders(const LLView::tab_order_t & a, const LLView::tab_order_t & b) const { return a < b; }
	LLView::child_tab_order_t mTabOrder;
};


#endif //LL_LLVIEW_H
