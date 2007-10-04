/** 
 * @file llmsgvariabletype.h
 * @brief Declaration of the EMsgVariableType enumeration.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
