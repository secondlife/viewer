/** 
 * @file llfloater.h
 * @brief LLFloater base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
const S32 LLFLOATER_HEADER_SIZE = 18;

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
	/*virtual*/ BOOL canSnapTo(const LLView* other_view);
	/*virtual*/ void snappedTo(const LLView* snap_view);
	/*virtual*/ void setFocus( BOOL b );
	/*virtual*/ void setIsChrome(BOOL is_chrome);

	// Can be called multiple times to reset floater parameters.
	// Deletes all children of the floater.
	virtual void		initFloater(const LLString& title, BOOL resizable, 
						S32 min_width, S32 min_height, BOOL drag_on_left,
						BOOL minimizable, BOOL close_btn);

	virtual void	open();	/* Flawfinder: ignore */

	// If allowed, close the floater cleanly, releasing focus.
	// app_quitting is passed to onClose() below.
	virtual void	close(bool app_quitting = false);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	// Release keyboard and mouse focus
	void			releaseFocus();

	// moves to center of gFloaterView
	void			center();
	// applies rectangle stored in mRectControl, if any
	void			applyRectControl();


	LLMultiFloater* getHost() { return (LLMultiFloater*)mHostHandle.get(); }

	void			setTitle( const LLString& title );
	const LLString&	getTitle() const;
	void			setShortTitle( const LLString& short_title );
	LLString		getShortTitle();
	virtual void	setMinimized(BOOL b);
	void			moveResizeHandlesToFront();
	void			addDependentFloater(LLFloater* dependent, BOOL reposition = TRUE);
	void			addDependentFloater(LLHandle<LLFloater> dependent_handle, BOOL reposition = TRUE);
	LLFloater*      getDependee() { return (LLFloater*)mDependeeHandle.get(); }
	void            removeDependentFloater(LLFloater* dependent);
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
	bool			isCloseable() const{ return (mButtonsEnabled[BUTTON_CLOSE]); }
	bool			isDragOnLeft() const{ return mDragOnLeft; }
	S32				getMinWidth() const{ return mMinWidth; }
	S32				getMinHeight() const{ return mMinHeight; }

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);

	virtual void	draw();

	virtual void	onOpen() {}

	// Call destroy() to free memory, or setVisible(FALSE) to keep it
	// If app_quitting, you might not want to save your visibility.
	// Defaults to destroy().
	virtual void	onClose(bool app_quitting) { destroy(); }

	// This cannot be "const" until all derived floater canClose()
	// methods are const as well.  JC
	virtual BOOL	canClose() { return TRUE; }

	virtual void	setVisible(BOOL visible);
	void			setFrontmost(BOOL take_focus = TRUE);

	// Defaults to false.
	virtual BOOL	canSaveAs() const { return FALSE; }

	virtual void	saveAs() {}

	void			setSnapTarget(LLHandle<LLFloater> handle) { mSnappedTo = handle; }
	void			clearSnapTarget() { mSnappedTo.markDead(); }
	LLHandle<LLFloater>	getSnapTarget() { return mSnappedTo; }

	LLHandle<LLFloater> getHandle() { return mHandle; }

	static void		closeFocusedFloater();

	static void		onClickClose(void *userdata);
	static void		onClickMinimize(void *userdata);
	static void		onClickTearOff(void *userdata);
	static void		onClickEdit(void *userdata);

	static void		setFloaterHost(LLMultiFloater* hostp) {sHostp = hostp; }
	static void		setEditModeEnabled(BOOL enable);
	static BOOL		getEditModeEnabled() { return sEditModeEnabled; }
	static LLMultiFloater*		getFloaterHost() {return sHostp; }

protected:

	virtual void	bringToFront(S32 x, S32 y);
	virtual void	setVisibleAndFrontmost(BOOL take_focus=TRUE);    
	
	void            setExpandedRect(const LLRect& rect) { mExpandedRect = rect; } // size when not minimized
    const LLRect&    getExpandedRect() const { return mExpandedRect; }

	void			setAutoFocus(BOOL focus) { mAutoFocus = focus; } // whether to automatically take focus when opened
	LLDragHandle*	getDragHandle() const { return mDragHandle; }

	void			destroy() { die(); } // Don't call this directly.  You probably want to call close(). JC

private:
	
	void			setForeground(BOOL b);	// called only by floaterview
	void			cleanupHandles(); // remove handles to dead floaters
	void			createMinimizeButton();
	void			updateButtons();
	void			buildButtons();

	LLRect			mExpandedRect;
	LLDragHandle*	mDragHandle;
	LLResizeBar*	mResizeBar[4];
	LLResizeHandle* mResizeHandle[4];
	LLButton		*mMinimizeButton;
	BOOL			mCanTearOff;
	BOOL			mMinimized;
	BOOL			mForeground;
	LLHandle<LLFloater>	mDependeeHandle;
	LLString		mShortTitle;

	BOOL			mFirstLook;			// TRUE if the _next_ time this floater is visible will be the first time in the session that it is visible.

	BOOL			mResizable;
	S32				mMinWidth;
	S32				mMinHeight;

	BOOL			mEditing;
	
	typedef std::set<LLHandle<LLFloater> > handle_set_t;
	typedef std::set<LLHandle<LLFloater> >::iterator handle_set_iter_t;
	handle_set_t	mDependents;
	bool            mDragOnLeft;

	BOOL			mButtonsEnabled[BUTTON_COUNT];
	LLButton*		mButtons[BUTTON_COUNT];
	F32				mButtonScale;
	BOOL			mAutoFocus;
	LLHandle<LLFloater> mSnappedTo;
	
	LLHandle<LLFloater> mHostHandle;
	LLHandle<LLFloater> mLastHostHandle;

	static LLMultiFloater* sHostp;
	static BOOL		sEditModeEnabled;
	static LLString	sButtonActiveImageNames[BUTTON_COUNT];
	static LLString	sButtonInactiveImageNames[BUTTON_COUNT];
	static LLString	sButtonPressedImageNames[BUTTON_COUNT];
	static LLString	sButtonNames[BUTTON_COUNT];
	static LLString	sButtonToolTips[BUTTON_COUNT];
	typedef void (*click_callback)(void *);
	static click_callback sButtonCallbacks[BUTTON_COUNT];

	typedef std::map<LLHandle<LLFloater>, LLFloater*> handle_map_t;
	typedef std::map<LLHandle<LLFloater>, LLFloater*>::iterator handle_map_iter_t;
	static handle_map_t	sFloaterMap;

	std::vector<LLHandle<LLView> > mMinimizedHiddenChildren;

	BOOL			mHasBeenDraggedWhileMinimized;
	S32				mPreviousMinimizedBottom;
	S32				mPreviousMinimizedLeft;
	
private:
	LLRootHandle<LLFloater>		mHandle;	
};

/////////////////////////////////////////////////////////////
// LLFloaterView
// Parent of all floating panels

class LLFloaterView : public LLUICtrl
{
public:
	LLFloaterView( const LLString& name, const LLRect& rect );

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void reshapeFloater(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical);

	/*virtual*/ void draw();
	/*virtual*/ LLRect getSnapRect() const;
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

	void			setCycleMode(BOOL mode) { mFocusCycleMode = mode; }
	BOOL			getCycleMode() const { return mFocusCycleMode; }
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

// https://wiki.lindenlab.com/mediawiki/index.php?title=LLMultiFloater&oldid=81376
class LLMultiFloater : public LLFloater
{
public:
	LLMultiFloater();
	LLMultiFloater(LLTabContainer::TabPosition tab_pos);
	LLMultiFloater(const LLString& name);
	LLMultiFloater(const LLString& name, const LLRect& rect, LLTabContainer::TabPosition tab_pos = LLTabContainer::TOP, BOOL auto_resize = TRUE);
	LLMultiFloater(const LLString& name, const LLString& rect_control, LLTabContainer::TabPosition tab_pos = LLTabContainer::TOP, BOOL auto_resize = TRUE);
	virtual ~LLMultiFloater() {};

	virtual BOOL postBuild();
	/*virtual*/ void open();	/* Flawfinder: ignore */
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);

	virtual void setCanResize(BOOL can_resize);
	virtual void growToFit(S32 content_width, S32 content_height);
	virtual void addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point = LLTabContainer::END);

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
	void setTabContainer(LLTabContainer* tab_container) { if (!mTabContainer) mTabContainer = tab_container; }
	static void onTabSelected(void* userdata, bool);

	virtual void updateResizeLimits();

protected:
	struct LLFloaterData
	{
		S32		mWidth;
		S32		mHeight;
		BOOL	mCanMinimize;
		BOOL	mCanResize;
	};

	LLTabContainer*		mTabContainer;
	
	typedef std::map<LLHandle<LLFloater>, LLFloaterData> floater_data_map_t;
	floater_data_map_t	mFloaterDataMap;
	
	LLTabContainer::TabPosition mTabPos;
	BOOL				mAutoResize;
	S32					mOrigMinWidth, mOrigMinHeight;  // logically const but initialized late
};

// visibility policy specialized for floaters
template<>
class VisibilityPolicy<LLFloater>
{
public:
	// visibility methods
	static bool visible(LLFloater* instance, const LLSD& key)
	{
		if (instance) 
		{
			return !instance->isMinimized() && instance->isInVisibleChain();
		}
		return FALSE;
	}

	static void show(LLFloater* instance, const LLSD& key)
	{
		if (instance) 
		{
			instance->open();
			if (instance->getHost())
			{
				instance->getHost()->open();
			}
		}
	}

	static void hide(LLFloater* instance, const LLSD& key)
	{
		if (instance) instance->close();
	}
};


// singleton implementation for floaters (provides visibility policy)
// https://wiki.lindenlab.com/mediawiki/index.php?title=LLFloaterSingleton&oldid=79410

template <class T> class LLFloaterSingleton : public LLUISingleton<T, VisibilityPolicy<LLFloater> >
{
};


extern LLFloaterView* gFloaterView;

#endif  // LL_FLOATER_H



