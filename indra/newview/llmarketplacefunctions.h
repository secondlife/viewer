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


LLSD getMarketplaceStringSubstitutions();


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
		MARKET_PLACE_MERCHANT = 3,
		MARKET_PLACE_NOT_MERCHANT = 4,
	};
}


class LLMarketplaceInventoryImporter
	: public LLSingleton<LLMarketplaceInventoryImporter>
{
public:
	static void update();
	
	LLMarketplaceInventoryImporter();
	
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
	
	status_report_signal_t *	mErrorInitSignal;
	status_changed_signal_t *	mStatusChangedSignal;
	status_report_signal_t *	mStatusReportSignal;
};


// Classes handling the data coming from and going to the Marketplace DB:
// * implement the Marketplace API (TBD)
// * cache the current Marketplace data (tuples)
// * provide methods to get Marketplace data on any inventory item
class LLMarketplaceData;

// A Marketplace item is known by its tuple
class LLMarketplaceTuple 
{
public:
	friend class LLMarketplaceData;

    LLMarketplaceTuple();
    LLMarketplaceTuple(const LLUUID& folder_id);
    LLMarketplaceTuple(const LLUUID& folder_id, std::string listing_id, const LLUUID& version_id, bool is_listed = false);
    
private:
    // Representation of a marketplace item in the Marketplace DB (well, what we know of it...)
    LLUUID mFolderListingId;
    std::string mListingId;
    LLUUID mActiveVersionFolderId;
    bool mIsActive;
};
// The folder UUID is used as a key to this map. It could therefore be taken off the object themselves
typedef std::map<LLUUID, LLMarketplaceTuple> marketplace_items_list_t;

// There's one and only one possible set of Marketplace data per agent and per session
class LLMarketplaceData
    : public LLSingleton<LLMarketplaceData>
{
public:
	LLMarketplaceData();
    
    // Access Marketplace Data : methods return default value if the folder_id can't be found
    bool getActivationState(const LLUUID& folder_id);
    std::string getListingID(const LLUUID& folder_id);
    LLUUID getVersionFolderID(const LLUUID& folder_id);
    
    bool isListed(const LLUUID& folder_id); // returns true if folder_id is in the items map
    
    // Modify Marketplace Data : methods return true is function succeeded, false if error
    bool setListingID(const LLUUID& folder_id, std::string listing_id);
    bool setVersionFolderID(const LLUUID& folder_id, const LLUUID& version_id);
    bool setActivation(const LLUUID& folder_id, bool activate);
    
    // Merov : DD Development : methods to populate the items list with something usefull using
    // inventory IDs and some pseudo random code so we can play with the UI...
    void addTestItem(const LLUUID& folder_id);
    void addTestItem(const LLUUID& folder_id, const LLUUID& version_id);
    
private:
    marketplace_items_list_t mMarketplaceItems;
};


#endif // LL_LLMARKETPLACEFUNCTIONS_H

