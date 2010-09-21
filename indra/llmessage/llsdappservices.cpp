/** 
 * @file llsdappservices.cpp
 * @author Phoenix
 * @date 2006-09-12
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

#include "linden_common.h"
#include "llsdappservices.h"

#include "llapp.h"
#include "llhttpnode.h"
#include "llsdserialize.h"

void LLSDAppServices::useServices()
{
	/*
		Having this function body here, causes the classes and globals in this
		file to be linked into any program that uses the llmessage library.
	*/
}

class LLHTTPConfigService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("GET an array of all the options in priority order.");
		desc.getAPI();
		desc.source(__FILE__, __LINE__);
	}
    
	virtual LLSD simpleGet() const
	{
		LLSD result;
		LLApp* app = LLApp::instance();
		for(int ii = 0; ii < LLApp::PRIORITY_COUNT; ++ii)
		{
			result.append(app->getOptionData((LLApp::OptionPriority)ii));
		}
		return result;
	}
};

LLHTTPRegistration<LLHTTPConfigService>
	gHTTPRegistratiAppConfig("/app/config");

class LLHTTPConfigRuntimeService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Manipulate a map of runtime-override options.");
		desc.getAPI();
		desc.postAPI();
		desc.source(__FILE__, __LINE__);
	}
    
	virtual LLSD simpleGet() const
	{
		return LLApp::instance()->getOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE);
	}

	virtual void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD result = LLApp::instance()->getOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE);
		LLSD::map_const_iterator iter = input.beginMap();
		LLSD::map_const_iterator end = input.endMap();
		for(; iter != end; ++iter)
		{
			result[(*iter).first] = (*iter).second;
		}
		LLApp::instance()->setOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE,
			result);
		response->result(result);
	}
};

LLHTTPRegistration<LLHTTPConfigRuntimeService>
	gHTTPRegistrationRuntimeConfig("/app/config/runtime-override");

class LLHTTPConfigRuntimeSingleService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Manipulate a single runtime-override option.");
		desc.getAPI();
		desc.putAPI();
		desc.delAPI();
		desc.source(__FILE__, __LINE__);
	}
    
	virtual bool validate(const std::string& name, LLSD& context) const
	{
		//llinfos << "validate: " << name << ", "
		//	<< LLSDOStreamer<LLSDNotationFormatter>(context) << llendl;
		if((std::string("PUT") == context["request"]["verb"].asString()) && !name.empty())
		{
			return true;
		}
		else
		{
			// This is for GET and DELETE
			LLSD options = LLApp::instance()->getOptionData(
				LLApp::PRIORITY_RUNTIME_OVERRIDE);
			if(options.has(name)) return true;
			else return false;
		}
	}

	virtual void get(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context) const
	{
		std::string name = context["request"]["wildcard"]["option-name"];
		LLSD options = LLApp::instance()->getOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE);
		response->result(options[name]);
	}

	virtual void put(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		std::string name = context["request"]["wildcard"]["option-name"];
		LLSD options = LLApp::instance()->getOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE);
		options[name] = input;
		LLApp::instance()->setOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE,
			options);
		response->result(input);
	}

	virtual void del(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context) const
	{
		std::string name = context["request"]["wildcard"]["option-name"];
		LLSD options = LLApp::instance()->getOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE);
		options.erase(name);
		LLApp::instance()->setOptionData(
			LLApp::PRIORITY_RUNTIME_OVERRIDE,
			options);
		response->result(LLSD());
	}
};

LLHTTPRegistration<LLHTTPConfigRuntimeSingleService>
	gHTTPRegistrationRuntimeSingleConfig(
		"/app/config/runtime-override/<option-name>");

template<int PRIORITY>
class LLHTTPConfigPriorityService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Get a map of the options at this priority.");
		desc.getAPI();
		desc.source(__FILE__, __LINE__);
	}

	virtual void get(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context) const
	{
		response->result(LLApp::instance()->getOptionData(
			(LLApp::OptionPriority)PRIORITY));
	}
};

LLHTTPRegistration< LLHTTPConfigPriorityService<LLApp::PRIORITY_COMMAND_LINE> >
	gHTTPRegistrationCommandLineConfig("/app/config/command-line");
LLHTTPRegistration<
	LLHTTPConfigPriorityService<LLApp::PRIORITY_SPECIFIC_CONFIGURATION> >
	gHTTPRegistrationSpecificConfig("/app/config/specific");
LLHTTPRegistration<
	LLHTTPConfigPriorityService<LLApp::PRIORITY_GENERAL_CONFIGURATION> >
	gHTTPRegistrationGeneralConfig("/app/config/general");
LLHTTPRegistration< LLHTTPConfigPriorityService<LLApp::PRIORITY_DEFAULT> >
	gHTTPRegistrationDefaultConfig("/app/config/default");

class LLHTTPLiveConfigService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Get a map of the currently live options.");
		desc.getAPI();
		desc.source(__FILE__, __LINE__);
	}

	virtual void get(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context) const
	{
		LLSD result;
		LLApp* app = LLApp::instance();
		LLSD::map_const_iterator iter;
		LLSD::map_const_iterator end;
		for(int ii = LLApp::PRIORITY_COUNT - 1; ii >= 0; --ii)
		{
			LLSD options = app->getOptionData((LLApp::OptionPriority)ii);
			iter = options.beginMap();
			end = options.endMap();
			for(; iter != end; ++iter)
			{
				result[(*iter).first] = (*iter).second;
			}
		}
		response->result(result);
	}
};

LLHTTPRegistration<LLHTTPLiveConfigService>
	gHTTPRegistrationLiveConfig("/app/config/live");

class LLHTTPLiveConfigSingleService : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Get the named live option.");
		desc.getAPI();
		desc.source(__FILE__, __LINE__);
	}

	virtual bool validate(const std::string& name, LLSD& context) const
	{
		llinfos << "LLHTTPLiveConfigSingleService::validate(" << name
			<< ")" << llendl;
		LLSD option = LLApp::instance()->getOption(name);
		if(option.isDefined()) return true;
		else return false;
	}

	virtual void get(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context) const
	{
		std::string name = context["request"]["wildcard"]["option-name"];
		response->result(LLApp::instance()->getOption(name));
	}
};

LLHTTPRegistration<LLHTTPLiveConfigSingleService>
	gHTTPRegistrationLiveSingleConfig("/app/config/live/<option-name>");
