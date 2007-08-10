/** 
 * @file llfloaterabout.cpp
 * @author James Cook
 * @brief The about box from Help->About
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
 
#include "llviewerprecompiledheaders.h"

#include "llfloaterabout.h"

#include "llsys.h"
#include "llgl.h"
#include "llui.h"	// for tr()
#include "v3dmath.h"

#include "llviewertexteditor.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llviewerstats.h"
#include "llviewerregion.h"
#include "llversionviewer.h"
#include "llviewerbuild.h"
#include "llvieweruictrlfactory.h"
#include "viewer.h"	// for gViewerDigest

#if LL_LIBXUL_ENABLED
#include "llmozlib.h"
#endif // LL_LIBXUL_ENABLED

#include "llglheaders.h"

extern LLCPUInfo gSysCPU;
extern LLMemoryInfo gSysMemory;
extern LLOSInfo gSysOS;
extern U32 gPacketsIn;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

LLFloaterAbout* LLFloaterAbout::sInstance = NULL;

///----------------------------------------------------------------------------
/// Class LLFloaterAbout
///----------------------------------------------------------------------------

// Default constructor
LLFloaterAbout::LLFloaterAbout() 
:	LLFloater("floater_about", "FloaterAboutRect", "")
{
	gUICtrlFactory->buildFloater(this, "floater_about.xml");

	// Support for changing product name.
	LLString title("About ");
	title += gSecondLife;
	setTitle(title);

	LLString support;

	// Version string
	LLString version = gSecondLife
		+ llformat(" %d.%d.%d (%d) %s %s (%s)",
				   LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VIEWER_BUILD,
				   __DATE__, __TIME__,
				   gChannelName.c_str());
	support.append(version);
	support.append("\n\n");

	// Position
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		//XUI:translate
		const LLVector3d &pos = gAgent.getPositionGlobal();
		LLString pos_text = llformat("You are at %.1f, %.1f, %.1f ", 
			pos.mdV[VX], pos.mdV[VY], pos.mdV[VZ]);
		support.append(pos_text);

		LLString region_text = llformat("in %s located at ",
				gAgent.getRegion()->getName().c_str());
		support.append(region_text);

		char buffer[MAX_STRING];		/*Flawfinder: ignore*/
		gAgent.getRegion()->getHost().getHostName(buffer, MAX_STRING);
		support.append(buffer);
		support.append(" (");
		gAgent.getRegion()->getHost().getString(buffer, MAX_STRING);
		support.append(buffer);
		support.append(")\n");
		support.append(gLastVersionChannel);
		support.append("\n\n");
	}

	// CPU
	support.append("CPU: ");
	support.append( gSysCPU.getCPUString() );
	support.append("\n");

	U32 memory = gSysMemory.getPhysicalMemory() / 1024 / 1024;
	// For some reason, the reported amount of memory is always wrong by one meg
	memory++;

	LLString mem_text = llformat("Memory: %u MB\n", memory );
	support.append(mem_text);

	support.append("OS Version: ");
	support.append( gSysOS.getOSString().c_str() );
	support.append("\n");

	support.append("Graphics Card Vendor: ");
	support.append( (const char*) glGetString(GL_VENDOR) );
	support.append("\n");

	support.append("Graphics Card: ");
	support.append( (const char*) glGetString(GL_RENDERER) );
	support.append("\n");

	support.append("OpenGL Version: ");
	support.append( (const char*) glGetString(GL_VERSION) );
	support.append("\n");

#if LL_LIBXUL_ENABLED
	support.append("LLMozLib Version: ");
	support.append( (const char*) LLMozLib::getInstance()->getVersion().c_str() );
	support.append("\n");
#endif // LL_LIBXUL_ENABLED

	if (gViewerStats
		&& gPacketsIn > 0)
	{
		LLString packet_loss = llformat("Packets Lost: %.0f/%.0f (%.1f%%)", 
			gViewerStats->mPacketsLostStat.getCurrent(),
			F32(gPacketsIn),
			100.f*gViewerStats->mPacketsLostStat.getCurrent() / F32(gPacketsIn) );
		support.append(packet_loss);
		support.append("\n");
	}

	// MD5 digest of executable
	support.append("Viewer Digest: ");
	char viewer_digest_string[UUID_STR_LENGTH]; /*Flawfinder: ignore*/
	gViewerDigest.toString( viewer_digest_string );
	support.append(viewer_digest_string);

	// Fix views
	childDisable("credits_editor");
	childDisable("support_editor");
	childSetText("support_editor", support);

	center();

	sInstance = this;
}

// Destroys the object
LLFloaterAbout::~LLFloaterAbout()
{
	sInstance = NULL;
}

// static
void LLFloaterAbout::show(void*)
{
	if (!sInstance)
	{
		sInstance = new LLFloaterAbout();
	}

	sInstance->open();	 /*Flawfinder: ignore*/
}
