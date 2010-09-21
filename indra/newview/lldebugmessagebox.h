/** 
 * @file lldebugmessagebox.h
 * @brief Debug message box.
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


#ifndef LL_LLDEBUGMESSAGEBOX_H
#define LL_LLDEBUGMESSAGEBOX_H

#include "lldarray.h"
#include "llfloater.h"
#include "v3math.h"
#include "lltextbox.h"
#include "llstring.h"
#include "llframetimer.h"
#include <vector>
#include <map>

class LLSliderCtrl;

//----------------------------------------------------------------------------
// LLDebugVarMessageBox
//----------------------------------------------------------------------------

typedef enum e_debug_var_type
{
	VAR_TYPE_F32,
	VAR_TYPE_S32,
	VAR_TYPE_VEC2,
	VAR_TYPE_VEC3,
	VAR_TYPE_VEC4,
	VAR_TYPE_COUNT
} EDebugVarType;

class LLDebugVarMessageBox : public LLFloater
{
protected:
	LLDebugVarMessageBox(const std::string& title, EDebugVarType var_type, void *var);
	~LLDebugVarMessageBox();

	static LLDebugVarMessageBox* show(const std::string& title, EDebugVarType var_type, void *var);
	void sliderChanged(const LLSD& data);
	void onAnimateClicked(const LLSD& data);

public:
	static void show(const std::string& title, F32 *var, F32 max_value = 100.f, F32 increment = 0.1f);
	static void show(const std::string& title, S32 *var, S32 max_value = 255, S32 increment = 1);
	static void show(const std::string& title, LLVector2 *var, LLVector2 max_value = LLVector2(100.f, 100.f), LLVector2 increment = LLVector2(0.1f, 0.1f));
	static void show(const std::string& title, LLVector3 *var, LLVector3 max_value = LLVector3(100.f, 100.f, 100.f), LLVector3 increment = LLVector3(0.1f, 0.1f, 0.1f));
	//static void show(const std::string& title, LLVector4 *var, LLVector4 max_value = LLVector4(100.f, 100.f, 100.f, 100.f), LLVector4 increment = LLVector4(0.1f, 0.1f, 0.1f, 0.1f));	
		
	virtual void	draw();

protected:
	EDebugVarType	mVarType;
	void*			mVarData;
	LLSliderCtrl*	mSlider1;
	LLSliderCtrl*	mSlider2;
	LLSliderCtrl*	mSlider3;
	LLButton*		mAnimateButton;
	LLTextBox*		mText;
	std::string		mTitle;
	BOOL			mAnimate;

	static std::map<std::string, LLDebugVarMessageBox*> sInstances;
};

#endif // LL_LLMESSAGEBOX_H
