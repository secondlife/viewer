/**
 * @file lltransutil.h
 * @brief LLTrans helper
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_TRANSUTIL_H
#define LL_TRANSUTIL_H

#include "lltrans.h"

namespace LLTransUtil
{
    /**
     * @brief Parses the xml file that holds the strings. Used once on startup
     * @param xml_filename Filename to parse
     * @param default_args Set of strings (expected to be in the file) to use as default replacement args, e.g. "SECOND_LIFE"
     * @returns true if the file was parsed successfully, true if something went wrong
     */
    bool parseStrings(const std::string& xml_filename, const std::set<std::string>& default_args);

    bool parseLanguageStrings(const std::string& xml_filename);
};

#endif
