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
#include "LLPathingLib.h"

class LLSD;
class LLRadioGroup;
class LLSliderCtrl;
class LLLineEditor;
class LLTextBase;
class LLCheckBoxCtrl;

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
	//Populates a data packet that is forwarded onto the LLPathingSystem
	void providePathingData( const LLVector3& point1, const LLVector3& point2 );

	ERegionOverlayDisplay getRegionOverlayDisplay() const;
    void                  setRegionOverlayDisplay(ERegionOverlayDisplay pRegionOverlayDisplay);

    EPathSelectionState   getPathSelectionState() const;
    void                  setPathSelectionState(EPathSelectionState pPathSelectionState);

    F32                   getCharacterWidth() const;
    void                  setCharacterWidth(F32 pCharacterWidth);

    ECharacterType        getCharacterType() const;
    void                  setCharacterType(ECharacterType pCharacterType);

    F32                   getTerrainMaterialA() const;
    void                  setTerrainMaterialA(F32 pTerrainMaterial);

    F32                   getTerrainMaterialB() const;
    void                  setTerrainMaterialB(F32 pTerrainMaterial);

    F32                   getTerrainMaterialC() const;
    void                  setTerrainMaterialC(F32 pTerrainMaterial);

    F32                   getTerrainMaterialD() const;
    void                  setTerrainMaterialD(F32 pTerrainMaterial);

	void setHasNavMeshReceived();
	void setHasNoNavMesh();

protected:

private:
	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	virtual void onOpen(const LLSD& pKey);

	void onShowNavMeshToggle();
	void onShowExcludeVolumesToggle();
	void onShowPathToggle();
	void onShowWaterPlaneToggle();
	void onRegionOverlayDisplaySwitch();
	void onPathSelectionSwitch();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onViewEditLinksetClicked();
	void onRebuildNavMeshClicked();
	void onRefreshNavMeshClicked();
	void onTerrainMaterialASet();
	void onTerrainMaterialBSet();
	void onTerrainMaterialCSet();
	void onTerrainMaterialDSet();

	LLCheckBoxCtrl *mShowNavMeshCheckBox;
	LLCheckBoxCtrl *mShowExcludeVolumesCheckBox;
	LLCheckBoxCtrl *mShowPathCheckBox;
	LLCheckBoxCtrl *mShowWaterPlaneCheckBox;
	LLRadioGroup   *mRegionOverlayDisplayRadioGroup;
	LLRadioGroup   *mPathSelectionRadioGroup;
	LLSliderCtrl   *mCharacterWidthSlider;
	LLRadioGroup   *mCharacterTypeRadioGroup;
	LLTextBase     *mPathfindingStatus;
	LLLineEditor   *mTerrainMaterialA;
	LLLineEditor   *mTerrainMaterialB;
	LLLineEditor   *mTerrainMaterialC;
	LLLineEditor   *mTerrainMaterialD;

	LLNavMeshDownloadObserver	mNavMeshDownloadObserver[10];
	int							mCurrentMDO;
	int							mNavMeshCnt;

	//Container that is populated and subsequently submitted to the LLPathingSystem for processing
	LLPathingLib::PathingPacket		mPathData;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
