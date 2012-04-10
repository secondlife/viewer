/** 
 * @file llpathfindingpathtool.cpp
 * @author William Todd Stinson
 * @brief XXX
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
#include "llpathfindingpathtool.h"
#include "llsingleton.h"
#include "lltool.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llpathfindingmanager.h"
#include "llpathinglib.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#define PATH_TOOL_NAME "PathfindingPathTool"

LLPathfindingPathTool::LLPathfindingPathTool()
	: LLTool(PATH_TOOL_NAME),
	LLSingleton<LLPathfindingPathTool>(),
	mPathData(),
	mPathResult(LLPathingLib::LLPL_PATH_NOT_GENERATED),
	mHasStartPoint(false),
	mHasEndPoint(false),
	mCharacterWidth(1.0f),
	mCharacterType(kCharacterTypeNone),
	mPathEventSignal()
{
	if (!LLPathingLib::getInstance())
	{
		LLPathingLib::initSystem();
	}

	mPathData.mCharacterWidth = mCharacterWidth;
}

LLPathfindingPathTool::~LLPathfindingPathTool()
{
}

BOOL LLPathfindingPathTool::handleMouseDown(S32 pX, S32 pY, MASK pMask)
{
	if ((pMask & (MASK_CONTROL|MASK_SHIFT)) != 0)
	{
		LLVector3 dv = gViewerWindow->mouseDirectionGlobal(pX, pY);
		LLVector3 mousePos = LLViewerCamera::getInstance()->getOrigin();
		LLVector3 rayStart = mousePos;
		LLVector3 rayEnd = mousePos + dv * 150;

		if (pMask & MASK_CONTROL)
		{
			mPathData.mStartPointA = rayStart;
			mPathData.mEndPointA = rayEnd;
			mHasStartPoint = true;
		}
		else if (pMask & MASK_SHIFT)
		{
			mPathData.mStartPointB = rayStart;
			mPathData.mEndPointB = rayEnd;
			mHasEndPoint = true;
		}
		computePath();
	}

	return ((pMask & (MASK_CONTROL|MASK_SHIFT)) != 0);
}

BOOL LLPathfindingPathTool::handleHover(S32 pX, S32 pY, MASK pMask)
{
	if ((pMask & (MASK_CONTROL|MASK_SHIFT)) != 0)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPATHFINDING);
	}

	return ((pMask & (MASK_CONTROL|MASK_SHIFT)) != 0);
}

LLPathfindingPathTool::EPathStatus LLPathfindingPathTool::getPathStatus() const
{
	EPathStatus status = kPathStatusUnknown;

	if (LLPathingLib::getInstance() == NULL)
	{
		status = kPathStatusNotImplemented;
	}
	else if (!LLPathfindingManager::getInstance()->isPathfindingEnabledForCurrentRegion())
	{
		status = kPathStatusNotEnabled;
	}
	else if (!mHasStartPoint && !mHasEndPoint)
	{
		status = kPathStatusChooseStartAndEndPoints;
	}
	else if (!mHasStartPoint)
	{
		status = kPathStatusChooseStartPoint;
	}
	else if (!mHasEndPoint)
	{
		status = kPathStatusChooseEndPoint;
	}
	else if (mPathResult == LLPathingLib::LLPL_PATH_GENERATED_OK)
	{
		status = kPathStatusHasValidPath;
	}
	else if (mPathResult == LLPathingLib::LLPL_NO_PATH)
	{
		status = kPathStatusHasInvalidPath;
	}
	else
	{
		status = kPathStatusError;
	}

	return status;
}

F32 LLPathfindingPathTool::getCharacterWidth() const
{
	return mCharacterWidth;
}

void LLPathfindingPathTool::setCharacterWidth(F32 pCharacterWidth)
{
	mCharacterWidth = pCharacterWidth;
	mPathData.mCharacterWidth = pCharacterWidth;
	computePath();
}

LLPathfindingPathTool::ECharacterType LLPathfindingPathTool::getCharacterType() const
{
	return mCharacterType;
}

void LLPathfindingPathTool::setCharacterType(ECharacterType pCharacterType)
{
	mCharacterType = pCharacterType;
	switch (pCharacterType)
	{
	case kCharacterTypeNone :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		break;
	case kCharacterTypeA :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_A;
		break;
	case kCharacterTypeB :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_B;
		break;
	case kCharacterTypeC :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_C;
		break;
	case kCharacterTypeD :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_D;
		break;
	default :
		mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		llassert(0);
		break;
	}
	computePath();
}

bool LLPathfindingPathTool::isRenderPath() const
{
	return (mHasStartPoint && mHasEndPoint);
}

void LLPathfindingPathTool::clearPath()
{
	mHasStartPoint = false;
	mHasEndPoint = false;
	computePath();
}

LLPathfindingPathTool::path_event_slot_t LLPathfindingPathTool::registerPathEventListener(path_event_callback_t pPathEventCallback)
{
	return mPathEventSignal.connect(pPathEventCallback);
}

void LLPathfindingPathTool::computePath()
{
	mPathResult = LLPathingLib::getInstance()->generatePath(mPathData);
	mPathEventSignal();
}
