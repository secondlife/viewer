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

#include "llassettype.h"

#include "lllandmark.h"

#include "llfloaterreg.h"
#include "llnotifications.h"
#include "llfiltereditor.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "lllandmarklist.h"
#include "llfloaterworldmap.h"
#include "llpanelplaces.h"
#include "llpanellandmarks.h"
#include "llpanelteleporthistory.h"
#include "llsidetray.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

// Helper function to get local position from global
const LLVector3 get_pos_local_from_global(const LLVector3d &pos_global);

static LLRegisterPanelClassWrapper<LLPanelPlaces> t_places("panel_places");

LLPanelPlaces::LLPanelPlaces()
	:	LLPanel(),
		mFilterSubString(LLStringUtil::null),
		mActivePanel(NULL),
		mFilterEditor(NULL),
		mPlaceInfo(NULL),
		mItem(NULL),
		mPosGlobal()
{
	gInventory.addObserver(this);

	LLViewerParcelMgr::getInstance()->setAgentParcelChangedCallback(
			boost::bind(&LLPanelPlaces::onAgentParcelChange, this));

	//LLUICtrlFactory::getInstance()->buildPanel(this, "panel_places.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLPanelPlaces::~LLPanelPlaces()
{
	if (gInventory.containsObserver(this))
		gInventory.removeObserver(this);
}

BOOL LLPanelPlaces::postBuild()
{
	mTeleportBtn = getChild<LLButton>("teleport_btn");
	mTeleportBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onTeleportButtonClicked, this));
	
	mShowOnMapBtn = getChild<LLButton>("map_btn");
	mShowOnMapBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShowOnMapButtonClicked, this));
	
	//mShareBtn = getChild<LLButton>("share_btn");
	//mShareBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShareButtonClicked, this));

	mOverflowBtn = getChild<LLButton>("overflow_btn");

	mTabContainer = getChild<LLTabContainer>("Places Tabs");
	if (mTabContainer)
	{
		mTabContainer->setCommitCallback(boost::bind(&LLPanelPlaces::onTabSelected, this));
	}

	mFilterEditor = getChild<LLFilterEditor>("Filter");
	if (mFilterEditor)
	{
		mFilterEditor->setCommitCallback(boost::bind(&LLPanelPlaces::onFilterEdit, this, _2));
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
		mOverflowBtn->setClickedCallback(boost::bind(&LLPanelPlaces::toggleMediaPanel, this));
	}

	return TRUE;
}

void LLPanelPlaces::onOpen(const LLSD& key)
{
	if(key.size() == 0)
		return;

	mPlaceInfoType = key["type"].asString();
	mPosGlobal.setZero();
	togglePlaceInfoPanel(TRUE);
	updateVerbs();

	if (mPlaceInfoType == "agent")
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayAgentParcelInfo();
		
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else if (mPlaceInfoType == "create_landmark")
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::CREATE_LANDMARK);
		mPlaceInfo->displayAgentParcelInfo();
		
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else if (mPlaceInfoType == "landmark")
	{
		LLUUID item_uuid = key["id"].asUUID();
		LLInventoryItem* item = gInventory.getItem(item_uuid);
		if (!item)
			return;
		
		setItem(item);
	}
	else if (mPlaceInfoType == "remote_place")
	{
		mPosGlobal = LLVector3d(key["x"].asReal(),
								key["y"].asReal(),
								key["z"].asReal());

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayParcelInfo(get_pos_local_from_global(mPosGlobal),
									  LLUUID(),
									  mPosGlobal);
	}
	else if (mPlaceInfoType == "teleport_history")
	{
		S32 index = key["id"].asInteger();

		const LLTeleportHistory::slurl_list_t& hist_items =
			LLTeleportHistory::getInstance()->getItems();

		LLVector3d pos_global = hist_items[index].mGlobalPos;

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::TELEPORT_HISTORY);
		mPlaceInfo->displayParcelInfo(get_pos_local_from_global(pos_global),
									  hist_items[index].mRegionID,
									  pos_global);
	}
}

void LLPanelPlaces::setItem(LLInventoryItem* item)
{
	mItem = item;
	
	// If the item is a link get a linked item
	if (mItem->getType() == LLAssetType::AT_LINK)
	{
		mItem = gInventory.getItem(mItem->getAssetUUID());
		if (mItem.isNull())
			return;
	}

	mPlaceInfo->setInfoType(LLPanelPlaceInfo::LANDMARK);
	mPlaceInfo->displayItemInfo(mItem);

	LLLandmark* lm = gLandmarkList.getAsset(mItem->getAssetUUID(),
											boost::bind(&LLPanelPlaces::onLandmarkLoaded, this, _1));
	if (lm)
	{
		onLandmarkLoaded(lm);
	}
}

void LLPanelPlaces::onLandmarkLoaded(LLLandmark* landmark)
{
	LLUUID region_id;
	landmark->getRegionID(region_id);
	LLVector3d pos_global;
	landmark->getGlobalPos(pos_global);
	mPlaceInfo->displayParcelInfo(landmark->getRegionPos(),
								  region_id,
								  pos_global);
}

void LLPanelPlaces::onFilterEdit(const std::string& search_string)
{
	if (mFilterSubString != search_string)
	{
		mFilterSubString = search_string;

		LLStringUtil::toUpper(mFilterSubString);
		LLStringUtil::trimHead(mFilterSubString);

		mFilterEditor->setText(mFilterSubString);

		mActivePanel->onSearchEdit(mFilterSubString);
	}
}

void LLPanelPlaces::onTabSelected()
{
	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());
	if (!mActivePanel)
		return;

	onFilterEdit(mFilterSubString);	
	mActivePanel->updateVerbs();
}

/*
void LLPanelPlaces::onShareButtonClicked()
{
	// TODO: Launch the "Things" Share wizard
}

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
	if (mPlaceInfo->getVisible())
	{
		if (mPlaceInfoType == "landmark")
		{
			LLSD payload;
			payload["asset_id"] = mItem->getAssetUUID();
			LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
		}
		else if (mPlaceInfoType == "remote_place")
		{
			LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
			if (!mPosGlobal.isExactlyZero() && worldmap_instance)
			{
				gAgent.teleportViaLocation(mPosGlobal);
				worldmap_instance->trackLocation(mPosGlobal);
			}
		}
	}
	else
	{
		mActivePanel->onTeleport();
	}
}

void LLPanelPlaces::onShowOnMapButtonClicked()
{
	if (mPlaceInfo->getVisible())
	{
		LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
		if(!worldmap_instance)
			return;

		if (mPlaceInfoType == "agent" ||
			mPlaceInfoType == "create_landmark" ||
			mPlaceInfoType == "remote_place")
		{
			if (!mPosGlobal.isExactlyZero())
			{
				worldmap_instance->trackLocation(mPosGlobal);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
		else if (mPlaceInfoType == "landmark")
		{
			LLLandmark* landmark = gLandmarkList.getAsset(mItem->getAssetUUID());
			if (!landmark)
				return;

			LLVector3d landmark_global_pos;
			if (!landmark->getGlobalPos(landmark_global_pos))
				return;
			
			if (!landmark_global_pos.isExactlyZero())
			{
				worldmap_instance->trackLocation(landmark_global_pos);
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

	// Resetting mPlaceInfoType when Place Info panel is closed.
	mPlaceInfoType = LLStringUtil::null;
	
	updateVerbs();
}

void LLPanelPlaces::toggleMediaPanel()
{
	if (!mPlaceInfo)
			return;

	mPlaceInfo->toggleMediaPanel(!mPlaceInfo->isMediaPanelVisible());
}

void LLPanelPlaces::togglePlaceInfoPanel(BOOL visible)
{
	if (!mPlaceInfo)
		return;

	mPlaceInfo->setVisible(visible);
	mFilterEditor->setVisible(!visible);
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
			label(getString("landmarks_tab_title")).
			insert_at(LLTabContainer::END));
	}

	LLTeleportHistoryPanel* teleport_history_panel = new LLTeleportHistoryPanel();
	if (teleport_history_panel)
	{
		teleport_history_panel->setPanelPlacesButtons(this);

		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(teleport_history_panel).
			label(getString("teleport_history_tab_title")).
			insert_at(LLTabContainer::END));
	}

	mTabContainer->selectFirstTab();

	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());

	// we don't need to monitor inventory changes anymore,
	// so remove the observer
	gInventory.removeObserver(this);
}

void LLPanelPlaces::onAgentParcelChange()
{
	if (mPlaceInfo->getVisible() && (mPlaceInfoType == "agent" || mPlaceInfoType == "create_landmark"))
	{
		LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", mPlaceInfoType));
	}
	else
	{
		updateVerbs();
	}
}

void LLPanelPlaces::updateVerbs()
{
	bool is_place_info_visible = mPlaceInfo->getVisible();
	bool is_agent_place_info_visible = mPlaceInfoType == "agent";
	if (is_place_info_visible)
	{
		if (is_agent_place_info_visible || mPlaceInfoType == "create_landmark")
		{
			// We don't need to teleport to the current location so disable the button
			mTeleportBtn->setEnabled(FALSE);
		}
		else if (mPlaceInfoType == "landmark" || mPlaceInfoType == "remote_place")
		{
			mTeleportBtn->setEnabled(TRUE);
		}

		mShowOnMapBtn->setEnabled(TRUE);
	}
	else
	{
		mActivePanel->updateVerbs();
	}

	// Enable overflow button only when showing the information about agent's current location.
	mOverflowBtn->setEnabled(is_place_info_visible && is_agent_place_info_visible);
}

const LLVector3 get_pos_local_from_global(const LLVector3d &pos_global)
{
	F32 region_x = (F32)fmod( pos_global.mdV[VX], (F64)REGION_WIDTH_METERS );
	F32 region_y = (F32)fmod( pos_global.mdV[VY], (F64)REGION_WIDTH_METERS );

	LLVector3 pos_local(region_x, region_y, (F32)pos_global.mdV[VZ]);
	return pos_local;
}	
