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

#include "llagent.h"
#include "llappviewer.h"
#include "llcombobox.h"
#include "llinventorymodel.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewermenufile.h"
#include "llviewertexture.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewerregion.h"
#include "llvovolume.h"
#include "roles_constants.h"
#include "tinygltf/tiny_gltf.h"
#include "llviewerobjectlist.h"
#include "llfloaterreg.h"
#include "llfilesystem.h"
#include "llsdserialize.h"
#include "llimagej2c.h"
#include "llviewertexturelist.h"
#include "llfloaterperms.h"

#include <strstream>

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLMaterialEditor::LLMaterialEditor(const LLSD& key)
    : LLPreview(key)
    , mHasUnsavedChanges(false)
{
    const LLInventoryItem* item = getItem();
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
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

    boost::function<void(LLUICtrl*, void*)> changes_callback = [this](LLUICtrl * ctrl, void*)
    {
        setHasUnsavedChanges(true);
        // Apply changes to object live
        applyToSelection();
    };
 
    childSetCommitCallback("double sided", changes_callback, NULL);

    // Albedo
    childSetCommitCallback("albedo color", changes_callback, NULL);
    childSetCommitCallback("transparency", changes_callback, NULL);
    childSetCommitCallback("alpha mode", changes_callback, NULL);
    childSetCommitCallback("alpha cutoff", changes_callback, NULL);

    // Metallic-Roughness
    childSetCommitCallback("metalness factor", changes_callback, NULL);
    childSetCommitCallback("roughness factor", changes_callback, NULL);

    // Metallic-Roughness
    childSetCommitCallback("metalness factor", changes_callback, NULL);
    childSetCommitCallback("roughness factor", changes_callback, NULL);

    // Emissive
    childSetCommitCallback("emissive color", changes_callback, NULL);

    childSetVisible("unsaved_changes", mHasUnsavedChanges);

    // Todo:
    // Disable/enable setCanApplyImmediately() based on
    // working from inventory, upload or editing inworld

	return LLPreview::postBuild();
}

void LLMaterialEditor::onClickCloseBtn(bool app_quitting)
{
    if (app_quitting)
    {
        closeFloater(app_quitting);
    }
    else
    {
        onClickCancel();
    }
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
    setTransparency(color.mV[3]);
}

F32 LLMaterialEditor::getTransparency()
{
    return childGetValue("transparency").asReal();
}

void LLMaterialEditor::setTransparency(F32 transparency)
{
    childSetValue("transparency", transparency);
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

void LLMaterialEditor::setHasUnsavedChanges(bool value)
{
    if (value != mHasUnsavedChanges)
    {
        mHasUnsavedChanges = value;
        childSetVisible("unsaved_changes", value);
    }
}

void LLMaterialEditor::onCommitAlbedoTexture(LLUICtrl * ctrl, const LLSD & data)
{
    // might be better to use arrays, to have a single callback
    // and not to repeat the same thing for each tecture control
    LLUUID new_val = mAlbedoTextureCtrl->getValue().asUUID();
    if (new_val == mAlbedoTextureUploadId && mAlbedoTextureUploadId.notNull())
    {
        childSetValue("albedo_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        mAlbedoJ2C = nullptr;
        childSetValue("albedo_upload_fee", getString("no_upload_fee_string"));
    }
    setHasUnsavedChanges(true);
    applyToSelection();
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
        mMetallicRoughnessJ2C = nullptr;
        childSetValue("metallic_upload_fee", getString("no_upload_fee_string"));
    }
    setHasUnsavedChanges(true);
    applyToSelection();
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
        mEmissiveJ2C = nullptr;
        childSetValue("emissive_upload_fee", getString("no_upload_fee_string"));
    }
    setHasUnsavedChanges(true);
    applyToSelection();
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
        mNormalJ2C = nullptr;
        childSetValue("normal_upload_fee", getString("no_upload_fee_string"));
    }
    setHasUnsavedChanges(true);
    applyToSelection();
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
    saveIfNeeded();
}


std::string LLMaterialEditor::getGLTFJson(bool prettyprint)
{
    tinygltf::Model model;
    getGLTFModel(model);

    std::ostringstream str;

    tinygltf::TinyGLTF gltf;
    
    gltf.WriteGltfSceneToStream(&model, str, prettyprint, false);

    std::string dump = str.str();

    return dump;
}

void LLMaterialEditor::getGLBData(std::vector<U8>& data)
{
    tinygltf::Model model;
    getGLTFModel(model);

    std::ostringstream str;

    tinygltf::TinyGLTF gltf;

    gltf.WriteGltfSceneToStream(&model, str, false, true);

    std::string dump = str.str();

    data.resize(dump.length());

    memcpy(&data[0], dump.c_str(), dump.length());
}

void LLMaterialEditor::getGLTFModel(tinygltf::Model& model)
{
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

    model.asset.version = "2.0";
}

std::string LLMaterialEditor::getEncodedAsset()
{
    LLSD asset;
    asset["version"] = "1.0";
    asset["type"] = "GLTF 2.0";
    asset["data"] = getGLTFJson(false);

    std::ostringstream str;
    LLSDSerialize::serialize(asset, str, LLSDSerialize::LLSD_BINARY);

    return str.str();
}

bool LLMaterialEditor::decodeAsset(const std::vector<char>& buffer)
{
    LLSD asset;
    
    std::istrstream str(&buffer[0], buffer.size());
    if (LLSDSerialize::deserialize(asset, str, buffer.size()))
    {
        if (asset.has("version") && asset["version"] == "1.0")
        {
            if (asset.has("type") && asset["type"] == "GLTF 2.0")
            {
                if (asset.has("data") && asset["data"].isString())
                {
                    std::string data = asset["data"];

                    tinygltf::TinyGLTF gltf;
                    tinygltf::TinyGLTF loader;
                    std::string        error_msg;
                    std::string        warn_msg;

                    tinygltf::Model model_in;

                    if (loader.LoadASCIIFromString(&model_in, &error_msg, &warn_msg, data.c_str(), data.length(), ""))
                    {
                        return setFromGltfModel(model_in, true);
                    }
                    else
                    {
                        LL_WARNS() << "Failed to decode material asset: " << LL_ENDL;
                        LL_WARNS() << warn_msg << LL_ENDL;
                        LL_WARNS() << error_msg << LL_ENDL;
                    }
                }
            }
        }
    }
    else
    {
        LL_WARNS() << "Failed to deserialize material LLSD" << LL_ENDL;
    }

    return false;
}

bool LLMaterialEditor::saveIfNeeded(LLInventoryItem* copyitem, bool sync)
{
    std::string buffer = getEncodedAsset();
    
    saveTextures();

    const LLInventoryItem* item = getItem();
    // save it out to database
    if (item)
    {
        const LLViewerRegion* region = gAgent.getRegion();
        if (!region)
        {
            LL_WARNS() << "Not connected to a region, cannot save material." << LL_ENDL;
            return false;
        }
        std::string agent_url = region->getCapability("UpdateMaterialAgentInventory");
        std::string task_url = region->getCapability("UpdateMaterialTaskInventory");

        if (!agent_url.empty() && !task_url.empty())
        {
            std::string url;
            LLResourceUploadInfo::ptr_t uploadInfo;

            if (mObjectUUID.isNull() && !agent_url.empty())
            {
                uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mItemUUID, LLAssetType::AT_MATERIAL, buffer,
                    [](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD) {
                        LLMaterialEditor::finishInventoryUpload(itemId, newAssetId, newItemId);
                    });
                url = agent_url;
            }
            else if (!mObjectUUID.isNull() && !task_url.empty())
            {
                LLUUID object_uuid(mObjectUUID);
                uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(mObjectUUID, mItemUUID, LLAssetType::AT_MATERIAL, buffer,
                    [object_uuid](LLUUID itemId, LLUUID, LLUUID newAssetId, LLSD) {
                        LLMaterialEditor::finishTaskUpload(itemId, newAssetId, object_uuid);
                    });
                url = task_url;
            }

            if (!url.empty() && uploadInfo)
            {
                mAssetStatus = PREVIEW_ASSET_LOADING;
                setEnabled(false);

                LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
            }

        }
        else // !gAssetStorage
        {
            LL_WARNS() << "Not connected to an materials capable region." << LL_ENDL;
            return false;
        }

        if (mCloseAfterSave)
        {
            closeFloater();
        }
    }
    else
    { //make a new inventory item
#if 1
        // gen a new uuid for this asset
        LLTransactionID tid;
        tid.generate();     // timestamp-based randomization + uniquification
        LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
        std::string res_desc = "Saved Material";
        U32 next_owner_perm = LLPermissions::DEFAULT.getMaskNextOwner();
        LLUUID parent = gInventory.findCategoryUUIDForType(LLFolderType::FT_MATERIAL);
        const U8 subtype = NO_INV_SUBTYPE;  // TODO maybe use AT_SETTINGS and LLSettingsType::ST_MATERIAL ?

        create_inventory_item(gAgent.getID(), gAgent.getSessionID(), parent, tid, mMaterialName, res_desc,
            LLAssetType::AT_MATERIAL, LLInventoryType::IT_MATERIAL, subtype, next_owner_perm,
            new LLBoostFuncInventoryCallback([output = buffer](LLUUID const& inv_item_id) {
                // from reference in LLSettingsVOBase::createInventoryItem()/updateInventoryItem()
                LLResourceUploadInfo::ptr_t uploadInfo =
                    std::make_shared<LLBufferedAssetUploadInfo>(
                        inv_item_id,
                        LLAssetType::AT_MATERIAL,
                        output,
                        [](LLUUID item_id, LLUUID new_asset_id, LLUUID new_item_id, LLSD response) {
                            LL_INFOS("Material") << "inventory item uploaded.  item: " << item_id << " asset: " << new_asset_id << " new_item_id: " << new_item_id << " response: " << response << LL_ENDL;
                            LLSD params = llsd::map("ASSET_ID", new_asset_id);
                            LLNotificationsUtil::add("MaterialCreated", params);
                        });

                const LLViewerRegion* region = gAgent.getRegion();
                if (region)
                {
                    std::string agent_url(region->getCapability("UpdateMaterialAgentInventory"));
                    if (agent_url.empty())
                    {
                        LL_ERRS() << "missing required agent inventory cap url" << LL_ENDL;
                    }
                    LLViewerAssetUpload::EnqueueInventoryUpload(agent_url, uploadInfo);
                }
                })
        );
#endif
    }
    
    return true;
}

void LLMaterialEditor::finishInventoryUpload(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId)
{
    // Update the UI with the new asset.
    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", LLSD(itemId));
    if (me)
    {
        if (newItemId.isNull())
        {
            me->setAssetId(newAssetId);
            me->refreshFromInventory();
        }
        else
        {
            me->refreshFromInventory(newItemId);
        }
    }
}

void LLMaterialEditor::finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId)
{

    LLSD floater_key;
    floater_key["taskid"] = taskId;
    floater_key["itemid"] = itemId;
    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", LLSD(itemId));
    if (me)
    {
        me->setAssetId(newAssetId);
        me->refreshFromInventory();
    }
}

void LLMaterialEditor::refreshFromInventory(const LLUUID& new_item_id)
{
    if (new_item_id.notNull())
    {
        mItemUUID = new_item_id;
        setKey(LLSD(new_item_id));
    }
    LL_DEBUGS() << "LLPreviewNotecard::refreshFromInventory()" << LL_ENDL;
    loadAsset();
}


void LLMaterialEditor::onClickSaveAs()
{
    LLSD args;
    args["DESC"] = mMaterialName;

    LLNotificationsUtil::add("SaveMaterialAs", args, LLSD(), boost::bind(&LLMaterialEditor::onSaveAsMsgCallback, this, _1, _2));
}

void LLMaterialEditor::onSaveAsMsgCallback(const LLSD& notification, const LLSD& response)
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
    if (mHasUnsavedChanges)
    {
        LLNotificationsUtil::add("UsavedMaterialChanges", LLSD(), LLSD(), boost::bind(&LLMaterialEditor::onCancelMsgCallback, this, _1, _2));
    }
    else
    {
        closeFloater();
    }
}

void LLMaterialEditor::onCancelMsgCallback(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (0 == option)
    {
        closeFloater();
    }
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

static LLImageRaw* get_texture(const std::string& folder, const tinygltf::Model& model, S32 texture_index, std::string& name)
{
    const tinygltf::Image* image = get_image_from_texture_index(model, texture_index);
    LLImageRaw* rawImage = nullptr;

    if (image != nullptr && 
        image->bits == 8 &&
        !image->image.empty() &&
        image->component <= 4)
    {
        name = image->name;
        rawImage = new LLImageRaw(&image->image[0], image->width, image->height, image->component);
        rawImage->verticalFlip();
    }

    return rawImage;
}

static void strip_alpha_channel(LLPointer<LLImageRaw>& img)
{
    if (img->getComponents() == 4)
    {
        LLImageRaw* tmp = new LLImageRaw(img->getWidth(), img->getHeight(), 3);
        tmp->copyUnscaled4onto3(img);
        img = tmp;
    }
}

// copy red channel from src_img to dst_img
// PRECONDITIONS:
// dst_img must be 3 component
// src_img and dst_image must have the same dimensions
static void copy_red_channel(LLPointer<LLImageRaw>& src_img, LLPointer<LLImageRaw>& dst_img)
{
    llassert(src_img->getWidth() == dst_img->getWidth() && src_img->getHeight() == dst_img->getHeight());
    llassert(dst_img->getComponents() == 3);

    U32 pixel_count = dst_img->getWidth() * dst_img->getHeight();
    U8* src = src_img->getData();
    U8* dst = dst_img->getData();
    S8 src_components = src_img->getComponents();

    for (U32 i = 0; i < pixel_count; ++i)
    {
        dst[i * 3] = src[i * src_components];
    }
}

static void pack_textures(tinygltf::Model& model, tinygltf::Material& material, 
    LLPointer<LLImageRaw>& albedo_img,
    LLPointer<LLImageRaw>& normal_img,
    LLPointer<LLImageRaw>& mr_img,
    LLPointer<LLImageRaw>& emissive_img,
    LLPointer<LLImageRaw>& occlusion_img,
    LLPointer<LLViewerFetchedTexture>& albedo_tex,
    LLPointer<LLViewerFetchedTexture>& normal_tex,
    LLPointer<LLViewerFetchedTexture>& mr_tex,
    LLPointer<LLViewerFetchedTexture>& emissive_tex,
    LLPointer<LLImageJ2C>& albedo_j2c,
    LLPointer<LLImageJ2C>& normal_j2c,
    LLPointer<LLImageJ2C>& mr_j2c,
    LLPointer<LLImageJ2C>& emissive_j2c)
{
    if (albedo_img)
    {
        albedo_tex = LLViewerTextureManager::getFetchedTexture(albedo_img, FTType::FTT_LOCAL_FILE, true);
        albedo_j2c = LLViewerTextureList::convertToUploadFile(albedo_img);
    }

    if (normal_img)
    {
        strip_alpha_channel(normal_img);
        normal_tex = LLViewerTextureManager::getFetchedTexture(normal_img, FTType::FTT_LOCAL_FILE, true);
        normal_j2c = LLViewerTextureList::convertToUploadFile(normal_img);
    }

    if (mr_img)
    {
        strip_alpha_channel(mr_img);

        if (occlusion_img && material.pbrMetallicRoughness.metallicRoughnessTexture.index != material.occlusionTexture.index)
        {
            // occlusion is a distinct texture from pbrMetallicRoughness
            // pack into mr red channel
            int occlusion_idx = material.occlusionTexture.index;
            int mr_idx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
            if (occlusion_idx != mr_idx)
            {
                //scale occlusion image to match resolution of mr image
                occlusion_img->scale(mr_img->getWidth(), mr_img->getHeight());
             
                copy_red_channel(occlusion_img, mr_img);
            }
        }
    }
    else if (occlusion_img)
    {
        //no mr but occlusion exists, make a white mr_img and copy occlusion red channel over
        mr_img = new LLImageRaw(occlusion_img->getWidth(), occlusion_img->getHeight(), 3);
        mr_img->clear(255, 255, 255);
        copy_red_channel(occlusion_img, mr_img);

    }

    if (mr_img)
    {
        mr_tex = LLViewerTextureManager::getFetchedTexture(mr_img, FTType::FTT_LOCAL_FILE, true);
        mr_j2c = LLViewerTextureList::convertToUploadFile(mr_img);
    }

    if (emissive_img)
    {
        strip_alpha_channel(emissive_img);
        emissive_tex = LLViewerTextureManager::getFetchedTexture(emissive_img, FTType::FTT_LOCAL_FILE, true);
        emissive_j2c = LLViewerTextureList::convertToUploadFile(emissive_img);
    }
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
    LLPointer<LLImageRaw> albedo_img = get_texture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index, mME->mAlbedoName);
    // get normal map
    LLPointer<LLImageRaw> normal_img = get_texture(folder, model_in, material_in.normalTexture.index, mME->mNormalName);
    // get metallic-roughness texture
    LLPointer<LLImageRaw> mr_img = get_texture(folder, model_in, material_in.pbrMetallicRoughness.metallicRoughnessTexture.index, mME->mMetallicRoughnessName);
    // get emissive texture
    LLPointer<LLImageRaw> emissive_img = get_texture(folder, model_in, material_in.emissiveTexture.index, mME->mEmissiveName);
    // get occlusion map if needed
    LLPointer<LLImageRaw> occlusion_img;
    if (material_in.occlusionTexture.index != material_in.pbrMetallicRoughness.metallicRoughnessTexture.index)
    {
        std::string tmp;
        occlusion_img = get_texture(folder, model_in, material_in.occlusionTexture.index, tmp);
    }

    LLPointer<LLViewerFetchedTexture> albedo_tex;
    LLPointer<LLViewerFetchedTexture> normal_tex;
    LLPointer<LLViewerFetchedTexture> mr_tex;
    LLPointer<LLViewerFetchedTexture> emissive_tex;
    
    pack_textures(model_in, material_in, albedo_img, normal_img, mr_img, emissive_img, occlusion_img,
        albedo_tex, normal_tex, mr_tex, emissive_tex, mME->mAlbedoJ2C, mME->mNormalJ2C, mME->mMetallicRoughnessJ2C, mME->mEmissiveJ2C);
    
    LLUUID albedo_id;
    if (albedo_tex != nullptr)
    {
        albedo_tex->forceToSaveRawImage(0, F32_MAX);
        albedo_id = albedo_tex->getID();
    }

    LLUUID normal_id;
    if (normal_tex != nullptr)
    {
        normal_tex->forceToSaveRawImage(0, F32_MAX);
        normal_id = normal_tex->getID();
    }

    LLUUID mr_id;
    if (mr_tex != nullptr)
    {
        mr_tex->forceToSaveRawImage(0, F32_MAX);
        mr_id = mr_tex->getID();
    }

    LLUUID emissive_id;
    if (emissive_tex != nullptr)
    {
        emissive_tex->forceToSaveRawImage(0, F32_MAX);
        emissive_id = emissive_tex->getID();
    }

    mME->setAlbedoId(albedo_id);
    mME->setMetallicRoughnessId(mr_id);
    mME->setEmissiveId(emissive_id);
    mME->setNormalId(normal_id);

    mME->setFromGltfModel(model_in);

    std::string new_material = LLTrans::getString("New Material");
    mME->setMaterialName(new_material);

    mME->setHasUnsavedChanges(true);
    mME->openFloater();

    mME->applyToSelection();
}

bool LLMaterialEditor::setFromGltfModel(tinygltf::Model& model, bool set_textures)
{
    if (model.materials.size() > 0)
    {
        tinygltf::Material& material_in = model.materials[0];

        if (set_textures)
        {
            S32 index;
            LLUUID id;

            // get albedo texture
            index = material_in.pbrMetallicRoughness.baseColorTexture.index;
            if (index >= 0)
            {
                id.set(model.images[index].uri);
                setAlbedoId(id);
            }
            else
            {
                setAlbedoId(LLUUID::null);
            }

            // get normal map
            index = material_in.normalTexture.index;
            if (index >= 0)
            {
                id.set(model.images[index].uri);
                setNormalId(id);
            }
            else
            {
                setNormalId(LLUUID::null);
            }

            // get metallic-roughness texture
            index = material_in.pbrMetallicRoughness.metallicRoughnessTexture.index;
            if (index >= 0)
            {
                id.set(model.images[index].uri);
                setMetallicRoughnessId(id);
            }
            else
            {
                setMetallicRoughnessId(LLUUID::null);
            }

            // get emissive texture
            index = material_in.emissiveTexture.index;
            if (index >= 0)
            {
                id.set(model.images[index].uri);
                setEmissiveId(id);
            }
            else
            {
                setEmissiveId(LLUUID::null);
            }
        }

        setAlphaMode(material_in.alphaMode);
        setAlphaCutoff(material_in.alphaCutoff);

        setAlbedoColor(get_color(material_in.pbrMetallicRoughness.baseColorFactor));
        setEmissiveColor(get_color(material_in.emissiveFactor));

        setMetalnessFactor(material_in.pbrMetallicRoughness.metallicFactor);
        setRoughnessFactor(material_in.pbrMetallicRoughness.roughnessFactor);

        setDoubleSided(material_in.doubleSided);
    }

    return true;
}

void LLMaterialEditor::importMaterial()
{
    (new LLMaterialFilePicker(this))->getFile();
}

void LLMaterialEditor::applyToSelection()
{
    // Todo: associate with a specific 'selection' instead
    // of modifying something that is selected
    // This should be disabled when working from agent's
    // inventory and for initial upload
    LLViewerObject* objectp = LLSelectMgr::instance().getSelection()->getFirstObject();
    if (objectp && objectp->getVolume())
    {
        LLGLTFMaterial* mat = new LLGLTFMaterial();
        getGLTFMaterial(mat);
        LLVOVolume* vobjp = (LLVOVolume*)objectp;
        for (int i = 0; i < vobjp->getNumTEs(); ++i)
        {
            vobjp->getTE(i)->setGLTFMaterial(mat);
            vobjp->updateTEMaterialTextures(i);
        }

        vobjp->markForUpdate(TRUE);
    }
}

void LLMaterialEditor::getGLTFMaterial(LLGLTFMaterial* mat)
{
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
}

void LLMaterialEditor::setFromGLTFMaterial(LLGLTFMaterial* mat)
{
    setAlbedoColor(mat->mAlbedoColor);
    setAlbedoId(mat->mAlbedoId);
    setNormalId(mat->mNormalId);

    setMetallicRoughnessId(mat->mMetallicRoughnessId);
    setMetalnessFactor(mat->mMetallicFactor);
    setRoughnessFactor(mat->mRoughnessFactor);

    setEmissiveColor(mat->mEmissiveColor);
    setEmissiveId(mat->mEmissiveId);

    setDoubleSided(mat->mDoubleSided);
    setAlphaMode(mat->getAlphaMode());
}

void LLMaterialEditor::loadAsset()
{
    // derived from LLPreviewNotecard::loadAsset
    
    // TODO: see commented out "editor" references and make them do something appropriate to the UI
   
    // request the asset.
    const LLInventoryItem* item = getItem();
    
    bool fail = false;

    if (item)
    {
        LLPermissions perm(item->getPermissions());
        BOOL is_owner = gAgent.allowOperation(PERM_OWNER, perm, GP_OBJECT_MANIPULATE);
        BOOL allow_copy = gAgent.allowOperation(PERM_COPY, perm, GP_OBJECT_MANIPULATE);
        BOOL allow_modify = canModify(mObjectUUID, item);
        BOOL source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(mItemUUID, gInventory.getLibraryRootFolderID());

        if (allow_copy || gAgent.isGodlike())
        {
            mAssetID = item->getAssetUUID();
            if (mAssetID.isNull())
            {
                mAssetStatus = PREVIEW_ASSET_LOADED;
            }
            else
            {
                LLHost source_sim = LLHost();
                LLSD* user_data = new LLSD();

                if (mObjectUUID.notNull())
                {
                    LLViewerObject* objectp = gObjectList.findObject(mObjectUUID);
                    if (objectp && objectp->getRegion())
                    {
                        source_sim = objectp->getRegion()->getHost();
                    }
                    else
                    {
                        // The object that we're trying to look at disappeared, bail.
                        LL_WARNS() << "Can't find object " << mObjectUUID << " associated with notecard." << LL_ENDL;
                        mAssetID.setNull();
                        /*editor->setText(getString("no_object"));
                        editor->makePristine();
                        editor->setEnabled(FALSE);*/
                        mAssetStatus = PREVIEW_ASSET_LOADED;
                        return;
                    }
                    user_data->with("taskid", mObjectUUID).with("itemid", mItemUUID);
                }
                else
                {
                    user_data = new LLSD(mItemUUID);
                }

                gAssetStorage->getInvItemAsset(source_sim,
                    gAgent.getID(),
                    gAgent.getSessionID(),
                    item->getPermissions().getOwner(),
                    mObjectUUID,
                    item->getUUID(),
                    item->getAssetUUID(),
                    item->getType(),
                    &onLoadComplete,
                    (void*)user_data,
                    TRUE);
                mAssetStatus = PREVIEW_ASSET_LOADING;
            }
        }
        else
        {
            mAssetID.setNull();
            /*editor->setText(getString("not_allowed"));
            editor->makePristine();
            editor->setEnabled(FALSE);*/
            mAssetStatus = PREVIEW_ASSET_LOADED;
        }

        if (!allow_modify)
        {
            //editor->setEnabled(FALSE);
            //getChildView("lock")->setVisible(TRUE);
            //getChildView("Edit")->setEnabled(FALSE);
        }

        if ((allow_modify || is_owner) && !source_library)
        {
            //getChildView("Delete")->setEnabled(TRUE);
        }
    }
    else if (mObjectUUID.notNull() && mItemUUID.notNull())
    {
        LLViewerObject* objectp = gObjectList.findObject(mObjectUUID);
        if (objectp && (objectp->isInventoryPending() || objectp->isInventoryDirty()))
        {
            // It's a material in object's inventory and we failed to get it because inventory is not up to date.
            // Subscribe for callback and retry at inventoryChanged()
            registerVOInventoryListener(objectp, NULL); //removes previous listener

            if (objectp->isInventoryDirty())
            {
                objectp->requestInventory();
            }
        }
        else
        {
            fail = true;
        }
    }
    else
    {
        fail = true;
    }

    if (fail)
    {
        /*editor->setText(LLStringUtil::null);
        editor->makePristine();
        editor->setEnabled(TRUE);*/
        // Don't set asset status here; we may not have set the item id yet
        // (e.g. when this gets called initially)
        //mAssetStatus = PREVIEW_ASSET_LOADED;
    }
}

// static
void LLMaterialEditor::onLoadComplete(const LLUUID& asset_uuid,
    LLAssetType::EType type,
    void* user_data, S32 status, LLExtStat ext_status)
{
    LL_INFOS() << "LLMaterialEditor::onLoadComplete()" << LL_ENDL;
    LLSD* floater_key = (LLSD*)user_data;
    LLMaterialEditor* editor = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", *floater_key);
    if (editor)
    {
        if (0 == status)
        {
            LLFileSystem file(asset_uuid, type, LLFileSystem::READ);

            S32 file_length = file.getSize();

            std::vector<char> buffer(file_length + 1);
            file.read((U8*)&buffer[0], file_length);

            editor->decodeAsset(buffer);

            BOOL modifiable = editor->canModify(editor->mObjectID, editor->getItem());
            editor->setEnabled(modifiable);
            editor->mAssetStatus = PREVIEW_ASSET_LOADED;
        }
        else
        {
            if (LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
                LL_ERR_FILE_EMPTY == status)
            {
                LLNotificationsUtil::add("MaterialMissing");
            }
            else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
            {
                LLNotificationsUtil::add("MaterialNoPermissions");
            }
            else
            {
                LLNotificationsUtil::add("UnableToLoadMaterial");
            }

            LL_WARNS() << "Problem loading material: " << status << LL_ENDL;
            editor->mAssetStatus = PREVIEW_ASSET_ERROR;
        }
    }
    delete floater_key;
}

void LLMaterialEditor::inventoryChanged(LLViewerObject* object,
    LLInventoryObject::object_list_t* inventory,
    S32 serial_num,
    void* user_data)
{
    removeVOInventoryListener();
    loadAsset();
}


void LLMaterialEditor::saveTexture(LLImageJ2C* img, const std::string& name, const LLUUID& asset_id)
{
    if (img == nullptr || img->getDataSize() == 0)
    {
        return;
    }

    // copy image bytes into string
    std::string buffer;
    buffer.assign((const char*) img->getData(), img->getDataSize());

    U32 expected_upload_cost = 10; // TODO: where do we get L$10 for textures from?

    LLAssetStorage::LLStoreAssetCallback callback;

    LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewBufferedResourceUploadInfo>(
        buffer,
        asset_id,
        name, 
        name, 
        0,
        LLFolderType::FT_TEXTURE, 
        LLInventoryType::IT_TEXTURE,
        LLAssetType::AT_TEXTURE,
        LLFloaterPerms::getNextOwnerPerms("Uploads"),
        LLFloaterPerms::getGroupPerms("Uploads"),
        LLFloaterPerms::getEveryonePerms("Uploads"),
        expected_upload_cost, 
        false));

    upload_new_resource(uploadInfo, callback, nullptr);
}

void LLMaterialEditor::saveTextures()
{
    saveTexture(mAlbedoJ2C, mAlbedoName, mAlbedoTextureUploadId);
    saveTexture(mNormalJ2C, mNormalName, mNormalTextureUploadId);
    saveTexture(mEmissiveJ2C, mEmissiveName, mEmissiveTextureUploadId);
    saveTexture(mMetallicRoughnessJ2C, mMetallicRoughnessName, mMetallicTextureUploadId);

    // discard upload buffers once textures have been saved
    mAlbedoJ2C = nullptr;
    mNormalJ2C = nullptr;
    mEmissiveJ2C = nullptr;
    mMetallicRoughnessJ2C = nullptr;
}

