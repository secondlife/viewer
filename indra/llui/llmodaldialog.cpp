/** 
 * @file llmodaldialog.cpp
 * @brief LLModalDialog base class
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

#include "linden_common.h"

#include "llmodaldialog.h"

#include "llfocusmgr.h"
#include "v4color.h"
#include "v2math.h"
#include "llui.h"
#include "llwindow.h"
#include "llkeyboard.h"
#include "llmenugl.h"
// static
std::list<LLModalDialog*> LLModalDialog::sModalStack;

LLModalDialog::LLModalDialog( const LLSD& key, BOOL modal )
    : LLFloater(key),
      mModal( modal )
{
    if (modal)
    {
        setCanMinimize(FALSE);
        setCanClose(FALSE);
    }
    setVisible( FALSE );
    setBackgroundVisible(TRUE);
    setBackgroundOpaque(TRUE);
    centerOnScreen(); // default position
    mCloseSignal.connect(boost::bind(&LLModalDialog::stopModal, this));
}

LLModalDialog::~LLModalDialog()
{
    // don't unlock focus unless we have it
    if (gFocusMgr.childHasKeyboardFocus(this))
    {
        gFocusMgr.unlockFocus();
    }
    
    std::list<LLModalDialog*>::iterator iter = std::find(sModalStack.begin(), sModalStack.end(), this);
    if (iter != sModalStack.end())
    {
        LL_ERRS() << "Attempt to delete dialog while still in sModalStack!" << LL_ENDL;
    }
}

// virtual
BOOL LLModalDialog::postBuild()
{
    return LLFloater::postBuild();
}

// virtual
void LLModalDialog::openFloater(const LLSD& key)
{
    // SJB: Hack! Make sure we don't ever host a modal dialog
    LLMultiFloater* thost = LLFloater::getFloaterHost();
    LLFloater::setFloaterHost(NULL);
    LLFloater::openFloater(key);
    LLFloater::setFloaterHost(thost);
}

void LLModalDialog::reshape(S32 width, S32 height, BOOL called_from_parent)
{
    LLFloater::reshape(width, height, called_from_parent);
    centerOnScreen();
}

// virtual
void LLModalDialog::onOpen(const LLSD& key)
{
    if (mModal)
    {
        // If Modal, Hide the active modal dialog
        if (!sModalStack.empty())
        {
            LLModalDialog* front = sModalStack.front();
            if (front != this)
            {
                front->setVisible(FALSE);
            }
        }
    
        // This is a modal dialog.  It sucks up all mouse and keyboard operations.
        gFocusMgr.setMouseCapture( this );
        LLUI::getInstance()->addPopup(this);
        setFocus(TRUE);

        std::list<LLModalDialog*>::iterator iter = std::find(sModalStack.begin(), sModalStack.end(), this);
        if (iter != sModalStack.end())
        {
            // if already present, we want to move it to front.
            sModalStack.erase(iter);
        }

        sModalStack.push_front(this);
    }
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
            LL_WARNS() << "LLModalDialog::stopModal not in list!" << LL_ENDL;
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
            LLUI::getInstance()->addPopup(this);
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
    LLView* popup_menu = LLMenuGL::sMenuContainer->getVisibleMenu();
    if (popup_menu != NULL)
    {
        S32 mx, my;
        LLUI::getInstance()->getMousePositionScreen(&mx, &my);
        LLRect menu_screen_rc = popup_menu->calcScreenRect();
        if(!menu_screen_rc.pointInRect(mx, my))
        {
            LLMenuGL::sMenuContainer->hideMenus();
        }
    }

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
        LL_DEBUGS("UserInput") << "hover handled by " << getName() << LL_ENDL;      
    }

    LLView* popup_menu = LLMenuGL::sMenuContainer->getVisibleMenu();
    if (popup_menu != NULL)
    {
        S32 mx, my;
        LLUI::getInstance()->getMousePositionScreen(&mx, &my);
        LLRect menu_screen_rc = popup_menu->calcScreenRect();
        if(menu_screen_rc.pointInRect(mx, my))
        {
            S32 local_x = mx - popup_menu->getRect().mLeft;
            S32 local_y = my - popup_menu->getRect().mBottom;
            popup_menu->handleHover(local_x, local_y, mask);
            gFocusMgr.setMouseCapture(NULL);
        }
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
    LLMenuGL::sMenuContainer->hideMenus();
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
            closeFloater();
            return TRUE;
        }
        return FALSE;
    }   
}

// virtual
void LLModalDialog::draw()
{
    static LLUIColor shadow_color = LLUIColorTable::instance().getColor("ColorDropShadow");
    static LLUICachedControl<S32> shadow_lines ("DropShadowFloater", 0);

    gl_drop_shadow( 0, getRect().getHeight(), getRect().getWidth(), 0,
        shadow_color, shadow_lines);

    LLFloater::draw();
    
    // Focus retrieval moved to LLFloaterView::refresh()
}

void LLModalDialog::centerOnScreen()
{
    LLVector2 window_size = LLUI::getInstance()->getWindowSize();
    centerWithin(LLRect(0, 0, ll_round(window_size.mV[VX]), ll_round(window_size.mV[VY])));
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

        instance->setFocus(FALSE);
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
        LLUI::getInstance()->addPopup(instance);

        instance->centerOnScreen();
    }
}

void LLModalDialog::shutdownModals()
{
    // This method is only for use during app shutdown. ~LLModalDialog()
    // checks sModalStack, and if the dialog instance is still there, it
    // crumps with "Attempt to delete dialog while still in sModalStack!" But
    // at app shutdown, all bets are off. If the user asks to shut down the
    // app, we shouldn't have to care WHAT's open. Put differently, if a modal
    // dialog is so crucial that we can't let the user terminate until s/he
    // addresses it, we should reject a termination request. The current state
    // of affairs is that we accept it, but then produce an LL_ERRS() popup that
    // simply makes our software look unreliable.
    sModalStack.clear();
}
