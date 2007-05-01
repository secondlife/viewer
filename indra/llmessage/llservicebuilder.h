/** 
* @file llservicebuilder.h
* @brief Declaration of the LLServiceBuilder class.
*
* Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
* $License$
*/

#ifndef LLSERVICEBUILDER_H
#define LLSERVICEBUILDER_H

#include <string>
#include <map>
#include "llerror.h"

class LLSD;

/** 
 * @class LLServiceBuilder
 * @brief This class builds urls for us to use when making web service calls.
 */

class LLServiceBuilder
{
	LOG_CLASS(LLServiceBuilder);
public:
	LLServiceBuilder(void) {}
	~LLServiceBuilder(void) {}

	/** 
	 * @brief Initialize this object with the service definitions.
	 *
	 * @param service_filename The services definition files -- services.xml.
	 */
	void loadServiceDefinitionsFromFile(const std::string& service_filename);

	/** 
	 * @brief Build a service url if the url needs no construction parameters.
	 *
	 * @param service_name The name of the service you want to call.
	 */
	std::string buildServiceURI(const std::string& service_name);

	/** 
	 * @brief Build a service url if the url with construction parameters.
	 *
	 * The parameter substitution supports string substituition from RUSS:
	 * [[Recursive_URL_Substitution_Syntax]]
	 * @param service_name The name of the service you want to call.
	 * @param option_map The parameters in a map of name:value for the service.
	 */
	std::string buildServiceURI(
		const std::string& service_name,
		const LLSD& option_map);	

public:
	/** 
	 * @brief Helper method which builds construction state for a service
	 *
	 * This method should probably be protected, but we need to test this
	 * method.
	 */
	void createServiceDefinition(
		const std::string& service_name,
		LLSD& service_url);

protected:
	std::map<std::string, std::string> mServiceMap;
};



#endif
