/** 
 * @file lljointpickname.h
 * @brief Defines OpenGL seleciton stack names
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


#ifndef LL_LLJOINTPICKNAME_H
#define LL_LLJOINTPICKNAME_H

class LLAvatarJointMesh;

// Sets the OpenGL selection stack name that is pushed and popped
// with this joint state.  The default value indicates that no name
// should be pushed/popped.
enum LLJointPickName
{
	PN_DEFAULT = -1,
	PN_0 = 0,
	PN_1 = 1,
	PN_2 = 2,
	PN_3 = 3,
	PN_4 = 4,
	PN_5 = 5
};

typedef std::vector<LLAvatarJointMesh*> avatar_joint_mesh_list_t;

#endif // LL_LLJOINTPICKNAME_H
