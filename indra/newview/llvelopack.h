/**
 * @file llvelopack.h
 * @brief Velopack installer and update framework integration
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

#ifndef LL_LLVELOPACK_H
#define LL_LLVELOPACK_H

#if LL_VELOPACK

#include <string>
#include <functional>

bool velopack_initialize();
void velopack_check_for_updates();
std::string velopack_get_current_version();
bool velopack_is_update_pending();
void velopack_apply_pending_update(bool restart = true);
void velopack_set_update_url(const std::string& url);
void velopack_set_progress_callback(std::function<void(int)> callback);

#if LL_WINDOWS
bool get_nsis_uninstaller_path(wchar_t* path_buffer, DWORD bufSize, S32 cur_major_ver, S32 cur_minor_ver, S32 cur_patch_ver, U64 cur_build_ver);
#endif

#endif // LL_VELOPACK
#endif
// EOF
