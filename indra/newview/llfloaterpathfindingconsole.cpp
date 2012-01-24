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

#include "llpathinglib.h"

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
// LLFloaterPathfindingConsole
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingConsole::postBuild()
{
	childSetAction("view_and_edit_linksets", boost::bind(&LLFloaterPathfindingConsole::onViewEditLinksetClicked, this));
	childSetAction("rebuild_navmesh", boost::bind(&LLFloaterPathfindingConsole::onRebuildNavMeshClicked, this));
	childSetAction("refresh_navmesh", boost::bind(&LLFloaterPathfindingConsole::onRefreshNavMeshClicked, this));

	mShowNavMeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh_overlay");
	llassert(mShowNavMeshCheckBox != NULL);
	mShowNavMeshCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowNavMeshToggle, this));

	mShowExcludeVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_exclusion_volumes");
	llassert(mShowExcludeVolumesCheckBox != NULL);
	mShowExcludeVolumesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowExcludeVolumesToggle, this));

	mShowPathCheckBox = findChild<LLCheckBoxCtrl>("show_path");
	llassert(mShowPathCheckBox != NULL);
	mShowPathCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowPathToggle, this));

	mShowWaterPlaneCheckBox = findChild<LLCheckBoxCtrl>("show_water_plane");
	llassert(mShowWaterPlaneCheckBox != NULL);
	mShowWaterPlaneCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWaterPlaneToggle, this));

	mRegionOverlayDisplayRadioGroup = findChild<LLRadioGroup>("region_overlay_display");
	llassert(mRegionOverlayDisplayRadioGroup != NULL);
	mRegionOverlayDisplayRadioGroup->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onRegionOverlayDisplaySwitch, this));

	mPathSelectionRadioGroup = findChild<LLRadioGroup>("path_selection");
	llassert(mPathSelectionRadioGroup  != NULL);
	mPathSelectionRadioGroup ->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onPathSelectionSwitch, this));

	mCharacterWidthSlider = findChild<LLSliderCtrl>("character_width");
	llassert(mCharacterWidthSlider != NULL);
	mCharacterWidthSlider->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterWidthSet, this));

	mCharacterTypeRadioGroup = findChild<LLRadioGroup>("character_type");
	llassert(mCharacterTypeRadioGroup  != NULL);
	mCharacterTypeRadioGroup->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterTypeSwitch, this));

	mPathfindingStatus = findChild<LLTextBase>("pathfinding_status");
	llassert(mPathfindingStatus != NULL);

	mTerrainMaterialA = findChild<LLLineEditor>("terrain_material_a");
	llassert(mTerrainMaterialA != NULL);
	mTerrainMaterialA->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onTerrainMaterialASet, this));
	mTerrainMaterialA->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialB = findChild<LLLineEditor>("terrain_material_b");
	llassert(mTerrainMaterialB != NULL);
	mTerrainMaterialB->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onTerrainMaterialBSet, this));
	mTerrainMaterialB->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialC = findChild<LLLineEditor>("terrain_material_c");
	llassert(mTerrainMaterialC != NULL);
	mTerrainMaterialC->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onTerrainMaterialCSet, this));
	mTerrainMaterialC->setPrevalidate(LLTextValidate::validateFloat);

	mTerrainMaterialD = findChild<LLLineEditor>("terrain_material_d");
	llassert(mTerrainMaterialD != NULL);
	mTerrainMaterialD->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onTerrainMaterialDSet, this));
	mTerrainMaterialD->setPrevalidate(LLTextValidate::validateFloat);

	return LLFloater::postBuild();
}

LLFloaterPathfindingConsole::ERegionOverlayDisplay LLFloaterPathfindingConsole::getRegionOverlayDisplay() const
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

void LLFloaterPathfindingConsole::setRegionOverlayDisplay(ERegionOverlayDisplay pRegionOverlayDisplay)
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

LLFloaterPathfindingConsole::EPathSelectionState LLFloaterPathfindingConsole::getPathSelectionState() const
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

void LLFloaterPathfindingConsole::setPathSelectionState(EPathSelectionState pPathSelectionState)
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

void LLFloaterPathfindingConsole::setCharacterType(ECharacterType pCharacterType)
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

F32 LLFloaterPathfindingConsole::getTerrainMaterialA() const
{
	return mTerrainMaterialA->getValue().asReal();
}

void LLFloaterPathfindingConsole::setTerrainMaterialA(F32 pTerrainMaterial)
{
	mTerrainMaterialA->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingConsole::getTerrainMaterialB() const
{
	return mTerrainMaterialB->getValue().asReal();
}

void LLFloaterPathfindingConsole::setTerrainMaterialB(F32 pTerrainMaterial)
{
	mTerrainMaterialB->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingConsole::getTerrainMaterialC() const
{
	return mTerrainMaterialC->getValue().asReal();
}

void LLFloaterPathfindingConsole::setTerrainMaterialC(F32 pTerrainMaterial)
{
	mTerrainMaterialC->setValue(LLSD(pTerrainMaterial));
}

F32 LLFloaterPathfindingConsole::getTerrainMaterialD() const
{
	return mTerrainMaterialD->getValue().asReal();
}

void LLFloaterPathfindingConsole::setTerrainMaterialD(F32 pTerrainMaterial)
{
	mTerrainMaterialD->setValue(LLSD(pTerrainMaterial));
}

void LLFloaterPathfindingConsole::setHasNavMeshReceived()
{
	std::string str = getString("navmesh_fetch_complete_available");
	mPathfindingStatus->setText((LLStringExplicit)str);
}

void LLFloaterPathfindingConsole::setHasNoNavMesh()
{
	std::string str = getString("navmesh_fetch_complete_none");
	mPathfindingStatus->setText((LLStringExplicit)str);
}

LLFloaterPathfindingConsole::LLFloaterPathfindingConsole(const LLSD& pSeed)
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
	mTerrainMaterialD(NULL)
{
	for (int i=0;i<MAX_OBSERVERS;++i)
	{
		mNavMeshDownloadObserver[i].setPathfindingConsole(this);
	}
}

LLFloaterPathfindingConsole::~LLFloaterPathfindingConsole()
{
}

void LLFloaterPathfindingConsole::onOpen(const LLSD& pKey)
{
	//make sure we have a pathing system
	if ( !LLPathingLib::getInstance() )
	{
		LLPathingLib::initSystem();
	}
	//prep# test remove
	//LLSD content;
	//LLPathingLib::getInstance()->extractNavMeshSrcFromLLSD( content );
	//return true;
	//prep# end test
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

		int regionCnt = regions.size();
		for ( int i=0; i<regionCnt; ++i )
		{
			std::string url = regions[i]->getCapability( capability );

			if ( !url.empty() )
			{
				llinfos<<"Region has required caps of type ["<<capability<<"]"<<llendl;
				LLNavMeshStation::getInstance()->setNavMeshDownloadURL( url );
				int dir = shift[i];
				LLNavMeshStation::getInstance()->downloadNavMeshSrc( mNavMeshDownloadObserver[mCurrentMDO].getObserverHandle(), dir );				
				++mCurrentMDO;
			}				
			else
			{
				std::string str = getString("navmesh_region_not_enabled");
				LLStyle::Params styleParams;
				styleParams.color = LLUIColorTable::instance().getColor("DrYellow");
				mPathfindingStatus->setText((LLStringExplicit)str, styleParams);
				llinfos<<"Region has does not required caps of type ["<<capability<<"]"<<llendl;
			}
		}
	}		
}

void LLFloaterPathfindingConsole::onShowNavMeshToggle()
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

void LLFloaterPathfindingConsole::onShowExcludeVolumesToggle()
{
	BOOL checkBoxValue = mShowExcludeVolumesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderNavMeshandShapes(checkBoxValue);
	}
	else
	{
		mShowExcludeVolumesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onShowPathToggle()
{
	BOOL checkBoxValue = mShowPathCheckBox->get();

	llwarns << "functionality has not yet been implemented to toggle '"
		<< mShowPathCheckBox->getLabel() << "' to "
		<< (checkBoxValue ? "ON" : "OFF") << llendl;
}

void LLFloaterPathfindingConsole::onShowWaterPlaneToggle()
{
	BOOL checkBoxValue = mShowWaterPlaneCheckBox->get();

	llwarns << "functionality has not yet been implemented to toggle '"
		<< mShowWaterPlaneCheckBox->getLabel() << "' to "
		<< (checkBoxValue ? "ON" : "OFF") << llendl;
}

void LLFloaterPathfindingConsole::onRegionOverlayDisplaySwitch()
{
	switch (getRegionOverlayDisplay())
	{
	case kRenderOverlayOnFixedPhysicsGeometry :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mRegionOverlayDisplayRadioGroup->getName() << "' to RenderOverlayOnFixedPhysicsGeometry"
			<< llendl;
		break;
	case kRenderOverlayOnAllRenderableGeometry :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mRegionOverlayDisplayRadioGroup->getName() << "' to RenderOverlayOnAllRenderableGeometry"
			<< llendl;
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::onPathSelectionSwitch()
{
	switch (getPathSelectionState())
	{
	case kPathSelectNone :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mPathSelectionRadioGroup->getName() << "' to PathSelectNone"
			<< llendl;
		break;
	case kPathSelectStartPoint :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mPathSelectionRadioGroup->getName() << "' to PathSelectStartPoint"
			<< llendl;
		break;
	case kPathSelectEndPoint :
		llwarns << "functionality has not yet been implemented to toggle '"
			<< mPathSelectionRadioGroup->getName() << "' to PathSelectEndPoint"
			<< llendl;
		break;
	default :
		llassert(0);
		break;
	}
}

void LLFloaterPathfindingConsole::onCharacterWidthSet()
{
	F32 characterWidth = getCharacterWidth();
	llwarns << "functionality has not yet been implemented to set '" << mCharacterWidthSlider->getName()
		<< "' to the value (" << characterWidth << ")" << llendl;
}

void LLFloaterPathfindingConsole::onCharacterTypeSwitch()
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

void LLFloaterPathfindingConsole::onViewEditLinksetClicked()
{
	LLFloaterPathfindingLinksets::openLinksetsEditor();
}

void LLFloaterPathfindingConsole::onRebuildNavMeshClicked()
{
	llwarns << "functionality has not yet been implemented to handle rebuilding of the navmesh" << llendl;
}

void LLFloaterPathfindingConsole::onRefreshNavMeshClicked()
{
	llwarns << "functionality has not yet been implemented to handle refreshing of the navmesh" << llendl;
}

void LLFloaterPathfindingConsole::onTerrainMaterialASet()
{
	F32 terrainMaterial = getTerrainMaterialA();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialA->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingConsole::onTerrainMaterialBSet()
{
	F32 terrainMaterial = getTerrainMaterialB();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialB->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingConsole::onTerrainMaterialCSet()
{
	F32 terrainMaterial = getTerrainMaterialC();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialC->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}

void LLFloaterPathfindingConsole::onTerrainMaterialDSet()
{
	F32 terrainMaterial = getTerrainMaterialD();
	llwarns << "functionality has not yet been implemented to setting '" << mTerrainMaterialD->getName()
		<< "' to value (" << terrainMaterial << ")" << llendl;
}


BOOL LLFloaterPathfindingConsole::allowAllRenderables() const
{
	return getRegionOverlayDisplay() == kRenderOverlayOnAllRenderableGeometry ? true : false;
}

void LLFloaterPathfindingConsole::providePathingData( const LLVector3& point1, const LLVector3& point2 )
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
		break;

	case kPathSelectEndPoint :
		mPathData.mStartPointB		= point1;
		mPathData.mEndPointB		= point2;		
		mPathData.mCharacterWidth	= getCharacterWidth();
		//prep#TODO# possibly consider an alternate behavior - perhaps add a "path" button to submit the data.
		LLPathingLib::getInstance()->generatePath( mPathData );
		break;

	default :
		llassert(0);
		break;
	}	
}
