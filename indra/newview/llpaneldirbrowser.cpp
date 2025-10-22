/**
 * @file llpaneldirbrowser.cpp
 * @brief Base class for panels in the legacy Search directory.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// Base class for the various search panels/results browsers
// in the Find floater.  For example, Find > Popular Places
// is derived from this.

#include "llviewerprecompiledheaders.h"

#include "llpaneldirbrowser.h"

// linden library includes
#include "lldir.h"
#include "lleventflags.h"
#include "llqueryflags.h"
#include "message.h"

// viewer project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpanelavatar.h"
#include "llpanelgroup.h"
#include "llpanelclassified.h"
#include "llpaneldirevents.h"
#include "llpaneldirland.h"
#include "llproductinforequest.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llfloaterdirectory.h"
#include "llpanelprofile.h"
#include "llpanelplaces.h"
#include "llpaneleventinfo.h"


std::map<LLUUID, LLPanelDirBrowser*> gDirBrowserInstances;

LLPanelDirBrowser::LLPanelDirBrowser()
:   LLPanel(),
    mSearchID(),
    mWantSelectID(),
    mCurrentSortColumn("name"),
    mCurrentSortAscending(true),
    mSearchStart(0),
    mResultsPerPage(100),
    mResultsReceived(0),
    mMinSearchChars(1),
    mResultsContents(),
    mHaveSearchResults(false),
    mDidAutoSelect(true),
    mLastResultTimer(),
    mFloaterDirectory(nullptr)
{
}

bool LLPanelDirBrowser::postBuild()
{
    childSetCommitCallback("results", onCommitList, this);

    childSetAction("< Prev", onClickPrev, this);
    childSetVisible("< Prev", false);

    childSetAction("Next >", onClickNext, this);
    childSetVisible("Next >", false);

    return true;
}

LLPanelDirBrowser::~LLPanelDirBrowser()
{
    // Children all cleaned up by default view destructor.
    gDirBrowserInstances.erase(mSearchID);
}

// virtual
void LLPanelDirBrowser::draw()
{
    // HACK: If the results panel has data, we want to select the first
    // item.  Unfortunately, we don't know when the find is actually done,
    // so only do this if it's been some time since the last packet of
    // results was received.
    if (mLastResultTimer.getElapsedTimeF32() > 0.5)
    {
        if (!mDidAutoSelect &&
            !childHasFocus("results"))
        {
            LLCtrlListInterface *list = childGetListInterface("results");
            if (list)
            {
                if (list->getCanSelect())
                {
                    list->selectFirstItem(); // select first item by default
                    childSetFocus("results", TRUE);
                }
                // Request specific data from the server
                onCommitList(NULL, this);
            }
        }
        mDidAutoSelect = TRUE;
    }

    LLPanel::draw();
}


// virtual
void LLPanelDirBrowser::nextPage()
{
    mSearchStart += mResultsPerPage;
    childSetVisible("< Prev", true);

    performQuery();
}


// virtual
void LLPanelDirBrowser::prevPage()
{
    mSearchStart -= mResultsPerPage;
    childSetVisible("< Prev", mSearchStart > 0);

    performQuery();
}


void LLPanelDirBrowser::resetSearchStart()
{
    mSearchStart = 0;
    childSetVisible("Next >", false);
    childSetVisible("< Prev", false);
}

// protected
void LLPanelDirBrowser::updateResultCount()
{
    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("results");

    S32 result_count = list->getItemCount();
    std::string result_text;

    if (!mHaveSearchResults) result_count = 0;

    LLView* viewp = getChild<LLView>("Next >");
    if (viewp && viewp->getVisible()) 
    {
        // Item count be off by a few if bogus items sent from database
        // Just use the number of results per page. JC
        result_text = llformat(">%d found", mResultsPerPage);
    }
    else 
    {
        result_text = llformat("%d found", result_count);
    }

    childSetValue("result_text", result_text);

    if (result_count == 0)
    {
        // add none found response
        if (list->getItemCount() == 0)
        {
            list->setCommentText(std::string("None found.")); // *TODO: Translate
            list->operateOnAll(LLCtrlListInterface::OP_DESELECT);
        }
    }
    else
    {
        childEnable("results");
    }
}

// static
void LLPanelDirBrowser::onClickPrev(void* data)
{
    LLPanelDirBrowser* self = (LLPanelDirBrowser*)data;
    self->prevPage();
}


// static
void LLPanelDirBrowser::onClickNext(void* data)
{
    LLPanelDirBrowser* self = (LLPanelDirBrowser*)data;
    self->nextPage();
}

// static
std::string LLPanelDirBrowser::filterShortWords(const std::string source_string, int shortest_word_length, bool& was_filtered)
{
    // degenerate case
    if ( source_string.length() < 1 ) 
        return "";

    std::stringstream codec( source_string );
    std::string each_word;
    std::vector< std::string > all_words;
    
    while( codec >> each_word )
        all_words.push_back( each_word );

    std::ostringstream dest_string( "" ); 

    was_filtered = false;

    std::vector< std::string >::iterator iter = all_words.begin();
    while( iter != all_words.end() )
    {
        if ( (int)(*iter).length() >= shortest_word_length )
        {
            dest_string << *iter;
            dest_string << " ";
        }
        else
        {
            was_filtered = true;
        }

        ++iter;
    };

    return dest_string.str();
}

void LLPanelDirBrowser::updateMaturityCheckbox()
{
    BOOL godlike = gAgent.isGodlike();
    // You only have a choice if your maturity is 'mature' or higher.
    // Logic: if you're not at least mature, hide the mature and adult options
    // After that, enable only the options you can legitimately choose.
    // If you're PG only, show you the checkbox but don't let you change it.
    // If you're God, you have everything.
    bool mature_enabled = gAgent.canAccessMature() || godlike;
    bool adult_enabled = gAgent.canAccessAdult() || godlike;

    // TODO: fix maturity settings
    // These check boxes can only be checked if you have the right access to use them
    //std::string control_name_pg = getChild<LLCheckBoxCtrl>("incpg")->getControlName();
    //std::string control_name_mature = getChild<LLCheckBoxCtrl>("incmature")->getControlName();
    //std::string control_name_adult = getChild<LLCheckBoxCtrl>("incadult")->getControlName();

    //childSetValue("incpg", gSavedSettings.getBOOL(control_name_pg));
    //childSetValue("incmature", gSavedSettings.getBOOL(control_name_mature) && mature_enabled);
    //childSetValue("incadult", gSavedSettings.getBOOL(control_name_adult) && adult_enabled);

    childSetValue("incpg", true);
    childSetValue("incmature", true);
    childSetValue("incadult", true);

    // Teens don't get mature/adult choices
    if (gAgent.wantsPGOnly())
    {
        childSetVisible("incmature", false);
        childSetVisible("incadult", false);
        childSetValue("incpg", TRUE);
        childDisable("incpg");
    }

    childSetEnabled("incmature", mature_enabled);		
    childSetEnabled("incadult", adult_enabled);

    if (mature_enabled)
    {
        childEnable("incpg");
        childSetVisible("incpg", TRUE);
        childSetVisible("incmature", TRUE);
        childSetVisible("incadult", TRUE);
    }
}

void LLPanelDirBrowser::selectByUUID(const LLUUID& id)
{
    LLCtrlListInterface *list = childGetListInterface("results");
    if (!list) return;
    BOOL found = list->setCurrentByID(id);
    if (found)
    {
        // we got it, don't wait for network
        // Don't bother looking for this in the draw loop.
        mWantSelectID.setNull();
        // Make sure UI updates.
        onCommitList(NULL, this);
    }
    else
    {
        // waiting for this item from the network
        mWantSelectID = id;
    }
}

void LLPanelDirBrowser::selectEventByID(S32 event_id)
{
    if (mFloaterDirectory)
    {
        if (mFloaterDirectory->mPanelEventp)
        {
            mFloaterDirectory->mPanelEventp->setVisible(true);
            mFloaterDirectory->mPanelEventp->setEventID(event_id);
        }
    }
}

void LLPanelDirBrowser::getSelectedInfo(LLUUID* id, S32 *type)
{
    LLCtrlListInterface *list = childGetListInterface("results");
    if (!list) return;

    LLSD id_sd = childGetValue("results");

    *id = id_sd.asUUID();

    std::string id_str = id_sd.asString();
    *type = mResultsContents[id_str]["type"];
}


// static
void LLPanelDirBrowser::onCommitList(LLUICtrl* ctrl, void* data)
{
    LLPanelDirBrowser* self = (LLPanelDirBrowser*)data;
    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    // Start with everyone invisible
    if (self->mFloaterDirectory)
    {
        self->mFloaterDirectory->hideAllDetailPanels();
    }

    if (FALSE == list->getCanSelect())
    {
        return;
    }

    std::string id_str = self->childGetValue("results").asString();
    if (id_str.empty())
    {
        return;
    }

    LLSD item_id = list->getCurrentID();
    S32 type = self->mResultsContents[id_str]["type"];
    if (type == EVENT_CODE)
    {
        // all but events use the UUID above
        item_id = self->mResultsContents[id_str]["event_id"];
    }
    //std::string name = self->mResultsContents[id_str]["name"].asString();
    self->showDetailPanel(type, item_id);
}

void LLPanelDirBrowser::showDetailPanel(S32 type, LLSD id)
{
    
    switch(type)
    {
    case AVATAR_CODE:
        if (mFloaterDirectory && mFloaterDirectory->mPanelAvatarp)
        {
            mFloaterDirectory->mPanelAvatarp->setVisible(true);
            mFloaterDirectory->mPanelAvatarp->onOpen(id);
            mFloaterDirectory->mPanelAvatarp->updateData();
        }
        break;
    case GROUP_CODE:
        if (mFloaterDirectory && mFloaterDirectory->mPanelGroupp)
        {
            mFloaterDirectory->mPanelGroupp->setVisible(true);
            mFloaterDirectory->mPanelGroupp->onOpen(LLSD().with("group_id", id));
        }
        break;
    case PLACE_CODE:
    case FOR_SALE_CODE:
    case AUCTION_CODE:
        if (mFloaterDirectory && mFloaterDirectory->mPanelPlacep)
        {
            mFloaterDirectory->mPanelPlacep->setVisible(true);
            LLSD key;
            key["type"] = "remote_place";
            key["id"]= id;
            mFloaterDirectory->mPanelPlacep->onOpen(key);
        }
        break;
    case CLASSIFIED_CODE:
        if (mFloaterDirectory && mFloaterDirectory->mPanelClassifiedp)
        {
            mFloaterDirectory->mPanelClassifiedp->setVisible(true);
            LLSD key;
            key["classified_id"] = id;
            key["from_search"] = true;
            mFloaterDirectory->mPanelClassifiedp->onOpen(key);
        }
        break;
    case EVENT_CODE:
        {
            U32 event_id = (U32)id.asInteger();
            showEvent(event_id);
        }
        break;
    default:
        {
            LL_WARNS() << "Unknown event type!" << LL_ENDL;
        }
        break;
    }
 
}


void LLPanelDirBrowser::showEvent(const U32 event_id)
{
    // Start with everyone invisible
    if (mFloaterDirectory)
    {
        mFloaterDirectory->hideAllDetailPanels();
        if (mFloaterDirectory->mPanelEventp)
        {
            mFloaterDirectory->mPanelEventp->setVisible(true);
            mFloaterDirectory->mPanelEventp->setEventID(event_id);
        }
    }
}

void LLPanelDirBrowser::processDirPeopleReply(LLMessageSystem *msg, void**)
{
    LLUUID query_id;
    std::string first_name;
    std::string last_name;
    LLUUID agent_id;

    msg->getUUIDFast(_PREHASH_QueryData,_PREHASH_QueryID, query_id);

    LLPanelDirBrowser* self = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!self) 
    {
        // data from an old query
        return;
    }

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    S32 rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
    self->mResultsReceived += rows;

    rows = self->showNextButton(rows);

    for (S32 i = 0; i < rows; i++)
    {
        msg->getStringFast(_PREHASH_QueryReplies,_PREHASH_FirstName, first_name, i);
        msg->getStringFast(_PREHASH_QueryReplies,_PREHASH_LastName,	last_name, i);
        msg->getUUIDFast(  _PREHASH_QueryReplies,_PREHASH_AgentID, agent_id, i);
        // msg->getU8Fast(    _PREHASH_QueryReplies,_PREHASH_Online, online, i);
        // unused
        // msg->getStringFast(_PREHASH_QueryReplies,_PREHASH_Group, group, i);
        // msg->getS32Fast(   _PREHASH_QueryReplies,_PREHASH_Reputation, reputation, i);

        if (agent_id.isNull())
        {
            continue;
        }

        LLSD content;

        LLSD row;
        row["id"] = agent_id;

        // We don't show online status in the finder anymore,
        // so just use the 'offline' icon as the generic 'person' icon
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = "icon_avatar_offline.tga";

        content["type"] = AVATAR_CODE;

        std::string fullname = first_name + " " + last_name;
        row["columns"][1]["column"] = "name";
        row["columns"][1]["value"] = fullname;
        row["columns"][1]["font"] = "SANSSERIF";

        content["name"] = fullname;

        list->addElement(row);
        self->mResultsContents[agent_id.asString()] = content;
    }

    list->sortByColumn(self->mCurrentSortColumn, self->mCurrentSortAscending);
    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}


void LLPanelDirBrowser::processDirPlacesReply(LLMessageSystem* msg, void**)
{
    LLUUID	agent_id;
    LLUUID	query_id;
    LLUUID	parcel_id;
    std::string	name;
    bool is_for_sale = false;
    bool is_auction = false;
    F32 dwell;

    msg->getUUID("AgentData", "AgentID", agent_id);
    msg->getUUID("QueryData", "QueryID", query_id );

    if (msg->getNumberOfBlocks("StatusData"))
    {
        U32 status;
        msg->getU32("StatusData", "Status", status);
        if (status & STATUS_SEARCH_PLACES_BANNEDWORD)
        {
            LLNotificationsUtil::add("SearchWordBanned");
        }
    }

    LLPanelDirBrowser* self = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!self) 
    {
        // data from an old query
        return;
    }

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    S32 count = msg->getNumberOfBlocks("QueryReplies");
    self->mResultsReceived += count;

    count = self->showNextButton(count);

    for (S32 i = 0; i < count ; i++)
    {
        msg->getUUID("QueryReplies", "ParcelID", parcel_id, i);
        msg->getString("QueryReplies", "Name", name, i);
        msg->getBOOL("QueryReplies", "ForSale", is_for_sale, i);
        msg->getBOOL("QueryReplies", "Auction", is_auction, i);
        msg->getF32("QueryReplies", "Dwell", dwell, i);

        if (parcel_id.isNull())
        {
            continue;
        }

        LLSD content;
        S32 type;

        LLSD row = self->createLandSale(parcel_id, is_auction, is_for_sale, name, &type);

        content["type"] = type;
        content["name"] = name;

        std::string buffer = llformat("%.0f", (F64)dwell);
        row["columns"][3]["column"] = "dwell";
        row["columns"][3]["value"] = buffer;
        row["columns"][3]["font"] = "SANSSERIFSMALL";

        list->addElement(row);
        self->mResultsContents[parcel_id.asString()] = content;
    }

    list->sortByColumn(self->mCurrentSortColumn, self->mCurrentSortAscending);
    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}


void LLPanelDirBrowser::processDirEventsReply(LLMessageSystem* msg, void**)
{
    LLUUID agent_id;
    LLUUID query_id;
    LLUUID owner_id;
    std::string	name;
    std::string	date;
    bool show_pg = gSavedSettings.getBOOL("ShowPGEvents");
    bool show_mature = gSavedSettings.getBOOL("ShowMatureEvents");
    bool show_adult = gSavedSettings.getBOOL("ShowAdultEvents");

    msg->getUUID("AgentData", "AgentID", agent_id);
    msg->getUUID("QueryData", "QueryID", query_id );

    LLPanelDirBrowser* self = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!self)
    {
        return;
    }

    if (msg->getNumberOfBlocks("StatusData"))
    {
        U32 status;
        msg->getU32("StatusData", "Status", status);
        if (status & STATUS_SEARCH_EVENTS_BANNEDWORD)
        {
            LLNotificationsUtil::add("SearchWordBanned");
        }
    }

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    S32 rows = msg->getNumberOfBlocks("QueryReplies");
    self->mResultsReceived += rows;

    rows = self->showNextButton(rows);

    for (S32 i = 0; i < rows; i++)
    {
        U32 event_id;
        U32 unix_time;
        U32 event_flags;

        msg->getUUID("QueryReplies", "OwnerID", owner_id, i);
        msg->getString("QueryReplies", "Name", name, i);
        msg->getU32("QueryReplies", "EventID", event_id, i);
        msg->getString("QueryReplies", "Date", date, i);
        msg->getU32("QueryReplies", "UnixTime", unix_time, i);
        msg->getU32("QueryReplies", "EventFlags", event_flags, i);

        // Skip empty events
        if (owner_id.isNull())
        {
            //RN: should this check event_id instead?
            LL_WARNS() << "skipped event due to owner_id null, event_id " << event_id << LL_ENDL;
            continue;
        }

        // skip events that don't match the flags
        // there's no PG flag, so we make sure neither adult nor mature is set
        if (((event_flags & (EVENT_FLAG_ADULT | EVENT_FLAG_MATURE)) == EVENT_FLAG_NONE) && !show_pg)
        {
            //llwarns << "Skipped pg event because we're not showing pg, event_id " << event_id << llendl;
            continue;
        }

        if ((event_flags & EVENT_FLAG_MATURE) && !show_mature)
        {
            //llwarns << "Skipped mature event because we're not showing mature, event_id " << event_id << llendl;
            continue;
        }

        if ((event_flags & EVENT_FLAG_ADULT) && !show_adult)
        {
            //llwarns << "Skipped adult event because we're not showing adult, event_id " << event_id << llendl;
            continue;
        }

        LLSD content;

        content["type"] = EVENT_CODE;
        content["name"] = name;
        content["event_id"] = (S32)event_id;

        LLSD row;
        row["id"] = llformat("%u", event_id);

        // Column 0 - event icon
        LLUUID image_id;
        if (event_flags == EVENT_FLAG_ADULT)
        {
            row["columns"][0]["column"] = "icon";
            row["columns"][0]["type"] = "icon";
            row["columns"][0]["value"] = "icon_event_adult.tga";
        }
        else if (event_flags == EVENT_FLAG_MATURE)
        {
            row["columns"][0]["column"] = "icon";
            row["columns"][0]["type"] = "icon";
            row["columns"][0]["value"] = "icon_event_mature.tga";
        }
        else
        {
            row["columns"][0]["column"] = "icon";
            row["columns"][0]["type"] = "icon";
            row["columns"][0]["value"] = "icon_event.tga";
        }

        row["columns"][1]["column"] = "name";
        row["columns"][1]["value"] = name;
        row["columns"][1]["font"] = "SANSSERIF";

        row["columns"][2]["column"] = "date";
        row["columns"][2]["value"] = date;
        row["columns"][2]["font"] = "SANSSERIFSMALL";

        row["columns"][3]["column"] = "time";
        row["columns"][3]["value"] = llformat("%u", unix_time);
        row["columns"][3]["font"] = "SANSSERIFSMALL";

        list->addElement(row, ADD_TOP /*ADD_SORTED*/);

        std::string id_str = llformat("%u", event_id);
        self->mResultsContents[id_str] = content;
    }

    list->sortByColumn(self->mCurrentSortColumn, self->mCurrentSortAscending);
    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}


// static
void LLPanelDirBrowser::processDirGroupsReply(LLMessageSystem* msg, void**)
{
    S32 i;

    LLUUID query_id;
    LLUUID group_id;
    std::string	group_name;
    S32 members;
    F32 search_order;

    msg->getUUIDFast(_PREHASH_QueryData,_PREHASH_QueryID, query_id );

    LLPanelDirBrowser* self = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!self)
    {
        return;
    }

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    S32 rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
    self->mResultsReceived += rows;

    rows = self->showNextButton(rows);

    for (i = 0; i < rows; i++)
    {
        msg->getUUIDFast(_PREHASH_QueryReplies, _PREHASH_GroupID,		group_id, i );
        msg->getStringFast(_PREHASH_QueryReplies, _PREHASH_GroupName,	group_name,		i);
        msg->getS32Fast(_PREHASH_QueryReplies, _PREHASH_Members,		members, i );
        msg->getF32Fast(_PREHASH_QueryReplies, _PREHASH_SearchOrder,	search_order, i );

        if (group_id.isNull())
        {
            continue;
        }

        LLSD content;
        content["type"] = GROUP_CODE;
        content["name"] = group_name;

        LLSD row;
        row["id"] = group_id;

        LLUUID image_id;
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = "icon_group.tga";

        row["columns"][1]["column"] = "name";
        row["columns"][1]["value"] = group_name;
        row["columns"][1]["font"] = "SANSSERIF";

        row["columns"][2]["column"] = "members";
        row["columns"][2]["value"] = members;
        row["columns"][2]["font"] = "SANSSERIFSMALL";

        row["columns"][3]["column"] = "score";
        row["columns"][3]["value"] = search_order;

        list->addElement(row);
        self->mResultsContents[group_id.asString()] = content;
    }
    list->sortByColumn(self->mCurrentSortColumn, self->mCurrentSortAscending);
    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}


// static
void LLPanelDirBrowser::processDirClassifiedReply(LLMessageSystem* msg, void**)
{
    S32 i;
    S32 num_new_rows;

    LLUUID agent_id;
    LLUUID query_id;

    msg->getUUID("AgentData", "AgentID", agent_id);
    if (agent_id != gAgent.getID())
    {
        LL_WARNS() << "Message for wrong agent " << agent_id
            << " in processDirClassifiedReply" << LL_ENDL;
        return;
    }

    msg->getUUID("QueryData", "QueryID", query_id);
    LLPanelDirBrowser* self = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!self)
    {
        return;
    }

    if (msg->getNumberOfBlocks("StatusData"))
    {
        U32 status;
        msg->getU32("StatusData", "Status", status);
        if (status & STATUS_SEARCH_CLASSIFIEDS_BANNEDWORD)
        {
            LLNotificationsUtil::add("SearchWordBanned");
        }
    }

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    num_new_rows = msg->getNumberOfBlocksFast(_PREHASH_QueryReplies);
    self->mResultsReceived += num_new_rows;

    num_new_rows = self->showNextButton(num_new_rows);
    for (i = 0; i < num_new_rows; i++)
    {
        LLUUID classified_id;
        std::string name;
        U32 creation_date = 0;	// unix timestamp
        U32 expiration_date = 0;	// future use
        S32 price_for_listing = 0;
        msg->getUUID("QueryReplies", "ClassifiedID", classified_id, i);
        msg->getString("QueryReplies", "Name", name, i);
        msg->getU32("QueryReplies","CreationDate",creation_date,i);
        msg->getU32("QueryReplies","ExpirationDate",expiration_date,i);
        msg->getS32("QueryReplies","PriceForListing",price_for_listing,i);

        if ( classified_id.notNull() )
        {
            self->addClassified(list, classified_id, name, creation_date, price_for_listing);

            LLSD content;
            content["type"] = CLASSIFIED_CODE;
            content["name"] = name;
            self->mResultsContents[classified_id.asString()] = content;
        }
    }
    // The server does the initial sort, by price paid per listing and date. JC
    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}

void LLPanelDirBrowser::processDirLandReply(LLMessageSystem *msg, void**)
{
    LLUUID agent_id;
    LLUUID query_id;
    LLUUID parcel_id;
    std::string	name;
    std::string land_sku;
    std::string land_type;
    bool auction = false;
    bool for_sale = false;
    S32 sale_price;
    S32 actual_area;

    msg->getUUID("AgentData", "AgentID", agent_id);
    msg->getUUID("QueryData", "QueryID", query_id );

    LLPanelDirBrowser* browser = get_if_there(gDirBrowserInstances, query_id, (LLPanelDirBrowser*)NULL);
    if (!browser) 
    {
        // data from an old query
        return;
    }

    // Only handled by LLPanelDirLand
    LLPanelDirLand* self = (LLPanelDirLand*)browser;

    self->mHaveSearchResults = TRUE;

    LLCtrlListInterface *list = self->childGetListInterface("results");
    if (!list) return;

    if (!list->getCanSelect())
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
        self->mResultsContents = LLSD();
    }

    BOOL use_price = gSavedSettings.getBOOL("FindLandPrice");
    S32 limit_price = self->childGetValue("priceedit").asInteger();

    BOOL use_area = gSavedSettings.getBOOL("FindLandArea");
    S32 limit_area = self->childGetValue("areaedit").asInteger();

    S32 i;
    S32 count = msg->getNumberOfBlocks("QueryReplies");
    self->mResultsReceived += count;

    S32 non_auction_count = 0;
    for (i = 0; i < count; i++)
    {
        msg->getUUID("QueryReplies", "ParcelID", parcel_id, i);
        msg->getString("QueryReplies", "Name", name, i);
        msg->getBOOL("QueryReplies", "Auction", auction, i);
        msg->getBOOL("QueryReplies", "ForSale", for_sale, i);
        msg->getS32("QueryReplies", "SalePrice", sale_price, i);
        msg->getS32("QueryReplies", "ActualArea", actual_area, i);

        if ( msg->getSizeFast(_PREHASH_QueryReplies, i, _PREHASH_ProductSKU) > 0 )
        {
            msg->getStringFast(_PREHASH_QueryReplies, _PREHASH_ProductSKU, land_sku, i);
            LL_INFOS() << "Land sku: " << land_sku << LL_ENDL;
            land_type = LLProductInfoRequestManager::instance().getDescriptionForSku(land_sku);
        }
        else
        {
            land_sku.clear();
            land_type = LLTrans::getString("land_type_unknown");
        }

        if (parcel_id.isNull()) continue;

        if (use_price && (sale_price > limit_price)) continue;

        if (use_area && (actual_area < limit_area)) continue;

        LLSD content;
        S32 type;

        LLSD row = self->createLandSale(parcel_id, auction, for_sale,  name, &type);

        content["type"] = type;
        content["name"] = name;
        content["landtype"] = land_type;

        std::string buffer = "Auction";
        if (!auction)
        {
            buffer = llformat("%d", sale_price);
            non_auction_count++;
        }
        row["columns"][3]["column"] = "price";
        row["columns"][3]["value"] = buffer;
        row["columns"][3]["font"] = "SANSSERIFSMALL";

        buffer = llformat("%d", actual_area);
        row["columns"][4]["column"] = "area";
        row["columns"][4]["value"] = buffer;
        row["columns"][4]["font"] = "SANSSERIFSMALL";

        if (!auction)
        {
            F32 price_per_meter;
            if (actual_area > 0)
            {
                price_per_meter = (F32)sale_price / (F32)actual_area;
            }
            else
            {
                price_per_meter = 0.f;
            }
            // Prices are usually L$1 - L$10 / meter
            buffer = llformat("%.1f", price_per_meter);
            row["columns"][5]["column"] = "per_meter";
            row["columns"][5]["value"] = buffer;
            row["columns"][5]["font"] = "SANSSERIFSMALL";
        }
        else
        {
            // Auctions start at L$1 per meter
            row["columns"][5]["column"] = "per_meter";
            row["columns"][5]["value"] = "1.0";
            row["columns"][5]["font"] = "SANSSERIFSMALL";
        }

        row["columns"][6]["column"] = "landtype";
        row["columns"][6]["value"] = land_type;
        row["columns"][6]["font"] = "SANSSERIFSMALL";

        list->addElement(row);
        self->mResultsContents[parcel_id.asString()] = content;
    }

    // All auction results are shown on the first page
    // But they don't count towards the 100 / page limit
    // So figure out the next button here, when we know how many aren't auctions
    count = self->showNextButton(non_auction_count);

    self->updateResultCount();

    // Poke the result received timer
    self->mLastResultTimer.reset();
    self->mDidAutoSelect = FALSE;
}

void LLPanelDirBrowser::addClassified(LLCtrlListInterface *list, const LLUUID& pick_id, const std::string& name, const U32 creation_date, const S32 price_for_listing)
{
    std::string type = llformat("%d", CLASSIFIED_CODE);

    LLSD row;
    row["id"] = pick_id;

    row["columns"][0]["column"] = "icon";
    row["columns"][0]["type"] = "icon";
    row["columns"][0]["value"] = "icon_top_pick.tga";

    row["columns"][1]["column"] = "name";
    row["columns"][1]["value"] = name;
    row["columns"][1]["font"] = "SANSSERIF";

    row["columns"][2]["column"] = "price";
    row["columns"][2]["value"] = price_for_listing;
    row["columns"][2]["font"] = "SANSSERIFSMALL";

    list->addElement(row);
}

LLSD LLPanelDirBrowser::createLandSale(const LLUUID& parcel_id, BOOL is_auction, BOOL is_for_sale,  const std::string& name, S32 *type)
{
    LLSD row;
    row["id"] = parcel_id;
    LLUUID image_id;

    // Icon and type
    if(is_auction)
    {
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = "icon_auction.tga";

        *type = AUCTION_CODE;
    }
    else if (is_for_sale)
    {
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = "icon_for_sale.tga";

        *type = FOR_SALE_CODE;
    }
    else
    {
        row["columns"][0]["column"] = "icon";
        row["columns"][0]["type"] = "icon";
        row["columns"][0]["value"] = "icon_place.tga";

        *type = PLACE_CODE;
    }

    row["columns"][2]["column"] = "name";
    row["columns"][2]["value"] = name;
    row["columns"][2]["font"] = "SANSSERIF";

    return row;
}

void LLPanelDirBrowser::setupNewSearch()
{
    LLScrollListCtrl* list = getChild<LLScrollListCtrl>("results");

    gDirBrowserInstances.erase(mSearchID);
    // Make a new query ID
    mSearchID.generate();

    gDirBrowserInstances.emplace(mSearchID, this);

    // ready the list for results
    list->operateOnAll(LLCtrlListInterface::OP_DELETE);
    list->setCommentText(std::string("Searching...")); // *TODO: Translate
    childDisable("results");

    mResultsReceived = 0;
    mHaveSearchResults = FALSE;

    // Set all panels to be invisible
    mFloaterDirectory->hideAllDetailPanels();

    updateResultCount();
}


// static
// called from calssifieds, events, groups, land, people, and places
void LLPanelDirBrowser::onClickSearchCore(void* userdata)
{
    LLPanelDirBrowser* self = (LLPanelDirBrowser*)userdata;
    if (!self) return;

    self->resetSearchStart();
    self->performQuery();
}


// static
void LLPanelDirBrowser::sendDirFindQuery(
    LLMessageSystem* msg,
    const LLUUID& query_id,
    const std::string& text,
    U32 flags,
    S32 query_start)
{
    msg->newMessage("DirFindQuery");
    msg->nextBlock("AgentData");
    msg->addUUID("AgentID", gAgent.getID());
    msg->addUUID("SessionID", gAgent.getSessionID());
    msg->nextBlock("QueryData");
    msg->addUUID("QueryID", query_id);
    msg->addString("QueryText", text);
    msg->addU32("QueryFlags", flags);
    msg->addS32("QueryStart", query_start);
    gAgent.sendReliableMessage();
}


void LLPanelDirBrowser::onKeystrokeName(LLLineEditor* line, void* data)
{
    LLPanelDirBrowser *self = (LLPanelDirBrowser*)data;
    if (line->getLength() >= (S32)self->mMinSearchChars)
    {
        self->setDefaultBtn( "Search" );
        self->childEnable("Search");
    }
    else
    {
        self->setDefaultBtn();
        self->childDisable("Search");
    }
}

// setup results when shown
void LLPanelDirBrowser::onVisibilityChange(BOOL new_visibility)
{
    if (new_visibility)
    {
        onCommitList(NULL, this);
    }
    LLPanel::onVisibilityChange(new_visibility);
}

S32 LLPanelDirBrowser::showNextButton(S32 rows)
{
    // HACK: This hack doesn't work for llpaneldirfind (ALL) 
    // because other data is being returned as well.
    if ( getName() != "find_all_old_panel")
    {
        // HACK: The (mResultsPerPage)+1th entry indicates there are 'more'
        bool show_next = (mResultsReceived > mResultsPerPage);
        childSetVisible("Next >", show_next);
        if (show_next)
        {
            rows -= (mResultsReceived - mResultsPerPage);
        }
    }
    else
    {
        // Hide page buttons
        childSetVisible("Next >", false);
        childSetVisible("< Prev", false);
    }
    return rows;
}
