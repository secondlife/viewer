/** 
* @file llfloatertestinspectors.cpp
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
#include "llviewerprecompiledheaders.h"

#include "llfloatertestinspectors.h"

// Viewer includes
#include "llstartup.h"

// Linden library includes
#include "llfloaterreg.h"
//#include "lluictrlfactory.h"

LLFloaterTestInspectors::LLFloaterTestInspectors(const LLSD& seed)
:   LLFloater(seed)
{
    mCommitCallbackRegistrar.add("ShowAvatarInspector",
        boost::bind(&LLFloaterTestInspectors::showAvatarInspector, this, _1, _2));
    mCommitCallbackRegistrar.add("ShowObjectInspector",
        boost::bind(&LLFloaterTestInspectors::showObjectInspector, this, _1, _2));
}

LLFloaterTestInspectors::~LLFloaterTestInspectors()
{}

BOOL LLFloaterTestInspectors::postBuild()
{
    // Test the dummy widget construction code
    getChild<LLUICtrl>("intentionally-not-found")->setEnabled(true);

//  getChild<LLUICtrl>("avatar_2d_btn")->setCommitCallback(
//      boost::bind(&LLFloaterTestInspectors::onClickAvatar2D, this));
    getChild<LLUICtrl>("avatar_3d_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickAvatar3D, this));
    getChild<LLUICtrl>("object_2d_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickObject2D, this));
    getChild<LLUICtrl>("object_3d_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickObject3D, this));
    getChild<LLUICtrl>("group_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickGroup, this));
    getChild<LLUICtrl>("place_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickPlace, this));
    getChild<LLUICtrl>("event_btn")->setCommitCallback(
        boost::bind(&LLFloaterTestInspectors::onClickEvent, this));

    return LLFloater::postBuild();
}

void LLFloaterTestInspectors::showAvatarInspector(LLUICtrl*, const LLSD& avatar_id)
{
    LLUUID id;  // defaults to null
    if (LLStartUp::getStartupState() >= STATE_STARTED)
    {
        id = avatar_id.asUUID();
    }
    // spawns off mouse position automatically
    LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", id));
}

void LLFloaterTestInspectors::showObjectInspector(LLUICtrl*, const LLSD& object_id)
{
    LLFloaterReg::showInstance("inspect_object", LLSD().with("object_id", object_id));
}

void LLFloaterTestInspectors::onClickAvatar2D()
{
}

void LLFloaterTestInspectors::onClickAvatar3D()
{
}

void LLFloaterTestInspectors::onClickObject2D()
{
}

void LLFloaterTestInspectors::onClickObject3D()
{
}

void LLFloaterTestInspectors::onClickGroup()
{
}

void LLFloaterTestInspectors::onClickPlace()
{
}

void LLFloaterTestInspectors::onClickEvent()
{
}
