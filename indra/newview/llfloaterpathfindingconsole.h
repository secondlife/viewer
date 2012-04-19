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
#include "llpathinglib.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmeshzone.h"
#include "llpathfindingpathtool.h"

#include <boost/signals2.hpp>

class LLSD;
class LLPanel;
class LLSliderCtrl;
class LLLineEditor;
class LLTextBase;
class LLCheckBoxCtrl;
class LLTabContainer;
class LLComboBox;
class LLButton;
class LLToolset;
class LLColor4;

class LLFloaterPathfindingConsole
:	public LLFloater
{
	friend class LLFloaterReg;

public:
	virtual BOOL postBuild();
	virtual void onOpen(const LLSD& pKey);
	virtual void onClose(bool pIsAppQuitting);

	static LLHandle<LLFloaterPathfindingConsole> getInstanceHandle();

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
	
	BOOL isRenderXRay() const;
	void setRenderXRay(BOOL pIsRenderXRay);
	
	BOOL isRenderAnyShapes() const;
	U32  getRenderShapeFlags();

	LLPathingLib::LLPLCharacterType getRenderHeatmapType() const;
	void                            setRenderHeatmapType(LLPathingLib::LLPLCharacterType pRenderHeatmapType);

protected:

private:
	typedef enum
	{
		kConsoleStateUnknown,
		kConsoleStateLibraryNotImplemented,
		kConsoleStateRegionNotEnabled,
		kConsoleStateCheckingVersion,
		kConsoleStateDownloading,
		kConsoleStateHasNavMesh,
		kConsoleStateError
	} EConsoleState;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	void onShowWalkabilitySet();
	void onViewCharactersClicked();
	void onTabSwitch();
	void onUnfreezeClicked();
	void onFreezeClicked();
	void onViewEditLinksetClicked();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onClearPathClicked();
	void onNavMeshZoneCB(LLPathfindingNavMeshZone::ENavMeshZoneRequestStatus pNavMeshZoneRequestStatus);
	void onAgentStateCB(LLPathfindingManager::EAgentState pAgentState);
	void onRegionBoundaryCross();
	void onPathEvent();

	void setDefaultInputs();
	void setConsoleState(EConsoleState pConsoleState);

	void        updateControlsOnConsoleState();
	void        updateStatusOnConsoleState();
	std::string getSimulatorStatusText() const;

	void initializeNavMeshZoneForCurrentRegion();

	void setAgentState(LLPathfindingManager::EAgentState pAgentState);

	void switchIntoTestPathMode();
	void switchOutOfTestPathMode();
	void updateCharacterWidth();
	void updateCharacterType();
	void clearPath();
	void updatePathTestStatus();

	void fillInColorsForNavMeshVisualization();

	LLRootHandle<LLFloaterPathfindingConsole>     mSelfHandle;
	LLCheckBoxCtrl                                *mShowNavMeshCheckBox;
	LLComboBox                                    *mShowNavMeshWalkabilityComboBox;
	LLCheckBoxCtrl                                *mShowWalkablesCheckBox;
	LLCheckBoxCtrl                                *mShowStaticObstaclesCheckBox;
	LLCheckBoxCtrl                                *mShowMaterialVolumesCheckBox;
	LLCheckBoxCtrl                                *mShowExclusionVolumesCheckBox;
	LLCheckBoxCtrl                                *mShowWorldCheckBox;
	LLCheckBoxCtrl								  *mShowXRayCheckBox;
	LLTextBase                                    *mPathfindingViewerStatus;
	LLTextBase                                    *mPathfindingSimulatorStatus;
	LLButton                                      *mViewCharactersButton;
	LLTabContainer                                *mEditTestTabContainer;
	LLPanel                                       *mEditTab;
	LLPanel                                       *mTestTab;
	LLTextBase                                    *mUnfreezeLabel;
	LLButton                                      *mUnfreezeButton;
	LLTextBase                                    *mLinksetsLabel;
	LLButton                                      *mLinksetsButton;
	LLTextBase                                    *mFreezeLabel;
	LLButton                                      *mFreezeButton;
	LLSliderCtrl                                  *mCharacterWidthSlider;
	LLComboBox                                    *mCharacterTypeComboBox;
	LLTextBase                                    *mPathTestingStatus;
	LLButton                                      *mClearPathButton;

	LLPathfindingNavMeshZone::navmesh_zone_slot_t mNavMeshZoneSlot;
	LLPathfindingNavMeshZone                      mNavMeshZone;
	bool                                          mIsNavMeshUpdating;

	LLPathfindingManager::agent_state_slot_t      mAgentStateSlot;
	boost::signals2::connection                   mRegionBoundarySlot;
	LLPathfindingPathTool::path_event_slot_t      mPathEventSlot;

	LLToolset                                     *mPathfindingToolset;
	LLToolset                                     *mSavedToolset;

	EConsoleState                                 mConsoleState;

	static LLHandle<LLFloaterPathfindingConsole>  sInstanceHandle;
	
	LLPathingLib::NavMeshColors                   mNavMeshColors;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
