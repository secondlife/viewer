/** 
 * @file llmodaldialog.h
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

#ifndef LL_LLMODALDIALOG_H
#define LL_LLMODALDIALOG_H

#include "llfloater.h"
#include "llframetimer.h"

class LLModalDialog;

// By default, a ModalDialog is modal, i.e. no other window can have focus
// However, for the sake of code reuse and simplicity, if mModal == false,
// the dialog behaves like a normal floater
// https://wiki.lindenlab.com/mediawiki/index.php?title=LLModalDialog&oldid=81385
class LLModalDialog : public LLFloater
{
public:
    LLModalDialog( const LLSD& key, BOOL modal = true );
    virtual     ~LLModalDialog();
    
    /*virtual*/ BOOL    postBuild();
    
    /*virtual*/ void    openFloater(const LLSD& key = LLSD());
    /*virtual*/ void    onOpen(const LLSD& key);
    
    /*virtual*/ void    reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
    
    /*virtual*/ BOOL    handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL    handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL    handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ BOOL    handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL    handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL    handleKeyHere(KEY key, MASK mask );

    /*virtual*/ void    setVisible(BOOL visible);
    /*virtual*/ void    draw();

    BOOL            isModal() const { return mModal; }
    void            stopModal();

    static void     onAppFocusLost();
    static void     onAppFocusGained();

    static S32      activeCount() { return sModalStack.size(); }
    static void     shutdownModals();

protected:
    void            centerOnScreen();

private:
    
    LLFrameTimer    mVisibleTime;
    const BOOL      mModal;

    static std::list<LLModalDialog*> sModalStack;  // Top of stack is currently being displayed
};

#endif  // LL_LLMODALDIALOG_H
