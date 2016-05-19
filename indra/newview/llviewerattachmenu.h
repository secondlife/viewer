/** 
 * @file llviewerattachmenu.h
 * @brief "Attach to" / "Attach to HUD" submenus.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLVIEWERATTACHMENU_H
#define LL_LLVIEWERATTACHMENU_H

class LLViewerAttachMenu
{
public:
	static void populateMenus(const std::string& attach_to_menu_name, const std::string& attach_to_hud_menu_name);
	static void attachObjects(const uuid_vec_t& items, const std::string& joint_name);
};

#endif // LL_LLVIEWERATTACHMENU_H
