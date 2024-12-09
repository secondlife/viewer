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
#include "llfloatertools.h"
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
#include "llvoinventorylistener.h"
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
const S32 MATMEDIA_MATERIAL = 0;    // Material
const S32 MATMEDIA_PBR = 1;         // PBR
const S32 MATMEDIA_MEDIA = 2;       // Media
const S32 MATTYPE_DIFFUSE = 0;      // Diffuse material texture
const S32 MATTYPE_NORMAL = 1;       // Normal map
const S32 MATTYPE_SPECULAR = 2;     // Specular map
const S32 ALPHAMODE_MASK = 2;       // Alpha masking mode
const S32 BUMPY_TEXTURE = 18;       // use supplied normal map
const S32 SHINY_TEXTURE = 4;        // use supplied specular map
const S32 PBRTYPE_RENDER_MATERIAL_ID = 0;  // Render Material ID
const S32 PBRTYPE_BASE_COLOR = 1;   // PBR Base Color
const S32 PBRTYPE_METALLIC_ROUGHNESS = 2; // PBR Metallic
const S32 PBRTYPE_EMISSIVE = 3;     // PBR Emissive
const S32 PBRTYPE_NORMAL = 4;       // PBR Normal

LLGLTFMaterial::TextureInfo LLPanelFace::getPBRTextureInfo()
{
    // Radiogroup [ "Complete material", "Base color", "Metallic/roughness", "Emissive", "Normal" ]
    S32 radio_group_index = mRadioPbrType->getSelectedIndex();
    switch (radio_group_index)
    {
    case PBRTYPE_BASE_COLOR:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR;
    case PBRTYPE_NORMAL:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL;
    case PBRTYPE_METALLIC_ROUGHNESS:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS;
    case PBRTYPE_EMISSIVE:
        return LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE;
    }
    // The default value is used as a fallback
    return LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;
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
    S32 matmedia_selection = mComboMatMedia->getCurrentIndex();
    switch (matmedia_selection)
    {
    case MATMEDIA_MATERIAL:
        return getMatTextureChannel();
    case MATMEDIA_PBR:
        return getPBRTextureChannel();
    }
    return (LLRender::eTexIndex)0;
}

LLRender::eTexIndex LLPanelFace::getMatTextureChannel()
{
    // Radiogroup [ "Texture (diffuse)", "Bumpiness (normal)", "Shininess (specular)" ]
    S32 radio_group_index = mRadioMaterialType->getSelectedIndex();
    switch (radio_group_index)
    {
    case MATTYPE_DIFFUSE: // "Texture (diffuse)"
        return LLRender::DIFFUSE_MAP;
    case MATTYPE_NORMAL: // "Bumpiness (normal)"
        if (getCurrentNormalMap().notNull())
            return LLRender::NORMAL_MAP;
        break;
    case MATTYPE_SPECULAR: // "Shininess (specular)"
        if (getCurrentNormalMap().notNull())
            return LLRender::SPECULAR_MAP;
        break;
    }
    // The default value is used as a fallback if no required texture is chosen
    return (LLRender::eTexIndex)0;
}

LLRender::eTexIndex LLPanelFace::getPBRTextureChannel()
{
    // Radiogroup [ "Complete material", "Base color", "Metallic/roughness", "Emissive", "Normal" ]
    S32 radio_group_index = mRadioPbrType->getSelectedIndex();
    switch (radio_group_index)
    {
    case PBRTYPE_RENDER_MATERIAL_ID: // "Complete material"
        return LLRender::NUM_TEXTURE_CHANNELS;
    case PBRTYPE_BASE_COLOR: // "Base color"
        return LLRender::BASECOLOR_MAP;
    case PBRTYPE_METALLIC_ROUGHNESS: // "Metallic/roughness"
        return LLRender::METALLIC_ROUGHNESS_MAP;
    case PBRTYPE_EMISSIVE: // "Emissive"
        return LLRender::EMISSIVE_MAP;
    case PBRTYPE_NORMAL: // "Normal"
        return LLRender::GLTF_NORMAL_MAP;
    }
    // The default value is used as a fallback
    return LLRender::NUM_TEXTURE_CHANNELS;
}

LLRender::eTexIndex LLPanelFace::getTextureDropChannel()
{
    if (mComboMatMedia->getCurrentIndex() == MATMEDIA_MATERIAL)
    {
        return getMatTextureChannel();
    }

    return (LLRender::eTexIndex)0;
}

LLGLTFMaterial::TextureInfo LLPanelFace::getPBRDropChannel()
{
    if (mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR)
    {
        return getPBRTextureInfo();
    }

    return (LLGLTFMaterial::TextureInfo)0;
}

// Things the UI provides...
//
LLUUID  LLPanelFace::getCurrentNormalMap()          { return mBumpyTextureCtrl->getImageAssetID(); }
LLUUID  LLPanelFace::getCurrentSpecularMap()        { return mShinyTextureCtrl->getImageAssetID(); }
U32     LLPanelFace::getCurrentShininess()          { return mComboShininess->getCurrentIndex(); }
U32     LLPanelFace::getCurrentBumpiness()          { return mComboBumpiness->getCurrentIndex(); }
U8      LLPanelFace::getCurrentDiffuseAlphaMode()   { return (U8)mComboAlphaMode->getCurrentIndex(); }
U8      LLPanelFace::getCurrentAlphaMaskCutoff()    { return (U8)mMaskCutoff->getValue().asInteger(); }
U8      LLPanelFace::getCurrentEnvIntensity()       { return (U8)mEnvironment->getValue().asInteger(); }
U8      LLPanelFace::getCurrentGlossiness()         { return (U8)mGlossiness->getValue().asInteger(); }
F32     LLPanelFace::getCurrentBumpyRot()           { return (F32)mBumpyRotate->getValue().asReal(); }
F32     LLPanelFace::getCurrentBumpyScaleU()        { return (F32)mBumpyScaleU->getValue().asReal(); }
F32     LLPanelFace::getCurrentBumpyScaleV()        { return (F32)mBumpyScaleV->getValue().asReal(); }
F32     LLPanelFace::getCurrentBumpyOffsetU()       { return (F32)mBumpyOffsetU->getValue().asReal(); }
F32     LLPanelFace::getCurrentBumpyOffsetV()       { return (F32)mBumpyOffsetV->getValue().asReal(); }
F32     LLPanelFace::getCurrentShinyRot()           { return (F32)mShinyRotate->getValue().asReal(); }
F32     LLPanelFace::getCurrentShinyScaleU()        { return (F32)mShinyScaleU->getValue().asReal(); }
F32     LLPanelFace::getCurrentShinyScaleV()        { return (F32)mShinyScaleV->getValue().asReal(); }
F32     LLPanelFace::getCurrentShinyOffsetU()       { return (F32)mShinyOffsetU->getValue().asReal(); }
F32     LLPanelFace::getCurrentShinyOffsetV()       { return (F32)mShinyOffsetV->getValue().asReal(); }

//
// Methods
//

bool LLPanelFace::postBuild()
{
    getChildSetCommitCallback(mComboShininess, "combobox shininess", [&](LLUICtrl*, const LLSD&) { onCommitShiny(); });
    getChildSetCommitCallback(mComboBumpiness, "combobox bumpiness", [&](LLUICtrl*, const LLSD&) { onCommitBump(); });
    getChildSetCommitCallback(mComboAlphaMode, "combobox alphamode", [&](LLUICtrl*, const LLSD&) { onCommitAlphaMode(); });
    getChildSetCommitCallback(mTexScaleU, "TexScaleU", [&](LLUICtrl*, const LLSD&) { onCommitTextureScaleX(); });
    getChildSetCommitCallback(mTexScaleV, "TexScaleV", [&](LLUICtrl*, const LLSD&) { onCommitTextureScaleY(); });
    getChildSetCommitCallback(mTexRotate, "TexRot", [&](LLUICtrl*, const LLSD&) { onCommitTextureRot(); });
    getChildSetCommitCallback(mTexRepeat, "rptctrl", [&](LLUICtrl*, const LLSD&) { onCommitRepeatsPerMeter(); });
    getChildSetCommitCallback(mPlanarAlign, "checkbox planar align", [&](LLUICtrl*, const LLSD&) { onCommitPlanarAlign(); });
    getChildSetCommitCallback(mTexOffsetU, "TexOffsetU", [&](LLUICtrl*, const LLSD&) { onCommitTextureOffsetX(); });
    getChildSetCommitCallback(mTexOffsetV, "TexOffsetV", [&](LLUICtrl*, const LLSD&) { onCommitTextureOffsetY(); });

    getChildSetCommitCallback(mBumpyScaleU, "bumpyScaleU", [&](LLUICtrl*, const LLSD&) { onCommitMaterialBumpyScaleX(); });
    getChildSetCommitCallback(mBumpyScaleV, "bumpyScaleV", [&](LLUICtrl*, const LLSD&) { onCommitMaterialBumpyScaleY(); });
    getChildSetCommitCallback(mBumpyRotate, "bumpyRot", [&](LLUICtrl*, const LLSD&) { onCommitMaterialBumpyRot(); });
    getChildSetCommitCallback(mBumpyOffsetU, "bumpyOffsetU", [&](LLUICtrl*, const LLSD&) { onCommitMaterialBumpyOffsetX(); });
    getChildSetCommitCallback(mBumpyOffsetV, "bumpyOffsetV", [&](LLUICtrl*, const LLSD&) { onCommitMaterialBumpyOffsetY(); });
    getChildSetCommitCallback(mShinyScaleU, "shinyScaleU", [&](LLUICtrl*, const LLSD&) { onCommitMaterialShinyScaleX(); });
    getChildSetCommitCallback(mShinyScaleV, "shinyScaleV", [&](LLUICtrl*, const LLSD&) { onCommitMaterialShinyScaleY(); });
    getChildSetCommitCallback(mShinyRotate, "shinyRot", [&](LLUICtrl*, const LLSD&) { onCommitMaterialShinyRot(); });
    getChildSetCommitCallback(mShinyOffsetU, "shinyOffsetU", [&](LLUICtrl*, const LLSD&) { onCommitMaterialShinyOffsetX(); });
    getChildSetCommitCallback(mShinyOffsetV, "shinyOffsetV", [&](LLUICtrl*, const LLSD&) { onCommitMaterialShinyOffsetY(); });

    getChildSetCommitCallback(mGlossiness, "glossiness", [&](LLUICtrl*, const LLSD&) { onCommitMaterialGloss(); });
    getChildSetCommitCallback(mEnvironment, "environment", [&](LLUICtrl*, const LLSD&) { onCommitMaterialEnv(); });
    getChildSetCommitCallback(mMaskCutoff, "maskcutoff", [&](LLUICtrl*, const LLSD&) { onCommitMaterialMaskCutoff(); });
    getChildSetCommitCallback(mAddMedia, "add_media", [&](LLUICtrl*, const LLSD&) { onClickBtnAddMedia(); });
    getChildSetCommitCallback(mDelMedia, "delete_media", [&](LLUICtrl*, const LLSD&) { onClickBtnDeleteMedia(); });

    getChildSetCommitCallback(mPBRScaleU, "gltfTextureScaleU", [&](LLUICtrl*, const LLSD&) { onCommitGLTFTextureScaleU(); });
    getChildSetCommitCallback(mPBRScaleV, "gltfTextureScaleV", [&](LLUICtrl*, const LLSD&) { onCommitGLTFTextureScaleV(); });
    getChildSetCommitCallback(mPBRRotate, "gltfTextureRotation", [&](LLUICtrl*, const LLSD&) { onCommitGLTFRotation(); });
    getChildSetCommitCallback(mPBROffsetU, "gltfTextureOffsetU", [&](LLUICtrl*, const LLSD&) { onCommitGLTFTextureOffsetU(); });
    getChildSetCommitCallback(mPBROffsetV, "gltfTextureOffsetV", [&](LLUICtrl*, const LLSD&) { onCommitGLTFTextureOffsetV(); });

    LLGLTFMaterialList::addSelectionUpdateCallback(&LLPanelFace::onMaterialOverrideReceived);
    sMaterialOverrideSelection.connect();

    getChildSetClickedCallback(mBtnAlign, "button align", [&](LLUICtrl*, const LLSD&) { onClickAutoFix(); });
    getChildSetClickedCallback(mBtnAlignTex, "button align textures", [&](LLUICtrl*, const LLSD&) { onAlignTexture(); });
    getChildSetClickedCallback(mBtnPbrFromInv, "pbr_from_inventory", [&](LLUICtrl*, const LLSD&) { onClickBtnLoadInvPBR(); });
    getChildSetClickedCallback(mBtnEditBbr, "edit_selected_pbr", [&](LLUICtrl*, const LLSD&) { onClickBtnEditPBR(); });
    getChildSetClickedCallback(mBtnSaveBbr, "save_selected_pbr", [&](LLUICtrl*, const LLSD&) { onClickBtnSavePBR(); });

    setMouseOpaque(false);

    mPBRTextureCtrl = getChild<LLTextureCtrl>("pbr_control");
    mPBRTextureCtrl->setDefaultImageAssetID(LLUUID::null);
    mPBRTextureCtrl->setBlankImageAssetID(BLANK_MATERIAL_ASSET_ID);
    mPBRTextureCtrl->setCommitCallback([&](LLUICtrl*, const LLSD&) { onCommitPbr(); });
    mPBRTextureCtrl->setOnCancelCallback([&](LLUICtrl*, const LLSD&) { onCancelPbr(); });
    mPBRTextureCtrl->setOnSelectCallback([&](LLUICtrl*, const LLSD&) { onSelectPbr(); });
    mPBRTextureCtrl->setDragCallback([&](LLUICtrl*, LLInventoryItem* item) { return onDragPbr(item); });
    mPBRTextureCtrl->setOnTextureSelectedCallback([&](LLInventoryItem* item) { onPbrSelectionChanged(item); });
    mPBRTextureCtrl->setOnCloseCallback([&](LLUICtrl*, const LLSD& data) { onCloseTexturePicker(data); });
    mPBRTextureCtrl->setFollowsTop();
    mPBRTextureCtrl->setFollowsLeft();
    mPBRTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
    mPBRTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);
    mPBRTextureCtrl->setBakeTextureEnabled(false);
    mPBRTextureCtrl->setInventoryPickType(PICK_MATERIAL);

    mTextureCtrl = getChild<LLTextureCtrl>("texture control");
    mTextureCtrl->setDefaultImageAssetID(DEFAULT_OBJECT_TEXTURE);
    mTextureCtrl->setCommitCallback([&](LLUICtrl*, const LLSD&) { onCommitTexture(); });
    mTextureCtrl->setOnCancelCallback([&](LLUICtrl*, const LLSD&) { onCancelTexture(); });
    mTextureCtrl->setOnSelectCallback([&](LLUICtrl*, const LLSD&) { onSelectTexture(); });
    mTextureCtrl->setDragCallback([&](LLUICtrl*, LLInventoryItem* item) { return onDragTexture(item); });
    mTextureCtrl->setOnTextureSelectedCallback([&](LLInventoryItem* item) { onTextureSelectionChanged(item); });
    mTextureCtrl->setOnCloseCallback([&](LLUICtrl*, const LLSD& data) { onCloseTexturePicker(data); });
    mTextureCtrl->setFollowsTop();
    mTextureCtrl->setFollowsLeft();
    mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
    mTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

    mShinyTextureCtrl = getChild<LLTextureCtrl>("shinytexture control");
    mShinyTextureCtrl->setDefaultImageAssetID(DEFAULT_OBJECT_SPECULAR);
    mShinyTextureCtrl->setCommitCallback([&](LLUICtrl*, const LLSD& data) { onCommitSpecularTexture(data); });
    mShinyTextureCtrl->setOnCancelCallback([&](LLUICtrl*, const LLSD& data) { onCancelSpecularTexture(data); });
    mShinyTextureCtrl->setOnSelectCallback([&](LLUICtrl*, const LLSD& data) { onSelectSpecularTexture(data); });
    mShinyTextureCtrl->setDragCallback([&](LLUICtrl*, LLInventoryItem* item) { return onDragTexture(item); });
    mShinyTextureCtrl->setOnTextureSelectedCallback([&](LLInventoryItem* item) { onTextureSelectionChanged(item); });
    mShinyTextureCtrl->setOnCloseCallback([&](LLUICtrl*, const LLSD& data) { onCloseTexturePicker(data); });
    mShinyTextureCtrl->setFollowsTop();
    mShinyTextureCtrl->setFollowsLeft();
    mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
    mShinyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

    mBumpyTextureCtrl = getChild<LLTextureCtrl>("bumpytexture control");
    mBumpyTextureCtrl->setDefaultImageAssetID(DEFAULT_OBJECT_NORMAL);
    mBumpyTextureCtrl->setBlankImageAssetID(BLANK_OBJECT_NORMAL);
    mBumpyTextureCtrl->setCommitCallback([&](LLUICtrl*, const LLSD& data) { onCommitNormalTexture(data); });
    mBumpyTextureCtrl->setOnCancelCallback([&](LLUICtrl*, const LLSD& data) { onCancelNormalTexture(data); });
    mBumpyTextureCtrl->setOnSelectCallback([&](LLUICtrl*, const LLSD& data) { onSelectNormalTexture(data); });
    mBumpyTextureCtrl->setDragCallback([&](LLUICtrl*, LLInventoryItem* item) { return onDragTexture(item); });
    mBumpyTextureCtrl->setOnTextureSelectedCallback([&](LLInventoryItem* item) { onTextureSelectionChanged(item); });
    mBumpyTextureCtrl->setOnCloseCallback([&](LLUICtrl*, const LLSD& data) { onCloseTexturePicker(data); });
    mBumpyTextureCtrl->setFollowsTop();
    mBumpyTextureCtrl->setFollowsLeft();
    mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
    mBumpyTextureCtrl->setDnDFilterPermMask(PERM_COPY | PERM_TRANSFER);

    mColorSwatch = getChild<LLColorSwatchCtrl>("colorswatch");
    mColorSwatch->setCommitCallback([&](LLUICtrl*, const LLSD&) { onCommitColor(); });
    mColorSwatch->setOnCancelCallback([&](LLUICtrl*, const LLSD&) { onCancelColor(); });
    mColorSwatch->setOnSelectCallback([&](LLUICtrl*, const LLSD&) { onSelectColor(); });
    mColorSwatch->setFollowsTop();
    mColorSwatch->setFollowsLeft();
    mColorSwatch->setCanApplyImmediately(true);

    mShinyColorSwatch = getChild<LLColorSwatchCtrl>("shinycolorswatch");
    mShinyColorSwatch->setCommitCallback([&](LLUICtrl*, const LLSD&) { onCommitShinyColor(); });
    mShinyColorSwatch->setOnCancelCallback([&](LLUICtrl*, const LLSD&) { onCancelShinyColor(); });
    mShinyColorSwatch->setOnSelectCallback([&](LLUICtrl*, const LLSD&) { onSelectShinyColor(); });
    mShinyColorSwatch->setFollowsTop();
    mShinyColorSwatch->setFollowsLeft();
    mShinyColorSwatch->setCanApplyImmediately(true);

    mLabelColorTransp = getChild<LLTextBox>("color trans");
    mLabelColorTransp->setFollowsTop();
    mLabelColorTransp->setFollowsLeft();

    mCtrlColorTransp = getChild<LLSpinCtrl>("ColorTrans");
    mCtrlColorTransp->setCommitCallback([&](LLUICtrl*, const LLSD&) { onCommitAlpha(); });
    mCtrlColorTransp->setPrecision(0);
    mCtrlColorTransp->setFollowsTop();
    mCtrlColorTransp->setFollowsLeft();

    getChildSetCommitCallback(mCheckFullbright, "checkbox fullbright", [&](LLUICtrl*, const LLSD&) { onCommitFullbright(); });

    mLabelTexGen = getChild<LLTextBox>("tex gen");
    getChildSetCommitCallback(mComboTexGen, "combobox texgen", [&](LLUICtrl*, const LLSD&) { onCommitTexGen(); });
    mComboTexGen->setFollows(FOLLOWS_LEFT | FOLLOWS_TOP);

    getChildSetCommitCallback(mComboMatMedia, "combobox matmedia", [&](LLUICtrl*, const LLSD&) { onCommitMaterialsMedia(); });
    mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);

    getChildSetCommitCallback(mRadioMaterialType, "radio_material_type", [&](LLUICtrl*, const LLSD&) { onCommitMaterialType(); });
    mRadioMaterialType->selectNthItem(MATTYPE_DIFFUSE);

    getChildSetCommitCallback(mRadioPbrType, "radio_pbr_type", [&](LLUICtrl*, const LLSD&) { onCommitPbrType(); });
    mRadioPbrType->selectNthItem(PBRTYPE_RENDER_MATERIAL_ID);

    mLabelGlow = getChild<LLTextBox>("glow label");
    getChildSetCommitCallback(mCtrlGlow, "glow", [&](LLUICtrl*, const LLSD&) { onCommitGlow(); });

    mMenuClipboardColor = getChild<LLMenuButton>("clipboard_color_params_btn");
    mMenuClipboardTexture = getChild<LLMenuButton>("clipboard_texture_params_btn");

    mTitleMedia = getChild<LLMediaCtrl>("title_media");
    mTitleMediaText = getChild<LLTextBox>("media_info");

    mLabelBumpiness = getChild<LLTextBox>("label bumpiness");
    mLabelShininess = getChild<LLTextBox>("label shininess");
    mLabelAlphaMode = getChild<LLTextBox>("label alphamode");
    mLabelGlossiness = getChild<LLTextBox>("label glossiness");
    mLabelEnvironment = getChild<LLTextBox>("label environment");
    mLabelMaskCutoff = getChild<LLTextBox>("label maskcutoff");
    mLabelShiniColor = getChild<LLTextBox>("label shinycolor");
    mLabelColor = getChild<LLTextBox>("color label");

    mLabelMatPermLoading = getChild<LLTextBox>("material_permissions_loading_label");

    mCheckSyncSettings = getChild<LLCheckBoxCtrl>("checkbox_sync_settings");

    clearCtrls();

    return true;
}

LLPanelFace::LLPanelFace()
:   LLPanel(),
    mIsAlpha(false),
    mComboMatMedia(NULL),
    mTitleMedia(NULL),
    mTitleMediaText(NULL),
    mNeedMediaTitle(true)
{
    USE_TEXTURE = LLTrans::getString("use_texture");
    mCommitCallbackRegistrar.add("PanelFace.menuDoToSelected", { boost::bind(&LLPanelFace::menuDoToSelected, this, _2) });
    mEnableCallbackRegistrar.add("PanelFace.menuEnable", boost::bind(&LLPanelFace::menuEnableItem, this, _2));
}

LLPanelFace::~LLPanelFace()
{
    unloadMedia();
}

void LLPanelFace::onVisibilityChange(bool new_visibility)
{
    if (new_visibility)
    {
        gAgent.showLatestFeatureNotification("gltf");
    }
    LLPanel::onVisibilityChange(new_visibility);
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
        LLMaterialEditor::updateLive();
    }
}

void LLPanelFace::sendTexture()
{
    if (!mTextureCtrl->getTentative())
    {
        // we grab the item id first, because we want to do a
        // permissions check in the selection manager. ARGH!
        LLUUID id = mTextureCtrl->getImageItemID();
        if(id.isNull())
        {
            id = mTextureCtrl->getImageAssetID();
        }
        if (!LLSelectMgr::getInstance()->selectionSetImage(id))
        {
            // need to refresh value in texture ctrl
            refresh();
        }
    }
}

void LLPanelFace::sendBump(U32 bumpiness)
{
    if (bumpiness < BUMPY_TEXTURE)
    {
        LL_DEBUGS("Materials") << "clearing bumptexture control" << LL_ENDL;
        mBumpyTextureCtrl->clear();
        mBumpyTextureCtrl->setImageAssetID(LLUUID());
    }

    updateBumpyControls(bumpiness == BUMPY_TEXTURE, true);

    LLUUID current_normal_map = mBumpyTextureCtrl->getImageAssetID();

    U8 bump = (U8)bumpiness & TEM_BUMP_MASK;

    // Clear legacy bump to None when using an actual normal map
    if (!current_normal_map.isNull())
    {
        bump = 0;
    }

    // Set the normal map or reset it to null as appropriate
    //
    LLSelectedTEMaterial::setNormalID(this, current_normal_map);

    LLSelectMgr::getInstance()->selectionSetBumpmap( bump, mBumpyTextureCtrl->getImageItemID() );
}

void LLPanelFace::sendTexGen()
{
    U8 tex_gen = (U8)mComboTexGen->getCurrentIndex() << TEM_TEX_GEN_SHIFT;
    LLSelectMgr::getInstance()->selectionSetTexGen(tex_gen);
}

void LLPanelFace::sendShiny(U32 shininess)
{
    if (shininess < SHINY_TEXTURE)
    {
        mShinyTextureCtrl->clear();
        mShinyTextureCtrl->setImageAssetID(LLUUID());
    }

    LLUUID specmap = getCurrentSpecularMap();

    U8 shiny = (U8) shininess & TEM_SHINY_MASK;
    if (!specmap.isNull())
    {
        shiny = 0;
    }

    LLSelectedTEMaterial::setSpecularID(this, specmap);

    LLSelectMgr::getInstance()->selectionSetShiny(shiny, mShinyTextureCtrl->getImageItemID());

    updateShinyControls(!specmap.isNull(), true);
}

void LLPanelFace::sendFullbright()
{
    U8 fullbright = mCheckFullbright->get() ? TEM_FULLBRIGHT_MASK : 0;
    LLSelectMgr::getInstance()->selectionSetFullbright(fullbright);
}

void LLPanelFace::sendColor()
{
    LLColor4 color = mColorSwatch->get();
    LLSelectMgr::getInstance()->selectionSetColorOnly(color);
}

void LLPanelFace::sendAlpha()
{
    F32 alpha = (100.f - mCtrlColorTransp->get()) / 100.f;
    LLSelectMgr::getInstance()->selectionSetAlphaOnly( alpha );
}

void LLPanelFace::sendGlow()
{
    F32 glow = mCtrlGlow->get();
    LLSelectMgr::getInstance()->selectionSetGlow(glow);
}

struct LLPanelFaceSetTEFunctor : public LLSelectedTEFunctor
{
    LLPanelFaceSetTEFunctor(LLPanelFace* panel) : mPanel(panel) {}
    virtual bool apply(LLViewerObject* object, S32 te)
    {
        LLSpinCtrl *ctrlTexScaleS, *ctrlTexScaleT, *ctrlTexOffsetS, *ctrlTexOffsetT, *ctrlTexRotation;

        // Effectively the same as MATMEDIA_PBR sans using different radio,
        // separate for the sake of clarity
        switch (mPanel->mRadioMaterialType->getSelectedIndex())
        {
        case MATTYPE_DIFFUSE:
            ctrlTexScaleS = mPanel->mTexScaleU;
            ctrlTexScaleT = mPanel->mTexScaleV;
            ctrlTexOffsetS = mPanel->mTexOffsetU;
            ctrlTexOffsetT = mPanel->mTexOffsetV;
            ctrlTexRotation = mPanel->mTexRotate;
            break;
        case MATTYPE_NORMAL:
            ctrlTexScaleS = mPanel->mBumpyScaleU;
            ctrlTexScaleT = mPanel->mBumpyScaleV;
            ctrlTexOffsetS = mPanel->mBumpyOffsetU;
            ctrlTexOffsetT = mPanel->mBumpyOffsetV;
            ctrlTexRotation = mPanel->mBumpyRotate;
            break;
        case MATTYPE_SPECULAR:
            ctrlTexScaleS = mPanel->mShinyScaleU;
            ctrlTexScaleT = mPanel->mShinyScaleV;
            ctrlTexOffsetS = mPanel->mShinyOffsetU;
            ctrlTexOffsetT = mPanel->mShinyOffsetV;
            ctrlTexRotation = mPanel->mShinyRotate;
            break;
        default:
            llassert(false);
            return false;
        }

        bool align_planar = mPanel->mPlanarAlign->get();

        llassert(object);

        if (ctrlTexScaleS)
        {
            bool valid = !ctrlTexScaleS->getTentative(); // || !checkFlipScaleS->getTentative();
            if (valid || align_planar)
            {
                F32 value = ctrlTexScaleS->get();
                if (mPanel->mComboTexGen->getCurrentIndex() == 1)
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
            bool valid = !ctrlTexScaleT->getTentative(); // || !checkFlipScaleT->getTentative();
            if (valid || align_planar)
            {
                F32 value = ctrlTexScaleT->get();
                //if (checkFlipScaleT->get())
                //{
                //  value = -value;
                //}
                if (mPanel->mComboTexGen->getCurrentIndex() == 1)
                {
                    value *= 0.5f;
                }
                object->setTEScaleT(te, value);

                if (align_planar)
                {
                    LLPanelFace::LLSelectedTEMaterial::setNormalRepeatY(mPanel, value, te, object->getID());
                    LLPanelFace::LLSelectedTEMaterial::setSpecularRepeatY(mPanel, value, te, object->getID());
                }
            }
        }

        if (ctrlTexOffsetS)
        {
            bool valid = !ctrlTexOffsetS->getTentative();
            if (valid || align_planar)
            {
                F32 value = ctrlTexOffsetS->get();
                object->setTEOffsetS(te, value);

                if (align_planar)
                {
                    LLPanelFace::LLSelectedTEMaterial::setNormalOffsetX(mPanel, value, te, object->getID());
                    LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetX(mPanel, value, te, object->getID());
                }
            }
        }

        if (ctrlTexOffsetT)
        {
            bool valid = !ctrlTexOffsetT->getTentative();
            if (valid || align_planar)
            {
                F32 value = ctrlTexOffsetT->get();
                object->setTEOffsetT(te, value);

                if (align_planar)
                {
                    LLPanelFace::LLSelectedTEMaterial::setNormalOffsetY(mPanel, value, te, object->getID());
                    LLPanelFace::LLSelectedTEMaterial::setSpecularOffsetY(mPanel, value, te, object->getID());
                }
            }
        }

        if (ctrlTexRotation)
        {
            bool valid = !ctrlTexRotation->getTentative();
            if (valid || align_planar)
            {
                F32 value = ctrlTexRotation->get() * DEG_TO_RAD;
                object->setTERotation(te, value);

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
        if (facep->calcAlignedPlanarTE(mCenterFace, &aligned_st_offset, &aligned_st_scale, &aligned_st_rot))
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
    if (mPlanarAlign->getValue().asBoolean())
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

void LLPanelFace::alignTextureLayer()
{
    LLFace* last_face = NULL;
    bool identical_face = false;
    LLSelectedTE::getFace(last_face, identical_face);

    LLPanelFaceSetAlignedConcreteTEFunctor setfunc(this, last_face, static_cast<LLRender::eTexIndex>(mRadioMaterialType->getSelectedIndex()));
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
}

void LLPanelFace::getState()
{
    updateUI();
}

void LLPanelFace::updateUI(bool force_set_values /*false*/)
{ //set state of UI to match state of texture entry(ies)  (calls setEnabled, setValue, etc, but NOT setVisible)
    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
    LLViewerObject* objectp = node ? node->getObject() : NULL;

    if (objectp
        && objectp->getPCode() == LL_PCODE_VOLUME
        && objectp->permModify())
    {
        bool editable = objectp->permModify() && !objectp->isPermanentEnforced();
        bool attachment = objectp->isAttachment();

        bool has_pbr_material;
        bool has_faces_without_pbr;
        updateUIGLTF(objectp, has_pbr_material, has_faces_without_pbr, force_set_values);

        const bool has_material = !has_pbr_material;

        // only turn on auto-adjust button if there is a media renderer and the media is loaded
        mBtnAlign->setEnabled(editable);

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

        // *NOTE: The "identical" variable is currently only used to decide if
        // the texgen control should be tentative - this is not used by GLTF
        // materials. -Cosmic;2022-11-09
        bool identical         = true;  // true because it is anded below
        bool identical_diffuse = false;
        bool identical_norm    = false;
        bool identical_spec    = false;

        LLUUID id;
        LLUUID normmap_id;
        LLUUID specmap_id;

        LLSelectedTE::getTexId(id, identical_diffuse);
        LLSelectedTEMaterial::getNormalID(normmap_id, identical_norm);
        LLSelectedTEMaterial::getSpecularID(specmap_id, identical_spec);

        static S32 selected_te = -1;
        static LLUUID prev_obj_id;
        if ((LLToolFace::getInstance() == LLToolMgr::getInstance()->getCurrentTool()) &&
            !LLSelectMgr::getInstance()->getSelection()->isMultipleTESelected())
        {
            S32 new_selection = -1; // Don't use getLastSelectedTE, it could have been deselected
            S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
            for (S32 te = 0; te < num_tes; ++te)
            {
                if (node->isTESelected(te))
                {
                    new_selection = te;
                    break;
                }
            }

            if ((new_selection != selected_te)
                || (prev_obj_id != objectp->getID()))
            {
                bool te_has_media = objectp->getTE(new_selection) && objectp->getTE(new_selection)->hasMedia();
                bool te_has_pbr = objectp->getRenderMaterialID(new_selection).notNull();

                if (te_has_pbr && !((mComboMatMedia->getCurrentIndex() == MATMEDIA_MEDIA) && te_has_media))
                {
                    mComboMatMedia->selectNthItem(MATMEDIA_PBR);
                }
                else if (te_has_media)
                {
                    mComboMatMedia->selectNthItem(MATMEDIA_MEDIA);
                }
                else if (id.notNull() || normmap_id.notNull() || specmap_id.notNull())
                {
                    mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
                }
                selected_te = new_selection;
                prev_obj_id = objectp->getID();
            }
        }
        else
        {
            if (prev_obj_id != objectp->getID())
            {
                if (has_pbr_material && (mComboMatMedia->getCurrentIndex() == MATMEDIA_MATERIAL))
                {
                    mComboMatMedia->selectNthItem(MATMEDIA_PBR);
                }
                else if (!has_pbr_material && (mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR))
                {
                    mComboMatMedia->selectNthItem(MATMEDIA_MATERIAL);
                }
                prev_obj_id = objectp->getID();
            }
        }
        mComboMatMedia->setEnabled(editable);

        if (mRadioMaterialType->getSelectedIndex() < MATTYPE_DIFFUSE)
        {
            mRadioMaterialType->selectNthItem(MATTYPE_DIFFUSE);
        }
        mRadioMaterialType->setEnabled(editable);

        if (mRadioPbrType->getSelectedIndex() < PBRTYPE_RENDER_MATERIAL_ID)
        {
            mRadioPbrType->selectNthItem(PBRTYPE_RENDER_MATERIAL_ID);
        }
        mRadioPbrType->setEnabled(editable);
        const bool pbr_selected = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR;
        const bool texture_info_selected = pbr_selected && mRadioPbrType->getSelectedIndex() != PBRTYPE_RENDER_MATERIAL_ID;

        mCheckSyncSettings->setEnabled(editable);
        mCheckSyncSettings->setValue(gSavedSettings.getBOOL("SyncMaterialSettings"));

        updateVisibility(objectp);

        // Color swatch
        mLabelColor->setEnabled(editable);
        LLColor4 color = LLColor4::white;
        bool identical_color = false;

        LLSelectedTE::getColor(color, identical_color);
        LLColor4 prev_color = mColorSwatch->get();
        mColorSwatch->setOriginal(color);
        mColorSwatch->set(color, force_set_values || (prev_color != color) || !editable);
        mColorSwatch->setValid(editable && !has_pbr_material);
        mColorSwatch->setEnabled( editable && !has_pbr_material);
        mColorSwatch->setCanApplyImmediately( editable && !has_pbr_material);

        // Color transparency
        mLabelColorTransp->setEnabled(editable);

        F32 transparency = (1.f - color.mV[VALPHA]) * 100.f;
        mCtrlColorTransp->setValue(editable ? transparency : 0);
        mCtrlColorTransp->setEnabled(editable && has_material);

        // Shiny
        U8 shiny = 0;
        {
            bool identical_shiny = false;

            LLSelectedTE::getShiny(shiny, identical_shiny);
            identical = identical && identical_shiny;

            shiny = specmap_id.isNull() ? shiny : SHINY_TEXTURE;

            mComboShininess->getSelectionInterface()->selectNthItem((S32)shiny);

            mLabelShininess->setEnabled(editable);
            mComboShininess->setEnabled(editable);

            mLabelGlossiness->setEnabled(editable);
            mGlossiness->setEnabled(editable);

            mLabelEnvironment->setEnabled(editable);
            mEnvironment->setEnabled(editable);
            mLabelShiniColor->setEnabled(editable);

            mComboShininess->setTentative(!identical_spec);
            mGlossiness->setTentative(!identical_spec);
            mEnvironment->setTentative(!identical_spec);
            mShinyColorSwatch->setTentative(!identical_spec);

            mShinyColorSwatch->setValid(editable);
            mShinyColorSwatch->setEnabled(editable);
            mShinyColorSwatch->setCanApplyImmediately(editable);
        }

        // Bumpy
        U8 bumpy = 0;
        {
            bool identical_bumpy = false;
            LLSelectedTE::getBumpmap(bumpy, identical_bumpy);

            LLUUID norm_map_id = getCurrentNormalMap();
            bumpy = norm_map_id.isNull() ? bumpy : BUMPY_TEXTURE;
            mComboBumpiness->getSelectionInterface()->selectNthItem((S32)bumpy);

            mComboBumpiness->setEnabled(editable);
            mComboBumpiness->setTentative(!identical_bumpy);
            mLabelBumpiness->setEnabled(editable);
        }

        // Texture
        {
            LLGLenum image_format = GL_RGB;
            bool identical_image_format = false;
            bool missing_asset = false;
            LLSelectedTE::getImageFormat(image_format, identical_image_format, missing_asset);

            if (!missing_asset)
            {
                mIsAlpha = false;
                switch (image_format)
                {
                case GL_RGBA:
                case GL_ALPHA:
                    {
                        mIsAlpha = true;
                    }
                    break;

                case GL_RGB: break;
                default:
                    {
                        LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
                    }
                    break;
                }
            }
            else
            {
                // Don't know image's properties, use material's mode value
                mIsAlpha = true;
            }

            if (LLViewerMedia::getInstance()->textureHasMedia(id))
            {
                mBtnAlign->setEnabled(editable);
            }

            // Diffuse Alpha Mode

            // Init to the default that is appropriate for the alpha content of the asset
            //
            U8 alpha_mode = mIsAlpha ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

            bool identical_alpha_mode = false;

            // See if that's been overridden by a material setting for same...
            //
            LLSelectedTEMaterial::getCurrentDiffuseAlphaMode(alpha_mode, identical_alpha_mode, mIsAlpha);

            // it is invalid to have any alpha mode other than blend if transparency is greater than zero ...
            // Want masking? Want emissive? Tough! You get BLEND!
            alpha_mode = (transparency > 0.f) ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : alpha_mode;

            // ... unless there is no alpha channel in the texture, in which case alpha mode MUST be none
            alpha_mode = mIsAlpha ? alpha_mode : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

            mComboAlphaMode->getSelectionInterface()->selectNthItem(alpha_mode);

            updateAlphaControls();

            if (mTextureCtrl)
            {
                if (identical_diffuse)
                {
                    mTextureCtrl->setTentative(false);
                    mTextureCtrl->setEnabled(editable && !has_pbr_material);
                    mTextureCtrl->setImageAssetID(id);

                    bool can_change_alpha = editable && mIsAlpha && !missing_asset && !has_pbr_material;
                    mComboAlphaMode->setEnabled(can_change_alpha && transparency <= 0.f);
                    mLabelAlphaMode->setEnabled(can_change_alpha);
                    mMaskCutoff->setEnabled(can_change_alpha);
                    mLabelMaskCutoff->setEnabled(can_change_alpha);

                    mTextureCtrl->setBakeTextureEnabled(true);
                }
                else if (id.isNull())
                {
                    // None selected
                    mTextureCtrl->setTentative(false);
                    mTextureCtrl->setEnabled(false);
                    mTextureCtrl->setImageAssetID(LLUUID::null);
                    mComboAlphaMode->setEnabled(false);
                    mLabelAlphaMode->setEnabled(false);
                    mMaskCutoff->setEnabled(false);
                    mLabelMaskCutoff->setEnabled(false);

                    mTextureCtrl->setBakeTextureEnabled(false);
                }
                else
                {
                    // Tentative: multiple selected with different textures
                    mTextureCtrl->setTentative(true);
                    mTextureCtrl->setEnabled(editable && !has_pbr_material);
                    mTextureCtrl->setImageAssetID(id);

                    bool can_change_alpha = editable && mIsAlpha && !missing_asset && !has_pbr_material;
                    mComboAlphaMode->setEnabled(can_change_alpha && transparency <= 0.f);
                    mLabelAlphaMode->setEnabled(can_change_alpha);
                    mMaskCutoff->setEnabled(can_change_alpha);
                    mLabelMaskCutoff->setEnabled(can_change_alpha);

                    mTextureCtrl->setBakeTextureEnabled(true);
                }

                if (attachment)
                {
                    // attachments are in world and in inventory,
                    // server doesn't support changing permissions
                    // in such case
                    mTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
                }
                else
                {
                    mTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
                }
            }

            if (mShinyTextureCtrl)
            {
                mShinyTextureCtrl->setTentative(!identical_spec);
                mShinyTextureCtrl->setEnabled(editable && !has_pbr_material);
                mShinyTextureCtrl->setImageAssetID(specmap_id);

                if (attachment)
                {
                    mShinyTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
                }
                else
                {
                    mShinyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
                }
            }

            if (mBumpyTextureCtrl)
            {
                mBumpyTextureCtrl->setTentative(!identical_norm);
                mBumpyTextureCtrl->setEnabled(editable && !has_pbr_material);
                mBumpyTextureCtrl->setImageAssetID(normmap_id);

                if (attachment)
                {
                    mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
                }
                else
                {
                    mBumpyTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
                }
            }
        }

        // planar align
        bool align_planar = mPlanarAlign->get();
        bool identical_planar_aligned = false;

        bool enabled = (editable && isIdenticalPlanarTexgen() && !texture_info_selected);
        mPlanarAlign->setValue(align_planar && enabled);
        mPlanarAlign->setVisible(enabled);
        mPlanarAlign->setEnabled(enabled);
        mBtnAlignTex->setEnabled(enabled && LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1);

        if (align_planar && enabled)
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);

            LLPanelFaceGetIsAlignedTEFunctor get_is_aligend_func(last_face);
            // this will determine if the texture param controls are tentative:
            identical_planar_aligned = LLSelectMgr::getInstance()->getSelection()->applyToTEs(&get_is_aligend_func);
        }

        // Needs to be public and before tex scale settings below to properly reflect
        // behavior when in planar vs default texgen modes in the
        // NORSPEC-84 et al
        //
        LLTextureEntry::e_texgen selected_texgen = LLTextureEntry::TEX_GEN_DEFAULT;
        bool identical_texgen = true;
        bool identical_planar_texgen = false;

        LLSelectedTE::getTexGen(selected_texgen, identical_texgen);
        identical_planar_texgen = (identical_texgen && (selected_texgen == LLTextureEntry::TEX_GEN_PLANAR));

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

            mTexScaleU->setValue(diff_scale_s);
            mShinyScaleU->setValue(spec_scale_s);
            mBumpyScaleU->setValue(norm_scale_s);

            mTexScaleU->setEnabled(editable && has_material);
            mShinyScaleU->setEnabled(editable && has_material && specmap_id.notNull());
            mBumpyScaleU->setEnabled(editable && has_material && normmap_id.notNull());

            bool diff_scale_tentative = !(identical && identical_diff_scale_s);
            bool norm_scale_tentative = !(identical && identical_norm_scale_s);
            bool spec_scale_tentative = !(identical && identical_spec_scale_s);

            mTexScaleU->setTentative(LLSD(diff_scale_tentative));
            mShinyScaleU->setTentative(LLSD(spec_scale_tentative));
            mBumpyScaleU->setTentative(LLSD(norm_scale_tentative));
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

            bool diff_scale_tentative = !identical_diff_scale_t;
            bool norm_scale_tentative = !identical_norm_scale_t;
            bool spec_scale_tentative = !identical_spec_scale_t;

            mTexScaleV->setEnabled(editable && has_material);
            mShinyScaleV->setEnabled(editable && has_material && specmap_id.notNull());
            mBumpyScaleV->setEnabled(editable && has_material && normmap_id.notNull());

            if (force_set_values)
            {
                mTexScaleV->forceSetValue(diff_scale_t);
            }
            else
            {
                mTexScaleV->setValue(diff_scale_t);
            }
            mShinyScaleV->setValue(spec_scale_t);
            mBumpyScaleV->setValue(norm_scale_t);

            mTexScaleV->setTentative(LLSD(diff_scale_tentative));
            mShinyScaleV->setTentative(LLSD(spec_scale_tentative));
            mBumpyScaleV->setTentative(LLSD(norm_scale_tentative));
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

            bool diff_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_s);
            bool norm_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_s);
            bool spec_offset_u_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_s);

            mTexOffsetU->setValue(editable ? diff_offset_s : 0.0f);
            mBumpyOffsetU->setValue(editable ? norm_offset_s : 0.0f);
            mShinyOffsetU->setValue(editable ? spec_offset_s : 0.0f);

            mTexOffsetU->setTentative(LLSD(diff_offset_u_tentative));
            mShinyOffsetU->setTentative(LLSD(spec_offset_u_tentative));
            mBumpyOffsetU->setTentative(LLSD(norm_offset_u_tentative));

            mTexOffsetU->setEnabled(editable && has_material);
            mShinyOffsetU->setEnabled(editable && has_material && specmap_id.notNull());
            mBumpyOffsetU->setEnabled(editable && has_material && normmap_id.notNull());
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

            bool diff_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_diff_offset_t);
            bool norm_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_norm_offset_t);
            bool spec_offset_v_tentative = !(align_planar ? identical_planar_aligned : identical_spec_offset_t);

            mTexOffsetV->setValue(  editable ? diff_offset_t : 0.0f);
            mBumpyOffsetV->setValue(editable ? norm_offset_t : 0.0f);
            mShinyOffsetV->setValue(editable ? spec_offset_t : 0.0f);

            mTexOffsetV->setTentative(LLSD(diff_offset_v_tentative));
            mBumpyOffsetV->setTentative(LLSD(norm_offset_v_tentative));
            mShinyOffsetV->setTentative(LLSD(spec_offset_v_tentative));

            mTexOffsetV->setEnabled(editable && has_material);
            mShinyOffsetV->setEnabled(editable && has_material && specmap_id.notNull());
            mBumpyOffsetV->setEnabled(editable && has_material && normmap_id.notNull());
        }

        // Texture rotation
        {
            bool identical_diff_rotation = false;
            bool identical_norm_rotation = false;
            bool identical_spec_rotation = false;

            F32 diff_rotation = 0.f;
            F32 norm_rotation = 0.f;
            F32 spec_rotation = 0.f;

            LLSelectedTE::getRotation(diff_rotation, identical_diff_rotation);
            LLSelectedTEMaterial::getSpecularRotation(spec_rotation, identical_spec_rotation);
            LLSelectedTEMaterial::getNormalRotation(norm_rotation, identical_norm_rotation);

            bool diff_rot_tentative = !(align_planar ? identical_planar_aligned : identical_diff_rotation);
            bool norm_rot_tentative = !(align_planar ? identical_planar_aligned : identical_norm_rotation);
            bool spec_rot_tentative = !(align_planar ? identical_planar_aligned : identical_spec_rotation);

            F32 diff_rot_deg = diff_rotation * RAD_TO_DEG;
            F32 norm_rot_deg = norm_rotation * RAD_TO_DEG;
            F32 spec_rot_deg = spec_rotation * RAD_TO_DEG;

            mTexRotate->setEnabled(editable && has_material);
            mShinyRotate->setEnabled(editable && has_material && specmap_id.notNull());
            mBumpyRotate->setEnabled(editable && has_material && normmap_id.notNull());

            mTexRotate->setTentative(LLSD(diff_rot_tentative));
            mShinyRotate->setTentative(LLSD(spec_rot_tentative));
            mBumpyRotate->setTentative(LLSD(norm_rot_tentative));

            mTexRotate->setValue(editable ? diff_rot_deg : 0.0f);
            mShinyRotate->setValue(editable ? spec_rot_deg : 0.0f);
            mBumpyRotate->setValue(editable ? norm_rot_deg : 0.0f);
        }

        {
            F32 glow = 0.f;
            bool identical_glow = false;
            LLSelectedTE::getGlow(glow, identical_glow);
            mCtrlGlow->setValue(glow);
            mCtrlGlow->setTentative(!identical_glow);
            mCtrlGlow->setEnabled(editable);
            mLabelGlow->setEnabled(editable);
        }

        {
            // Maps from enum to combobox entry index
            mComboTexGen->selectNthItem(((S32)selected_texgen) >> 1);

            mComboTexGen->setEnabled(editable);
            mComboTexGen->setTentative(!identical);
            mLabelTexGen->setEnabled(editable);
        }

        {
            U8 fullbright_flag = 0;
            bool identical_fullbright = false;

            LLSelectedTE::getFullbright(fullbright_flag, identical_fullbright);

            mCheckFullbright->setValue((S32)(fullbright_flag != 0));
            mCheckFullbright->setEnabled(editable && !has_pbr_material);
            mCheckFullbright->setTentative(!identical_fullbright);
            mComboMatMedia->setEnabledByValue("Materials", !has_pbr_material);
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

            {
                S32 index = mComboTexGen ? mComboTexGen->getCurrentIndex() : 0;
                bool enabled = editable && (index != 1);
                bool identical_repeats = true;
                S32 material_selection = mComboMatMedia->getCurrentIndex();
                F32 repeats = 1.0f;

                U32 material_type = MATTYPE_DIFFUSE;
                if (material_selection == MATMEDIA_MATERIAL)
                {
                    material_type = mRadioMaterialType->getSelectedIndex();
                }
                else if (material_selection == MATMEDIA_PBR)
                {
                    enabled = editable && has_pbr_material;
                    material_type = mRadioPbrType->getSelectedIndex();
                }

                switch (material_type)
                {
                default:
                case MATTYPE_DIFFUSE:
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = editable && !id.isNull();
                    }
                    identical_repeats = identical_diff_repeats;
                    repeats = repeats_diff;
                    break;
                case MATTYPE_SPECULAR:
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = (editable && ((shiny == SHINY_TEXTURE) && !specmap_id.isNull()));
                    }
                    identical_repeats = identical_spec_repeats;
                    repeats = repeats_spec;
                    break;
                case MATTYPE_NORMAL:
                    if (material_selection != MATMEDIA_PBR)
                    {
                        enabled = (editable && ((bumpy == BUMPY_TEXTURE) && !normmap_id.isNull()));
                    }
                    identical_repeats = identical_norm_repeats;
                    repeats = repeats_norm;
                    break;
                }

                bool repeats_tentative = !identical_repeats;

                if (force_set_values)
                {
                    // onCommit, previosly edited element updates related ones
                    mTexRepeat->forceSetValue(editable ? repeats : 1.0f);
                }
                else
                {
                    mTexRepeat->setValue(editable ? repeats : 1.0f);
                }
                mTexRepeat->setTentative(LLSD(repeats_tentative));
                mTexRepeat->setEnabled(has_material && !identical_planar_texgen && enabled);
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

                    mComboAlphaMode->getSelectionInterface()->selectNthItem(alpha_mode);
                }

                mMaskCutoff->setValue(material->getAlphaMaskCutoff());
                updateAlphaControls();

                identical_planar_texgen = isIdenticalPlanarTexgen();

                // Shiny (specular)
                F32 offset_x, offset_y, repeat_x, repeat_y, rot;
                mShinyTextureCtrl->setImageAssetID(material->getSpecularID());

                if (!material->getSpecularID().isNull() && (shiny == SHINY_TEXTURE))
                {
                    material->getSpecularOffset(offset_x, offset_y);
                    material->getSpecularRepeat(repeat_x, repeat_y);

                    if (identical_planar_texgen)
                    {
                        repeat_x *= 2.0f;
                        repeat_y *= 2.0f;
                    }

                    rot = material->getSpecularRotation();
                    mShinyScaleU->setValue(repeat_x);
                    mShinyScaleV->setValue(repeat_y);
                    mShinyRotate->setValue(rot * RAD_TO_DEG);
                    mShinyOffsetU->setValue(offset_x);
                    mShinyOffsetV->setValue(offset_y);
                    mGlossiness->setValue(material->getSpecularLightExponent());
                    mEnvironment->setValue(material->getEnvironmentIntensity());

                    updateShinyControls(!material->getSpecularID().isNull(), true);
                }

                // Assert desired colorswatch color to match material AFTER updateShinyControls
                // to avoid getting overwritten with the default on some UI state changes.
                //
                if (!material->getSpecularID().isNull())
                {
                    LLColor4 new_color = material->getSpecularLightColor();
                    LLColor4 old_color = mShinyColorSwatch->get();

                    mShinyColorSwatch->setOriginal(new_color);
                    mShinyColorSwatch->set(new_color, force_set_values || old_color != new_color || !editable);
                }

                // Bumpy (normal)
                mBumpyTextureCtrl->setImageAssetID(material->getNormalID());

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
                    mBumpyScaleU->setValue(repeat_x);
                    mBumpyScaleV->setValue(repeat_y);
                    mBumpyRotate->setValue(rot*RAD_TO_DEG);
                    mBumpyOffsetU->setValue(offset_x);
                    mBumpyOffsetV->setValue(offset_y);

                    updateBumpyControls(!material->getNormalID().isNull(), true);
                }
            }
        }

        S32 selected_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
        bool single_volume = (selected_count == 1);
        mMenuClipboardColor->setEnabled(editable && single_volume);

        // Set variable values for numeric expressions
        LLCalc* calcp = LLCalc::getInstance();
        calcp->setVar(LLCalc::TEX_U_SCALE, (F32)mTexScaleU->getValue().asReal());
        calcp->setVar(LLCalc::TEX_V_SCALE, (F32)mTexScaleV->getValue().asReal());
        calcp->setVar(LLCalc::TEX_U_OFFSET, (F32)mTexOffsetU->getValue().asReal());
        calcp->setVar(LLCalc::TEX_V_OFFSET, (F32)mTexOffsetV->getValue().asReal());
        calcp->setVar(LLCalc::TEX_ROTATION, (F32)mTexRotate->getValue().asReal());
        calcp->setVar(LLCalc::TEX_TRANSPARENCY, (F32)mCtrlColorTransp->getValue().asReal());
        calcp->setVar(LLCalc::TEX_GLOW, (F32)mCtrlGlow->getValue().asReal());
    }
    else
    {
        // Disable all UICtrls
        clearCtrls();

        // Disable non-UICtrls
        if (mPBRTextureCtrl)
        {
            mPBRTextureCtrl->setImageAssetID(LLUUID::null);
            mPBRTextureCtrl->setEnabled(false);
        }

        if (mTextureCtrl)
        {
            mTextureCtrl->setImageAssetID( LLUUID::null );
            mTextureCtrl->setEnabled( false );  // this is a LLUICtrl, but we don't want it to have keyboard focus so we add it as a child, not a ctrl.
//          mTextureCtrl->setValid(false);
        }

        if (mColorSwatch)
        {
            mColorSwatch->setEnabled( false );
            mColorSwatch->setFallbackImage(LLUI::getUIImage("locked_image.j2c") );
            mColorSwatch->setValid(false);
        }

        if (mRadioMaterialType)
        {
            mRadioMaterialType->setSelectedIndex(0);
        }
        mLabelColorTransp->setEnabled(false);
        mTexRepeat->setEnabled(false);
        mLabelTexGen->setEnabled(false);
        mLabelShininess->setEnabled(false);
        mLabelBumpiness->setEnabled(false);
        mBtnAlign->setEnabled(false);
        mBtnPbrFromInv->setEnabled(false);
        mBtnEditBbr->setEnabled(false);
        mBtnSaveBbr->setEnabled(false);

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

// One-off listener that updates the build floater UI when the agent inventory adds or removes an item
class PBRPickerAgentListener : public LLInventoryObserver
{
protected:
    bool mChangePending = true;
public:
    PBRPickerAgentListener() : LLInventoryObserver()
    {
        gInventory.addObserver(this);
    }

    const bool isListening()
    {
        return mChangePending;
    }

    void changed(U32 mask) override
    {
        if (!(mask & (ADD | REMOVE)))
        {
            return;
        }

        if (gFloaterTools)
        {
            gFloaterTools->dirty();
        }
        gInventory.removeObserver(this);
        mChangePending = false;
    }

    ~PBRPickerAgentListener() override
    {
        gInventory.removeObserver(this);
        mChangePending = false;
    }
};

// One-off listener that updates the build floater UI when the prim inventory updates
class PBRPickerObjectListener : public LLVOInventoryListener
{
protected:
    LLViewerObject* mObjectp;
    bool mChangePending = true;
public:

    PBRPickerObjectListener(LLViewerObject* object)
    : mObjectp(object)
    {
        registerVOInventoryListener(mObjectp, nullptr);
    }

    const bool isListeningFor(const LLViewerObject* objectp) const
    {
        return mChangePending && (objectp == mObjectp);
    }

    void inventoryChanged(LLViewerObject* object,
        LLInventoryObject::object_list_t* inventory,
        S32 serial_num,
        void* user_data) override
    {
        if (gFloaterTools)
        {
            gFloaterTools->dirty();
        }
        removeVOInventoryListener();
        mChangePending = false;
    }

    ~PBRPickerObjectListener()
    {
        removeVOInventoryListener();
        mChangePending = false;
    }
};

void LLPanelFace::updateUIGLTF(LLViewerObject* objectp, bool& has_pbr_material, bool& has_faces_without_pbr, bool force_set_values)
{
    has_pbr_material = false;

    bool has_pbr_capabilities = LLMaterialEditor::capabilitiesAvailable();
    bool identical_pbr = true;
    const bool settable = has_pbr_capabilities && objectp->permModify() && !objectp->isPermanentEnforced();
    const bool editable = LLMaterialEditor::canModifyObjectsMaterial();
    const bool saveable = LLMaterialEditor::canSaveObjectsMaterial();

    // pbr material
    LLUUID pbr_id;
    if (mPBRTextureCtrl)
    {
        LLSelectedTE::getPbrMaterialId(pbr_id, identical_pbr, has_pbr_material, has_faces_without_pbr);

        mPBRTextureCtrl->setTentative(!identical_pbr);
        mPBRTextureCtrl->setEnabled(settable);
        mPBRTextureCtrl->setImageAssetID(pbr_id);

        if (objectp->isAttachment())
        {
            mPBRTextureCtrl->setFilterPermissionMasks(PERM_COPY | PERM_TRANSFER | PERM_MODIFY);
        }
        else
        {
            mPBRTextureCtrl->setImmediateFilterPermMask(PERM_NONE);
        }
    }

    mBtnPbrFromInv->setEnabled(settable);
    mBtnEditBbr->setEnabled(editable && !has_faces_without_pbr);
    mBtnSaveBbr->setEnabled(saveable && identical_pbr);
    if (objectp->isInventoryPending())
    {
        // Reuse the same listener when possible
        if (!mVOInventoryListener || !mVOInventoryListener->isListeningFor(objectp))
        {
            mVOInventoryListener = std::make_unique<PBRPickerObjectListener>(objectp);
        }
    }
    else
    {
        mVOInventoryListener = nullptr;
    }
    if (!identical_pbr || pbr_id.isNull() || pbr_id == BLANK_MATERIAL_ASSET_ID)
    {
        mAgentInventoryListener = nullptr;
    }
    else
    {
        if (!mAgentInventoryListener || !mAgentInventoryListener->isListening())
        {
            mAgentInventoryListener = std::make_unique<PBRPickerAgentListener>();
        }
    }

    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();
    if (show_pbr)
    {
        const bool new_state = has_pbr_capabilities && has_pbr_material && !has_faces_without_pbr;

        mPBRScaleU->setEnabled(new_state);
        mPBRScaleV->setEnabled(new_state);
        mPBRRotate->setEnabled(new_state);
        mPBROffsetU->setEnabled(new_state);
        mPBROffsetV->setEnabled(new_state);

        // Control values will be set once per frame in
        // setMaterialOverridesFromSelection
        sMaterialOverrideSelection.setDirty();
    }
}

void LLPanelFace::updateVisibilityGLTF(LLViewerObject* objectp /*= nullptr */)
{
    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();
    const bool inventory_pending = objectp && objectp->isInventoryPending();

    mRadioPbrType->setVisible(show_pbr);

    const U32 pbr_type = mRadioPbrType->getSelectedIndex();
    const bool show_pbr_render_material_id = show_pbr && (pbr_type == PBRTYPE_RENDER_MATERIAL_ID);

    mPBRTextureCtrl->setVisible(show_pbr_render_material_id);

    mBtnPbrFromInv->setVisible(show_pbr_render_material_id);
    mBtnEditBbr->setVisible(show_pbr_render_material_id && !inventory_pending);
    mBtnSaveBbr->setVisible(show_pbr_render_material_id && !inventory_pending);
    mLabelMatPermLoading->setVisible(show_pbr_render_material_id && inventory_pending);

    mPBRScaleU->setVisible(show_pbr);
    mPBRScaleV->setVisible(show_pbr);
    mPBRRotate->setVisible(show_pbr);
    mPBROffsetU->setVisible(show_pbr);
    mPBROffsetV->setVisible(show_pbr);
}

void LLPanelFace::updateCopyTexButton()
{
    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    mMenuClipboardTexture->setEnabled(objectp && objectp->getPCode() == LL_PCODE_VOLUME && objectp->permModify()
                                                    && !objectp->isPermanentEnforced() && !objectp->isInventoryPending()
                                                    && (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1)
                                                    && LLMaterialEditor::canClipboardObjectsMaterial());
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
        mAddMedia->setEnabled(false);
        mTitleMediaText->clear();
        clearMediaSettings();
        return;
    }

    std::string url = first_object->getRegion()->getCapability("ObjectMedia");
    bool has_media_capability = (!url.empty());

    if (!has_media_capability)
    {
        mAddMedia->setEnabled(false);
        LL_WARNS("LLFloaterToolsMedia") << "Media not enabled (no capability) in this region!" << LL_ENDL;
        clearMediaSettings();
        return;
    }

    bool is_nonpermanent_enforced = (LLSelectMgr::getInstance()->getSelection()->getFirstRootNode()
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

    mAddMedia->setEnabled(editable);
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

        mDelMedia->setEnabled(bool_has_media && editable);
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

        mDelMedia->setEnabled(true);
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
void LLPanelFace::navigateToTitleMedia(const std::string& url)
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
        if (mTitleMedia->getCurrentNavUrl() != url || media_plugin == nullptr)
        {
            mTitleMedia->navigateTo(url);

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
    // set default to auto play true -- angela  EXT-5172
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
            //return mMediaEntry.getAutoPlay(); set default to auto play true -- angela  EXT-5172
            return true;
        };

        const LLMediaEntry &mMediaEntry;

    } func_auto_play(default_media_data);
    identical = selected_objects->getSelectedTEValue(&func_auto_play, value_bool);
    base_key = std::string(LLMediaEntry::AUTO_PLAY_KEY);
    mMediaSettings[base_key] = value_bool;
    mMediaSettings[base_key + std::string(LLPanelContents::TENTATIVE_SUFFIX)] = !identical;


    // Auto scale
    // set default to auto scale true -- angela  EXT-5172
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
            // return mMediaEntry.getAutoScale();  set default to auto scale true -- angela  EXT-5172
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

// static
F32 LLPanelFace::valueGlow(LLViewerObject* object, S32 face)
{
    return (F32)(object->getTE(face)->getGlow());
}

void LLPanelFace::onCommitColor()
{
    sendColor();
}

void LLPanelFace::onCommitShinyColor()
{
    LLSelectedTEMaterial::setSpecularLightColor(this, mShinyColorSwatch->get());
}

void LLPanelFace::onCommitAlpha()
{
    sendAlpha();
}

void LLPanelFace::onCancelColor()
{
    LLSelectMgr::getInstance()->selectionRevertColors();
}

void LLPanelFace::onCancelShinyColor()
{
    LLSelectMgr::getInstance()->selectionRevertShinyColors();
}

void LLPanelFace::onSelectColor()
{
    LLSelectMgr::getInstance()->saveSelectedObjectColors();
    sendColor();
}

void LLPanelFace::onSelectShinyColor()
{
    LLSelectedTEMaterial::setSpecularLightColor(this, mShinyColorSwatch->get());
    LLSelectMgr::getInstance()->saveSelectedShinyColors();
}

void LLPanelFace::onCommitMaterialsMedia()
{
    // Force to default states to side-step problems with menu contents
    // and generally reflecting old state when switching tabs or objects
    //
    updateShinyControls(false, true);
    updateBumpyControls(false, true);
    updateUI();
    refreshMedia();
}

void LLPanelFace::updateVisibility(LLViewerObject* objectp /* = nullptr */)
{
    if (!mRadioMaterialType || !mRadioPbrType)
    {
        LL_WARNS("Materials") << "Combo box not found...exiting." << LL_ENDL;
        return;
    }
    U32 materials_media = mComboMatMedia->getCurrentIndex();
    U32 material_type = mRadioMaterialType->getSelectedIndex();
    bool show_media = (materials_media == MATMEDIA_MEDIA) && mComboMatMedia->getEnabled();
    bool show_material = materials_media == MATMEDIA_MATERIAL;
    bool show_texture = (show_media || (show_material && (material_type == MATTYPE_DIFFUSE) && mComboMatMedia->getEnabled()));
    bool show_bumpiness = show_material && (material_type == MATTYPE_NORMAL) && mComboMatMedia->getEnabled();
    bool show_shininess = show_material && (material_type == MATTYPE_SPECULAR) && mComboMatMedia->getEnabled();
    const bool show_pbr = mComboMatMedia->getCurrentIndex() == MATMEDIA_PBR && mComboMatMedia->getEnabled();
    const LLGLTFMaterial::TextureInfo texture_info = getPBRTextureInfo();
    const bool show_pbr_asset = show_pbr && texture_info == LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT;

    mRadioMaterialType->setVisible(show_material);

    // Shared material controls
    mCheckSyncSettings->setVisible(show_material || show_media);
    mLabelTexGen->setVisible(show_material || show_media || show_pbr_asset);
    mComboTexGen->setVisible(show_material || show_media || show_pbr_asset);
    mBtnAlignTex->setVisible(show_material || show_media);

    // Media controls
    mTitleMediaText->setVisible(show_media);
    mAddMedia->setVisible(show_media);
    mDelMedia->setVisible(show_media);
    mBtnAlign->setVisible(show_media);

    // Diffuse texture controls
    mTextureCtrl->setVisible(show_texture && show_material);
    mLabelAlphaMode->setVisible(show_texture && show_material);
    mComboAlphaMode->setVisible(show_texture && show_material);
    mLabelMaskCutoff->setVisible(false);
    mMaskCutoff->setVisible(false);
    if (show_texture && show_material)
    {
        updateAlphaControls();
    }
    // texture scale and position controls
    mTexScaleU->setVisible(show_texture);
    mTexScaleV->setVisible(show_texture);
    mTexRotate->setVisible(show_texture);
    mTexOffsetU->setVisible(show_texture);
    mTexOffsetV->setVisible(show_texture);

    // Specular map controls
    mShinyTextureCtrl->setVisible(show_shininess);
    mComboShininess->setVisible(show_shininess);
    mLabelShininess->setVisible(show_shininess);
    mLabelGlossiness->setVisible(false);
    mGlossiness->setVisible(false);
    mLabelEnvironment->setVisible(false);
    mEnvironment->setVisible(false);
    mLabelShiniColor->setVisible(false);
    mShinyColorSwatch->setVisible(false);
    if (show_shininess)
    {
        updateShinyControls();
    }
    mShinyScaleU->setVisible(show_shininess);
    mShinyScaleV->setVisible(show_shininess);
    mShinyRotate->setVisible(show_shininess);
    mShinyOffsetU->setVisible(show_shininess);
    mShinyOffsetV->setVisible(show_shininess);

    // Normal map controls
    if (show_bumpiness)
    {
        updateBumpyControls();
    }
    mBumpyTextureCtrl->setVisible(show_bumpiness);
    mComboBumpiness->setVisible(show_bumpiness);
    mLabelBumpiness->setVisible(show_bumpiness);
    mBumpyScaleU->setVisible(show_bumpiness);
    mBumpyScaleV->setVisible(show_bumpiness);
    mBumpyRotate->setVisible(show_bumpiness);
    mBumpyOffsetU->setVisible(show_bumpiness);
    mBumpyOffsetV->setVisible(show_bumpiness);

    mTexRepeat->setVisible(show_material || show_media);

    // PBR controls
    updateVisibilityGLTF(objectp);
}

void LLPanelFace::onCommitMaterialType()
{
     // Force to default states to side-step problems with menu contents
     // and generally reflecting old state when switching tabs or objects
     //
     updateShinyControls(false, true);
     updateBumpyControls(false, true);
     updateUI();
}

void LLPanelFace::onCommitPbrType()
{
    // Force to default states to side-step problems with menu contents
    // and generally reflecting old state when switching tabs or objects
    //
    updateUI();
}

void LLPanelFace::onCommitBump()
{
    sendBump(mComboBumpiness->getCurrentIndex());
}

void LLPanelFace::onCommitTexGen()
{
    sendTexGen();
}

void LLPanelFace::updateShinyControls(bool is_setting_texture, bool mess_with_shiny_combobox)
{
    LLUUID shiny_texture_ID = mShinyTextureCtrl->getImageAssetID();
    LL_DEBUGS("Materials") << "Shiny texture selected: " << shiny_texture_ID << LL_ENDL;

    if (mess_with_shiny_combobox)
    {
        if (!shiny_texture_ID.isNull() && is_setting_texture)
        {
            if (!mComboShininess->itemExists(USE_TEXTURE))
            {
                mComboShininess->add(USE_TEXTURE);
            }
            mComboShininess->setSimple(USE_TEXTURE);
        }
        else
        {
            if (mComboShininess->itemExists(USE_TEXTURE))
            {
                mComboShininess->remove(SHINY_TEXTURE);
                mComboShininess->selectFirstItem();
            }
        }
    }
    else
    {
        if (shiny_texture_ID.isNull() && mComboShininess->itemExists(USE_TEXTURE))
        {
            mComboShininess->remove(SHINY_TEXTURE);
            mComboShininess->selectFirstItem();
        }
    }

    U32 materials_media = mComboMatMedia->getCurrentIndex();
    U32 material_type = mRadioMaterialType->getSelectedIndex();
    bool show_material = (materials_media == MATMEDIA_MATERIAL);
    bool show_shininess = show_material && (material_type == MATTYPE_SPECULAR) && mComboMatMedia->getEnabled();
    U32 shiny_value = mComboShininess->getCurrentIndex();
    bool show_shinyctrls = (shiny_value == SHINY_TEXTURE) && show_shininess; // Use texture
    mLabelGlossiness->setVisible(show_shinyctrls);
    mGlossiness->setVisible(show_shinyctrls);
    mLabelEnvironment->setVisible(show_shinyctrls);
    mEnvironment->setVisible(show_shinyctrls);
    mLabelShiniColor->setVisible(show_shinyctrls);
    mShinyColorSwatch->setVisible(show_shinyctrls);
}

void LLPanelFace::updateBumpyControls(bool is_setting_texture, bool mess_with_combobox)
{
    LLUUID bumpy_texture_ID = mBumpyTextureCtrl->getImageAssetID();
    LL_DEBUGS("Materials") << "texture: " << bumpy_texture_ID << (mess_with_combobox ? "" : " do not") << " update combobox" << LL_ENDL;

    if (mess_with_combobox)
    {
        if (!bumpy_texture_ID.isNull() && is_setting_texture)
        {
            if (!mComboBumpiness->itemExists(USE_TEXTURE))
            {
                mComboBumpiness->add(USE_TEXTURE);
            }
            mComboBumpiness->setSimple(USE_TEXTURE);
        }
        else
        {
            if (mComboBumpiness->itemExists(USE_TEXTURE))
            {
                mComboBumpiness->remove(BUMPY_TEXTURE);
                mComboBumpiness->selectFirstItem();
            }
        }
    }
}

void LLPanelFace::onCommitShiny()
{
    sendShiny(mComboShininess->getCurrentIndex());
}

void LLPanelFace::updateAlphaControls()
{
    U32 alpha_value = mComboAlphaMode->getCurrentIndex();
    bool show_alphactrls = (alpha_value == ALPHAMODE_MASK); // Alpha masking

    U32 mat_media = mComboMatMedia->getCurrentIndex();
    U32 mat_type = mRadioMaterialType->getSelectedIndex();

    show_alphactrls = show_alphactrls && (mat_media == MATMEDIA_MATERIAL);
    show_alphactrls = show_alphactrls && (mat_type == MATTYPE_DIFFUSE);

    mLabelMaskCutoff->setVisible(show_alphactrls);
    mMaskCutoff->setVisible(show_alphactrls);
}

void LLPanelFace::onCommitAlphaMode()
{
    updateAlphaControls();
    LLSelectedTEMaterial::setDiffuseAlphaMode(this, getCurrentDiffuseAlphaMode());
}

void LLPanelFace::onCommitFullbright()
{
    sendFullbright();
}

void LLPanelFace::onCommitGlow()
{
    sendGlow();
}

bool LLPanelFace::onDragPbr(LLInventoryItem* item)
{
    bool accept = true;
    for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
        iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* obj = node->getObject();
        if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
        {
            accept = false;
            break;
        }
    }
    return accept;
}

void LLPanelFace::onCommitPbr()
{
    if (!mPBRTextureCtrl->getTentative())
    {
        // we grab the item id first, because we want to do a
        // permissions check in the selection manager. ARGH!
        LLUUID id = mPBRTextureCtrl->getImageItemID();
        if (id.isNull())
        {
            id = mPBRTextureCtrl->getImageAssetID();
        }
        if (!LLSelectMgr::getInstance()->selectionSetGLTFMaterial(id))
        {
            // If failed to set material, refresh mPBRTextureCtrl's value
            refresh();
        }
    }
}

void LLPanelFace::onCancelPbr()
{
    LLSelectMgr::getInstance()->selectionRevertGLTFMaterials();
}

void LLPanelFace::onSelectPbr()
{
    LLSelectMgr::getInstance()->saveSelectedObjectTextures();

    if (!mPBRTextureCtrl->getTentative())
    {
        // we grab the item id first, because we want to do a
        // permissions check in the selection manager. ARGH!
        LLUUID id = mPBRTextureCtrl->getImageItemID();
        if (id.isNull())
        {
            id = mPBRTextureCtrl->getImageAssetID();
        }
        if (!LLSelectMgr::getInstance()->selectionSetGLTFMaterial(id))
        {
            refresh();
        }
    }
}

bool LLPanelFace::onDragTexture(LLInventoryItem* item)
{
    bool accept = true;
    for (LLObjectSelection::root_iterator iter = LLSelectMgr::getInstance()->getSelection()->root_begin();
        iter != LLSelectMgr::getInstance()->getSelection()->root_end(); iter++)
    {
        LLSelectNode* node = *iter;
        LLViewerObject* obj = node->getObject();
        if (!LLToolDragAndDrop::isInventoryDropAcceptable(obj, item))
        {
            accept = false;
            break;
        }
    }
    return accept;
}

void LLPanelFace::onCommitTexture()
{
    add(LLStatViewer::EDIT_TEXTURE, 1);
    sendTexture();
}

void LLPanelFace::onCancelTexture()
{
    LLSelectMgr::getInstance()->selectionRevertTextures();
}

void LLPanelFace::onSelectTexture()
{
    LLSelectMgr::getInstance()->saveSelectedObjectTextures();
    sendTexture();

    LLGLenum image_format;
    bool identical_image_format = false;
    bool missing_asset = false;
    LLSelectedTE::getImageFormat(image_format, identical_image_format, missing_asset);

    U32 alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
    if (!missing_asset)
    {
        switch (image_format)
        {
        case GL_RGBA:
        case GL_ALPHA:
            alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_BLEND;
            break;
        case GL_RGB:
            break;
        default:
            LL_WARNS() << "Unexpected tex format in LLPanelFace...resorting to no alpha" << LL_ENDL;
            break;
        }

        mComboAlphaMode->getSelectionInterface()->selectNthItem(alpha_mode);
    }

    LLSelectedTEMaterial::setDiffuseAlphaMode(this, getCurrentDiffuseAlphaMode());
}

void LLPanelFace::onCloseTexturePicker(const LLSD& data)
{
    LL_DEBUGS("Materials") << data << LL_ENDL;
    updateUI();
}

void LLPanelFace::onCommitSpecularTexture(const LLSD& data)
{
    LL_DEBUGS("Materials") << data << LL_ENDL;
    sendShiny(SHINY_TEXTURE);
}

void LLPanelFace::onCommitNormalTexture(const LLSD& data)
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
    LLUUID spec_map_id = mShinyTextureCtrl->getImageAssetID();
    shiny = spec_map_id.isNull() ? shiny : SHINY_TEXTURE;
    sendShiny(shiny);
}

void LLPanelFace::onCancelNormalTexture(const LLSD& data)
{
    U8 bumpy = 0;
    bool identical_bumpy = false;
    LLSelectedTE::getBumpmap(bumpy, identical_bumpy);
    LLUUID spec_map_id = mBumpyTextureCtrl->getImageAssetID();
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
void LLPanelFace::onClickBtnEditMedia()
{
    refreshMedia();
    LLFloaterReg::showInstance("media_settings");
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to delete media from a prim or prim face
void LLPanelFace::onClickBtnDeleteMedia()
{
    LLNotificationsUtil::add("DeleteMedia", LLSD(), LLSD(), deleteMediaConfirm);
}

//////////////////////////////////////////////////////////////////////////////
// called when a user wants to add media to a prim or prim face
void LLPanelFace::onClickBtnAddMedia()
{
    // check if multiple faces are selected
    if (LLSelectMgr::getInstance()->getSelection()->isMultipleTESelected())
    {
        refreshMedia();
        LLNotificationsUtil::add("MultipleFacesSelected", LLSD(), LLSD(), multipleFacesSelectedConfirm);
    }
    else
    {
        onClickBtnEditMedia();
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

void LLPanelFace::syncOffsetX(F32 offsetU)
{
    LLSelectedTEMaterial::setNormalOffsetX(this, offsetU);
    LLSelectedTEMaterial::setSpecularOffsetX(this, offsetU);
    mTexOffsetU->forceSetValue(LLSD(offsetU));
    sendTextureInfo();
}

void LLPanelFace::syncOffsetY(F32 offsetV)
{
    LLSelectedTEMaterial::setNormalOffsetY(this, offsetV);
    LLSelectedTEMaterial::setSpecularOffsetY(this, offsetV);
    mTexOffsetV->forceSetValue(LLSD(offsetV));
    sendTextureInfo();
}

void LLPanelFace::onCommitMaterialBumpyOffsetX()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetX(getCurrentBumpyOffsetU());
    }
    else
    {
        LLSelectedTEMaterial::setNormalOffsetX(this, getCurrentBumpyOffsetU());
    }
}

void LLPanelFace::onCommitMaterialBumpyOffsetY()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetY(getCurrentBumpyOffsetV());
    }
    else
    {
        LLSelectedTEMaterial::setNormalOffsetY(this, getCurrentBumpyOffsetV());
    }
}

void LLPanelFace::onCommitMaterialShinyOffsetX()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetX(getCurrentShinyOffsetU());
    }
    else
    {
        LLSelectedTEMaterial::setSpecularOffsetX(this, getCurrentShinyOffsetU());
    }
}

void LLPanelFace::onCommitMaterialShinyOffsetY()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetY(getCurrentShinyOffsetV());
    }
    else
    {
        LLSelectedTEMaterial::setSpecularOffsetY(this, getCurrentShinyOffsetV());
    }
}

void LLPanelFace::syncRepeatX(F32 scaleU)
{
    LLSelectedTEMaterial::setNormalRepeatX(this, scaleU);
    LLSelectedTEMaterial::setSpecularRepeatX(this, scaleU);
    sendTextureInfo();
}

void LLPanelFace::syncRepeatY(F32 scaleV)
{
    LLSelectedTEMaterial::setNormalRepeatY(this, scaleV);
    LLSelectedTEMaterial::setSpecularRepeatY(this, scaleV);
    sendTextureInfo();
}

void LLPanelFace::onCommitMaterialBumpyScaleX()
{
    F32 bumpy_scale_u = getCurrentBumpyScaleU();
    if (isIdenticalPlanarTexgen())
    {
        bumpy_scale_u *= 0.5f;
    }

    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexScaleU->forceSetValue(LLSD(getCurrentBumpyScaleU()));
        syncRepeatX(bumpy_scale_u);
    }
    else
    {
        LLSelectedTEMaterial::setNormalRepeatX(this, bumpy_scale_u);
    }
}

void LLPanelFace::onCommitMaterialBumpyScaleY()
{
    F32 bumpy_scale_v = getCurrentBumpyScaleV();
    if (isIdenticalPlanarTexgen())
    {
        bumpy_scale_v *= 0.5f;
    }

    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexScaleV->forceSetValue(LLSD(getCurrentBumpyScaleV()));
        syncRepeatY(bumpy_scale_v);
    }
    else
    {
        LLSelectedTEMaterial::setNormalRepeatY(this, bumpy_scale_v);
    }
}

void LLPanelFace::onCommitMaterialShinyScaleX()
{
    F32 shiny_scale_u = getCurrentShinyScaleU();
    if (isIdenticalPlanarTexgen())
    {
        shiny_scale_u *= 0.5f;
    }

    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexScaleU->forceSetValue(LLSD(getCurrentShinyScaleU()));
        syncRepeatX(shiny_scale_u);
    }
    else
    {
        LLSelectedTEMaterial::setSpecularRepeatX(this, shiny_scale_u);
    }
}

void LLPanelFace::onCommitMaterialShinyScaleY()
{
    F32 shiny_scale_v = getCurrentShinyScaleV();
    if (isIdenticalPlanarTexgen())
    {
        shiny_scale_v *= 0.5f;
    }

    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexScaleV->forceSetValue(LLSD(getCurrentShinyScaleV()));
        syncRepeatY(shiny_scale_v);
    }
    else
    {
        LLSelectedTEMaterial::setSpecularRepeatY(this, shiny_scale_v);
    }
}

void LLPanelFace::syncMaterialRot(F32 rot, int te)
{
    LLSelectedTEMaterial::setNormalRotation(this, rot * DEG_TO_RAD, te);
    LLSelectedTEMaterial::setSpecularRotation(this, rot * DEG_TO_RAD, te);
    sendTextureInfo();
}

void LLPanelFace::onCommitMaterialBumpyRot()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexRotate->forceSetValue(LLSD(getCurrentBumpyRot()));
        syncMaterialRot(getCurrentBumpyRot());
    }
    else
    {
        if (mPlanarAlign->getValue().asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setNormalRotation(this, getCurrentBumpyRot() * DEG_TO_RAD);
        }
    }
}

void LLPanelFace::onCommitMaterialShinyRot()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        mTexRotate->forceSetValue(LLSD(getCurrentShinyRot()));
        syncMaterialRot(getCurrentShinyRot());
    }
    else
    {
        if (mPlanarAlign->getValue().asBoolean())
        {
            LLFace* last_face = NULL;
            bool identical_face = false;
            LLSelectedTE::getFace(last_face, identical_face);
            LLPanelFaceSetAlignedTEFunctor setfunc(this, last_face);
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);
        }
        else
        {
            LLSelectedTEMaterial::setSpecularRotation(this, getCurrentShinyRot() * DEG_TO_RAD);
        }
    }
}

void LLPanelFace::onCommitMaterialGloss()
{
    LLSelectedTEMaterial::setSpecularLightExponent(this, getCurrentGlossiness());
}

void LLPanelFace::onCommitMaterialEnv()
{
    LLSelectedTEMaterial::setEnvironmentIntensity(this, getCurrentEnvIntensity());
}

void LLPanelFace::onCommitMaterialMaskCutoff()
{
    LLSelectedTEMaterial::setAlphaMaskCutoff(this, getCurrentAlphaMaskCutoff());
}

void LLPanelFace::onCommitTextureInfo()
{
    sendTextureInfo();
    // vertical scale and repeats per meter depends on each other, so force set on changes
    updateUI(true);
}

void LLPanelFace::onCommitTextureScaleX()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        F32 bumpy_scale_u = (F32)mTexScaleU->getValue().asReal();
        if (isIdenticalPlanarTexgen())
        {
            bumpy_scale_u *= 0.5f;
        }
        syncRepeatX(bumpy_scale_u);
    }
    else
    {
        sendTextureInfo();
    }
    updateUI(true);
}

void LLPanelFace::onCommitTextureScaleY()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        F32 bumpy_scale_v = (F32)mTexScaleV->getValue().asReal();
        if (isIdenticalPlanarTexgen())
        {
            bumpy_scale_v *= 0.5f;
        }
        syncRepeatY(bumpy_scale_v);
    }
    else
    {
        sendTextureInfo();
    }
    updateUI(true);
}

void LLPanelFace::onCommitTextureRot()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncMaterialRot((F32)mTexRotate->getValue().asReal());
    }
    else
    {
        sendTextureInfo();
    }
    updateUI(true);
}

void LLPanelFace::onCommitTextureOffsetX()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetX((F32)mTexOffsetU->getValue().asReal());
    }
    else
    {
        sendTextureInfo();
    }
    updateUI(true);
}

void LLPanelFace::onCommitTextureOffsetY()
{
    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        syncOffsetY((F32)mTexOffsetV->getValue().asReal());
    }
    else
    {
        sendTextureInfo();
    }
    updateUI(true);
}

// Commit the number of repeats per meter
void LLPanelFace::onCommitRepeatsPerMeter()
{
    F32 repeats_per_meter = (F32)mTexRepeat->getValue().asReal();

    F32 obj_scale_s = 1.0f;
    F32 obj_scale_t = 1.0f;

    bool identical_scale_s = false;
    bool identical_scale_t = false;

    LLSelectedTE::getObjectScaleS(obj_scale_s, identical_scale_s);
    LLSelectedTE::getObjectScaleS(obj_scale_t, identical_scale_t);

    if (gSavedSettings.getBOOL("SyncMaterialSettings"))
    {
        LLSelectMgr::getInstance()->selectionTexScaleAutofit(repeats_per_meter);

        mBumpyScaleU->setValue(obj_scale_s * repeats_per_meter);
        mBumpyScaleV->setValue(obj_scale_t * repeats_per_meter);

        LLSelectedTEMaterial::setNormalRepeatX(this, obj_scale_s * repeats_per_meter);
        LLSelectedTEMaterial::setNormalRepeatY(this, obj_scale_t * repeats_per_meter);

        mShinyScaleU->setValue(obj_scale_s * repeats_per_meter);
        mShinyScaleV->setValue(obj_scale_t * repeats_per_meter);

        LLSelectedTEMaterial::setSpecularRepeatX(this, obj_scale_s * repeats_per_meter);
        LLSelectedTEMaterial::setSpecularRepeatY(this, obj_scale_t * repeats_per_meter);
    }
    else
    {
        U32 material_type = mRadioMaterialType->getSelectedIndex();
        switch (material_type)
        {
        case MATTYPE_DIFFUSE:
            LLSelectMgr::getInstance()->selectionTexScaleAutofit(repeats_per_meter);
            break;
        case MATTYPE_NORMAL:
            mBumpyScaleU->setValue(obj_scale_s * repeats_per_meter);
            mBumpyScaleV->setValue(obj_scale_t * repeats_per_meter);

            LLSelectedTEMaterial::setNormalRepeatX(this, obj_scale_s * repeats_per_meter);
            LLSelectedTEMaterial::setNormalRepeatY(this, obj_scale_t * repeats_per_meter);
            break;
        case MATTYPE_SPECULAR:
            mBumpyScaleU->setValue(obj_scale_s * repeats_per_meter);
            mBumpyScaleV->setValue(obj_scale_t * repeats_per_meter);

            LLSelectedTEMaterial::setSpecularRepeatX(this, obj_scale_s * repeats_per_meter);
            LLSelectedTEMaterial::setSpecularRepeatY(this, obj_scale_t * repeats_per_meter);
            break;
        default:
            llassert(false);
            break;
        }
    }
    // vertical scale and repeats per meter depends on each other, so force set on changes
    updateUI(true);
}

struct LLPanelFaceSetMediaFunctor : public LLSelectedTEFunctor
{
    virtual bool apply(LLViewerObject* object, S32 te)
    {
        viewer_media_t pMediaImpl;

        const LLTextureEntry* tep = object->getTE(te);
        if (const LLMediaEntry* mep = tep->hasMedia() ? tep->getMediaData() : NULL)
        {
            pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(mep->getMediaID());
        }

        if (pMediaImpl.isNull())
        {
            // If we didn't find face media for this face, check whether this face is showing parcel media.
            pMediaImpl = LLViewerMedia::getInstance()->getMediaImplFromTextureID(tep->getID());
        }

        if (pMediaImpl.notNull())
        {
            if (LLPluginClassMedia* media = pMediaImpl->getMediaPlugin())
            {
                S32 media_width = media->getWidth();
                S32 media_height = media->getHeight();
                S32 texture_width = media->getTextureWidth();
                S32 texture_height = media->getTextureHeight();
                F32 scale_s = (F32)media_width / (F32)texture_width;
                F32 scale_t = (F32)media_height / (F32)texture_height;

                // set scale and adjust offset
                object->setTEScaleS(te, scale_s);
                object->setTEScaleT(te, scale_t); // don't need to flip Y anymore since QT does this for us now.
                object->setTEOffsetS(te, -( 1.0f - scale_s ) / 2.0f);
                object->setTEOffsetT(te, -( 1.0f - scale_t ) / 2.0f);
            }
        }
        return true;
    };
};

void LLPanelFace::onClickAutoFix()
{
    LLPanelFaceSetMediaFunctor setfunc;
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&setfunc);

    LLPanelFaceSendFunctor sendfunc;
    LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
}

void LLPanelFace::onAlignTexture()
{
    alignTextureLayer();
}

void LLPanelFace::onClickBtnLoadInvPBR()
{
    // Shouldn't this be "save to inventory?"
    mPBRTextureCtrl->showPicker(true);
}

void LLPanelFace::onClickBtnEditPBR()
{
    LLMaterialEditor::loadLive();
}

void LLPanelFace::onClickBtnSavePBR()
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
        || selected_count > 1
        || !LLMaterialEditor::canClipboardObjectsMaterial())
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
                te_data["te"]["texgen"] = tep->getTexGen();
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
                            mat_data["NormMap"] = DEFAULT_OBJECT_TEXTURE;
                            mat_data["NormMapNoCopy"] = true;
                        }

                    }
                    if (mat_data.has("SpecMap"))
                    {
                        LLUUID id = mat_data["SpecMap"].asUUID();
                        if (id.notNull() && !get_can_copy_texture(id))
                        {
                            mat_data["SpecMap"] = DEFAULT_OBJECT_TEXTURE;
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
        || selected_count > 1
        || !LLMaterialEditor::canClipboardObjectsMaterial())
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
                            LLUUID::null,
                            false);
                    }
                    else // one face
                    {
                        LLToolDragAndDrop::dropTextureOneFace(objectp,
                            te,
                            itemp_res,
                            from_library ? LLToolDragAndDrop::SOURCE_LIBRARY : LLToolDragAndDrop::SOURCE_AGENT,
                            LLUUID::null,
                            false,
                            0);
                    }
                }
                // not an inventory item or no complete items
                else if (full_perm)
                {
                    // Either library, local or existed as fullperm when user made a copy
                    LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(imageid, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
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
            if (te_data["te"].has("texgen"))
            {
                objectp->setTETexGen(te, (U8)te_data["te"]["texgen"].asInteger());
            }

            // PBR/GLTF
            if (te_data["te"].has("pbr"))
            {
                objectp->setRenderMaterialID(te, te_data["te"]["pbr"].asUUID(), false /*managing our own update*/);
                objectp->setTEGLTFMaterialOverride(te, nullptr);

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
                objectp->setTEGLTFMaterialOverride(te, nullptr);

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
            LLColor4U spec_color(te_data["material"]["SpecColor"]);
            LLSelectedTEMaterial::setSpecularLightColor(this, spec_color, te);
            LLSelectedTEMaterial::setSpecularLightExponent(this, (U8)te_data["material"]["SpecExp"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setEnvironmentIntensity(this, (U8)te_data["material"]["EnvIntensity"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setDiffuseAlphaMode(this, (U8)te_data["material"]["DiffuseAlphaMode"].asInteger(), te, object_id);
            LLSelectedTEMaterial::setAlphaMaskCutoff(this, (U8)te_data["material"]["AlphaMaskCutoff"].asInteger(), te, object_id);
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

void LLPanelFace::onCommitPlanarAlign()
{
    getState();
    sendTextureInfo();
}

void LLPanelFace::updateGLTFTextureTransform(std::function<void(LLGLTFMaterial::TextureTransform*)> edit)
{
    const LLGLTFMaterial::TextureInfo texture_info = getPBRTextureInfo();
    if (texture_info == LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT)
    {
        updateSelectedGLTFMaterials([&](LLGLTFMaterial* new_override)
            {
                for (U32 i = 0; i < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++i)
                {
                    LLGLTFMaterial::TextureTransform& new_transform = new_override->mTextureTransform[(LLGLTFMaterial::TextureInfo)i];
                    edit(&new_transform);
                }
            });
    }
    else
    {
        updateSelectedGLTFMaterials([&](LLGLTFMaterial* new_override)
            {
                LLGLTFMaterial::TextureTransform& new_transform = new_override->mTextureTransform[texture_info];
                edit(&new_transform);
            });
    }
}

void LLPanelFace::setMaterialOverridesFromSelection()
{
    const LLGLTFMaterial::TextureInfo texture_info = getPBRTextureInfo();
    U32 texture_info_start;
    U32 texture_info_end;
    if (texture_info == LLGLTFMaterial::TextureInfo::GLTF_TEXTURE_INFO_COUNT)
    {
        texture_info_start = 0;
        texture_info_end = LLGLTFMaterial::TextureInfo::GLTF_TEXTURE_INFO_COUNT;
    }
    else
    {
        texture_info_start = texture_info;
        texture_info_end = texture_info + 1;
    }

    bool read_transform = true;
    LLGLTFMaterial::TextureTransform transform;
    bool scale_u_same = true;
    bool scale_v_same = true;
    bool rotation_same = true;
    bool offset_u_same = true;
    bool offset_v_same = true;

    for (U32 i = texture_info_start; i < texture_info_end; ++i)
    {
        LLGLTFMaterial::TextureTransform this_transform;
        bool this_scale_u_same = true;
        bool this_scale_v_same = true;
        bool this_rotation_same = true;
        bool this_offset_u_same = true;
        bool this_offset_v_same = true;

        readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
        {
            return mat ? mat->mTextureTransform[i].mScale[VX] : 0.f;
        }, this_transform.mScale[VX], this_scale_u_same, true, 1e-3f);
        readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
        {
            return mat ? mat->mTextureTransform[i].mScale[VY] : 0.f;
        }, this_transform.mScale[VY], this_scale_v_same, true, 1e-3f);
        readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
        {
            return mat ? mat->mTextureTransform[i].mRotation : 0.f;
        }, this_transform.mRotation, this_rotation_same, true, 1e-3f);
        readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
        {
            return mat ? mat->mTextureTransform[i].mOffset[VX] : 0.f;
        }, this_transform.mOffset[VX], this_offset_u_same, true, 1e-3f);
        readSelectedGLTFMaterial<float>([&](const LLGLTFMaterial* mat)
        {
            return mat ? mat->mTextureTransform[i].mOffset[VY] : 0.f;
        }, this_transform.mOffset[VY], this_offset_v_same, true, 1e-3f);

        scale_u_same = scale_u_same && this_scale_u_same;
        scale_v_same = scale_v_same && this_scale_v_same;
        rotation_same = rotation_same && this_rotation_same;
        offset_u_same = offset_u_same && this_offset_u_same;
        offset_v_same = offset_v_same && this_offset_v_same;

        if (read_transform)
        {
            read_transform = false;
            transform = this_transform;
        }
        else
        {
            scale_u_same = scale_u_same && (this_transform.mScale[VX] == transform.mScale[VX]);
            scale_v_same = scale_v_same && (this_transform.mScale[VY] == transform.mScale[VY]);
            rotation_same = rotation_same && (this_transform.mRotation == transform.mRotation);
            offset_u_same = offset_u_same && (this_transform.mOffset[VX] == transform.mOffset[VX]);
            offset_v_same = offset_v_same && (this_transform.mOffset[VY] == transform.mOffset[VY]);
        }
    }

    mPBRScaleU->setValue(transform.mScale[VX]);
    mPBRScaleV->setValue(transform.mScale[VY]);
    mPBRRotate->setValue(transform.mRotation * RAD_TO_DEG);
    mPBROffsetU->setValue(transform.mOffset[VX]);
    mPBROffsetV->setValue(transform.mOffset[VY]);

    mPBRScaleU->setTentative(!scale_u_same);
    mPBRScaleV->setTentative(!scale_v_same);
    mPBRRotate->setTentative(!rotation_same);
    mPBROffsetU->setTentative(!offset_u_same);
    mPBROffsetV->setTentative(!offset_v_same);
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

void LLPanelFace::Selection::onSelectedObjectUpdated(const LLUUID& object_id, S32 side)
{
    if (object_id == mSelectedObjectID)
    {
        if (side == mLastSelectedSide)
        {
            mChanged = true;
        }
        else if (mLastSelectedSide == -1) // if last selected face was deselected
        {
            LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode();
            if (node && node->isTESelected(side))
            {
                mChanged = true;
            }
        }
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
    const S32 old_te_count = mSelectedTECount;
    const LLUUID old_object_id = mSelectedObjectID;
    const S32 old_side = mLastSelectedSide;

    LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
    LLSelectNode* node = selection->getFirstNode();
    if (node)
    {
        LLViewerObject* object = node->getObject();
        mSelectedObjectCount = selection->getObjectCount();
        mSelectedTECount = selection->getTECount();
        mSelectedObjectID = object->getID();
        mLastSelectedSide = node->getLastSelectedTE();
    }
    else
    {
        mSelectedObjectCount = 0;
        mSelectedTECount = 0;
        mSelectedObjectID = LLUUID::null;
        mLastSelectedSide = -1;
    }

    const bool selection_changed =
        old_object_count != mSelectedObjectCount
        || old_te_count != mSelectedTECount
        || old_object_id != mSelectedObjectID
        || old_side != mLastSelectedSide;
    mChanged = mChanged || selection_changed;
    return selection_changed;
}

void LLPanelFace::onCommitGLTFTextureScaleU()
{
    F32 value = (F32)mPBRScaleU->getValue().asReal();
    updateGLTFTextureTransform([&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mScale.mV[VX] = value;
    });
}

void LLPanelFace::onCommitGLTFTextureScaleV()
{
    F32 value = (F32)mPBRScaleV->getValue().asReal();
    updateGLTFTextureTransform([&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mScale.mV[VY] = value;
    });
}

void LLPanelFace::onCommitGLTFRotation()
{
    F32 value = (F32)mPBRRotate->getValue().asReal() * DEG_TO_RAD;
    updateGLTFTextureTransform([&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mRotation = value;
    });
}

void LLPanelFace::onCommitGLTFTextureOffsetU()
{
    F32 value = (F32)mPBROffsetU->getValue().asReal();
    updateGLTFTextureTransform([&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mOffset.mV[VX] = value;
    });
}

void LLPanelFace::onCommitGLTFTextureOffsetV()
{
    F32 value = (F32)mPBROffsetV->getValue().asReal();
    updateGLTFTextureTransform([&](LLGLTFMaterial::TextureTransform* new_transform)
    {
        new_transform->mOffset.mV[VY] = value;
    });
}

void LLPanelFace::onTextureSelectionChanged(LLInventoryItem* itemp)
{
    LL_DEBUGS("Materials") << "item asset " << itemp->getAssetUUID() << LL_ENDL;

    LLTextureCtrl* texture_ctrl;
    U32 mattype = mRadioMaterialType->getSelectedIndex();
    switch (mattype)
    {
        case MATTYPE_SPECULAR:
            texture_ctrl = mShinyTextureCtrl;
            break;
        case MATTYPE_NORMAL:
            texture_ctrl = mBumpyTextureCtrl;
            break;
        default:
            texture_ctrl = mTextureCtrl;
    }

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

void LLPanelFace::onPbrSelectionChanged(LLInventoryItem* itemp)
{
    if (mPBRTextureCtrl)
    {
        LLUUID obj_owner_id;
        std::string obj_owner_name;
        LLSelectMgr::instance().selectGetOwner(obj_owner_id, obj_owner_name);

        LLSaleInfo sale_info;
        LLSelectMgr::instance().selectGetSaleInfo(sale_info);

        bool can_copy = itemp->getPermissions().allowCopyBy(gAgentID); // do we have perm to copy this material?
        bool can_transfer = itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgentID); // do we have perm to transfer this material?
        bool can_modify = itemp->getPermissions().allowOperationBy(PERM_MODIFY, gAgentID); // do we have perm to transfer this material?
        bool is_object_owner = gAgentID == obj_owner_id; // does object for which we are going to apply material belong to the agent?
        bool not_for_sale = !sale_info.isForSale(); // is object for which we are going to apply material not for sale?
        bool from_library = ALEXANDRIA_LINDEN_ID == itemp->getPermissions().getOwner();

        if ((can_copy && can_transfer && can_modify) || from_library)
        {
            mPBRTextureCtrl->setCanApply(true, true);
            return;
        }

        // if material has (no-transfer) attribute it can be applied only for object which we own and is not for sale
        mPBRTextureCtrl->setCanApply(false, can_transfer ? true : is_object_owner && not_for_sale);

        if (gSavedSettings.getBOOL("TextureLivePreview"))
        {
            LLNotificationsUtil::add("LivePreviewUnavailablePBR");
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

void LLPanelFace::LLSelectedTE::getImageFormat(LLGLenum& image_format_to_return, bool& identical_face, bool& missing_asset)
{
    struct LLSelectedTEGetmatId : public LLSelectedTEFunctor
    {
        LLSelectedTEGetmatId()
            : mImageFormat(GL_RGB)
            , mIdentical(true)
            , mMissingAsset(false)
            , mFirstRun(true)
        {
        }
        bool apply(LLViewerObject* object, S32 te_index) override
        {
            LLViewerTexture* image = object ? object->getTEImage(te_index) : nullptr;
            LLGLenum format = GL_RGB;
            bool missing = false;
            if (image)
            {
                format = image->getPrimaryFormat();
                missing = image->isMissingAsset();
            }

            if (mFirstRun)
            {
                mFirstRun = false;
                mImageFormat = format;
                mMissingAsset = missing;
            }
            else
            {
                mIdentical &= (mImageFormat == format);
                mIdentical &= (mMissingAsset == missing);
            }
            return true;
        }
        LLGLenum mImageFormat;
        bool mIdentical;
        bool mMissingAsset;
        bool mFirstRun;
    } func;
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);

    image_format_to_return = func.mImageFormat;
    identical_face = func.mIdentical;
    missing_asset = func.mMissingAsset;
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

void LLPanelFace::LLSelectedTE::getPbrMaterialId(LLUUID& id, bool& identical, bool& has_faces_with_pbr, bool& has_faces_without_pbr)
{
    struct LLSelectedTEGetmatId : public LLSelectedTEFunctor
    {
        LLSelectedTEGetmatId()
            : mHasFacesWithoutPBR(false)
            , mHasFacesWithPBR(false)
            , mIdenticalId(true)
            , mIdenticalOverride(true)
            , mInitialized(false)
            , mMaterialOverride(LLGLTFMaterial::sDefault)
        {
        }
        bool apply(LLViewerObject* object, S32 te_index) override
        {
            LLUUID pbr_id = object->getRenderMaterialID(te_index);
            if (pbr_id.isNull())
            {
                mHasFacesWithoutPBR = true;
            }
            else
            {
                mHasFacesWithPBR = true;
            }
            if (mInitialized)
            {
                if (mPBRId != pbr_id)
                {
                    mIdenticalId = false;
                }

                LLGLTFMaterial* te_override = object->getTE(te_index)->getGLTFMaterialOverride();
                if (te_override)
                {
                    LLGLTFMaterial override = *te_override;
                    override.sanitizeAssetMaterial();
                    mIdenticalOverride &= (override == mMaterialOverride);
                }
                else
                {
                    mIdenticalOverride &= (mMaterialOverride == LLGLTFMaterial::sDefault);
                }
            }
            else
            {
                mInitialized = true;
                mPBRId = pbr_id;
                LLGLTFMaterial* override = object->getTE(te_index)->getGLTFMaterialOverride();
                if (override)
                {
                    mMaterialOverride = *override;
                    mMaterialOverride.sanitizeAssetMaterial();
                }
            }
            return true;
        }
        bool mHasFacesWithoutPBR;
        bool mHasFacesWithPBR;
        bool mIdenticalId;
        bool mIdenticalOverride;
        bool mInitialized;
        LLGLTFMaterial mMaterialOverride;
        LLUUID mPBRId;
    } func;
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);
    id = func.mPBRId;
    identical = func.mIdenticalId && func.mIdenticalOverride;
    has_faces_with_pbr = func.mHasFacesWithPBR;
    has_faces_without_pbr = func.mHasFacesWithoutPBR;
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
