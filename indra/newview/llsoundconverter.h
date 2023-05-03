/** 
 * @file llsoundconverter.h
 * @brief Implementation of LLSoundConverterThread class
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifndef LL_LLSOUNDCONVERTER_H
#define LL_LLSOUNDCONVERTER_H

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// class LLSoundConventerThread
//-----------------------------------------------------------------------------


class LLSoundConverterThread: public LLThread
{
public:
    typedef std::function<void(const std::string)> on_complition_cb;
    LLSoundConverterThread(const std::string& filename, on_complition_cb callback);

protected:
    void run() override;

    std::string mInFilename;
    on_complition_cb mOnCompletionCallback;
};


#endif // LL_LLSOUNDCONVERTER_H


