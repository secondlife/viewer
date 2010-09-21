/** 
* @file llservicebuilder.h
* @brief Declaration of the LLServiceBuilder class.
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

#ifndef LLSERVICEBUILDER_H
#define LLSERVICEBUILDER_H

#include <string>
#include <map>
#include "llerror.h"

class LLSD;

/**
 * @brief  Format format string according to rules for RUSS.
 *
 * This function appears alongside the service builder since the
 * algorithm was originally implemented there. This can eventually be
 * moved when someone wants to take the time.
 * @see https://osiris.lindenlab.com/mediawiki/index.php/Recursive_URL_Substitution_Syntax
 * @param format_str The input string to format.
 * @param context A map used for string substitutions.
 * @return Returns the formatted string. If no match is found for a
 * substitution target, the braces remain intact.
 */
std::string russ_format(const std::string& format_str, const LLSD& context);

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
	std::string buildServiceURI(const std::string& service_name) const;

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
		const LLSD& option_map) const;	

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
