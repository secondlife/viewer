/** 
* @file   llpathfindingpathtool.h
* @brief  Header file for llpathfindingpathtool
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
#ifndef LL_LLPATHFINDINGPATHTOOL_H
#define LL_LLPATHFINDINGPATHTOOL_H

#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llpathinglib.h"
#include "llsingleton.h"
#include "lltool.h"

class LLPathfindingPathTool : public LLTool, public LLSingleton<LLPathfindingPathTool>
{
public:
	typedef enum
	{
		kPathStatusUnknown,
		kPathStatusChooseStartAndEndPoints,
		kPathStatusChooseStartPoint,
		kPathStatusChooseEndPoint,
		kPathStatusHasValidPath,
		kPathStatusHasInvalidPath,
		kPathStatusNotEnabled,
		kPathStatusNotImplemented,
		kPathStatusError
	} EPathStatus;

	typedef enum
	{
		kCharacterTypeNone,
		kCharacterTypeA,
		kCharacterTypeB,
		kCharacterTypeC,
		kCharacterTypeD
	} ECharacterType;

	LLPathfindingPathTool();
	virtual ~LLPathfindingPathTool();

	typedef boost::function<void (void)>         path_event_callback_t;
	typedef boost::signals2::signal<void (void)> path_event_signal_t;
	typedef boost::signals2::connection          path_event_slot_t;

	virtual BOOL      handleMouseDown(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleMouseUp(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleMiddleMouseDown(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleMiddleMouseUp(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleRightMouseDown(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleRightMouseUp(S32 pX, S32 pY, MASK pMask);
	virtual BOOL      handleDoubleClick(S32 x, S32 y, MASK mask);

	virtual BOOL      handleHover(S32 pX, S32 pY, MASK pMask);

	virtual BOOL      handleKey(KEY pKey, MASK pMask);

	EPathStatus       getPathStatus() const;

	F32               getCharacterWidth() const;
	void              setCharacterWidth(F32 pCharacterWidth);

	ECharacterType    getCharacterType() const;
	void              setCharacterType(ECharacterType pCharacterType);

	bool              isRenderPath() const;
	void              clearPath();

	path_event_slot_t registerPathEventListener(path_event_callback_t pPathEventCallback);

protected:

private:
	bool              isAnyPathToolModKeys(MASK pMask) const;
	bool              isPointAModKeys(MASK pMask) const;
	bool              isPointBModKeys(MASK pMask) const;
	bool              isCameraModKeys(MASK pMask) const;

	void              getRayPoints(S32 pX, S32 pY, LLVector3 &pRayStart, LLVector3 &pRayEnd) const;
	void              computeFinalPoints(S32 pX, S32 pY, MASK pMask);
	void              computeTempPoints(S32 pX, S32 pY, MASK pMask);

	void              setFinalA(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint);
	bool              hasFinalA() const;
	const LLVector3   &getFinalAStart() const;
	const LLVector3   &getFinalAEnd() const;

	void              setTempA(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint);
	bool              hasTempA() const;

	void              setFinalB(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint);
	bool              hasFinalB() const;
	const LLVector3   &getFinalBStart() const;
	const LLVector3   &getFinalBEnd() const;

	void              setTempB(const LLVector3 &pStartPoint, const LLVector3 &pEndPoint);
	bool              hasTempB() const;

	void              clearFinal();
	void              clearTemp();

	void              computeFinalPath();
	void              computeTempPath();

	LLPathingLib::PathingPacket mFinalPathData;
	LLPathingLib::PathingPacket mTempPathData;
	LLPathingLib::LLPLResult    mPathResult;
	ECharacterType              mCharacterType;
	path_event_signal_t         mPathEventSignal;
	bool                        mIsLeftMouseButtonHeld;
	bool                        mIsMiddleMouseButtonHeld;
	bool                        mIsRightMouseButtonHeld;
};

#endif // LL_LLPATHFINDINGPATHTOOL_H
