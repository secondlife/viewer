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
#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llfloaterreg.h"
#include "llfilesystem.h"
#include "llgltfmateriallist.h"
#include "llinventorymodel.h"
#include "lllocalgltfmaterials.h"
#include "llnotificationsutil.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewermenufile.h"
#include "llviewertexture.h"
#include "llsdutil.h"
#include "llselectmgr.h"
#include "llstatusbar.h"	// can_afford_transaction()
#include "lltoolpie.h"
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

// Dirty flags
static const U32 MATERIAL_BASE_COLOR_DIRTY = 0x1 << 0;
static const U32 MATERIAL_BASE_COLOR_TEX_DIRTY = 0x1 << 1;

static const U32 MATERIAL_NORMAL_TEX_DIRTY = 0x1 << 2;

static const U32 MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY = 0x1 << 3;
static const U32 MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY = 0x1 << 4;
static const U32 MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY = 0x1 << 5;

static const U32 MATERIAL_EMISIVE_COLOR_DIRTY = 0x1 << 6;
static const U32 MATERIAL_EMISIVE_TEX_DIRTY = 0x1 << 7;

static const U32 MATERIAL_DOUBLE_SIDED_DIRTY = 0x1 << 8;
static const U32 MATERIAL_ALPHA_MODE_DIRTY = 0x1 << 9;
static const U32 MATERIAL_ALPHA_CUTOFF_DIRTY = 0x1 << 10;

LLUUID LLMaterialEditor::mOverrideObjectId;
S32 LLMaterialEditor::mOverrideObjectTE = -1;
bool LLMaterialEditor::mOverrideInProgress = false;
bool LLMaterialEditor::mSelectionNeedsUpdate = true;

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
/// Class LLSelectedTEGetMatData
/// For finding selected applicable inworld material
///----------------------------------------------------------------------------

struct LLSelectedTEGetMatData : public LLSelectedTEFunctor
{
    LLSelectedTEGetMatData(bool for_override);

    bool apply(LLViewerObject* objectp, S32 te_index);

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
    LLUUID mObjectId;
    S32 mObjectTE;
    LLPointer<LLGLTFMaterial> mMaterial;
    LLPointer<LLLocalGLTFMaterial> mLocalMaterial;
};

LLSelectedTEGetMatData::LLSelectedTEGetMatData(bool for_override)
    : mIsOverride(for_override)
    , mIdenticalTexColor(true)
    , mIdenticalTexMetal(true)
    , mIdenticalTexEmissive(true)
    , mIdenticalTexNormal(true)
    , mObjectTE(-1)
    , mFirst(true)
{}

bool LLSelectedTEGetMatData::apply(LLViewerObject* objectp, S32 te_index)
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
        if (mIsOverride)
        {
            LLPointer<LLGLTFMaterial> mat = tep->getGLTFRenderMaterial();

            LLUUID tex_color_id;
            LLUUID tex_metal_id;
            LLUUID tex_emissive_id;
            LLUUID tex_normal_id;
            llassert(mat.notNull()); // by this point shouldn't be null
            if (mat.notNull())
            {
                tex_color_id = mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR];
                tex_metal_id = mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS];
                tex_emissive_id = mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE];
                tex_normal_id = mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL];
            }
            if (mFirst)
            {
                mMaterial = mat;
                mTexColorId = tex_color_id;
                mTexMetalId = tex_metal_id;
                mTexEmissiveId = tex_emissive_id;
                mTexNormalId = tex_normal_id;
                mObjectTE = te_index;
                mObjectId = objectp->getID();
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
        else
        {
            LLGLTFMaterial *mat = tep->getGLTFMaterial();
            LLLocalGLTFMaterial *local_mat = dynamic_cast<LLLocalGLTFMaterial*>(mat);

            if (local_mat)
            {
                mLocalMaterial = local_mat;
            }
            mMaterial = tep->getGLTFRenderMaterial();

            if (mMaterial.isNull())
            {
                // Shouldn't be possible?
                LL_WARNS("MaterialEditor") << "Object has material id, but no material" << LL_ENDL;
                mMaterial = gGLTFMaterialList.getMaterial(mat_id);
            }
        }
        return true;
    }
    return false;
}

///----------------------------------------------------------------------------
/// Class LLMaterialEditor
///----------------------------------------------------------------------------

// Default constructor
LLMaterialEditor::LLMaterialEditor(const LLSD& key)
    : LLPreview(key)
    , mUnsavedChanges(0)
    , mRevertedChanges(0)
    , mExpectedUploadCost(0)
    , mUploadingTexturesCount(0)
    , mUploadingTexturesFailure(false)
{
    const LLInventoryItem* item = getItem();
    if (item)
    {
        mAssetID = item->getAssetUUID();
    }
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
    // if this is a 'live editor' instance, it is also
    // single instance and uses live overrides
    mIsOverride = getIsSingleInstance();

    mBaseColorTextureCtrl = getChild<LLTextureCtrl>("base_color_texture");
    mMetallicTextureCtrl = getChild<LLTextureCtrl>("metallic_roughness_texture");
    mEmissiveTextureCtrl = getChild<LLTextureCtrl>("emissive_texture");
    mNormalTextureCtrl = getChild<LLTextureCtrl>("normal_texture");
    mBaseColorCtrl = getChild<LLColorSwatchCtrl>("base color");
    mEmissiveColorCtrl = getChild<LLColorSwatchCtrl>("emissive color");

    if (!gAgent.isGodlike())
    {
        // Only allow fully permissive textures
        mBaseColorTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
        mMetallicTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
        mEmissiveTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
        mNormalTextureCtrl->setImmediateFilterPermMask(PERM_COPY | PERM_TRANSFER);
    }

    // Texture callback
    mBaseColorTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitTexture, this, _1, _2, MATERIAL_BASE_COLOR_TEX_DIRTY));
    mMetallicTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitTexture, this, _1, _2, MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY));
    mEmissiveTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitTexture, this, _1, _2, MATERIAL_EMISIVE_TEX_DIRTY));
    mNormalTextureCtrl->setCommitCallback(boost::bind(&LLMaterialEditor::onCommitTexture, this, _1, _2, MATERIAL_NORMAL_TEX_DIRTY));

    if (mIsOverride)
    {
        // Material override change success callback
        LLGLTFMaterialList::addSelectionUpdateCallback(&LLMaterialEditor::updateLive);

        // Live editing needs a recovery mechanism on cancel
        mBaseColorTextureCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_BASE_COLOR_TEX_DIRTY));
        mMetallicTextureCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY));
        mEmissiveTextureCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_EMISIVE_TEX_DIRTY));
        mNormalTextureCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_NORMAL_TEX_DIRTY));

        // Save applied changes on 'OK' to our recovery mechanism.
        mBaseColorTextureCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_BASE_COLOR_TEX_DIRTY));
        mMetallicTextureCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY));
        mEmissiveTextureCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_EMISIVE_TEX_DIRTY));
        mNormalTextureCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_NORMAL_TEX_DIRTY));
    }
    else
    {
        mBaseColorTextureCtrl->setCanApplyImmediately(false);
        mMetallicTextureCtrl->setCanApplyImmediately(false);
        mEmissiveTextureCtrl->setCanApplyImmediately(false);
        mNormalTextureCtrl->setCanApplyImmediately(false);
    }

    if (!mIsOverride)
    {
        childSetAction("save", boost::bind(&LLMaterialEditor::onClickSave, this));
        childSetAction("save_as", boost::bind(&LLMaterialEditor::onClickSaveAs, this));
        childSetAction("cancel", boost::bind(&LLMaterialEditor::onClickCancel, this));
    }

    if (mIsOverride)
    {
        childSetVisible("base_color_upload_fee", FALSE);
        childSetVisible("metallic_upload_fee", FALSE);
        childSetVisible("emissive_upload_fee", FALSE);
        childSetVisible("normal_upload_fee", FALSE);
    }
    else
    {
        S32 upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost();
        getChild<LLUICtrl>("base_color_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
        getChild<LLUICtrl>("metallic_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
        getChild<LLUICtrl>("emissive_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
        getChild<LLUICtrl>("normal_upload_fee")->setTextArg("[FEE]", llformat("%d", upload_cost));
    }

    boost::function<void(LLUICtrl*, void*)> changes_callback = [this](LLUICtrl * ctrl, void* userData)
    {
        const U32 *flag = (const U32*)userData;
        markChangesUnsaved(*flag);
        // Apply changes to object live
        applyToSelection();
    };
 
    childSetCommitCallback("double sided", changes_callback, (void*)&MATERIAL_DOUBLE_SIDED_DIRTY);

    // BaseColor
    mBaseColorCtrl->setCommitCallback(changes_callback, (void*)&MATERIAL_BASE_COLOR_DIRTY);
    if (mIsOverride)
    {
        mBaseColorCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_BASE_COLOR_DIRTY));
        mBaseColorCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_BASE_COLOR_DIRTY));
    }
    else
    {
        mBaseColorCtrl->setCanApplyImmediately(false);
    }
    // transparency is a part of base color
    childSetCommitCallback("transparency", changes_callback, (void*)&MATERIAL_BASE_COLOR_DIRTY);
    childSetCommitCallback("alpha mode", changes_callback, (void*)&MATERIAL_ALPHA_MODE_DIRTY);
    childSetCommitCallback("alpha cutoff", changes_callback, (void*)&MATERIAL_ALPHA_CUTOFF_DIRTY);

    // Metallic-Roughness
    childSetCommitCallback("metalness factor", changes_callback, (void*)&MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY);
    childSetCommitCallback("roughness factor", changes_callback, (void*)&MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY);

    // Emissive
    mEmissiveColorCtrl->setCommitCallback(changes_callback, (void*)&MATERIAL_EMISIVE_COLOR_DIRTY);
    if (mIsOverride)
    {
        mEmissiveColorCtrl->setOnCancelCallback(boost::bind(&LLMaterialEditor::onCancelCtrl, this, _1, _2, MATERIAL_EMISIVE_COLOR_DIRTY));
        mEmissiveColorCtrl->setOnSelectCallback(boost::bind(&LLMaterialEditor::onSelectCtrl, this, _1, _2, MATERIAL_EMISIVE_COLOR_DIRTY));
    }
    else
    {
        mEmissiveColorCtrl->setCanApplyImmediately(false);
    }

    if (!mIsOverride)
    {
        // "unsaved_changes" doesn't exist in live editor
        childSetVisible("unsaved_changes", mUnsavedChanges);

        // Doesn't exist in live editor
        getChild<LLUICtrl>("total_upload_fee")->setTextArg("[FEE]", llformat("%d", 0));
    }

    // Todo:
    // Disable/enable setCanApplyImmediately() based on
    // working from inventory, upload or editing inworld

	return LLPreview::postBuild();
}

void LLMaterialEditor::onClickCloseBtn(bool app_quitting)
{
    if (app_quitting || mIsOverride)
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

void LLMaterialEditor::draw()
{
    if (mIsOverride)
    {
        bool selection_empty = LLSelectMgr::getInstance()->getSelection()->isEmpty();
        if (selection_empty && mHasSelection)
        {
            mSelectionNeedsUpdate = true;
        }

        if (mSelectionNeedsUpdate)
        {
            mSelectionNeedsUpdate = false;
            clearTextures();
            setFromSelection();
        }
    }
    LLPreview::draw();
}

void LLMaterialEditor::handleReshape(const LLRect& new_rect, bool by_user)
{
    if (by_user)
    {
        const LLRect old_rect = getRect();
        LLRect clamp_rect(new_rect);
        clamp_rect.mRight = clamp_rect.mLeft + old_rect.getWidth();
        LLPreview::handleReshape(clamp_rect, by_user);
    }
    else
    {
        LLPreview::handleReshape(new_rect, by_user);
    }
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
    LLColor4 ret = linearColor4(LLColor4(mBaseColorCtrl->getValue()));
    ret.mV[3] = getTransparency();
    return ret;
}

void LLMaterialEditor::setBaseColor(const LLColor4& color)
{
    mBaseColorCtrl->setValue(srgbColor4(color).getValue());
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
    return linearColor4(LLColor4(mEmissiveColorCtrl->getValue()));
}

void LLMaterialEditor::setEmissiveColor(const LLColor4& color)
{
    mEmissiveColorCtrl->setValue(srgbColor4(color).getValue());
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
    mRevertedChanges = 0;
    if (!mIsOverride)
    {
        childSetVisible("unsaved_changes", false);
        setCanSave(false);

        mExpectedUploadCost = 0;
        getChild<LLUICtrl>("total_upload_fee")->setTextArg("[FEE]", llformat("%d", mExpectedUploadCost));
    }
}

void LLMaterialEditor::markChangesUnsaved(U32 dirty_flag)
{
    mUnsavedChanges |= dirty_flag;
    if (mIsOverride)
    {
        // at the moment live editing (mIsOverride) applies everything 'live'
        // and "unsaved_changes", save/cancel buttons don't exist there
        return;
    }

    childSetVisible("unsaved_changes", mUnsavedChanges);

    if (mUnsavedChanges)
    {
        const LLInventoryItem* item = getItem();
        if (item)
        {
            //LLPermissions perm(item->getPermissions());
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
    if (!mIsOverride)
    {
        childSetEnabled("save_as", value);
    }
}

void LLMaterialEditor::setCanSave(bool value)
{
    if (!mIsOverride)
    {
        childSetEnabled("save", value);
    }
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

void LLMaterialEditor::onCommitTexture(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag)
{
    if (!mIsOverride)
    {
        std::string upload_fee_ctrl_name;
        LLUUID old_uuid;

        switch (dirty_flag)
        {
        case MATERIAL_BASE_COLOR_TEX_DIRTY:
        {
            upload_fee_ctrl_name = "base_color_upload_fee";
            old_uuid = mBaseColorTextureUploadId;
            break;
        }
        case MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY:
        {
            upload_fee_ctrl_name = "metallic_upload_fee";
            old_uuid = mMetallicTextureUploadId;
            break;
        }
        case MATERIAL_EMISIVE_TEX_DIRTY:
        {
            upload_fee_ctrl_name = "emissive_upload_fee";
            old_uuid = mEmissiveTextureUploadId;
            break;
        }
        case MATERIAL_NORMAL_TEX_DIRTY:
        {
            upload_fee_ctrl_name = "normal_upload_fee";
            old_uuid = mNormalTextureUploadId;
            break;
        }
        default:
            break;
        }
        LLUUID new_val = ctrl->getValue().asUUID();
        if (new_val == old_uuid && old_uuid.notNull())
        {
            childSetValue(upload_fee_ctrl_name, getString("upload_fee_string"));
        }
        else
        {
            // Texture picker has 'apply now' with 'cancel' support.
            // Don't clean mBaseColorJ2C and mBaseColorFetched, it's our
            // storage in case user decides to cancel changes.
            // Without mBaseColorFetched, viewer will eventually cleanup
            // the texture that is not in use
            childSetValue(upload_fee_ctrl_name, getString("no_upload_fee_string"));
        }
    }

    markChangesUnsaved(dirty_flag);
    applyToSelection();
}

void LLMaterialEditor::onCancelCtrl(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag)
{
    mRevertedChanges |= dirty_flag;
    applyToSelection();
}

void LLMaterialEditor::onSelectCtrl(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag)
{
    mUnsavedChanges |= dirty_flag;
    applyToSelection();

    struct f : public LLSelectedNodeFunctor
    {
        f(LLUICtrl* ctrl, S32 dirty_flag) : mCtrl(ctrl), mDirtyFlag(dirty_flag)
        {
        }

        virtual bool apply(LLSelectNode* nodep)
        {
            LLViewerObject* objectp = nodep->getObject();
            if (!objectp)
            {
                return false;
            }
            S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces()); // avatars have TEs but no faces
            for (S32 te = 0; te < num_tes; ++te)
            {
                if (nodep->isTESelected(te) && nodep->mSavedGLTFOverrideMaterials.size() > te)
                {
                    if (nodep->mSavedGLTFOverrideMaterials[te].isNull())
                    {
                        // populate with default values, default values basically mean 'not in use'
                        nodep->mSavedGLTFOverrideMaterials[te] = new LLGLTFMaterial();
                    }

                    switch (mDirtyFlag)
                    {
                    //Textures
                    case MATERIAL_BASE_COLOR_TEX_DIRTY:
                    {
                        nodep->mSavedGLTFOverrideMaterials[te]->setBaseColorId(mCtrl->getValue().asUUID(), true);
                        break;
                    }
                    case MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY:
                    {
                        nodep->mSavedGLTFOverrideMaterials[te]->setOcclusionRoughnessMetallicId(mCtrl->getValue().asUUID(), true);
                        break;
                    }
                    case MATERIAL_EMISIVE_TEX_DIRTY:
                    {
                        nodep->mSavedGLTFOverrideMaterials[te]->setEmissiveId(mCtrl->getValue().asUUID(), true);
                        break;
                    }
                    case MATERIAL_NORMAL_TEX_DIRTY:
                    {
                        nodep->mSavedGLTFOverrideMaterials[te]->setNormalId(mCtrl->getValue().asUUID(), true);
                        break;
                    }
                    // Colors
                    case MATERIAL_BASE_COLOR_DIRTY:
                    {
                        LLColor4 ret = linearColor4(LLColor4(mCtrl->getValue()));
                        // except transparency
                        ret.mV[3] = nodep->mSavedGLTFOverrideMaterials[te]->mBaseColor.mV[3];
                        nodep->mSavedGLTFOverrideMaterials[te]->setBaseColorFactor(ret, true);
                        break;
                    }
                    case MATERIAL_EMISIVE_COLOR_DIRTY:
                    {
                        nodep->mSavedGLTFOverrideMaterials[te]->setEmissiveColorFactor(LLColor3(mCtrl->getValue()), true);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
            return true;
        }

        LLUICtrl* mCtrl;
        S32 mDirtyFlag;
    } func(ctrl, dirty_flag);

    LLSelectMgr::getInstance()->getSelection()->applyToNodes(&func);
}

void LLMaterialEditor::onClickSave()
{
    if (!capabilitiesAvailable())
    {
        LLNotificationsUtil::add("MissingMaterialCaps");
        return;
    }
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

std::string LLMaterialEditor::getEncodedAsset()
{
    LLSD asset;
    asset["version"] = LLGLTFMaterial::ASSET_VERSION;
    asset["type"] = LLGLTFMaterial::ASSET_TYPE;
    LLGLTFMaterial mat;
    getGLTFMaterial(&mat);
    asset["data"] = mat.asJSON();

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
        if (asset.has("version") && LLGLTFMaterial::isAcceptedVersion(asset["version"].asString()))
        {
            if (asset.has("type") && asset["type"] == LLGLTFMaterial::ASSET_TYPE)
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
                        // *NOTE: This duplicates some functionality from
                        // LLGLTFMaterial::fromJSON, but currently does the job
                        // better for the material editor use case.
                        // However, LLGLTFMaterial::asJSON should always be
                        // used when uploading materials, to ensure the
                        // asset is valid.
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
        // Upload already in progress, wait until
        // textures upload will retry saving on callback.
        // Also should prevent some failure-callbacks
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
        if (!updateInventoryItem(buffer, mItemUUID, mObjectUUID))
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
    { 
        //make a new inventory item
        std::string res_desc = buildMaterialDescription();
        createInventoryItem(buffer, mMaterialName, res_desc);

        // We do not update floater with uploaded asset yet, so just close it.
        closeFloater();
    }
    
    return true;
}

// static
bool LLMaterialEditor::updateInventoryItem(const std::string &buffer, const LLUUID &item_id, const LLUUID &task_id)
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
                [](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD)
                {
                    // done callback
                    LLMaterialEditor::finishInventoryUpload(itemId, newAssetId, newItemId);
                },
                [](LLUUID itemId, LLUUID taskId, LLSD response, std::string reason)
                {
                    // failure callback
                    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", LLSD(itemId));
                    if (me)
                    {
                        me->setEnabled(true);
                    }
                    return true;
                }
                );
            url = agent_url;
        }
        else if (!task_id.isNull() && !task_url.empty())
        {
            uploadInfo = std::make_shared<LLBufferedAssetUploadInfo>(task_id, item_id, LLAssetType::AT_MATERIAL, buffer,
                [](LLUUID itemId, LLUUID task_id, LLUUID newAssetId, LLSD)
                {
                    // done callback
                    LLMaterialEditor::finishTaskUpload(itemId, newAssetId, task_id);
                },
                [](LLUUID itemId, LLUUID task_id, LLSD response, std::string reason)
                {
                    // failure callback
                    LLSD floater_key;
                    floater_key["taskid"] = task_id;
                    floater_key["itemid"] = itemId;
                    LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", floater_key);
                    if (me)
                    {
                        me->setEnabled(true);
                    }
                    return true;
                }
                );
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

void LLMaterialEditor::createInventoryItem(const std::string &buffer, const std::string &name, const std::string &desc)
{
    // gen a new uuid for this asset
    LLTransactionID tid;
    tid.generate();     // timestamp-based randomization + uniquification
    U32 next_owner_perm = LLFloaterPerms::getNextOwnerPerms("Materials");
    LLUUID parent = gInventory.findUserDefinedCategoryUUIDForType(LLFolderType::FT_MATERIAL);
    const U8 subtype = NO_INV_SUBTYPE;  // TODO maybe use AT_SETTINGS and LLSettingsType::ST_MATERIAL ?

    create_inventory_item(gAgent.getID(), gAgent.getSessionID(), parent, tid, name, desc,
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
                [](LLUUID item_id, LLUUID new_asset_id, LLUUID new_item_id, LLSD response)
                {
                    // done callback
                    LL_INFOS("Material") << "inventory item uploaded.  item: " << item_id << " asset: " << new_asset_id << " new_item_id: " << new_item_id << " response: " << response << LL_ENDL;
                    LLSD params = llsd::map("ASSET_ID", new_asset_id);
                    LLNotificationsUtil::add("MaterialCreated", params);
                },
                nullptr // failure callback, floater already closed
            );

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
                if (!updateInventoryItem(buffer, newItemId, LLUUID::null))
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
            updateInventoryItem(buffer, newItemId, LLUUID::null);
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
    if (!LLMaterialEditor::capabilitiesAvailable())
    {
        LLNotificationsUtil::add("MissingMaterialCaps");
        return;
    }

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

void LLMaterialEditor::uploadMaterialFromFile(const std::string& filename, S32 index)
{
    if (index < 0 || !LLMaterialEditor::capabilitiesAvailable())
    {
        return;
    }

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
        return;
    }

    if (index >= 0 && model_in.materials.size() <= index)
    {
        // material is missing
        return;
    }

    // Todo: no point in loading whole editor
    // This uses 'filename' to make sure multiple bulk uploads work
    // instead of fighting for a single instance.
    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor", LLSD().with("filename", filename).with("index", LLSD::Integer(index)));
    me->loadMaterial(model_in, filename_lc, index, false);
    me->saveIfNeeded();
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
    // Drop selection updates if we are waiting for
    // overrides to finish applying to not reset values
    // (might need a timeout)
    if (!mOverrideInProgress)
    {
        // mUpdateSignal triggers a lot per frame, breakwater
        mSelectionNeedsUpdate = true;
    }
}

void LLMaterialEditor::updateLive()
{
    mSelectionNeedsUpdate = true;
    mOverrideInProgress = false;
}

void LLMaterialEditor::updateLive(const LLUUID &object_id, S32 te)
{
    if (mOverrideObjectId != object_id
        || mOverrideObjectTE != te)
    {
        // Ignore if waiting for override,
        // if not waiting, mark selection dirty
        mSelectionNeedsUpdate |= !mOverrideInProgress;
        return;
    }

    // update for currently displayed object and face
    mSelectionNeedsUpdate = true;
    mOverrideInProgress = false;
}

void LLMaterialEditor::loadLive()
{
    LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("live_material_editor");
    if (me)
    {
        me->mOverrideInProgress = false;
        me->setFromSelection();

        // Set up for selection changes updates
        if (!me->mSelectionUpdateSlot.connected())
        {
            me->mSelectionUpdateSlot = LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLMaterialEditor::onSelectionChanged, me));
        }

        me->openFloater();
        me->setFocus(TRUE);
    }
}

void LLMaterialEditor::saveObjectsMaterialAs()
{
    LLSelectedTEGetMatData func(false);
    LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func, true /*first applicable*/);
    saveMaterialAs(func.mMaterial, func.mLocalMaterial);
}
void LLMaterialEditor::savePickedMaterialAs()
{
    LLPickInfo pick = LLToolPie::getInstance()->getPick();
    if (pick.mPickType != LLPickInfo::PICK_OBJECT || !pick.getObject())
    {
        return;
    }

    LLPointer<LLGLTFMaterial> render_material;
    LLPointer<LLLocalGLTFMaterial> local_material;

    LLViewerObject *objectp = pick.getObject();
    LLUUID mat_id = objectp->getRenderMaterialID(pick.mObjectFace);
    if (mat_id.notNull() && objectp->permCopy())
    {
        // Try a face user picked first
        // (likely the only method we need, but in such case
        // enable_object_save_gltf_material will need to check this)
        LLTextureEntry *tep = objectp->getTE(pick.mObjectFace);
        LLGLTFMaterial *mat = tep->getGLTFMaterial();
        LLLocalGLTFMaterial *local_mat = dynamic_cast<LLLocalGLTFMaterial*>(mat);

        if (local_mat)
        {
            local_material = local_mat;
        }
        render_material = tep->getGLTFRenderMaterial();
    }
    else
    {
        // Find an applicable material.
        // Do this before showing message, because
        // message is going to drop selection.
        LLSelectedTEGetMatData func(false);
        LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func, true /*first applicable*/);
        local_material = func.mLocalMaterial;
        render_material = func.mMaterial;
    }

    saveMaterialAs(render_material, local_material);
}

void LLMaterialEditor::saveMaterialAs(const LLGLTFMaterial* render_material, const LLLocalGLTFMaterial *local_material)
{
    if (local_material)
    {
        // This is a local material, reload it from file
        // so that user won't end up with grey textures
        // on next login.
        LLMaterialEditor::loadMaterialFromFile(local_material->getFilename(), local_material->getIndexInFile());

        LLMaterialEditor* me = (LLMaterialEditor*)LLFloaterReg::getInstance("material_editor");
        if (me)
        {
            // don't use override material here, it has 'hacked ids'
            // and values, use end result, apply it on top of local.
            const LLColor4& base_color = render_material->mBaseColor;
            me->setBaseColor(LLColor3(base_color));
            me->setTransparency(base_color[VW]);
            me->setMetalnessFactor(render_material->mMetallicFactor);
            me->setRoughnessFactor(render_material->mRoughnessFactor);
            me->setEmissiveColor(render_material->mEmissiveColor);
            me->setDoubleSided(render_material->mDoubleSided);
            me->setAlphaMode(render_material->getAlphaMode());
            me->setAlphaCutoff(render_material->mAlphaCutoff);

            // most things like colors we can apply without verifying
            // but texture ids are going to be different from both, base and override
            // so only apply override id if there is actually a difference
            if (local_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] != render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR])
            {
                me->setBaseColorId(render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
                me->childSetValue("base_color_upload_fee", me->getString("no_upload_fee_string"));
            }
            if (local_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] != render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL])
            {
                me->setNormalId(render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL]);
                me->childSetValue("normal_upload_fee", me->getString("no_upload_fee_string"));
            }
            if (local_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] != render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS])
            {
                me->setMetallicRoughnessId(render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
                me->childSetValue("metallic_upload_fee", me->getString("no_upload_fee_string"));
            }
            if (local_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] != render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE])
            {
                me->setEmissiveId(render_material->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE]);
                me->childSetValue("emissive_upload_fee", me->getString("no_upload_fee_string"));
            }

            // recalculate upload prices
            me->markChangesUnsaved(0);
        }

        return;
    }

    LLSD payload;
    if (render_material)
    {
        // Make a copy of the render material with unsupported transforms removed
        LLGLTFMaterial asset_material = *render_material;
        asset_material.sanitizeAssetMaterial();
        // Serialize the sanitized render material
        payload["data"] = asset_material.asJSON();
    }
    else
    {
        // Menu shouldn't allow this, but as a fallback
        // pick defaults from a blank material
        LLGLTFMaterial blank_mat;
        payload["data"] = blank_mat.asJSON();
        LL_WARNS() << "Got no material when trying to save material" << LL_ENDL;
    }

    LLSD args;
    args["DESC"] = LLTrans::getString("New Material");

    LLNotificationsUtil::add("SaveMaterialAs", args, payload, boost::bind(&LLMaterialEditor::onSaveObjectsMaterialAsMsgCallback, _1, _2));
}

void LLMaterialEditor::onSaveObjectsMaterialAsMsgCallback(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (0 == option)
    {
        LLSD asset;
        asset["version"] = LLGLTFMaterial::ASSET_VERSION;
        asset["type"] = LLGLTFMaterial::ASSET_TYPE;
        // This is the string serialized from LLGLTFMaterial::asJSON
        asset["data"] = notification["payload"]["data"];

        std::ostringstream str;
        LLSDSerialize::serialize(asset, str, LLSDSerialize::LLSD_BINARY);

        std::string new_name = response["message"].asString();
        createInventoryItem(str.str(), new_name, std::string());
    }
}

void LLMaterialEditor::loadMaterial(const tinygltf::Model &model_in, const std::string &filename_lc, S32 index, bool open_floater)
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

    if (open_floater)
    {
        openFloater(getKey());
        setFocus(TRUE);
        setCanSave(true);
        setCanSaveAs(true);

        applyToSelection();
    }
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

class LLRenderMaterialOverrideFunctor : public LLSelectedNodeFunctor
{
public:
    LLRenderMaterialOverrideFunctor(
        LLMaterialEditor * me,
        const LLUUID &report_on_object_id,
        S32 report_on_te)
    : mEditor(me)
    , mSuccess(false)
    , mObjectId(report_on_object_id)
    , mObjectTE(report_on_te)
    {
    }

    virtual bool apply(LLSelectNode* nodep) override
    {
        LLViewerObject* objectp = nodep->getObject();
        if (!objectp || !objectp->permModify() || !objectp->getVolume())
        {
            return false;
        }
        S32 num_tes = llmin((S32)objectp->getNumTEs(), (S32)objectp->getNumFaces()); // avatars have TEs but no faces

        // post override from given object and te to the simulator
        // requestData should have:
        //  object_id - UUID of LLViewerObject
        //  side - S32 index of texture entry
        //  gltf_json - String of GLTF json for override data

        for (S32 te = 0; te < num_tes; ++te)
        {
            if (!nodep->isTESelected(te))
            {
                continue;
            }

            // Get material from object
            // Selection can cover multiple objects, and live editor is
            // supposed to overwrite changed values only
            LLTextureEntry* tep = objectp->getTE(te);

            if (tep->getGLTFMaterial() == nullptr)
            {
                // overrides are not supposed to work or apply if
                // there is no base material to work from
                continue;
            }

            LLPointer<LLGLTFMaterial> material = tep->getGLTFMaterialOverride();
            // make a copy to not invalidate existing
            // material for multiple objects
            if (material.isNull())
            {
                // Start with a material override which does not make any changes
                material = new LLGLTFMaterial();
            }
            else
            {
                material = new LLGLTFMaterial(*material);
            }

            U32 changed_flags = mEditor->getUnsavedChangesFlags();
            U32 reverted_flags = mEditor->getRevertedChangesFlags();

            LLPointer<LLGLTFMaterial> revert_mat;
            if (nodep->mSavedGLTFOverrideMaterials.size() > te)
            {
                if (nodep->mSavedGLTFOverrideMaterials[te].notNull())
                {
                    revert_mat = nodep->mSavedGLTFOverrideMaterials[te];
                }
                else
                {
                    // mSavedGLTFOverrideMaterials[te] being present but null
                    // means we need to use a default value 
                    revert_mat = new LLGLTFMaterial();
                }
            }
            // else can not revert at all

            // Override object's values with values from editor where appropriate
            if (changed_flags & MATERIAL_BASE_COLOR_DIRTY)
            {
                material->setBaseColorFactor(mEditor->getBaseColor(), true);
            }
            else if ((reverted_flags & MATERIAL_BASE_COLOR_DIRTY) && revert_mat.notNull())
            {
                material->setBaseColorFactor(revert_mat->mBaseColor, false);
            }

            if (changed_flags & MATERIAL_BASE_COLOR_TEX_DIRTY)
            {
                material->setBaseColorId(mEditor->getBaseColorId(), true);
            }
            else if ((reverted_flags & MATERIAL_BASE_COLOR_TEX_DIRTY) && revert_mat.notNull())
            {
                material->setBaseColorId(revert_mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR], false);
            }

            if (changed_flags & MATERIAL_NORMAL_TEX_DIRTY)
            {
                material->setNormalId(mEditor->getNormalId(), true);
            }
            else if ((reverted_flags & MATERIAL_NORMAL_TEX_DIRTY) && revert_mat.notNull())
            {
                material->setNormalId(revert_mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL], false);
            }

            if (changed_flags & MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY)
            {
                material->setOcclusionRoughnessMetallicId(mEditor->getMetallicRoughnessId(), true);
            }
            else if ((reverted_flags & MATERIAL_METALLIC_ROUGHTNESS_TEX_DIRTY) && revert_mat.notNull())
            {
                material->setOcclusionRoughnessMetallicId(revert_mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS], false);
            }

            if (changed_flags & MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY)
            {
                material->setMetallicFactor(mEditor->getMetalnessFactor(), true);
            }
            else if ((reverted_flags & MATERIAL_METALLIC_ROUGHTNESS_METALNESS_DIRTY) && revert_mat.notNull())
            {
                material->setMetallicFactor(revert_mat->mMetallicFactor, false);
            }

            if (changed_flags & MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY)
            {
                material->setRoughnessFactor(mEditor->getRoughnessFactor(), true);
            }
            else if ((reverted_flags & MATERIAL_METALLIC_ROUGHTNESS_ROUGHNESS_DIRTY) && revert_mat.notNull())
            {
                material->setRoughnessFactor(revert_mat->mRoughnessFactor, false);
            }

            if (changed_flags & MATERIAL_EMISIVE_COLOR_DIRTY)
            {
                material->setEmissiveColorFactor(LLColor3(mEditor->getEmissiveColor()), true);
            }
            else if ((reverted_flags & MATERIAL_EMISIVE_COLOR_DIRTY) && revert_mat.notNull())
            {
                material->setEmissiveColorFactor(revert_mat->mEmissiveColor, false);
            }

            if (changed_flags & MATERIAL_EMISIVE_TEX_DIRTY)
            {
                material->setEmissiveId(mEditor->getEmissiveId(), true);
            }
            else if ((reverted_flags & MATERIAL_EMISIVE_TEX_DIRTY) && revert_mat.notNull())
            {
                material->setEmissiveId(revert_mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE], false);
            }

            if (changed_flags & MATERIAL_DOUBLE_SIDED_DIRTY)
            {
                material->setDoubleSided(mEditor->getDoubleSided(), true);
            }
            else if ((reverted_flags & MATERIAL_DOUBLE_SIDED_DIRTY) && revert_mat.notNull())
            {
                material->setDoubleSided(revert_mat->mDoubleSided, false);
            }

            if (changed_flags & MATERIAL_ALPHA_MODE_DIRTY)
            {
                material->setAlphaMode(mEditor->getAlphaMode(), true);
            }
            else if ((reverted_flags & MATERIAL_ALPHA_MODE_DIRTY) && revert_mat.notNull())
            {
                material->setAlphaMode(revert_mat->mAlphaMode, false);
            }

            if (changed_flags & MATERIAL_ALPHA_CUTOFF_DIRTY)
            {
                material->setAlphaCutoff(mEditor->getAlphaCutoff(), true);
            }
            else if ((reverted_flags & MATERIAL_ALPHA_CUTOFF_DIRTY) && revert_mat.notNull())
            {
                material->setAlphaCutoff(revert_mat->mAlphaCutoff, false);
            }

            if (mObjectTE == te
                && mObjectId == objectp->getID())
            {
                mSuccess = true;
            }
            LLGLTFMaterialList::queueModify(objectp, te, material);
        }
        return true;
    }

    static void modifyCallback(bool success)
    {
        if (!success)
        {
            // something went wrong update selection
            LLMaterialEditor::updateLive();
        }
        // else we will get updateLive(obj, id) from applied overrides
    }

    bool getResult() { return mSuccess; }

private:
    LLMaterialEditor * mEditor;
    LLUUID mObjectId;
    S32 mObjectTE;
    bool mSuccess;
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
        // Don't send data if there is nothing to send.
        // Some UI elements will cause multiple commits,
        // like spin ctrls on click and on down
        if (mUnsavedChanges != 0 || mRevertedChanges != 0)
        {
            mOverrideInProgress = true;
            LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
            LLRenderMaterialOverrideFunctor override_func(this, mOverrideObjectId, mOverrideObjectTE);
            selected_objects->applyToNodes(&override_func);

            void(*done_callback)(bool) = LLRenderMaterialOverrideFunctor::modifyCallback;

            LLGLTFMaterialList::flushUpdates(done_callback);

            if (!override_func.getResult())
            {
                // OverrideFunctor didn't find expected object or face
                mOverrideInProgress = false;
            }

            // we posted all changes
            mUnsavedChanges = 0;
            mRevertedChanges = 0;
        }
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

// Get a dump of the json representation of the current state of the editor UI
// in GLTF format, excluding transforms as they are not supported in material
// assets. (See also LLGLTFMaterial::sanitizeAssetMaterial())
void LLMaterialEditor::getGLTFMaterial(LLGLTFMaterial* mat)
{
    mat->mBaseColor = getBaseColor();
    mat->mBaseColor.mV[3] = getTransparency();
    mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR] = getBaseColorId();

    mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL] = getNormalId();

    mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS] = getMetallicRoughnessId();
    mat->mMetallicFactor = getMetalnessFactor();
    mat->mRoughnessFactor = getRoughnessFactor();

    mat->mEmissiveColor = getEmissiveColor();
    mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE] = getEmissiveId();

    mat->mDoubleSided = getDoubleSided();
    mat->setAlphaMode(getAlphaMode());
    mat->mAlphaCutoff = getAlphaCutoff();
}

void LLMaterialEditor::setFromGLTFMaterial(LLGLTFMaterial* mat)
{
    setBaseColor(mat->mBaseColor);
    setBaseColorId(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR]);
    setNormalId(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_NORMAL]);

    setMetallicRoughnessId(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_METALLIC_ROUGHNESS]);
    setMetalnessFactor(mat->mMetallicFactor);
    setRoughnessFactor(mat->mRoughnessFactor);

    setEmissiveColor(mat->mEmissiveColor);
    setEmissiveId(mat->mTextureId[LLGLTFMaterial::GLTF_TEXTURE_INFO_EMISSIVE]);

    setDoubleSided(mat->mDoubleSided);
    setAlphaMode(mat->getAlphaMode());
    setAlphaCutoff(mat->mAlphaCutoff);
}

bool LLMaterialEditor::setFromSelection()
{
    LLObjectSelectionHandle selected_objects = LLSelectMgr::getInstance()->getSelection();
    LLSelectedTEGetMatData func(mIsOverride);

    selected_objects->applyToTEs(&func);
    mHasSelection = !selected_objects->isEmpty();
    mSelectionNeedsUpdate = false;

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

        // Memorize selection data for filtering further updates
        mOverrideObjectId = func.mObjectId;
        mOverrideObjectTE = func.mObjectTE;
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

                mAssetStatus = PREVIEW_ASSET_LOADING;

                // May callback immediately
                gAssetStorage->getAssetData(item->getAssetUUID(),
                    LLAssetType::AT_MATERIAL,
                    &onLoadComplete,
                    (void*)user_data,
                    TRUE);
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
            LL_WARNS("MaterialEditor") << "Asset id mismatch, expected: " << editor->mAssetID << " got: " << asset_uuid << LL_ENDL;
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
                // Not supposed to happen?
                LL_WARNS("MaterialEditor") << "No permission to view material " << asset_uuid << LL_ENDL;
                LLNotificationsUtil::add("MaterialNoPermissions");
            }
            else
            {
                LLNotificationsUtil::add("UnableToLoadMaterial");
            }
            editor->setEnableEditing(false);

            LL_WARNS("MaterialEditor") << "Problem loading material: " << status << LL_ENDL;
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

    LLSD key = getKey();
    std::function<bool(LLUUID itemId, LLSD response, std::string reason)> failed_upload([key](LLUUID assetId, LLSD response, std::string reason)
    {
        LLMaterialEditor* me = LLFloaterReg::findTypedInstance<LLMaterialEditor>("material_editor", key);
        if (me)
        {
            me->setFailedToUploadTexture();
        }
        return true; // handled
    });

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
        cb,
        failed_upload));

    upload_new_resource(uploadInfo);
}

void LLMaterialEditor::setFailedToUploadTexture()
{
    mUploadingTexturesFailure = true;
    mUploadingTexturesCount--;
    if (mUploadingTexturesCount == 0)
    {
        setEnabled(true);
    }
}

S32 LLMaterialEditor::saveTextures()
{
    mUploadingTexturesFailure = false; // not supposed to get here if already uploading

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

                    // discard upload buffers once texture have been saved
                    me->mBaseColorJ2C = nullptr;
                    me->mBaseColorFetched = nullptr;
                    me->mBaseColorTextureUploadId.setNull();

                    me->mUploadingTexturesCount--;

                    if (!me->mUploadingTexturesFailure)
                    {
                        // try saving
                        me->saveIfNeeded();
                    }
                    else if (me->mUploadingTexturesCount == 0)
                    {
                        me->setEnabled(true);
                    }
                }
                else
                {
                    // stop upload if possible, unblock and let user decide
                    me->setFailedToUploadTexture();
                }
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

                    // discard upload buffers once texture have been saved
                    me->mNormalJ2C = nullptr;
                    me->mNormalFetched = nullptr;
                    me->mNormalTextureUploadId.setNull();

                    me->mUploadingTexturesCount--;

                    if (!me->mUploadingTexturesFailure)
                    {
                        // try saving
                        me->saveIfNeeded();
                    }
                    else if (me->mUploadingTexturesCount == 0)
                    {
                        me->setEnabled(true);
                    }
                }
                else
                {
                    // stop upload if possible, unblock and let user decide
                    me->setFailedToUploadTexture();
                }
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

                    // discard upload buffers once texture have been saved
                    me->mMetallicRoughnessJ2C = nullptr;
                    me->mMetallicRoughnessFetched = nullptr;
                    me->mMetallicTextureUploadId.setNull();

                    me->mUploadingTexturesCount--;

                    if (!me->mUploadingTexturesFailure)
                    {
                        // try saving
                        me->saveIfNeeded();
                    }
                    else if (me->mUploadingTexturesCount == 0)
                    {
                        me->setEnabled(true);
                    }
                }
                else
                {
                    // stop upload if possible, unblock and let user decide
                    me->setFailedToUploadTexture();
                }
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

                    // discard upload buffers once texture have been saved
                    me->mEmissiveJ2C = nullptr;
                    me->mEmissiveFetched = nullptr;
                    me->mEmissiveTextureUploadId.setNull();

                    me->mUploadingTexturesCount--;

                    if (!me->mUploadingTexturesFailure)
                    {
                        // try saving
                        me->saveIfNeeded();
                    }
                    else if (me->mUploadingTexturesCount == 0)
                    {
                        me->setEnabled(true);
                    }
                }
                else
                {
                    // stop upload if possible, unblock and let user decide
                    me->setFailedToUploadTexture();
                }
            }
        });
    }

    if (!work_count)
    {
        // Discard upload buffers once textures have been confirmed as saved.
        // Otherwise we keep buffers for potential upload failure recovery.
        clearTextures();
    }

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

bool LLMaterialEditor::capabilitiesAvailable()
{
    const LLViewerRegion* region = gAgent.getRegion();
    if (!region)
    {
        LL_WARNS("MaterialEditor") << "Not connected to a region, cannot save material." << LL_ENDL;
        return false;
    }
    std::string agent_url = region->getCapability("UpdateMaterialAgentInventory");
    std::string task_url = region->getCapability("UpdateMaterialTaskInventory");

    return (!agent_url.empty() && !task_url.empty());
}

