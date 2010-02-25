/** 
 * @file llfloatersettingsdebug.cpp
 * @brief floater for debugging internal viewer settings
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "llfloatersettingsdebug.h"
#include "llfloater.h"
#include "lluictrlfactory.h"
//#include "llfirstuse.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"


LLFloaterSettingsDebug::LLFloaterSettingsDebug(const LLSD& key) 
:	LLFloater(key)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_settings_debug.xml");
	mCommitCallbackRegistrar.add("SettingSelect",	boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this,_1));
	mCommitCallbackRegistrar.add("CommitSettings",	boost::bind(&LLFloaterSettingsDebug::onCommitSettings, this));
	mCommitCallbackRegistrar.add("ClickDefault",	boost::bind(&LLFloaterSettingsDebug::onClickDefault, this));

}

LLFloaterSettingsDebug::~LLFloaterSettingsDebug()
{}

BOOL LLFloaterSettingsDebug::postBuild()
{
	LLComboBox* settings_combo = getChild<LLComboBox>("settings_combo");

	struct f : public LLControlGroup::ApplyFunctor
	{
		LLComboBox* combo;
		f(LLComboBox* c) : combo(c) {}
		virtual void apply(const std::string& name, LLControlVariable* control)
		{
			if (!control->isHiddenFromSettingsEditor())
			{
				combo->add(name, (void*)control);
			}
		}
	} func(settings_combo);

	std::string key = getKey().asString();
	if (key == "all" || key == "base")
	{
		gSavedSettings.applyToAll(&func);
	}
	if (key == "all" || key == "account")
	{
		gSavedPerAccountSettings.applyToAll(&func);
	}

	settings_combo->sortByName();
	settings_combo->updateSelection();
	mComment = getChild<LLTextEditor>("comment_text");
	return TRUE;
}

void LLFloaterSettingsDebug::draw()
{
	LLComboBox* settings_combo = getChild<LLComboBox>("settings_combo");
	LLControlVariable* controlp = (LLControlVariable*)settings_combo->getCurrentUserdata();
	updateControl(controlp);

	LLFloater::draw();
}

//static 
void LLFloaterSettingsDebug::onSettingSelect(LLUICtrl* ctrl)
{
	LLComboBox* combo_box = (LLComboBox*)ctrl;
	LLControlVariable* controlp = (LLControlVariable*)combo_box->getCurrentUserdata();

	updateControl(controlp);
}

void LLFloaterSettingsDebug::onCommitSettings()
{
	LLComboBox* settings_combo = getChild<LLComboBox>("settings_combo");
	LLControlVariable* controlp = (LLControlVariable*)settings_combo->getCurrentUserdata();

	if (!controlp)
	{
		return;
	}

	LLVector3 vector;
	LLVector3d vectord;
	LLRect rect;
	LLColor4 col4;
	LLColor3 col3;
	LLColor4U col4U;
	LLColor4 color_with_alpha;

	switch(controlp->type())
	{		
	  case TYPE_U32:
		controlp->set(childGetValue("val_spinner_1"));
		break;
	  case TYPE_S32:
		controlp->set(childGetValue("val_spinner_1"));
		break;
	  case TYPE_F32:
		controlp->set(LLSD(childGetValue("val_spinner_1").asReal()));
		break;
	  case TYPE_BOOLEAN:
		controlp->set(childGetValue("boolean_combo"));
		break;
	  case TYPE_STRING:
		controlp->set(LLSD(childGetValue("val_text").asString()));
		break;
	  case TYPE_VEC3:
		vector.mV[VX] = (F32)childGetValue("val_spinner_1").asReal();
		vector.mV[VY] = (F32)childGetValue("val_spinner_2").asReal();
		vector.mV[VZ] = (F32)childGetValue("val_spinner_3").asReal();
		controlp->set(vector.getValue());
		break;
	  case TYPE_VEC3D:
		vectord.mdV[VX] = childGetValue("val_spinner_1").asReal();
		vectord.mdV[VY] = childGetValue("val_spinner_2").asReal();
		vectord.mdV[VZ] = childGetValue("val_spinner_3").asReal();
		controlp->set(vectord.getValue());
		break;
	  case TYPE_RECT:
		rect.mLeft = childGetValue("val_spinner_1").asInteger();
		rect.mRight = childGetValue("val_spinner_2").asInteger();
		rect.mBottom = childGetValue("val_spinner_3").asInteger();
		rect.mTop = childGetValue("val_spinner_4").asInteger();
		controlp->set(rect.getValue());
		break;
	  case TYPE_COL4:
		col3.setValue(childGetValue("val_color_swatch"));
		col4 = LLColor4(col3, (F32)childGetValue("val_spinner_4").asReal());
		controlp->set(col4.getValue());
		break;
	  case TYPE_COL3:
		controlp->set(childGetValue("val_color_swatch"));
		//col3.mV[VRED] = (F32)floaterp->childGetValue("val_spinner_1").asC();
		//col3.mV[VGREEN] = (F32)floaterp->childGetValue("val_spinner_2").asReal();
		//col3.mV[VBLUE] = (F32)floaterp->childGetValue("val_spinner_3").asReal();
		//controlp->set(col3.getValue());
		break;
	  default:
		break;
	}
}

// static
void LLFloaterSettingsDebug::onClickDefault()
{
	LLComboBox* settings_combo = getChild<LLComboBox>("settings_combo");
	LLControlVariable* controlp = (LLControlVariable*)settings_combo->getCurrentUserdata();

	if (controlp)
	{
		controlp->resetToDefault();
		updateControl(controlp);
	}
}

// we've switched controls, or doing per-frame update, so update spinners, etc.
void LLFloaterSettingsDebug::updateControl(LLControlVariable* controlp)
{
	LLSpinCtrl* spinner1 = getChild<LLSpinCtrl>("val_spinner_1");
	LLSpinCtrl* spinner2 = getChild<LLSpinCtrl>("val_spinner_2");
	LLSpinCtrl* spinner3 = getChild<LLSpinCtrl>("val_spinner_3");
	LLSpinCtrl* spinner4 = getChild<LLSpinCtrl>("val_spinner_4");
	LLColorSwatchCtrl* color_swatch = getChild<LLColorSwatchCtrl>("val_color_swatch");

	if (!spinner1 || !spinner2 || !spinner3 || !spinner4 || !color_swatch)
	{
		llwarns << "Could not find all desired controls by name"
			<< llendl;
		return;
	}

	spinner1->setVisible(FALSE);
	spinner2->setVisible(FALSE);
	spinner3->setVisible(FALSE);
	spinner4->setVisible(FALSE);
	color_swatch->setVisible(FALSE);
	childSetVisible("val_text", FALSE);
	mComment->setText(LLStringUtil::null);

	if (controlp)
	{
		eControlType type = controlp->type();

		//hide combo box only for non booleans, otherwise this will result in the combo box closing every frame
		childSetVisible("boolean_combo", type == TYPE_BOOLEAN);
		

		mComment->setText(controlp->getComment());
		spinner1->setMaxValue(F32_MAX);
		spinner2->setMaxValue(F32_MAX);
		spinner3->setMaxValue(F32_MAX);
		spinner4->setMaxValue(F32_MAX);
		spinner1->setMinValue(-F32_MAX);
		spinner2->setMinValue(-F32_MAX);
		spinner3->setMinValue(-F32_MAX);
		spinner4->setMinValue(-F32_MAX);
		if (!spinner1->hasFocus())
		{
			spinner1->setIncrement(0.1f);
		}
		if (!spinner2->hasFocus())
		{
			spinner2->setIncrement(0.1f);
		}
		if (!spinner3->hasFocus())
		{
			spinner3->setIncrement(0.1f);
		}
		if (!spinner4->hasFocus())
		{
			spinner4->setIncrement(0.1f);
		}

		LLSD sd = controlp->get();
		switch(type)
		{
		  case TYPE_U32:
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!spinner1->hasFocus())
			{
				spinner1->setValue(sd);
				spinner1->setMinValue((F32)U32_MIN);
				spinner1->setMaxValue((F32)U32_MAX);
				spinner1->setIncrement(1.f);
				spinner1->setPrecision(0);
			}
			break;
		  case TYPE_S32:
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!spinner1->hasFocus())
			{
				spinner1->setValue(sd);
				spinner1->setMinValue((F32)S32_MIN);
				spinner1->setMaxValue((F32)S32_MAX);
				spinner1->setIncrement(1.f);
				spinner1->setPrecision(0);
			}
			break;
		  case TYPE_F32:
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("value")); // Debug, don't translate
			if (!spinner1->hasFocus())
			{
				spinner1->setPrecision(3);
				spinner1->setValue(sd);
			}
			break;
		  case TYPE_BOOLEAN:
			if (!childHasFocus("boolean_combo"))
			{
				if (sd.asBoolean())
				{
					childSetValue("boolean_combo", LLSD("true"));
				}
				else
				{
					childSetValue("boolean_combo", LLSD(""));
				}
			}
			break;
		  case TYPE_STRING:
			childSetVisible("val_text", TRUE);
			if (!childHasFocus("val_text"))
			{
				childSetValue("val_text", sd);
			}
			break;
		  case TYPE_VEC3:
		  {
			LLVector3 v;
			v.setValue(sd);
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("X"));
			spinner2->setVisible(TRUE);
			spinner2->setLabel(std::string("Y"));
			spinner3->setVisible(TRUE);
			spinner3->setLabel(std::string("Z"));
			if (!spinner1->hasFocus())
			{
				spinner1->setPrecision(3);
				spinner1->setValue(v[VX]);
			}
			if (!spinner2->hasFocus())
			{
				spinner2->setPrecision(3);
				spinner2->setValue(v[VY]);
			}
			if (!spinner3->hasFocus())
			{
				spinner3->setPrecision(3);
				spinner3->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_VEC3D:
		  {
			LLVector3d v;
			v.setValue(sd);
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("X"));
			spinner2->setVisible(TRUE);
			spinner2->setLabel(std::string("Y"));
			spinner3->setVisible(TRUE);
			spinner3->setLabel(std::string("Z"));
			if (!spinner1->hasFocus())
			{
				spinner1->setPrecision(3);
				spinner1->setValue(v[VX]);
			}
			if (!spinner2->hasFocus())
			{
				spinner2->setPrecision(3);
				spinner2->setValue(v[VY]);
			}
			if (!spinner3->hasFocus())
			{
				spinner3->setPrecision(3);
				spinner3->setValue(v[VZ]);
			}
			break;
		  }
		  case TYPE_RECT:
		  {
			LLRect r;
			r.setValue(sd);
			spinner1->setVisible(TRUE);
			spinner1->setLabel(std::string("Left"));
			spinner2->setVisible(TRUE);
			spinner2->setLabel(std::string("Right"));
			spinner3->setVisible(TRUE);
			spinner3->setLabel(std::string("Bottom"));
			spinner4->setVisible(TRUE);
			spinner4->setLabel(std::string("Top"));
			if (!spinner1->hasFocus())
			{
				spinner1->setPrecision(0);
				spinner1->setValue(r.mLeft);
			}
			if (!spinner2->hasFocus())
			{
				spinner2->setPrecision(0);
				spinner2->setValue(r.mRight);
			}
			if (!spinner3->hasFocus())
			{
				spinner3->setPrecision(0);
				spinner3->setValue(r.mBottom);
			}
			if (!spinner4->hasFocus())
			{
				spinner4->setPrecision(0);
				spinner4->setValue(r.mTop);
			}

			spinner1->setMinValue((F32)S32_MIN);
			spinner1->setMaxValue((F32)S32_MAX);
			spinner1->setIncrement(1.f);

			spinner2->setMinValue((F32)S32_MIN);
			spinner2->setMaxValue((F32)S32_MAX);
			spinner2->setIncrement(1.f);

			spinner3->setMinValue((F32)S32_MIN);
			spinner3->setMaxValue((F32)S32_MAX);
			spinner3->setIncrement(1.f);

			spinner4->setMinValue((F32)S32_MIN);
			spinner4->setMaxValue((F32)S32_MAX);
			spinner4->setIncrement(1.f);
			break;
		  }
		  case TYPE_COL4:
		  {
			LLColor4 clr;
			clr.setValue(sd);
			color_swatch->setVisible(TRUE);
			// only set if changed so color picker doesn't update
			if(clr != LLColor4(color_swatch->getValue()))
			{
				color_swatch->set(LLColor4(sd), TRUE, FALSE);
			}
			spinner4->setVisible(TRUE);
			spinner4->setLabel(std::string("Alpha"));
			if (!spinner4->hasFocus())
			{
				spinner4->setPrecision(3);
				spinner4->setMinValue(0.0);
				spinner4->setMaxValue(1.f);
				spinner4->setValue(clr.mV[VALPHA]);
			}
			break;
		  }
		  case TYPE_COL3:
		  {
			LLColor3 clr;
			clr.setValue(sd);
			color_swatch->setVisible(TRUE);
			color_swatch->setValue(sd);
			break;
		  }
		  default:
			mComment->setText(std::string("unknown"));
			break;
		}
	}

}
