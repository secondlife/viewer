/** 
 * @file llfloater.cpp
 * @brief LLFloater base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "llviewborder.h"
#include "llwindow.h"
#include "llstl.h"
#include "llcontrol.h"
#include "lltabcontainer.h"
#include "v2math.h"

extern BOOL gNoRender;

const S32 MINIMIZED_WIDTH = 160;
const S32 CLOSE_BOX_FROM_TOP = 1;

LLString	LLFloater::sButtonActiveImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnCloseActiveUUID",		//BUTTON_CLOSE
	"UIImgBtnRestoreActiveUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizeActiveUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffActiveUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnCloseActiveUUID",		//BUTTON_EDIT
};

LLString	LLFloater::sButtonInactiveImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnCloseInactiveUUID",	//BUTTON_CLOSE
	"UIImgBtnRestoreInactiveUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizeInactiveUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffInactiveUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnCloseInactiveUUID",	//BUTTON_EDIT
};

LLString	LLFloater::sButtonPressedImageNames[BUTTON_COUNT] = 
{
	"UIImgBtnClosePressedUUID",		//BUTTON_CLOSE
	"UIImgBtnRestorePressedUUID",	//BUTTON_RESTORE
	"UIImgBtnMinimizePressedUUID",	//BUTTON_MINIMIZE
	"UIImgBtnTearOffPressedUUID",	//BUTTON_TEAR_OFF
	"UIImgBtnClosePressedUUID",		//BUTTON_EDIT
};

LLString	LLFloater::sButtonNames[BUTTON_COUNT] = 
{
	"llfloater_close_btn",	//BUTTON_CLOSE
	"llfloater_restore_btn",	//BUTTON_RESTORE
	"llfloater_minimize_btn",	//BUTTON_MINIMIZE
	"llfloater_tear_off_btn",	//BUTTON_TEAR_OFF
	"llfloater_edit_btn",		//BUTTON_EDIT
};

LLString	LLFloater::sButtonToolTips[BUTTON_COUNT] = 
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

LLFloater::LLFloater() 
{
	// automatically take focus when opened
	mAutoFocus = TRUE;
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	mDragHandle = NULL;
}

LLFloater::LLFloater(const LLString& name)
:	LLPanel(name)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}

	LLString title; // null string
	// automatically take focus when opened
	mAutoFocus = TRUE;
	init(title, FALSE, DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT, FALSE, TRUE, TRUE); // defaults
}


LLFloater::LLFloater(const LLString& name, const LLRect& rect, const LLString& title, 
	BOOL resizable, 
	S32 min_width, 
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect, bordered)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	// automatically take focus when opened
	mAutoFocus = TRUE;
	init( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}

LLFloater::LLFloater(const LLString& name, const LLString& rect_control, const LLString& title, 
	BOOL resizable, 
	S32 min_width, 
	S32 min_height,
	BOOL drag_on_left,
	BOOL minimizable,
	BOOL close_btn,
	BOOL bordered)
:	LLPanel(name, rect_control, bordered)
{
	for (S32 i = 0; i < BUTTON_COUNT; i++)
	{
		mButtonsEnabled[i] = FALSE;
		mButtons[i] = NULL;
	}
	// automatically take focus when opened
	mAutoFocus = TRUE;
	init( title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);
}


// Note: Floaters constructed from XML call init() twice!
void LLFloater::init(const LLString& title,
					 BOOL resizable, S32 min_width, S32 min_height,
					 BOOL drag_on_left, BOOL minimizable, BOOL close_btn)
{
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

	LLPanel::deleteAllChildren();
	//sjb: HACK! we had a border which was just deleted, so re-create it
	if (mBorder != NULL)
	{
		addBorder();
	}

	// chrome floaters don't take focus at all
	mIsFocusRoot = !getIsChrome();

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
	mPreviousRect.set(0,0,0,0);
	
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
	LLRect drag_handle_rect;
	if (!drag_on_left)
	{
		drag_handle_rect.set( 0, mRect.getHeight(), mRect.getWidth(), 0 );

		/*
		drag_handle_rect.setLeftTopAndSize(
			0, mRect.getHeight(),
			mRect.getWidth() 
				- LLPANEL_BORDER_WIDTH 
				- drag_pad
				- minimize_box_size - minimize_pad 
				- close_box_size - close_pad,
			DRAG_HANDLE_HEIGHT);
			*/
		mDragHandle = new LLDragHandleTop( "Drag Handle", drag_handle_rect, title );
	}
	else
	{
		drag_handle_rect.setOriginAndSize(
			0, 0,
			DRAG_HANDLE_WIDTH,
			mRect.getHeight() - LLPANEL_BORDER_WIDTH - close_box_size);
		mDragHandle = new LLDragHandleLeft("drag", drag_handle_rect, title );
	}
	mDragHandle->setSaveToXML(false);
	addChild(mDragHandle);

	// Resize Handle
	mResizable = resizable;
	mMinWidth = min_width;
	mMinHeight = min_height;

	if( mResizable )
	{
		// Resize bars (sides)
		const S32 RESIZE_BAR_THICKNESS = 3;
		mResizeBar[0] = new LLResizeBar( 
			"resizebar_left",
			LLRect( 0, mRect.getHeight(), RESIZE_BAR_THICKNESS, 0), 
			min_width, min_height, LLResizeBar::LEFT );
		mResizeBar[0]->setSaveToXML(false);
		addChild( mResizeBar[0] );

		mResizeBar[1] = new LLResizeBar( 
			"resizebar_top",
			LLRect( 0, mRect.getHeight(), mRect.getWidth(), mRect.getHeight() - RESIZE_BAR_THICKNESS), 
			min_width, min_height, LLResizeBar::TOP );
		mResizeBar[1]->setSaveToXML(false);
		addChild( mResizeBar[1] );

		mResizeBar[2] = new LLResizeBar( 
			"resizebar_right",
			LLRect( mRect.getWidth() - RESIZE_BAR_THICKNESS, mRect.getHeight(), mRect.getWidth(), 0), 
			min_width, min_height, LLResizeBar::RIGHT );
		mResizeBar[2]->setSaveToXML(false);
		addChild( mResizeBar[2] );

		mResizeBar[3] = new LLResizeBar( 
			"resizebar_bottom",
			LLRect( 0, RESIZE_BAR_THICKNESS, mRect.getWidth(), 0), 
			min_width, min_height, LLResizeBar::BOTTOM );
		mResizeBar[3]->setSaveToXML(false);
		addChild( mResizeBar[3] );


		// Resize handles (corners)
		mResizeHandle[0] = new LLResizeHandle( 
			"Resize Handle",
			LLRect( mRect.getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, mRect.getWidth(), 0),
			min_width,
			min_height,
			LLResizeHandle::RIGHT_BOTTOM);
		mResizeHandle[0]->setSaveToXML(false);
		addChild(mResizeHandle[0]);

		mResizeHandle[1] = new LLResizeHandle( "resize", 
			LLRect( mRect.getWidth() - RESIZE_HANDLE_WIDTH, mRect.getHeight(), mRect.getWidth(), mRect.getHeight() - RESIZE_HANDLE_HEIGHT),
			min_width,
			min_height,
			LLResizeHandle::RIGHT_TOP );
		mResizeHandle[1]->setSaveToXML(false);
		addChild(mResizeHandle[1]);
		
		mResizeHandle[2] = new LLResizeHandle( "resize", 
			LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 ),
			min_width,
			min_height,
			LLResizeHandle::LEFT_BOTTOM );
		mResizeHandle[2]->setSaveToXML(false);
		addChild(mResizeHandle[2]);

		mResizeHandle[3] = new LLResizeHandle( "resize", 
			LLRect( 0, mRect.getHeight(), RESIZE_HANDLE_WIDTH, mRect.getHeight() - RESIZE_HANDLE_HEIGHT ),
			min_width,
			min_height,
			LLResizeHandle::LEFT_TOP );
		mResizeHandle[3]->setSaveToXML(false);
		addChild(mResizeHandle[3]);
	}
	else
	{
		mResizeBar[0]	= NULL;
		mResizeBar[1]	= NULL;
		mResizeBar[2]	= NULL;
		mResizeBar[3]	= NULL;

		mResizeHandle[0] = NULL;
		mResizeHandle[1] = NULL;
		mResizeHandle[2] = NULL;
		mResizeHandle[3] = NULL;
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

	buildButtons();

	// JC - Don't do this here, because many floaters first construct themselves,
	// then show themselves.  Put it in setVisibleAndFrontmost.
	// make_ui_sound("UISndWindowOpen");

	// RN: floaters are created in the invisible state	
	setVisible(FALSE);

	// add self to handle->floater map
	sFloaterMap[mViewHandle] = this;

	if (!getParent())
	{
		gFloaterView->addChild(this);
	}
}

// virtual
LLFloater::~LLFloater()
{
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

	sFloaterMap.erase(mViewHandle);

	delete mDragHandle;
	for (S32 i = 0; i < 4; i++) 
	{
		delete mResizeBar[i];
		delete mResizeHandle[i];
	}
}

// virtual
EWidgetType LLFloater::getWidgetType() const
{
	return WIDGET_TYPE_FLOATER;
}

// virtual
LLString LLFloater::getWidgetTag() const
{
	return LL_FLOATER_TAG;
}

void LLFloater::destroy()
{
	die();	
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
		if( gFocusMgr.childIsTopView( this ) )
		{
			gFocusMgr.setTopView(NULL, NULL);
		}

		if( gFocusMgr.childHasMouseCapture( this ) )
		{
			gFocusMgr.setMouseCapture(NULL, NULL);
		}
	}

	for(handle_set_iter_t dependent_it = mDependents.begin();
		dependent_it != mDependents.end(); )
	{
		LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);

		if (floaterp)
		{
			floaterp->setVisible(visible);
		}
		++dependent_it;
	}
}

LLView*	LLFloater::getRootMostFastFrameView()
{
	// trying to render a background floater in a fast frame, abort!!!
	//if (!isFrontmost())
	//{
	//	gViewerWindow->finishFastFrame();
	//}

	return LLView::getRootMostFastFrameView();
}

void LLFloater::open()	/* Flawfinder: ignore */
{
	//RN: for now, we don't allow rehosting from one multifloater to another
	// just need to fix the bugs
	LLMultiFloater* hostp = getHost();
	if (sHostp != NULL && hostp == NULL)
	{
		// needs a host
		// only select tabs if window they are hosted in is visible
		sHostp->addFloater(this, sHostp->getVisible());
	}
	else if (hostp != NULL)
	{
		// already hosted
		hostp->showFloater(this);
	}
	else
	{
		setMinimized(FALSE);
		setVisibleAndFrontmost(mAutoFocus);
	}

	if (mSoundFlags != SILENT)
	{
		if (!getVisible() || isMinimized())
		{
			make_ui_sound("UISndWindowOpen");
		}
	}
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
		}

		if (mSoundFlags != SILENT
			&& getVisible()
			&& !app_quitting)
		{
			make_ui_sound("UISndWindowClose");
		}

		// now close dependent floater
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			
			LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
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

		// Do this early, so UI controls will commit before the
		// window is taken down.
		releaseFocus();

		// give focus to dependee floater if it exists, and we had focus first
		if (isDependent())
		{
			LLFloater* dependee = LLFloater::getFloaterByHandle(mDependeeHandle);
			if (dependee && !dependee->isDead())
			{
				dependee->setFocus(TRUE);
			}
		}

		// Let floater do cleanup.
		onClose(app_quitting);
	}
}


void LLFloater::releaseFocus()
{
	if( gFocusMgr.childIsTopView( this ) )
	{
		gFocusMgr.setTopView(NULL, NULL);
	}

	if( gFocusMgr.childHasKeyboardFocus( this ) )
	{
		gFocusMgr.setKeyboardFocus(NULL, NULL);
	}

	if( gFocusMgr.childHasMouseCapture( this ) )
	{
		gFocusMgr.setMouseCapture(NULL, NULL);
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
			mResizeBar[i]->setResizeLimits( min_width, min_height );
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
	const LLRect &window = gFloaterView->getRect();

	S32 left   = window.mLeft + (window.getWidth() - mRect.getWidth()) / 2;
	S32 bottom = window.mBottom + (window.getHeight() - mRect.getHeight()) / 2;

	translate( left - mRect.mLeft, bottom - mRect.mBottom );
}

void LLFloater::applyRectControl()
{
	if (!mRectControl.empty())
	{
		const LLRect& rect = LLUI::sConfigGroup->getRect(mRectControl);
		translate( rect.mLeft - mRect.mLeft, rect.mBottom - mRect.mBottom);
		if (mResizable)
		{
			reshape(llmax(mMinWidth, rect.getWidth()), llmax(mMinHeight, rect.getHeight()));
		}
	}
}

void LLFloater::setTitle( const LLString& title )
{
	if (gNoRender)
	{
		return;
	}
	mDragHandle->setTitle( title );
}

const LLString& LLFloater::getTitle() const
{
	return mDragHandle ? mDragHandle->getTitle() : LLString::null;
}

void LLFloater::translate(S32 x, S32 y)
{
	LLView::translate(x, y);

	if (x != 0 || y != 0)
	{
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); ++dependent_it)
		{
			LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
			// is a dependent snapped to us?
			if (floaterp && floaterp->getSnapTarget() == mViewHandle)
			{
				floaterp->translate(x, y);
			}
		}
	}
}

BOOL LLFloater::canSnapTo(LLView* other_view)
{
	if (other_view && other_view != getParent())
	{
		LLFloater* other_floaterp = (LLFloater*)other_view;
		
		if (other_floaterp->getSnapTarget() == mViewHandle && mDependents.find(other_floaterp->getHandle()) != mDependents.end())
		{
			// this is a dependent that is already snapped to us, so don't snap back to it
			return FALSE;
		}
	}

	return LLView::canSnapTo(other_view);
}

void LLFloater::snappedTo(LLView* snap_view)
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

void LLFloater::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	S32 old_width = mRect.getWidth();
	S32 old_height = mRect.getHeight();

	LLView::reshape(width, height, called_from_parent);

	if (width != old_width || height != old_height)
	{
		// gather all snapped dependents
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); ++dependent_it)
		{
			LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
			// is a dependent snapped to us?
			if (floaterp && floaterp->getSnapTarget() == mViewHandle)
			{
				S32 delta_x = 0;
				S32 delta_y = 0;
				// check to see if it snapped to right or top
				LLRect floater_rect = floaterp->getRect();
				if (floater_rect.mLeft - mRect.mLeft >= old_width ||
					floater_rect.mRight == mRect.mLeft + old_width)
				{
					// was snapped directly onto right side or aligned with it
					delta_x += width - old_width;
				}
				if (floater_rect.mBottom - mRect.mBottom >= old_height ||
					floater_rect.mTop == mRect.mBottom + old_height)
				{
					// was snapped directly onto top side or aligned with it
					delta_y += height - old_height;
				}

				floaterp->translate(delta_x, delta_y);
			}
		}
	}
}

void LLFloater::setMinimized(BOOL minimize)
{
	if (minimize == mMinimized) return;

	if (minimize)
	{
		mMinimized = TRUE;

		mPreviousRect = mRect;

		reshape( MINIMIZED_WIDTH, LLFLOATER_HEADER_SIZE, TRUE);

		S32 left, bottom;
		gFloaterView->getMinimizePosition(&left, &bottom);
		setOrigin( left, bottom );

		if (mButtonsEnabled[BUTTON_MINIMIZE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = FALSE;
			mButtonsEnabled[BUTTON_RESTORE] = TRUE;
		}

		mMinimizedHiddenChildren.clear();
		// hide all children
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			if (!viewp->getVisible())
			{
				mMinimizedHiddenChildren.push_back(viewp);
			}
			viewp->setVisible(FALSE);
		}

		// except the special controls
		if (mDragHandle)
		{
			mDragHandle->setVisible(TRUE);
		}

		setBorderVisible(TRUE);

		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
			if (floaterp)
			{
				floaterp->setVisible(FALSE);
			}
			++dependent_it;
		}

		// Lose keyboard focus when minimized
		releaseFocus();
	}
	else
	{
		reshape( mPreviousRect.getWidth(), mPreviousRect.getHeight(), TRUE );
		setOrigin( mPreviousRect.mLeft, mPreviousRect.mBottom );

		mMinimized = FALSE;

		if (mButtonsEnabled[BUTTON_RESTORE])
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
			mButtonsEnabled[BUTTON_RESTORE] = FALSE;
		}

		// show all children
		for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
		{
			LLView* viewp = *child_it;
			viewp->setVisible(TRUE);
		}

		std::vector<LLView*>::iterator itor = mMinimizedHiddenChildren.begin();
		while (itor != mMinimizedHiddenChildren.end())
		{
			(*itor)->setVisible(FALSE);
			++itor;
		}
		mMinimizedHiddenChildren.clear();

		// show dependent floater
		for(handle_set_iter_t dependent_it = mDependents.begin();
			dependent_it != mDependents.end(); )
		{
			LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
			if (floaterp)
			{
				floaterp->setVisible(TRUE);
			}
			++dependent_it;
		}
	}
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

void LLFloater::setIsChrome(BOOL is_chrome)
{
	// chrome floaters don't take focus at all
	if (is_chrome)
	{
		// remove focus if we're changing to chrome
		setFocus(FALSE);
		// can't Ctrl-Tab to "chrome" floaters
		mIsFocusRoot = FALSE;
	}
	
	// no titles displayed on "chrome" floaters
	mDragHandle->setTitleVisible(!is_chrome);
	
	LLPanel::setIsChrome(is_chrome);
}

// Change the draw style to account for the foreground state.
void LLFloater::setForeground(BOOL front)
{
	if (front != mForeground)
	{
		mForeground = front;
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
		LLFloater* floaterp = LLFloater::getFloaterByHandle(*dependent_it);
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

		mIsFocusRoot = FALSE;
	}
	else if (!mHostHandle.isDead() && !host)
	{
		mButtonScale = 1.f;
		mIsFocusRoot = TRUE;
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

void LLFloater::moveResizeHandleToFront()
{
	// 0 is the bottom  right
	if( mResizeHandle[0] )
	{
		sendChildToFront(mResizeHandle[0]);
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
		floaterp->setSnapTarget(mViewHandle);
	}
	gFloaterView->adjustToFitScreen(floaterp, FALSE);
	if (floaterp->isFrontmost())
	{
		// make sure to bring self and sibling floaters to front
		gFloaterView->bringToFront(floaterp);
	}
}

void LLFloater::addDependentFloater(LLViewHandle dependent, BOOL reposition)
{
	LLFloater* dependent_floaterp = LLFloater::getFloaterByHandle(dependent);
	if(dependent_floaterp)
	{
		addDependentFloater(dependent_floaterp, reposition);
	}
}

void LLFloater::removeDependentFloater(LLFloater* floaterp)
{
	mDependents.erase(floaterp->getHandle());
	floaterp->mDependeeHandle = LLViewHandle::sDeadHandle;
}

// virtual
BOOL LLFloater::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if( mMinimized )
	{
		// Offer the click to the close button.
		// Any other click = restore
		if( mButtonsEnabled[BUTTON_CLOSE] )
		{
			S32 local_x = x - mButtons[BUTTON_CLOSE]->getRect().mLeft;
			S32 local_y = y - mButtons[BUTTON_CLOSE]->getRect().mBottom;

			if (mButtons[BUTTON_CLOSE]->pointInView(local_x, local_y)
				&& mButtons[BUTTON_CLOSE]->handleMouseDown(local_x, local_y, mask))
			{
				// close button handled it, return
				return TRUE;
			}
		}

		// restore
		bringToFront( x, y );
		return TRUE;
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

// static
LLFloater*	LLFloater::getFloaterByHandle(LLViewHandle handle)
{
	LLFloater* floater = NULL;
	if (sFloaterMap.count(handle))
	{
		floater = sFloaterMap[handle];
	}
	if (floater && !floater->isDead())
	{
		return floater;
	}
	else
	{
		return NULL;
	}
}

//static
void LLFloater::setEditModeEnabled(BOOL enable)
{
	if (enable != sEditModeEnabled)
	{
		S32 count = 0;
		std::map<LLViewHandle, LLFloater*>::iterator iter;
		for(iter = sFloaterMap.begin(); iter != sFloaterMap.end(); ++iter)
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

//static
BOOL LLFloater::getEditModeEnabled()
{
	return sEditModeEnabled;
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

		new_rect.setLeftTopAndSize(host_floater->getRect().mLeft + 5, host_floater->getRect().mTop - LLFLOATER_HEADER_SIZE - 5, self->mRect.getWidth(), self->mRect.getHeight());

		self->open();	/* Flawfinder: ignore */
		self->setRect(new_rect);
		gFloaterView->adjustToFitScreen(self, FALSE);
		self->setCanDrag(TRUE);		
		self->setCanResize(TRUE);		
		self->setCanMinimize(TRUE);
	}
	else  //Attach to parent.
	{
		LLMultiFloater* new_host = (LLMultiFloater*)LLFloater::getFloaterByHandle(self->mLastHostHandle);
		if (new_host)
		{
			new_host->showFloater(self);
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
void LLFloater::closeByMenu( void* userdata )
{
	LLFloater* self = (LLFloater*) userdata;
	if (!self || self->getHost()) return;

	LLFloaterView* parent = (LLFloaterView*) self->getParent();

	// grab focus status before close just in case floater is deleted
	BOOL has_focus = gFocusMgr.childHasKeyboardFocus(self);
	self->close();

	// if this floater used to have focus and now nothing took focus
	// give it to next floater (to allow closing multiple windows via keyboard in rapid succession)
	if (has_focus && gFocusMgr.getKeyboardFocus() == NULL)
	{
		parent->focusFrontFloater();
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
	if( getVisible() )
	{
		// draw background
		if( mBgVisible )
		{
			S32 left = LLPANEL_BORDER_WIDTH;
			S32 top = mRect.getHeight() - LLPANEL_BORDER_WIDTH;
			S32 right = mRect.getWidth() - LLPANEL_BORDER_WIDTH;
			S32 bottom = LLPANEL_BORDER_WIDTH;

			LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
			F32 shadow_offset = (F32)LLUI::sConfigGroup->getS32("DropShadowFloater");
			if (!mBgOpaque)
			{
				shadow_offset *= 0.2f;
				shadow_color.mV[VALPHA] *= 0.5f;
			}
			gl_drop_shadow(left, top, right, bottom, 
				shadow_color, 
				llround(shadow_offset));

			// No transparent windows in simple UI
			if (mBgOpaque)
			{
				gl_rect_2d( left, top, right, bottom, mBgColorOpaque );
			}
			else
			{
				gl_rect_2d( left, top, right, bottom, mBgColorAlpha );
			}

			if(gFocusMgr.childHasKeyboardFocus(this) && !getIsChrome() && !getTitle().empty())
			{
				// draw highlight on title bar to indicate focus.  RDW
				const LLFontGL* font = gResMgr->getRes( LLFONT_SANSSERIF );
				LLRect r = getRect();
				gl_rect_2d_offset_local(0, r.getHeight(), r.getWidth(), r.getHeight() - (S32)font->getLineHeight() - 1, 
					LLUI::sColorsGroup->getColor("TitleBarFocusColor"), 0, TRUE);
			}
		}

		if( mDefaultBtn)
		{
			if (gFocusMgr.childHasKeyboardFocus( this ) && mDefaultBtn->getEnabled())
			{
				LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
				// is this button a direct descendent and not a nested widget (e.g. checkbox)?
				BOOL focus_is_child_button = focus_ctrl->getWidgetType() == WIDGET_TYPE_BUTTON && focus_ctrl->getParent() == this;
				// only enable default button when current focus is not a button
				mDefaultBtn->setBorderEnabled(!focus_is_child_button);
			}
			else
			{
				mDefaultBtn->setBorderEnabled(FALSE);
			}
		}

		// draw children
		LLView* focused_child = gFocusMgr.getKeyboardFocus();
		BOOL focused_child_visible = FALSE;
		if (focused_child && focused_child->getParent() == this)
		{
			focused_child_visible = focused_child->getVisible();
			focused_child->setVisible(FALSE);
		}

		LLView::draw();

		if( mBgVisible )
		{
			// add in a border to improve spacialized visual aclarity ;)
			// use lines instead of gl_rect_2d so we can round the edges as per james' recommendation
			LLUI::setLineWidth(1.5f);
			LLColor4 outlineColor = gFocusMgr.childHasKeyboardFocus(this) ? LLUI::sColorsGroup->getColor("FloaterFocusBorderColor") : LLUI::sColorsGroup->getColor("FloaterUnfocusBorderColor");
			gl_rect_2d_offset_local(0, mRect.getHeight() + 1, mRect.getWidth() + 1, 0, outlineColor, -LLPANEL_BORDER_WIDTH, FALSE);
			LLUI::setLineWidth(1.f);
		}
	
		if (focused_child_visible)
		{
			focused_child->setVisible(TRUE);
		}
		drawChild(focused_child);
	}
}

// virtual
void LLFloater::onClose(bool app_quitting)
{
	destroy();
}

// virtual
BOOL LLFloater::canClose()
{
	return TRUE;
}

// virtual
BOOL LLFloater::canSaveAs()
{
	return FALSE;
}

// virtual
void LLFloater::saveAs()
{
}

void	LLFloater::setCanMinimize(BOOL can_minimize)
{
	// removing minimize/restore button programmatically,
	// go ahead and uniminimize floater
	if (!can_minimize)
	{
		setMinimized(FALSE);
	}

	if (can_minimize)
	{
		if (isMinimized())
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = FALSE;
			mButtonsEnabled[BUTTON_RESTORE] = TRUE;
		}
		else
		{
			mButtonsEnabled[BUTTON_MINIMIZE] = TRUE;
			mButtonsEnabled[BUTTON_RESTORE] = FALSE;
		}
	}
	else
	{
		mButtonsEnabled[BUTTON_MINIMIZE] = FALSE;
		mButtonsEnabled[BUTTON_RESTORE] = FALSE;
	}

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
		removeChild(mResizeBar[0]);
		removeChild(mResizeBar[1]);
		removeChild(mResizeBar[2]);
		removeChild(mResizeBar[3]);
		removeChild(mResizeHandle[0]);
		removeChild(mResizeHandle[1]);
		removeChild(mResizeHandle[2]);
		removeChild(mResizeHandle[3]);
		delete mResizeBar[0];
		delete mResizeBar[1];
		delete mResizeBar[2];
		delete mResizeBar[3];
		delete mResizeHandle[0];
		delete mResizeHandle[1];
		delete mResizeHandle[2];
		mResizeHandle[3] = NULL;
		mResizeBar[0] = NULL;
		mResizeBar[1] = NULL;
		mResizeBar[2] = NULL;
		mResizeBar[3] = NULL;
		mResizeHandle[0] = NULL;
		mResizeHandle[1] = NULL;
		mResizeHandle[2] = NULL;
		mResizeHandle[3] = NULL;
	}
	else if (!mResizable && can_resize)
	{
		// Resize bars (sides)
		const S32 RESIZE_BAR_THICKNESS = 3;
		mResizeBar[0] = new LLResizeBar( 
			"resizebar_left",
			LLRect( 0, mRect.getHeight(), RESIZE_BAR_THICKNESS, 0), 
			mMinWidth, mMinHeight, LLResizeBar::LEFT );
		mResizeBar[0]->setSaveToXML(false);
		addChild( mResizeBar[0] );

		mResizeBar[1] = new LLResizeBar( 
			"resizebar_top",
			LLRect( 0, mRect.getHeight(), mRect.getWidth(), mRect.getHeight() - RESIZE_BAR_THICKNESS), 
			mMinWidth, mMinHeight, LLResizeBar::TOP );
		mResizeBar[1]->setSaveToXML(false);
		addChild( mResizeBar[1] );

		mResizeBar[2] = new LLResizeBar( 
			"resizebar_right",
			LLRect( mRect.getWidth() - RESIZE_BAR_THICKNESS, mRect.getHeight(), mRect.getWidth(), 0), 
			mMinWidth, mMinHeight, LLResizeBar::RIGHT );
		mResizeBar[2]->setSaveToXML(false);
		addChild( mResizeBar[2] );

		mResizeBar[3] = new LLResizeBar( 
			"resizebar_bottom",
			LLRect( 0, RESIZE_BAR_THICKNESS, mRect.getWidth(), 0), 
			mMinWidth, mMinHeight, LLResizeBar::BOTTOM );
		mResizeBar[3]->setSaveToXML(false);
		addChild( mResizeBar[3] );


		// Resize handles (corners)
		mResizeHandle[0] = new LLResizeHandle( 
			"Resize Handle",
			LLRect( mRect.getWidth() - RESIZE_HANDLE_WIDTH, RESIZE_HANDLE_HEIGHT, mRect.getWidth(), 0),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::RIGHT_BOTTOM);
		mResizeHandle[0]->setSaveToXML(false);
		addChild(mResizeHandle[0]);

		mResizeHandle[1] = new LLResizeHandle( "resize", 
			LLRect( mRect.getWidth() - RESIZE_HANDLE_WIDTH, mRect.getHeight(), mRect.getWidth(), mRect.getHeight() - RESIZE_HANDLE_HEIGHT),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::RIGHT_TOP );
		mResizeHandle[1]->setSaveToXML(false);
		addChild(mResizeHandle[1]);
		
		mResizeHandle[2] = new LLResizeHandle( "resize", 
			LLRect( 0, RESIZE_HANDLE_HEIGHT, RESIZE_HANDLE_WIDTH, 0 ),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::LEFT_BOTTOM );
		mResizeHandle[2]->setSaveToXML(false);
		addChild(mResizeHandle[2]);

		mResizeHandle[3] = new LLResizeHandle( "resize", 
			LLRect( 0, mRect.getHeight(), RESIZE_HANDLE_WIDTH, mRect.getHeight() - RESIZE_HANDLE_HEIGHT ),
			mMinWidth,
			mMinHeight,
			LLResizeHandle::LEFT_TOP );
		mResizeHandle[3]->setSaveToXML(false);
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
		if (mButtonsEnabled[i])
		{
			button_count++;

			LLRect btn_rect;
			if (mDragOnLeft)
			{
				btn_rect.setLeftTopAndSize(
					LLPANEL_BORDER_WIDTH,
					mRect.getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * button_count,
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}
			else
			{
				btn_rect.setLeftTopAndSize(
					mRect.getWidth() - LLPANEL_BORDER_WIDTH - (LLFLOATER_CLOSE_BOX_SIZE + 1) * button_count,
					mRect.getHeight() - CLOSE_BOX_FROM_TOP,
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
					llround((F32)LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
			}

			mButtons[i]->setRect(btn_rect);
			mButtons[i]->setVisible(TRUE);
			mButtons[i]->setEnabled(TRUE);
			// the restore button should have a tab stop so that it takes action when you Ctrl-Tab to a minimized floater
			mButtons[i]->setTabStop(i == BUTTON_RESTORE);
		}
		else if (mButtons[i])
		{
			mButtons[i]->setVisible(FALSE);
			mButtons[i]->setEnabled(FALSE);
		}
	}

	mDragHandle->setMaxTitleWidth(mRect.getWidth() - (button_count * (LLFLOATER_CLOSE_BOX_SIZE + 1)));
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
				mRect.getHeight() - CLOSE_BOX_FROM_TOP - (LLFLOATER_CLOSE_BOX_SIZE + 1) * (i + 1),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}
		else
		{
			btn_rect.setLeftTopAndSize(
				mRect.getWidth() - LLPANEL_BORDER_WIDTH - (LLFLOATER_CLOSE_BOX_SIZE + 1) * (i + 1),
				mRect.getHeight() - CLOSE_BOX_FROM_TOP,
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale),
				llround(LLFLOATER_CLOSE_BOX_SIZE * mButtonScale));
		}

		LLButton* buttonp = new LLButton(
			sButtonNames[i],
			btn_rect,
			sButtonActiveImageNames[i],
			sButtonPressedImageNames[i],
			"",
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

LLFloaterView::LLFloaterView( const LLString& name, const LLRect& rect )
:	LLUICtrl( name, rect, FALSE, NULL, NULL, FOLLOWS_ALL ),
	mFocusCycleMode(FALSE),
	mSnapOffsetBottom(0)
{
	setTabStop(FALSE);
	resetStartingFloaterPosition();
}

EWidgetType LLFloaterView::getWidgetType() const
{
	return WIDGET_TYPE_FLOATER_VIEW;
}

LLString LLFloaterView::getWidgetTag() const
{
	return LL_FLOATER_VIEW_TAG;
}

// By default, adjust vertical.
void LLFloaterView::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	reshape(width, height, called_from_parent, ADJUST_VERTICAL_YES);
}

// When reshaping this view, make the floaters follow their closest edge.
void LLFloaterView::reshape(S32 width, S32 height, BOOL called_from_parent, BOOL adjust_vertical)
{
	S32 old_width = mRect.getWidth();
	S32 old_height = mRect.getHeight();

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
			LLFloater* dependent_floaterp = getFloaterByHandle(*dependent_it);
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
		LLFloater* sibling = LLFloater::getFloaterByHandle(*dependent_it);
		// check for dependents within 10 pixels of base floater
		if (sibling && 
			sibling != neighbor && 
			sibling->getVisible() && 
			expanded_base_rect.rectInRect(&sibling->getRect()))
		{
			base_rect |= sibling->getRect();
		}
	}

	S32 left_margin = llmax(0, base_rect.mLeft);
	S32 right_margin = llmax(0, mRect.getWidth() - base_rect.mRight);
	S32 top_margin = llmax(0, mRect.getHeight() - base_rect.mTop);
	S32 bottom_margin = llmax(0, base_rect.mBottom);

	// find position for floater in following order
	// right->left->bottom->top
	for (S32 i = 0; i < 5; i++)
	{
		if (right_margin > width)
		{
			new_rect.translate(base_rect.mRight - neighbor->mRect.mLeft, base_rect.mTop - neighbor->mRect.mTop);
			return new_rect;
		}
		else if (left_margin > width)
		{
			new_rect.translate(base_rect.mLeft - neighbor->mRect.mRight, base_rect.mTop - neighbor->mRect.mTop);
			return new_rect;
		}
		else if (bottom_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->mRect.mLeft, base_rect.mBottom - neighbor->mRect.mTop);
			return new_rect;
		}
		else if (top_margin > height)
		{
			new_rect.translate(base_rect.mLeft - neighbor->mRect.mLeft, base_rect.mTop - neighbor->mRect.mBottom);
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

void LLFloaterView::setCycleMode(BOOL mode)
{
	mFocusCycleMode = mode;
}

BOOL LLFloaterView::getCycleMode()
{
	return mFocusCycleMode;
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
					LLFloater* sibling = LLFloater::getFloaterByHandle(*dependent_it);
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

		floaterp->setMinimized(FALSE);
	}
	floaters_to_move.clear();

	// ...then bringing my own dependents to the front...
	for(LLFloater::handle_set_iter_t dependent_it = child->mDependents.begin();
		dependent_it != child->mDependents.end(); )
	{
		LLFloater* dependent = getFloaterByHandle(*dependent_it);
		if (dependent)
		{
			sendChildToFront(dependent);
			dependent->setMinimized(FALSE);
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
			LLFloater* dependent_floaterp = getFloaterByHandle(*dependent_it);
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
			LLFloater* dependent_floaterp = getFloaterByHandle(*dependent_it);
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
	// count the number of minimized children
	S32 count = 0;
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		LLFloater *floater = (LLFloater *)viewp;
		if (floater->isMinimized())
		{
			count++;
		}
	}

	// space over for that many and up if necessary
	S32 tiles_per_row = mRect.getWidth() / MINIMIZED_WIDTH;

	*left = (count % tiles_per_row) * MINIMIZED_WIDTH;
	*bottom = (count / tiles_per_row) * LLFLOATER_HEADER_SIZE;
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
		if (floaterp->canClose())
		{
			floaterp->close(app_quitting);
		}
	}
}


BOOL LLFloaterView::allChildrenClosed()
{
	// see if there are any visible floaters (some floaters "close"
	// by setting themselves invisible)
	S32 visible_count = 0;
	for (child_list_const_iter_t it = getChildList()->begin(); it != getChildList()->end(); ++it)
	{
		LLView* viewp = *it;
		LLFloater* floaterp = (LLFloater*)viewp;

		if (floaterp->getVisible() && floaterp->canClose())
		{
			visible_count++;
		}
	}

	return (visible_count == 0);
}


void LLFloaterView::refresh()
{
	// Constrain children to be entirely on the screen
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLFloater* floaterp = (LLFloater*)*child_it;
		if( floaterp->getVisible() )
		{
			adjustToFitScreen(floaterp, TRUE);
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
	LLRect snap_rect_local = getSnapRect();
	snap_rect_local.translate(-mRect.mLeft, -mRect.mBottom);

	if( floater->isResizable() )
	{
		LLRect view_rect = floater->getRect();
		S32 view_width = view_rect.getWidth();
		S32 view_height = view_rect.getHeight();
		S32 min_width;
		S32 min_height;
		floater->getResizeLimits( &min_width, &min_height );

		S32 new_width = llmax( min_width, view_width );
		S32 new_height = llmax( min_height, view_height );

		if( (new_width > screen_width) || (new_height > screen_height) )
		{
			new_width = llmin(new_width, screen_width);
			new_height = llmin(new_height, screen_height);

			floater->reshape( new_width, new_height, TRUE );

			// Make sure the damn thing is actually onscreen.
			if (floater->translateIntoRect(snap_rect_local, FALSE))
			{
				floater->clearSnapTarget();
			}
		}
		else if (!floater->isMinimized())
		{
			floater->reshape(new_width, new_height, TRUE);
		}
	}

	if (floater->translateIntoRect( snap_rect_local, allow_partial_outside ))
	{
		floater->clearSnapTarget();
	}
}

void LLFloaterView::draw()
{
	if( getVisible() )
	{
		refresh();

		// hide focused floater if in cycle mode, so that it can be drawn on top
		LLFloater* focused_floater = getFocusedFloater();
		BOOL floater_visible = FALSE;
		if (mFocusCycleMode && focused_floater)
		{
			floater_visible = focused_floater->getVisible();
			focused_floater->setVisible(FALSE);
		}

		// And actually do the draw
		LLView::draw();

		// manually draw focused floater on top when in cycle mode
		if (mFocusCycleMode && focused_floater)
		{
			// draw focused item on top for better feedback
			focused_floater->setVisible(floater_visible);
			if (floater_visible)
			{
				drawChild(focused_floater);
			}
		}
	}
}

const LLRect LLFloaterView::getSnapRect() const
{
	LLRect snap_rect = mRect;
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

LLFloater* LLFloaterView::getFloaterByHandle(LLViewHandle handle)
{
	if (handle == LLViewHandle::sDeadHandle)
	{
		return NULL;
	}
	for ( child_list_const_iter_t child_it = getChildList()->begin(); child_it != getChildList()->end(); ++child_it)
	{
		LLView* viewp = *child_it;
		if (((LLFloater*)viewp)->getHandle() == handle)
		{
			return (LLFloater*)viewp;
		}
	}
	return NULL;
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
	mTabPos(LLTabContainerCommon::TOP),
	mAutoResize(FALSE)
{

}

LLMultiFloater::LLMultiFloater(LLTabContainerCommon::TabPosition tab_pos) :
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(FALSE)
{

}

LLMultiFloater::LLMultiFloater(const LLString &name) :
	LLFloater(name),
	mTabContainer(NULL),
	mTabPos(LLTabContainerCommon::TOP),
	mAutoResize(FALSE)
{
}

LLMultiFloater::LLMultiFloater(
	const LLString& name,
	const LLRect& rect,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) : 
	LLFloater(name, rect, name),
	mTabContainer(NULL),
	mTabPos(LLTabContainerCommon::TOP),
	mAutoResize(auto_resize)
{
	mTabContainer = new LLTabContainer("Preview Tabs", 
		LLRect(LLPANEL_BORDER_WIDTH, mRect.getHeight() - LLFLOATER_HEADER_SIZE, mRect.getWidth() - LLPANEL_BORDER_WIDTH, 0), 
		mTabPos, 
		NULL, 
		NULL);
	mTabContainer->setFollowsAll();
	if (mResizable)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}

	addChild(mTabContainer);
}

LLMultiFloater::LLMultiFloater(
	const LLString& name,
	const LLString& rect_control,
	LLTabContainer::TabPosition tab_pos,
	BOOL auto_resize) : 
	LLFloater(name, rect_control, name),
	mTabContainer(NULL),
	mTabPos(tab_pos),
	mAutoResize(auto_resize)
{
	mTabContainer = new LLTabContainer("Preview Tabs", 
	LLRect(LLPANEL_BORDER_WIDTH, mRect.getHeight() - LLFLOATER_HEADER_SIZE, mRect.getWidth() - LLPANEL_BORDER_WIDTH, 0), 
	mTabPos, 
	NULL, 
	NULL);
	mTabContainer->setFollowsAll();
	if (mResizable && mTabPos == LLTabContainerCommon::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}

	addChild(mTabContainer);
	
}

LLMultiFloater::~LLMultiFloater()
{
}

// virtual
EWidgetType LLMultiFloater::getWidgetType() const
{
	return WIDGET_TYPE_MULTI_FLOATER;
}

// virtual
LLString LLMultiFloater::getWidgetTag() const
{
	return LL_MULTI_FLOATER_TAG;
}

void LLMultiFloater::init(const LLString& title, BOOL resizable, 
						S32 min_width, S32 min_height, BOOL drag_on_left,
						BOOL minimizable, BOOL close_btn)
{
	LLFloater::init(title, resizable, min_width, min_height, drag_on_left, minimizable, close_btn);

	/*mTabContainer = new LLTabContainer("Preview Tabs", 
		LLRect(LLPANEL_BORDER_WIDTH, mRect.getHeight() - LLFLOATER_HEADER_SIZE, mRect.getWidth() - LLPANEL_BORDER_WIDTH, 0), 
		mTabPos, 
		NULL, 
		NULL);
	mTabContainer->setFollowsAll();
	if (mResizable && mTabPos == LLTabContainerCommon::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}

	addChild(mTabContainer);*/
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
		LLFloater::onClose(app_quitting ? true : false);
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
			if (floaterp->getTitle() != mTabContainer->getPanelTitle(i))
			{
				mTabContainer->setPanelTitle(i, floaterp->getTitle());
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

void LLMultiFloater::growToFit(LLFloater* floaterp, S32 width, S32 height)
{
	floater_data_map_t::iterator found_data_it;
	found_data_it = mFloaterDataMap.find(floaterp->getHandle());
	if (found_data_it != mFloaterDataMap.end())
	{
		// store new width and height with this floater so that it will keep its size when detached
		found_data_it->second.mWidth = width;
		found_data_it->second.mHeight = height;

		S32 cur_height = mRect.getHeight();
		reshape(llmax(mRect.getWidth(), width + LLPANEL_BORDER_WIDTH * 2), llmax(mRect.getHeight(), height + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT + (LLPANEL_BORDER_WIDTH * 2)));
		
		// make sure upper left corner doesn't move
		translate(0, mRect.getHeight() - cur_height);

		// Try to keep whole view onscreen, don't allow partial offscreen.
		gFloaterView->adjustToFitScreen(this, FALSE);
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

	S32 new_width = llmax(mRect.getWidth(), floaterp->getRect().getWidth()); 
	S32 new_height = llmax(mRect.getHeight(), floaterp->getRect().getHeight() + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT); 

	reshape(new_width, new_height);

	//add the panel, add it to proper maps
	mTabContainer->addTabPanel(floaterp, floaterp->getTitle(), FALSE, onTabSelected, this, 0, FALSE, insertion_point);
	mFloaterDataMap[floaterp->getHandle()] = floater_data;

	if ( select_added_floater )
	{
		mTabContainer->selectLastTab();
	}

	floaterp->setHost(this);
	if (mMinimized)
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
	setVisibleAndFrontmost();
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
	floaterp->setHost(NULL);

	if (mAutoResize)
	{
		floater_data_map_t::iterator floater_it;
		S32 new_width = 0;
		S32 new_height = 0;
		for (floater_it = mFloaterDataMap.begin(); floater_it != mFloaterDataMap.end(); ++floater_it)
		{
			new_width = llmax(new_width, floater_it->second.mWidth + LLPANEL_BORDER_WIDTH * 2);
			new_height = llmax(new_height, floater_it->second.mHeight + LLFLOATER_HEADER_SIZE + TABCNTR_HEADER_HEIGHT);
		}	

		S32 cur_height = mRect.getHeight();

		reshape(new_width, new_height);

		// make sure upper left corner doesn't move
		translate(0, cur_height - new_height);

		// Try to keep whole view onscreen, don't allow partial offscreen.
		gFloaterView->adjustToFitScreen(this, FALSE);
	}

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

BOOL LLMultiFloater::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if (getEnabled()
		&& mask == (MASK_CONTROL|MASK_SHIFT))
	{
		if (key == 'W')
		{
			LLFloater* floater = getActiveFloater();
			if (floater && floater->canClose())
			{
				floater->close();
			}
			return TRUE;
		}
	}

	return LLFloater::handleKeyHere(key, mask, called_from_parent);
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
	if (mResizable && mTabContainer->getTabPosition() == LLTabContainer::BOTTOM)
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
	if (mTabContainer)
	{
		return TRUE;
	}

	requires("Preview Tabs", WIDGET_TYPE_TAB_CONTAINER);
	if (checkRequirements())
	{
		mTabContainer = LLUICtrlFactory::getTabContainerByName(this, "Preview Tabs");
		return TRUE;
	}

	return FALSE;
}

// virtual
LLXMLNodePtr LLFloater::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLPanel::getXML();

	node->createChild("title", TRUE)->setStringValue(getTitle());

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
	LLString name("floater");
	node->getAttributeString("name", name);

	LLFloater *floaterp = new LLFloater(name);

	LLString filename;
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
	LLString name(getName());
	LLString title(getTitle());
	LLString rect_control("");
	BOOL resizable = isResizable();
	S32 min_width = getMinWidth();
	S32 min_height = getMinHeight();
	BOOL drag_on_left = isDragOnLeft();
	BOOL minimizable = isMinimizeable();
	BOOL close_btn = isCloseable();
	LLRect rect;

	node->getAttributeString("name", name);
	node->getAttributeString("title", title);
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
	
	init(title,
			resizable,
			min_width,
			min_height,
			drag_on_left,
			minimizable,
			close_btn);

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

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		factory->createWidget(this, child);
	}
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
}
