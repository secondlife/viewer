/** 
* @file llfloatertestinspectors.cpp
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include "llviewerprecompiledheaders.h"

#include "llfloatertestinspectors.h"

// Viewer includes
#include "llstartup.h"

// Linden library includes
#include "llfloaterreg.h"
//#include "lluictrlfactory.h"

LLFloaterTestInspectors::LLFloaterTestInspectors(const LLSD& seed)
:	LLFloater(seed)
{
	mCommitCallbackRegistrar.add("ShowAvatarInspector",
		boost::bind(&LLFloaterTestInspectors::showAvatarInspector, this, _1, _2));
}

LLFloaterTestInspectors::~LLFloaterTestInspectors()
{}

BOOL LLFloaterTestInspectors::postBuild()
{
//	getChild<LLUICtrl>("avatar_2d_btn")->setCommitCallback(
//		boost::bind(&LLFloaterTestInspectors::onClickAvatar2D, this));
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
	LLFloaterReg::showInstance("inspect_avatar", id);
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
