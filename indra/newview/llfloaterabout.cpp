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
#include "llslurl.h"
#include "llvoiceclient.h"
#include "lluictrlfactory.h"
#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llversioninfo.h"
#include "llweb.h"

// Linden library includes
#include "llaudioengine.h"
#include "llbutton.h"
#include "llcurl.h"
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

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

extern LLMemoryInfo gSysMemory;
extern U32 gPacketsIn;

///----------------------------------------------------------------------------
/// Class LLServerReleaseNotesURLFetcher
///----------------------------------------------------------------------------
class LLServerReleaseNotesURLFetcher : public LLHTTPClient::Responder
{
	LOG_CLASS(LLServerReleaseNotesURLFetcher);
public:
	static void startFetch();
private:
	/* virtual */ void httpCompleted();
};

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

private:
	void setSupportText(const std::string& server_release_notes_url);
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

	LLViewerTextEditor *linden_names_widget = 
		getChild<LLViewerTextEditor>("linden_names", true);

	LLViewerTextEditor *contrib_names_widget = 
		getChild<LLViewerTextEditor>("contrib_names", true);

	LLViewerTextEditor *trans_names_widget = 
		getChild<LLViewerTextEditor>("trans_names", true);

	getChild<LLUICtrl>("copy_btn")->setCommitCallback(
		boost::bind(&LLFloaterAbout::onClickCopyToClipboard, this));

	if (gAgent.getRegion())
	{
		// start fetching server release notes URL
		setSupportText(LLTrans::getString("RetrievingData"));
		LLServerReleaseNotesURLFetcher::startFetch();
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

	// Get the names of Lindens, added by viewer_manifest.py at build time
	std::string lindens_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"lindens.txt");
	llifstream linden_file;
	std::string lindens;
	linden_file.open(lindens_path);		/* Flawfinder: ignore */
	if (linden_file.is_open())
	{
		std::getline(linden_file, lindens); // all names are on a single line
		linden_file.close();
		linden_names_widget->setText(lindens);
	}
	else
	{
		LL_INFOS("AboutInit") << "Could not read lindens file at " << lindens_path << LL_ENDL;
	}
	linden_names_widget->setEnabled(FALSE);
	linden_names_widget->startOfDoc();

	// Get the names of contributors, extracted from .../doc/contributions.txt by viewer_manifest.py at build time
	std::string contributors_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"contributors.txt");
	llifstream contrib_file;
	std::string contributors;
	contrib_file.open(contributors_path);		/* Flawfinder: ignore */
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

	// Get the names of translators, extracted from .../doc/tranlations.txt by viewer_manifest.py at build time
	std::string translators_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"translators.txt");
	llifstream trans_file;
	std::string translators;
	trans_file.open(translators_path);		/* Flawfinder: ignore */
	if (trans_file.is_open())
	{
		std::getline(trans_file, translators); // all names are on a single line
		trans_file.close();
	}
	else
	{
		LL_WARNS("AboutInit") << "Could not read translators file at " << translators_path << LL_ENDL;
	}
	trans_names_widget->setText(translators);
	trans_names_widget->setEnabled(FALSE);
	trans_names_widget->startOfDoc();

	return TRUE;
}

LLSD LLFloaterAbout::getInfo()
{
	return LLAppViewer::instance()->getViewerInfo();
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
/// LLFloaterAboutUtil
///----------------------------------------------------------------------------
void LLFloaterAboutUtil::registerFloater()
{
	LLFloaterReg::add("sl_about", "floater_about.xml",
		&LLFloaterReg::build<LLFloaterAbout>);

}

///----------------------------------------------------------------------------
/// Class LLServerReleaseNotesURLFetcher implementation
///----------------------------------------------------------------------------
// static
void LLServerReleaseNotesURLFetcher::startFetch()
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	// We cannot display the URL returned by the ServerReleaseNotes capability
	// because opening it in an external browser will trigger a warning about untrusted
	// SSL certificate.
	// So we query the URL ourselves, expecting to find
	// an URL suitable for external browsers in the "Location:" HTTP header.
	std::string cap_url = region->getCapability("ServerReleaseNotes");
	LLHTTPClient::get(cap_url, new LLServerReleaseNotesURLFetcher);
}

// virtual
void LLServerReleaseNotesURLFetcher::httpCompleted()
{
	LL_DEBUGS("ServerReleaseNotes") << dumpResponse() 
									<< " [headers:" << getResponseHeaders() << "]" << LL_ENDL;

	LLFloaterAbout* floater_about = LLFloaterReg::getTypedInstance<LLFloaterAbout>("sl_about");
	if (floater_about)
	{
		std::string location = getResponseHeader(HTTP_IN_HEADER_LOCATION);
		if (location.empty())
		{
			location = LLTrans::getString("ErrorFetchingServerReleaseNotesURL");
		}
		LLAppViewer::instance()->setServerReleaseNotesURL(location);
	}
}

