/** 
 * @file llfloater.h
 * @brief LLFloater base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.


#ifndef LL_FLOATER_H
#define LL_FLOATER_H

#include "llpanel.h"
#include "lluuid.h"
//#include "llnotificationsutil.h"
#include <set>

class LLDragHandle;
class LLResizeHandle;
class LLResizeBar;
class LLButton;
class LLMultiFloater;
class LLFloater;


const BOOL RESIZE_YES = TRUE;
const BOOL RESIZE_NO = FALSE;

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
friend class LLFloaterReg;
friend class LLMultiFloater;
public:
	struct KeyCompare
	{
//		static bool compare(const LLSD& a, const LLSD& b);
		static bool equate(const LLSD& a, const LLSD& b);
/*==========================================================================*|
		bool operator()(const LLSD& a, const LLSD& b) const
		{
			return compare(a, b);
		}
|*==========================================================================*/
	};
	
	enum EFloaterButton
	{
		BUTTON_CLOSE = 0,
		BUTTON_RESTORE,
		BUTTON_MINIMIZE,
		BUTTON_TEAR_OFF,
		BUTTON_DOCK,
		BUTTON_HELP,
		BUTTON_COUNT
	};
	
	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Optional<std::string>	title,
								short_title;
		
		Optional<bool>			single_instance,
								auto_tile,
								can_resize,
								can_minimize,
								can_close,
								can_drag_on_left,
								can_tear_off,
								save_rect,
								save_visibility,
								save_dock_state,
								can_dock;
		Optional<S32>			header_height,
								legacy_header_height; // HACK see initFromXML()

		// Images for top-right controls
		Optional<LLUIImage*>	close_image,
								restore_image,
								minimize_image,
								tear_off_image,
								dock_image,
								help_image;
		Optional<LLUIImage*>	close_pressed_image,
								restore_pressed_image,
								minimize_pressed_image,
								tear_off_pressed_image,
								dock_pressed_image,
								help_pressed_image;
		
		Optional<CommitCallbackParam> open_callback,
									  close_callback;
		
		Params();
	};
	
	// use this to avoid creating your own default LLFloater::Param instance
	static const Params& getDefaultParams();

	// Load translations for tooltips for standard buttons
	static void initClass();

	LLFloater(const LLSD& key, const Params& params = getDefaultParams());

	virtual ~LLFloater();

	// Don't export top/left for rect, only height/width
	static void setupParamsForExport(Params& p, LLView* parent);

	void initFromParams(const LLFloater::Params& p);
	bool initFloaterXML(LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node = NULL);

	/*virtual*/ void handleReshape(const LLRect& new_rect, bool by_user = false);
	/*virtual*/ BOOL canSnapTo(const LLView* other_view);
	/*virtual*/ void setSnappedTo(const LLView* snap_view);
	/*virtual*/ void setFocus( BOOL b );
	/*virtual*/ void setIsChrome(BOOL is_chrome);
	/*virtual*/ void setRect(const LLRect &rect);

	void 			initFloater(const Params& p);

	void			openFloater(const LLSD& key = LLSD());

	// If allowed, close the floater cleanly, releasing focus.
	void			closeFloater(bool app_quitting = false);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	// Release keyboard and mouse focus
	void			releaseFocus();

	// moves to center of gFloaterView
	void			center();

	LLMultiFloater* getHost();

	void			applyTitle();
	std::string		getCurrentTitle() const;
	void			setTitle( const std::string& title);
	std::string		getTitle() const;
	void			setShortTitle( const std::string& short_title );
	std::string		getShortTitle() const;
	void			setTitleVisible(bool visible);
	virtual void	setMinimized(BOOL b);
	void			moveResizeHandlesToFront();
	void			addDependentFloater(LLFloater* dependent, BOOL reposition = TRUE);
	void			addDependentFloater(LLHandle<LLFloater> dependent_handle, BOOL reposition = TRUE);
	LLFloater*		getDependee() { return (LLFloater*)mDependeeHandle.get(); }
	void		removeDependentFloater(LLFloater* dependent);
	BOOL			isMinimized() const				{ return mMinimized; }
	/// isShown() differs from getVisible() in that isShown() also considers
	/// isMinimized(). isShown() is true only if visible and not minimized.
	bool			isShown() const;
	/// The static isShown() can accept a NULL pointer (which of course
	/// returns false). When non-NULL, it calls the non-static isShown().
	static bool		isShown(const LLFloater* floater);
	BOOL			isFirstLook() { return mFirstLook; } // EXT-2653: This function is necessary to prevent overlapping for secondary showed toasts
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

	bool			isMinimizeable() const{ return mCanMinimize; }
	bool			isCloseable() const{ return mCanClose; }
	bool			isDragOnLeft() const{ return mDragOnLeft; }
	S32				getMinWidth() const{ return mMinWidth; }
	S32				getMinHeight() const{ return mMinHeight; }
	S32				getHeaderHeight() const { return mHeaderHeight; }

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMiddleMouseDown(S32 x, S32 y, MASK mask);
	
	virtual BOOL	handleScrollWheel(S32 x, S32 y, S32 mask);
	
	virtual void	draw();
	
	virtual void	onOpen(const LLSD& key) {}
	virtual void	onClose(bool app_quitting) {}

	// This cannot be "const" until all derived floater canClose()
	// methods are const as well.  JC
	virtual BOOL	canClose() { return TRUE; }

	/*virtual*/ void setVisible(BOOL visible); // do not override
	/*virtual*/ void handleVisibilityChange ( BOOL new_visibility ); // do not override
	
	void			setFrontmost(BOOL take_focus = TRUE);
	
	// Defaults to false.
	virtual BOOL	canSaveAs() const { return FALSE; }

	virtual void	saveAs() {}

	void			setSnapTarget(LLHandle<LLFloater> handle) { mSnappedTo = handle; }
	void			clearSnapTarget() { mSnappedTo.markDead(); }
	LLHandle<LLFloater>	getSnapTarget() const { return mSnappedTo; }

	LLHandle<LLFloater> getHandle() const { return mHandle; }
	const LLSD& 	getKey() { return mKey; }
	BOOL		 	matchesKey(const LLSD& key) { return mSingleInstance || KeyCompare::equate(key, mKey); }
	
	const std::string& getInstanceName() { return mInstanceName; }
	
	bool            isDockable() const { return mCanDock; }
	void            setCanDock(bool b);

	bool            isDocked() const { return mDocked; }
	virtual void    setDocked(bool docked, bool pop_on_undock = true);

	virtual void    setTornOff(bool torn_off) { mTornOff = torn_off; }

	// Return a closeable floater, if any, given the current focus.
	static LLFloater* getClosableFloaterFromFocus(); 

	// Close the floater returned by getClosableFloaterFromFocus() and 
	// handle refocusing.
	static void		closeFocusedFloater();

//	LLNotification::Params contextualNotification(const std::string& name) 
//	{ 
//	    return LLNotification::Params(name).context(mNotificationContext); 
//	}

	static void		onClickClose(LLFloater* floater);
	static void		onClickMinimize(LLFloater* floater);
	static void		onClickTearOff(LLFloater* floater);
	static void     onClickDock(LLFloater* floater);
	static void		onClickHelp(LLFloater* floater);

	static void		setFloaterHost(LLMultiFloater* hostp) {sHostp = hostp; }
	static LLMultiFloater* getFloaterHost() {return sHostp; }
		
protected:

	void			setRectControl(const std::string& rectname) { mRectControl = rectname; };

	virtual void    applySavedVariables();

	void			applyRectControl();
	void			applyDockState();
	void			storeRectControl();
	void			storeVisibilityControl();
	void			storeDockStateControl();

	void		 	setKey(const LLSD& key);
	void		 	setInstanceName(const std::string& name);
	
	virtual void	bringToFront(S32 x, S32 y);
	virtual void	setVisibleAndFrontmost(BOOL take_focus=TRUE);    
	
	void			setExpandedRect(const LLRect& rect) { mExpandedRect = rect; } // size when not minimized
	const LLRect&	getExpandedRect() const { return mExpandedRect; }

	void			setAutoFocus(BOOL focus) { mAutoFocus = focus; } // whether to automatically take focus when opened
	LLDragHandle*	getDragHandle() const { return mDragHandle; }

	void			destroy() { die(); } // Don't call this directly.  You probably want to call closeFloater()

private:
	void			setForeground(BOOL b);	// called only by floaterview
	void			cleanupHandles(); // remove handles to dead floaters
	void			createMinimizeButton();
	void			updateButtons();
	void			buildButtons(const Params& p);
	
	// Images and tooltips are named in the XML, but we want to look them
	// up by index.
	static LLUIImage*	getButtonImage(const Params& p, EFloaterButton e);
	static LLUIImage*	getButtonPressedImage(const Params& p, EFloaterButton e);
	static std::string	getButtonTooltip(const Params& p, EFloaterButton e);
	
	BOOL			offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButton index);
	void			addResizeCtrls();
	void			layoutResizeCtrls();
	void			enableResizeCtrls(bool enable);
	void 			addDragHandle();
	void			layoutDragHandle();		// repair layout

public:
	// Called when floater is opened, passes mKey
	// Public so external views or floaters can watch for this floater opening
	commit_signal_t mOpenSignal;

	// Called when floater is closed, passes app_qitting as LLSD()
	// Public so external views or floaters can watch for this floater closing
	commit_signal_t mCloseSignal;		

protected:
	std::string		mRectControl;
	std::string		mVisibilityControl;
	std::string		mDocStateControl;
	LLSD			mKey;				// Key used for retrieving instances; set (for now) by LLFLoaterReg

	LLDragHandle*	mDragHandle;
	LLResizeBar*	mResizeBar[4];
	LLResizeHandle*	mResizeHandle[4];

private:
	LLRect			mExpandedRect;
	
	LLUIString		mTitle;
	LLUIString		mShortTitle;
	
	BOOL			mSingleInstance;	// TRUE if there is only ever one instance of the floater
	std::string		mInstanceName;		// Store the instance name so we can remove ourselves from the list
	BOOL			mAutoTile;			// TRUE if placement of new instances tiles
	
	BOOL			mCanTearOff;
	BOOL			mCanMinimize;
	BOOL			mCanClose;
	BOOL			mDragOnLeft;
	BOOL			mResizable;
	
	S32				mMinWidth;
	S32				mMinHeight;
	S32				mHeaderHeight;		// height in pixels of header for title, drag bar
	S32				mLegacyHeaderHeight;// HACK see initFloaterXML()
	
	BOOL			mMinimized;
	BOOL			mForeground;
	LLHandle<LLFloater>	mDependeeHandle;
	

	BOOL			mFirstLook;			// TRUE if the _next_ time this floater is visible will be the first time in the session that it is visible.
	
	typedef std::set<LLHandle<LLFloater> > handle_set_t;
	typedef std::set<LLHandle<LLFloater> >::iterator handle_set_iter_t;
	handle_set_t	mDependents;

	bool			mButtonsEnabled[BUTTON_COUNT];
	LLButton*		mButtons[BUTTON_COUNT];
	F32				mButtonScale;
	BOOL			mAutoFocus;
	LLHandle<LLFloater> mSnappedTo;
	
	LLHandle<LLFloater> mHostHandle;
	LLHandle<LLFloater> mLastHostHandle;

	bool            mCanDock;
	bool            mDocked;
	bool            mTornOff;

	static LLMultiFloater* sHostp;
	static BOOL		sQuitting;
	static std::string	sButtonNames[BUTTON_COUNT];
	static std::string	sButtonToolTips[BUTTON_COUNT];
	static std::string  sButtonToolTipsIndex[BUTTON_COUNT];
	
	typedef void(*click_callback)(LLFloater*);
	static click_callback sButtonCallbacks[BUTTON_COUNT];

	typedef std::map<LLHandle<LLFloater>, LLFloater*> handle_map_t;
	typedef std::map<LLHandle<LLFloater>, LLFloater*>::iterator handle_map_iter_t;
	static handle_map_t	sFloaterMap;

	std::vector<LLHandle<LLView> > mMinimizedHiddenChildren;

	BOOL			mHasBeenDraggedWhileMinimized;
	S32				mPreviousMinimizedBottom;
	S32				mPreviousMinimizedLeft;

//	LLFloaterNotificationContext* mNotificationContext;
	LLRootHandle<LLFloater>		mHandle;	
};


/////////////////////////////////////////////////////////////
// LLFloaterView
// Parent of all floating panels

class LLFloaterView : public LLUICtrl
{
protected:
	LLFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void reshapeFloater(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical);

	/*virtual*/ void draw();
	/*virtual*/ LLRect getSnapRect() const;
	/*virtual*/ void refresh();

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

	LLFloater* getFrontmost() const;
	LLFloater* getBackmost() const;
	LLFloater* getParentFloater(LLView* viewp) const;
	LLFloater* getFocusedFloater() const;
	void		syncFloaterTabOrder();

	// Returns z order of child provided. 0 is closest, larger numbers
	// are deeper in the screen. If there is no such child, the return
	// value is not defined.
	S32 getZOrder(LLFloater* child);

	void setSnapOffsetBottom(S32 offset) { mSnapOffsetBottom = offset; }
	void setSnapOffsetRight(S32 offset) { mSnapOffsetRight = offset; }

private:
	S32				mColumn;
	S32				mNextLeft;
	S32				mNextTop;
	BOOL			mFocusCycleMode;
	S32				mSnapOffsetBottom;
	S32				mSnapOffsetRight;
};

//
// Globals
//

extern LLFloaterView* gFloaterView;

#endif  // LL_FLOATER_H



