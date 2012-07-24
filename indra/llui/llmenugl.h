/** 
 * @file llmenugl.h
 * @brief Declaration of the opengl based menu system.
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

#ifndef LL_LLMENUGL_H
#define LL_LLMENUGL_H

#include <list>

#include "llstring.h"
#include "v4color.h"
#include "llframetimer.h"

#include "llkeyboard.h"
#include "llfloater.h"
#include "lluistring.h"
#include "llview.h"
#include <boost/function.hpp>

extern S32 MENU_BAR_HEIGHT;
extern S32 MENU_BAR_WIDTH;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemGL
//
// The LLMenuItemGL represents a single menu item in a menu. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemGL : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<std::string>	shortcut;
		Optional<KEY>			jump_key;
		Optional<bool>			use_mac_ctrl,
								allow_key_repeat;

		Ignored					rect,
								left,
								top,
								right,
								bottom,
								width,
								height,
								bottom_delta,
								left_delta;

		Optional<LLUIColor>		enabled_color,
								disabled_color,
								highlight_bg_color,
								highlight_fg_color;


		Params();
	};

protected:
	LLMenuItemGL(const Params&);
	friend class LLUICtrlFactory;
public:
	// LLView overrides
	/*virtual*/ void handleVisibilityChange(BOOL new_visibility);
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleRightMouseUp(S32 x, S32 y, MASK mask);

	// LLUICtrl overrides
	/*virtual*/ void setValue(const LLSD& value);
	/*virtual*/ LLSD getValue() const;

	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);

	LLColor4 getHighlightBgColor() { return mHighlightBackground.get(); }

	void setJumpKey(KEY key);
	KEY getJumpKey() const { return mJumpKey; }
	
	// set the font used by this item.
	void setFont(const LLFontGL* font) { mFont = font; }
	const LLFontGL* getFont() const { return mFont; }

	// returns the height in pixels for the current font.
	virtual U32 getNominalHeight( void ) const;
	
	// Marks item as not needing space for check marks or accelerator keys
	virtual void setBriefItem(BOOL brief);
	virtual BOOL isBriefItem() const;

	virtual BOOL addToAcceleratorList(std::list<LLKeyBinding*> *listp);
	void setAllowKeyRepeat(BOOL allow) { mAllowKeyRepeat = allow; }
	BOOL getAllowKeyRepeat() const { return mAllowKeyRepeat; }

	// change the label
	void setLabel( const LLStringExplicit& label ) { mLabel = label; }	
	std::string getLabel( void ) const { return mLabel.getString(); }
	virtual BOOL setLabelArg( const std::string& key, const LLStringExplicit& text );

	// Get the parent menu for this item
	virtual class LLMenuGL*	getMenu() const;

	// returns the normal width of this control in pixels - this is
	// used for calculating the widest item, as well as for horizontal
	// arrangement.
	virtual U32 getNominalWidth( void ) const;

	// buildDrawLabel() - constructs the string used during the draw()
	// function. This reduces the overall string manipulation, but can
	// lead to visual errors if the state of the object changes
	// without the knowledge of the menu item. For example, if a
	// boolean being watched is changed outside of the menu item's
	// onCommit() function, the draw buffer will not be updated and will
	// reflect the wrong value. If this ever becomes an issue, there
	// are ways to fix this.
	// Returns the enabled state of the item.
	virtual void buildDrawLabel( void );

	// for branching menu items, bring sub menus up to root level of menu hierarchy
	virtual void updateBranchParent( LLView* parentp ){};
	
	virtual void onCommit( void );

	virtual void setHighlight( BOOL highlight );
	virtual BOOL getHighlight() const { return mHighlight; }

	// determine if this represents an active sub-menu
	virtual BOOL isActive( void ) const { return FALSE; }

	// determine if this represents an open sub-menu
	virtual BOOL isOpen( void ) const { return FALSE; }

	virtual void setEnabledSubMenus(BOOL enable){};

	// LLView Functionality
	virtual BOOL handleKeyHere( KEY key, MASK mask );
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleMouseUp( S32 x, S32 y, MASK mask );
	virtual BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
	virtual void draw( void );

	BOOL getHover() const { return mGotHover; }

	void setDrawTextDisabled(BOOL disabled) { mDrawTextDisabled = disabled; }
	BOOL getDrawTextDisabled() const { return mDrawTextDisabled; }

protected:
	void setHover(BOOL hover) { mGotHover = hover; }

	// This function appends the character string representation of
	// the current accelerator key and mask to the provided string.
	void appendAcceleratorString( std::string& st ) const;
		
protected:
	KEY mAcceleratorKey;
	MASK mAcceleratorMask;
	// mLabel contains the actual label specified by the user.
	LLUIString mLabel;

	// The draw labels contain some of the labels that we draw during
	// the draw() routine. This optimizes away some of the string
	// manipulation.
	LLUIString mDrawBoolLabel;
	LLUIString mDrawAccelLabel;
	LLUIString mDrawBranchLabel;

	LLUIColor mEnabledColor;
	LLUIColor mDisabledColor;
	LLUIColor mHighlightBackground;
	LLUIColor mHighlightForeground;

	BOOL mHighlight;
private:
	// Keyboard and mouse variables
	BOOL mAllowKeyRepeat;
	BOOL mGotHover;

	// If true, suppress normal space for check marks on the left and accelerator
	// keys on the right.
	BOOL mBriefItem;

	// Font for this item
	const LLFontGL* mFont;
	BOOL mDrawTextDisabled;

	KEY mJumpKey;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemSeparatorGL
//
// This class represents a separator.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLMenuItemSeparatorGL : public LLMenuItemGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuItemGL::Params>
	{
		Params();
	};
	LLMenuItemSeparatorGL(const LLMenuItemSeparatorGL::Params& p = LLMenuItemSeparatorGL::Params());

	/*virtual*/ void draw( void );
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);

	/*virtual*/ U32 getNominalHeight( void ) const;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemCallGL
//
// The LLMenuItemCallerGL represents a single menu item in a menu that
// calls a user defined callback.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemCallGL : public LLMenuItemGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuItemGL::Params>
	{
		Optional<EnableCallbackParam > on_enable;
		Optional<CommitCallbackParam > on_click;
		Optional<EnableCallbackParam > on_visible;
		Params()
			: on_enable("on_enable"),
			  on_click("on_click"),
			  on_visible("on_visible")
		{}
	};
protected:
	LLMenuItemCallGL(const Params&);
	friend class LLUICtrlFactory;
	void updateEnabled( void );
	void updateVisible( void );

public:
	void initFromParams(const Params& p);
	
	// called to rebuild the draw label
	virtual void buildDrawLabel( void );

	virtual void onCommit( void );

	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	
	//virtual void draw();
	
	boost::signals2::connection setClickCallback( const commit_signal_t::slot_type& cb )
	{
		return setCommitCallback(cb);
	}
	
	boost::signals2::connection setEnableCallback( const enable_signal_t::slot_type& cb )
	{
		return mEnableSignal.connect(cb);
	}
		
private:
	enable_signal_t mEnableSignal;
	enable_signal_t mVisibleSignal;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemCheckGL
//
// The LLMenuItemCheckGL is an extension of the LLMenuItemCallGL
// class, by allowing another method to be specified which determines
// if the menu item should consider itself checked as true or not.  Be
// careful that the provided callback is fast - it needs to be VERY
// EFFICIENT because it may need to be checked a lot.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemCheckGL 
:	public LLMenuItemCallGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuItemCallGL::Params>
	{
		Optional<EnableCallbackParam > on_check;
		Params()
		:	on_check("on_check")
		{}
	};

protected:
	LLMenuItemCheckGL(const Params&);
	friend class LLUICtrlFactory;
public:
	
	void initFromParams(const Params& p);

	virtual void onCommit( void );
	
	virtual void setValue(const LLSD& value);
	virtual LLSD getValue() const;

	// called to rebuild the draw label
	virtual void buildDrawLabel( void );
	
	boost::signals2::connection setCheckCallback( const enable_signal_t::slot_type& cb )
	{
		return mCheckSignal.connect(cb);
	}
	
private:
	enable_signal_t mCheckSignal;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuGL
//
// The Menu class represents a normal rectangular menu somewhere on
// screen. A Menu can have menu items (described above) or sub-menus
// attached to it. Sub-menus are implemented via a specialized
// menu-item type known as a branch. Since it's easy to do wrong, I've
// taken the branch functionality out of public view, and encapsulate
// it in the appendMenu() method.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// child widget registry
struct MenuRegistry : public LLChildRegistry<MenuRegistry>
{};


class LLMenuGL 
:	public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<KEY>					jump_key;
		Optional<bool>					horizontal_layout,
										can_tear_off,
										drop_shadow,
										bg_visible,
										create_jump_keys,
										keep_fixed_size,
										scrollable;
		Optional<U32>					max_scrollable_items;
		Optional<U32>					preferred_width;
		Optional<LLUIColor>				bg_color;
		Optional<S32>					shortcut_pad;

		Params()
		:	jump_key("jump_key", KEY_NONE),
			horizontal_layout("horizontal_layout"),
			can_tear_off("tear_off", false),
			drop_shadow("drop_shadow", true),
			bg_visible("bg_visible", true),
			create_jump_keys("create_jump_keys", false),
			keep_fixed_size("keep_fixed_size", false),
			bg_color("bg_color",  LLUIColorTable::instance().getColor( "MenuDefaultBgColor" )),
			scrollable("scrollable", false), 
			max_scrollable_items("max_scrollable_items", U32_MAX),
			preferred_width("preferred_width", U32_MAX),
			shortcut_pad("shortcut_pad")
		{
			addSynonym(bg_visible, "opaque");
			addSynonym(bg_color, "color");
			addSynonym(can_tear_off, "can_tear_off");
		}
	};

	// my valid children are contained in MenuRegistry
	typedef MenuRegistry child_registry_t;

	void initFromParams(const Params&);

	// textual artwork which menugl-imitators may want to match
	static const std::string BOOLEAN_TRUE_PREFIX;
	static const std::string BRANCH_SUFFIX;
	static const std::string ARROW_UP;
	static const std::string ARROW_DOWN;

	// for scrollable menus
	typedef enum e_scrolling_direction
	{
		SD_UP = 0,
		SD_DOWN = 1,
		SD_BEGIN = 2,
		SD_END = 3
	} EScrollingDirection;

protected:
	LLMenuGL(const LLMenuGL::Params& p);
	friend class LLUICtrlFactory;
	// let branching menu items use my protected traversal methods
	friend class LLMenuItemBranchGL;
public:
	virtual ~LLMenuGL( void );

	void parseChildXML(LLXMLNodePtr child, LLView* parent);

	// LLView Functionality
	/*virtual*/ BOOL handleUnicodeCharHere( llwchar uni_char );
	/*virtual*/ BOOL handleHover( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL handleScrollWheel( S32 x, S32 y, S32 clicks );
	/*virtual*/ void draw( void );
	/*virtual*/ void drawBackground(LLMenuItemGL* itemp, F32 alpha);
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ bool addChild(LLView* view, S32 tab_group = 0);
	/*virtual*/ void removeChild( LLView* ctrl);
	/*virtual*/ BOOL postBuild();

	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);

	LLMenuGL* findChildMenuByName(const std::string& name, BOOL recurse) const;
	
	BOOL clearHoverItem();

	// return the name label
	const std::string& getLabel( void ) const { return mLabel.getString(); }
	void setLabel(const LLStringExplicit& label) { mLabel = label; }

	// background colors
	void setBackgroundColor( const LLUIColor& color ) { mBackgroundColor = color; }
	const LLUIColor& getBackgroundColor() const { return mBackgroundColor; }
	void setBackgroundVisible( BOOL b )	{ mBgVisible = b; }
	void setCanTearOff(BOOL tear_off);

	// add a separator to this menu
	virtual BOOL addSeparator();

	// for branching menu items, bring sub menus up to root level of menu hierarchy
	virtual void updateParent( LLView* parentp );

	// setItemEnabled() - pass the name and the enable flag for a
	// menu item. TRUE will make sure it's enabled, FALSE will disable
	// it.
	void setItemEnabled( const std::string& name, BOOL enable ); 
	
	// propagate message to submenus
	void setEnabledSubMenus(BOOL enable);

	void setItemVisible( const std::string& name, BOOL visible);
	
	// sets the left,bottom corner of menu, useful for popups
	void setLeftAndBottom(S32 left, S32 bottom);

	virtual BOOL handleJumpKey(KEY key);

	virtual BOOL jumpKeysActive();

	virtual BOOL isOpen();

	void needsArrange() { mNeedsArrange = TRUE; }
	// Shape this menu to fit the current state of the children, and
	// adjust the child rects to fit. This is called automatically
	// when you add items. *FIX: We may need to deal with visibility
	// arrangement.
	virtual void arrange( void );
	void arrangeAndClear( void );

	// remove all items on the menu
	void empty( void );

	void			setItemLastSelected(LLMenuItemGL* item);	// must be in menu
	U32				getItemCount();				// number of menu items
	LLMenuItemGL*	getItem(S32 number);		// 0 = first item
	LLMenuItemGL*	getHighlightedItem();				

	LLMenuItemGL*	highlightNextItem(LLMenuItemGL* cur_item, BOOL skip_disabled = TRUE);
	LLMenuItemGL*	highlightPrevItem(LLMenuItemGL* cur_item, BOOL skip_disabled = TRUE);

	void buildDrawLabels();
	void createJumpKeys();

	// Show popup at a specific location, in the spawn_view's coordinate frame
	static void showPopup(LLView* spawning_view, LLMenuGL* menu, S32 x, S32 y);

	// Whether to drop shadow menu bar 
	void setDropShadowed( const BOOL shadowed );

	void setParentMenuItem( LLMenuItemGL* parent_menu_item ) { mParentMenuItem = parent_menu_item->getHandle(); }
	LLMenuItemGL* getParentMenuItem() const { return dynamic_cast<LLMenuItemGL*>(mParentMenuItem.get()); }

	void setTornOff(BOOL torn_off);
	BOOL getTornOff() { return mTornOff; }

	BOOL getCanTearOff() { return mTearOffItem != NULL; }

	KEY getJumpKey() const { return mJumpKey; }
	void setJumpKey(KEY key) { mJumpKey = key; }

	static void setKeyboardMode(BOOL mode) { sKeyboardMode = mode; }
	static BOOL getKeyboardMode() { return sKeyboardMode; }

	S32 getShortcutPad() { return mShortcutPad; }

	bool scrollItems(EScrollingDirection direction);
	BOOL isScrollable() const { return mScrollable; }

	static class LLMenuHolderGL* sMenuContainer;
	
	void resetScrollPositionOnShow(bool reset_scroll_pos) { mResetScrollPositionOnShow = reset_scroll_pos; }
	bool isScrollPositionOnShowReset() { return mResetScrollPositionOnShow; }

protected:
	void createSpilloverBranch();
	void cleanupSpilloverBranch();
	// Add the menu item to this menu.
	virtual BOOL append( LLMenuItemGL* item );

	// add a menu - this will create a cascading menu
	virtual BOOL appendMenu( LLMenuGL* menu );

	// TODO: create accessor methods for these?
	typedef std::list< LLMenuItemGL* > item_list_t;
	item_list_t mItems;
	LLMenuItemGL*mFirstVisibleItem;
	LLMenuItemGL *mArrowUpItem, *mArrowDownItem;

	typedef std::map<KEY, LLMenuItemGL*> navigation_key_map_t;
	navigation_key_map_t mJumpKeys;
	S32				mLastMouseX;
	S32				mLastMouseY;
	S32				mMouseVelX;
	S32				mMouseVelY;
	U32				mMaxScrollableItems;
	U32				mPreferredWidth;
	BOOL			mHorizontalLayout;
	BOOL			mScrollable;
	BOOL			mKeepFixedSize;
	BOOL			mNeedsArrange;

private:


	static LLColor4 sDefaultBackgroundColor;
	static BOOL		sKeyboardMode;

	LLUIColor		mBackgroundColor;
	BOOL			mBgVisible;
	LLHandle<LLView> mParentMenuItem;
	LLUIString		mLabel;
	BOOL mDropShadowed; 	//  Whether to drop shadow 
	bool			mHasSelection;
	LLFrameTimer	mFadeTimer;
	LLTimer			mScrollItemsTimer;
	BOOL			mTornOff;
	class LLMenuItemTearOffGL* mTearOffItem;
	class LLMenuItemBranchGL* mSpilloverBranch;
	LLMenuGL*		mSpilloverMenu;
	KEY				mJumpKey;
	BOOL			mCreateJumpKeys;
	S32				mShortcutPad;
	bool			mResetScrollPositionOnShow;
}; // end class LLMenuGL



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemBranchGL
//
// The LLMenuItemBranchGL represents a menu item that has a
// sub-menu. This is used to make cascading menus.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemBranchGL : public LLMenuItemGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuItemGL::Params>
	{
		Optional<LLMenuGL*>	branch;
	};

protected:
	LLMenuItemBranchGL(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLMenuItemBranchGL();
	
	virtual BOOL handleMouseUp(S32 x, S32 y, MASK mask);

	virtual BOOL handleAcceleratorKey(KEY key, MASK mask);

	// check if we've used these accelerators already
	virtual BOOL addToAcceleratorList(std::list <LLKeyBinding*> *listp);

	// called to rebuild the draw label
	virtual void buildDrawLabel( void );

	virtual void onCommit( void );

	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);

	// set the hover status (called by it's menu) and if the object is
	// active. This is used for behavior transfer.
	virtual void setHighlight( BOOL highlight );

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	virtual BOOL isActive() const;

	virtual BOOL isOpen() const;

	LLMenuGL* getBranch() const { return (LLMenuGL*)mBranchHandle.get(); }

	virtual void updateBranchParent( LLView* parentp );

	// LLView Functionality
	virtual void handleVisibilityChange( BOOL curVisibilityIn );

	virtual void draw();

	virtual void setEnabledSubMenus(BOOL enabled) { if (getBranch()) getBranch()->setEnabledSubMenus(enabled); }

	virtual void openMenu();

	virtual LLView* getChildView(const std::string& name, BOOL recurse = TRUE) const;
	virtual LLView* findChildView(const std::string& name, BOOL recurse = TRUE) const;

private:
	LLHandle<LLView> mBranchHandle;
}; // end class LLMenuItemBranchGL


//-----------------------------------------------------------------------------
// class LLContextMenu
// A context menu
//-----------------------------------------------------------------------------

class LLContextMenu
: public LLMenuGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuGL::Params>
	{
		Params()
		{
			changeDefault(visible, false);
		}
	};

protected:
	LLContextMenu(const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLContextMenu() {}

	// LLView Functionality
	// can't set visibility directly, must call show or hide
	virtual void	setVisible			(BOOL visible);
	
	virtual void	draw				();
	
	virtual void	show				(S32 x, S32 y, LLView* spawning_view = NULL);
	virtual void	hide				();

	virtual BOOL	handleHover			( S32 x, S32 y, MASK mask );
	virtual BOOL	handleRightMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL	handleRightMouseUp	( S32 x, S32 y, MASK mask );

	virtual bool	addChild			(LLView* view, S32 tab_group = 0);

			BOOL	appendContextSubMenu(LLContextMenu *menu);

			LLHandle<LLContextMenu> getHandle() { return getDerivedHandle<LLContextMenu>(); }

			LLView*	getSpawningView() const		{ return mSpawningViewHandle.get(); }
			void	setSpawningView(LLHandle<LLView> spawning_view) { mSpawningViewHandle = spawning_view; }

protected:
	BOOL						mHoveredAnyItem;
	LLMenuItemGL*				mHoverItem;
	LLRootHandle<LLContextMenu>	mHandle;
	LLHandle<LLView>			mSpawningViewHandle;
};



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuBarGL
//
// A menu bar displays menus horizontally.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuBarGL : public LLMenuGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuGL::Params>
	{};
	LLMenuBarGL( const Params& p );
	virtual ~LLMenuBarGL();

	/*virtual*/ BOOL handleAcceleratorKey(KEY key, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask);
	/*virtual*/ BOOL handleJumpKey(KEY key);
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	/*virtual*/ void draw();
	/*virtual*/ BOOL jumpKeysActive();

	// add a vertical separator to this menu
	virtual BOOL addSeparator();

	// LLView Functionality
	virtual BOOL handleHover( S32 x, S32 y, MASK mask );

	// Returns x position of rightmost child, usually Help menu
	S32 getRightmostMenuEdge();

	void resetMenuTrigger() { mAltKeyTrigger = FALSE; }

private:
	// add a menu - this will create a drop down menu.
	virtual BOOL appendMenu( LLMenuGL* menu );
	// rearrange the child rects so they fit the shape of the menu
	// bar.
	virtual void arrange( void );

	void checkMenuTrigger();

	std::list <LLKeyBinding*>	mAccelerators;
	BOOL						mAltKeyTrigger;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuHolderGL
//
// High level view that serves as parent for all menus
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLMenuHolderGL : public LLPanel
{
public:
	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{};
	LLMenuHolderGL(const Params& p);
	virtual ~LLMenuHolderGL() {}

	virtual BOOL hideMenus();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	void setCanHide(BOOL can_hide) { mCanHide = can_hide; }

	// LLView functionality
	virtual void draw();
	virtual BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	virtual BOOL handleRightMouseDown( S32 x, S32 y, MASK mask );

	// Close context menus on right mouse up not handled by menus.
	/*virtual*/ BOOL handleRightMouseUp( S32 x, S32 y, MASK mask );

	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual const LLRect getMenuRect() const { return getLocalRect(); }
	LLView*const getVisibleMenu() const;
	virtual BOOL hasVisibleMenu() const {return getVisibleMenu() != NULL;}

	static void setActivatedItem(LLMenuItemGL* item);

	// Need to detect if mouse-up after context menu spawn has moved.
	// If not, need to keep the menu up.
	static LLCoordGL sContextMenuSpawnPos;

private:
	static LLHandle<LLView> sItemLastSelectedHandle;
	static LLFrameTimer sItemActivationTimer;

	BOOL mCanHide;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLTearOffMenu
//
// Floater that hosts a menu
// https://wiki.lindenlab.com/mediawiki/index.php?title=LLTearOffMenu&oldid=81344
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLTearOffMenu : public LLFloater
{
public:
	static LLTearOffMenu* create(LLMenuGL* menup);
	virtual ~LLTearOffMenu();

	virtual void draw(void);
	virtual void onFocusReceived();
	virtual void onFocusLost();
	virtual BOOL handleUnicodeChar(llwchar uni_char, BOOL called_from_parent);
	virtual BOOL handleKeyHere(KEY key, MASK mask);
	virtual void translate(S32 x, S32 y);

private:
	LLTearOffMenu(LLMenuGL* menup);
	
	void closeTearOff();
	
	LLView*		mOldParent;
	LLMenuGL*	mMenu;
	F32			mTargetHeight;
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLMenuItemTearOffGL
//
// This class represents a separator.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLMenuItemTearOffGL : public LLMenuItemGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuItemGL::Params>
	{};

	LLMenuItemTearOffGL( const Params& );
	
	virtual void onCommit(void);
	virtual void draw(void);
	virtual U32 getNominalHeight() const;

	LLFloater* getParentFloater();
};


// *TODO: this is currently working, so finish implementation
class LLEditMenuHandlerMgr
{
public:
	LLEditMenuHandlerMgr& getInstance() {
		static LLEditMenuHandlerMgr instance;
		return instance;
	}
	virtual ~LLEditMenuHandlerMgr() {}
private:
	LLEditMenuHandlerMgr() {};
};


// *TODO: Eliminate
// For backwards compatability only; generally just use boost::bind
class view_listener_t : public boost::signals2::trackable
{
public:
	virtual bool handleEvent(const LLSD& userdata) = 0;
	virtual ~view_listener_t() {}
	
	static void addEnable(view_listener_t* listener, const std::string& name)
	{
		LLUICtrl::EnableCallbackRegistry::currentRegistrar().add(name, boost::bind(&view_listener_t::handleEvent, listener, _2));
	}
	
	static void addCommit(view_listener_t* listener, const std::string& name)
	{
		LLUICtrl::CommitCallbackRegistry::currentRegistrar().add(name, boost::bind(&view_listener_t::handleEvent, listener, _2));
	}
	
	static void addMenu(view_listener_t* listener, const std::string& name)
	{
		// For now, add to both click and enable registries
		addEnable(listener, name);
		addCommit(listener, name);
	}
};

#endif // LL_LLMENUGL_H
