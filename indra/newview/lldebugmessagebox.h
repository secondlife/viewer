/** 
 * @file lldebugmessagebox.h
 * @brief Debug message box.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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


#ifndef LL_LLDEBUGMESSAGEBOX_H
#define LL_LLDEBUGMESSAGEBOX_H

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
