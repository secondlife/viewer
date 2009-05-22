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

#include "llsys.h"
#include "llgl.h"
#include "llui.h"	// for tr()
#include "v3dmath.h"

#include "llcurl.h"
#include "llimagej2c.h"
#include "audioengine.h"

#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llversionviewer.h"
#include "llviewerbuild.h"
#include "lluictrlfactory.h"
#include "lluri.h"
#include "llweb.h"
#include "llsecondlifeurls.h"
#include "lltrans.h"
#include "llappviewer.h" 
#include "llglheaders.h"
#include "llmediamanager.h"


extern U32 gPacketsIn;

static std::string get_viewer_release_notes_url();


///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------

// Default constructor
LLFloaterAbout::LLFloaterAbout(const LLSD& key) 
:	LLFloater()
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_about.xml");
	center();
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
}

BOOL LLFloaterAbout::postBuild()
{
	LLViewerTextEditor *support_widget = 
		getChild<LLViewerTextEditor>("support_editor", true);

	LLViewerTextEditor *credits_widget = 
		getChild<LLViewerTextEditor>("credits_editor", true);

	// For some reason, adding style doesn't work unless this is true.
	support_widget->setParseHTML(TRUE);

	// Text styles for release notes hyperlinks
	LLStyleSP viewer_link_style(new LLStyle);
	viewer_link_style->setVisible(true);
	viewer_link_style->setFontName(LLStringUtil::null);
	viewer_link_style->setLinkHREF(get_viewer_release_notes_url());
	viewer_link_style->setColor(gSavedSkinSettings.getColor4("HTMLLinkColor"));

	// Version string
	std::string version = LLTrans::getString("SECOND_LIFE_VIEWER")
		+ llformat(" %d.%d.%d (%d) %s %s (%s)\n",
				   LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VIEWER_BUILD,
				   __DATE__, __TIME__,
				   gSavedSettings.getString("VersionChannelName").c_str());
	support_widget->appendColoredText(version, FALSE, FALSE, LLUI::sSettingGroups["color"]->getColor("TextFgReadOnlyColor"));
	support_widget->appendStyledText(LLTrans::getString("ReleaseNotes"), false, false, viewer_link_style);

	std::string support;
	support.append("\n\n");

#if LL_MSVC
    support.append(llformat("Built with MSVC version %d\n\n", _MSC_VER));
#endif

#if LL_GNUC
    support.append(llformat("Built with GCC version %d\n\n", GCC_VERSION));
#endif

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		LLStyleSP server_link_style(new LLStyle);
		server_link_style->setVisible(true);
		server_link_style->setFontName(LLStringUtil::null);
		server_link_style->setLinkHREF(region->getCapability("ServerReleaseNotes"));
		server_link_style->setColor(gSavedSkinSettings.getColor4("HTMLLinkColor"));

		const LLVector3d &pos = gAgent.getPositionGlobal();
		LLUIString pos_text = getString("you_are_at");
		pos_text.setArg("[POSITION]",
						llformat("%.1f, %.1f, %.1f ", pos.mdV[VX], pos.mdV[VY], pos.mdV[VZ]));
		support.append(pos_text);

		LLUIString region_text = getString ("in_region") + " ";
		region_text.setArg("[REGION]", llformat ("%s", gAgent.getRegion()->getName().c_str()));
		support.append(region_text);

		std::string buffer;
		buffer = gAgent.getRegion()->getHost().getHostName();
		support.append(buffer);
		support.append(" (");
		buffer = gAgent.getRegion()->getHost().getString();
		support.append(buffer);
		support.append(")\n");
		support.append(gLastVersionChannel);
		support.append("\n");

		support_widget->appendColoredText(support, FALSE, FALSE, LLUI::sSettingGroups["color"]->getColor("TextFgReadOnlyColor"));
		support_widget->appendStyledText(LLTrans::getString("ReleaseNotes"), false, false, server_link_style);

		support = "\n\n";
	}

	// *NOTE: Do not translate text like GPU, Graphics Card, etc -
	//  Most PC users that know what these mean will be used to the english versions,
	//  and this info sometimes gets sent to support
	
	// CPU
	support.append(getString("CPU") + " ");
	support.append( gSysCPU.getCPUString() );
	support.append("\n");

	U32 memory = gSysMemory.getPhysicalMemoryKB() / 1024;
	// Moved hack adjustment to Windows memory size into llsys.cpp

	LLStringUtil::format_map_t args;
	args["[MEM]"] = llformat ("%u", memory);
	support.append(getString("Memory", args) + "\n");

	support.append(getString("OSVersion") + " ");
	support.append( LLAppViewer::instance()->getOSInfo().getOSString() );
	support.append("\n");

	support.append(getString("GraphicsCardVendor") + " ");
	support.append( (const char*) glGetString(GL_VENDOR) );
	support.append("\n");

	support.append(getString("GraphicsCard") + " ");
	support.append( (const char*) glGetString(GL_RENDERER) );
	support.append("\n");

	support.append(getString("OpenGLVersion") + " ");
	support.append( (const char*) glGetString(GL_VERSION) );
	support.append("\n");

	support.append("\n");

	support.append(getString("LibCurlVersion") + " ");
	support.append( LLCurl::getVersionString() );
	support.append("\n");

	support.append(getString("J2CDecoderVersion") + " ");
	support.append( LLImageJ2C::getEngineInfo() );
	support.append("\n");

	support.append(getString("AudioDriverVersion") + " ");
	bool want_fullname = true;
	support.append( gAudiop ? gAudiop->getDriverName(want_fullname) : getString("none") );
	support.append("\n");

	LLMediaManager *mgr = LLMediaManager::getInstance();
	if (mgr)
	{
		LLMediaBase *media_source = mgr->createSourceFromMimeType("http", "text/html");
		if (media_source)
		{
			support.append(getString("LLMozLibVersion") + " ");
			support.append(media_source->getVersion());
			support.append("\n");
			mgr->destroySource(media_source);
		}
	}

	if (gPacketsIn > 0)
	{
		args["[LOST]"] = llformat ("%.0f", LLViewerStats::getInstance()->mPacketsLostStat.getCurrent());
		args["[IN]"] = llformat ("%.0f", F32(gPacketsIn));
		args["[PCT]"] = llformat ("%.1f", 100.f*LLViewerStats::getInstance()->mPacketsLostStat.getCurrent() / F32(gPacketsIn) );
		support.append(getString ("PacketsLost", args) + "\n");
	}

	support_widget->appendColoredText(support, FALSE, FALSE, LLUI::sSettingGroups["color"]->getColor("TextFgReadOnlyColor"));

	// Fix views
	support_widget->setCursorPos(0);
	support_widget->setEnabled(FALSE);
	support_widget->setTakesFocus(TRUE);
	support_widget->setHandleEditKeysDirectly(TRUE);

	credits_widget->setCursorPos(0);
	credits_widget->setEnabled(FALSE);
	credits_widget->setTakesFocus(TRUE);
	credits_widget->setHandleEditKeysDirectly(TRUE);

	return TRUE;
}


 static std::string get_viewer_release_notes_url()
 {
 	std::ostringstream version;
 	version << LL_VERSION_MAJOR << "."
 		<< LL_VERSION_MINOR << "."
 		<< LL_VERSION_PATCH << "."
 		<< LL_VERSION_BUILD;

 	LLSD query;
 	query["channel"] = gSavedSettings.getString("VersionChannelName");
 	query["version"] = version.str();

 	std::ostringstream url;
	 url << LLFloaterAbout::getInstance()->getString("RELEASE_NOTES_BASE_URL") << LLURI::mapToQueryString(query);

 	return url.str();
 }
