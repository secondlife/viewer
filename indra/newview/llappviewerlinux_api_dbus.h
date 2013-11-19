/** 
 * @file llappviewerlinux_api_dbus.h
 * @brief DBus-glib symbol handling
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#if LL_DBUS_ENABLED

extern "C" {
#include <dbus/dbus-glib.h>
}

#define DBUSGLIB_DYLIB_DEFAULT_NAME "libdbus-glib-1.so.2"

bool grab_dbus_syms(std::string dbus_dso_name);
void ungrab_dbus_syms();

#define LL_DBUS_SYM(REQUIRED, DBUSSYM, RTN, ...) extern RTN (*ll##DBUSSYM)(__VA_ARGS__)
#include "llappviewerlinux_api_dbus_syms_raw.inc"
#undef LL_DBUS_SYM

#endif // LL_DBUS_ENABLED
