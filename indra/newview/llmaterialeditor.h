/**
 * @file llmaterialeditor.h
 * @brief LLMaterialEditor class header file
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

#pragma once

#include "llpreview.h"
#include "llvoinventorylistener.h"
#include "llimagej2c.h"
#include "llviewertexture.h"

class LLButton;
class LLColorSwatchCtrl;
class LLComboBox;
class LLGLTFMaterial;
class LLLocalGLTFMaterial;
class LLTextureCtrl;
class LLTextBox;
class LLViewerInventoryItem;

namespace tinygltf
{
    class Model;
}

// todo: Consider making into a notification or just merging with
// presets. Layout is identical to camera/graphics presets so there
// is no point in having multiple separate xmls and classes.
class LLFloaterComboOptions : public LLFloater
{
public:
    typedef std::function<void(const std::string&, S32)> combo_callback;
    LLFloaterComboOptions();

    virtual ~LLFloaterComboOptions();
    /*virtual*/ bool    postBuild();

    static LLFloaterComboOptions* showUI(
        combo_callback callback,
        const std::string &title,
        const std::string &description,
        const std::list<std::string> &options);

    static LLFloaterComboOptions* showUI(
        combo_callback callback,
        const std::string &title,
        const std::string &description,
        const std::string &ok_text,
        const std::string &cancel_text,
        const std::list<std::string> &options);

private:
    void onConfirm();
    void onCancel();

protected:
    combo_callback mCallback;

    LLButton *mConfirmButton;
    LLButton *mCancelButton;
    LLComboBox *mComboOptions;
    LLTextBox *mComboText;
};

class LLMaterialEditor : public LLPreview, public LLVOInventoryListener
{ public:
    LLMaterialEditor(const LLSD& key);
    ~LLMaterialEditor();

    bool setFromGltfModel(const tinygltf::Model& model, S32 index, bool set_textures = false);

    void setFromGltfMetaData(const std::string& filename, const  tinygltf::Model& model, S32 index);

    // open a file dialog and select a gltf/glb file for import
    static void importMaterial(const LLUUID dest_folder = LLUUID::null);

    // for live preview, apply current material to currently selected object
    void applyToSelection();

    // get a dump of the json representation of the current state of the editor UI as a material object
    void getGLTFMaterial(LLGLTFMaterial* mat);

    void loadAsset() override;
    // @index if -1 and file contains more than one material,
    // will promt to select specific one
    static void uploadMaterialFromModel(const std::string& filename,
                                        tinygltf::Model& model,
                                        S32 index,
                                        const LLUUID& dest_folder_id = LLUUID::null);
    static void loadMaterialFromFile(const std::string& filename, S32 index = -1, const LLUUID& dest_folder = LLUUID::null);

    void onSelectionChanged(); // live overrides selection changes

    static void updateLive();
    static void loadLive();

    static bool canModifyObjectsMaterial();
    static bool canSaveObjectsMaterial();
    static bool canClipboardObjectsMaterial();
    static void saveObjectsMaterialAs();
    static void onSaveObjectsMaterialAsMsgCallback(const LLSD& notification, const LLSD& response, const LLPermissions& permissions);

    static void onLoadComplete(const LLUUID& asset_uuid, LLAssetType::EType type, void* user_data, S32 status, LLExtStat ext_status);

    void inventoryChanged(LLViewerObject* object, LLInventoryObject::object_list_t* inventory, S32 serial_num, void* user_data) override;

    typedef std::function<void(LLUUID newAssetId, LLSD response)> upload_callback_f;
    void saveTexture(LLImageJ2C* img, const std::string& name, const LLUUID& asset_id, upload_callback_f cb);
    void setFailedToUploadTexture();

    // save textures to inventory if needed
    // returns amount of scheduled uploads
    S32 saveTextures();
    void clearTextures();

    void onClickSave();

    std::string getEncodedAsset();

    bool decodeAsset(const std::vector<char>& buffer);

    bool saveIfNeeded();

    static void finishInventoryUpload(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId);

    static void finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId);

    static void finishSaveAs(
        const LLSD &oldKey,
        const LLUUID &newItemId,
        const std::string &buffer,
        bool has_unsaved_changes);

    void refreshFromInventory(const LLUUID& new_item_id = LLUUID::null);

    void onClickSaveAs();
    void onSaveAsMsgCallback(const LLSD& notification, const LLSD& response);
    void onClickCancel();
    void onCancelMsgCallback(const LLSD& notification, const LLSD& response);

    // llpreview
    void setObjectID(const LLUUID& object_id) override;
    void setAuxItem(const LLInventoryItem* item) override;

    // llpanel
    bool postBuild() override;
    void onClickCloseBtn(bool app_quitting = false) override;

    void onClose(bool app_quitting) override;
    void draw() override;
    void handleReshape(const LLRect& new_rect, bool by_user = false) override;

    LLUUID getBaseColorId();
    void setBaseColorId(const LLUUID& id);
    void setBaseColorUploadId(const LLUUID& id);

    LLColor4 getBaseColor();

    // sets both base color and transparency
    void    setBaseColor(const LLColor4& color);

    F32     getTransparency();
    void     setTransparency(F32 transparency);

    std::string getAlphaMode();
    void setAlphaMode(const std::string& alpha_mode);

    F32 getAlphaCutoff();
    void setAlphaCutoff(F32 alpha_cutoff);

    void setMaterialName(const std::string &name);

    LLUUID getMetallicRoughnessId();
    void setMetallicRoughnessId(const LLUUID& id);
    void setMetallicRoughnessUploadId(const LLUUID& id);

    F32 getMetalnessFactor();
    void setMetalnessFactor(F32 factor);

    F32 getRoughnessFactor();
    void setRoughnessFactor(F32 factor);

    LLUUID getEmissiveId();
    void setEmissiveId(const LLUUID& id);
    void setEmissiveUploadId(const LLUUID& id);

    LLColor4 getEmissiveColor();
    void setEmissiveColor(const LLColor4& color);

    LLUUID getNormalId();
    void setNormalId(const LLUUID& id);
    void setNormalUploadId(const LLUUID& id);

    bool getDoubleSided();
    void setDoubleSided(bool double_sided);

    void setCanSaveAs(bool value);
    void setCanSave(bool value);
    void setEnableEditing(bool can_modify);

    void subscribeToLocalTexture(S32 dirty_flag, const LLUUID& tracking_id);
    void replaceLocalTexture(const LLUUID& old_id, const LLUUID& new_id); // Local texture support
    void onCommitTexture(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag);
    void onCancelCtrl(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag);
    void onSelectCtrl(LLUICtrl* ctrl, const LLSD& data, S32 dirty_flag);

    // initialize the UI from a default GLTF material
    void loadDefaults();

    U32 getUnsavedChangesFlags() { return mUnsavedChanges; }
    U32 getRevertedChangesFlags() { return mRevertedChanges; }
    LLUUID getLocalTextureTrackingIdFromFlag(U32 flag);
    bool updateMaterialLocalSubscription(LLGLTFMaterial* mat);

    static bool capabilitiesAvailable();

private:
    static void saveObjectsMaterialAs(const LLGLTFMaterial *render_material, const LLLocalGLTFMaterial *local_material, const LLPermissions& permissions, const LLUUID& object_id /* = LLUUID::null */, const LLUUID& item /* = LLUUID::null */);

    static bool updateInventoryItem(const std::string &buffer, const LLUUID &item_id, const LLUUID &task_id);
    static void createInventoryItem(const std::string &buffer, const std::string &name, const std::string &desc, const LLPermissions& permissions, const LLUUID& upload_folder);

    void setFromGLTFMaterial(LLGLTFMaterial* mat);
    bool setFromSelection();

    void loadMaterial(const tinygltf::Model &model, const std::string & filename, S32 index, bool open_floater = true);

    friend class LLMaterialFilePicker;

    LLUUID mAssetID;
    LLUUID mUploadFolder;

    LLTextureCtrl* mBaseColorTextureCtrl;
    LLTextureCtrl* mMetallicTextureCtrl;
    LLTextureCtrl* mEmissiveTextureCtrl;
    LLTextureCtrl* mNormalTextureCtrl;
    LLColorSwatchCtrl* mBaseColorCtrl;
    LLColorSwatchCtrl* mEmissiveColorCtrl;

    // 'Default' texture, unless it's null or from inventory is the one with the fee
    LLUUID mBaseColorTextureUploadId;
    LLUUID mMetallicTextureUploadId;
    LLUUID mEmissiveTextureUploadId;
    LLUUID mNormalTextureUploadId;

    // last known name of each texture
    std::string mBaseColorName;
    std::string mNormalName;
    std::string mMetallicRoughnessName;
    std::string mEmissiveName;

    // keep pointers to fetched textures or viewer will remove them
    // if user temporary selects something else with 'apply now'
    LLPointer<LLViewerFetchedTexture> mBaseColorFetched;
    LLPointer<LLViewerFetchedTexture> mNormalFetched;
    LLPointer<LLViewerFetchedTexture> mMetallicRoughnessFetched;
    LLPointer<LLViewerFetchedTexture> mEmissiveFetched;

    // J2C versions of packed buffers for uploading
    LLPointer<LLImageJ2C> mBaseColorJ2C;
    LLPointer<LLImageJ2C> mNormalJ2C;
    LLPointer<LLImageJ2C> mMetallicRoughnessJ2C;
    LLPointer<LLImageJ2C> mEmissiveJ2C;

    // utility function for converting image uri into a texture name
    const std::string getImageNameFromUri(std::string image_uri, const std::string texture_type);

    // utility function for building a description of the imported material
    // based on what we know about it.
    const std::string buildMaterialDescription();
    void refreshUploadCost();

    void resetUnsavedChanges();
    void markChangesUnsaved(U32 dirty_flag);

    U32 mUnsavedChanges; // flags to indicate individual changed parameters
    U32 mRevertedChanges; // flags to indicate individual reverted parameters
    S32 mUploadingTexturesCount;
    S32 mExpectedUploadCost;
    bool mUploadingTexturesFailure;
    std::string mMaterialNameShort;
    std::string mMaterialName;

    // if true, this instance is live instance editing overrides
    bool mIsOverride = false;
    bool mHasSelection = false;
    // local id, texture ids per face for object overrides
    // for "cancel" support
    static LLUUID mOverrideObjectId; // static to avoid searching for the floater
    static S32 mOverrideObjectTE;
    static bool mOverrideInProgress;
    static bool mSelectionNeedsUpdate;
    boost::signals2::connection mSelectionUpdateSlot;

    struct LocalTextureConnection
    {
        LLUUID mTrackingId;
        boost::signals2::connection mConnection;
    };
    typedef std::map<S32, LocalTextureConnection> mat_connection_map_t;
    mat_connection_map_t mTextureChangesUpdates;
};

