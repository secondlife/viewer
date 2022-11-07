/** 
 * @file llmarketplacefunctions.h
 * @brief Miscellaneous marketplace-related functions and classes
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLMARKETPLACEFUNCTIONS_H
#define LL_LLMARKETPLACEFUNCTIONS_H


#include <llsd.h>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llsingleton.h"
#include "llstring.h"



namespace MarketplaceErrorCodes
{
    enum eCode
    {
        IMPORT_DONE = 200,
        IMPORT_PROCESSING = 202,
        IMPORT_REDIRECT = 302,
        IMPORT_BAD_REQUEST = 400,
        IMPORT_AUTHENTICATION_ERROR = 401,
        IMPORT_FORBIDDEN = 403,
        IMPORT_NOT_FOUND = 404,
        IMPORT_DONE_WITH_ERRORS = 409,
        IMPORT_JOB_FAILED = 410,
        IMPORT_JOB_TIMEOUT = 499,
        IMPORT_SERVER_SITE_DOWN = 500,
        IMPORT_SERVER_API_DISABLED = 503,
    };
}

namespace MarketplaceStatusCodes
{
    enum sCode
    {
        MARKET_PLACE_NOT_INITIALIZED = 0,
        MARKET_PLACE_INITIALIZING = 1,
        MARKET_PLACE_CONNECTION_FAILURE = 2,
        MARKET_PLACE_NOT_MERCHANT = 3,
        MARKET_PLACE_MERCHANT = 4,
        MARKET_PLACE_NOT_MIGRATED_MERCHANT = 5,
        MARKET_PLACE_MIGRATED_MERCHANT = 6
    };
}

namespace MarketplaceFetchCodes
{
    enum sCode
    {
        MARKET_FETCH_NOT_DONE = 0,
        MARKET_FETCH_LOADING = 1,
        MARKET_FETCH_FAILED = 2,
        MARKET_FETCH_DONE = 3
    };
}


class LLMarketplaceInventoryImporter
    : public LLSingleton<LLMarketplaceInventoryImporter>
{
    LLSINGLETON(LLMarketplaceInventoryImporter);
public:
    static void update();

    typedef boost::signals2::signal<void (bool)> status_changed_signal_t;
    typedef boost::signals2::signal<void (U32, const LLSD&)> status_report_signal_t;

    boost::signals2::connection setInitializationErrorCallback(const status_report_signal_t::slot_type& cb);
    boost::signals2::connection setStatusChangedCallback(const status_changed_signal_t::slot_type& cb);
    boost::signals2::connection setStatusReportCallback(const status_report_signal_t::slot_type& cb);
    
    void initialize();
    bool triggerImport();
    bool isImportInProgress() const { return mImportInProgress; }
    bool isInitialized() const { return mInitialized; }
    U32 getMarketPlaceStatus() const { return mMarketPlaceStatus; }
    
protected:
    void reinitializeAndTriggerImport();
    void updateImport();
    
private:
    bool mAutoTriggerImport;
    bool mImportInProgress;
    bool mInitialized;
    U32  mMarketPlaceStatus;
    
    status_report_signal_t *    mErrorInitSignal;
    status_changed_signal_t *   mStatusChangedSignal;
    status_report_signal_t *    mStatusReportSignal;
};


// Classes handling the data coming from and going to the Marketplace SLM Server DB:
// * implement the Marketplace API
// * cache the current Marketplace data (tuples)
// * provide methods to get Marketplace data on any inventory item
// * set Marketplace data
// * signal Marketplace updates to inventory
namespace SLMErrorCodes
{
    enum eCode
    {
        SLM_SUCCESS = 200,
        SLM_RECORD_CREATED = 201,
        SLM_MALFORMED_PAYLOAD = 400,
        SLM_NOT_FOUND = 404,
    };
}

class LLMarketplaceData;
class LLInventoryObserver;

// A Marketplace item is known by its tuple
class LLMarketplaceTuple 
{
public:
    friend class LLMarketplaceData;

    LLMarketplaceTuple();
    LLMarketplaceTuple(const LLUUID& folder_id);
    LLMarketplaceTuple(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed = false);
    
private:
    // Representation of a marketplace item in the Marketplace DB (well, what we know of it...)
    LLUUID mListingFolderId;
    S32 mListingId;
    LLUUID mVersionFolderId;
    bool mIsActive;
    S32 mCountOnHand;
    std::string mEditURL;
};
// Notes:
// * The mListingFolderId is used as a key to this map. It could therefore be taken off the LLMarketplaceTuple objects themselves.
// * The SLM DB however uses mListingId as its primary key and it shows in its API. In the viewer though, the mListingFolderId is what we use to grab an inventory record.
typedef std::map<LLUUID, LLMarketplaceTuple> marketplace_items_list_t;
typedef std::map<LLUUID, LLUUID> version_folders_list_t;

// Session cache of all Marketplace tuples
// Notes:
// * There's one and only one possible set of Marketplace dataset per agent and per session thus making it an LLSingleton
// * Some of those records might correspond to folders that do not exist in the inventory anymore. We do not clear them out though. They just won't show up in the UI.

class LLSLMGetMerchantResponder;
class LLSLMGetListingsResponder;
class LLSLMCreateListingsResponder;
class LLSLMGetListingResponder;
class LLSLMUpdateListingsResponder;
class LLSLMAssociateListingsResponder;
class LLSLMDeleteListingsResponder;

class LLMarketplaceData
    : public LLSingleton<LLMarketplaceData>
{
    LLSINGLETON(LLMarketplaceData);
    virtual ~LLMarketplaceData();

public:
    friend class LLSLMGetMerchantResponder;
    friend class LLSLMGetListingsResponder;
    friend class LLSLMCreateListingsResponder;
    friend class LLSLMGetListingResponder;
    friend class LLSLMUpdateListingsResponder;
    friend class LLSLMAssociateListingsResponder;
    friend class LLSLMDeleteListingsResponder;

    static LLSD getMarketplaceStringSubstitutions();

    // Public SLM API : Initialization and status
    typedef boost::signals2::signal<void ()> status_updated_signal_t;
    void initializeSLM(const status_updated_signal_t::slot_type& cb);
    U32  getSLMStatus() const { return mMarketPlaceStatus; }
    std::string getSLMConnectionfailureReason() { return mMarketPlaceFailureReason; }
    void setSLMStatus(U32 status);
    void setSLMConnectionFailure(const std::string& reason);
    void getSLMListings();
    bool isEmpty() { return (mMarketplaceItems.size() == 0); }
    void setDataFetchedSignal(const status_updated_signal_t::slot_type& cb);
    void setSLMDataFetched(U32 status);
    U32 getSLMDataFetched() { return mMarketPlaceDataFetched; }

    bool isSLMDataFetched();

    // High level create/delete/set Marketplace data: each method returns true if the function succeeds, false if error
    bool createListing(const LLUUID& folder_id);
    bool activateListing(const LLUUID& folder_id, bool activate, S32 depth = -1);
    bool clearListing(const LLUUID& folder_id, S32 depth = -1);
    bool setVersionFolder(const LLUUID& folder_id, const LLUUID& version_id, S32 depth = -1);
    bool associateListing(const LLUUID& folder_id, const LLUUID& source_folder_id, S32 listing_id);
    bool updateCountOnHand(const LLUUID& folder_id, S32 depth = -1);
    bool getListing(const LLUUID& folder_id, S32 depth = -1);
    bool getListing(S32 listing_id);
    bool deleteListing(S32 listing_id, bool update = true);
    
    // Probe the Marketplace data set to identify folders
    bool isListed(const LLUUID& folder_id); // returns true if folder_id is a Listing folder
    bool isListedAndActive(const LLUUID& folder_id); // returns true if folder_id is an active (listed) Listing folder
    bool isVersionFolder(const LLUUID& folder_id); // returns true if folder_id is a Version folder
    bool isInActiveFolder(const LLUUID& obj_id, S32 depth = -1); // returns true if the obj_id is buried in an active version folder
    LLUUID getActiveFolder(const LLUUID& obj_id, S32 depth = -1); // returns the UUID of the active version folder obj_id is in
    bool isUpdating(const LLUUID& folder_id, S32 depth = -1); // returns true if we're waiting from SLM incoming data for folder_id
   
    // Access Marketplace data set : each method returns a default value if the argument can't be found
    bool getActivationState(const LLUUID& folder_id);
    S32 getListingID(const LLUUID& folder_id);
    LLUUID getVersionFolder(const LLUUID& folder_id);
    std::string getListingURL(const LLUUID& folder_id, S32 depth = -1);
    LLUUID getListingFolder(S32 listing_id);
    S32 getCountOnHand(const LLUUID& folder_id);
    
    // Used to flag if stock count values for Marketplace have to be updated
    bool checkDirtyCount() { if (mDirtyCount) { mDirtyCount = false; return true; } else { return false; } }
    void setDirtyCount() { mDirtyCount = true; }
    void setUpdating(const LLUUID& folder_id, bool isUpdating);
    
    // Used to decide when to run a validation on listing folders
    void setValidationWaiting(const LLUUID& folder_id, S32 count);
    void decrementValidationWaiting(const LLUUID& folder_id, S32 count = 1);

private:
    // Modify Marketplace data set  : each method returns true if the function succeeds, false if error
    // Used internally only by SLM Responders when data are received from the SLM Server
    bool addListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed, const std::string& edit_url, S32 count);
    bool deleteListing(const LLUUID& folder_id, bool update = true);
    bool setListingID(const LLUUID& folder_id, S32 listing_id, bool update = true);
    bool setVersionFolderID(const LLUUID& folder_id, const LLUUID& version_id, bool update = true);
    bool setActivationState(const LLUUID& folder_id, bool activate, bool update = true);
    bool setListingURL(const LLUUID& folder_id, const std::string& edit_url, bool update = true);
    bool setCountOnHand(const LLUUID& folder_id, S32 count, bool update = true);
    
    // Private SLM API : package data and get/post/put requests to the SLM Server through the SLM API
    void createSLMListing(const LLUUID& folder_id, const LLUUID& version_id, S32 count);
    void getSLMListing(S32 listing_id);
    void updateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed, S32 count);
    void associateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, const LLUUID& source_folder_id);
    void deleteSLMListing(S32 listing_id);
    std::string getSLMConnectURL(const std::string& route);

    void getMerchantStatusCoro();
    void getSLMListingsCoro(LLUUID folderId);
    void getSingleListingCoro(S32 listingId, LLUUID folderId);
    void createSLMListingCoro(LLUUID folderId, LLUUID versionId, S32 count);
    void updateSLMListingCoro(LLUUID folderId, S32 listingId, LLUUID versionId, bool isListed, S32 count);
    void associateSLMListingCoro(LLUUID folderId, S32 listingId, LLUUID versionId, LLUUID sourceFolderId);
    void deleteSLMListingCoro(S32 listingId);

    // Handling Marketplace connection and inventory connection
    U32  mMarketPlaceStatus;
    std::string mMarketPlaceFailureReason;
    status_updated_signal_t* mStatusUpdatedSignal;
    LLInventoryObserver* mInventoryObserver;
    bool mDirtyCount;   // If true, stock count value need to be updated at the next check
    
    // Update data
    U32 mMarketPlaceDataFetched;
    status_updated_signal_t* mDataFetchedSignal;
    std::set<LLUUID> mPendingUpdateSet;

    // Listing folders waiting for validation
    typedef std::map<LLUUID,S32> waiting_list_t;
    waiting_list_t mValidationWaitingList;
    
    // The cache of SLM data (at last...)
    marketplace_items_list_t mMarketplaceItems;
    // We need a list (version folder -> listing folder) because such reverse lookups are frequent
    version_folders_list_t mVersionFolders;
};


#endif // LL_LLMARKETPLACEFUNCTIONS_H

