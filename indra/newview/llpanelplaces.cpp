/**
 * @file llpanelplaces.cpp
 * @brief Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 *
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 *
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 *
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterreg.h"
#include "llsearcheditor.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "lllandmark.h"

#include "llagent.h"
#include "lllandmarklist.h"
#include "llfloaterworldmap.h"
#include "llpanelplaces.h"
#include "llpanellandmarks.h"
#include "llpanelteleporthistory.h"
#include "llviewerregion.h"

static LLRegisterPanelClassWrapper<LLPanelPlaces> t_places("panel_places");

LLPanelPlaces::LLPanelPlaces()
	:	LLPanel(),
		mFilterSubString(LLStringUtil::null),
		mActivePanel(NULL),
		mSearchEditor(NULL),
		mPlaceInfo(NULL)
{
	gInventory.addObserver(this);

	//LLUICtrlFactory::getInstance()->buildPanel(this, "panel_places.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLPanelPlaces::~LLPanelPlaces()
{
	if (gInventory.containsObserver(this))
		gInventory.removeObserver(this);
}

BOOL LLPanelPlaces::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("Places Tabs");
	if (mTabContainer)
	{
		mTabContainer->setCommitCallback(boost::bind(&LLPanelPlaces::onTabSelected, this));
	}

	mSearchEditor = getChild<LLSearchEditor>("Filter");
	if (mSearchEditor)
	{
		mSearchEditor->setSearchCallback(boost::bind(&LLPanelPlaces::onSearchEdit, this, _1));
	}

	mPlaceInfo = getChild<LLPanelPlaceInfo>("panel_place_info", TRUE, FALSE);
	if (mPlaceInfo)
	{
		LLButton* back_btn = mPlaceInfo->getChild<LLButton>("back_btn");
		if (back_btn)
		{
			back_btn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));
		}

		// *TODO: Assign the action to an appropriate event.
		childSetAction("overflow_btn", boost::bind(&LLPanelPlaceInfo::toggleMediaPanel, mPlaceInfo), this);
	}

	//childSetAction("share_btn", boost::bind(&LLPanelPlaces::onShareButtonClicked, this), this);
	childSetAction("teleport_btn", boost::bind(&LLPanelPlaces::onTeleportButtonClicked, this), this);
	childSetAction("map_btn", boost::bind(&LLPanelPlaces::onShowOnMapButtonClicked, this), this);

	return TRUE;
}

void LLPanelPlaces::draw()
{
	LLPanel::draw();
}

void LLPanelPlaces::onOpen(const LLSD& key)
{
	if(key.size() == 0)
		return;

	togglePlaceInfoPanel(TRUE);

	mPlaceInfoType = key["type"].asString();

	if (mPlaceInfoType == "agent")
	{
		// We don't need to teleport to the current location so disable the button
		getChild<LLButton>("teleport_btn")->setEnabled(FALSE);
		getChild<LLButton>("map_btn")->setEnabled(TRUE);

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayParcelInfo(gAgent.getPositionAgent(),
									  gAgent.getRegion()->getRegionID(),
									  gAgent.getPositionGlobal());
	}
	else if (mPlaceInfoType == "landmark")
	{
		LLInventoryItem* item = gInventory.getItem(key["id"].asUUID());
		if (!item)
			return;

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::LANDMARK);
		mPlaceInfo->displayItemInfo(item);

		LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
		if (!landmark)
			return;

		LLUUID region_id;
		landmark->getRegionID(region_id);
		LLVector3d pos_global;
		landmark->getGlobalPos(pos_global);
		mPlaceInfo->displayParcelInfo(landmark->getRegionPos(),
									  region_id,
									  pos_global);

	}
	else if (mPlaceInfoType == "teleport_history")
	{
		S32 index = key["id"].asInteger();

		const LLTeleportHistory::slurl_list_t& hist_items =
			LLTeleportHistory::getInstance()->getItems();

		LLVector3d pos_global = hist_items[index].mGlobalPos;

		F32 region_x = (F32)fmod( pos_global.mdV[VX], (F64)REGION_WIDTH_METERS );
		F32 region_y = (F32)fmod( pos_global.mdV[VY], (F64)REGION_WIDTH_METERS );

		LLVector3 pos_local(region_x, region_y, (F32)pos_global.mdV[VZ]);

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayParcelInfo(pos_local,
									  hist_items[index].mRegionID,
									  pos_global);
	}
}

void LLPanelPlaces::onSearchEdit(const std::string& search_string)
{
	if (mFilterSubString != search_string)
	{
		mFilterSubString = search_string;

		LLStringUtil::toUpper(mFilterSubString);
		LLStringUtil::trimHead(mFilterSubString);

		mSearchEditor->setText(mFilterSubString);

		mActivePanel->onSearchEdit(mFilterSubString);
	}
}

void LLPanelPlaces::onTabSelected()
{
	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());
	if (mActivePanel)
	{
		mActivePanel->onSearchEdit(mFilterSubString);
		mActivePanel->updateVerbs();
	}
}

void LLPanelPlaces::onShareButtonClicked()
{
	// TODO: Launch the "Things" Share wizard
}

/*
void LLPanelPlaces::onAddLandmarkButtonClicked()
{
	LLFloaterReg::showInstance("add_landmark");
}

void LLPanelPlaces::onCopySLURLButtonClicked()
{
	mActivePanel->onCopySLURL();
}
*/

void LLPanelPlaces::onTeleportButtonClicked()
{
	mActivePanel->onTeleport();
}

void LLPanelPlaces::onShowOnMapButtonClicked()
{
	if (mPlaceInfoType == "agent")
	{
		LLVector3d global_pos = gAgent.getPositionGlobal();
		if (!global_pos.isExactlyZero())
			{
				LLFloaterWorldMap* instance = LLFloaterWorldMap::getInstance();
				if(instance)
				{
					instance->trackLocation(global_pos);
					LLFloaterReg::showInstance("world_map", "center");

				}
			}
	}
	else
	{
		mActivePanel->onShowOnMap();
	}
}

void LLPanelPlaces::onBackButtonClicked()
{
	togglePlaceInfoPanel(FALSE);
}

void LLPanelPlaces::togglePlaceInfoPanel(BOOL visible)
{
	if (!mPlaceInfo)
		return;

	mPlaceInfo->setVisible(visible);
	mSearchEditor->setVisible(!visible);
	mTabContainer->setVisible(!visible);

	if (visible)
	{
		mPlaceInfo->resetLocation();

		LLRect rect = getRect();
		LLRect new_rect = LLRect(rect.mLeft, rect.mTop, rect.mRight, mTabContainer->getRect().mBottom);
		mPlaceInfo->reshape(new_rect.getWidth(),new_rect.getHeight());	
	}
}

//virtual
void LLPanelPlaces::changed(U32 mask)
{
	if (!(gInventory.isInventoryUsable() && LLTeleportHistory::getInstance()))
		return;

	LLLandmarksPanel* landmarks_panel = new LLLandmarksPanel();
	if (landmarks_panel)
	{
		landmarks_panel->setPanelPlacesButtons(this);

		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(landmarks_panel).
			label("Landmarks").
			insert_at(LLTabContainer::END));
	}

	LLTeleportHistoryPanel* teleport_history_panel = new LLTeleportHistoryPanel();
	if (teleport_history_panel)
	{
		teleport_history_panel->setPanelPlacesButtons(this);

		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(teleport_history_panel).
			label("Teleport History").
			insert_at(LLTabContainer::END));
	}

	mTabContainer->selectFirstTab();

	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());

	// we don't need to monitor inventory changes anymore,
	// so remove the observer
	gInventory.removeObserver(this);
}
