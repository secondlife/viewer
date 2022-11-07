/** 
 * @file llboost.h
 * @brief helper object & functions for use with boost
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#ifndef LL_LLBOOST_H
#define LL_LLBOOST_H

#include <boost/tokenizer.hpp>

// boost_tokenizer typedef
/* example usage:
    boost_tokenizer tokens(input_string, boost::char_separator<char>(" \t\n"));
    for (boost_tokenizer::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
    {
        std::string tok = *token_iter;
        process_token_string( tok );
    }
*/
typedef boost::tokenizer<boost::char_separator<char> > boost_tokenizer;

// Useful combiner for boost signals that return a bool (e.g. validation)
//  returns false if any of the callbacks return false
struct boost_boolean_combiner
{
    typedef bool result_type;
    template<typename InputIterator>
    bool operator()(InputIterator first, InputIterator last) const
    {
        bool res = true;
        while (first != last)
            res &= *first++;
        return res;
    }
};

#endif // LL_LLBOOST_H
