/** 
 * @file llviewerhelp.cpp
 * @brief Utility functions for the Help system
 * @author Soft Linden
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llversionviewer.h"
#include "llviewerbuild.h"

//#include "llfloaterhelpbrowser.h"
//#include "llfloaterreg.h"
//#include "llfocusmgr.h"
//#include "llviewercontrol.h"
//#include "llappviewer.h"

#include "llstring.h"
#include "lluri.h"
#include "llsys.h"

#include "llcontrol.h"

#include "llviewerhelputil.h"


//////////////////////////////////////////////
// Build a help URL from a topic and formatter

//static
std::string LLViewerHelpUtil::helpURLEncode( const std::string &component )
{
	// Every character rfc3986 allows as unreserved in 2.3, minus the tilde
	// which we may grant special meaning. Yay.
	const char* allowed =   
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789"
	"-._";
	std::string escaped = LLURI::escape(component, allowed);
	
	return escaped;
}

static std::string buildHelpVersion( const U32 ver_int )
{
	std::ostringstream ver_str;
	ver_str << ver_int;
	return ver_str.str(); // not encoded - numbers are rfc3986-safe
}

//static
std::string LLViewerHelpUtil::buildHelpURL( const std::string &topic,
					    LLControlGroup &savedSettings,
					    const LLOSInfo &osinfo )
{
	std::string helpURL = savedSettings.getString("HelpURLFormat");
	LLSD substitution;
	substitution["TOPIC"] = helpURLEncode(topic);
	
	substitution["CHANNEL"] = helpURLEncode(savedSettings.getString("VersionChannelName"));

	substitution["VERSION"] = helpURLEncode(llGetViewerVersion());
	substitution["VERSION_MAJOR"] = buildHelpVersion(LL_VERSION_MAJOR);
	substitution["VERSION_MINOR"] = buildHelpVersion(LL_VERSION_MINOR);
	substitution["VERSION_PATCH"] = buildHelpVersion(LL_VERSION_PATCH);
	substitution["VERSION_BUILD"] = buildHelpVersion(LL_VERSION_BUILD);
	
	substitution["OS"] = helpURLEncode(osinfo.getOSStringSimple());

	std::string language = savedSettings.getString("Language");
	if( language.empty() || language == "default" )
	{
		language = savedSettings.getString("SystemLanguage");
	}
	substitution["LANGUAGE"] = helpURLEncode(language);
		
	LLStringUtil::format(helpURL, substitution);

	return helpURL;
}
