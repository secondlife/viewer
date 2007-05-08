/** 
 * @file llfloater.h
 * @brief LLFloater base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.


#ifndef LL_FLOATER_H
#define LL_FLOATER_H

#include "llpanel.h"
#include "lluuid.h"
#include "lltabcontainer.h"
#include <set>

class LLDragHandle;
class LLResizeHandle;
class LLResizeBar;
class LLButton;
class LLMultiFloater;

const S32 LLFLOATER_VPAD = 6;
const S32 LLFLOATER_HPAD = 6;
const S32 LLFLOATER_CLOSE_BOX_SIZE = 16;
const S32 LLFLOATER_HEADER_SIZE = 16;

const BOOL RESIZE_YES = TRUE;
const BOOL RESIZE_NO = FALSE;

const S32 DEFAULT_MIN_WIDTH = 100;
const S32 DEFAULT_MIN_HEIGHT = 100;

const BOOL DRAG_ON_TOP = FALSE;
const BOOL DRAG_ON_LEFT = TRUE;

const BOOL MINIMIZE_YES = TRUE;
const BOOL MINIMIZE_NO = FALSE;

const BOOL CLOSE_YES = TRUE;
const BOOL CLOSE_NO = FALSE;

const BOOL ADJUST_VERTICAL_YES = TRUE;
const BOOL ADJUST_VERTICAL_NO = FALSE;


class LLFloater : public LLPanel
{
friend class LLFloaterView;
public:
	enum EFloaterButtons
	{
		BUTTON_CLOSE,
		BUTTON_RESTORE,
		BUTTON_MINIMIZE,
		BUTTON_TEAR_OFF,
		BUTTON_EDIT,
		BUTTON_COUNT
	};
	
	LLFloater();
 	LLFloater(const LLString& name); //simple constructor for data-driven initialization
	LLFloater(	const LLString& name, const LLRect& rect, const LLString& title,
		BOOL resizable = FALSE,
		S32 min_width = DEFAULT_MIN_WIDTH,
		S32 min_height = DEFAULT_MIN_HEIGHT,
		BOOL drag_on_left = FALSE,
		BOOL minimizable = TRUE,
		BOOL close_btn = TRUE,
		BOOL bordered = BORDER_NO);

	LLFloater(	const LLString& name, const LLString& rect_control, const LLString& title,
		BOOL resizable = FALSE,
		S32 min_width = DEFAULT_MIN_WIDTH, 
		S32 min_height = DEFAULT_MIN_HEIGHT,
		BOOL drag_on_left = FALSE,
		BOOL minimizable = TRUE,
		BOOL close_btn = TRUE,
		BOOL bordered = BORDER_NO);

	virtual ~LLFloater();

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);
	void initFloaterXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory, BOOL open = TRUE);

	/*virtual*/ void userSetShape(const LLRect& new_rect);
	/*virtual*/ BOOL canSnapTo(LLView* other_view);
	/*virtual*/ void snappedTo(LLView* snap_view);
	/*virtual*/ void setFocus( BOOL b );
	/*virtual*/ void setIsChrome(BOOL is_chrome);

	// Can be called multiple times to reset floater parameters.
	// Deletes all children of the floater.
	virtual void		init(const LLString& title, BOOL resizable, 
						S32 min_width, S32 min_height, BOOL drag_on_left,
						BOOL minimizable, BOOL close_btn);
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	virtual void	open();	/* Flawfinder: ignore */

	// If allowed, close the floater cleanly, releasing focus.
	// app_quitting is passed to onClose() below.
	virtual void	close(bool app_quitting = false);

	void			setAutoFocus(BOOL focus) { mAutoFocus = focus; setFocus(focus); }
	
	// Release keyboard and mouse focus
	void			releaseFocus();

	// moves to center of gFloaterView
	void			center();
	// applies rectangle stored in mRectControl, if any
	void			applyRectControl();


	LLMultiFloater* getHost() { return (LLMultiFloater*)LLFloater::getFloaterByHandle(mHostHandle); }

	void			setTitle( const LLString& title );
	const LLString&		getTitle() const;
	virtual void	setMinimized(BOOL b);
	void			moveResizeHandleToFront();
	void			addDependentFloater(LLFloater* dependent, BOOL reposition = TRUE);
	void			addDependentFloater(LLViewHandle dependent_handle, BOOL reposition = TRUE);
	LLFloater*              getDependee() { return (LLFloater*)LLFloater::getFloaterByHandle(mDependeeHandle); }
	void                    removeDependentFloater(LLFloater* dependent);
	BOOL			isMinimized()					{ return mMinimized; }
	BOOL			isFrontmost();
	BOOL			isDependent()					{ return !mDependeeHandle.isDead(); }
	void			setCanMinimize(BOOL can_minimize);
	void			setCanClose(BOOL can_close);
	void			setCanTearOff(BOOL can_tear_off);
	virtual void	setCanResize(BOOL can_resize);
	void			setCanDrag(BOOL can_drag);
	void			setHost(LLMultiFloater* host);
	BOOL			isResizable() const				{ return mResizable; }
	void			setResizeLimits( S32 min_width, S32 min_height );
	void			getResizeLimits( S32* min_width, S32* min_height ) { *min_width = mMinWidth; *min_height = mMinHeight; }


	bool			isMinimizeable() const{ return mButtonsEnabled[BUTTON_MINIMIZE]; }
	// Does this window have a close button, NOT can we close it right now.
	bool			isCloseable() const{ return (mButtonsEnabled[BUTTON_CLOSE] ? true : false); }
	bool			isDragOnLeft() const{ return mDragOnLeft; }
	S32				getMinWidth() const{ return mMinWidth; }
	S32				getMinHeight() const{ return mMinHeight; }

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);

	virtual void	draw();

	// does nothing by default
	virtual void	onOpen();

	// Call destroy() to free memory, or setVisible(FALSE) to keep it
	// If app_quitting, you might not want to save your visibility.
	// Defaults to destroy().
	virtual void	onClose(bool app_quitting);

	// Defaults to true.
	virtual BOOL	canClose();

	virtual void	setVisible(BOOL visible);
	void			setFrontmost(BOOL take_focus = TRUE);

	// Defaults to false.
	virtual BOOL	canSaveAs();

	// Defaults to no-op.
	virtual void	saveAs();

	void			setSnapTarget(LLViewHandle handle) { mSnappedTo = handle; }
	void			clearSnapTarget() { mSnappedTo.markDead(); }
	LLViewHandle	getSnapTarget() { return mSnappedTo; }

	static void		closeFocusedFloater();

	static void		onClickClose(void *userdata);
	static void		onClickMinimize(void *userdata);
	static void		onClickTearOff(void *userdata);
	static void		onClickEdit(void *userdata);

	static void		setFloaterHost(LLMultiFloater* hostp) {sHostp = hostp; }
	static void		setEditModeEnabled(BOOL enable);
	static BOOL		getEditModeEnabled();
	static LLMultiFloater*		getFloaterHost() {return sHostp; }

	static LLFloater* getFloaterByHandle(LLViewHandle handle);

protected:
	// Don't call this directly.  You probably want to call close(). JC
	void			destroy();
	virtual void	bringToFront(S32 x, S32 y);
	virtual void	setVisibleAndFrontmost(BOOL take_focus=TRUE);
	void			setForeground(BOOL b);	// called only by floaterview
	void			cleanupHandles(); // remove handles to dead floaters
	void			createMinimizeButton();
	void			updateButtons();
	void			buildButtons();

protected:
//	static LLViewerImage*		sBackgroundImage;
//	static LLViewerImage*		sShadowImage;

	LLDragHandle*	mDragHandle;
	LLResizeBar*	mResizeBar[4];
	LLResizeHandle* mResizeHandle[4];
	LLButton		*mMinimizeButton;
	BOOL			mCanTearOff;
	BOOL			mMinimized;
	LLRect			mPreviousRect;
	BOOL			mForeground;
	LLViewHandle	mDependeeHandle;

	BOOL			mFirstLook;			// TRUE if the _next_ time this floater is visible will be the first time in the session that it is visible.

	BOOL			mResizable;
	S32				mMinWidth;
	S32				mMinHeight;

	BOOL			mAutoFocus;
	BOOL			mEditing;
	
	typedef std::set<LLViewHandle> handle_set_t;
	typedef std::set<LLViewHandle>::iterator handle_set_iter_t;
	handle_set_t	mDependents;
	bool            mDragOnLeft;

	BOOL			mButtonsEnabled[BUTTON_COUNT];
	LLButton*		mButtons[BUTTON_COUNT];
	F32				mButtonScale;

	LLViewHandle	mSnappedTo;
	
	LLViewHandle mHostHandle;
	LLViewHandle mLastHostHandle;

	static BOOL		sEditModeEnabled;
	static LLMultiFloater* sHostp;
	static LLString	sButtonActiveImageNames[BUTTON_COUNT];
	static LLString	sButtonInactiveImageNames[BUTTON_COUNT];
	static LLString	sButtonPressedImageNames[BUTTON_COUNT];
	static LLString	sButtonNames[BUTTON_COUNT];
	static LLString	sButtonToolTips[BUTTON_COUNT];
	typedef void (*click_callback)(void *);
	static click_callback sButtonCallbacks[BUTTON_COUNT];

	typedef std::map<LLViewHandle, LLFloater*> handle_map_t;
	typedef std::map<LLViewHandle, LLFloater*>::iterator handle_map_iter_t;
	static handle_map_t	sFloaterMap;

	std::vector<LLView*> mMinimizedHiddenChildren;
};


/////////////////////////////////////////////////////////////
// LLFloaterView
// Parent of all floating panels

class LLFloaterView : public LLUICtrl
{
public:
	LLFloaterView( const LLString& name, const LLRect& rect );

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);
	void reshape(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical);

	/*virtual*/ void draw();
	/*virtual*/ const LLRect getSnapRect() const;
	void refresh();

	void			getNewFloaterPosition( S32* left, S32* top );
	void			resetStartingFloaterPosition();
	LLRect			findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor );

	// Given a child of gFloaterView, make sure this view can fit entirely onscreen.
	void			adjustToFitScreen(LLFloater* floater, BOOL allow_partial_outside);

	void			getMinimizePosition( S32 *left, S32 *bottom);
	void			restoreAll();		// un-minimize all floaters
	typedef std::set<LLView*> skip_list_t;
	void pushVisibleAll(BOOL visible, const skip_list_t& skip_list = skip_list_t());
	void popVisibleAll(const skip_list_t& skip_list = skip_list_t());

	void			setCycleMode(BOOL mode);
	BOOL			getCycleMode();
	void			bringToFront( LLFloater* child, BOOL give_focus = TRUE );
	void			highlightFocusedFloater();
	void			unhighlightFocusedFloater();
	void			focusFrontFloater();
	void			destroyAllChildren();
	// attempt to close all floaters
	void			closeAllChildren(bool app_quitting);
	BOOL			allChildrenClosed();

	LLFloater* getFrontmost();
	LLFloater* getBackmost();
	LLFloater* getParentFloater(LLView* viewp);
	LLFloater* getFocusedFloater();
	void		syncFloaterTabOrder();

	// Get a floater based the handle. If this returns NULL, it is up
	// to the caller to discard the handle.
	LLFloater* getFloaterByHandle(LLViewHandle handle);

	// Returns z order of child provided. 0 is closest, larger numbers
	// are deeper in the screen. If there is no such child, the return
	// value is not defined.
	S32 getZOrder(LLFloater* child);

	void setSnapOffsetBottom(S32 offset) { mSnapOffsetBottom = offset; }

private:
	S32				mColumn;
	S32				mNextLeft;
	S32				mNextTop;
	BOOL			mFocusCycleMode;
	S32				mSnapOffsetBottom;
};

class LLMultiFloater : public LLFloater
{
public:
	LLMultiFloater();
	LLMultiFloater(LLTabContainerCommon::TabPosition tab_pos);
	LLMultiFloater(const LLString& name);
	LLMultiFloater(const LLString& name, const LLRect& rect, LLTabContainer::TabPosition tab_pos = LLTabContainer::TOP, BOOL auto_resize = FALSE);
	LLMultiFloater(const LLString& name, const LLString& rect_control, LLTabContainer::TabPosition tab_pos = LLTabContainer::TOP, BOOL auto_resize = FALSE);
	virtual ~LLMultiFloater();

	virtual BOOL postBuild();
	/*virtual*/ void open();	/* Flawfinder: ignore */
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);

	/*virtual*/ EWidgetType getWidgetType() const;
	/*virtual*/ LLString getWidgetTag() const;

	virtual void setCanResize(BOOL can_resize);
	virtual void growToFit(LLFloater* floaterp, S32 width, S32 height);
	virtual void addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainerCommon::eInsertionPoint insertion_point = LLTabContainerCommon::END);

	virtual void showFloater(LLFloater* floaterp);
	virtual void removeFloater(LLFloater* floaterp);

	virtual void tabOpen(LLFloater* opened_floater, bool from_click);
	virtual void tabClose();

	virtual BOOL selectFloater(LLFloater* floaterp);
	virtual void selectNextFloater();
	virtual void selectPrevFloater();

	virtual LLFloater* getActiveFloater();
	virtual BOOL       isFloaterFlashing(LLFloater* floaterp);
	virtual S32			getFloaterCount();

	virtual void setFloaterFlashing(LLFloater* floaterp, BOOL flashing);
	virtual BOOL closeAllFloaters();	//Returns FALSE if the floater could not be closed due to pending confirmation dialogs
	void setTabContainer(LLTabContainerCommon* tab_container) { if (!mTabContainer) mTabContainer = tab_container; }
	static void onTabSelected(void* userdata, bool);

	virtual void resizeToContents();

protected:
	struct LLFloaterData
	{
		S32		mWidth;
		S32		mHeight;
		BOOL	mCanMinimize;
		BOOL	mCanResize;
	};

	LLTabContainerCommon*		mTabContainer;
	
	typedef std::map<LLViewHandle, LLFloaterData> floater_data_map_t;
	floater_data_map_t	mFloaterDataMap;
	
	LLTabContainerCommon::TabPosition mTabPos;
	BOOL				mAutoResize;
};


extern LLFloaterView* gFloaterView;

#endif  // LL_FLOATER_H

