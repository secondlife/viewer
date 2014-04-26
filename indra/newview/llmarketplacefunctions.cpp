/** 
 * @file llmarketplacefunctions.cpp
 * @brief Implementation of assorted functions related to the marketplace
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

#include "llmarketplacefunctions.h"

#include "llagent.h"
#include "llhttpclient.h"
#include "llinventoryfunctions.h"
#include "llsdserialize.h"
#include "lltimer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"


//
// Helpers
//

static std::string getMarketplaceDomain()
{
	std::string domain = "secondlife.com";
	
	if (!LLGridManager::getInstance()->isInProductionGrid())
	{
		const std::string& grid_id = LLGridManager::getInstance()->getGridId();
		const std::string& grid_id_lower = utf8str_tolower(grid_id);
		
		if (grid_id_lower == "damballah")
		{
			domain = "secondlife-staging.com";
		}
		else
		{
			domain = llformat("%s.lindenlab.com", grid_id_lower.c_str());
		}
	}
	
	return domain;
}

static std::string getMarketplaceURL(const std::string& urlStringName)
{
	LLStringUtil::format_map_t domain_arg;
	domain_arg["[MARKETPLACE_DOMAIN_NAME]"] = getMarketplaceDomain();
	
	std::string marketplace_url = LLTrans::getString(urlStringName, domain_arg);
	
	return marketplace_url;
}

LLSD getMarketplaceStringSubstitutions()
{
	std::string marketplace_url = getMarketplaceURL("MarketplaceURL");
	std::string marketplace_url_create = getMarketplaceURL("MarketplaceURL_CreateStore");
	std::string marketplace_url_dashboard = getMarketplaceURL("MarketplaceURL_Dashboard");
	std::string marketplace_url_imports = getMarketplaceURL("MarketplaceURL_Imports");
	std::string marketplace_url_info = getMarketplaceURL("MarketplaceURL_LearnMore");
	
	LLSD marketplace_sub_map;

	marketplace_sub_map["[MARKETPLACE_URL]"] = marketplace_url;
	marketplace_sub_map["[MARKETPLACE_CREATE_STORE_URL]"] = marketplace_url_create;
	marketplace_sub_map["[MARKETPLACE_LEARN_MORE_URL]"] = marketplace_url_info;
	marketplace_sub_map["[MARKETPLACE_DASHBOARD_URL]"] = marketplace_url_dashboard;
	marketplace_sub_map["[MARKETPLACE_IMPORTS_URL]"] = marketplace_url_imports;
	
	return marketplace_sub_map;
}

///////////////////////////////////////////////////////////////////////////////
// SLM Responder
class LLSLMMerchantResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMMerchantResponder);
public:
	
    LLSLMMerchantResponder() {}
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
            llinfos << "Merov : completed successful, status = " << status << ", reason = " << reason << ", content = " << content << llendl;
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_MERCHANT);
		}
		else
		{
            llinfos << "Merov : completed with error, status = " << status << ", reason = " << reason << ", content = " << content << llendl;
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT);
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
		if (isGoodStatus(status))
		{
            llinfos << "Merov : completed header successful, status = " << status << ", reason = " << reason << ", content = " << content << llendl;
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_MERCHANT);
		}
		else
		{
            llinfos << "Merov : completed header with error, status = " << status << ", reason = " << reason << ", content = " << content << llendl;
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT);
		}
    }
};
// SLM Responder End
///////////////////////////////////////////////////////////////////////////////

namespace LLMarketplaceImport
{
	// Basic interface for this namespace

	bool hasSessionCookie();
	bool inProgress();
	bool resultPending();
	U32 getResultStatus();
	const LLSD& getResults();

	bool establishMarketplaceSessionCookie();
	bool pollStatus();
	bool triggerImport();
	
	// Internal state variables

	static std::string sMarketplaceCookie = "";
	static LLSD sImportId = LLSD::emptyMap();
	static bool sImportInProgress = false;
	static bool sImportPostPending = false;
	static bool sImportGetPending = false;
	static U32 sImportResultStatus = 0;
	static LLSD sImportResults = LLSD::emptyMap();

	static LLTimer slmGetTimer;
	static LLTimer slmPostTimer;

	// Responders
	
	class LLImportPostResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportPostResponder() : LLCurl::Responder() {}
		
		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			slmPostTimer.stop();

			if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
			{
				llinfos << " SLM POST status: " << status << llendl;
				llinfos << " SLM POST reason: " << reason << llendl;
				llinfos << " SLM POST content: " << content.asString() << llendl;
				llinfos << " SLM POST timer: " << slmPostTimer.getElapsedTimeF32() << llendl;
			}

			// MAINT-2301 : we determined we can safely ignore that error in that context
			if (status == MarketplaceErrorCodes::IMPORT_JOB_TIMEOUT)
			{
				if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
				{
					llinfos << " SLM POST : Ignoring time out status and treating it as success" << llendl;
				}
				status = MarketplaceErrorCodes::IMPORT_DONE;
			}
			
			if (status >= MarketplaceErrorCodes::IMPORT_BAD_REQUEST)
			{
				if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
				{
					llinfos << " SLM POST clearing marketplace cookie due to client or server error" << llendl;
				}
				sMarketplaceCookie.clear();
			}

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_DONE);
			sImportPostPending = false;
			sImportResultStatus = status;
			sImportId = content;
		}
	};
	
	class LLImportGetResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportGetResponder() : LLCurl::Responder() {}
		
		void completedHeader(U32 status, const std::string& reason, const LLSD& content)
		{
			const std::string& set_cookie_string = content["set-cookie"].asString();
			
			if (!set_cookie_string.empty())
			{
				sMarketplaceCookie = set_cookie_string;
			}
		}
		
		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			slmGetTimer.stop();

			if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
			{
				llinfos << " SLM GET status: " << status << llendl;
				llinfos << " SLM GET reason: " << reason << llendl;
				llinfos << " SLM GET content: " << content.asString() << llendl;
				llinfos << " SLM GET timer: " << slmGetTimer.getElapsedTimeF32() << llendl;
			}
			
            // MAINT-2452 : Do not clear the cookie on IMPORT_DONE_WITH_ERRORS : Happens when trying to import objects with wrong permissions
            // ACME-1221 : Do not clear the cookie on IMPORT_NOT_FOUND : Happens for newly created Merchant accounts that are initally empty
			if ((status >= MarketplaceErrorCodes::IMPORT_BAD_REQUEST) &&
                (status != MarketplaceErrorCodes::IMPORT_DONE_WITH_ERRORS) &&
                (status != MarketplaceErrorCodes::IMPORT_NOT_FOUND))
			{
				if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
				{
					llinfos << " SLM GET clearing marketplace cookie due to client or server error" << llendl;
				}
				sMarketplaceCookie.clear();
			}
            else if (gSavedSettings.getBOOL("InventoryOutboxLogging") && (status >= MarketplaceErrorCodes::IMPORT_BAD_REQUEST))
            {
                llinfos << " SLM GET : Got error status = " << status << ", but marketplace cookie not cleared." << llendl;
            }

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_PROCESSING);
			sImportGetPending = false;
			sImportResultStatus = status;
			sImportResults = content;
		}
	};

	// Basic API

	bool hasSessionCookie()
	{
		return !sMarketplaceCookie.empty();
	}
	
	bool inProgress()
	{
		return sImportInProgress;
	}
	
	bool resultPending()
	{
		return (sImportPostPending || sImportGetPending);
	}
	
	U32 getResultStatus()
	{
		return sImportResultStatus;
	}
	
	const LLSD& getResults()
	{
		return sImportResults;
	}
	
	static std::string getInventoryImportURL()
	{
		std::string url = getMarketplaceURL("MarketplaceURL");
		
		url += "api/1/";
		url += gAgent.getID().getString();
		url += "/inventory/import/";
		
		return url;
	}
	
	bool establishMarketplaceSessionCookie()
	{
		if (hasSessionCookie())
		{
			return false;
		}

		sImportInProgress = true;
		sImportGetPending = true;
		
		std::string url = getInventoryImportURL();
		
		if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
		{
            llinfos << " SLM GET: establishMarketplaceSessionCookie, LLHTTPClient::get, url = " << url << llendl;
            LLSD headers = LLViewerMedia::getHeaders();
            std::stringstream str;
            LLSDSerialize::toPrettyXML(headers, str);
            llinfos << " SLM GET: headers " << llendl;
            llinfos << str.str() << llendl;
		}

		slmGetTimer.start();
		LLHTTPClient::get(url, new LLImportGetResponder(), LLViewerMedia::getHeaders());
		
		return true;
	}
	
	bool pollStatus()
	{
		if (!hasSessionCookie())
		{
			return false;
		}
		
		sImportGetPending = true;

		std::string url = getInventoryImportURL();

		url += sImportId.asString();

		// Make the headers for the post
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Cookie"] = sMarketplaceCookie;
		headers["Content-Type"] = "application/llsd+xml";
		headers["User-Agent"] = LLViewerMedia::getCurrentUserAgent();
		
		if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
		{
            llinfos << " SLM GET: pollStatus, LLHTTPClient::get, url = " << url << llendl;
            std::stringstream str;
            LLSDSerialize::toPrettyXML(headers, str);
            llinfos << " SLM GET: headers " << llendl;
            llinfos << str.str() << llendl;
		}

		slmGetTimer.start();
		LLHTTPClient::get(url, new LLImportGetResponder(), headers);
		
		return true;
	}
	
	bool triggerImport()
	{
		if (!hasSessionCookie())
		{
			return false;
		}

		sImportId = LLSD::emptyMap();
		sImportInProgress = true;
		sImportPostPending = true;
		sImportResultStatus = MarketplaceErrorCodes::IMPORT_PROCESSING;
		sImportResults = LLSD::emptyMap();

		std::string url = getInventoryImportURL();
		
		// Make the headers for the post
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Connection"] = "Keep-Alive";
		headers["Cookie"] = sMarketplaceCookie;
		headers["Content-Type"] = "application/xml";
		headers["User-Agent"] = LLViewerMedia::getCurrentUserAgent();
		
		if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
		{
            llinfos << " SLM POST: triggerImport, LLHTTPClient::post, url = " << url << llendl;
            std::stringstream str;
            LLSDSerialize::toPrettyXML(headers, str);
            llinfos << " SLM POST: headers " << llendl;
            llinfos << str.str() << llendl;
		}

		slmPostTimer.start();
        LLHTTPClient::post(url, LLSD(), new LLImportPostResponder(), headers);
		
		return true;
	}
}


//
// Interface class
//

static const F32 MARKET_IMPORTER_UPDATE_FREQUENCY = 1.0f;

//static
void LLMarketplaceInventoryImporter::update()
{
	if (instanceExists())
	{
		static LLTimer update_timer;
		if (update_timer.hasExpired())
		{
			LLMarketplaceInventoryImporter::instance().updateImport();
			update_timer.setTimerExpirySec(MARKET_IMPORTER_UPDATE_FREQUENCY);
		}
	}
}

LLMarketplaceInventoryImporter::LLMarketplaceInventoryImporter()
	: mAutoTriggerImport(false)
	, mImportInProgress(false)
	, mInitialized(false)
	, mMarketPlaceStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_INITIALIZED)
	, mErrorInitSignal(NULL)
	, mStatusChangedSignal(NULL)
	, mStatusReportSignal(NULL)
{
}

boost::signals2::connection LLMarketplaceInventoryImporter::setInitializationErrorCallback(const status_report_signal_t::slot_type& cb)
{
	if (mErrorInitSignal == NULL)
	{
		mErrorInitSignal = new status_report_signal_t();
	}
	
	return mErrorInitSignal->connect(cb);
}

boost::signals2::connection LLMarketplaceInventoryImporter::setStatusChangedCallback(const status_changed_signal_t::slot_type& cb)
{
	if (mStatusChangedSignal == NULL)
	{
		mStatusChangedSignal = new status_changed_signal_t();
	}

	return mStatusChangedSignal->connect(cb);
}

boost::signals2::connection LLMarketplaceInventoryImporter::setStatusReportCallback(const status_report_signal_t::slot_type& cb)
{
	if (mStatusReportSignal == NULL)
	{
		mStatusReportSignal = new status_report_signal_t();
	}

	return mStatusReportSignal->connect(cb);
}

void LLMarketplaceInventoryImporter::initialize()
{
    if (mInitialized)
    {
        return;
    }

    if (!LLMarketplaceImport::hasSessionCookie())
    {
        mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING;
        LLMarketplaceImport::establishMarketplaceSessionCookie();
    }
    else
    {
        mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_MERCHANT;
    }
}

void LLMarketplaceInventoryImporter::reinitializeAndTriggerImport()
{
	mInitialized = false;
	mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_NOT_INITIALIZED;
	initialize();
	mAutoTriggerImport = true;
}

bool LLMarketplaceInventoryImporter::triggerImport()
{
	const bool import_triggered = LLMarketplaceImport::triggerImport();
	
	if (!import_triggered)
	{
		reinitializeAndTriggerImport();
	}
	
	return import_triggered;
}

void LLMarketplaceInventoryImporter::updateImport()
{
	const bool in_progress = LLMarketplaceImport::inProgress();
	
	if (in_progress && !LLMarketplaceImport::resultPending())
	{
		const bool polling_status = LLMarketplaceImport::pollStatus();
		
		if (!polling_status)
		{
			reinitializeAndTriggerImport();
		}
	}	
	
	if (mImportInProgress != in_progress)
	{
		mImportInProgress = in_progress;

		// If we are no longer in progress
		if (!mImportInProgress)
		{
			if (mInitialized)
			{
				// Report results
				if (mStatusReportSignal)
				{
					(*mStatusReportSignal)(LLMarketplaceImport::getResultStatus(), LLMarketplaceImport::getResults());
				}
			}
			else
			{
				// Look for results success
				mInitialized = LLMarketplaceImport::hasSessionCookie();
				
				if (mInitialized)
				{
					mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_MERCHANT;
					// Follow up with auto trigger of import
					if (mAutoTriggerImport)
					{
						mAutoTriggerImport = false;
						mImportInProgress = triggerImport();
					}
				}
				else
				{
					U32 status = LLMarketplaceImport::getResultStatus();
					if ((status == MarketplaceErrorCodes::IMPORT_FORBIDDEN) ||
						(status == MarketplaceErrorCodes::IMPORT_AUTHENTICATION_ERROR))
					{
						mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT;
					}
					else 
					{
						mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE;
					}
					if (mErrorInitSignal && (mMarketPlaceStatus == MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE))
					{
						(*mErrorInitSignal)(LLMarketplaceImport::getResultStatus(), LLMarketplaceImport::getResults());
					}
				}
			}
		}

		// Make sure we trigger the status change with the final state (in case of auto trigger after initialize)
		if (mStatusChangedSignal)
		{
			(*mStatusChangedSignal)(mImportInProgress);
		}
	}
}

//
// Direct Delivery : Marketplace tuples and data
//

// Tuple == Item
LLMarketplaceTuple::LLMarketplaceTuple() :
    mListingFolderId(),
    mListingId(0),
    mVersionFolderId(),
    mIsActive(false)
{
}

LLMarketplaceTuple::LLMarketplaceTuple(const LLUUID& folder_id) :
    mListingFolderId(folder_id),
    mListingId(0),
    mVersionFolderId(),
    mIsActive(false)
{
}

LLMarketplaceTuple::LLMarketplaceTuple(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed) :
    mListingFolderId(folder_id),
    mListingId(listing_id),
    mVersionFolderId(version_id),
    mIsActive(is_listed)
{
}


// Data map
LLMarketplaceData::LLMarketplaceData() : 
 mMarketPlaceStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_INITIALIZED)
{
    mTestCurrentMarketplaceID = 1234567;
}

S32 LLMarketplaceData::getTestMarketplaceID()
{
    return mTestCurrentMarketplaceID++;
}

void LLMarketplaceData::initializeSLM()
{
    mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING;
    
    // Get DirectDelivery cap
    std::string url = "";
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
    {
        url = region->getCapability("DirectDelivery");
        llinfos << "Merov : Test DirectDelivery cap : url = " << url << llendl;
    }
    else
    {
        llinfos << "Merov : Test DirectDelivery cap : no region accessible" << llendl;
    }
    // *TODO : Take this DirectDelivery cap coping hack out
    if (url == "")
    {
        url = "https://marketplace.secondlife-staging.com/api/1/viewer/" + gAgentID.asString() + "/merchant";
    }
    llinfos << "Merov : Testing get : " << url << llendl;
    
	LLHTTPClient::get(url, new LLSLMMerchantResponder(), LLSD());
}

// Creation / Deletion
bool LLMarketplaceData::addListing(const LLUUID& folder_id)
{
    if (isListed(folder_id))
    {
        // Listing already exists -> exit with error
        return false;
    }
	mMarketplaceItems[folder_id] = LLMarketplaceTuple(folder_id);
    
    // *TODO : Create the listing on SLM and get the ID (blocking?)
    // For the moment, we use that wonky test ID generator...
    S32 listing_id = LLMarketplaceData::instance().getTestMarketplaceID();
    
    setListingID(folder_id,listing_id);
    update_marketplace_category(folder_id);
    return true;
}

bool LLMarketplaceData::associateListing(const LLUUID& folder_id, S32 listing_id)
{
    if (isListed(folder_id))
    {
        // Listing already exists -> exit with error
        return false;
    }
	mMarketplaceItems[folder_id] = LLMarketplaceTuple(folder_id);
    
    // Check that the listing ID is not already associated to some other record
    LLUUID old_listing = getListingFolder(listing_id);
    if (old_listing.notNull())
    {
        // If it is already used, unlist the old record (we can't have 2 listings with the same listing ID)
        deleteListing(old_listing);
    }
    
    setListingID(folder_id,listing_id);
    update_marketplace_category(folder_id);
    return true;
}

bool LLMarketplaceData::deleteListing(const LLUUID& folder_id)
{
    if (!isListed(folder_id))
    {
        // Listing doesn't exist -> exit with error
        return false;
    }
	mMarketplaceItems.erase(folder_id);
    update_marketplace_category(folder_id);
    return true;
}

// Accessors
bool LLMarketplaceData::getActivationState(const LLUUID& folder_id)
{
    // Listing folder case
    if (isListed(folder_id))
    {
        marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
        return (it->second).mIsActive;
    }
    // We need to iterate through the list to check it's not a version folder
    marketplace_items_list_t::iterator it = mMarketplaceItems.begin();
    while (it != mMarketplaceItems.end())
    {
        if ((it->second).mVersionFolderId == folder_id)
        {
            return (it->second).mIsActive;
        }
        it++;
    }
    return false;
}

S32 LLMarketplaceData::getListingID(const LLUUID& folder_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    return (it == mMarketplaceItems.end() ? 0 : (it->second).mListingId);
}

LLUUID LLMarketplaceData::getVersionFolderID(const LLUUID& folder_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    return (it == mMarketplaceItems.end() ? LLUUID::null : (it->second).mVersionFolderId);
}

// Reverse lookup : find the listing folder id from the listing id
LLUUID LLMarketplaceData::getListingFolder(S32 listing_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.begin();
    while (it != mMarketplaceItems.end())
    {
        if ((it->second).mListingId == listing_id)
        {
            return (it->second).mListingFolderId;
        }
        it++;
    }
    return LLUUID::null;
}

bool LLMarketplaceData::isListed(const LLUUID& folder_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    return (it != mMarketplaceItems.end());
}

bool LLMarketplaceData::isVersionFolder(const LLUUID& folder_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.begin();
    while (it != mMarketplaceItems.end())
    {
        if ((it->second).mVersionFolderId == folder_id)
        {
            return true;
        }
        it++;
    }
    return false;
}

std::string LLMarketplaceData::getListingURL(const LLUUID& folder_id)
{
    // Get the listing id (i.e. go up the hierarchy to find the listing folder
    // URL format will be something like : https://marketplace.secondlife.com/p/listing/<listing_id>
	std::string marketplace_url = getMarketplaceURL("MarketplaceURL");
    
    S32 depth = depth_nesting_in_marketplace(folder_id);
    LLUUID listing_uuid = nested_parent_id(folder_id, depth);
    S32 listing_id = getListingID(listing_uuid);
    
    if (listing_id != 0)
    {
        marketplace_url += llformat("p/listing/%d",listing_id);
    }
    return marketplace_url;
}

// Modifiers
bool LLMarketplaceData::setListingID(const LLUUID& folder_id, S32 listing_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    if (it == mMarketplaceItems.end())
    {
        return false;
    }
    else
    {
        (it->second).mListingId = listing_id;
        update_marketplace_category(folder_id);
        return true;
    }
}

bool LLMarketplaceData::setVersionFolderID(const LLUUID& folder_id, const LLUUID& version_id)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    if (it == mMarketplaceItems.end())
    {
        return false;
    }
    else
    {
        LLUUID old_version_id = (it->second).mVersionFolderId;
        if (old_version_id != version_id)
        {
            (it->second).mVersionFolderId = version_id;
            update_marketplace_category(old_version_id);
            update_marketplace_category(version_id);
        }
        return true;
    }
}

bool LLMarketplaceData::setActivation(const LLUUID& folder_id, bool activate)
{
    // Listing folder case
    if (isListed(folder_id))
    {
        marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
        (it->second).mIsActive = activate;
        update_marketplace_category((it->second).mListingFolderId);
        return true;
    }
    // We need to iterate through the list to check it's not a version folder
    marketplace_items_list_t::iterator it = mMarketplaceItems.begin();
    while (it != mMarketplaceItems.end())
    {
        if ((it->second).mVersionFolderId == folder_id)
        {
            (it->second).mIsActive = activate;
            update_marketplace_category((it->second).mListingFolderId);
            return true;
        }
        it++;
    }
    return false;
}


