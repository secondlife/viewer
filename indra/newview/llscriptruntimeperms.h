/**
 * @file llscriptruntimeperms.h
 * @brief Script runtime permission definitions
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

#ifndef LL_LLSCRIPTRUNTIME_PERMS_H
#define LL_LLSCRIPTRUNTIME_PERMS_H

#include <boost/array.hpp>

typedef struct _script_perm {
	std::string question;
	U32 permbit;
	bool caution;
	_script_perm(const std::string& q, const U32 b, const bool c) :
	question(q), permbit(b), caution(c) {}
} script_perm_t;

const U32 NUM_SCRIPT_PERMISSIONS = 18;
const S32 SCRIPT_PERMISSION_DEBIT = 0;
const S32 SCRIPT_PERMISSION_TRIGGER_ANIMATION = 3;
const S32 SCRIPT_PERMISSION_OVERRIDE_ANIMATIONS = 14;

static const boost::array<script_perm_t, NUM_SCRIPT_PERMISSIONS> SCRIPT_PERMISSIONS = {{
	_script_perm("ScriptTakeMoney",		(0x1 << 1),  true),
	_script_perm("ActOnControlInputs",	(0x1 << 2),  false),
	_script_perm("RemapControlInputs",	(0x1 << 3),  false),
	_script_perm("AnimateYourAvatar",	(0x1 << 4),  false),
	_script_perm("AttachToYourAvatar",	(0x1 << 5),  false),
	_script_perm("ReleaseOwnership",	(0x1 << 6),  false),
	_script_perm("LinkAndDelink",		(0x1 << 7),  false),
	_script_perm("AddAndRemoveJoints",	(0x1 << 8),  false),
	_script_perm("ChangePermissions",	(0x1 << 9),  false),
	_script_perm("TrackYourCamera",		(0x1 << 10), false),
	_script_perm("ControlYourCamera",	(0x1 << 11), false),
	_script_perm("TeleportYourAgent",	(0x1 << 12), false),
	_script_perm("JoinAnExperience",	(0x1 << 13), false),
	_script_perm("SilentlyManageEstateAccess", (0x1 << 14), false),
	_script_perm("OverrideYourAnimations", (0x1 << 15), false),
	_script_perm("ScriptReturnObjects",	(0x1 << 16), false),
    _script_perm("ForceSitAvatar",      (0x1 << 17), false),
    _script_perm("ChangeEnvSettings",   (0x1 << 18), false)
    } };

#endif // LL_LLSCRIPTRUNTIME_PERMS_H
