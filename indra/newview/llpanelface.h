/**
 * @file llpanelface.h
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

#ifndef LL_LLPANELFACE_H
#define LL_LLPANELFACE_H

#include "v4color.h"
#include "llpanel.h"
#include "llgltfmaterial.h"
#include "llmaterial.h"
#include "llmaterialmgr.h"
#include "lltextureentry.h"
#include "llselectmgr.h"

#include <memory>

class LLButton;
class LLCheckBoxCtrl;
class LLColorSwatchCtrl;
class LLComboBox;
class LLInventoryItem;
class LLLineEditor;
class LLSpinCtrl;
class LLTextBox;
class LLTextureCtrl;
class LLUICtrl;
class LLViewerObject;
class LLFloater;
class LLMaterialID;
class LLMediaCtrl;
class LLMenuButton;
class LLRadioGroup;

class PBRPickerAgentListener;
class PBRPickerObjectListener;

// Represents an edit for use in replicating the op across one or more materials in the selection set.
//
// The apply function optionally performs the edit which it implements
// as a functor taking Data that calls member func MaterialFunc taking SetValueType
// on an instance of the LLMaterial class.
//
// boost who?
//
template<
    typename DataType,
    typename SetValueType,
    void (LLMaterial::*MaterialEditFunc)(SetValueType data) >
class LLMaterialEditFunctor
{
public:
    LLMaterialEditFunctor(const DataType& data) : _data(data) {}
    virtual ~LLMaterialEditFunctor() {}
    virtual void apply(LLMaterialPtr& material) { (material->*(MaterialEditFunc))(_data); }
    DataType _data;
};

template<
    typename DataType,
    DataType (LLMaterial::*MaterialGetFunc)() >
class LLMaterialGetFunctor
{
public:
    LLMaterialGetFunctor() {}
    virtual DataType get(LLMaterialPtr& material) { return (material->*(MaterialGetFunc)); }
};

template<
    typename DataType,
    DataType (LLTextureEntry::*TEGetFunc)() >
class LLTEGetFunctor
{
public:
    LLTEGetFunctor() {}
    virtual DataType get(LLTextureEntry* entry) { return (entry*(TEGetFunc)); }
};

class LLPanelFace : public LLPanel
{
public:
    virtual bool    postBuild();
    LLPanelFace();
    virtual ~LLPanelFace();

    void            refresh();
    void            refreshMedia();
    void            unloadMedia();

    static void onMaterialOverrideReceived(const LLUUID& object_id, S32 side);

    /*virtual*/ void onVisibilityChange(bool new_visibility);
    /*virtual*/ void draw();

    LLMaterialPtr createDefaultMaterial(LLMaterialPtr current_material)
    {
        LLMaterialPtr new_material(!current_material.isNull() ? new LLMaterial(current_material->asLLSD()) : new LLMaterial());
        llassert_always(new_material);

        // Preserve old diffuse alpha mode or assert correct default blend mode as appropriate for the alpha channel content of the diffuse texture
        //
        new_material->setDiffuseAlphaMode(current_material.isNull() ? (isAlpha() ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE) : current_material->getDiffuseAlphaMode());
        return new_material;
    }

    LLGLTFMaterial::TextureInfo getPBRTextureInfo();
    LLRender::eTexIndex getTextureChannelToEdit();
    LLRender::eTexIndex getMatTextureChannel();
    LLRender::eTexIndex getPBRTextureChannel();
    LLRender::eTexIndex getTextureDropChannel();
    LLGLTFMaterial::TextureInfo getPBRDropChannel();

protected:
    void navigateToTitleMedia(const std::string& url);
    bool selectedMediaEditable();
    void clearMediaSettings();
    void updateMediaSettings();
    void updateMediaTitle();

    void getState();

    void sendTexture();            // applies and sends texture
    void sendTextureInfo();        // applies and sends texture scale, offset, etc.
    void sendColor();              // applies and sends color
    void sendAlpha();              // applies and sends transparency
    void sendBump(U32 bumpiness);  // applies and sends bump map
    void sendTexGen();             // applies and sends bump map
    void sendShiny(U32 shininess); // applies and sends shininess
    void sendFullbright();         // applies and sends full bright

    void sendGlow();
    void alignTextureLayer();

    void updateCopyTexButton();

    void onCommitPbr();
    void onCancelPbr();
    void onSelectPbr();

    // These functions are to return true if the drag should succeed
    bool onDragPbr(LLInventoryItem* item);
    bool onDragTexture(LLInventoryItem* item);

    void onCommitTexture();
    void onCancelTexture();
    void onSelectTexture();
    void onCommitSpecularTexture(const LLSD& data);
    void onCancelSpecularTexture(const LLSD& data);
    void onSelectSpecularTexture(const LLSD& data);
    void onCommitNormalTexture(const LLSD& data);
    void onCancelNormalTexture(const LLSD& data);
    void onSelectNormalTexture(const LLSD& data);
    void onCommitColor();
    void onCommitShinyColor();
    void onCommitAlpha();
    void onCancelColor();
    void onCancelShinyColor();
    void onSelectColor();
    void onSelectShinyColor();

    void onCloseTexturePicker(const LLSD& data);

    static bool deleteMediaConfirm(const LLSD& notification, const LLSD& response);
    static bool multipleFacesSelectedConfirm(const LLSD& notification, const LLSD& response);

    // Make UI reflect state of currently selected material (refresh)
    // and UI mode (e.g. editing normal map v diffuse map)
    //
    // @param force_set_values forces spinners to set value even if they are focused
    void updateUI(bool force_set_values = false);

    // Convenience func to determine if all faces in selection have
    // identical planar texgen settings during edits
    //
    bool isIdenticalPlanarTexgen();

    // Callback funcs for individual controls
    //
    void onCommitTextureInfo();
    void onCommitTextureScaleX();
    void onCommitTextureScaleY();
    void onCommitTextureRot();
    void onCommitTextureOffsetX();
    void onCommitTextureOffsetY();

    void onCommitMaterialBumpyScaleX();
    void onCommitMaterialBumpyScaleY();
    void onCommitMaterialBumpyRot();
    void onCommitMaterialBumpyOffsetX();
    void onCommitMaterialBumpyOffsetY();

    void syncRepeatX(F32 scaleU);
    void syncRepeatY(F32 scaleV);
    void syncOffsetX(F32 offsetU);
    void syncOffsetY(F32 offsetV);
    void syncMaterialRot(F32 rot, int te = -1);

    void onCommitMaterialShinyScaleX();
    void onCommitMaterialShinyScaleY();
    void onCommitMaterialShinyRot();
    void onCommitMaterialShinyOffsetX();
    void onCommitMaterialShinyOffsetY();

    void onCommitMaterialGloss();
    void onCommitMaterialEnv();
    void onCommitMaterialMaskCutoff();

    void onCommitMaterialsMedia();
    void onCommitMaterialType();
    void onCommitPbrType();
    void onClickBtnEditMedia();
    void onClickBtnDeleteMedia();
    void onClickBtnAddMedia();
    void onCommitBump();
    void onCommitTexGen();
    void onCommitShiny();
    void onCommitAlphaMode();
    void onCommitFullbright();
    void onCommitGlow();
    void onCommitPlanarAlign();
    void onCommitRepeatsPerMeter();

    void onCommitGLTFTextureScaleU();
    void onCommitGLTFTextureScaleV();
    void onCommitGLTFRotation();
    void onCommitGLTFTextureOffsetU();
    void onCommitGLTFTextureOffsetV();

    void onClickAutoFix();
    void onAlignTexture();
    void onClickBtnLoadInvPBR();
    void onClickBtnEditPBR();
    void onClickBtnSavePBR();

public: // needs to be accessible to selection manager
    void onCopyColor(); // records all selected faces
    void onPasteColor(); // to specific face
    void onPasteColor(LLViewerObject* objectp, S32 te); // to specific face
    void onCopyTexture();
    void onPasteTexture();
    void onPasteTexture(LLViewerObject* objectp, S32 te);

protected:
    void menuDoToSelected(const LLSD& userdata);
    bool menuEnableItem(const LLSD& userdata);

    static F32 valueGlow(LLViewerObject* object, S32 face);

private:
    bool isAlpha() const { return mIsAlpha; }

    // Convenience funcs to keep the visual flack to a minimum
    //
    LLUUID  getCurrentNormalMap();
    LLUUID  getCurrentSpecularMap();
    U32     getCurrentShininess();
    U32     getCurrentBumpiness();
    U8      getCurrentDiffuseAlphaMode();
    U8      getCurrentAlphaMaskCutoff();
    U8      getCurrentEnvIntensity();
    U8      getCurrentGlossiness();
    F32     getCurrentBumpyRot();
    F32     getCurrentBumpyScaleU();
    F32     getCurrentBumpyScaleV();
    F32     getCurrentBumpyOffsetU();
    F32     getCurrentBumpyOffsetV();
    F32     getCurrentShinyRot();
    F32     getCurrentShinyScaleU();
    F32     getCurrentShinyScaleV();
    F32     getCurrentShinyOffsetU();
    F32     getCurrentShinyOffsetV();

    LLTextureCtrl* mPBRTextureCtrl { nullptr };
    LLTextureCtrl* mTextureCtrl { nullptr };
    LLTextureCtrl* mShinyTextureCtrl { nullptr };
    LLTextureCtrl* mBumpyTextureCtrl { nullptr };
    LLTextBox* mLabelColor { nullptr };
    LLColorSwatchCtrl* mColorSwatch { nullptr };
    LLTextBox* mLabelShiniColor { nullptr };
    LLColorSwatchCtrl* mShinyColorSwatch { nullptr };

    LLTextBox* mLabelTexGen { nullptr };
    LLComboBox* mComboTexGen { nullptr };

    LLRadioGroup* mRadioMaterialType { nullptr };
    LLRadioGroup* mRadioPbrType { nullptr };

    LLCheckBoxCtrl* mCheckFullbright { nullptr };

    LLTextBox* mLabelColorTransp { nullptr };
    LLSpinCtrl* mCtrlColorTransp { nullptr }; // transparency = 1 - alpha

    LLTextBox* mLabelGlow { nullptr };
    LLSpinCtrl* mCtrlGlow { nullptr };
    LLComboBox* mComboMatMedia { nullptr };
    LLMediaCtrl* mTitleMedia { nullptr };
    LLTextBox* mTitleMediaText { nullptr };

    LLTextBox* mLabelMatPermLoading { nullptr };
    LLCheckBoxCtrl* mCheckSyncSettings { nullptr };

    LLTextBox* mLabelBumpiness { nullptr };
    LLComboBox* mComboBumpiness { nullptr };
    LLTextBox* mLabelShininess { nullptr };
    LLComboBox* mComboShininess { nullptr };
    LLTextBox* mLabelAlphaMode { nullptr };
    LLComboBox* mComboAlphaMode { nullptr };
    LLSpinCtrl* mTexScaleU { nullptr };
    LLSpinCtrl* mTexScaleV { nullptr };
    LLSpinCtrl* mTexRotate { nullptr };
    LLSpinCtrl* mTexRepeat { nullptr };
    LLSpinCtrl* mTexOffsetU { nullptr };
    LLSpinCtrl* mTexOffsetV { nullptr };
    LLCheckBoxCtrl* mPlanarAlign{ nullptr };
    LLSpinCtrl* mBumpyScaleU { nullptr };
    LLSpinCtrl* mBumpyScaleV { nullptr };
    LLSpinCtrl* mBumpyRotate { nullptr };
    LLSpinCtrl* mBumpyOffsetU { nullptr };
    LLSpinCtrl* mBumpyOffsetV { nullptr };
    LLSpinCtrl* mShinyScaleU { nullptr };
    LLSpinCtrl* mShinyScaleV { nullptr };
    LLSpinCtrl* mShinyRotate { nullptr };
    LLSpinCtrl* mShinyOffsetU { nullptr };
    LLSpinCtrl* mShinyOffsetV { nullptr };
    LLTextBox* mLabelGlossiness { nullptr };
    LLSpinCtrl* mGlossiness { nullptr };
    LLTextBox* mLabelEnvironment { nullptr };
    LLSpinCtrl* mEnvironment { nullptr };
    LLTextBox* mLabelMaskCutoff { nullptr };
    LLSpinCtrl* mMaskCutoff { nullptr };
    LLButton* mAddMedia { nullptr };
    LLButton* mDelMedia { nullptr };
    LLSpinCtrl* mPBRScaleU { nullptr };
    LLSpinCtrl* mPBRScaleV { nullptr };
    LLSpinCtrl* mPBRRotate { nullptr };
    LLSpinCtrl* mPBROffsetU { nullptr };
    LLSpinCtrl* mPBROffsetV { nullptr };

    LLButton* mBtnAlign { nullptr };
    LLButton* mBtnAlignTex { nullptr };
    LLButton* mBtnPbrFromInv { nullptr };
    LLButton* mBtnEditBbr { nullptr };
    LLButton* mBtnSaveBbr { nullptr };

    // Update visibility of controls to match current UI mode
    // (e.g. materials vs media editing)
    //
    // Do NOT call updateUI from within this function.
    //
    void updateVisibility(LLViewerObject* objectp = nullptr);

    // Hey look everyone, a type-safe alternative to copy and paste! :)
    //

    template<class T>
    void getChildSetCommitCallback(T*& ctrl, std::string_view name, std::function<void(LLUICtrl*, const LLSD&)> cb)
    {
        ctrl = this->getChild<T>(name);
        ctrl->setCommitCallback(cb);
    }

    template<class T>
    void getChildSetClickedCallback(T*& ctrl, std::string_view name, std::function<void(LLUICtrl*, const LLSD&)> cb)
    {
        ctrl = this->getChild<T>(name);
        ctrl->setClickedCallback(cb);
    }

    // Update material parameters by applying 'edit_func' to selected TEs
    //
    template<
        typename DataType,
        typename SetValueType,
        void (LLMaterial::*MaterialEditFunc)(SetValueType data) >
    static void edit(LLPanelFace* p, DataType data, int te = -1, const LLUUID &only_for_object_id = LLUUID())
    {
        LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc > edit(data);
        struct LLSelectedTEEditMaterial : public LLSelectedTEMaterialFunctor
        {
            LLSelectedTEEditMaterial(LLPanelFace* panel, LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc >* editp, const LLUUID &only_for_object_id) : _panel(panel), _edit(editp), _only_for_object_id(only_for_object_id) {}
            virtual ~LLSelectedTEEditMaterial() {};
            virtual LLMaterialPtr apply(LLViewerObject* object, S32 face, LLTextureEntry* tep, LLMaterialPtr& current_material)
            {
                if (_edit && (_only_for_object_id.isNull() || _only_for_object_id == object->getID()))
                {
                    LLMaterialPtr new_material = _panel->createDefaultMaterial(current_material);
                    llassert_always(new_material);

                    // Determine correct alpha mode for current diffuse texture
                    // (i.e. does it have an alpha channel that makes alpha mode useful)
                    //
                    // _panel->isAlpha() "lies" when one face has alpha and the rest do not (NORSPEC-329)
                    // need to get per-face answer to this question for sane alpha mode retention on updates.
                    //
                    bool is_alpha_face = object->isImageAlphaBlended(face);

                    // need to keep this original answer for valid comparisons in logic below
                    //
                    U8 original_default_alpha_mode = is_alpha_face ? LLMaterial::DIFFUSE_ALPHA_MODE_BLEND : LLMaterial::DIFFUSE_ALPHA_MODE_NONE;

                    U8 default_alpha_mode = original_default_alpha_mode;

                    if (!current_material.isNull())
                    {
                        default_alpha_mode = current_material->getDiffuseAlphaMode();
                    }

                    // Insure we don't inherit the default of blend by accident...
                    // this will be stomped by a legit request to change the alpha mode by the apply() below
                    //
                    new_material->setDiffuseAlphaMode(default_alpha_mode);

                    // Do "It"!
                    //
                    _edit->apply(new_material);

                    U32     new_alpha_mode          = new_material->getDiffuseAlphaMode();
                    LLUUID  new_normal_map_id       = new_material->getNormalID();
                    LLUUID  new_spec_map_id         = new_material->getSpecularID();

                    if ((new_alpha_mode == LLMaterial::DIFFUSE_ALPHA_MODE_BLEND) && !is_alpha_face)
                    {
                        new_alpha_mode = LLMaterial::DIFFUSE_ALPHA_MODE_NONE;
                        new_material->setDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_NONE);
                    }

                    bool is_default_blend_mode      = (new_alpha_mode == original_default_alpha_mode);
                    bool is_need_material           = !is_default_blend_mode || !new_normal_map_id.isNull() || !new_spec_map_id.isNull();

                    if (!is_need_material)
                    {
                        LL_DEBUGS("Materials") << "Removing material from object " << object->getID() << " face " << face << LL_ENDL;
                        LLMaterialMgr::getInstance()->remove(object->getID(),face);
                        new_material = NULL;
                    }
                    else
                    {
                        LL_DEBUGS("Materials") << "Putting material on object " << object->getID() << " face " << face << ", material: " << new_material->asLLSD() << LL_ENDL;
                        LLMaterialMgr::getInstance()->put(object->getID(),face,*new_material);
                    }

                    object->setTEMaterialParams(face, new_material);
                    return new_material;
                }
                return NULL;
            }
            LLMaterialEditFunctor< DataType, SetValueType, MaterialEditFunc >*  _edit;
            LLPanelFace *_panel;
            const LLUUID & _only_for_object_id;
        } editor(p, &edit, only_for_object_id);
        LLSelectMgr::getInstance()->selectionSetMaterialParams(&editor, te);
    }

    template<
        typename DataType,
        typename ReturnType,
        ReturnType (LLMaterial::* const MaterialGetFunc)() const  >
    static void getTEMaterialValue(DataType& data_to_return, bool& identical,DataType default_value, bool has_tolerance = false, DataType tolerance = DataType())
    {
        DataType data_value{};
        struct GetTEMaterialVal : public LLSelectedTEGetFunctor<DataType>
        {
            GetTEMaterialVal(DataType default_value) : _default(default_value) {}
            virtual ~GetTEMaterialVal() {}

            DataType get(LLViewerObject* object, S32 face)
            {
                DataType ret = _default;
                LLMaterialPtr material_ptr;
                LLTextureEntry* tep = object ? object->getTE(face) : NULL;
                if (tep)
                {
                    material_ptr = tep->getMaterialParams();
                    if (!material_ptr.isNull())
                    {
                        ret = (material_ptr->*(MaterialGetFunc))();
                    }
                }
                return ret;
            }
            DataType _default;
        } GetFunc(default_value);
        identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &GetFunc, data_value, has_tolerance, tolerance);
        data_to_return = data_value;
    }

    template<
        typename DataType,
        typename ReturnType, // some kids just have to different...
        ReturnType (LLTextureEntry::* const TEGetFunc)() const >
    static void getTEValue(DataType& data_to_return, bool& identical, DataType default_value, bool has_tolerance = false, DataType tolerance = DataType())
    {
        DataType data_value {};
        struct GetTEVal : public LLSelectedTEGetFunctor<DataType>
        {
            GetTEVal(DataType default_value) : _default(default_value) {}
            virtual ~GetTEVal() {}

            DataType get(LLViewerObject* object, S32 face) {
                LLTextureEntry* tep = object ? object->getTE(face) : NULL;
                return tep ? ((tep->*(TEGetFunc))()) : _default;
            }
            DataType _default;
        } GetTEValFunc(default_value);
        identical = LLSelectMgr::getInstance()->getSelection()->getSelectedTEValue( &GetTEValFunc, data_value, has_tolerance, tolerance );
        data_to_return = data_value;
    }

    // Update vis and enabling of specific subsets of controls based on material params
    // (e.g. hide the spec controls if no spec texture is applied)
    //
    void updateShinyControls(bool is_setting_texture = false, bool mess_with_combobox = false);
    void updateBumpyControls(bool is_setting_texture = false, bool mess_with_combobox = false);
    void updateAlphaControls();

    /*
     * Checks whether the selected texture from the LLFloaterTexturePicker can be applied to the currently selected object.
     * If agent selects texture which is not allowed to be applied for the currently selected object,
     * all controls of the floater texture picker which allow to apply the texture will be disabled.
     */
    void onTextureSelectionChanged(LLInventoryItem* itemp);
    void onPbrSelectionChanged(LLInventoryItem* itemp);

    void updateUIGLTF(LLViewerObject* objectp, bool& has_pbr_material, bool& has_faces_without_pbr, bool force_set_values);
    void updateVisibilityGLTF(LLViewerObject* objectp = nullptr);

    void updateSelectedGLTFMaterials(std::function<void(LLGLTFMaterial*)> func);
    void updateGLTFTextureTransform(std::function<void(LLGLTFMaterial::TextureTransform*)> edit);

    void setMaterialOverridesFromSelection();

    LLMenuButton*   mMenuClipboardColor;
    LLMenuButton*   mMenuClipboardTexture;

    bool mIsAlpha;

    LLSD            mClipboardParams;

    LLSD mMediaSettings;
    bool mNeedMediaTitle;

    class Selection
    {
    public:
        void connect();

        // Returns true if the selected objects or sides have changed since
        // this was last called, and no object update is pending
        bool update();

        // Prevents update() returning true until the provided object is
        // updated. Necessary to prevent controls updating when the mouse is
        // held down.
        void setDirty() { mChanged = true; };

        // Callbacks
        void onSelectionChanged() { mNeedsSelectionCheck = true; }
        void onSelectedObjectUpdated(const LLUUID &object_id, S32 side);

    protected:
        bool compareSelection();

        bool mChanged = false;

        boost::signals2::scoped_connection mSelectConnection;
        bool mNeedsSelectionCheck = true;
        S32 mSelectedObjectCount = 0;
        S32 mSelectedTECount = 0;
        LLUUID mSelectedObjectID;
        S32 mLastSelectedSide = -1;
    };

    static Selection sMaterialOverrideSelection;

    std::unique_ptr<PBRPickerAgentListener> mAgentInventoryListener;
    std::unique_ptr<PBRPickerObjectListener> mVOInventoryListener;

public:
    #if defined(DEF_GET_MAT_STATE)
        #undef DEF_GET_MAT_STATE
    #endif

    #if defined(DEF_GET_TE_STATE)
        #undef DEF_GET_TE_STATE
    #endif

    #if defined(DEF_EDIT_MAT_STATE)
        DEF_EDIT_MAT_STATE
    #endif

    // Accessors for selected TE material state
    //
    #define DEF_GET_MAT_STATE(DataType,ReturnType,MaterialMemberFunc,DefaultValue,HasTolerance,Tolerance)                                           \
        static void MaterialMemberFunc(DataType& data, bool& identical, bool has_tolerance = HasTolerance, DataType tolerance = Tolerance)          \
        {                                                                                                                                           \
            getTEMaterialValue< DataType, ReturnType, &LLMaterial::MaterialMemberFunc >(data, identical, DefaultValue, has_tolerance, tolerance);   \
        }

    // Mutators for selected TE material
    //
    #define DEF_EDIT_MAT_STATE(DataType,ReturnType,MaterialMemberFunc)                                                              \
        static void MaterialMemberFunc(LLPanelFace* p, DataType data, int te = -1, const LLUUID &only_for_object_id = LLUUID())     \
        {                                                                                                                           \
            edit< DataType, ReturnType, &LLMaterial::MaterialMemberFunc >(p, data, te, only_for_object_id);                         \
        }

    // Accessors for selected TE state proper (legacy settings etc)
    //
    #define DEF_GET_TE_STATE(DataType,ReturnType,TexEntryMemberFunc,DefaultValue,HasTolerance,Tolerance)                                        \
        static void TexEntryMemberFunc(DataType& data, bool& identical, bool has_tolerance = HasTolerance, DataType tolerance = Tolerance)      \
        {                                                                                                                                       \
            getTEValue< DataType, ReturnType, &LLTextureEntry::TexEntryMemberFunc >(data, identical, DefaultValue, has_tolerance, tolerance);   \
        }

    class LLSelectedTEMaterial
    {
    public:
        static void getCurrent(LLMaterialPtr& material_ptr, bool& identical_material);
        static void getMaxSpecularRepeats(F32& repeats, bool& identical);
        static void getMaxNormalRepeats(F32& repeats, bool& identical);
        static void getCurrentDiffuseAlphaMode(U8& diffuse_alpha_mode, bool& identical, bool diffuse_texture_has_alpha);

        DEF_GET_MAT_STATE(LLUUID, const LLUUID&, getNormalID, LLUUID::null, false, LLUUID::null);
        DEF_GET_MAT_STATE(LLUUID, const LLUUID&, getSpecularID, LLUUID::null, false, LLUUID::null);
        DEF_GET_MAT_STATE(F32, F32, getSpecularRepeatX, 1.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getSpecularRepeatY, 1.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getSpecularOffsetX, 0.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getSpecularOffsetY, 0.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getSpecularRotation, 0.0f, true, 0.001f);

        DEF_GET_MAT_STATE(F32, F32, getNormalRepeatX, 1.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getNormalRepeatY, 1.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getNormalOffsetX, 0.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getNormalOffsetY, 0.0f, true, 0.001f);
        DEF_GET_MAT_STATE(F32, F32, getNormalRotation, 0.0f, true, 0.001f);

        DEF_EDIT_MAT_STATE(U8, U8, setDiffuseAlphaMode);
        DEF_EDIT_MAT_STATE(U8, U8, setAlphaMaskCutoff);

        DEF_EDIT_MAT_STATE(F32, F32, setNormalOffsetX);
        DEF_EDIT_MAT_STATE(F32, F32, setNormalOffsetY);
        DEF_EDIT_MAT_STATE(F32, F32, setNormalRepeatX);
        DEF_EDIT_MAT_STATE(F32, F32, setNormalRepeatY);
        DEF_EDIT_MAT_STATE(F32, F32, setNormalRotation);

        DEF_EDIT_MAT_STATE(F32, F32, setSpecularOffsetX);
        DEF_EDIT_MAT_STATE(F32, F32, setSpecularOffsetY);
        DEF_EDIT_MAT_STATE(F32, F32, setSpecularRepeatX);
        DEF_EDIT_MAT_STATE(F32, F32, setSpecularRepeatY);
        DEF_EDIT_MAT_STATE(F32, F32, setSpecularRotation);

        DEF_EDIT_MAT_STATE(U8, U8, setEnvironmentIntensity);
        DEF_EDIT_MAT_STATE(U8, U8, setSpecularLightExponent);

        DEF_EDIT_MAT_STATE(LLUUID, const LLUUID&,setNormalID);
        DEF_EDIT_MAT_STATE(LLUUID, const LLUUID&,setSpecularID);
        DEF_EDIT_MAT_STATE(LLColor4U, const LLColor4U&, setSpecularLightColor);
    };

    class LLSelectedTE
    {
    public:
        static void getFace(class LLFace*& face_to_return, bool& identical_face);
        static void getImageFormat(LLGLenum& image_format_to_return, bool& identical_face, bool& missing_asset);
        static void getTexId(LLUUID& id, bool& identical);
        static void getPbrMaterialId(LLUUID& id, bool& identical, bool& has_pbr, bool& has_faces_without_pbr);
        static void getObjectScaleS(F32& scale_s, bool& identical);
        static void getObjectScaleT(F32& scale_t, bool& identical);
        static void getMaxDiffuseRepeats(F32& repeats, bool& identical);

        DEF_GET_TE_STATE(U8,U8,getBumpmap,0, false, 0)
        DEF_GET_TE_STATE(U8,U8,getShiny,0, false, 0)
        DEF_GET_TE_STATE(U8,U8,getFullbright,0, false, 0)
        DEF_GET_TE_STATE(F32,F32,getRotation,0.0f, true, 0.001f)
        DEF_GET_TE_STATE(F32,F32,getOffsetS,0.0f, true, 0.001f)
        DEF_GET_TE_STATE(F32,F32,getOffsetT,0.0f, true, 0.001f)
        DEF_GET_TE_STATE(F32,F32,getScaleS,1.0f, true, 0.001f)
        DEF_GET_TE_STATE(F32,F32,getScaleT,1.0f, true, 0.001f)
        DEF_GET_TE_STATE(F32,F32,getGlow,0.0f, true, 0.001f)
        DEF_GET_TE_STATE(LLTextureEntry::e_texgen,LLTextureEntry::e_texgen,getTexGen,LLTextureEntry::TEX_GEN_DEFAULT, false, LLTextureEntry::TEX_GEN_DEFAULT)
        DEF_GET_TE_STATE(LLColor4,const LLColor4&,getColor,LLColor4::white, false, LLColor4::black);
    };

    friend struct LLPanelFaceSetTEFunctor;
};

#endif
