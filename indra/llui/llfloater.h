/** 
 * @file llfloater.h
 * @brief LLFloater base class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.


#ifndef LL_FLOATER_H
#define LL_FLOATER_H

#include "llpanel.h"
#include "lluuid.h"
//#include "llnotificationsutil.h"
#include <set>
#include <boost/signals2.hpp>

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

namespace LLFloaterEnums
{
	enum EOpenPositioning
	{
		POSITIONING_RELATIVE,
		POSITIONING_CASCADING,
		POSITIONING_CASCADE_GROUP,
		POSITIONING_CENTERED,
		POSITIONING_SPECIFIED,
		POSITIONING_COUNT
	};
}

namespace LLInitParam
{
	template<>
	struct TypeValues<LLFloaterEnums::EOpenPositioning> : public TypeValuesHelper<LLFloaterEnums::EOpenPositioning>
	{
		static void declareValues();
	};
}

struct LL_COORD_FLOATER
{
	typedef F32 value_t;

	LLCoordCommon convertToCommon() const;
	void convertFromCommon(const LLCoordCommon& from);
protected:
	LLHandle<LLFloater> mFloater;
};

struct LLCoordFloater : LLCoord<LL_COORD_FLOATER>
{
	typedef LLCoord<LL_COORD_FLOATER> coord_t;

	LLCoordFloater() {}
	LLCoordFloater(F32 x, F32 y, LLFloater& floater);
	LLCoordFloater(const LLCoordCommon& other, LLFloater& floater);

	LLCoordFloater& operator=(const LLCoordCommon& other)
	{
		convertFromCommon(other);
		return *this;
	}

	LLCoordFloater& operator=(const LLCoordFloater& other);

	bool operator==(const LLCoordFloater& other) const;
	bool operator!=(const LLCoordFloater& other) const { return !(*this == other); }

	void setFloater(LLFloater& floater);
};

class LLFloater : public LLPanel, public LLInstanceTracker<LLFloater>
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
								reuse_instance,
								can_resize,
								can_minimize,
								can_close,
								can_drag_on_left,
								can_tear_off,
								save_rect,
								save_visibility,
								save_dock_state,
								can_dock,
								show_title;
		
		Optional<LLFloaterEnums::EOpenPositioning>	positioning;
		
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

		Ignored					follows;
		
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
	bool buildFromFile(const std::string &filename);

	boost::signals2::connection setMinimizeCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setOpenCallback( const commit_signal_t::slot_type& cb );
	boost::signals2::connection setCloseCallback( const commit_signal_t::slot_type& cb );

	void initFromParams(const LLFloater::Params& p);
	bool initFloaterXML(LLXMLNodePtr node, LLView *parent, const std::string& filename, LLXMLNodePtr output_node = NULL);

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
	static bool     isVisible(const LLFloater* floater);
	static bool     isMinimized(const LLFloater* floater);
	BOOL			isFirstLook() { return mFirstLook; } // EXT-2653: This function is necessary to prevent overlapping for secondary showed toasts
	BOOL			isFrontmost();
	BOOL			isDependent()					{ return !mDependeeHandle.isDead(); }
	void			setCanMinimize(BOOL can_minimize);
	void			setCanClose(BOOL can_close);
	void			setCanTearOff(BOOL can_tear_off);
	virtual void	setCanResize(BOOL can_resize);
	void			setCanDrag(BOOL can_drag);
	bool			getCanDrag();
	void			setHost(LLMultiFloater* host);
	BOOL			isResizable() const				{ return mResizable; }
	void			setResizeLimits( S32 min_width, S32 min_height );
	void			getResizeLimits( S32* min_width, S32* min_height ) { *min_width = mMinWidth; *min_height = mMinHeight; }

	static std::string		getControlName(const std::string& name, const LLSD& key);
	static LLControlGroup*	getControlGroup();

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
	virtual void	drawShadow(LLPanel* panel);
	
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

	LLHandle<LLFloater> getHandle() const { return getDerivedHandle<LLFloater>(); }
	const LLSD& 	getKey() { return mKey; }
	virtual bool	matchesKey(const LLSD& key) { return mSingleInstance || KeyCompare::equate(key, mKey); }
	
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

	void			updateTransparency(ETypeTransparency transparency_type);
		
	void			enableResizeCtrls(bool enable, bool width = true, bool height = true);

	bool			isPositioning(LLFloaterEnums::EOpenPositioning p) const { return (p == mPositioning); }
protected:
	void			applyControlsAndPosition(LLFloater* other);

	void			stackWith(LLFloater& other);

	virtual bool	applyRectControl();
	bool			applyDockState();
	void			applyPositioning(LLFloater* other, bool on_open);
	void			applyRelativePosition();

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
	BOOL			getAutoFocus() const { return mAutoFocus; }
	LLDragHandle*	getDragHandle() const { return mDragHandle; }

	void			destroy(); // Don't call this directly.  You probably want to call closeFloater()

	virtual	void	onClickCloseBtn();

	virtual void	updateTitleButtons();

private:
	void			setForeground(BOOL b);	// called only by floaterview
	void			cleanupHandles(); // remove handles to dead floaters
	void			createMinimizeButton();
	void			buildButtons(const Params& p);
	
	// Images and tooltips are named in the XML, but we want to look them
	// up by index.
	static LLUIImage*	getButtonImage(const Params& p, EFloaterButton e);
	static LLUIImage*	getButtonPressedImage(const Params& p, EFloaterButton e);
	
	/**
	 * @params is_chrome - if floater is Chrome it means that floater will never get focus.
	 * Therefore it can't be closed with 'Ctrl+W'. So the tooltip text of close button( X )
	 * should be 'Close' not 'Close(Ctrl+W)' as for usual floaters.
	 */
	static std::string	getButtonTooltip(const Params& p, EFloaterButton e, bool is_chrome);

	BOOL			offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButton index);
	void			addResizeCtrls();
	void			layoutResizeCtrls();
	void 			addDragHandle();
	void			layoutDragHandle();		// repair layout

	static void		updateActiveFloaterTransparency();
	static void		updateInactiveFloaterTransparency();
	void			updateTransparency(LLView* view, ETypeTransparency transparency_type);

public:
	// Called when floater is opened, passes mKey
	// Public so external views or floaters can watch for this floater opening
	commit_signal_t mOpenSignal;

	// Called when floater is closed, passes app_qitting as LLSD()
	// Public so external views or floaters can watch for this floater closing
	commit_signal_t mCloseSignal;		

	commit_signal_t* mMinimizeSignal;

protected:
	bool			mSaveRect;
	std::string		mRectControl;
	std::string		mPosXControl;
	std::string		mPosYControl;
	std::string		mVisibilityControl;
	std::string		mDocStateControl;
	LLSD			mKey;				// Key used for retrieving instances; set (for now) by LLFLoaterReg

	LLDragHandle*	mDragHandle;
	LLResizeBar*	mResizeBar[4];
	LLResizeHandle*	mResizeHandle[4];

	LLButton*		mButtons[BUTTON_COUNT];
private:
	LLRect			mExpandedRect;
	
	LLUIString		mTitle;
	LLUIString		mShortTitle;
	
	BOOL			mSingleInstance;	// TRUE if there is only ever one instance of the floater
	bool			mReuseInstance;		// true if we want to hide the floater when we close it instead of destroying it
	std::string		mInstanceName;		// Store the instance name so we can remove ourselves from the list
	
	BOOL			mCanTearOff;
	BOOL			mCanMinimize;
	BOOL			mCanClose;
	BOOL			mDragOnLeft;
	BOOL			mResizable;

	LLFloaterEnums::EOpenPositioning	mPositioning;
	LLCoordFloater	mPosition;
	
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

	BOOL			mHasBeenDraggedWhileMinimized;
	S32				mPreviousMinimizedBottom;
	S32				mPreviousMinimizedLeft;
};


/////////////////////////////////////////////////////////////
// LLFloaterView
// Parent of all floating panels

class LLFloaterView : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>{};

protected:
	LLFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void draw();
	/*virtual*/ LLRect getSnapRect() const;
	/*virtual*/ void refresh();

	LLRect			findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor );

	// Given a child of gFloaterView, make sure this view can fit entirely onscreen.
	void			adjustToFitScreen(LLFloater* floater, BOOL allow_partial_outside);

	void			setMinimizePositionVerticalOffset(S32 offset) { mMinimizePositionVOffset = offset; }
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
	void			shiftFloaters(S32 x_offset, S32 y_offset);

	void			hideAllFloaters();
	void			showHiddenFloaters();


	LLFloater* getFrontmost() const;
	LLFloater* getBackmost() const;
	LLFloater* getParentFloater(LLView* viewp) const;
	LLFloater* getFocusedFloater() const;
	void		syncFloaterTabOrder();

	// Returns z order of child provided. 0 is closest, larger numbers
	// are deeper in the screen. If there is no such child, the return
	// value is not defined.
	S32 getZOrder(LLFloater* child);

	void setFloaterSnapView(LLHandle<LLView> snap_view) {mSnapView = snap_view; }

private:
	void hiddenFloaterClosed(LLFloater* floater);

	LLRect				mLastSnapRect;
	LLHandle<LLView>	mSnapView;
	BOOL			mFocusCycleMode;
	S32				mSnapOffsetBottom;
	S32				mSnapOffsetRight;
	S32				mMinimizePositionVOffset;
	typedef std::vector<std::pair<LLHandle<LLFloater>, boost::signals2::connection> > hidden_floaters_t;
	hidden_floaters_t mHiddenFloaters;
};

//
// Globals
//

extern LLFloaterView* gFloaterView;

#endif  // LL_FLOATER_H



