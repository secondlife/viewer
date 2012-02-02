/** 
* @file llfloaterpathfindingsetup.cpp
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
#include "llfloaterpathfindingsetup.h"
#include "llfloaterpathfindinglinksets.h"

#include "llsd.h"
#include "llagent.h"
#include "llbutton.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "lllineeditor.h"
#include "lltextbase.h"
#include "lltextvalidate.h"
#include "llnavmeshstation.h"
#include "llviewerregion.h"

#include "LLPathingLib.h"

#define XUI_RENDER_OVERLAY_ON_FIXED_PHYSICS_GEOMETRY 1
#define XUI_RENDER_OVERLAY_ON_ALL_RENDERABLE_GEOMETRY 2

#define XUI_PATH_SELECT_NONE 0
#define XUI_PATH_SELECT_START_POINT 1
#define XUI_PATH_SELECT_END_POINT 2

#define XUI_CHARACTER_TYPE_A 1
#define XUI_CHARACTER_TYPE_B 2
#define XUI_CHARACTER_TYPE_C 3
#define XUI_CHARACTER_TYPE_D 4

const int CURRENT_REGION = 99;
const int MAX_OBSERVERS = 10;
//---------------------------------------------------------------------------
// LLFloaterPathfindingSetup
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingSetup::postBuild()
{
	childSetAction("view_and_edit_linksets", boost::bind(&LLFloaterPathfindingSetup::onViewEditLinksetClicked, this));
	childSetAction("rebuild_navmesh", boost::bind(&LLFloaterPathfindingSetup::onRebuildNavMeshClicked, this));
	childSetAction("refresh_navmesh", boost::bind(&LLFloaterPathfindingSetup::onRefreshNavMeshClicked, this));

	mShowNavMeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh_overlay");
	llassert(mShowNavMeshCheckBox != NULL);
	mShowNavMeshCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onShowNavMeshToggle, this));

	mShowExcludeVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_exclusion_volumes");
	llassert(mShowExcludeVolumesCheckBox != NULL);
	mShowExcludeVolumesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onShowExcludeVolumesToggle, this));

	mShowPathCheckBox = findChild<LLCheckBoxCtrl>("show_path");
	llassert(mShowPathCheckBox != NULL);
	mShowPathCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onShowPathToggle, this));

	mShowWaterPlaneCheckBox = findChild<LLCheckBoxCtrl>("show_water_plane");
	llassert(mShowWaterPlaneCheckBox != NULL);
	mShowWaterPlaneCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onShowWaterPlaneToggle, this));

	mRegionOverlayDisplayRadioGroup = findChild<LLRadioGroup>("region_overlay_display");
	llassert(mRegionOverlayDisplayRadioGroup != NULL);
	mRegionOverlayDisplayRadioGroup->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onRegionOverlayDisplaySwitch, this));

	mPathSelectionRadioGroup = findChild<LLRadioGroup>("path_selection");
	llassert(mPathSelectionRadioGroup  != NULL);
	mPathSelectionRadioGroup ->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onPathSelectionSwitch, this));

	mCharacterWidthSlider = findChild<LLSliderCtrl>("character_width");
	llassert(mCharacterWidthSlider != NULL);
	mCharacterWidthSlider->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onCharacterWidthSet, this));

	mCharacterTypeRadioGroup = findChild<LLRadioGroup>("character_type");
	llassert(mCharacterTypeRadioGroup  != NULL);
	mCharacterTypeRadioGroup->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onCharacterTypeSwitch, this));

	mPathfindingStatus = findChild<LLTextBase>("pathfinding_status");
	llassert(mPathfindingStatus != NULL);

	mTerrainMaterialA = findChild<LLLineEditor>("terrain_material_a");
	llassert(mTerrainMaterialA != NULL);
	mTerrainMaterialA->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onTerrainMaterialASet, this));
	mTerrainMaterialA->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialB = findChild<LLLineEditor>("terrain_material_b");
	llassert(mTerrainMaterialB != NULL);
	mTerrainMaterialB->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onTerrainMaterialBSet, this));
	mTerrainMaterialB->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialC = findChild<LLLineEditor>("terrain_material_c");
	llassert(mTerrainMaterialC != NULL);
	mTerrainMaterialC->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onTerrainMaterialCSet, this));
	mTerrainMaterialC->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialD = findChild<LLLineEditor>("terrain_material_d");
	llassert(mTerrainMaterialD != NULL);
	mTerrainMaterialD->setCommitCallback(boost::bind(&LLFloaterPathfindingSetup::onTerrainMaterialDSet, this));
	mTerrainMaterialD->setPrevalidate(LLTextValidate::validateFloat);

	return LLFloater::postBuild();
}

LLFloaterPathfindingSetup::ERegionOverlayDisplay LLFloaterPathfindingSetup::getRegionOverlayDisplay() const
{
	ERegionOverlayDisplay regionOverlayDisplay;
	switch (mRegionOverlayDisplayRadioGroup->getValue().asInteger())
	{
	case XUI_RENDER_OVERLAY_ON_FIXED_PHYSICS_GEOMETRY :
		regionOverlayDisplay = kRenderOverlayOnFixedPhysicsGeometry;
		break;
	case XUI_RENDER_OVERLAY_ON_ALL_RENDERABLE_GEOMETRY :
		regionOverlayDisplay = kRenderOverlayOnAllRenderableGeometry;
		break;
	default :
		regionOverlayDisplay = kRenderOverlayOnFixedPhysicsGeometry;
		llassert(0);
		break;
	}

	return regionOverlayDisplay;
}

void LLFloaterPathfindingSetup::setRegionOverlayDisplay(ERegionOverlayDisplay pRegionOverlayDisplay)
{
	LLSD radioGroupValue;

	switch (pRegionOverlayDisplay)
	{
	case kRenderOverlayOnFixedPhysicsGeometry :
		radioGroupValue = XUI_RENDER_OVERLAY_ON_FIXED_PHYSICS_GEOMETRY;
		break;
	case kRenderOverlayOnAllRenderableGeometry :
		radioGroupValue = XUI_RENDER_OVERLAY_ON_ALL_RENDERABLE_GEOMETRY;
		break;
	default :
		radioGroupValue = XUI_RENDER_OVERLAY_ON_FIXED_PHYSICS_GEOMETRY;
		llassert(0);
		break;
	}

	mRegionOverlayDisplayRadioGroup->setValue(radioGroupValue);
}

LLFloaterPathfindingSetup::EPathSelectionState LLFloaterPathfindingSetup::getPathSelectionState() const
{
	EPathSelectionState pathSelectionState;

	switch (mPathSelectionRadioGroup->getValue().asInteger())
	{
	case XUI_PATH_SELECT_START_POINT :
		pathSelectionState = kPathSelectStartPoint;
		break;
	case XUI_PATH_SELECT_END_POINT :
		pathSelectionState = kPathSelectEndPoint;
		break;
	default :
		pathSelectionState = kPathSelectNone;
		break;
	}

	return pathSelectionState;
}

void LLFloaterPathfindingSetup::setPathSelectionState(EPathSelectionState pPathSelectionState)
{
	LLSD radioGroupValue;

	switch (pPathSelectionState)
	{
	case kPathSelectStartPoint :
		radioGroupValue = XUI_PATH_SELECT_START_POINT;
		break;
	case kPathSelectEndPoint :
		radioGroupValue = XUI_PATH_SELECT_END_POINT;
		break;
	default :
		radioGroupValue = XUI_PATH_SELECT_NONE;
		break;
	}

	mPathSelectionRadioGroup->setValue(radioGroupValue);
}

F32 LLFloaterPathfindingSetup::getCharacterWidth() const
{
	return mCharacterWidthSlider->getValueF32();
}

void LLFloaterPathfindingSetup::setCharacterWidth(F32 pCharacterWidth)
{
	mCharacterWidthSlider->setValue(LLSD(pCharacterWidth));
}

LLFloaterPathfindingSetup::ECharacterType LLFloaterPathfindingSetup::getCharacterType() const
{
	ECharacterType characterType;

	switch (mCharacterTypeRadioGroup->getValue().asInteger())
	{
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
		characterType = kCharacterTypeA;
		llassert(0);
		break;
	}

	return characterType;
}

void LLFloaterPathfindingSetup::setCharacterType(ECharacterType pCharacterType)
{
	LLSD radioGroupValue;

	switch (pCharacterType)
	{
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
		radioGroupValue = XUI_CHARACTER_TYPE_A;
		llassert(0);
		break;
	}

	mCharacterTypeRadioGroup->setValue(radioGroupValue);
}

F32 LLFloaterPathfindingSetup::getTerrainMaterialA() const
{
	return mTerrainMaterialA->getValue().asReal();
}

void LLFloaterPathfindingSetup::setTerrainMaterialA(F32 pTerrainMaterial)
{
	mTerrainMaterialA->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingSetup::getTerrainMaterialB() const
{
	return mTerrainMaterialB->getValue().asReal();
}

void LLFloaterPathfindingSetup::setTerrainMaterialB(F32 pTerrainMaterial)
{
	mTerrainMaterialB->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingSetup::getTerrainMaterialC() const
{
	return mTerrainMaterialC->getValue().asReal();
}

void LLFloaterPathfindingSetup::setTerrainMaterialC(F32 pTerrainMaterial)
{
	mTerrainMaterialC->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingSetup::getTerrainMaterialD() const
{
	return mTerrainMaterialD->getValue().asReal();
}

void LLFloaterPathfindingSetup::setTerrainMaterialD(F32 pTerrainMaterial)
{
	mTerrainMaterialD->setValue(LLSD(pTerrainMaterial));
}

void LLFloaterPathfindingSetup::setHasNavMeshReceived()
{
	std::string str = getString("navmesh_fetch_complete_available");
	mPathfindingStatus->setText((LLStringExplicit)str);
	//check to see if all regions are done loading and they are then stitch the navmeshes together
	--mNavMeshCnt;
	if ( mNavMeshCnt == 0 )
	{
		//LLPathingLib::getInstance()->stitchNavMeshes();
	}
}

void LLFloaterPathfindingSetup::setHasNoNavMesh()
{
	std::string str = getString("navmesh_fetch_complete_none");
	mPathfindingStatus->setText((LLStringExplicit)str);
}

LLFloaterPathfindingSetup::LLFloaterPathfindingSetup(const LLSD& pSeed)
	: LLFloater(pSeed),
	mShowNavMeshCheckBox(NULL),
	mShowExcludeVolumesCheckBox(NULL),
	mShowPathCheckBox(NULL),
	mShowWaterPlaneCheckBox(NULL),
	mRegionOverlayDisplayRadioGroup(NULL),
	mPathSelectionRadioGroup(NULL),
	mCharacterWidthSlider(NULL),
	mCharacterTypeRadioGroup(NULL),
	mPathfindingStatus(NULL),
	mTerrainMaterialA(NULL),
	mTerrainMaterialB(NULL),
	mTerrainMaterialC(NULL),
	mTerrainMaterialD(NULL),
	mNavMeshCnt(0),
	mHasStartPoint(false),
	mHasEndPoint(false)
{
	for (int i=0;i<MAX_OBSERVERS;++i)
	{
		mNavMeshDownloadObserver[i].setPathfindingConsole(this);
	}
}

LLFloaterPathfindingSetup::~LLFloaterPathfindingSetup()
{
}

void LLFloaterPathfindingSetup::onOpen(const LLSD& pKey)
{
	//make sure we have a pathing system
	if ( !LLPathingLib::getInstance() )
	{
		LLPathingLib::initSystem();
	}	
	if ( LLPathingLib::getInstance() == NULL )
	{ 
		std::string str = getString("navmesh_library_not_implemented");
		LLStyle::Params styleParams;
		styleParams.color = LLUIColorTable::instance().getColor("DrYellow");
		mPathfindingStatus->setText((LLStringExplicit)str, styleParams);
		llwarns <<"Errror: cannout find pathing library implementation."<<llendl;
	}
	else
	{		
		mCurrentMDO = 0;
		mNavMeshCnt = 0;

		//make sure the region is essentially enabled for navmesh support
		std::string capability = "RetrieveNavMeshSrc";

		//prep# neighboring navmesh support proto
		LLViewerRegion* pCurrentRegion = gAgent.getRegion();
		std::vector<LLViewerRegion*> regions;
		regions.push_back( pCurrentRegion );
		//pCurrentRegion->getNeighboringRegions( regions );

		std::vector<int> shift;
		shift.push_back( CURRENT_REGION );
		//pCurrentRegion->getNeighboringRegionsStatus( shift );

		//If the navmesh shift ops and the total region counts do not match - use the current region, only.
		if ( shift.size() != regions.size() )
		{
			shift.clear();regions.clear();
			regions.push_back( pCurrentRegion );
			shift.push_back( CURRENT_REGION );				
		}
		int regionCnt = regions.size();
		mNavMeshCnt = regionCnt;
		for ( int i=0; i<regionCnt; ++i )
		{
			std::string url = regions[i]->getCapability( capability );

			if ( !url.empty() )
			{
				std::string str = getString("navmesh_fetch_inprogress");
				mPathfindingStatus->setText((LLStringExplicit)str);
				LLNavMeshStation::getInstance()->setNavMeshDownloadURL( url );
				int dir = shift[i];
				LLNavMeshStation::getInstance()->downloadNavMeshSrc( mNavMeshDownloadObserver[mCurrentMDO].getObserverHandle(), dir );				
				++mCurrentMDO;
			}				
			else
			{
				--mNavMeshCnt;
				std::string str = getString("navmesh_region_not_enabled");
				LLStyle::Params styleParams;
				styleParams.color = LLUIColorTable::instance().getColor("DrYellow");
				mPathfindingStatus->setText((LLStringExplicit)str, styleParams);
				llinfos<<"Region has does not required caps of type ["<<capability<<"]"<<llendl;
			}
		}
	}		
}

void LLFloaterPathfindingSetup::onShowNavMeshToggle()
{
	BOOL checkBoxValue = mShowNavMeshCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderNavMesh(checkBoxValue);
	}
	else
	{
		mShowNavMeshCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingSetup::onShowExcludeVolumesToggle()
{
	BOOL checkBoxValue = mShowExcludeVolumesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderShapes(checkBoxValue);
	}
	else
	{
		mShowExcludeVolumesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingSetup::onShowPathToggle()
{
	BOOL checkBoxValue = mShowPathCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderPath(checkBoxValue);
	}
	else
	{
		mShowPathCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingSetup::onShowWaterPlaneToggle()
{
	BOOL checkBoxValue = mShowWaterPlaneCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderWaterPlane(checkBoxValue);
	}
	else
	{
		mShowWaterPlaneCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}

	llwarns << "functionality has not yet been implemented to toggle '"
		<< mShowWaterPlaneCheckBox->getLabel() << "' to "
		<< (checkBoxValue ? "ON" : "OFF") << llendl;
}

void LLFloaterPathfindingSetup::onRegionOverlayDisplaySwitch()
{
	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		switch (getRegionOverlayDisplay())
		{
		case kRenderOverlayOnFixedPhysicsGeometry :
			llPathingLibInstance->setRenderOverlayMode(false);
			break;
		case kRenderOverlayOnAllRenderableGeometry :
			llPathingLibInstance->setRenderOverlayMode(true);
			break;
		default :
			llPathingLibInstance->setRenderOverlayMode(false);
			llassert(0);
			break;
		}
	}
	else
	{
		this->setRegionOverlayDisplay(kRenderOverlayOnFixedPhysicsGeometry);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingSetup::onPathSelectionSwitch()
{
	switch (getPathSelectionState())
	{
	case kPathSelectNone :
		break;
	case kPathSelectStartPoint :
		break;
	case kPathSelectEndPoint :
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingSetup::onCharacterWidthSet()
{
	generatePath();
}

void LLFloaterPathfindingSetup::onCharacterTypeSwitch()
{
	switch (getCharacterType())
	{
	case kCharacterTypeA :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mCharacterTypeRadioGroup->getName() << "' to CharacterTypeA"
			<< llendl;
		break;
	case kCharacterTypeB :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mCharacterTypeRadioGroup->getName() << "' to CharacterTypeB"
			<< llendl;
		break;
	case kCharacterTypeC :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mCharacterTypeRadioGroup->getName() << "' to CharacterTypeC"
			<< llendl;
		break;
	case kCharacterTypeD :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mCharacterTypeRadioGroup->getName() << "' to CharacterTypeD"
			<< llendl;
		break;
	default :
		llassert(0);
		break;
	}

}

void LLFloaterPathfindingSetup::onViewEditLinksetClicked()
{
	LLFloaterPathfindingLinksets::openLinksetsEditor();
}

void LLFloaterPathfindingSetup::onRebuildNavMeshClicked()
{
	llwarns << "functionality has not yet been implemented to handle rebuilding of the navmesh" << llendl;
}

void LLFloaterPathfindingSetup::onRefreshNavMeshClicked()
{
	llwarns << "functionality has not yet been implemented to handle refreshing of the navmesh" << llendl;
}

void LLFloaterPathfindingSetup::onTerrainMaterialASet()
{
	F32 terrainMaterial = getTerrainMaterialA();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialA->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingSetup::onTerrainMaterialBSet()
{
	F32 terrainMaterial = getTerrainMaterialB();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialB->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingSetup::onTerrainMaterialCSet()
{
	F32 terrainMaterial = getTerrainMaterialC();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialC->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingSetup::onTerrainMaterialDSet()
{
	F32 terrainMaterial = getTerrainMaterialD();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialD->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}


void LLFloaterPathfindingSetup::providePathingData( const LLVector3& point1, const LLVector3& point2 )
{
	switch (getPathSelectionState())
	{
	case kPathSelectNone :
		llwarns << "not yet been implemented to toggle '"
			<< mPathSelectionRadioGroup->getName() << "' to PathSelectNone"
			<< llendl;
		break;

	case kPathSelectStartPoint :
		mPathData.mStartPointA	= point1;
		mPathData.mEndPointA	= point2;
		mHasStartPoint = true;
		break;

	case kPathSelectEndPoint :
		mPathData.mStartPointB		= point1;
		mPathData.mEndPointB		= point2;		
		mHasEndPoint = true;
		break;

	default :
		llassert(0);
		break;
	}

	generatePath();
}

void LLFloaterPathfindingSetup::generatePath()
{
	if (mHasStartPoint && mHasEndPoint)
	{
		mPathData.mCharacterWidth = getCharacterWidth();
		LLPathingLib::getInstance()->generatePath(mPathData);
	}
}
