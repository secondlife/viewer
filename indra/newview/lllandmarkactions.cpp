/**
* @file lllandmarkactions.cpp
* @brief LLLandmarkActions class implementation
*
* $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "lllandmarkactions.h"

#include "roles_constants.h"

#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "lllandmark.h"
#include "llparcel.h"
#include "llregionhandle.h"

#include "llnotificationsutil.h"

#include "llagent.h"
#include "llagentui.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "lllandmarklist.h"
#include "llslurl.h"
#include "llstring.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llworldmapmessage.h"
#include "llviewernetwork.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llworldmap.h"

void copy_slurl_to_clipboard_callback(const std::string& slurl);

class LLFetchlLandmarkByPos : public LLInventoryCollectFunctor
{
private:
    LLVector3d mPos;
public:
    LLFetchlLandmarkByPos(const LLVector3d& pos) :
        mPos(pos)
    {}

    /*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
    {
        if (!item || item->getType() != LLAssetType::AT_LANDMARK)
            return false;

        LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
        if (!landmark) // the landmark not been loaded yet
            return false;

        LLVector3d landmark_global_pos;
        if (!landmark->getGlobalPos(landmark_global_pos))
            return false;
        //we have to round off each coordinates to compare positions properly
        return ll_round(mPos.mdV[VX]) ==  ll_round(landmark_global_pos.mdV[VX])
                && ll_round(mPos.mdV[VY]) ==  ll_round(landmark_global_pos.mdV[VY])
                && ll_round(mPos.mdV[VZ]) ==  ll_round(landmark_global_pos.mdV[VZ]);
    }
};

class LLFetchLandmarksByName : public LLInventoryCollectFunctor
{
private:
    std::string name;
    bool use_substring;
    //this member will be contain copy of founded items to keep the result unique
    std::set<std::string> check_duplicate;

public:
LLFetchLandmarksByName(std::string &landmark_name, bool if_use_substring)
:name(landmark_name),
use_substring(if_use_substring)
    {
    LLStringUtil::toLower(name);
    }

public:
    /*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
    {
        if (!item || item->getType() != LLAssetType::AT_LANDMARK)
            return false;

        LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
        if (!landmark) // the landmark not been loaded yet
            return false;

        bool acceptable = false;
        std::string landmark_name = item->getName();
        LLStringUtil::toLower(landmark_name);
        if(use_substring)
        {
            acceptable =  landmark_name.find( name ) != std::string::npos;
        }
        else
        {
            acceptable = landmark_name == name;
        }
        if(acceptable){
            if(check_duplicate.find(landmark_name) != check_duplicate.end()){
                // we have duplicated items in landmarks
                acceptable = false;
            }else{
                check_duplicate.insert(landmark_name);
            }
        }

        return acceptable;
    }
};

// Returns true if the given inventory item is a landmark pointing to the current parcel.
// Used to find out if there is at least one landmark from current parcel.
class LLFirstAgentParcelLandmark : public LLInventoryCollectFunctor
{
private:
    bool mFounded;// to avoid unnecessary  check

public:
    LLFirstAgentParcelLandmark(): mFounded(false){}

    /*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
    {
        if (mFounded || !item || item->getType() != LLAssetType::AT_LANDMARK)
            return false;

        LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
        if (!landmark) // the landmark not been loaded yet
            return false;

        LLVector3d landmark_global_pos;
        if (!landmark->getGlobalPos(landmark_global_pos))
            return false;
        mFounded = LLViewerParcelMgr::getInstance()->inAgentParcel(landmark_global_pos);
        return mFounded;
    }
};

static void fetch_landmarks(LLInventoryModel::cat_array_t& cats,
                            LLInventoryModel::item_array_t& items,
                            LLInventoryCollectFunctor& add)
{
    // Look in "My Favorites"
    const LLUUID favorites_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
    gInventory.collectDescendentsIf(favorites_folder_id,
        cats,
        items,
        LLInventoryModel::EXCLUDE_TRASH,
        add);

    // Look in "Landmarks"
    const LLUUID landmarks_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
    gInventory.collectDescendentsIf(landmarks_folder_id,
        cats,
        items,
        LLInventoryModel::EXCLUDE_TRASH,
        add);
}

LLInventoryModel::item_array_t LLLandmarkActions::fetchLandmarksByName(std::string& name, bool use_substring)
{
    LLInventoryModel::cat_array_t cats;
    LLInventoryModel::item_array_t items;
    LLFetchLandmarksByName by_name(name, use_substring);
    fetch_landmarks(cats, items, by_name);

    return items;
}

bool LLLandmarkActions::landmarkAlreadyExists()
{
    // Determine whether there are landmarks pointing to the current global  agent position.
    return findLandmarkForAgentPos() != NULL;
}

//static
bool LLLandmarkActions::hasParcelLandmark()
{
    LLFirstAgentParcelLandmark get_first_agent_landmark;
    LLInventoryModel::cat_array_t cats;
    LLInventoryModel::item_array_t items;
    fetch_landmarks(cats, items, get_first_agent_landmark);
    return !items.empty();

}

// *TODO: This could be made more efficient by only fetching the FIRST
// landmark that meets the criteria
LLViewerInventoryItem* LLLandmarkActions::findLandmarkForGlobalPos(const LLVector3d &pos)
{
    // Determine whether there are landmarks pointing to the current parcel.
    LLInventoryModel::cat_array_t cats;
    LLInventoryModel::item_array_t items;
    LLFetchlLandmarkByPos is_current_pos_landmark(pos);
    fetch_landmarks(cats, items, is_current_pos_landmark);

    if(items.empty())
    {
        return NULL;
    }

    return items[0];
}

LLViewerInventoryItem* LLLandmarkActions::findLandmarkForAgentPos()
{
    return findLandmarkForGlobalPos(gAgent.getPositionGlobal());
}

void LLLandmarkActions::createLandmarkHere(
    const std::string& name,
    const std::string& desc,
    const LLUUID& folder_id)
{
    if (!gAgent.getRegion())
    {
        LL_WARNS() << "No agent region" << LL_ENDL;
        return;
    }

    if (!LLViewerParcelMgr::getInstance()->getAgentParcel())
    {
        LL_WARNS() << "No agent parcel" << LL_ENDL;
        return;
    }

    create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
        folder_id, LLTransactionID::tnull,
        name, desc,
        LLAssetType::AT_LANDMARK,
        LLInventoryType::IT_LANDMARK,
        NO_INV_SUBTYPE, PERM_ALL,
        NULL);
}

void LLLandmarkActions::createLandmarkHere()
{
    std::string landmark_name, landmark_desc;

    LLAgentUI::buildLocationString(landmark_name, LLAgentUI::LOCATION_FORMAT_LANDMARK);
    LLAgentUI::buildLocationString(landmark_desc, LLAgentUI::LOCATION_FORMAT_FULL);
    const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);

    createLandmarkHere(landmark_name, landmark_desc, folder_id);
}

void LLLandmarkActions::showFloaterCreateLandmarkForUrl(const std::string& url, const std::string& title)
{
    LLSLURL slurl(url);
    if ((slurl.getType() != LLSLURL::LOCATION) &&
        (slurl.getType() != LLSLURL::APP || slurl.getAppCmd() != LLSLURL::SLURL_REGION_PATH))
    {
        LL_INFOS() << "Unsupported URL: '" << url << "'" << LL_ENDL;
        LLNotificationsUtil::add("CantCreateLandmark");
        return;
    }

    S32 x = (S32)std::round(slurl.getPosition()[VX]);
    S32 y = (S32)std::round(slurl.getPosition()[VY]);
    S32 z = (S32)std::round(slurl.getPosition()[VZ]);
    // When title == url we provide an empty string to create a human-readable title
    showFloaterCreateLandmarkForCoords(slurl.getRegion(), x, y, z, title == url ? LLStringUtil::null : title);
}

void LLLandmarkActions::showFloaterCreateLandmarkForPos(const LLVector3d& global_pos, const std::string& title)
{
    if (LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromPosGlobal(global_pos))
    {
        std::string region_name = info->getName();
        LLVector3 local_pos = info->getLocalPos(global_pos);
        S32 x = ll_round(local_pos.mV[VX]);
        S32 y = ll_round(local_pos.mV[VY]);
        S32 z = ll_round(local_pos.mV[VZ]);
        showFloaterCreateLandmarkForCoords(region_name, x, y, z, title);
        return;
    }

    LL_WARNS() << "No region found for global pos " << global_pos << LL_ENDL;
    LLNotificationsUtil::add("CantCreateLandmarkTryAgain");

    S32 x = S32(global_pos.mdV[0] / REGION_WIDTH_UNITS);
    S32 y = S32(global_pos.mdV[1] / REGION_WIDTH_UNITS);
    LLWorldMapMessage::getInstance()->sendMapBlockRequest(x, y, x, y, true);
}

void LLLandmarkActions::showFloaterCreateLandmarkForCoords(const std::string& region_name,
    S32 x, S32 y, S32 z, const std::string& title)
{
    LLSD data;
    data["region"] = region_name;
    data["x"] = x;
    data["y"] = y;
    data["z"] = z;
    data["title"] = title.empty() ? llformat("%s (%d, %d, %d)", region_name.c_str(), x, y, z) : title;

    LLFloaterReg::showInstance("add_landmark", data);
}

bool LLLandmarkActions::canCreateLandmarkForUrl(const std::string& url)
{
    if (LLApp::isExiting())
        return false;

    std::string cap = gAgent.getRegionCapability("CreateLandmarkForPosition");
    if (cap.empty())
        return false; // No region or cap is not supported by the region

    LLSLURL slurl(url);
    if (slurl.getType() == LLSLURL::LOCATION) // LLUrlEntryPlace
    {
        if (LLGridManager* gm = LLGridManager::getInstance())
        {
            std::string current_grid = gm->getGrid();
            std::string grid_from_url = gm->getGrid(slurl.getGrid());
            if (grid_from_url == current_grid) // The same grid location
            {
                return true;
            }
        }
    }
    else if (slurl.getType() == LLSLURL::APP)
    {
        if (slurl.getAppCmd() == LLSLURL::SLURL_REGION_PATH) // LLUrlEntryRegion
        {
            return true;
        }
    }

    return false;
}

void LLLandmarkActions::getSLURLfromPosGlobal(const LLVector3d& global_pos, slurl_callback_t cb, bool escaped /* = true */)
{
    std::string sim_name;
    bool gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal(global_pos, sim_name);
    if (gotSimName)
    {
      std::string slurl = LLSLURL(sim_name, global_pos).getSLURLString();
        cb(slurl);

        return;
    }
    else
    {
        U64 new_region_handle = to_region_handle(global_pos);

        LLWorldMapMessage::url_callback_t url_cb = boost::bind(&LLLandmarkActions::onRegionResponseSLURL,
                                                        cb,
                                                        global_pos,
                                                        escaped,
                                                        _2);

        LLWorldMapMessage::getInstance()->sendHandleRegionRequest(new_region_handle, url_cb, std::string("unused"), false);
    }
}

void LLLandmarkActions::getRegionNameAndCoordsFromPosGlobal(const LLVector3d& global_pos, region_name_and_coords_callback_t cb)
{
    std::string sim_name;
    LLSimInfo* sim_infop = LLWorldMap::getInstance()->simInfoFromPosGlobal(global_pos);
    if (sim_infop)
    {
        LLVector3 pos = sim_infop->getLocalPos(global_pos);
        std::string name = sim_infop->getName() ;
        cb(name, ll_round(pos.mV[VX]), ll_round(pos.mV[VY]),ll_round(pos.mV[VZ]));
    }
    else
    {
        U64 new_region_handle = to_region_handle(global_pos);

        LLWorldMapMessage::url_callback_t url_cb = boost::bind(&LLLandmarkActions::onRegionResponseNameAndCoords,
                                                        cb,
                                                        global_pos,
                                                        _1);

        LLWorldMapMessage::getInstance()->sendHandleRegionRequest(new_region_handle, url_cb, std::string("unused"), false);
    }
}

void LLLandmarkActions::onRegionResponseSLURL(slurl_callback_t cb,
                                         const LLVector3d& global_pos,
                                         bool escaped,
                                         const std::string& url)
{
    std::string sim_name;
    std::string slurl;
    bool gotSimName = LLWorldMap::getInstance()->simNameFromPosGlobal(global_pos, sim_name);
    if (gotSimName)
    {
      slurl = LLSLURL(sim_name, global_pos).getSLURLString();
    }
    else
    {
        slurl = "";
    }

    cb(slurl);
}

void LLLandmarkActions::onRegionResponseNameAndCoords(region_name_and_coords_callback_t cb,
                                         const LLVector3d& global_pos,
                                         U64 region_handle)
{
    LLSimInfo* sim_infop = LLWorldMap::getInstance()->simInfoFromHandle(region_handle);
    if (sim_infop)
    {
        LLVector3 local_pos = sim_infop->getLocalPos(global_pos);
        std::string name = sim_infop->getName() ;
        cb(name, ll_round(local_pos.mV[VX]), ll_round(local_pos.mV[VY]), ll_round(local_pos.mV[VZ]));
    }
}

bool LLLandmarkActions::getLandmarkGlobalPos(const LLUUID& landmarkInventoryItemID, LLVector3d& posGlobal)
{
    LLViewerInventoryItem* item = gInventory.getItem(landmarkInventoryItemID);
    if (NULL == item)
        return false;

    const LLUUID& asset_id = item->getAssetUUID();

    LLLandmark* landmark = gLandmarkList.getAsset(asset_id, NULL);
    if (NULL == landmark)
        return false;

    return landmark->getGlobalPos(posGlobal);
}

LLLandmark* LLLandmarkActions::getLandmark(const LLUUID& landmarkInventoryItemID, LLLandmarkList::loaded_callback_t cb)
{
    LLViewerInventoryItem* item = gInventory.getItem(landmarkInventoryItemID);
    if (NULL == item)
        return NULL;

    const LLUUID& asset_id = item->getAssetUUID();

    LLLandmark* landmark = gLandmarkList.getAsset(asset_id, cb);
    if (landmark)
    {
        return landmark;
    }

    return NULL;
}

void LLLandmarkActions::copySLURLtoClipboard(const LLUUID& landmarkInventoryItemID)
{
    LLLandmark* landmark = LLLandmarkActions::getLandmark(landmarkInventoryItemID);
    if(landmark)
    {
        LLVector3d global_pos;
        landmark->getGlobalPos(global_pos);
        LLLandmarkActions::getSLURLfromPosGlobal(global_pos,&copy_slurl_to_clipboard_callback,true);
    }
}

void copy_slurl_to_clipboard_callback(const std::string& slurl)
{
    gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
    LLSD args;
    args["SLURL"] = slurl;
    LLNotificationsUtil::add("CopySLURL", args);
}
