/**
 * @file llpanelplaces.cpp
 * @brief Side Bar "Places" panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelplaces.h"

#include "llassettype.h"
#include "lltimer.h"

#include "llinventory.h"
#include "lllandmark.h"
#include "llparcel.h"

#include "llcombobox.h"
#include "llfiltereditor.h"
#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "lltabcontainer.h"
#include "lltexteditor.h"
#include "lltrans.h"
#include "lluictrlfactory.h"

#include "llwindow.h"

#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llavatarpropertiesprocessor.h"
#include "llcommandhandler.h"
#include "lldndbutton.h"
#include "llfloaterworldmap.h"
#include "llinventorybridge.h"
#include "llinventoryobserver.h"
#include "llinventorymodel.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "lllayoutstack.h"
#include "llpanellandmarkinfo.h"
#include "llpanellandmarks.h"
#include "llpanelplaceprofile.h"
#include "llpanelteleporthistory.h"
#include "llremoteparcelrequest.h"
#include "llteleporthistorystorage.h"
#include "lltoggleablemenu.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

// Constants
static const F32 PLACE_INFO_UPDATE_INTERVAL = 3.0;
static const std::string AGENT_INFO_TYPE			= "agent";
static const std::string CREATE_LANDMARK_INFO_TYPE	= "create_landmark";
static const std::string CREATE_PICK_TYPE			= "create_pick";
static const std::string LANDMARK_INFO_TYPE			= "landmark";
static const std::string REMOTE_PLACE_INFO_TYPE		= "remote_place";
static const std::string TELEPORT_HISTORY_INFO_TYPE	= "teleport_history";
static const std::string LANDMARK_TAB_INFO_TYPE     = "open_landmark_tab";

// Support for secondlife:///app/parcel/{UUID}/about SLapps
class LLParcelHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLParcelHandler() : LLCommandHandler("parcel", UNTRUSTED_THROTTLE) { }
	bool handle(const LLSD& params, const LLSD& query_map,
				LLMediaCtrl* web)
	{		
		if (params.size() < 2)
		{
			return false;
		}

		if (!LLUI::getInstance()->mSettingGroups["config"]->getBOOL("EnablePlaceProfile"))
		{
			LLNotificationsUtil::add("NoPlaceInfo", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
			return true;
		}

		LLUUID parcel_id;
		if (!parcel_id.set(params[0], FALSE))
		{
			return false;
		}
		if (params[1].asString() == "about")
		{
			if (parcel_id.notNull())
			{
				LLSD key;
				key["type"] = "remote_place";
				key["id"] = parcel_id;
				LLFloaterSidePanelContainer::showPanel("places", key);
				return true;
			}
		}
		return false;
	}
};
LLParcelHandler gParcelHandler;

// Helper functions
static bool is_agent_in_selected_parcel(LLParcel* parcel);
static void onSLURLBuilt(std::string& slurl);

//Observer classes
class LLPlacesParcelObserver : public LLParcelObserver
{
public:
	LLPlacesParcelObserver(LLPanelPlaces* places_panel) :
		LLParcelObserver(),
		mPlaces(places_panel)
	{}

	/*virtual*/ void changed()
	{
		if (mPlaces)
			mPlaces->changedParcelSelection();
	}

private:
	LLPanelPlaces*		mPlaces;
};

class LLPlacesInventoryObserver : public LLInventoryAddedObserver
{
public:
	LLPlacesInventoryObserver(LLPanelPlaces* places_panel) :
		mPlaces(places_panel)
	{}

	/*virtual*/ void changed(U32 mask)
	{
		LLInventoryAddedObserver::changed(mask);

		if (mPlaces && !mPlaces->tabsCreated())
		{
			mPlaces->createTabs();
		}
	}

protected:
	/*virtual*/ void done()
	{
		mPlaces->showAddedLandmarkInfo(gInventory.getAddedIDs());
	}

private:
	LLPanelPlaces*		mPlaces;
};

class LLPlacesRemoteParcelInfoObserver : public LLRemoteParcelInfoObserver
{
public:
	LLPlacesRemoteParcelInfoObserver(LLPanelPlaces* places_panel) :
		LLRemoteParcelInfoObserver(),
		mPlaces(places_panel)
	{}

	~LLPlacesRemoteParcelInfoObserver()
	{
		// remove any in-flight observers
		std::set<LLUUID>::iterator it;
		for (it = mParcelIDs.begin(); it != mParcelIDs.end(); ++it)
		{
			const LLUUID &id = *it;
			LLRemoteParcelInfoProcessor::getInstance()->removeObserver(id, this);
		}
		mParcelIDs.clear();
	}

	/*virtual*/ void processParcelInfo(const LLParcelData& parcel_data)
	{
		if (mPlaces)
		{
			mPlaces->changedGlobalPos(LLVector3d(parcel_data.global_x,
												 parcel_data.global_y,
												 parcel_data.global_z));
		}

		mParcelIDs.erase(parcel_data.parcel_id);
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(parcel_data.parcel_id, this);
	}
	/*virtual*/ void setParcelID(const LLUUID& parcel_id)
	{
		if (!parcel_id.isNull())
		{
			mParcelIDs.insert(parcel_id);
			LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
			LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
		}
	}
	/*virtual*/ void setErrorStatus(S32 status, const std::string& reason)
	{
		LL_ERRS() << "Can't complete remote parcel request. Http Status: "
			   << status << ". Reason : " << reason << LL_ENDL;
	}

private:
	std::set<LLUUID>	mParcelIDs;
	LLPanelPlaces*		mPlaces;
};


static LLPanelInjector<LLPanelPlaces> t_places("panel_places");

LLPanelPlaces::LLPanelPlaces()
	:	LLPanel(),
		mActivePanel(NULL),
		mFilterEditor(NULL),
		mPlaceProfile(NULL),
		mLandmarkInfo(NULL),
		mItem(NULL),
		mPlaceMenu(NULL),
		mLandmarkMenu(NULL),
		mPosGlobal(),
		isLandmarkEditModeOn(false),
		mTabsCreated(false)
{
	mParcelObserver = new LLPlacesParcelObserver(this);
	mInventoryObserver = new LLPlacesInventoryObserver(this);
	mRemoteParcelObserver = new LLPlacesRemoteParcelInfoObserver(this);

	gInventory.addObserver(mInventoryObserver);

	mAgentParcelChangedConnection = gAgent.addParcelChangedCallback(
			boost::bind(&LLPanelPlaces::updateVerbs, this));

	//buildFromFile( "panel_places.xml"); // Called from LLRegisterPanelClass::defaultPanelClassBuilder()
}

LLPanelPlaces::~LLPanelPlaces()
{
	if (gInventory.containsObserver(mInventoryObserver))
		gInventory.removeObserver(mInventoryObserver);

	LLViewerParcelMgr::getInstance()->removeObserver(mParcelObserver);

	delete mInventoryObserver;
	delete mParcelObserver;
	delete mRemoteParcelObserver;

	if (mAgentParcelChangedConnection.connected())
	{
		mAgentParcelChangedConnection.disconnect();
	}
}

BOOL LLPanelPlaces::postBuild()
{
	mTeleportBtn = getChild<LLButton>("teleport_btn");
	mTeleportBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onTeleportButtonClicked, this));
	
	mShowOnMapBtn = getChild<LLButton>("map_btn");
	mShowOnMapBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onShowOnMapButtonClicked, this));

	mSaveBtn = getChild<LLButton>("save_btn");
	mSaveBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onSaveButtonClicked, this));

	mCancelBtn = getChild<LLButton>("cancel_btn");
	mCancelBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onCancelButtonClicked, this));

	mCloseBtn = getChild<LLButton>("close_btn");
	mCloseBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));

	mOverflowBtn = getChild<LLMenuButton>("overflow_btn");
	mOverflowBtn->setMouseDownCallback(boost::bind(&LLPanelPlaces::onOverflowButtonClicked, this));

    mGearMenuButton = getChild<LLMenuButton>("options_gear_btn");
    mGearMenuButton->setMouseDownCallback(boost::bind(&LLPanelPlaces::onGearMenuClick, this));

    mSortingMenuButton = getChild<LLMenuButton>("sorting_menu_btn");
    mSortingMenuButton->setMouseDownCallback(boost::bind(&LLPanelPlaces::onSortingMenuClick, this));

    mAddMenuButton = getChild<LLMenuButton>("add_menu_btn");
    mAddMenuButton->setMouseDownCallback(boost::bind(&LLPanelPlaces::onAddMenuClick, this));

    mRemoveSelectedBtn = getChild<LLButton>("trash_btn");
    mRemoveSelectedBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onRemoveButtonClicked, this));

    LLDragAndDropButton* trash_btn = (LLDragAndDropButton*)mRemoveSelectedBtn;
    trash_btn->setDragAndDropHandler(boost::bind(&LLPanelPlaces::handleDragAndDropToTrash, this
        , _4 // BOOL drop
        , _5 // EDragAndDropType cargo_type
        , _6 // void* cargo_data
        , _7 // EAcceptance* accept
    ));

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Places.OverflowMenu.Action",  boost::bind(&LLPanelPlaces::onOverflowMenuItemClicked, this, _2));
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	enable_registrar.add("Places.OverflowMenu.Enable",  boost::bind(&LLPanelPlaces::onOverflowMenuItemEnable, this, _2));

	mPlaceMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_place.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (mPlaceMenu)
	{
		mPlaceMenu->setAlwaysShowMenu(TRUE);
	}
	else
	{
		LL_WARNS() << "Error loading Place menu" << LL_ENDL;
	}

	mLandmarkMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_landmark.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mLandmarkMenu)
	{
		LL_WARNS() << "Error loading Landmark menu" << LL_ENDL;
	}

	mTabContainer = getChild<LLTabContainer>("Places Tabs");
	if (mTabContainer)
	{
		mTabContainer->setCommitCallback(boost::bind(&LLPanelPlaces::onTabSelected, this));
	}

    mButtonsContainer = getChild<LLPanel>("button_layout_panel");
    mButtonsContainer->setVisible(FALSE);
    mFilterContainer = getChild<LLLayoutStack>("top_menu_panel");

	mFilterEditor = getChild<LLFilterEditor>("Filter");
	if (mFilterEditor)
	{
		//when list item is being clicked the filter editor looses focus
		//committing on focus lost leads to detaching list items
		//BUT a detached list item cannot be made selected and must not be clicked onto
		mFilterEditor->setCommitOnFocusLost(false);

		mFilterEditor->setCommitCallback(boost::bind(&LLPanelPlaces::onFilterEdit, this, _2, false));
	}

	mPlaceProfile = findChild<LLPanelPlaceProfile>("panel_place_profile");
	mLandmarkInfo = findChild<LLPanelLandmarkInfo>("panel_landmark_info");
	if (!mPlaceProfile || !mLandmarkInfo)
		return FALSE;

	mPlaceProfileBackBtn = mPlaceProfile->getChild<LLButton>("back_btn");
	mPlaceProfileBackBtn->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));

	mLandmarkInfo->getChild<LLButton>("back_btn")->setClickedCallback(boost::bind(&LLPanelPlaces::onBackButtonClicked, this));

	LLLineEditor* title_editor = mLandmarkInfo->getChild<LLLineEditor>("title_editor");
	title_editor->setKeystrokeCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this), NULL);

	LLTextEditor* notes_editor = mLandmarkInfo->getChild<LLTextEditor>("notes_editor");
	notes_editor->setKeystrokeCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));

	LLComboBox* folder_combo = mLandmarkInfo->getChild<LLComboBox>("folder_combo");
	folder_combo->setCommitCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));

	LLButton* edit_btn = mLandmarkInfo->getChild<LLButton>("edit_btn");
	edit_btn->setCommitCallback(boost::bind(&LLPanelPlaces::onEditButtonClicked, this));

	createTabs();
	updateVerbs();

	return TRUE;
}

void LLPanelPlaces::onOpen(const LLSD& key)
{
	if (!mPlaceProfile || !mLandmarkInfo)
		return;

	if (key.size() != 0)
	{
		isLandmarkEditModeOn = false;
		std::string key_type = key["type"].asString();
		if (key_type == LANDMARK_TAB_INFO_TYPE)
		{
			// Small hack: We need to toggle twice. The first toggle moves from the Landmark 
			// or Teleport History info panel to the Landmark or Teleport History list panel.
			// For this first toggle, the mPlaceInfoType should be the one previously used so 
			// that the state can be corretly set.
			// The second toggle forces the list to be set to Landmark.
			// This avoids extracting and duplicating all the state logic from togglePlaceInfoPanel() 
			// here or some specific private method
			togglePlaceInfoPanel(FALSE);
			mPlaceInfoType = key_type;
			togglePlaceInfoPanel(FALSE);
			// Update the active tab
			onTabSelected();
			// Update the buttons at the bottom of the panel
			updateVerbs();
		}
        else if (key_type == CREATE_PICK_TYPE)
        {
            LLUUID item_id = key["item_id"];

            LLLandmarksPanel* landmarks_panel =
                dynamic_cast<LLLandmarksPanel*>(mTabContainer->getPanelByName("Landmarks"));
            if (landmarks_panel && item_id.notNull())
            {
                LLLandmark* landmark = LLLandmarkActions::getLandmark(item_id, boost::bind(&LLLandmarksPanel::doCreatePick, landmarks_panel, _1, item_id));
                if (landmark)
                {
                    landmarks_panel->doCreatePick(landmark, item_id);
                }
            }
        }
		else // "create_landmark"
		{
			mFilterEditor->clear();
			onFilterEdit("", false);

			mPlaceInfoType = key_type;
			mPosGlobal.setZero();
			mItem = NULL;
			mRegionId.setNull();
			togglePlaceInfoPanel(TRUE);

			if (mPlaceInfoType == AGENT_INFO_TYPE)
			{
				mPlaceProfile->setInfoType(LLPanelPlaceInfo::AGENT);
				if (gAgent.getRegion())
				{
					mRegionId = gAgent.getRegion()->getRegionID();
				}
			}
			else if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
			{
				LLUUID dest_folder = key["dest_folder"];
				mLandmarkInfo->setInfoAndCreateLandmark(dest_folder);

				if (key.has("x") && key.has("y") && key.has("z"))
				{
					mPosGlobal = LLVector3d(key["x"].asReal(),
											key["y"].asReal(),
											key["z"].asReal());
				}
				else
				{
					mPosGlobal = gAgent.getPositionGlobal();
				}

				mLandmarkInfo->displayParcelInfo(LLUUID(), mPosGlobal);

				mSaveBtn->setEnabled(FALSE);
			}
			else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
			{
				mLandmarkInfo->setInfoType(LLPanelPlaceInfo::LANDMARK);

				LLUUID id = key["id"].asUUID();
				LLInventoryItem* item = gInventory.getItem(id);
				if (!item)
					return;

                BOOL is_editable = gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID())
                                   && item->getPermissions().allowModifyBy(gAgent.getID());
                mLandmarkInfo->setCanEdit(is_editable);

				setItem(item);
			}
			else if (mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
			{
				if (key.has("id"))
				{
					LLUUID parcel_id = key["id"].asUUID();
					mPlaceProfile->setParcelID(parcel_id);

					// query the server to get the global 3D position of this
					// parcel - we need this for teleport/mapping functions.
					mRemoteParcelObserver->setParcelID(parcel_id);
				}
				else
				{
					mPosGlobal = LLVector3d(key["x"].asReal(),
											key["y"].asReal(),
											key["z"].asReal());
					mPlaceProfile->displayParcelInfo(LLUUID(), mPosGlobal);
				}

				mPlaceProfile->setInfoType(LLPanelPlaceInfo::PLACE);
			}
			else if (mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
			{
				S32 index = key["id"].asInteger();

				const LLTeleportHistoryStorage::slurl_list_t& hist_items =
							LLTeleportHistoryStorage::getInstance()->getItems();

				mPosGlobal = hist_items[index].mGlobalPos;

				mPlaceProfile->setInfoType(LLPanelPlaceInfo::TELEPORT_HISTORY);
				mPlaceProfile->displayParcelInfo(LLUUID(), mPosGlobal);
			}

			updateVerbs();
		}
	}

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
	if (!parcel_mgr)
		return;

	mParcelLocalId = parcel_mgr->getAgentParcel()->getLocalID();

	// Start using LLViewerParcelMgr for land selection if
	// information about nearby land is requested.
	// Otherwise stop using land selection and deselect land.
	if (mPlaceInfoType == AGENT_INFO_TYPE)
	{
		// We don't know if we are already added to LLViewerParcelMgr observers list
		// so try to remove observer not to add an extra one.
		parcel_mgr->removeObserver(mParcelObserver);

		parcel_mgr->addObserver(mParcelObserver);
		parcel_mgr->selectParcelAt(gAgent.getPositionGlobal());
	}
	else
	{
		parcel_mgr->removeObserver(mParcelObserver);

		// Clear the reference to selection to allow its removal in deselectUnused().
		mParcel.clear();

		if (!parcel_mgr->selectionEmpty())
		{
			parcel_mgr->deselectUnused();
		}
	}
}

void LLPanelPlaces::setItem(LLInventoryItem* item)
{
	if (!mLandmarkInfo || !item)
		return;

	mItem = item;

	LLAssetType::EType item_type = mItem->getActualType();
	if (item_type == LLAssetType::AT_LANDMARK || item_type == LLAssetType::AT_LINK)
	{
		// If the item is a link get a linked item
		if (item_type == LLAssetType::AT_LINK)
		{
			mItem = gInventory.getItem(mItem->getLinkedUUID());
			if (mItem.isNull())
				return;
		}
	}
	else
	{
		return;
	}

	// Check if item is in agent's inventory and he has the permission to modify it.
	BOOL is_landmark_editable = gInventory.isObjectDescendentOf(mItem->getUUID(), gInventory.getRootFolderID()) &&
								mItem->getPermissions().allowModifyBy(gAgent.getID());

	mSaveBtn->setEnabled(is_landmark_editable);

	if (is_landmark_editable)
	{
		if(!mLandmarkInfo->setLandmarkFolder(mItem->getParentUUID()) && !mItem->getParentUUID().isNull())
		{
			const LLViewerInventoryCategory* cat = gInventory.getCategory(mItem->getParentUUID());
			if (cat)
			{
				std::string cat_fullname = LLPanelLandmarkInfo::getFullFolderName(cat);
				LLComboBox* folderList = mLandmarkInfo->getChild<LLComboBox>("folder_combo");
				folderList->add(cat_fullname, cat->getUUID(), ADD_TOP);
			}
		}
	}

	mLandmarkInfo->displayItemInfo(mItem);

	LLLandmark* lm = gLandmarkList.getAsset(mItem->getAssetUUID(),
											boost::bind(&LLPanelPlaces::onLandmarkLoaded, this, _1));
	if (lm)
	{
		onLandmarkLoaded(lm);
	}
}

S32 LLPanelPlaces::notifyParent(const LLSD& info)
{
	if(info.has("update_verbs"))
	{
		if(mPosGlobal.isExactlyZero())
		{
			mPosGlobal.setVec(info["global_x"], info["global_y"], info["global_z"]);
		}

		updateVerbs();
		
		return 1;
	}
	return LLPanel::notifyParent(info);
}

void LLPanelPlaces::onLandmarkLoaded(LLLandmark* landmark)
{
	if (!mLandmarkInfo)
		return;

	LLUUID region_id;
	landmark->getRegionID(region_id);
	landmark->getGlobalPos(mPosGlobal);
	mLandmarkInfo->displayParcelInfo(region_id, mPosGlobal);

	updateVerbs();
}

void LLPanelPlaces::onFilterEdit(const std::string& search_string, bool force_filter)
{
	if (!mActivePanel)
		return;

	if (force_filter || mActivePanel->getFilterSubString() != search_string)
	{
		std::string string = search_string;

		// Searches are case-insensitive
		// but we don't convert the typed string to upper-case so that it can be fed to the web search as-is.

		mActivePanel->onSearchEdit(string);
	}
}

void LLPanelPlaces::onTabSelected()
{
	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());
	if (!mActivePanel)
		return;

	onFilterEdit(mActivePanel->getFilterSubString(), true);
	mActivePanel->updateVerbs();

    // History panel does not support deletion nor creation
    // Hide menus
    bool supports_create = mActivePanel->getCreateMenu() != NULL;
    childSetVisible("add_btn_panel", supports_create);

    // favorites and inventory can remove items, history can clear history
    childSetVisible("trash_btn_panel", TRUE);

    if (supports_create)
    {
        mRemoveSelectedBtn->setToolTip(getString("tooltip_trash_items"));
    }
    else
    {
        mRemoveSelectedBtn->setToolTip(getString("tooltip_trash_history"));
    }
}

void LLPanelPlaces::onTeleportButtonClicked()
{
	LLPanelPlaceInfo* panel = getCurrentInfoPanel();
	if (panel && panel->getVisible())
	{
		if (mPlaceInfoType == LANDMARK_INFO_TYPE)
		{
			if (mItem.isNull())
			{
				LL_WARNS() << "NULL landmark item" << LL_ENDL;
				llassert(mItem.notNull());
				return;
			}

			LLSD payload;
			payload["asset_id"] = mItem->getAssetUUID();
			LLSD args; 
			args["LOCATION"] = mItem->getName(); 
			LLNotificationsUtil::add("TeleportFromLandmark", args, payload);
		}
		else if (mPlaceInfoType == AGENT_INFO_TYPE ||
				 mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
				 mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
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
		if (mActivePanel)
			mActivePanel->onTeleport();
	}
}

void LLPanelPlaces::onShowOnMapButtonClicked()
{
	LLPanelPlaceInfo* panel = getCurrentInfoPanel();
	if (panel && panel->getVisible())
	{
		LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
		if(!worldmap_instance)
			return;

		if (mPlaceInfoType == AGENT_INFO_TYPE ||
			mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE ||
			mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
			mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
		{
			if (!mPosGlobal.isExactlyZero())
			{
				worldmap_instance->trackLocation(mPosGlobal);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE)
		{
			if (mItem.isNull())
			{
				LL_WARNS() << "NULL landmark item" << LL_ENDL;
				llassert(mItem.notNull());
				return;
			}
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
		if (mActivePanel && mActivePanel->isSingleItemSelected())
		{
			mActivePanel->onShowOnMap();
		}
		else
		{
			LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
			LLVector3d global_pos = gAgent.getPositionGlobal();

			if (!global_pos.isExactlyZero() && worldmap_instance)
			{
				worldmap_instance->trackLocation(global_pos);
				LLFloaterReg::showInstance("world_map", "center");
			}
		}
	}
}

void LLPanelPlaces::onEditButtonClicked()
{
	if (!mLandmarkInfo || isLandmarkEditModeOn)
		return;

	isLandmarkEditModeOn = true;

	mLandmarkInfo->toggleLandmarkEditMode(TRUE);

	updateVerbs();
}

void LLPanelPlaces::onSaveButtonClicked()
{
	if (!mLandmarkInfo || mItem.isNull())
		return;

	std::string current_title_value = mLandmarkInfo->getLandmarkTitle();
	std::string item_title_value = mItem->getName();
	std::string current_notes_value = mLandmarkInfo->getLandmarkNotes();
	std::string item_notes_value = mItem->getDescription();

	LLStringUtil::trim(current_title_value);
	LLStringUtil::trim(current_notes_value);

	LLUUID folder_id = mLandmarkInfo->getLandmarkFolder();
	bool change_parent = folder_id != mItem->getParentUUID();

	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(mItem);

	if (!current_title_value.empty() &&
		(item_title_value != current_title_value || item_notes_value != current_notes_value))
	{
		new_item->rename(current_title_value);
		new_item->setDescription(current_notes_value);
		LLPointer<LLInventoryCallback> cb;
		if (change_parent)
		{
			cb = new LLUpdateLandmarkParent(new_item, folder_id);
		}
		LLInventoryModel::LLCategoryUpdate up(mItem->getParentUUID(), 0);
		gInventory.accountForUpdate(up);
		update_inventory_item(new_item, cb);
	}
	else if (change_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(mItem->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(folder_id, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		new_item->setParent(folder_id);
		new_item->updateParentOnServer(FALSE);
	}

	gInventory.updateItem(new_item);
	gInventory.notifyObservers();

	onCancelButtonClicked();
}

void LLPanelPlaces::onCancelButtonClicked()
{
	if (!mLandmarkInfo)
		return;

	if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE)
	{
		onBackButtonClicked();
	}
	else
	{
		mLandmarkInfo->toggleLandmarkEditMode(FALSE);
		isLandmarkEditModeOn = false;

		updateVerbs();

		// Reload the landmark properties.
		mLandmarkInfo->displayItemInfo(mItem);
	}
}

void LLPanelPlaces::onOverflowButtonClicked()
{
	LLToggleableMenu* menu;

	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;

	if ((is_agent_place_info_visible ||
		 mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
		 mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE) && mPlaceMenu != NULL)
	{
		menu = mPlaceMenu;

		bool landmark_item_enabled = false;
		LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
		if (is_agent_place_info_visible
			&& gAgent.getRegion()
			&& mRegionId == gAgent.getRegion()->getRegionID()
			&& parcel_mgr
			&& parcel_mgr->getAgentParcel()->getLocalID() == mParcelLocalId)
		{
			// Floater still shows location identical to agent's position
			landmark_item_enabled = !LLLandmarkActions::landmarkAlreadyExists();
		}

		// Enable adding a landmark only for agent current parcel and if
		// there is no landmark already pointing to that parcel in agent's inventory.
		menu->getChild<LLMenuItemCallGL>("landmark")->setEnabled(landmark_item_enabled);
		// STORM-411
		// Creating landmarks for remote locations is impossible.
		// So hide menu item "Make a Landmark" in "Teleport History Profile" panel.
		menu->setItemVisible("landmark", mPlaceInfoType != TELEPORT_HISTORY_INFO_TYPE);
		menu->arrangeAndClear();
	}
	else if (mPlaceInfoType == LANDMARK_INFO_TYPE && mLandmarkMenu != NULL)
	{
		menu = mLandmarkMenu;

		BOOL is_landmark_removable = FALSE;
		if (mItem.notNull())
		{
			const LLUUID& item_id = mItem->getUUID();
			const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
			is_landmark_removable = gInventory.isObjectDescendentOf(item_id, gInventory.getRootFolderID()) &&
									!gInventory.isObjectDescendentOf(item_id, trash_id);
		}

		menu->getChild<LLMenuItemCallGL>("delete")->setEnabled(is_landmark_removable);
	}
	else
	{
		return;
	}

	mOverflowBtn->setMenu(menu, LLMenuButton::MP_TOP_RIGHT);
}

bool LLPanelPlaces::onOverflowMenuItemEnable(const LLSD& param)
{
	std::string value = param.asString();
	if("can_create_pick" == value)
	{
		return !LLAgentPicksInfo::getInstance()->isPickLimitReached();
	}
	return true;
}

void LLPanelPlaces::onOverflowMenuItemClicked(const LLSD& param)
{
	std::string item = param.asString();
	if (item == "landmark")
	{
		LLSD key;
		key["type"] = CREATE_LANDMARK_INFO_TYPE;
		key["x"] = mPosGlobal.mdV[VX];
		key["y"] = mPosGlobal.mdV[VY];
		key["z"] = mPosGlobal.mdV[VZ];
		onOpen(key);
	}
	else if (item == "copy")
	{
		LLLandmarkActions::getSLURLfromPosGlobal(mPosGlobal, boost::bind(&onSLURLBuilt, _1));
	}
	else if (item == "delete")
	{
		gInventory.removeItem(mItem->getUUID());

		onBackButtonClicked();
	}
	else if (item == "pick")
	{
		LLPanelPlaceInfo* panel = getCurrentInfoPanel();
		if (panel)
		{
			panel->createPick(mPosGlobal);
		}
	}
	else if (item == "add_to_favbar")
	{
		if ( mItem.notNull() )
		{
			const LLUUID& favorites_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
			if ( favorites_id.notNull() )
			{
				copy_inventory_item(gAgent.getID(),
									mItem->getPermissions().getOwner(),
									mItem->getUUID(),
									favorites_id,
									std::string(),
									LLPointer<LLInventoryCallback>(NULL));
				LL_INFOS() << "Copied inventory item #" << mItem->getUUID() << " to favorites." << LL_ENDL;
			}
		}
	}
}

void LLPanelPlaces::onBackButtonClicked()
{
	togglePlaceInfoPanel(FALSE);

	// Resetting mPlaceInfoType when Place Info panel is closed.
	mPlaceInfoType = LLStringUtil::null;

	isLandmarkEditModeOn = false;

	updateVerbs();
}

void LLPanelPlaces::onGearMenuClick()
{
    if (mActivePanel)
    {
        LLToggleableMenu* menu = mActivePanel->getSelectionMenu();
        mGearMenuButton->setMenu(menu, LLMenuButton::MP_BOTTOM_LEFT);
    }
}

void LLPanelPlaces::onSortingMenuClick()
{
    if (mActivePanel)
    {
        LLToggleableMenu* menu = mActivePanel->getSortingMenu();
        mSortingMenuButton->setMenu(menu, LLMenuButton::MP_BOTTOM_LEFT);
    }
}

void LLPanelPlaces::onAddMenuClick()
{
    if (mActivePanel)
    {
        LLToggleableMenu* menu = mActivePanel->getCreateMenu();
        mAddMenuButton->setMenu(menu, LLMenuButton::MP_BOTTOM_LEFT);
    }
}

void LLPanelPlaces::onRemoveButtonClicked()
{
    if (mActivePanel)
    {
        mActivePanel->onRemoveSelected();
    }
}

bool LLPanelPlaces::handleDragAndDropToTrash(BOOL drop, EDragAndDropType cargo_type, void* cargo_data, EAcceptance* accept)
{
    if (mActivePanel)
    {
        return mActivePanel->handleDragAndDropToTrash(drop, cargo_type, cargo_data, accept);
    }
    return false;
}

void LLPanelPlaces::togglePlaceInfoPanel(BOOL visible)
{
	if (!mPlaceProfile || !mLandmarkInfo)
		return;

	mTabContainer->setVisible(!visible);
	mButtonsContainer->setVisible(visible);
	mFilterContainer->setVisible(!visible);

	if (mPlaceInfoType == AGENT_INFO_TYPE ||
		mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
		mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
	{
		mPlaceProfile->setVisible(visible);

		if (visible)
		{
			mPlaceProfile->resetLocation();

			// Do not reset location info until mResetInfoTimer has expired
			// to avoid text blinking.
			mResetInfoTimer.setTimerExpirySec(PLACE_INFO_UPDATE_INTERVAL);

			mLandmarkInfo->setVisible(FALSE);
		}
		else if (mPlaceInfoType == AGENT_INFO_TYPE)
		{
			LLViewerParcelMgr::getInstance()->removeObserver(mParcelObserver);

			// Clear reference to parcel selection when closing place profile panel.
			// LLViewerParcelMgr removes the selection if it has 1 reference to it.
			mParcel.clear();
		}
	}
	else if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE ||
			 mPlaceInfoType == LANDMARK_INFO_TYPE ||
			 mPlaceInfoType == LANDMARK_TAB_INFO_TYPE)
	{
		mLandmarkInfo->setVisible(visible);
		mPlaceProfile->setVisible(FALSE);
		if (visible)
		{
			mLandmarkInfo->resetLocation();
		}
		else
		{
			std::string tab_panel_name("Landmarks");
			if (mItem.notNull())
			{
				if (gInventory.isObjectDescendentOf(mItem->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
				{
					tab_panel_name = "Favorites";
				}
			}
			
			LLLandmarksPanel* landmarks_panel = dynamic_cast<LLLandmarksPanel*>(mTabContainer->getPanelByName(tab_panel_name));
			if (landmarks_panel)
			{
				// If a landmark info is being closed we open the landmarks tab
				// and set this landmark selected.
				mTabContainer->selectTabPanel(landmarks_panel);
				if (mItem.notNull())
				{
					landmarks_panel->setItemSelected(mItem->getUUID(), TRUE);
				}
				else
				{
					landmarks_panel->resetSelection();
				}
			}
		}
	}
}

// virtual
void LLPanelPlaces::onVisibilityChange(BOOL new_visibility)
{
	LLPanel::onVisibilityChange(new_visibility);

	if (!new_visibility && mPlaceInfoType == AGENT_INFO_TYPE)
	{
		LLViewerParcelMgr::getInstance()->removeObserver(mParcelObserver);

		// Clear reference to parcel selection when closing places panel.
		mParcel.clear();
	}
}

void LLPanelPlaces::changedParcelSelection()
{
	if (!mPlaceProfile)
		return;

	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();
	mParcel = parcel_mgr->getFloatingParcelSelection();
	LLParcel* parcel = mParcel->getParcel();
	LLViewerRegion* region = parcel_mgr->getSelectionRegion();
	if (!region || !parcel)
		return;

	LLVector3d prev_pos_global = mPosGlobal;

	// If agent is inside the selected parcel show agent's region<X, Y, Z>,
	// otherwise show region<X, Y, Z> of agent's selection point.
	bool is_current_parcel = is_agent_in_selected_parcel(parcel);
	if (is_current_parcel)
	{
		mPosGlobal = gAgent.getPositionGlobal();
	}
	else
	{
		LLVector3d pos_global = gViewerWindow->getLastPick().mPosGlobal;
		if (!pos_global.isExactlyZero())
		{
			mPosGlobal = pos_global;
		}
	}

	// Reset location info only if global position has changed
	// and update timer has expired to reduce unnecessary text and icons updates.
	if (prev_pos_global != mPosGlobal && mResetInfoTimer.hasExpired())
	{
		mPlaceProfile->resetLocation();
		mResetInfoTimer.setTimerExpirySec(PLACE_INFO_UPDATE_INTERVAL);
	}

	mPlaceProfile->displaySelectedParcelInfo(parcel, region, mPosGlobal, is_current_parcel);

	updateVerbs();
}

void LLPanelPlaces::createTabs()
{
	if (!(gInventory.isInventoryUsable() && LLTeleportHistory::getInstance() && !mTabsCreated))
		return;

	LLFavoritesPanel* favorites_panel = new LLFavoritesPanel();
	if (favorites_panel)
	{
		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(favorites_panel).
			label(getString("favorites_tab_title")).
			insert_at(LLTabContainer::END));
	}

	LLLandmarksPanel* landmarks_panel = new LLLandmarksPanel();
	if (landmarks_panel)
	{
		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(landmarks_panel).
			label(getString("landmarks_tab_title")).
			insert_at(LLTabContainer::END));
	}

	LLTeleportHistoryPanel* teleport_history_panel = new LLTeleportHistoryPanel();
	if (teleport_history_panel)
	{
		mTabContainer->addTabPanel(
			LLTabContainer::TabPanelParams().
			panel(teleport_history_panel).
			label(getString("teleport_history_tab_title")).
			insert_at(LLTabContainer::END));
	}

	mTabContainer->selectFirstTab();

	mActivePanel = dynamic_cast<LLPanelPlacesTab*>(mTabContainer->getCurrentPanel());

    if (mActivePanel)
    {
        // Filter applied to show all items.
        mActivePanel->onSearchEdit(mActivePanel->getFilterSubString());

        // History panel does not support deletion nor creation
        // Hide menus
        bool supports_create = mActivePanel->getCreateMenu() != NULL;
        childSetVisible("add_btn_panel", supports_create);

        // favorites and inventory can remove items, history can clear history
        childSetVisible("trash_btn_panel", TRUE);

        if (supports_create)
        {
            mRemoveSelectedBtn->setToolTip(getString("tooltip_trash_items"));
        }
        else
        {
            mRemoveSelectedBtn->setToolTip(getString("tooltip_trash_history"));
        }

        mActivePanel->setRemoveBtn(mRemoveSelectedBtn);
		mActivePanel->updateVerbs();
    }

	mTabsCreated = true;
}

void LLPanelPlaces::changedGlobalPos(const LLVector3d &global_pos)
{
	mPosGlobal = global_pos;
	updateVerbs();
}

void LLPanelPlaces::showAddedLandmarkInfo(const uuid_set_t& items)
{
	for (uuid_set_t::const_iterator item_iter = items.begin();
		 item_iter != items.end();
		 ++item_iter)
	{
		const LLUUID& item_id = (*item_iter);
		if(!highlight_offered_object(item_id))
		{
			continue;
		}

		LLInventoryItem* item = gInventory.getItem(item_id);

		llassert(item);
		if (item && (LLAssetType::AT_LANDMARK == item->getType()) )
		{
			// Created landmark is passed to Places panel to allow its editing.
			// If the panel is closed we don't reopen it until created landmark is loaded.
			if("create_landmark" == getPlaceInfoType() && !getItem())
			{
				setItem(item);
			}
		}
	}
}

void LLPanelPlaces::updateVerbs()
{
	bool is_place_info_visible;

	LLPanelPlaceInfo* panel = getCurrentInfoPanel();
	if (panel)
	{
		is_place_info_visible = panel->getVisible();
	}
	else
	{
		is_place_info_visible = false;
	}

	bool is_agent_place_info_visible = mPlaceInfoType == AGENT_INFO_TYPE;
	bool is_create_landmark_visible = mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE;

	bool have_3d_pos = ! mPosGlobal.isExactlyZero();

	mTeleportBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mShowOnMapBtn->setVisible(!is_create_landmark_visible && !isLandmarkEditModeOn);
	mSaveBtn->setVisible(isLandmarkEditModeOn);
	mCancelBtn->setVisible(isLandmarkEditModeOn);
	mCloseBtn->setVisible(is_create_landmark_visible && !isLandmarkEditModeOn);

	bool show_options_btn = is_place_info_visible && !is_create_landmark_visible && !isLandmarkEditModeOn;
	mOverflowBtn->setVisible(show_options_btn);
	getChild<LLLayoutPanel>("lp_options")->setVisible(show_options_btn);
	getChild<LLLayoutPanel>("lp2")->setVisible(!show_options_btn);

	if (is_place_info_visible)
	{
		mShowOnMapBtn->setEnabled(have_3d_pos);

		if (is_agent_place_info_visible)
		{
			// We don't need to teleport to the current location
			// so check if the location is not within the current parcel.
			mTeleportBtn->setEnabled(have_3d_pos &&
									 !LLViewerParcelMgr::getInstance()->inAgentParcel(mPosGlobal));
		}
		else if (mPlaceInfoType == LANDMARK_INFO_TYPE || mPlaceInfoType == REMOTE_PLACE_INFO_TYPE)
		{
			mTeleportBtn->setEnabled(have_3d_pos);
		}
	}
	else
	{
		if (mActivePanel)
			mActivePanel->updateVerbs();
	}
}

LLPanelPlaceInfo* LLPanelPlaces::getCurrentInfoPanel()
{
	if (mPlaceInfoType == AGENT_INFO_TYPE ||
		mPlaceInfoType == REMOTE_PLACE_INFO_TYPE ||
		mPlaceInfoType == TELEPORT_HISTORY_INFO_TYPE)
	{
		return mPlaceProfile;
	}
	else if (mPlaceInfoType == CREATE_LANDMARK_INFO_TYPE ||
			 mPlaceInfoType == LANDMARK_INFO_TYPE ||
			 mPlaceInfoType == LANDMARK_TAB_INFO_TYPE)
	{
		return mLandmarkInfo;
	}

	return NULL;
}

static bool is_agent_in_selected_parcel(LLParcel* parcel)
{
	LLViewerParcelMgr* parcel_mgr = LLViewerParcelMgr::getInstance();

	LLViewerRegion* region = parcel_mgr->getSelectionRegion();
	if (!region || !parcel)
		return false;

	return	region == gAgent.getRegion() &&
			parcel->getLocalID() == parcel_mgr->getAgentParcel()->getLocalID();
}

static void onSLURLBuilt(std::string& slurl)
{
	LLView::getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
		
	LLSD args;
	args["SLURL"] = slurl;

	LLNotificationsUtil::add("CopySLURL", args);
}
