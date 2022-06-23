/** 
 * @file llmaterialeditor.cpp
 * @brief Implementation of the notecard editor
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
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

#include "llmaterialeditor.h"

#include "llappviewer.h"
#include "llcombobox.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewermenufile.h"
#include "llviewertexture.h"
#include "llselectmgr.h"
#include "llvovolume.h"

#include "tinygltf/tiny_gltf.h"

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLMaterialEditor::LLMaterialEditor(const LLSD& key)
    : LLFloater(key)
{
}

BOOL LLMaterialEditor::postBuild()
{
    mAlbedoTextureCtrl = getChild<LLTextureCtrl>("albedo_texture");
    mMetallicTextureCtrl = getChild<LLTextureCtrl>("metallic_roughness_texture");
    mEmissiveTextureCtrl = getChild<LLTextureCtrl>("emissive_texture");
    mNormalTextureCtrl = getChild<LLTextureCtrl>("normal_texture");

    mAlbedoTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitAlbedoTexture, this, _1, _2));
    mMetallicTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitMetallicTexture, this, _1, _2));
    mEmissiveTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitEmissiveTexture, this, _1, _2));
    mNormalTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitNormalTexture, this, _1, _2));

    childSetAction("save", boost::bind(&LLMaterialEditor::onClickSave, this));
    childSetAction("save_as", boost::bind(&LLMaterialEditor::onClickSaveAs, this));
    childSetAction("cancel", boost::bind(&LLMaterialEditor::onClickCancel, this));

	return LLFloater::postBuild();
}

LLUUID LLMaterialEditor::getAlbedoId()
{
    return mAlbedoTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setAlbedoId(const LLUUID& id)
{
    mAlbedoTextureCtrl->setValue(id);
    mAlbedoTextureCtrl->setDefaultImageAssetID(id);

    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("albedo_upload_fee", getString("upload_fee_string"));
        // Only set if we will need to upload this texture
        mAlbedoTextureUploadId = id;
    }
}

LLColor4 LLMaterialEditor::getAlbedoColor()
{
    LLColor4 ret = LLColor4(childGetValue("albedo color"));
    ret.mV[3] = getTransparency();
    return ret;
}

void LLMaterialEditor::setAlbedoColor(const LLColor4& color)
{
    childSetValue("albedo color", color.getValue());
    childSetValue("transparency", color.mV[3]);
}

F32 LLMaterialEditor::getTransparency()
{
    return childGetValue("transparency").asReal();
}

std::string LLMaterialEditor::getAlphaMode()
{
    return childGetValue("alpha mode").asString();
}

void LLMaterialEditor::setAlphaMode(const std::string& alpha_mode)
{
    childSetValue("alpha mode", alpha_mode);
}

F32 LLMaterialEditor::getAlphaCutoff()
{
    return childGetValue("alpha cutoff").asReal();
}

void LLMaterialEditor::setAlphaCutoff(F32 alpha_cutoff)
{
    childSetValue("alpha cutoff", alpha_cutoff);
}

void LLMaterialEditor::setMaterialName(const std::string &name)
{
    setTitle(name);
    mMaterialName = name;
}

LLUUID LLMaterialEditor::getMetallicRoughnessId()
{
    return mMetallicTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setMetallicRoughnessId(const LLUUID& id)
{
    mMetallicTextureCtrl->setValue(id);
    mMetallicTextureCtrl->setDefaultImageAssetID(id);

    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("metallic_upload_fee", getString("upload_fee_string"));
        mMetallicTextureUploadId = id;
    }
}

F32 LLMaterialEditor::getMetalnessFactor()
{
    return childGetValue("metalness factor").asReal();
}

void LLMaterialEditor::setMetalnessFactor(F32 factor)
{
    childSetValue("metalness factor", factor);
}

F32 LLMaterialEditor::getRoughnessFactor()
{
    return childGetValue("roughness factor").asReal();
}

void LLMaterialEditor::setRoughnessFactor(F32 factor)
{
    childSetValue("roughness factor", factor);
}

LLUUID LLMaterialEditor::getEmissiveId()
{
    return mEmissiveTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setEmissiveId(const LLUUID& id)
{
    mEmissiveTextureCtrl->setValue(id);
    mEmissiveTextureCtrl->setDefaultImageAssetID(id);

    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("emissive_upload_fee", getString("upload_fee_string"));
        mEmissiveTextureUploadId = id;
    }
}

LLColor4 LLMaterialEditor::getEmissiveColor()
{
    return LLColor4(childGetValue("emissive color"));
}

void LLMaterialEditor::setEmissiveColor(const LLColor4& color)
{
    childSetValue("emissive color", color.getValue());
}

LLUUID LLMaterialEditor::getNormalId()
{
    return mNormalTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setNormalId(const LLUUID& id)
{
    mNormalTextureCtrl->setValue(id);
    mNormalTextureCtrl->setDefaultImageAssetID(id);

    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("normal_upload_fee", getString("upload_fee_string"));
        mNormalTextureUploadId = id;
    }
}

bool LLMaterialEditor::getDoubleSided()
{
    return childGetValue("double sided").asBoolean();
}

void LLMaterialEditor::setDoubleSided(bool double_sided)
{
    childSetValue("double sided", double_sided);
}

void LLMaterialEditor::onCommitAlbedoTexture(LLUICtrl * ctrl, const LLSD & data)
{
    // might be better to use arrays, to have a single callback
    // and not to repeat the same thing for each tecture controls
    LLUUID new_val = mAlbedoTextureCtrl->getValue().asUUID();
    if (new_val == mAlbedoTextureUploadId && mAlbedoTextureUploadId.notNull())
    {
        childSetValue("albedo_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        childSetValue("albedo_upload_fee", getString("no_upload_fee_string"));
    }
}

void LLMaterialEditor::onCommitMetallicTexture(LLUICtrl * ctrl, const LLSD & data)
{
    LLUUID new_val = mMetallicTextureCtrl->getValue().asUUID();
    if (new_val == mMetallicTextureUploadId && mMetallicTextureUploadId.notNull())
    {
        childSetValue("metallic_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        childSetValue("metallic_upload_fee", getString("no_upload_fee_string"));
    }
}

void LLMaterialEditor::onCommitEmissiveTexture(LLUICtrl * ctrl, const LLSD & data)
{
    LLUUID new_val = mEmissiveTextureCtrl->getValue().asUUID();
    if (new_val == mEmissiveTextureUploadId && mEmissiveTextureUploadId.notNull())
    {
        childSetValue("emissive_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        childSetValue("emissive_upload_fee", getString("no_upload_fee_string"));
    }
}

void LLMaterialEditor::onCommitNormalTexture(LLUICtrl * ctrl, const LLSD & data)
{
    LLUUID new_val = mNormalTextureCtrl->getValue().asUUID();
    if (new_val == mNormalTextureUploadId && mNormalTextureUploadId.notNull())
    {
        childSetValue("normal_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        childSetValue("normal_upload_fee", getString("no_upload_fee_string"));
    }
}


static void write_color(const LLColor4& color, std::vector<double>& c)
{
    for (int i = 0; i < c.size(); ++i) // NOTE -- use c.size because some gltf colors are 3-component
    {
        c[i] = color.mV[i];
    }
}

static U32 write_texture(const LLUUID& id, tinygltf::Model& model)
{
    tinygltf::Image image;
    image.uri = id.asString();
    model.images.push_back(image);
    U32 image_idx = model.images.size() - 1;

    tinygltf::Texture texture;
    texture.source = image_idx;
    model.textures.push_back(texture);
    U32 texture_idx = model.textures.size() - 1;

    return texture_idx;
}

void LLMaterialEditor::onClickSave()
{
    applyToSelection();

    tinygltf::Model model;
    model.materials.resize(1);
    tinygltf::PbrMetallicRoughness& pbrMaterial = model.materials[0].pbrMetallicRoughness;

    // write albedo
    LLColor4 albedo_color = getAlbedoColor();
    albedo_color.mV[3] = getTransparency();
    write_color(albedo_color, pbrMaterial.baseColorFactor);

    model.materials[0].alphaCutoff = getAlphaCutoff();
    model.materials[0].alphaMode = getAlphaMode();

    LLUUID albedo_id = getAlbedoId();
    
    if (albedo_id.notNull())
    {
        U32 texture_idx = write_texture(albedo_id, model);
        
        pbrMaterial.baseColorTexture.index = texture_idx;
    }

    // write metallic/roughness
    F32 metalness = getMetalnessFactor();
    F32 roughness = getRoughnessFactor();

    pbrMaterial.metallicFactor = metalness;
    pbrMaterial.roughnessFactor = roughness;
    
    LLUUID mr_id = getMetallicRoughnessId();
    if (mr_id.notNull())
    {
        U32 texture_idx = write_texture(mr_id, model);
        pbrMaterial.metallicRoughnessTexture.index = texture_idx;
    }
    
    //write emissive
    LLColor4 emissive_color = getEmissiveColor();
    model.materials[0].emissiveFactor.resize(3);
    write_color(emissive_color, model.materials[0].emissiveFactor);

    LLUUID emissive_id = getEmissiveId();
    if (emissive_id.notNull())
    {
        U32 idx = write_texture(emissive_id, model);
        model.materials[0].emissiveTexture.index = idx;
    }

    //write normal
    LLUUID normal_id = getNormalId();
    if (normal_id.notNull())
    {
        U32 idx = write_texture(normal_id, model);
        model.materials[0].normalTexture.index = idx;
    }

    //write doublesided
    model.materials[0].doubleSided = getDoubleSided();

    std::ostringstream str;

    tinygltf::TinyGLTF gltf;
    model.asset.version = "2.0";
    gltf.WriteGltfSceneToStream(&model, str, true, false);
    
    std::string dump = str.str();

    LL_INFOS() << mMaterialName << ": " << dump << LL_ENDL;
}

void LLMaterialEditor::onClickSaveAs()
{
    LLSD args;
    args["DESC"] = mMaterialName;

    LLNotificationsUtil::add("SaveMaterialAs", args, LLSD(), boost::bind(&LLMaterialEditor::onSaveAsCommitCallback, this, _1, _2));
}

void LLMaterialEditor::onSaveAsCommitCallback(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (0 == option)
    {
        std::string new_name = response["message"].asString();
        LLStringUtil::trim(new_name);
        if (!new_name.empty())
        {
            setMaterialName(new_name);
            onClickSave();
        }
        else
        {
            LLNotificationsUtil::add("InvalidMaterialName");
        }
    }
}

void LLMaterialEditor::onClickCancel()
{
    // Todo: confirm usaved changes
    closeFloater();
}

class LLMaterialFilePicker : public LLFilePickerThread
{
public:
    LLMaterialFilePicker(LLMaterialEditor* me);
    virtual void notify(const std::vector<std::string>& filenames);
    void loadMaterial(const std::string& filename);
    static void	textureLoadedCallback(BOOL success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata);
private:
    LLMaterialEditor* mME;
};

LLMaterialFilePicker::LLMaterialFilePicker(LLMaterialEditor* me)
    : LLFilePickerThread(LLFilePicker::FFLOAD_MATERIAL)
{
    mME = me;
}

void LLMaterialFilePicker::notify(const std::vector<std::string>& filenames)
{
    if (LLAppViewer::instance()->quitRequested())
    {
        return;
    }

    
    if (filenames.size() > 0)
    {
        loadMaterial(filenames[0]);
    }
}

const tinygltf::Image* get_image_from_texture_index(const tinygltf::Model& model, S32 texture_index)
{
    if (texture_index >= 0)
    {
        S32 source_idx = model.textures[texture_index].source;
        if (source_idx >= 0)
        {
            return &(model.images[source_idx]);
        }
    }

    return nullptr;
}

static LLViewerFetchedTexture* get_texture(const std::string& folder, const tinygltf::Model& model, S32 texture_index)
{
    LLViewerFetchedTexture* ret = nullptr;

    const tinygltf::Image* image = get_image_from_texture_index(model, texture_index);

    if (image != nullptr && 
        image->bits == 8 &&
        !image->image.empty() &&
        image->component <= 4)
    {
        LLPointer<LLImageRaw> rawImage = new LLImageRaw(&image->image[0], image->width, image->height, image->component);
        rawImage->verticalFlip();
        
        ret = LLViewerTextureManager::getFetchedTexture(rawImage, FTType::FTT_LOCAL_FILE, true);

        ret->forceToSaveRawImage(0, F32_MAX);
    }

    // TODO: provide helpful error message if image fails to load

    return ret;
}

static LLColor4 get_color(const std::vector<double>& in)
{
    LLColor4 out;
    for (S32 i = 0; i < llmin((S32) in.size(), 4); ++i)
    {
        out.mV[i] = in[i];
    }

    return out;
}

void LLMaterialFilePicker::textureLoadedCallback(BOOL success, LLViewerFetchedTexture* src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata)
{
}


void LLMaterialFilePicker::loadMaterial(const std::string& filename)
{
    tinygltf::TinyGLTF loader;
    std::string        error_msg;
    std::string        warn_msg;

    bool loaded = false;
    tinygltf::Model model_in;

    std::string filename_lc = filename;
    LLStringUtil::toLower(filename_lc);

    // Load a tinygltf model fom a file. Assumes that the input filename has already been
    // been sanitized to one of (.gltf , .glb) extensions, so does a simple find to distinguish.
    if (std::string::npos == filename_lc.rfind(".gltf"))
    {  // file is binary
        loaded = loader.LoadBinaryFromFile(&model_in, &error_msg, &warn_msg, filename);
    }
    else
    {  // file is ascii
        loaded = loader.LoadASCIIFromFile(&model_in, &error_msg, &warn_msg, filename);
    }

    if (!loaded)
    {
        LLNotificationsUtil::add("CannotUploadMaterial");
        return;
    }

    if (model_in.materials.empty())
    {
        // materials are missing
        LLNotificationsUtil::add("CannotUploadMaterial");
        return;
    }

    std::string folder = gDirUtilp->getDirName(filename);


    tinygltf::Material material_in = model_in.materials[0];

    tinygltf::Model  model_out;
    model_out.asset.version = "2.0";
    model_out.materials.resize(1);

    // get albedo texture
    LLPointer<LLViewerFetchedTexture> albedo_tex = get_texture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index);

    LLUUID albedo_id;
    if (albedo_tex != nullptr)
    {
        albedo_id = albedo_tex->getID();
    }

    // get metallic-roughness texture
    LLPointer<LLViewerFetchedTexture> mr_tex = get_texture(folder, model_in, material_in.pbrMetallicRoughness.metallicRoughnessTexture.index);

    LLUUID mr_id;
    if (mr_tex != nullptr)
    {
        mr_id = mr_tex->getID();
    }

    // get emissive texture
    LLPointer<LLViewerFetchedTexture> emissive_tex = get_texture(folder, model_in, material_in.emissiveTexture.index);

    LLUUID emissive_id;
    if (emissive_tex != nullptr)
    {
        emissive_id = emissive_tex->getID();
    }

    // get normal map
    LLPointer<LLViewerFetchedTexture> normal_tex = get_texture(folder, model_in, material_in.normalTexture.index);

    LLUUID normal_id;
    if (normal_tex != nullptr)
    {
        normal_id = normal_tex->getID();
    }

    mME->setAlbedoId(albedo_id);
    mME->setMetallicRoughnessId(mr_id);
    mME->setEmissiveId(emissive_id);
    mME->setNormalId(normal_id);

    mME->setAlphaMode(material_in.alphaMode);
    mME->setAlphaCutoff(material_in.alphaCutoff);
    
    mME->setAlbedoColor(get_color(material_in.pbrMetallicRoughness.baseColorFactor));
    mME->setEmissiveColor(get_color(material_in.emissiveFactor));
    
    mME->setMetalnessFactor(material_in.pbrMetallicRoughness.metallicFactor);
    mME->setRoughnessFactor(material_in.pbrMetallicRoughness.roughnessFactor);

    mME->setDoubleSided(material_in.doubleSided);

    std::string new_material = LLTrans::getString("New Material");
    mME->setMaterialName(new_material);

    mME->openFloater();

    mME->applyToSelection();
}

void LLMaterialEditor::importMaterial()
{
    (new LLMaterialFilePicker(this))->getFile();
}

void LLMaterialEditor::applyToSelection()
{
    LLViewerObject* objectp = LLSelectMgr::instance().getSelection()->getFirstObject();
    if (objectp && objectp->getVolume())
    {
        LLGLTFMaterial* mat = new LLGLTFMaterial();
        mat->mAlbedoColor = getAlbedoColor();
        mat->mAlbedoColor.mV[3] = getTransparency();
        mat->mAlbedoId = getAlbedoId();
        
        mat->mNormalId = getNormalId();

        mat->mMetallicRoughnessId = getMetallicRoughnessId();
        mat->mMetallicFactor = getMetalnessFactor();
        mat->mRoughnessFactor = getRoughnessFactor();
        
        mat->mEmissiveColor = getEmissiveColor();
        mat->mEmissiveId = getEmissiveId();
        
        mat->mDoubleSided = getDoubleSided();
        mat->setAlphaMode(getAlphaMode());

        LLVOVolume* vobjp = (LLVOVolume*)objectp;
        for (int i = 0; i < vobjp->getNumTEs(); ++i)
        {
            vobjp->getTE(i)->setGLTFMaterial(mat);
            vobjp->updateTEMaterialTextures(i);
        }

        vobjp->markForUpdate(TRUE);
    }
}
