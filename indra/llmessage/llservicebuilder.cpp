/** 
* @file llservicebuilder.cpp
* @brief Implementation of the LLServiceBuilder class.
*
* Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
* $License$
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
			LLSD base_url = app->getOption("services-base-url");
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
	std::string service_url = buildServiceURI(service_name);

	// Find the Service Name
	if(!service_url.empty() && option_map.isMap())
	{
		// Do brace replacements - NOT CURRENTLY RECURSIVE
		for(LLSD::map_const_iterator option_itr = option_map.beginMap();
			option_itr != option_map.endMap();
			++option_itr)
		{
			std::string variable_name = "{$";
			variable_name.append((*option_itr).first);
			variable_name.append("}");
			std::string::size_type find_pos = service_url.find(variable_name);
			if(find_pos != std::string::npos)
			{
				service_url.replace(
					find_pos,
					variable_name.length(),
					(*option_itr).second.asString());
			}
		}
	}

	return service_url;
}
