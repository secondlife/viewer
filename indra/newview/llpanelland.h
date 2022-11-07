/** 
 * @file llpanelland.h
 * @brief Land information in the tool floater, NOT the "About Land" floater
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

#ifndef LL_LLPANELLAND_H
#define LL_LLPANELLAND_H

#include "llpanel.h"

class LLTextBox;
class LLCheckBoxCtrl;
class LLButton;
class LLSpinCtrl;
class LLLineEditor;
class LLPanelLandSelectObserver;

class LLPanelLandInfo
:   public LLPanel
{
public:
    LLPanelLandInfo();
    virtual ~LLPanelLandInfo();

    void refresh();
    static void refreshAll();
    
    LLCheckBoxCtrl  *mCheckShowOwners;

protected:
    static void onClickClaim();
    static void onClickRelease();
    static void onClickDivide();
    static void onClickJoin();
    static void onClickAbout();

protected:
    virtual BOOL    postBuild();

    static LLPanelLandSelectObserver* sObserver;
    static LLPanelLandInfo* sInstance;
};

#endif
