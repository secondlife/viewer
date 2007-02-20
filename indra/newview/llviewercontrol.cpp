/** 
 * @file llviewercontrol.cpp
 * @brief Viewer configuration
 * @author Richard Nelson
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewercontrol.h"

#include "indra_constants.h"

#include "v3math.h"
#include "v3dmath.h"
#include "llrect.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v3color.h"

#include "llfloater.h"
#include "llvieweruictrlfactory.h"
#include "llfirstuse.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llcolorswatch.h"

LLFloaterSettingsDebug* LLFloaterSettingsDebug::sInstance = NULL;

LLControlGroup gSavedSettings;	// saved at end of session
LLControlGroup gSavedPerAccountSettings; // saved at end of session
LLControlGroup gViewerArt;		// read-only
LLControlGroup gColors;			// read-only
LLControlGroup gCrashSettings;	// saved at end of session

LLString gLastRunVersion;
LLString gCurrentVersion;

LLString gSettingsFileName;
LLString gPerAccountSettingsFileName;

LLFloaterSettingsDebug::LLFloaterSettingsDebug() : LLFloater("Configuration Editor")
{
}

LLFloaterSettingsDebug::~LLFloaterSettingsDebug()
{
	sInstance = NULL;
}

BOOL LLFloaterSettingsDebug::postBuild()
{
	LLComboBox* settings_combo = LLUICtrlFactory::getComboBoxByName(this, "settings_combo");

	LLControlGroup::ctrl_name_table_t::iterator name_it;
	for(name_it = gSavedSettings.mNameTable.begin(); name_it != gSavedSettings.mNameTable.end(); ++name_it)
	{
		settings_combo->add(name_it->first, (void*)name_it->second);
	}
	for(name_it = gSavedPerAccountSettings.mNameTable.begin(); name_it != gSavedPerAccountSettings.mNameTable.end(); ++name_it)
	{
		settings_combo->add(name_it->first, (void*)name_it->second);
	}
	for(name_it = gColors.mNameTable.begin(); name_it != gColors.mNameTable.end(); ++name_it)
	{
		settings_combo->add(name_it->first, (void*)name_it->second);
	}
	settings_combo->sortByName();
	settings_combo->setCommitCallback(onSettingSelect);
	settings_combo->setCallbackUserData(this);
	settings_combo->updateSelection();

	childSetCommitCallback("val_spinner_1", onCommitSettings);
	childSetUserData("val_spinner_1", this);
	childSetCommitCallback("val_spinner_2", onCommitSettings);
	childSetUserData("val_spinner_2", this);
	childSetCommitCallback("val_spinner_3", onCommitSettings);
	childSetUserData("val_spinner_3", this);
	childSetCommitCallback("val_spinner_4", onCommitSettings);
	childSetUserData("val_spinner_4", this);
	childSetCommitCallback("val_text", onCommitSettings);
	childSetUserData("val_text", this);
	childSetCommitCallback("boolean_combo", onCommitSettings);
	childSetUserData("boolean_combo", this);
	childSetCommitCallback("color_swatch", onCommitSettings);
	childSetUserData("color_swatch", this);
	childSetAction("default_btn", onClickDefault, this);
	mComment = (LLTextEditor*)getChildByName("comment_text");
	return TRUE;
}

void LLFloaterSettingsDebug::draw()
{
	LLComboBox* settings_combo = (LLComboBox*)getChildByName("settings_combo");
	LLControlBase* controlp = (LLControlBase*)settings_combo->getCurrentUserdata();
	updateControl(controlp);

	LLFloater::draw();
}

//static
void LLFloaterSettingsDebug::show(void*)
{
	if (sInstance == NULL)
	{
		sInstance = new LLFloaterSettingsDebug();

		gUICtrlFactory->buildFloater(sInstance, "floater_settings_debug.xml");
	}

	sInstance->open();		/* Flawfinder: ignore */
}

//static 
void LLFloaterSettingsDebug::onSettingSelect(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterSettingsDebug* floaterp = (LLFloaterSettingsDebug*)user_data;
	LLComboBox* combo_box = (LLComboBox*)ctrl;
	LLControlBase* controlp = (LLControlBase*)combo_box->getCurrentUserdata();

	floaterp->updateControl(controlp);
}

//static
void LLFloaterSettingsDebug::onCommitSettings(LLUICtrl* ctrl, void* user_data)
{
	LLFloaterSettingsDebug* floaterp = (LLFloaterSettingsDebug*)user_data;

	LLComboBox* settings_combo = (LLComboBox*)floaterp->getChildByName("settings_combo");
	LLControlBase* controlp = (LLControlBase*)settings_combo->getCurrentUserdata();

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
		controlp->set(floaterp->childGetValue("val_spinner_1"));
		break;
	  case TYPE_S32:
		controlp->set(floaterp->childGetValue("val_spinner_1"));
		break;
	  case TYPE_F32:
		controlp->set(LLSD(floaterp->childGetValue("val_spinner_1").asReal()));
		break;
	  case TYPE_BOOLEAN:
		controlp->set(floaterp->childGetValue("boolean_combo"));
		break;
	  case TYPE_STRING:
		controlp->set(LLSD(floaterp->childGetValue("val_text").asString()));
		break;
	  case TYPE_VEC3:
		vector.mV[VX] = (F32)floaterp->childGetValue("val_spinner_1").asReal();
		vector.mV[VY] = (F32)floaterp->childGetValue("val_spinner_2").asReal();
		vector.mV[VZ] = (F32)floaterp->childGetValue("val_spinner_3").asReal();
		controlp->set(vector.getValue());
		break;
	  case TYPE_VEC3D:
		vectord.mdV[VX] = floaterp->childGetValue("val_spinner_1").asReal();
		vectord.mdV[VY] = floaterp->childGetValue("val_spinner_2").asReal();
		vectord.mdV[VZ] = floaterp->childGetValue("val_spinner_3").asReal();
		controlp->set(vectord.getValue());
		break;
	  case TYPE_RECT:
		rect.mLeft = floaterp->childGetValue("val_spinner_1").asInteger();
		rect.mRight = floaterp->childGetValue("val_spinner_2").asInteger();
		rect.mBottom = floaterp->childGetValue("val_spinner_3").asInteger();
		rect.mTop = floaterp->childGetValue("val_spinner_4").asInteger();
		controlp->set(rect.getValue());
		break;
	  case TYPE_COL4:
		col3.setValue(floaterp->childGetValue("color_swatch"));
		col4 = LLColor4(col3, (F32)floaterp->childGetValue("val_spinner_4").asReal());
		controlp->set(col4.getValue());
		break;
	  case TYPE_COL3:
		controlp->set(floaterp->childGetValue("color_swatch"));
		//col3.mV[VRED] = (F32)floaterp->childGetValue("val_spinner_1").asC();
		//col3.mV[VGREEN] = (F32)floaterp->childGetValue("val_spinner_2").asReal();
		//col3.mV[VBLUE] = (F32)floaterp->childGetValue("val_spinner_3").asReal();
		//controlp->set(col3.getValue());
		break;
	  case TYPE_COL4U:
		col3.setValue(floaterp->childGetValue("color_swatch"));
		col4U.setVecScaleClamp(col3);
		col4U.mV[VALPHA] = floaterp->childGetValue("val_spinner_4").asInteger();
		controlp->set(col4U.getValue());
		break;
	  default:
		break;
	}
}

// static
void LLFloaterSettingsDebug::onClickDefault(void* user_data)
{
	LLFloaterSettingsDebug* floaterp = (LLFloaterSettingsDebug*)user_data;
	LLComboBox* settings_combo = (LLComboBox*)floaterp->getChildByName("settings_combo");
	LLControlBase* controlp = (LLControlBase*)settings_combo->getCurrentUserdata();

	if (controlp)
	{
		controlp->resetToDefault();
		floaterp->updateControl(controlp);
	}
}

// we've switched controls, or doing per-frame update, so update spinners, etc.
void LLFloaterSettingsDebug::updateControl(LLControlBase* controlp)
{
	LLSpinCtrl* spinner1 = LLUICtrlFactory::getSpinnerByName(this, "val_spinner_1");
	LLSpinCtrl* spinner2 = LLUICtrlFactory::getSpinnerByName(this, "val_spinner_2");
	LLSpinCtrl* spinner3 = LLUICtrlFactory::getSpinnerByName(this, "val_spinner_3");
	LLSpinCtrl* spinner4 = LLUICtrlFactory::getSpinnerByName(this, "val_spinner_4");
	LLColorSwatchCtrl* color_swatch = LLUICtrlFactory::getColorSwatchByName(this, "color_swatch");
	spinner1->setVisible(FALSE);
	spinner2->setVisible(FALSE);
	spinner3->setVisible(FALSE);
	spinner4->setVisible(FALSE);
	color_swatch->setVisible(FALSE);
	childSetVisible("val_text", FALSE);
	childSetVisible("boolean_combo", FALSE);
	mComment->setText("");

	if (controlp)
	{
		eControlType type = controlp->type();
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
			spinner1->setLabel("value");
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
			spinner1->setLabel("value");
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
			spinner1->setLabel("value");
			if (!spinner1->hasFocus())
			{
				spinner1->setPrecision(3);
				spinner1->setValue(sd);
			}
			break;
		  case TYPE_BOOLEAN:
			childSetVisible("boolean_combo", TRUE);
			
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
			spinner1->setLabel("X");
			spinner2->setVisible(TRUE);
			spinner2->setLabel("Y");
			spinner3->setVisible(TRUE);
			spinner3->setLabel("Z");
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
			spinner1->setLabel("X");
			spinner2->setVisible(TRUE);
			spinner2->setLabel("Y");
			spinner3->setVisible(TRUE);
			spinner3->setLabel("Z");
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
			spinner1->setLabel("Left");
			spinner2->setVisible(TRUE);
			spinner2->setLabel("Right");
			spinner3->setVisible(TRUE);
			spinner3->setLabel("Bottom");
			spinner4->setVisible(TRUE);
			spinner4->setLabel("Top");
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
			spinner4->setLabel("Alpha");
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
		  case TYPE_COL4U:
		  {
			LLColor4U clr;
			clr.setValue(sd);
			color_swatch->setVisible(TRUE);
			if(LLColor4(clr) != LLColor4(color_swatch->getValue()))
			{
				color_swatch->set(LLColor4(clr), TRUE, FALSE);
			}
			spinner4->setVisible(TRUE);
			spinner4->setLabel("Alpha");
			if(!spinner4->hasFocus())
			{
				spinner4->setPrecision(0);
				spinner4->setValue(clr.mV[VALPHA]);
			}

			spinner4->setMinValue(0);
			spinner4->setMaxValue(255);
			spinner4->setIncrement(1.f);

			break;
		  }
		  default:
			mComment->setText("unknown");
			break;
		}
	}

}
