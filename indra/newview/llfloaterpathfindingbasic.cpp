/** 
* @file llfloaterpathfindingbasic.cpp
* @author William Todd Stinson
* @brief "Pathfinding basic" floater, allowing for basic freezing and unfreezing of the pathfinding avator mode.
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindingbasic.h"

#include "llsd.h"
#include "llagent.h"
#include "lltextbase.h"
#include "llbutton.h"
#include "llviewerregion.h"

//---------------------------------------------------------------------------
// LLFloaterPathfindingBasic
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingBasic::postBuild()
{
	mRegionNotEnabledText = findChild<LLTextBase>("disabled_warning_label");
	llassert(mRegionNotEnabledText != NULL);

	mUnfreezeLabel = findChild<LLTextBase>("unfreeze_label");
	llassert(mUnfreezeLabel != NULL);

	mUnfreezeButton = findChild<LLButton>("enter_unfrozen_mode");
	llassert(mUnfreezeButton != NULL);
	mUnfreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingBasic::onUnfreezeClicked, this));

	mFreezeLabel = findChild<LLTextBase>("freeze_label");
	llassert(mFreezeLabel != NULL);

	mFreezeButton = findChild<LLButton>("enter_frozen_mode");
	llassert(mFreezeButton != NULL);
	mFreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingBasic::onFreezeClicked, this));

	updateOnFrozenState();

	return LLFloater::postBuild();
}

void LLFloaterPathfindingBasic::onOpen(const LLSD& key)
{
}

LLFloaterPathfindingBasic::LLFloaterPathfindingBasic(const LLSD& pSeed)
	: LLFloater(pSeed),
	mRegionNotEnabledText(NULL),
	mUnfreezeLabel(NULL),
	mUnfreezeButton(NULL),
	mFreezeLabel(NULL),
	mFreezeButton(NULL),
	mIsRegionFrozenXXX(true)
{
}

LLFloaterPathfindingBasic::~LLFloaterPathfindingBasic()
{
}

std::string LLFloaterPathfindingBasic::getCapabilityURL() const
{
	std::string capURL("");

	LLViewerRegion *region = gAgent.getRegion();
	if (region != NULL)
	{
		capURL = region->getCapability("RetrieveNavMeshSrc");
	}

	return capURL;
}

void LLFloaterPathfindingBasic::updateOnFrozenState()
{
	std::string capURL = getCapabilityURL();

	if (capURL.empty())
	{
		mRegionNotEnabledText->setVisible(TRUE);
		mUnfreezeLabel->setEnabled(FALSE);
		mUnfreezeButton->setEnabled(FALSE);
		mFreezeLabel->setEnabled(FALSE);
		mFreezeButton->setEnabled(FALSE);
	}
	else
	{
		mRegionNotEnabledText->setVisible(FALSE);
		if (mIsRegionFrozenXXX)
		{
			mUnfreezeLabel->setEnabled(TRUE);
			mUnfreezeButton->setEnabled(TRUE);
			mFreezeLabel->setEnabled(FALSE);
			mFreezeButton->setEnabled(FALSE);
		}
		else
		{
			mUnfreezeLabel->setEnabled(FALSE);
			mUnfreezeButton->setEnabled(FALSE);
			mFreezeLabel->setEnabled(TRUE);
			mFreezeButton->setEnabled(TRUE);
		}
	}
}

void LLFloaterPathfindingBasic::onUnfreezeClicked()
{
	mIsRegionFrozenXXX = false;
	updateOnFrozenState();
	llwarns << "functionality has not yet been implemented to set unfrozen state" << llendl;
}

void LLFloaterPathfindingBasic::onFreezeClicked()
{
	mIsRegionFrozenXXX = true;
	updateOnFrozenState();
	llwarns << "functionality has not yet been implemented to set frozen state" << llendl;
}
