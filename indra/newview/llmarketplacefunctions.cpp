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
#include "lltimer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewernetwork.h"


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

namespace LLMarketplaceImport
{
	// Basic interface for this namespace

	bool hasSessionCookie();
	bool inProgress();
	bool resultPending();
	S32 getResultStatus();
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
	static S32 sImportResultStatus = 0;
	static LLSD sImportResults = LLSD::emptyMap();

	static LLTimer slmGetTimer;
	static LLTimer slmPostTimer;

	// Responders
	
	class LLImportPostResponder : public LLHTTPClient::Responder
	{
		LOG_CLASS(LLImportPostResponder);
	public:
		LLImportPostResponder() : LLCurl::Responder() {}

	protected:
		/* virtual */ void httpCompleted()
		{
			slmPostTimer.stop();

			if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
			{
				llinfos << " SLM [timer:" << slmPostTimer.getElapsedTimeF32() << "] "
						<< dumpResponse() << llendl;
			}

			S32 status = getStatus();
			if ((status == MarketplaceErrorCodes::IMPORT_REDIRECT) ||
				(status == MarketplaceErrorCodes::IMPORT_AUTHENTICATION_ERROR) ||
				(status == MarketplaceErrorCodes::IMPORT_JOB_TIMEOUT))
			{
				if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
				{
					llinfos << " SLM POST clearing marketplace cookie due to authentication failure or timeout" << llendl;
				}

				sMarketplaceCookie.clear();
			}

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_DONE);
			sImportPostPending = false;
			sImportResultStatus = status;
			sImportId = getContent();
		}
	};
	
	class LLImportGetResponder : public LLHTTPClient::Responder
	{
		LOG_CLASS(LLImportGetResponder);
	public:
		LLImportGetResponder() : LLCurl::Responder() {}
		
	protected:
		/* virtual */ void httpCompleted()
		{
			const bool check_lower = true;
			const std::string& set_cookie_string = getResponseHeader(HTTP_HEADER_SET_COOKIE, check_lower);
			
			if (!set_cookie_string.empty())
			{
				sMarketplaceCookie = set_cookie_string;
			}

			slmGetTimer.stop();

			if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
			{
				llinfos << " SLM [timer:" << slmGetTimer.getElapsedTimeF32() << "] "
						<< dumpResponse() << llendl;
			}
			
			S32 status = getStatus();
			if ((status == MarketplaceErrorCodes::IMPORT_AUTHENTICATION_ERROR) ||
				(status == MarketplaceErrorCodes::IMPORT_JOB_TIMEOUT))
			{
				if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
				{
					llinfos << " SLM GET clearing marketplace cookie due to authentication failure or timeout" << llendl;
				}

				sMarketplaceCookie.clear();
			}

			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_PROCESSING);
			sImportGetPending = false;
			sImportResultStatus = status;
			sImportResults = getContent();
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
	
	S32 getResultStatus()
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
			llinfos << " SLM GET: " << url << llendl;
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
		headers[HTTP_HEADER_ACCEPT] = "*/*";
		headers[HTTP_HEADER_COOKIE] = sMarketplaceCookie;
		headers[HTTP_HEADER_CONTENT_TYPE] = HTTP_CONTENT_LLSD_XML;
		headers[HTTP_HEADER_USER_AGENT] = LLViewerMedia::getCurrentUserAgent();
		
		if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
		{
			llinfos << " SLM GET: " << url << llendl;
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
		headers[HTTP_HEADER_ACCEPT] = "*/*";
		headers[HTTP_HEADER_CONNECTION] = "Keep-Alive";
		headers[HTTP_HEADER_COOKIE] = sMarketplaceCookie;
		// *TODO: Should this be 'application/llsd+xml'?
		headers[HTTP_HEADER_CONTENT_TYPE] = HTTP_CONTENT_XML;
		headers[HTTP_HEADER_USER_AGENT] = LLViewerMedia::getCurrentUserAgent();
		
		if (gSavedSettings.getBOOL("InventoryOutboxLogging"))
		{
			llinfos << " SLM POST: " << url << llendl;
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
	llassert(!mInitialized);

	if (!LLMarketplaceImport::hasSessionCookie())
	{
		LLMarketplaceImport::establishMarketplaceSessionCookie();
	}
}

void LLMarketplaceInventoryImporter::reinitializeAndTriggerImport()
{
	mInitialized = false;

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
					// Follow up with auto trigger of import
					if (mAutoTriggerImport)
					{
						mAutoTriggerImport = false;

						mImportInProgress = triggerImport();
					}
				}
				else if (mErrorInitSignal)
				{
					(*mErrorInitSignal)(LLMarketplaceImport::getResultStatus(), LLMarketplaceImport::getResults());
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

