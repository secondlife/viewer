/** 
 * @file llfttype.h
 * @brief Texture request constants
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

#ifndef LL_FTTYPE_H
#define LL_FTTYPE_H

enum FTType
{
    FTT_UNKNOWN = -1,
    FTT_DEFAULT = 0, // standard texture fetched by id.
    FTT_SERVER_BAKE, // texture produced by appearance service and fetched from there.
    FTT_HOST_BAKE, // old-style baked texture uploaded by viewer and fetched from avatar's host.
    FTT_MAP_TILE, // tiles are fetched from map server directly.
    FTT_LOCAL_FILE // fetch directly from a local file.
};

#endif
