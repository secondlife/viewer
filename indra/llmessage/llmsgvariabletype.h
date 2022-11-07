/** 
 * @file llmsgvariabletype.h
 * @brief Declaration of the EMsgVariableType enumeration.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLMSGVARIABLETYPE_H
#define LL_LLMSGVARIABLETYPE_H

typedef enum e_message_variable_type
{
    MVT_NULL,
    MVT_FIXED,
    MVT_VARIABLE,
    MVT_U8,
    MVT_U16,
    MVT_U32,
    MVT_U64,
    MVT_S8,
    MVT_S16,
    MVT_S32,
    MVT_S64,
    MVT_F32,
    MVT_F64,
    MVT_LLVector3,
    MVT_LLVector3d,
    MVT_LLVector4,
    MVT_LLQuaternion,
    MVT_LLUUID, 
    MVT_BOOL,
    MVT_IP_ADDR,
    MVT_IP_PORT,
    MVT_U16Vec3,
    MVT_U16Quat,
    MVT_S16Array,
    MVT_EOL
} EMsgVariableType;

#endif // LL_LLMSGVARIABLETYPE_H
