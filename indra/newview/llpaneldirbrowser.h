/**
 * @file llpaneldirbrowser.h
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

#ifndef LL_LLPANELDIRBROWSER_H
#define LL_LLPANELDIRBROWSER_H

#include "llpanel.h"

#include "llframetimer.h"

class LLMessageSystem;
class LLFloaterDirectory;
class LLLineEditor;

class LLPanelDirBrowser: public LLPanel
{
public:
    LLPanelDirBrowser();
    virtual ~LLPanelDirBrowser();

    bool postBuild() override;
    void setFloaterDirectory(LLFloaterDirectory* floater) { mFloaterDirectory = floater; }

    // Use to get periodic updates.
    void draw() override;

    void onVisibilityChange(bool curVisibilityIn) override;

    // Redo your search for the prev/next page of results
    virtual void prevPage();
    virtual void nextPage();
    void resetSearchStart();
    // Do the current query (used by prevPage/nextPage)
    virtual void performQuery() {};

    const LLUUID& getSearchID() const { return mSearchID; }

    // Select the line in the scroll list control with this ID,
    // either now or when data arrives from the server.
    void selectByUUID(const LLUUID& id);

    void selectEventByID(S32 event_id);

    void getSelectedInfo(LLUUID* id, S32 *type);

    void showDetailPanel(S32 type, LLSD item_id);
        // type is EVENT_CODE, PLACE_CODE, etc. from below.
        // item_id is integer for events, UUID for all others.

    // from llpaneldirbase
    void setupNewSearch();

    // default handler for clicking the search button resets the
    // next/prev state and performs the query.
    // Expects a pointer to an LLPanelDirBrowser object.
    static void onClickSearchCore(void* userdata);

    // query_start indicates the first result row to
    // return, usually 0 or 100 or 200 because the searches
    // return a max of 100 rows
    static void sendDirFindQuery(
        LLMessageSystem* msg,
        const LLUUID& query_id,
        const std::string& text,
        U32 flags,
        S32 query_start);

    void showEvent(const U32 event_id);

    static void onCommitList(LLUICtrl* ctrl, void* data);

    static void processDirPeopleReply(LLMessageSystem* msg, void**);
    static void processDirPlacesReply(LLMessageSystem* msg, void**);
    static void processDirEventsReply(LLMessageSystem* msg, void**);
    static void processDirGroupsReply(LLMessageSystem* msg, void**);
    static void processDirClassifiedReply(LLMessageSystem* msg, void**);
    static void processDirLandReply(LLMessageSystem *msg, void**);

    std::string filterShortWords( const std::string source_string, int shortest_word_length, bool& was_filtered );

protected:
    void updateResultCount();

    void addClassified(LLCtrlListInterface *list, const LLUUID& classified_id, const std::string& name, const U32 creation_date, const S32 price_for_listing);
    LLSD createLandSale(const LLUUID& parcel_id, bool is_auction, bool is_for_sale, const std::string& name, S32 *type);

    static void onKeystrokeName(LLLineEditor* line, void* data);

    // If this is a search for a panel like "people_panel" (and not the "all" panel)
    // optionally show the "Next" button.  Return the actual number of
    // rows to display.
    S32 showNextButton(S32 rows);

protected:
    LLUUID mSearchID; // Unique ID for a pending search
    LLUUID mWantSelectID; // scroll item to select on arrival
    std::string mCurrentSortColumn;
    bool mCurrentSortAscending;
    // Some searches return a max of 100 items per page, so we can
    // start the search from the 100th item rather than the 0th, etc.
    S32 mSearchStart;
    // Places is 100 per page, events is 200 per page
    S32 mResultsPerPage;
    S32 mResultsReceived;

    U32 mMinSearchChars;

    LLSD mResultsContents;

    bool mHaveSearchResults;
    bool mDidAutoSelect;
    LLFrameTimer mLastResultTimer;

    LLFloaterDirectory* mFloaterDirectory;
    LLButton* mPrevPageBtn;
    LLButton* mNextPageBtn;
};

constexpr S32 RESULTS_PER_PAGE_DEFAULT = 100;
constexpr S32 RESULTS_PER_PAGE_EVENTS = 200;

// Codes used for sorting by type.
const S32 INVALID_CODE = -1;
const S32 EVENT_CODE = 0;
const S32 PLACE_CODE = 1;
// We no longer show online vs. offline in search result icons.
//const S32 ONLINE_CODE = 2;
//const S32 OFFLINE_CODE = 3;
const S32 AVATAR_CODE = 3;
const S32 GROUP_CODE = 4;
const S32 CLASSIFIED_CODE = 5;
const S32 FOR_SALE_CODE = 6;    // for sale place
const S32 AUCTION_CODE = 7;     // for auction place
const S32 POPULAR_CODE = 8;     // popular by dwell

// mask values for search flags
const S32 SEARCH_NONE = 0;	// should try not to send this to the search engine
const S32 SEARCH_PG = 1;
const S32 SEARCH_MATURE = 2;
const S32 SEARCH_ADULT = 4;

extern std::map<LLUUID, LLPanelDirBrowser*> gDirBrowserInstances;

#endif // LL_LLPANELDIRBROWSER_H
