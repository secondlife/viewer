/** 
 * @file llfloater.cpp
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

#include "linden_common.h"

#include "llfloater.h"

#include "llfocusmgr.h"

#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llkeyboard.h"
#include "llmenugl.h"	// MENU_BAR_HEIGHT
#include "lltextbox.h"
#include "llresmgr.h"
#include "llui.h"
#include "llwindow.h"
#include "llstl.h"
#include "llcontrol.h"
#include "lltabcontainer.h"
#include "v2math.h"

const S32 MINIMIZED_WIDTH = 160;
const S32 CLOSE_BOX_FROM_TOP = 1;
// use this to control "jumping" behavior when Ctrl-Tabbing
const S32 TABBED_FLOATER_OFFSET = 0;

std::string	LLFloater::sButtonActiveImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnCloseActiveUUID",		//BUTTON_CLOSE
	"UIImgBtnRestoreActiveUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizeActiveUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffActiveUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnCloseActiveUUID",		//BUTTON_EDIT
};

std::string	LLFloater::sButtonInactiveImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnCloseInactiveUUID",	//BUTTON_CLOSE
	"UIImgBtnRestoreInactiveUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizeInactiveUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffInactiveUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnCloseInactiveUUID",	//BUTTON_EDIT
};

std::string	LLFloater::sButtonPressedImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnClosePressedUUID",		//BUTTON_CLOSE
	"UIImgBtnRestorePressedUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizePressedUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffPressedUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnClosePressedUUID",		//BUTTON_EDIT
};

std::string	LLFloater::sButtonNames[BUTTON_COUNT] = 
{
	"llfloater_close_btn",	//BUTTON_CLOSE
	"llfloater_restore_btn",	//BUTTON_RESTORE
	"llfloater_minimize_btn",	//BUTTON_MINIMIZE
	"llfloater_tear_off_btn",	//BUTTON_TEAR_OFF
	"llfloater_edit_btn",		//BUTTON_EDIT
};

std::string	LLFloater::sButtonToolTips[BUTTON_COUNT] = 
{
#ifdef LL_DARWIN
	"Close (Cmd-W)",	//BUTTON_CLOSE
#else
	"Close (Ctrl-W)",	//BUTTON_CLOSE
#endif
	"Restore",	//BUTTON_RESTORE
	"Minimize",	//BUTTON_MINIMIZE
	"Tear Off",	//BUTTON_TEAR_OFF
	"Edit",		//BUTTON_EDIT
};

LLFloater::click_callback LLFloater::sButtonCallbacks[BUTTON_COUNT] =
{
	LLFloater::onClickClose,	//BUTTON_CLOSE
	LLFloater::onClickMinimize, //BUTTON_RESTORE
	LLFloater::onClickMinimize, //BUTTON_MINIMIZE
	LLFloater::onClickTearOff,	//BUTTON_TEAR_OFF
	LLFloater::onClickEdit,	//BUTTON_EDIT
};

LLMultiFloater* LLFloater::sHostp = NULL;
BOOL			LLFloater::sEditModeEnabled;
LLFloater::handle_map_t	LLFloater::sFloaterMap;

LLFloaterView* gFloaterView = NULL;

LLFloater::LLFloater() :
	//FIXME: we should initialize *all* member variables here
	LLPanel(), mAutoFocus(TRUE),
	mResizable(FALSE),
	mDragOnLeft(FALSE),
	mMinWidth(0),
	mMinHeight(0)
{
	// automatically take focus when opened
	mAutoFocus = TRUE;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++) 
	{
		mResizeBar[i] = NULL; 
		mResizeHandle[i] = NULL;
	}
	mDragHandle = NULL;
	mHandle.bind(this);
	mNotificationContext = new LLFloaterNotificationContext(getHandle());
}

LLFloater::LLFloater(const std::string& name)
:	LLPanel(name), mAutoFocus(TRUE) // automatically take focus when opened
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++) 
	{
		mResizeBar[i] = NULL; 
		mResizeHandle[i] = NULL;
	}
	std::string title; // null string
	initFloater(title, FALSE, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT, FALSE, TRUE, TRUE); // defaults
}


LLFloater::LLFloater(const std::string& name, const LLRect& rect, const std::string& title, 
	BOOL resizable, 
	S32 min_width, 
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect, bordered), mAutoFocus(TRUE) // automatically take focus when opened
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++) 
	{
		mResizeBar[i] = NULL; 
		mResizeHandle[i] = NULL;
	}
	initFloater( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}

LLFloater::LLFloater(const std::string& name, const std::string& rect_control, const std::string& title, 
	BOOL resizable, 
	S32 min_width, 
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect_control, bordered), mAutoFocus(TRUE) // automatically take focus when opened
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	for (S32 i = 0; i < 4; i++) 
	{
		mResizeBar[i] = NULL; 
		mResizeHandle[i] = NULL;
	}
	initFloater( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}


// Note: Floaters constructed from XML call init() twice!
void LLFloater::initFloater(const std::string& title,
					 BOOL resizable, S32 min_width, S32 min_height,
					 BOOL drag_on_left, BOOL minimizable, BOOL close_btn)
{
	mHandle.bind(this);
	mNotificationContext = new LLFloaterNotificationContext(getHandle());

	// Init function can be called more than once, so clear out old data.
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		if (mButtons[i] != NULL)
		{
			removeChild(mButtons[i]);
			delete mButtons[i];
			mButtons[i] = NULL;
		}
	}
	mButtonScale = 1.f;

	//sjb: Thia is a bit of a hack:
	BOOL need_border = hasBorder();
	// remove the border since deleteAllChildren() will also delete the border (but not clear mBorder)
	removeBorder();
	// this will delete mBorder too
	deleteAllChildren();
	// add the border back if we want it
	if (need_border)
	{
	    addBorder();
	}

	// chrome floaters don't take focus at all
	setFocusRoot(!getIsChrome());

	// Reset cached pointers
	mDragHandle = NULL;
	for (S32 i = 0; i < 4; i++) 
	{
		mResizeBar[i] = NULL;
		mResizeHandle[i] = NULL;
	}
	mCanTearOff = TRUE;
	mEditing = FALSE;

	// Clicks stop here.
	setMouseOpaque(TRUE);

	mFirstLook = TRUE;
	mForeground = FALSE;
	mDragOnLeft = drag_on_left == TRUE;

	// Floaters always draw their background, unlike every other panel.
	setBackgroundVisible(TRUE);

	// Floaters start not minimized.  When minimized, they save their
	// prior rectangle to be used on restore.
	mMinimized = FALSE;
	mExpandedRect.set(0,0,0,0);
	
	S32 close_pad;			// space to the right of close box
	S32 close_box_size;		// For layout purposes, how big is the close box?
	if (close_btn)
	{
		close_box_size = LLFLOATER_CLOSE_BOX_SIZE;
		close_pad = 0;
	}
	else
	{
		close_box_size = 0;
		close_pad = 0;
	}

	S32 minimize_box_size;
	S32 minimize_pad;
	if (minimizable && !drag_on_left)
	{
		minimize_box_size = LLFLOATER_CLOSE_BOX_SIZE;
		minimize_pad = 0;
	}
	else
	{
		minimize_box_size = 0;
		minimize_pad = 0;
	}

	// Drag Handle
	// Add first so it's in the background.
//	const S32 drag_pad = 2;
	if (drag_on_left)
	{
		LLRect drag_handle_rect;
		drag_handle_rect.setOriginAndSize(
			0, 0,
			DRAG_HANDLE_WIDTH,
			getRect().getHeight() - LLPANEL_BORDER_WIDTH - close_box_size);
		mDragHandle = new LLDragHandleLeft(std::string("drag"), drag_handle_rect, title );
	}
	else // drag on top
	{
		LLRect drag_handle_rect( 0, getRect().getHeight(), getRect().getWidth(), 0 );
		mDragHandle = new LLDragHandleTop( std::string("Drag Handle"), drag_handle_rect, title );
	}
	addChild(mDragHandle);

	// Resize Handle
	mResizable = resizable;
	mMinWidth = min_width;
	mMinHeight = min_height;

	if( mResizable )
	{
		// Resize bars (sides)
		const S32 RESIZE_BAR_THICKNESS = 3;
		mResizeBar[LLResizeBar::LEFT] = new LLResizeBar( 
			std::string("resizebar_left"),
			this,
			LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0), 
			min_width, S32_MAX, LLResizeBar::LEFT );
		addChild( mResizeBar[0] );

		mResizeBar[LLResizeBar::TOP] = new LLResizeBar( 
			std::string("resizebar_top"),
			this,
			LLRect( 0, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_BAR_THICKNESS), 
			min_height, S32_MAX, LLResizeBar::TOP );
		addChild( mResizeBar[1] );

		mResizeBar[LLResizeBar::RIGHT] = new LLResizeBar( 
			std::string("resizebar_right"),
			this,
			LLRect( getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0), 
			min_width, S32_MAX, LLResizeBar::RIGHT );
		addChild( mResizeBar[2] );

		mResizeBar[LLResizeBar::BOTTOM] = new LLResizeBar( 
			std::string("resizebar_bottom"),
			this,
			LLRect( 0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0), 
			min_height, S32_MAX, LLResizeBar::BOTTOM );
		addChild( mResizeBar[3] );


		// Resize handles (corners)
		mResizeHandle[0] = new LLResizeHandle( 
			std::string("Resize Handle"),
			LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, getRect().getWidth(), 0),
			min_width,
			min_height,
			LLResizeHandle::RIGHT_BOTTOM);
		addChild(mResizeHandle[0]);

		mResizeHandle[1] = new LLResizeHandle(
			std::string("resize"), 
			LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_HANDLE_HEIGHT),
			min_width,
			min_height,
			LLResizeHandle::RIGHT_TOP );
		addChild(mResizeHandle[1]);
		
		mResizeHandle[2] = new LLResizeHandle( std::string("resize"), 
											   LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 ),
											   min_width,
											   min_height,
											   LLResizeHandle::LEFT_BOTTOM );
		addChild(mResizeHandle[2]);

		mResizeHandle[3] = new LLResizeHandle( std::string("resize"), 
			LLRect( 0, getRect().getHeight(), RESIZE_HANDLE_WIDTH, getRect().getHeight() - RESIZE_HANDLE_HEIGHT ),
			min_width,
			min_height,
			LLResizeHandle::LEFT_TOP );
		addChild(mResizeHandle[3]);
	}

	// Close button.
	if (close_btn)
	{
		mButtonsEnabled[BUTTON_CLOSE] = TRUE;
	}

	// Minimize button only for top draggers
	if ( !drag_on_left && minimizable )
	{
		mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
	}

	// Keep track of whether this window has ever been dragged while it
	// was minimized.  If it has, we'll remember its position for the
	// next time it's minimized.
	mHasBeenDraggedWhileMinimized = FALSE;
	mPreviousMinimizedLeft = 0;
	mPreviousMinimizedBottom = 0;

	buildButtons();

	// JC - Don't do this here, because many floaters first construct themselves,
	// then show themselves.  Put it in setVisibleAndFrontmost.
	// make_ui_sound("UISndWindowOpen");

	// RN: floaters are created in the invisible state	
	setVisible(FALSE);

	// add self to handle->floater map
	sFloaterMap[mHandle] = this;

	if (!getParent())
	{
		gFloaterView->addChild(this);
	}
}

// virtual
LLFloater::~LLFloater()
{
	delete mNotificationContext;
	mNotificationContext = NULL;

	control_map_t::iterator itor;
	for (itor = mFloaterControls.begin(); itor != mFloaterControls.end(); ++itor)
	{
		delete itor->second;
	}
	mFloaterControls.clear();

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
}


void LLFloater::setVisible( BOOL visible )
{
	LLPanel::setVisible(visible);
	if( visible && mFirstLook )
	{
		mFirstLook = FALSE;
	}

	if( !visible )
	{
		if( gFocusMgr.childIsTopCtrl( this ) )
		{
			gFocusMgr.setTopCtrl(NULL);
		}

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
}

void LLFloater::open()	/* Flawfinder: ignore */
{
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
	else if (getHost() != NULL)
	{
		// already hosted
		getHost()->showFloater(this);
	}
	else
	{
		setMinimized(FALSE);
		setVisibleAndFrontmost(mAutoFocus);
	}

	onOpen();
}

void LLFloater::close(bool app_quitting)
{
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
				floaterp->close();
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

		// Let floater do cleanup.
		onClose(app_quitting);
	}
}

/*virtual*/
void LLFloater::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);
}

void LLFloater::releaseFocus()
{
	if( gFocusMgr.childIsTopCtrl( this ) )
	{
		gFocusMgr.setTopCtrl(NULL);
	}

	if( gFocusMgr.childHasKeyboardFocus( this ) )
	{
		gFocusMgr.setKeyboardFocus(NULL);
	}

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

void LLFloater::applyRectControl()
{
	if (!getRectControl().empty())
	{
		const LLRect& rect = LLUI::sConfigGroup->getRect(getRectControl());
		translate( rect.mLeft - getRect().mLeft, rect.mBottom - getRect().mBottom);
		if (mResizable)
		{
			reshape(llmax(mMinWidth, rect.getWidth()), llmax(mMinHeight, rect.getHeight()));
		}
	}
}

void LLFloater::applyTitle()
{
	if (gNoRender)
	{
		return;
	}

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
}

const std::string& LLFloater::getCurrentTitle() const
{
	return mDragHandle ? mDragHandle->getTitle() : LLStringUtil::null;
}

void LLFloater::setTitle( const std::string& title )
{
	mTitle = title;
	applyTitle();
}

std::string LLFloater::getTitle()
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

std::string LLFloater::getShortTitle()
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

void LLFloater::snappedTo(const LLView* snap_view)
{
	if (!snap_view || snap_view == getParent())
	{
		clearSnapTarget();
	}
	else
	{
		//RN: assume it's a floater as it must be a sibling to our parent floater
		LLFloater* floaterp = (LLFloater*)snap_view;
		
		setSnapTarget(floaterp->getHandle());
	}
}

void LLFloater::userSetShape(const LLRect& new_rect)
{
	const LLRect& old_rect = getRect();
	LLView::userSetShape(new_rect);

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
				floaterp->userSetShape(dependent_rect);
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
	if (minimize == mMinimized) return;

	if (minimize)
	{
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
		
		mMinimized = TRUE;

		// Reshape *after* setting mMinimized
		reshape( MINIMIZED_WIDTH, LLFLOATER_HEADER_SIZE, TRUE);
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
	
	applyTitle ();

	make_ui_sound("UISndWindowClose");
	updateButtons();
}

void LLFloater::setFocus( BOOL b )
{
	if (b && getIsChrome())
	{
		return;
	}
	LLUICtrl* last_focus = gFocusMgr.getLastFocusForGroup(this);
	// a descendent already has focus
	BOOL child_had_focus = gFocusMgr.childHasKeyboardFocus(this);

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
	updateButtons();
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

BOOL LLFloater::offerClickToButton(S32 x, S32 y, MASK mask, EFloaterButtons index)
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
	}
}

//static
void LLFloater::setEditModeEnabled(BOOL enable)
{
	if (enable != sEditModeEnabled)
	{
		S32 count = 0;
		for(handle_map_iter_t iter = sFloaterMap.begin(); iter != sFloaterMap.end(); ++iter)
		{
			LLFloater* floater = iter->second;
			if (!floater->isDead())
			{
				iter->second->mButtonsEnabled[BUTTON_EDIT] = enable;
				iter->second->updateButtons();
			}
			count++;
		}
	}

	sEditModeEnabled = enable;
}


// static
void LLFloater::onClickMinimize(void *userdata)
{
	LLFloater* self = (LLFloater*) userdata;
	if (!self) return;

	self->setMinimized( !self->isMinimized() );
}

void LLFloater::onClickTearOff(void *userdata)
{
	LLFloater* self = (LLFloater*) userdata;
	if (!self) return;

	LLMultiFloater* host_floater = self->getHost();
	if (host_floater) //Tear off
	{
		LLRect new_rect;
		host_floater->removeFloater(self);
		// reparent to floater view
		gFloaterView->addChild(self);

		self->open();	/* Flawfinder: ignore */
		
		// only force position for floaters that don't have that data saved
		if (self->getRectControl().empty())
		{
			new_rect.setLeftTopAndSize(host_floater->getRect().mLeft + 5, host_floater->getRect().mTop - LLFLOATER_HEADER_SIZE - 5, self->getRect().getWidth(), self->getRect().getHeight());
			self->setRect(new_rect);
		}
		gFloaterView->adjustToFitScreen(self, FALSE);
		// give focus to new window to keep continuity for the user
		self->setFocus(TRUE);
	}
	else  //Attach to parent.
	{
		LLMultiFloater* new_host = (LLMultiFloater*)self->mLastHostHandle.get();
		if (new_host)
		{
			self->setMinimized(FALSE); // to reenable minimize button if it was minimized
			new_host->showFloater(self);
			// make sure host is visible
			new_host->open();
		}
	}
}

// static
void LLFloater::onClickEdit(void *userdata)
{
	LLFloater* self = (LLFloater*) userdata;
	if (!self) return;

	self->mEditing = self->mEditing ? FALSE : TRUE;
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
	for(LLFloater* floater_to_close = focused_floater; 
		NULL != floater_to_close; 
		floater_to_close = gFloaterView->getParentFloater(floater_to_close))
	{
		if(floater_to_close->isCloseable())
		{
			return floater_to_close;
		}
	}

	return NULL;
}

// static
void LLFloater::closeFocusedFloater()
{
	LLFloater* floater_to_close = LLFloater::getClosableFloaterFromFocus();
	if(floater_to_close)
	{
		floater_to_close->close();
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
void LLFloater::onClickClose( void* userdata )
{
	LLFloater* self = (LLFloater*) userdata;
	if (!self) return;

	self->close();
}


// virtual
void LLFloater::draw()
{
	// draw background
	if( isBackgroundVisible() )
	{
		S32 left = LLPANEL_BORDER_WIDTH;
		S32 top = getRect().getHeight() - LLPANEL_BORDER_WIDTH;
		S32 right = getRect().getWidth() - LLPANEL_BORDER_WIDTH;
		S32 bottom = LLPANEL_BORDER_WIDTH;

		LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
		F32 shadow_offset = (F32)LLUI::sConfigGroup->getS32("DropShadowFloater");
		if (!isBackgroundOpaque())
		{
			shadow_offset *= 0.2f;
			shadow_color.mV[VALPHA] *= 0.5f;
		}
		gl_drop_shadow(left, top, right, bottom, 
			shadow_color, 
			llround(shadow_offset));

		// No transparent windows in simple UI
		if (isBackgroundOpaque())
		{
			gl_rect_2d( left, top, right, bottom, getBackgroundColor() );
		}
		else
		{
			gl_rect_2d( left, top, right, bottom, getTransparentColor() );
		}

		if(gFocusMgr.childHasKeyboardFocus(this) && !getIsChrome() && !getCurrentTitle().empty())
		{
			// draw highlight on title bar to indicate focus.  RDW
			const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF );
			LLRect r = getRect();
			gl_rect_2d_offset_local(0, r.getHeight(), r.getWidth(), r.getHeight() - (S32)font->getLineHeight() - 1, 
				LLUI::sColorsGroup->getColor("TitleBarFocusColor"), 0, TRUE);
		}
	}

	LLPanel::updateDefaultBtn();

	if( getDefaultButton() )
	{
		if (hasFocus() && getDefaultButton()->getEnabled())
		{
			LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
			// is this button a direct descendent and not a nested widget (e.g. checkbox)?
			BOOL focus_is_child_button = dynamic_cast<LLButton*>(focus_ctrl) != NULL && focus_ctrl->getParent() == this;
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
		// draw children
		LLView* focused_child = gFocusMgr.getKeyboardFocus();
		BOOL focused_child_visible = FALSE;
		if (focused_child && focused_child->getParent() == this)
		{
			focused_child_visible = focused_child->getVisible();
			focused_child->setVisible(FALSE);
		}

		// don't call LLPanel::draw() since we've implemented custom background rendering
		LLView::draw();

		if (focused_child_visible)
		{
			focused_child->setVisible(TRUE);
		}
		drawChild(focused_child);
	}

	if( isBackgroundVisible() )
	{
		// add in a border to improve spacialized visual aclarity ;)
		// use lines instead of gl_rect_2d so we can round the edges as per james' recommendation
		LLUI::setLineWidth(1.5f);
		LLColor4 outlineColor = gFocusMgr.childHasKeyboardFocus(this) ? LLUI::sColorsGroup->getColor("FloaterFocusBorderColor") : LLUI::sColorsGroup->getColor("FloaterUnfocusBorderColor");
		gl_rect_2d_offset_local(0, getRect().getHeight() + 1, getRect().getWidth() + 1, 0, outlineColor, -LLPANEL_BORDER_WIDTH, FALSE);
		LLUI::setLineWidth(1.f);
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

void	LLFloater::setCanMinimize(BOOL can_minimize)
{
	// if removing minimize/restore button programmatically,
	// go ahead and unminimize floater
	if (!can_minimize)
	{
		setMinimized(FALSE);
	}

	mButtonsEnabled[BUTTON_MINIMIZE] = can_minimize && !isMinimized();
	mButtonsEnabled[BUTTON_RESTORE]  = can_minimize &&  isMinimized();

	updateButtons();
}

void	LLFloater::setCanClose(BOOL can_close)
{
	mButtonsEnabled[BUTTON_CLOSE] = can_close;

	updateButtons();
}

void	LLFloater::setCanTearOff(BOOL can_tear_off)
{
	mCanTearOff = can_tear_off;
	mButtonsEnabled[BUTTON_TEAR_OFF] = mCanTearOff && !mHostHandle.isDead();

	updateButtons();
}


void	LLFloater::setCanResize(BOOL can_resize)
{
	if (mResizable && !can_resize)
	{
		for (S32 i = 0; i < 4; i++) 
		{
			removeChild(mResizeBar[i], TRUE);
			mResizeBar[i] = NULL; 

			removeChild(mResizeHandle[i], TRUE);
			mResizeHandle[i] = NULL;
		}
	}
	else if (!mResizable && can_resize)
	{
		// Resize bars (sides)
		const S32 RESIZE_BAR_THICKNESS = 3;
		mResizeBar[0] = new LLResizeBar( 
			std::string("resizebar_left"),
			this,
			LLRect( 0, getRect().getHeight(), RESIZE_BAR_THICKNESS, 0), 
			mMinWidth, S32_MAX, LLResizeBar::LEFT );
		addChild( mResizeBar[0] );

		mResizeBar[1] = new LLResizeBar( 
			std::string("resizebar_top"),
			this,
			LLRect( 0, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_BAR_THICKNESS), 
			mMinHeight, S32_MAX, LLResizeBar::TOP );
		addChild( mResizeBar[1] );

		mResizeBar[2] = new LLResizeBar( 
			std::string("resizebar_right"),
			this,
			LLRect( getRect().getWidth() - RESIZE_BAR_THICKNESS, getRect().getHeight(), getRect().getWidth(), 0), 
			mMinWidth, S32_MAX, LLResizeBar::RIGHT );
		addChild( mResizeBar[2] );

		mResizeBar[3] = new LLResizeBar( 
			std::string("resizebar_bottom"),
			this,
			LLRect( 0, RESIZE_BAR_THICKNESS, getRect().getWidth(), 0), 
			mMinHeight, S32_MAX, LLResizeBar::BOTTOM );
		addChild( mResizeBar[3] );


		// Resize handles (corners)
		mResizeHandle[0] = new LLResizeHandle( 
			std::string("Resize Handle"),
			LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, getRect().getWidth(), 0),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::RIGHT_BOTTOM);
		addChild(mResizeHandle[0]);

		mResizeHandle[1] = new LLResizeHandle( std::string("resize"), 
			LLRect( getRect().getWidth() - RESIZE_HANDLE_WIDTH, getRect().getHeight(), getRect().getWidth(), getRect().getHeight() - RESIZE_HANDLE_HEIGHT),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::RIGHT_TOP );
		addChild(mResizeHandle[1]);
		
		mResizeHandle[2] = new LLResizeHandle( std::string("resize"), 
											   LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 ),
											   mMinWidth,
											   mMinHeight,
											   LLResizeHandle::LEFT_BOTTOM );
		addChild(mResizeHandle[2]);

		mResizeHandle[3] = new LLResizeHandle( std::string("resize"), 
			LLRect( 0, getRect().getHeight(), RESIZE_HANDLE_WIDTH, getRect().getHeight() - RESIZE_HANDLE_HEIGHT ),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::LEFT_TOP );
		addChild(mResizeHandle[3]);
	}
	mResizable = can_resize;
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

void LLFloater::updateButtons()
{
	S32 button_count = 0;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		if(!mButtons[i]) continue;
		mButtons[i]->setEnabled(mButtonsEnabled[i]);

		if (mButtonsEnabled[i] 
			//*HACK: always render close button for hosted floaters
			// so that users don't accidentally hit the button when closing multiple windows
			// in the chatterbox
			|| (i == BUTTON_CLOSE && mButtonScale != 1.f))
		{
			button_count++;

			LLRect btn_rect;
			if (mDragOnLeft)
			{
				btn_rect.setLeftTopAndSize(
					LLPANEL_BORDER_WIDTH,
					getRect().getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * button_count,
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}
			else
			{
				btn_rect.setLeftTopAndSize(
					getRect().getWidth() - LLPANEL_BORDER_WIDTH - (LLFLOATER_CLOSE_BOX_SIZE + 1) * button_count,
					getRect().getHeight() - CLOSE_BOX_FROM_TOP,
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}

			mButtons[i]->setRect(btn_rect);
			mButtons[i]->setVisible(TRUE);
			// the restore button should have a tab stop so that it takes action when you Ctrl-Tab to a minimized floater
			mButtons[i]->setTabStop(i == BUTTON_RESTORE);
		}
		else if (mButtons[i])
		{
			mButtons[i]->setVisible(FALSE);
		}
	}
	if (mDragHandle)
		mDragHandle->setMaxTitleWidth(getRect().getWidth() - (button_count * (LLFLOATER_CLOSE_BOX_SIZE + 1)));
}

void LLFloater::buildButtons()
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		LLRect btn_rect;
		if (mDragOnLeft)
		{
			btn_rect.setLeftTopAndSize(
				LLPANEL_BORDER_WIDTH,
				getRect().getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * (i + 1),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}
		else
		{
			btn_rect.setLeftTopAndSize(
				getRect().getWidth() - LLPANEL_BORDER_WIDTH - (LLFLOATER_CLOSE_BOX_SIZE + 1) * (i + 1),
				getRect().getHeight() - CLOSE_BOX_FROM_TOP,
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}

		LLButton* buttonp = new LLButton(
			sButtonNames[i],
			btn_rect,
			sButtonActiveImageNames[i],
			sButtonPressedImageNames[i],
			LLStringUtil::null,
			sButtonCallbacks[i],
			this,
			LLFontGL::sSansSerif);

		buttonp->setTabStop(FALSE);
		buttonp->setFollowsTop();
		buttonp->setFollowsRight();
		buttonp->setToolTip( sButtonToolTips[i] );
		buttonp->setImageColor(LLUI::sColorsGroup->getColor("FloaterButtonImageColor"));
		buttonp->setHoverImages(sButtonPressedImageNames[i],
								sButtonPressedImageNames[i]);
		buttonp->setScaleImage(TRUE);
		buttonp->setSaveToXML(false);
		addChild(buttonp);
		mButtons[i] = buttonp;
	}

	updateButtons();
}

/////////////////////////////////////////////////////
// LLFloaterView

LLFloaterView::LLFloaterView( const std::string& name, const LLRect& rect )
:	LLUICtrl( name, rect, FALSE, NULL, NULL, FOLLOWS_ALL ),
	mFocusCycleMode(FALSE),
	mSnapOffsetBottom(0)
{
	setTabStop(FALSE);
	resetStartingFloaterPosition();
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
		LLRect r = floaterp->getRect();

		// Compute absolute distance from each edge of screen
		S32 left_offset = llabs(r.mLeft - 0);
		S32 right_offset = llabs(old_width - r.mRight);

		S32 top_offset = llabs(old_height - r.mTop);
		S32 bottom_offset = llabs(r.mBottom - 0);

		// Make if follow the edge it is closest to
		U32 follow_flags = 0x0;

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


void LLFloaterView::getNewFloaterPosition(S32* left,S32* top)
{
	// Workaround: mRect may change between when this object is created and the first time it is used.
	static BOOL first = TRUE;
	if( first )
	{
		resetStartingFloaterPosition();
		first = FALSE;
	}
	
	const S32 FLOATER_PAD = 16;
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	LLRect full_window(0, window_size.mY, window_size.mX, 0);
	LLRect floater_creation_rect(
		160,
		full_window.getHeight() - 2 * MENU_BAR_HEIGHT,
		full_window.getWidth() * 2 / 3,
		130 );
	floater_creation_rect.stretch( -FLOATER_PAD );

	*left = mNextLeft;
	*top = mNextTop;

	const S32 STEP = 25;
	S32 bottom = floater_creation_rect.mBottom + 2 * STEP;
	S32 right = floater_creation_rect.mRight - 4 * STEP;

	mNextTop -= STEP;
	mNextLeft += STEP;

	if( (mNextTop < bottom ) || (mNextLeft > right) )
	{
		mColumn++;
		mNextTop = floater_creation_rect.mTop;
		mNextLeft = STEP * mColumn;

		if( (mNextTop < bottom) || (mNextLeft > right) )
		{
			// Advancing the column didn't work, so start back at the beginning
			resetStartingFloaterPosition();
		}
	}
}

void LLFloaterView::resetStartingFloaterPosition()
{
	const S32 FLOATER_PAD = 16;
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	LLRect full_window(0, window_size.mY, window_size.mX, 0);
	LLRect floater_creation_rect(
		160,
		full_window.getHeight() - 2 * MENU_BAR_HEIGHT,
		full_window.getWidth() * 2 / 3,
		130 );
	floater_creation_rect.stretch( -FLOATER_PAD );

	mNextLeft = floater_creation_rect.mLeft;
	mNextTop = floater_creation_rect.mTop;
	mColumn = 0;
}

LLRect LLFloaterView::findNeighboringPosition( LLFloater* reference_floater, LLFloater* neighbor )
{
	LLRect base_rect = reference_floater->getRect();
	S32 width = neighbor->getRect().getWidth();
	S32 height = neighbor->getRect().getHeight();
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
			expanded_base_rect.rectInRect(&sibling->getRect()))
		{
			base_rect.unionWith(sibling->getRect());
		}
	}

	S32 left_margin = llmax(0, base_rect.mLeft);
	S32 right_margin = llmax(0, getRect().getWidth() - base_rect.mRight);
	S32 top_margin = llmax(0, getRect().getHeight() - base_rect.mTop);
	S32 bottom_margin = llmax(0, base_rect.mBottom);

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
	S32 col = 0;
	LLRect snap_rect_local = getLocalSnapRect();
	for(S32 row = snap_rect_local.mBottom;
		row < snap_rect_local.getHeight() - LLFLOATER_HEADER_SIZE;
		row += LLFLOATER_HEADER_SIZE ) //loop rows
	{
		for(col = snap_rect_local.mLeft;
			col < snap_rect_local.getWidth() - MINIMIZED_WIDTH;
			col += MINIMIZED_WIDTH)
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
					if((r.mBottom < (row + LLFLOATER_HEADER_SIZE))
					   && (r.mBottom > (row - LLFLOATER_HEADER_SIZE))
					   && (r.mLeft < (col + MINIMIZED_WIDTH))
					   && (r.mLeft > (col - MINIMIZED_WIDTH)))
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
		if (floaterp->canClose() && !floaterp->isDead())
		{
			floaterp->close(app_quitting);
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

		if (floaterp->getVisible() && !floaterp->isDead() && floaterp->canClose())
		{
			return false;
		}
	}
	return true;
}


void LLFloaterView::refresh()
{
	// Constrain children to be entirely on the screen
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		if( floaterp->getVisible() )
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
	S32 screen_width = getSnapRect().getWidth();
	S32 screen_height = getSnapRect().getHeight();
	// convert to local coordinate frame
	LLRect snap_rect_local = getLocalSnapRect();

	if( floater->isResizable() )
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

			floater->reshape( new_width, new_height, TRUE );
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
	if (floater->translateIntoRect( snap_rect_local, allow_partial_outside ))
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
	LLRect snap_rect = getRect();
	snap_rect.mBottom += mSnapOffsetBottom;

	return snap_rect;
}

LLFloater *LLFloaterView::getFocusedFloater()
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

LLFloater *LLFloaterView::getFrontmost()
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

LLFloater *LLFloaterView::getBackmost()
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
	// bring focused floater to front
	for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		if (gFocusMgr.childHasKeyboardFocus(floaterp))
		{
			bringToFront(floaterp, FALSE);
			break;
		}
	}

	// then sync draw order to tab order
	for ( child_list_const_reverse_iter_t child_it = getChildList()->rbegin(); child_it != getChildList()->rend(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		moveChildToFrontOfTabGroup(floaterp);
	}
}

LLFloater*	LLFloaterView::getParentFloater(LLView* viewp)
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
}

void LLFloaterView::popVisibleAll(const skip_list_t& skip_list)
{
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *view = *child_iter;
		if (skip_list.find(view) == skip_list.end())
		{
			view->popVisible();
		}
	}
}

//
// LLMultiFloater
//

LLMultiFloater::LLMultiFloater() :
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(TRUE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{

}

LLMultiFloater::LLMultiFloater(LLTabContainer::TabPosition tab_pos) :
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(TRUE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{

}

LLMultiFloater::LLMultiFloater(const std::string &name) :
	LLFloater(name),
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(FALSE),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
}

LLMultiFloater::LLMultiFloater(
	const std::string& name,
	const LLRect& rect,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) : 
	LLFloater(name, rect, name),
	mTabContainer(NULL),
	mTabPos(LLTabContainer::TOP),
	mAutoResize(auto_resize),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
	mTabContainer = new LLTabContainer(std::string("Preview Tabs"), 
		LLRect(LLPANEL_BORDER_WIDTH, getRect().getHeight() - LLFLOATER_HEADER_SIZE, getRect().getWidth() - LLPANEL_BORDER_WIDTH, 0), 
		mTabPos, 
		FALSE, 
		FALSE);
	mTabContainer->setFollowsAll();
	if (isResizable())
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}

	addChild(mTabContainer);
}

LLMultiFloater::LLMultiFloater(
	const std::string& name,
	const std::string& rect_control,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) : 
	LLFloater(name, rect_control, name),
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(auto_resize),
	mOrigMinWidth(0),
	mOrigMinHeight(0)
{
	mTabContainer = new LLTabContainer(std::string("Preview Tabs"), 
		LLRect(LLPANEL_BORDER_WIDTH, getRect().getHeight() - LLFLOATER_HEADER_SIZE, getRect().getWidth() - LLPANEL_BORDER_WIDTH, 0), 
		mTabPos, 
		FALSE, 
		FALSE);
	mTabContainer->setFollowsAll();
	if (isResizable() && mTabPos == LLTabContainer::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}

	addChild(mTabContainer);
	
}


void LLMultiFloater::open()	/* Flawfinder: ignore */
{
	if (mTabContainer->getTabCount() > 0)
	{
		LLFloater::open();	/* Flawfinder: ignore */
	}
	else
	{
		// for now, don't allow multifloaters
		// without any child floaters
		close();
	}
}

void LLMultiFloater::onClose(bool app_quitting)
{
	if(closeAllFloaters() == TRUE)
	{
		LLFloater::onClose(app_quitting);
	}//else not all tabs could be closed...
}

void LLMultiFloater::draw()
{
	if (mTabContainer->getTabCount() == 0)
	{
		//RN: could this potentially crash in draw hierarchy?
		close();
	}
	else
	{
		for (S32 i = 0; i < mTabContainer->getTabCount(); i++)
		{
			LLFloater* floaterp = (LLFloater*)mTabContainer->getPanelByIndex(i);
			if (floaterp->getShortTitle() != mTabContainer->getPanelTitle(i))
			{
				mTabContainer->setPanelTitle(i, floaterp->getShortTitle());
			}
		}
		LLFloater::draw();
	}
}

BOOL LLMultiFloater::closeAllFloaters()
{
	S32	tabToClose = 0;
	S32	lastTabCount = mTabContainer->getTabCount();
	while (tabToClose < mTabContainer->getTabCount())
	{
		LLFloater* first_floater = (LLFloater*)mTabContainer->getPanelByIndex(tabToClose);
		first_floater->close();
		if(lastTabCount == mTabContainer->getTabCount())
		{
			//Tab did not actually close, possibly due to a pending Save Confirmation dialog..
			//so try and close the next one in the list...
			tabToClose++;
		}else
		{
			//Tab closed ok.
			lastTabCount = mTabContainer->getTabCount();
		}
	}
	if( mTabContainer->getTabCount() != 0 )
		return FALSE; // Couldn't close all the tabs (pending save dialog?) so return FALSE.
	return TRUE; //else all tabs were successfully closed...
}

void LLMultiFloater::growToFit(S32 content_width, S32 content_height)
{
	S32 new_width = llmax(getRect().getWidth(), content_width + LLPANEL_BORDER_WIDTH * 2);
	S32 new_height = llmax(getRect().getHeight(), content_height + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT);

    if (isMinimized())
    {
        LLRect newrect;
        newrect.setLeftTopAndSize(getExpandedRect().mLeft, getExpandedRect().mTop, new_width, new_height);
        setExpandedRect(newrect);
    }
	else
	{
		S32 old_height = getRect().getHeight();
		reshape(new_width, new_height);
		// keep top left corner in same position
		translate(0, old_height - new_height);
	}
}

/**
  void addFloater(LLFloater* floaterp, BOOL select_added_floater)

  Adds the LLFloater pointed to by floaterp to this.
  If floaterp is already hosted by this, then it is re-added to get
  new titles, etc.
  If select_added_floater is true, the LLFloater pointed to by floaterp will
  become the selected tab in this

  Affects: mTabContainer, floaterp
**/
void LLMultiFloater::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	if (!floaterp)
	{
		return;
	}

	if (!mTabContainer)
	{
		llerrs << "Tab Container used without having been initialized." << llendl;
		return;
	}

	if (floaterp->getHost() == this)
	{
		// already hosted by me, remove
		// do this so we get updated title, etc.
		mFloaterDataMap.erase(floaterp->getHandle());
		mTabContainer->removeTabPanel(floaterp);
	}
	else if (floaterp->getHost())
	{
		// floaterp is hosted by somebody else and
		// this is adding it, so remove it from it's old host
		floaterp->getHost()->removeFloater(floaterp);
	}
	else if (floaterp->getParent() == gFloaterView)
	{
		// rehost preview floater as child panel
		gFloaterView->removeChild(floaterp);
	}

	// store original configuration
	LLFloaterData floater_data;
	floater_data.mWidth = floaterp->getRect().getWidth();
	floater_data.mHeight = floaterp->getRect().getHeight();
	floater_data.mCanMinimize = floaterp->isMinimizeable();
	floater_data.mCanResize = floaterp->isResizable();

	// remove minimize and close buttons
	floaterp->setCanMinimize(FALSE);
	floaterp->setCanResize(FALSE);
	floaterp->setCanDrag(FALSE);
	floaterp->storeRectControl();
	// avoid double rendering of floater background (makes it more opaque)
	floaterp->setBackgroundVisible(FALSE);

	if (mAutoResize)
	{
		growToFit(floater_data.mWidth, floater_data.mHeight);
	}

	//add the panel, add it to proper maps
	mTabContainer->addTabPanel(floaterp, floaterp->getShortTitle(), FALSE, onTabSelected, this, 0, FALSE, insertion_point);
	mFloaterDataMap[floaterp->getHandle()] = floater_data;

	updateResizeLimits();

	if ( select_added_floater )
	{
		mTabContainer->selectTabPanel(floaterp);
	}
	else
	{
		// reassert visible tab (hiding new floater if necessary)
		mTabContainer->selectTab(mTabContainer->getCurrentPanelIndex());
	}

	floaterp->setHost(this);
	if (isMinimized())
	{
		floaterp->setVisible(FALSE);
	}
}

/**
	BOOL selectFloater(LLFloater* floaterp)

	If the LLFloater pointed to by floaterp is hosted by this,
	then its tab is selected and returns true.  Otherwise returns false.

	Affects: mTabContainer
**/
BOOL LLMultiFloater::selectFloater(LLFloater* floaterp)
{
	return mTabContainer->selectTabPanel(floaterp);
}

// virtual
void LLMultiFloater::selectNextFloater()
{
	mTabContainer->selectNextTab();
}

// virtual
void LLMultiFloater::selectPrevFloater()
{
	mTabContainer->selectPrevTab();
}

void LLMultiFloater::showFloater(LLFloater* floaterp)
{
	// we won't select a panel that already is selected
	// it is hard to do this internally to tab container
	// as tab selection is handled via index and the tab at a given
	// index might have changed
	if (floaterp != mTabContainer->getCurrentPanel() &&
		!mTabContainer->selectTabPanel(floaterp))
	{
		addFloater(floaterp, TRUE);
	}
}

void LLMultiFloater::removeFloater(LLFloater* floaterp)
{
	if ( floaterp->getHost() != this )
		return;

	floater_data_map_t::iterator found_data_it = mFloaterDataMap.find(floaterp->getHandle());
	if (found_data_it != mFloaterDataMap.end())
	{
		LLFloaterData& floater_data = found_data_it->second;
		floaterp->setCanMinimize(floater_data.mCanMinimize);
		if (!floater_data.mCanResize)
		{
			// restore original size
			floaterp->reshape(floater_data.mWidth, floater_data.mHeight);
		}
		floaterp->setCanResize(floater_data.mCanResize);
		mFloaterDataMap.erase(found_data_it);
	}
	mTabContainer->removeTabPanel(floaterp);
	floaterp->setBackgroundVisible(TRUE);
	floaterp->setCanDrag(TRUE);
	floaterp->setHost(NULL);
	floaterp->applyRectControl();

	updateResizeLimits();

	tabOpen((LLFloater*)mTabContainer->getCurrentPanel(), false);
}

void LLMultiFloater::tabOpen(LLFloater* opened_floater, bool from_click)
{
	// default implementation does nothing
}

void LLMultiFloater::tabClose()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// no more children, close myself
		close();
	}
}

void LLMultiFloater::setVisible(BOOL visible)
{
	// *FIX: shouldn't have to do this, fix adding to minimized multifloater
	LLFloater::setVisible(visible);
	
	if (mTabContainer)
	{
		LLPanel* cur_floaterp = mTabContainer->getCurrentPanel();

		if (cur_floaterp)
		{
			cur_floaterp->setVisible(visible);
		}

		// if no tab selected, and we're being shown,
		// select last tab to be added
		if (visible && !cur_floaterp)
		{
			mTabContainer->selectLastTab();
		}
	}
}

BOOL LLMultiFloater::handleKeyHere(KEY key, MASK mask)
{
	if (key == 'W' && mask == MASK_CONTROL)
	{
		LLFloater* floater = getActiveFloater();
		// is user closeable and is system closeable
		if (floater && floater->canClose() && floater->isCloseable())
		{
			floater->close();
		}
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

LLFloater* LLMultiFloater::getActiveFloater()
{
	return (LLFloater*)mTabContainer->getCurrentPanel();
}

S32	LLMultiFloater::getFloaterCount()
{
	return mTabContainer->getTabCount();
}

/**
	BOOL isFloaterFlashing(LLFloater* floaterp)

	Returns true if the LLFloater pointed to by floaterp
	is currently in a flashing state and is hosted by this.
	False otherwise.

	Requires: floaterp != NULL
**/
BOOL LLMultiFloater::isFloaterFlashing(LLFloater* floaterp)
{
	if ( floaterp && floaterp->getHost() == this )
		return mTabContainer->getTabPanelFlashing(floaterp);

	return FALSE;
}

/**
	BOOL setFloaterFlashing(LLFloater* floaterp, BOOL flashing)

	Sets the current flashing state of the LLFloater pointed
	to by floaterp to be the BOOL flashing if the LLFloater pointed
	to by floaterp is hosted by this.

	Requires: floaterp != NULL
**/
void LLMultiFloater::setFloaterFlashing(LLFloater* floaterp, BOOL flashing)
{
	if ( floaterp && floaterp->getHost() == this )
		mTabContainer->setTabPanelFlashing(floaterp, flashing);
}

//static
void LLMultiFloater::onTabSelected(void* userdata, bool from_click)
{
	LLMultiFloater* floaterp = (LLMultiFloater*)userdata;

	floaterp->tabOpen((LLFloater*)floaterp->mTabContainer->getCurrentPanel(), from_click);
}

void LLMultiFloater::setCanResize(BOOL can_resize)
{
	LLFloater::setCanResize(can_resize);
	if (isResizable() && mTabContainer->getTabPosition() == LLTabContainer::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
	else
	{
		mTabContainer->setRightTabBtnOffset(0);
	}
}

BOOL LLMultiFloater::postBuild()
{
	// remember any original xml minimum size
	getResizeLimits(&mOrigMinWidth, &mOrigMinHeight);

	if (mTabContainer)
	{
		return TRUE;
	}

	requires<LLTabContainer>("Preview Tabs");
	if (checkRequirements())
	{
		mTabContainer = getChild<LLTabContainer>("Preview Tabs");
		return TRUE;
	}

	return FALSE;
}

void LLMultiFloater::updateResizeLimits()
{
	// initialize minimum size constraint to the original xml values.
	S32 new_min_width = mOrigMinWidth;
	S32 new_min_height = mOrigMinHeight;
	// possibly increase minimum size constraint due to children's minimums.
	for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
	{
		LLFloater* floaterp = (LLFloater*)mTabContainer->getPanelByIndex(tab_idx);
		if (floaterp)
		{
			new_min_width = llmax(new_min_width, floaterp->getMinWidth() + LLPANEL_BORDER_WIDTH * 2);
			new_min_height = llmax(new_min_height, floaterp->getMinHeight() + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT);
		}
	}
	setResizeLimits(new_min_width, new_min_height);

	S32 cur_height = getRect().getHeight();
	S32 new_width = llmax(getRect().getWidth(), new_min_width);
	S32 new_height = llmax(getRect().getHeight(), new_min_height);

	if (isMinimized())
	{
		const LLRect& expanded = getExpandedRect();
		LLRect newrect;
		newrect.setLeftTopAndSize(expanded.mLeft, expanded.mTop, llmax(expanded.getWidth(), new_width), llmax(expanded.getHeight(), new_height));
		setExpandedRect(newrect);
	}
	else
	{
		reshape(new_width, new_height);

		// make sure upper left corner doesn't move
		translate(0, cur_height - getRect().getHeight());

		// make sure this window is visible on screen when it has been modified
		// (tab added, etc)
		gFloaterView->adjustToFitScreen(this, TRUE);
	}
}

// virtual
LLXMLNodePtr LLFloater::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML();

	node->createChild("title", TRUE)->setStringValue(getCurrentTitle());

	node->createChild("can_resize", TRUE)->setBoolValue(isResizable());

	node->createChild("can_minimize", TRUE)->setBoolValue(isMinimizeable());

	node->createChild("can_close", TRUE)->setBoolValue(isCloseable());

	node->createChild("can_drag_on_left", TRUE)->setBoolValue(isDragOnLeft());

	node->createChild("min_width", TRUE)->setIntValue(getMinWidth());

	node->createChild("min_height", TRUE)->setIntValue(getMinHeight());

	node->createChild("can_tear_off", TRUE)->setBoolValue(mCanTearOff);
	
	return node;
}

// static
LLView* LLFloater::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	std::string name("floater");
	node->getAttributeString("name", name);

	LLFloater *floaterp = new LLFloater(name);

	std::string filename;
	node->getAttributeString("filename", filename);

	if (filename.empty())
	{
		// Load from node
		floaterp->initFloaterXML(node, parent, factory);
	}
	else
	{
		// Load from file
		factory->buildFloater(floaterp, filename);
	}

	return floaterp;
}

void LLFloater::initFloaterXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory, BOOL open)	/* Flawfinder: ignore */
{
	std::string name(getName());
	std::string title(getCurrentTitle());
	std::string short_title(getShortTitle());
	std::string rect_control("");
	BOOL resizable = isResizable();
	S32 min_width = getMinWidth();
	S32 min_height = getMinHeight();
	BOOL drag_on_left = isDragOnLeft();
	BOOL minimizable = isMinimizeable();
	BOOL close_btn = isCloseable();
	LLRect rect;

	node->getAttributeString("name", name);
	node->getAttributeString("title", title);
	node->getAttributeString("short_title", short_title);
	node->getAttributeString("rect_control", rect_control);
	node->getAttributeBOOL("can_resize", resizable);
	node->getAttributeBOOL("can_minimize", minimizable);
	node->getAttributeBOOL("can_close", close_btn);
	node->getAttributeBOOL("can_drag_on_left", drag_on_left);
	node->getAttributeS32("min_width", min_width);
	node->getAttributeS32("min_height", min_height);

	if (! rect_control.empty())
	{
		setRectControl(rect_control);
	}

	createRect(node, rect, parent, LLRect());
	
	setRect(rect);
	setName(name);
	
	initFloater(title,
			resizable,
			min_width,
			min_height,
			drag_on_left,
			minimizable,
			close_btn);

	setTitle(title);
	applyTitle ();

	setShortTitle(short_title);

	BOOL can_tear_off;
	if (node->getAttributeBOOL("can_tear_off", can_tear_off))
	{
		setCanTearOff(can_tear_off);
	}
	
	initFromXML(node, parent);

	LLMultiFloater* last_host = LLFloater::getFloaterHost();
	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost((LLMultiFloater*) this);
	}

	initChildrenXML(node, factory);

	if (node->hasName("multi_floater"))
	{
		LLFloater::setFloaterHost(last_host);
	}

	BOOL result = postBuild();

	if (!result)
	{
		llerrs << "Failed to construct floater " << name << llendl;
	}

	applyRectControl();
	if (open)	/* Flawfinder: ignore */
	{
		this->open();	/* Flawfinder: ignore */
	}

	moveResizeHandlesToFront();
}
