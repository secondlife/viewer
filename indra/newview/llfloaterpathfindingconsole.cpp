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
#include "llhandle.h"
#include "llagent.h"
#include "llbutton.h"
#include "llradiogroup.h"
#include "llsliderctrl.h"
#include "lllineeditor.h"
#include "lltextbase.h"
#include "lltabcontainer.h"
#include "llnavmeshstation.h"
#include "llfloaterreg.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"

#include "LLPathingLib.h"

#define XUI_CHARACTER_TYPE_A 1
#define XUI_CHARACTER_TYPE_B 2
#define XUI_CHARACTER_TYPE_C 3
#define XUI_CHARACTER_TYPE_D 4

const int CURRENT_REGION = 99;
const int MAX_OBSERVERS = 10;

LLHandle<LLFloaterPathfindingConsole> LLFloaterPathfindingConsole::sInstanceHandle;

//---------------------------------------------------------------------------
// LLFloaterPathfindingConsole
//---------------------------------------------------------------------------

BOOL LLFloaterPathfindingConsole::postBuild()
{
	childSetAction("view_and_edit_linksets", boost::bind(&LLFloaterPathfindingConsole::onViewEditLinksetClicked, this));

	mShowNavMeshCheckBox = findChild<LLCheckBoxCtrl>("show_navmesh");
	llassert(mShowNavMeshCheckBox != NULL);
	mShowNavMeshCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowNavMeshToggle, this));

	mShowWalkablesCheckBox = findChild<LLCheckBoxCtrl>("show_walkables");
	llassert(mShowWalkablesCheckBox != NULL);
	mShowWalkablesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWalkablesToggle, this));

	mShowStaticObstaclesCheckBox = findChild<LLCheckBoxCtrl>("show_static_obstacles");
	llassert(mShowStaticObstaclesCheckBox != NULL);
	mShowStaticObstaclesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowStaticObstaclesToggle, this));

	mShowMaterialVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_material_volumes");
	llassert(mShowMaterialVolumesCheckBox != NULL);
	mShowMaterialVolumesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowMaterialVolumesToggle, this));

	mShowExclusionVolumesCheckBox = findChild<LLCheckBoxCtrl>("show_exclusion_volumes");
	llassert(mShowExclusionVolumesCheckBox != NULL);
	mShowExclusionVolumesCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowExclusionVolumesToggle, this));

	mShowWorldCheckBox = findChild<LLCheckBoxCtrl>("show_world");
	llassert(mShowWorldCheckBox != NULL);
	mShowWorldCheckBox->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onShowWorldToggle, this));

	mPathfindingStatus = findChild<LLTextBase>("pathfinding_status");
	llassert(mPathfindingStatus != NULL);

	mEditTestTabContainer = findChild<LLTabContainer>("edit_test_tab_container");
	llassert(mEditTestTabContainer != NULL);

	mCharacterWidthSlider = findChild<LLSliderCtrl>("character_width");
	llassert(mCharacterWidthSlider != NULL);
	mCharacterWidthSlider->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterWidthSet, this));

	mCharacterTypeRadioGroup = findChild<LLRadioGroup>("character_type");
	llassert(mCharacterTypeRadioGroup  != NULL);
	mCharacterTypeRadioGroup->setCommitCallback(boost::bind(&LLFloaterPathfindingConsole::onCharacterTypeSwitch, this));

	return LLFloater::postBuild();
}

BOOL LLFloaterPathfindingConsole::handleAnyMouseClick(S32 x, S32 y, MASK mask, EClickType clicktype, BOOL down)
{
	if (down && (clicktype == LLMouseHandler::CLICK_LEFT) &&
		(((mask & MASK_CONTROL) && !(mask & (~MASK_CONTROL))) ||
		((mask & MASK_SHIFT) && !(mask & (~MASK_SHIFT)))))
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

		return TRUE;
	}
	else
	{
		return LLFloater::handleAnyMouseClick(x, y, mask, clicktype, down);
	}
}

BOOL LLFloaterPathfindingConsole::isGeneratePathMode() const
{
	return (getVisible() && (mEditTestTabContainer->getCurrentPanelIndex() == 1));
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

void LLFloaterPathfindingConsole::setHasNavMeshReceived()
{
	std::string str = getString("navmesh_fetch_complete_available");
	mPathfindingStatus->setText((LLStringExplicit)str);
	//check to see if all regions are done loading and they are then stitch the navmeshes together
	--mNavMeshCnt;
	if ( mNavMeshCnt == 0 )
	{
		LLPathingLib::getInstance()->stitchNavMeshes();
	}
}

void LLFloaterPathfindingConsole::setHasNoNavMesh()
{
	std::string str = getString("navmesh_fetch_complete_none");
	mPathfindingStatus->setText((LLStringExplicit)str);
}

LLFloaterPathfindingConsole::LLFloaterPathfindingConsole(const LLSD& pSeed)
	: LLFloater(pSeed),
	mSelfHandle(),
	mShowNavMeshCheckBox(NULL),
	mShowWalkablesCheckBox(NULL),
	mShowStaticObstaclesCheckBox(NULL),
	mShowMaterialVolumesCheckBox(NULL),
	mShowExclusionVolumesCheckBox(NULL),
	mShowWorldCheckBox(NULL),
	mPathfindingStatus(NULL),
	mEditTestTabContainer(NULL),
	mCharacterWidthSlider(NULL),
	mCharacterTypeRadioGroup(NULL),
	mNavMeshCnt(0),
	mHasStartPoint(false),
	mHasEndPoint(false)
{
	mSelfHandle.bind(this);

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
		LLPathingLib::getInstance()->cleanupResidual();

		mCurrentMDO = 0;
		mNavMeshCnt = 0;

		//make sure the region is essentially enabled for navmesh support
		std::string capability = "RetrieveNavMeshSrc";

		//prep# neighboring navmesh support proto
		LLViewerRegion* pCurrentRegion = gAgent.getRegion();
		std::vector<LLViewerRegion*> regions;
		regions.push_back( pCurrentRegion );
		pCurrentRegion->getNeighboringRegions( regions );

		std::vector<int> shift;
		shift.push_back( CURRENT_REGION );
		pCurrentRegion->getNeighboringRegionsStatus( shift );

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
		LLPathingLib::getInstance()->setRenderPath(true);
	}		
}

void LLFloaterPathfindingConsole::onClose(bool app_quitting)
{
	//make sure we have a pathing system
	if ( !LLPathingLib::getInstance() )
	{
		LLPathingLib::getInstance()->setRenderPath(false);
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

void LLFloaterPathfindingConsole::onShowWalkablesToggle()
{
	BOOL checkBoxValue = mShowWalkablesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		//llPathingLibInstance->setRenderNavMesh(checkBoxValue);
		llwarns << "functionality has not yet been implemented to set walkables to "
			<< (checkBoxValue ? "TRUE" : "FALSE") << llendl;

	}
	else
	{
		mShowWalkablesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onShowStaticObstaclesToggle()
{
	BOOL checkBoxValue = mShowStaticObstaclesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		//llPathingLibInstance->setRenderNavMesh(checkBoxValue);
		llwarns << "functionality has not yet been implemented to set static obstacles to "
			<< (checkBoxValue ? "TRUE" : "FALSE") << llendl;
	}
	else
	{
		mShowStaticObstaclesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onShowMaterialVolumesToggle()
{
	BOOL checkBoxValue = mShowMaterialVolumesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		//llPathingLibInstance->setRenderNavMesh(checkBoxValue);
		llwarns << "functionality has not yet been implemented to set material volumes to "
			<< (checkBoxValue ? "TRUE" : "FALSE") << llendl;
	}
	else
	{
		mShowMaterialVolumesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onShowExclusionVolumesToggle()
{
	BOOL checkBoxValue = mShowExclusionVolumesCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderShapes(checkBoxValue);
	}
	else
	{
		mShowExclusionVolumesCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onShowWorldToggle()
{
	BOOL checkBoxValue = mShowWorldCheckBox->get();

	LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
	if (llPathingLibInstance != NULL)
	{
		llPathingLibInstance->setRenderOverlayMode(checkBoxValue);
	}
	else
	{
		mShowWorldCheckBox->set(FALSE);
		llwarns << "cannot find LLPathingLib instance" << llendl;
	}
}

void LLFloaterPathfindingConsole::onCharacterWidthSet()
{
	generatePath();
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

void LLFloaterPathfindingConsole::generatePath()
{
	if (mHasStartPoint && mHasEndPoint)
	{
		mPathData.mCharacterWidth = getCharacterWidth();
		LLPathingLib::getInstance()->generatePath(mPathData);
	}
}
