/** 
 * @file llcallbackmap.h
 * @brief LLCallbackMap base class
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LLCALLBACKMAP_H
#define LLCALLBACKMAP_H

#include <map>
#include <string>
#include <boost/function.hpp>

class LLCallbackMap
{
public:
    // callback definition.
    typedef boost::function<void* (void* data)> callback_t;
    
    typedef std::map<std::string, LLCallbackMap> map_t;
    typedef map_t::iterator map_iter_t;
    typedef map_t::const_iterator map_const_iter_t;
    
    template <class T>
    static void* buildPanel(void* data)
    {
        T* panel = new T();
        return (void*)panel;
    }
    
    LLCallbackMap() : mCallback(NULL), mData(NULL) { }
    LLCallbackMap(callback_t callback, void* data = NULL) : mCallback(callback), mData(data) { }

    callback_t  mCallback;
    void*       mData;
};

#endif // LLCALLBACKMAP_H
