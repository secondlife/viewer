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
void velopack_check_for_updates(const std::string& required_version, const std::string& relnotes_url);
std::string velopack_get_current_version();
bool velopack_is_update_pending();
bool velopack_is_required_update_in_progress();
std::string velopack_get_required_update_version();
bool velopack_should_restart_after_update();
void velopack_request_restart_after_update();
void velopack_apply_pending_update(bool restart = true);
void velopack_set_update_url(const std::string& url);
void velopack_set_progress_callback(std::function<void(int)> callback);
void velopack_cleanup();

#if LL_WINDOWS
void clear_nsis_links();
bool get_nsis_version(
    int& nsis_major,
    int& nsis_minor,
    int& nsis_patch,
    uint64_t& nsis_build);
#endif

#endif // LL_VELOPACK
#endif
// EOF
