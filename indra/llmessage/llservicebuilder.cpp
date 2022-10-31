/** 
 * @file llservicebuilder.cpp
 * @brief Implementation of the LLServiceBuilder class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "llapp.h"
#include "llfile.h"
#include "llservicebuilder.h"
#include "llsd.h"
#include "llsdserialize.h"

void LLServiceBuilder::loadServiceDefinitionsFromFile(
    const std::string& service_filename)
{
    llifstream service_file(service_filename.c_str(), std::ios::binary);
    if(service_file.is_open())
    {
        LLSD service_data;
        LLSDSerialize::fromXMLDocument(service_data, service_file);
        service_file.close();
        // Load service 
        LLSD service_map = service_data["services"];
        for(LLSD::array_iterator array_itr = service_map.beginArray();
            array_itr != service_map.endArray();
            ++array_itr)
        {   
            LLSD service_llsd = (*array_itr)["service-builder"];
            std::string service_name = (*array_itr)["name"].asString();
            createServiceDefinition(service_name, service_llsd);
        }
        LL_INFOS() << "loaded config file: " << service_filename << LL_ENDL;
    }
    else
    {
        LL_WARNS() << "unable to find config file: " << service_filename << LL_ENDL;
    }
}

void LLServiceBuilder::createServiceDefinition(
    const std::string& service_name,
    LLSD& service_llsd)
{
    if(service_llsd.isString())
    {
        mServiceMap[ service_name ] = service_llsd.asString();
    }           
    else if(service_llsd.isMap())
    {
        for(LLSD::map_iterator map_itr = service_llsd.beginMap();
            map_itr != service_llsd.endMap();
            ++map_itr)
        {
            std::string compound_builder_name = service_name;
            compound_builder_name.append("-");
            compound_builder_name.append((*map_itr).first);
            mServiceMap[ compound_builder_name ] = (*map_itr).second.asString();
        }
    }
}

static
bool starts_with(const std::string& text, const char* prefix)
{
    return text.substr(0, strlen(prefix)) == prefix;
}

// TODO: Build a real services.xml for windows development.
//       and remove the base_url logic below.
std::string LLServiceBuilder::buildServiceURI(const std::string& service_name) const
{
    std::ostringstream service_url;
    // Find the service builder
    std::map<std::string, std::string>::const_iterator it =
        mServiceMap.find(service_name);
    if(it != mServiceMap.end())
    {
        // construct the service builder url
        LLApp* app = LLApp::instance();
        if(app)
        {
            // We define a base-url for some development configurations
            // In production neither of these are defined and all services have full urls
            LLSD base_url;

            if (starts_with(service_name,"cap"))
            {
                base_url = app->getOption("cap-base-url");
            }

            if (base_url.asString().empty())
            {
                base_url = app->getOption("services-base-url");
            }
            service_url << base_url.asString();
        }
        service_url << it->second;
    }
    else
    {
        LL_WARNS() << "Cannot find service " << service_name << LL_ENDL;
    }
    return service_url.str();
}

std::string LLServiceBuilder::buildServiceURI(
    const std::string& service_name,
    const LLSD& option_map) const
{
    return russ_format(buildServiceURI(service_name), option_map);
}

std::string russ_format(const std::string& format_str, const LLSD& context)
{
    std::string service_url(format_str);
    if(!service_url.empty() && context.isMap())
    {
        // throw in a ridiculously large limiter to make sure we don't
        // loop forever with bad input.
        int iterations = 100;
        bool keep_looping = true;
        while(keep_looping)
        {
            if(0 == --iterations)
            {
                keep_looping = false;
            }

            int depth = 0;
            int deepest = 0;
            bool find_match = false;
            std::string::iterator iter(service_url.begin());
            std::string::iterator end(service_url.end());
            std::string::iterator deepest_node(service_url.end());
            std::string::iterator deepest_node_end(service_url.end());
            // parse out the variables to replace by going through {}s
            // one at a time, starting with the "deepest" in series
            // {{}}, and otherwise replacing right-to-left
            for(; iter != end; ++iter)
            {
                switch(*iter)
                {
                case '{':
                    ++depth;
                    if(depth > deepest)
                    {
                        deepest = depth;
                        deepest_node = iter;
                        find_match = true;
                    }
                    break;
                case '}':
                    --depth;
                    if(find_match)
                    {
                        deepest_node_end = iter;
                        find_match = false;
                    }
                    break;
                default:
                    break;
                }
            }
            if((deepest_node == end) || (deepest_node_end == end))
            {
                break;
            }
            //replace the variable we found in the {} above.
            // *NOTE: since the c++ implementation only understands
            // params and straight string substitution, so it's a
            // known distance of 2 to skip the directive.
            std::string key(deepest_node + 2, deepest_node_end);
            LLSD value = context[key];
            switch(*(deepest_node + 1))
            {
            case '$':
                if(value.isDefined())
                {
                    service_url.replace(
                        deepest_node,
                        deepest_node_end + 1,
                        value.asString());
                }
                else
                {
                    LL_WARNS() << "Unknown key: " << key << " in option map: "
                        << LLSDOStreamer<LLSDNotationFormatter>(context)
                        << LL_ENDL;
                    keep_looping = false;
                }
                break;
            case '%':
                {
                    std::string query_str = LLURI::mapToQueryString(value);
                    service_url.replace(
                        deepest_node,
                        deepest_node_end + 1,
                        query_str);
                }
                break;
            default:
                LL_INFOS() << "Unknown directive: " << *(deepest_node + 1)
                    << LL_ENDL;
                keep_looping = false;
                break;
            }
        }
    }
    if (service_url.find('{') != std::string::npos)
    {
        LL_WARNS() << "Constructed a likely bogus service URL: " << service_url
            << LL_ENDL;
    }
    return service_url;
}
