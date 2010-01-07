/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llfloaterabout.h"

// Viewer includes
#include "llagent.h"
#include "llappviewer.h" 
#include "llsecondlifeurls.h"
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

static std::string get_viewer_release_notes_url();


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
};


// Default constructor
LLFloaterAbout::LLFloaterAbout(const LLSD& key) 
:	LLFloater(key)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_about.xml");
	
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

	LLViewerTextEditor *credits_widget = 
		getChild<LLViewerTextEditor>("credits_editor", true);

	getChild<LLUICtrl>("copy_btn")->setCommitCallback(
		boost::bind(&LLFloaterAbout::onClickCopyToClipboard, this));

#if LL_WINDOWS
	getWindow()->incBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif
	LLSD info(getInfo());
#if LL_WINDOWS
	getWindow()->decBusyCount();
	getWindow()->setCursor(UI_CURSOR_ARROW);
#endif

	std::ostringstream support;

	// Render the LLSD from getInfo() as a format_map_t
	LLStringUtil::format_map_t args;
	// For reasons I don't yet understand, [ReleaseNotes] is not part of the
	// default substitution strings whereas [APP_NAME] is. But it works to
	// simply copy it into these specific args.
	args["ReleaseNotes"] = LLTrans::getString("ReleaseNotes");
	for (LLSD::map_const_iterator ii(info.beginMap()), iend(info.endMap());
		 ii != iend; ++ii)
	{
		if (! ii->second.isArray())
		{
			// Scalar value
			if (ii->second.isUndefined())
			{
				args[ii->first] = getString("none");
			}
			else
			{
				// don't forget to render value asString()
				args[ii->first] = ii->second.asString();
			}
		}
		else
		{
			// array value: build KEY_0, KEY_1 etc. entries
			for (LLSD::Integer n(0), size(ii->second.size()); n < size; ++n)
			{
				args[STRINGIZE(ii->first << '_' << n)] = ii->second[n].asString();
			}
		}
	}

	// Now build the various pieces
	support << getString("AboutHeader", args);
	if (info.has("REGION"))
	{
		support << "\n\n" << getString("AboutPosition", args);
	}
	support << "\n\n" << getString("AboutSystem", args);
	support << "\n";
	if (info.has("GRAPHICS_DRIVER_VERSION"))
	{
		support << "\n" << getString("AboutDriver", args);
	}
	support << "\n" << getString("AboutLibs", args);
	if (info.has("COMPILER"))
	{
		support << "\n" << getString("AboutCompiler", args);
	}
	if (info.has("PACKETS_IN"))
	{
		support << '\n' << getString("AboutTraffic", args);
	}

	support_widget->appendText(support.str(), 
								FALSE, 
								LLStyle::Params()
									.color(LLUIColorTable::instance().getColor("TextFgReadOnlyColor")));
	support_widget->blockUndo();

	// Fix views
	support_widget->setEnabled(FALSE);
	support_widget->startOfDoc();

	credits_widget->setEnabled(FALSE);
	credits_widget->startOfDoc();

	return TRUE;
}

// static
LLSD LLFloaterAbout::getInfo()
{
	// The point of having one method build an LLSD info block and the other
	// construct the user-visible About string is to ensure that the same info
	// is available to a getInfo() caller as to the user opening
	// LLFloaterAbout.
	LLSD info;
	LLSD version;
	version.append(LLVersionInfo::getMajor());
	version.append(LLVersionInfo::getMinor());
	version.append(LLVersionInfo::getPatch());
	version.append(LLVersionInfo::getBuild());
	info["VIEWER_VERSION"] = version;
	info["VIEWER_VERSION_STR"] = LLVersionInfo::getVersion();
	info["BUILD_DATE"] = __DATE__;
	info["BUILD_TIME"] = __TIME__;
	info["CHANNEL"] = gSavedSettings.getString("VersionChannelName");

	info["VIEWER_RELEASE_NOTES_URL"] = get_viewer_release_notes_url();

#if LL_MSVC
	info["COMPILER"] = "MSVC";
	info["COMPILER_VERSION"] = _MSC_VER;
#elif LL_GNUC
	info["COMPILER"] = "GCC";
	info["COMPILER_VERSION"] = GCC_VERSION;
#endif

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		const LLVector3d &pos = gAgent.getPositionGlobal();
		info["POSITION"] = ll_sd_from_vector3d(pos);
		info["REGION"] = gAgent.getRegion()->getName();
		info["HOSTNAME"] = gAgent.getRegion()->getHost().getHostName();
		info["HOSTIP"] = gAgent.getRegion()->getHost().getString();
		info["SERVER_VERSION"] = gLastVersionChannel;
		info["SERVER_RELEASE_NOTES_URL"] = LLWeb::escapeURL(region->getCapability("ServerReleaseNotes"));
	}

	// CPU
	info["CPU"] = gSysCPU.getCPUString();
	info["MEMORY_MB"] = LLSD::Integer(gSysMemory.getPhysicalMemoryKB() / 1024);
	// Moved hack adjustment to Windows memory size into llsys.cpp
	info["OS_VERSION"] = LLAppViewer::instance()->getOSInfo().getOSString();
	info["GRAPHICS_CARD_VENDOR"] = (const char*)(glGetString(GL_VENDOR));
	info["GRAPHICS_CARD"] = (const char*)(glGetString(GL_RENDERER));

#if LL_WINDOWS
    LLSD driver_info = gDXHardware.getDisplayInfo();
    if (driver_info.has("DriverVersion"))
    {
        info["GRAPHICS_DRIVER_VERSION"] = driver_info["DriverVersion"];
    }
#endif

	info["OPENGL_VERSION"] = (const char*)(glGetString(GL_VERSION));
	info["LIBCURL_VERSION"] = LLCurl::getVersionString();
	info["J2C_VERSION"] = LLImageJ2C::getEngineInfo();
	bool want_fullname = true;
	info["AUDIO_DRIVER_VERSION"] = gAudiop ? LLSD(gAudiop->getDriverName(want_fullname)) : LLSD();
	info["VIVOX_VERSION"] = gVoiceClient ? gVoiceClient->getAPIVersion() : LLTrans::getString("NotConnected");

	// TODO: Implement media plugin version query
	info["QT_WEBKIT_VERSION"] = "4.5.2 (version number hard-coded)";

	if (gPacketsIn > 0)
	{
		info["PACKETS_LOST"] = LLViewerStats::getInstance()->mPacketsLostStat.getCurrent();
		info["PACKETS_IN"] = F32(gPacketsIn);
		info["PACKETS_PCT"] = 100.f*info["PACKETS_LOST"].asReal() / info["PACKETS_IN"].asReal();
	}

    return info;
}

static std::string get_viewer_release_notes_url()
{
	LLSD query;
	query["channel"] = gSavedSettings.getString("VersionChannelName");
	query["version"] = LLVersionInfo::getVersion();

	std::ostringstream url;
	url << LLTrans::getString("RELEASE_NOTES_BASE_URL") << LLURI::mapToQueryString(query);

	return LLWeb::escapeURL(url.str());
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

///----------------------------------------------------------------------------
/// LLFloaterAboutUtil
///----------------------------------------------------------------------------
void LLFloaterAboutUtil::registerFloater()
{
	LLFloaterReg::add("sl_about", "floater_about.xml",
		&LLFloaterReg::build<LLFloaterAbout>);

}
