/** 
 * @file llmeshreduction.h
 * @brief LLMeshReduction class definition
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

#ifndef LL_LLMESHREDUCTION_H
#define LL_LLMESHREDUCTION_H

#include "llmodel.h"

class LLMeshReduction
{
 public:
	enum EReductionMode
	{
		TRIANGLE_BUDGET,
		ERROR_THRESHOLD
	};
	
	LLMeshReduction();
	~LLMeshReduction();

	LLPointer<LLModel> reduce(LLModel* in_model, F32 limit, S32 mode);
	
private:
	U32 mCounter;
};

#endif  // LL_LLMESHREDUCTION_H
