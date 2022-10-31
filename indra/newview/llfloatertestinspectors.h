/** 
* @file llfloatertestinspectors.h
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#ifndef LLFLOATERTESTINSPECTORS_H
#define LLFLOATERTESTINSPECTORS_H

#include "llfloater.h"

class LLSD;

class LLFloaterTestInspectors : public LLFloater
{
    friend class LLFloaterReg;
public:
    // nothing yet

private:
    // Construction handled by LLFloaterReg
    LLFloaterTestInspectors(const LLSD& seed);
    ~LLFloaterTestInspectors();

    /*virtual*/ BOOL postBuild();

    // Button callback to show
    void showAvatarInspector(LLUICtrl*, const LLSD& avatar_id);
    void showObjectInspector(LLUICtrl*, const LLSD& avatar_id);
    
    // Debug function hookups for buttons
    void onClickAvatar2D();
    void onClickAvatar3D();
    void onClickObject2D();
    void onClickObject3D();
    void onClickGroup();
    void onClickPlace();
    void onClickEvent();
};

#endif
