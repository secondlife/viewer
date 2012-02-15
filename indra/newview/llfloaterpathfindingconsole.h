/** 
 * @file llfloaterpathfindingconsole.h
 * @author William Todd Stinson
 * @brief "Pathfinding console" floater, allowing manipulation of the Havok AI pathfinding settings.
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

#ifndef LL_LLFLOATERPATHFINDINGCONSOLE_H
#define LL_LLFLOATERPATHFINDINGCONSOLE_H

#include "llfloater.h"
#include "llhandle.h"
#include "llnavmeshstation.h"
#include "LLPathingLib.h"

class LLSD;
class LLRadioGroup;
class LLSliderCtrl;
class LLLineEditor;
class LLTextBase;
class LLCheckBoxCtrl;
class LLTabContainer;

class LLFloaterPathfindingConsole
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kCharacterTypeA = 0,
		kCharacterTypeB = 1,
		kCharacterTypeC = 2,
		kCharacterTypeD = 3
	} ECharacterType;

	virtual BOOL postBuild();
	virtual BOOL handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down);

	BOOL isGeneratePathMode(MASK mask, EClickType clicktype, BOOL down) const;

	static LLHandle<LLFloaterPathfindingConsole> getInstanceHandle();

	BOOL isRenderPath() const;

	BOOL isRenderNavMesh() const;
	void setRenderNavMesh(BOOL pIsRenderNavMesh);

	BOOL isRenderWalkables() const;
	void setRenderWalkables(BOOL pIsRenderWalkables);

	BOOL isRenderStaticObstacles() const;
	void setRenderStaticObstacles(BOOL pIsRenderStaticObstacles);

	BOOL isRenderMaterialVolumes() const;
	void setRenderMaterialVolumes(BOOL pIsRenderMaterialVolumes);

	BOOL isRenderExclusionVolumes() const;
	void setRenderExclusionVolumes(BOOL pIsRenderExclusionVolumes);

	BOOL isRenderWorld() const;
	void setRenderWorld(BOOL pIsRenderWorld);

    F32                   getCharacterWidth() const;
    void                  setCharacterWidth(F32 pCharacterWidth);

    ECharacterType        getCharacterType() const;
    void                  setCharacterType(ECharacterType pCharacterType);

	void setHasNavMeshReceived();
	void setHasNoNavMesh();

protected:

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	virtual void onOpen(const LLSD& pKey);

	void onShowWorldToggle();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onViewCharactersClicked();
	void onViewEditLinksetClicked();
	void onClearPathClicked();

	void generatePath();

	LLRootHandle<LLFloaterPathfindingConsole> mSelfHandle;
	LLCheckBoxCtrl                            *mShowNavMeshCheckBox;
	LLCheckBoxCtrl                            *mShowWalkablesCheckBox;
	LLCheckBoxCtrl                            *mShowStaticObstaclesCheckBox;
	LLCheckBoxCtrl                            *mShowMaterialVolumesCheckBox;
	LLCheckBoxCtrl                            *mShowExclusionVolumesCheckBox;
	LLCheckBoxCtrl                            *mShowWorldCheckBox;
	LLTextBase                                *mPathfindingStatus;
	LLTabContainer                            *mEditTestTabContainer;
	LLSliderCtrl                              *mCharacterWidthSlider;
	LLRadioGroup                              *mCharacterTypeRadioGroup;

	LLNavMeshDownloadObserver	mNavMeshDownloadObserver[10];
	int							mCurrentMDO;
	int							mNavMeshCnt;

	//Container that is populated and subsequently submitted to the LLPathingSystem for processing
	LLPathingLib::PathingPacket		mPathData;
	bool mHasStartPoint;
	bool mHasEndPoint;

	static LLHandle<LLFloaterPathfindingConsole> sInstanceHandle;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
