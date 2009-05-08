/** 
 * @file llhudmanager.h
 * @brief LLHUDManager class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLHUDMANAGER_H
#define LL_LLHUDMANAGER_H

// Responsible for managing all HUD elements.

#include "llhudobject.h"
#include "lldarrayptr.h"

class LLViewerObject;
class LLHUDEffect;
//Ventrella 9/16/05
class LLHUDAnimalControls;
// End Ventrella
class LLMessageSystem;

class LLHUDManager : public LLSingleton<LLHUDManager>
{
public:
	LLHUDManager();
	~LLHUDManager();

	LLHUDEffect *createViewerEffect(const U8 type, BOOL send_to_sim = TRUE, BOOL originated_here = TRUE);

	void updateEffects();
	void sendEffects();
	void cleanupEffects();

	static void shutdownClass();

	static void processViewerEffect(LLMessageSystem *mesgsys, void **user_data);

	static LLColor4 sParentColor;
	static LLColor4 sChildColor;

protected:
	LLDynamicArrayPtr<LLPointer<LLHUDEffect>				> mHUDEffects;
};

#endif // LL_LLHUDMANAGER_H
