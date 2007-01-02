/** 
 * @file llmodaldialog.cpp
 * @brief LLModalDialog base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llmodaldialog.h"

#include "llfocusmgr.h"
#include "v4color.h"
#include "v2math.h"
#include "llui.h"
#include "llwindow.h"
#include "llkeyboard.h"

// static
std::list<LLModalDialog*> LLModalDialog::sModalStack;

LLModalDialog::LLModalDialog( const LLString& title, S32 width, S32 height, BOOL modal )
	: LLFloater( "modal container",
				 LLRect( 0, height, width, 0 ),
				 title,
				 FALSE, // resizable
				 DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT,
				 FALSE, // drag_on_left
				 modal ? FALSE : TRUE, // minimizable
				 modal ? FALSE : TRUE, // close button
				 TRUE), // bordered
	  mModal( modal )
{
	setVisible( FALSE );
	setBackgroundVisible(TRUE);
	setBackgroundOpaque(TRUE);
	centerOnScreen(); // default position
}

LLModalDialog::~LLModalDialog()
{
	// don't unlock focus unless we have it
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		gFocusMgr.unlockFocus();
	}
}

void LLModalDialog::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLFloater::reshape(width, height, called_from_parent);
	centerOnScreen();
}

void LLModalDialog::startModal()
{
	if (mModal)
	{
		// If Modal, Hide the active modal dialog
		if (!sModalStack.empty())
		{
			LLModalDialog* front = sModalStack.front();
			front->setVisible(FALSE);
		}
	
		// This is a modal dialog.  It sucks up all mouse and keyboard operations.
		gFocusMgr.setMouseCapture( this, NULL );
		gFocusMgr.setTopView( this, NULL );
		setFocus(TRUE);

		sModalStack.push_front( this );
	}

	setVisible( TRUE );
}

void LLModalDialog::stopModal()
{
	gFocusMgr.unlockFocus();
	gFocusMgr.releaseFocusIfNeeded( this );

	if (mModal)
	{
		std::list<LLModalDialog*>::iterator iter = std::find(sModalStack.begin(), sModalStack.end(), this);
		if (iter != sModalStack.end())
		{
			sModalStack.erase(iter);
		}
		else
		{
			llwarns << "LLModalDialog::stopModal not in list!" << llendl;
		}
	}
	if (!sModalStack.empty())
	{
		LLModalDialog* front = sModalStack.front();
		front->setVisible(TRUE);
	}
}


void LLModalDialog::setVisible( BOOL visible )
{
	if (mModal)
	{
		if( visible )
		{
			// This is a modal dialog.  It sucks up all mouse and keyboard operations.
			gFocusMgr.setMouseCapture( this, NULL );

			// The dialog view is a root view
			gFocusMgr.setTopView( this, NULL );
			setFocus( TRUE );
		}
		else
		{
			gFocusMgr.releaseFocusIfNeeded( this );
		}
	}
	
	LLFloater::setVisible( visible );
}

BOOL LLModalDialog::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (!LLFloater::handleMouseDown(x, y, mask))
	{
		if (mModal)
		{
			// Click was outside the panel
			make_ui_sound("UISndInvalidOp");
		}
	}
	return TRUE;
}

BOOL LLModalDialog::handleHover(S32 x, S32 y, MASK mask)		
{ 
	if( childrenHandleHover(x, y, mask) == NULL )
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
		lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << llendl;		
	}
	return TRUE;
}

BOOL LLModalDialog::handleMouseUp(S32 x, S32 y, MASK mask)
{
	childrenHandleMouseUp(x, y, mask);
	return TRUE;
}

BOOL LLModalDialog::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	childrenHandleScrollWheel(x, y, clicks);
	return TRUE;
}

BOOL LLModalDialog::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	if (!LLFloater::handleDoubleClick(x, y, mask))
	{
		// Click outside the panel
		make_ui_sound("UISndInvalidOp");
	}
	return TRUE;
}

BOOL LLModalDialog::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	childrenHandleRightMouseDown(x, y, mask);
	return TRUE;
}


BOOL LLModalDialog::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent )
{
	childrenHandleKey(key, mask);

	LLFloater::handleKeyHere(key, mask, called_from_parent );

	if (mModal)
	{
		// Suck up all keystokes except CTRL-Q.
		BOOL is_quit = ('Q' == key) && (MASK_CONTROL == mask);
		return !is_quit;
	}
	else
	{
		// don't process escape key until message box has been on screen a minimal amount of time
		// to avoid accidentally destroying the message box when user is hitting escape at the time it appears
		BOOL enough_time_elapsed = mVisibleTime.getElapsedTimeF32() > 1.0f;
		if (enough_time_elapsed && key == KEY_ESCAPE)
		{
			close();
			return TRUE;
		}
		return FALSE;
	}	
}

void LLModalDialog::onClose(bool app_quitting)
{
	stopModal();
	LLFloater::onClose(app_quitting);
}

// virtual
void LLModalDialog::draw()
{
	if (getVisible())
	{
		LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
		S32 shadow_lines = LLUI::sConfigGroup->getS32("DropShadowFloater");

		gl_drop_shadow( 0, mRect.getHeight(), mRect.getWidth(), 0,
			shadow_color, shadow_lines);

		LLFloater::draw();

		if (mModal)
		{
			// If we've lost focus to a non-child, get it back ASAP.
			if( gFocusMgr.getTopView() != this )
			{
				gFocusMgr.setTopView( this, NULL);
			}

			if( !gFocusMgr.childHasKeyboardFocus( this ) )
			{
				setFocus(TRUE);
			}

			if( !gFocusMgr.childHasMouseCapture( this ) )
			{
				gFocusMgr.setMouseCapture( this, NULL );
			}
		}
	}
}

void LLModalDialog::centerOnScreen()
{
	LLVector2 window_size = LLUI::getWindowSize();

	S32 dialog_left = (llround(window_size.mV[VX]) - mRect.getWidth()) / 2;
	S32 dialog_bottom = (llround(window_size.mV[VY]) - mRect.getHeight()) / 2;

	translate( dialog_left - mRect.mLeft, dialog_bottom - mRect.mBottom );
}


// static 
void LLModalDialog::onAppFocusLost()
{
	if( !sModalStack.empty() )
	{
		LLModalDialog* instance = LLModalDialog::sModalStack.front();
		if( gFocusMgr.childHasMouseCapture( instance ) )
		{
			gFocusMgr.setMouseCapture( NULL, NULL );
		}

		if( gFocusMgr.childHasKeyboardFocus( instance ) )
		{
			gFocusMgr.setKeyboardFocus( NULL, NULL );
		}
	}
}

// static 
void LLModalDialog::onAppFocusGained()
{
	if( !sModalStack.empty() )
	{
		LLModalDialog* instance = LLModalDialog::sModalStack.front();

		// This is a modal dialog.  It sucks up all mouse and keyboard operations.
		gFocusMgr.setMouseCapture( instance, NULL );
		instance->setFocus(TRUE);
		gFocusMgr.setTopView( instance, NULL );

		instance->centerOnScreen();
	}
}


