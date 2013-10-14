/** 
 * @file llallocator_heap_profile.cpp
 * @brief Implementation of the parser for tcmalloc heap profile data.
 * @author Brad Kittenbrink
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

#include "linden_common.h"
#include "llallocator_heap_profile.h"

#if LL_MSVC
// disable warning about boost::lexical_cast returning uninitialized data
// when it fails to parse the string
#pragma warning (disable:4701)
#pragma warning (disable:4702)
#endif

#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/iterator_range.hpp>

static const std::string HEAP_PROFILE_MAGIC_STR = "heap profile:";

static bool is_separator(char c)
{
    return isspace(c) || c == '[' || c == ']' || c == ':';
}

void LLAllocatorHeapProfile::parse(std::string const & prof_text)
{
    // a typedef for handling a token in the string buffer
    // it's a begin/end pair of string::const_iterators
    typedef boost::iterator_range<std::string::const_iterator> range_t;

    mLines.clear();

    if(prof_text.compare(0, HEAP_PROFILE_MAGIC_STR.length(), HEAP_PROFILE_MAGIC_STR) != 0)
    {
        // *TODO - determine if there should be some better error state than
        // mLines being empty. -brad
        llwarns << "invalid heap profile data passed into parser." << llendl;
        return;
    }

    std::vector< range_t > prof_lines;

    std::string::const_iterator prof_begin = prof_text.begin() + HEAP_PROFILE_MAGIC_STR.length();

	range_t prof_range(prof_begin, prof_text.end());
    boost::algorithm::split(prof_lines,
        prof_range,
        boost::bind(std::equal_to<llwchar>(), '\n', _1));

    std::vector< range_t >::const_iterator i;
    for(i = prof_lines.begin(); i != prof_lines.end() && !i->empty(); ++i)
    {
        range_t const & line_text = *i;

        std::vector<range_t> line_elems;

        boost::algorithm::split(line_elems,
            line_text,
            is_separator);

        std::vector< range_t >::iterator j;
        j = line_elems.begin();

        while(j != line_elems.end() && j->empty()) { ++j; } // skip any separator tokens
        llassert_always(j != line_elems.end());
        U32 live_count = boost::lexical_cast<U32>(*j);
        ++j;

        while(j != line_elems.end() && j->empty()) { ++j; } // skip any separator tokens
        llassert_always(j != line_elems.end());
        U64 live_size = boost::lexical_cast<U64>(*j);
        ++j;

        while(j != line_elems.end() && j->empty()) { ++j; } // skip any separator tokens
        llassert_always(j != line_elems.end());
        U32 tot_count = boost::lexical_cast<U32>(*j);
        ++j;

        while(j != line_elems.end() && j->empty()) { ++j; } // skip any separator tokens
        llassert_always(j != line_elems.end());
        U64 tot_size = boost::lexical_cast<U64>(*j);
        ++j;

        while(j != line_elems.end() && j->empty()) { ++j; } // skip any separator tokens
	llassert(j != line_elems.end());
        if (j != line_elems.end())
	{
		++j; // skip the '@'

		mLines.push_back(line(live_count, live_size, tot_count, tot_size));
		line & current_line = mLines.back();
		
		for(; j != line_elems.end(); ++j)
		{
			if(!j->empty())
			{
				U32 marker = boost::lexical_cast<U32>(*j);
				current_line.mTrace.push_back(marker);
			}
		}
	}
    }
    // *TODO - parse MAPPED_LIBRARIES section here if we're ever interested in it
}

void LLAllocatorHeapProfile::dump(std::ostream & out) const
{
    lines_t::const_iterator i;
    for(i = mLines.begin(); i != mLines.end(); ++i)
    {
        out << i->mLiveCount << ": " << i->mLiveSize << '[' << i->mTotalCount << ": " << i->mTotalSize << "] @";

        stack_trace::const_iterator j;
        for(j = i->mTrace.begin(); j != i->mTrace.end(); ++j)
        {
            out << ' ' << *j;
        }
        out << '\n';
    }
    out.flush();
}

