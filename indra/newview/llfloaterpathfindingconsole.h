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
#include "llnavmeshstation.h"

class LLSD;
class LLCheckBoxCtrl;
class LLRadioGroup;
class LLSliderCtrl;
class LLLineEditor;

class LLFloaterPathfindingConsole
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	typedef enum
	{
		kRenderOverlayOnFixedPhysicsGeometry  = 0,
		kRenderOverlayOnAllRenderableGeometry = 1
	} ERegionOverlayDisplay;

	typedef enum
	{
		kPathSelectNone       = 0,
		kPathSelectStartPoint = 1,
		kPathSelectEndPoint   = 2
	} EPathSelectionState;

	typedef enum
	{
		kCharacterTypeA = 0,
		kCharacterTypeB = 1,
		kCharacterTypeC = 2,
		kCharacterTypeD = 3
	} ECharacterType;

	virtual BOOL postBuild();
	BOOL allowAllRenderables();

	ERegionOverlayDisplay getRegionOverlayDisplay() const;
	void                  setRegionOverlayDisplay(ERegionOverlayDisplay regionOverlayDisplay);

	EPathSelectionState   getPathSelectionState() const;
	void                  setPathSelectionState(EPathSelectionState pathSelectionState);

	F32                   getCharacterWidth() const;
	void                  setCharacterWidth(F32 characterWidth);

	ECharacterType        getCharacterType() const;
	void                  setCharacterType(ECharacterType characterType);

	F32                   getTerrainMaterialA() const;
	void                  setTerrainMaterialA(F32 terrainMaterial);

	F32                   getTerrainMaterialB() const;
	void                  setTerrainMaterialB(F32 terrainMaterial);

	F32                   getTerrainMaterialC() const;
	void                  setTerrainMaterialC(F32 terrainMaterial);

	F32                   getTerrainMaterialD() const;
	void                  setTerrainMaterialD(F32 terrainMaterial);

protected:

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	virtual void onOpen(const LLSD& pKey);

	void onShowNavmeshToggle();
	void onShowExcludeVolumesToggle();
	void onShowPathToggle();
	void onShowWaterPlaneToggle();
	void onRegionOverlayDisplaySwitch();
	void onPathSelectionSwitch();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onViewEditLinksetClicked();
	void onRebuildNavmeshClicked();
	void onRefreshNavmeshClicked();
	void onTerrainMaterialASet();
	void onTerrainMaterialBSet();
	void onTerrainMaterialCSet();
	void onTerrainMaterialDSet();

	LLCheckBoxCtrl *mShowNavmeshCheckBox;
	LLCheckBoxCtrl *mShowExcludeVolumesCheckBox;
	LLCheckBoxCtrl *mShowPathCheckBox;
	LLCheckBoxCtrl *mShowWaterPlaneCheckBox;
	LLRadioGroup   *mRegionOverlayDisplayRadioGroup;
	LLRadioGroup   *mPathSelectionRadioGroup;
	LLSliderCtrl   *mCharacterWidthSlider;
	LLRadioGroup   *mCharacterTypeRadioGroup;
	LLLineEditor   *mTerrainMaterialA;
	LLLineEditor   *mTerrainMaterialB;
	LLLineEditor   *mTerrainMaterialC;
	LLLineEditor   *mTerrainMaterialD;

	LLNavMeshDownloadObserver mNavmeshDownloadObserver;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
