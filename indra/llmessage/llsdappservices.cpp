/** 
 * @file llsdappservices.cpp
 * @author Phoenix
 * @date 2006-09-12
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
        //LL_INFOS() << "validate: " << name << ", "
        //  << LLSDOStreamer<LLSDNotationFormatter>(context) << LL_ENDL;
        if((std::string("PUT") == context[CONTEXT_REQUEST][CONTEXT_VERB].asString()) && !name.empty())
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
        std::string name = context[CONTEXT_REQUEST][CONTEXT_WILDCARD]["option-name"];
        LLSD options = LLApp::instance()->getOptionData(
            LLApp::PRIORITY_RUNTIME_OVERRIDE);
        response->result(options[name]);
    }

    virtual void put(
        LLHTTPNode::ResponsePtr response,
        const LLSD& context,
        const LLSD& input) const
    {
        std::string name = context[CONTEXT_REQUEST][CONTEXT_WILDCARD]["option-name"];
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
        std::string name = context[CONTEXT_REQUEST][CONTEXT_WILDCARD]["option-name"];
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
        LL_INFOS() << "LLHTTPLiveConfigSingleService::validate(" << name
            << ")" << LL_ENDL;
        LLSD option = LLApp::instance()->getOption(name);
        if(option.isDefined()) return true;
        else return false;
    }

    virtual void get(
        LLHTTPNode::ResponsePtr response,
        const LLSD& context) const
    {
        std::string name = context[CONTEXT_REQUEST][CONTEXT_WILDCARD]["option-name"];
        response->result(LLApp::instance()->getOption(name));
    }
};

LLHTTPRegistration<LLHTTPLiveConfigSingleService>
    gHTTPRegistrationLiveSingleConfig("/app/config/live/<option-name>");
