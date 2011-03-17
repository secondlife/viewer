/** 

 * @file llfloater.cpp
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

#include "linden_common.h"

#include "llfloater.h"

#include "llfocusmgr.h"

#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lldir.h"
#include "lldraghandle.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llkeyboard.h"
#include "llmenugl.h"	// MENU_BAR_HEIGHT
#include "llmodaldialog.h"
#include "lltextbox.h"
#include "llresmgr.h"
#include "llui.h"
#include "llwindow.h"
#include "llstl.h"
#include "llcontrol.h"
#include "lltabcontainer.h"
#include "v2math.h"
#include "lltrans.h"
#include "llhelp.h"
#include "llmultifloater.h"
#include "llsdutil.h"

// use this to control "jumping" behavior when Ctrl-Tabbing
const S32 TABBED_FLOATER_OFFSET = 0;

std::string	LLFloater::sButtonNames[BUTTON_COUNT] = 
{
	"llfloater_close_btn",		//BUTTON_CLOSE
	"llfloater_restore_btn",	//BUTTON_RESTORE
	"llfloater_minimize_btn",	//BUTTON_MINIMIZE
	"llfloater_tear_off_btn",	//BUTTON_TEAR_OFF
	"llfloater_dock_btn",		//BUTTON_DOCK
	"llfloater_help_btn"		//BUTTON_HELP
};

std::string LLFloater::sButtonToolTips[BUTTON_COUNT];

std::string LLFloater::sButtonToolTipsIndex[BUTTON_COUNT]=
{
#ifdef LL_DARWIN
	"BUTTON_CLOSE_DARWIN",	//"Close (Cmd-W)",	//BUTTON_CLOSE
#else
	"BUTTON_CLOSE_WIN",		//"Close (Ctrl-W)",	//BUTTON_CLOSE
#endif
	"BUTTON_RESTORE",		//"Restore",	//BUTTON_RESTORE
	"BUTTON_MINIMIZE",		//"Minimize",	//BUTTON_MINIMIZE
	"BUTTON_TEAR_OFF",		//"Tear Off",	//BUTTON_TEAR_OFF
	"BUTTON_DOCK",
	"BUTTON_HELP"
};

LLFloater::click_callback LLFloater::sButtonCallbacks[BUTTON_COUNT] =
{
	LLFloater::onClickClose,	//BUTTON_CLOSE
	LLFloater::onClickMinimize, //BUTTON_RESTORE
	LLFloater::onClickMinimize, //BUTTON_MINIMIZE
	LLFloater::onClickTearOff,	//BUTTON_TEAR_OFF
	LLFloater::onClickDock,		//BUTTON_DOCK
	LLFloater::onClickHelp		//BUTTON_HELP
};

LLMultiFloater* LLFloater::sHostp = NULL;
BOOL			LLFloater::sQuitting = FALSE; // Flag to prevent storing visibility controls while quitting
LLFloater::handle_map_t	LLFloater::sFloaterMap;

LLFloaterView* gFloaterView = NULL;

/*==========================================================================*|
// DEV-38598: The fundamental problem with this operation is that it can only
// support a subset of LLSD values. While it's plausible to compare two arrays
// lexicographically, what strict ordering can you impose on maps?
// (LLFloaterTOS's current key is an LLSD map.)

// Of course something like this is necessary if you want to build a std::set
// or std::map with LLSD keys. Fortunately we're getting by with other
// container types for now.

//static
bool LLFloater::KeyCompare::compare(const LLSD& a, const LLSD& b)
{
	if (a.type() != b.type())
	{
		//llerrs << "Mismatched LLSD types: (" << a << ") mismatches (" << b << ")" << llendl;
		return false;
	}
	else if (a.isUndefined())
		return false;
	else if (a.isInteger())
		return a.asInteger() < b.asInteger();
	else if (a.isReal())
		return a.asReal() < b.asReal();
	else if (a.isString())
		return a.asString() < b.asString();
	else if (a.isUUID())
		return a.asUUID() < b.asUUID();
	else if (a.isDate())
		return a.asDate() < b.asDate();
	else if (a.isURI())
		return a.asString() < b.asString(); // compare URIs as strings
	else if (a.isBoolean())
		return a.asBoolean() < b.asBoolean();
	else
		return false; // no valid operation for Binary
}
|*==========================================================================*/

bool LLFloater::KeyCompare::equate(const LLSD& a, const LLSD& b)
{
	return llsd_equals(a, b);
}

//************************************

LLFloater::Params::Params()
:	title("title"),
	short_title("short_title"),
	single_instance("single_instance", false),
	auto_tile("auto_tile", false),
	can_resize("can_resize", false),
	can_minimize("can_minimize", true),
	can_close("can_close", true),
	can_drag_on_left("can_drag_on_left", false),
	can_tear_off("can_tear_off", true),
	save_dock_state("save_dock_state", false),
	save_rect("save_rect", false),
	save_visibility("save_visibility", false),
	can_dock("can_dock", false),
	open_centered("open_centered", false),
	header_height("header_height", 0),
	legacy_header_height("legacy_header_height", 0),
	close_image("close_image"),
	restore_image("restore_image"),
	minimize_image("minimize_image"),
	tear_off_image("tear_off_image"),
	dock_image("dock_image"),
	help_image("help_image"),
	close_pressed_image("close_pressed_image"),
	restore_pressed_image("restore_pressed_image"),
	minimize_pressed_image("minimize_pressed_image"),
	tear_off_pressed_image("tear_off_pressed_image"),
	dock_pressed_image("dock_pressed_image"),
	help_pressed_image("help_pressed_image"),
	open_callback("open_callback"),
	close_callback("close_callback")
{
	visible = false;
}


//static 
const LLFloater::Params& LLFloater::getDefaultParams()
{
	return LLUICtrlFactory::getDefaultParams<LLFloater>();
}

//static
void LLFloater::initClass()
{
	// translate tooltips for floater buttons
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		sButtonToolTips[i] = LLTrans::getString( sButtonToolTipsIndex[i] );
	}

	LLControlVariable* ctrl = LLUI::sSettingGroups["config"]->getControl("ActiveFloaterTransparency").get();
	if (ctrl)
	{
		ctrl->getSignal()->connect(boost::bind(&LLFloater::updateActiveFloaterTransparency));
		updateActiveFloaterTransparency();
	}

	ctrl = LLUI::sSettingGroups["config"]->getControl("InactiveFloaterTransparency").get();
	if (ctrl)
	{
		ctrl->getSignal()->connect(boost::bind(&LLFloater::updateInactiveFloaterTransparency));
		updateInactiveFloaterTransparency();
	}

}

// defaults for floater param block pulled from widgets/floater.xml
static LLWidgetNameRegistry::StaticRegistrar sRegisterFloaterParams(&typeid(LLFloater::Params), "floater");

LLFloater::LLFloater(const LLSD& key, const LLFloater::Params& p)
:	LLPanel(),	// intentionally do not pass params here, see initFromParams
 	mDragHandle(NULL),
	mTitle(p.title),
	mShortTitle(p.short_title),
	mSingleInstance(p.single_instance),
	mKey(key),
	mAutoTile(p.auto_tile),
	mCanTearOff(p.can_tear_off),
	mCanMinimize(p.can_minimize),
	mCanClose(p.can_close),
	mDragOnLeft(p.can_drag_on_left),
	mResizable(p.can_resize),
	mMinWidth(p.min_width),
	mMinHeight(p.min_height),
	mHeaderHeight(p.header_height),
	mLegacyHeaderHeight(p.legacy_header_height),
	mMinimized(FALSE),
	mForeground(FALSE),
	mFirstLook(TRUE),
	mButtonScale(1.0f),
	mAutoFocus(TRUE), // automatically take focus when opened
	mCanDock(false),
	mDocked(false),
	mTornOff(false),
	mHasBeenDraggedWhileMinimized(FALSE),
	mPreviousMinimizedBottom(0),
	mPreviousMinimizedLeft(0),
	mMinimizeSignal(NULL)
//	mNotificationContext(NULL)
{
	mHandle.bind(this);
//	mNotificationContext = new LLFloaterNotificationContext(getHandle());

	// Clicks stop here.
	setMouseOpaque(TRUE);
	
	// Floaters always draw their background, unlike every other panel.
	setBackgroundVisible(TRUE);

	// Floaters start not minimized.  When minimized, they save their
	// prior rectangle to be used on restore.
	mExpandedRect.set(0,0,0,0);
	
	memset(mButtonsEnabled, 0, BUTTON_COUNT * sizeof(bool));
	memset(mButtons, 0, BUTTON_COUNT * sizeof(LLButton*));
	
	addDragHandle();
	addResizeCtrls();
	
	initFromParams(p);
	
	initFloater(p);
}

// Note: Floaters constructed from XML call init() twice!
void LLFloater::initFloater(const Params& p)
{
	// Close button.
	if (mCanClose)
	{
		mButtonsEnabled[BUTTON_CLOSE] = TRUE;
	}

	// Help button: '?'
	if ( !mHelpTopic.empty() )
	{
		mButtonsEnabled[BUTTON_HELP] = TRUE;
	}

	// Minimize button only for top draggers
	if ( !mDragOnLeft && mCanMinimize )
	{
		mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
	}

	if(mCanDock)
	{
		mButtonsEnabled[BUTTON_DOCK] = TRUE;
	}

	buildButtons(p);

	// Floaters are created in the invisible state	
	setVisible(FALSE);

	// add self to handle->floater map
	sFloaterMap[mHandle] = this;

	if (!getParent())
	{
		gFloaterView->addChild(this);
	}
}

void LLFloater::addDragHandle()
{
	if (!mDragHandle)
	{
		if (mDragOnLeft)
		{
			LLDragHandleLeft::Params p;
			p.name("drag");
			p.follows.flags(FOLLOWS_ALL);
			p.label(mTitle);
			mDragHandle = LLUICtrlFactory::create<LLDragHandleLeft>(p);
		}
		else // drag on top
		{
			LLDragHandleTop::Params p;
			p.name("Drag Handle");
			p.follows.flags(FOLLOWS_ALL);
			p.label(mTitle);
			mDragHandle = LLUICtrlFactory::create<LLDragHandleTop>(p);
		}
		addChild(mDragHandle);
	}
	layoutDragHandle();
	applyTitle();
}

void LLFloater::layoutDragHandle()
{
	static LLUICachedControl<S32> floater_close_box_size ("UIFloaterCloseBoxSize", 0);
	S32 close_box_size = mCanClose ? floater_close_box_size : 0;
	
	LLRect rect;
	if (mDragOnLeft)
	{
		rect.setLeftTopAndSize(0, 0, DRAG_HANDLE_WIDTH, getRect().getHeight() - LLPANEL_BORDER_WIDTH - close_box_size);
	}
	else // drag on top
	{
		rect = getLocalRect();
	}
	mDragHandle->setShape(rect);
	updateTitleButtons();
}

// static
void LLFloater::updateActiveFloaterTransparency()
{
	sActiveControlTransparency = LLUI::sSettingGroups["config"]->getF32("ActiveFloaterTransparency");
}

// static
void LLFloater::updateInactiveFloaterTransparency()
{
	sInactiveControlTransparency = LLUI::sSettingGroups["config"]->getF32("InactiveFloaterTransparency");
}

void LLFloater::addResizeCtrls()
{	
	// Resize bars (sides)
	LLResizeBar::Params p;
	p.name("resizebar_left");
	p.resizing_view(this);
	p.min_size(mMinWidth);
	p.side(LLResizeBar::LEFT);
	mResizeBar[LLResizeBar::LEFT] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::LEFT] );

	p.name("resizebar_top");
	p.min_size(mMinHeight);
	p.side(LLResizeBar::TOP);

	mResizeBar[LLResizeBar::TOP] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::TOP] );

	p.name("resizebar_right");
	p.min_size(mMinWidth);
	p.side(LLResizeBar::RIGHT);	
	mResizeBar[LLResizeBar::RIGHT] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::RIGHT] );

	p.name("resizebar_bottom");
	p.min_size(mMinHeight);
	p.side(LLResizeBar::BOTTOM);
	mResizeBar[LLResizeBar::BOTTOM] = LLUICtrlFactory::create<LLResizeBar>(p);
	addChild( mResizeBar[LLResizeBar::BOTTOM] );

	// Resize handles (corners)
	LLResizeHandle::Params handle_p;
	// handles must not be mouse-opaque, otherwise they block hover events
	// to other buttons like the close box. JC
	handle_p.mouse_opaque(false);
	handle_p.min_width(mMinWidth);
	handle_p.min_height(mMinHeight);
	handle_p.corner(LLResizeHandle::RIGHT_BOTTOM);
	mResizeHandle[0] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[0]);

	handle_p.corner(LLResizeHandle::RIGHT_TOP);
	mResizeHandle[1] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[1]);
	
	handle_p.corner(LLResizeHandle::LEFT_BOTTOM);
	mResizeHandle[2] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[2]);

	handle_p.corner(LLResizeHandle::LEFT_TOP);
	mResizeHandle[3] = LLUICtrlFactory::create<LLResizeHandle>(handle_p);
	addChild(mResizeHandle[3]);

	layoutResizeCtrls();
}

void LLFloater::layoutResizeCtrls()
{
	LLRect rect;

	// Resize bars (sides)
	const S32 RESIZE_BAR_THICKNESS = 3;
	rect = LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0);
	mResizeBar[LLResizeBar::LEFT]->setRect(rect);

	rect = LLRect( 0, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_BAR_THICKNESS);
	mResizeBar[LLResizeBar::TOP]->setRect(rect);

	rect = LLRect(getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0);
	mResizeBar[LLResizeBar::RIGHT]->setRect(rect);

	rect = LLRect(0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0);
	mResizeBar[LLResizeBar::BOTTOM]->setRect(rect);

	// Resize handles (corners)
	rect = LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, getRect().getWidth(), 0);
	mResizeHandle[0]->setRect(rect);

	rect = LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_HANDLE_HEIGHT);
	mResizeHandle[1]->setRect(rect);
	
	rect = LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 );
	mResizeHandle[2]->setRect(rect);

	rect = LLRect( 0, getRect().getHeight(), RESIZE_HANDLE_WIDTH, getRect().getHeight() - RESIZE_HANDLE_HEIGHT );
	mResizeHandle[3]->setRect(rect);
}

void LLFloater::enableResizeCtrls(bool enable)
{
	for (S32 i = 0; i < 4; ++i)
	{
		mResizeBar[i]->setVisible(enable);
		mResizeBar[i]->setEnabled(enable);

		mResizeHandle[i]->setVisible(enable);
		mResizeHandle[i]->setEnabled(enable);
	}
}

void LLFloater::destroy()
{
	// LLFloaterReg should be synchronized with "dead" floater to avoid returning dead instance before
	// it was deleted via LLMortician::updateClass(). See EXT-8458.
	LLFloaterReg::removeInstance(mInstanceName, mKey);
	die();
}

// virtual
LLFloater::~LLFloater()
{
	LLFloaterReg::removeInstance(mInstanceName, mKey);
	
//	delete mNotificationContext;
//	mNotificationContext = NULL;

	//// am I not hosted by another floater?
	//if (mHostHandle.isDead())
	//{
	//	LLFloaterView* parent = (LLFloaterView*) getParent();

	//	if( parent )
	//	{
	//		parent->removeChild( this );
	//	}
	//}

	// Just in case we might still have focus here, release it.
	releaseFocus();

	// This is important so that floaters with persistent rects (i.e., those
	// created with rect control rather than an LLRect) are restored in their
	// correct, non-minimized positions.
	setMinimized( FALSE );

	sFloaterMap.erase(mHandle);

	delete mDragHandle;
	for (S32 i = 0; i < 4; i++) 
	{
		delete mResizeBar[i];
		delete mResizeHandle[i];
	}

	storeRectControl();
	setVisible(false); // We're not visible if we're destroyed
	storeVisibilityControl();
	storeDockStateControl();

	delete mMinimizeSignal;
}

void LLFloater::storeRectControl()
{
	if( mRectControl.size() > 1 )
	{
		getControlGroup()->setRect( mRectControl, getRect() );
	}
}

void LLFloater::storeVisibilityControl()
{
	if( !sQuitting && mVisibilityControl.size() > 1 )
	{
		getControlGroup()->setBOOL( mVisibilityControl, getVisible() );
	}
}

void LLFloater::storeDockStateControl()
{
	if( !sQuitting && mDocStateControl.size() > 1 )
	{
		getControlGroup()->setBOOL( mDocStateControl, isDocked() );
	}
}

LLRect LLFloater::getSavedRect() const
{
	LLRect rect;

	if (mRectControl.size() > 1)
	{
		rect = getControlGroup()->getRect(mRectControl);
	}

	return rect;
}

bool LLFloater::hasSavedRect() const
{
	return !getSavedRect().isEmpty();
}

// static
std::string LLFloater::getControlName(const std::string& name, const LLSD& key)
{
	std::string ctrl_name = name;

	// Add the key to the control name if appropriate.
	if (key.isString() && !key.asString().empty())
	{
		ctrl_name += "_" + key.asString();
	}

	return ctrl_name;
}

// static
LLControlGroup*	LLFloater::getControlGroup()
{
	// Floater size, position, visibility, etc are saved in per-account settings.
	return LLUI::sSettingGroups["account"];
}

void LLFloater::setVisible( BOOL visible )
{
	LLPanel::setVisible(visible); // calls handleVisibilityChange()
	if( visible && mFirstLook )
	{
		mFirstLook = FALSE;
	}

	if( !visible )
	{
		LLUI::removePopup(this);

		if( gFocusMgr.childHasMouseCapture( this ) )
		{
			gFocusMgr.setMouseCapture(NULL);
		}
	}

	for(handle_set_iter_t dependent_it = mDependents.begin();
		dependent_it != mDependents.end(); )
	{
		LLFloater* floaterp = dependent_it->get();

		if (floaterp)
		{
			floaterp->setVisible(visible);
		}
		++dependent_it;
	}

	storeVisibilityControl();
}

// virtual
void LLFloater::handleVisibilityChange ( BOOL new_visibility )
{
	if (new_visibility)
	{
		if (getHost())
			getHost()->setFloaterFlashing(this, FALSE);
	}
	LLPanel::handleVisibilityChange ( new_visibility );
}

void LLFloater::openFloater(const LLSD& key)
{
	llinfos << "Opening floater " << getName() << llendl;
	mKey = key; // in case we need to open ourselves again
	
	if (getSoundFlags() != SILENT 
	// don't play open sound for hosted (tabbed) windows
		&& !getHost() 
		&& !getFloaterHost()
		&& (!getVisible() || isMinimized()))
	{
		make_ui_sound("UISndWindowOpen");
	}

	//RN: for now, we don't allow rehosting from one multifloater to another
	// just need to fix the bugs
	if (getFloaterHost() != NULL && getHost() == NULL)
	{
		// needs a host
		// only select tabs if window they are hosted in is visible
		getFloaterHost()->addFloater(this, getFloaterHost()->getVisible());
	}

	if (getHost() != NULL)
	{
		getHost()->setMinimized(FALSE);
		getHost()->setVisibleAndFrontmost(mAutoFocus);
		getHost()->showFloater(this);
	}
	else
	{
		setMinimized(FALSE);
		setVisibleAndFrontmost(mAutoFocus);
	}

	mOpenSignal(this, key);
	onOpen(key);

	dirtyRect();
}

void LLFloater::closeFloater(bool app_quitting)
{
	llinfos << "Closing floater " << getName() << llendl;
	if (app_quitting)
	{
		LLFloater::sQuitting = true;
	}
	
	// Always unminimize before trying to close.
	// Most of the time the user will never see this state.
	setMinimized(FALSE);

	if (canClose())
	{
		if (getHost())
		{
			((LLMultiFloater*)getHost())->removeFloater(this);
			gFloaterView->addChild(this);
		}

		if (getSoundFlags() != SILENT
			&& getVisible()
			&& !getHost()
			&& !app_quitting)
		{
			make_ui_sound("UISndWindowClose");
		}

		// now close dependent floater
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				++dependent_it;
				floaterp->closeFloater(app_quitting);
			}
			else
			{
				mDependents.erase(dependent_it++);
			}
		}
		
		cleanupHandles();
		gFocusMgr.clearLastFocusForGroup(this);

		if (hasFocus())
		{
			// Do this early, so UI controls will commit before the
			// window is taken down.
			releaseFocus();

			// give focus to dependee floater if it exists, and we had focus first
			if (isDependent())
			{
				LLFloater* dependee = mDependeeHandle.get();
				if (dependee && !dependee->isDead())
				{
					dependee->setFocus(TRUE);
				}
			}
		}

		dirtyRect();

		// Close callbacks
		onClose(app_quitting);
		mCloseSignal(this, LLSD(app_quitting));
		
		// Hide or Destroy
		if (mSingleInstance)
		{
			// Hide the instance
			if (getHost())
			{
				getHost()->setVisible(FALSE);
			}
			else
			{
				setVisible(FALSE);
			}
		}
		else
		{
			setVisible(FALSE); // hide before destroying (so handleVisibilityChange() gets called)
			destroy();
		}
	}
}

/*virtual*/
void LLFloater::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
	storeRectControl();
}

void LLFloater::releaseFocus()
{
	LLUI::removePopup(this);

	setFocus(FALSE);

	if( gFocusMgr.childHasMouseCapture( this ) )
	{
		gFocusMgr.setMouseCapture(NULL);
	}
}


void LLFloater::setResizeLimits( S32 min_width, S32 min_height )
{
	mMinWidth = min_width;
	mMinHeight = min_height;

	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeBar[i] )
		{
			if (i == LLResizeBar::LEFT || i == LLResizeBar::RIGHT)
			{
				mResizeBar[i]->setResizeLimits( min_width, S32_MAX );
			}
			else
			{
				mResizeBar[i]->setResizeLimits( min_height, S32_MAX );
			}
		}
		if( mResizeHandle[i] )
		{
			mResizeHandle[i]->setResizeLimits( min_width, min_height );
		}
	}
}


void LLFloater::center()
{
	if(getHost())
	{
		// hosted floaters can't move
		return;
	}
	centerWithin(gFloaterView->getRect());
}

LLMultiFloater* LLFloater::getHost()
{ 
	return (LLMultiFloater*)mHostHandle.get(); 
}

void    LLFloater::applySavedVariables()
{
	applyRectControl();
	applyDockState();
}

void LLFloater::applyRectControl()
{
	// first, center on screen if requested	
	if (mOpenCentered)
	{
		center();
	}

	// override center if we have saved rect control
	if (mRectControl.size() > 1)
	{
		const LLRect& rect = getControlGroup()->getRect(mRectControl);
		if (rect.getWidth() > 0 && rect.getHeight() > 0)
		{
			translate( rect.mLeft - getRect().mLeft, rect.mBottom - getRect().mBottom);
			if (mResizable)
			{
				reshape(llmax(mMinWidth, rect.getWidth()), llmax(mMinHeight, rect.getHeight()));
			}
		}
	}
}

void LLFloater::applyDockState()
{
	if (mDocStateControl.size() > 1)
	{
		bool dockState = getControlGroup()->getBOOL(mDocStateControl);
		setDocked(dockState);
	}

}

void LLFloater::applyTitle()
{
	if (!mDragHandle)
	{
		return;
	}

	if (isMinimized() && !mShortTitle.empty())
	{
		mDragHandle->setTitle( mShortTitle );
	}
	else
	{
		mDragHandle->setTitle ( mTitle );
	}

	if (getHost())
	{
		getHost()->updateFloaterTitle(this);	
	}
}

std::string LLFloater::getCurrentTitle() const
{
	return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
}

void LLFloater::setTitle( const std::string& title )
{
	mTitle = title;
	applyTitle();
}

std::string LLFloater::getTitle() const
{
	if (mTitle.empty())
	{
		return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
	}
	else
	{
		return mTitle;
	}
}

void LLFloater::setShortTitle( const std::string& short_title )
{
	mShortTitle = short_title;
	applyTitle();
}

std::string LLFloater::getShortTitle() const
{
	if (mShortTitle.empty())
	{
		return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
	}
	else
	{
		return mShortTitle;
	}
}

BOOL LLFloater::canSnapTo(const LLView* other_view)
{
	if (NULL == other_view)
	{
		llwarns << "other_view is NULL" << llendl;
		return FALSE;
	}

	if (other_view != getParent())
	{
		const LLFloater* other_floaterp = dynamic_cast<const LLFloater*>(other_view);		
		if (other_floaterp && other_floaterp->getSnapTarget() == getHandle() && mDependents.find(other_floaterp->getHandle()) != mDependents.end())
		{
			// this is a dependent that is already snapped to us, so don't snap back to it
			return FALSE;
		}
	}

	return LLPanel::canSnapTo(other_view);
}

void LLFloater::setSnappedTo(const LLView* snap_view)
{
	if (!snap_view || snap_view == getParent())
	{
		clearSnapTarget();
	}
	else
	{
		//RN: assume it's a floater as it must be a sibling to our parent floater
		const LLFloater* floaterp = dynamic_cast<const LLFloater*>(snap_view);
		if (floaterp)
		{
			setSnapTarget(floaterp->getHandle());
		}
	}
}

void LLFloater::handleReshape(const LLRect& new_rect, bool by_user)
{
	const LLRect old_rect = getRect();
	LLView::handleReshape(new_rect, by_user);

	// if not minimized, adjust all snapped dependents to new shape
	if (!isMinimized())
	{
		// gather all snapped dependents
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); ++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			// is a dependent snapped to us?
			if (floaterp && floaterp->getSnapTarget() == getHandle())
			{
				S32 delta_x = 0;
				S32 delta_y = 0;
				// check to see if it snapped to right or top, and move if dependee floater is resizing
				LLRect dependent_rect = floaterp->getRect();
				if (dependent_rect.mLeft - getRect().mLeft >= old_rect.getWidth() || // dependent on my right?
					dependent_rect.mRight == getRect().mLeft + old_rect.getWidth()) // dependent aligned with my right
				{
					// was snapped directly onto right side or aligned with it
					delta_x += new_rect.getWidth() - old_rect.getWidth();
				}
				if (dependent_rect.mBottom - getRect().mBottom >= old_rect.getHeight() ||
					dependent_rect.mTop == getRect().mBottom + old_rect.getHeight())
				{
					// was snapped directly onto top side or aligned with it
					delta_y += new_rect.getHeight() - old_rect.getHeight();
				}

				// take translation of dependee floater into account as well
				delta_x += new_rect.mLeft - old_rect.mLeft;
				delta_y += new_rect.mBottom - old_rect.mBottom;

				dependent_rect.translate(delta_x, delta_y);
				floaterp->setShape(dependent_rect, by_user);
			}
		}
	}
	else
	{
		// If minimized, and origin has changed, set
		// mHasBeenDraggedWhileMinimized to TRUE
		if ((new_rect.mLeft != old_rect.mLeft) ||
			(new_rect.mBottom != old_rect.mBottom))
		{
			mHasBeenDraggedWhileMinimized = TRUE;
		}
	}
}

void LLFloater::setMinimized(BOOL minimize)
{
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	static LLUICachedControl<S32> minimized_width ("UIMinimizedWidth", 0);

	if (minimize == mMinimized) return;

	if(mMinimizeSignal)
	{
		(*mMinimizeSignal)(this, LLSD(minimize));
	}

	if (minimize)
	{
		// minimized flag should be turned on before release focus
		mMinimized = TRUE;

		mExpandedRect = getRect();

		// If the floater has been dragged while minimized in the
		// past, then locate it at its previous minimized location.
		// Otherwise, ask the view for a minimize position.
		if (mHasBeenDraggedWhileMinimized)
		{
			setOrigin(mPreviousMinimizedLeft, mPreviousMinimizedBottom);
		}
		else
		{
			S32 left, bottom;
			gFloaterView->getMinimizePosition(&left, &bottom);
			setOrigin( left, bottom );
		}

		if (mButtonsEnabled[BUTTON_MINIMIZE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = FALSE;
			mButtonsEnabled[BUTTON_RESTORE] = TRUE;
		}

		if (mDragHandle)
		{
			mDragHandle->setVisible(TRUE);
		}
		setBorderVisible(TRUE);

		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end();
			++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				if (floaterp->isMinimizeable())
				{
					floaterp->setMinimized(TRUE);
				}
				else if (!floaterp->isMinimized())
				{
					floaterp->setVisible(FALSE);
				}
			}
		}

		// Lose keyboard focus when minimized
		releaseFocus();

		for (S32 i = 0; i < 4; i++)
		{
			if (mResizeBar[i] != NULL)
			{
				mResizeBar[i]->setEnabled(FALSE);
			}
			if (mResizeHandle[i] != NULL)
			{
				mResizeHandle[i]->setEnabled(FALSE);
			}
		}
		
		// Reshape *after* setting mMinimized
		reshape( minimized_width, floater_header_size, TRUE);
	}
	else
	{
		// If this window has been dragged while minimized (at any time),
		// remember its position for the next time it's minimized.
		if (mHasBeenDraggedWhileMinimized)
		{
			const LLRect& currentRect = getRect();
			mPreviousMinimizedLeft = currentRect.mLeft;
			mPreviousMinimizedBottom = currentRect.mBottom;
		}

		setOrigin( mExpandedRect.mLeft, mExpandedRect.mBottom );

		if (mButtonsEnabled[BUTTON_RESTORE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
			mButtonsEnabled[BUTTON_RESTORE] = FALSE;
		}

		// show dependent floater
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end();
			++dependent_it)
		{
			LLFloater* floaterp = dependent_it->get();
			if (floaterp)
			{
				floaterp->setMinimized(FALSE);
				floaterp->setVisible(TRUE);
			}
		}

		for (S32 i = 0; i < 4; i++)
		{
			if (mResizeBar[i] != NULL)
			{
				mResizeBar[i]->setEnabled(isResizable());
			}
			if (mResizeHandle[i] != NULL)
			{
				mResizeHandle[i]->setEnabled(isResizable());
			}
		}
		
		mMinimized = FALSE;

		// Reshape *after* setting mMinimized
		reshape( mExpandedRect.getWidth(), mExpandedRect.getHeight(), TRUE );
	}

	make_ui_sound("UISndWindowClose");
	updateTitleButtons();
	applyTitle ();
}

void LLFloater::setFocus( BOOL b )
{
	if (b && getIsChrome())
	{
		return;
	}
	LLUICtrl* last_focus = gFocusMgr.getLastFocusForGroup(this);
	// a descendent already has focus
	BOOL child_had_focus = hasFocus();

	// give focus to first valid descendent
	LLPanel::setFocus(b);

	if (b)
	{
		// only push focused floaters to front of stack if not in midst of ctrl-tab cycle
		if (!getHost() && !((LLFloaterView*)getParent())->getCycleMode())
		{
			if (!isFrontmost())
			{
				setFrontmost();
			}
		}

		// when getting focus, delegate to last descendent which had focus
		if (last_focus && !child_had_focus && 
			last_focus->isInEnabledChain() &&
			last_focus->isInVisibleChain())
		{
			// *FIX: should handle case where focus doesn't stick
			last_focus->setFocus(TRUE);
		}
	}
	updateTransparency(b ? TT_ACTIVE : TT_INACTIVE);
}

// virtual
void LLFloater::setRect(const LLRect &rect)
{
	LLPanel::setRect(rect);
	layoutDragHandle();
	layoutResizeCtrls();
}

// virtual
void LLFloater::setIsChrome(BOOL is_chrome)
{
	// chrome floaters don't take focus at all
	if (is_chrome)
	{
		// remove focus if we're changing to chrome
		setFocus(FALSE);
		// can't Ctrl-Tab to "chrome" floaters
		setFocusRoot(FALSE);
		mButtons[BUTTON_CLOSE]->setToolTip(LLStringExplicit(getButtonTooltip(Params(), BUTTON_CLOSE, is_chrome)));
	}
	
	// no titles displayed on "chrome" floaters
	if (mDragHandle)
		mDragHandle->setTitleVisible(!is_chrome);
	
	LLPanel::setIsChrome(is_chrome);
}

void LLFloater::setTitleVisible(bool visible)
{
	if (mDragHandle)
		mDragHandle->setTitleVisible(visible);
}

// Change the draw style to account for the foreground state.
void LLFloater::setForeground(BOOL front)
{
	if (front != mForeground)
	{
		mForeground = front;
		if (mDragHandle)
			mDragHandle->setForeground( front );

		if (!front)
		{
			releaseFocus();
		}

		setBackgroundOpaque( front ); 
	}
}

void LLFloater::cleanupHandles()
{
	// remove handles to non-existent dependents
	for(handle_set_iter_t dependent_it = mDependents.begin();
		dependent_it != mDependents.end(); )
	{
		LLFloater* floaterp = dependent_it->get();
		if (!floaterp)
		{
			mDependents.erase(dependent_it++);
		}
		else
		{
			++dependent_it;
		}
	}
}

void LLFloater::setHost(LLMultiFloater* host)
{
	if (mHostHandle.isDead() && host)
	{
		// make buttons smaller for hosted windows to differentiate from parent
		mButtonScale = 0.9f;

		// add tear off button
		if (mCanTearOff)
		{
			mButtonsEnabled[BUTTON_TEAR_OFF] = TRUE;
		}
	}
	else if (!mHostHandle.isDead() && !host)
	{
		mButtonScale = 1.f;
		//mButtonsEnabled[BUTTON_TEAR_OFF] = FALSE;
	}
	updateTitleButtons();
	if (host)
	{
		mHostHandle = host->getHandle();
		mLastHostHandle = host->getHandle();
	}
	else
	{
		mHostHandle.markDead();
	}
}

void LLFloater::moveResizeHandlesToFront()
{
	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeBar[i] )
		{
			sendChildToFront(mResizeBar[i]);
		}
	}

	for( S32 i = 0; i < 4; i++ )
	{
		if( mResizeHandle[i] )
		{
			sendChildToFront(mResizeHandle[i]);
		}
	}
}

BOOL LLFloater::isFrontmost()
{
	return gFloaterView && gFloaterView->getFrontmost() == this && getVisible();
}

void LLFloater::addDependentFloater(LLFloater* floaterp, BOOL reposition)
{
	mDependents.insert(floaterp->getHandle());
	floaterp->mDependeeHandle = getHandle();

	if (reposition)
	{
		floaterp->setRect(gFloaterView->findNeighboringPosition(this, floaterp));
		floaterp->setSnapTarget(getHandle());
	}
	gFloaterView->adjustToFitScreen(floaterp, FALSE);
	if (floaterp->isFrontmost())
	{
		// make sure to bring self and sibling floaters to front
		gFloaterView->bringToFront(floaterp);
	}
}

void LLFloater::addDependentFloater(LLHandle<LLFloater> dependent, BOOL reposition)
{
	LLFloater* dependent_floaterp = dependent.get();
	if(dependent_floaterp)
	{
		addDependentFloater(dependent_floaterp, reposition);
	}
}

void LLFloater::removeDependentFloater(LLFloater* floaterp)
{
	mDependents.erase(floaterp->getHandle());
	floaterp->mDependeeHandle = LLHandle<LLFloater>();
}

BOOL LLFloater::offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButton index)
{
	if( mButtonsEnabled[index] )
	{
		LLButton* my_butt = mButtons[index];
		S32 local_x = x - my_butt->getRect().mLeft;
		S32 local_y = y - my_butt->getRect().mBottom;

		if (
			my_butt->pointInView(local_x, local_y) &&
			my_butt->handleMouseDown(local_x, local_y, mask))
		{
			// the button handled it
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLFloater::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	LLPanel::handleScrollWheel(x,y,clicks);
	return TRUE;//always
}

// virtual
BOOL LLFloater::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if( mMinimized )
	{
		// Offer the click to titlebar buttons.
		// Note: this block and the offerClickToButton helper method can be removed
		// because the parent container will handle it for us but we'll keep it here
		// for safety until after reworking the panel code to manage hidden children.
		if(offerClickToButton(x, y, mask, BUTTON_CLOSE)) return TRUE;
		if(offerClickToButton(x, y, mask, BUTTON_RESTORE)) return TRUE;
		if(offerClickToButton(x, y, mask, BUTTON_TEAR_OFF)) return TRUE;

		// Otherwise pass to drag handle for movement
		return mDragHandle->handleMouseDown(x, y, mask);
	}
	else
	{
		bringToFront( x, y );
		return LLPanel::handleMouseDown( x, y, mask );
	}
}

// virtual
BOOL LLFloater::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL was_minimized = mMinimized;
	bringToFront( x, y );
	return was_minimized || LLPanel::handleRightMouseDown( x, y, mask );
}

BOOL LLFloater::handleMiddleMouseDown(S32 x, S32 y, MASK mask)
{
	bringToFront( x, y );
	return LLPanel::handleMiddleMouseDown( x, y, mask );
}


// virtual
BOOL LLFloater::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL was_minimized = mMinimized;
	setMinimized(FALSE);
	return was_minimized || LLPanel::handleDoubleClick(x, y, mask);
}

void LLFloater::bringToFront( S32 x, S32 y )
{
	if (getVisible() && pointInView(x, y))
	{
		LLMultiFloater* hostp = getHost();
		if (hostp)
		{
			hostp->showFloater(this);
		}
		else
		{
			LLFloaterView* parent = (LLFloaterView*) getParent();
			if (parent)
			{
				parent->bringToFront( this );
			}
		}
	}
}


// virtual
void LLFloater::setVisibleAndFrontmost(BOOL take_focus)
{
	setVisible(TRUE);
	setFrontmost(take_focus);
}

void LLFloater::setFrontmost(BOOL take_focus)
{
	LLMultiFloater* hostp = getHost();
	if (hostp)
	{
		// this will bring the host floater to the front and select
		// the appropriate panel
		hostp->showFloater(this);
	}
	else
	{
		// there are more than one floater view
		// so we need to query our parent directly
		((LLFloaterView*)getParent())->bringToFront(this, take_focus);

		// Make sure to set the appropriate transparency type (STORM-732).
		updateTransparency(hasFocus() || getIsChrome() ? TT_ACTIVE : TT_INACTIVE);
	}
}

void LLFloater::setCanDock(bool b)
{
	if(b != mCanDock)
	{
		mCanDock = b;
		if(mCanDock)
		{
			mButtonsEnabled[BUTTON_DOCK] = !mDocked;
		}
		else
		{
			mButtonsEnabled[BUTTON_DOCK] = FALSE;
		}
	}
	updateTitleButtons();
}

void LLFloater::setDocked(bool docked, bool pop_on_undock)
{
	if(docked != mDocked && mCanDock)
	{
		mDocked = docked;
		mButtonsEnabled[BUTTON_DOCK] = !mDocked;
		updateTitleButtons();

		storeDockStateControl();
	}
	
}

// static
void LLFloater::onClickMinimize(LLFloater* self)
{
	if (!self)
		return;
	self->setMinimized( !self->isMinimized() );
}

void LLFloater::onClickTearOff(LLFloater* self)
{
	if (!self)
		return;
	S32 floater_header_size = self->mHeaderHeight;
	LLMultiFloater* host_floater = self->getHost();
	if (host_floater) //Tear off
	{
		LLRect new_rect;
		host_floater->removeFloater(self);
		// reparent to floater view
		gFloaterView->addChild(self);

		self->openFloater(self->getKey());
		
		// only force position for floaters that don't have that data saved
		if (self->mRectControl.size() <= 1)
		{
			new_rect.setLeftTopAndSize(host_floater->getRect().mLeft + 5, host_floater->getRect().mTop - floater_header_size - 5, self->getRect().getWidth(), self->getRect().getHeight());
			self->setRect(new_rect);
		}
		gFloaterView->adjustToFitScreen(self, FALSE);
		// give focus to new window to keep continuity for the user
		self->setFocus(TRUE);
		self->setTornOff(true);
	}
	else  //Attach to parent.
	{
		LLMultiFloater* new_host = (LLMultiFloater*)self->mLastHostHandle.get();
		if (new_host)
		{
			self->setMinimized(FALSE); // to reenable minimize button if it was minimized
			new_host->showFloater(self);
			// make sure host is visible
			new_host->openFloater(new_host->getKey());
		}
		self->setTornOff(false);
	}
	self->updateTitleButtons();
}

// static
void LLFloater::onClickDock(LLFloater* self)
{
	if(self && self->mCanDock)
	{
		self->setDocked(!self->mDocked, true);
	}
}

// static
void LLFloater::onClickHelp( LLFloater* self )
{
	if (self && LLUI::sHelpImpl)
	{
		// find the current help context for this floater
		std::string help_topic;
		if (self->findHelpTopic(help_topic))
		{
			LLUI::sHelpImpl->showTopic(help_topic);
		}
	}
}

// static 
LLFloater* LLFloater::getClosableFloaterFromFocus()
{
	LLFloater* focused_floater = NULL;

	handle_map_iter_t iter;
	for(iter = sFloaterMap.begin(); iter != sFloaterMap.end(); ++iter)
	{
		focused_floater = iter->second;
		if (focused_floater->hasFocus())
		{
			break;
		}
	}

	if (iter == sFloaterMap.end())
	{
		// nothing found, return
		return NULL;
	}

	// The focused floater may not be closable,
	// Find and close a parental floater that is closeable, if any.
	LLFloater* prev_floater = NULL;
	for(LLFloater* floater_to_close = focused_floater;
		NULL != floater_to_close; 
		floater_to_close = gFloaterView->getParentFloater(floater_to_close))
	{
		if(floater_to_close->isCloseable())
		{
			return floater_to_close;
		}

		// If floater has as parent root view
		// gFloaterView->getParentFloater(floater_to_close) returns
		// the same floater_to_close, so we need to check this.
		if (prev_floater == floater_to_close) {
			break;
		}
		prev_floater = floater_to_close;
	}

	return NULL;
}

// static
void LLFloater::closeFocusedFloater()
{
	LLFloater* floater_to_close = LLFloater::getClosableFloaterFromFocus();
	if(floater_to_close)
	{
		floater_to_close->closeFloater();
	}

	// if nothing took focus after closing focused floater
	// give it to next floater (to allow closing multiple windows via keyboard in rapid succession)
	if (gFocusMgr.getKeyboardFocus() == NULL)
	{
		// HACK: use gFloaterView directly in case we are using Ctrl-W to close snapshot window
		// which sits in gSnapshotFloaterView, and needs to pass focus on to normal floater view
		gFloaterView->focusFrontFloater();
	}
}


// static
void LLFloater::onClickClose( LLFloater* self )
{
	if (!self)
		return;
	self->onClickCloseBtn();
}

void	LLFloater::onClickCloseBtn()
{
	closeFloater(false);
}


// virtual
void LLFloater::draw()
{
	const F32 alpha = getCurrentTransparency();

	// draw background
	if( isBackgroundVisible() )
	{
		drawShadow(this);

		S32 left = LLPANEL_BORDER_WIDTH;
		S32 top = getRect().getHeight() - LLPANEL_BORDER_WIDTH;
		S32 right = getRect().getWidth() - LLPANEL_BORDER_WIDTH;
		S32 bottom = LLPANEL_BORDER_WIDTH;

		LLUIImage* image = NULL;
		LLColor4 color;
		LLColor4 overlay_color;
		if (isBackgroundOpaque())
		{
			// NOTE: image may not be set
			image = getBackgroundImage();
			color = getBackgroundColor();
			overlay_color = getBackgroundImageOverlay();
		}
		else
		{
			image = getTransparentImage();
			color = getTransparentColor();
			overlay_color = getTransparentImageOverlay();
		}

		if (image)
		{
			// We're using images for this floater's backgrounds
			image->draw(getLocalRect(), overlay_color % alpha);
		}
		else
		{
			// We're not using images, use old-school flat colors
			gl_rect_2d( left, top, right, bottom, color % alpha );

			// draw highlight on title bar to indicate focus.  RDW
			if(hasFocus() 
				&& !getIsChrome() 
				&& !getCurrentTitle().empty())
			{
				static LLUIColor titlebar_focus_color = LLUIColorTable::instance().getColor("TitleBarFocusColor");
				
				const LLFontGL* font = LLFontGL::getFontSansSerif();
				LLRect r = getRect();
				gl_rect_2d_offset_local(0, r.getHeight(), r.getWidth(), r.getHeight() - (S32)font->getLineHeight() - 1, 
					titlebar_focus_color % alpha, 0, TRUE);
			}
		}
	}

	LLPanel::updateDefaultBtn();

	if( getDefaultButton() )
	{
		if (hasFocus() && getDefaultButton()->getEnabled())
		{
			LLFocusableElement* focus_ctrl = gFocusMgr.getKeyboardFocus();
			// is this button a direct descendent and not a nested widget (e.g. checkbox)?
			BOOL focus_is_child_button = dynamic_cast<LLButton*>(focus_ctrl) != NULL && dynamic_cast<LLButton*>(focus_ctrl)->getParent() == this;
			// only enable default button when current focus is not a button
			getDefaultButton()->setBorderEnabled(!focus_is_child_button);
		}
		else
		{
			getDefaultButton()->setBorderEnabled(FALSE);
		}
	}
	if (isMinimized())
	{
		for (S32 i = 0; i < BUTTON_COUNT; i++)
		{
			drawChild(mButtons[i]);
		}
		drawChild(mDragHandle);
	}
	else
	{
		// don't call LLPanel::draw() since we've implemented custom background rendering
		LLView::draw();
	}

	// update tearoff button for torn off floaters
	// when last host goes away
	if (mCanTearOff && !getHost())
	{
		LLFloater* old_host = mLastHostHandle.get();
		if (!old_host)
		{
			setCanTearOff(FALSE);
		}
	}
}

void	LLFloater::drawShadow(LLPanel* panel)
{
	S32 left = LLPANEL_BORDER_WIDTH;
	S32 top = panel->getRect().getHeight() - LLPANEL_BORDER_WIDTH;
	S32 right = panel->getRect().getWidth() - LLPANEL_BORDER_WIDTH;
	S32 bottom = LLPANEL_BORDER_WIDTH;

	static LLUICachedControl<S32> shadow_offset_S32 ("DropShadowFloater", 0);
	static LLUIColor shadow_color_cached = LLUIColorTable::instance().getColor("ColorDropShadow");
	LLColor4 shadow_color = shadow_color_cached;
	F32 shadow_offset = (F32)shadow_offset_S32;

	if (!panel->isBackgroundOpaque())
	{
		shadow_offset *= 0.2f;
		shadow_color.mV[VALPHA] *= 0.5f;
	}
	gl_drop_shadow(left, top, right, bottom, 
		shadow_color % getCurrentTransparency(),
		llround(shadow_offset));
}

void LLFloater::updateTransparency(LLView* view, ETypeTransparency transparency_type)
{
	child_list_t children = *view->getChildList();
	child_list_t::iterator it = children.begin();

	LLUICtrl* ctrl = dynamic_cast<LLUICtrl*>(view);
	if (ctrl)
	{
		ctrl->setTransparencyType(transparency_type);
	}

	for(; it != children.end(); ++it)
	{
		updateTransparency(*it, transparency_type);
	}
}

void LLFloater::updateTransparency(ETypeTransparency transparency_type)
{
	updateTransparency(this, transparency_type);
}

void	LLFloater::setCanMinimize(BOOL can_minimize)
{
	// if removing minimize/restore button programmatically,
	// go ahead and unminimize floater
	mCanMinimize = can_minimize;
	if (!can_minimize)
	{
		setMinimized(FALSE);
	}

	mButtonsEnabled[BUTTON_MINIMIZE] = can_minimize && !isMinimized();
	mButtonsEnabled[BUTTON_RESTORE]  = can_minimize &&  isMinimized();

	updateTitleButtons();
}

void	LLFloater::setCanClose(BOOL can_close)
{
	mCanClose = can_close;
	mButtonsEnabled[BUTTON_CLOSE] = can_close;

	updateTitleButtons();
}

void	LLFloater::setCanTearOff(BOOL can_tear_off)
{
	mCanTearOff = can_tear_off;
	mButtonsEnabled[BUTTON_TEAR_OFF] = mCanTearOff && !mHostHandle.isDead();

	updateTitleButtons();
}


void LLFloater::setCanResize(BOOL can_resize)
{
	mResizable = can_resize;
	enableResizeCtrls(can_resize);
}

void LLFloater::setCanDrag(BOOL can_drag)
{
	// if we delete drag handle, we no longer have access to the floater's title
	// so just enable/disable it
	if (!can_drag && mDragHandle->getEnabled())
	{
		mDragHandle->setEnabled(FALSE);
	}
	else if (can_drag && !mDragHandle->getEnabled())
	{
		mDragHandle->setEnabled(TRUE);
	}
}

void LLFloater::updateTitleButtons()
{
	static LLUICachedControl<S32> floater_close_box_size ("UIFloaterCloseBoxSize", 0);
	static LLUICachedControl<S32> close_box_from_top ("UICloseBoxFromTop", 0);
	LLRect buttons_rect;
	S32 button_count = 0;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		if (!mButtons[i])
		{
			continue;
		}

		bool enabled = mButtonsEnabled[i];
		if (i == BUTTON_HELP)
		{
			// don't show the help button if the floater is minimized
			// or if it is a docked tear-off floater
			if (isMinimized() || (mButtonsEnabled[BUTTON_TEAR_OFF] && ! mTornOff))
			{
				enabled = false;
			}
		}
		if (i == BUTTON_CLOSE && mButtonScale != 1.f)
		{
			//*HACK: always render close button for hosted floaters so
			//that users don't accidentally hit the button when
			//closing multiple windows in the chatterbox
			enabled = true;
		}

		mButtons[i]->setEnabled(enabled);

		if (enabled)
		{
			button_count++;

			LLRect btn_rect;
			if (mDragOnLeft)
			{
				btn_rect.setLeftTopAndSize(
					LLPANEL_BORDER_WIDTH,
					getRect().getHeight() - close_box_from_top - (floater_close_box_size + 1) * button_count,
					llround((F32)floater_close_box_size * mButtonScale),
					llround((F32)floater_close_box_size * mButtonScale));
			}
			else
			{
				btn_rect.setLeftTopAndSize(
					getRect().getWidth() - LLPANEL_BORDER_WIDTH - (floater_close_box_size + 1) * button_count,
					getRect().getHeight() - close_box_from_top,
					llround((F32)floater_close_box_size * mButtonScale),
					llround((F32)floater_close_box_size * mButtonScale));
			}

			// first time here, init 'buttons_rect'
			if(1 == button_count)
			{
				buttons_rect = btn_rect;
			}
			else
			{
				// if mDragOnLeft=true then buttons are on top-left side vertically aligned
				// title is not displayed in this case, calculating 'buttons_rect' for future use
				mDragOnLeft ? buttons_rect.mBottom -= btn_rect.mBottom : 
					buttons_rect.mLeft = btn_rect.mLeft;
			}
			mButtons[i]->setRect(btn_rect);
			mButtons[i]->setVisible(TRUE);
			// the restore button should have a tab stop so that it takes action when you Ctrl-Tab to a minimized floater
			mButtons[i]->setTabStop(i == BUTTON_RESTORE);
		}
		else
		{
			mButtons[i]->setVisible(FALSE);
		}
	}
	if (mDragHandle)
	{
		localRectToOtherView(buttons_rect, &buttons_rect, mDragHandle);
		mDragHandle->setButtonsRect(buttons_rect);
	}
}

void LLFloater::buildButtons(const Params& floater_params)
{
	static LLUICachedControl<S32> floater_close_box_size ("UIFloaterCloseBoxSize", 0);
	static LLUICachedControl<S32> close_box_from_top ("UICloseBoxFromTop", 0);
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		if (mButtons[i])
		{
			removeChild(mButtons[i]);
			delete mButtons[i];
			mButtons[i] = NULL;
		}
		
		LLRect btn_rect;
		if (mDragOnLeft)
		{
			btn_rect.setLeftTopAndSize(
				LLPANEL_BORDER_WIDTH,
				getRect().getHeight() - close_box_from_top - (floater_close_box_size + 1) * (i + 1),
				llround(floater_close_box_size * mButtonScale),
				llround(floater_close_box_size * mButtonScale));
		}
		else
		{
			btn_rect.setLeftTopAndSize(
				getRect().getWidth() - LLPANEL_BORDER_WIDTH - (floater_close_box_size + 1) * (i + 1),
				getRect().getHeight() - close_box_from_top,
				llround(floater_close_box_size * mButtonScale),
				llround(floater_close_box_size * mButtonScale));
		}

		LLButton::Params p;
		p.name(sButtonNames[i]);
		p.rect(btn_rect);
		p.image_unselected = getButtonImage(floater_params, (EFloaterButton)i);
		// Selected, no matter if hovered or not, is "pressed"
		LLUIImage* pressed_image = getButtonPressedImage(floater_params, (EFloaterButton)i);
		p.image_selected = pressed_image;
		p.image_hover_selected = pressed_image;
		// Use a glow effect when the user hovers over the button
		// These icons are really small, need glow amount increased
		p.hover_glow_amount( 0.33f );
		p.click_callback.function(boost::bind(sButtonCallbacks[i], this));
		p.tab_stop(false);
		p.follows.flags(FOLLOWS_TOP|FOLLOWS_RIGHT);
		p.tool_tip = getButtonTooltip(floater_params, (EFloaterButton)i, getIsChrome());
		p.scale_image(true);
		p.chrome(true);

		LLButton* buttonp = LLUICtrlFactory::create<LLButton>(p);
		addChild(buttonp);
		mButtons[i] = buttonp;
	}

	updateTitleButtons();
}

// static
LLUIImage* LLFloater::getButtonImage(const Params& p, EFloaterButton e)
{
	switch(e)
	{
		default:
		case BUTTON_CLOSE:
			return p.close_image;
		case BUTTON_RESTORE:
			return p.restore_image;
		case BUTTON_MINIMIZE:
			return p.minimize_image;
		case BUTTON_TEAR_OFF:
			return p.tear_off_image;
		case BUTTON_DOCK:
			return p.dock_image;
		case BUTTON_HELP:
			return p.help_image;
	}
}

// static
LLUIImage* LLFloater::getButtonPressedImage(const Params& p, EFloaterButton e)
{
	switch(e)
	{
		default:
		case BUTTON_CLOSE:
			return p.close_pressed_image;
		case BUTTON_RESTORE:
			return p.restore_pressed_image;
		case BUTTON_MINIMIZE:
			return p.minimize_pressed_image;
		case BUTTON_TEAR_OFF:
			return p.tear_off_pressed_image;
		case BUTTON_DOCK:
			return p.dock_pressed_image;
		case BUTTON_HELP:
			return p.help_pressed_image;
	}
}

// static
std::string LLFloater::getButtonTooltip(const Params& p, EFloaterButton e, bool is_chrome)
{
	// EXT-4081 (Lag Meter: Ctrl+W does not close floater)
	// If floater is chrome set 'Close' text for close button's tooltip
	if(is_chrome && BUTTON_CLOSE == e)
	{
		static std::string close_tooltip_chrome = LLTrans::getString("BUTTON_CLOSE_CHROME");
		return close_tooltip_chrome;
	}
	// TODO: per-floater localizable tooltips set in XML
	return sButtonToolTips[e];
}

/////////////////////////////////////////////////////
// LLFloaterView

static LLDefaultChildRegistry::Register<LLFloaterView> r("floater_view");

LLFloaterView::LLFloaterView (const Params& p)
:	LLUICtrl (p),

	mFocusCycleMode(FALSE),
	mMinimizePositionVOffset(0),
	mSnapOffsetBottom(0),
	mSnapOffsetRight(0)
{
}

// By default, adjust vertical.
void LLFloaterView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	reshapeFloater(width, height, called_from_parent, ADJUST_VERTICAL_YES);
}

// When reshaping this view, make the floaters follow their closest edge.
void LLFloaterView::reshapeFloater(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical)
{
	S32 old_width = getRect().getWidth();
	S32 old_height = getRect().getHeight();

	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		LLFloater* floaterp = (LLFloater*)viewp;
		if (floaterp->isDependent())
		{
			// dependents use same follow flags as their "dependee"
			continue;
		}

		// Make if follow the edge it is closest to
		U32 follow_flags = 0x0;

		if (floaterp->isMinimized())
		{
			follow_flags |= (FOLLOWS_LEFT | FOLLOWS_TOP);
		}
		else
		{
			LLRect r = floaterp->getRect();

			// Compute absolute distance from each edge of screen
			S32 left_offset = llabs(r.mLeft - 0);
			S32 right_offset = llabs(old_width - r.mRight);

			S32 top_offset = llabs(old_height - r.mTop);
			S32 bottom_offset = llabs(r.mBottom - 0);


			if (left_offset < right_offset)
			{
				follow_flags |= FOLLOWS_LEFT;
			}
			else
			{
				follow_flags |= FOLLOWS_RIGHT;
			}

			// "No vertical adjustment" usually means that the bottom of the view
			// has been pushed up or down.  Hence we want the floaters to follow
			// the top.
			if (!adjust_vertical)
			{
				follow_flags |= FOLLOWS_TOP;
			}
			else if (top_offset < bottom_offset)
			{
				follow_flags |= FOLLOWS_TOP;
			}
			else
			{
				follow_flags |= FOLLOWS_BOTTOM;
			}
		}

		floaterp->setFollows(follow_flags);

		//RN: all dependent floaters copy follow behavior of "parent"
		for(LLFloater::handle_set_iter_t dependent_it = floaterp->mDependents.begin();
			dependent_it != floaterp->mDependents.end(); ++dependent_it)
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp)
			{
				dependent_floaterp->setFollows(follow_flags);
			}
		}
	}

	LLView::reshape(width, height, called_from_parent);
}


void LLFloaterView::restoreAll()
{
	// make sure all subwindows aren't minimized
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		floaterp->setMinimized(FALSE);
	}

	// *FIX: make sure dependents are restored

	// children then deleted by default view constructor
}


LLRect LLFloaterView::findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor )
{
	LLRect base_rect = reference_floater->getRect();
	LLRect::tCoordType width = neighbor->getRect().getWidth();
	LLRect::tCoordType height = neighbor->getRect().getHeight();
	LLRect new_rect = neighbor->getRect();

	LLRect expanded_base_rect = base_rect;
	expanded_base_rect.stretch(10);
	for(LLFloater::handle_set_iter_t dependent_it = reference_floater->mDependents.begin();
		dependent_it != reference_floater->mDependents.end(); ++dependent_it)
	{
		LLFloater* sibling = dependent_it->get();
		// check for dependents within 10 pixels of base floater
		if (sibling && 
			sibling != neighbor && 
			sibling->getVisible() && 
			expanded_base_rect.overlaps(sibling->getRect()))
		{
			base_rect.unionWith(sibling->getRect());
		}
	}

	LLRect::tCoordType left_margin = llmax(0, base_rect.mLeft);
	LLRect::tCoordType right_margin = llmax(0, getRect().getWidth() - base_rect.mRight);
	LLRect::tCoordType top_margin = llmax(0, getRect().getHeight() - base_rect.mTop);
	LLRect::tCoordType bottom_margin = llmax(0, base_rect.mBottom);

	// find position for floater in following order
	// right->left->bottom->top
	for (S32 i = 0; i < 5; i++)
	{
		if (right_margin > width)
		{
			new_rect.translate(base_rect.mRight - neighbor->getRect().mLeft, base_rect.mTop - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (left_margin > width)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mRight, base_rect.mTop - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (bottom_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mLeft, base_rect.mBottom - neighbor->getRect().mTop);
			return new_rect;
		}
		else if (top_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->getRect().mLeft, base_rect.mTop - neighbor->getRect().mBottom);
			return new_rect;
		}

		// keep growing margins to find "best" fit
		left_margin += 20;
		right_margin += 20;
		top_margin += 20;
		bottom_margin += 20;
	}

	// didn't find anything, return initial rect
	return new_rect;
}


void LLFloaterView::bringToFront(LLFloater* child, BOOL give_focus)
{
	// *TODO: make this respect floater's mAutoFocus value, instead of
	// using parameter
	if (child->getHost())
 	{
		// this floater is hosted elsewhere and hence not one of our children, abort
		return;
	}
	std::vector<LLView*> floaters_to_move;
	// Look at all floaters...tab
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		LLFloater *floater = (LLFloater *)viewp;

		// ...but if I'm a dependent floater...
		if (child->isDependent())
		{
			// ...look for floaters that have me as a dependent...
			LLFloater::handle_set_iter_t found_dependent = floater->mDependents.find(child->getHandle());

			if (found_dependent != floater->mDependents.end())
			{
				// ...and make sure all children of that floater (including me) are brought to front...
				for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
					dependent_it != floater->mDependents.end(); )
				{
					LLFloater* sibling = dependent_it->get();
					if (sibling)
					{
						floaters_to_move.push_back(sibling);
					}
					++dependent_it;
				}
				//...before bringing my parent to the front...
				floaters_to_move.push_back(floater);
			}
		}
	}

	std::vector<LLView*>::iterator view_it;
	for(view_it = floaters_to_move.begin(); view_it != floaters_to_move.end(); ++view_it)
	{
		LLFloater* floaterp = (LLFloater*)(*view_it);
		sendChildToFront(floaterp);

		// always unminimize dependee, but allow dependents to stay minimized
		if (!floaterp->isDependent())
		{
			floaterp->setMinimized(FALSE);
		}
	}
	floaters_to_move.clear();

	// ...then bringing my own dependents to the front...
	for(LLFloater::handle_set_iter_t dependent_it = child->mDependents.begin();
		dependent_it != child->mDependents.end(); )
	{
		LLFloater* dependent = dependent_it->get();
		if (dependent)
		{
			sendChildToFront(dependent);
			//don't un-minimize dependent windows automatically
			// respect user's wishes
			//dependent->setMinimized(FALSE);
		}
		++dependent_it;
	}

	// ...and finally bringing myself to front 
	// (do this last, so that I'm left in front at end of this call)
	if( *getChildList()->begin() != child ) 
	{
		sendChildToFront(child);
	}
	child->setMinimized(FALSE);
	if (give_focus && !gFocusMgr.childHasKeyboardFocus(child))
	{
		child->setFocus(TRUE);
		// floater did not take focus, so relinquish focus to world
		if (!child->hasFocus())
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}
	}
}

void LLFloaterView::highlightFocusedFloater()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater *floater = (LLFloater *)(*child_it);

		// skip dependent floaters, as we'll handle them in a batch along with their dependee(?)
		if (floater->isDependent())
		{
			continue;
		}

		BOOL floater_or_dependent_has_focus = gFocusMgr.childHasKeyboardFocus(floater);
		for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
			dependent_it != floater->mDependents.end(); 
			++dependent_it)
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp && gFocusMgr.childHasKeyboardFocus(dependent_floaterp))
			{
				floater_or_dependent_has_focus = TRUE;
			}
		}

		// now set this floater and all its dependents
		floater->setForeground(floater_or_dependent_has_focus);

		for(LLFloater::handle_set_iter_t dependent_it = floater->mDependents.begin();
			dependent_it != floater->mDependents.end(); )
		{
			LLFloater* dependent_floaterp = dependent_it->get();
			if (dependent_floaterp)
			{
				dependent_floaterp->setForeground(floater_or_dependent_has_focus);
			}
			++dependent_it;
		}
			
		floater->cleanupHandles();
	}
}

void LLFloaterView::unhighlightFocusedFloater()
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater *floater = (LLFloater *)(*child_it);

		floater->setForeground(FALSE);
	}
}

void LLFloaterView::focusFrontFloater()
{
	LLFloater* floaterp = getFrontmost();
	if (floaterp)
	{
		floaterp->setFocus(TRUE);
	}
}

void LLFloaterView::getMinimizePosition(S32 *left, S32 *bottom)
{
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	static LLUICachedControl<S32> minimized_width ("UIMinimizedWidth", 0);
	LLRect snap_rect_local = getLocalSnapRect();
	snap_rect_local.mTop += mMinimizePositionVOffset;
	for(S32 col = snap_rect_local.mLeft;
		col < snap_rect_local.getWidth() - minimized_width;
		col += minimized_width)
	{	
		for(S32 row = snap_rect_local.mTop - floater_header_size;
		row > floater_header_size;
		row -= floater_header_size ) //loop rows
		{

			bool foundGap = TRUE;
			for(child_list_const_iter_t child_it = getChildList()->begin();
				child_it != getChildList()->end();
				++child_it) //loop floaters
			{
				// Examine minimized children.
				LLFloater* floater = (LLFloater*)((LLView*)*child_it);
				if(floater->isMinimized()) 
				{
					LLRect r = floater->getRect();
					if((r.mBottom < (row + floater_header_size))
					   && (r.mBottom > (row - floater_header_size))
					   && (r.mLeft < (col + minimized_width))
					   && (r.mLeft > (col - minimized_width)))
					{
						// needs the check for off grid. can't drag,
						// but window resize makes them off
						foundGap = FALSE;
						break;
					}
				}
			} //done floaters
			if(foundGap)
			{
				*left = col;
				*bottom = row;
				return; //done
			}
		} //done this col
	}

	// crude - stack'em all at 0,0 when screen is full of minimized
	// floaters.
	*left = snap_rect_local.mLeft;
	*bottom = snap_rect_local.mBottom;
}


void LLFloaterView::destroyAllChildren()
{
	LLView::deleteAllChildren();
}

void LLFloaterView::closeAllChildren(bool app_quitting)
{
	// iterate over a copy of the list, because closing windows will destroy
	// some windows on the list.
	child_list_t child_list = *(getChildList());

	for (child_list_const_iter_t it = child_list.begin(); it != child_list.end(); ++it)
	{
		LLView* viewp = *it;
		child_list_const_iter_t exists = std::find(getChildList()->begin(), getChildList()->end(), viewp);
		if (exists == getChildList()->end())
		{
			// this floater has already been removed
			continue;
		}

		LLFloater* floaterp = (LLFloater*)viewp;

		// Attempt to close floater.  This will cause the "do you want to save"
		// dialogs to appear.
		// Skip invisible floaters if we're not quitting (STORM-192).
		if (floaterp->canClose() && !floaterp->isDead() &&
			(app_quitting || floaterp->getVisible()))
		{
			floaterp->closeFloater(app_quitting);
		}
	}
}


BOOL LLFloaterView::allChildrenClosed()
{
	// see if there are any visible floaters (some floaters "close"
	// by setting themselves invisible)
	for (child_list_const_iter_t it = getChildList()->begin(); it != getChildList()->end(); ++it)
	{
		LLView* viewp = *it;
		LLFloater* floaterp = (LLFloater*)viewp;

		if (floaterp->getVisible() && !floaterp->isDead() && floaterp->isCloseable())
		{
			return false;
		}
	}
	return true;
}

void LLFloaterView::shiftFloaters(S32 x_offset, S32 y_offset)
{
	for (child_list_const_iter_t it = getChildList()->begin(); it != getChildList()->end(); ++it)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(*it);

		if (floaterp && floaterp->isMinimized())
		{
			floaterp->translate(x_offset, y_offset);
		}
	}
}

void LLFloaterView::refresh()
{
	// Constrain children to be entirely on the screen
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = dynamic_cast<LLFloater*>(*child_it);
		if (floaterp && floaterp->getVisible() )
		{
			// minimized floaters are kept fully onscreen
			adjustToFitScreen(floaterp, !floaterp->isMinimized());
		}
	}
}

void LLFloaterView::adjustToFitScreen(LLFloater* floater, BOOL allow_partial_outside)
{
	if (floater->getParent() != this)
	{
		// floater is hosted elsewhere, so ignore
		return;
	}
	LLRect::tCoordType screen_width = getSnapRect().getWidth();
	LLRect::tCoordType screen_height = getSnapRect().getHeight();

	
	// only automatically resize non-minimized, resizable floaters
	if( floater->isResizable() && !floater->isMinimized() )
	{
		LLRect view_rect = floater->getRect();
		S32 old_width = view_rect.getWidth();
		S32 old_height = view_rect.getHeight();
		S32 min_width;
		S32 min_height;
		floater->getResizeLimits( &min_width, &min_height );

		// Make sure floater isn't already smaller than its min height/width?
		S32 new_width = llmax( min_width, old_width );
		S32 new_height = llmax( min_height, old_height);

		if((new_width > screen_width) || (new_height > screen_height))
		{
			// We have to make this window able to fit on screen
			new_width = llmin(new_width, screen_width);
			new_height = llmin(new_height, screen_height);

			// ...while respecting minimum width/height
			new_width = llmax(new_width, min_width);
			new_height = llmax(new_height, min_height);

			LLRect new_rect;
			new_rect.setLeftTopAndSize(view_rect.mLeft,view_rect.mTop,new_width, new_height);

			floater->setShape(new_rect);

			if (floater->followsRight())
			{
				floater->translate(old_width - new_width, 0);
			}

			if (floater->followsTop())
			{
				floater->translate(0, old_height - new_height);
			}
		}
	}

	// move window fully onscreen
	if (floater->translateIntoRect( getLocalRect(), allow_partial_outside ))
	{
		floater->clearSnapTarget();
	}
}

void LLFloaterView::draw()
{
	refresh();

	// hide focused floater if in cycle mode, so that it can be drawn on top
	LLFloater* focused_floater = getFocusedFloater();

	if (mFocusCycleMode && focused_floater)
	{
		child_list_const_iter_t child_it = getChildList()->begin();
		for (;child_it != getChildList()->end(); ++child_it)
		{
			if ((*child_it) != focused_floater)
			{
				drawChild(*child_it);
			}
		}

		drawChild(focused_floater, -TABBED_FLOATER_OFFSET, TABBED_FLOATER_OFFSET);
	}
	else
	{
		LLView::draw();
	}
}

LLRect LLFloaterView::getSnapRect() const
{
	LLRect snap_rect = getLocalRect();

	LLView* snap_view = mSnapView.get();
	if (snap_view)
	{
		snap_view->localRectToOtherView(snap_view->getLocalRect(), &snap_rect, this);
	}

	return snap_rect;
}

LLFloater *LLFloaterView::getFocusedFloater() const
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLUICtrl* ctrlp = (*child_it)->isCtrl() ? static_cast<LLUICtrl*>(*child_it) : NULL;
		if ( ctrlp && ctrlp->hasFocus() )
		{
			return static_cast<LLFloater *>(ctrlp);
		}
	}
	return NULL;
}

LLFloater *LLFloaterView::getFrontmost() const
{
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if ( viewp->getVisible() && !viewp->isDead())
		{
			return (LLFloater *)viewp;
		}
	}
	return NULL;
}

LLFloater *LLFloaterView::getBackmost() const
{
	LLFloater* back_most = NULL;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if ( viewp->getVisible() )
		{
			back_most = (LLFloater *)viewp;
		}
	}
	return back_most;
}

void LLFloaterView::syncFloaterTabOrder()
{
	// look for a visible modal dialog, starting from first
	LLModalDialog* modal_dialog = NULL;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLModalDialog* dialog = dynamic_cast<LLModalDialog*>(*child_it);
		if (dialog && dialog->isModal() && dialog->getVisible())
		{
			modal_dialog = dialog;
			break;
		}
	}

	if (modal_dialog)
	{
		// If we have a visible modal dialog, make sure that it has focus
		LLUI::addPopup(modal_dialog);
		
		if( !gFocusMgr.childHasKeyboardFocus( modal_dialog ) )
		{
			modal_dialog->setFocus(TRUE);
		}
				
		if( !gFocusMgr.childHasMouseCapture( modal_dialog ) )
		{
			gFocusMgr.setMouseCapture( modal_dialog );
		}
	}
	else
	{
		// otherwise, make sure the focused floater is in the front of the child list
		for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
		{
			LLFloater* floaterp = (LLFloater*)*child_it;
			if (gFocusMgr.childHasKeyboardFocus(floaterp))
			{
				bringToFront(floaterp, FALSE);
				break;
			}
		}
	}

	// sync draw order to tab order
	for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		moveChildToFrontOfTabGroup(floaterp);
	}
}

LLFloater*	LLFloaterView::getParentFloater(LLView* viewp) const
{
	LLView* parentp = viewp->getParent();

	while(parentp && parentp != this)
	{
		viewp = parentp;
		parentp = parentp->getParent();
	}

	if (parentp == this)
	{
		return (LLFloater*)viewp;
	}

	return NULL;
}

S32 LLFloaterView::getZOrder(LLFloater* child)
{
	S32 rv = 0;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if(viewp == child)
		{
			break;
		}
		++rv;
	}
	return rv;
}

void LLFloaterView::pushVisibleAll(BOOL visible, const skip_list_t& skip_list)
{
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		if (skip_list.find(view) == skip_list.end())
		{
			view->pushVisible(visible);
		}
	}

	LLFloaterReg::blockShowFloaters(true);
}

void LLFloaterView::popVisibleAll(const skip_list_t& skip_list)
{
	// make a copy of the list since some floaters change their
	// order in the childList when changing visibility.
	child_list_t child_list_copy = *getChildList();

	for (child_list_const_iter_t child_iter = child_list_copy.begin();
		 child_iter != child_list_copy.end(); ++child_iter)
	{
		LLView *view = *child_iter;
		if (skip_list.find(view) == skip_list.end())
		{
			view->popVisible();
		}
	}

	LLFloaterReg::blockShowFloaters(false);
}

void LLFloater::setInstanceName(const std::string& name)
{
	if (name == mInstanceName)
		return;
	llassert_always(mInstanceName.empty());
	mInstanceName = name;
	if (!mInstanceName.empty())
	{
		std::string ctrl_name = getControlName(mInstanceName, mKey);

		// save_rect and save_visibility only apply to registered floaters
		if (!mRectControl.empty())
		{
			mRectControl = LLFloaterReg::declareRectControl(ctrl_name);
		}
		if (!mVisibilityControl.empty())
		{
			mVisibilityControl = LLFloaterReg::declareVisibilityControl(ctrl_name);
		}
		if(!mDocStateControl.empty())
		{
			mDocStateControl = LLFloaterReg::declareDockStateControl(ctrl_name);
		}

	}
}

void LLFloater::setKey(const LLSD& newkey)
{
	// Note: We don't have to do anything special with registration when we change keys
	mKey = newkey;
}

//static
void LLFloater::setupParamsForExport(Params& p, LLView* parent)
{
	// Do rectangle munging to topleft layout first
	LLPanel::setupParamsForExport(p, parent);

	// Copy the rectangle out to apply layout constraints
	LLRect rect = p.rect;

	// Null out other settings
	p.rect.left.setProvided(false);
	p.rect.top.setProvided(false);
	p.rect.right.setProvided(false);
	p.rect.bottom.setProvided(false);

	// Explicitly set width/height
	p.rect.width.set( rect.getWidth(), true );
	p.rect.height.set( rect.getHeight(), true );

	// If you can't resize this floater, don't export min_height
	// and min_width
	bool can_resize = p.can_resize;
	if (!can_resize)
	{
		p.min_height.setProvided(false);
		p.min_width.setProvided(false);
	}
}

void LLFloater::initFromParams(const LLFloater::Params& p)
{
	// *NOTE: We have too many classes derived from LLFloater to retrofit them 
	// all to pass in params via constructors.  So we use this method.

	 // control_name, tab_stop, focus_lost_callback, initial_value, rect, enabled, visible
	LLPanel::initFromParams(p);

	mTitle = p.title;
	mShortTitle = p.short_title;
	applyTitle();

	setCanTearOff(p.can_tear_off);
	setCanMinimize(p.can_minimize);
	setCanClose(p.can_close);
	setCanDock(p.can_dock);
	setCanResize(p.can_resize);
	setResizeLimits(p.min_width, p.min_height);
	
	mDragOnLeft = p.can_drag_on_left;
	mHeaderHeight = p.header_height;
	mLegacyHeaderHeight = p.legacy_header_height;
	mSingleInstance = p.single_instance;
	mAutoTile = p.auto_tile;
	mOpenCentered = p.open_centered;

	if (p.save_rect)
	{
		mRectControl = "t"; // flag to build mRectControl name once mInstanceName is set
	}
	if (p.save_visibility)
	{
		mVisibilityControl = "t"; // flag to build mVisibilityControl name once mInstanceName is set
	}

	if(p.save_dock_state)
	{
		mDocStateControl = "t"; // flag to build mDocStateControl name once mInstanceName is set
	}
	
	// open callback 
	if (p.open_callback.isProvided())
	{
		mOpenSignal.connect(initCommitCallback(p.open_callback));
	}
	// close callback 
	if (p.close_callback.isProvided())
	{
		mCloseSignal.connect(initCommitCallback(p.close_callback));
	}
}

boost::signals2::connection LLFloater::setMinimizeCallback( const commit_signal_t::slot_type& cb ) 
{ 
	if (!mMinimizeSignal) mMinimizeSignal = new commit_signal_t();
	return mMinimizeSignal->connect(cb); 
}

LLFastTimer::DeclareTimer POST_BUILD("Floater Post Build");

bool LLFloater::initFloaterXML(LLXMLNodePtr node, LLView *parent, const std::string& filename, LLXMLNodePtr output_node)
{
	Params params(LLUICtrlFactory::getDefaultParams<LLFloater>());
	LLXUIParser parser;
	parser.readXUI(node, params, filename); // *TODO: Error checking

	if (output_node)
	{
		Params output_params(params);
		setupParamsForExport(output_params, parent);
        Params default_params(LLUICtrlFactory::getDefaultParams<LLFloater>());
		output_node->setName(node->getName()->mString);
		parser.writeXUI(output_node, output_params, &default_params);
	}

	// Default floater position to top-left corner of screen
	// However, some legacy floaters have explicit top or bottom
	// coordinates set, so respect their wishes.
	if (!params.rect.top.isProvided() && !params.rect.bottom.isProvided())
	{
		params.rect.top.set(0);
	}
	if (!params.rect.left.isProvided() && !params.rect.right.isProvided())
	{
		params.rect.left.set(0);
	}

	params.from_xui = true;
	applyXUILayout(params, parent);
 	initFromParams(params);

	initFloater(params);
	
	LLMultiFloater* last_host = LLFloater::getFloaterHost();
	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost((LLMultiFloater*) this);
	}

	LLUICtrlFactory::createChildren(this, node, child_registry_t::instance(), output_node);

	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost(last_host);
	}
	
	// HACK: When we changed the header height to 25 pixels in Viewer 2, rather
	// than re-layout all the floaters we use this value in pixels to make the
	// whole floater bigger and change the top-left coordinate for widgets.
	// The goal is to eventually set mLegacyHeaderHeight to zero, which would
	// make the top-left corner for widget layout the same as the top-left
	// corner of the window's content area.  James
	S32 header_stretch = (mHeaderHeight - mLegacyHeaderHeight);
	if (header_stretch > 0)
	{
		// Stretch the floater vertically, don't move widgets
		LLRect rect = getRect();
		rect.mTop += header_stretch;

		// This will also update drag handle, title bar, close box, etc.
		setRect(rect);
	}

	BOOL result;
	{
		LLFastTimer ft(POST_BUILD);

		result = postBuild();
	}

	if (!result)
	{
		llerrs << "Failed to construct floater " << getName() << llendl;
	}

	applyRectControl(); // If we have a saved rect control, apply it
	gFloaterView->adjustToFitScreen(this, FALSE); // Floaters loaded from XML should all fit on screen	

	moveResizeHandlesToFront();

	applyDockState();

	return true; // *TODO: Error checking
}

bool LLFloater::isShown() const
{
    return ! isMinimized() && isInVisibleChain();
}

/* static */
bool LLFloater::isShown(const LLFloater* floater)
{
    return floater && floater->isShown();
}

/* static */
bool LLFloater::isMinimized(const LLFloater* floater)
{
    return floater && floater->isMinimized();
}

/* static */
bool LLFloater::isVisible(const LLFloater* floater)
{
    return floater && floater->getVisible();
}

static LLFastTimer::DeclareTimer FTM_BUILD_FLOATERS("Build Floaters");

bool LLFloater::buildFromFile(const std::string& filename, LLXMLNodePtr output_node)
{
	LLFastTimer timer(FTM_BUILD_FLOATERS);
	LLXMLNodePtr root;

	//if exporting, only load the language being exported, 
	//instead of layering localized version on top of english
	if (output_node)
	{
		if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root))
		{
			llwarns << "Couldn't parse floater from: " << LLUI::getLocalizedSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
			return false;
		}
	}
	else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		llwarns << "Couldn't parse floater from: " << LLUI::getSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
		return false;
	}
	
	// root must be called floater
	if( !(root->hasName("floater") || root->hasName("multi_floater")) )
	{
		llwarns << "Root node should be named floater in: " << filename << llendl;
		return false;
	}
	
	bool res = true;
	
	lldebugs << "Building floater " << filename << llendl;
	LLUICtrlFactory::instance().pushFileName(filename);
	{
		if (!getFactoryMap().empty())
		{
			LLPanel::sFactoryStack.push_front(&getFactoryMap());
		}

		 // for local registry callbacks; define in constructor, referenced in XUI or postBuild
		getCommitCallbackRegistrar().pushScope();
		getEnableCallbackRegistrar().pushScope();
		
		res = initFloaterXML(root, getParent(), filename, output_node);

		setXMLFilename(filename);
		
		getCommitCallbackRegistrar().popScope();
		getEnableCallbackRegistrar().popScope();
		
		if (!getFactoryMap().empty())
		{
			LLPanel::sFactoryStack.pop_front();
		}
	}
	LLUICtrlFactory::instance().popFileName();
	
	return res;
}
