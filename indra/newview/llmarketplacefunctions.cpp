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
#include "llviewermedia.h"
#include "llviewernetwork.h"


//
// Helpers
//

namespace LLMarketplaceImport
{
	// Basic interface for this namespace

	bool inProgress();
	bool resultPending();
	U32 getResultStatus();
	const LLSD& getResults();

	void establishMarketplaceSessionCookie();
	void pollStatus();
	void triggerImport();
	
	// Internal state variables

	static std::string sMarketplaceCookie = "";
	static bool sImportInProgress = false;
	static bool sImportPostPending = false;
	static bool sImportGetPending = false;
	static U32 sImportResultStatus = 0;
	static LLSD sImportResults = LLSD::emptyMap();
		
	
	// Internal helper functions
	
	std::string getBaseURL()
	{
		std::string url = "https://marketplace.secondlife.com/";

		if (!LLGridManager::getInstance()->isInProductionGrid())
		{
			std::string gridLabel = utf8str_tolower(LLGridManager::getInstance()->getGridLabel());
			
			if (gridLabel == "damballah")
			{
				url = "https://marketplace.secondlife-staging.com/";
			}
			else
			{
				url = llformat("https://marketplace.%s.lindenlab.com/", gridLabel.c_str());
			}
		}

		url += "api/1/";
		url += gAgent.getID().getString();
		url += "/inventory/";

		return url;
	}

	std::string getInventoryImportURL()
	{
		std::string url = getBaseURL();

		url += "import/";

		return url;
	}
	
	// Responders
	
	class LLImportPostResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportPostResponder() : LLCurl::Responder() {}
		
		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_DONE);
			sImportPostPending = false;
			sImportResultStatus = status;
		}
	};
	
	class LLImportGetResponder : public LLHTTPClient::Responder
	{
	public:
		LLImportGetResponder() : LLCurl::Responder() {}
		
		void completedHeader(U32 status, const std::string& reason, const LLSD& content)
		{
			sMarketplaceCookie = content["set-cookie"].asString();
		}
		
		void completed(U32 status, const std::string& reason, const LLSD& content)
		{
			sImportInProgress = (status == MarketplaceErrorCodes::IMPORT_PROCESSING);
			sImportGetPending = false;
			sImportResultStatus = status;
			sImportResults = content;
		}
	};
	
	// Coroutine testing
/*
	std::string gTimeDelayDebugFunc = "";
	
	void timeDelay(LLCoros::self& self, LLPanelMarketplaceOutbox* outboxPanel)
	{
		waitForEventOn(self, "mainloop");
		
		LLTimer delayTimer;
		delayTimer.reset();
		delayTimer.setTimerExpirySec(5.0f);
		
		while (!delayTimer.hasExpired())
		{
			waitForEventOn(self, "mainloop");
		}
		
		outboxPanel->onImportPostComplete(MarketplaceErrorCodes::IMPORT_DONE, LLSD::emptyMap());
		
		gTimeDelayDebugFunc = "";
	}
	
	std::string gImportPollingFunc = "";
	
	void importPoll(LLCoros::self& self, LLPanelMarketplaceOutbox* outboxPanel)
	{
		waitForEventOn(self, "mainloop");
		
		while (outboxPanel->isImportInProgress())
		{
			LLTimer delayTimer;
			delayTimer.reset();
			delayTimer.setTimerExpirySec(5.0f);
			
			while (!delayTimer.hasExpired())
			{
				waitForEventOn(self, "mainloop");
			}
			
			//outboxPanel->
		}
		
		gImportPollingFunc = "";
	}
	
*/	
	
	// Basic API

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
	
	void establishMarketplaceSessionCookie()
	{
		sImportInProgress = true;
		sImportGetPending = true;
		
		std::string url = getInventoryImportURL();
		
		LLHTTPClient::get(url, new LLImportGetResponder(), LLViewerMedia::getHeaders());
	}
	
	void pollStatus()
	{
		sImportGetPending = true;

		std::string url = getInventoryImportURL();

		// Make the headers for the post
		LLSD headers = LLSD::emptyMap();
		headers["Accept"] = "*/*";
		headers["Cookie"] = sMarketplaceCookie;
		headers["Content-Type"] = "application/xml";
		headers["User-Agent"] = LLViewerMedia::getCurrentUserAgent();
		
		LLHTTPClient::get(url, new LLImportGetResponder(), headers);
	}
	
	void triggerImport()
	{
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
		
		LLHTTPClient::post(url, LLSD(), new LLImportPostResponder(), headers);
		
		// Set a timer (for testing only)
		//gTimeDelayDebugFunc = LLCoros::instance().launch("LLPanelMarketplaceOutbox timeDelay", boost::bind(&timeDelay, _1, this));
	}
}


//
// Interface class
//


//static
void LLMarketplaceInventoryImporter::update()
{
	if (instanceExists())
	{
		LLMarketplaceInventoryImporter::instance().updateImport();
	}
}

LLMarketplaceInventoryImporter::LLMarketplaceInventoryImporter()
	: mImportInProgress(false)
	, mInitialized(false)
	, mStatusChangedSignal(NULL)
	, mStatusReportSignal(NULL)
{
}

void LLMarketplaceInventoryImporter::initialize()
{
	if (!mInitialized)
	{
		LLMarketplaceImport::establishMarketplaceSessionCookie();
	}
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

bool LLMarketplaceInventoryImporter::triggerImport()
{
	LLMarketplaceImport::triggerImport();
	
	return LLMarketplaceImport::inProgress();
}

void LLMarketplaceInventoryImporter::updateImport()
{
	const bool in_progress = LLMarketplaceImport::inProgress();
	
	if (in_progress && !LLMarketplaceImport::resultPending())
	{
		LLMarketplaceImport::pollStatus();
	}	
	
	if (mImportInProgress != in_progress)
	{
		mImportInProgress = in_progress;
		
		if (mStatusChangedSignal)
		{
			(*mStatusChangedSignal)(mImportInProgress);
		}
		
		// If we are no longer in progress, report results
		if (!mImportInProgress && mStatusReportSignal)
		{
			if (mInitialized)
			{
				(*mStatusReportSignal)(LLMarketplaceImport::getResultStatus(), LLMarketplaceImport::getResults());
			}
			else
			{
				mInitialized = true;
			}
		}
	}
}

