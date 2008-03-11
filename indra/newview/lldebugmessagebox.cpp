/** 
 * @file lldebugmessagebox.cpp
 * @brief Implementation of a simple, non-modal message box.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldebugmessagebox.h"

#include "llresmgr.h"
#include "llfontgl.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llcheckboxctrl.h"
#include "lltextbox.h"
#include "lllineeditor.h"
#include "llfocusmgr.h"

///----------------------------------------------------------------------------
/// Class LLDebugVarMessageBox
///----------------------------------------------------------------------------

std::map<LLString, LLDebugVarMessageBox*> LLDebugVarMessageBox::sInstances;

LLDebugVarMessageBox::LLDebugVarMessageBox(const std::string& title, EDebugVarType var_type, void *var) : 
	LLFloater("msg box", LLRect(10,160,400,10), title),
	mVarType(var_type), mVarData(var), mAnimate(FALSE)
{
	switch(var_type)
	{
	case VAR_TYPE_F32:
	  mSlider1 = new LLSliderCtrl("slider 1", LLRect(20,130,190,110), title, NULL, 70, 130, TRUE, TRUE, FALSE, NULL, NULL, *((F32*)var), -100.f, 100.f, 0.1f, NULL);
		mSlider1->setPrecision(3);
		addChild(mSlider1);
		mSlider2 = NULL;
		mSlider3 = NULL;
		break;
	case VAR_TYPE_S32:
		mSlider1 = new LLSliderCtrl("slider 1", LLRect(20,100,190,80), title, NULL, 70, 130, TRUE, TRUE, FALSE, NULL, NULL, (F32)*((S32*)var), -255.f, 255.f, 1.f, NULL);
		mSlider1->setPrecision(0);
		addChild(mSlider1);
		mSlider2 = NULL;
		mSlider3 = NULL;
		break;
	case VAR_TYPE_VEC3:
		mSlider1 = new LLSliderCtrl("slider 1", LLRect(20,130,190,110), "x: ", NULL, 70, 130, TRUE, TRUE, FALSE, NULL, NULL, ((LLVector3*)var)->mV[VX], -100.f, 100.f, 0.1f, NULL);
		mSlider1->setPrecision(3);
		mSlider2 = new LLSliderCtrl("slider 2", LLRect(20,100,190,80), "y: ", NULL, 70, 130, TRUE, TRUE, FALSE, NULL, NULL, ((LLVector3*)var)->mV[VY], -100.f, 100.f, 0.1f, NULL);
		mSlider2->setPrecision(3);
		mSlider3 = new LLSliderCtrl("slider 3", LLRect(20,70,190,50), "z: ", NULL, 70, 130, TRUE, TRUE, FALSE, NULL, NULL, ((LLVector3*)var)->mV[VZ], -100.f, 100.f, 0.1f, NULL);
		mSlider3->setPrecision(3);
		addChild(mSlider1);
		addChild(mSlider2);
		addChild(mSlider3);
		break;
	default:
		llwarns << "Unhandled var type " << var_type << llendl;
		break;
	}

	mAnimateButton = new LLButton("Animate", LLRect(20, 45, 180, 25), "", onAnimateClicked, this);
	addChild(mAnimateButton);

	mText = new LLTextBox("value", LLRect(20,20,190,0));
	addChild(mText);

	//disable hitting enter closes dialog
	setDefaultBtn();
}

LLDebugVarMessageBox::~LLDebugVarMessageBox()
{
	sInstances.erase(mTitle);
}

void LLDebugVarMessageBox::show(const std::string& title, F32 *var, F32 max_value, F32 increment)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLDebugVarMessageBox* box = show(title, VAR_TYPE_F32, (void*)var);
	max_value = llabs(max_value);
	box->mSlider1->setMaxValue(max_value);
	box->mSlider1->setMinValue(-max_value);
	box->mSlider1->setIncrement(increment);
	if (!gFocusMgr.childHasKeyboardFocus(box))
	{
		box->mSlider1->setValue(*var);
	}
	box->mSlider1->setCommitCallback(slider_changed);
	box->mSlider1->setCallbackUserData(box);
#endif
}

void LLDebugVarMessageBox::show(const std::string& title, S32 *var, S32 max_value, S32 increment)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLDebugVarMessageBox* box = show(title, VAR_TYPE_S32, (void*)var);
	F32 max_val = llabs((F32)max_value);
	box->mSlider1->setMaxValue(max_val);
	box->mSlider1->setMinValue(-max_val);
	box->mSlider1->setIncrement((F32)increment);
	if (!gFocusMgr.childHasKeyboardFocus(box))
	{
		box->mSlider1->setValue((F32)*var);
	}
	box->mSlider1->setCommitCallback(slider_changed);
	box->mSlider1->setCallbackUserData(box);
#endif
}

void LLDebugVarMessageBox::show(const std::string& title, LLVector3 *var, LLVector3 max_value, LLVector3 increment)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LLDebugVarMessageBox* box = show(title, VAR_TYPE_VEC3, (LLVector3*)var);
	max_value.abs();
	box->mSlider1->setMaxValue(max_value.mV[VX]);
	box->mSlider1->setMinValue(-max_value.mV[VX]);
	box->mSlider1->setIncrement(increment.mV[VX]);
	box->mSlider1->setCommitCallback(slider_changed);
	box->mSlider1->setCallbackUserData(box);

	box->mSlider2->setMaxValue(max_value.mV[VX]);
	box->mSlider2->setMinValue(-max_value.mV[VX]);
	box->mSlider2->setIncrement(increment.mV[VX]);
	box->mSlider2->setCommitCallback(slider_changed);
	box->mSlider2->setCallbackUserData(box);

	box->mSlider3->setMaxValue(max_value.mV[VX]);
	box->mSlider3->setMinValue(-max_value.mV[VX]);
	box->mSlider3->setIncrement(increment.mV[VX]);
	box->mSlider3->setCommitCallback(slider_changed);
	box->mSlider3->setCallbackUserData(box);
#endif
}

LLDebugVarMessageBox* LLDebugVarMessageBox::show(const std::string& title, EDebugVarType var_type, void *var)
{
	LLString title_string(title);

	LLDebugVarMessageBox *box = sInstances[title_string];
	if (!box)
	{
		box = new LLDebugVarMessageBox(title, var_type, var);
		sInstances[title_string] = box;
		gFloaterView->addChild(box);
		box->reshape(200,150);
		box->open();		 /*Flawfinder: ignore*/
		box->mTitle = title_string;
	}

	return box;
}

void LLDebugVarMessageBox::slider_changed(LLUICtrl* ctrl, void* user_data)
{
	LLDebugVarMessageBox *msg_box = (LLDebugVarMessageBox*)user_data;
	if (!msg_box || !msg_box->mVarData) return;

	switch(msg_box->mVarType)
	{
	case VAR_TYPE_F32:
		*((F32*)msg_box->mVarData) = (F32)msg_box->mSlider1->getValue().asReal();
		break;
	case VAR_TYPE_S32:
		*((S32*)msg_box->mVarData) = (S32)msg_box->mSlider1->getValue().asInteger();
		break;
	case VAR_TYPE_VEC3:
	{
		LLVector3* vec_p = (LLVector3*)msg_box->mVarData;
		vec_p->setVec((F32)msg_box->mSlider1->getValue().asReal(), 
			(F32)msg_box->mSlider2->getValue().asReal(), 
			(F32)msg_box->mSlider3->getValue().asReal());
		break;
	}
	default:
		llwarns << "Unhandled var type " << msg_box->mVarType << llendl;
		break;
	}
}

void LLDebugVarMessageBox::onAnimateClicked(void* user_data)
{
	LLDebugVarMessageBox* msg_boxp = (LLDebugVarMessageBox*)user_data;
	msg_boxp->mAnimate = !msg_boxp->mAnimate;
	msg_boxp->mAnimateButton->setToggleState(msg_boxp->mAnimate);
}

void LLDebugVarMessageBox::onClose(bool app_quitting)
{
	setVisible(FALSE);
}

void LLDebugVarMessageBox::draw()
{
	LLString text;
	switch(mVarType)
	{
	  case VAR_TYPE_F32:
		text = llformat("%.3f", *((F32*)mVarData));
		break;
	  case VAR_TYPE_S32:
		text = llformat("%d", *((S32*)mVarData));
		break;
	  case VAR_TYPE_VEC3:
	  {
		  LLVector3* vec_p = (LLVector3*)mVarData;
		  text= llformat("%.3f %.3f %.3f", vec_p->mV[VX], vec_p->mV[VY], vec_p->mV[VZ]);
		  break;
	  }
	  default:
		llwarns << "Unhandled var type " << mVarType << llendl;
		break;
	}
	mText->setText(text);

	if(mAnimate)
	{
		if (mSlider1)
		{
			F32 animated_val = clamp_rescale(fmodf((F32)LLFrameTimer::getElapsedSeconds() / 5.f, 1.f), 0.f, 1.f, 0.f, mSlider1->getMaxValue());
			mSlider1->setValue(animated_val);
			slider_changed(mSlider1, this);
			if (mSlider2)
			{
				mSlider2->setValue(animated_val);
				slider_changed(mSlider2, this);
			}
			if (mSlider3)
			{
				mSlider3->setValue(animated_val);
				slider_changed(mSlider3, this);
			}
		}
	}
	LLFloater::draw();
}
