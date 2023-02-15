/** 
 * @file llpanelface.cpp
 * @brief Panel in the tools floater for editing face textures, colors, etc.
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
#include "llpanelface.h"
 
// library includes
#include "llcalc.h"
#include "llerror.h"
#include "llrect.h"
#include "llstring.h"
#include "llfontgl.h"

// project includes
#include "llagent.h"
#include "llagentdata.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llgltfmateriallist.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h" // gInventory
#include "llinventorymodelbackgroundfetch.h"
#include "llfloatermediasettings.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llmaterialmgr.h"
#include "llmaterialeditor.h"
#include "llmediactrl.h"
#include "llmediaentry.h"
#include "llmenubutton.h"
#include "llnotificationsutil.h"
#include "llpanelcontents.h"
#include "llradiogroup.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "lltooldraganddrop.h"
#include "lltoolface.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llui.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "lluictrlfactory.h"
#include "llpluginclassmedia.h"
#include "llviewertexturelist.h"// Update sel manager as to which channel we're editing so it can reflect the correct overlay UI



#include "llagent.h"
#include "llfilesystem.h"
#include "llviewerassetupload.h"
#include "llviewermenufile.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "llinventorymodel.h"

using namespace std::literals;

LLPanelFace::Selection LLPanelFace::sMaterialOverrideSelection;

//
// Constant definitions for comboboxes
// Must match the commbobox definitions in panel_tools_texture.xml
//
const S32 MATMEDIA_MATERIAL = 0;	// Material
const S32 MATMEDIA_PBR = 1;			// PBR
const S32 MATMEDIA_MEDIA = 2;		// Media
const S32 MATTYPE_DIFFUSE = 0;		// Diffuse material texture
const S32 MATTYPE_NORMAL = 1;		// Normal map
const S32 MATTYPE_SPECULAR = 2;		// Specular map
const S32 ALPHAMODE_MASK = 2;		// Alpha masking mode
const S32 BUMPY_TEXTURE = 18;		// use supplied normal map
const S32 SHINY_TEXTURE = 4;		// use supplied specular map
const S32 PBRTYPE_RENDER_MATERIAL_ID = 0;  // Render Material ID
const S32 PBRTYPE_BASE_COLOR = 1;   // PBR Base Color
const S32 PBRTYPE_NORMAL = 2;       // PBR Normal
const S32 PBRTYPE_METALLIC_ROUGHNESS = 3; // PBR Metallic
const S32 PBRTYPE_EMISSIVE = 4;     // PBR Emissive

LLGLTFMaterial::TextureInfo texture_info_from_pbrtype(S32 pbr_type)
{
    switch (pbr_type)
    {
    case PBRTYPE_BASE_COLOR:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR;
        break;
    case PBRTYPE_NORMAL:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL;
        break;
    case PBRTYPE_METALLIC_ROUGHNESS:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS;
        break;
    case PBRTYPE_EMISSIVE:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE;
        break;
    default:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
        break;
    }
}

void LLPanelFace::updateSelectedGLTFMaterials(std::function<void(LLGLTFMaterial*)> func)
{
    struct LLSelectedTEGLTFMaterialFunctor : public LLSelectedTEFunctor
    {
        LLSelectedTEGLTFMaterialFunctor(std::function<void(LLGLTFMaterial*)> func) : mFunc(func) {}
        virtual ~LLSelectedTEGLTFMaterialFunctor() {};
        bool apply(LLViewerObject* object, S32 face) override
        {
            LLGLTFMaterial new_override;
            const LLTextureEntry* tep = object->getTE(face);
            if (tep->getGLTFMaterialOverride())
            {
                new_override = *tep->getGLTFMaterialOverride();
            }
            mFunc(&new_override);
            LLGLTFMaterialList::queueModify(object, face, &new_override);

            return true;
        }

		std::function<void(LLGLTFMaterial*)> mFunc;
    } select_func(func);

    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&select_func);
}

template<typename T>
void readSelectedGLTFMaterial(std::function<T(const LLGLTFMaterial*)> func, T& value, bool& identical, bool has_tolerance, T tolerance)
{
    struct LLSelectedTEGetGLTFMaterialFunctor : public LLSelectedTEGetFunctor<T>
    {
        LLSelectedTEGetGLTFMaterialFunctor(std::function<T(const LLGLTFMaterial*)> func) : mFunc(func) {}
        virtual ~LLSelectedTEGetGLTFMaterialFunctor() {};
        T get(LLViewerObject* object, S32 face) override
        {
            const LLTextureEntry* tep = object->getTE(face);
            const LLGLTFMaterial* render_material = tep->getGLTFRenderMaterial();
            
            return mFunc(render_material);
        }

        std::function<T(const LLGLTFMaterial*)> mFunc;
    } select_func(func);
    identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&select_func, value, has_tolerance, tolerance);
}

BOOST_STATIC_ASSERT(MATTYPE_DIFFUSE == LLRender::DIFFUSE_MAP && MATTYPE_NORMAL == LLRender::NORMAL_MAP && MATTYPE_SPECULAR == LLRender::SPECULAR_MAP);

//
// "Use texture" label for normal/specular type comboboxes
// Filled in at initialization from translated strings
//
std::string USE_TEXTURE;

LLRender::eTexIndex LLPanelFace::getTextureChannelToEdit()
{
    LLRender::eTexIndex channel_to_edit = LLRender::DIFFUSE_MAP;
    if (mComboMatMedia)
    {
        U32 matmedia_selection = mComboMatMedia->getCurrentIndex();
        if (matmedia_selection == MATMEDIA_MATERIAL)
        {
            LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
            channel_to_edit = (LLRender::eTexIndex)radio_mat_type->getSelectedIndex();
        }
        if (matmedia_selection == MATMEDIA_PBR)
        {
            LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_pbr_type");
            channel_to_edit = (LLRender::eTexIndex)radio_mat_type->getSelectedIndex();
        }
    }

	channel_to_edit = (channel_to_edit == LLRender::NORMAL_MAP)		? (getCurrentNormalMap().isNull()		? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	channel_to_edit = (channel_to_edit == LLRender::SPECULAR_MAP)	? (getCurrentSpecularMap().isNull()		? LLRender::DIFFUSE_MAP : channel_to_edit) : channel_to_edit;
	return channel_to_edit;
}

LLRender::eTexIndex LLPanelFace::getTextureDropChannel()
{
    if (mComboMatMedia && mComboMatMedia->getCurrentIndex() == MATMEDIA_MATERIAL)
    {
        LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
        return LLRender::eTexIndex(radio_mat_type->getSelectedIndex());
    }

    return LLRender::eTexIndex(MATTYPE_DIFFUSE);
}

// Things the UI provides...
//
LLUUID	LLPanelFace::getCurrentNormalMap()			{ return getChild<LLTextureCtrl>("bumpytexture control")->getImageAssetID();	}
LLUUID	LLPanelFace::getCurrentSpecularMap()		{ return getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();	}
U32		LLPanelFace::getCurrentShininess()			{ return getChild<LLComboBox>("combobox shininess")->getCurrentIndex();			}
U32		LLPanelFace::getCurrentBumpiness()			{ return getChild<LLComboBox>("combobox bumpiness")->getCurrentIndex();			}
U8			LLPanelFace::getCurrentDiffuseAlphaMode()	{ return (U8)getChild<LLComboBox>("combobox alphamode")->getCurrentIndex();	}
U8			LLPanelFace::getCurrentAlphaMaskCutoff()	{ return (U8)getChild<LLUICtrl>("maskcutoff")->getValue().asInteger();			}
U8			LLPanelFace::getCurrentEnvIntensity()		{ return (U8)getChild<LLUICtrl>("environment")->getValue().asInteger();			}
U8			LLPanelFace::getCurrentGlossiness()			{ return (U8)getChild<LLUICtrl>("glossiness")->getValue().asInteger();			}
F32		LLPanelFace::getCurrentBumpyRot()			{ return getChild<LLUICtrl>("bumpyRot")->getValue().asReal();						}
F32		LLPanelFace::getCurrentBumpyScaleU()		{ return getChild<LLUICtrl>("bumpyScaleU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyScaleV()		{ return getChild<LLUICtrl>("bumpyScaleV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyOffsetU()		{ return getChild<LLUICtrl>("bumpyOffsetU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentBumpyOffsetV()		{ return getChild<LLUICtrl>("bumpyOffsetV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyRot()			{ return getChild<LLUICtrl>("shinyRot")->getValue().asReal();						}
F32		LLPanelFace::getCurrentShinyScaleU()		{ return getChild<LLUICtrl>("shinyScaleU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyScaleV()		{ return getChild<LLUICtrl>("shinyScaleV")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyOffsetU()		{ return getChild<LLUICtrl>("shinyOffsetU")->getValue().asReal();					}
F32		LLPanelFace::getCurrentShinyOffsetV()		{ return getChild<LLUICtrl>("shinyOffsetV")->getValue().asReal();					}

//
// Methods
//

BOOL	LLPanelFace::postBuild()
{
	childSetCommitCallback("combobox shininess",&LLPanelFace::onCommitShiny,this);
	childSetCommitCallback("combobox bumpiness",&LLPanelFace::onCommitBump,this);
	childSetCommitCallback("combobox alphamode",&LLPanelFace::onCommitAlphaMode,this);
	childSetCommitCallback("TexScaleU",&LLPanelFace::onCommitTextureScaleX, this);
	childSetCommitCallback("TexScaleV",&LLPanelFace::onCommitTextureScaleY, this);
	childSetCommitCallback("TexRot",&LLPanelFace::onCommitTextureRot, this);
	childSetCommitCallback("rptctrl",&LLPanelFace::onCommitRepeatsPerMeter, this);
	childSetCommitCallback("checkbox planar align",&LLPanelFace::onCommitPlanarAlign, this);
	childSetCommitCallback("TexOffsetU",LLPanelFace::onCommitTextureOffsetX, this);
	childSetCommitCallback("TexOffsetV",LLPanelFace::onCommitTextureOffsetY, this);

	childSetCommitCallback("bumpyScaleU",&LLPanelFace::onCommitMaterialBumpyScaleX, this);
	childSetCommitCallback("bumpyScaleV",&LLPanelFace::onCommitMaterialBumpyScaleY, this);
	childSetCommitCallback("bumpyRot",&LLPanelFace::onCommitMaterialBumpyRot, this);
	childSetCommitCallback("bumpyOffsetU",&LLPanelFace::onCommitMaterialBumpyOffsetX, this);
	childSetCommitCallback("bumpyOffsetV",&LLPanelFace::onCommitMaterialBumpyOffsetY, this);
	childSetCommitCallback("shinyScaleU",&LLPanelFace::onCommitMaterialShinyScaleX, this);
	childSetCommitCallback("shinyScaleV",&LLPanelFace::onCommitMaterialShinyScaleY, this);
	childSetCommitCallback("shinyRot",&LLPanelFace::onCommitMaterialShinyRot, this);
	childSetCommitCallback("shinyOffsetU",&LLPanelFace::onCommitMaterialShinyOffsetX, this);
	childSetCommitCallback("shinyOffsetV",&LLPanelFace::onCommitMaterialShinyOffsetY, this);
	childSetCommitCallback("glossiness",&LLPanelFace::onCommitMaterialGloss, this);
	childSetCommitCallback("environment",&LLPanelFace::onCommitMaterialEnv, this);
	childSetCommitCallback("maskcutoff",&LLPanelFace::onCommitMaterialMaskCutoff, this);
    childSetCommitCallback("add_media", &LLPanelFace::onClickBtnAddMedia, this);
    childSetCommitCallback("delete_media", &LLPanelFace::onClickBtnDeleteMedia, this);

    getChild<LLUICtrl>("gltfTextureScaleU")->setCommitCallback(boost::bind(&LLPanelFace::onCommitGLTFTextureScaleU, this, _1), nullptr);
    getChild<LLUICtrl>("gltfTextureScaleV")->setCommitCallback(boost::bind(&LLPanelFace::onCommitGLTFTextureScaleV, this, _1), nullptr);
    getChild<LLUICtrl>("gltfTextureRotation")->setCommitCallback(boost::bind(&LLPanelFace::onCommitGLTFRotation, this, _1), nullptr);
    getChild<LLUICtrl>("gltfTextureOffsetU")->setCommitCallback(boost::bind(&LLPanelFace::onCommitGLTFTextureOffsetU, this, _1), nullptr);
    getChild<LLUICtrl>("gltfTextureOffsetV")->setCommitCallback(boost::bind(&LLPanelFace::onCommitGLTFTextureOffsetV, this, _1), nullptr);

    LLGLTFMaterialList::addSelectionUpdateCallback(&LLPanelFace::onMaterialOverrideReceived);
    sMaterialOverrideSelection.connect();

	childSetAction("button align",&LLPanelFace::onClickAutoFix,this);
	childSetAction("button align textures", &LLPanelFace::onAlignTexture, this);
    childSetAction("pbr_from_inventory", &LLPanelFace::onClickBtnLoadInvPBR, this);
    childSetAction("edit_selected_pbr", &LLPanelFace::onClickBtnEditPBR, this);
    childSetAction("save_selected_pbr", &LLPanelFace::onClickBtnSavePBR, this);

	LLTextureCtrl*	mTextureCtrl;
	LLTextureCtrl*	mShinyTextureCtrl;
	LLTextureCtrl*	mBumpyTextureCtrl;
	LLColorSwatchCtrl*	mColorSwatch;
	LLColorSwatchCtrl*	mShinyColorSwatch;

	LLComboBox*		mComboTexGen;

	LLCheckBoxCtrl	*mCheckFullbright;
	
	LLTextBox*		mLabelColorTransp;
	LLSpinCtrl*		mCtrlColorTransp;		// transparency = 1 - alpha

	LLSpinCtrl*     mCtrlGlow;

	setMouseOpaque(FALSE);

    LLTextureCtrl*	pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
    if (pbr_ctrl)
    {
        pbr_ctrl->setDefaultImageAssetID(LLUUID::null);
        pbr_ctrl->setBlankImageAssetID(LLGLTFMaterialList::BLANK_MATERIAL_ASSET_ID);
        pbr_ctrl->setCommitCallback(boost::bind(&LLPanelFace::onCommitPbr, this, _2));
        pbr_ctrl->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelPbr, this, _2));
        pbr_ctrl->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectPbr, this, _2));
        pbr_ctrl->setDragCallback(boost::bind(&LLPanelFace::onDragPbr, this, _2));
        pbr_ctrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onPbrSelectionChanged, this, _1));
        pbr_ctrl->setOnCloseCallback(boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2));

        pbr_ctrl->setFollowsTop();
        pbr_ctrl->setFollowsLeft();
        pbr_ctrl->setImmediateFilterPermMask(PERM_NONE);
        pbr_ctrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
        pbr_ctrl->setBakeTextureEnabled(false);
        pbr_ctrl->setInventoryPickType(LLTextureCtrl::PICK_MATERIAL);
    }

	mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(mTextureCtrl)
	{
		mTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectTexture" )));
		mTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitTexture, this, _2) );
		mTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelTexture, this, _2) );
		mTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectTexture, this, _2) );
		mTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );

		mTextureCtrl->setFollowsTop();
		mTextureCtrl->setFollowsLeft();
		mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mShinyTextureCtrl = getChild<LLTextureCtrl>("shinytexture control");
	if(mShinyTextureCtrl)
	{
		mShinyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectSpecularTexture" )));
		mShinyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectSpecularTexture, this, _2) );
		mShinyTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );
		
		mShinyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mShinyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mShinyTextureCtrl->setFollowsTop();
		mShinyTextureCtrl->setFollowsLeft();
		mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mShinyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mBumpyTextureCtrl = getChild<LLTextureCtrl>("bumpytexture control");
	if(mBumpyTextureCtrl)
	{
		mBumpyTextureCtrl->setDefaultImageAssetID(LLUUID( gSavedSettings.getString( "DefaultObjectNormalTexture" )));
		mBumpyTextureCtrl->setBlankImageAssetID(LLUUID( gSavedSettings.getString( "DefaultBlankNormalTexture" )));
		mBumpyTextureCtrl->setCommitCallback( boost::bind(&LLPanelFace::onCommitNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnCancelCallback( boost::bind(&LLPanelFace::onCancelNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnSelectCallback( boost::bind(&LLPanelFace::onSelectNormalTexture, this, _2) );
		mBumpyTextureCtrl->setOnCloseCallback( boost::bind(&LLPanelFace::onCloseTexturePicker, this, _2) );

		mBumpyTextureCtrl->setDragCallback(boost::bind(&LLPanelFace::onDragTexture, this, _2));
		mBumpyTextureCtrl->setOnTextureSelectedCallback(boost::bind(&LLPanelFace::onTextureSelectionChanged, this, _1));
		mBumpyTextureCtrl->setFollowsTop();
		mBumpyTextureCtrl->setFollowsLeft();
		mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
		mBumpyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
	}

	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(mColorSwatch)
	{
		mColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitColor, this, _2));
		mColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelColor, this, _2));
		mColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectColor, this, _2));
		mColorSwatch->setFollowsTop();
		mColorSwatch->setFollowsLeft();
		mColorSwatch->setCanApplyImmediately(TRUE);
	}

	mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
	if(mShinyColorSwatch)
	{
		mShinyColorSwatch->setCommitCallback(boost::bind(&LLPanelFace::onCommitShinyColor, this, _2));
		mShinyColorSwatch->setOnCancelCallback(boost::bind(&LLPanelFace::onCancelShinyColor, this, _2));
		mShinyColorSwatch->setOnSelectCallback(boost::bind(&LLPanelFace::onSelectShinyColor, this, _2));
		mShinyColorSwatch->setFollowsTop();
		mShinyColorSwatch->setFollowsLeft();
		mShinyColorSwatch->setCanApplyImmediately(TRUE);
	}

	mLabelColorTransp = getChild<LLTextBox>("color trans");
	if(mLabelColorTransp)
	{
		mLabelColorTransp->setFollowsTop();
		mLabelColorTransp->setFollowsLeft();
	}

	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(mCtrlColorTransp)
	{
		mCtrlColorTransp->setCommitCallback(boost::bind(&LLPanelFace::onCommitAlpha, this, _2));
		mCtrlColorTransp->setPrecision(0);
		mCtrlColorTransp->setFollowsTop();
		mCtrlColorTransp->setFollowsLeft();
	}

	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if (mCheckFullbright)
	{
		mCheckFullbright->setCommitCallback(LLPanelFace::onCommitFullbright, this);
	}

	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(mComboTexGen)
	{
		mComboTexGen->setCommitCallback(LLPanelFace::onCommitTexGen, this);
		mComboTexGen->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);	
	}

    mComboMatMedia = getChild<LLComboBox>("combobox matmedia");
	if(mComboMatMedia)
	{
        mComboMatMedia->setCommitCallback(LLPanelFace::onCommitMaterialsMedia,this);
        mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
	}

	LLRadioGroup* radio_mat_type = findChild<LLRadioGroup>("radio_material_type");
    if(radio_mat_type)
    {
        radio_mat_type->setCommitCallback(LLPanelFace::onCommitMaterialType, this);
        radio_mat_type->selectNthItem(MATTYPE_DIFFUSE);
    }

    LLRadioGroup* radio_pbr_type = findChild<LLRadioGroup>("radio_pbr_type");
    if (radio_pbr_type)
    {
        radio_pbr_type->setCommitCallback(LLPanelFace::onCommitPbrType, this);
        radio_pbr_type->selectNthItem(PBRTYPE_RENDER_MATERIAL_ID);
    }

	mCtrlGlow = getChild<LLSpinCtrl>("glow");
	if(mCtrlGlow)
	{
		mCtrlGlow->setCommitCallback(LLPanelFace::onCommitGlow, this);
	}

    mMenuClipboardColor = getChild<LLMenuButton>("clipboard_color_params_btn");
    mMenuClipboardTexture = getChild<LLMenuButton>("clipboard_texture_params_btn");
    
    mTitleMedia = getChild<LLMediaCtrl>("title_media");
    mTitleMediaText = getChild<LLTextBox>("media_info");

	clearCtrls();

	return TRUE;
}

LLPanelFace::LLPanelFace()
:	LLPanel(),
    mIsAlpha(false),
    mComboMatMedia(NULL),
    mTitleMedia(NULL),
    mTitleMediaText(NULL),
    mNeedMediaTitle(true)
{
    USE_TEXTURE = LLTrans::getString("use_texture");
    mCommitCallbackRegistrar.add("PanelFace.menuDoToSelected", boost::bind(&LLPanelFace::menuDoToSelected, this, _2));
    mEnableCallbackRegistrar.add("PanelFace.menuEnable", boost::bind(&LLPanelFace::menuEnableItem, this, _2));
}

LLPanelFace::~LLPanelFace()
{
    unloadMedia();
}

void LLPanelFace::draw()
{
    updateCopyTexButton();

    // grab media name/title and update the UI widget
    // Todo: move it, it's preferable not to update
    // labels inside draw
    updateMediaTitle();

    LLPanel::draw();

    if (sMaterialOverrideSelection.update())
    {
        setMaterialOverridesFromSelection();
    }
}

void LLPanelFace::sendTexture()
{
	LLTextureCtrl* mTextureCtrl = getChild<LLTextureCtrl>("texture control");
	if(!mTextureCtrl) return;
	if( !mTextureCtrl->getTentative() )
	{
		// we grab the item id first, because we want to do a
		// permissions check in the selection manager. ARGH!
		LLUUID id = mTextureCtrl->getImageItemID();
		if(id.isNull())
		{
			id = mTextureCtrl->getImageAssetID();
		}
		LLSelectMgr::getInstance()->selectionSetImage(id);
	}
}

void LLPanelFace::sendBump(U32 bumpiness)
{	
	LLTextureCtrl* bumpytexture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
	if (bumpiness < BUMPY_TEXTURE)
{	
		LL_DEBUGS("Materials") << "clearing bumptexture control" << LL_ENDL;	
		bumpytexture_ctrl->clear();
		bumpytexture_ctrl->setImageAssetID(LLUUID());		
	}

	updateBumpyControls(bumpiness == BUMPY_TEXTURE, true);

	LLUUID current_normal_map = bumpytexture_ctrl->getImageAssetID();

	U8 bump = (U8) bumpiness & TEM_BUMP_MASK;

	// Clear legacy bump to None when using an actual normal map
	//
	if (!current_normal_map.isNull())
		bump = 0;

	// Set the normal map or reset it to null as appropriate
	//
	LLSelectedTEMaterial::setNormalID(this, current_normal_map);

	LLSelectMgr::getInstance()->selectionSetBumpmap( bump, bumpytexture_ctrl->getImageItemID() );
}

void LLPanelFace::sendTexGen()
{
	LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
	if(!mComboTexGen)return;
	U8 tex_gen = (U8) mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
	LLSelectMgr::getInstance()->selectionSetTexGen( tex_gen );
}

void LLPanelFace::sendShiny(U32 shininess)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");

	if (shininess < SHINY_TEXTURE)
{
		texture_ctrl->clear();
		texture_ctrl->setImageAssetID(LLUUID());		
	}

	LLUUID specmap = getCurrentSpecularMap();

	U8 shiny = (U8) shininess & TEM_SHINY_MASK;
	if (!specmap.isNull())
		shiny = 0;

	LLSelectedTEMaterial::setSpecularID(this, specmap);

	LLSelectMgr::getInstance()->selectionSetShiny( shiny, texture_ctrl->getImageItemID() );

	updateShinyControls(!specmap.isNull(), true);
	
}

void LLPanelFace::sendFullbright()
{
	LLCheckBoxCtrl*	mCheckFullbright = getChild<LLCheckBoxCtrl>("checkbox fullbright");
	if(!mCheckFullbright)return;
	U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
	LLSelectMgr::getInstance()->selectionSetFullbright( fullbright );
}

void LLPanelFace::sendColor()
{
	
	LLColorSwatchCtrl*	mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
	if(!mColorSwatch)return;
	LLColor4 color = mColorSwatch->get();

	LLSelectMgr::getInstance()->selectionSetColorOnly( color );
}

void LLPanelFace::sendAlpha()
{	
	LLSpinCtrl*	mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
	if(!mCtrlColorTransp)return;
	F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;

	LLSelectMgr::getInstance()->selectionSetAlphaOnly( alpha );
}


void LLPanelFace::sendGlow()
{
	LLSpinCtrl* mCtrlGlow = getChild<LLSpinCtrl>("glow");
	llassert(mCtrlGlow);
	if (mCtrlGlow)
	{
		F32 glow = mCtrlGlow->get();
		LLSelectMgr::getInstance()->selectionSetGlow( glow );
	}
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		BOOL valid;
		F32 value;
        std::string prefix;

        // Effectively the same as MATMEDIA_PBR sans using different radio,
        // separate for the sake of clarity
        LLRadioGroup * radio_mat_type = mPanel->getChild<LLRadioGroup>("radio_material_type");
        switch (radio_mat_type->getSelectedIndex())
        {
        case MATTYPE_DIFFUSE:
            prefix = "Tex";
            break;
        case MATTYPE_NORMAL:
            prefix = "bumpy";
            break;
        case MATTYPE_SPECULAR:
            prefix = "shiny";
            break;
        }
        
        LLSpinCtrl * ctrlTexScaleS = mPanel->getChild<LLSpinCtrl>(prefix + "ScaleU");
        LLSpinCtrl * ctrlTexScaleT = mPanel->getChild<LLSpinCtrl>(prefix + "ScaleV");
        LLSpinCtrl * ctrlTexOffsetS = mPanel->getChild<LLSpinCtrl>(prefix + "OffsetU");
        LLSpinCtrl * ctrlTexOffsetT = mPanel->getChild<LLSpinCtrl>(prefix + "OffsetV");
        LLSpinCtrl * ctrlTexRotation = mPanel->getChild<LLSpinCtrl>(prefix + "Rot");

		LLComboBox*	comboTexGen = mPanel->getChild<LLComboBox>("combobox texgen");
		LLCheckBoxCtrl*	cb_planar_align = mPanel->getChild<LLCheckBoxCtrl>("checkbox planar align");
		bool align_planar = (cb_planar_align && cb_planar_align->get());

		llassert(comboTexGen);
		llassert(object);

		if (ctrlTexScaleS)
		{
			valid = !ctrlTexScaleS->getTentative(); // || !checkFlipScaleS->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexScaleS->get();
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleS( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexScaleT)
		{
			valid = !ctrlTexScaleT->getTentative(); // || !checkFlipScaleT->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexScaleT->get();
				//if( checkFlipScaleT->get() )
				//{
				//	value = -value;
				//}
				if (comboTexGen &&
				    comboTexGen->getCurrentIndex() == 1)
				{
					value *= 0.5f;
				}
				object->setTEScaleT( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexOffsetS)
		{
			valid = !ctrlTexOffsetS->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexOffsetS->get();
				object->setTEOffsetS( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexOffsetT)
		{
			valid = !ctrlTexOffsetT->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexOffsetT->get();
				object->setTEOffsetT( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, value, te, object->getID());
				}
			}
		}

		if (ctrlTexRotation)
		{
			valid = !ctrlTexRotation->getTentative();
			if (valid || align_planar)
			{
				value = ctrlTexRotation->get() * DEG_TO_RAD;
				object->setTERotation( te, value );

				if (align_planar) 
				{
					LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, value, te, object->getID());
					LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, value, te, object->getID());
				}
			}
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
};

// Functor that aligns a face to mCenterFace
struct LLPanelFaceSetAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceSetAlignedTEFunctor(LLPanelFace* panel, LLFace* center_face) :
		mPanel(panel),
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return true;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{
			return true;
		}

		bool set_aligned = true;
		if (facep == mCenterFace)
		{
			set_aligned = false;
		}
		if (set_aligned)
		{
			LLVector2 uv_offset, uv_scale;
			F32 uv_rot;
			set_aligned = facep->calcAlignedPlanarTE(mCenterFace, &uv_offset, &uv_scale, &uv_rot);
			if (set_aligned)
			{
				object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
				object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
				object->setTERotation(te, uv_rot);

				LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, uv_rot, te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, uv_rot, te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());

				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
				LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
			}
		}
		if (!set_aligned)
		{
			LLPanelFaceSetTEFunctor setfunc(mPanel);
			setfunc.apply(object, te);
		}
		return true;
	}
private:
	LLPanelFace* mPanel;
	LLFace* mCenterFace;
};

struct LLPanelFaceSetAlignedConcreteTEFunctor : public LLSelectedTEFunctor
{
    LLPanelFaceSetAlignedConcreteTEFunctor(LLPanelFace* panel, LLFace* center_face, LLRender::eTexIndex map) :
        mPanel(panel),
        mChefFace(center_face),
        mMap(map)
    {}

    virtual bool apply(LLViewerObject* object, S32 te)
    {
        LLFace* facep = object->mDrawable->getFace(te);
        if (!facep)
        {
            return true;
        }

        if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
        {
            return true;
        }

        if (mChefFace != facep)
        {
            LLVector2 uv_offset, uv_scale;
            F32 uv_rot;
            if (facep->calcAlignedPlanarTE(mChefFace, &uv_offset, &uv_scale, &uv_rot, mMap))
            {
                switch (mMap)
                {
                case LLRender::DIFFUSE_MAP:
                        object->setTEOffset(te, uv_offset.mV[VX], uv_offset.mV[VY]);
                        object->setTEScale(te, uv_scale.mV[VX], uv_scale.mV[VY]);
                        object->setTERotation(te, uv_rot);
                    break;
                case LLRender::NORMAL_MAP:
                        LLPanelFace::LLSelectedTEMaterial::setNormalRotation(mPanel, uv_rot, te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
                    break;
                case LLRender::SPECULAR_MAP:
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRotation(mPanel, uv_rot, te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, uv_offset.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, uv_offset.mV[VY], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatX(mPanel, uv_scale.mV[VX], te, object->getID());
                        LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, uv_scale.mV[VY], te, object->getID());
                    break;
                default: /*make compiler happy*/
                    break;
                }
            }
        }
        
        return true;
    }
private:
    LLPanelFace* mPanel;
    LLFace* mChefFace;
    LLRender::eTexIndex mMap;
};

// Functor that tests if a face is aligned to mCenterFace
struct LLPanelFaceGetIsAlignedTEFunctor : public LLSelectedTEFunctor
{
	LLPanelFaceGetIsAlignedTEFunctor(LLFace* center_face) :
		mCenterFace(center_face) {}

	virtual bool apply(LLViewerObject* object, S32 te)
	{
		LLFace* facep = object->mDrawable->getFace(te);
		if (!facep)
		{
			return false;
		}

		if (facep->getViewerObject()->getVolume()->getNumVolumeFaces() <= te)
		{ //volume face does not exist, can't be aligned
			return false;
		}

		if (facep == mCenterFace)
		{
			return true;
		}
		
		LLVector2 aligned_st_offset, aligned_st_scale;
		F32 aligned_st_rot;
		if ( facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot) )
		{
			const LLTextureEntry* tep = facep->getTextureEntry();
			LLVector2 st_offset, st_scale;
			tep->getOffset(&st_offset.mV[VX], &st_offset.mV[VY]);
			tep->getScale(&st_scale.mV[VX], &st_scale.mV[VY]);
			F32 st_rot = tep->getRotation();

            bool eq_offset_x = is_approx_equal_fraction(st_offset.mV[VX], aligned_st_offset.mV[VX], 12);
            bool eq_offset_y = is_approx_equal_fraction(st_offset.mV[VY], aligned_st_offset.mV[VY], 12);
            bool eq_scale_x  = is_approx_equal_fraction(st_scale.mV[VX], aligned_st_scale.mV[VX], 12);
            bool eq_scale_y  = is_approx_equal_fraction(st_scale.mV[VY], aligned_st_scale.mV[VY], 12);
            bool eq_rot      = is_approx_equal_fraction(st_rot, aligned_st_rot, 6);

			// needs a fuzzy comparison, because of fp errors
			if (eq_offset_x && 
				eq_offset_y && 
				eq_scale_x &&
				eq_scale_y &&
				eq_rot)
			{
				return true;
			}
		}
		return false;
	}
private:
	LLFace* mCenterFace;
};

struct LLPanelFaceSendFunctor : public LLSelectedObjectFunctor
{
	virtual bool apply(LLViewerObject* object)
	{
		object->sendTEUpdate();
		return true;
	}
};

void LLPanelFace::sendTextureInfo()
{
	if ((bool)childGetValue("checkbox planar align").asBoolean())
	{
		LLFace* last_face = NULL;
		bool identical_face =false;
		LLSelectedTE::getFace(last_face, identical_face);		
		LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}
	else
	{
		LLPanelFaceSetTEFunctor setfunc(this);
		LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
	}

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::alignTestureLayer()
{
    LLFace* last_face = NULL;
    bool identical_face = false;
    LLSelectedTE::getFace(last_face, identical_face);

    LLRadioGroup * radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
    LLPanelFaceSetAlignedConcreteTEFunctor setfunc(this, last_face, static_cast<LLRender::eTexIndex>(radio_mat_type->getSelectedIndex()));
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
}

void LLPanelFace::getState()
{
	updateUI();
}

void LLPanelFace::updateUI(bool force_set_values /*false*/)
{ //set state of UI to match state of texture entry(ies)  (calls setEnabled, setValue, etc, but NOT setVisible)
	LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();

	if( objectp
		&& objectp->getPCode() == LL_PCODE_VOLUME
		&& objectp->permModify())
	{
		BOOL editable = objectp->permModify() && !objectp->isPermanentEnforced();

        bool has_pbr_material;
        updateUIGLTF(objectp, has_pbr_material, force_set_values);

        const bool has_material = !has_pbr_material;

		// only turn on auto-adjust button if there is a media renderer and the media is loaded
        childSetEnabled("button align", editable);
		
        if (mComboMatMedia->getCurrentIndex() < MATMEDIA_MATERIAL)
        {
            // When selecting an object with a pbr and UI combo is not set,
            // set to pbr option, otherwise to a texture (material)
            if (has_pbr_material)
            {
                mComboMatMedia->selectNthItem(MATMEDIA_PBR);
            }
            else
            {
                mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
            }
        }

        mComboMatMedia->setEnabled(editable);

		LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
        if (radio_mat_type->getSelectedIndex() < MATTYPE_DIFFUSE)
        {
            radio_mat_type->selectNthItem(MATTYPE_DIFFUSE);
        }
        radio_mat_type->setEnabled(editable);

        LLRadioGroup* radio_pbr_type = getChild<LLRadioGroup>("radio_pbr_type");
        if (radio_pbr_type->getSelectedIndex() < PBRTYPE_RENDER_MATERIAL_ID)
        {
            radio_pbr_type->selectNthItem(PBRTYPE_RENDER_MATERIAL_ID);
        }
        radio_pbr_type->setEnabled(editable);
        const bool pbr_selected = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR;
        const bool texture_info_selected = pbr_selected && radio_pbr_type->getSelectedIndex() != PBRTYPE_RENDER_MATERIAL_ID;

		getChildView("checkbox_sync_settings")->setEnabled(editable);
		childSetValue("checkbox_sync_settings", gSavedSettings.getBOOL("SyncMaterialSettings"));

		updateVisibility();

        // *NOTE: The "identical" variable is currently only used to decide if
        // the texgen control should be tentative - this is not used by GLTF
        // materials. -Cosmic;2022-11-09
		bool identical				= true;	// true because it is anded below
        bool identical_diffuse	= false;
        bool identical_norm		= false;
        bool identical_spec		= false;

		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control");
		LLTextureCtrl*	shinytexture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
		LLTextureCtrl*	bumpytexture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		
		LLUUID id;
		LLUUID normmap_id;
		LLUUID specmap_id;
		
		// Color swatch
		{
			getChildView("color label")->setEnabled(editable);
		}
		LLColorSwatchCtrl*	color_swatch = findChild<LLColorSwatchCtrl>("colorswatch");

		LLColor4 color					= LLColor4::white;
		bool		identical_color	= false;

		if(color_swatch)
		{
			LLSelectedTE::getColor(color, identical_color);
			LLColor4 prev_color = color_swatch->get();

            color_swatch->setOriginal(color);
            color_swatch->set(color, force_set_values || (prev_color != color) || !editable);

            color_swatch->setValid(editable && !has_pbr_material);
            color_swatch->setEnabled( editable && !has_pbr_material);
            color_swatch->setCanApplyImmediately( editable && !has_pbr_material);
		}

		// Color transparency
		getChildView("color trans")->setEnabled(editable);

		F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
		getChild<LLUICtrl>("ColorTrans")->setValue(editable ? transparency : 0);
		getChildView("ColorTrans")->setEnabled(editable && has_material);

		// Specular map
		LLSelectedTEMaterial::getSpecularID(specmap_id, identical_spec);
		
		U8 shiny = 0;
		bool identical_shiny = false;

		// Shiny
		LLSelectedTE::getShiny(shiny, identical_shiny);
		identical = identical && identical_shiny;

		shiny = specmap_id.isNull() ? shiny : SHINY_TEXTURE;

		LLCtrlSelectionInterface* combobox_shininess = childGetSelectionInterface("combobox shininess");
		if (combobox_shininess)
				{
			combobox_shininess->selectNthItem((S32)shiny);
		}

		getChildView("label shininess")->setEnabled(editable);
		getChildView("combobox shininess")->setEnabled(editable);

		getChildView("label glossiness")->setEnabled(editable);			
		getChildView("glossiness")->setEnabled(editable);

		getChildView("label environment")->setEnabled(editable);
		getChildView("environment")->setEnabled(editable);
		getChildView("label shinycolor")->setEnabled(editable);
					
		getChild<LLUICtrl>("combobox shininess")->setTentative(!identical_spec);
		getChild<LLUICtrl>("glossiness")->setTentative(!identical_spec);
		getChild<LLUICtrl>("environment")->setTentative(!identical_spec);			
		getChild<LLUICtrl>("shinycolorswatch")->setTentative(!identical_spec);
					
		LLColorSwatchCtrl*	mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
		if(mShinyColorSwatch)
					{
			mShinyColorSwatch->setValid(editable);
			mShinyColorSwatch->setEnabled( editable );
			mShinyColorSwatch->setCanApplyImmediately( editable );
		}

		U8 bumpy = 0;
		// Bumpy
		{
			bool identical_bumpy = false;
			LLSelectedTE::getBumpmap(bumpy,identical_bumpy);

			LLUUID norm_map_id = getCurrentNormalMap();
			LLCtrlSelectionInterface* combobox_bumpiness = childGetSelectionInterface("combobox bumpiness");

			bumpy = norm_map_id.isNull() ? bumpy : BUMPY_TEXTURE;

			if (combobox_bumpiness)
							{
				combobox_bumpiness->selectNthItem((S32)bumpy);
							}
			else
							{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox bumpiness'" << LL_ENDL;
							}

			getChildView("combobox bumpiness")->setEnabled(editable);
			getChild<LLUICtrl>("combobox bumpiness")->setTentative(!identical_bumpy);
			getChildView("label bumpiness")->setEnabled(editable);
        }

		// Texture
		{
			LLSelectedTE::getTexId(id,identical_diffuse);

			// Normal map
			LLSelectedTEMaterial::getNormalID(normmap_id, identical_norm);

			mIsAlpha = FALSE;
			LLGLenum image_format = GL_RGB;
			bool identical_image_format = false;
			LLSelectedTE::getImageFormat(image_format, identical_image_format);
            
         mIsAlpha = FALSE;
         switch (image_format)
         {
               case GL_RGBA:
               case GL_ALPHA:
               {
                  mIsAlpha = TRUE;
               }
               break;

               case GL_RGB: break;
               default:
               {
                  LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
					}
               break;
				}

			if(LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				getChildView("button align")->setEnabled(editable);
			}
			
			// Diffuse Alpha Mode

			// Init to the default that is appropriate for the alpha content of the asset
			//
			U8 alpha_mode = mIsAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			bool identical_alpha_mode = false;

			// See if that's been overridden by a material setting for same...
			//
			LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(alpha_mode, identical_alpha_mode, mIsAlpha);

			LLCtrlSelectionInterface* combobox_alphamode = childGetSelectionInterface("combobox alphamode");
			if (combobox_alphamode)
			{
				//it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
				// Want masking? Want emissive? Tough! You get BLEND!
				alpha_mode = (transparency > 0.f) ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : alpha_mode;

				// ... unless there is no alpha channel in the texture, in which case alpha mode MUST be none
				alpha_mode = mIsAlpha ? alpha_mode : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

				combobox_alphamode->selectNthItem(alpha_mode);
			}
			else
			{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox alphamode'" << LL_ENDL;
			}

			updateAlphaControls();

			if (texture_ctrl)
			{
				if (identical_diffuse)
				{
					texture_ctrl->setTentative(FALSE);
					texture_ctrl->setEnabled(editable && !has_pbr_material);
					texture_ctrl->setImageAssetID(id);
					getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha && transparency <= 0.f && !has_pbr_material);
					getChildView("label alphamode")->setEnabled(editable && mIsAlpha && !has_pbr_material);
					getChildView("maskcutoff")->setEnabled(editable && mIsAlpha && !has_pbr_material);
					getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha && !has_pbr_material);

					texture_ctrl->setBakeTextureEnabled(TRUE);
				}
				else if (id.isNull())
				{
						// None selected
					texture_ctrl->setTentative(FALSE);
					texture_ctrl->setEnabled(FALSE);
					texture_ctrl->setImageAssetID(LLUUID::null);
					getChildView("combobox alphamode")->setEnabled(FALSE);
					getChildView("label alphamode")->setEnabled(FALSE);
					getChildView("maskcutoff")->setEnabled(FALSE);
					getChildView("label maskcutoff")->setEnabled(FALSE);

					texture_ctrl->setBakeTextureEnabled(false);
				}
				else
				{
						// Tentative: multiple selected with different textures
					texture_ctrl->setTentative(TRUE);
					texture_ctrl->setEnabled(editable && !has_pbr_material);
					texture_ctrl->setImageAssetID(id);
					getChildView("combobox alphamode")->setEnabled(editable && mIsAlpha && transparency <= 0.f && !has_pbr_material);
					getChildView("label alphamode")->setEnabled(editable && mIsAlpha && !has_pbr_material);
					getChildView("maskcutoff")->setEnabled(editable && mIsAlpha && !has_pbr_material);
					getChildView("label maskcutoff")->setEnabled(editable && mIsAlpha && !has_pbr_material);
					
					texture_ctrl->setBakeTextureEnabled(TRUE);
				}
				
			}

			if (shinytexture_ctrl)
			{
				shinytexture_ctrl->setTentative( !identical_spec );
				shinytexture_ctrl->setEnabled( editable && !has_pbr_material);
				shinytexture_ctrl->setImageAssetID( specmap_id );
			}

			if (bumpytexture_ctrl)
			{
				bumpytexture_ctrl->setTentative( !identical_norm );
				bumpytexture_ctrl->setEnabled( editable && !has_pbr_material);
				bumpytexture_ctrl->setImageAssetID( normmap_id );
			}
		}

		// planar align
		bool align_planar = false;
		bool identical_planar_aligned = false;
		{
			LLCheckBoxCtrl*	cb_planar_align = getChild<LLCheckBoxCtrl>("checkbox planar align");
			align_planar = (cb_planar_align && cb_planar_align->get());

			bool enabled = (editable && isIdenticalPlanarTexgen() && (!pbr_selected || texture_info_selected));
			childSetValue("checkbox planar align", align_planar && enabled);
            childSetVisible("checkbox planar align", enabled);
			childSetEnabled("checkbox planar align", enabled);
			childSetEnabled("button align textures", enabled && LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1);

			if (align_planar && enabled)
			{
				LLFace* last_face = NULL;
				bool identical_face = false;
				LLSelectedTE::getFace(last_face, identical_face);

				LLPanelFaceGetIsAlignedTEFunctor get_is_aligend_func(last_face);
				// this will determine if the texture param controls are tentative:
				identical_planar_aligned = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&get_is_aligend_func);
			}
		}
		
		// Needs to be public and before tex scale settings below to properly reflect
		// behavior when in planar vs default texgen modes in the
		// NORSPEC-84 et al
		//
		LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
		bool identical_texgen = true;		
		bool identical_planar_texgen = false;

		{	
			LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
			identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
		}

		// Texture scale
		{
			bool identical_diff_scale_s = false;
			bool identical_spec_scale_s = false;
			bool identical_norm_scale_s = false;

			identical = align_planar ? identical_planar_aligned : identical;

			F32 diff_scale_s = 1.f;			
			F32 spec_scale_s = 1.f;
			F32 norm_scale_s = 1.f;

			LLSelectedTE::getScaleS(diff_scale_s, identical_diff_scale_s);			
			LLSelectedTEMaterial::getSpecularRepeatX(spec_scale_s, identical_spec_scale_s);
			LLSelectedTEMaterial::getNormalRepeatX(norm_scale_s, identical_norm_scale_s);

			diff_scale_s = editable ? diff_scale_s : 1.0f;
			diff_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;
			
			norm_scale_s = editable ? norm_scale_s : 1.0f;
			norm_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_s = editable ? spec_scale_s : 1.0f;
			spec_scale_s *= identical_planar_texgen ? 2.0f : 1.0f;

			getChild<LLUICtrl>("TexScaleU")->setValue(diff_scale_s);
			getChild<LLUICtrl>("shinyScaleU")->setValue(spec_scale_s);
			getChild<LLUICtrl>("bumpyScaleU")->setValue(norm_scale_s);

            getChildView("TexScaleU")->setEnabled(editable && has_material);
            getChildView("shinyScaleU")->setEnabled(editable && has_material && specmap_id.notNull());
            getChildView("bumpyScaleU")->setEnabled(editable && has_material && normmap_id.notNull());

			BOOL diff_scale_tentative = !(identical && identical_diff_scale_s);
			BOOL norm_scale_tentative = !(identical && identical_norm_scale_s);
			BOOL spec_scale_tentative = !(identical && identical_spec_scale_s);

			getChild<LLUICtrl>("TexScaleU")->setTentative(  LLSD(diff_scale_tentative));			
			getChild<LLUICtrl>("shinyScaleU")->setTentative(LLSD(spec_scale_tentative));			
			getChild<LLUICtrl>("bumpyScaleU")->setTentative(LLSD(norm_scale_tentative));
		}

		{
			bool identical_diff_scale_t = false;
			bool identical_spec_scale_t = false;
			bool identical_norm_scale_t = false;

			F32 diff_scale_t = 1.f;			
			F32 spec_scale_t = 1.f;
			F32 norm_scale_t = 1.f;

			LLSelectedTE::getScaleT(diff_scale_t, identical_diff_scale_t);
			LLSelectedTEMaterial::getSpecularRepeatY(spec_scale_t, identical_spec_scale_t);
			LLSelectedTEMaterial::getNormalRepeatY(norm_scale_t, identical_norm_scale_t);

			diff_scale_t = editable ? diff_scale_t : 1.0f;
			diff_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			norm_scale_t = editable ? norm_scale_t : 1.0f;
			norm_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			spec_scale_t = editable ? spec_scale_t : 1.0f;
			spec_scale_t *= identical_planar_texgen ? 2.0f : 1.0f;

			BOOL diff_scale_tentative = !identical_diff_scale_t;
			BOOL norm_scale_tentative = !identical_norm_scale_t;
			BOOL spec_scale_tentative = !identical_spec_scale_t;

            getChildView("TexScaleV")->setEnabled(editable && has_material);
            getChildView("shinyScaleV")->setEnabled(editable && has_material && specmap_id.notNull());
            getChildView("bumpyScaleV")->setEnabled(editable && has_material && normmap_id.notNull());

			if (force_set_values)
			{
				getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(diff_scale_t);
			}
			else
			{
				getChild<LLSpinCtrl>("TexScaleV")->setValue(diff_scale_t);
			}
			getChild<LLUICtrl>("shinyScaleV")->setValue(norm_scale_t);
			getChild<LLUICtrl>("bumpyScaleV")->setValue(spec_scale_t);

			getChild<LLUICtrl>("TexScaleV")->setTentative(LLSD(diff_scale_tentative));
			getChild<LLUICtrl>("shinyScaleV")->setTentative(LLSD(norm_scale_tentative));
			getChild<LLUICtrl>("bumpyScaleV")->setTentative(LLSD(spec_scale_tentative));
		}

		// Texture offset
		{
			bool identical_diff_offset_s = false;
			bool identical_norm_offset_s = false;
			bool identical_spec_offset_s = false;

			F32 diff_offset_s = 0.0f;
			F32 norm_offset_s = 0.0f;
			F32 spec_offset_s = 0.0f;

			LLSelectedTE::getOffsetS(diff_offset_s, identical_diff_offset_s);
			LLSelectedTEMaterial::getNormalOffsetX(norm_offset_s, identical_norm_offset_s);
			LLSelectedTEMaterial::getSpecularOffsetX(spec_offset_s, identical_spec_offset_s);

			BOOL diff_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_s);
			BOOL norm_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_s);
			BOOL spec_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_s);

			getChild<LLUICtrl>("TexOffsetU")->setValue(  editable ? diff_offset_s : 0.0f);
			getChild<LLUICtrl>("bumpyOffsetU")->setValue(editable ? norm_offset_s : 0.0f);
			getChild<LLUICtrl>("shinyOffsetU")->setValue(editable ? spec_offset_s : 0.0f);

			getChild<LLUICtrl>("TexOffsetU")->setTentative(LLSD(diff_offset_u_tentative));
			getChild<LLUICtrl>("shinyOffsetU")->setTentative(LLSD(norm_offset_u_tentative));
			getChild<LLUICtrl>("bumpyOffsetU")->setTentative(LLSD(spec_offset_u_tentative));

            getChildView("TexOffsetU")->setEnabled(editable && has_material);
            getChildView("shinyOffsetU")->setEnabled(editable && has_material && specmap_id.notNull());
            getChildView("bumpyOffsetU")->setEnabled(editable && has_material && normmap_id.notNull());
		}

		{
			bool identical_diff_offset_t = false;
			bool identical_norm_offset_t = false;
			bool identical_spec_offset_t = false;

			F32 diff_offset_t = 0.0f;
			F32 norm_offset_t = 0.0f;
			F32 spec_offset_t = 0.0f;

			LLSelectedTE::getOffsetT(diff_offset_t, identical_diff_offset_t);
			LLSelectedTEMaterial::getNormalOffsetY(norm_offset_t, identical_norm_offset_t);
			LLSelectedTEMaterial::getSpecularOffsetY(spec_offset_t, identical_spec_offset_t);
			
			BOOL diff_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_t);
			BOOL norm_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_t);
			BOOL spec_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_t);

			getChild<LLUICtrl>("TexOffsetV")->setValue(  editable ? diff_offset_t : 0.0f);
			getChild<LLUICtrl>("bumpyOffsetV")->setValue(editable ? norm_offset_t : 0.0f);
			getChild<LLUICtrl>("shinyOffsetV")->setValue(editable ? spec_offset_t : 0.0f);

			getChild<LLUICtrl>("TexOffsetV")->setTentative(LLSD(diff_offset_v_tentative));
			getChild<LLUICtrl>("shinyOffsetV")->setTentative(LLSD(norm_offset_v_tentative));
			getChild<LLUICtrl>("bumpyOffsetV")->setTentative(LLSD(spec_offset_v_tentative));

            getChildView("TexOffsetV")->setEnabled(editable && has_material);
            getChildView("shinyOffsetV")->setEnabled(editable && has_material && specmap_id.notNull());
            getChildView("bumpyOffsetV")->setEnabled(editable && has_material && normmap_id.notNull());
		}

		// Texture rotation
		{
			bool identical_diff_rotation = false;
			bool identical_norm_rotation = false;
			bool identical_spec_rotation = false;

			F32 diff_rotation = 0.f;
			F32 norm_rotation = 0.f;
			F32 spec_rotation = 0.f;

			LLSelectedTE::getRotation(diff_rotation,identical_diff_rotation);
			LLSelectedTEMaterial::getSpecularRotation(spec_rotation,identical_spec_rotation);
			LLSelectedTEMaterial::getNormalRotation(norm_rotation,identical_norm_rotation);

			BOOL diff_rot_tentative = !(align_planar ? identical_planar_aligned : identical_diff_rotation);
			BOOL norm_rot_tentative = !(align_planar ? identical_planar_aligned : identical_norm_rotation);
			BOOL spec_rot_tentative = !(align_planar ? identical_planar_aligned : identical_spec_rotation);

			F32 diff_rot_deg = diff_rotation * RAD_TO_DEG;
			F32 norm_rot_deg = norm_rotation * RAD_TO_DEG;
			F32 spec_rot_deg = spec_rotation * RAD_TO_DEG;

            getChildView("TexRot")->setEnabled(editable && has_material);
            getChildView("shinyRot")->setEnabled(editable && has_material && specmap_id.notNull());
            getChildView("bumpyRot")->setEnabled(editable && has_material && normmap_id.notNull());

			getChild<LLUICtrl>("TexRot")->setTentative(diff_rot_tentative);
			getChild<LLUICtrl>("shinyRot")->setTentative(LLSD(norm_rot_tentative));
			getChild<LLUICtrl>("bumpyRot")->setTentative(LLSD(spec_rot_tentative));

			getChild<LLUICtrl>("TexRot")->setValue(  editable ? diff_rot_deg : 0.0f);			
			getChild<LLUICtrl>("shinyRot")->setValue(editable ? spec_rot_deg : 0.0f);
			getChild<LLUICtrl>("bumpyRot")->setValue(editable ? norm_rot_deg : 0.0f);
		}

		{
			F32 glow = 0.f;
			bool identical_glow = false;
			LLSelectedTE::getGlow(glow,identical_glow);
			getChild<LLUICtrl>("glow")->setValue(glow);
			getChild<LLUICtrl>("glow")->setTentative(!identical_glow);
			getChildView("glow")->setEnabled(editable);
			getChildView("glow label")->setEnabled(editable);
		}

		{
			LLCtrlSelectionInterface* combobox_texgen = childGetSelectionInterface("combobox texgen");
			if (combobox_texgen)
			{
				// Maps from enum to combobox entry index
				combobox_texgen->selectNthItem(((S32)selected_texgen) >> 1);
			}
			else
				{
				LL_WARNS() << "failed childGetSelectionInterface for 'combobox texgen'" << LL_ENDL;
				}

			getChildView("combobox texgen")->setEnabled(editable);
			getChild<LLUICtrl>("combobox texgen")->setTentative(!identical);
			getChildView("tex gen")->setEnabled(editable);

			}

		{
			U8 fullbright_flag = 0;
			bool identical_fullbright = false;
			
			LLSelectedTE::getFullbright(fullbright_flag,identical_fullbright);

			getChild<LLUICtrl>("checkbox fullbright")->setValue((S32)(fullbright_flag != 0));
			getChildView("checkbox fullbright")->setEnabled(editable && !has_pbr_material);
			getChild<LLUICtrl>("checkbox fullbright")->setTentative(!identical_fullbright);
		}
		
		// Repeats per meter
		{
			F32 repeats_diff = 1.f;
			F32 repeats_norm = 1.f;
			F32 repeats_spec = 1.f;

			bool identical_diff_repeats = false;
			bool identical_norm_repeats = false;
			bool identical_spec_repeats = false;

			LLSelectedTE::getMaxDiffuseRepeats(repeats_diff, identical_diff_repeats);
			LLSelectedTEMaterial::getMaxNormalRepeats(repeats_norm, identical_norm_repeats);
			LLSelectedTEMaterial::getMaxSpecularRepeats(repeats_spec, identical_spec_repeats);			

			LLComboBox*	mComboTexGen = getChild<LLComboBox>("combobox texgen");
			if (mComboTexGen)
            {
				S32 index = mComboTexGen ? mComboTexGen->getCurrentIndex() : 0;
                bool enabled = editable && (index != 1);
                bool identical_repeats = true;
                S32 material_selection = mComboMatMedia->getCurrentIndex();
				F32  repeats = 1.0f;

                U32 material_type = MATTYPE_DIFFUSE;
                if (material_selection == MATMEDIA_MATERIAL)
                {
                    material_type = radio_mat_type->getSelectedIndex();
                }
                else if (material_selection == MATMEDIA_PBR)
                {
                    enabled = editable && has_pbr_material;
                    material_type = radio_pbr_type->getSelectedIndex();
                }

                switch (material_type)
                {
                default:
                case MATTYPE_DIFFUSE:
                {
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = editable && !id.isNull();
                    }
                    identical_repeats = identical_diff_repeats;
                    repeats = repeats_diff;
                }
                break;

                case MATTYPE_SPECULAR:
                {
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = (editable && ((shiny == SHINY_TEXTURE) && !specmap_id.isNull()));
                    }
                    identical_repeats = identical_spec_repeats;
                    repeats = repeats_spec;
                }
                break;

                case MATTYPE_NORMAL:
                {
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = (editable && ((bumpy == BUMPY_TEXTURE) && !normmap_id.isNull()));
                    }
                    identical_repeats = identical_norm_repeats;
                    repeats = repeats_norm;
                }
                break;
                }

				BOOL repeats_tentative = !identical_repeats;

				LLSpinCtrl* rpt_ctrl = getChild<LLSpinCtrl>("rptctrl");
				if (force_set_values)
				{
					//onCommit, previosly edited element updates related ones
					rpt_ctrl->forceSetValue(editable ? repeats : 1.0f);
				}
				else
				{
					rpt_ctrl->setValue(editable ? repeats : 1.0f);
				}
				rpt_ctrl->setTentative(LLSD(repeats_tentative));
                rpt_ctrl->setEnabled(has_material && !identical_planar_texgen && enabled);
			}
		}

		// Materials
		{
			LLMaterialPtr material;
			LLSelectedTEMaterial::getCurrent(material, identical);

            if (material && editable)
			{
				LL_DEBUGS("Materials") << material->asLLSD() << LL_ENDL;

				// Alpha
				LLCtrlSelectionInterface* combobox_alphamode =
					childGetSelectionInterface("combobox alphamode");
				if (combobox_alphamode)
				{
					U32 alpha_mode = material->getDiffuseAlphaMode();

					if (transparency > 0.f)
					{ //it is invalid to have any alpha mode other than blend if transparency is greater than zero ... 
						alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
					}

					if (!mIsAlpha)
					{ // ... unless there is no alpha channel in the texture, in which case alpha mode MUST ebe none
						alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
				}

					combobox_alphamode->selectNthItem(alpha_mode);
			}
			else
			{
					LL_WARNS() << "failed childGetSelectionInterface for 'combobox alphamode'" << LL_ENDL;
			}
				getChild<LLUICtrl>("maskcutoff")->setValue(material->getAlphaMaskCutoff());
				updateAlphaControls();

				identical_planar_texgen = isIdenticalPlanarTexgen();

				// Shiny (specular)
				F32 offset_x, offset_y, repeat_x, repeat_y, rot;
				LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
				texture_ctrl->setImageAssetID(material->getSpecularID());

				if (!material->getSpecularID().isNull() && (shiny == SHINY_TEXTURE))
			{
					material->getSpecularOffset(offset_x,offset_y);
					material->getSpecularRepeat(repeat_x,repeat_y);

					if (identical_planar_texgen)
			{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
			}

					rot = material->getSpecularRotation();
					getChild<LLUICtrl>("shinyScaleU")->setValue(repeat_x);
					getChild<LLUICtrl>("shinyScaleV")->setValue(repeat_y);
					getChild<LLUICtrl>("shinyRot")->setValue(rot*RAD_TO_DEG);
					getChild<LLUICtrl>("shinyOffsetU")->setValue(offset_x);
					getChild<LLUICtrl>("shinyOffsetV")->setValue(offset_y);
					getChild<LLUICtrl>("glossiness")->setValue(material->getSpecularLightExponent());
					getChild<LLUICtrl>("environment")->setValue(material->getEnvironmentIntensity());

					updateShinyControls(!material->getSpecularID().isNull(), true);
		}

				// Assert desired colorswatch color to match material AFTER updateShinyControls
				// to avoid getting overwritten with the default on some UI state changes.
				//
				if (!material->getSpecularID().isNull())
				{
					LLColorSwatchCtrl*	shiny_swatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
					LLColor4 new_color = material->getSpecularLightColor();
					LLColor4 old_color = shiny_swatch->get();

					shiny_swatch->setOriginal(new_color);
					shiny_swatch->set(new_color, force_set_values || old_color != new_color || !editable);
				}

				// Bumpy (normal)
				texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
				texture_ctrl->setImageAssetID(material->getNormalID());

				if (!material->getNormalID().isNull())
				{
					material->getNormalOffset(offset_x,offset_y);
					material->getNormalRepeat(repeat_x,repeat_y);

					if (identical_planar_texgen)
					{
						repeat_x *= 2.0f;
						repeat_y *= 2.0f;
					}
			
					rot = material->getNormalRotation();
					getChild<LLUICtrl>("bumpyScaleU")->setValue(repeat_x);
					getChild<LLUICtrl>("bumpyScaleV")->setValue(repeat_y);
					getChild<LLUICtrl>("bumpyRot")->setValue(rot*RAD_TO_DEG);
					getChild<LLUICtrl>("bumpyOffsetU")->setValue(offset_x);
					getChild<LLUICtrl>("bumpyOffsetV")->setValue(offset_y);

					updateBumpyControls(!material->getNormalID().isNull(), true);
				}
			}
		}
        S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
        BOOL single_volume = (selected_count == 1);
        mMenuClipboardColor->setEnabled(editable && single_volume);

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->setVar(LLCalc::TEX_U_SCALE, childGetValue("TexScaleU").asReal());
		calcp->setVar(LLCalc::TEX_V_SCALE, childGetValue("TexScaleV").asReal());
		calcp->setVar(LLCalc::TEX_U_OFFSET, childGetValue("TexOffsetU").asReal());
		calcp->setVar(LLCalc::TEX_V_OFFSET, childGetValue("TexOffsetV").asReal());
		calcp->setVar(LLCalc::TEX_ROTATION, childGetValue("TexRot").asReal());
		calcp->setVar(LLCalc::TEX_TRANSPARENCY, childGetValue("ColorTrans").asReal());
		calcp->setVar(LLCalc::TEX_GLOW, childGetValue("glow").asReal());
	}
	else
	{
		// Disable all UICtrls
		clearCtrls();

		// Disable non-UICtrls
        LLTextureCtrl*	pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
        if (pbr_ctrl)
        {
            pbr_ctrl->setImageAssetID(LLUUID::null);
            pbr_ctrl->setEnabled(FALSE);
        }
		LLTextureCtrl*	texture_ctrl = getChild<LLTextureCtrl>("texture control"); 
		if(texture_ctrl)
		{
			texture_ctrl->setImageAssetID( LLUUID::null );
			texture_ctrl->setEnabled( FALSE );  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
// 			texture_ctrl->setValid(FALSE);
		}
		LLColorSwatchCtrl* mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
		if(mColorSwatch)
		{
			mColorSwatch->setEnabled( FALSE );			
			mColorSwatch->setFallbackImage(LLUI::getUIImage("locked_image.j2c") );
			mColorSwatch->setValid(FALSE);
		}
		LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
		if (radio_mat_type)
		{
			radio_mat_type->setSelectedIndex(0);
		}
		getChildView("color trans")->setEnabled(FALSE);
		getChildView("rptctrl")->setEnabled(FALSE);
		getChildView("tex gen")->setEnabled(FALSE);
		getChildView("label shininess")->setEnabled(FALSE);
		getChildView("label bumpiness")->setEnabled(FALSE);
		getChildView("button align")->setEnabled(FALSE);
        getChildView("pbr_from_inventory")->setEnabled(FALSE);
        getChildView("edit_selected_pbr")->setEnabled(FALSE);
        getChildView("save_selected_pbr")->setEnabled(FALSE);
		
		updateVisibility();

		// Set variable values for numeric expressions
		LLCalc* calcp = LLCalc::getInstance();
		calcp->clearVar(LLCalc::TEX_U_SCALE);
		calcp->clearVar(LLCalc::TEX_V_SCALE);
		calcp->clearVar(LLCalc::TEX_U_OFFSET);
		calcp->clearVar(LLCalc::TEX_V_OFFSET);
		calcp->clearVar(LLCalc::TEX_ROTATION);
		calcp->clearVar(LLCalc::TEX_TRANSPARENCY);
		calcp->clearVar(LLCalc::TEX_GLOW);		
	}
}

void LLPanelFace::updateUIGLTF(LLViewerObject* objectp, bool& has_pbr_material, bool force_set_values)
{
    has_pbr_material = false;

    const bool editable = objectp->permModify() && !objectp->isPermanentEnforced();
    bool has_pbr_capabilities = LLMaterialEditor::capabilitiesAvailable();

    // pbr material
    LLTextureCtrl* pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
    if (pbr_ctrl)
    {
        LLUUID pbr_id;
        bool identical_pbr;
        LLSelectedTE::getPbrMaterialId(pbr_id, identical_pbr);

        has_pbr_material = pbr_id.notNull();

        pbr_ctrl->setTentative(identical_pbr ? FALSE : TRUE);
        pbr_ctrl->setEnabled(editable && has_pbr_capabilities);
        pbr_ctrl->setImageAssetID(pbr_id);
    }

    getChildView("pbr_from_inventory")->setEnabled(editable && has_pbr_capabilities);
    getChildView("edit_selected_pbr")->setEnabled(editable && has_pbr_material && has_pbr_capabilities);
    getChildView("save_selected_pbr")->setEnabled(objectp->permCopy() && has_pbr_material && has_pbr_capabilities);

    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();
    if (show_pbr)
    {
        const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
        const LLGLTFMaterial::TextureInfo texture_info = texture_info_from_pbrtype(pbr_type);
        const bool show_texture_info = texture_info != LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
        const bool new_state = show_texture_info && has_pbr_capabilities && has_pbr_material;

        LLUICtrl* gltfCtrlTextureScaleU = getChild<LLUICtrl>("gltfTextureScaleU");
        LLUICtrl* gltfCtrlTextureScaleV = getChild<LLUICtrl>("gltfTextureScaleV");
        LLUICtrl* gltfCtrlTextureRotation = getChild<LLUICtrl>("gltfTextureRotation");
        LLUICtrl* gltfCtrlTextureOffsetU = getChild<LLUICtrl>("gltfTextureOffsetU");
        LLUICtrl* gltfCtrlTextureOffsetV = getChild<LLUICtrl>("gltfTextureOffsetV");

        gltfCtrlTextureScaleU->setEnabled(new_state);
        gltfCtrlTextureScaleV->setEnabled(new_state);
        gltfCtrlTextureRotation->setEnabled(new_state);
        gltfCtrlTextureOffsetU->setEnabled(new_state);
        gltfCtrlTextureOffsetV->setEnabled(new_state);

        // Control values will be set once per frame in
        // setMaterialOverridesFromSelection
        sMaterialOverrideSelection.setDirty();
    }
}

void LLPanelFace::updateVisibilityGLTF()
{
    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();

    LLRadioGroup* radio_pbr_type = findChild<LLRadioGroup>("radio_pbr_type");
    radio_pbr_type->setVisible(show_pbr);

    const U32 pbr_type = radio_pbr_type->getSelectedIndex();
    const bool show_pbr_render_material_id = show_pbr && (pbr_type == PBRTYPE_RENDER_MATERIAL_ID);
    const bool show_pbr_base_color = show_pbr && (pbr_type == PBRTYPE_BASE_COLOR);
    const bool show_pbr_normal = show_pbr && (pbr_type == PBRTYPE_NORMAL);
    const bool show_pbr_metallic_roughness = show_pbr && (pbr_type == PBRTYPE_METALLIC_ROUGHNESS);
    const bool show_pbr_emissive = show_pbr && (pbr_type == PBRTYPE_EMISSIVE);
    const bool show_pbr_transform = show_pbr_base_color || show_pbr_normal || show_pbr_metallic_roughness || show_pbr_emissive;

    getChildView("pbr_control")->setVisible(show_pbr_render_material_id);

    getChildView("pbr_from_inventory")->setVisible(show_pbr_render_material_id);
    getChildView("edit_selected_pbr")->setVisible(show_pbr_render_material_id);
    getChildView("save_selected_pbr")->setVisible(show_pbr_render_material_id);

    getChildView("gltfTextureScaleU")->setVisible(show_pbr_transform);
    getChildView("gltfTextureScaleV")->setVisible(show_pbr_transform);
    getChildView("gltfTextureRotation")->setVisible(show_pbr_transform);
    getChildView("gltfTextureOffsetU")->setVisible(show_pbr_transform);
    getChildView("gltfTextureOffsetV")->setVisible(show_pbr_transform);
}

void LLPanelFace::updateCopyTexButton()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    mMenuClipboardTexture->setEnabled(objectp && objectp->getPCode() == LL_PCODE_VOLUME && objectp->permModify() 
                                                    && !objectp->isPermanentEnforced() && !objectp->isInventoryPending() 
                                                    && (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1));
    std::string tooltip = (objectp && objectp->isInventoryPending()) ? LLTrans::getString("LoadingContents") : getString("paste_options");
    mMenuClipboardTexture->setToolTip(tooltip);

}

void LLPanelFace::refresh()
{
	LL_DEBUGS("Materials") << LL_ENDL;
	getState();
}

void LLPanelFace::refreshMedia()
{
    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
    LLViewerObject* first_object = selected_objects->getFirstObject();

    if (!(first_object
        && first_object->getPCode() == LL_PCODE_VOLUME
        && first_object->permModify()
        ))
    {
        getChildView("add_media")->setEnabled(FALSE);
        mTitleMediaText->clear();
        clearMediaSettings();
        return;
    }

    std::string url = first_object->getRegion()->getCapability("ObjectMedia");
    bool has_media_capability = (!url.empty());

    if (!has_media_capability)
    {
        getChildView("add_media")->setEnabled(FALSE);
        LL_WARNS("LLFloaterToolsMedia") << "Media not enabled (no capability) in this region!" << LL_ENDL;
        clearMediaSettings();
        return;
    }

    BOOL is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
        && LLSelectMgr::getInstance()->selectGetRootsNonPermanentEnforced())
        || LLSelectMgr::getInstance()->selectGetNonPermanentEnforced();
    bool editable = is_nonpermanent_enforced && (first_object->permModify() || selectedMediaEditable());

    // Check modify permissions and whether any selected objects are in
    // the process of being fetched.  If they are, then we're not editable
    if (editable)
    {
        LLObjectSelection::iterator iter = selected_objects->begin();
        LLObjectSelection::iterator end = selected_objects->end();
        for (; iter != end; ++iter)
        {
            LLSelectNode* node = *iter;
            LLVOVolume* object = dynamic_cast<LLVOVolume*>(node->getObject());
            if (NULL != object)
            {
                if (!object->permModify())
                {
                    LL_INFOS("LLFloaterToolsMedia")
                        << "Selection not editable due to lack of modify permissions on object id "
                        << object->getID() << LL_ENDL;

                    editable = false;
                    break;
                }
            }
        }
    }

    // Media settings
    bool bool_has_media = false;
    struct media_functor : public LLSelectedTEGetFunctor<bool>
    {
        bool get(LLViewerObject* object, S32 face)
        {
            LLTextureEntry *te = object->getTE(face);
            if (te)
            {
                return te->hasMedia();
            }
            return false;
        }
    } func;


    // check if all faces have media(or, all dont have media)
    LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo = selected_objects->getSelectedTEValue(&func, bool_has_media);

    const LLMediaEntry default_media_data;

    struct functor_getter_media_data : public LLSelectedTEGetFunctor< LLMediaEntry>
    {
        functor_getter_media_data(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        LLMediaEntry get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return *(object->getTE(face)->getMediaData());
            return mMediaEntry;
        };

        const LLMediaEntry& mMediaEntry;

    } func_media_data(default_media_data);

    LLMediaEntry media_data_get;
    LLFloaterMediaSettings::getInstance()->mMultipleMedia = !(selected_objects->getSelectedTEValue(&func_media_data, media_data_get));

    std::string multi_media_info_str = LLTrans::getString("Multiple Media");
    std::string media_title = "";
    // update UI depending on whether "object" (prim or face) has media
    // and whether or not you are allowed to edit it.

    getChildView("add_media")->setEnabled(editable);
    // IF all the faces have media (or all dont have media)
    if (LLFloaterMediaSettings::getInstance()->mIdenticalHasMediaInfo)
    {
        // TODO: get media title and set it.
        mTitleMediaText->clear();
        // if identical is set, all faces are same (whether all empty or has the same media)
        if (!(LLFloaterMediaSettings::getInstance()->mMultipleMedia))
        {
            // Media data is valid
            if (media_data_get != default_media_data)
            {
                // initial media title is the media URL (until we get the name)
                media_title = media_data_get.getHomeURL();
            }
            // else all faces might be empty. 
        }
        else // there' re Different Medias' been set on on the faces.
        {
            media_title = multi_media_info_str;
        }

        getChildView("delete_media")->setEnabled(bool_has_media && editable);
        // TODO: display a list of all media on the face - use 'identical' flag
    }
    else // not all face has media but at least one does.
    {
        // seleted faces have not identical value
        LLFloaterMediaSettings::getInstance()->mMultipleValidMedia = selected_objects->isMultipleTEValue(&func_media_data, default_media_data);

        if (LLFloaterMediaSettings::getInstance()->mMultipleValidMedia)
        {
            media_title = multi_media_info_str;
        }
        else
        {
            // Media data is valid
            if (media_data_get != default_media_data)
            {
                // initial media title is the media URL (until we get the name)
                media_title = media_data_get.getHomeURL();
            }
        }

        getChildView("delete_media")->setEnabled(TRUE);
    }

    U32 materials_media = mComboMatMedia->getCurrentIndex();
    if (materials_media == MATMEDIA_MEDIA)
    {
        // currently displaying media info, navigateTo and update title
        navigateToTitleMedia(media_title);
    }
    else
    {
        // Media can be heavy, don't keep it around
        // MAC specific: MAC doesn't support setVolume(0) so if  not
        // unloaded, it might keep playing audio until user closes editor
        unloadMedia();
        mNeedMediaTitle = false;
    }

    mTitleMediaText->setText(media_title);

    // load values for media settings
    updateMediaSettings();

    LLFloaterMediaSettings::initValues(mMediaSettings, editable);
}

void LLPanelFace::unloadMedia()
{
    // destroy media source used to grab media title
    if (mTitleMedia)
        mTitleMedia->unloadMediaSource();
}

// static
void LLPanelFace::onMaterialOverrideReceived(const LLUUID& object_id, S32 side)
{
    sMaterialOverrideSelection.onSelectedObjectUpdated(object_id, side);
}

//////////////////////////////////////////////////////////////////////////////
//
void LLPanelFace::navigateToTitleMedia( const std::string url )
{
	std::string multi_media_info_str = LLTrans::getString("Multiple Media");
	if (url.empty() || multi_media_info_str == url)
	{
		// nothing to show
		mNeedMediaTitle = false;
	}
	else if (mTitleMedia)
	{
		LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
		// check if url changed or if we need a new media source
		if (mTitleMedia->getCurrentNavUrl() != url || media_plugin == NULL)
		{
			mTitleMedia->navigateTo( url );

            LLViewerMediaImpl* impl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mTitleMedia->getTextureID());
            if (impl)
            {
                // if it's a page with a movie, we don't want to hear it
                impl->setVolume(0);
            };
		}

		// flag that we need to update the title (even if no request were made)
		mNeedMediaTitle = true;
	}
}

bool LLPanelFace::selectedMediaEditable()
{
    U32 owner_mask_on;
    U32 owner_mask_off;
    U32 valid_owner_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_OWNER,
        &owner_mask_on, &owner_mask_off);
    U32 group_mask_on;
    U32 group_mask_off;
    U32 valid_group_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_GROUP,
        &group_mask_on, &group_mask_off);
    U32 everyone_mask_on;
    U32 everyone_mask_off;
    S32 valid_everyone_perms = LLSelectMgr::getInstance()->selectGetPerm(PERM_EVERYONE,
        &everyone_mask_on, &everyone_mask_off);

    bool selected_Media_editable = false;

    // if perms we got back are valid
    if (valid_owner_perms &&
        valid_group_perms &&
        valid_everyone_perms)
    {

        if ((owner_mask_on & PERM_MODIFY) ||
            (group_mask_on & PERM_MODIFY) ||
            (everyone_mask_on & PERM_MODIFY))
        {
            selected_Media_editable = true;
        }
        else
            // user is NOT allowed to press the RESET button
        {
            selected_Media_editable = false;
        };
    };

    return selected_Media_editable;
}

void LLPanelFace::clearMediaSettings()
{
    LLFloaterMediaSettings::clearValues(false);
}

void LLPanelFace::updateMediaSettings()
{
    bool identical(false);
    std::string base_key("");
    std::string value_str("");
    int value_int = 0;
    bool value_bool = false;
    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
    // TODO: (CP) refactor this using something clever or boost or both !!

    const LLMediaEntry default_media_data;

    // controls 
    U8 value_u8 = default_media_data.getControls();
    struct functor_getter_controls : public LLSelectedTEGetFunctor< U8 >
    {
        functor_getter_controls(const LLMediaEntry &entry) : mMediaEntry(entry) {}

        U8 get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getControls();
            return mMediaEntry.getControls();
        };

        const LLMediaEntry &mMediaEntry;

    } func_controls(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_controls, value_u8);
    base_key = std::string(LLMediaEntry::CONTROLS_KEY);
    mMediaSettings[base_key] = value_u8;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // First click (formerly left click)
    value_bool = default_media_data.getFirstClickInteract();
    struct functor_getter_first_click : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_first_click(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getFirstClickInteract();
            return mMediaEntry.getFirstClickInteract();
        };

        const LLMediaEntry &mMediaEntry;

    } func_first_click(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_first_click, value_bool);
    base_key = std::string(LLMediaEntry::FIRST_CLICK_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Home URL
    value_str = default_media_data.getHomeURL();
    struct functor_getter_home_url : public LLSelectedTEGetFunctor< std::string >
    {
        functor_getter_home_url(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::string get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getHomeURL();
            return mMediaEntry.getHomeURL();
        };

        const LLMediaEntry &mMediaEntry;

    } func_home_url(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_home_url, value_str);
    base_key = std::string(LLMediaEntry::HOME_URL_KEY);
    mMediaSettings[base_key] = value_str;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Current URL
    value_str = default_media_data.getCurrentURL();
    struct functor_getter_current_url : public LLSelectedTEGetFunctor< std::string >
    {
        functor_getter_current_url(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::string get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getCurrentURL();
            return mMediaEntry.getCurrentURL();
        };

        const LLMediaEntry &mMediaEntry;

    } func_current_url(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_current_url, value_str);
    base_key = std::string(LLMediaEntry::CURRENT_URL_KEY);
    mMediaSettings[base_key] = value_str;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto zoom
    value_bool = default_media_data.getAutoZoom();
    struct functor_getter_auto_zoom : public LLSelectedTEGetFunctor< bool >
    {

        functor_getter_auto_zoom(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoZoom();
            return mMediaEntry.getAutoZoom();
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_zoom(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_zoom, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_ZOOM_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto play
    //value_bool = default_media_data.getAutoPlay();
    // set default to auto play TRUE -- angela  EXT-5172
    value_bool = true;
    struct functor_getter_auto_play : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_play(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoPlay();
            //return mMediaEntry.getAutoPlay(); set default to auto play TRUE -- angela  EXT-5172
            return true;
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_play(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_play, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_PLAY_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;


    // Auto scale
    // set default to auto scale TRUE -- angela  EXT-5172
    //value_bool = default_media_data.getAutoScale();
    value_bool = true;
    struct functor_getter_auto_scale : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_scale(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoScale();
            // return mMediaEntry.getAutoScale();  set default to auto scale TRUE -- angela  EXT-5172
            return true;
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_scale(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_scale, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_SCALE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Auto loop
    value_bool = default_media_data.getAutoLoop();
    struct functor_getter_auto_loop : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_auto_loop(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAutoLoop();
            return mMediaEntry.getAutoLoop();
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_loop(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_loop, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_LOOP_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // width pixels (if not auto scaled)
    value_int = default_media_data.getWidthPixels();
    struct functor_getter_width_pixels : public LLSelectedTEGetFunctor< int >
    {
        functor_getter_width_pixels(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        int get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWidthPixels();
            return mMediaEntry.getWidthPixels();
        };

        const LLMediaEntry &mMediaEntry;

    } func_width_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_width_pixels, value_int);
    base_key = std::string(LLMediaEntry::WIDTH_PIXELS_KEY);
    mMediaSettings[base_key] = value_int;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // height pixels (if not auto scaled)
    value_int = default_media_data.getHeightPixels();
    struct functor_getter_height_pixels : public LLSelectedTEGetFunctor< int >
    {
        functor_getter_height_pixels(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        int get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getHeightPixels();
            return mMediaEntry.getHeightPixels();
        };

        const LLMediaEntry &mMediaEntry;

    } func_height_pixels(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_height_pixels, value_int);
    base_key = std::string(LLMediaEntry::HEIGHT_PIXELS_KEY);
    mMediaSettings[base_key] = value_int;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Enable Alt image
    value_bool = default_media_data.getAltImageEnable();
    struct functor_getter_enable_alt_image : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_enable_alt_image(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getAltImageEnable();
            return mMediaEntry.getAltImageEnable();
        };

        const LLMediaEntry &mMediaEntry;

    } func_enable_alt_image(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_enable_alt_image, value_bool);
    base_key = std::string(LLMediaEntry::ALT_IMAGE_ENABLE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - owner interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_OWNER);
    struct functor_getter_perms_owner_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_owner_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_OWNER));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_OWNER);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_owner_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_owner_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_OWNER_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - owner control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_OWNER);
    struct functor_getter_perms_owner_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_owner_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_OWNER));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_OWNER);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_owner_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_owner_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_OWNER_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - group interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_GROUP);
    struct functor_getter_perms_group_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_group_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_GROUP));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_GROUP);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_group_interact(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_group_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_GROUP_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - group control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_GROUP);
    struct functor_getter_perms_group_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_group_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_GROUP));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_GROUP);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_group_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_group_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_GROUP_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - anyone interact
    value_bool = 0 != (default_media_data.getPermsInteract() & LLMediaEntry::PERM_ANYONE);
    struct functor_getter_perms_anyone_interact : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_anyone_interact(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsInteract() & LLMediaEntry::PERM_ANYONE));
            return 0 != (mMediaEntry.getPermsInteract() & LLMediaEntry::PERM_ANYONE);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_anyone_interact(default_media_data);
    identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func_perms_anyone_interact, value_bool);
    base_key = std::string(LLPanelContents::PERMS_ANYONE_INTERACT_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // Perms - anyone control
    value_bool = 0 != (default_media_data.getPermsControl() & LLMediaEntry::PERM_ANYONE);
    struct functor_getter_perms_anyone_control : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_perms_anyone_control(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return (0 != (object->getTE(face)->getMediaData()->getPermsControl() & LLMediaEntry::PERM_ANYONE));
            return 0 != (mMediaEntry.getPermsControl() & LLMediaEntry::PERM_ANYONE);
        };

        const LLMediaEntry &mMediaEntry;

    } func_perms_anyone_control(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_perms_anyone_control, value_bool);
    base_key = std::string(LLPanelContents::PERMS_ANYONE_CONTROL_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // security - whitelist enable
    value_bool = default_media_data.getWhiteListEnable();
    struct functor_getter_whitelist_enable : public LLSelectedTEGetFunctor< bool >
    {
        functor_getter_whitelist_enable(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        bool get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWhiteListEnable();
            return mMediaEntry.getWhiteListEnable();
        };

        const LLMediaEntry &mMediaEntry;

    } func_whitelist_enable(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_whitelist_enable, value_bool);
    base_key = std::string(LLMediaEntry::WHITELIST_ENABLE_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;

    // security - whitelist URLs
    std::vector<std::string> value_vector_str = default_media_data.getWhiteList();
    struct functor_getter_whitelist_urls : public LLSelectedTEGetFunctor< std::vector<std::string> >
    {
        functor_getter_whitelist_urls(const LLMediaEntry& entry) : mMediaEntry(entry) {}

        std::vector<std::string> get(LLViewerObject* object, S32 face)
        {
            if (object)
                if (object->getTE(face))
                    if (object->getTE(face)->getMediaData())
                        return object->getTE(face)->getMediaData()->getWhiteList();
            return mMediaEntry.getWhiteList();
        };

        const LLMediaEntry &mMediaEntry;

    } func_whitelist_urls(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_whitelist_urls, value_vector_str);
    base_key = std::string(LLMediaEntry::WHITELIST_KEY);
    mMediaSettings[base_key].clear();
    std::vector< std::string >::iterator iter = value_vector_str.begin();
    while (iter != value_vector_str.end())
    {
        std::string white_list_url = *iter;
        mMediaSettings[base_key].append(white_list_url);
        ++iter;
    };

    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;
}

void LLPanelFace::updateMediaTitle()
{
    // only get the media name if we need it
    if (!mNeedMediaTitle)
        return;

    // get plugin impl
    LLPluginClassMedia* media_plugin = mTitleMedia->getMediaPlugin();
    if (media_plugin && mTitleMedia->getCurrentNavUrl() == media_plugin->getNavigateURI())
    {
        // get the media name (asynchronous - must call repeatedly)
        std::string media_title = media_plugin->getMediaName();

        // only replace the title if what we get contains something
        if (!media_title.empty())
        {
            // update the UI widget
            if (mTitleMediaText)
            {
                mTitleMediaText->setText(media_title);

                // stop looking for a title when we get one
                mNeedMediaTitle = false;
            };
        };
    };
}

//
// Static functions
//

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
	return (F32)(object->getTE(face)->getGlow());
}


void LLPanelFace::onCommitColor(const LLSD& data)
{
	sendColor();
}

void LLPanelFace::onCommitShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
}

void LLPanelFace::onCommitAlpha(const LLSD& data)
{
	sendAlpha();
}

void LLPanelFace::onCancelColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertColors();
}

void LLPanelFace::onCancelShinyColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertShinyColors();
}

void LLPanelFace::onSelectColor(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectColors();
	sendColor();
}

void LLPanelFace::onSelectShinyColor(const LLSD& data)
{
	LLSelectedTEMaterial::setSpecularLightColor(this, getChild<LLColorSwatchCtrl>("shinycolorswatch")->get());
	LLSelectMgr::getInstance()->saveSelectedShinyColors();
}

// static
void LLPanelFace::onCommitMaterialsMedia(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	// Force to default states to side-step problems with menu contents
	// and generally reflecting old state when switching tabs or objects
	//
	self->updateShinyControls(false,true);
	self->updateBumpyControls(false,true);
	self->updateUI();
	self->refreshMedia();
}

void LLPanelFace::updateVisibility()
{
    LLRadioGroup* radio_mat_type = findChild<LLRadioGroup>("radio_material_type");
    LLRadioGroup* radio_pbr_type = findChild<LLRadioGroup>("radio_pbr_type");
    LLComboBox* combo_shininess = findChild<LLComboBox>("combobox shininess");
    LLComboBox* combo_bumpiness = findChild<LLComboBox>("combobox bumpiness");
	if (!radio_mat_type || !radio_pbr_type || !mComboMatMedia || !combo_shininess || !combo_bumpiness)
	{
		LL_WARNS("Materials") << "Combo box not found...exiting." << LL_ENDL;
		return;
	}
	U32 materials_media = mComboMatMedia->getCurrentIndex();
	U32 material_type = radio_mat_type->getSelectedIndex();
	bool show_media = (materials_media == MATMEDIA_MEDIA) && mComboMatMedia->getEnabled();
    bool show_material = materials_media == MATMEDIA_MATERIAL;
	bool show_texture = (show_media || (show_material && (material_type == MATTYPE_DIFFUSE) && mComboMatMedia->getEnabled()));
	bool show_bumpiness = show_material && (material_type == MATTYPE_NORMAL) && mComboMatMedia->getEnabled();
	bool show_shininess = show_material && (material_type == MATTYPE_SPECULAR) && mComboMatMedia->getEnabled();
    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    const LLGLTFMaterial::TextureInfo texture_info = texture_info_from_pbrtype(pbr_type);
    const bool show_texture_info = show_pbr && texture_info != LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;

    radio_mat_type->setVisible(show_material);

    // Shared material controls
    getChildView("checkbox_sync_settings")->setVisible(show_material || show_media || show_texture_info);
    getChildView("tex gen")->setVisible(show_material || show_media || show_texture_info);
    getChildView("combobox texgen")->setVisible(show_material || show_media || show_texture_info);
    getChildView("button align textures")->setVisible(show_material || show_media);

	// Media controls
    mTitleMediaText->setVisible(show_media);
	getChildView("add_media")->setVisible(show_media);
	getChildView("delete_media")->setVisible(show_media);
	getChildView("button align")->setVisible(show_media);

	// Diffuse texture controls
	getChildView("texture control")->setVisible(show_texture && show_material);
	getChildView("label alphamode")->setVisible(show_texture && show_material);
	getChildView("combobox alphamode")->setVisible(show_texture && show_material);
	getChildView("label maskcutoff")->setVisible(false);
	getChildView("maskcutoff")->setVisible(false);
	if (show_texture && show_material)
	{
		updateAlphaControls();
	}
    // texture scale and position controls
	getChildView("TexScaleU")->setVisible(show_texture);
	getChildView("TexScaleV")->setVisible(show_texture);
	getChildView("TexRot")->setVisible(show_texture);
	getChildView("TexOffsetU")->setVisible(show_texture);
	getChildView("TexOffsetV")->setVisible(show_texture);

	// Specular map controls
	getChildView("shinytexture control")->setVisible(show_shininess);
	getChildView("combobox shininess")->setVisible(show_shininess);
	getChildView("label shininess")->setVisible(show_shininess);
	getChildView("label glossiness")->setVisible(false);
	getChildView("glossiness")->setVisible(false);
	getChildView("label environment")->setVisible(false);
	getChildView("environment")->setVisible(false);
	getChildView("label shinycolor")->setVisible(false);
	getChildView("shinycolorswatch")->setVisible(false);
	if (show_shininess)
	{
		updateShinyControls();
	}
	getChildView("shinyScaleU")->setVisible(show_shininess);
	getChildView("shinyScaleV")->setVisible(show_shininess);
	getChildView("shinyRot")->setVisible(show_shininess);
	getChildView("shinyOffsetU")->setVisible(show_shininess);
	getChildView("shinyOffsetV")->setVisible(show_shininess);

	// Normal map controls
	if (show_bumpiness)
	{
		updateBumpyControls();
	}
	getChildView("bumpytexture control")->setVisible(show_bumpiness);
	getChildView("combobox bumpiness")->setVisible(show_bumpiness);
	getChildView("label bumpiness")->setVisible(show_bumpiness);
	getChildView("bumpyScaleU")->setVisible(show_bumpiness);
	getChildView("bumpyScaleV")->setVisible(show_bumpiness);
	getChildView("bumpyRot")->setVisible(show_bumpiness);
	getChildView("bumpyOffsetU")->setVisible(show_bumpiness);
	getChildView("bumpyOffsetV")->setVisible(show_bumpiness);

    getChild<LLSpinCtrl>("rptctrl")->setVisible(show_material || show_media);

    // PBR controls
    updateVisibilityGLTF();
}

// static
void LLPanelFace::onCommitMaterialType(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*) userdata;
	 // Force to default states to side-step problems with menu contents
	 // and generally reflecting old state when switching tabs or objects
	 //
	 self->updateShinyControls(false,true);
	 self->updateBumpyControls(false,true);
    self->updateUI();
}

// static
void LLPanelFace::onCommitPbrType(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*)userdata;
    // Force to default states to side-step problems with menu contents
    // and generally reflecting old state when switching tabs or objects
    //
    self->updateUI();
}

// static
void LLPanelFace::onCommitBump(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	LLComboBox*	mComboBumpiness = self->getChild<LLComboBox>("combobox bumpiness");
	if(!mComboBumpiness)
		return;

	U32 bumpiness = mComboBumpiness->getCurrentIndex();

	self->sendBump(bumpiness);
}

// static
void LLPanelFace::onCommitTexGen(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTexGen();
}

// static
void LLPanelFace::updateShinyControls(bool is_setting_texture, bool mess_with_shiny_combobox)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("shinytexture control");
	LLUUID shiny_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "Shiny texture selected: " << shiny_texture_ID << LL_ENDL;
	LLComboBox* comboShiny = getChild<LLComboBox>("combobox shininess");

	if(mess_with_shiny_combobox)
	{
		if (!comboShiny)
		{
			return;
		}
		if (!shiny_texture_ID.isNull() && is_setting_texture)
		{
			if (!comboShiny->itemExists(USE_TEXTURE))
			{
				comboShiny->add(USE_TEXTURE);
			}
			comboShiny->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboShiny->itemExists(USE_TEXTURE))
			{
				comboShiny->remove(SHINY_TEXTURE);
				comboShiny->selectFirstItem();
			}
		}
	}
	else
	{
		if (shiny_texture_ID.isNull() && comboShiny && comboShiny->itemExists(USE_TEXTURE))
		{
			comboShiny->remove(SHINY_TEXTURE);
			comboShiny->selectFirstItem();
		}
	}


	LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
	U32 materials_media = mComboMatMedia->getCurrentIndex();
	U32 material_type = radio_mat_type->getSelectedIndex();
	bool show_material = (materials_media == MATMEDIA_MATERIAL);
	bool show_shininess = show_material && (material_type == MATTYPE_SPECULAR) && mComboMatMedia->getEnabled();
	U32 shiny_value = comboShiny->getCurrentIndex();
	bool show_shinyctrls = (shiny_value == SHINY_TEXTURE) && show_shininess; // Use texture
	getChildView("label glossiness")->setVisible(show_shinyctrls);
	getChildView("glossiness")->setVisible(show_shinyctrls);
	getChildView("label environment")->setVisible(show_shinyctrls);
	getChildView("environment")->setVisible(show_shinyctrls);
	getChildView("label shinycolor")->setVisible(show_shinyctrls);
	getChildView("shinycolorswatch")->setVisible(show_shinyctrls);
}

// static
void LLPanelFace::updateBumpyControls(bool is_setting_texture, bool mess_with_combobox)
{
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
	LLUUID bumpy_texture_ID = texture_ctrl->getImageAssetID();
	LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;
	LLComboBox* comboBumpy = getChild<LLComboBox>("combobox bumpiness");
	if (!comboBumpy)
	{
		return;
	}

	if (mess_with_combobox)
	{
		LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>("bumpytexture control");
		LLUUID bumpy_texture_ID = texture_ctrl->getImageAssetID();
		LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;

		if (!bumpy_texture_ID.isNull() && is_setting_texture)
		{
			if (!comboBumpy->itemExists(USE_TEXTURE))
			{
				comboBumpy->add(USE_TEXTURE);
			}
			comboBumpy->setSimple(USE_TEXTURE);
		}
		else
		{
			if (comboBumpy->itemExists(USE_TEXTURE))
			{
				comboBumpy->remove(BUMPY_TEXTURE);
				comboBumpy->selectFirstItem();
			}
		}
	}
}

// static
void LLPanelFace::onCommitShiny(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;


	LLComboBox*	mComboShininess = self->getChild<LLComboBox>("combobox shininess");
	if(!mComboShininess)
		return;
	
	U32 shininess = mComboShininess->getCurrentIndex();

	self->sendShiny(shininess);
}

// static
void LLPanelFace::updateAlphaControls()
{
	LLComboBox* comboAlphaMode = getChild<LLComboBox>("combobox alphamode");
	if (!comboAlphaMode)
	{
		return;
	}
	U32 alpha_value = comboAlphaMode->getCurrentIndex();
	bool show_alphactrls = (alpha_value == ALPHAMODE_MASK); // Alpha masking
    
    U32 mat_media = MATMEDIA_MATERIAL;
    if (mComboMatMedia)
    {
        mat_media = mComboMatMedia->getCurrentIndex();
    }
    
    U32 mat_type = MATTYPE_DIFFUSE;
    LLRadioGroup* radio_mat_type = getChild<LLRadioGroup>("radio_material_type");
    if(radio_mat_type)
    {
        mat_type = radio_mat_type->getSelectedIndex();
    }

    show_alphactrls = show_alphactrls && (mat_media == MATMEDIA_MATERIAL);
    show_alphactrls = show_alphactrls && (mat_type == MATTYPE_DIFFUSE);
    
	getChildView("label maskcutoff")->setVisible(show_alphactrls);
	getChildView("maskcutoff")->setVisible(show_alphactrls);
}

// static
void LLPanelFace::onCommitAlphaMode(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->updateAlphaControls();
	LLSelectedTEMaterial::setDiffuseAlphaMode(self,self->getCurrentDiffuseAlphaMode());
}

// static
void LLPanelFace::onCommitFullbright(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendFullbright();
}

// static
void LLPanelFace::onCommitGlow(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendGlow();
}

// static
BOOL LLPanelFace::onDragPbr(LLUICtrl*, LLInventoryItem* item)
{
    BOOL accept = TRUE;
    for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
        iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* obj = node->getObject();
        if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
        {
            accept = FALSE;
            break;
        }
    }
    return accept;
}

void LLPanelFace::onCommitPbr(const LLSD& data)
{
    LLTextureCtrl* pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
    if (!pbr_ctrl) return;
    if (!pbr_ctrl->getTentative())
    {
        // we grab the item id first, because we want to do a
        // permissions check in the selection manager. ARGH!
        LLUUID id = pbr_ctrl->getImageItemID();
        if (id.isNull())
        {
            id = pbr_ctrl->getImageAssetID();
        }
        LLSelectMgr::getInstance()->selectionSetGLTFMaterial(id);
    }
}

void LLPanelFace::onCancelPbr(const LLSD& data)
{
    LLSelectMgr::getInstance()->selectionRevertGLTFMaterials();
}

void LLPanelFace::onSelectPbr(const LLSD& data)
{
    LLSelectMgr::getInstance()->saveSelectedObjectTextures();

    LLTextureCtrl* pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
    if (!pbr_ctrl) return;
    if (!pbr_ctrl->getTentative())
    {
        // we grab the item id first, because we want to do a
        // permissions check in the selection manager. ARGH!
        LLUUID id = pbr_ctrl->getImageItemID();
        if (id.isNull())
        {
            id = pbr_ctrl->getImageAssetID();
        }
        LLSelectMgr::getInstance()->selectionSetGLTFMaterial(id);
        LLSelectedTEMaterial::setMaterialID(this, id);
    }
}

// static
BOOL LLPanelFace::onDragTexture(LLUICtrl*, LLInventoryItem* item)
{
    BOOL accept = TRUE;
    for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
        iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* obj = node->getObject();
        if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
        {
            accept = FALSE;
            break;
        }
    }
    return accept;
}

void LLPanelFace::onCommitTexture( const LLSD& data )
{
	add(LLStatViewer::EDIT_TEXTURE, 1);
	sendTexture();
}

void LLPanelFace::onCancelTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->selectionRevertTextures();
}

void LLPanelFace::onSelectTexture(const LLSD& data)
{
	LLSelectMgr::getInstance()->saveSelectedObjectTextures();
	sendTexture();

	LLGLenum image_format;
	bool identical_image_format = false;
	LLSelectedTE::getImageFormat(image_format, identical_image_format);
    
	LLCtrlSelectionInterface* combobox_alphamode =
		childGetSelectionInterface("combobox alphamode");

	U32 alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
	if (combobox_alphamode)
	{
		switch (image_format)
		{
		case GL_RGBA:
		case GL_ALPHA:
			{
				alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
			}
			break;

		case GL_RGB: break;
		default:
			{
				LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
			}
			break;
		}

		combobox_alphamode->selectNthItem(alpha_mode);
	}
	LLSelectedTEMaterial::setDiffuseAlphaMode(this, getCurrentDiffuseAlphaMode());
}

void LLPanelFace::onCloseTexturePicker(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	updateUI();
}

void LLPanelFace::onCommitSpecularTexture( const LLSD& data )
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onCommitNormalTexture( const LLSD& data )
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

void LLPanelFace::onCancelSpecularTexture(const LLSD& data)
{
	U8 shiny = 0;
	bool identical_shiny = false;
	LLSelectedTE::getShiny(shiny, identical_shiny);
	LLUUID spec_map_id = getChild<LLTextureCtrl>("shinytexture control")->getImageAssetID();
	shiny = spec_map_id.isNull() ? shiny : SHINY_TEXTURE;
	sendShiny(shiny);
}

void LLPanelFace::onCancelNormalTexture(const LLSD& data)
{
	U8 bumpy = 0;
	bool identical_bumpy = false;
	LLSelectedTE::getBumpmap(bumpy, identical_bumpy);
	LLUUID spec_map_id = getChild<LLTextureCtrl>("bumpytexture control")->getImageAssetID();
	bumpy = spec_map_id.isNull() ? bumpy : BUMPY_TEXTURE;
	sendBump(bumpy);
}

void LLPanelFace::onSelectSpecularTexture(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onSelectNormalTexture(const LLSD& data)
{
	LL_DEBUGS("Materials") << data << LL_ENDL;
	LLUUID nmap_id = getCurrentNormalMap();
	sendBump(nmap_id.isNull() ? 0 : BUMPY_TEXTURE);
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to edit existing media settings on a prim or prim face
// TODO: test if there is media on the item and only allow editing if present
void LLPanelFace::onClickBtnEditMedia(LLUICtrl* ctrl, void* userdata)
{
    LLPanelFace* self = (LLPanelFace*)userdata;
    self->refreshMedia();
    LLFloaterReg::showInstance("media_settings");
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to delete media from a prim or prim face
void LLPanelFace::onClickBtnDeleteMedia(LLUICtrl* ctrl, void* userdata)
{
    LLNotificationsUtil::add("DeleteMedia", LLSD(), LLSD(), deleteMediaConfirm);
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to add media to a prim or prim face
void LLPanelFace::onClickBtnAddMedia(LLUICtrl* ctrl, void* userdata)
{
    // check if multiple faces are selected
    if (LLSelectMgr::getInstance()->getSelection()->isMultipleTESelected())
    {
        LLPanelFace* self = (LLPanelFace*)userdata;
        self->refreshMedia();
        LLNotificationsUtil::add("MultipleFacesSelected", LLSD(), LLSD(), multipleFacesSelectedConfirm);
    }
    else
    {
        onClickBtnEditMedia(ctrl, userdata);
    }
}

// static
bool LLPanelFace::deleteMediaConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch (option)
    {
    case 0:  // "Yes"
        LLSelectMgr::getInstance()->selectionSetMedia(0, LLSD());
        if (LLFloaterReg::instanceVisible("media_settings"))
        {
            LLFloaterReg::hideInstance("media_settings");
        }
        break;

    case 1:  // "No"
    default:
        break;
    }
    return false;
}

// static
bool LLPanelFace::multipleFacesSelectedConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch (option)
    {
    case 0:  // "Yes"
        LLFloaterReg::showInstance("media_settings");
        break;
    case 1:  // "No"
    default:
        break;
    }
    return false;
}

//static
void LLPanelFace::syncOffsetX(LLPanelFace* self, F32 offsetU)
{
	LLSelectedTEMaterial::setNormalOffsetX(self,offsetU);
	LLSelectedTEMaterial::setSpecularOffsetX(self,offsetU);
	self->getChild<LLSpinCtrl>("TexOffsetU")->forceSetValue(offsetU);
	self->sendTextureInfo();
}

//static
void LLPanelFace::syncOffsetY(LLPanelFace* self, F32 offsetV)
{
	LLSelectedTEMaterial::setNormalOffsetY(self,offsetV);
	LLSelectedTEMaterial::setSpecularOffsetY(self,offsetV);
	self->getChild<LLSpinCtrl>("TexOffsetV")->forceSetValue(offsetV);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyOffsetX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self,self->getCurrentBumpyOffsetU());
	}
	else
	{
		LLSelectedTEMaterial::setNormalOffsetX(self,self->getCurrentBumpyOffsetU());
	}

}

//static
void LLPanelFace::onCommitMaterialBumpyOffsetY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self,self->getCurrentBumpyOffsetV());
	}
	else
	{
		LLSelectedTEMaterial::setNormalOffsetY(self,self->getCurrentBumpyOffsetV());
	}
}

//static
void LLPanelFace::onCommitMaterialShinyOffsetX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self, self->getCurrentShinyOffsetU());
	}
	else
	{
		LLSelectedTEMaterial::setSpecularOffsetX(self,self->getCurrentShinyOffsetU());
	}
}

//static
void LLPanelFace::onCommitMaterialShinyOffsetY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self,self->getCurrentShinyOffsetV());
	}
	else
	{
		LLSelectedTEMaterial::setSpecularOffsetY(self,self->getCurrentShinyOffsetV());
	}
}

//static
void LLPanelFace::syncRepeatX(LLPanelFace* self, F32 scaleU)
{
	LLSelectedTEMaterial::setNormalRepeatX(self,scaleU);
	LLSelectedTEMaterial::setSpecularRepeatX(self,scaleU);
	self->sendTextureInfo();
}

//static
void LLPanelFace::syncRepeatY(LLPanelFace* self, F32 scaleV)
{
	LLSelectedTEMaterial::setNormalRepeatY(self,scaleV);
	LLSelectedTEMaterial::setSpecularRepeatY(self,scaleV);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyScaleX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 bumpy_scale_u = self->getCurrentBumpyScaleU();
	if (self->isIdenticalPlanarTexgen())
	{
		bumpy_scale_u *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleU")->forceSetValue(self->getCurrentBumpyScaleU());
		syncRepeatX(self, bumpy_scale_u);
	}
	else
	{
		LLSelectedTEMaterial::setNormalRepeatX(self,bumpy_scale_u);
	}
}

//static
void LLPanelFace::onCommitMaterialBumpyScaleY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 bumpy_scale_v = self->getCurrentBumpyScaleV();
	if (self->isIdenticalPlanarTexgen())
	{
		bumpy_scale_v *= 0.5f;
	}


	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(self->getCurrentBumpyScaleV());
		syncRepeatY(self, bumpy_scale_v);
	}
	else
	{
		LLSelectedTEMaterial::setNormalRepeatY(self,bumpy_scale_v);
	}
}

//static
void LLPanelFace::onCommitMaterialShinyScaleX(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 shiny_scale_u = self->getCurrentShinyScaleU();
	if (self->isIdenticalPlanarTexgen())
	{
		shiny_scale_u *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleU")->forceSetValue(self->getCurrentShinyScaleU());
		syncRepeatX(self, shiny_scale_u);
	}
	else
	{
		LLSelectedTEMaterial::setSpecularRepeatX(self,shiny_scale_u);
	}
}

//static
void LLPanelFace::onCommitMaterialShinyScaleY(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	F32 shiny_scale_v = self->getCurrentShinyScaleV();
	if (self->isIdenticalPlanarTexgen())
	{
		shiny_scale_v *= 0.5f;
	}

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexScaleV")->forceSetValue(self->getCurrentShinyScaleV());
		syncRepeatY(self, shiny_scale_v);
	}
	else
	{
		LLSelectedTEMaterial::setSpecularRepeatY(self,shiny_scale_v);
	}
}

//static
void LLPanelFace::syncMaterialRot(LLPanelFace* self, F32 rot, int te)
{
	LLSelectedTEMaterial::setNormalRotation(self,rot * DEG_TO_RAD, te);
	LLSelectedTEMaterial::setSpecularRotation(self,rot * DEG_TO_RAD, te);
	self->sendTextureInfo();
}

//static
void LLPanelFace::onCommitMaterialBumpyRot(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexRot")->forceSetValue(self->getCurrentBumpyRot());
		syncMaterialRot(self, self->getCurrentBumpyRot());
	}
	else
	{
        if ((bool)self->childGetValue("checkbox planar align").asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(self, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setNormalRotation(self, self->getCurrentBumpyRot() * DEG_TO_RAD);
        }
	}
}

//static
void LLPanelFace::onCommitMaterialShinyRot(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		self->getChild<LLSpinCtrl>("TexRot")->forceSetValue(self->getCurrentShinyRot());
		syncMaterialRot(self, self->getCurrentShinyRot());
	}
	else
	{
        if ((bool)self->childGetValue("checkbox planar align").asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(self, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setSpecularRotation(self, self->getCurrentShinyRot() * DEG_TO_RAD);
        }
	}
}

//static
void LLPanelFace::onCommitMaterialGloss(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	LLSelectedTEMaterial::setSpecularLightExponent(self,self->getCurrentGlossiness());
}

//static
void LLPanelFace::onCommitMaterialEnv(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	llassert_always(self);
	LLSelectedTEMaterial::setEnvironmentIntensity(self,self->getCurrentEnvIntensity());
}

//static
void LLPanelFace::onCommitMaterialMaskCutoff(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	LLSelectedTEMaterial::setAlphaMaskCutoff(self,self->getCurrentAlphaMaskCutoff());
}

// static
void LLPanelFace::onCommitTextureInfo( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->sendTextureInfo();
	// vertical scale and repeats per meter depends on each other, so force set on changes
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureScaleX( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		F32 bumpy_scale_u = self->getChild<LLUICtrl>("TexScaleU")->getValue().asReal();
		if (self->isIdenticalPlanarTexgen())
		{
			bumpy_scale_u *= 0.5f;
		}
		syncRepeatX(self, bumpy_scale_u);
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureScaleY( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		F32 bumpy_scale_v = self->getChild<LLUICtrl>("TexScaleV")->getValue().asReal();
		if (self->isIdenticalPlanarTexgen())
		{
			bumpy_scale_v *= 0.5f;
		}
		syncRepeatY(self, bumpy_scale_v);
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureRot( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;

	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncMaterialRot(self, self->getChild<LLUICtrl>("TexRot")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureOffsetX( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetX(self, self->getChild<LLUICtrl>("TexOffsetU")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// static
void LLPanelFace::onCommitTextureOffsetY( LLUICtrl* ctrl, void* userdata )
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		syncOffsetY(self, self->getChild<LLUICtrl>("TexOffsetV")->getValue().asReal());
	}
	else
	{
		self->sendTextureInfo();
	}
	self->updateUI(true);
}

// Commit the number of repeats per meter
// static
void LLPanelFace::onCommitRepeatsPerMeter(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	
	LLUICtrl*	repeats_ctrl	= self->getChild<LLUICtrl>("rptctrl");
	
	U32 materials_media = self->mComboMatMedia->getCurrentIndex();
    U32 material_type = 0;
    if (materials_media == MATMEDIA_PBR)
    {
        LLRadioGroup* radio_mat_type = self->getChild<LLRadioGroup>("radio_pbr_type");
        material_type = radio_mat_type->getSelectedIndex();
    }
    if (materials_media == MATMEDIA_MATERIAL)
    {
        LLRadioGroup* radio_mat_type = self->getChild<LLRadioGroup>("radio_material_type");
        material_type = radio_mat_type->getSelectedIndex();
    }

	F32 repeats_per_meter	= repeats_ctrl->getValue().asReal();
	
   F32 obj_scale_s = 1.0f;
   F32 obj_scale_t = 1.0f;

	bool identical_scale_s = false;
	bool identical_scale_t = false;

	LLSelectedTE::getObjectScaleS(obj_scale_s, identical_scale_s);
	LLSelectedTE::getObjectScaleS(obj_scale_t, identical_scale_t);

	LLUICtrl* bumpy_scale_u = self->getChild<LLUICtrl>("bumpyScaleU");
	LLUICtrl* bumpy_scale_v = self->getChild<LLUICtrl>("bumpyScaleV");
	LLUICtrl* shiny_scale_u = self->getChild<LLUICtrl>("shinyScaleU");
	LLUICtrl* shiny_scale_v = self->getChild<LLUICtrl>("shinyScaleV");
 
	if (gSavedSettings.getBOOL("SyncMaterialSettings"))
	{
		LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );

		bumpy_scale_u->setValue(obj_scale_s * repeats_per_meter);
		bumpy_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setNormalRepeatX(self,obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setNormalRepeatY(self,obj_scale_t * repeats_per_meter);

		shiny_scale_u->setValue(obj_scale_s * repeats_per_meter);
		shiny_scale_v->setValue(obj_scale_t * repeats_per_meter);

		LLSelectedTEMaterial::setSpecularRepeatX(self,obj_scale_s * repeats_per_meter);
		LLSelectedTEMaterial::setSpecularRepeatY(self,obj_scale_t * repeats_per_meter);
	}
	else
	{
		switch (material_type)
		{
			case MATTYPE_DIFFUSE:
			{
				LLSelectMgr::getInstance()->selectionTexScaleAutofit( repeats_per_meter );
			}
			break;

			case MATTYPE_NORMAL:
			{
				bumpy_scale_u->setValue(obj_scale_s * repeats_per_meter);
				bumpy_scale_v->setValue(obj_scale_t * repeats_per_meter);

				LLSelectedTEMaterial::setNormalRepeatX(self,obj_scale_s * repeats_per_meter);
				LLSelectedTEMaterial::setNormalRepeatY(self,obj_scale_t * repeats_per_meter);
			}
			break;

			case MATTYPE_SPECULAR:
			{
				shiny_scale_u->setValue(obj_scale_s * repeats_per_meter);
				shiny_scale_v->setValue(obj_scale_t * repeats_per_meter);

				LLSelectedTEMaterial::setSpecularRepeatX(self,obj_scale_s * repeats_per_meter);
				LLSelectedTEMaterial::setSpecularRepeatY(self,obj_scale_t * repeats_per_meter);
			}
			break;

			default:
				llassert(false);
				break;
		}
	}
	// vertical scale and repeats per meter depends on each other, so force set on changes
	self->updateUI(true);
}

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
	virtual bool apply(LLViewerObject* object, S32 te)
	{
		viewer_media_t pMediaImpl;
				
		const LLTextureEntry* tep = object->getTE(te);
		const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL;
		if ( mep )
		{
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());
		}
		
		if ( pMediaImpl.isNull())
		{
			// If we didn't find face media for this face, check whether this face is showing parcel media.
			pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getID());
		}
		
		if ( pMediaImpl.notNull())
		{
			LLPluginClassMedia *media = pMediaImpl->getMediaPlugin();
			if(media)
			{
				S32 media_width = media->getWidth();
				S32 media_height = media->getHeight();
				S32 texture_width = media->getTextureWidth();
				S32 texture_height = media->getTextureHeight();
				F32 scale_s = (F32)media_width / (F32)texture_width;
				F32 scale_t = (F32)media_height / (F32)texture_height;

				// set scale and adjust offset
				object->setTEScaleS( te, scale_s );
				object->setTEScaleT( te, scale_t );	// don't need to flip Y anymore since QT does this for us now.
				object->setTEOffsetS( te, -( 1.0f - scale_s ) / 2.0f );
				object->setTEOffsetT( te, -( 1.0f - scale_t ) / 2.0f );
			}
		}
		return true;
	};
};

void LLPanelFace::onClickAutoFix(void* userdata)
{
	LLPanelFaceSetMediaFunctor setfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

	LLPanelFaceSendFunctor sendfunc;
	LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::onAlignTexture(void* userdata)
{
    LLPanelFace* self = (LLPanelFace*)userdata;
    self->alignTestureLayer();
}

void LLPanelFace::onClickBtnLoadInvPBR(void* userdata)
{
    // Shouldn't this be "save to inventory?"
    LLPanelFace* self = (LLPanelFace*)userdata;
    LLTextureCtrl* pbr_ctrl = self->findChild<LLTextureCtrl>("pbr_control");
    pbr_ctrl->showPicker(true);
}

void LLPanelFace::onClickBtnEditPBR(void* userdata)
{
    LLMaterialEditor::loadLive();
}

void LLPanelFace::onClickBtnSavePBR(void* userdata)
{
    LLMaterialEditor::saveObjectsMaterialAs();
}

enum EPasteMode
{
    PASTE_COLOR,
    PASTE_TEXTURE
};

struct LLPanelFacePasteTexFunctor : public LLSelectedTEFunctor
{
    LLPanelFacePasteTexFunctor(LLPanelFace* panel, EPasteMode mode) :
        mPanelFace(panel), mMode(mode) {}

    virtual bool apply(LLViewerObject* objectp, S32 te)
    {
        switch (mMode)
        {
        case PASTE_COLOR:
            mPanelFace->onPasteColor(objectp, te);
            break;
        case PASTE_TEXTURE:
            mPanelFace->onPasteTexture(objectp, te);
            break;
        }
        return true;
    }
private:
    LLPanelFace *mPanelFace;
    EPasteMode mMode;
};

struct LLPanelFaceUpdateFunctor : public LLSelectedObjectFunctor
{
    LLPanelFaceUpdateFunctor(bool update_media)
        : mUpdateMedia(update_media)
    {}

    virtual bool apply(LLViewerObject* object)
    {
        object->sendTEUpdate();

        if (mUpdateMedia)
        {
            LLVOVolume *vo = dynamic_cast<LLVOVolume*>(object);
            if (vo && vo->hasMedia())
            {
                vo->sendMediaDataUpdate();
            }
        }
        return true;
    }
private:
    bool mUpdateMedia;
};

struct LLPanelFaceNavigateHomeFunctor : public LLSelectedTEFunctor
{
    virtual bool apply(LLViewerObject* objectp, S32 te)
    {
        if (objectp && objectp->getTE(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            const LLMediaEntry *media_data = tep->getMediaData();
            if (media_data)
            {
                if (media_data->getCurrentURL().empty() && media_data->getAutoPlay())
                {
                    viewer_media_t media_impl =
                        LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getMediaData()->getMediaID());
                    if (media_impl)
                    {
                        media_impl->navigateHome();
                    }
                }
            }
        }
        return true;
    }
};

void LLPanelFace::onCopyColor()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        return;
    }

    if (mClipboardParams.has("color"))
    {
        mClipboardParams["color"].clear();
    }
    else
    {
        mClipboardParams["color"] = LLSD::emptyArray();
    }

    std::map<LLUUID, LLUUID> asset_item_map;

    // a way to resolve situations where source and target have different amount of faces
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    mClipboardParams["color_all_tes"] = (num_tes != 1) || (LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool());
    for (S32 te = 0; te < num_tes; ++te)
    {
        if (node->isTESelected(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            if (tep)
            {
                LLSD te_data;

                // asLLSD() includes media
                te_data["te"] = tep->asLLSD(); // Note: includes a lot more than just color/alpha/glow

                mClipboardParams["color"].append(te_data);
            }
        }
    }
}

void LLPanelFace::onPasteColor()
{
    if (!mClipboardParams.has("color"))
    {
        return;
    }

    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        // not supposed to happen
        LL_WARNS() << "Failed to paste color due to missing or wrong selection" << LL_ENDL;
        return;
    }

    bool face_selection_mode = LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
    LLSD &clipboard = mClipboardParams["color"]; // array
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    S32 compare_tes = num_tes;

    if (face_selection_mode)
    {
        compare_tes = 0;
        for (S32 te = 0; te < num_tes; ++te)
        {
            if (node->isTESelected(te))
            {
                compare_tes++;
            }
        }
    }

    // we can copy if single face was copied in edit face mode or if face count matches
    if (!((clipboard.size() == 1) && mClipboardParams["color_all_tes"].asBoolean())
        && compare_tes != clipboard.size())
    {
        LLSD notif_args;
        if (face_selection_mode)
        {
            static std::string reason = getString("paste_error_face_selection_mismatch");
            notif_args["REASON"] = reason;
        }
        else
        {
            static std::string reason = getString("paste_error_object_face_count_mismatch");
            notif_args["REASON"] = reason;
        }
        LLNotificationsUtil::add("FacePasteFailed", notif_args);
        return;
    }

    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();

    LLPanelFacePasteTexFunctor paste_func(this, PASTE_COLOR);
    selected_objects->applyToTEs(&paste_func);

    LLPanelFaceUpdateFunctor sendfunc(false);
    selected_objects->applyToObjects(&sendfunc);
}

void LLPanelFace::onPasteColor(LLViewerObject* objectp, S32 te)
{
    LLSD te_data;
    LLSD &clipboard = mClipboardParams["color"]; // array
    if ((clipboard.size() == 1) && mClipboardParams["color_all_tes"].asBoolean())
    {
        te_data = *(clipboard.beginArray());
    }
    else if (clipboard[te])
    {
        te_data = clipboard[te];
    }
    else
    {
        return;
    }

    LLTextureEntry* tep = objectp->getTE(te);
    if (tep)
    {
        if (te_data.has("te"))
        {
            // Color / Alpha
            if (te_data["te"].has("colors"))
            {
                LLColor4 color = tep->getColor();

                LLColor4 clip_color;
                clip_color.setValue(te_data["te"]["colors"]);

                // Color
                color.mV[VRED] = clip_color.mV[VRED];
                color.mV[VGREEN] = clip_color.mV[VGREEN];
                color.mV[VBLUE] = clip_color.mV[VBLUE];

                // Alpha
                color.mV[VALPHA] = clip_color.mV[VALPHA];

                objectp->setTEColor(te, color);
            }

            // Color/fullbright
            if (te_data["te"].has("fullbright"))
            {
                objectp->setTEFullbright(te, te_data["te"]["fullbright"].asInteger());
            }

            // Glow
            if (te_data["te"].has("glow"))
            {
                objectp->setTEGlow(te, (F32)te_data["te"]["glow"].asReal());
            }
        }
    }
}

void LLPanelFace::onCopyTexture()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        return;
    }

    if (mClipboardParams.has("texture"))
    {
        mClipboardParams["texture"].clear();
    }
    else
    {
        mClipboardParams["texture"] = LLSD::emptyArray();
    }

    std::map<LLUUID, LLUUID> asset_item_map;

    // a way to resolve situations where source and target have different amount of faces
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    mClipboardParams["texture_all_tes"] = (num_tes != 1) || (LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool());
    for (S32 te = 0; te < num_tes; ++te)
    {
        if (node->isTESelected(te))
        {
            LLTextureEntry* tep = objectp->getTE(te);
            if (tep)
            {
                LLSD te_data;

                // asLLSD() includes media
                te_data["te"] = tep->asLLSD();
                te_data["te"]["shiny"] = tep->getShiny();
                te_data["te"]["bumpmap"] = tep->getBumpmap();
                te_data["te"]["bumpshiny"] = tep->getBumpShiny();
                te_data["te"]["bumpfullbright"] = tep->getBumpShinyFullbright();
                te_data["te"]["pbr"] = objectp->getRenderMaterialID(te);
                if (tep->getGLTFMaterialOverride() != nullptr)
                {
                    te_data["te"]["pbr_override"] = tep->getGLTFMaterialOverride()->asJSON();
                }

                if (te_data["te"].has("imageid"))
                {
                    LLUUID item_id;
                    LLUUID id = te_data["te"]["imageid"].asUUID();
                    bool from_library = get_is_predefined_texture(id);
                    bool full_perm = from_library;

                    if (!full_perm
                        && objectp->permCopy()
                        && objectp->permTransfer()
                        && objectp->permModify())
                    {
                        // If agent created this object and nothing is limiting permissions, mark as full perm
                        // If agent was granted permission to edit objects owned and created by somebody else, mark full perm
                        // This check is not perfect since we can't figure out whom textures belong to so this ended up restrictive
                        std::string creator_app_link;
                        LLUUID creator_id;
                        LLSelectMgr::getInstance()->selectGetCreator(creator_id, creator_app_link);
                        full_perm = objectp->mOwnerID == creator_id;
                    }

                    if (id.notNull() && !full_perm)
                    {
                        std::map<LLUUID, LLUUID>::iterator iter = asset_item_map.find(id);
                        if (iter != asset_item_map.end())
                        {
                            item_id = iter->second;
                        }
                        else
                        {
                            // What this does is simply searches inventory for item with same asset id,
                            // as result it is Hightly unreliable, leaves little control to user, borderline hack
                            // but there are little options to preserve permissions - multiple inventory
                            // items might reference same asset and inventory search is expensive.
                            bool no_transfer = false;
                            if (objectp->getInventoryItemByAsset(id))
                            {
                                no_transfer = !objectp->getInventoryItemByAsset(id)->getIsFullPerm();
                            }
                            item_id = get_copy_free_item_by_asset_id(id, no_transfer);
                            // record value to avoid repeating inventory search when possible
                            asset_item_map[id] = item_id;
                        }
                    }

                    if (item_id.notNull() && gInventory.isObjectDescendentOf(item_id, gInventory.getLibraryRootFolderID()))
                    {
                        full_perm = true;
                        from_library = true;
                    }

                    {
                        te_data["te"]["itemfullperm"] = full_perm;
                        te_data["te"]["fromlibrary"] = from_library; 

                        // If full permission object, texture is free to copy,
                        // but otherwise we need to check inventory and extract permissions
                        //
                        // Normally we care only about restrictions for current user and objects
                        // don't inherit any 'next owner' permissions from texture, so there is
                        // no need to record item id if full_perm==true
                        if (!full_perm && !from_library && item_id.notNull())
                        {
                            LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                            if (itemp)
                            {
                                LLPermissions item_permissions = itemp->getPermissions();
                                if (item_permissions.allowOperationBy(PERM_COPY,
                                    gAgent.getID(),
                                    gAgent.getGroupID()))
                                {
                                    te_data["te"]["imageitemid"] = item_id;
                                    te_data["te"]["itemfullperm"] = itemp->getIsFullPerm();
                                    if (!itemp->isFinished())
                                    {
                                        // needed for dropTextureAllFaces
                                        LLInventoryModelBackgroundFetch::instance().start(item_id, false);
                                    }
                                }
                            }
                        }
                    }
                }

                LLMaterialPtr material_ptr = tep->getMaterialParams();
                if (!material_ptr.isNull())
                {
                    LLSD mat_data;

                    mat_data["NormMap"] = material_ptr->getNormalID();
                    mat_data["SpecMap"] = material_ptr->getSpecularID();

                    mat_data["NormRepX"] = material_ptr->getNormalRepeatX();
                    mat_data["NormRepY"] = material_ptr->getNormalRepeatY();
                    mat_data["NormOffX"] = material_ptr->getNormalOffsetX();
                    mat_data["NormOffY"] = material_ptr->getNormalOffsetY();
                    mat_data["NormRot"] = material_ptr->getNormalRotation();

                    mat_data["SpecRepX"] = material_ptr->getSpecularRepeatX();
                    mat_data["SpecRepY"] = material_ptr->getSpecularRepeatY();
                    mat_data["SpecOffX"] = material_ptr->getSpecularOffsetX();
                    mat_data["SpecOffY"] = material_ptr->getSpecularOffsetY();
                    mat_data["SpecRot"] = material_ptr->getSpecularRotation();

                    mat_data["SpecColor"] = material_ptr->getSpecularLightColor().getValue();
                    mat_data["SpecExp"] = material_ptr->getSpecularLightExponent();
                    mat_data["EnvIntensity"] = material_ptr->getEnvironmentIntensity();
                    mat_data["AlphaMaskCutoff"] = material_ptr->getAlphaMaskCutoff();
                    mat_data["DiffuseAlphaMode"] = material_ptr->getDiffuseAlphaMode();

                    // Replace no-copy textures, destination texture will get used instead if available
                    if (mat_data.has("NormMap"))
                    {
                        LLUUID id = mat_data["NormMap"].asUUID();
                        if (id.notNull() && !get_can_copy_texture(id))
                        {
                            mat_data["NormMap"] = LLUUID(gSavedSettings.getString("DefaultObjectTexture"));
                            mat_data["NormMapNoCopy"] = true;
                        }

                    }
                    if (mat_data.has("SpecMap"))
                    {
                        LLUUID id = mat_data["SpecMap"].asUUID();
                        if (id.notNull() && !get_can_copy_texture(id))
                        {
                            mat_data["SpecMap"] = LLUUID(gSavedSettings.getString("DefaultObjectTexture"));
                            mat_data["SpecMapNoCopy"] = true;
                        }

                    }

                    te_data["material"] = mat_data;
                }

                mClipboardParams["texture"].append(te_data);
            }
        }
    }
}

void LLPanelFace::onPasteTexture()
{
    if (!mClipboardParams.has("texture"))
    {
        return;
    }

    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
    if (!objectp || !node
        || objectp->getPCode() != LL_PCODE_VOLUME
        || !objectp->permModify()
        || objectp->isPermanentEnforced()
        || selected_count > 1)
    {
        // not supposed to happen
        LL_WARNS() << "Failed to paste texture due to missing or wrong selection" << LL_ENDL;
        return;
    }

    bool face_selection_mode = LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool();
    LLSD &clipboard = mClipboardParams["texture"]; // array
    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
    S32 compare_tes = num_tes;

    if (face_selection_mode)
    {
        compare_tes = 0;
        for (S32 te = 0; te < num_tes; ++te)
        {
            if (node->isTESelected(te))
            {
                compare_tes++;
            }
        }
    }

    // we can copy if single face was copied in edit face mode or if face count matches
    if (!((clipboard.size() == 1) && mClipboardParams["texture_all_tes"].asBoolean()) 
        && compare_tes != clipboard.size())
    {
        LLSD notif_args;
        if (face_selection_mode)
        {
            static std::string reason = getString("paste_error_face_selection_mismatch");
            notif_args["REASON"] = reason;
        }
        else
        {
            static std::string reason = getString("paste_error_object_face_count_mismatch");
            notif_args["REASON"] = reason;
        }
        LLNotificationsUtil::add("FacePasteFailed", notif_args);
        return;
    }

    bool full_perm_object = true;
    LLSD::array_const_iterator iter = clipboard.beginArray();
    LLSD::array_const_iterator end = clipboard.endArray();
    for (; iter != end; ++iter)
    {
        const LLSD& te_data = *iter;
        if (te_data.has("te") && te_data["te"].has("imageid"))
        {
            bool full_perm = te_data["te"].has("itemfullperm") && te_data["te"]["itemfullperm"].asBoolean();
            full_perm_object &= full_perm;
            if (!full_perm)
            {
                if (te_data["te"].has("imageitemid"))
                {
                    LLUUID item_id = te_data["te"]["imageitemid"].asUUID();
                    if (item_id.notNull())
                    {
                        LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                        if (!itemp)
                        {
                            // image might be in object's inventory, but it can be not up to date
                            LLSD notif_args;
                            static std::string reason = getString("paste_error_inventory_not_found");
                            notif_args["REASON"] = reason;
                            LLNotificationsUtil::add("FacePasteFailed", notif_args);
                            return;
                        }
                    }
                }
                else
                {
                    // Item was not found on 'copy' stage
                    // Since this happened at copy, might be better to either show this
                    // at copy stage or to drop clipboard here
                    LLSD notif_args;
                    static std::string reason = getString("paste_error_inventory_not_found");
                    notif_args["REASON"] = reason;
                    LLNotificationsUtil::add("FacePasteFailed", notif_args);
                    return;
                }
            }
        }
    }

    if (!full_perm_object)
    {
        LLNotificationsUtil::add("FacePasteTexturePermissions");
    }

    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();

    LLPanelFacePasteTexFunctor paste_func(this, PASTE_TEXTURE);
    selected_objects->applyToTEs(&paste_func);

    LLPanelFaceUpdateFunctor sendfunc(true);
    selected_objects->applyToObjects(&sendfunc);

    LLGLTFMaterialList::flushUpdates();

    LLPanelFaceNavigateHomeFunctor navigate_home_func;
    selected_objects->applyToTEs(&navigate_home_func);
}

void LLPanelFace::onPasteTexture(LLViewerObject* objectp, S32 te)
{
    LLSD te_data;
    LLSD &clipboard = mClipboardParams["texture"]; // array
    if ((clipboard.size() == 1) && mClipboardParams["texture_all_tes"].asBoolean())
    {
        te_data = *(clipboard.beginArray());
    }
    else if (clipboard[te])
    {
        te_data = clipboard[te];
    }
    else
    {
        return;
    }

    LLTextureEntry* tep = objectp->getTE(te);
    if (tep)
    {
        if (te_data.has("te"))
        {
            // Texture
            bool full_perm = te_data["te"].has("itemfullperm") && te_data["te"]["itemfullperm"].asBoolean();
            bool from_library = te_data["te"].has("fromlibrary") && te_data["te"]["fromlibrary"].asBoolean();
            if (te_data["te"].has("imageid"))
            {
                const LLUUID& imageid = te_data["te"]["imageid"].asUUID(); //texture or asset id
                LLViewerInventoryItem* itemp_res = NULL;

                if (te_data["te"].has("imageitemid"))
                {
                    LLUUID item_id = te_data["te"]["imageitemid"].asUUID();
                    if (item_id.notNull())
                    {
                        LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
                        if (itemp && itemp->isFinished())
                        {
                            // dropTextureAllFaces will fail if incomplete
                            itemp_res = itemp;
                        }
                        else
                        {
                            // Theoretically shouldn't happend, but if it does happen, we
                            // might need to add a notification to user that paste will fail
                            // since inventory isn't fully loaded
                            LL_WARNS() << "Item " << item_id << " is incomplete, paste might fail silently." << LL_ENDL;
                        }
                    }
                }
                // for case when item got removed from inventory after we pressed 'copy'
                // or texture got pasted into previous object
                if (!itemp_res && !full_perm)
                {
                    // Due to checks for imageitemid in LLPanelFace::onPasteTexture() this should no longer be reachable.
                    LL_INFOS() << "Item " << te_data["te"]["imageitemid"].asUUID() << " no longer in inventory." << LL_ENDL;
                    // Todo: fix this, we are often searching same texture multiple times (equal to number of faces)
                    // Perhaps just mPanelFace->onPasteTexture(objectp, te, &asset_to_item_id_map); ? Not pretty, but will work
                    LLViewerInventoryCategory::cat_array_t cats;
                    LLViewerInventoryItem::item_array_t items;
                    LLAssetIDMatches asset_id_matches(imageid);
                    gInventory.collectDescendentsIf(LLUUID::null,
                        cats,
                        items,
                        LLInventoryModel::INCLUDE_TRASH,
                        asset_id_matches);

                    // Extremely unreliable and perfomance unfriendly.
                    // But we need this to check permissions and it is how texture control finds items
                    for (S32 i = 0; i < items.size(); i++)
                    {
                        LLViewerInventoryItem* itemp = items[i];
                        if (itemp && itemp->isFinished())
                        {
                            // dropTextureAllFaces will fail if incomplete
                            LLPermissions item_permissions = itemp->getPermissions();
                            if (item_permissions.allowOperationBy(PERM_COPY,
                                gAgent.getID(),
                                gAgent.getGroupID()))
                            {
                                itemp_res = itemp;
                                break; // first match
                            }
                        }
                    }
                }

                if (itemp_res)
                {
                    if (te == -1) // all faces
                    {
                        LLToolDragAndDrop::dropTextureAllFaces(objectp,
                            itemp_res,
                            from_library ? LLToolDragAndDrop::SOURCE_LIBRARY : LLToolDragAndDrop::SOURCE_AGENT,
                            LLUUID::null);
                    }
                    else // one face
                    {
                        LLToolDragAndDrop::dropTextureOneFace(objectp,
                            te,
                            itemp_res,
                            from_library ? LLToolDragAndDrop::SOURCE_LIBRARY : LLToolDragAndDrop::SOURCE_AGENT,
                            LLUUID::null,
                            0);
                    }
                }
                // not an inventory item or no complete items
                else if (full_perm)
                {
                    // Either library, local or existed as fullperm when user made a copy
                    LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(imageid, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
                    objectp->setTEImage(U8(te), image);
                }
            }

            if (te_data["te"].has("bumpmap"))
            {
                objectp->setTEBumpmap(te, (U8)te_data["te"]["bumpmap"].asInteger());
            }
            if (te_data["te"].has("bumpshiny"))
            {
                objectp->setTEBumpShiny(te, (U8)te_data["te"]["bumpshiny"].asInteger());
            }
            if (te_data["te"].has("bumpfullbright"))
            {
                objectp->setTEBumpShinyFullbright(te, (U8)te_data["te"]["bumpfullbright"].asInteger());
            }
            // PBR/GLTF
            if (te_data["te"].has("pbr"))
            {
                objectp->setRenderMaterialID(te, te_data["te"]["pbr"].asUUID(), false /*managing our own update*/);
                tep->setGLTFRenderMaterial(nullptr);
                tep->setGLTFMaterialOverride(nullptr);

                LLSD override_data;
                override_data["object_id"] = objectp->getID();
                override_data["side"] = te;
                if (te_data["te"].has("pbr_override"))
                {
                    override_data["gltf_json"] = te_data["te"]["pbr_override"];
                }
                else
                {
                    override_data["gltf_json"] = "";
                }

                override_data["asset_id"] = te_data["te"]["pbr"].asUUID();

                LLGLTFMaterialList::queueUpdate(override_data);
            }
            else
            {
                objectp->setRenderMaterialID(te, LLUUID::null, false /*send in bulk later*/ );
                tep->setGLTFRenderMaterial(nullptr);
                tep->setGLTFMaterialOverride(nullptr);

                // blank out most override data on the server
                LLGLTFMaterialList::queueApply(objectp, te, LLUUID::null);
            }

            // Texture map
            if (te_data["te"].has("scales") && te_data["te"].has("scalet"))
            {
                objectp->setTEScale(te, (F32)te_data["te"]["scales"].asReal(), (F32)te_data["te"]["scalet"].asReal());
            }
            if (te_data["te"].has("offsets") && te_data["te"].has("offsett"))
            {
                objectp->setTEOffset(te, (F32)te_data["te"]["offsets"].asReal(), (F32)te_data["te"]["offsett"].asReal());
            }
            if (te_data["te"].has("imagerot"))
            {
                objectp->setTERotation(te, (F32)te_data["te"]["imagerot"].asReal());
            }

            // Media
            if (te_data["te"].has("media_flags"))
            {
                U8 media_flags = te_data["te"]["media_flags"].asInteger();
                objectp->setTEMediaFlags(te, media_flags);
                LLVOVolume *vo = dynamic_cast<LLVOVolume*>(objectp);
                if (vo && te_data["te"].has(LLTextureEntry::TEXTURE_MEDIA_DATA_KEY))
                {
                    vo->syncMediaData(te, te_data["te"][LLTextureEntry::TEXTURE_MEDIA_DATA_KEY], true/*merge*/, true/*ignore_agent*/);
                }
            }
            else
            {
                // Keep media flags on destination unchanged
            }
        }

        if (te_data.has("material"))
        {
            LLUUID object_id = objectp->getID();

            LLSelectedTEMaterial::setAlphaMaskCutoff(this, (U8)te_data["material"]["SpecRot"].asInteger(), te, object_id);

            // Normal
            // Replace placeholders with target's
            if (te_data["material"].has("NormMapNoCopy"))
            {
                LLMaterialPtr material = tep->getMaterialParams();
                if (material.notNull())
                {
                    LLUUID id = material->getNormalID();
                    if (id.notNull())
                    {
                        te_data["material"]["NormMap"] = id;
                    }
                }
            }
            LLSelectedTEMaterial::setNormalID(this, te_data["material"]["NormMap"].asUUID(), te, object_id);
            LLSelectedTEMaterial::setNormalRepeatX(this, (F32)te_data["material"]["NormRepX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalRepeatY(this, (F32)te_data["material"]["NormRepY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalOffsetX(this, (F32)te_data["material"]["NormOffX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalOffsetY(this, (F32)te_data["material"]["NormOffY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setNormalRotation(this, (F32)te_data["material"]["NormRot"].asReal(), te, object_id);

            // Specular
                // Replace placeholders with target's
            if (te_data["material"].has("SpecMapNoCopy"))
            {
                LLMaterialPtr material = tep->getMaterialParams();
                if (material.notNull())
                {
                    LLUUID id = material->getSpecularID();
                    if (id.notNull())
                    {
                        te_data["material"]["SpecMap"] = id;
                    }
                }
            }
            LLSelectedTEMaterial::setSpecularID(this, te_data["material"]["SpecMap"].asUUID(), te, object_id);
            LLSelectedTEMaterial::setSpecularRepeatX(this, (F32)te_data["material"]["SpecRepX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularRepeatY(this, (F32)te_data["material"]["SpecRepY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularOffsetX(this, (F32)te_data["material"]["SpecOffX"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularOffsetY(this, (F32)te_data["material"]["SpecOffY"].asReal(), te, object_id);
            LLSelectedTEMaterial::setSpecularRotation(this, (F32)te_data["material"]["SpecRot"].asReal(), te, object_id);
            LLColor4 spec_color(te_data["material"]["SpecColor"]);
            LLSelectedTEMaterial::setSpecularLightColor(this, spec_color, te);
            LLSelectedTEMaterial::setSpecularLightExponent(this, (U8)te_data["material"]["SpecExp"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setEnvironmentIntensity(this, (U8)te_data["material"]["EnvIntensity"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setDiffuseAlphaMode(this, (U8)te_data["material"]["SpecRot"].asInteger(), te, object_id);
            if (te_data.has("te") && te_data["te"].has("shiny"))
            {
                objectp->setTEShiny(te, (U8)te_data["te"]["shiny"].asInteger());
            }
        }
    }
}

void LLPanelFace::menuDoToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste
    if (command == "color_paste")
    {
        onPasteColor();
    }
    else if (command == "texture_paste")
    {
        onPasteTexture();
    }
    // copy
    else if (command == "color_copy")
    {
        onCopyColor();
    }
    else if (command == "texture_copy")
    {
        onCopyTexture();
    }
}

bool LLPanelFace::menuEnableItem(const LLSD& userdata)
{
    std::string command = userdata.asString();

    // paste options
    if (command == "color_paste")
    {
        return mClipboardParams.has("color");
    }
    else if (command == "texture_paste")
    {
        return mClipboardParams.has("texture");
    }
    return false;
}


// static
void LLPanelFace::onCommitPlanarAlign(LLUICtrl* ctrl, void* userdata)
{
	LLPanelFace* self = (LLPanelFace*) userdata;
	self->getState();
	self->sendTextureInfo();
}

void LLPanelFace::updateGLTFTextureTransform(float value, U32 pbr_type, std::function<void(LLGLTFMaterial::TextureTransform*)> edit)
{
    U32 texture_info_start;
    U32 texture_info_end;
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        texture_info_start = 0;
        texture_info_end = LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
    }
    else
    {
        texture_info_start = texture_info_from_pbrtype(pbr_type);
        texture_info_end = texture_info_start + 1;
    }
    updateSelectedGLTFMaterials([&](LLGLTFMaterial* new_override)
    {
        for (U32 ti = texture_info_start; ti < texture_info_end; ++ti)
        {
            LLGLTFMaterial::TextureTransform& new_transform = new_override->mTextureTransform[(LLGLTFMaterial::TextureInfo)ti];
            edit(&new_transform);
        }
    });

    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    if (node)
    {
        LLViewerObject* object = node->getObject();
        sMaterialOverrideSelection.setObjectUpdatePending(object->getID(), node->getLastSelectedTE());
    }
}

void LLPanelFace::setMaterialOverridesFromSelection()
{
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    const LLGLTFMaterial::TextureInfo texture_info = texture_info_from_pbrtype(pbr_type);
    if (texture_info == LLGLTFMaterial::TextureInfo::GLTF_TEXTURE_INFO_COUNT)
    {
        return;
    }

    LLGLTFMaterial::TextureTransform transform;
    bool scale_u_same = true;
    bool scale_v_same = true;
    bool rotation_same = true;
    bool offset_u_same = true;
    bool offset_v_same = true;

    readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
    {
        return mat ? mat->mTextureTransform[texture_info].mScale[VX] : 0.f;
    }, transform.mScale[VX], scale_u_same, true, 1e-3f);
    readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
    {
        return mat ? mat->mTextureTransform[texture_info].mScale[VY] : 0.f;
    }, transform.mScale[VY], scale_v_same, true, 1e-3f);
    readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
    {
        return mat ? mat->mTextureTransform[texture_info].mRotation : 0.f;
    }, transform.mRotation, rotation_same, true, 1e-3f);
    readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
    {
        return mat ? mat->mTextureTransform[texture_info].mOffset[VX] : 0.f;
    }, transform.mOffset[VX], offset_u_same, true, 1e-3f);
    readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
    {
        return mat ? mat->mTextureTransform[texture_info].mOffset[VY] : 0.f;
    }, transform.mOffset[VY], offset_v_same, true, 1e-3f);

    LLUICtrl* gltfCtrlTextureScaleU = getChild<LLUICtrl>("gltfTextureScaleU");
    LLUICtrl* gltfCtrlTextureScaleV = getChild<LLUICtrl>("gltfTextureScaleV");
    LLUICtrl* gltfCtrlTextureRotation = getChild<LLUICtrl>("gltfTextureRotation");
    LLUICtrl* gltfCtrlTextureOffsetU = getChild<LLUICtrl>("gltfTextureOffsetU");
    LLUICtrl* gltfCtrlTextureOffsetV = getChild<LLUICtrl>("gltfTextureOffsetV");

    gltfCtrlTextureScaleU->setValue(transform.mScale[VX]);
    gltfCtrlTextureScaleV->setValue(transform.mScale[VY]);
    gltfCtrlTextureRotation->setValue(transform.mRotation * RAD_TO_DEG);
    gltfCtrlTextureOffsetU->setValue(transform.mOffset[VX]);
    gltfCtrlTextureOffsetV->setValue(transform.mOffset[VY]);

    gltfCtrlTextureScaleU->setTentative(!scale_u_same);
    gltfCtrlTextureScaleV->setTentative(!scale_v_same);
    gltfCtrlTextureRotation->setTentative(!rotation_same);
    gltfCtrlTextureOffsetU->setTentative(!offset_u_same);
    gltfCtrlTextureOffsetV->setTentative(!offset_v_same);
}

void LLPanelFace::Selection::connect()
{
    if (!mSelectConnection.connected())
    {
        mSelectConnection = LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLPanelFace::Selection::onSelectionChanged, this));
    }
}

bool LLPanelFace::Selection::update()
{
    const bool changed = mChanged || compareSelection();
    mChanged = false;
    return changed;
}

void LLPanelFace::Selection::setObjectUpdatePending(const LLUUID &object_id, S32 side)
{
    mPendingObjectID = object_id;
    mPendingSide = side;
}

void LLPanelFace::Selection::onSelectedObjectUpdated(const LLUUID& object_id, S32 side)
{
    if (object_id == mSelectedObjectID && side == mSelectedSide)
    {
        mChanged = true;
    }
}

bool LLPanelFace::Selection::compareSelection()
{
    if (!mNeedsSelectionCheck)
    {
        return false;
    }
    mNeedsSelectionCheck = false;

    const S32 old_object_count = mSelectedObjectCount;
    const LLUUID old_object_id = mSelectedObjectID;
    const S32 old_side = mSelectedSide;

    LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
    LLSelectNode* node = selection->getFirstNode();
    if (node)
    {
        LLViewerObject* object = node->getObject();
        mSelectedObjectCount = selection->getObjectCount();
        mSelectedObjectID = object->getID();
        mSelectedSide = node->getLastSelectedTE();
    }
    else
    {
        mSelectedObjectCount = 0;
        mSelectedObjectID = LLUUID::null;
        mSelectedSide = -1;
    }

    const bool selection_changed = old_object_count != mSelectedObjectCount || old_object_id != mSelectedObjectID || old_side != mSelectedSide;
    mChanged = mChanged || selection_changed;
    return selection_changed;
}

void LLPanelFace::onCommitGLTFTextureScaleU(LLUICtrl* ctrl)
{
    const float value = ctrl->getValue().asReal();
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    updateGLTFTextureTransform(value, pbr_type, [&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mScale.mV[VX] = value;
    });
}

void LLPanelFace::onCommitGLTFTextureScaleV(LLUICtrl* ctrl)
{
    const float value = ctrl->getValue().asReal();
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    updateGLTFTextureTransform(value, pbr_type, [&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mScale.mV[VY] = value;
    });
}

void LLPanelFace::onCommitGLTFRotation(LLUICtrl* ctrl)
{
    const float value = ctrl->getValue().asReal() * DEG_TO_RAD;
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    updateGLTFTextureTransform(value, pbr_type, [&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mRotation = value;
    });
}

void LLPanelFace::onCommitGLTFTextureOffsetU(LLUICtrl* ctrl)
{
    const float value = ctrl->getValue().asReal();
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    updateGLTFTextureTransform(value, pbr_type, [&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mOffset.mV[VX] = value;
    });
}

void LLPanelFace::onCommitGLTFTextureOffsetV(LLUICtrl* ctrl)
{
    const float value = ctrl->getValue().asReal();
    const U32 pbr_type = findChild<LLRadioGroup>("radio_pbr_type")->getSelectedIndex();
    updateGLTFTextureTransform(value, pbr_type, [&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mOffset.mV[VY] = value;
    });
}

void LLPanelFace::onTextureSelectionChanged(LLInventoryItem* itemp)
{
	LL_DEBUGS("Materials") << "item asset " << itemp->getAssetUUID() << LL_ENDL;
	LLRadioGroup* radio_mat_type = findChild<LLRadioGroup>("radio_material_type");
	if(!radio_mat_type)
	{
	    return;
	}
	U32 mattype = radio_mat_type->getSelectedIndex();
	std::string which_control="texture control";
	switch (mattype)
	{
		case MATTYPE_SPECULAR:
			which_control = "shinytexture control";
			break;
		case MATTYPE_NORMAL:
			which_control = "bumpytexture control";
			break;
		// no default needed
	}
	LL_DEBUGS("Materials") << "control " << which_control << LL_ENDL;
	LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(which_control);
	if (texture_ctrl)
	{
		LLUUID obj_owner_id;
		std::string obj_owner_name;
		LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

		LLSaleInfo sale_info;
		LLSelectMgr::instance().selectGetSaleInfo(sale_info);

		bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this texture?
		bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this texture?
		bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply texture belong to the agent?
		bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply texture not for sale?

		if (can_copy && can_transfer)
		{
			texture_ctrl->setCanApply(true, true);
			return;
		}

		// if texture has (no-transfer) attribute it can be applied only for object which we own and is not for sale
		texture_ctrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

		if (gSavedSettings.getBOOL("TextureLivePreview"))
		{
			LLNotificationsUtil::add("LivePreviewUnavailable");
		}
	}
}

void LLPanelFace::onPbrSelectionChanged(LLInventoryItem* itemp)
{
    LLTextureCtrl* pbr_ctrl = findChild<LLTextureCtrl>("pbr_control");
    if (pbr_ctrl)
    {
        LLUUID obj_owner_id;
        std::string obj_owner_name;
        LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

        LLSaleInfo sale_info;
        LLSelectMgr::instance().selectGetSaleInfo(sale_info);

        bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this texture?
        bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this texture?
        bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply texture belong to the agent?
        bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply texture not for sale?

        if (can_copy && can_transfer)
        {
            pbr_ctrl->setCanApply(true, true);
            return;
        }

        // if texture has (no-transfer) attribute it can be applied only for object which we own and is not for sale
        pbr_ctrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

        if (gSavedSettings.getBOOL("TextureLivePreview"))
        {
            LLNotificationsUtil::add("LivePreviewUnavailable");
        }
    }
}

bool LLPanelFace::isIdenticalPlanarTexgen()
{
	LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
	bool identical_texgen = false;
	LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
	return (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));
}

void LLPanelFace::LLSelectedTE::getFace(LLFace*& face_to_return, bool& identical_face)
{		
	struct LLSelectedTEGetFace : public LLSelectedTEGetFunctor<LLFace *>
	{
		LLFace* get(LLViewerObject* object, S32 te)
		{
			return (object->mDrawable) ? object->mDrawable->getFace(te): NULL;
		}
	} get_te_face_func;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_te_face_func, face_to_return, false, (LLFace*)nullptr);
}

void LLPanelFace::LLSelectedTE::getImageFormat(LLGLenum& image_format_to_return, bool& identical_face)
{
	LLGLenum image_format;
	struct LLSelectedTEGetImageFormat : public LLSelectedTEGetFunctor<LLGLenum>
	{
		LLGLenum get(LLViewerObject* object, S32 te_index)
		{
			LLViewerTexture* image = object->getTEImage(te_index);
			return image ? image->getPrimaryFormat() : GL_RGB;
		}
	} get_glenum;
	identical_face = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&get_glenum, image_format);
	image_format_to_return = image_format;
}

void LLPanelFace::LLSelectedTE::getTexId(LLUUID& id, bool& identical)
{		
	struct LLSelectedTEGetTexId : public LLSelectedTEGetFunctor<LLUUID>
	{
		LLUUID get(LLViewerObject* object, S32 te_index)
		{
			LLTextureEntry *te = object->getTE(te_index);
			if (te)
			{
				if ((te->getID() == IMG_USE_BAKED_EYES) || (te->getID() == IMG_USE_BAKED_HAIR) || (te->getID() == IMG_USE_BAKED_HEAD) || (te->getID() == IMG_USE_BAKED_LOWER) || (te->getID() == IMG_USE_BAKED_SKIRT) || (te->getID() == IMG_USE_BAKED_UPPER)
					|| (te->getID() == IMG_USE_BAKED_LEFTARM) || (te->getID() == IMG_USE_BAKED_LEFTLEG) || (te->getID() == IMG_USE_BAKED_AUX1) || (te->getID() == IMG_USE_BAKED_AUX2) || (te->getID() == IMG_USE_BAKED_AUX3))
				{
					return te->getID();
				}
			}

			LLUUID id;
			LLViewerTexture* image = object->getTEImage(te_index);
			if (image)
			{
				id = image->getID();
			}

			if (!id.isNull() && LLViewerMedia::getInstance()->textureHasMedia(id))
			{
				if (te)
				{
					LLViewerTexture* tex = te->getID().notNull() ? gTextureList.findImage(te->getID(), TEX_LIST_STANDARD) : NULL;
					if(!tex)
					{
						tex = LLViewerFetchedTexture::sDefaultImagep;
					}
					if (tex)
					{
						id = tex->getID();
					}
				}
			}
			return id;
		}
	} func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, id );
}

void LLPanelFace::LLSelectedTE::getPbrMaterialId(LLUUID& id, bool& identical)
{
    struct LLSelectedTEGetmatId : public LLSelectedTEGetFunctor<LLUUID>
    {
        LLUUID get(LLViewerObject* object, S32 te_index)
        {
            return object->getRenderMaterialID(te_index);
        }
    } func;
    identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue(&func, id);
}

void LLPanelFace::LLSelectedTEMaterial::getCurrent(LLMaterialPtr& material_ptr, bool& identical_material)
{
	struct MaterialFunctor : public LLSelectedTEGetFunctor<LLMaterialPtr>
	{
		LLMaterialPtr get(LLViewerObject* object, S32 te_index)
		{
			return object->getTE(te_index)->getMaterialParams();
		}
	} func;
	identical_material = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &func, material_ptr);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxSpecularRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxSpecRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getSpecularRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}					
			return llmax(repeats_s, repeats_t);
		}

	} max_spec_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_spec_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getMaxNormalRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxNormRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			LLMaterial* mat = object->getTE(face)->getMaterialParams().get();
			U32 s_axis = VX;
			U32 t_axis = VY;
			F32 repeats_s = 1.0f;
			F32 repeats_t = 1.0f;
			if (mat)
			{
				mat->getNormalRepeat(repeats_s, repeats_t);
				repeats_s /= object->getScale().mV[s_axis];
				repeats_t /= object->getScale().mV[t_axis];
			}					
			return llmax(repeats_s, repeats_t);
		}

	} max_norm_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_norm_repeats_func, repeats);
}

void LLPanelFace::LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(U8& diffuse_alpha_mode, bool& identical, bool diffuse_texture_has_alpha)
{
	struct LLSelectedTEGetDiffuseAlphaMode : public LLSelectedTEGetFunctor<U8>
	{
		LLSelectedTEGetDiffuseAlphaMode() : _isAlpha(false) {}
		LLSelectedTEGetDiffuseAlphaMode(bool diffuse_texture_has_alpha) : _isAlpha(diffuse_texture_has_alpha) {}
		virtual ~LLSelectedTEGetDiffuseAlphaMode() {}

		U8 get(LLViewerObject* object, S32 face)
		{
			U8 diffuse_mode = _isAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

			LLTextureEntry* tep = object->getTE(face);
			if (tep)
			{
				LLMaterial* mat = tep->getMaterialParams().get();
				if (mat)
				{
					diffuse_mode = mat->getDiffuseAlphaMode();
				}
			}
			
			return diffuse_mode;
		}
		bool _isAlpha; // whether or not the diffuse texture selected contains alpha information
	} get_diff_mode(diffuse_texture_has_alpha);
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &get_diff_mode, diffuse_alpha_mode);
}

void LLPanelFace::LLSelectedTE::getObjectScaleS(F32& scale_s, bool& identical)
{	
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[s_axis];
		}

	} scale_s_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &scale_s_func, scale_s );
}

void LLPanelFace::LLSelectedTE::getObjectScaleT(F32& scale_t, bool& identical)
{	
	struct LLSelectedTEGetObjectScaleS : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			return object->getScale().mV[t_axis];
		}

	} scale_t_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &scale_t_func, scale_t );
}

void LLPanelFace::LLSelectedTE::getMaxDiffuseRepeats(F32& repeats, bool& identical)
{
	struct LLSelectedTEGetMaxDiffuseRepeats : public LLSelectedTEGetFunctor<F32>
	{
		F32 get(LLViewerObject* object, S32 face)
		{
			U32 s_axis = VX;
			U32 t_axis = VY;
			LLPrimitive::getTESTAxes(face, &s_axis, &t_axis);
			F32 repeats_s = object->getTE(face)->mScaleS / object->getScale().mV[s_axis];
			F32 repeats_t = object->getTE(face)->mScaleT / object->getScale().mV[t_axis];
			return llmax(repeats_s, repeats_t);
		}

	} max_diff_repeats_func;
	identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &max_diff_repeats_func, repeats );
}

