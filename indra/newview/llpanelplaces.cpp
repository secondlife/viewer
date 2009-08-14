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
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "llfloaterworldmap.h"
#include "llpanelplaces.h"
#include "llpanellandmarks.h"
#include "llpanelteleporthistory.h"
#include "llsidetray.h"
#include "lltoggleablemenu.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

static const std::string AGENT_INFO_TYPE			= "agent";
static const std::string CREATE_LANDMARK_INFO_TYPE	= "create_landmark";
static const std::string LANDMARK_INFO_TYPE			= "landmark";
static const std::string REMOTE_PLACE_INFO_TYPE		= "remote_place";
static const std::string TELEPORT_HISTORY_INFO_TYPE	= "teleport_history";

// Helper functions
static bool cmp_folders(const folder_pair_t& left, const folder_pair_t& right);
static std::string getFullFolderName(const LLViewerInventoryCategory* cat);
static void collectLandmarkFolders(LLInventoryModel::cat_array_t& cats);
static const LLVector3 get_pos_local_from_global(const LLVector3d &pos_global);
static void onSLURLBuilt(std::string& slurl);

static LLRegisterPanelClassWrapper<LLPanelPlaces> t_places("panel_places");

LLPanelPlaces::LLPanelPlaces()
	:	LLPanel(),
		mFilterSubString(LLStringUtil::null),
		mActivePanel(NULL),
		mFilterEditor(NULL),
		mPlaceInfo(NULL),
		mItem(NULL),
		mPlaceMenu(NULL),
		mLandmarkMenu(NULL),		
		mLandmarkFoldersMenuHandle(),
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
	
	LLView::deleteViewByHandle(mLandmarkFoldersMenuHandle);
}

BOOL LLPanelPlaces::postBuild()
{
	mCreateLandmarkBtn = getChild<LLButton>("create_landmark_btn");
	mCreateLandmarkBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onCreateLandmarkButtonClicked, this, LLUUID()));
	
	mFolderMenuBtn = getChild<LLButton>("folder_menu_btn");
	mFolderMenuBtn->setClickedCallback(boost::bind(&LLPanelPlaces::showLandmarkFoldersMenu, this));

	mTeleportBtn = getChild<LLButton>("teleport_btn");
	mTeleportBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onTeleportButtonClicked, this));
	
	mShowOnMapBtn = getChild<LLButton>("map_btn");
	mShowOnMapBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShowOnMapButtonClicked, this));
	
	mShareBtn = getChild<LLButton>("share_btn");
	//mShareBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShareButtonClicked, this));

	mOverflowBtn = getChild<LLButton>("overflow_btn");

	// *TODO: Assign the action to an appropriate event.
	//mOverflowBtn->setClickedCallback(boost::bind(&LLPanelPlaces::toggleMediaPanel, this));
	mOverflowBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onOverflowButtonClicked, this));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Places.OverflowMenu.Action",  boost::bind(&LLPanelPlaces::onOverflowMenuItemClicked, this, _2));

	mPlaceMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_place.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mPlaceMenu)
	{
		llwarns << "Error loading Place menu" << llendl;
	}

	mLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mLandmarkMenu)
	{
		llwarns << "Error loading Landmark menu" << llendl;
	}

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

	mPlaceInfo = getChild<LLPanelPlaceInfo>("panel_place_info");
	if (!mPlaceInfo)
		return FALSE;
	
	LLButton* back_btn = mPlaceInfo->getChild<LLButton>("back_btn");
	if (back_btn)
	{
		back_btn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));
	}

	return TRUE;
}

void LLPanelPlaces::onOpen(const LLSD& key)
{
	mFilterEditor->clear();
	onFilterEdit("");
	
	if(mPlaceInfo == NULL || key.size() == 0)
		return;

	mPlaceInfoType = key["type"].asString();
	mPosGlobal.setZero();
	togglePlaceInfoPanel(TRUE);
	updateVerbs();

	if (mPlaceInfoType == AGENT_INFO_TYPE)
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayAgentParcelInfo();
		
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
	{
		mPlaceInfo->setInfoType(LLPanelPlaceInfo::CREATE_LANDMARK);
		mPlaceInfo->displayAgentParcelInfo();
		
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
	{
		LLUUID item_uuid = key["id"].asUUID();
		LLInventoryItem* item = gInventory.getItem(item_uuid);
		if (!item)
			return;
		
		setItem(item);
	}
	else if (mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
	{
		if (mPlaceInfo->isMediaPanelVisible())
		{
			toggleMediaPanel();
		}

		mPosGlobal = LLVector3d(key["x"].asReal(),
								key["y"].asReal(),
								key["z"].asReal());

		mPlaceInfo->setInfoType(LLPanelPlaceInfo::PLACE);
		mPlaceInfo->displayParcelInfo(get_pos_local_from_global(mPosGlobal),
									  LLUUID(),
									  mPosGlobal);
	}
	else if (mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
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
	if (!mPlaceInfo)
		return;

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
	if (!mPlaceInfo)
		return;

	LLUUID region_id;
	landmark->getRegionID(region_id);
	landmark->getGlobalPos(mPosGlobal);
	mPlaceInfo->displayParcelInfo(landmark->getRegionPos(),
								  region_id,
								  mPosGlobal);
}

void LLPanelPlaces::onFilterEdit(const std::string& search_string)
{
	if (mFilterSubString != search_string)
	{
		mFilterSubString = search_string;

		// Searches are case-insensitive
		LLStringUtil::toUpper(mFilterSubString);
		LLStringUtil::trimHead(mFilterSubString);

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
*/

void LLPanelPlaces::copySLURL()
{
	LLLandmarkActions::getSLURLfromPosGlobal(mPosGlobal, boost::bind(&onSLURLBuilt, _1));
}

void LLPanelPlaces::deleteLandmark()
{
	gInventory.removeItem(mItem->getUUID());
}

void LLPanelPlaces::onTeleportButtonClicked()
{
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->getVisible())
	{
		if (mPlaceInfoType == LANDMARK_INFO_TYPE)
		{
			LLSD payload;
			payload["asset_id"] = mItem->getAssetUUID();
			LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
		}
		else if (mPlaceInfoType == AGENT_INFO_TYPE || mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
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
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->getVisible())
	{
		LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
		if(!worldmap_instance)
			return;

		if (mPlaceInfoType == AGENT_INFO_TYPE ||
			mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE ||
			mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
		{
			if (!mPosGlobal.isExactlyZero())
			{
				worldmap_instance->trackLocation(mPosGlobal);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
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

void LLPanelPlaces::onOverflowButtonClicked()
{
	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;
	bool is_landmark_info_visible = mPlaceInfoType == LANDMARK_INFO_TYPE;

	LLToggleableMenu* menu;

	if (is_agent_place_info_visible && mPlaceMenu != NULL)
	{
		menu = mPlaceMenu;
	}
	else if (is_landmark_info_visible && mLandmarkMenu != NULL)
	{
		menu = mLandmarkMenu;
	}
	else
	{
		return;
	}

	if (!menu->toggleVisibility())
		return;

	menu->updateParent(LLMenuGL::sMenuContainer);
	LLRect rect = mOverflowBtn->getRect();
	menu->setButtonRect(rect, this);
	LLMenuGL::showPopup(this, menu, rect.mRight, rect.mTop);
}

void LLPanelPlaces::onOverflowMenuItemClicked(const LLSD& param)
{
	std::string item = param.asString();
	if (item == "landmark")
	{
		onOpen(LLSD().insert("type", CREATE_LANDMARK_INFO_TYPE));
	}
	else if (item == "copy")
	{
		copySLURL();
	}
	else if (item == "delete")
	{
		deleteLandmark();
	}
}

void LLPanelPlaces::onCreateLandmarkButtonClicked(const LLUUID& folder_id)
{
	if (!mPlaceInfo)
		return;

	mPlaceInfo->createLandmark(folder_id);

	onBackButtonClicked();
	LLSideTray::getInstance()->collapseSideBar();
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

	// Refresh the current place info because
	// the media panel controls can't refer to
	// the remote parcel media.
	onOpen(LLSD().insert("type", AGENT_INFO_TYPE));
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
	if (!mPlaceInfo)
		return;

	if (mPlaceInfo->getVisible() && mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
	{
		onOpen(LLSD().insert("type", mPlaceInfoType));
	}
	else if (mPlaceInfo->isMediaPanelVisible())
	{
		onOpen(LLSD().insert("type", AGENT_INFO_TYPE));
	}
	else
	{
		updateVerbs();
	}
}

void LLPanelPlaces::updateVerbs()
{
	if (!mPlaceInfo)
		return;

	bool is_place_info_visible = mPlaceInfo->getVisible();
	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;
	bool is_create_landmark_visible = mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE;
	bool is_media_panel_visible = mPlaceInfo->isMediaPanelVisible();
	
	mTeleportBtn->setVisible(!is_create_landmark_visible);
	mShareBtn->setVisible(!is_create_landmark_visible);
	mCreateLandmarkBtn->setVisible(is_create_landmark_visible);
	mFolderMenuBtn->setVisible(is_create_landmark_visible);

	mOverflowBtn->setEnabled(is_place_info_visible && !is_media_panel_visible && !is_create_landmark_visible);

	if (is_place_info_visible)
	{
		if (is_agent_place_info_visible)
		{
			// We don't need to teleport to the current location
			// so check if the location is not within the current parcel.
			mTeleportBtn->setEnabled(!is_media_panel_visible &&
									 !mPosGlobal.isExactlyZero() &&
									 !LLViewerParcelMgr::getInstance()->inAgentParcel(mPosGlobal));
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE || mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
		{
			mTeleportBtn->setEnabled(TRUE);
		}

		mShowOnMapBtn->setEnabled(!is_media_panel_visible);
	}
	else
	{
		mActivePanel->updateVerbs();
	}
}

void LLPanelPlaces::showLandmarkFoldersMenu()
{
	if (mLandmarkFoldersMenuHandle.isDead())
	{
		LLToggleableMenu::Params menu_p;
		menu_p.name("landmarks_folders_menu");
		menu_p.can_tear_off(false);
		menu_p.visible(false);
		menu_p.scrollable(true);

		LLToggleableMenu* menu = LLUICtrlFactory::create<LLToggleableMenu>(menu_p);

		mLandmarkFoldersMenuHandle = menu->getHandle();
	}

	LLToggleableMenu* menu = (LLToggleableMenu*)mLandmarkFoldersMenuHandle.get();
	if(!menu)
		return;

	if (!menu->toggleVisibility())
		return;

	// Collect all folders that can contain landmarks.
	LLInventoryModel::cat_array_t cats;
	collectLandmarkFolders(cats);

	// Sort the folders by their full name.
	folder_vec_t folders;
	S32 count = cats.count();
	for (S32 i = 0; i < count; i++)
	{
		const LLViewerInventoryCategory* cat = cats.get(i);
		std::string cat_full_name = getFullFolderName(cat);
		folders.push_back(folder_pair_t(cat->getUUID(), cat_full_name));
	}
	sort(folders.begin(), folders.end(), cmp_folders);

	LLRect btn_rect = mFolderMenuBtn->getRect();

	LLRect root_rect = getRootView()->getRect();
	
	// Check it there are changed items or viewer dimensions 
	// have changed since last call
	if (mLandmarkFoldersCache.size() == count &&
		mRootViewWidth == root_rect.getWidth() &&
		mRootViewHeight == root_rect.getHeight())
	{
		S32 i;
		for (i = 0; i < count; i++)
		{
			if (mLandmarkFoldersCache[i].second != folders[i].second)
			{
				break;
			}
		}

		// Check passed, just show the menu
		if (i == count)
		{
			menu->buildDrawLabels();
			menu->updateParent(LLMenuGL::sMenuContainer);

			menu->setButtonRect(btn_rect, this);
			LLMenuGL::showPopup(this, menu, btn_rect.mRight, btn_rect.mTop);
			return;
		}
	}

	// If there are changes, store the new viewer dimensions
	// and a list of folders
	mRootViewWidth = root_rect.getWidth();
	mRootViewHeight = root_rect.getHeight();
	mLandmarkFoldersCache = folders;

	menu->empty();
	U32 max_width = 0;

	// Menu width must not exceed the root view limits,
	// so we assume the space between the left edge of
	// the root view and 
	LLRect screen_btn_rect;
	localRectToScreen(btn_rect, &screen_btn_rect);
	S32 free_space = screen_btn_rect.mRight;

	for(folder_vec_t::const_iterator it = mLandmarkFoldersCache.begin(); it != mLandmarkFoldersCache.end(); it++)
	{
		const std::string& item_name = it->second;

		LLMenuItemCallGL::Params item_params;
		item_params.name(item_name);
		item_params.label(item_name);

		item_params.on_click.function(boost::bind(&LLPanelPlaces::onCreateLandmarkButtonClicked, this, it->first));

		LLMenuItemCallGL *menu_item = LLUICtrlFactory::create<LLMenuItemCallGL>(item_params);

		// Check whether item name wider than menu
		if ((S32) menu_item->getNominalWidth() > free_space)
		{
			S32 chars_total = item_name.length();
			S32 chars_fitted = 1;
			menu_item->setLabel(LLStringExplicit(""));
			S32 label_space = free_space - menu_item->getFont()->getWidth("...") -
				menu_item->getNominalWidth(); // This returns width of menu item with empty label (pad pixels)

			while (chars_fitted < chars_total && menu_item->getFont()->getWidth(item_name, 0, chars_fitted) < label_space)
			{
				chars_fitted++;
			}
			chars_fitted--; // Rolling back one char, that doesn't fit

			menu_item->setLabel(item_name.substr(0, chars_fitted) + "...");
		}

		max_width = llmax(max_width, menu_item->getNominalWidth());

		menu->addChild(menu_item);
	}

	menu->buildDrawLabels();
	menu->updateParent(LLMenuGL::sMenuContainer);
	menu->setButtonRect(btn_rect, this);
	LLMenuGL::showPopup(this, menu, btn_rect.mRight, btn_rect.mTop);
}

static bool cmp_folders(const folder_pair_t& left, const folder_pair_t& right)
{
	return left.second < right.second;
}

static std::string getFullFolderName(const LLViewerInventoryCategory* cat)
{
	std::string name = cat->getName();
	LLUUID parent_id;

	// translate category name, if it's right below the root
	// FIXME: it can throw notification about non existent string in strings.xml
	if (cat->getParentUUID().notNull() && cat->getParentUUID() == gInventory.getRootFolderID())
	{
		name = LLTrans::getString("InvFolder " + name);
	}

	// we don't want "My Inventory" to appear in the name
	while ((parent_id = cat->getParentUUID()).notNull() && parent_id != gInventory.getRootFolderID())
	{
		cat = gInventory.getCategory(parent_id);
		name = cat->getName() + "/" + name;
	}

	return name;
}

static void collectLandmarkFolders(LLInventoryModel::cat_array_t& cats)
{
	// Add the "Landmarks" category itself.
	LLUUID landmarks_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LANDMARK);
	LLViewerInventoryCategory* landmarks_cat = gInventory.getCategory(landmarks_id);
	if (!landmarks_cat)
	{
		llwarns << "Cannot find the landmarks folder" << llendl;
	}
	else
	{
		cats.put(landmarks_cat);
	}

	// Add descendent folders of the "Landmarks" category.
	LLInventoryModel::item_array_t items; // unused
	LLIsType is_category(LLAssetType::AT_CATEGORY);
	gInventory.collectDescendentsIf(
		landmarks_id,
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_category);

	// Add the "My Favorites" category.
	LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_FAVORITE);
	LLViewerInventoryCategory* favorites_cat = gInventory.getCategory(favorites_id);
	if (!favorites_cat)
	{
		llwarns << "Cannot find the favorites folder" << llendl;
	}
	else
	{
		cats.put(favorites_cat);
	}
}

static const LLVector3 get_pos_local_from_global(const LLVector3d &pos_global)
{
	F32 region_x = (F32)fmod( pos_global.mdV[VX], (F64)REGION_WIDTH_METERS );
	F32 region_y = (F32)fmod( pos_global.mdV[VY], (F64)REGION_WIDTH_METERS );

	LLVector3 pos_local(region_x, region_y, (F32)pos_global.mdV[VZ]);
	return pos_local;
}

static void onSLURLBuilt(std::string& slurl)
{
	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
		
	LLSD args;
	args["SLURL"] = slurl;

	LLNotifications::instance().add("CopySLURL", args);
}
