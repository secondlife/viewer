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
#include "llcheckboxctrl.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "llnavmeshstation.h"
#include "llviewerregion.h"

#include "llpathinglib.h"

//---------------------------------------------------------------------------
// LLFloaterPathfindingConsole
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingConsole::postBuild()
{
	childSetAction("view_and_edit_linksets", boost::bind(&LLFloaterPathfindingConsole::onViewEditLinksetClicked, this));

	mShowNavmeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh_overlay");
	llassert(mShowNavmeshCheckBox != NULL);
	mShowNavmeshCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowNavmeshToggle, this));

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

	return LLFloater::postBuild();
}

LLFloaterPathfindingConsole::LLFloaterPathfindingConsole(const LLSD& pSeed)
	: LLFloater(pSeed),
	mShowNavmeshCheckBox(NULL),
	mShowExcludeVolumesCheckBox(NULL),
	mShowPathCheckBox(NULL),
	mShowWaterPlaneCheckBox(NULL),
	mRegionOverlayDisplayRadioGroup(NULL),
	mPathSelectionRadioGroup(NULL),
	mCharacterWidthSlider(NULL),
	mNavmeshDownloadObserver()
{
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
		llinfos<<"No implementation of pathing library."<<llendl;
	}
	else
	{		
		//make sure the region is essentially enabled for navmesh support
		std::string capability = "RetrieveNavMeshSrc";
		std::string url = gAgent.getRegion()->getCapability( capability );
		if ( !url.empty() )
		{
			llinfos<<"Region has required caps of type ["<<capability<<"]"<<llendl;
			LLNavMeshStation::getInstance()->setNavMeshDownloadURL( url );
			LLNavMeshStation::getInstance()->downloadNavMeshSrc( mNavmeshDownloadObserver.getObserverHandle() );				
		}				
		else
		{
			llinfos<<"Region has does not required caps of type ["<<capability<<"]"<<llendl;
		}
	}		
}

void LLFloaterPathfindingConsole::onShowNavmeshToggle()
{
	BOOL checkBoxValue = mShowNavmeshCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderNavMesh(checkBoxValue);
	}
	else
	{
		mShowNavmeshCheckBox->set(FALSE);
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
	llwarns << "functionality has not yet been implemented to set " << mCharacterWidthSlider->getName()
		<< " to the value (" << characterWidth << ")" << llendl;
}

void LLFloaterPathfindingConsole::onViewEditLinksetClicked()
{
	LLFloaterPathfindingLinksets::openLinksetsEditor();
}

LLFloaterPathfindingConsole::ERegionOverlayDisplay LLFloaterPathfindingConsole::getRegionOverlayDisplay() const
{
	ERegionOverlayDisplay regionOverlayDisplay;
	switch (mRegionOverlayDisplayRadioGroup->getValue().asInteger())
	{
	case 0 :
		regionOverlayDisplay = kRenderOverlayOnFixedPhysicsGeometry;
		break;
	case 1:
		regionOverlayDisplay = kRenderOverlayOnAllRenderableGeometry;
		break;
	default :
		regionOverlayDisplay = kRenderOverlayOnFixedPhysicsGeometry;
		break;
	}

	return regionOverlayDisplay;
}

LLFloaterPathfindingConsole::EPathSelectionState LLFloaterPathfindingConsole::getPathSelectionState() const
{
	EPathSelectionState pathSelectionState;

	switch (mPathSelectionRadioGroup->getValue().asInteger())
	{
	case 1 :
		pathSelectionState = kPathSelectStartPoint;
		break;
	case 2:
		pathSelectionState = kPathSelectEndPoint;
		break;
	default :
		pathSelectionState = kPathSelectNone;
		break;
	}

	return pathSelectionState;
}

F32 LLFloaterPathfindingConsole::getCharacterWidth() const
{
	return mCharacterWidthSlider->getValueF32();
}