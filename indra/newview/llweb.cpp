/** 
 * @file llweb.cpp
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llweb.h"

// Library includes
#include "llwindow.h"	// spawnWebBrowser()

#include "llagent.h"
#include "llappviewer.h"
#include "llfloatermediabrowser.h"
#include "llfloaterreg.h"
#include "lllogininstance.h"
#include "llparcel.h"
#include "llsd.h"
#include "lltoastalertpanel.h"
#include "llui.h"
#include "lluri.h"
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

class URLLoader : public LLToastAlertPanel::URLLoader
{
	virtual void load(const std::string& url , bool force_open_externally)
	{
		if (force_open_externally)
		{
			LLWeb::loadURLExternal(url);
		}
		else
		{
			LLWeb::loadURL(url);
		}
	}
};
static URLLoader sAlertURLLoader;


// static
void LLWeb::initClass()
{
	LLToastAlertPanel::setURLLoader(&sAlertURLLoader);
}


// static
void LLWeb::loadURL(const std::string& url)
{
	if (gSavedSettings.getBOOL("UseExternalBrowser"))
	{
		loadURLExternal(url);
	}
	else
	{
		loadURLInternal(url);
	}
}


// static
void LLWeb::loadURLInternal(const std::string &url)
{
	LLFloaterReg::showInstance("media_browser", url);
}


// static
void LLWeb::loadURLExternal(const std::string& url)
{
	std::string escaped_url = escapeURL(url);
	gViewerWindow->getWindow()->spawnWebBrowser(escaped_url);
}


// static
std::string LLWeb::escapeURL(const std::string& url)
{
	// The CURL curl_escape() function escapes colons, slashes,
	// and all characters but A-Z and 0-9.  Do a cheesy mini-escape.
	std::string escaped_url;
	S32 len = url.length();
	for (S32 i = 0; i < len; i++)
	{
		char c = url[i];
		if (c == ' ')
		{
			escaped_url += "%20";
		}
		else if (c == '\\')
		{
			escaped_url += "%5C";
		}
		else
		{
			escaped_url += c;
		}
	}
	return escaped_url;
}

//static
std::string LLWeb::expandURLSubstitutions(const std::string &url,
										  const LLSD &default_subs)
{
	LLSD substitution = default_subs;
	substitution["VERSION"] = LLVersionInfo::getVersion();
	substitution["VERSION_MAJOR"] = LLVersionInfo::getMajor();
	substitution["VERSION_MINOR"] = LLVersionInfo::getMinor();
	substitution["VERSION_PATCH"] = LLVersionInfo::getPatch();
	substitution["VERSION_BUILD"] = LLVersionInfo::getBuild();
	substitution["CHANNEL"] = LLVersionInfo::getChannel();
	substitution["GRID"] = LLViewerLogin::getInstance()->getGridLabel();
	substitution["OS"] = LLAppViewer::instance()->getOSInfo().getOSStringSimple();
	substitution["SESSION_ID"] = gAgent.getSessionID();

	// work out the current language
	std::string lang = LLUI::getLanguage();
	if (lang == "en-us")
	{
		// *HACK: the correct fix is to change English.lproj/language.txt,
		// but we're late in the release cycle and this is a less risky fix
		lang = "en";
	}
	substitution["LANGUAGE"] = lang;

	// find the region ID
	LLUUID region_id;
	LLViewerRegion *region = gAgent.getRegion();
	if (region)
	{
		region_id = region->getRegionID();
	}
	substitution["REGION_ID"] = region_id;

	// find the parcel local ID
	S32 parcel_id = 0;
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (parcel)
	{
		parcel_id = parcel->getLocalID();
	}
	substitution["PARCEL_ID"] = llformat("%d", parcel_id);

	// expand all of the substitution strings and escape the url
	std::string expanded_url = url;
	LLStringUtil::format(expanded_url, substitution);

	return LLWeb::escapeURL(expanded_url);
}
