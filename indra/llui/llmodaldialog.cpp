/** 
 * @file llmodaldialog.cpp
 * @brief LLModalDialog base class
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

LLModalDialog::LLModalDialog( const std::string& title, S32 width, S32 height, BOOL modal )
	: LLFloater( std::string("modal container"),
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

// virtual
void LLModalDialog::open()	/* Flawfinder: ignore */
{
	// SJB: Hack! Make sure we don't ever host a modal dialog
	LLMultiFloater* thost = LLFloater::getFloaterHost();
	LLFloater::setFloaterHost(NULL);
	LLFloater::open();
	LLFloater::setFloaterHost(thost);
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
		gFocusMgr.setMouseCapture( this );
		gFocusMgr.setTopCtrl( this );
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
			gFocusMgr.setMouseCapture( this );

			// The dialog view is a root view
			gFocusMgr.setTopCtrl( this );
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
	if (mModal)
	{
		if (!LLFloater::handleMouseDown(x, y, mask))
		{
			// Click was outside the panel
			make_ui_sound("UISndInvalidOp");
		}
	}
	else
	{
		LLFloater::handleMouseDown(x, y, mask);
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


BOOL LLModalDialog::handleKeyHere(KEY key, MASK mask )
{
	LLFloater::handleKeyHere(key, mask );

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
	LLColor4 shadow_color = LLUI::sColorsGroup->getColor("ColorDropShadow");
	S32 shadow_lines = LLUI::sConfigGroup->getS32("DropShadowFloater");

	gl_drop_shadow( 0, getRect().getHeight(), getRect().getWidth(), 0,
		shadow_color, shadow_lines);

	LLFloater::draw();

	if (mModal)
	{
		// If we've lost focus to a non-child, get it back ASAP.
		if( gFocusMgr.getTopCtrl() != this )
		{
			gFocusMgr.setTopCtrl( this );
		}

		if( !gFocusMgr.childHasKeyboardFocus( this ) )
		{
			setFocus(TRUE);
		}

		if( !gFocusMgr.childHasMouseCapture( this ) )
		{
			gFocusMgr.setMouseCapture( this );
		}
	}
}

void LLModalDialog::centerOnScreen()
{
	LLVector2 window_size = LLUI::getWindowSize();
	centerWithin(LLRect(0, 0, llround(window_size.mV[VX]), llround(window_size.mV[VY])));
}


// static 
void LLModalDialog::onAppFocusLost()
{
	if( !sModalStack.empty() )
	{
		LLModalDialog* instance = LLModalDialog::sModalStack.front();
		if( gFocusMgr.childHasMouseCapture( instance ) )
		{
			gFocusMgr.setMouseCapture( NULL );
		}

		if( gFocusMgr.childHasKeyboardFocus( instance ) )
		{
			gFocusMgr.setKeyboardFocus( NULL );
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
		gFocusMgr.setMouseCapture( instance );
		instance->setFocus(TRUE);
		gFocusMgr.setTopCtrl( instance );

		instance->centerOnScreen();
	}
}



