/** 
 * @file llservicebuilder.cpp
 * @brief Implementation of the LLServiceBuilder class.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "llapp.h"
#include "llfile.h"
#include "llservicebuilder.h"
#include "llsd.h"
#include "llsdserialize.h"

void LLServiceBuilder::loadServiceDefinitionsFromFile(
	const std::string& service_filename)
{
	llifstream service_file(service_filename, std::ios::binary);
	if(service_file.is_open())
	{
		LLSD service_data;
		LLSDSerialize::fromXML(service_data, service_file);
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
		llinfos << "loaded config file: " << service_filename << llendl;
	}
	else
	{
		llwarns << "unable to find config file: " << service_filename << llendl;
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
std::string LLServiceBuilder::buildServiceURI(const std::string& service_name)
{
	std::ostringstream service_url;
	// Find the service builder
	if(mServiceMap.find(service_name) != mServiceMap.end())
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
		service_url << mServiceMap[service_name];
	}
	else
	{
		llwarns << "Cannot find service " << service_name << llendl;
	}
	return service_url.str();
}

std::string LLServiceBuilder::buildServiceURI(
	const std::string& service_name,
	const LLSD& option_map)
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
					llwarns << "Unknown key: " << key << " in option map: "
						<< LLSDOStreamer<LLSDNotationFormatter>(context)
						<< llendl;
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
				llinfos << "Unknown directive: " << *(deepest_node + 1)
					<< llendl;
				keep_looping = false;
				break;
			}
		}
	}
	if (service_url.find('{') != std::string::npos)
	{
		llwarns << "Constructed a likely bogus service URL: " << service_url
			<< llendl;
	}
	return service_url;
}
