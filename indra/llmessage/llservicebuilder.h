/** 
* @file llservicebuilder.h
* @brief Declaration of the LLServiceBuilder class.
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
