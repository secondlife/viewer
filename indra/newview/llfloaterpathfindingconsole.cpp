/** 
* @file llfloaterpathfindingconsole.cpp
* @brief "Pathfinding console" floater, allowing for viewing and testing of the pathfinding navmesh through Havok AI utilities.
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

#include "llfloaterpathfindingconsole.h"

#include <vector>

#include <boost/signals2.hpp>

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcontrol.h"
#include "llenvmanager.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterreg.h"
#include "llhandle.h"
#include "llpanel.h"
#include "llpathfindingnavmeshzone.h"
#include "llpathfindingpathtool.h"
#include "llpathinglib.h"
#include "llsliderctrl.h"
#include "llsd.h"
#include "lltabcontainer.h"
#include "lltextbase.h"
#include "lltoolmgr.h"
#include "lltoolfocus.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "pipeline.h"

#define XUI_RENDER_HEATMAP_NONE 0
#define XUI_RENDER_HEATMAP_A 1
#define XUI_RENDER_HEATMAP_B 2
#define XUI_RENDER_HEATMAP_C 3
#define XUI_RENDER_HEATMAP_D 4

#define XUI_CHARACTER_TYPE_NONE 0
#define XUI_CHARACTER_TYPE_A 1
#define XUI_CHARACTER_TYPE_B 2
#define XUI_CHARACTER_TYPE_C 3
#define XUI_CHARACTER_TYPE_D 4

#define XUI_VIEW_TAB_INDEX 0
#define XUI_TEST_TAB_INDEX 1

#define SET_SHAPE_RENDER_FLAG(_flag,_type) _flag |= (1U << _type)

#define CONTROL_NAME_RETRIEVE_NEIGHBOR       "PathfindingRetrieveNeighboringRegion"
#define CONTROL_NAME_WALKABLE_OBJECTS        "PathfindingWalkable"
#define CONTROL_NAME_STATIC_OBSTACLE_OBJECTS "PathfindingObstacle"
#define CONTROL_NAME_MATERIAL_VOLUMES        "PathfindingMaterial"
#define CONTROL_NAME_EXCLUSION_VOLUMES       "PathfindingExclusion"
#define CONTROL_NAME_INTERIOR_EDGE           "PathfindingConnectedEdge"
#define CONTROL_NAME_EXTERIOR_EDGE           "PathfindingBoundaryEdge"
#define CONTROL_NAME_HEATMAP_MIN             "PathfindingHeatColorBase"
#define CONTROL_NAME_HEATMAP_MAX             "PathfindingHeatColorMax"
#define CONTROL_NAME_NAVMESH_FACE            "PathfindingFaceColor"
#define CONTROL_NAME_TEST_PATH_VALID_END     "PathfindingTestPathValidEndColor"
#define CONTROL_NAME_TEST_PATH_INVALID_END   "PathfindingTestPathInvalidEndColor"
#define CONTROL_NAME_TEST_PATH               "PathfindingTestPathColor"
#define CONTROL_NAME_WATER					 "PathfindingWaterColor"

LLHandle<LLFloaterPathfindingConsole> LLFloaterPathfindingConsole::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingConsole
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingConsole::postBuild()
{
	mViewTestTabContainer = findChild<LLTabContainer>("view_test_tab_container");
	llassert(mViewTestTabContainer != NULL);
	mViewTestTabContainer->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onTabSwitch, this));

	mViewTab = findChild<LLPanel>("view_panel");
	llassert(mViewTab != NULL);

	mShowLabel = findChild<LLTextBase>("show_label");
	llassert(mShowLabel != NULL);

	mShowWorldCheckBox = findChild<LLCheckBoxCtrl>("show_world");
	llassert(mShowWorldCheckBox != NULL);
	mShowWorldCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWorldSet, this));
	
	mShowWorldMovablesOnlyCheckBox = findChild<LLCheckBoxCtrl>("show_world_movables_only");
	llassert(mShowWorldMovablesOnlyCheckBox != NULL);
	mShowWorldMovablesOnlyCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWorldMovablesOnlySet, this));

	mShowNavMeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh");
	llassert(mShowNavMeshCheckBox != NULL);
	mShowNavMeshCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowNavMeshSet, this));

	mShowNavMeshWalkabilityLabel = findChild<LLTextBase>("show_walkability_label");
	llassert(mShowNavMeshWalkabilityLabel != NULL);

	mShowNavMeshWalkabilityComboBox = findChild<LLComboBox>("show_heatmap_mode");
	llassert(mShowNavMeshWalkabilityComboBox != NULL);
	mShowNavMeshWalkabilityComboBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWalkabilitySet, this));

	mShowWalkablesCheckBox = findChild<LLCheckBoxCtrl>("show_walkables");
	llassert(mShowWalkablesCheckBox != NULL);

	mShowStaticObstaclesCheckBox = findChild<LLCheckBoxCtrl>("show_static_obstacles");
	llassert(mShowStaticObstaclesCheckBox != NULL);

	mShowMaterialVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_material_volumes");
	llassert(mShowMaterialVolumesCheckBox != NULL);

	mShowExclusionVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_exclusion_volumes");
	llassert(mShowExclusionVolumesCheckBox != NULL);

	mShowRenderWaterPlaneCheckBox = findChild<LLCheckBoxCtrl>("show_water_plane");
	llassert(mShowRenderWaterPlaneCheckBox != NULL);

	mShowXRayCheckBox = findChild<LLCheckBoxCtrl>("show_xray");
	llassert(mShowXRayCheckBox != NULL);

	mTestTab = findChild<LLPanel>("test_panel");
	llassert(mTestTab != NULL);

	mPathfindingViewerStatus = findChild<LLTextBase>("pathfinding_viewer_status");
	llassert(mPathfindingViewerStatus != NULL);

	mPathfindingSimulatorStatus = findChild<LLTextBase>("pathfinding_simulator_status");
	llassert(mPathfindingSimulatorStatus != NULL);

	mCtrlClickLabel = findChild<LLTextBase>("ctrl_click_label");
	llassert(mCtrlClickLabel != NULL);

	mShiftClickLabel = findChild<LLTextBase>("shift_click_label");
	llassert(mShiftClickLabel != NULL);

	mCharacterWidthLabel = findChild<LLTextBase>("character_width_label");
	llassert(mCharacterWidthLabel != NULL);

	mCharacterWidthSlider = findChild<LLSliderCtrl>("character_width");
	llassert(mCharacterWidthSlider != NULL);
	mCharacterWidthSlider->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterWidthSet, this));

	mCharacterWidthUnitLabel = findChild<LLTextBase>("character_width_unit_label");
	llassert(mCharacterWidthUnitLabel != NULL);

	mCharacterTypeLabel = findChild<LLTextBase>("character_type_label");
	llassert(mCharacterTypeLabel != NULL);

	mCharacterTypeComboBox = findChild<LLComboBox>("path_character_type");
	llassert(mCharacterTypeComboBox != NULL);
	mCharacterTypeComboBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterTypeSwitch, this));

	mPathTestingStatus = findChild<LLTextBase>("path_test_status");
	llassert(mPathTestingStatus != NULL);

	mClearPathButton = findChild<LLButton>("clear_path");
	llassert(mClearPathButton != NULL);
	mClearPathButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onClearPathClicked, this));

	mErrorColor = LLUIColorTable::instance().getColor("PathfindingErrorColor");
	mWarningColor = LLUIColorTable::instance().getColor("PathfindingWarningColor");

	if (LLPathingLib::getInstance() != NULL)
	{
		mPathfindingToolset = new LLToolset();
		mPathfindingToolset->addTool(LLPathfindingPathTool::getInstance());
		mPathfindingToolset->addTool(LLToolCamera::getInstance());
		mPathfindingToolset->setShowFloaterTools(false);
	}

	updateCharacterWidth();
	updateCharacterType();

	return LLFloater::postBuild();
}

void LLFloaterPathfindingConsole::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);
	//make sure we have a pathing system
	if ( LLPathingLib::getInstance() == NULL )
	{ 
		setConsoleState(kConsoleStateLibraryNotImplemented);
		llwarns <<"Errror: cannot find pathing library implementation."<<llendl;
	}
	else
	{	
		if (!mNavMeshZoneSlot.connected())
		{
			mNavMeshZoneSlot = mNavMeshZone.registerNavMeshZoneListener(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshZoneStatus, this, _1));
		}

		mIsNavMeshUpdating = false;
		initializeNavMeshZoneForCurrentRegion();
		registerSavedSettingsListeners();
		fillInColorsForNavMeshVisualization();
	}		

	if (!mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterPathfindingConsole::onRegionBoundaryCross, this));
	}

	if (!mTeleportFailedSlot.connected())
	{
		mTeleportFailedSlot = LLViewerParcelMgr::getInstance()->setTeleportFailedCallback(boost::bind(&LLFloaterPathfindingConsole::onRegionBoundaryCross, this));
	}

	if (!mPathEventSlot.connected())
	{
		mPathEventSlot = LLPathfindingPathTool::getInstance()->registerPathEventListener(boost::bind(&LLFloaterPathfindingConsole::onPathEvent, this));
	}

	setDefaultInputs();
	updatePathTestStatus();

	if (mViewTestTabContainer->getCurrentPanelIndex() == XUI_TEST_TAB_INDEX)
	{
		switchIntoTestPathMode();
	}
}

void LLFloaterPathfindingConsole::onClose(bool pIsAppQuitting)
{
	switchOutOfTestPathMode();
	
	if (mPathEventSlot.connected())
	{
		mPathEventSlot.disconnect();
	}

	if (mTeleportFailedSlot.connected())
	{
		mTeleportFailedSlot.disconnect();
	}

	if (mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot.disconnect();
	}

	if (mNavMeshZoneSlot.connected())
	{
		mNavMeshZoneSlot.disconnect();
	}

	if (LLPathingLib::getInstance() != NULL)
	{
		mNavMeshZone.disable();
	}
	deregisterSavedSettingsListeners();

	setDefaultInputs();
	setConsoleState(kConsoleStateUnknown);
	cleanupRenderableRestoreItems();

	LLFloater::onClose(pIsAppQuitting);
}

LLHandle<LLFloaterPathfindingConsole> LLFloaterPathfindingConsole::getInstanceHandle()
{
	if (sInstanceHandle.isDead())
	{
		LLFloaterPathfindingConsole *floaterInstance = LLFloaterReg::findTypedInstance<LLFloaterPathfindingConsole>("pathfinding_console");
		if (floaterInstance != NULL)
		{
			sInstanceHandle = floaterInstance->mSelfHandle;
		}
	}

	return sInstanceHandle;
}

BOOL LLFloaterPathfindingConsole::isRenderNavMesh() const
{
	return mShowNavMeshCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderNavMesh(BOOL pIsRenderNavMesh)
{
	mShowNavMeshCheckBox->set(pIsRenderNavMesh);
	setNavMeshRenderState();
}

BOOL LLFloaterPathfindingConsole::isRenderWalkables() const
{
	return mShowWalkablesCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderWalkables(BOOL pIsRenderWalkables)
{
	mShowWalkablesCheckBox->set(pIsRenderWalkables);
}

BOOL LLFloaterPathfindingConsole::isRenderStaticObstacles() const
{
	return mShowStaticObstaclesCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderStaticObstacles(BOOL pIsRenderStaticObstacles)
{
	mShowStaticObstaclesCheckBox->set(pIsRenderStaticObstacles);
}

BOOL LLFloaterPathfindingConsole::isRenderMaterialVolumes() const
{
	return mShowMaterialVolumesCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderMaterialVolumes(BOOL pIsRenderMaterialVolumes)
{
	mShowMaterialVolumesCheckBox->set(pIsRenderMaterialVolumes);
}

BOOL LLFloaterPathfindingConsole::isRenderExclusionVolumes() const
{
	return mShowExclusionVolumesCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderExclusionVolumes(BOOL pIsRenderExclusionVolumes)
{
	mShowExclusionVolumesCheckBox->set(pIsRenderExclusionVolumes);
}

BOOL LLFloaterPathfindingConsole::isRenderWorld() const
{
	return mShowWorldCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderWorld(BOOL pIsRenderWorld)
{
	mShowWorldCheckBox->set(pIsRenderWorld);
	setWorldRenderState();
}

BOOL LLFloaterPathfindingConsole::isRenderWorldMovablesOnly() const
{
	return (mShowWorldCheckBox->get() && mShowWorldMovablesOnlyCheckBox->get());
}

void LLFloaterPathfindingConsole::setRenderWorldMovablesOnly(BOOL pIsRenderWorldMovablesOnly)
{
	mShowWorldMovablesOnlyCheckBox->set(pIsRenderWorldMovablesOnly);
}

BOOL LLFloaterPathfindingConsole::isRenderWaterPlane() const
{
	return mShowRenderWaterPlaneCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderWaterPlane(BOOL pIsRenderWaterPlane)
{
	mShowRenderWaterPlaneCheckBox->set(pIsRenderWaterPlane);
}

BOOL LLFloaterPathfindingConsole::isRenderXRay() const
{
	return mShowXRayCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderXRay(BOOL pIsRenderXRay)
{
	mShowXRayCheckBox->set(pIsRenderXRay);
}

LLPathingLib::LLPLCharacterType LLFloaterPathfindingConsole::getRenderHeatmapType() const
{
	LLPathingLib::LLPLCharacterType renderHeatmapType;

	switch (mShowNavMeshWalkabilityComboBox->getValue().asInteger())
	{
	case XUI_RENDER_HEATMAP_NONE :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		break;
	case XUI_RENDER_HEATMAP_A :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_A;
		break;
	case XUI_RENDER_HEATMAP_B :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_B;
		break;
	case XUI_RENDER_HEATMAP_C :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_C;
		break;
	case XUI_RENDER_HEATMAP_D :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_D;
		break;
	default :
		renderHeatmapType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
		llassert(0);
		break;
	}

	return renderHeatmapType;
}

void LLFloaterPathfindingConsole::setRenderHeatmapType(LLPathingLib::LLPLCharacterType pRenderHeatmapType)
{
	LLSD comboBoxValue;

	switch (pRenderHeatmapType)
	{
	case LLPathingLib::LLPL_CHARACTER_TYPE_NONE :
		comboBoxValue = XUI_RENDER_HEATMAP_NONE;
		break;
	case LLPathingLib::LLPL_CHARACTER_TYPE_A :
		comboBoxValue = XUI_RENDER_HEATMAP_A;
		break;
	case LLPathingLib::LLPL_CHARACTER_TYPE_B :
		comboBoxValue = XUI_RENDER_HEATMAP_B;
		break;
	case LLPathingLib::LLPL_CHARACTER_TYPE_C :
		comboBoxValue = XUI_RENDER_HEATMAP_C;
		break;
	case LLPathingLib::LLPL_CHARACTER_TYPE_D :
		comboBoxValue = XUI_RENDER_HEATMAP_D;
		break;
	default :
		comboBoxValue = XUI_RENDER_HEATMAP_NONE;
		llassert(0);
		break;
	}

	mShowNavMeshWalkabilityComboBox->setValue(comboBoxValue);
}

LLFloaterPathfindingConsole::LLFloaterPathfindingConsole(const LLSD& pSeed)
	: LLFloater(pSeed),
	mSelfHandle(),
	mViewTestTabContainer(NULL),
	mViewTab(NULL),
	mShowLabel(NULL),
	mShowWorldCheckBox(NULL),
	mShowWorldMovablesOnlyCheckBox(NULL),
	mShowNavMeshCheckBox(NULL),
	mShowNavMeshWalkabilityLabel(NULL),
	mShowNavMeshWalkabilityComboBox(NULL),
	mShowWalkablesCheckBox(NULL),
	mShowStaticObstaclesCheckBox(NULL),
	mShowMaterialVolumesCheckBox(NULL),
	mShowExclusionVolumesCheckBox(NULL),
	mShowRenderWaterPlaneCheckBox(NULL),
	mShowXRayCheckBox(NULL),
	mPathfindingViewerStatus(NULL),
	mPathfindingSimulatorStatus(NULL),
	mTestTab(NULL),
	mCtrlClickLabel(),
	mShiftClickLabel(),
	mCharacterWidthLabel(),
	mCharacterWidthUnitLabel(),
	mCharacterWidthSlider(NULL),
	mCharacterTypeLabel(),
	mCharacterTypeComboBox(NULL),
	mPathTestingStatus(NULL),
	mClearPathButton(NULL),
	mErrorColor(),
	mWarningColor(),
	mNavMeshZoneSlot(),
	mNavMeshZone(),
	mIsNavMeshUpdating(false),
	mRegionBoundarySlot(),
	mTeleportFailedSlot(),
	mPathEventSlot(),
	mPathfindingToolset(NULL),
	mSavedToolset(NULL),
	mSavedSettingRetrieveNeighborSlot(),
	mSavedSettingWalkableSlot(),
	mSavedSettingStaticObstacleSlot(),
	mSavedSettingMaterialVolumeSlot(),
	mSavedSettingExclusionVolumeSlot(),
	mSavedSettingInteriorEdgeSlot(),
	mSavedSettingExteriorEdgeSlot(),
	mSavedSettingHeatmapMinSlot(),
	mSavedSettingHeatmapMaxSlot(),
	mSavedSettingNavMeshFaceSlot(),
	mSavedSettingTestPathValidEndSlot(),
	mSavedSettingTestPathInvalidEndSlot(),
	mSavedSettingTestPathSlot(),
	mSavedSettingWaterSlot(),
	mConsoleState(kConsoleStateUnknown),
	mRenderableRestoreList()
{
	mSelfHandle.bind(this);
}

LLFloaterPathfindingConsole::~LLFloaterPathfindingConsole()
{
}

void LLFloaterPathfindingConsole::onTabSwitch()
{
	if (mViewTestTabContainer->getCurrentPanelIndex() == XUI_TEST_TAB_INDEX)
	{
		switchIntoTestPathMode();
	}
	else
	{
		switchOutOfTestPathMode();
	}
}

void LLFloaterPathfindingConsole::onShowWorldSet()
{
	setWorldRenderState();
	updateRenderablesObjects();
}

void LLFloaterPathfindingConsole::onShowWorldMovablesOnlySet()
{
	updateRenderablesObjects();
}

void LLFloaterPathfindingConsole::onShowNavMeshSet()
{
	setNavMeshRenderState();
}

void LLFloaterPathfindingConsole::onShowWalkabilitySet()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::getInstance()->setNavMeshMaterialType(getRenderHeatmapType());
	}
}

void LLFloaterPathfindingConsole::onCharacterWidthSet()
{
	updateCharacterWidth();
}

void LLFloaterPathfindingConsole::onCharacterTypeSwitch()
{
	updateCharacterType();
}

void LLFloaterPathfindingConsole::onClearPathClicked()
{
	clearPath();
}

void LLFloaterPathfindingConsole::handleNavMeshZoneStatus(LLPathfindingNavMeshZone::ENavMeshZoneRequestStatus pNavMeshZoneRequestStatus)
{
	switch (pNavMeshZoneRequestStatus)
	{
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestUnknown :
		setConsoleState(kConsoleStateUnknown);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestWaiting :
		setConsoleState(kConsoleStateRegionLoading);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestChecking :
		setConsoleState(kConsoleStateCheckingVersion);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestNeedsUpdate :
		mIsNavMeshUpdating = true;
		mNavMeshZone.refresh();
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestStarted :
		setConsoleState(kConsoleStateDownloading);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestCompleted :
		mIsNavMeshUpdating = false;
		setConsoleState(kConsoleStateHasNavMesh);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestNotEnabled :
		setConsoleState(kConsoleStateRegionNotEnabled);
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestError :
		setConsoleState(kConsoleStateError);
		break;
	default:
		setConsoleState(kConsoleStateUnknown);
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::onRegionBoundaryCross()
{	
	initializeNavMeshZoneForCurrentRegion();	
	setRenderWorld(TRUE);
	setRenderWorldMovablesOnly(FALSE);
}

void LLFloaterPathfindingConsole::onPathEvent()
{
	const LLPathfindingPathTool *pathToolInstance = LLPathfindingPathTool::getInstance();

	mCharacterWidthSlider->setValue(LLSD(pathToolInstance->getCharacterWidth()));

	LLSD characterType;
	switch (pathToolInstance->getCharacterType())
	{
	case LLPathfindingPathTool::kCharacterTypeNone :
		characterType = XUI_CHARACTER_TYPE_NONE;
		break;
	case LLPathfindingPathTool::kCharacterTypeA :
		characterType = XUI_CHARACTER_TYPE_A;
		break;
	case LLPathfindingPathTool::kCharacterTypeB :
		characterType = XUI_CHARACTER_TYPE_B;
		break;
	case LLPathfindingPathTool::kCharacterTypeC :
		characterType = XUI_CHARACTER_TYPE_C;
		break;
	case LLPathfindingPathTool::kCharacterTypeD :
		characterType = XUI_CHARACTER_TYPE_D;
		break;
	default :
		characterType = XUI_CHARACTER_TYPE_NONE;
		llassert(0);
		break;
	}
	mCharacterTypeComboBox->setValue(characterType);

	updatePathTestStatus();
}

void LLFloaterPathfindingConsole::setDefaultInputs()
{
	mViewTestTabContainer->selectTab(XUI_VIEW_TAB_INDEX);
	setRenderWorld(TRUE);
	setRenderWorldMovablesOnly(FALSE);
	setRenderNavMesh(FALSE);
	setRenderWalkables(FALSE);
	setRenderMaterialVolumes(FALSE);
	setRenderStaticObstacles(FALSE);
	setRenderExclusionVolumes(FALSE);
	setRenderWaterPlane(FALSE);
	setRenderXRay(FALSE);
}

void LLFloaterPathfindingConsole::setConsoleState(EConsoleState pConsoleState)
{
	mConsoleState = pConsoleState;
	updateControlsOnConsoleState();
	updateViewerStatusOnConsoleState();
	updateSimulatorStatusOnConsoleState();
}

void LLFloaterPathfindingConsole::setWorldRenderState()
{
	BOOL renderWorld = isRenderWorld();

	mShowWorldMovablesOnlyCheckBox->setEnabled(renderWorld && mShowWorldCheckBox->getEnabled());
	if (!renderWorld)
	{
		mShowWorldMovablesOnlyCheckBox->set(FALSE);
	}
}

void LLFloaterPathfindingConsole::setNavMeshRenderState()
{
	BOOL renderNavMesh = isRenderNavMesh();

	mShowNavMeshWalkabilityLabel->setEnabled(renderNavMesh);
	mShowNavMeshWalkabilityComboBox->setEnabled(renderNavMesh);
}

void LLFloaterPathfindingConsole::updateRenderablesObjects()
{
	if ( isRenderWorldMovablesOnly() )
	{
		gPipeline.hidePermanentObjects( mRenderableRestoreList );
	}
	else
	{
		cleanupRenderableRestoreItems();
	}
}

void LLFloaterPathfindingConsole::updateControlsOnConsoleState()
{
	switch (mConsoleState)
	{
	case kConsoleStateUnknown :
	case kConsoleStateRegionNotEnabled :
	case kConsoleStateRegionLoading :
		mViewTestTabContainer->selectTab(XUI_VIEW_TAB_INDEX);
		mViewTab->setEnabled(FALSE);
		mShowLabel->setEnabled(FALSE);
		mShowWorldCheckBox->setEnabled(FALSE);
		mShowWorldMovablesOnlyCheckBox->setEnabled(FALSE);
		mShowNavMeshCheckBox->setEnabled(FALSE);
		mShowNavMeshWalkabilityLabel->setEnabled(FALSE);
		mShowNavMeshWalkabilityComboBox->setEnabled(FALSE);
		mShowWalkablesCheckBox->setEnabled(FALSE);
		mShowStaticObstaclesCheckBox->setEnabled(FALSE);
		mShowMaterialVolumesCheckBox->setEnabled(FALSE);
		mShowExclusionVolumesCheckBox->setEnabled(FALSE);
		mShowRenderWaterPlaneCheckBox->setEnabled(FALSE);
		mShowXRayCheckBox->setEnabled(FALSE);
		mTestTab->setEnabled(FALSE);
		mCtrlClickLabel->setEnabled(FALSE);
		mShiftClickLabel->setEnabled(FALSE);
		mCharacterWidthLabel->setEnabled(FALSE);
		mCharacterWidthUnitLabel->setEnabled(FALSE);
		mCharacterWidthSlider->setEnabled(FALSE);
		mCharacterTypeLabel->setEnabled(FALSE);
		mCharacterTypeComboBox->setEnabled(FALSE);
		mClearPathButton->setEnabled(FALSE);
		clearPath();
		break;
	case kConsoleStateLibraryNotImplemented :
		mViewTestTabContainer->selectTab(XUI_VIEW_TAB_INDEX);
		mViewTab->setEnabled(FALSE);
		mShowLabel->setEnabled(FALSE);
		mShowWorldCheckBox->setEnabled(FALSE);
		mShowWorldMovablesOnlyCheckBox->setEnabled(FALSE);
		mShowNavMeshCheckBox->setEnabled(FALSE);
		mShowNavMeshWalkabilityLabel->setEnabled(FALSE);
		mShowNavMeshWalkabilityComboBox->setEnabled(FALSE);
		mShowWalkablesCheckBox->setEnabled(FALSE);
		mShowStaticObstaclesCheckBox->setEnabled(FALSE);
		mShowMaterialVolumesCheckBox->setEnabled(FALSE);
		mShowExclusionVolumesCheckBox->setEnabled(FALSE);
		mShowRenderWaterPlaneCheckBox->setEnabled(FALSE);
		mShowXRayCheckBox->setEnabled(FALSE);
		mTestTab->setEnabled(FALSE);
		mCtrlClickLabel->setEnabled(FALSE);
		mShiftClickLabel->setEnabled(FALSE);
		mCharacterWidthLabel->setEnabled(FALSE);
		mCharacterWidthUnitLabel->setEnabled(FALSE);
		mCharacterWidthSlider->setEnabled(FALSE);
		mCharacterTypeLabel->setEnabled(FALSE);
		mCharacterTypeComboBox->setEnabled(FALSE);
		mClearPathButton->setEnabled(FALSE);
		clearPath();
		break;
	case kConsoleStateCheckingVersion :
	case kConsoleStateDownloading :
	case kConsoleStateError :
		mViewTestTabContainer->selectTab(XUI_VIEW_TAB_INDEX);
		mViewTab->setEnabled(FALSE);
		mShowLabel->setEnabled(FALSE);
		mShowWorldCheckBox->setEnabled(FALSE);
		mShowWorldMovablesOnlyCheckBox->setEnabled(FALSE);
		mShowNavMeshCheckBox->setEnabled(FALSE);
		mShowNavMeshWalkabilityLabel->setEnabled(FALSE);
		mShowNavMeshWalkabilityComboBox->setEnabled(FALSE);
		mShowWalkablesCheckBox->setEnabled(FALSE);
		mShowStaticObstaclesCheckBox->setEnabled(FALSE);
		mShowMaterialVolumesCheckBox->setEnabled(FALSE);
		mShowExclusionVolumesCheckBox->setEnabled(FALSE);
		mShowRenderWaterPlaneCheckBox->setEnabled(FALSE);
		mShowXRayCheckBox->setEnabled(FALSE);
		mTestTab->setEnabled(FALSE);
		mCtrlClickLabel->setEnabled(FALSE);
		mShiftClickLabel->setEnabled(FALSE);
		mCharacterWidthLabel->setEnabled(FALSE);
		mCharacterWidthUnitLabel->setEnabled(FALSE);
		mCharacterWidthSlider->setEnabled(FALSE);
		mCharacterTypeLabel->setEnabled(FALSE);
		mCharacterTypeComboBox->setEnabled(FALSE);
		mClearPathButton->setEnabled(FALSE);
		clearPath();
		break;
	case kConsoleStateHasNavMesh :
		mViewTab->setEnabled(TRUE);
		mShowLabel->setEnabled(TRUE);
		mShowWorldCheckBox->setEnabled(TRUE);
		setWorldRenderState();
		mShowNavMeshCheckBox->setEnabled(TRUE);
		setNavMeshRenderState();
		mShowWalkablesCheckBox->setEnabled(TRUE);
		mShowStaticObstaclesCheckBox->setEnabled(TRUE);
		mShowMaterialVolumesCheckBox->setEnabled(TRUE);
		mShowExclusionVolumesCheckBox->setEnabled(TRUE);
		mShowRenderWaterPlaneCheckBox->setEnabled(TRUE);
		mShowXRayCheckBox->setEnabled(TRUE);
		mTestTab->setEnabled(TRUE);
		mCtrlClickLabel->setEnabled(TRUE);
		mShiftClickLabel->setEnabled(TRUE);
		mCharacterWidthLabel->setEnabled(TRUE);
		mCharacterWidthUnitLabel->setEnabled(TRUE);
		mCharacterWidthSlider->setEnabled(TRUE);
		mCharacterTypeLabel->setEnabled(TRUE);
		mCharacterTypeComboBox->setEnabled(TRUE);
		mClearPathButton->setEnabled(TRUE);
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::updateViewerStatusOnConsoleState()
{
	std::string viewerStatusText("");
	LLStyle::Params viewerStyleParams;

	switch (mConsoleState)
	{
	case kConsoleStateUnknown :
		viewerStatusText = getString("navmesh_viewer_status_unknown");
		viewerStyleParams.color = mErrorColor;
		break;
	case kConsoleStateLibraryNotImplemented :
		viewerStatusText = getString("navmesh_viewer_status_library_not_implemented");
		viewerStyleParams.color = mErrorColor;
		break;
	case kConsoleStateRegionNotEnabled :
		viewerStatusText = getString("navmesh_viewer_status_region_not_enabled");
		viewerStyleParams.color = mErrorColor;
		break;
	case kConsoleStateRegionLoading :
		viewerStatusText = getString("navmesh_viewer_status_region_loading");
		viewerStyleParams.color = mWarningColor;
		break;
	case kConsoleStateCheckingVersion :
		viewerStatusText = getString("navmesh_viewer_status_checking_version");
		viewerStyleParams.color = mWarningColor;
		break;
	case kConsoleStateDownloading :
		if (mIsNavMeshUpdating)
		{
			viewerStatusText = getString("navmesh_viewer_status_updating");
		}
		else
		{
			viewerStatusText = getString("navmesh_viewer_status_downloading");
		}
		viewerStyleParams.color = mWarningColor;
		break;
	case kConsoleStateHasNavMesh :
		viewerStatusText = getString("navmesh_viewer_status_has_navmesh");
		break;
	case kConsoleStateError :
		viewerStatusText = getString("navmesh_viewer_status_error");
		viewerStyleParams.color = mErrorColor;
		break;
	default :
		viewerStatusText = getString("navmesh_viewer_status_unknown");
		viewerStyleParams.color = mErrorColor;
		llassert(0);
		break;
	}

	mPathfindingViewerStatus->setText((LLStringExplicit)viewerStatusText, viewerStyleParams);
}

void LLFloaterPathfindingConsole::updateSimulatorStatusOnConsoleState()
{
	std::string simulatorStatusText("");
	LLStyle::Params simulatorStyleParams;

	switch (mConsoleState)
	{
	case kConsoleStateUnknown :
	case kConsoleStateLibraryNotImplemented :
	case kConsoleStateRegionNotEnabled :
	case kConsoleStateRegionLoading :
	case kConsoleStateCheckingVersion :
	case kConsoleStateError :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		simulatorStyleParams.color = mErrorColor;
		break;
	case kConsoleStateDownloading :
	case kConsoleStateHasNavMesh :
		switch (mNavMeshZone.getNavMeshZoneStatus())
		{
		case LLPathfindingNavMeshZone::kNavMeshZonePending : 
			simulatorStatusText = getString("navmesh_simulator_status_pending");
			simulatorStyleParams.color = mWarningColor;
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_building");
			simulatorStyleParams.color = mWarningColor;
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneSomePending : 
			simulatorStatusText = getString("navmesh_simulator_status_some_pending");
			simulatorStyleParams.color = mWarningColor;
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneSomeBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_some_building");
			simulatorStyleParams.color = mWarningColor;
			break;
		case LLPathfindingNavMeshZone::kNavMeshZonePendingAndBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_pending_and_building");
			simulatorStyleParams.color = mWarningColor;
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneComplete : 
			simulatorStatusText = getString("navmesh_simulator_status_complete");
			break;
		default : 
			simulatorStatusText = getString("navmesh_simulator_status_unknown");
			simulatorStyleParams.color = mErrorColor;
			break;
		}
		break;
	default :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		simulatorStyleParams.color = mErrorColor;
		llassert(0);
		break;
	}

	mPathfindingSimulatorStatus->setText((LLStringExplicit)simulatorStatusText, simulatorStyleParams);
}

void LLFloaterPathfindingConsole::initializeNavMeshZoneForCurrentRegion()
{
	mNavMeshZone.disable();
	mNavMeshZone.initialize();
	mNavMeshZone.enable();
	mNavMeshZone.refresh();
	cleanupRenderableRestoreItems();
}

void LLFloaterPathfindingConsole::cleanupRenderableRestoreItems()
{
	if ( !mRenderableRestoreList.empty() ) 
	{ 
		gPipeline.restorePermanentObjects( mRenderableRestoreList ); 
		mRenderableRestoreList.clear();
	}
	else
	{
		gPipeline.skipRenderingOfTerrain( false );
	}
}

void LLFloaterPathfindingConsole::switchIntoTestPathMode()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		llassert(mPathfindingToolset != NULL);
		LLToolMgr *toolMgrInstance = LLToolMgr::getInstance();
		if (toolMgrInstance->getCurrentToolset() != mPathfindingToolset)
		{
			mSavedToolset = toolMgrInstance->getCurrentToolset();
			toolMgrInstance->setCurrentToolset(mPathfindingToolset);
		}
	}
}

void LLFloaterPathfindingConsole::switchOutOfTestPathMode()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		llassert(mPathfindingToolset != NULL);
		LLToolMgr *toolMgrInstance = LLToolMgr::getInstance();
		if (toolMgrInstance->getCurrentToolset() == mPathfindingToolset)
		{
			toolMgrInstance->setCurrentToolset(mSavedToolset);
			mSavedToolset = NULL;
		}
	}
}

void LLFloaterPathfindingConsole::updateCharacterWidth()
{
	LLPathfindingPathTool::getInstance()->setCharacterWidth(mCharacterWidthSlider->getValueF32());
}

void LLFloaterPathfindingConsole::updateCharacterType()
{
	LLPathfindingPathTool::ECharacterType characterType;

	switch (mCharacterTypeComboBox->getValue().asInteger())
	{
	case XUI_CHARACTER_TYPE_NONE :
		characterType = LLPathfindingPathTool::kCharacterTypeNone;
		break;
	case XUI_CHARACTER_TYPE_A :
		characterType = LLPathfindingPathTool::kCharacterTypeA;
		break;
	case XUI_CHARACTER_TYPE_B :
		characterType = LLPathfindingPathTool::kCharacterTypeB;
		break;
	case XUI_CHARACTER_TYPE_C :
		characterType = LLPathfindingPathTool::kCharacterTypeC;
		break;
	case XUI_CHARACTER_TYPE_D :
		characterType = LLPathfindingPathTool::kCharacterTypeD;
		break;
	default :
		characterType = LLPathfindingPathTool::kCharacterTypeNone;
		llassert(0);
		break;
	}

	LLPathfindingPathTool::getInstance()->setCharacterType(characterType);
}

void LLFloaterPathfindingConsole::clearPath()
{
	LLPathfindingPathTool::getInstance()->clearPath();
}

void LLFloaterPathfindingConsole::updatePathTestStatus()
{
	std::string statusText("");
	LLStyle::Params styleParams;

	switch (LLPathfindingPathTool::getInstance()->getPathStatus())
	{
	case LLPathfindingPathTool::kPathStatusUnknown :
		statusText = getString("pathing_unknown");
		styleParams.color = mErrorColor;
		break;
	case LLPathfindingPathTool::kPathStatusChooseStartAndEndPoints :
		statusText = getString("pathing_choose_start_and_end_points");
		styleParams.color = mWarningColor;
		break;
	case LLPathfindingPathTool::kPathStatusChooseStartPoint :
		statusText = getString("pathing_choose_start_point");
		styleParams.color = mWarningColor;
		break;
	case LLPathfindingPathTool::kPathStatusChooseEndPoint :
		statusText = getString("pathing_choose_end_point");
		styleParams.color = mWarningColor;
		break;
	case LLPathfindingPathTool::kPathStatusHasValidPath :
		statusText = getString("pathing_path_valid");
		break;
	case LLPathfindingPathTool::kPathStatusHasInvalidPath :
		statusText = getString("pathing_path_invalid");
		styleParams.color = mErrorColor;
		break;
	case LLPathfindingPathTool::kPathStatusNotEnabled :
		statusText = getString("pathing_region_not_enabled");
		styleParams.color = mErrorColor;
		break;
	case LLPathfindingPathTool::kPathStatusNotImplemented :
		statusText = getString("pathing_library_not_implemented");
		styleParams.color = mErrorColor;
		break;
	case LLPathfindingPathTool::kPathStatusError :
		statusText = getString("pathing_error");
		styleParams.color = mErrorColor;
		break;
	default :
		statusText = getString("pathing_unknown");
		styleParams.color = mErrorColor;
		break;
	}

	mPathTestingStatus->setText((LLStringExplicit)statusText, styleParams);
}


BOOL LLFloaterPathfindingConsole::isRenderAnyShapes() const
{
	return (isRenderWalkables() || isRenderStaticObstacles() ||
		isRenderMaterialVolumes() ||  isRenderExclusionVolumes());
}

U32 LLFloaterPathfindingConsole::getRenderShapeFlags()
{
	U32 shapeRenderFlag = 0U;

	if (isRenderWalkables())
	{ 
		SET_SHAPE_RENDER_FLAG(shapeRenderFlag, LLPathingLib::LLST_WalkableObjects); 
	}
	if (isRenderStaticObstacles())
	{ 
		SET_SHAPE_RENDER_FLAG(shapeRenderFlag, LLPathingLib::LLST_ObstacleObjects); 
	}
	if (isRenderMaterialVolumes())
	{ 
		SET_SHAPE_RENDER_FLAG(shapeRenderFlag, LLPathingLib::LLST_MaterialPhantoms); 
	}
	if (isRenderExclusionVolumes())
	{ 
		SET_SHAPE_RENDER_FLAG(shapeRenderFlag, LLPathingLib::LLST_ExclusionPhantoms); 
	}

	return shapeRenderFlag;
}

void LLFloaterPathfindingConsole::registerSavedSettingsListeners()
{
	if (!mSavedSettingRetrieveNeighborSlot.connected())
	{
		mSavedSettingRetrieveNeighborSlot = gSavedSettings.getControl(CONTROL_NAME_RETRIEVE_NEIGHBOR)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleRetrieveNeighborChange, this, _1, _2));
	}
	if (!mSavedSettingWalkableSlot.connected())
	{
		mSavedSettingWalkableSlot = gSavedSettings.getControl(CONTROL_NAME_WALKABLE_OBJECTS)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingStaticObstacleSlot.connected())
	{
		mSavedSettingStaticObstacleSlot = gSavedSettings.getControl(CONTROL_NAME_STATIC_OBSTACLE_OBJECTS)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingMaterialVolumeSlot.connected())
	{
		mSavedSettingMaterialVolumeSlot = gSavedSettings.getControl(CONTROL_NAME_MATERIAL_VOLUMES)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingExclusionVolumeSlot.connected())
	{
		mSavedSettingExclusionVolumeSlot = gSavedSettings.getControl(CONTROL_NAME_EXCLUSION_VOLUMES)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingInteriorEdgeSlot.connected())
	{
		mSavedSettingInteriorEdgeSlot = gSavedSettings.getControl(CONTROL_NAME_INTERIOR_EDGE)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingExteriorEdgeSlot.connected())
	{
		mSavedSettingExteriorEdgeSlot = gSavedSettings.getControl(CONTROL_NAME_EXTERIOR_EDGE)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingHeatmapMinSlot.connected())
	{
		mSavedSettingHeatmapMinSlot = gSavedSettings.getControl(CONTROL_NAME_HEATMAP_MIN)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingHeatmapMaxSlot.connected())
	{
		mSavedSettingHeatmapMaxSlot = gSavedSettings.getControl(CONTROL_NAME_HEATMAP_MAX)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingNavMeshFaceSlot.connected())
	{
		mSavedSettingNavMeshFaceSlot = gSavedSettings.getControl(CONTROL_NAME_NAVMESH_FACE)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingTestPathValidEndSlot.connected())
	{
		mSavedSettingTestPathValidEndSlot = gSavedSettings.getControl(CONTROL_NAME_TEST_PATH_VALID_END)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingTestPathInvalidEndSlot.connected())
	{
		mSavedSettingTestPathInvalidEndSlot = gSavedSettings.getControl(CONTROL_NAME_TEST_PATH_INVALID_END)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingTestPathSlot.connected())
	{
		mSavedSettingTestPathSlot = gSavedSettings.getControl(CONTROL_NAME_TEST_PATH)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
	if (!mSavedSettingWaterSlot.connected())
	{
		mSavedSettingWaterSlot = gSavedSettings.getControl(CONTROL_NAME_WATER)->getSignal()->connect(boost::bind(&LLFloaterPathfindingConsole::handleNavMeshColorChange, this, _1, _2));
	}
}

void LLFloaterPathfindingConsole::deregisterSavedSettingsListeners()
{
	if (mSavedSettingRetrieveNeighborSlot.connected())
	{
		mSavedSettingRetrieveNeighborSlot.disconnect();
	}
	if (mSavedSettingWalkableSlot.connected())
	{
		mSavedSettingWalkableSlot.disconnect();
	}
	if (mSavedSettingStaticObstacleSlot.connected())
	{
		mSavedSettingStaticObstacleSlot.disconnect();
	}
	if (mSavedSettingMaterialVolumeSlot.connected())
	{
		mSavedSettingMaterialVolumeSlot.disconnect();
	}
	if (mSavedSettingExclusionVolumeSlot.connected())
	{
		mSavedSettingExclusionVolumeSlot.disconnect();
	}
	if (mSavedSettingInteriorEdgeSlot.connected())
	{
		mSavedSettingInteriorEdgeSlot.disconnect();
	}
	if (mSavedSettingExteriorEdgeSlot.connected())
	{
		mSavedSettingExteriorEdgeSlot.disconnect();
	}
	if (mSavedSettingHeatmapMinSlot.connected())
	{
		mSavedSettingHeatmapMinSlot.disconnect();
	}
	if (mSavedSettingHeatmapMaxSlot.connected())
	{
		mSavedSettingHeatmapMaxSlot.disconnect();
	}
	if (mSavedSettingNavMeshFaceSlot.connected())
	{
		mSavedSettingNavMeshFaceSlot.disconnect();
	}
	if (mSavedSettingTestPathValidEndSlot.connected())
	{
		mSavedSettingTestPathValidEndSlot.disconnect();
	}
	if (mSavedSettingTestPathInvalidEndSlot.connected())
	{
		mSavedSettingTestPathInvalidEndSlot.disconnect();
	}
	if (mSavedSettingTestPathSlot.connected())
	{
		mSavedSettingTestPathSlot.disconnect();
	}
	if (mSavedSettingWaterSlot.connected())
	{
		mSavedSettingWaterSlot.disconnect();
	}
}

void LLFloaterPathfindingConsole::handleRetrieveNeighborChange(LLControlVariable *pControl, const LLSD &pNewValue)
{
	initializeNavMeshZoneForCurrentRegion();
}

void LLFloaterPathfindingConsole::handleNavMeshColorChange(LLControlVariable *pControl, const LLSD &pNewValue)
{
	fillInColorsForNavMeshVisualization();
}

void LLFloaterPathfindingConsole::fillInColorsForNavMeshVisualization()
{
	if (LLPathingLib::getInstance() != NULL)
	{
		LLPathingLib::NavMeshColors navMeshColors;

		LLColor4 in = gSavedSettings.getColor4(CONTROL_NAME_WALKABLE_OBJECTS);
		navMeshColors.mWalkable= LLColor4U(in); 

		in = gSavedSettings.getColor4(CONTROL_NAME_STATIC_OBSTACLE_OBJECTS);
		navMeshColors.mObstacle= LLColor4U(in); 

		in = gSavedSettings.getColor4(CONTROL_NAME_MATERIAL_VOLUMES);
		navMeshColors.mMaterial= LLColor4U(in); 

		in = gSavedSettings.getColor4(CONTROL_NAME_EXCLUSION_VOLUMES);
		navMeshColors.mExclusion= LLColor4U(in); 

		in = gSavedSettings.getColor4(CONTROL_NAME_INTERIOR_EDGE);
		navMeshColors.mConnectedEdge= LLColor4U(in); 

		in = gSavedSettings.getColor4(CONTROL_NAME_EXTERIOR_EDGE);
		navMeshColors.mBoundaryEdge= LLColor4U(in); 

		navMeshColors.mHeatColorBase = gSavedSettings.getColor4(CONTROL_NAME_HEATMAP_MIN);

		navMeshColors.mHeatColorMax = gSavedSettings.getColor4(CONTROL_NAME_HEATMAP_MAX);

		in = gSavedSettings.getColor4(CONTROL_NAME_NAVMESH_FACE);
		navMeshColors.mFaceColor= LLColor4U(in); 	

		in = gSavedSettings.getColor4(CONTROL_NAME_TEST_PATH_VALID_END);
		navMeshColors.mStarValid= LLColor4U(in); 	

		in = gSavedSettings.getColor4(CONTROL_NAME_TEST_PATH_INVALID_END);
		navMeshColors.mStarInvalid= LLColor4U(in);

		in = gSavedSettings.getColor4(CONTROL_NAME_TEST_PATH);
		navMeshColors.mTestPath= LLColor4U(in); 	

		in = gSavedSettings.getColor4(CONTROL_NAME_WATER);
		navMeshColors.mWaterColor= LLColor4U(in); 	

		LLPathingLib::getInstance()->setNavMeshColors(navMeshColors);
	}
}
