/** 
 * @file llpanelvolume.cpp
 * @brief Object editing (position, scale, etc.) in the tools floater
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "lltexturectrl.h"
#include "llcombobox.h"
//#include "llfirstuse.h"
#include "llfocusmgr.h"
#include "llmanipscale.h"
#include "llpreviewscript.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llnotificationsutil.h"

#include "lldrawpool.h"
#include "lluictrlfactory.h"

// For mesh physics
#include "llagent.h"
#include "llviewercontrol.h"
#include "llmeshrepository.h"

#include <boost/bind.hpp>

// "Features" Tab

BOOL	LLPanelVolume::postBuild()
{
	// Flexible Objects Parameters
	{
		childSetCommitCallback("Flexible1D Checkbox Ctrl", boost::bind(&LLPanelVolume::onCommitIsFlexible, this, _1, _2), NULL);
		childSetCommitCallback("FlexNumSections",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexNumSections")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexGravity",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexGravity")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexFriction",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexFriction")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexWind",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexWind")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexTension",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexTension")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexForceX",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexForceX")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexForceY",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexForceY")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("FlexForceZ",onCommitFlexible,this);
		getChild<LLUICtrl>("FlexForceZ")->setValidateBeforeCommit(precommitValidate);
	}

	// LIGHT Parameters
	{
		childSetCommitCallback("Light Checkbox Ctrl",onCommitIsLight,this);
		LLColorSwatchCtrl*	LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch){
			LightColorSwatch->setOnCancelCallback(boost::bind(&LLPanelVolume::onLightCancelColor, this, _2));
			LightColorSwatch->setOnSelectCallback(boost::bind(&LLPanelVolume::onLightSelectColor, this, _2));
			childSetCommitCallback("colorswatch",onCommitLight,this);
		}

		LLTextureCtrl* LightTexPicker = getChild<LLTextureCtrl>("light texture control");
		if (LightTexPicker)
		{
			LightTexPicker->setOnCancelCallback(boost::bind(&LLPanelVolume::onLightCancelTexture, this, _2));
			LightTexPicker->setOnSelectCallback(boost::bind(&LLPanelVolume::onLightSelectTexture, this, _2));
			childSetCommitCallback("light texture control", onCommitLight, this);
		}

		childSetCommitCallback("Light Intensity",onCommitLight,this);
		getChild<LLUICtrl>("Light Intensity")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("Light Radius",onCommitLight,this);
		getChild<LLUICtrl>("Light Radius")->setValidateBeforeCommit(precommitValidate);
		childSetCommitCallback("Light Falloff",onCommitLight,this);
		getChild<LLUICtrl>("Light Falloff")->setValidateBeforeCommit(precommitValidate);
		
		childSetCommitCallback("Light FOV", onCommitLight, this);
		getChild<LLUICtrl>("Light FOV")->setValidateBeforeCommit( precommitValidate);
		childSetCommitCallback("Light Focus", onCommitLight, this);
		getChild<LLUICtrl>("Light Focus")->setValidateBeforeCommit( precommitValidate);
		childSetCommitCallback("Light Ambiance", onCommitLight, this);
		getChild<LLUICtrl>("Light Ambiance")->setValidateBeforeCommit( precommitValidate);
	}
	
	// PHYSICS Parameters
	{
		// PhysicsShapeType combobox
		mComboPhysicsShapeType = getChild<LLComboBox>("Physics Shape Type Combo Ctrl");
		mComboPhysicsShapeType->setCommitCallback(boost::bind(&LLPanelVolume::sendPhysicsShapeType, this, _1, mComboPhysicsShapeType));
	
		// PhysicsGravity
		mSpinPhysicsGravity = getChild<LLSpinCtrl>("Physics Gravity");
		mSpinPhysicsGravity->setCommitCallback(boost::bind(&LLPanelVolume::sendPhysicsGravity, this, _1, mSpinPhysicsGravity));

		// PhysicsFriction
		mSpinPhysicsFriction = getChild<LLSpinCtrl>("Physics Friction");
		mSpinPhysicsFriction->setCommitCallback(boost::bind(&LLPanelVolume::sendPhysicsFriction, this, _1, mSpinPhysicsFriction));

		// PhysicsDensity
		mSpinPhysicsDensity = getChild<LLSpinCtrl>("Physics Density");
		mSpinPhysicsDensity->setCommitCallback(boost::bind(&LLPanelVolume::sendPhysicsDensity, this, _1, mSpinPhysicsDensity));

		// PhysicsRestitution
		mSpinPhysicsRestitution = getChild<LLSpinCtrl>("Physics Restitution");
		mSpinPhysicsRestitution->setCommitCallback(boost::bind(&LLPanelVolume::sendPhysicsRestitution, this, _1, mSpinPhysicsRestitution));
	}

	std::map<std::string, std::string> material_name_map;
	material_name_map["Stone"]= LLTrans::getString("Stone");
	material_name_map["Metal"]= LLTrans::getString("Metal");	
	material_name_map["Glass"]= LLTrans::getString("Glass");	
	material_name_map["Wood"]= LLTrans::getString("Wood");	
	material_name_map["Flesh"]= LLTrans::getString("Flesh");
	material_name_map["Plastic"]= LLTrans::getString("Plastic");
	material_name_map["Rubber"]= LLTrans::getString("Rubber");	
	material_name_map["Light"]= LLTrans::getString("Light");		
	
	LLMaterialTable::basic.initTableTransNames(material_name_map);

	// material type popup
	mComboMaterial = getChild<LLComboBox>("material");
	childSetCommitCallback("material",onCommitMaterial,this);
	mComboMaterial->removeall();

	for (LLMaterialTable::info_list_t::iterator iter = LLMaterialTable::basic.mMaterialInfoList.begin();
		 iter != LLMaterialTable::basic.mMaterialInfoList.end(); ++iter)
	{
		LLMaterialInfo* minfop = *iter;
		if (minfop->mMCode != LL_MCODE_LIGHT)
		{
			mComboMaterial->add(minfop->mName);  
		}
	}
	mComboMaterialItemCount = mComboMaterial->getItemCount();
	
	// Start with everyone disabled
	clearCtrls();

	return TRUE;
}

LLPanelVolume::LLPanelVolume()
	: LLPanel(),
	  mComboMaterialItemCount(0)
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
			LLViewerObject* parentp = objectp->getRootEdit();

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
	std::string owner_name;
	owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);

	// BUG? Check for all objects being editable?
	BOOL editable = root_objectp->permModify() && !root_objectp->isPermanentEnforced();
	BOOL single_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME )
		&& LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1;

	// Select Single Message
	if (single_volume)
	{
		getChildView("edit_object")->setVisible(true);
		getChildView("edit_object")->setEnabled(true);
		getChildView("select_single")->setVisible(false);
	}
	else
	{	
		getChildView("edit_object")->setVisible(false);
		getChildView("select_single")->setVisible(true);
		getChildView("select_single")->setEnabled(true);
	}
	
	// Light properties
	BOOL is_light = volobjp && volobjp->getIsLight();
	getChild<LLUICtrl>("Light Checkbox Ctrl")->setValue(is_light);
	getChildView("Light Checkbox Ctrl")->setEnabled(editable && single_volume && volobjp);
	
	if (is_light && editable && single_volume)
	{
		getChildView("label color")->setEnabled(true);
		//mLabelColor		 ->setEnabled( TRUE );
		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( TRUE );
			LightColorSwatch->setValid( TRUE );
			LightColorSwatch->set(volobjp->getLightBaseColor());
		}

		LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
		if (LightTextureCtrl)
		{
			LightTextureCtrl->setEnabled(TRUE);
			LightTextureCtrl->setValid(TRUE);
			LightTextureCtrl->setImageAssetID(volobjp->getLightTextureID());
		}

		getChildView("Light Intensity")->setEnabled(true);
		getChildView("Light Radius")->setEnabled(true);
		getChildView("Light Falloff")->setEnabled(true);

		getChildView("Light FOV")->setEnabled(true);
		getChildView("Light Focus")->setEnabled(true);
		getChildView("Light Ambiance")->setEnabled(true);
		
		getChild<LLUICtrl>("Light Intensity")->setValue(volobjp->getLightIntensity());
		getChild<LLUICtrl>("Light Radius")->setValue(volobjp->getLightRadius());
		getChild<LLUICtrl>("Light Falloff")->setValue(volobjp->getLightFalloff());

		LLVector3 params = volobjp->getSpotLightParams();
		getChild<LLUICtrl>("Light FOV")->setValue(params.mV[0]);
		getChild<LLUICtrl>("Light Focus")->setValue(params.mV[1]);
		getChild<LLUICtrl>("Light Ambiance")->setValue(params.mV[2]);

		mLightSavedColor = volobjp->getLightColor();
	}
	else
	{
		getChild<LLSpinCtrl>("Light Intensity", true)->clear();
		getChild<LLSpinCtrl>("Light Radius", true)->clear();
		getChild<LLSpinCtrl>("Light Falloff", true)->clear();

		getChildView("label color")->setEnabled(false);	
		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( FALSE );
			LightColorSwatch->setValid( FALSE );
		}
		LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
		if (LightTextureCtrl)
		{
			LightTextureCtrl->setEnabled(FALSE);
			LightTextureCtrl->setValid(FALSE);
		}

		getChildView("Light Intensity")->setEnabled(false);
		getChildView("Light Radius")->setEnabled(false);
		getChildView("Light Falloff")->setEnabled(false);

		getChildView("Light FOV")->setEnabled(false);
		getChildView("Light Focus")->setEnabled(false);
		getChildView("Light Ambiance")->setEnabled(false);
	}
	
	// Flexible properties
	BOOL is_flexible = volobjp && volobjp->isFlexible();
	getChild<LLUICtrl>("Flexible1D Checkbox Ctrl")->setValue(is_flexible);
	if (is_flexible || (volobjp && volobjp->canBeFlexible()))
	{
		getChildView("Flexible1D Checkbox Ctrl")->setEnabled(editable && single_volume && volobjp && !volobjp->isMesh() && !objectp->isPermanentEnforced());
	}
	else
	{
		getChildView("Flexible1D Checkbox Ctrl")->setEnabled(false);
	}
	if (is_flexible && editable && single_volume)
	{
		getChildView("FlexNumSections")->setVisible(true);
		getChildView("FlexGravity")->setVisible(true);
		getChildView("FlexTension")->setVisible(true);
		getChildView("FlexFriction")->setVisible(true);
		getChildView("FlexWind")->setVisible(true);
		getChildView("FlexForceX")->setVisible(true);
		getChildView("FlexForceY")->setVisible(true);
		getChildView("FlexForceZ")->setVisible(true);

		getChildView("FlexNumSections")->setEnabled(true);
		getChildView("FlexGravity")->setEnabled(true);
		getChildView("FlexTension")->setEnabled(true);
		getChildView("FlexFriction")->setEnabled(true);
		getChildView("FlexWind")->setEnabled(true);
		getChildView("FlexForceX")->setEnabled(true);
		getChildView("FlexForceY")->setEnabled(true);
		getChildView("FlexForceZ")->setEnabled(true);

		LLFlexibleObjectData *attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
		
		getChild<LLUICtrl>("FlexNumSections")->setValue((F32)attributes->getSimulateLOD());
		getChild<LLUICtrl>("FlexGravity")->setValue(attributes->getGravity());
		getChild<LLUICtrl>("FlexTension")->setValue(attributes->getTension());
		getChild<LLUICtrl>("FlexFriction")->setValue(attributes->getAirFriction());
		getChild<LLUICtrl>("FlexWind")->setValue(attributes->getWindSensitivity());
		getChild<LLUICtrl>("FlexForceX")->setValue(attributes->getUserForce().mV[VX]);
		getChild<LLUICtrl>("FlexForceY")->setValue(attributes->getUserForce().mV[VY]);
		getChild<LLUICtrl>("FlexForceZ")->setValue(attributes->getUserForce().mV[VZ]);
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

		getChildView("FlexNumSections")->setEnabled(false);
		getChildView("FlexGravity")->setEnabled(false);
		getChildView("FlexTension")->setEnabled(false);
		getChildView("FlexFriction")->setEnabled(false);
		getChildView("FlexWind")->setEnabled(false);
		getChildView("FlexForceX")->setEnabled(false);
		getChildView("FlexForceY")->setEnabled(false);
		getChildView("FlexForceZ")->setEnabled(false);
	}
	
	// Material properties

	// Update material part
	// slightly inefficient - materials are unique per object, not per TE
	U8 material_code = 0;
	struct f : public LLSelectedTEGetFunctor<U8>
	{
		U8 get(LLViewerObject* object, S32 te)
		{
			return object->getMaterial();
		}
	} func;
	bool material_same = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, material_code );
	std::string LEGACY_FULLBRIGHT_DESC = LLTrans::getString("Fullbright");
	if (editable && single_volume && material_same)
	{
		mComboMaterial->setEnabled( TRUE );
		if (material_code == LL_MCODE_LIGHT)
		{
			if (mComboMaterial->getItemCount() == mComboMaterialItemCount)
			{
				mComboMaterial->add(LEGACY_FULLBRIGHT_DESC);
			}
			mComboMaterial->setSimple(LEGACY_FULLBRIGHT_DESC);
		}
		else
		{
			if (mComboMaterial->getItemCount() != mComboMaterialItemCount)
			{
				mComboMaterial->remove(LEGACY_FULLBRIGHT_DESC);
			}
			
			mComboMaterial->setSimple(std::string(LLMaterialTable::basic.getName(material_code)));
		}
	}
	else
	{
		mComboMaterial->setEnabled( FALSE );
	}

	// Physics properties
	
	mSpinPhysicsGravity->set(objectp->getPhysicsGravity());
	mSpinPhysicsGravity->setEnabled(editable);

	mSpinPhysicsFriction->set(objectp->getPhysicsFriction());
	mSpinPhysicsFriction->setEnabled(editable);
	
	mSpinPhysicsDensity->set(objectp->getPhysicsDensity());
	mSpinPhysicsDensity->setEnabled(editable);
	
	mSpinPhysicsRestitution->set(objectp->getPhysicsRestitution());
	mSpinPhysicsRestitution->setEnabled(editable);

	// update the physics shape combo to include allowed physics shapes
	mComboPhysicsShapeType->removeall();
	mComboPhysicsShapeType->add(getString("None"), LLSD(1));

	BOOL isMesh = FALSE;
	LLSculptParams *sculpt_params = (LLSculptParams *)objectp->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
	if (sculpt_params)
	{
		U8 sculpt_type = sculpt_params->getSculptType();
		U8 sculpt_stitching = sculpt_type & LL_SCULPT_TYPE_MASK;
		isMesh = (sculpt_stitching == LL_SCULPT_TYPE_MESH);
	}

	if(isMesh && objectp)
	{
		const LLVolumeParams &volume_params = objectp->getVolume()->getParams();
		LLUUID mesh_id = volume_params.getSculptID();
		if(gMeshRepo.hasPhysicsShape(mesh_id))
		{
			// if a mesh contains an uploaded or decomposed physics mesh,
			// allow 'Prim'
			mComboPhysicsShapeType->add(getString("Prim"), LLSD(0));			
		}
	}
	else
	{
		// simple prims always allow physics shape prim
		mComboPhysicsShapeType->add(getString("Prim"), LLSD(0));	
	}

	mComboPhysicsShapeType->add(getString("Convex Hull"), LLSD(2));	
	mComboPhysicsShapeType->setValue(LLSD(objectp->getPhysicsShapeType()));
	mComboPhysicsShapeType->setEnabled(editable && !objectp->isPermanentEnforced() && ((root_objectp == NULL) || !root_objectp->isPermanentEnforced()));

	mObject = objectp;
	mRootObject = root_objectp;
}

// static
bool LLPanelVolume::precommitValidate( const LLSD& data )
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

	BOOL visible = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_DEFERRED) > 0 ? TRUE : FALSE;

	getChildView("label texture")->setVisible( visible);
	getChildView("Light FOV")->setVisible( visible);
	getChildView("Light Focus")->setVisible( visible);
	getChildView("Light Ambiance")->setVisible( visible);
	getChildView("light texture control")->setVisible( visible);

	bool enable_mesh = false;

	LLSD sim_features;
	LLViewerRegion *region = gAgent.getRegion();
	if(region)
	{
		LLSD sim_features;
		region->getSimulatorFeatures(sim_features);		 
		enable_mesh = sim_features.has("PhysicsShapeTypes");
	}
	getChildView("label physicsshapetype")->setVisible(enable_mesh);
	getChildView("Physics Shape Type Combo Ctrl")->setVisible(enable_mesh);
	getChildView("Physics Gravity")->setVisible(enable_mesh);
	getChildView("Physics Friction")->setVisible(enable_mesh);
	getChildView("Physics Density")->setVisible(enable_mesh);
	getChildView("Physics Restitution")->setVisible(enable_mesh);
	
    /* TODO: add/remove individual physics shape types as per the PhysicsShapeTypes simulator features */
}


void LLPanelVolume::draw()
{
	LLPanel::draw();
}

// virtual
void LLPanelVolume::clearCtrls()
{
	LLPanel::clearCtrls();

	getChildView("select_single")->setEnabled(false);
	getChildView("select_single")->setVisible(true);
	getChildView("edit_object")->setEnabled(false);
	getChildView("edit_object")->setVisible(false);
	getChildView("Light Checkbox Ctrl")->setEnabled(false);
	getChildView("label color")->setEnabled(false);
	getChildView("label color")->setEnabled(false);
	LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LightColorSwatch->setEnabled( FALSE );
		LightColorSwatch->setValid( FALSE );
	}
	LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
	if(LightTextureCtrl)
	{
		LightTextureCtrl->setEnabled( FALSE );
		LightTextureCtrl->setValid( FALSE );
	}

	getChildView("Light Intensity")->setEnabled(false);
	getChildView("Light Radius")->setEnabled(false);
	getChildView("Light Falloff")->setEnabled(false);

	getChildView("Flexible1D Checkbox Ctrl")->setEnabled(false);
	getChildView("FlexNumSections")->setEnabled(false);
	getChildView("FlexGravity")->setEnabled(false);
	getChildView("FlexTension")->setEnabled(false);
	getChildView("FlexFriction")->setEnabled(false);
	getChildView("FlexWind")->setEnabled(false);
	getChildView("FlexForceX")->setEnabled(false);
	getChildView("FlexForceY")->setEnabled(false);
	getChildView("FlexForceZ")->setEnabled(false);

	mSpinPhysicsGravity->setEnabled(FALSE);
	mSpinPhysicsFriction->setEnabled(FALSE);
	mSpinPhysicsDensity->setEnabled(FALSE);
	mSpinPhysicsRestitution->setEnabled(FALSE);

	mComboMaterial->setEnabled( FALSE );
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
	
	BOOL value = getChild<LLUICtrl>("Light Checkbox Ctrl")->getValue();
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
	
	BOOL is_flexible = getChild<LLUICtrl>("Flexible1D Checkbox Ctrl")->getValue();
	//BOOL is_flexible = mCheckFlexible1D->get();

	if (is_flexible)
	{
		//LLFirstUse::useFlexible();

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

void LLPanelVolume::sendPhysicsShapeType(LLUICtrl* ctrl, void* userdata)
{
	U8 type = ctrl->getValue().asInteger();
	LLSelectMgr::getInstance()->selectionSetPhysicsType(type);

	refreshCost();
}

void LLPanelVolume::sendPhysicsGravity(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetGravity(val);
}

void LLPanelVolume::sendPhysicsFriction(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetFriction(val);
}

void LLPanelVolume::sendPhysicsRestitution(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetRestitution(val);
}

void LLPanelVolume::sendPhysicsDensity(LLUICtrl* ctrl, void* userdata)
{
	F32 val = ctrl->getValue().asReal();
	LLSelectMgr::getInstance()->selectionSetDensity(val);
}

void LLPanelVolume::refreshCost()
{
	LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
	
	if (obj)
	{
		obj->getObjectCost();
	}
}

void LLPanelVolume::onLightCancelColor(const LLSD& data)
{
	LLColorSwatchCtrl*	LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LightColorSwatch->setColor(mLightSavedColor);
	}
	onLightSelectColor(data);
}

void LLPanelVolume::onLightCancelTexture(const LLSD& data)
{
	LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
	if (LightTextureCtrl)
	{
		LightTextureCtrl->setImageAssetID(mLightSavedTexture);
	}
}

void LLPanelVolume::onLightSelectColor(const LLSD& data)
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;


	LLColorSwatchCtrl*	LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LLColor4	clr = LightColorSwatch->get();
		LLColor3	clr3( clr );
		volobjp->setLightColor(clr3);
		mLightSavedColor = clr;
	}
}

void LLPanelVolume::onLightSelectTexture(const LLSD& data)
{
	if (mObject.isNull() || (mObject->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *) mObject.get();


	LLTextureCtrl*	LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
	if(LightTextureCtrl)
	{
		LLUUID id = LightTextureCtrl->getImageAssetID();
		volobjp->setLightTextureID(id);
		mLightSavedTexture = id;
	}
}

// static
void LLPanelVolume::onCommitMaterial( LLUICtrl* ctrl, void* userdata )
{
	//LLPanelObject* self = (LLPanelObject*) userdata;
	LLComboBox* box = (LLComboBox*) ctrl;

	if (box)
	{
		// apply the currently selected material to the object
		const std::string& material_name = box->getSimple();
		std::string LEGACY_FULLBRIGHT_DESC = LLTrans::getString("Fullbright");
		if (material_name != LEGACY_FULLBRIGHT_DESC)
		{
			U8 material_code = LLMaterialTable::basic.getMCode(material_name);
			LLSelectMgr::getInstance()->selectionSetMaterial(material_code);
		}
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
	
	
	volobjp->setLightIntensity((F32)self->getChild<LLUICtrl>("Light Intensity")->getValue().asReal());
	volobjp->setLightRadius((F32)self->getChild<LLUICtrl>("Light Radius")->getValue().asReal());
	volobjp->setLightFalloff((F32)self->getChild<LLUICtrl>("Light Falloff")->getValue().asReal());

	LLColorSwatchCtrl*	LightColorSwatch = self->getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LLColor4	clr = LightColorSwatch->get();
		volobjp->setLightColor(LLColor3(clr));
	}

	LLTextureCtrl*	LightTextureCtrl = self->getChild<LLTextureCtrl>("light texture control");
	if(LightTextureCtrl)
	{
		LLUUID id = LightTextureCtrl->getImageAssetID();
		if (id.notNull())
		{
			if (!volobjp->isLightSpotlight())
			{ //this commit is making this a spot light, set UI to default params
				volobjp->setLightTextureID(id);
				LLVector3 spot_params = volobjp->getSpotLightParams();
				self->getChild<LLUICtrl>("Light FOV")->setValue(spot_params.mV[0]);
				self->getChild<LLUICtrl>("Light Focus")->setValue(spot_params.mV[1]);
				self->getChild<LLUICtrl>("Light Ambiance")->setValue(spot_params.mV[2]);
			}
			else
			{ //modifying existing params
				LLVector3 spot_params;
				spot_params.mV[0] = (F32) self->getChild<LLUICtrl>("Light FOV")->getValue().asReal();
				spot_params.mV[1] = (F32) self->getChild<LLUICtrl>("Light Focus")->getValue().asReal();
				spot_params.mV[2] = (F32) self->getChild<LLUICtrl>("Light Ambiance")->getValue().asReal();
				volobjp->setSpotLightParams(spot_params);
			}
		}
		else if (volobjp->isLightSpotlight())
		{ //no longer a spot light
			volobjp->setLightTextureID(id);
			//self->getChildView("Light FOV")->setEnabled(FALSE);
			//self->getChildView("Light Focus")->setEnabled(FALSE);
			//self->getChildView("Light Ambiance")->setEnabled(FALSE);
		}
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


		new_attributes.setSimulateLOD(self->getChild<LLUICtrl>("FlexNumSections")->getValue().asInteger());//(S32)self->mSpinSections->get());
		new_attributes.setGravity((F32)self->getChild<LLUICtrl>("FlexGravity")->getValue().asReal());
		new_attributes.setTension((F32)self->getChild<LLUICtrl>("FlexTension")->getValue().asReal());
		new_attributes.setAirFriction((F32)self->getChild<LLUICtrl>("FlexFriction")->getValue().asReal());
		new_attributes.setWindSensitivity((F32)self->getChild<LLUICtrl>("FlexWind")->getValue().asReal());
		F32 fx = (F32)self->getChild<LLUICtrl>("FlexForceX")->getValue().asReal();
		F32 fy = (F32)self->getChild<LLUICtrl>("FlexForceY")->getValue().asReal();
		F32 fz = (F32)self->getChild<LLUICtrl>("FlexForceZ")->getValue().asReal();
		LLVector3 force(fx,fy,fz);

		new_attributes.setUserForce(force);
		objectp->setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, new_attributes, true);
	}

	// Values may fail validation
	self->refresh();
}

void LLPanelVolume::onCommitIsFlexible(LLUICtrl *, void*)
{
	if (mObject->flagObjectPermanent())
	{
		LLNotificationsUtil::add("PathfindingLinksets_ChangeToFlexiblePath", LLSD(), LLSD(), boost::bind(&LLPanelVolume::handleResponseChangeToFlexible, this, _1, _2));
	}
	else
	{
		sendIsFlexible();
	}
}

void LLPanelVolume::handleResponseChangeToFlexible(const LLSD &pNotification, const LLSD &pResponse)
{
	if (LLNotificationsUtil::getSelectedOption(pNotification, pResponse) == 0)
	{
		sendIsFlexible();
	}
	else
	{
		getChild<LLUICtrl>("Flexible1D Checkbox Ctrl")->setValue(FALSE);
	}
}
