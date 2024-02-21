/** 
 * @file lldebugmessagebox.cpp
 * @brief Implementation of a simple, non-modal message box.
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

std::map<std::string, LLDebugVarMessageBox*> LLDebugVarMessageBox::sInstances;

LLDebugVarMessageBox::LLDebugVarMessageBox(const std::string& title, EDebugVarType var_type, void *var) : 
	LLFloater(LLSD()),
	mVarType(var_type), mVarData(var), mAnimate(false)
{
	setRect(LLRect(10,160,400,10));
	
	LLSliderCtrl::Params slider_p;
	slider_p.label(title);
	slider_p.label_width(70);
	slider_p.text_width(40);
	slider_p.can_edit_text(true);
	slider_p.show_text(true);

	mSlider1 = NULL;
	mSlider2 = NULL;
	mSlider3 = NULL;

	switch(var_type)
	{
	case VAR_TYPE_F32:
		slider_p.name("slider 1");
		slider_p.rect(LLRect(20,130,190,110));
		slider_p.initial_value(*((F32*)var));
		slider_p.min_value(-100.f);
		slider_p.max_value(100.f);
		slider_p.increment(0.1f);
		slider_p.decimal_digits(3);
		mSlider1 = LLUICtrlFactory::create<LLSliderCtrl>(slider_p);
		addChild(mSlider1);
		break;
	case VAR_TYPE_S32:
		slider_p.name("slider 1");
		slider_p.rect(LLRect(20,100,190,80));
		slider_p.initial_value((F32)*((S32*)var));
		slider_p.min_value(-255.f);
		slider_p.max_value(255.f);
		slider_p.increment(1.f);
		slider_p.decimal_digits(0);
		mSlider1 = LLUICtrlFactory::create<LLSliderCtrl>(slider_p);
		addChild(mSlider1);
		break;
	case VAR_TYPE_VEC3:
		slider_p.name("slider 1");
		slider_p.label("x: ");
		slider_p.rect(LLRect(20,130,190,110));
		slider_p.initial_value(((LLVector3*)var)->mV[VX]);
		slider_p.min_value(-100.f);
		slider_p.max_value(100.f);
		slider_p.increment(0.1f);
		slider_p.decimal_digits(3);
		mSlider1 = LLUICtrlFactory::create<LLSliderCtrl>(slider_p);

		slider_p.name("slider 2");
		slider_p.label("y: ");
		slider_p.rect(LLRect(20,100,190,80));
		slider_p.initial_value(((LLVector3*)var)->mV[VY]);
		mSlider2 = LLUICtrlFactory::create<LLSliderCtrl>(slider_p);

		slider_p.name("slider 3");
		slider_p.label("z: ");
		slider_p.rect(LLRect(20,70,190,50));
		slider_p.initial_value(((LLVector3*)var)->mV[VZ]);
		mSlider2 = LLUICtrlFactory::create<LLSliderCtrl>(slider_p);

		addChild(mSlider1);
		addChild(mSlider2);
		addChild(mSlider3);
		break;
	default:
		LL_WARNS() << "Unhandled var type " << var_type << LL_ENDL;
		break;
	}

	LLButton::Params p;
	p.name(std::string("Animate"));
	p.label(std::string("Animate"));
	p.rect(LLRect(20, 45, 180, 25));
	p.click_callback.function(boost::bind(&LLDebugVarMessageBox::onAnimateClicked, this, _2));
	mAnimateButton = LLUICtrlFactory::create<LLButton>(p);
	addChild(mAnimateButton);

	LLTextBox::Params params;
	params.name("value");
	params.initial_value(params.name());
	params.rect(LLRect(20,20,190,0));
	mText = LLUICtrlFactory::create<LLTextBox> (params);
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
	box->mSlider1->setCommitCallback(boost::bind(&LLDebugVarMessageBox::sliderChanged, box, _2));
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
	box->mSlider1->setCommitCallback(boost::bind(&LLDebugVarMessageBox::sliderChanged, box, _2));
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
	box->mSlider1->setCommitCallback(boost::bind(&LLDebugVarMessageBox::sliderChanged, box, _2));

	box->mSlider2->setMaxValue(max_value.mV[VX]);
	box->mSlider2->setMinValue(-max_value.mV[VX]);
	box->mSlider2->setIncrement(increment.mV[VX]);
	box->mSlider2->setCommitCallback(boost::bind(&LLDebugVarMessageBox::sliderChanged, box, _2));

	box->mSlider3->setMaxValue(max_value.mV[VX]);
	box->mSlider3->setMinValue(-max_value.mV[VX]);
	box->mSlider3->setIncrement(increment.mV[VX]);
	box->mSlider3->setCommitCallback(boost::bind(&LLDebugVarMessageBox::sliderChanged, box, _2));
#endif
}

LLDebugVarMessageBox* LLDebugVarMessageBox::show(const std::string& title, EDebugVarType var_type, void *var)
{
	std::string title_string(title);

	LLDebugVarMessageBox *box = sInstances[title_string];
	if (!box)
	{
		box = new LLDebugVarMessageBox(title, var_type, var);
		sInstances[title_string] = box;
		gFloaterView->addChild(box);
		box->reshape(200,150);
		box->openFloater();
		box->mTitle = title_string;
	}

	return box;
}

void LLDebugVarMessageBox::sliderChanged(const LLSD& data)
{
	if (!mVarData)
		return;

	switch(mVarType)
	{
	case VAR_TYPE_F32:
		*((F32*)mVarData) = (F32)mSlider1->getValue().asReal();
		break;
	case VAR_TYPE_S32:
		*((S32*)mVarData) = (S32)mSlider1->getValue().asInteger();
		break;
	case VAR_TYPE_VEC3:
	{
		LLVector3* vec_p = (LLVector3*)mVarData;
		vec_p->setVec((F32)mSlider1->getValue().asReal(), 
			(F32)mSlider2->getValue().asReal(), 
			(F32)mSlider3->getValue().asReal());
		break;
	}
	default:
		LL_WARNS() << "Unhandled var type " << mVarType << LL_ENDL;
		break;
	}
}

void LLDebugVarMessageBox::onAnimateClicked(const LLSD& data)
{
	mAnimate = !mAnimate;
	mAnimateButton->setToggleState(mAnimate);
}

void LLDebugVarMessageBox::draw()
{
	std::string text;
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
		LL_WARNS() << "Unhandled var type " << mVarType << LL_ENDL;
		break;
	}
	mText->setText(text);

	if(mAnimate)
	{
		if (mSlider1)
		{
			F32 animated_val = clamp_rescale(fmodf((F32)LLFrameTimer::getElapsedSeconds() / 5.f, 1.f), 0.f, 1.f, 0.f, mSlider1->getMaxValue());
			mSlider1->setValue(animated_val);
			sliderChanged(LLSD());
			if (mSlider2)
			{
				mSlider2->setValue(animated_val);
				sliderChanged(LLSD());
			}
			if (mSlider3)
			{
				mSlider3->setValue(animated_val);
				sliderChanged(LLSD());
			}
		}
	}
	LLFloater::draw();
}
