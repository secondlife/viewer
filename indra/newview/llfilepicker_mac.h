/** 
 * @file llfilepicker_mac.h
 * @brief OS-specific file picker
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// OS specific file selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. When you call getMultipleOpenFile(), it locks the picker
// until you iterate to the end of the list of selected files with
// getNextFile() or call reset().

#ifndef LL_LLFILEPICKER_MAC_H
#define LL_LLFILEPICKER_MAC_H

#if LL_DARWIN

#include <string>
#include <vector>

//void modelessPicker();
std::vector<std::string>* doLoadDialog(const std::vector<std::string>* allowed_types, 
                 unsigned int flags);
std::string* doSaveDialog(const std::string* file, 
                  const std::string* type,
                  const std::string* creator,
                  const std::string* extension,
                  unsigned int flags);
enum {
    F_FILE =      0x00000001,
    F_DIRECTORY = 0x00000002,
    F_MULTIPLE =  0x00000004,
    F_NAV_SUPPORT=0x00000008
};

#endif

#endif
