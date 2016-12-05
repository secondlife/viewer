/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
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
#include <iostream>
#include <fstream>

#include "llfloaterabout.h"

// Viewer includes
#include "llagent.h"
#include "llagentui.h"
#include "llappviewer.h"
#include "llnotificationsutil.h"
#include "llslurl.h"
#include "llvoiceclient.h"
#include "lluictrlfactory.h"
#include "llupdaterservice.h"
#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llversioninfo.h"
#include "llweb.h"

// Linden library includes
#include "llaudioengine.h"
#include "llbutton.h"
#include "llglheaders.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llimagej2c.h"
#include "llsys.h"
#include "lltrans.h"
#include "lluri.h"
#include "v3dmath.h"
#include "llwindow.h"
#include "stringize.h"
#include "llsdutil_math.h"
#include "lleventapi.h"
#include "llcorehttputil.h"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

extern LLMemoryInfo gSysMemory;
extern U32 gPacketsIn;

///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------
class LLFloaterAbout 
	: public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterAbout(const LLSD& key);
	virtual ~LLFloaterAbout();

public:
	/*virtual*/ BOOL postBuild();

	/// Obtain the data used to fill out the contents string. This is
	/// separated so that we can programmatically access the same info.
	static LLSD getInfo();
	void onClickCopyToClipboard();
	void onClickUpdateCheck();

	// checks state of updater service and starts a check outside of schedule.
	// subscribes callback for closest state update
	static void setUpdateListener();

private:
	void setSupportText(const std::string& server_release_notes_url);

	// notifications for user requested checks
	static void showCheckUpdateNotification(S32 state);

	// callback method for manual checks
	static bool callbackCheckUpdate(LLSD const & event);

	// listener name for update checks
	static const std::string sCheckUpdateListenerName;
	
    static void startFetchServerReleaseNotes();
    static void fetchServerReleaseNotesCoro(const std::string& cap_url);
    static void handleServerReleaseNotes(LLSD results);
};


// Default constructor
LLFloaterAbout::LLFloaterAbout(const LLSD& key) 
:	LLFloater(key)
{
	
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
}

BOOL LLFloaterAbout::postBuild()
{
	center();
	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);

	LLViewerTextEditor *contrib_names_widget = 
		getChild<LLViewerTextEditor>("contrib_names", true);

	LLViewerTextEditor *licenses_widget = 
		getChild<LLViewerTextEditor>("licenses_editor", true);

	getChild<LLUICtrl>("copy_btn")->setCommitCallback(
		boost::bind(&LLFloaterAbout::onClickCopyToClipboard, this));

	getChild<LLUICtrl>("update_btn")->setCommitCallback(
		boost::bind(&LLFloaterAbout::onClickUpdateCheck, this));

	static const LLUIColor about_color = LLUIColorTable::instance().getColor("TextFgReadOnlyColor");

	if (gAgent.getRegion())
	{
		// start fetching server release notes URL
		setSupportText(LLTrans::getString("RetrievingData"));
        startFetchServerReleaseNotes();
	}
	else // not logged in
	{
		LL_DEBUGS("ViewerInfo") << "cannot display region info when not connected" << LL_ENDL;
		setSupportText(LLTrans::getString("NotConnected"));
	}

	support_widget->blockUndo();

	// Fix views
	support_widget->setEnabled(FALSE);
	support_widget->startOfDoc();

	// Get the names of contributors, extracted from .../doc/contributions.txt by viewer_manifest.py at build time
	std::string contributors_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"contributors.txt");
	llifstream contrib_file;
	std::string contributors;
	contrib_file.open(contributors_path.c_str());		/* Flawfinder: ignore */
	if (contrib_file.is_open())
	{
		std::getline(contrib_file, contributors); // all names are on a single line
		contrib_file.close();
	}
	else
	{
		LL_WARNS("AboutInit") << "Could not read contributors file at " << contributors_path << LL_ENDL;
	}
	contrib_names_widget->setText(contributors);
	contrib_names_widget->setEnabled(FALSE);
	contrib_names_widget->startOfDoc();

    // Get the Versions and Copyrights, created at build time
	std::string licenses_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"packages-info.txt");
	llifstream licenses_file;
	licenses_file.open(licenses_path.c_str());		/* Flawfinder: ignore */
	if (licenses_file.is_open())
	{
		std::string license_line;
		licenses_widget->clear();
		while ( std::getline(licenses_file, license_line) )
		{
			licenses_widget->appendText(license_line+"\n", FALSE,
										LLStyle::Params() .color(about_color));
		}
		licenses_file.close();
	}
	else
	{
		// this case will use the (out of date) hard coded value from the XUI
		LL_INFOS("AboutInit") << "Could not read licenses file at " << licenses_path << LL_ENDL;
	}
	licenses_widget->setEnabled(FALSE);
	licenses_widget->startOfDoc();

	return TRUE;
}

LLSD LLFloaterAbout::getInfo()
{
	return LLAppViewer::instance()->getViewerInfo();
}

/*static*/
void LLFloaterAbout::startFetchServerReleaseNotes()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (!region) return;

    // We cannot display the URL returned by the ServerReleaseNotes capability
    // because opening it in an external browser will trigger a warning about untrusted
    // SSL certificate.
    // So we query the URL ourselves, expecting to find
    // an URL suitable for external browsers in the "Location:" HTTP header.
    std::string cap_url = region->getCapability("ServerReleaseNotes");

    LLCoros::instance().launch("fetchServerReleaseNotesCoro", boost::bind(&LLFloaterAbout::fetchServerReleaseNotesCoro, cap_url));

}

/*static*/
void LLFloaterAbout::fetchServerReleaseNotesCoro(const std::string& cap_url)
{
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("fetchServerReleaseNotesCoro", LLCore::HttpRequest::DEFAULT_POLICY_ID));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, cap_url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        handleServerReleaseNotes(httpResults);
    }
    else
    {
        handleServerReleaseNotes(result);
    }
}

/*static*/
void LLFloaterAbout::handleServerReleaseNotes(LLSD results)
{
    LLSD http_headers;
    if (results.has(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS))
    {
        LLSD http_results = results[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        http_headers = http_results[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
    }
    else
    {
        http_headers = results[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];
    }

    std::string location = http_headers[HTTP_IN_HEADER_LOCATION].asString();
    if (location.empty())
    {
        location = LLTrans::getString("ErrorFetchingServerReleaseNotesURL");
    }
    LLAppViewer::instance()->setServerReleaseNotesURL(location);

    LLFloaterAbout* floater_about = LLFloaterReg::findTypedInstance<LLFloaterAbout>("sl_about");
    if (floater_about)
    {
        floater_about->setSupportText(location);
    }
}

class LLFloaterAboutListener: public LLEventAPI
{
public:
	LLFloaterAboutListener():
		LLEventAPI("LLFloaterAbout",
                   "LLFloaterAbout listener to retrieve About box info")
	{
		add("getInfo",
            "Request an LLSD::Map containing information used to populate About box",
            &LLFloaterAboutListener::getInfo,
            LLSD().with("reply", LLSD()));
	}

private:
	void getInfo(const LLSD& request) const
	{
		LLReqID reqid(request);
		LLSD reply(LLFloaterAbout::getInfo());
		reqid.stamp(reply);
		LLEventPumps::instance().obtain(request["reply"]).post(reply);
	}
};

static LLFloaterAboutListener floaterAboutListener;

void LLFloaterAbout::onClickCopyToClipboard()
{
	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);
	support_widget->selectAll();
	support_widget->copy();
	support_widget->deselect();
}

void LLFloaterAbout::onClickUpdateCheck()
{
	setUpdateListener();
}

void LLFloaterAbout::setSupportText(const std::string& server_release_notes_url)
{
#if LL_WINDOWS
	getWindow()->incBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif
#if LL_WINDOWS
	getWindow()->decBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif

	LLViewerTextEditor *support_widget =
		getChild<LLViewerTextEditor>("support_editor", true);

	LLUIColor about_color = LLUIColorTable::instance().getColor("TextFgReadOnlyColor");
	support_widget->clear();
	support_widget->appendText(LLAppViewer::instance()->getViewerInfoString(),
							   FALSE, LLStyle::Params() .color(about_color));
}

///----------------------------------------------------------------------------
/// Floater About Update-check related functions
///----------------------------------------------------------------------------

const std::string LLFloaterAbout::sCheckUpdateListenerName = "LLUpdateNotificationListener";

void LLFloaterAbout::showCheckUpdateNotification(S32 state)
{
	switch (state)
	{
	case LLUpdaterService::UP_TO_DATE:
		LLNotificationsUtil::add("UpdateViewerUpToDate");
		break;
	case LLUpdaterService::DOWNLOADING:
	case LLUpdaterService::INSTALLING:
		LLNotificationsUtil::add("UpdateDownloadInProgress");
		break;
	case LLUpdaterService::TERMINAL:
		// download complete, user triggered check after download pop-up appeared
		LLNotificationsUtil::add("UpdateDownloadComplete");
		break;
	default:
		LLNotificationsUtil::add("UpdateCheckError");
		break;
	}
}

bool LLFloaterAbout::callbackCheckUpdate(LLSD const & event)
{
	if (!event.has("payload"))
	{
		return false;
	}

	LLSD payload = event["payload"];
	if (payload.has("type") && payload["type"].asInteger() == LLUpdaterService::STATE_CHANGE)
	{
		LLEventPumps::instance().obtain("mainlooprepeater").stopListening(sCheckUpdateListenerName);
		showCheckUpdateNotification(payload["state"].asInteger());
	}
	return false;
}

void LLFloaterAbout::setUpdateListener()
{
	LLUpdaterService update_service;
	S32 service_state = update_service.getState();
	// Note: Do not set state listener before forceCheck() since it set's new state
	if (update_service.forceCheck() || service_state == LLUpdaterService::CHECKING_FOR_UPDATE)
	{
		LLEventPump& mainloop(LLEventPumps::instance().obtain("mainlooprepeater"));
		if (mainloop.getListener(sCheckUpdateListenerName) == LLBoundListener()) // dummy listener
		{
			mainloop.listen(sCheckUpdateListenerName, boost::bind(&callbackCheckUpdate, _1));
		}
	}
	else
	{
		showCheckUpdateNotification(service_state);
	}
}

///----------------------------------------------------------------------------
/// LLFloaterAboutUtil
///----------------------------------------------------------------------------
void LLFloaterAboutUtil::registerFloater()
{
	LLFloaterReg::add("sl_about", "floater_about.xml",
		&LLFloaterReg::build<LLFloaterAbout>);

}

void LLFloaterAboutUtil::checkUpdatesAndNotify()
{
	LLFloaterAbout::setUpdateListener();
}

