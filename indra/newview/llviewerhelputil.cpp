/** 
 * @file llviewerhelp.cpp
 * @brief Utility functions for the Help system
 * @author Soft Linden
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llviewerhelputil.h"

#include "llagent.h"
#include "llsd.h"
#include "llstring.h"
#include "lluri.h"
#include "llweb.h"
#include "llviewercontrol.h"

//////////////////////////////////////////////
// Build a help URL from a topic and formatter

//static
std::string LLViewerHelpUtil::helpDataEncode(const std::string &component)
{
    const std::string data_marker = "data:";
    if (component.compare(0, data_marker.length(), data_marker) == 0)
    {
        // This is not url, but data, data part needs to be properly escaped
        // data part is separated by ',' from header. Minimal data uri is "data:,"
        // See "data URI scheme"
        size_t separator = component.find(",");
        if (separator != std::string::npos)
        {
            size_t header_size = separator + 1;
            std::string header = component.substr(0, header_size);
            if (header.find("base64") != std::string::npos)
            {
                // assume url-safe data
                return component;
            }

            std::string data = component.substr(header_size, component.length() - header_size);
            
            const std::string allowed_sorted =
                "%-." //include % since it can be a sign of being pre-escaped and is an allowed symbol
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "_"
                "abcdefghijklmnopqrstuvwxyz";

            bool needs_escaping = false;

            std::string::const_iterator it = data.begin();
            std::string::const_iterator end = data.end();
            std::string::const_iterator allowed_begin(allowed_sorted.begin());
            std::string::const_iterator allowed_end(allowed_sorted.end());
            std::string::value_type c;
            for (; it != end; ++it)
            {
                c = *it;
                if (!std::binary_search(allowed_begin, allowed_end, c))
                {
                    // found unescaped symbol
                    needs_escaping = true;
                    break;
                }
            }

            if (!needs_escaping)
            {
                // pre-escaped
                return component;
            }

            // Notes: Teoretically file can be partially pre-escaped,
            // but we can't reliably distinguish what user intended to do,
            // so escape everything and let user control the rest.
            // Escapes % as well
            std::string url = header + helpURLEncode(data);

            return url;
        }
        else
        {
            // corrupted or not a data scheme? Nothing we can do.
            return component;
        }
    }
    else
    {
        // Not a data scheme
        return component;
    }
}

//static
std::string LLViewerHelpUtil::helpURLEncode( const std::string &component )
{
	// Every character rfc3986 allows as unreserved in 2.3, minus the tilde
	// which we may grant special meaning. Yay.
	const char* allowed =   
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789"
	"-._";
	std::string escaped = LLURI::escape(component, allowed);
	
	return escaped;
}

//static
std::string LLViewerHelpUtil::buildHelpURL( const std::string &topic)
{
	LLSD substitution;
	substitution["TOPIC"] = helpURLEncode(topic);
	substitution["DEBUG_MODE"] = gAgent.isGodlike() ? "/debug" : "";
	
	// get the help URL and expand all of the substitutions
	// (also adds things like [LANGUAGE], [VERSION], [OS], etc.)
	std::string helpURL = gSavedSettings.getString("HelpURLFormat");
	return LLWeb::expandURLSubstitutions(helpURL, substitution);
}
