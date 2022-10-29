/** 
 * @file llmaterialeditor.cpp
 * @brief Implementation of the gltf material editor
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
#include "llagentbenefits.h"
#include "llappviewer.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llfilesystem.h"
#include "llgltfmateriallist.h"
#include "llinventorymodel.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewermenufile.h"
#include "llviewertexture.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llstatusbar.h"	// can_afford_transaction()
#include "llviewerinventory.h"
#include "llinventory.h"
#include "llviewerregion.h"
#include "llvovolume.h"
#include "roles_constants.h"
#include "llviewerobjectlist.h"
#include "llsdserialize.h"
#include "llimagej2c.h"
#include "llviewertexturelist.h"
#include "llfloaterperms.h"

#include "tinygltf/tiny_gltf.h"
#include "lltinygltfhelper.h"
#include <strstream>


const std::string MATERIAL_BASE_COLOR_DEFAULT_NAME = "Base Color";
const std::string MATERIAL_NORMAL_DEFAULT_NAME = "Normal";
const std::string MATERIAL_METALLIC_DEFAULT_NAME = "Metallic Roughness";
const std::string MATERIAL_EMISSIVE_DEFAULT_NAME = "Emissive";

// Don't use ids here, LLPreview will attempt to use it as an inventory item
static const std::string LIVE_MATERIAL_EDITOR_KEY = "Live Editor";

// Dirty flags
static const U32 MATERIAL_BASE_COLOR_DIRTY = 0x1 << 0;
static const U32 MATERIAL_BASE_TRANSPARENCY_DIRTY = 0x1 << 1;
static const U32 MATERIAL_BASE_COLOR_TEX_DIRTY = 0x1 << 2;

static const U32 MATERIAL_NORMAL_TEX_DIRTY = 0x1 << 3;

static const U32 MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY = 0x1 << 4;
static const U32 MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY = 0x1 << 5;
static const U32 MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY = 0x1 << 6;

static const U32 MATERIAL_EMISIVE_COLOR_DIRTY = 0x1 << 7;
static const U32 MATERIAL_EMISIVE_TEX_DIRTY = 0x1 << 8;

static const U32 MATERIAL_DOUBLE_SIDED_DIRTY = 0x1 << 9;
static const U32 MATERIAL_ALPHA_MODE_DIRTY = 0x1 << 10;
static const U32 MATERIAL_ALPHA_CUTOFF_DIRTY = 0x1 << 11;

LLFloaterComboOptions::LLFloaterComboOptions()
    : LLFloater(LLSD())
{
    buildFromFile("floater_combobox_ok_cancel.xml");
}

LLFloaterComboOptions::~LLFloaterComboOptions()
{

}

BOOL LLFloaterComboOptions::postBuild()
{
    mConfirmButton = getChild<LLButton>("combo_ok", TRUE);
    mCancelButton = getChild<LLButton>("combo_cancel", TRUE);
    mComboOptions = getChild<LLComboBox>("combo_options", TRUE);
    mComboText = getChild<LLTextBox>("combo_text", TRUE);

    mConfirmButton->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) {onConfirm(); });
    mCancelButton->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) {onCancel(); });

    return TRUE;
}

LLFloaterComboOptions* LLFloaterComboOptions::showUI(
    combo_callback callback,
    const std::string &title,
    const std::string &description,
    const std::list<std::string> &options)
{
    LLFloaterComboOptions* combo_picker = new LLFloaterComboOptions();
    if (combo_picker)
    {
        combo_picker->mCallback = callback;
        combo_picker->setTitle(title);

        combo_picker->mComboText->setText(description);

        std::list<std::string>::const_iterator iter = options.begin();
        std::list<std::string>::const_iterator end = options.end();
        for (; iter != end; iter++)
        {
            combo_picker->mComboOptions->addSimpleElement(*iter);
        }
        combo_picker->mComboOptions->selectFirstItem();

        combo_picker->openFloater(LLSD(title));
        combo_picker->setFocus(TRUE);
        combo_picker->center();
    }
    return combo_picker;
}

LLFloaterComboOptions* LLFloaterComboOptions::showUI(
    combo_callback callback,
    const std::string &title,
    const std::string &description,
    const std::string &ok_text,
    const std::string &cancel_text,
    const std::list<std::string> &options)
{
    LLFloaterComboOptions* combo_picker = showUI(callback, title, description, options);
    if (combo_picker)
    {
        combo_picker->mConfirmButton->setLabel(ok_text);
        combo_picker->mCancelButton->setLabel(cancel_text);
    }
    return combo_picker;
}

void LLFloaterComboOptions::onConfirm()
{
    mCallback(mComboOptions->getSimple(), mComboOptions->getCurrentIndex());
    closeFloater();
}

void LLFloaterComboOptions::onCancel()
{
    mCallback(std::string(), -1);
    closeFloater();
}

class LLMaterialEditorCopiedCallback : public LLInventoryCallback
{
public:
    LLMaterialEditorCopiedCallback(
        const std::string &buffer,
        const LLSD &old_key,
        bool has_unsaved_changes)
        : mBuffer(buffer),
          mOldKey(old_key),
          mHasUnsavedChanges(has_unsaved_changes)
    {}

    LLMaterialEditorCopiedCallback(
        const LLSD &old_key,
        const std::string &new_name)
        : mOldKey(old_key),
          mNewName(new_name),
          mHasUnsavedChanges(false)
    {}

    virtual void fire(const LLUUID& inv_item_id)
    {
        if (!mNewName.empty())
        {
            // making a copy from a notecard doesn't change name, do it now
            LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);
            if (item->getName() != mNewName)
            {
                LLSD updates;
                updates["name"] = mNewName;
                update_inventory_item(inv_item_id, updates, NULL);
            }
        }
        LLMaterialEditor::finishSaveAs(mOldKey, inv_item_id, mBuffer, mHasUnsavedChanges);
    }

private:
    std::string mBuffer;
    LLSD mOldKey;
    std::string mNewName;
    bool mHasUnsavedChanges;
};

///----------------------------------------------------------------------------
/// Class LLMaterialEditor
///----------------------------------------------------------------------------

// Default constructor
LLMaterialEditor::LLMaterialEditor(const LLSD& key)
    : LLPreview(key)
    , mUnsavedChanges(0)
    , mExpectedUploadCost(0)
    , mUploadingTexturesCount(0)
{
    const LLInventoryItem* item = getItem();
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
    // if this is a 'live editor' instance, it uses live overrides
    mIsOverride = key.asString() == LIVE_MATERIAL_EDITOR_KEY;
}

void LLMaterialEditor::setObjectID(const LLUUID& object_id)
{
    LLPreview::setObjectID(object_id);
    const LLInventoryItem* item = getItem();
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
}

void LLMaterialEditor::setAuxItem(const LLInventoryItem* item)
{
    LLPreview::setAuxItem(item);
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
}

BOOL LLMaterialEditor::postBuild()
{
    mBaseColorTextureCtrl = getChild<LLTextureCtrl>("base_color_texture");
    mMetallicTextureCtrl = getChild<LLTextureCtrl>("metallic_roughness_texture");
    mEmissiveTextureCtrl = getChild<LLTextureCtrl>("emissive_texture");
    mNormalTextureCtrl = getChild<LLTextureCtrl>("normal_texture");

    mBaseColorTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitBaseColorTexture, this, _1, _2));
    mMetallicTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitMetallicTexture, this, _1, _2));
    mEmissiveTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitEmissiveTexture, this, _1, _2));
    mNormalTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitNormalTexture, this, _1, _2));

    childSetAction("save", boost::bind(&LLMaterialEditor::onClickSave, this));
    childSetAction("save_as", boost::bind(&LLMaterialEditor::onClickSaveAs, this));
    childSetAction("cancel", boost::bind(&LLMaterialEditor::onClickCancel, this));

    S32 upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
    getChild<LLUICtrl>("base_color_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
    getChild<LLUICtrl>("metallic_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
    getChild<LLUICtrl>("emissive_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
    getChild<LLUICtrl>("normal_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));

    boost::function<void(LLUICtrl*, void*)> changes_callback = [this](LLUICtrl * ctrl, void* userData)
    {
        const U32 *flag = (const U32*)userData;
        markChangesUnsaved(*flag);
        // Apply changes to object live
        applyToSelection();
    };
 
    childSetCommitCallback("double sided", changes_callback, (void*)&MATERIAL_DOUBLE_SIDED_DIRTY);

    // BaseColor
    childSetCommitCallback("base color", changes_callback, (void*)&MATERIAL_BASE_COLOR_DIRTY);
    childSetCommitCallback("transparency", changes_callback, (void*)&MATERIAL_BASE_TRANSPARENCY_DIRTY);
    childSetCommitCallback("alpha mode", changes_callback, (void*)&MATERIAL_ALPHA_MODE_DIRTY);
    childSetCommitCallback("alpha cutoff", changes_callback, (void*)&MATERIAL_ALPHA_CUTOFF_DIRTY);

    // Metallic-Roughness
    childSetCommitCallback("metalness factor", changes_callback, (void*)&MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY);
    childSetCommitCallback("roughness factor", changes_callback, (void*)&MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY);

    // Emissive
    childSetCommitCallback("emissive color", changes_callback, (void*)&MATERIAL_EMISIVE_COLOR_DIRTY);

    childSetVisible("unsaved_changes", mUnsavedChanges && !mIsOverride);

    getChild<LLUICtrl>("total_upload_fee")->setTextArg("[FEE]", llformat("%d", 0));

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

void LLMaterialEditor::onClose(bool app_quitting)
{
    if (mSelectionUpdateSlot.connected())
    {
        mSelectionUpdateSlot.disconnect();
    }

    LLPreview::onClose(app_quitting);
}

LLUUID LLMaterialEditor::getBaseColorId()
{
    return mBaseColorTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setBaseColorId(const LLUUID& id)
{
    mBaseColorTextureCtrl->setValue(id);
    mBaseColorTextureCtrl->setDefaultImageAssetID(id);
    mBaseColorTextureCtrl->setTentative(FALSE);
}

void LLMaterialEditor::setBaseColorUploadId(const LLUUID& id)
{
    // Might be better to use local textures and
    // assign a fee in case of a local texture
    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("base_color_upload_fee", getString("upload_fee_string"));
        // Only set if we will need to upload this texture
        mBaseColorTextureUploadId = id;
    }
    markChangesUnsaved(MATERIAL_BASE_COLOR_TEX_DIRTY);
}

LLColor4 LLMaterialEditor::getBaseColor()
{
    LLColor4 ret = linearColor4(LLColor4(childGetValue("base color")));
    ret.mV[3] = getTransparency();
    return ret;
}

void LLMaterialEditor::setBaseColor(const LLColor4& color)
{
    childSetValue("base color", srgbColor4(color).getValue());
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
    mMetallicTextureCtrl->setTentative(FALSE);
}

void LLMaterialEditor::setMetallicRoughnessUploadId(const LLUUID& id)
{
    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("metallic_upload_fee", getString("upload_fee_string"));
        mMetallicTextureUploadId = id;
    }
    markChangesUnsaved(MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY);
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
    mEmissiveTextureCtrl->setTentative(FALSE);
}

void LLMaterialEditor::setEmissiveUploadId(const LLUUID& id)
{
    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("emissive_upload_fee", getString("upload_fee_string"));
        mEmissiveTextureUploadId = id;
    }
    markChangesUnsaved(MATERIAL_EMISIVE_TEX_DIRTY);
}

LLColor4 LLMaterialEditor::getEmissiveColor()
{
    return linearColor4(LLColor4(childGetValue("emissive color")));
}

void LLMaterialEditor::setEmissiveColor(const LLColor4& color)
{
    childSetValue("emissive color", srgbColor4(color).getValue());
}

LLUUID LLMaterialEditor::getNormalId()
{
    return mNormalTextureCtrl->getValue().asUUID();
}

void LLMaterialEditor::setNormalId(const LLUUID& id)
{
    mNormalTextureCtrl->setValue(id);
    mNormalTextureCtrl->setDefaultImageAssetID(id);
    mNormalTextureCtrl->setTentative(FALSE);
}

void LLMaterialEditor::setNormalUploadId(const LLUUID& id)
{
    if (id.notNull())
    {
        // todo: this does not account for posibility of texture
        // being from inventory, need to check that
        childSetValue("normal_upload_fee", getString("upload_fee_string"));
        mNormalTextureUploadId = id;
    }
    markChangesUnsaved(MATERIAL_NORMAL_TEX_DIRTY);
}

bool LLMaterialEditor::getDoubleSided()
{
    return childGetValue("double sided").asBoolean();
}

void LLMaterialEditor::setDoubleSided(bool double_sided)
{
    childSetValue("double sided", double_sided);
}

void LLMaterialEditor::resetUnsavedChanges()
{
    mUnsavedChanges = 0;
    childSetVisible("unsaved_changes", false);
    setCanSave(false);

    mExpectedUploadCost = 0;
    getChild<LLUICtrl>("total_upload_fee")->setTextArg("[FEE]", llformat("%d", mExpectedUploadCost));
}

void LLMaterialEditor::markChangesUnsaved(U32 dirty_flag)
{
    mUnsavedChanges |= dirty_flag;
    // at the moment live editing (mIsOverride) applies everything 'live'
    childSetVisible("unsaved_changes", mUnsavedChanges && !mIsOverride);

    if (mUnsavedChanges)
    {
        const LLInventoryItem* item = getItem();
        if (item)
        {
            LLPermissions perm(item->getPermissions());
            bool allow_modify = canModify(mObjectUUID, item);
            bool source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(mItemUUID, gInventory.getLibraryRootFolderID());
            bool source_notecard = mNotecardInventoryID.notNull();

            setCanSave(allow_modify && !source_library && !source_notecard);
        }
    }
    else
    {
        setCanSave(false);
    }

    S32 upload_texture_count = 0;
    if (mBaseColorTextureUploadId.notNull() && mBaseColorTextureUploadId == getBaseColorId())
    {
        upload_texture_count++;
    }
    if (mMetallicTextureUploadId.notNull() && mMetallicTextureUploadId == getMetallicRoughnessId())
    {
        upload_texture_count++;
    }
    if (mEmissiveTextureUploadId.notNull() && mEmissiveTextureUploadId == getEmissiveId())
    {
        upload_texture_count++;
    }
    if (mNormalTextureUploadId.notNull() && mNormalTextureUploadId == getNormalId())
    {
        upload_texture_count++;
    }

    mExpectedUploadCost = upload_texture_count * LLAgentBenefitsMgr::current().getTextureUploadCost();
    getChild<LLUICtrl>("total_upload_fee")->setTextArg("[FEE]", llformat("%d", mExpectedUploadCost));
}

void LLMaterialEditor::setCanSaveAs(bool value)
{
    childSetEnabled("save_as", value);
}

void LLMaterialEditor::setCanSave(bool value)
{
    childSetEnabled("save", value);
}

void LLMaterialEditor::setEnableEditing(bool can_modify)
{
    childSetEnabled("double sided", can_modify);

    // BaseColor
    childSetEnabled("base color", can_modify);
    childSetEnabled("transparency", can_modify);
    childSetEnabled("alpha mode", can_modify);
    childSetEnabled("alpha cutoff", can_modify);

    // Metallic-Roughness
    childSetEnabled("metalness factor", can_modify);
    childSetEnabled("roughness factor", can_modify);

    // Metallic-Roughness
    childSetEnabled("metalness factor", can_modify);
    childSetEnabled("roughness factor", can_modify);

    // Emissive
    childSetEnabled("emissive color", can_modify);

    mBaseColorTextureCtrl->setEnabled(can_modify);
    mMetallicTextureCtrl->setEnabled(can_modify);
    mEmissiveTextureCtrl->setEnabled(can_modify);
    mNormalTextureCtrl->setEnabled(can_modify);
}

void LLMaterialEditor::onCommitBaseColorTexture(LLUICtrl * ctrl, const LLSD & data)
{
    // might be better to use arrays, to have a single callback
    // and not to repeat the same thing for each tecture control
    LLUUID new_val = mBaseColorTextureCtrl->getValue().asUUID();
    if (new_val == mBaseColorTextureUploadId && mBaseColorTextureUploadId.notNull())
    {
        childSetValue("base_color_upload_fee", getString("upload_fee_string"));
    }
    else
    {
        // Texture picker has 'apply now' with 'cancel' support.
        // Keep mBaseColorJ2C and mBaseColorFetched, it's our storage in
        // case user decides to cancel changes.
        // Without mBaseColorFetched, viewer will eventually cleanup
        // the texture that is not in use
        childSetValue("base_color_upload_fee", getString("no_upload_fee_string"));
    }
    markChangesUnsaved(MATERIAL_BASE_COLOR_TEX_DIRTY);
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
        childSetValue("metallic_upload_fee", getString("no_upload_fee_string"));
    }
    markChangesUnsaved(MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY);
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
        childSetValue("emissive_upload_fee", getString("no_upload_fee_string"));
    }
    markChangesUnsaved(MATERIAL_EMISIVE_TEX_DIRTY);
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
        childSetValue("normal_upload_fee", getString("no_upload_fee_string"));
    }
    markChangesUnsaved(MATERIAL_NORMAL_TEX_DIRTY);
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
    if (!can_afford_transaction(mExpectedUploadCost))
    {
        LLSD args;
        args["COST"] = llformat("%d", mExpectedUploadCost);
        LLNotificationsUtil::add("ErrorCannotAffordUpload", args);
        return;
    }

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

    // write base color
    LLColor4 base_color = getBaseColor();
    base_color.mV[3] = getTransparency();
    write_color(base_color, pbrMaterial.baseColorFactor);

    model.materials[0].alphaCutoff = getAlphaCutoff();
    model.materials[0].alphaMode = getAlphaMode();

    LLUUID base_color_id = getBaseColorId();

    if (base_color_id.notNull())
    {
        U32 texture_idx = write_texture(base_color_id, model);

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
                        // assets are only supposed to have one item
                        return setFromGltfModel(model_in, 0, true);
                    }
                    else
                    {
                        LL_WARNS("MaterialEditor") << "Floater " << getKey() << " Failed to decode material asset: " << LL_NEWLINE
                         << warn_msg << LL_NEWLINE
                         << error_msg << LL_ENDL;
                    }
                }
            }
        }
        else
        {
            LL_WARNS("MaterialEditor") << "Invalid LLSD content "<< asset << " for flaoter " << getKey() << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("MaterialEditor") << "Failed to deserialize material LLSD for flaoter " << getKey() << LL_ENDL;
    }

    return false;
}

/**
 * Build a description of the material we just imported.
 * Currently this means a list of the textures present but we
 * may eventually want to make it more complete - will be guided
 * by what the content creators say they need.
 */
const std::string LLMaterialEditor::buildMaterialDescription()
{
    std::ostringstream desc;
    desc << LLTrans::getString("Material Texture Name Header");

    // add the texture names for each just so long as the material
    // we loaded has an entry for it (i think testing the texture 
    // control UUI for NULL is a valid metric for if it was loaded
    // or not but I suspect this code will change a lot so may need
    // to revisit
    if (!mBaseColorTextureCtrl->getValue().asUUID().isNull())
    {
        desc << mBaseColorName;
        desc << ", ";
    }
    if (!mMetallicTextureCtrl->getValue().asUUID().isNull())
    {
        desc << mMetallicRoughnessName;
        desc << ", ";
    }
    if (!mEmissiveTextureCtrl->getValue().asUUID().isNull())
    {
        desc << mEmissiveName;
        desc << ", ";
    }
    if (!mNormalTextureCtrl->getValue().asUUID().isNull())
    {
        desc << mNormalName;
    }

    // trim last char if it's a ',' in case there is no normal texture
    // present and the code above inserts one
    // (no need to check for string length - always has initial string)
    std::string::iterator iter = desc.str().end() - 1;
    if (*iter == ',')
    {
        desc.str().erase(iter);
    }

    // sanitize the material description so that it's compatible with the inventory
    // note: split this up because clang doesn't like operating directly on the
    // str() - error: lvalue reference to type 'basic_string<...>' cannot bind to a
    // temporary of type 'basic_string<...>'
    std::string inv_desc = desc.str();
    LLInventoryObject::correctInventoryName(inv_desc);

    return inv_desc;
}

bool LLMaterialEditor::saveIfNeeded()
{
    if (mUploadingTexturesCount > 0)
    {
        // upload already in progress
        // wait until textures upload
        // will retry saving on callback
        return true;
    }

    if (saveTextures() > 0)
    {
        // started texture upload
        setEnabled(false);
        return true;
    }

    std::string buffer = getEncodedAsset();

    const LLInventoryItem* item = getItem();
    // save it out to database
    if (item)
    {
        if (!saveToInventoryItem(buffer, mItemUUID, mObjectUUID))
        {
            return false;
        }

        if (mCloseAfterSave)
        {
            closeFloater();
        }
        else
        {
            mAssetStatus = PREVIEW_ASSET_LOADING;
            setEnabled(false);
        }
    }
    else
    { //make a new inventory item
#if 1
        // gen a new uuid for this asset
        LLTransactionID tid;
        tid.generate();     // timestamp-based randomization + uniquification
        LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());
        std::string res_desc = buildMaterialDescription();
        U32 next_owner_perm = LLFloaterPerms::getNextOwnerPerms("Materials");
        LLUUID parent = gInventory.findUserDefinedCategoryUUIDForType(LLFolderType::FT_MATERIAL);
        const U8 subtype = NO_INV_SUBTYPE;  // TODO maybe use AT_SETTINGS and LLSettingsType::ST_MATERIAL ?

        create_inventory_item(gAgent.getID(), gAgent.getSessionID(), parent, tid, mMaterialName, res_desc,
            LLAssetType::AT_MATERIAL, LLInventoryType::IT_MATERIAL, subtype, next_owner_perm,
            new LLBoostFuncInventoryCallback([output = buffer](LLUUID const& inv_item_id)
            {
                LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);
                if (item)
                {
                    // create_inventory_item doesn't allow presetting some permissions, fix it now
                    LLPermissions perm = item->getPermissions();
                    if (perm.getMaskEveryone() != LLFloaterPerms::getEveryonePerms("Materials")
                        || perm.getMaskGroup() != LLFloaterPerms::getGroupPerms("Materials"))
                    {
                        perm.setMaskEveryone(LLFloaterPerms::getEveryonePerms("Materials"));
                        perm.setMaskGroup(LLFloaterPerms::getGroupPerms("Materials"));

                        item->setPermissions(perm);

                        item->updateServer(FALSE);
                        gInventory.updateItem(item);
                        gInventory.notifyObservers();
                    }
                }

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

                // todo: apply permissions from textures here if server doesn't
                // if any texture is 'no transfer', material should be 'no transfer' as well
                const LLViewerRegion* region = gAgent.getRegion();
                if (region)
                {
                    std::string agent_url(region->getCapability("UpdateMaterialAgentInventory"));
                    if (agent_url.empty())
                    {
                        LL_ERRS("MaterialEditor") << "missing required agent inventory cap url" << LL_ENDL;
                    }
                    LLViewerAssetUpload::EnqueueInventoryUpload(agent_url, uploadInfo);
                }
            })
        );

        // We do not update floater with uploaded asset yet, so just close it.
        closeFloater();
#endif
    }
    
    return true;
}

// static
bool LLMaterialEditor::saveToInventoryItem(const std::string &buffer, const LLUUID &item_id, const LLUUID &task_id)
{
    const LLViewerRegion* region = gAgent.getRegion();
    if (!region)
    {
        LL_WARNS("MaterialEditor") << "Not connected to a region, cannot save material." << LL_ENDL;
        return false;
    }
    std::string agent_url = region->getCapability("UpdateMaterialAgentInventory");
    std::string task_url = region->getCapability("UpdateMaterialTaskInventory");

    if (!agent_url.empty() && !task_url.empty())
    {
        std::string url;
        LLResourceUploadInfo::ptr_t uploadInfo;

        if (task_id.isNull() && !agent_url.empty())
        {
            uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(item_id, LLAssetType::AT_MATERIAL, buffer,
                [](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD) {
                LLMaterialEditor::finishInventoryUpload(itemId, newAssetId, newItemId);
            });
            url = agent_url;
        }
        else if (!task_id.isNull() && !task_url.empty())
        {
            LLUUID object_uuid(task_id);
            uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(task_id, item_id, LLAssetType::AT_MATERIAL, buffer,
                [object_uuid](LLUUID itemId, LLUUID, LLUUID newAssetId, LLSD) {
                LLMaterialEditor::finishTaskUpload(itemId, newAssetId, object_uuid);
            });
            url = task_url;
        }

        if (!url.empty() && uploadInfo)
        {
            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        }
        else
        {
            return false;
        }

    }
    else // !gAssetStorage
    {
        LL_WARNS("MaterialEditor") << "Not connected to an materials capable region." << LL_ENDL;
        return false;
    }

    // todo: apply permissions from textures here if server doesn't
    // if any texture is 'no transfer', material should be 'no transfer' as well

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
        else if (newItemId.notNull())
        {
            // Not supposed to happen?
            me->refreshFromInventory(newItemId);
        }
        else
        {
            me->refreshFromInventory(itemId);
        }
    }
}

void LLMaterialEditor::finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId)
{
    LLSD floater_key;
    floater_key["taskid"] = taskId;
    floater_key["itemid"] = itemId;
    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", floater_key);
    if (me)
    {
        me->setAssetId(newAssetId);
        me->refreshFromInventory();
        me->setEnabled(true);
    }
}

void LLMaterialEditor::finishSaveAs(
    const LLSD &oldKey,
    const LLUUID &newItemId,
    const std::string &buffer,
    bool has_unsaved_changes)
{
    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", oldKey);
    LLViewerInventoryItem* item = gInventory.getItem(newItemId);
    if (item)
    {
        if (me)
        {
            me->mItemUUID = newItemId;
            me->mObjectUUID = LLUUID::null;
            me->mNotecardInventoryID = LLUUID::null;
            me->mNotecardObjectID = LLUUID::null;
            me->mAuxItem = nullptr;
            me->setKey(LLSD(newItemId)); // for findTypedInstance
            me->setMaterialName(item->getName());
            if (has_unsaved_changes)
            {
                if (!saveToInventoryItem(buffer, newItemId, LLUUID::null))
                {
                    me->setEnabled(true);
                }
            }
            else
            {
                me->loadAsset();
                me->setEnabled(true);
            }
        }
        else if(has_unsaved_changes)
        {
            saveToInventoryItem(buffer, newItemId, LLUUID::null);
        }
    }
    else if (me)
    {
        me->setEnabled(true);
        LL_WARNS("MaterialEditor") << "Item does not exist, floater " << me->getKey() << LL_ENDL;
    }
}

void LLMaterialEditor::refreshFromInventory(const LLUUID& new_item_id)
{
    if (mIsOverride)
    {
        // refreshFromInventory shouldn't be called for overrides,
        // but just in case.
        LL_WARNS("MaterialEditor") << "Tried to refresh from inventory for live editor" << LL_ENDL;
        return;
    }
    LLSD old_key = getKey();
    if (new_item_id.notNull())
    {
        mItemUUID = new_item_id;
        if (mNotecardInventoryID.notNull())
        {
            LLSD floater_key;
            floater_key["objectid"] = mNotecardObjectID;
            floater_key["notecardid"] = mNotecardInventoryID;
            setKey(floater_key);
        }
        else if (mObjectUUID.notNull())
        {
            LLSD floater_key;
            floater_key["taskid"] = new_item_id;
            floater_key["itemid"] = mObjectUUID;
            setKey(floater_key);
        }
        else
        {
            setKey(LLSD(new_item_id));
        }
    }
    LL_DEBUGS("MaterialEditor") << "New floater key: " << getKey() << " Old key: " << old_key << LL_ENDL;
    loadAsset();
}


void LLMaterialEditor::onClickSaveAs()
{
    if (!can_afford_transaction(mExpectedUploadCost))
    {
        LLSD args;
        args["COST"] = llformat("%d", mExpectedUploadCost);
        LLNotificationsUtil::add("ErrorCannotAffordUpload", args);
        return;
    }

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
        LLInventoryObject::correctInventoryName(new_name);
        if (!new_name.empty())
        {
            const LLInventoryItem* item;
            if (mNotecardInventoryID.notNull())
            {
                item = mAuxItem.get();
            }
            else
            {
                item = getItem();
            }
            if (item)
            {
                const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
                LLUUID parent_id = item->getParentUUID();
                if (mObjectUUID.notNull() || marketplacelistings_id == parent_id || gInventory.isObjectDescendentOf(item->getUUID(), gInventory.getLibraryRootFolderID()))
                {
                    parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MATERIAL);
                }

                // A two step process, first copy an existing item, then create new asset
                if (mNotecardInventoryID.notNull())
                {
                    LLPointer<LLInventoryCallback> cb = new LLMaterialEditorCopiedCallback(getKey(), new_name);
                    copy_inventory_from_notecard(parent_id,
                        mNotecardObjectID,
                        mNotecardInventoryID,
                        mAuxItem.get(),
                        gInventoryCallbacks.registerCB(cb));
                }
                else
                {
                    std::string buffer = getEncodedAsset();
                    LLPointer<LLInventoryCallback> cb = new LLMaterialEditorCopiedCallback(buffer, getKey(), mUnsavedChanges);
                    copy_inventory_item(
                        gAgent.getID(),
                        item->getPermissions().getOwner(),
                        item->getUUID(),
                        parent_id,
                        new_name,
                        cb);
                }

                mAssetStatus = PREVIEW_ASSET_LOADING;
                setEnabled(false);
            }
            else
            {
                setMaterialName(new_name);
                onClickSave();
            }
        }
        else
        {
            LLNotificationsUtil::add("InvalidMaterialName");
        }
    }
}

void LLMaterialEditor::onClickCancel()
{
    if (mUnsavedChanges)
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
        if (mIsOverride && !mObjectOverridesSavedValues.empty())
        {
            // Reapply ids back onto selection.
            // TODO: monitor selection changes and resave on selection changes
            struct g : public LLSelectedObjectFunctor
            {
                g(LLMaterialEditor* me) : mEditor(me) {}
                virtual bool apply(LLViewerObject* objectp)
                {
                    if (!objectp || !objectp->permModify())
                    {
                        return false;
                    }

                    U32 local_id = objectp->getLocalID();
                    if (mEditor->mObjectOverridesSavedValues.find(local_id) == mEditor->mObjectOverridesSavedValues.end())
                    {
                        return false;
                    }

                    S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
                    for (U8 te = 0; te < num_tes; te++)
                    {
                        if (mEditor->mObjectOverridesSavedValues[local_id].size() > te
                            && objectp->getTE(te)->isSelected())
                        {
                            objectp->setRenderMaterialID(
                                te,
                                mEditor->mObjectOverridesSavedValues[local_id][te],
                                false /*wait for bulk update*/);
                        }
                    }
                    return true;
                }
                LLMaterialEditor* mEditor;
            } restorefunc(this);
            LLSelectMgr::getInstance()->getSelection()->applyToObjects(&restorefunc);

            struct f : public LLSelectedObjectFunctor
            {
                virtual bool apply(LLViewerObject* object)
                {
                    if (object && !object->permModify())
                    {
                        return false;
                    }

                    LLRenderMaterialParams* param_block = (LLRenderMaterialParams*)object->getParameterEntry(LLNetworkData::PARAMS_RENDER_MATERIAL);
                    if (param_block)
                    {
                        if (param_block->isEmpty())
                        {
                            object->setHasRenderMaterialParams(false);
                        }
                        else
                        {
                            object->parameterChanged(LLNetworkData::PARAMS_RENDER_MATERIAL, true);
                        }
                    }

                    object->sendTEUpdate();
                    return true;
                }
            } sendfunc;
            LLSelectMgr::getInstance()->getSelection()->applyToObjects(&sendfunc);
        }

        closeFloater();
    }
}

static void pack_textures(
    LLPointer<LLImageRaw>& base_color_img,
    LLPointer<LLImageRaw>& normal_img,
    LLPointer<LLImageRaw>& mr_img,
    LLPointer<LLImageRaw>& emissive_img,
    LLPointer<LLImageRaw>& occlusion_img,
    LLPointer<LLImageJ2C>& base_color_j2c,
    LLPointer<LLImageJ2C>& normal_j2c,
    LLPointer<LLImageJ2C>& mr_j2c,
    LLPointer<LLImageJ2C>& emissive_j2c)
{
    // NOTE : remove log spam and lossless vs lossy comparisons when the logs are no longer useful

    if (base_color_img)
    {
        base_color_j2c = LLViewerTextureList::convertToUploadFile(base_color_img);
        LL_DEBUGS("MaterialEditor") << "BaseColor: " << base_color_j2c->getDataSize() << LL_ENDL;
    }

    if (normal_img)
    {
        normal_j2c = LLViewerTextureList::convertToUploadFile(normal_img);

        LLPointer<LLImageJ2C> test;
        test = LLViewerTextureList::convertToUploadFile(normal_img, 1024, true);

        S32 lossy_bytes = normal_j2c->getDataSize();
        S32 lossless_bytes = test->getDataSize();

        LL_DEBUGS("MaterialEditor") << llformat("Lossless vs Lossy: (%d/%d) = %.2f", lossless_bytes, lossy_bytes, (F32)lossless_bytes / lossy_bytes) << LL_ENDL;

        normal_j2c = test;
    }

    if (mr_img)
    {
        mr_j2c = LLViewerTextureList::convertToUploadFile(mr_img);
        LL_DEBUGS("MaterialEditor") << "Metallic/Roughness: " << mr_j2c->getDataSize() << LL_ENDL;
    }

    if (emissive_img)
    {
        emissive_j2c = LLViewerTextureList::convertToUploadFile(emissive_img);
        LL_DEBUGS("MaterialEditor") << "Emissive: " << emissive_j2c->getDataSize() << LL_ENDL;
    }
}

void LLMaterialEditor::loadMaterialFromFile(const std::string& filename, S32 index)
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

    if (index >= 0 && model_in.materials.size() <= index)
    {
        // material is missing
        LLNotificationsUtil::add("CannotUploadMaterial");
        return;
    }

    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor");

    if (index >= 0)
    {
        // Prespecified material
        me->loadMaterial(model_in, filename_lc, index);
    }
    else if (model_in.materials.size() == 1)
    {
        // Only one, just load it
        me->loadMaterial(model_in, filename_lc, 0);
    }
    else
    {
        // Promt user to select material
        std::list<std::string> material_list;
        std::vector<tinygltf::Material>::const_iterator mat_iter = model_in.materials.begin();
        std::vector<tinygltf::Material>::const_iterator mat_end = model_in.materials.end();
        for (; mat_iter != mat_end; mat_iter++)
        {
            std::string mat_name = mat_iter->name;
            if (mat_name.empty())
            {
                material_list.push_back("Material " + std::to_string(material_list.size()));
            }
            else
            {
                material_list.push_back(mat_name);
            }
        }
        LLFloaterComboOptions::showUI(
            [me, model_in, filename_lc](const std::string& option, S32 index)
        {
            me->loadMaterial(model_in, filename_lc, index);
        },
            me->getString("material_selection_title"),
            me->getString("material_selection_text"),
            material_list
            );
    }
}

void LLMaterialEditor::onSelectionChanged()
{
    // This won't get deletion or deselectAll()
    // Might need to handle that separately
    clearTextures();
    setFromSelection();
    // At the moment all cahges are 'live' so don't reset dirty flags
    // saveLiveValues(); todo
}

void LLMaterialEditor::saveLiveValues()
{
    // Collect ids to be able to revert overrides.
    // TODO: monitor selection changes and resave on selection changes
    mObjectOverridesSavedValues.clear();
    struct g : public LLSelectedObjectFunctor
    {
        g(LLMaterialEditor* me) : mEditor(me) {}
        virtual bool apply(LLViewerObject* objectp)
        {
            if (!objectp)
            {
                return false;
            }

            U32 local_id = objectp->getLocalID();
            S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces());
            for (U8 te = 0; te < num_tes; te++)
            {
                // Todo: fix this, overrides don't care about ids,
                // we will have to save actual values or materials
                LLUUID mat_id = objectp->getRenderMaterialID(te);
                mEditor->mObjectOverridesSavedValues[local_id].push_back(mat_id);
            }
            return true;
        }
        LLMaterialEditor* mEditor;
    } savefunc(this);
    LLSelectMgr::getInstance()->getSelection()->applyToObjects(&savefunc);
}

void LLMaterialEditor::updateLive()
{
    const LLSD floater_key(LIVE_MATERIAL_EDITOR_KEY);
    LLFloater* instance = LLFloaterReg::findInstance("material_editor", floater_key);
    if (instance && LLFloater::isVisible(instance))
    {
        LLMaterialEditor* me = (LLMaterialEditor*)instance;
        if (me)
        {
            me->clearTextures();
            me->setFromSelection();
        }
    }
}

void LLMaterialEditor::loadLive()
{
    // Allow only one 'live' instance
    const LLSD floater_key(LIVE_MATERIAL_EDITOR_KEY);
    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor", floater_key);
    if (me)
    {
        me->setFromSelection();
        me->setTitle(me->getString("material_override_title"));
        me->childSetVisible("save", false);
        me->childSetVisible("save_as", false);

        // Set up for selection changes updates
        if (!me->mSelectionUpdateSlot.connected())
        {
            me->mSelectionUpdateSlot = LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLMaterialEditor::onSelectionChanged, me));
        }
        // Collect ids to be able to revert overrides on cancel.
        me->saveLiveValues();

        me->openFloater(floater_key);
        me->setFocus(TRUE);
    }
}

void LLMaterialEditor::loadObjectSave()
{
    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor");
    if (me && me->setFromSelection())
    {
        me->childSetVisible("save", false);
        me->mMaterialName = LLTrans::getString("New Material");
        me->setTitle(me->mMaterialName);
        me->openFloater();
        me->setFocus(TRUE);
    }
}

void LLMaterialEditor::loadFromGLTFMaterial(LLUUID &asset_id)
{
    if (asset_id.isNull())
    {
        LL_WARNS("MaterialEditor") << "Trying to open material with null id" << LL_ENDL;
        return;
    }
    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor");
    me->mMaterialName = LLTrans::getString("New Material");
    me->setTitle(me->mMaterialName);
    me->setFromGLTFMaterial(gGLTFMaterialList.getMaterial(asset_id));
    me->openFloater();
    me->setFocus(TRUE);
}

void LLMaterialEditor::loadMaterial(const tinygltf::Model &model_in, const std::string &filename_lc, S32 index)
{
    if (model_in.materials.size() <= index)
    {
        return;
    }
    std::string folder = gDirUtilp->getDirName(filename_lc);

    tinygltf::Material material_in = model_in.materials[index];

    tinygltf::Model  model_out;
    model_out.asset.version = "2.0";
    model_out.materials.resize(1);

    // get base color texture
    LLPointer<LLImageRaw> base_color_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.baseColorTexture.index, mBaseColorName);
    // get normal map
    LLPointer<LLImageRaw> normal_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.normalTexture.index, mNormalName);
    // get metallic-roughness texture
    LLPointer<LLImageRaw> mr_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.pbrMetallicRoughness.metallicRoughnessTexture.index, mMetallicRoughnessName);
    // get emissive texture
    LLPointer<LLImageRaw> emissive_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.emissiveTexture.index, mEmissiveName);
    // get occlusion map if needed
    LLPointer<LLImageRaw> occlusion_img;
    if (material_in.occlusionTexture.index != material_in.pbrMetallicRoughness.metallicRoughnessTexture.index)
    {
        std::string tmp;
        occlusion_img = LLTinyGLTFHelper::getTexture(folder, model_in, material_in.occlusionTexture.index, tmp);
    }

    LLTinyGLTFHelper::initFetchedTextures(material_in, base_color_img, normal_img, mr_img, emissive_img, occlusion_img,
        mBaseColorFetched, mNormalFetched, mMetallicRoughnessFetched, mEmissiveFetched);
    pack_textures(base_color_img, normal_img, mr_img, emissive_img, occlusion_img,
        mBaseColorJ2C, mNormalJ2C, mMetallicRoughnessJ2C, mEmissiveJ2C);

    LLUUID base_color_id;
    if (mBaseColorFetched.notNull())
    {
        mBaseColorFetched->forceToSaveRawImage(0, F32_MAX);
        base_color_id = mBaseColorFetched->getID();

        if (mBaseColorName.empty())
        {
            mBaseColorName = MATERIAL_BASE_COLOR_DEFAULT_NAME;
        }
    }

    LLUUID normal_id;
    if (mNormalFetched.notNull())
    {
        mNormalFetched->forceToSaveRawImage(0, F32_MAX);
        normal_id = mNormalFetched->getID();

        if (mNormalName.empty())
        {
            mNormalName = MATERIAL_NORMAL_DEFAULT_NAME;
        }
    }

    LLUUID mr_id;
    if (mMetallicRoughnessFetched.notNull())
    {
        mMetallicRoughnessFetched->forceToSaveRawImage(0, F32_MAX);
        mr_id = mMetallicRoughnessFetched->getID();

        if (mMetallicRoughnessName.empty())
        {
            mMetallicRoughnessName = MATERIAL_METALLIC_DEFAULT_NAME;
        }
    }

    LLUUID emissive_id;
    if (mEmissiveFetched.notNull())
    {
        mEmissiveFetched->forceToSaveRawImage(0, F32_MAX);
        emissive_id = mEmissiveFetched->getID();

        if (mEmissiveName.empty())
        {
            mEmissiveName = MATERIAL_EMISSIVE_DEFAULT_NAME;
        }
    }

    setBaseColorId(base_color_id);
    setBaseColorUploadId(base_color_id);
    setMetallicRoughnessId(mr_id);
    setMetallicRoughnessUploadId(mr_id);
    setEmissiveId(emissive_id);
    setEmissiveUploadId(emissive_id);
    setNormalId(normal_id);
    setNormalUploadId(normal_id);

    setFromGltfModel(model_in, index);

    setFromGltfMetaData(filename_lc, model_in, index);

    markChangesUnsaved(U32_MAX);

    openFloater();
    setFocus(TRUE);

    applyToSelection();
}

bool LLMaterialEditor::setFromGltfModel(const tinygltf::Model& model, S32 index, bool set_textures)
{
    if (model.materials.size() > index)
    {
        const tinygltf::Material& material_in = model.materials[index];

        if (set_textures)
        {
            S32 index;
            LLUUID id;

            // get base color texture
            index = material_in.pbrMetallicRoughness.baseColorTexture.index;
            if (index >= 0)
            {
                id.set(model.images[index].uri);
                setBaseColorId(id);
            }
            else
            {
                setBaseColorId(LLUUID::null);
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

        setBaseColor(LLTinyGLTFHelper::getColor(material_in.pbrMetallicRoughness.baseColorFactor));
        setEmissiveColor(LLTinyGLTFHelper::getColor(material_in.emissiveFactor));

        setMetalnessFactor(material_in.pbrMetallicRoughness.metallicFactor);
        setRoughnessFactor(material_in.pbrMetallicRoughness.roughnessFactor);

        setDoubleSided(material_in.doubleSided);
    }

    return true;
}

/**
 * Build a texture name from the contents of the (in tinyGLFT parlance) 
 * Image URI. This often is filepath to the original image on the users'
 *  local file system.
 */
const std::string LLMaterialEditor::getImageNameFromUri(std::string image_uri, const std::string texture_type)
{
    // getBaseFileName() works differently on each platform and file patchs 
    // can contain both types of delimiter so unify them then extract the 
    // base name (no path or extension)
    std::replace(image_uri.begin(), image_uri.end(), '\\', gDirUtilp->getDirDelimiter()[0]);
    std::replace(image_uri.begin(), image_uri.end(), '/', gDirUtilp->getDirDelimiter()[0]);
    const bool strip_extension = true;
    std::string stripped_uri = gDirUtilp->getBaseFileName(image_uri, strip_extension);

    // sometimes they can be really long and unwieldy - 64 chars is enough for anyone :) 
    const int max_texture_name_length = 64;
    if (stripped_uri.length() > max_texture_name_length)
    {
        stripped_uri = stripped_uri.substr(0, max_texture_name_length - 1);
    }

    // We intend to append the type of texture (base color, emissive etc.) to the 
    // name of the texture but sometimes the creator already did that.  To try
    // to avoid repeats (not perfect), we look for the texture type in the name
    // and if we find it, do not append the type, later on. One way this fails
    // (and it's fine for now) is I see some texture/image uris have a name like
    // "metallic roughness" and of course, that doesn't match our predefined
    // name "metallicroughness" - consider fix later..
    bool name_includes_type = false;
    std::string stripped_uri_lower = stripped_uri;
    LLStringUtil::toLower(stripped_uri_lower);
    stripped_uri_lower.erase(std::remove_if(stripped_uri_lower.begin(), stripped_uri_lower.end(), isspace), stripped_uri_lower.end());
    std::string texture_type_lower = texture_type;
    LLStringUtil::toLower(texture_type_lower);
    texture_type_lower.erase(std::remove_if(texture_type_lower.begin(), texture_type_lower.end(), isspace), texture_type_lower.end());
    if (stripped_uri_lower.find(texture_type_lower) != std::string::npos)
    {
        name_includes_type = true;
    }

    // uri doesn't include the type at all
    if (name_includes_type == false)
    {
        // uri doesn't include the type and the uri is not empty
        // so we can include everything
        if (stripped_uri.length() > 0)
        {
            // example "DamagedHelmet: base layer"
            return STRINGIZE(
                mMaterialNameShort <<
                ": " <<
                stripped_uri <<
                " (" <<
                texture_type <<
                ")"
            );
        }
        else
        // uri doesn't include the type (because the uri is empty)
        // so we must reorganize the string a bit to include the name
        // and an explicit name type
        {
            // example "DamagedHelmet: (Emissive)"
            return STRINGIZE(
                mMaterialNameShort <<
                " (" <<
                texture_type <<
                ")"
            );
        }
    }
    else
    // uri includes the type so just use it directly with the
    // name of the material
    {
        return STRINGIZE(
            // example: AlienBust: normal_layer
            mMaterialNameShort <<
            ": " <<
            stripped_uri
        );
    }
}

/**
 * Update the metadata for the material based on what we find in the loaded
 * file (along with some assumptions and interpretations...). Fields include
 * the name of the material, a material description and the names of the 
 * composite textures.
 */
void LLMaterialEditor::setFromGltfMetaData(const std::string& filename, const tinygltf::Model& model, S32 index)
{
    // Use the name (without any path/extension) of the file that was 
    // uploaded as the base of the material name. Then if the name of the 
    // scene is present and not blank, append that and use the result as
    // the name of the material. This is a first pass at creating a 
    // naming scheme that is useful to real content creators and hopefully
    // avoid 500 materials in your inventory called "scene" or "Default"
    const bool strip_extension = true;
    std::string base_filename = gDirUtilp->getBaseFileName(filename, strip_extension);

    // Extract the name of the scene. Note it is often blank or some very
    // generic name like "Scene" or "Default" so using this in the name
    // is less useful than you might imagine.
    std::string material_name;
    if (model.materials.size() > index && !model.materials[index].name.empty())
    {
        material_name = model.materials[index].name;
    }
    else if (model.scenes.size() > 0)
    {
        const tinygltf::Scene& scene_in = model.scenes[0];
        if (scene_in.name.length())
        {
            material_name = scene_in.name;
        }
        else
        {
            // scene name is empty so no point using it
        }
    }
    else
    {
        // scene name isn't present so no point using it
    }

    // If we have a valid material or scene name, use it to build the short and 
    // long versions of the material name. The long version is used 
    // as you might expect, for the material name. The short version is
    // used as part of the image/texture name - the theory is that will 
    // allow content creators to track the material and the corresponding
    // textures
    if (material_name.length())
    {
        mMaterialNameShort = base_filename;

        mMaterialName = STRINGIZE(
            base_filename << 
            " " << 
            "(" << 
            material_name <<
            ")"
        );
    }
    else
    // otherwise, just use the trimmed filename as is
    {
        mMaterialNameShort = base_filename;
        mMaterialName = base_filename;
    }

    // sanitize the material name so that it's compatible with the inventory
    LLInventoryObject::correctInventoryName(mMaterialName);
    LLInventoryObject::correctInventoryName(mMaterialNameShort);

    // We also set the title of the floater to match the 
    // name of the material
    setTitle(mMaterialName);

    /**
     * Extract / derive the names of each composite texture. For each, the 
     * index is used to to determine which of the "Images" is used. If the index
     * is -1 then that texture type is not present in the material (Seems to be 
     * quite common that a material is missing 1 or more types of texture)
     */
    if (model.materials.size() > index)
    {
        const tinygltf::Material& first_material = model.materials[index];

        mBaseColorName = MATERIAL_BASE_COLOR_DEFAULT_NAME;
        // note: unlike the other textures, base color doesn't have its own entry 
        // in the tinyGLTF Material struct. Rather, it is taken from a 
        // sub-texture in the pbrMetallicRoughness member
        int index = first_material.pbrMetallicRoughness.baseColorTexture.index;
        if (index > -1 && index < model.images.size())
        {
            // sanitize the name we decide to use for each texture
            std::string texture_name = getImageNameFromUri(model.images[index].uri, MATERIAL_BASE_COLOR_DEFAULT_NAME);
            LLInventoryObject::correctInventoryName(texture_name);
            mBaseColorName = texture_name;
        }

        mEmissiveName = MATERIAL_EMISSIVE_DEFAULT_NAME;
        index = first_material.emissiveTexture.index;
        if (index > -1 && index < model.images.size())
        {
            std::string texture_name = getImageNameFromUri(model.images[index].uri, MATERIAL_EMISSIVE_DEFAULT_NAME);
            LLInventoryObject::correctInventoryName(texture_name);
            mEmissiveName = texture_name;
        }

        mMetallicRoughnessName = MATERIAL_METALLIC_DEFAULT_NAME;
        index = first_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (index > -1 && index < model.images.size())
        {
            std::string texture_name = getImageNameFromUri(model.images[index].uri, MATERIAL_METALLIC_DEFAULT_NAME);
            LLInventoryObject::correctInventoryName(texture_name);
            mMetallicRoughnessName = texture_name;
        }

        mNormalName = MATERIAL_NORMAL_DEFAULT_NAME;
        index = first_material.normalTexture.index;
        if (index > -1 && index < model.images.size())
        {
            std::string texture_name = getImageNameFromUri(model.images[index].uri, MATERIAL_NORMAL_DEFAULT_NAME);
            LLInventoryObject::correctInventoryName(texture_name);
            mNormalName = texture_name;
        }
    }
}

void LLMaterialEditor::importMaterial()
{
    LLFilePickerReplyThread::startPicker(
        [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
            {
                if (LLAppViewer::instance()->quitRequested())
                {
                    return;
                }
                if (filenames.size() > 0)
                {
                    LLMaterialEditor::loadMaterialFromFile(filenames[0], -1);
                }
            },
        LLFilePicker::FFLOAD_MATERIAL,
        true);
}

class LLRenderMaterialFunctor : public LLSelectedTEFunctor
{
public:
    LLRenderMaterialFunctor(const LLUUID &id)
        : mMatId(id)
    {
    }

    bool apply(LLViewerObject* objectp, S32 te) override
    {
        if (objectp && objectp->permModify() && objectp->getVolume())
        {
            LLVOVolume* vobjp = (LLVOVolume*)objectp;
            vobjp->setRenderMaterialID(te, mMatId, false /*preview only*/);
            vobjp->updateTEMaterialTextures(te);
        }
        return true;
    }
private:
    LLUUID mMatId;
};

class LLRenderMaterialOverrideFunctor : public LLSelectedTEFunctor
{
public:
    LLRenderMaterialOverrideFunctor(LLMaterialEditor * me, std::string const & url)
    : mEditor(me), mCapUrl(url)
    {

    }

    bool apply(LLViewerObject* objectp, S32 te) override
    {
        // post override from given object and te to the simulator
        // requestData should have:
        //  object_id - UUID of LLViewerObject
        //  side - S32 index of texture entry
        //  gltf_json - String of GLTF json for override data

        if (objectp && objectp->permModify() && objectp->getVolume())
        {
            // Get material from object
            // Selection can cover multiple objects, and live editor is
            // supposed to overwrite changed values only
            LLTextureEntry* tep = objectp->getTE(te);
            LLPointer<LLGLTFMaterial> material = tep->getGLTFRenderMaterial();

            if (material.isNull())
            {
                // overrides are not supposed to work or apply if
                // there is no base material to work from
                return false;
            }

            // make a copy to not invalidate existing
            // material for multiple objects
            material = new LLGLTFMaterial(*material);

            // Override object's values with values from editor where appropriate
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_BASE_COLOR_DIRTY)
            {
                material->mBaseColor = mEditor->getBaseColor();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_BASE_TRANSPARENCY_DIRTY)
            {
                material->mBaseColor.mV[3] = mEditor->getTransparency();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_BASE_COLOR_TEX_DIRTY)
            {
                material->mBaseColorId = mEditor->getBaseColorId();
            }

            if (mEditor->getUnsavedChangesFlags() & MATERIAL_NORMAL_TEX_DIRTY)
            {
                material->mNormalId = mEditor->getNormalId();
            }

            if (mEditor->getUnsavedChangesFlags() & MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY)
            {
                material->mMetallicRoughnessId = mEditor->getMetallicRoughnessId();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY)
            {
                material->mMetallicFactor = mEditor->getMetalnessFactor();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY)
            {
                material->mRoughnessFactor = mEditor->getRoughnessFactor();
            }

            if (mEditor->getUnsavedChangesFlags() & MATERIAL_EMISIVE_COLOR_DIRTY)
            {
                material->mEmissiveColor = mEditor->getEmissiveColor();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_EMISIVE_TEX_DIRTY)
            {
                material->mEmissiveId = mEditor->getEmissiveId();
            }

            if (mEditor->getUnsavedChangesFlags() & MATERIAL_DOUBLE_SIDED_DIRTY)
            {
                material->mDoubleSided = mEditor->getDoubleSided();
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_ALPHA_MODE_DIRTY)
            {
                material->setAlphaMode(mEditor->getAlphaMode());
            }
            if (mEditor->getUnsavedChangesFlags() & MATERIAL_ALPHA_CUTOFF_DIRTY)
            {
                material->mAlphaCutoff = mEditor->getAlphaCutoff();
            }

            std::string overrides_json = material->asJSON();
            
            
            LLSD overrides = llsd::map(
                "object_id", objectp->getID(),
                "side", te,
                "gltf_json", overrides_json
            );
            LLCoros::instance().launch("modifyMaterialCoro", std::bind(&LLGLTFMaterialList::modifyMaterialCoro, mCapUrl, overrides));
        }
        return true;
    }

private:
    LLMaterialEditor * mEditor;
    std::string mCapUrl;
};

void LLMaterialEditor::applyToSelection()
{
    if (!mIsOverride)
    {
        // Only apply if working with 'live' materials
        // Might need a better way to distinguish 'live' mode.
        // But only one live edit is supposed to work at a time
        // as a pair to tools floater.
        return;
    }

    std::string url = gAgent.getRegionCapability("ModifyMaterialParams");
    if (!url.empty())
    {
        LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
        // TODO figure out how to get the right asset id in cases where we don't have a good one
        LLRenderMaterialOverrideFunctor override_func(this, url);
        selected_objects->applyToTEs(&override_func);

        // we posted all changes
        mUnsavedChanges = 0;
    }
    else
    {
        LL_WARNS("MaterialEditor") << "Not connected to materials capable region, missing ModifyMaterialParams cap" << LL_ENDL;

        // Fallback local preview. Will be removed once override systems is finished and new cap is deployed everywhere.
        LLPointer<LLFetchedGLTFMaterial> mat = new LLFetchedGLTFMaterial();
        getGLTFMaterial(mat);
        static const LLUUID placeholder("984e183e-7811-4b05-a502-d79c6f978a98");
        gGLTFMaterialList.addMaterial(placeholder, mat);
        LLRenderMaterialFunctor mat_func(placeholder);
        LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
        selected_objects->applyToTEs(&mat_func);
    }
}

void LLMaterialEditor::getGLTFMaterial(LLGLTFMaterial* mat)
{
    mat->mBaseColor = getBaseColor();
    mat->mBaseColor.mV[3] = getTransparency();
    mat->mBaseColorId = getBaseColorId();

    mat->mNormalId = getNormalId();

    mat->mMetallicRoughnessId = getMetallicRoughnessId();
    mat->mMetallicFactor = getMetalnessFactor();
    mat->mRoughnessFactor = getRoughnessFactor();

    mat->mEmissiveColor = getEmissiveColor();
    mat->mEmissiveId = getEmissiveId();

    mat->mDoubleSided = getDoubleSided();
    mat->setAlphaMode(getAlphaMode());
    mat->mAlphaCutoff = getAlphaCutoff();
}

void LLMaterialEditor::setFromGLTFMaterial(LLGLTFMaterial* mat)
{
    setBaseColor(mat->mBaseColor);
    setBaseColorId(mat->mBaseColorId);
    setNormalId(mat->mNormalId);

    setMetallicRoughnessId(mat->mMetallicRoughnessId);
    setMetalnessFactor(mat->mMetallicFactor);
    setRoughnessFactor(mat->mRoughnessFactor);

    setEmissiveColor(mat->mEmissiveColor);
    setEmissiveId(mat->mEmissiveId);

    setDoubleSided(mat->mDoubleSided);
    setAlphaMode(mat->getAlphaMode());
    setAlphaCutoff(mat->mAlphaCutoff);
}

bool LLMaterialEditor::setFromSelection()
{
    struct LLSelectedTEGetmatIdAndPermissions : public LLSelectedTEFunctor
    {
        LLSelectedTEGetmatIdAndPermissions(bool for_override)
            : mIsOverride(for_override)
            , mIdenticalTexColor(true)
            , mIdenticalTexMetal(true)
            , mIdenticalTexEmissive(true)
            , mIdenticalTexNormal(true)
            , mFirst(true)
        {}

        bool apply(LLViewerObject* objectp, S32 te_index)
        {
            if (!objectp)
            {
                return false;
            }
            LLUUID mat_id = objectp->getRenderMaterialID(te_index);
            bool can_use = mIsOverride ? objectp->permModify() : objectp->permCopy();
            LLTextureEntry *tep = objectp->getTE(te_index);
            // We might want to disable this entirely if at least
            // something in selection is no-copy or no modify
            // or has no base material
            if (can_use && tep && mat_id.notNull())
            {
                LLPointer<LLGLTFMaterial> mat = tep->getGLTFRenderMaterial();
                LLUUID tex_color_id;
                LLUUID tex_metal_id;
                LLUUID tex_emissive_id;
                LLUUID tex_normal_id;
                llassert(mat.notNull()); // by this point shouldn't be null
                if (mat.notNull())
                {
                    tex_color_id = mat->mBaseColorId;
                    tex_metal_id = mat->mMetallicRoughnessId;
                    tex_emissive_id = mat->mEmissiveId;
                    tex_normal_id = mat->mNormalId;
                }
                if (mFirst)
                {
                    mMaterial = mat;
                    mTexColorId = tex_color_id;
                    mTexMetalId = tex_metal_id;
                    mTexEmissiveId = tex_emissive_id;
                    mTexNormalId = tex_normal_id;
                    mFirst = false;
                }
                else
                {
                    if (mTexColorId != tex_color_id)
                    {
                        mIdenticalTexColor = false;
                    }
                    if (mTexMetalId != tex_metal_id)
                    {
                        mIdenticalTexMetal = false;
                    }
                    if (mTexEmissiveId != tex_emissive_id)
                    {
                        mIdenticalTexEmissive = false;
                    }
                    if (mTexNormalId != tex_normal_id)
                    {
                        mIdenticalTexNormal = false;
                    }
                }
            }
            return true;
        }
        bool mIsOverride;
        bool mIdenticalTexColor;
        bool mIdenticalTexMetal;
        bool mIdenticalTexEmissive;
        bool mIdenticalTexNormal;
        bool mFirst;
        LLUUID mTexColorId;
        LLUUID mTexMetalId;
        LLUUID mTexEmissiveId;
        LLUUID mTexNormalId;
        LLPointer<LLGLTFMaterial> mMaterial;
    } func(mIsOverride);

    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);
    if (func.mMaterial.notNull())
    {
        setFromGLTFMaterial(func.mMaterial);
        setEnableEditing(true);
    }
    else
    {
        // pick defaults from a blank material;
        LLGLTFMaterial blank_mat;
        setFromGLTFMaterial(&blank_mat);
        if (mIsOverride)
        {
            setEnableEditing(false);
        }
    }

    if (mIsOverride)
    {
        mBaseColorTextureCtrl->setTentative(!func.mIdenticalTexColor);
        mMetallicTextureCtrl->setTentative(!func.mIdenticalTexMetal);
        mEmissiveTextureCtrl->setTentative(!func.mIdenticalTexEmissive);
        mNormalTextureCtrl->setTentative(!func.mIdenticalTexNormal);
    }

    return func.mMaterial.notNull();
}


void LLMaterialEditor::loadAsset()
{
    const LLInventoryItem* item;
    if (mNotecardInventoryID.notNull())
    {
        item = mAuxItem.get();
    }
    else
    {
        item = getItem();
    }
    
    bool fail = false;

    if (item)
    {
        LLPermissions perm(item->getPermissions());
        bool allow_copy = gAgent.allowOperation(PERM_COPY, perm, GP_OBJECT_MANIPULATE);
        bool allow_modify = canModify(mObjectUUID, item);
        bool source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(mItemUUID, gInventory.getLibraryRootFolderID());

        setCanSaveAs(allow_copy);
        setMaterialName(item->getName());

        {
            mAssetID = item->getAssetUUID();

            if (mAssetID.isNull())
            {
                mAssetStatus = PREVIEW_ASSET_LOADED;
                loadDefaults();
                resetUnsavedChanges();
                setEnableEditing(allow_modify && !source_library);
            }
            else
            {
                LLHost source_sim = LLHost();
                LLSD* user_data = new LLSD();

                if (mNotecardInventoryID.notNull())
                {
                    user_data->with("objectid", mNotecardObjectID).with("notecardid", mNotecardInventoryID);
                }
                else if (mObjectUUID.notNull())
                {
                    LLViewerObject* objectp = gObjectList.findObject(mObjectUUID);
                    if (objectp && objectp->getRegion())
                    {
                        source_sim = objectp->getRegion()->getHost();
                    }
                    else
                    {
                        // The object that we're trying to look at disappeared, bail.
                        LL_WARNS("MaterialEditor") << "Can't find object " << mObjectUUID << " associated with material." << LL_ENDL;
                        mAssetID.setNull();
                        mAssetStatus = PREVIEW_ASSET_LOADED;
                        resetUnsavedChanges();
                        setEnableEditing(allow_modify && !source_library);
                        return;
                    }
                    user_data->with("taskid", mObjectUUID).with("itemid", mItemUUID);
                }
                else
                {
                    user_data = new LLSD(mItemUUID);
                }

                setEnableEditing(false); // wait for it to load

                // request the asset.
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
    LLSD* floater_key = (LLSD*)user_data;
    LL_DEBUGS("MaterialEditor") << "loading " << asset_uuid << " for " << *floater_key << LL_ENDL;
    LLMaterialEditor* editor = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", *floater_key);
    if (editor)
    {
        if (asset_uuid != editor->mAssetID)
        {
            LL_WARNS() << "Asset id mismatch, expected: " << editor->mAssetID << " got: " << asset_uuid << LL_ENDL;
        }
        if (0 == status)
        {
            LLFileSystem file(asset_uuid, type, LLFileSystem::READ);

            S32 file_length = file.getSize();

            std::vector<char> buffer(file_length + 1);
            file.read((U8*)&buffer[0], file_length);

            editor->decodeAsset(buffer);

            BOOL allow_modify = editor->canModify(editor->mObjectUUID, editor->getItem());
            BOOL source_library = editor->mObjectUUID.isNull() && gInventory.isObjectDescendentOf(editor->mItemUUID, gInventory.getLibraryRootFolderID());
            editor->setEnableEditing(allow_modify && !source_library);
            editor->resetUnsavedChanges();
            editor->mAssetStatus = PREVIEW_ASSET_LOADED;
            editor->setEnabled(true); // ready for use
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
            editor->setEnableEditing(false);

            LL_WARNS() << "Problem loading material: " << status << LL_ENDL;
            editor->mAssetStatus = PREVIEW_ASSET_ERROR;
        }
    }
    else
    {
        LL_DEBUGS("MaterialEditor") << "Floater " << *floater_key << " does not exist." << LL_ENDL;
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


void LLMaterialEditor::saveTexture(LLImageJ2C* img, const std::string& name, const LLUUID& asset_id, upload_callback_f cb)
{
    if (asset_id.isNull()
        || img == nullptr
        || img->getDataSize() == 0)
    {
        return;
    }

    // copy image bytes into string
    std::string buffer;
    buffer.assign((const char*) img->getData(), img->getDataSize());

    U32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();


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
        false,
        cb));

    upload_new_resource(uploadInfo);
}

S32 LLMaterialEditor::saveTextures()
{
    S32 work_count = 0;
    LLSD key = getKey(); // must be locally declared for lambda's capture to work
    if (mBaseColorTextureUploadId == getBaseColorId() && mBaseColorTextureUploadId.notNull())
    {
        mUploadingTexturesCount++;
        work_count++;
        saveTexture(mBaseColorJ2C, mBaseColorName, mBaseColorTextureUploadId, [key](LLUUID newAssetId, LLSD response)
        {
            LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", key);
            if (me)
            {
                if (response["success"].asBoolean())
                {
                    me->setBaseColorId(newAssetId);
                }
                else
                {
                    // To make sure that we won't retry (some failures can cb immediately)
                    me->setBaseColorId(LLUUID::null);
                }
                me->mUploadingTexturesCount--;

                // try saving
                me->saveIfNeeded();
            }
        });
    }
    if (mNormalTextureUploadId == getNormalId() && mNormalTextureUploadId.notNull())
    {
        mUploadingTexturesCount++;
        work_count++;
        saveTexture(mNormalJ2C, mNormalName, mNormalTextureUploadId, [key](LLUUID newAssetId, LLSD response)
        {
            LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", key);
            if (me)
            {
                if (response["success"].asBoolean())
                {
                    me->setNormalId(newAssetId);
                }
                else
                {
                    me->setNormalId(LLUUID::null);
                }
                me->setNormalId(newAssetId);
                me->mUploadingTexturesCount--;

                // try saving
                me->saveIfNeeded();
            }
        });
    }
    if (mMetallicTextureUploadId == getMetallicRoughnessId() && mMetallicTextureUploadId.notNull())
    {
        mUploadingTexturesCount++;
        work_count++;
        saveTexture(mMetallicRoughnessJ2C, mMetallicRoughnessName, mMetallicTextureUploadId, [key](LLUUID newAssetId, LLSD response)
        {
            LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", key);
            if (me)
            {
                if (response["success"].asBoolean())
                {
                    me->setMetallicRoughnessId(newAssetId);
                }
                else
                {
                    me->setMetallicRoughnessId(LLUUID::null);
                }
                me->mUploadingTexturesCount--;

                // try saving
                me->saveIfNeeded();
            }
        });
    }

    if (mEmissiveTextureUploadId == getEmissiveId() && mEmissiveTextureUploadId.notNull())
    {
        mUploadingTexturesCount++;
        work_count++;
        saveTexture(mEmissiveJ2C, mEmissiveName, mEmissiveTextureUploadId, [key](LLUUID newAssetId, LLSD response)
        {
            LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", LLSD(key));
            if (me)
            {
                if (response["success"].asBoolean())
                {
                    me->setEmissiveId(newAssetId);
                }
                else
                {
                    me->setEmissiveId(LLUUID::null);
                }
                me->mUploadingTexturesCount--;

                // try saving
                me->saveIfNeeded();
            }
        });
    }

    // discard upload buffers once textures have been saved
    clearTextures();

    // asset storage can callback immediately, causing a decrease
    // of mUploadingTexturesCount, report amount of work scheduled
    // not amount of work remaining
    return work_count;
}

void LLMaterialEditor::clearTextures()
{
    mBaseColorJ2C = nullptr;
    mNormalJ2C = nullptr;
    mEmissiveJ2C = nullptr;
    mMetallicRoughnessJ2C = nullptr;

    mBaseColorFetched = nullptr;
    mNormalFetched = nullptr;
    mMetallicRoughnessFetched = nullptr;
    mEmissiveFetched = nullptr;

    mBaseColorTextureUploadId.setNull();
    mNormalTextureUploadId.setNull();
    mMetallicTextureUploadId.setNull();
    mEmissiveTextureUploadId.setNull();
}

void LLMaterialEditor::loadDefaults()
{
    tinygltf::Model model_in;
    model_in.materials.resize(1);
    setFromGltfModel(model_in, 0, true);
}

