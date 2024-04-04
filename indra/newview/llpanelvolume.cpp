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
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llmanipscale.h"
#include "llinventorymodel.h"
#include "llmenubutton.h"
#include "llpreviewscript.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltooldraganddrop.h"
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

#include "llvoavatarself.h"

#include <boost/bind.hpp>


const F32 DEFAULT_GRAVITY_MULTIPLIER = 1.f;
const F32 DEFAULT_DENSITY = 1000.f;

// "Features" Tab
bool	LLPanelVolume::postBuild()
{
	// Flexible Objects Parameters
	{
		childSetCommitCallback("Animated Mesh Checkbox Ctrl", boost::bind(&LLPanelVolume::onCommitAnimatedMeshCheckbox, this, _1, _2), NULL);
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
	
    // REFLECTION PROBE Parameters
    {
        childSetCommitCallback("Reflection Probe", onCommitIsReflectionProbe, this);
        childSetCommitCallback("Probe Dynamic", onCommitProbe, this);
        childSetCommitCallback("Probe Volume Type", onCommitProbe, this);
        childSetCommitCallback("Probe Ambiance", onCommitProbe, this);
        childSetCommitCallback("Probe Near Clip", onCommitProbe, this);
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

    mMenuClipboardFeatures = getChild<LLMenuButton>("clipboard_features_params_btn");
    mMenuClipboardLight = getChild<LLMenuButton>("clipboard_light_params_btn");

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

	return true;
}

LLPanelVolume::LLPanelVolume()
	: LLPanel(),
	  mComboMaterialItemCount(0)
{
	setMouseOpaque(false);

    mCommitCallbackRegistrar.add("PanelVolume.menuDoToSelected", boost::bind(&LLPanelVolume::menuDoToSelected, this, _2));
    mEnableCallbackRegistrar.add("PanelVolume.menuEnable", boost::bind(&LLPanelVolume::menuEnableItem, this, _2));
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
	LLVOVolume *root_volobjp = NULL;
    if (root_objectp && (root_objectp->getPCode() == LL_PCODE_VOLUME))
    {
        root_volobjp  = (LLVOVolume *)root_objectp;
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

	LLUUID owner_id;
	std::string owner_name;
	LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);

	// BUG? Check for all objects being editable?
	bool editable = root_objectp->permModify() && !root_objectp->isPermanentEnforced();
	bool single_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME )
		&& LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1;
    bool single_root_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME ) && 
        LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() == 1;

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
	bool is_light = volobjp && volobjp->getIsLight();
	getChild<LLUICtrl>("Light Checkbox Ctrl")->setValue(is_light);
	getChildView("Light Checkbox Ctrl")->setEnabled(editable && single_volume && volobjp);
	
	if (is_light && editable && single_volume)
	{
		//mLabelColor		 ->setEnabled( true );
		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( true );
			LightColorSwatch->setValid( true );
			LightColorSwatch->set(volobjp->getLightSRGBBaseColor());
		}

		LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
		if (LightTextureCtrl)
		{
			LightTextureCtrl->setEnabled(true);
			LightTextureCtrl->setValid(true);
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

		mLightSavedColor = volobjp->getLightSRGBBaseColor();
	}
	else
	{
		getChild<LLSpinCtrl>("Light Intensity", true)->clear();
		getChild<LLSpinCtrl>("Light Radius", true)->clear();
		getChild<LLSpinCtrl>("Light Falloff", true)->clear();

		LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(LightColorSwatch)
		{
			LightColorSwatch->setEnabled( false );
			LightColorSwatch->setValid( false );
		}
		LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
		if (LightTextureCtrl)
		{
			LightTextureCtrl->setEnabled(false);
			LightTextureCtrl->setValid(false);

            if (objectp->isAttachment())
            {
                LightTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
            }
            else
            {
                LightTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
            }
		}

		getChildView("Light Intensity")->setEnabled(false);
		getChildView("Light Radius")->setEnabled(false);
		getChildView("Light Falloff")->setEnabled(false);

		getChildView("Light FOV")->setEnabled(false);
		getChildView("Light Focus")->setEnabled(false);
		getChildView("Light Ambiance")->setEnabled(false);
	}

    // Reflection Probe
    bool is_probe = volobjp && volobjp->isReflectionProbe();
    getChild<LLUICtrl>("Reflection Probe")->setValue(is_probe);
    getChildView("Reflection Probe")->setEnabled(editable && single_volume && volobjp && !volobjp->isMesh());

    bool probe_enabled = is_probe && editable && single_volume;

    getChildView("Probe Dynamic")->setEnabled(probe_enabled);
    getChildView("Probe Volume Type")->setEnabled(probe_enabled);
    getChildView("Probe Ambiance")->setEnabled(probe_enabled);
    getChildView("Probe Near Clip")->setEnabled(probe_enabled);

    if (!probe_enabled)
    {
        getChild<LLComboBox>("Probe Volume Type", true)->clear();
        getChild<LLSpinCtrl>("Probe Ambiance", true)->clear();
        getChild<LLSpinCtrl>("Probe Near Clip", true)->clear();
        getChild<LLCheckBoxCtrl>("Probe Dynamic", true)->clear();
    }
    else
    {
        std::string volume_type;
        if (volobjp->getReflectionProbeIsBox())
        {
            volume_type = "Box";
        }
        else
        {
            volume_type = "Sphere";
        }

        getChild<LLComboBox>("Probe Volume Type", true)->setValue(volume_type);
        getChild<LLSpinCtrl>("Probe Ambiance", true)->setValue(volobjp->getReflectionProbeAmbiance());
        getChild<LLSpinCtrl>("Probe Near Clip", true)->setValue(volobjp->getReflectionProbeNearClip());
        getChild<LLCheckBoxCtrl>("Probe Dynamic", true)->setValue(volobjp->getReflectionProbeIsDynamic());
    }

    // Animated Mesh
	bool is_animated_mesh = single_root_volume && root_volobjp && root_volobjp->isAnimatedObject();
	getChild<LLUICtrl>("Animated Mesh Checkbox Ctrl")->setValue(is_animated_mesh);
    bool enabled_animated_object_box = false;
    if (root_volobjp && root_volobjp == volobjp)
    {
        enabled_animated_object_box = single_root_volume && root_volobjp && root_volobjp->canBeAnimatedObject() && editable; 
#if 0
        if (!enabled_animated_object_box)
        {
            LL_INFOS() << "not enabled: srv " << single_root_volume << " root_volobjp " << (bool) root_volobjp << LL_ENDL;
            if (root_volobjp)
            {
                LL_INFOS() << " cba " << root_volobjp->canBeAnimatedObject()
                           << " editable " << editable << " permModify() " << root_volobjp->permModify()
                           << " ispermenf " << root_volobjp->isPermanentEnforced() << LL_ENDL;
            }
        }
#endif
        if (enabled_animated_object_box && !is_animated_mesh && 
            root_volobjp->isAttachment() && !gAgentAvatarp->canAttachMoreAnimatedObjects())
        {
            // Turning this attachment animated would cause us to exceed the limit.
            enabled_animated_object_box = false;
        }
    }
    getChildView("Animated Mesh Checkbox Ctrl")->setEnabled(enabled_animated_object_box);
	
	//refresh any bakes
	if (root_volobjp)
	{
		root_volobjp->refreshBakeTexture();

		LLViewerObject::const_child_list_t& child_list = root_volobjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			iter != child_list.end(); ++iter)
		{
			LLViewerObject* objectp = *iter;
			if (objectp)
			{
				objectp->refreshBakeTexture();
			}
		}

		if (gAgentAvatarp)
		{
			gAgentAvatarp->updateMeshVisibility();
		}
	}

	// Flexible properties
	bool is_flexible = volobjp && volobjp->isFlexible();
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
		mComboMaterial->setEnabled( true );
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
		mComboMaterial->setEnabled( false );
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

	bool isMesh = false;
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

    mMenuClipboardFeatures->setEnabled(editable && single_volume && volobjp); // Note: physics doesn't need to be limited by single volume
    mMenuClipboardLight->setEnabled(editable && single_volume && volobjp);
}

// static
bool LLPanelVolume::precommitValidate( const LLSD& data )
{
	// TODO: Richard will fill this in later.  
	return true; // false means that validation failed and new value should not be commited.
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
	getChildView("Light Checkbox Ctrl")->setEnabled(false);;
	LLColorSwatchCtrl* LightColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(LightColorSwatch)
	{
		LightColorSwatch->setEnabled( false );
		LightColorSwatch->setValid( false );
	}
	LLTextureCtrl* LightTextureCtrl = getChild<LLTextureCtrl>("light texture control");
	if(LightTextureCtrl)
	{
		LightTextureCtrl->setEnabled( false );
		LightTextureCtrl->setValid( false );
	}

	getChildView("Light Intensity")->setEnabled(false);
	getChildView("Light Radius")->setEnabled(false);
	getChildView("Light Falloff")->setEnabled(false);

    getChildView("Reflection Probe")->setEnabled(false);;
    getChildView("Probe Volume Type")->setEnabled(false);
    getChildView("Probe Dynamic")->setEnabled(false);
    getChildView("Probe Ambiance")->setEnabled(false);
    getChildView("Probe Near Clip")->setEnabled(false);
    getChildView("Animated Mesh Checkbox Ctrl")->setEnabled(false);
	getChildView("Flexible1D Checkbox Ctrl")->setEnabled(false);
	getChildView("FlexNumSections")->setEnabled(false);
	getChildView("FlexGravity")->setEnabled(false);
	getChildView("FlexTension")->setEnabled(false);
	getChildView("FlexFriction")->setEnabled(false);
	getChildView("FlexWind")->setEnabled(false);
	getChildView("FlexForceX")->setEnabled(false);
	getChildView("FlexForceY")->setEnabled(false);
	getChildView("FlexForceZ")->setEnabled(false);

	mSpinPhysicsGravity->setEnabled(false);
	mSpinPhysicsFriction->setEnabled(false);
	mSpinPhysicsDensity->setEnabled(false);
	mSpinPhysicsRestitution->setEnabled(false);

	mComboMaterial->setEnabled( false );
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
	
	bool value = getChild<LLUICtrl>("Light Checkbox Ctrl")->getValue();
	volobjp->setIsLight(value);
	LL_INFOS() << "update light sent" << LL_ENDL;
}

void notify_cant_select_reflection_probe()
{
    if (!gSavedSettings.getBOOL("SelectReflectionProbes"))
    {
        LLNotificationsUtil::add("CantSelectReflectionProbe");
    }
}

void LLPanelVolume::sendIsReflectionProbe()
{
    LLViewerObject* objectp = mObject;
    if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
    {
        return;
    }
    LLVOVolume* volobjp = (LLVOVolume*)objectp;

    bool value = getChild<LLUICtrl>("Reflection Probe")->getValue();
    bool old_value = volobjp->isReflectionProbe();

    if (value && value != old_value)
    { // defer to notification util as to whether or not we *really* make this object a reflection probe
        LLNotificationsUtil::add("ReflectionProbeApplied", LLSD(), LLSD(), boost::bind(&LLPanelVolume::doSendIsReflectionProbe, this, _1, _2));
    }
    else
    {
        if (value)
        {
            notify_cant_select_reflection_probe();
        }
        else if (objectp->flagPhantom())
        {
            LLViewerObject* root = objectp->getRootEdit();
            bool in_linkeset = root != objectp || objectp->numChildren() > 0;
            if (in_linkeset)
            {
                // In linkset with a phantom flag
                objectp->setFlags(FLAGS_PHANTOM, false);
            }
        }
        volobjp->setIsReflectionProbe(value);
    }
}

void LLPanelVolume::doSendIsReflectionProbe(const LLSD & notification, const LLSD & response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        LLViewerObject* objectp = mObject;
        if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
        {
            return;
        }
        LLVOVolume* volobjp = (LLVOVolume*)objectp;

        notify_cant_select_reflection_probe();
        volobjp->setIsReflectionProbe(true);

        { // has become a reflection probe, slam to a 10m sphere and pop up a message
            // warning people about the pitfalls of reflection probes

            auto* select_mgr = LLSelectMgr::getInstance();

            select_mgr->selectionUpdatePhantom(true);
            select_mgr->selectionSetGLTFMaterial(LLUUID::null);
            select_mgr->selectionSetAlphaOnly(0.f);

            LLVolumeParams params;
            params.getPathParams().setCurveType(LL_PCODE_PATH_CIRCLE);
            params.getProfileParams().setCurveType(LL_PCODE_PROFILE_CIRCLE_HALF);
            mObject->updateVolume(params);
        }
    }
    else
    {
        // cancelled, touch up UI state
        getChild<LLUICtrl>("Reflection Probe")->setValue(false);
    }
}

void LLPanelVolume::sendIsFlexible()
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
	}	
	LLVOVolume *volobjp = (LLVOVolume *)objectp;
	
	bool is_flexible = getChild<LLUICtrl>("Flexible1D Checkbox Ctrl")->getValue();
	//bool is_flexible = mCheckFlexible1D->get();

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

	LL_INFOS() << "update flexible sent" << LL_ENDL;
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
	LLVOVolume *volobjp = (LLVOVolume *) mObject.get();

	if (volobjp && LightTextureCtrl)
	{
		// Cancel the light texture as requested
		// NORSPEC-292
        //
        // Texture picker triggers cancel both in case of actual cancel and in case of
        // selection of "None" texture.
        LLUUID tex_id = LightTextureCtrl->getImageAssetID();
        bool is_spotlight = volobjp->isLightSpotlight();
        setLightTextureID(tex_id, LightTextureCtrl->getImageItemID(), volobjp); //updates spotlight

        if (!is_spotlight && tex_id.notNull())
        {
            LLVector3 spot_params = volobjp->getSpotLightParams();
            getChild<LLUICtrl>("Light FOV")->setValue(spot_params.mV[0]);
            getChild<LLUICtrl>("Light Focus")->setValue(spot_params.mV[1]);
            getChild<LLUICtrl>("Light Ambiance")->setValue(spot_params.mV[2]);
        }
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
		volobjp->setLightSRGBColor(clr3);
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
		setLightTextureID(id, LightTextureCtrl->getImageItemID(), volobjp);
	}
}

void LLPanelVolume::onCopyFeatures()
{
    LLViewerObject* objectp = mObject;
    if (!objectp)
    {
        return;
    }

    LLSD clipboard;

    LLVOVolume *volobjp = NULL;
    if (objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
    {
        volobjp = (LLVOVolume *)objectp;
    }

    // Flexi Prim
    if (volobjp && volobjp->isFlexible())
    {
        LLFlexibleObjectData *attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
        if (attributes)
        {
            clipboard["flex"]["lod"] = attributes->getSimulateLOD();
            clipboard["flex"]["gav"] = attributes->getGravity();
            clipboard["flex"]["ten"] = attributes->getTension();
            clipboard["flex"]["fri"] = attributes->getAirFriction();
            clipboard["flex"]["sen"] = attributes->getWindSensitivity();
            LLVector3 force = attributes->getUserForce();
            clipboard["flex"]["forx"] = force.mV[0];
            clipboard["flex"]["fory"] = force.mV[1];
            clipboard["flex"]["forz"] = force.mV[2];
        }
    }

    // Physics
    {
        clipboard["physics"]["shape"] = objectp->getPhysicsShapeType();
        clipboard["physics"]["gravity"] = objectp->getPhysicsGravity();
        clipboard["physics"]["friction"] = objectp->getPhysicsFriction();
        clipboard["physics"]["density"] = objectp->getPhysicsDensity();
        clipboard["physics"]["restitution"] = objectp->getPhysicsRestitution();

        U8 material_code = 0;
        struct f : public LLSelectedTEGetFunctor<U8>
        {
            U8 get(LLViewerObject* object, S32 te)
            {
                return object->getMaterial();
            }
        } func;
        bool material_same = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func, material_code);
        // This should always be true since material should be per object.
        if (material_same)
        {
            clipboard["physics"]["material"] = material_code;
        }
    }

    mClipboardParams["features"] = clipboard;
}

void LLPanelVolume::onPasteFeatures()
{
    LLViewerObject* objectp = mObject;
    if (!objectp && mClipboardParams.has("features"))
    {
        return;
    }

    LLSD &clipboard = mClipboardParams["features"];

    LLVOVolume *volobjp = NULL;
    if (objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
    {
        volobjp = (LLVOVolume *)objectp;
    }

    // Physics
    bool is_root = objectp->isRoot();

    // Not sure if phantom should go under physics, but doesn't fit elsewhere
    bool is_phantom = clipboard["is_phantom"].asBoolean() && is_root;
    LLSelectMgr::getInstance()->selectionUpdatePhantom(is_phantom);

    bool is_physical = clipboard["is_physical"].asBoolean() && is_root;
    LLSelectMgr::getInstance()->selectionUpdatePhysics(is_physical);

    if (clipboard.has("physics"))
    {
        objectp->setPhysicsShapeType((U8)clipboard["physics"]["shape"].asInteger());
        U8 cur_material = objectp->getMaterial();
        U8 material = (U8)clipboard["physics"]["material"].asInteger() | (cur_material & ~LL_MCODE_MASK);

        objectp->setMaterial(material);
        objectp->sendMaterialUpdate();
        objectp->setPhysicsGravity(clipboard["physics"]["gravity"].asReal());
        objectp->setPhysicsFriction(clipboard["physics"]["friction"].asReal());
        objectp->setPhysicsDensity(clipboard["physics"]["density"].asReal());
        objectp->setPhysicsRestitution(clipboard["physics"]["restitution"].asReal());
        objectp->updateFlags(true);
    }

    // Flexible
    bool is_flexible = clipboard.has("flex");
    if (is_flexible && volobjp->canBeFlexible())
    {
        LLVOVolume *volobjp = (LLVOVolume *)objectp;
        bool update_shape = false;

        // do before setParameterEntry or it will think that it is already flexi
        update_shape = volobjp->setIsFlexible(is_flexible);

        if (objectp->getClickAction() == CLICK_ACTION_SIT)
        {
            objectp->setClickAction(CLICK_ACTION_NONE);
        }

        LLFlexibleObjectData *attributes = (LLFlexibleObjectData *)objectp->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
        if (attributes)
        {
            LLFlexibleObjectData new_attributes;
            new_attributes = *attributes;
            new_attributes.setSimulateLOD(clipboard["flex"]["lod"].asInteger());
            new_attributes.setGravity(clipboard["flex"]["gav"].asReal());
            new_attributes.setTension(clipboard["flex"]["ten"].asReal());
            new_attributes.setAirFriction(clipboard["flex"]["fri"].asReal());
            new_attributes.setWindSensitivity(clipboard["flex"]["sen"].asReal());
            F32 fx = (F32)clipboard["flex"]["forx"].asReal();
            F32 fy = (F32)clipboard["flex"]["fory"].asReal();
            F32 fz = (F32)clipboard["flex"]["forz"].asReal();
            LLVector3 force(fx, fy, fz);
            new_attributes.setUserForce(force);
            objectp->setParameterEntry(LLNetworkData::PARAMS_FLEXIBLE, new_attributes, true);
        }

        if (update_shape)
        {
            mObject->sendShapeUpdate();
            LLSelectMgr::getInstance()->selectionUpdatePhantom(volobjp->flagPhantom());
        }
    }
    else
    {
        LLVOVolume *volobjp = (LLVOVolume *)objectp;
        if (volobjp->setIsFlexible(false))
        {
            mObject->sendShapeUpdate();
            LLSelectMgr::getInstance()->selectionUpdatePhantom(volobjp->flagPhantom());
        }
    }
}

void LLPanelVolume::onCopyLight()
{
    LLViewerObject* objectp = mObject;
    if (!objectp)
    {
        return;
    }

    LLSD clipboard;

    LLVOVolume *volobjp = NULL;
    if (objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
    {
        volobjp = (LLVOVolume *)objectp;
    }

    // Light Source
    if (volobjp && volobjp->getIsLight())
    {
        clipboard["light"]["intensity"] = volobjp->getLightIntensity();
        clipboard["light"]["radius"] = volobjp->getLightRadius();
        clipboard["light"]["falloff"] = volobjp->getLightFalloff();
        LLColor3 color = volobjp->getLightSRGBColor();
        clipboard["light"]["r"] = color.mV[0];
        clipboard["light"]["g"] = color.mV[1];
        clipboard["light"]["b"] = color.mV[2];

        // Spotlight
        if (volobjp->isLightSpotlight())
        {
            LLUUID id = volobjp->getLightTextureID();
            if (id.notNull() && get_can_copy_texture(id))
            {
                clipboard["spot"]["id"] = id;
                LLVector3 spot_params = volobjp->getSpotLightParams();
                clipboard["spot"]["fov"] = spot_params.mV[0];
                clipboard["spot"]["focus"] = spot_params.mV[1];
                clipboard["spot"]["ambiance"] = spot_params.mV[2];
            }
        }
    }

    if (volobjp && volobjp->isReflectionProbe())
    {
        clipboard["reflection_probe"]["is_box"] = volobjp->getReflectionProbeIsBox();
        clipboard["reflection_probe"]["ambiance"] = volobjp->getReflectionProbeAmbiance();
        clipboard["reflection_probe"]["near_clip"] = volobjp->getReflectionProbeNearClip();
        clipboard["reflection_probe"]["dynamic"] = volobjp->getReflectionProbeIsDynamic();
    }

    mClipboardParams["light"] = clipboard;
}

void LLPanelVolume::onPasteLight()
{
    LLViewerObject* objectp = mObject;
    if (!objectp && mClipboardParams.has("light"))
    {
        return;
    }

    LLSD &clipboard = mClipboardParams["light"];

    LLVOVolume *volobjp = NULL;
    if (objectp && (objectp->getPCode() == LL_PCODE_VOLUME))
    {
        volobjp = (LLVOVolume *)objectp;
    }

    // Light Source
    if (volobjp)
    {
        if (clipboard.has("light"))
        {
            volobjp->setIsLight(true);
            volobjp->setLightIntensity((F32)clipboard["light"]["intensity"].asReal());
            volobjp->setLightRadius((F32)clipboard["light"]["radius"].asReal());
            volobjp->setLightFalloff((F32)clipboard["light"]["falloff"].asReal());
            F32 r = (F32)clipboard["light"]["r"].asReal();
            F32 g = (F32)clipboard["light"]["g"].asReal();
            F32 b = (F32)clipboard["light"]["b"].asReal();
            volobjp->setLightSRGBColor(LLColor3(r, g, b));
        }
        else
        {
            volobjp->setIsLight(false);
        }

        if (clipboard.has("spot"))
        {
            volobjp->setLightTextureID(clipboard["spot"]["id"].asUUID());
            LLVector3 spot_params;
            spot_params.mV[0] = (F32)clipboard["spot"]["fov"].asReal();
            spot_params.mV[1] = (F32)clipboard["spot"]["focus"].asReal();
            spot_params.mV[2] = (F32)clipboard["spot"]["ambiance"].asReal();
            volobjp->setSpotLightParams(spot_params);
        }

        if (clipboard.has("reflection_probe"))
        {
            volobjp->setIsReflectionProbe(true);
            volobjp->setReflectionProbeIsBox(clipboard["reflection_probe"]["is_box"].asBoolean());
            volobjp->setReflectionProbeAmbiance((F32)clipboard["reflection_probe"]["ambiance"].asReal());
            volobjp->setReflectionProbeNearClip((F32)clipboard["reflection_probe"]["near_clip"].asReal());
            volobjp->setReflectionProbeIsDynamic(clipboard["reflection_probe"]["dynamic"].asBoolean());
        }
        else
        {
            if (objectp->flagPhantom())
            {
                LLViewerObject* root = objectp->getRootEdit();
                bool in_linkeset = root != objectp || objectp->numChildren() > 0;
                if (in_linkeset)
                {
                    // In linkset with a phantom flag
                    objectp->setFlags(FLAGS_PHANTOM, false);
                }
            }

            volobjp->setIsReflectionProbe(false);
        }
    }
}

void LLPanelVolume::menuDoToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste
    if (command == "features_paste")
    {
        onPasteFeatures();
    }
    else if (command == "light_paste")
    {
        onPasteLight();
    }
    // copy
    else if (command == "features_copy")
    {
        onCopyFeatures();
    }
    else if (command == "light_copy")
    {
        onCopyLight();
    }
}

bool LLPanelVolume::menuEnableItem(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste options
    if (command == "features_paste")
    {
        return mClipboardParams.has("features");
    }
    else if (command == "light_paste")
    {
        return mClipboardParams.has("light");
    }
    return false;
}

// static
void LLPanelVolume::onCommitMaterial( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*)userdata;
	LLComboBox* box = (LLComboBox*) ctrl;

	if (box)
	{
		// apply the currently selected material to the object
		const std::string& material_name = box->getSimple();
		std::string LEGACY_FULLBRIGHT_DESC = LLTrans::getString("Fullbright");
		if (material_name != LEGACY_FULLBRIGHT_DESC)
		{
			U8 material_code = LLMaterialTable::basic.getMCode(material_name);
			if (self)
			{
				LLViewerObject* objectp = self->mObject;
				if (objectp)
				{
					objectp->setPhysicsGravity(DEFAULT_GRAVITY_MULTIPLIER);
					objectp->setPhysicsFriction(LLMaterialTable::basic.getFriction(material_code));
					//currently density is always set to 1000 serverside regardless of chosen material,
					//actual material density should be used here, if this behavior change
					objectp->setPhysicsDensity(DEFAULT_DENSITY);
					objectp->setPhysicsRestitution(LLMaterialTable::basic.getRestitution(material_code));
				}
			}
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
		volobjp->setLightSRGBColor(LLColor3(clr));
	}

	LLTextureCtrl*	LightTextureCtrl = self->getChild<LLTextureCtrl>("light texture control");
	if(LightTextureCtrl)
	{
		LLUUID id = LightTextureCtrl->getImageAssetID();
        LLUUID item_id = LightTextureCtrl->getImageItemID();
		if (id.notNull())
		{
			if (!volobjp->isLightSpotlight())
			{ //this commit is making this a spot light, set UI to default params
                setLightTextureID(id, item_id, volobjp);
				LLVector3 spot_params = volobjp->getSpotLightParams();
				self->getChild<LLUICtrl>("Light FOV")->setValue(spot_params.mV[0]);
				self->getChild<LLUICtrl>("Light Focus")->setValue(spot_params.mV[1]);
				self->getChild<LLUICtrl>("Light Ambiance")->setValue(spot_params.mV[2]);
			}
			else
			{ //modifying existing params, this time volobjp won't change params on its own.
                if (volobjp->getLightTextureID() != id)
                {
                    setLightTextureID(id, item_id, volobjp);
                }

				LLVector3 spot_params;
				spot_params.mV[0] = (F32) self->getChild<LLUICtrl>("Light FOV")->getValue().asReal();
				spot_params.mV[1] = (F32) self->getChild<LLUICtrl>("Light Focus")->getValue().asReal();
				spot_params.mV[2] = (F32) self->getChild<LLUICtrl>("Light Ambiance")->getValue().asReal();
				volobjp->setSpotLightParams(spot_params);
			}
		}
		else if (volobjp->isLightSpotlight())
		{ //no longer a spot light
			setLightTextureID(id, item_id, volobjp);
			//self->getChildView("Light FOV")->setEnabled(false);
			//self->getChildView("Light Focus")->setEnabled(false);
			//self->getChildView("Light Ambiance")->setEnabled(false);
		}
	}

	
}

//static 
void LLPanelVolume::onCommitProbe(LLUICtrl* ctrl, void* userdata)
{
    LLPanelVolume* self = (LLPanelVolume*)userdata;
    LLViewerObject* objectp = self->mObject;
    if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
    {
        return;
    }
    LLVOVolume* volobjp = (LLVOVolume*)objectp;

    volobjp->setReflectionProbeAmbiance((F32)self->getChild<LLUICtrl>("Probe Ambiance")->getValue().asReal());
    volobjp->setReflectionProbeNearClip((F32)self->getChild<LLUICtrl>("Probe Near Clip")->getValue().asReal());
    volobjp->setReflectionProbeIsDynamic(self->getChild<LLUICtrl>("Probe Dynamic")->getValue().asBoolean());

    std::string shape_type = self->getChild<LLUICtrl>("Probe Volume Type")->getValue().asString();

    bool is_box = shape_type == "Box";

    if (volobjp->setReflectionProbeIsBox(is_box))
    {
        // make the volume match the probe
        auto* select_mgr = LLSelectMgr::getInstance();

        select_mgr->selectionUpdatePhantom(true);
        select_mgr->selectionSetGLTFMaterial(LLUUID::null);
        select_mgr->selectionSetAlphaOnly(0.f);

        U8 profile, path;

        if (!is_box)
        {
            profile = LL_PCODE_PROFILE_CIRCLE_HALF;
            path = LL_PCODE_PATH_CIRCLE;

            F32 scale = volobjp->getScale().mV[0];
            volobjp->setScale(LLVector3(scale, scale, scale), false);
            LLSelectMgr::getInstance()->sendMultipleUpdate(UPD_ROTATION | UPD_POSITION | UPD_SCALE);
        }
        else
        {
            profile = LL_PCODE_PROFILE_SQUARE;
            path = LL_PCODE_PATH_LINE;
        }
        
        LLVolumeParams params;
        params.getProfileParams().setCurveType(profile);
        params.getPathParams().setCurveType(path);
        objectp->updateVolume(params);
    }

}

// static
void LLPanelVolume::onCommitIsLight( LLUICtrl* ctrl, void* userdata )
{
	LLPanelVolume* self = (LLPanelVolume*) userdata;
	self->sendIsLight();
}

// static
void LLPanelVolume::setLightTextureID(const LLUUID &asset_id, const LLUUID &item_id, LLVOVolume* volobjp)
{
    if (volobjp)
    {
        LLViewerInventoryItem* item = gInventory.getItem(item_id);

        if (item && volobjp->isAttachment())
        {
            const LLPermissions& perm = item->getPermissions();
            bool unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? true : false;
            if (!unrestricted)
            {
                // Attachments are in world and in inventory simultaneously,
                // at the moment server doesn't support such a situation.
                return;
            }
        }

        if (item && !item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
        {
            LLToolDragAndDrop::handleDropMaterialProtections(volobjp, item, LLToolDragAndDrop::SOURCE_AGENT, LLUUID::null);
        }    
        volobjp->setLightTextureID(asset_id);
    }
}
//----------------------------------------------------------------------------

// static
void LLPanelVolume::onCommitIsReflectionProbe(LLUICtrl* ctrl, void* userdata)
{
    LLPanelVolume* self = (LLPanelVolume*)userdata;
    self->sendIsReflectionProbe();
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

void LLPanelVolume::onCommitAnimatedMeshCheckbox(LLUICtrl *, void*)
{
	LLViewerObject* objectp = mObject;
	if (!objectp || (objectp->getPCode() != LL_PCODE_VOLUME))
	{
		return;
    }
	LLVOVolume *volobjp = (LLVOVolume *)objectp;
	bool animated_mesh = getChild<LLUICtrl>("Animated Mesh Checkbox Ctrl")->getValue();
    U32 flags = volobjp->getExtendedMeshFlags();
    U32 new_flags = flags;
    if (animated_mesh)
    {
        new_flags |= LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG;
    }
    else
    {
        new_flags &= ~LLExtendedMeshParams::ANIMATED_MESH_ENABLED_FLAG;
    }
    if (new_flags != flags)
    {
        volobjp->setExtendedMeshFlags(new_flags);
    }

	//refresh any bakes
	if (volobjp)
	{
		volobjp->refreshBakeTexture();

		LLViewerObject::const_child_list_t& child_list = volobjp->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			iter != child_list.end(); ++iter)
		{
			LLViewerObject* objectp = *iter;
			if (objectp)
			{
				objectp->refreshBakeTexture();
			}
		}

		if (gAgentAvatarp)
		{
			gAgentAvatarp->updateMeshVisibility();
		}
	}
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
		getChild<LLUICtrl>("Flexible1D Checkbox Ctrl")->setValue(false);
	}
}
