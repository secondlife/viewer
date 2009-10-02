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
#include "../test/lltut.h"

#include "../llviewerhelputil.h"
#include "llcontrol.h"
#include "llsys.h"

#include <iostream>

//----------------------------------------------------------------------------
// Implementation of enough of LLControlGroup to support the tests:

static std::map<std::string,std::string> test_stringvec;

LLControlGroup::LLControlGroup(const std::string& name)
	: LLInstanceTracker<LLControlGroup, std::string>(name)
{
}

LLControlGroup::~LLControlGroup()
{
}

// Implementation of just the LLControlGroup methods we requre
BOOL LLControlGroup::declareString(const std::string& name,
				   const std::string& initial_val,
				   const std::string& comment,
				   BOOL persist)
{
	test_stringvec[name] = initial_val;
	return true;
}

void LLControlGroup::setString(const std::string& name, const std::string& val)
{
	test_stringvec[name] = val;
}

std::string LLControlGroup::getString(const std::string& name)
{
	return test_stringvec[name];
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
		LLOSInfo osinfo;
		LLControlGroup cgr("test");
		cgr.declareString("HelpURLFormat", "fooformat", "declared_for_test", FALSE);
		cgr.declareString("VersionChannelName", "foochannelname", "declared_for_test", FALSE);
		cgr.declareString("Language", "foolanguage", "declared_for_test", FALSE);
		std::string topic("test_topic");

		std::string subresult;

		cgr.setString("HelpURLFormat", "fooformat");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("no substitution tags", subresult, "fooformat");

		cgr.setString("HelpURLFormat", "");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("blank substitution format", subresult, "");

		cgr.setString("HelpURLFormat", "[LANGUAGE]");
		cgr.setString("Language", "");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("simple substitution with blank", subresult, "");

		cgr.setString("HelpURLFormat", "[LANGUAGE]");
		cgr.setString("Language", "Esperanto");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("simple substitution", subresult, "Esperanto");

		cgr.setString("HelpURLFormat", "[XXX]");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("unknown substitution", subresult, "[XXX]");

		cgr.setString("HelpURLFormat", "[LANGUAGE]/[LANGUAGE]");
		cgr.setString("Language", "Esperanto");
		subresult = LLViewerHelpUtil::buildHelpURL(topic, cgr, osinfo);
		ensure_equals("multiple substitution", subresult, "Esperanto/Esperanto");
	}
    
}
