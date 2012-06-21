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
class LLControlVariable;

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
	
	BOOL isRenderWorldMovablesOnly() const;
	void setRenderWorldMovablesOnly(BOOL pIsRenderWorldMovablesOnly);

	BOOL isRenderWaterPlane() const;
	void setRenderWaterPlane(BOOL pIsRenderWaterPlane);
	
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
		kConsoleStateRegionLoading,
		kConsoleStateCheckingVersion,
		kConsoleStateDownloading,
		kConsoleStateHasNavMesh,
		kConsoleStateError
	} EConsoleState;

	// Does its own instance management, so clients not allowed
	// to allocate or destroy.
	LLFloaterPathfindingConsole(const LLSD& pSeed);
	virtual ~LLFloaterPathfindingConsole();

	void onTabSwitch();
	void onShowWorldSet();
	void onShowWorldMovablesOnlySet();
	void onShowNavMeshSet();
	void onShowWalkabilitySet();
	void onCharacterWidthSet();
	void onCharacterTypeSwitch();
	void onClearPathClicked();

	void onNavMeshZoneCB(LLPathfindingNavMeshZone::ENavMeshZoneRequestStatus pNavMeshZoneRequestStatus);
	void onRegionBoundaryCross();
	void onPathEvent();

	void setDefaultInputs();
	void setConsoleState(EConsoleState pConsoleState);
	void setWorldRenderState();
	void setNavMeshRenderState();
	void updateRenderablesObjects();

	void        updateControlsOnConsoleState();
	void        updateStatusOnConsoleState();
	std::string getSimulatorStatusText() const;

	void initializeNavMeshZoneForCurrentRegion();

	void switchIntoTestPathMode();
	void switchOutOfTestPathMode();
	void updateCharacterWidth();
	void updateCharacterType();
	void clearPath();
	void updatePathTestStatus();

	void registerSavedSettingsListeners();
	void deregisterSavedSettingsListeners();
	void handleRetrieveNeighborChange(LLControlVariable *pControl, const LLSD &pNewValue);
	void handleNavMeshColorChange(LLControlVariable *pControl, const LLSD &pNewValue);
	void fillInColorsForNavMeshVisualization();
	void cleanupRenderableRestoreItems();

	LLRootHandle<LLFloaterPathfindingConsole>     mSelfHandle;
	LLTabContainer                                *mViewTestTabContainer;
	LLPanel                                       *mViewTab;
	LLTextBase                                    *mShowLabel;
	LLCheckBoxCtrl                                *mShowWorldCheckBox;
	LLCheckBoxCtrl                                *mShowWorldMovablesOnlyCheckBox;
	LLCheckBoxCtrl                                *mShowNavMeshCheckBox;
	LLTextBase                                    *mShowNavMeshWalkabilityLabel;
	LLComboBox                                    *mShowNavMeshWalkabilityComboBox;
	LLCheckBoxCtrl                                *mShowWalkablesCheckBox;
	LLCheckBoxCtrl                                *mShowStaticObstaclesCheckBox;
	LLCheckBoxCtrl                                *mShowMaterialVolumesCheckBox;
	LLCheckBoxCtrl                                *mShowExclusionVolumesCheckBox;
	LLCheckBoxCtrl								  *mShowRenderWaterPlaneCheckBox;
	LLCheckBoxCtrl								  *mShowXRayCheckBox;
	LLTextBase                                    *mPathfindingViewerStatus;
	LLTextBase                                    *mPathfindingSimulatorStatus;
	LLPanel                                       *mTestTab;
	LLTextBase                                    *mCtrlClickLabel;
	LLTextBase                                    *mShiftClickLabel;
	LLTextBase                                    *mCharacterWidthLabel;
	LLTextBase                                    *mCharacterWidthUnitLabel;
	LLSliderCtrl                                  *mCharacterWidthSlider;
	LLTextBase                                    *mCharacterTypeLabel;
	LLComboBox                                    *mCharacterTypeComboBox;
	LLTextBase                                    *mPathTestingStatus;
	LLButton                                      *mClearPathButton;

	LLPathfindingNavMeshZone::navmesh_zone_slot_t mNavMeshZoneSlot;
	LLPathfindingNavMeshZone                      mNavMeshZone;
	bool                                          mIsNavMeshUpdating;

	boost::signals2::connection                   mRegionBoundarySlot;
	boost::signals2::connection                   mTeleportFailedSlot;
	LLPathfindingPathTool::path_event_slot_t      mPathEventSlot;

	LLToolset                                     *mPathfindingToolset;
	LLToolset                                     *mSavedToolset;

	boost::signals2::connection                   mSavedSettingRetrieveNeighborSlot;
	boost::signals2::connection                   mSavedSettingWalkableSlot;
	boost::signals2::connection                   mSavedSettingStaticObstacleSlot;
	boost::signals2::connection                   mSavedSettingMaterialVolumeSlot;
	boost::signals2::connection                   mSavedSettingExclusionVolumeSlot;
	boost::signals2::connection                   mSavedSettingInteriorEdgeSlot;
	boost::signals2::connection                   mSavedSettingExteriorEdgeSlot;
	boost::signals2::connection                   mSavedSettingHeatmapMinSlot;
	boost::signals2::connection                   mSavedSettingHeatmapMaxSlot;
	boost::signals2::connection                   mSavedSettingNavMeshFaceSlot;
	boost::signals2::connection                   mSavedSettingTestPathValidEndSlot;
	boost::signals2::connection                   mSavedSettingTestPathInvalidEndSlot;
	boost::signals2::connection                   mSavedSettingTestPathSlot;
	boost::signals2::connection                   mSavedSettingWaterSlot;

	EConsoleState                                 mConsoleState;
 
	std::vector<U32>							  mRenderableRestoreList;

	static LLHandle<LLFloaterPathfindingConsole>  sInstanceHandle;
};

#endif // LL_LLFLOATERPATHFINDINGCONSOLE_H
