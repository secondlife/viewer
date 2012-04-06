/** 
* @file llfloaterpathfindingconsole.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llfloaterpathfindingconsole.h"
#include "llfloaterpathfindinglinksets.h"
#include "llfloaterpathfindingcharacters.h"

#include "llsd.h"
#include "llhandle.h"
#include "llagent.h"
#include "llpanel.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llsliderctrl.h"
#include "lllineeditor.h"
#include "lltextbase.h"
#include "lltabcontainer.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llpathfindingnavmeshzone.h"
#include "llpathfindingmanager.h"
#include "llenvmanager.h"

#include "LLPathingLib.h"

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

#define XUI_TEST_TAB_INDEX 1

LLHandle<LLFloaterPathfindingConsole> LLFloaterPathfindingConsole::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingConsole
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingConsole::postBuild()
{
	mShowNavMeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh");
	llassert(mShowNavMeshCheckBox != NULL);

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

	mShowWorldCheckBox = findChild<LLCheckBoxCtrl>("show_world");
	llassert(mShowWorldCheckBox != NULL);

	mShowXRayCheckBox = findChild<LLCheckBoxCtrl>("x-ray");
	llassert(mShowXRayCheckBox != NULL);

	mViewCharactersButton = findChild<LLButton>("view_characters_floater");
	llassert(mViewCharactersButton != NULL);
	mViewCharactersButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onViewCharactersClicked, this));

	mEditTestTabContainer = findChild<LLTabContainer>("edit_test_tab_container");
	llassert(mEditTestTabContainer != NULL);

	mEditTab = findChild<LLPanel>("edit_panel");
	llassert(mEditTab != NULL);

	mTestTab = findChild<LLPanel>("test_panel");
	llassert(mTestTab != NULL);

	mUnfreezeLabel = findChild<LLTextBase>("unfreeze_label");
	llassert(mUnfreezeLabel != NULL);

	mUnfreezeButton = findChild<LLButton>("enter_unfrozen_mode");
	llassert(mUnfreezeButton != NULL);
	mUnfreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onUnfreezeClicked, this));

	mLinksetsLabel = findChild<LLTextBase>("edit_linksets_label");
	llassert(mLinksetsLabel != NULL);

	mLinksetsButton = findChild<LLButton>("view_and_edit_linksets");
	llassert(mLinksetsButton != NULL);
	mLinksetsButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onViewEditLinksetClicked, this));

	mFreezeLabel = findChild<LLTextBase>("freeze_label");
	llassert(mFreezeLabel != NULL);

	mFreezeButton = findChild<LLButton>("enter_frozen_mode");
	llassert(mFreezeButton != NULL);
	mFreezeButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onFreezeClicked, this));

	mPathfindingViewerStatus = findChild<LLTextBase>("pathfinding_viewer_status");
	llassert(mPathfindingViewerStatus != NULL);

	mPathfindingSimulatorStatus = findChild<LLTextBase>("pathfinding_simulator_status");
	llassert(mPathfindingSimulatorStatus != NULL);

	mCharacterWidthSlider = findChild<LLSliderCtrl>("character_width");
	llassert(mCharacterWidthSlider != NULL);
	mCharacterWidthSlider->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterWidthSet, this));

	mCharacterTypeComboBox = findChild<LLComboBox>("path_character_type");
	llassert(mCharacterTypeComboBox != NULL);
	mCharacterTypeComboBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterTypeSwitch, this));

	mPathTestingStatus = findChild<LLTextBase>("path_test_status");
	llassert(mPathTestingStatus != NULL);

	mClearPathButton = findChild<LLButton>("clear_path");
	llassert(mClearPathButton != NULL);
	mClearPathButton->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onClearPathClicked, this));

	return LLFloater::postBuild();
}

void LLFloaterPathfindingConsole::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);
	//make sure we have a pathing system
	if ( !LLPathingLib::getInstance() )
	{
		LLPathingLib::initSystem();
	}	
	if ( LLPathingLib::getInstance() == NULL )
	{ 
		setConsoleState(kConsoleStateLibraryNotImplemented);
		llwarns <<"Errror: cannot find pathing library implementation."<<llendl;
	}
	else
	{	
		fillInColorsForNavMeshVisualization();
		if (!mNavMeshZoneSlot.connected())
		{
			mNavMeshZoneSlot = mNavMeshZone.registerNavMeshZoneListener(boost::bind(&LLFloaterPathfindingConsole::onNavMeshZoneCB, this, _1));
		}

		mIsNavMeshUpdating = false;
		initializeNavMeshZoneForCurrentRegion();
	}		

	if (!mAgentStateSlot.connected())
	{
		mAgentStateSlot = LLPathfindingManager::getInstance()->registerAgentStateListener(boost::bind(&LLFloaterPathfindingConsole::onAgentStateCB, this, _1));
	}

	if (!mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterPathfindingConsole::onRegionBoundaryCross, this));
	}

	setAgentState(LLPathfindingManager::getInstance()->getAgentState());
	setDefaultInputs();
	updatePathTestStatus();
}

void LLFloaterPathfindingConsole::onClose(bool pIsAppQuitting)
{
	if (mRegionBoundarySlot.connected())
	{
		mRegionBoundarySlot.disconnect();
	}

	if (mAgentStateSlot.connected())
	{
		mAgentStateSlot.disconnect();
	}

	if (mNavMeshZoneSlot.connected())
	{
		mNavMeshZoneSlot.disconnect();
	}

	if (LLPathingLib::getInstance() != NULL)
	{
		mNavMeshZone.disable();
	}

	setDefaultInputs();
	setConsoleState(kConsoleStateUnknown);

	LLFloater::onClose(pIsAppQuitting);
}

BOOL LLFloaterPathfindingConsole::handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down)
{
	if (isGeneratePathMode(mask, clicktype, down))
	{
		LLVector3 dv = gViewerWindow->mouseDirectionGlobal(x, y);
		LLVector3 mousePos = LLViewerCamera::getInstance()->getOrigin();
		LLVector3 rayStart = mousePos;
		LLVector3 rayEnd = mousePos + dv * 150;

		if (mask & MASK_CONTROL)
		{
			mPathData.mStartPointA = rayStart;
			mPathData.mEndPointA = rayEnd;
			mHasStartPoint = true;
		}
		else if (mask & MASK_SHIFT)
		{
			mPathData.mStartPointB = rayStart;
			mPathData.mEndPointB = rayEnd;
			mHasEndPoint = true;
		}
		generatePath();
		updatePathTestStatus();

		return TRUE;
	}
	else
	{
		return LLFloater::handleAnyMouseClick(x, y, mask, clicktype, down);
	}
}

BOOL LLFloaterPathfindingConsole::isGeneratePathMode(MASK mask, EClickType clicktype, BOOL down) const
{
	return (isShown() && (mEditTestTabContainer->getCurrentPanelIndex() == XUI_TEST_TAB_INDEX) &&
		(clicktype == LLMouseHandler::CLICK_LEFT) && down && 
		(((mask & MASK_CONTROL) && !(mask & (~MASK_CONTROL))) ||
		((mask & MASK_SHIFT) && !(mask & (~MASK_SHIFT)))));
}

LLHandle<LLFloaterPathfindingConsole> LLFloaterPathfindingConsole::getInstanceHandle()
{
	if (sInstanceHandle.isDead())
	{
		LLFloaterPathfindingConsole *floaterInstance = LLFloaterReg::getTypedInstance<LLFloaterPathfindingConsole>("pathfinding_console");
		if (floaterInstance != NULL)
		{
			sInstanceHandle = floaterInstance->mSelfHandle;
		}
	}

	return sInstanceHandle;
}

BOOL LLFloaterPathfindingConsole::isRenderPath() const
{
	return (mHasStartPoint && mHasEndPoint);
}

BOOL LLFloaterPathfindingConsole::isRenderNavMesh() const
{
	return mShowNavMeshCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderNavMesh(BOOL pIsRenderNavMesh)
{
	mShowNavMeshCheckBox->set(pIsRenderNavMesh);
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
}

BOOL LLFloaterPathfindingConsole::isRenderXRay() const
{
	return mShowXRayCheckBox->get();
}

void LLFloaterPathfindingConsole::setRenderXRay(BOOL pIsRenderXRay)
{
	mShowXRayCheckBox->set(pIsRenderXRay);
}

LLFloaterPathfindingConsole::ERenderHeatmapType LLFloaterPathfindingConsole::getRenderHeatmapType() const
{
	ERenderHeatmapType renderHeatmapType;

	switch (mShowNavMeshWalkabilityComboBox->getValue().asInteger())
	{
	case XUI_RENDER_HEATMAP_NONE :
		renderHeatmapType = kRenderHeatmapNone;
		break;
	case XUI_RENDER_HEATMAP_A :
		renderHeatmapType = kRenderHeatmapA;
		break;
	case XUI_RENDER_HEATMAP_B :
		renderHeatmapType = kRenderHeatmapB;
		break;
	case XUI_RENDER_HEATMAP_C :
		renderHeatmapType = kRenderHeatmapC;
		break;
	case XUI_RENDER_HEATMAP_D :
		renderHeatmapType = kRenderHeatmapD;
		break;
	default :
		renderHeatmapType = kRenderHeatmapNone;
		llassert(0);
		break;
	}

	LLPathingLib::getInstance()->rebuildNavMesh( getHeatMapType() );
	return renderHeatmapType;
}

int LLFloaterPathfindingConsole::getHeatMapType() const
{
	//converts the pathfinding console values to the navmesh filter values

	int renderHeatmapType = 4; //none

	switch ( mShowNavMeshWalkabilityComboBox->getValue().asInteger() )
	{
		case XUI_RENDER_HEATMAP_A :
			renderHeatmapType = 0;
			break;
		case XUI_RENDER_HEATMAP_B :
			renderHeatmapType = 1;
			break;
		case XUI_RENDER_HEATMAP_C :
			renderHeatmapType = 2;
			break;
		case XUI_RENDER_HEATMAP_D :
			renderHeatmapType = 3;
			break;
		default :
			renderHeatmapType = 4;
			break;
	}

	return renderHeatmapType;
}


void LLFloaterPathfindingConsole::setRenderHeatmapType(ERenderHeatmapType pRenderHeatmapType)
{
	LLSD comboBoxValue;

	switch (pRenderHeatmapType)
	{
	case kRenderHeatmapNone :
		comboBoxValue = XUI_RENDER_HEATMAP_NONE;
		break;
	case kRenderHeatmapA :
		comboBoxValue = XUI_RENDER_HEATMAP_A;
		break;
	case kRenderHeatmapB :
		comboBoxValue = XUI_RENDER_HEATMAP_B;
		break;
	case kRenderHeatmapC :
		comboBoxValue = XUI_RENDER_HEATMAP_C;
		break;
	case kRenderHeatmapD :
		comboBoxValue = XUI_RENDER_HEATMAP_D;
		break;
	default :
		comboBoxValue = XUI_RENDER_HEATMAP_NONE;
		llassert(0);
		break;
	}

	return mShowNavMeshWalkabilityComboBox->setValue(comboBoxValue);
}

F32 LLFloaterPathfindingConsole::getCharacterWidth() const
{
	return mCharacterWidthSlider->getValueF32();
}

void LLFloaterPathfindingConsole::setCharacterWidth(F32 pCharacterWidth)
{
	mCharacterWidthSlider->setValue(LLSD(pCharacterWidth));
}

LLFloaterPathfindingConsole::ECharacterType LLFloaterPathfindingConsole::getCharacterType() const
{
	ECharacterType characterType;

	switch (mCharacterTypeComboBox->getValue().asInteger())
	{
	case XUI_CHARACTER_TYPE_NONE :
		characterType = kCharacterTypeNone;
		break;
	case XUI_CHARACTER_TYPE_A :
		characterType = kCharacterTypeA;
		break;
	case XUI_CHARACTER_TYPE_B :
		characterType = kCharacterTypeB;
		break;
	case XUI_CHARACTER_TYPE_C :
		characterType = kCharacterTypeC;
		break;
	case XUI_CHARACTER_TYPE_D :
		characterType = kCharacterTypeD;
		break;
	default :
		characterType = kCharacterTypeNone;
		llassert(0);
		break;
	}

	return characterType;
}

void LLFloaterPathfindingConsole::setCharacterType(ECharacterType pCharacterType)
{
	LLSD radioGroupValue;

	switch (pCharacterType)
	{
	case kCharacterTypeNone :
		radioGroupValue = XUI_CHARACTER_TYPE_NONE;
		break;
	case kCharacterTypeA :
		radioGroupValue = XUI_CHARACTER_TYPE_A;
		break;
	case kCharacterTypeB :
		radioGroupValue = XUI_CHARACTER_TYPE_B;
		break;
	case kCharacterTypeC :
		radioGroupValue = XUI_CHARACTER_TYPE_C;
		break;
	case kCharacterTypeD :
		radioGroupValue = XUI_CHARACTER_TYPE_D;
		break;
	default :
		radioGroupValue = XUI_CHARACTER_TYPE_NONE;
		llassert(0);
		break;
	}

	mCharacterTypeComboBox->setValue(radioGroupValue);
}

LLFloaterPathfindingConsole::LLFloaterPathfindingConsole(const LLSD& pSeed)
	: LLFloater(pSeed),
	mSelfHandle(),
	mShowNavMeshCheckBox(NULL),
	mShowNavMeshWalkabilityComboBox(NULL),
	mShowWalkablesCheckBox(NULL),
	mShowStaticObstaclesCheckBox(NULL),
	mShowMaterialVolumesCheckBox(NULL),
	mShowExclusionVolumesCheckBox(NULL),
	mShowWorldCheckBox(NULL),
	mShowXRayCheckBox(NULL),
	mPathfindingViewerStatus(NULL),
	mPathfindingSimulatorStatus(NULL),
	mViewCharactersButton(NULL),
	mEditTestTabContainer(NULL),
	mEditTab(NULL),
	mTestTab(NULL),
	mUnfreezeLabel(NULL),
	mUnfreezeButton(NULL),
	mLinksetsLabel(NULL),
	mLinksetsButton(NULL),
	mFreezeLabel(NULL),
	mFreezeButton(NULL),
	mCharacterWidthSlider(NULL),
	mCharacterTypeComboBox(NULL),
	mPathTestingStatus(NULL),
	mClearPathButton(NULL),
	mNavMeshZoneSlot(),
	mNavMeshZone(),
	mIsNavMeshUpdating(false),
	mAgentStateSlot(),
	mRegionBoundarySlot(),
	mConsoleState(kConsoleStateUnknown),
	mPathData(),
	mHasStartPoint(false),
	mHasEndPoint(false)
{
	mSelfHandle.bind(this);
}

LLFloaterPathfindingConsole::~LLFloaterPathfindingConsole()
{
}

void LLFloaterPathfindingConsole::onShowWalkabilitySet()
{
	switch (getRenderHeatmapType())
	{
	case kRenderHeatmapNone :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mShowNavMeshWalkabilityComboBox->getName() << "' to RenderHeatmapNone"
			<< llendl;
		break;
	case kRenderHeatmapA :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mShowNavMeshWalkabilityComboBox->getName() << "' to RenderHeatmapA"
			<< llendl;
		break;
	case kRenderHeatmapB :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mShowNavMeshWalkabilityComboBox->getName() << "' to RenderHeatmapB"
			<< llendl;
		break;
	case kRenderHeatmapC :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mShowNavMeshWalkabilityComboBox->getName() << "' to RenderHeatmapC"
			<< llendl;
		break;
	case kRenderHeatmapD :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mShowNavMeshWalkabilityComboBox->getName() << "' to RenderHeatmapD"
			<< llendl;
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::onCharacterWidthSet()
{
	generatePath();
	updatePathTestStatus();
}

void LLFloaterPathfindingConsole::onCharacterTypeSwitch()
{
	generatePath();
	updatePathTestStatus();
}

void LLFloaterPathfindingConsole::onViewCharactersClicked()
{
	LLFloaterPathfindingCharacters::openCharactersViewer();
}

void LLFloaterPathfindingConsole::onUnfreezeClicked()
{
	mUnfreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateUnfrozen);
}

void LLFloaterPathfindingConsole::onFreezeClicked()
{
	mFreezeButton->setEnabled(FALSE);
	LLPathfindingManager::getInstance()->requestSetAgentState(LLPathfindingManager::kAgentStateFrozen);
}

void LLFloaterPathfindingConsole::onViewEditLinksetClicked()
{
	LLFloaterPathfindingLinksets::openLinksetsEditor();
}

void LLFloaterPathfindingConsole::onClearPathClicked()
{
	mHasStartPoint = false;
	mHasEndPoint = false;
	updatePathTestStatus();
}

void LLFloaterPathfindingConsole::onNavMeshZoneCB(LLPathfindingNavMeshZone::ENavMeshZoneRequestStatus pNavMeshZoneRequestStatus)
{
	switch (pNavMeshZoneRequestStatus)
	{
	case LLPathfindingNavMeshZone::kNavMeshZoneRequestUnknown :
		setConsoleState(kConsoleStateUnknown);
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

void LLFloaterPathfindingConsole::onAgentStateCB(LLPathfindingManager::EAgentState pAgentState)
{
	setAgentState(pAgentState);
}

void LLFloaterPathfindingConsole::onRegionBoundaryCross()
{
	initializeNavMeshZoneForCurrentRegion();
}

void LLFloaterPathfindingConsole::setDefaultInputs()
{
	mEditTestTabContainer->selectTab(0);
	mShowNavMeshCheckBox->set(FALSE);
	mShowWalkablesCheckBox->set(FALSE);
	mShowMaterialVolumesCheckBox->set(FALSE);
	mShowStaticObstaclesCheckBox->set(FALSE);
	mShowExclusionVolumesCheckBox->set(FALSE);
	mShowWorldCheckBox->set(TRUE);	
	mShowXRayCheckBox->set(FALSE);
}

void LLFloaterPathfindingConsole::setConsoleState(EConsoleState pConsoleState)
{
	mConsoleState = pConsoleState;
	updateControlsOnConsoleState();
	updateStatusOnConsoleState();
}

void LLFloaterPathfindingConsole::updateControlsOnConsoleState()
{
	switch (mConsoleState)
	{
	case kConsoleStateUnknown :
	case kConsoleStateLibraryNotImplemented :
	case kConsoleStateRegionNotEnabled :
		mShowNavMeshCheckBox->setEnabled(FALSE);
		mShowNavMeshWalkabilityComboBox->setEnabled(FALSE);
		mShowWalkablesCheckBox->setEnabled(FALSE);
		mShowStaticObstaclesCheckBox->setEnabled(FALSE);
		mShowMaterialVolumesCheckBox->setEnabled(FALSE);
		mShowExclusionVolumesCheckBox->setEnabled(FALSE);
		mShowWorldCheckBox->setEnabled(FALSE);
		mShowXRayCheckBox->setEnabled(FALSE);
		mViewCharactersButton->setEnabled(FALSE);
		mEditTestTabContainer->selectTab(0);
		mTestTab->setEnabled(FALSE);
		mCharacterWidthSlider->setEnabled(FALSE);
		mCharacterTypeComboBox->setEnabled(FALSE);
		mClearPathButton->setEnabled(FALSE);
		mHasStartPoint = false;
		mHasEndPoint = false;
		break;
	case kConsoleStateCheckingVersion :
	case kConsoleStateDownloading :
	case kConsoleStateError :
		mShowNavMeshCheckBox->setEnabled(FALSE);
		mShowNavMeshWalkabilityComboBox->setEnabled(FALSE);
		mShowWalkablesCheckBox->setEnabled(FALSE);
		mShowStaticObstaclesCheckBox->setEnabled(FALSE);
		mShowMaterialVolumesCheckBox->setEnabled(FALSE);
		mShowExclusionVolumesCheckBox->setEnabled(FALSE);
		mShowWorldCheckBox->setEnabled(FALSE);
		mShowXRayCheckBox->setEnabled(FALSE);
		mViewCharactersButton->setEnabled(TRUE);
		mEditTestTabContainer->selectTab(0);
		mTestTab->setEnabled(FALSE);
		mCharacterWidthSlider->setEnabled(FALSE);
		mCharacterTypeComboBox->setEnabled(FALSE);
		mClearPathButton->setEnabled(FALSE);
		mHasStartPoint = false;
		mHasEndPoint = false;
		break;
	case kConsoleStateHasNavMesh :
		mShowNavMeshCheckBox->setEnabled(TRUE);
		mShowNavMeshWalkabilityComboBox->setEnabled(TRUE);
		mShowWalkablesCheckBox->setEnabled(TRUE);
		mShowStaticObstaclesCheckBox->setEnabled(TRUE);
		mShowMaterialVolumesCheckBox->setEnabled(TRUE);
		mShowExclusionVolumesCheckBox->setEnabled(TRUE);
		mShowWorldCheckBox->setEnabled(TRUE);
		mShowXRayCheckBox->setEnabled(TRUE);
		mViewCharactersButton->setEnabled(TRUE);
		mTestTab->setEnabled(TRUE);
		mCharacterWidthSlider->setEnabled(TRUE);
		mCharacterTypeComboBox->setEnabled(TRUE);
		mClearPathButton->setEnabled(TRUE);
		mTestTab->setEnabled(TRUE);
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::updateStatusOnConsoleState()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string simulatorStatusText("");
	std::string viewerStatusText("");
	LLStyle::Params viewerStyleParams;

	switch (mConsoleState)
	{
	case kConsoleStateUnknown :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_unknown");
		break;
	case kConsoleStateLibraryNotImplemented :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_library_not_implemented");
		viewerStyleParams.color = warningColor;
		break;
	case kConsoleStateRegionNotEnabled :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_region_not_enabled");
		viewerStyleParams.color = warningColor;
		break;
	case kConsoleStateCheckingVersion :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_checking_version");
		break;
	case kConsoleStateDownloading :
		simulatorStatusText = getSimulatorStatusText();
		if (mIsNavMeshUpdating)
		{
			viewerStatusText = getString("navmesh_viewer_status_updating");
		}
		else
		{
			viewerStatusText = getString("navmesh_viewer_status_downloading");
		}
		break;
	case kConsoleStateHasNavMesh :
		simulatorStatusText = getSimulatorStatusText();
		viewerStatusText = getString("navmesh_viewer_status_has_navmesh");
		break;
	case kConsoleStateError :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_error");
		viewerStyleParams.color = warningColor;
		break;
	default :
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		viewerStatusText = getString("navmesh_viewer_status_unknown");
		llassert(0);
		break;
	}

	mPathfindingViewerStatus->setText((LLStringExplicit)viewerStatusText, viewerStyleParams);
	mPathfindingSimulatorStatus->setText((LLStringExplicit)simulatorStatusText);
}

std::string LLFloaterPathfindingConsole::getSimulatorStatusText() const
{
	std::string simulatorStatusText("");

#ifdef DEPRECATED_UNVERSIONED_NAVMESH
	if (LLPathfindingManager::getInstance()->isPathfindingNavMeshVersioningEnabledForCurrentRegionXXX())
	{
		switch (mNavMeshZone.getNavMeshZoneStatus())
		{
		case LLPathfindingNavMeshZone::kNavMeshZonePending : 
			simulatorStatusText = getString("navmesh_simulator_status_pending");
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_building");
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneSomePending : 
			simulatorStatusText = getString("navmesh_simulator_status_some_pending");
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneSomeBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_some_building");
			break;
		case LLPathfindingNavMeshZone::kNavMeshZonePendingAndBuilding : 
			simulatorStatusText = getString("navmesh_simulator_status_pending_and_building");
			break;
		case LLPathfindingNavMeshZone::kNavMeshZoneComplete : 
			simulatorStatusText = getString("navmesh_simulator_status_complete");
			break;
		default : 
			simulatorStatusText = getString("navmesh_simulator_status_unknown");
			break;
		}
	}
	else
	{
		simulatorStatusText = getString("navmesh_simulator_status_region_not_enabled");
	}
#else // DEPRECATED_UNVERSIONED_NAVMESH
	switch (mNavMeshZone.getNavMeshZoneStatus())
	{
	case LLPathfindingNavMeshZone::kNavMeshZonePending : 
		simulatorStatusText = getString("navmesh_simulator_status_pending");
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneBuilding : 
		simulatorStatusText = getString("navmesh_simulator_status_building");
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneSomePending : 
		simulatorStatusText = getString("navmesh_simulator_status_some_pending");
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneSomeBuilding : 
		simulatorStatusText = getString("navmesh_simulator_status_some_building");
		break;
	case LLPathfindingNavMeshZone::kNavMeshZonePendingAndBuilding : 
		simulatorStatusText = getString("navmesh_simulator_status_pending_and_building");
		break;
	case LLPathfindingNavMeshZone::kNavMeshZoneComplete : 
		simulatorStatusText = getString("navmesh_simulator_status_complete");
		break;
	default : 
		simulatorStatusText = getString("navmesh_simulator_status_unknown");
		break;
	}
#endif // DEPRECATED_UNVERSIONED_NAVMESH

	return simulatorStatusText;
}

void LLFloaterPathfindingConsole::initializeNavMeshZoneForCurrentRegion()
{
	mNavMeshZone.disable();
	mNavMeshZone.initialize();
	mNavMeshZone.enable();
	mNavMeshZone.refresh();
}

void LLFloaterPathfindingConsole::setAgentState(LLPathfindingManager::EAgentState pAgentState)
{
	switch (LLPathfindingManager::getInstance()->getLastKnownNonErrorAgentState())
	{
	case LLPathfindingManager::kAgentStateUnknown :
	case LLPathfindingManager::kAgentStateNotEnabled :
		mEditTab->setEnabled(FALSE);
		mUnfreezeLabel->setEnabled(FALSE);
		mUnfreezeButton->setEnabled(FALSE);
		mLinksetsLabel->setEnabled(FALSE);
		mLinksetsButton->setEnabled(FALSE);
		mFreezeLabel->setEnabled(FALSE);
		mFreezeButton->setEnabled(FALSE);
		break;
	case LLPathfindingManager::kAgentStateFrozen :
		mEditTab->setEnabled(TRUE);
		mUnfreezeLabel->setEnabled(TRUE);
		mUnfreezeButton->setEnabled(TRUE);
		mLinksetsLabel->setEnabled(FALSE);
		mLinksetsButton->setEnabled(FALSE);
		mFreezeLabel->setEnabled(FALSE);
		mFreezeButton->setEnabled(FALSE);
		break;
	case LLPathfindingManager::kAgentStateUnfrozen :
		mEditTab->setEnabled(TRUE);
		mUnfreezeLabel->setEnabled(FALSE);
		mUnfreezeButton->setEnabled(FALSE);
		mLinksetsLabel->setEnabled(TRUE);
		mLinksetsButton->setEnabled(TRUE);
		mFreezeLabel->setEnabled(TRUE);
		mFreezeButton->setEnabled(TRUE);
		break;
	default :
		llassert(0);
		break;
	}
}
 
void LLFloaterPathfindingConsole::generatePath()
{
	if (mHasStartPoint && mHasEndPoint)
	{
		mPathData.mCharacterWidth = getCharacterWidth();
		switch (getCharacterType())
		{
		case kCharacterTypeNone :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
			break;
		case kCharacterTypeA :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_A;
			break;
		case kCharacterTypeB :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_B;
			break;
		case kCharacterTypeC :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_C;
			break;
		case kCharacterTypeD :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_D;
			break;
		default :
			mPathData.mCharacterType = LLPathingLib::LLPL_CHARACTER_TYPE_NONE;
			break;
		}
		LLPathingLib::getInstance()->generatePath(mPathData);
	}
}

void LLFloaterPathfindingConsole::updatePathTestStatus()
{
	static const LLColor4 warningColor = LLUIColorTable::instance().getColor("DrYellow");

	std::string statusText("");
	LLStyle::Params styleParams;

	if (!mHasStartPoint && !mHasEndPoint)
	{
		statusText = getString("pathing_choose_start_and_end_points");
		styleParams.color = warningColor;
	}
	else if (!mHasStartPoint && mHasEndPoint)
	{
		statusText = getString("pathing_choose_start_point");
		styleParams.color = warningColor;
	}
	else if (mHasStartPoint && !mHasEndPoint)
	{
		statusText = getString("pathing_choose_end_point");
		styleParams.color = warningColor;
	}
	else
	{
		statusText = getString("pathing_path_valid");
	}

	mPathTestingStatus->setText((LLStringExplicit)statusText, styleParams);
}


BOOL LLFloaterPathfindingConsole::isRenderAnyShapes() const
{
	if ( isRenderWalkables() || isRenderStaticObstacles() ||
		 isRenderMaterialVolumes() ||  isRenderExclusionVolumes() )
	{
		return true;
	}
	
	return false;
}

U32 LLFloaterPathfindingConsole::getRenderShapeFlags()
{
	resetShapeRenderFlags();

	if ( isRenderWalkables() )			
	{ 
		setShapeRenderFlag( LLPathingLib::LLST_WalkableObjects ); 
	}
	if ( isRenderStaticObstacles() )	
	{ 
		setShapeRenderFlag( LLPathingLib::LLST_ObstacleObjects ); 
	}
	if ( isRenderMaterialVolumes() )	
	{ 
		setShapeRenderFlag( LLPathingLib::LLST_MaterialPhantoms ); 
	}
	if ( isRenderExclusionVolumes() )	
	{ 
		setShapeRenderFlag( LLPathingLib::LLST_ExclusionPhantoms ); 
	}
	return mShapeRenderFlags;
}

void LLFloaterPathfindingConsole::fillInColorsForNavMeshVisualization()
{

	LLPathingLib::NavMeshColors colors;
	
	LLColor4 in = gSavedSettings.getColor4("PathfindingWalkable");
	colors.mWalkable= LLColor4U(in); 

	in = gSavedSettings.getColor4("PathfindingObstacle");
	colors.mObstacle= LLColor4U(in); 

	in = gSavedSettings.getColor4("PathfindingMaterial");
	colors.mMaterial= LLColor4U(in); 

	in = gSavedSettings.getColor4("PathfindingExclusion");
	colors.mExclusion= LLColor4U(in); 
	
	in = gSavedSettings.getColor4("PathfindingConnectedEdge");
	colors.mConnectedEdge= LLColor4U(in); 

	in = gSavedSettings.getColor4("PathfindingBoundaryEdge");
	colors.mBoundaryEdge= LLColor4U(in); 

	in = gSavedSettings.getColor4("PathfindingHeatColorBase");
	colors.mHeatColorBase= LLVector4(in.mV);

	in = gSavedSettings.getColor4("PathfindingHeatColorMax");
	colors.mHeatColorMax= LLVector4( in.mV ); 
	
	in = gSavedSettings.getColor4("PathfindingFaceColor");
	colors.mFaceColor= LLColor4U(in); 	

	in = gSavedSettings.getColor4("PathfindingStarValidColor");
	colors.mStarValid= LLColor4U(in); 	

	in = gSavedSettings.getColor4("PathfindingStarInvalidColor");
	colors.mStarInvalid= LLColor4U(in);

	in = gSavedSettings.getColor4("PathfindingTestPathColor");
	colors.mTestPath= LLColor4U(in); 	

	in = gSavedSettings.getColor4("PathfindingNavMeshClear");
	colors.mNavMeshClear= LLColor4(in); 

	mNavMeshColors = colors;

	LLPathingLib::getInstance()->setNavMeshColors( colors );
}
