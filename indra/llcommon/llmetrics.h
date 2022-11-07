/**
 * @file llmetrics.h
 * @author Kelly
 * @date 2007-05-25
 * @brief Declaration of metrics accumulation and associated functions
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

#ifndef LL_LLMETRICS_H
#define LL_LLMETRICS_H

class LLMetricsImpl;
class LLSD;

class LL_COMMON_API LLMetrics
{
public:
    LLMetrics();
    virtual ~LLMetrics();

    // Adds this event to aggregate totals and records details to syslog (LL_INFOS())
    virtual void recordEventDetails(const std::string& location, 
                        const std::string& mesg, 
                        bool success, 
                        LLSD stats);

    // Adds this event to aggregate totals
    virtual void recordEvent(const std::string& location, const std::string& mesg, bool success);

    // Prints aggregate totals and resets the counts.
    virtual void printTotals(LLSD meta);


private:
    
    LLMetricsImpl* mImpl;
};

#endif

