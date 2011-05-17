/** 
 * @file llviewerhelputil_test.cpp
 * @brief LLViewerHelpUtil tests
 * @author Tofu Linden
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
// Precompiled header
#include "../llviewerprecompiledheaders.h"

#include "../test/lltut.h"

#include "../llviewerhelputil.h"
#include "../llweb.h"
#include "llcontrol.h"

#include <iostream>

// values for all of the supported substitutions parameters
static std::string gHelpURL;
static std::string gVersion;
static std::string gChannel;
static std::string gLanguage;
static std::string gGrid;
static std::string gOS;

//----------------------------------------------------------------------------
// Mock objects for the dependencies of the code we're testing

LLControlGroup::LLControlGroup(const std::string& name)
	: LLInstanceTracker<LLControlGroup, std::string>(name) {}
LLControlGroup::~LLControlGroup() {}
BOOL LLControlGroup::declareString(const std::string& name,
				   const std::string& initial_val,
				   const std::string& comment,
				   BOOL persist) {return TRUE;}
void LLControlGroup::setString(const std::string& name, const std::string& val){}
std::string LLControlGroup::getString(const std::string& name)
{
	if (name == "HelpURLFormat")
		return gHelpURL;
	return "";
}
LLControlGroup gSavedSettings("test");

static void substitute_string(std::string &input, const std::string &search, const std::string &replace)
{
	size_t pos = input.find(search);
	while (pos != std::string::npos)
	{
		input = input.replace(pos, search.size(), replace);
		pos = input.find(search);
	}
}

#include "../llagent.h"
LLAgent::LLAgent() : mAgentAccess(NULL) { }
LLAgent::~LLAgent() { }
bool LLAgent::isGodlike() const { return FALSE; }

LLAgent gAgent;

std::string LLWeb::expandURLSubstitutions(const std::string &url,
										  const LLSD &default_subs)
{
	(void)gAgent.isGodlike(); // ref symbol to stop compiler from stripping it
	std::string new_url = url;
	substitute_string(new_url, "[TOPIC]", default_subs["TOPIC"].asString());
	substitute_string(new_url, "[VERSION]", gVersion);
	substitute_string(new_url, "[CHANNEL]", gChannel);
	substitute_string(new_url, "[LANGUAGE]", gLanguage);
	substitute_string(new_url, "[GRID]", gGrid);
	substitute_string(new_url, "[OS]", gOS);
	return new_url;
}


//----------------------------------------------------------------------------
	
namespace tut
{
    struct viewerhelputil
    {
    };
    
	typedef test_group<viewerhelputil> viewerhelputil_t;
	typedef viewerhelputil_t::object viewerhelputil_object_t;
	tut::viewerhelputil_t tut_viewerhelputil("LLViewerHelpUtil");

	template<> template<>
	void viewerhelputil_object_t::test<1>()
	{
		std::string topic("test_topic");
		std::string subresult;

		gHelpURL = "fooformat";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("no substitution tags", subresult, "fooformat");

		gHelpURL = "";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("blank substitution format", subresult, "");

		gHelpURL = "[TOPIC]";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("topic name", subresult, "test_topic");

		gHelpURL = "[LANGUAGE]";
		gLanguage = "";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("simple substitution with blank", subresult, "");

		gHelpURL = "[LANGUAGE]";
		gLanguage = "Esperanto";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("simple substitution", subresult, "Esperanto");

		gHelpURL = "http://secondlife.com/[LANGUAGE]";
		gLanguage = "Gaelic";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("simple substitution with url", subresult, "http://secondlife.com/Gaelic");

		gHelpURL = "[XXX]";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("unknown substitution", subresult, "[XXX]");

		gHelpURL = "[LANGUAGE]/[LANGUAGE]";
		gLanguage = "Esperanto";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("multiple substitution", subresult, "Esperanto/Esperanto");

		gHelpURL = "http://[CHANNEL]/[VERSION]/[LANGUAGE]/[OS]/[GRID]/[XXX]";
		gChannel = "Second Life Test";
		gVersion = "2.0";
		gLanguage = "gaelic";
		gOS = "AmigaOS 2.1";
		gGrid = "mysim";
		subresult = LLViewerHelpUtil::buildHelpURL(topic);
		ensure_equals("complex substitution", subresult, "http://Second Life Test/2.0/gaelic/AmigaOS 2.1/mysim/[XXX]");
	}
}
