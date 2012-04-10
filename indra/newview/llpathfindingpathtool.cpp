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
#include "LLPathingLib.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#define PATH_TOOL_NAME "PathfindingPathTool"

LLPathfindingPathTool::LLPathfindingPathTool()
	: LLTool(PATH_TOOL_NAME),
	LLSingleton<LLPathfindingPathTool>(),
	mFinalPathData(),
	mTempPathData(),
	mPathResult(LLPathingLib::LLPL_PATH_NOT_GENERATED),
	mHasFinalStartPoint(false),
	mHasFinalEndPoint(false),
	mHasTempStartPoint(false),
	mHasTempEndPoint(false),
	mCharacterWidth(1.0f),
	mCharacterType(kCharacterTypeNone),
	mPathEventSignal()
{
	if (!LLPathingLib::getInstance())
	{
		LLPathingLib::initSystem();
	}	

	setCharacterWidth(mCharacterWidth);
	setCharacterType(mCharacterType);
}

LLPathfindingPathTool::~LLPathfindingPathTool()
{
}

BOOL LLPathfindingPathTool::handleMouseDown(S32 pX, S32 pY, MASK pMask)
{
	BOOL returnVal = FALSE;

	if (isAnyPathToolModKeys(pMask))
	{
		LLVector3 dv = gViewerWindow->mouseDirectionGlobal(pX, pY);
		LLVector3 mousePos = LLViewerCamera::getInstance()->getOrigin();
		LLVector3 rayStart = mousePos;
		LLVector3 rayEnd = mousePos + dv * 150;

		if (isStartPathToolModKeys(pMask))
		{
			mFinalPathData.mStartPointA = rayStart;
			mFinalPathData.mEndPointA = rayEnd;
			mHasFinalStartPoint = true;
		}
		else if (isEndPathToolModKeys(pMask))
		{
			mFinalPathData.mStartPointB = rayStart;
			mFinalPathData.mEndPointB = rayEnd;
			mHasFinalEndPoint = true;
		}
		computeFinalPath();

		returnVal = TRUE;
	}

	return returnVal;
}

BOOL LLPathfindingPathTool::handleHover(S32 pX, S32 pY, MASK pMask)
{
	BOOL returnVal = FALSE;

	if (isAnyPathToolModKeys(pMask))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPATHFINDING);

		LLVector3 dv = gViewerWindow->mouseDirectionGlobal(pX, pY);
		LLVector3 mousePos = LLViewerCamera::getInstance()->getOrigin();
		LLVector3 rayStart = mousePos;
		LLVector3 rayEnd = mousePos + dv * 150;

		if (isStartPathToolModKeys(pMask))
		{
			mTempPathData.mStartPointA = rayStart;
			mTempPathData.mEndPointA = rayEnd;
			mHasTempStartPoint = true;
			mTempPathData.mStartPointB = mFinalPathData.mStartPointB;
			mTempPathData.mEndPointB = mFinalPathData.mEndPointB;
			mHasTempEndPoint = mHasFinalEndPoint;
	}
		else if (isEndPathToolModKeys(pMask))
		{
			mTempPathData.mStartPointB = rayStart;
			mTempPathData.mEndPointB = rayEnd;
			mHasTempEndPoint = true;
			mTempPathData.mStartPointA = mFinalPathData.mStartPointA;
			mTempPathData.mEndPointA = mFinalPathData.mEndPointA;
			mHasTempStartPoint = mHasFinalStartPoint;
		}
		computeTempPath();

		returnVal = TRUE;
	}
	else
	{
		mHasTempStartPoint = false;
		mHasTempEndPoint = false;
		computeFinalPath();
	}

	return returnVal;
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
	else if (!mHasFinalStartPoint && !mHasFinalEndPoint)
	{
		status = kPathStatusChooseStartAndEndPoints;
	}
	else if (!mHasFinalStartPoint)
	{
		status = kPathStatusChooseStartPoint;
	}
	else if (!mHasFinalEndPoint)
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
	mFinalPathData.mCharacterWidth = pCharacterWidth;
	mTempPathData.mCharacterWidth = pCharacterWidth;
	computeFinalPath();
	computeTempPath();
}

LLPathfindingPathTool::ECharacterType LLPathfindingPathTool::getCharacterType() const
{
	return mCharacterType;
}

void LLPathfindingPathTool::setCharacterType(ECharacterType pCharacterType)
{
	mCharacterType = pCharacterType;

	LLPathingLib::LLPLCharacterType characterType;
	switch (pCharacterType)
	{
	case kCharacterTypeNone :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		break;
	case kCharacterTypeA :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_A;
		break;
	case kCharacterTypeB :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_B;
		break;
	case kCharacterTypeC :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_C;
		break;
	case kCharacterTypeD :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_D;
		break;
	default :
		characterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		llassert(0);
		break;
	}
	mFinalPathData.mCharacterType = characterType;
	mTempPathData.mCharacterType = characterType;
	computeFinalPath();
	computeTempPath();
}

bool LLPathfindingPathTool::isRenderPath() const
{
	return (mHasFinalStartPoint && mHasFinalEndPoint) || (mHasTempStartPoint && mHasTempEndPoint);
}

void LLPathfindingPathTool::clearPath()
{
	mHasFinalStartPoint = false;
	mHasFinalEndPoint = false;
	mHasTempStartPoint = false;
	mHasTempEndPoint = false;
	computeFinalPath();
}

LLPathfindingPathTool::path_event_slot_t LLPathfindingPathTool::registerPathEventListener(path_event_callback_t pPathEventCallback)
{
	return mPathEventSignal.connect(pPathEventCallback);
}

bool LLPathfindingPathTool::isAnyPathToolModKeys(MASK pMask) const
{
	return ((pMask & (MASK_CONTROL|MASK_SHIFT)) != 0);
}

bool LLPathfindingPathTool::isStartPathToolModKeys(MASK pMask) const
{
	return ((pMask & MASK_CONTROL) != 0);
}

bool LLPathfindingPathTool::isEndPathToolModKeys(MASK pMask) const
{
	return ((pMask & MASK_SHIFT) != 0);
}

void LLPathfindingPathTool::computeFinalPath()
{
	mPathResult = LLPathingLib::LLPL_PATH_NOT_GENERATED;
	if (mHasFinalStartPoint && mHasFinalEndPoint && (LLPathingLib::getInstance() != NULL))
	{
		mPathResult = LLPathingLib::getInstance()->generatePath(mFinalPathData);
	}
	mPathEventSignal();
}

void LLPathfindingPathTool::computeTempPath()
{
	if (mHasTempStartPoint && mHasTempEndPoint && (LLPathingLib::getInstance() != NULL))
	{
		mPathResult = LLPathingLib::getInstance()->generatePath(mTempPathData);
	}
	mPathEventSignal();
}
