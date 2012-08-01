/** 
* @file llpathfindingpathtool.cpp
* @brief Implementation of llpathfindingpathtool
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llpathfindingmanager.h"
#include "llpathinglib.h"
#include "llsingleton.h"
#include "lltool.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

#define PATH_TOOL_NAME "PathfindingPathTool"

LLPathfindingPathTool::LLPathfindingPathTool()
	: LLTool(PATH_TOOL_NAME),
	LLSingleton<LLPathfindingPathTool>(),
	mFinalPathData(),
	mTempPathData(),
	mPathResult(LLPathingLib::LLPL_NO_PATH),
	mCharacterType(kCharacterTypeNone),
	mPathEventSignal(),
	mIsLeftMouseButtonHeld(false),
	mIsMiddleMouseButtonHeld(false),
	mIsRightMouseButtonHeld(false)
{
	setCharacterWidth(1.0f);
	setCharacterType(mCharacterType);
}

LLPathfindingPathTool::~LLPathfindingPathTool()
{
}

BOOL LLPathfindingPathTool::handleMouseDown(S32 pX, S32 pY, MASK pMask)
{
	BOOL returnVal = FALSE;

	if (!mIsLeftMouseButtonHeld && !mIsMiddleMouseButtonHeld && !mIsRightMouseButtonHeld)
	{
		if (isAnyPathToolModKeys(pMask))
		{
			gViewerWindow->setCursor(isPointAModKeys(pMask)
				? UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD
				: UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD);
			computeFinalPoints(pX, pY, pMask);
			mIsLeftMouseButtonHeld = true;
			setMouseCapture(TRUE);
			returnVal = TRUE;
		}
		else if (!isCameraModKeys(pMask))
		{
			gViewerWindow->setCursor(UI_CURSOR_TOOLNO);
			mIsLeftMouseButtonHeld = true;
			setMouseCapture(TRUE);
			returnVal = TRUE;
		}
	}
	mIsLeftMouseButtonHeld = true;

	return returnVal;
}

BOOL LLPathfindingPathTool::handleMouseUp(S32 pX, S32 pY, MASK pMask)
{
	BOOL returnVal = FALSE;

	if (mIsLeftMouseButtonHeld && !mIsMiddleMouseButtonHeld && !mIsRightMouseButtonHeld)
	{
		computeFinalPoints(pX, pY, pMask);
		setMouseCapture(FALSE);
		returnVal = TRUE;
	}
	mIsLeftMouseButtonHeld = false;

	return returnVal;
}

BOOL LLPathfindingPathTool::handleMiddleMouseDown(S32 pX, S32 pY, MASK pMask)
{
	setMouseCapture(TRUE);
	mIsMiddleMouseButtonHeld = true;
	gViewerWindow->setCursor(UI_CURSOR_TOOLNO);

	return TRUE;
}

BOOL LLPathfindingPathTool::handleMiddleMouseUp(S32 pX, S32 pY, MASK pMask)
{
	if (!mIsLeftMouseButtonHeld && mIsMiddleMouseButtonHeld && !mIsRightMouseButtonHeld)
	{
		setMouseCapture(FALSE);
	}
	mIsMiddleMouseButtonHeld = false;

	return TRUE;
}

BOOL LLPathfindingPathTool::handleRightMouseDown(S32 pX, S32 pY, MASK pMask)
{
	setMouseCapture(TRUE);
	mIsRightMouseButtonHeld = true;
	gViewerWindow->setCursor(UI_CURSOR_TOOLNO);

	return TRUE;
}

BOOL LLPathfindingPathTool::handleRightMouseUp(S32 pX, S32 pY, MASK pMask)
{
	if (!mIsLeftMouseButtonHeld && !mIsMiddleMouseButtonHeld && mIsRightMouseButtonHeld)
	{
		setMouseCapture(FALSE);
	}
	mIsRightMouseButtonHeld = false;

	return TRUE;
}

BOOL LLPathfindingPathTool::handleDoubleClick(S32 pX, S32 pY, MASK pMask)
{
	return TRUE;
}

BOOL LLPathfindingPathTool::handleHover(S32 pX, S32 pY, MASK pMask)
{
	BOOL returnVal = FALSE;

	if (!mIsLeftMouseButtonHeld && !mIsMiddleMouseButtonHeld && !mIsRightMouseButtonHeld && !isAnyPathToolModKeys(pMask))
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPATHFINDING);
	}

	if (!mIsMiddleMouseButtonHeld && !mIsRightMouseButtonHeld && isAnyPathToolModKeys(pMask))
	{
		gViewerWindow->setCursor(isPointAModKeys(pMask)
			? (mIsLeftMouseButtonHeld ? UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD : UI_CURSOR_TOOLPATHFINDING_PATH_START)
			: (mIsLeftMouseButtonHeld ? UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD : UI_CURSOR_TOOLPATHFINDING_PATH_END));
		computeTempPoints(pX, pY, pMask);
		returnVal = TRUE;
	}
	else
	{
		clearTemp();
		computeFinalPath();
	}

	return returnVal;
}

BOOL LLPathfindingPathTool::handleKey(KEY pKey, MASK pMask)
{
	// Eat the escape key or else the camera tool will pick up and reset to default view.  This,
	// in turn, will cause some other methods to get called.  And one of those methods will reset
	// the current toolset back to the basic toolset.  This means that the pathfinding path toolset
	// will no longer be active, but typically with pathfinding path elements on screen.
	return (pKey == KEY_ESCAPE);
}

LLPathfindingPathTool::EPathStatus LLPathfindingPathTool::getPathStatus() const
{
	EPathStatus status = kPathStatusUnknown;

	if (LLPathingLib::getInstance() == NULL)
	{
		status = kPathStatusNotImplemented;
	}
	else if ((gAgent.getRegion() != NULL) && !gAgent.getRegion()->capabilitiesReceived())
	{
		status = kPathStatusUnknown;
	}
	else if (!LLPathfindingManager::getInstance()->isPathfindingEnabledForCurrentRegion())
	{
		status = kPathStatusNotEnabled;
	}
	else if (!hasFinalA() && !hasFinalB())
	{
		status = kPathStatusChooseStartAndEndPoints;
	}
	else if (!hasFinalA())
	{
		status = kPathStatusChooseStartPoint;
	}
	else if (!hasFinalB())
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
	return mFinalPathData.mCharacterWidth;
}

void LLPathfindingPathTool::setCharacterWidth(F32 pCharacterWidth)
{
	mFinalPathData.mCharacterWidth = pCharacterWidth;
	mTempPathData.mCharacterWidth = pCharacterWidth;
	computeFinalPath();
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
}

bool LLPathfindingPathTool::isRenderPath() const
{
	return (hasFinalA() || hasFinalB() || hasTempA() || hasTempB());
}

void LLPathfindingPathTool::clearPath()
{
	clearFinal();
	clearTemp();
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

bool LLPathfindingPathTool::isPointAModKeys(MASK pMask) const
{
	return ((pMask & MASK_CONTROL) != 0);
}

bool LLPathfindingPathTool::isPointBModKeys(MASK pMask) const
{
	return ((pMask & MASK_SHIFT) != 0);
}

bool LLPathfindingPathTool::isCameraModKeys(MASK pMask) const
{
	return ((pMask & MASK_ALT) != 0);
}

void LLPathfindingPathTool::getRayPoints(S32 pX, S32 pY, LLVector3 &pRayStart, LLVector3 &pRayEnd) const
{
	LLVector3 dv = gViewerWindow->mouseDirectionGlobal(pX, pY);
	LLVector3 mousePos = LLViewerCamera::getInstance()->getOrigin();
	pRayStart = mousePos;
	pRayEnd = mousePos + dv * 150;
}

void LLPathfindingPathTool::computeFinalPoints(S32 pX, S32 pY, MASK pMask)
{
	LLVector3 rayStart, rayEnd;
	getRayPoints(pX, pY, rayStart, rayEnd);

	if (isPointAModKeys(pMask))
	{
		setFinalA(rayStart, rayEnd);
	}
	else if (isPointBModKeys(pMask))
	{
		setFinalB(rayStart, rayEnd);
	}
	computeFinalPath();
}

void LLPathfindingPathTool::computeTempPoints(S32 pX, S32 pY, MASK pMask)
{
	LLVector3 rayStart, rayEnd;
	getRayPoints(pX, pY, rayStart, rayEnd);

	if (isPointAModKeys(pMask))
	{
		setTempA(rayStart, rayEnd);
		if (hasFinalB())
		{
			setTempB(getFinalBStart(), getFinalBEnd());
		}
	}
	else if (isPointBModKeys(pMask))
	{
		if (hasFinalA())
		{
			setTempA(getFinalAStart(), getFinalAEnd());
		}
		setTempB(rayStart, rayEnd);
	}
	computeTempPath();
}

void LLPathfindingPathTool::setFinalA(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint)
{
	mFinalPathData.mStartPointA = pStartPoint;
	mFinalPathData.mEndPointA = pEndPoint;
	mFinalPathData.mHasPointA = true;
}

bool LLPathfindingPathTool::hasFinalA() const
{
	return mFinalPathData.mHasPointA;
}

const LLVector3 &LLPathfindingPathTool::getFinalAStart() const
{
	return mFinalPathData.mStartPointA;
}

const LLVector3 &LLPathfindingPathTool::getFinalAEnd() const
{
	return mFinalPathData.mEndPointA;
}

void LLPathfindingPathTool::setTempA(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint)
{
	mTempPathData.mStartPointA = pStartPoint;
	mTempPathData.mEndPointA = pEndPoint;
	mTempPathData.mHasPointA = true;
}

bool LLPathfindingPathTool::hasTempA() const
{
	return mTempPathData.mHasPointA;
}

void LLPathfindingPathTool::setFinalB(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint)
{
	mFinalPathData.mStartPointB = pStartPoint;
	mFinalPathData.mEndPointB = pEndPoint;
	mFinalPathData.mHasPointB = true;
}

bool LLPathfindingPathTool::hasFinalB() const
{
	return mFinalPathData.mHasPointB;
}

const LLVector3 &LLPathfindingPathTool::getFinalBStart() const
{
	return mFinalPathData.mStartPointB;
}

const LLVector3 &LLPathfindingPathTool::getFinalBEnd() const
{
	return mFinalPathData.mEndPointB;
}

void LLPathfindingPathTool::setTempB(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint)
{
	mTempPathData.mStartPointB = pStartPoint;
	mTempPathData.mEndPointB = pEndPoint;
	mTempPathData.mHasPointB = true;
}

bool LLPathfindingPathTool::hasTempB() const
{
	return mTempPathData.mHasPointB;
}

void LLPathfindingPathTool::clearFinal()
{
	mFinalPathData.mHasPointA = false;
	mFinalPathData.mHasPointB = false;
}

void LLPathfindingPathTool::clearTemp()
{
	mTempPathData.mHasPointA = false;
	mTempPathData.mHasPointB = false;
}

void LLPathfindingPathTool::computeFinalPath()
{
	mPathResult = LLPathingLib::LLPL_NO_PATH;
	if (LLPathingLib::getInstance() != NULL)
	{
		mPathResult = LLPathingLib::getInstance()->generatePath(mFinalPathData);
	}
	mPathEventSignal();
}

void LLPathfindingPathTool::computeTempPath()
{
	mPathResult = LLPathingLib::LLPL_NO_PATH;
	if (LLPathingLib::getInstance() != NULL)
	{
		mPathResult = LLPathingLib::getInstance()->generatePath(mTempPathData);
	}
	mPathEventSignal();
}
