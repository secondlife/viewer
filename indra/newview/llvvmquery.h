/**
 * @file llvvmquery.h
 * @brief Query the Viewer Version Manager (VVM) for update information
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_LLVVMQUERY_H
#define LL_LLVVMQUERY_H

/**
 * Initialize the VVM update check.
 *
 * This launches a coroutine that queries the Viewer Version Manager (VVM)
 * to check for available updates. If an update is available, it configures
 * Velopack with the update URL and initiates the update check/download.
 *
 * The release notes URL from the VVM response is posted to the "relnotes"
 * event pump for display.
 */
void initVVMUpdateCheck();

#endif // LL_LLVVMQUERY_H
