/** 
 * @file lldebugmessagebox.h
 * @brief Debug message box.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	static void slider_changed(LLUICtrl* ctrl, void* user_data);
	static void onAnimateClicked(void* user_data);

public:
	static void show(const std::string& title, F32 *var, F32 max_value = 100.f, F32 increment = 0.1f);
	static void show(const std::string& title, S32 *var, S32 max_value = 255, S32 increment = 1);
	static void show(const std::string& title, LLVector2 *var, LLVector2 max_value = LLVector2(100.f, 100.f), LLVector2 increment = LLVector2(0.1f, 0.1f));
	static void show(const std::string& title, LLVector3 *var, LLVector3 max_value = LLVector3(100.f, 100.f, 100.f), LLVector3 increment = LLVector3(0.1f, 0.1f, 0.1f));
	//static void show(const std::string& title, LLVector4 *var, LLVector4 max_value = LLVector4(100.f, 100.f, 100.f, 100.f), LLVector4 increment = LLVector4(0.1f, 0.1f, 0.1f, 0.1f));	
		
	virtual void onClose(bool app_quitting);
	virtual void	draw();

protected:
	EDebugVarType	mVarType;
	void*			mVarData;
	LLSliderCtrl*	mSlider1;
	LLSliderCtrl*	mSlider2;
	LLSliderCtrl*	mSlider3;
	LLButton*		mAnimateButton;
	LLTextBox*		mText;
	LLString		mTitle;
	BOOL			mAnimate;

	static std::map<LLString, LLDebugVarMessageBox*> sInstances;
};

#endif // LL_LLMESSAGEBOX_H
