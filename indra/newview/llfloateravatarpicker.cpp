/**
 * @file llfloateravatarpicker.cpp
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llfloateravatarpicker.h"

// Viewer includes
#include "llagent.h"
#include "llcallingcard.h"
#include "llfocusmgr.h"
#include "llfloaterreg.h"
#include "llimview.h"           // for gIMMgr
#include "lltooldraganddrop.h"  // for LLToolDragAndDrop
#include "llviewercontrol.h"
#include "llviewerregion.h"     // getCapability()
#include "llworld.h"

// Linden libraries
#include "llavatarnamecache.h"  // IDEVO
#include "llbutton.h"
#include "llcachename.h"
#include "lllineeditor.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"
#include "llfocusmgr.h"
#include "lldraghandle.h"
#include "message.h"
#include "llcorehttputil.h"

//#include "llsdserialize.h"

static const U32 AVATAR_PICKER_SEARCH_TIMEOUT = 180U;

//put it back as a member once the legacy path is out?
static std::map<LLUUID, LLAvatarName> sAvatarNameMap;

LLFloaterAvatarPicker* LLFloaterAvatarPicker::show(select_callback_t callback,
                                                   bool allow_multiple,
                                                   bool closeOnSelect,
                                                   bool skip_agent,
                                                   const std::string& name,
                                                   LLView * frustumOrigin)
{
    // *TODO: Use a key to allow this not to be an effective singleton
    LLFloaterAvatarPicker* floater =
        LLFloaterReg::showTypedInstance<LLFloaterAvatarPicker>("avatar_picker", LLSD(name));
    if (!floater)
    {
        LL_WARNS() << "Cannot instantiate avatar picker" << LL_ENDL;
        return NULL;
    }

    floater->mSelectionCallback = callback;
    floater->setAllowMultiple(allow_multiple);
    floater->mNearMeListComplete = false;
    floater->mCloseOnSelect = closeOnSelect;
    floater->mExcludeAgentFromSearchResults = skip_agent;

    if (!closeOnSelect)
    {
        // Use Select/Close
        std::string select_string = floater->getString("Select");
        std::string close_string = floater->getString("Close");
        floater->getChild<LLButton>("ok_btn")->setLabel(select_string);
        floater->getChild<LLButton>("cancel_btn")->setLabel(close_string);
    }

    if(frustumOrigin)
    {
        floater->mFrustumOrigin = frustumOrigin->getHandle();
    }

    return floater;
}

// Default constructor
LLFloaterAvatarPicker::LLFloaterAvatarPicker(const LLSD& key)
  : LLFloater(key),
    mNumResultsReturned(0),
    mNearMeListComplete(false),
    mCloseOnSelect(false),
    mExcludeAgentFromSearchResults(false),
    mContextConeOpacity (0.f),
    mContextConeInAlpha(CONTEXT_CONE_IN_ALPHA),
    mContextConeOutAlpha(CONTEXT_CONE_OUT_ALPHA),
    mContextConeFadeTime(CONTEXT_CONE_FADE_TIME)
{
    mCommitCallbackRegistrar.add("Refresh.FriendList", { boost::bind(&LLFloaterAvatarPicker::populateFriend, this), cb_info::UNTRUSTED_THROTTLE });
}

bool LLFloaterAvatarPicker::postBuild()
{
    getChild<LLLineEditor>("Edit")->setKeystrokeCallback( boost::bind(&LLFloaterAvatarPicker::editKeystroke, this, _1, _2),NULL);

    childSetAction("Find", boost::bind(&LLFloaterAvatarPicker::onBtnFind, this));
    getChildView("Find")->setEnabled(false);
    childSetAction("Refresh", boost::bind(&LLFloaterAvatarPicker::onBtnRefresh, this));
    getChild<LLUICtrl>("near_me_range")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onRangeAdjust, this));

    LLScrollListCtrl* searchresults = getChild<LLScrollListCtrl>("SearchResults");
    searchresults->setDoubleClickCallback( boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    searchresults->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));
    getChildView("SearchResults")->setEnabled(false);

    LLScrollListCtrl* nearme = getChild<LLScrollListCtrl>("NearMe");
    nearme->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    nearme->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

    LLScrollListCtrl* friends = getChild<LLScrollListCtrl>("Friends");
    friends->setDoubleClickCallback(boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    getChild<LLUICtrl>("Friends")->setCommitCallback(boost::bind(&LLFloaterAvatarPicker::onList, this));

    childSetAction("ok_btn", boost::bind(&LLFloaterAvatarPicker::onBtnSelect, this));
    getChildView("ok_btn")->setEnabled(false);
    childSetAction("cancel_btn", boost::bind(&LLFloaterAvatarPicker::onBtnClose, this));

    getChild<LLUICtrl>("Edit")->setFocus(true);

    LLPanel* search_panel = getChild<LLPanel>("SearchPanel");
    if (search_panel)
    {
        // Start searching when Return is pressed in the line editor.
        search_panel->setDefaultBtn("Find");
    }

    getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("no_results"));

    getChild<LLTabContainer>("ResidentChooserTabs")->setCommitCallback(
        boost::bind(&LLFloaterAvatarPicker::onTabChanged, this));

    setAllowMultiple(false);

    center();

    populateFriend();

    return true;
}

void LLFloaterAvatarPicker::setOkBtnEnableCb(validate_callback_t cb)
{
    mOkButtonValidateSignal.connect(cb);
}

void LLFloaterAvatarPicker::onTabChanged()
{
    getChildView("ok_btn")->setEnabled(isSelectBtnEnabled());
}

// Destroys the object
LLFloaterAvatarPicker::~LLFloaterAvatarPicker()
{
    gFocusMgr.releaseFocusIfNeeded( this );
}

void LLFloaterAvatarPicker::onBtnFind()
{
    find();
}

static void getSelectedAvatarData(const LLScrollListCtrl* from, uuid_vec_t& avatar_ids, std::vector<LLAvatarName>& avatar_names)
{
    std::vector<LLScrollListItem*> items = from->getAllSelected();
    for (std::vector<LLScrollListItem*>::iterator iter = items.begin(); iter != items.end(); ++iter)
    {
        LLScrollListItem* item = *iter;
        if (item->getUUID().notNull())
        {
            avatar_ids.push_back(item->getUUID());

            std::map<LLUUID, LLAvatarName>::iterator iter = sAvatarNameMap.find(item->getUUID());
            if (iter != sAvatarNameMap.end())
            {
                avatar_names.push_back(iter->second);
            }
            else
            {
                // the only case where it isn't in the name map is friends
                // but it should be in the name cache
                LLAvatarName av_name;
                LLAvatarNameCache::get(item->getUUID(), &av_name);
                avatar_names.push_back(av_name);
            }
        }
    }
}

void LLFloaterAvatarPicker::onBtnSelect()
{

    // If select btn not enabled then do not callback
    if (!isSelectBtnEnabled())
        return;

    if(mSelectionCallback)
    {
        std::string acvtive_panel_name;
        LLScrollListCtrl* list =  NULL;
        LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();
        if(active_panel)
        {
            acvtive_panel_name = active_panel->getName();
        }
        if(acvtive_panel_name == "SearchPanel")
        {
            list = getChild<LLScrollListCtrl>("SearchResults");
        }
        else if(acvtive_panel_name == "NearMePanel")
        {
            list = getChild<LLScrollListCtrl>("NearMe");
        }
        else if (acvtive_panel_name == "FriendsPanel")
        {
            list = getChild<LLScrollListCtrl>("Friends");
        }

        if(list)
        {
            uuid_vec_t          avatar_ids;
            std::vector<LLAvatarName>   avatar_names;
            getSelectedAvatarData(list, avatar_ids, avatar_names);
            mSelectionCallback(avatar_ids, avatar_names);
        }
    }
    getChild<LLScrollListCtrl>("SearchResults")->deselectAllItems(true);
    getChild<LLScrollListCtrl>("NearMe")->deselectAllItems(true);
    getChild<LLScrollListCtrl>("Friends")->deselectAllItems(true);
    if(mCloseOnSelect)
    {
        mCloseOnSelect = false;
        closeFloater();
    }
}

void LLFloaterAvatarPicker::onBtnRefresh()
{
    getChild<LLScrollListCtrl>("NearMe")->deleteAllItems();
    getChild<LLScrollListCtrl>("NearMe")->setCommentText(getString("searching"));
    mNearMeListComplete = false;
}

void LLFloaterAvatarPicker::onBtnClose()
{
    closeFloater();
}

void LLFloaterAvatarPicker::onRangeAdjust()
{
    onBtnRefresh();
}

void LLFloaterAvatarPicker::onList()
{
    getChildView("ok_btn")->setEnabled(isSelectBtnEnabled());
}

void LLFloaterAvatarPicker::populateNearMe()
{
    bool all_loaded = true;
    bool empty = true;
    LLScrollListCtrl* near_me_scroller = getChild<LLScrollListCtrl>("NearMe");
    near_me_scroller->deleteAllItems();

    uuid_vec_t avatar_ids;
    LLWorld::getInstance()->getAvatars(&avatar_ids, NULL, gAgent.getPositionGlobal(), gSavedSettings.getF32("NearMeRange"));
    for(U32 i=0; i<avatar_ids.size(); i++)
    {
        LLUUID& av = avatar_ids[i];
        if(mExcludeAgentFromSearchResults && (av == gAgent.getID())) continue;
        LLSD element;
        element["id"] = av; // value
        LLAvatarName av_name;

        if (!LLAvatarNameCache::get(av, &av_name))
        {
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = gCacheName->getDefaultName();
            all_loaded = false;
        }
        else
        {
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = av_name.getDisplayName();
            element["columns"][1]["column"] = "username";
            element["columns"][1]["value"] = av_name.getUserName();

            sAvatarNameMap[av] = av_name;
        }
        near_me_scroller->addElement(element);
        empty = false;
    }

    if (empty)
    {
        getChildView("NearMe")->setEnabled(false);
        getChildView("ok_btn")->setEnabled(false);
        near_me_scroller->setCommentText(getString("no_one_near"));
    }
    else
    {
        getChildView("NearMe")->setEnabled(true);
        getChildView("ok_btn")->setEnabled(true);
        near_me_scroller->selectFirstItem();
        onList();
        near_me_scroller->setFocus(true);
    }

    if (all_loaded)
    {
        mNearMeListComplete = true;
    }
}

void LLFloaterAvatarPicker::populateFriend()
{
    LLScrollListCtrl* friends_scroller = getChild<LLScrollListCtrl>("Friends");
    friends_scroller->deleteAllItems();
    LLCollectAllBuddies collector;
    LLAvatarTracker::instance().applyFunctor(collector);
    LLCollectAllBuddies::buddy_map_t::iterator it;

    for(it = collector.mOnline.begin(); it!=collector.mOnline.end(); it++)
    {
        friends_scroller->addStringUUIDItem(it->second, it->first);
    }
    for(it = collector.mOffline.begin(); it!=collector.mOffline.end(); it++)
    {
        friends_scroller->addStringUUIDItem(it->second, it->first);
    }
    friends_scroller->sortByColumnIndex(0, true);
}

void LLFloaterAvatarPicker::drawFrustum()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mFrustumOrigin.get(), mContextConeFadeTime, mContextConeInAlpha, mContextConeOutAlpha);
}

void LLFloaterAvatarPicker::draw()
{
    drawFrustum();

    // sometimes it is hard to determine when Select/Ok button should be disabled (see LLAvatarActions::shareWithAvatars).
    // lets check this via mOkButtonValidateSignal callback periodically.
    static LLFrameTimer timer;
    if (timer.hasExpired())
    {
        timer.setTimerExpirySec(0.33f); // three times per second should be enough.

        // simulate list changes.
        onList();
        timer.start();
    }

    LLFloater::draw();
    if (!mNearMeListComplete && getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel() == getChild<LLPanel>("NearMePanel"))
    {
        populateNearMe();
    }
}

bool LLFloaterAvatarPicker::visibleItemsSelected() const
{
    LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();

    if(active_panel == getChild<LLPanel>("SearchPanel"))
    {
        return getChild<LLScrollListCtrl>("SearchResults")->getFirstSelectedIndex() >= 0;
    }
    else if(active_panel == getChild<LLPanel>("NearMePanel"))
    {
        return getChild<LLScrollListCtrl>("NearMe")->getFirstSelectedIndex() >= 0;
    }
    else if(active_panel == getChild<LLPanel>("FriendsPanel"))
    {
        return getChild<LLScrollListCtrl>("Friends")->getFirstSelectedIndex() >= 0;
    }
    return false;
}

/*static*/
void LLFloaterAvatarPicker::findCoro(std::string url, LLUUID queryID, std::string name)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    httpOpts->setTimeout(AVATAR_PICKER_SEARCH_TIMEOUT);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status || (status == LLCore::HttpStatus(HTTP_BAD_REQUEST)))
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    }
    else
    {
        result["failure_reason"] = status.toString();
    }

    LLFloaterAvatarPicker* floater =
        LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker", name);
    if (floater)
    {
        floater->processResponse(queryID, result);
    }
}


void LLFloaterAvatarPicker::find()
{
    //clear our stored LLAvatarNames
    sAvatarNameMap.clear();

    std::string text = getChild<LLUICtrl>("Edit")->getValue().asString();

    size_t separator_index = text.find_first_of(" ._");
    if (separator_index != text.npos)
    {
        std::string first = text.substr(0, separator_index);
        std::string last = text.substr(separator_index+1, text.npos);
        LLStringUtil::trim(last);
        if("Resident" == last)
        {
            text = first;
        }
    }

    mQueryID.generate();

    std::string url;
    url.reserve(128); // avoid a memory allocation or two

    LLViewerRegion* region = gAgent.getRegion();
    if(region)
    {
        url = region->getCapability("AvatarPickerSearch");
        // Prefer use of capabilities to search on both SLID and display name
        if (!url.empty())
        {
            // capability urls don't end in '/', but we need one to parse
            // query parameters correctly
            if (url.size() > 0 && url[url.size()-1] != '/')
            {
                url += "/";
            }
            url += "?page_size=100&names=";
            std::replace(text.begin(), text.end(), '.', ' ');
            url += LLURI::escape(text);
            LL_INFOS() << "avatar picker " << url << LL_ENDL;

            LLCoros::instance().launch("LLFloaterAvatarPicker::findCoro",
                boost::bind(&LLFloaterAvatarPicker::findCoro, url, mQueryID, getKey().asString()));
        }
        else
        {
            LLMessageSystem* msg = gMessageSystem;
            msg->newMessage("AvatarPickerRequest");
            msg->nextBlock("AgentData");
            msg->addUUID("AgentID", gAgent.getID());
            msg->addUUID("SessionID", gAgent.getSessionID());
            msg->addUUID("QueryID", mQueryID);  // not used right now
            msg->nextBlock("Data");
            msg->addString("Name", text);
            gAgent.sendReliableMessage();
        }
    }
    getChild<LLScrollListCtrl>("SearchResults")->deleteAllItems();
    getChild<LLScrollListCtrl>("SearchResults")->setCommentText(getString("searching"));

    getChildView("ok_btn")->setEnabled(false);
    mNumResultsReturned = 0;
}

void LLFloaterAvatarPicker::setAllowMultiple(bool allow_multiple)
{
    getChild<LLScrollListCtrl>("SearchResults")->setAllowMultipleSelection(allow_multiple);
    getChild<LLScrollListCtrl>("NearMe")->setAllowMultipleSelection(allow_multiple);
    getChild<LLScrollListCtrl>("Friends")->setAllowMultipleSelection(allow_multiple);
}

LLScrollListCtrl* LLFloaterAvatarPicker::getActiveList()
{
    std::string acvtive_panel_name;
    LLScrollListCtrl* list = NULL;
    LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();
    if(active_panel)
    {
        acvtive_panel_name = active_panel->getName();
    }
    if(acvtive_panel_name == "SearchPanel")
    {
        list = getChild<LLScrollListCtrl>("SearchResults");
    }
    else if(acvtive_panel_name == "NearMePanel")
    {
        list = getChild<LLScrollListCtrl>("NearMe");
    }
    else if (acvtive_panel_name == "FriendsPanel")
    {
        list = getChild<LLScrollListCtrl>("Friends");
    }
    return list;
}

bool LLFloaterAvatarPicker::handleDragAndDrop(S32 x, S32 y, MASK mask,
                                              bool drop, EDragAndDropType cargo_type,
                                              void *cargo_data, EAcceptance *accept,
                                              std::string& tooltip_msg)
{
    LLScrollListCtrl* list = getActiveList();
    if(list)
    {
        LLRect rc_list;
        LLRect rc_point(x,y,x,y);
        if (localRectToOtherView(rc_point, &rc_list, list))
        {
            // Keep selected only one item
            list->deselectAllItems(true);
            list->selectItemAt(rc_list.mLeft, rc_list.mBottom, mask);
            LLScrollListItem* selection = list->getFirstSelected();
            if (selection)
            {
                LLUUID session_id = LLUUID::null;
                LLUUID dest_agent_id = selection->getUUID();
                std::string avatar_name = selection->getColumn(0)->getValue().asString();
                if (dest_agent_id.notNull() && dest_agent_id != gAgentID)
                {
                    if (drop)
                    {
                        // Start up IM before give the item
                        session_id = gIMMgr->addSession(avatar_name, IM_NOTHING_SPECIAL, dest_agent_id);
                    }
                    return LLToolDragAndDrop::handleGiveDragAndDrop(dest_agent_id, session_id, drop,
                                                                    cargo_type, cargo_data, accept, getName());
                }
            }
        }
    }
    *accept = ACCEPT_NO;
    return true;
}


void LLFloaterAvatarPicker::openFriendsTab()
{
    LLTabContainer* tab_container = getChild<LLTabContainer>("ResidentChooserTabs");
    if (tab_container == NULL)
    {
        llassert(tab_container != NULL);
        return;
    }

    tab_container->selectTabByName("FriendsPanel");
}

// static
void LLFloaterAvatarPicker::processAvatarPickerReply(LLMessageSystem* msg, void**)
{
    LLUUID  agent_id;
    LLUUID  query_id;
    LLUUID  avatar_id;
    std::string first_name;
    std::string last_name;

    msg->getUUID("AgentData", "AgentID", agent_id);
    msg->getUUID("AgentData", "QueryID", query_id);

    // Not for us
    if (agent_id != gAgent.getID()) return;

    LLFloaterAvatarPicker* floater = LLFloaterReg::findTypedInstance<LLFloaterAvatarPicker>("avatar_picker");

    // floater is closed or these are not results from our last request
    if (NULL == floater || query_id != floater->mQueryID)
    {
        return;
    }

    LLScrollListCtrl* search_results = floater->getChild<LLScrollListCtrl>("SearchResults");

    // clear "Searching" label on first results
    if (floater->mNumResultsReturned++ == 0)
    {
        search_results->deleteAllItems();
    }

    bool found_one = false;
    S32 num_new_rows = msg->getNumberOfBlocks("Data");
    for (S32 i = 0; i < num_new_rows; i++)
    {
        msg->getUUIDFast(  _PREHASH_Data,_PREHASH_AvatarID, avatar_id, i);
        msg->getStringFast(_PREHASH_Data,_PREHASH_FirstName, first_name, i);
        msg->getStringFast(_PREHASH_Data,_PREHASH_LastName, last_name, i);

        if (avatar_id != agent_id || !floater->isExcludeAgentFromSearchResults()) // exclude agent from search results?
        {
            std::string avatar_name;
            if (avatar_id.isNull())
            {
                LLStringUtil::format_map_t map;
                map["[TEXT]"] = floater->getChild<LLUICtrl>("Edit")->getValue().asString();
                avatar_name = floater->getString("not_found", map);
                search_results->setEnabled(false);
                floater->getChildView("ok_btn")->setEnabled(false);
            }
            else
            {
                avatar_name = LLCacheName::buildFullName(first_name, last_name);
                search_results->setEnabled(true);
                found_one = true;

                LLAvatarName av_name;
                av_name.fromString(avatar_name);
                const LLUUID& agent_id = avatar_id;
                sAvatarNameMap[agent_id] = av_name;

            }
            LLSD element;
            element["id"] = avatar_id; // value
            element["columns"][0]["column"] = "name";
            element["columns"][0]["value"] = avatar_name;
            search_results->addElement(element);
        }
    }

    if (found_one)
    {
        floater->getChildView("ok_btn")->setEnabled(true);
        search_results->selectFirstItem();
        floater->onList();
        search_results->setFocus(true);
    }
}

void LLFloaterAvatarPicker::processResponse(const LLUUID& query_id, const LLSD& content)
{
    // Check for out-of-date query
    if (query_id == mQueryID)
    {
        LLScrollListCtrl* search_results = getChild<LLScrollListCtrl>("SearchResults");

        // clear "Searching" label on first results
        search_results->deleteAllItems();

        if (content.has("failure_reason"))
        {
            getChild<LLScrollListCtrl>("SearchResults")->setCommentText(content["failure_reason"].asString());
            getChildView("ok_btn")->setEnabled(false);
        }
        else
        {
            LLSD agents = content["agents"];

            LLSD item;
            LLSD::array_const_iterator it = agents.beginArray();
            for (; it != agents.endArray(); ++it)
            {
                const LLSD& row = *it;
                if (row["id"].asUUID() != gAgent.getID() || !mExcludeAgentFromSearchResults)
                {
                    item["id"] = row["id"];
                    LLSD& columns = item["columns"];
                    columns[0]["column"] = "name";
                    columns[0]["value"] = row["display_name"];
                    columns[1]["column"] = "username";
                    columns[1]["value"] = row["username"];
                    search_results->addElement(item);

                    // add the avatar name to our list
                    LLAvatarName avatar_name;
                    avatar_name.fromLLSD(row);
                    sAvatarNameMap[row["id"].asUUID()] = avatar_name;
                }
            }

            if (search_results->isEmpty())
            {
                std::string name = "'" + getChild<LLUICtrl>("Edit")->getValue().asString() + "'";
                LLSD item;
                item["id"] = LLUUID::null;
                item["columns"][0]["column"] = "name";
                item["columns"][0]["value"] = name;
                item["columns"][1]["column"] = "username";
                item["columns"][1]["value"] = getString("not_found_text");
                search_results->addElement(item);
                search_results->setEnabled(false);
                getChildView("ok_btn")->setEnabled(false);
            }
            else
            {
                getChildView("ok_btn")->setEnabled(true);
                search_results->setEnabled(true);
                search_results->sortByColumnIndex(1, true);
                std::string text = getChild<LLUICtrl>("Edit")->getValue().asString();
                if (!search_results->selectItemByLabel(text, true, 1))
                {
                    search_results->selectFirstItem();
                }
                onList();
                search_results->setFocus(true);
            }
        }
    }
}

void LLFloaterAvatarPicker::editKeystroke(LLLineEditor* caller, void* user_data)
{
    getChildView("Find")->setEnabled(caller->getText().size() > 0);
}

// virtual
bool LLFloaterAvatarPicker::handleKeyHere(KEY key, MASK mask)
{
    if (key == KEY_RETURN && mask == MASK_NONE)
    {
        if (getChild<LLUICtrl>("Edit")->hasFocus())
        {
            onBtnFind();
        }
        else
        {
            onBtnSelect();
        }
        return true;
    }
    else if (key == KEY_ESCAPE && mask == MASK_NONE)
    {
        closeFloater();
        return true;
    }

    return LLFloater::handleKeyHere(key, mask);
}

bool LLFloaterAvatarPicker::isSelectBtnEnabled()
{
    bool ret_val = visibleItemsSelected();

    if ( ret_val && !isMinimized())
    {
        std::string acvtive_panel_name;
        LLScrollListCtrl* list =  NULL;
        LLPanel* active_panel = getChild<LLTabContainer>("ResidentChooserTabs")->getCurrentPanel();

        if(active_panel)
        {
            acvtive_panel_name = active_panel->getName();
        }

        if(acvtive_panel_name == "SearchPanel")
        {
            list = getChild<LLScrollListCtrl>("SearchResults");
        }
        else if(acvtive_panel_name == "NearMePanel")
        {
            list = getChild<LLScrollListCtrl>("NearMe");
        }
        else if (acvtive_panel_name == "FriendsPanel")
        {
            list = getChild<LLScrollListCtrl>("Friends");
        }

        if(list)
        {
            uuid_vec_t avatar_ids;
            std::vector<LLAvatarName> avatar_names;
            getSelectedAvatarData(list, avatar_ids, avatar_names);
            if (avatar_ids.size() >= 1)
            {
                ret_val = mOkButtonValidateSignal.num_slots()?mOkButtonValidateSignal(avatar_ids):true;
            }
            else
            {
                ret_val = false;
            }
        }
    }

    return ret_val;
}
