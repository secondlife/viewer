/** 
 * @file llpathfindingpathtool.h
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

#ifndef LL_LLPATHFINDINGPATHTOOL_H
#define LL_LLPATHFINDINGPATHTOOL_H

#include "llsingleton.h"
#include "lltool.h"
#include "LLPathingLib.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

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
	virtual BOOL      handleHover(S32 pX, S32 pY, MASK pMask);

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
	bool              isStartPathToolModKeys(MASK pMask) const;
	bool              isEndPathToolModKeys(MASK pMask) const;

	void              computeFinalPath();
	void              computeTempPath();

	LLPathingLib::PathingPacket mFinalPathData;
	LLPathingLib::PathingPacket mTempPathData;
	LLPathingLib::LLPLResult    mPathResult;
	bool                        mHasFinalStartPoint;
	bool                        mHasFinalEndPoint;
	bool                        mHasTempStartPoint;
	bool                        mHasTempEndPoint;
	F32                         mCharacterWidth;
	ECharacterType              mCharacterType;
	path_event_signal_t         mPathEventSignal;
};

#endif // LL_LLPATHFINDINGPATHTOOL_H
