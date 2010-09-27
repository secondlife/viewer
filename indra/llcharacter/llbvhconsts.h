/** 
 * @file llbvhconsts.h
 * @brief Consts and types useful to BVH files and LindenLabAnimation format.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLBVHCONSTS_H
#define LL_LLBVHCONSTS_H

const F32 MAX_ANIM_DURATION = 30.f;

typedef enum e_constraint_type
	{
		CONSTRAINT_TYPE_POINT,
		CONSTRAINT_TYPE_PLANE,
		NUM_CONSTRAINT_TYPES
	} EConstraintType;

typedef enum e_constraint_target_type
	{
		CONSTRAINT_TARGET_TYPE_BODY,
		CONSTRAINT_TARGET_TYPE_GROUND,
		NUM_CONSTRAINT_TARGET_TYPES
	} EConstraintTargetType;

#endif // LL_LLBVHCONSTS_H
