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
#include "llbufferstream.h"
#include "llhttpclient.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llsdserialize.h"
#include "lltimer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"
#include "llviewerregion.h"

#include "reader.h" // JSON
#include "writer.h" // JSON

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
// SLM Responders
void log_SLM_error(const std::string& request, U32 status, const std::string& reason, const std::string& code, const std::string& description)
{
		LL_WARNS("SLM") << "Merov : " << request << " request failed. status : " << status << ", reason : " << reason << ", code : " << code << ", description : " << description << LL_ENDL;
}

// Merov: This is a temporary hack used by dev while secondlife-staging is down...
// *TODO : Suppress that before shipping!
static bool sBypassMerchant = false;

class LLSLMGetMerchantResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMGetMerchantResponder);
public:
	
    LLSLMGetMerchantResponder() {}
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content) { }
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
		if (isGoodStatus(status) || sBypassMerchant)
		{
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_MERCHANT);
		}
		else if (status == SLMErrorCodes::SLM_NOT_FOUND)
		{
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_MERCHANT);
		}
		else
		{
            log_SLM_error("Get merchant", status, reason, content.get("error_code"), content.get("error_description"));
            LLMarketplaceData::instance().setSLMStatus(MarketplaceStatusCodes::MARKET_PLACE_CONNECTION_FAILURE);
		}
    }
};

class LLSLMGetListingsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMGetListingsResponder);
public:
	
    LLSLMGetListingsResponder() {}
    
    virtual void completedRaw(U32 status,
                              const std::string& reason,
                              const LLChannelDescriptors& channels,
                              const LLIOPipe::buffer_ptr_t& buffer)
    {
		if (!isGoodStatus(status))
		{
            log_SLM_error("Get listings", status, reason, "", "");
            return;
		}
        
        LLBufferStream istr(channels, buffer.get());
        std::stringstream strstrm;
        strstrm << istr.rdbuf();
        const std::string body = strstrm.str();
        
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(body,root))
        {
            log_SLM_error("Get listings", status, "Json parsing failed", reader.getFormatedErrorMessages(), body);
            return;
        }

        llinfos << "Merov : Get listings completedRaw : data = " << body << llendl;

        // Extract the info from the Json string
        Json::ValueIterator it = root["listings"].begin();
        
        while (it != root["listings"].end())
        {
            Json::Value listing = *it;
            
            int listing_id = listing["id"].asInt();
            bool is_listed = listing["is_listed"].asBool();
            std::string edit_url = listing["edit_url"].asString();
            std::string folder_uuid_string = listing["inventory_info"]["listing_folder_id"].asString();
            std::string version_uuid_string = listing["inventory_info"]["version_folder_id"].asString();
            
            LLUUID folder_id(folder_uuid_string);
            LLUUID version_id(version_uuid_string);
            LLMarketplaceData::instance().addListing(folder_id,listing_id,version_id,is_listed);
            LLMarketplaceData::instance().setEditURL(folder_id, edit_url);
            it++;
        }
    }
};

class LLSLMCreateListingsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMCreateListingsResponder);
public:
	
    LLSLMCreateListingsResponder() {}
    
    virtual void completedRaw(U32 status,
                              const std::string& reason,
                              const LLChannelDescriptors& channels,
                              const LLIOPipe::buffer_ptr_t& buffer)
    {
		if (!isGoodStatus(status))
		{
            log_SLM_error("Post listings", status, reason, "", "");
            return;
		}
        
        LLBufferStream istr(channels, buffer.get());
        std::stringstream strstrm;
        strstrm << istr.rdbuf();
        const std::string body = strstrm.str();
        
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(body,root))
        {
            log_SLM_error("Post listings", status, "Json parsing failed", reader.getFormatedErrorMessages(), body);
            return;
        }

        llinfos << "Merov : Create listing completedRaw : data = " << body << llendl;

        // Extract the info from the Json string
        Json::ValueIterator it = root["listings"].begin();
        
        while (it != root["listings"].end())
        {
            Json::Value listing = *it;
            
            int listing_id = listing["id"].asInt();
            bool is_listed = listing["is_listed"].asBool();
            std::string edit_url = listing["edit_url"].asString();
            std::string folder_uuid_string = listing["inventory_info"]["listing_folder_id"].asString();
            std::string version_uuid_string = listing["inventory_info"]["version_folder_id"].asString();
            
            LLUUID folder_id(folder_uuid_string);
            LLUUID version_id(version_uuid_string);
            LLMarketplaceData::instance().addListing(folder_id,listing_id,version_id,is_listed);
            LLMarketplaceData::instance().setEditURL(folder_id, edit_url);
            it++;
        }
    }
};

class LLSLMUpdateListingsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMUpdateListingsResponder);
public:
	
    LLSLMUpdateListingsResponder() {}
    
    virtual void completedRaw(U32 status,
                              const std::string& reason,
                              const LLChannelDescriptors& channels,
                              const LLIOPipe::buffer_ptr_t& buffer)
    {
		if (!isGoodStatus(status))
		{
            log_SLM_error("Put listings", status, reason, "", "");
            return;
		}
        
        LLBufferStream istr(channels, buffer.get());
        std::stringstream strstrm;
        strstrm << istr.rdbuf();
        const std::string body = strstrm.str();
        
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(body,root))
        {
            log_SLM_error("Put listings", status, "Json parsing failed", reader.getFormatedErrorMessages(), body);
            return;
        }
        
        llinfos << "Merov : Update listing completedRaw : data = " << body << llendl;

        // Extract the info from the Json string
        Json::ValueIterator it = root["listings"].begin();
        
        while (it != root["listings"].end())
        {
            Json::Value listing = *it;
            
            int listing_id = listing["id"].asInt();
            bool is_listed = listing["is_listed"].asBool();
            std::string edit_url = listing["edit_url"].asString();
            std::string folder_uuid_string = listing["inventory_info"]["listing_folder_id"].asString();
            std::string version_uuid_string = listing["inventory_info"]["version_folder_id"].asString();
            
            LLUUID folder_id(folder_uuid_string);
            LLUUID version_id(version_uuid_string);
            
            // Update that listing
            LLMarketplaceData::instance().setListingID(folder_id, listing_id);
            LLMarketplaceData::instance().setVersionFolderID(folder_id, version_id);
            LLMarketplaceData::instance().setActivation(folder_id, is_listed);
            LLMarketplaceData::instance().setEditURL(folder_id, edit_url);
            
            it++;
        }
    }
};

class LLSLMAssociateListingsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLSLMAssociateListingsResponder);
public:
	
    LLSLMAssociateListingsResponder() {}
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content) { }
    
    virtual void completedRaw(U32 status,
                              const std::string& reason,
                              const LLChannelDescriptors& channels,
                              const LLIOPipe::buffer_ptr_t& buffer)
    {
		if (!isGoodStatus(status))
		{
            log_SLM_error("Associate listings", status, reason, "", "");
            return;
		}
        
        LLBufferStream istr(channels, buffer.get());
        std::stringstream strstrm;
        strstrm << istr.rdbuf();
        const std::string body = strstrm.str();
        
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(body,root))
        {
            log_SLM_error("Associate listings", status, "Json parsing failed", reader.getFormatedErrorMessages(), body);
            return;
        }
        
        llinfos << "Merov : Update listing completedRaw : data = " << body << llendl;
        
        // Extract the info from the Json string
        Json::ValueIterator it = root["listings"].begin();
        
        while (it != root["listings"].end())
        {
            Json::Value listing = *it;
            
            int listing_id = listing["id"].asInt();
            bool is_listed = listing["is_listed"].asBool();
            std::string edit_url = listing["edit_url"].asString();
            std::string folder_uuid_string = listing["inventory_info"]["listing_folder_id"].asString();
            std::string version_uuid_string = listing["inventory_info"]["version_folder_id"].asString();
            
            LLUUID folder_id(folder_uuid_string);
            LLUUID version_id(version_uuid_string);
            
            // Check that the listing ID is not already associated to some other record
            LLUUID old_listing = LLMarketplaceData::instance().getListingFolder(listing_id);
            if (old_listing.notNull())
            {
                // If it is already used, unlist the old record (we can't have 2 listings with the same listing ID)
                LLMarketplaceData::instance().deleteListing(old_listing);
            }
            
            // Add the new association
            LLMarketplaceData::instance().addListing(folder_id,listing_id,version_id,is_listed);
            LLMarketplaceData::instance().setEditURL(folder_id, edit_url);
            it++;
        }
    }
};

// SLM Responders End
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
class LLMarketplaceInventoryObserver : public LLInventoryObserver
{
public:
	LLMarketplaceInventoryObserver() {}
	virtual ~LLMarketplaceInventoryObserver() {}
	virtual void changed(U32 mask);
};

void LLMarketplaceInventoryObserver::changed(U32 mask)
{
    // When things are changed in the inventory, this can trigger a host of changes in the marketplace listings folder:
    // * stock counts changing : no copy items coming in and out will change the stock count on folders
    // * version and listing folders : moving those might invalidate the marketplace data itself
    // Since we should cannot raise inventory change while in the observer is called (the list will be cleared once observers are called)
    // we need to raise a flag in the inventory to signal that things have been dirtied.

    // That's the only changes that really do make sense for marketplace to worry about
	if ((mask & (LLInventoryObserver::INTERNAL | LLInventoryObserver::STRUCTURE)) != 0)
	{
        const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();
    
        std::set<LLUUID>::const_iterator id_it = changed_items.begin();
        std::set<LLUUID>::const_iterator id_end = changed_items.end();
        for (;id_it != id_end; ++id_it)
        {
            LLInventoryObject* obj = gInventory.getObject(*id_it);
            if (LLAssetType::AT_CATEGORY == obj->getType())
            {
                // If it's a folder known to the marketplace, let's check it's in proper shape
                if (LLMarketplaceData::instance().isListed(*id_it) || LLMarketplaceData::instance().isVersionFolder(*id_it))
                {
                    LLInventoryCategory* cat = (LLInventoryCategory*)(obj);
                    validate_marketplacelistings(cat);
                }
            }
            else
            {
                // If it's not a category, it's an item...
                LLInventoryItem* item = (LLInventoryItem*)(obj);
                // If it's a no copy item, we may need to update the label count of marketplace listings
                if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
                {
                    LLMarketplaceData::instance().setDirtyCount();
                }   
            }
        }
	}
}

// Tuple == Item
LLMarketplaceTuple::LLMarketplaceTuple() :
    mListingFolderId(),
    mListingId(0),
    mVersionFolderId(),
    mIsActive(false),
    mEditURL("")
{
}

LLMarketplaceTuple::LLMarketplaceTuple(const LLUUID& folder_id) :
    mListingFolderId(folder_id),
    mListingId(0),
    mVersionFolderId(),
    mIsActive(false),
    mEditURL("")
{
}

LLMarketplaceTuple::LLMarketplaceTuple(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed) :
    mListingFolderId(folder_id),
    mListingId(listing_id),
    mVersionFolderId(version_id),
    mIsActive(is_listed),
    mEditURL("")
{
}


// Data map
LLMarketplaceData::LLMarketplaceData() : 
 mMarketPlaceStatus(MarketplaceStatusCodes::MARKET_PLACE_NOT_INITIALIZED),
 mStatusUpdatedSignal(NULL),
 mDirtyCount(false)
{
    mInventoryObserver = new LLMarketplaceInventoryObserver;
    gInventory.addObserver(mInventoryObserver);
}

LLMarketplaceData::~LLMarketplaceData()
{
	gInventory.removeObserver(mInventoryObserver);
}

void LLMarketplaceData::initializeSLM(const status_updated_signal_t::slot_type& cb)
{
    mMarketPlaceStatus = MarketplaceStatusCodes::MARKET_PLACE_INITIALIZING;
	if (mStatusUpdatedSignal == NULL)
	{
		mStatusUpdatedSignal = new status_updated_signal_t();
	}
	mStatusUpdatedSignal->connect(cb);
	LLHTTPClient::get(getSLMConnectURL("/merchant"), new LLSLMGetMerchantResponder(), LLSD());
}

void LLMarketplaceData::getSLMListings()
{
	LLHTTPClient::get(getSLMConnectURL("/listings"), new LLSLMGetListingsResponder(), LLSD());
}

void LLMarketplaceData::createSLMListing(const LLUUID& folder_id)
{
	LLSD headers = LLSD::emptyMap();
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
    
    LLViewerInventoryCategory* category = gInventory.getCategory(folder_id);

    Json::Value root;
    Json::FastWriter writer;
    
    root["listing"]["name"] = category->getName();
    root["listing"]["inventory_info"]["listing_folder_id"] = category->getUUID().asString();
    
    std::string json_str = writer.write(root);
    
	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = json_str.size();
	U8 *data = new U8[size];
	memcpy(data, (U8*)(json_str.c_str()), size);
    
	// Send request
	LLHTTPClient::postRaw(getSLMConnectURL("/listings"), data, size, new LLSLMCreateListingsResponder(), headers);
}

void LLMarketplaceData::updateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed)
{
	LLSD headers = LLSD::emptyMap();
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";

    Json::Value root;
    Json::FastWriter writer;
    
    // Note : we're assuming that sending unchanged info won't break anything server side...
    root["listing"]["id"] = listing_id;
    root["listing"]["is_listed"] = is_listed;
    root["listing"]["inventory_info"]["listing_folder_id"] = folder_id.asString();
    root["listing"]["inventory_info"]["version_folder_id"] = version_id.asString();
    
    std::string json_str = writer.write(root);
    
	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = json_str.size();
	U8 *data = new U8[size];
	memcpy(data, (U8*)(json_str.c_str()), size);
    
	// Send request
    std::string url = getSLMConnectURL("/listing/") + llformat("%d",listing_id);
    llinfos << "Merov : updating listing : " << url << ", data = " << json_str << llendl;
	LLHTTPClient::putRaw(url, data, size, new LLSLMUpdateListingsResponder(), headers);
}

void LLMarketplaceData::associateSLMListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id)
{
	LLSD headers = LLSD::emptyMap();
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";
    
    Json::Value root;
    Json::FastWriter writer;
    
    // Note : we're assuming that sending unchanged info won't break anything server side...
    root["listing"]["id"] = listing_id;
    root["listing"]["inventory_info"]["listing_folder_id"] = folder_id.asString();
    root["listing"]["inventory_info"]["version_folder_id"] = version_id.asString();
    
    std::string json_str = writer.write(root);
    
	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = json_str.size();
	U8 *data = new U8[size];
	memcpy(data, (U8*)(json_str.c_str()), size);
    
	// Send request
    std::string url = getSLMConnectURL("/associate_inventory/") + llformat("%d",listing_id);
    llinfos << "Merov : associate listing : " << url << ", data = " << json_str << llendl;
	LLHTTPClient::putRaw(url, data, size, new LLSLMAssociateListingsResponder(), headers);
}

std::string LLMarketplaceData::getSLMConnectURL(const std::string& route)
{
    std::string url("");
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
        // Get DirectDelivery cap
        url = regionp->getCapability("DirectDelivery");
        // *TODO : Take this DirectDelivery cap coping mechanism hack out
        if (url == "")
        {
            url = "https://marketplace.secondlife-staging.com/api/1/viewer/" + gAgentID.asString();
        }
        url += route;
    }
    llinfos << "Merov : Testing getSLMConnectURL : " << url << llendl;
	return url;
}

void LLMarketplaceData::setSLMStatus(U32 status)
{
    mMarketPlaceStatus = status;
    if (mStatusUpdatedSignal)
    {
        (*mStatusUpdatedSignal)();
    }
}

// Creation / Deletion / Update
// Methods publicly called
bool LLMarketplaceData::createListing(const LLUUID& folder_id)
{
    if (isListed(folder_id))
    {
        // Listing already exists -> exit with error
        return false;
    }
    
    // Post the listing creation request to SLM
    createSLMListing(folder_id);
    
    return true;
}

bool LLMarketplaceData::activateListing(const LLUUID& folder_id, bool activate)
{
    // Folder id can be the root of the listing or not so we need to retrieve the root first
    S32 depth = depth_nesting_in_marketplace(folder_id);
    LLUUID listing_uuid = nested_parent_id(folder_id, depth);
    S32 listing_id = getListingID(listing_uuid);
    if (listing_id == 0)
    {
        // Listing doesn't exists -> exit with error
        return false;
    }
    
    LLUUID version_uuid = getVersionFolderID(listing_uuid);

    // Post the listing update request to SLM
    updateSLMListing(listing_uuid, listing_id, version_uuid, activate);
    
    return true;
}

bool LLMarketplaceData::setVersionFolder(const LLUUID& folder_id, const LLUUID& version_id)
{
    // Folder id can be the root of the listing or not so we need to retrieve the root first
    S32 depth = depth_nesting_in_marketplace(folder_id);
    LLUUID listing_uuid = nested_parent_id(folder_id, depth);
    S32 listing_id = getListingID(listing_uuid);
    if (listing_id == 0)
    {
        // Listing doesn't exists -> exit with error
        return false;
    }
    
    bool is_listed = getActivationState(listing_uuid);
    
    // Post the listing update request to SLM
    updateSLMListing(listing_uuid, listing_id, version_id, is_listed);
    
    return true;
}

bool LLMarketplaceData::associateListing(const LLUUID& folder_id, S32 listing_id)
{
    if (isListed(folder_id))
    {
        // Listing already exists -> exit with error
        return false;
    }
    
    // Post the listing update request to SLM
    LLUUID version_id;
    associateSLMListing(folder_id, listing_id, version_id);
    
    return true;
}

// Methods privately called or called by SLM responders to perform changes
bool LLMarketplaceData::addListing(const LLUUID& folder_id, S32 listing_id, const LLUUID& version_id, bool is_listed)
{
    if (isListed(folder_id))
    {
        // Listing already exists -> exit with error
        return false;
    }
	mMarketplaceItems[folder_id] = LLMarketplaceTuple(folder_id, listing_id, version_id, is_listed);

    update_marketplace_category(folder_id);
    gInventory.notifyObservers();
    return true;
}

bool LLMarketplaceData::deleteListing(const LLUUID& folder_id)
{
    if (!isListed(folder_id))
    {
        // Listing doesn't exist -> exit with error
        return false;
    }
    // *TODO : Implement SLM API for deleting SLM records once it exists there...
	mMarketplaceItems.erase(folder_id);
    update_marketplace_category(folder_id);
    gInventory.notifyObservers();
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
    S32 depth = depth_nesting_in_marketplace(folder_id);
    LLUUID listing_uuid = nested_parent_id(folder_id, depth);
    
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(listing_uuid);
    return (it == mMarketplaceItems.end() ? "" : (it->second).mEditURL);
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
        gInventory.notifyObservers();
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
            gInventory.notifyObservers();
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
        gInventory.notifyObservers();
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
            gInventory.notifyObservers();
            return true;
        }
        it++;
    }
    return false;
}

bool LLMarketplaceData::setEditURL(const LLUUID& folder_id, const std::string& edit_url)
{
    marketplace_items_list_t::iterator it = mMarketplaceItems.find(folder_id);
    if (it == mMarketplaceItems.end())
    {
        return false;
    }
    else
    {
        (it->second).mEditURL = edit_url;
        return true;
    }
}



