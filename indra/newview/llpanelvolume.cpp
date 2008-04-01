/** 
 * @file llpanelvolume.cpp
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

// file include
#include "llpanelvolume.h"

// linden library includes
#include "llclickaction.h"
#include "lleconomy.h"
#include "llerror.h"
#include "llfontgl.h"
#include "llflexibleobject.h"
#include "llmaterialtable.h"
#include "llpermissionsflags.h"
#include "llstring.h"
#include "llvolume.h"
#include "m3math.h"
#include "material_codes.h"

// project includes
#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llfirstuse.h"
#include "llfocusmgr.h"
#include "llmanipscale.h"
#include "llpanelinventory.h"
#include "llpreviewscript.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"

#include "lldrawpool.h"
#include "lluictrlfactory.h"

// "Features" Tab

BOOL	LLPanelVolume::postBuild()
{
	// Flexible Objects Parameters
	{
		childSetCommitCallback("Flexible1D Checkbox Ctrl",onCommitIsFlexible,this);
		childSetCommitCallback("FlexNumSections",onCommitFlexible,this);
		childSetValidate("FlexNumSections",precommitValidate);
		childSetCommitCallback("FlexGravity",onCommitFlexible,this);
		childSetValidate("FlexGravity",precommitValidate);
		childSetCommitCallback("FlexFriction",onCommitFlexible,this);
		childSetValidate("FlexFriction",precommitValidate);
		childSetCommitCallback("FlexWind",onCommitFlexible,this);
		childSetValidate("FlexWind",precommitValidate);
		childSetCommitCallback("FlexTension",onCommitFlexible,this);
		childSetValidate("FlexTension",precommitValidate);
		childSetCommitCallback("FlexForceX",onCommitFlexible,this);
		childSetValidate("FlexForceX",precommitValidate);
		childSetCommitCallback("FlexForceY",onCommitFlexible,this);
		childSetValidate("FlexForceY",precommitValidate);
		childSetCommitCallback("FlexForceZ",onCommitFlexible,this);
		childSetValidate("FlexForceZ",precommitValidate);
	}

	// LIGHT Parameters
	{
		childSetCommitCallback("Light Checkbox Ctrl",onCommitIsLight,this);
		LLColorSwatchCtrl*	LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch){
			LightColorSwatch->setOnCancelCallback(onLightCancelColor);
			LightColorSwatch->setOnSelectCallback(onLightSelectColor);
			childSetCommitCallback("colorswatch",onCommitLight,this);
		}
		childSetCommitCallback("Light Intensity",onCommitLight,this);
		childSetValidate("Light Intensity",precommitValidate);
		childSetCommitCallback("Light Radius",onCommitLight,this);
		childSetValidate("Light Radius",precommitValidate);
		childSetCommitCallback("Light Falloff",onCommitLight,this);
		childSetValidate("Light Falloff",precommitValidate);
	}
	
	// Start with everyone disabled
	clearCtrls();

	return TRUE;
}

LLPanelVolume::LLPanelVolume(const std::string& name)
	:	LLPanel(name)
{
	setMouseOpaque(FALSE);

}


LLPanelVolume::~LLPanelVolume()
{
	// Children all cleaned up by default view destructor.
}

void LLPanelVolume::getState( )
{
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject();
	LLViewerObject* root_objectp = objectp;
	if(!objectp)
	{
		objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
		// *FIX: shouldn't we just keep the child?
		if (objectp)
		{
			LLViewerObject* parentp = objectp->getSubParent();

			if (parentp)
			{
				root_objectp = parentp;
			}
			else
			{
				root_objectp = objectp;
			}
		}
	}

	LLVOVolume *volobjp = NULL;
	if ( objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
	{
		volobjp = (LLVOVolume *)objectp;
	}
	
	if( !objectp )
	{
		//forfeit focus
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			gFocusMgr.setKeyboardFocus(NULL);
		}

		// Disable all text input fields
		clearCtrls();

		return;
	}

	BOOL owners_identical;
	LLUUID owner_id;
	LLString owner_name;
	owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);

	// BUG? Check for all objects being editable?
	BOOL editable = root_objectp->permModify();
	BOOL single_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME )
		&& LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1;

	// Select Single Message
	if (single_volume)
	{
		childSetVisible("edit_object",true);
		childSetEnabled("edit_object",true);
		childSetVisible("select_single",false);
	}
	else
	{	
		childSetVisible("edit_object",false);
		childSetVisible("select_single",true);
		childSetEnabled("select_single",true);
	}
	
	// Light properties
	BOOL is_light = volobjp && volobjp->getIsLight();
	childSetValue("Light Checkbox Ctrl",is_light);
	childSetEnabled("Light Checkbox Ctrl",editable && single_volume && volobjp);
	
	if (is_light && editable && single_volume)
	{
		childSetEnabled("label color",true);
		//mLabelColor		 ->setEnabled( TRUE );
		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( TRUE );
			LightColorSwatch->setValid( TRUE );
			LightColorSwatch->set(volobjp->getLightBaseColor());
		}
		childSetEnabled("Light Intensity",true);
		childSetEnabled("Light Radius",true);
		childSetEnabled("Light Falloff",true);

		childSetValue("Light Intensity",volobjp->getLightIntensity());
		childSetValue("Light Radius",volobjp->getLightRadius());
		childSetValue("Light Falloff",volobjp->getLightFalloff());

		mLightSavedColor = volobjp->getLightColor();
	}
	else
	{
		getChild<LLSpinCtrl>("Light Intensity", true)->clear();
		getChild<LLSpinCtrl>("Light Radius", true)->clear();
		getChild<LLSpinCtrl>("Light Falloff", true)->clear();

		childSetEnabled("label color",false);	
		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( FALSE );
			LightColorSwatch->setValid( FALSE );
		}
		childSetEnabled("Light Intensity",false);
		childSetEnabled("Light Radius",false);
		childSetEnabled("Light Falloff",false);
	}
	
	// Flexible properties
	BOOL is_flexible = volobjp && volobjp->isFlexible();
	childSetValue("Flexible1D Checkbox Ctrl",is_flexible);
	if (is_flexible || (volobjp && volobjp->canBeFlexible()))
	{
		childSetEnabled("Flexible1D Checkbox Ctrl", editable && single_volume && volobjp);
	}
	else
	{
		childSetEnabled("Flexible1D Checkbox Ctrl", false);
	}
	if (is_flexible && editable && single_volume)
	{
		childSetVisible("FlexNumSections",true);
		childSetVisible("FlexGravity",true);
		childSetVisible("FlexTension",true);
		childSetVisible("FlexFriction",true);
		childSetVisible("FlexWind",true);
		childSetVisible("FlexForceX",true);
		childSetVisible("FlexForceY",true);
		childSetVisible("FlexForceZ",true);

		childSetEnabled("FlexNumSections",true);
		childSetEnabled("FlexGravity",true);
		childSetEnabled("FlexTension",true);
		childSetEnabled("FlexFriction",true);
		childSetEnabled("FlexWind",true);
		childSetEnabled("FlexForceX",true);
		childSetEnabled("FlexForceY",true);
		childSetEnabled("FlexForceZ",true);

		LLFlexibleObjectData *attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
		
		childSetValue("FlexNumSections",(F32)attributes->getSimulateLOD());
		childSetValue("FlexGravity",attributes->getGravity());
		childSetValue("FlexTension",attributes->getTension());
		childSetValue("FlexFriction",attributes->getAirFriction());
		childSetValue("FlexWind",attributes->getWindSensitivity());
		childSetValue("FlexForceX",attributes->getUserForce().mV[VX]);
		childSetValue("FlexForceY",attributes->getUserForce().mV[VY]);
		childSetValue("FlexForceZ",attributes->getUserForce().mV[VZ]);
	}
	else
	{
		getChild<LLSpinCtrl>("FlexNumSections", true)->clear();
		getChild<LLSpinCtrl>("FlexGravity", true)->clear();
		getChild<LLSpinCtrl>("FlexTension", true)->clear();
		getChild<LLSpinCtrl>("FlexFriction", true)->clear();
		getChild<LLSpinCtrl>("FlexWind", true)->clear();
		getChild<LLSpinCtrl>("FlexForceX", true)->clear();
		getChild<LLSpinCtrl>("FlexForceY", true)->clear();
		getChild<LLSpinCtrl>("FlexForceZ", true)->clear();

		childSetEnabled("FlexNumSections",false);
		childSetEnabled("FlexGravity",false);
		childSetEnabled("FlexTension",false);
		childSetEnabled("FlexFriction",false);
		childSetEnabled("FlexWind",false);
		childSetEnabled("FlexForceX",false);
		childSetEnabled("FlexForceY",false);
		childSetEnabled("FlexForceZ",false);
	}
	
	mObject = objectp;
	mRootObject = root_objectp;
}

// static
BOOL LLPanelVolume::precommitValidate( LLUICtrl* ctrl, void* userdata )
{
	// TODO: Richard will fill this in later.  
	return TRUE; // FALSE means that validation failed and new value should not be commited.
}


void LLPanelVolume::refresh()
{
	getState();
	if (mObject.notNull() && mObject->isDead())
	{
		mObject = NULL;
	}

	if (mRootObject.notNull() && mRootObject->isDead())
	{
		mRootObject = NULL;
	}
}


void LLPanelVolume::draw()
{
	LLPanel::draw();
}

// virtual
void LLPanelVolume::clearCtrls()
{
	LLPanel::clearCtrls();

	childSetEnabled("select_single",false);
	childSetVisible("select_single",true);
	childSetEnabled("edit_object",false);
	childSetVisible("edit_object",false);
	childSetEnabled("Light Checkbox Ctrl",false);
	childSetEnabled("label color",false);
	childSetEnabled("label color",false);
	LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LightColorSwatch->setEnabled( FALSE );
		LightColorSwatch->setValid( FALSE );
	}
	childSetEnabled("Light Intensity",false);
	childSetEnabled("Light Radius",false);
	childSetEnabled("Light Falloff",false);

	childSetEnabled("Flexible1D Checkbox Ctrl",false);
	childSetEnabled("FlexNumSections",false);
	childSetEnabled("FlexGravity",false);
	childSetEnabled("FlexTension",false);
	childSetEnabled("FlexFriction",false);
	childSetEnabled("FlexWind",false);
	childSetEnabled("FlexForceX",false);
	childSetEnabled("FlexForceY",false);
	childSetEnabled("FlexForceZ",false);
}

//
// Static functions
//

void LLPanelVolume::sendIsLight()
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;
	
	BOOL value = childGetValue("Light Checkbox Ctrl");
	volobjp->setIsLight(value);
	llinfos << "update light sent" << llendl;
}

void LLPanelVolume::sendIsFlexible()
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;
	
	BOOL is_flexible = childGetValue("Flexible1D Checkbox Ctrl");
	//BOOL is_flexible = mCheckFlexible1D->get();

	if (is_flexible)
	{
		LLFirstUse::useFlexible();

		if (objectp->getClickAction() == CLICK_ACTION_SIT)
		{
			LLSelectMgr::getInstance()->selectionSetClickAction(CLICK_ACTION_NONE);
		}

	}

	if (volobjp->setIsFlexible(is_flexible))
	{
		mObject->sendShapeUpdate();
		LLSelectMgr::getInstance()->selectionUpdatePhantom(volobjp->flagPhantom());
	}

	llinfos << "update flexible sent" << llendl;
}

void LLPanelVolume::onLightCancelColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	LLColorSwatchCtrl*	LightColorSwatch = self->getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LightColorSwatch->setColor(self->mLightSavedColor);
	}
	onLightSelectColor(NULL, userdata);
}

void LLPanelVolume::onLightSelectColor(LLUICtrl* ctrl, void* userdata)
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	LLViewerObject* objectp = self->mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;


	LLColorSwatchCtrl*	LightColorSwatch = self->getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LLColor4	clr = LightColorSwatch->get();
		LLColor3	clr3( clr );
		volobjp->setLightColor(clr3);
		self->mLightSavedColor = clr;
	}
}

// static
void LLPanelVolume::onCommitLight( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	LLViewerObject* objectp = self->mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;
	
	
	volobjp->setLightIntensity((F32)self->childGetValue("Light Intensity").asReal());
	volobjp->setLightRadius((F32)self->childGetValue("Light Radius").asReal());
	volobjp->setLightFalloff((F32)self->childGetValue("Light Falloff").asReal());
	LLColorSwatchCtrl*	LightColorSwatch = self->getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LLColor4	clr = LightColorSwatch->get();
		volobjp->setLightColor(LLColor3(clr));
	}
}

// static
void LLPanelVolume::onCommitIsLight( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	self->sendIsLight();
}

//----------------------------------------------------------------------------

// static
void LLPanelVolume::onCommitFlexible( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	LLViewerObject* objectp = self->mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}
	
	LLFlexibleObjectData *attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
	if (attributes)
	{
		LLFlexibleObjectData new_attributes;
		new_attributes = *attributes;


		new_attributes.setSimulateLOD(self->childGetValue("FlexNumSections").asInteger());//(S32)self->mSpinSections->get());
		new_attributes.setGravity((F32)self->childGetValue("FlexGravity").asReal());
		new_attributes.setTension((F32)self->childGetValue("FlexTension").asReal());
		new_attributes.setAirFriction((F32)self->childGetValue("FlexFriction").asReal());
		new_attributes.setWindSensitivity((F32)self->childGetValue("FlexWind").asReal());
		F32 fx = (F32)self->childGetValue("FlexForceX").asReal();
		F32 fy = (F32)self->childGetValue("FlexForceY").asReal();
		F32 fz = (F32)self->childGetValue("FlexForceZ").asReal();
		LLVector3 force(fx,fy,fz);

		new_attributes.setUserForce(force);
		objectp->setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, new_attributes, true);
	}

	// Values may fail validation
	self->refresh();
}

// static
void LLPanelVolume::onCommitIsFlexible( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	self->sendIsFlexible();
}

