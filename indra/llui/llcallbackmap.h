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

#pragma once

#include "llstl.h"

#include <string>
#include <functional>
#include <unordered_map>

class LLCallbackMap
{
public:
    // callback definition.
    using callback_t = std::function<void* (void* data)>;

    using map_t = std::unordered_map<std::string, LLCallbackMap>;
    using map_iter_t = map_t::iterator;
    using map_const_iter_t = map_t::const_iterator;

    template <class T>
    static void* buildPanel(void* data)
    {
        T* panel = new T();
        return (void*)panel;
    }

    LLCallbackMap() : mCallback(nullptr), mData(nullptr) {}
    explicit LLCallbackMap(callback_t callback, void* data = nullptr) : mCallback(callback), mData(data) {}

    callback_t  mCallback;
    void*       mData;
};

