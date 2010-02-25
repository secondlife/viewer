/** 
 * @file llviewerhelputil_test.cpp
 * @brief LLViewerHelpUtil tests
 * @author Tofu Linden
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

class LLAgent
{
public:
	LLAgent() {}
	~LLAgent() {}
	BOOL isGodlike() const { return FALSE; }
private:
	int dummy;
};
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
	tut::viewerhelputil_t tut_viewerhelputil("viewerhelputil");

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
