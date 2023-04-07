/** 
 * @file llpaneleditwearable.cpp
 * @brief UI panel for editing of a particular wearable item.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpaneleditwearable.h"
#include "llpanel.h"
#include "llviewerwearable.h"
#include "lluictrl.h"
#include "lllocaltextureobject.h"
#include "llscrollingpanellist.h"
#include "llvisualparam.h"
#include "lltoolmorph.h"
#include "llviewerjointmesh.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llvoavatarself.h"
#include "lltexteditor.h"
#include "lltextbox.h"
#include "llaccordionctrl.h"
#include "llaccordionctrltab.h"
#include "llagentwearables.h"
#include "llscrollingpanelparam.h"
#include "llradiogroup.h"
#include "llnotificationsutil.h"

#include "llcolorswatch.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "llviewercontrol.h"    // gSavedSettings
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llagentcamera.h"
#include "llmorphview.h"

#include "llcommandhandler.h"
#include "lltextutil.h"
#include "llappearancemgr.h"

// register panel with appropriate XML
static LLPanelInjector<LLPanelEditWearable> t_edit_wearable("panel_edit_wearable");

// subparts of the UI for focus, camera position, etc.
enum ESubpart {
        SUBPART_SHAPE_HEAD = 1, // avoid 0
        SUBPART_SHAPE_EYES,
        SUBPART_SHAPE_EARS,
        SUBPART_SHAPE_NOSE,
        SUBPART_SHAPE_MOUTH,
        SUBPART_SHAPE_CHIN,
        SUBPART_SHAPE_TORSO,
        SUBPART_SHAPE_LEGS,
        SUBPART_SHAPE_WHOLE,
        SUBPART_SHAPE_DETAIL,
        SUBPART_SKIN_COLOR,
        SUBPART_SKIN_FACEDETAIL,
        SUBPART_SKIN_MAKEUP,
        SUBPART_SKIN_BODYDETAIL,
        SUBPART_HAIR_COLOR,
        SUBPART_HAIR_STYLE,
        SUBPART_HAIR_EYEBROWS,
        SUBPART_HAIR_FACIAL,
        SUBPART_EYES,
        SUBPART_SHIRT,
        SUBPART_PANTS,
        SUBPART_SHOES,
        SUBPART_SOCKS,
        SUBPART_JACKET,
        SUBPART_GLOVES,
        SUBPART_UNDERSHIRT,
        SUBPART_UNDERPANTS,
        SUBPART_SKIRT,
        SUBPART_ALPHA,
        SUBPART_TATTOO,
		SUBPART_UNIVERSAL,
        SUBPART_PHYSICS_BREASTS_UPDOWN,
        SUBPART_PHYSICS_BREASTS_INOUT,
        SUBPART_PHYSICS_BREASTS_LEFTRIGHT,
        SUBPART_PHYSICS_BELLY_UPDOWN,
        SUBPART_PHYSICS_BUTT_UPDOWN,
        SUBPART_PHYSICS_BUTT_LEFTRIGHT,
        SUBPART_PHYSICS_ADVANCED,
 };

using namespace LLAvatarAppearanceDefines;

typedef std::vector<ESubpart> subpart_vec_t;

// Locally defined classes

class LLEditWearableDictionary : public LLSingleton<LLEditWearableDictionary>
{
        //--------------------------------------------------------------------
        // Constructors and Destructors
        //--------------------------------------------------------------------
        LLSINGLETON(LLEditWearableDictionary);
        virtual ~LLEditWearableDictionary();
        
        //--------------------------------------------------------------------
        // Wearable Types
        //--------------------------------------------------------------------
public:
        struct WearableEntry : public LLDictionaryEntry
        {
                WearableEntry(LLWearableType::EType type,
                                          const std::string &title,
                                          const std::string &desc_title,
                                          const texture_vec_t& color_swatches,  // 'color_swatches'
                                          const texture_vec_t& texture_pickers, // 'texture_pickers'
                                          const subpart_vec_t& subparts); // subparts


                const LLWearableType::EType mWearableType;
                const std::string   mTitle;
                const std::string       mDescTitle;
                subpart_vec_t           mSubparts;
                texture_vec_t           mColorSwatchCtrls;
                texture_vec_t           mTextureCtrls;
        };

        struct Wearables : public LLDictionary<LLWearableType::EType, WearableEntry>
        {
                Wearables();
        } mWearables;

        const WearableEntry*    getWearable(LLWearableType::EType type) const { return mWearables.lookup(type); }

        //--------------------------------------------------------------------
        // Subparts
        //--------------------------------------------------------------------
public:
        struct SubpartEntry : public LLDictionaryEntry
        {
                SubpartEntry(ESubpart part,
                                         const std::string &joint,
                                         const std::string &edit_group,
                                         const std::string &param_list,
                                         const std::string &accordion_tab,
                                         const LLVector3d  &target_offset,
                                         const LLVector3d  &camera_offset,
                                         const ESex        &sex);

                ESubpart                        mSubpart;
                std::string                     mTargetJoint;
                std::string                     mEditGroup;
                std::string                     mParamList;
                std::string                     mAccordionTab;
                LLVector3d                      mTargetOffset;
                LLVector3d                      mCameraOffset;
                ESex                            mSex;
        };

        struct Subparts : public LLDictionary<ESubpart, SubpartEntry>
        {
                Subparts();
        } mSubparts;

        const SubpartEntry*  getSubpart(ESubpart subpart) const { return mSubparts.lookup(subpart); }

        //--------------------------------------------------------------------
        // Picker Control Entries
        //--------------------------------------------------------------------
public:
        struct PickerControlEntry : public LLDictionaryEntry
        {
                PickerControlEntry(ETextureIndex tex_index,
                                                   const std::string name,
                                                   const LLUUID default_image_id = LLUUID::null,
                                                   const bool allow_no_texture = false);
                ETextureIndex           mTextureIndex;
                const std::string       mControlName;
                const LLUUID            mDefaultImageId;
                const bool                      mAllowNoTexture;
        };

        struct ColorSwatchCtrls : public LLDictionary<ETextureIndex, PickerControlEntry>
        {
                ColorSwatchCtrls();
        } mColorSwatchCtrls;

        struct TextureCtrls : public LLDictionary<ETextureIndex, PickerControlEntry>
        {
                TextureCtrls();
        } mTextureCtrls;

        const PickerControlEntry* getTexturePicker(ETextureIndex index) const { return mTextureCtrls.lookup(index); }
        const PickerControlEntry* getColorSwatch(ETextureIndex index) const { return mColorSwatchCtrls.lookup(index); }
};

LLEditWearableDictionary::LLEditWearableDictionary()
{

}

//virtual 
LLEditWearableDictionary::~LLEditWearableDictionary()
{
}

LLEditWearableDictionary::Wearables::Wearables()
{
        // note the subpart that is listed first is treated as "default", regardless of what order is in enum.
        // Please match the order presented in XUI. -Nyx
        // this will affect what camera angle is shown when first editing a wearable
        addEntry(LLWearableType::WT_SHAPE,              new WearableEntry(LLWearableType::WT_SHAPE,"edit_shape_title","shape_desc_text", texture_vec_t(), texture_vec_t(), subpart_vec_t{SUBPART_SHAPE_WHOLE, SUBPART_SHAPE_HEAD,        SUBPART_SHAPE_EYES,     SUBPART_SHAPE_EARS,     SUBPART_SHAPE_NOSE,     SUBPART_SHAPE_MOUTH, SUBPART_SHAPE_CHIN, SUBPART_SHAPE_TORSO, SUBPART_SHAPE_LEGS}));
        addEntry(LLWearableType::WT_SKIN,               new WearableEntry(LLWearableType::WT_SKIN,"edit_skin_title","skin_desc_text", texture_vec_t(), texture_vec_t{TEX_HEAD_BODYPAINT, TEX_UPPER_BODYPAINT, TEX_LOWER_BODYPAINT}, subpart_vec_t{SUBPART_SKIN_COLOR, SUBPART_SKIN_FACEDETAIL, SUBPART_SKIN_MAKEUP, SUBPART_SKIN_BODYDETAIL}));
        addEntry(LLWearableType::WT_HAIR,               new WearableEntry(LLWearableType::WT_HAIR,"edit_hair_title","hair_desc_text", texture_vec_t(), texture_vec_t{TEX_HAIR}, subpart_vec_t{SUBPART_HAIR_COLOR,       SUBPART_HAIR_STYLE,     SUBPART_HAIR_EYEBROWS, SUBPART_HAIR_FACIAL}));
        addEntry(LLWearableType::WT_EYES,               new WearableEntry(LLWearableType::WT_EYES,"edit_eyes_title","eyes_desc_text", texture_vec_t(), texture_vec_t{TEX_EYES_IRIS}, subpart_vec_t{SUBPART_EYES}));
        addEntry(LLWearableType::WT_SHIRT,              new WearableEntry(LLWearableType::WT_SHIRT,"edit_shirt_title","shirt_desc_text", texture_vec_t{TEX_UPPER_SHIRT}, texture_vec_t{TEX_UPPER_SHIRT}, subpart_vec_t{SUBPART_SHIRT}));
        addEntry(LLWearableType::WT_PANTS,              new WearableEntry(LLWearableType::WT_PANTS,"edit_pants_title","pants_desc_text", texture_vec_t{TEX_LOWER_PANTS}, texture_vec_t{TEX_LOWER_PANTS}, subpart_vec_t{SUBPART_PANTS}));
        addEntry(LLWearableType::WT_SHOES,              new WearableEntry(LLWearableType::WT_SHOES,"edit_shoes_title","shoes_desc_text", texture_vec_t{TEX_LOWER_SHOES}, texture_vec_t{TEX_LOWER_SHOES}, subpart_vec_t{SUBPART_SHOES}));
        addEntry(LLWearableType::WT_SOCKS,              new WearableEntry(LLWearableType::WT_SOCKS,"edit_socks_title","socks_desc_text", texture_vec_t{TEX_LOWER_SOCKS}, texture_vec_t{TEX_LOWER_SOCKS}, subpart_vec_t{SUBPART_SOCKS}));
        addEntry(LLWearableType::WT_JACKET,     new WearableEntry(LLWearableType::WT_JACKET,"edit_jacket_title","jacket_desc_text", texture_vec_t{TEX_UPPER_JACKET}, texture_vec_t{TEX_UPPER_JACKET, TEX_LOWER_JACKET}, subpart_vec_t{SUBPART_JACKET}));
        addEntry(LLWearableType::WT_GLOVES,     new WearableEntry(LLWearableType::WT_GLOVES,"edit_gloves_title","gloves_desc_text", texture_vec_t{TEX_UPPER_GLOVES}, texture_vec_t{TEX_UPPER_GLOVES}, subpart_vec_t{SUBPART_GLOVES}));
        addEntry(LLWearableType::WT_UNDERSHIRT, new WearableEntry(LLWearableType::WT_UNDERSHIRT,"edit_undershirt_title","undershirt_desc_text", texture_vec_t{TEX_UPPER_UNDERSHIRT}, texture_vec_t{TEX_UPPER_UNDERSHIRT}, subpart_vec_t{SUBPART_UNDERSHIRT}));
        addEntry(LLWearableType::WT_UNDERPANTS, new WearableEntry(LLWearableType::WT_UNDERPANTS,"edit_underpants_title","underpants_desc_text", texture_vec_t{TEX_LOWER_UNDERPANTS}, texture_vec_t{TEX_LOWER_UNDERPANTS}, subpart_vec_t{SUBPART_UNDERPANTS}));
        addEntry(LLWearableType::WT_SKIRT,              new WearableEntry(LLWearableType::WT_SKIRT,"edit_skirt_title","skirt_desc_text", texture_vec_t{TEX_SKIRT}, texture_vec_t{TEX_SKIRT}, subpart_vec_t{SUBPART_SKIRT}));
        addEntry(LLWearableType::WT_ALPHA,              new WearableEntry(LLWearableType::WT_ALPHA,"edit_alpha_title","alpha_desc_text", texture_vec_t(), texture_vec_t{TEX_LOWER_ALPHA, TEX_UPPER_ALPHA, TEX_HEAD_ALPHA, TEX_EYES_ALPHA, TEX_HAIR_ALPHA}, subpart_vec_t{SUBPART_ALPHA}));
        addEntry(LLWearableType::WT_TATTOO,     new WearableEntry(LLWearableType::WT_TATTOO,"edit_tattoo_title","tattoo_desc_text", texture_vec_t{TEX_HEAD_TATTOO}, texture_vec_t{TEX_LOWER_TATTOO, TEX_UPPER_TATTOO, TEX_HEAD_TATTOO}, subpart_vec_t{SUBPART_TATTOO}));
		addEntry(LLWearableType::WT_UNIVERSAL, new WearableEntry(LLWearableType::WT_UNIVERSAL, "edit_universal_title", "universal_desc_text", texture_vec_t{ TEX_HEAD_UNIVERSAL_TATTOO }, texture_vec_t{ TEX_HEAD_UNIVERSAL_TATTOO, TEX_UPPER_UNIVERSAL_TATTOO, TEX_LOWER_UNIVERSAL_TATTOO, TEX_SKIRT_TATTOO, TEX_HAIR_TATTOO, TEX_EYES_TATTOO, TEX_LEFT_ARM_TATTOO, TEX_LEFT_LEG_TATTOO, TEX_AUX1_TATTOO, TEX_AUX2_TATTOO, TEX_AUX3_TATTOO }, subpart_vec_t{ SUBPART_UNIVERSAL }));
		addEntry(LLWearableType::WT_PHYSICS,    new WearableEntry(LLWearableType::WT_PHYSICS,"edit_physics_title","physics_desc_text", texture_vec_t(), texture_vec_t(), subpart_vec_t{SUBPART_PHYSICS_BREASTS_UPDOWN, SUBPART_PHYSICS_BREASTS_INOUT, SUBPART_PHYSICS_BREASTS_LEFTRIGHT, SUBPART_PHYSICS_BELLY_UPDOWN, SUBPART_PHYSICS_BUTT_UPDOWN, SUBPART_PHYSICS_BUTT_LEFTRIGHT, SUBPART_PHYSICS_ADVANCED}));
}

LLEditWearableDictionary::WearableEntry::WearableEntry(LLWearableType::EType type,
                                          const std::string &title,
                                          const std::string &desc_title,
                                          const texture_vec_t& color_swatches,
                                          const texture_vec_t& texture_pickers,
                                          const subpart_vec_t& subparts) :
        LLDictionaryEntry(title),
        mWearableType(type),
        mTitle(title),
        mDescTitle(desc_title),
        mSubparts(subparts),
        mColorSwatchCtrls(color_swatches),
        mTextureCtrls(texture_pickers)
{}

LLEditWearableDictionary::Subparts::Subparts()
{
        addEntry(SUBPART_SHAPE_WHOLE, new SubpartEntry(SUBPART_SHAPE_WHOLE, "mPelvis", "shape_body","shape_body_param_list", "shape_body_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_HEAD, new SubpartEntry(SUBPART_SHAPE_HEAD, "mHead", "shape_head", "shape_head_param_list", "shape_head_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_EYES, new SubpartEntry(SUBPART_SHAPE_EYES, "mHead", "shape_eyes", "shape_eyes_param_list", "shape_eyes_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_EARS, new SubpartEntry(SUBPART_SHAPE_EARS, "mHead", "shape_ears", "shape_ears_param_list", "shape_ears_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_NOSE, new SubpartEntry(SUBPART_SHAPE_NOSE, "mHead", "shape_nose", "shape_nose_param_list", "shape_nose_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_MOUTH, new SubpartEntry(SUBPART_SHAPE_MOUTH, "mHead", "shape_mouth", "shape_mouth_param_list", "shape_mouth_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_CHIN, new SubpartEntry(SUBPART_SHAPE_CHIN, "mHead", "shape_chin", "shape_chin_param_list", "shape_chin_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_TORSO, new SubpartEntry(SUBPART_SHAPE_TORSO, "mTorso", "shape_torso", "shape_torso_param_list", "shape_torso_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
        addEntry(SUBPART_SHAPE_LEGS, new SubpartEntry(SUBPART_SHAPE_LEGS, "mPelvis", "shape_legs", "shape_legs_param_list", "shape_legs_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));

        addEntry(SUBPART_SKIN_COLOR, new SubpartEntry(SUBPART_SKIN_COLOR, "mHead", "skin_color", "skin_color_param_list", "skin_color_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SKIN_FACEDETAIL, new SubpartEntry(SUBPART_SKIN_FACEDETAIL, "mHead", "skin_facedetail", "skin_face_param_list", "skin_face_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SKIN_MAKEUP, new SubpartEntry(SUBPART_SKIN_MAKEUP, "mHead", "skin_makeup", "skin_makeup_param_list", "skin_makeup_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_SKIN_BODYDETAIL, new SubpartEntry(SUBPART_SKIN_BODYDETAIL, "mPelvis", "skin_bodydetail", "skin_body_param_list", "skin_body_tab", LLVector3d(0.f, 0.f, -0.2f), LLVector3d(-2.5f, 0.5f, 0.5f),SEX_BOTH));

        addEntry(SUBPART_HAIR_COLOR, new SubpartEntry(SUBPART_HAIR_COLOR, "mHead", "hair_color", "hair_color_param_list", "hair_color_tab", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
        addEntry(SUBPART_HAIR_STYLE, new SubpartEntry(SUBPART_HAIR_STYLE, "mHead", "hair_style", "hair_style_param_list", "hair_style_tab", LLVector3d(0.f, 0.f, 0.10f), LLVector3d(-0.4f, 0.05f, 0.10f),SEX_BOTH));
        addEntry(SUBPART_HAIR_EYEBROWS, new SubpartEntry(SUBPART_HAIR_EYEBROWS, "mHead", "hair_eyebrows", "hair_eyebrows_param_list", "hair_eyebrows_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));
        addEntry(SUBPART_HAIR_FACIAL, new SubpartEntry(SUBPART_HAIR_FACIAL, "mHead", "hair_facial", "hair_facial_param_list", "hair_facial_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_MALE));

        addEntry(SUBPART_EYES, new SubpartEntry(SUBPART_EYES, "mHead", "eyes", "eyes_main_param_list", "eyes_main_tab", LLVector3d(0.f, 0.f, 0.05f), LLVector3d(-0.5f, 0.05f, 0.07f),SEX_BOTH));

        addEntry(SUBPART_SHIRT, new SubpartEntry(SUBPART_SHIRT, "mTorso", "shirt", "shirt_main_param_list", "shirt_main_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
        addEntry(SUBPART_PANTS, new SubpartEntry(SUBPART_PANTS, "mPelvis", "pants", "pants_main_param_list", "pants_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
        addEntry(SUBPART_SHOES, new SubpartEntry(SUBPART_SHOES, "mPelvis", "shoes", "shoes_main_param_list", "shoes_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
        addEntry(SUBPART_SOCKS, new SubpartEntry(SUBPART_SOCKS, "mPelvis", "socks", "socks_main_param_list", "socks_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
        addEntry(SUBPART_JACKET, new SubpartEntry(SUBPART_JACKET, "mTorso", "jacket", "jacket_main_param_list", "jacket_main_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(-2.f, 0.1f, 0.3f),SEX_BOTH));
        addEntry(SUBPART_SKIRT, new SubpartEntry(SUBPART_SKIRT, "mPelvis", "skirt", "skirt_main_param_list", "skirt_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
        addEntry(SUBPART_GLOVES, new SubpartEntry(SUBPART_GLOVES, "mTorso", "gloves", "gloves_main_param_list", "gloves_main_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(-1.f, 0.15f, 0.f),SEX_BOTH));
        addEntry(SUBPART_UNDERSHIRT, new SubpartEntry(SUBPART_UNDERSHIRT, "mTorso", "undershirt", "undershirt_main_param_list", "undershirt_main_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(-1.f, 0.15f, 0.3f),SEX_BOTH));
        addEntry(SUBPART_UNDERPANTS, new SubpartEntry(SUBPART_UNDERPANTS, "mPelvis", "underpants", "underpants_main_param_list", "underpants_main_tab", LLVector3d(0.f, 0.f, -0.5f), LLVector3d(-1.6f, 0.15f, -0.5f),SEX_BOTH));
        addEntry(SUBPART_ALPHA, new SubpartEntry(SUBPART_ALPHA, "mPelvis", "alpha", "alpha_main_param_list", "alpha_main_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
        addEntry(SUBPART_TATTOO, new SubpartEntry(SUBPART_TATTOO, "mPelvis", "tattoo", "tattoo_main_param_list", "tattoo_main_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f),SEX_BOTH));
		addEntry(SUBPART_UNIVERSAL, new SubpartEntry(SUBPART_UNIVERSAL, "mPelvis", "universal", "universal_main_param_list", "universal_main_tab", LLVector3d(0.f, 0.f, 0.1f), LLVector3d(-2.5f, 0.5f, 0.8f), SEX_BOTH));

		addEntry(SUBPART_PHYSICS_BREASTS_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BREASTS_UPDOWN, "mTorso", "physics_breasts_updown", "physics_breasts_updown_param_list", "physics_breasts_updown_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(0.f, 0.f, 0.f),SEX_FEMALE));
        addEntry(SUBPART_PHYSICS_BREASTS_INOUT, new SubpartEntry(SUBPART_PHYSICS_BREASTS_INOUT, "mTorso", "physics_breasts_inout", "physics_breasts_inout_param_list", "physics_breasts_inout_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(0.f, 0.f, 0.f),SEX_FEMALE));
        addEntry(SUBPART_PHYSICS_BREASTS_LEFTRIGHT, new SubpartEntry(SUBPART_PHYSICS_BREASTS_LEFTRIGHT, "mTorso", "physics_breasts_leftright", "physics_breasts_leftright_param_list", "physics_breasts_leftright_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(0.f, 0.f, 0.f),SEX_FEMALE));
        addEntry(SUBPART_PHYSICS_BELLY_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BELLY_UPDOWN, "mTorso", "physics_belly_updown", "physics_belly_updown_param_list", "physics_belly_updown_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(0.f, 0.f, 0.f),SEX_BOTH));
        addEntry(SUBPART_PHYSICS_BUTT_UPDOWN, new SubpartEntry(SUBPART_PHYSICS_BUTT_UPDOWN, "mTorso", "physics_butt_updown", "physics_butt_updown_param_list", "physics_butt_updown_tab", LLVector3d(0.f, 0.f, 0.3f), LLVector3d(0.f, 0.f, 0.f),SEX_BOTH));
        addEntry(SUBPART_PHYSICS_BUTT_LEFTRIGHT, new SubpartEntry(SUBPART_PHYSICS_BUTT_LEFTRIGHT, "mTorso", "physics_butt_leftright", "physics_butt_leftright_param_list", "physics_butt_leftright_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(0.f, 0.f, 0.f),SEX_BOTH));
        addEntry(SUBPART_PHYSICS_ADVANCED, new SubpartEntry(SUBPART_PHYSICS_ADVANCED, "mTorso", "physics_advanced", "physics_advanced_param_list", "physics_advanced_tab", LLVector3d(0.f, 0.f, 0.f), LLVector3d(0.f, 0.f, 0.f),SEX_BOTH));
}

LLEditWearableDictionary::SubpartEntry::SubpartEntry(ESubpart part,
                                         const std::string &joint,
                                         const std::string &edit_group,
                                         const std::string &param_list,
                                         const std::string &accordion_tab,
                                         const LLVector3d  &target_offset,
                                         const LLVector3d  &camera_offset,
                                         const ESex        &sex) :
        LLDictionaryEntry(edit_group),
        mSubpart(part),
        mTargetJoint(joint),
        mEditGroup(edit_group),
        mParamList(param_list),
        mAccordionTab(accordion_tab),
        mTargetOffset(target_offset),
        mCameraOffset(camera_offset),
        mSex(sex)
{
}

LLEditWearableDictionary::ColorSwatchCtrls::ColorSwatchCtrls()
{
        addEntry ( TEX_UPPER_SHIRT,  new PickerControlEntry (TEX_UPPER_SHIRT, "Color/Tint" ));
        addEntry ( TEX_LOWER_PANTS,  new PickerControlEntry (TEX_LOWER_PANTS, "Color/Tint" ));
        addEntry ( TEX_LOWER_SHOES,  new PickerControlEntry (TEX_LOWER_SHOES, "Color/Tint" ));
        addEntry ( TEX_LOWER_SOCKS,  new PickerControlEntry (TEX_LOWER_SOCKS, "Color/Tint" ));
        addEntry ( TEX_UPPER_JACKET, new PickerControlEntry (TEX_UPPER_JACKET, "Color/Tint" ));
        addEntry ( TEX_SKIRT,  new PickerControlEntry (TEX_SKIRT, "Color/Tint" ));
        addEntry ( TEX_UPPER_GLOVES, new PickerControlEntry (TEX_UPPER_GLOVES, "Color/Tint" ));
        addEntry ( TEX_UPPER_UNDERSHIRT, new PickerControlEntry (TEX_UPPER_UNDERSHIRT, "Color/Tint" ));
        addEntry ( TEX_LOWER_UNDERPANTS, new PickerControlEntry (TEX_LOWER_UNDERPANTS, "Color/Tint" ));
        addEntry ( TEX_HEAD_TATTOO, new PickerControlEntry(TEX_HEAD_TATTOO, "Color/Tint" ));
		addEntry (TEX_HEAD_UNIVERSAL_TATTOO, new PickerControlEntry(TEX_HEAD_UNIVERSAL_TATTOO, "Color/Tint"));
}

LLEditWearableDictionary::TextureCtrls::TextureCtrls()
{
        addEntry ( TEX_HEAD_BODYPAINT,  new PickerControlEntry (TEX_HEAD_BODYPAINT,  "Head", LLUUID::null, TRUE ));
        addEntry ( TEX_UPPER_BODYPAINT, new PickerControlEntry (TEX_UPPER_BODYPAINT, "Upper Body", LLUUID::null, TRUE ));
        addEntry ( TEX_LOWER_BODYPAINT, new PickerControlEntry (TEX_LOWER_BODYPAINT, "Lower Body", LLUUID::null, TRUE ));
        addEntry ( TEX_HAIR, new PickerControlEntry (TEX_HAIR, "Texture", LLUUID( gSavedSettings.getString( "UIImgDefaultHairUUID" ) ), FALSE ));
        addEntry ( TEX_EYES_IRIS, new PickerControlEntry (TEX_EYES_IRIS, "Iris", LLUUID( gSavedSettings.getString( "UIImgDefaultEyesUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_SHIRT, new PickerControlEntry (TEX_UPPER_SHIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultShirtUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_PANTS, new PickerControlEntry (TEX_LOWER_PANTS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultPantsUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_SHOES, new PickerControlEntry (TEX_LOWER_SHOES, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultShoesUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_SOCKS, new PickerControlEntry (TEX_LOWER_SOCKS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultSocksUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_JACKET, new PickerControlEntry (TEX_UPPER_JACKET, "Upper Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultJacketUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_JACKET, new PickerControlEntry (TEX_LOWER_JACKET, "Lower Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultJacketUUID" ) ), FALSE ));
        addEntry ( TEX_SKIRT, new PickerControlEntry (TEX_SKIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultSkirtUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_GLOVES, new PickerControlEntry (TEX_UPPER_GLOVES, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultGlovesUUID" ) ), FALSE ));
        addEntry ( TEX_UPPER_UNDERSHIRT, new PickerControlEntry (TEX_UPPER_UNDERSHIRT, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultUnderwearUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_UNDERPANTS, new PickerControlEntry (TEX_LOWER_UNDERPANTS, "Fabric", LLUUID( gSavedSettings.getString( "UIImgDefaultUnderwearUUID" ) ), FALSE ));
        addEntry ( TEX_LOWER_ALPHA, new PickerControlEntry (TEX_LOWER_ALPHA, "Lower Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE ));
        addEntry ( TEX_UPPER_ALPHA, new PickerControlEntry (TEX_UPPER_ALPHA, "Upper Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE ));
        addEntry ( TEX_HEAD_ALPHA, new PickerControlEntry (TEX_HEAD_ALPHA, "Head Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE ));
        addEntry ( TEX_EYES_ALPHA, new PickerControlEntry (TEX_EYES_ALPHA, "Eye Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE ));
        addEntry ( TEX_HAIR_ALPHA, new PickerControlEntry (TEX_HAIR_ALPHA, "Hair Alpha", LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) ), TRUE ));
        addEntry ( TEX_LOWER_TATTOO, new PickerControlEntry (TEX_LOWER_TATTOO, "Lower Tattoo", LLUUID::null, TRUE ));
        addEntry ( TEX_UPPER_TATTOO, new PickerControlEntry (TEX_UPPER_TATTOO, "Upper Tattoo", LLUUID::null, TRUE ));
        addEntry ( TEX_HEAD_TATTOO, new PickerControlEntry (TEX_HEAD_TATTOO, "Head Tattoo", LLUUID::null, TRUE ));
		addEntry ( TEX_LOWER_UNIVERSAL_TATTOO, new PickerControlEntry( TEX_LOWER_UNIVERSAL_TATTOO, "Lower Universal Tattoo", LLUUID::null, TRUE));
		addEntry ( TEX_UPPER_UNIVERSAL_TATTOO, new PickerControlEntry( TEX_UPPER_UNIVERSAL_TATTOO, "Upper Universal Tattoo", LLUUID::null, TRUE));
		addEntry ( TEX_HEAD_UNIVERSAL_TATTOO, new PickerControlEntry( TEX_HEAD_UNIVERSAL_TATTOO, "Head Universal Tattoo", LLUUID::null, TRUE));
		addEntry ( TEX_SKIRT_TATTOO, new PickerControlEntry(TEX_SKIRT_TATTOO, "Skirt Tattoo", LLUUID::null, TRUE));
		addEntry ( TEX_HAIR_TATTOO, new PickerControlEntry(TEX_HAIR_TATTOO, "Hair Tattoo", LLUUID::null, TRUE));
		addEntry ( TEX_EYES_TATTOO, new PickerControlEntry(TEX_EYES_TATTOO, "Eyes Tattoo", LLUUID::null, TRUE));
		addEntry (TEX_LEFT_ARM_TATTOO, new PickerControlEntry(TEX_LEFT_ARM_TATTOO, "Left Arm Tattoo", LLUUID::null, TRUE));
		addEntry (TEX_LEFT_LEG_TATTOO, new PickerControlEntry(TEX_LEFT_LEG_TATTOO, "Left Leg Tattoo", LLUUID::null, TRUE));
		addEntry (TEX_AUX1_TATTOO, new PickerControlEntry(TEX_AUX1_TATTOO, "Aux1 Tattoo", LLUUID::null, TRUE));
		addEntry (TEX_AUX2_TATTOO, new PickerControlEntry(TEX_AUX2_TATTOO, "Aux2 Tattoo", LLUUID::null, TRUE));
		addEntry (TEX_AUX3_TATTOO, new PickerControlEntry(TEX_AUX3_TATTOO, "Aux3 Tattoo", LLUUID::null, TRUE));
}

LLEditWearableDictionary::PickerControlEntry::PickerControlEntry(ETextureIndex tex_index,
                                         const std::string name,
                                         const LLUUID default_image_id,
                                         const bool allow_no_texture) :
        LLDictionaryEntry(name),
        mTextureIndex(tex_index),
        mControlName(name),
        mDefaultImageId(default_image_id),
        mAllowNoTexture(allow_no_texture)
{
}

/**
 * Class to prevent hack in LLButton's constructor and use paddings declared in xml.
 */
class LLLabledBackButton : public LLButton
{
public:
        struct Params : public LLInitParam::Block<Params, LLButton::Params>
        {
                Params() {}
        };
protected:
        friend class LLUICtrlFactory;
        LLLabledBackButton(const Params&);
};

static LLDefaultChildRegistry::Register<LLLabledBackButton> labeled_back_btn("labeled_back_button");

LLLabledBackButton::LLLabledBackButton(const Params& params)
: LLButton(params)
{
        // override hack in LLButton's constructor to use paddings have been set in xml
        setLeftHPad(params.pad_left);
        setRightHPad(params.pad_right);
}

// Helper functions.
static const texture_vec_t null_texture_vec;

// Specializations of this template function return a vector of texture indexes of particular control type
// (i.e. LLColorSwatchCtrl or LLTextureCtrl) which are contained in given WearableEntry.
template <typename T>
const texture_vec_t&
get_pickers_indexes(const LLEditWearableDictionary::WearableEntry *wearable_entry) { return null_texture_vec; }

// Specializations of this template function return picker control entry for particular control type.
template <typename T>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry (const ETextureIndex index) { return NULL; }

typedef boost::function<void(LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry*)> function_t;

typedef struct PickerControlEntryNamePredicate
{
        PickerControlEntryNamePredicate(const std::string name) : mName (name) {};
        bool operator()(const LLEditWearableDictionary::PickerControlEntry* entry) const
        {
                return (entry && entry->mName == mName);
        }
private:
        const std::string mName;
} PickerControlEntryNamePredicate;

// A full specialization of get_pickers_indexes for LLColorSwatchCtrl
template <>
const texture_vec_t&
get_pickers_indexes<LLColorSwatchCtrl> (const LLEditWearableDictionary::WearableEntry *wearable_entry)
{
        if (!wearable_entry)
        {
                LL_WARNS() << "could not get LLColorSwatchCtrl indexes for null wearable entry." << LL_ENDL;
                return null_texture_vec;
        }
        return wearable_entry->mColorSwatchCtrls;
}

// A full specialization of get_pickers_indexes for LLTextureCtrl
template <>
const texture_vec_t&
get_pickers_indexes<LLTextureCtrl> (const LLEditWearableDictionary::WearableEntry *wearable_entry)
{
        if (!wearable_entry)
        {
                LL_WARNS() << "could not get LLTextureCtrl indexes for null wearable entry." << LL_ENDL;
                return null_texture_vec;
        }
        return wearable_entry->mTextureCtrls;
}

// A full specialization of get_picker_entry for LLColorSwatchCtrl
template <>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry<LLColorSwatchCtrl> (const ETextureIndex index)
{
        return LLEditWearableDictionary::getInstance()->getColorSwatch(index);
}

// A full specialization of get_picker_entry for LLTextureCtrl
template <>
const LLEditWearableDictionary::PickerControlEntry*
get_picker_entry<LLTextureCtrl> (const ETextureIndex index)
{
        return LLEditWearableDictionary::getInstance()->getTexturePicker(index);
}

template <typename CtrlType, class Predicate>
const LLEditWearableDictionary::PickerControlEntry*
find_picker_ctrl_entry_if(LLWearableType::EType type, const Predicate pred)
{
        const LLEditWearableDictionary::WearableEntry *wearable_entry
                = LLEditWearableDictionary::getInstance()->getWearable(type);
        if (!wearable_entry)
        {
                LL_WARNS() << "could not get wearable dictionary entry for wearable of type: " << type << LL_ENDL;
                return NULL;
        }
        const texture_vec_t& indexes = get_pickers_indexes<CtrlType>(wearable_entry);
        for (texture_vec_t::const_iterator
                         iter = indexes.begin(),
                         iter_end = indexes.end();
                 iter != iter_end; ++iter)
        {
                const ETextureIndex te = *iter;
                const LLEditWearableDictionary::PickerControlEntry*     entry
                        = get_picker_entry<CtrlType>(te);
                if (!entry)
                {
                        LL_WARNS() << "could not get picker dictionary entry (" << te << ") for wearable of type: " << type << LL_ENDL;
                        continue;
                }
                if (pred(entry))
                {
                        return entry;
                }
        }
        return NULL;
}

template <typename CtrlType>
void
for_each_picker_ctrl_entry(LLPanel* panel, LLWearableType::EType type, function_t fun)
{
        if (!panel)
        {
                LL_WARNS() << "the panel wasn't passed for wearable of type: " << type << LL_ENDL;
                return;
        }
        const LLEditWearableDictionary::WearableEntry *wearable_entry
                = LLEditWearableDictionary::getInstance()->getWearable(type);
        if (!wearable_entry)
        {
                LL_WARNS() << "could not get wearable dictionary entry for wearable of type: " << type << LL_ENDL;
                return;
        }
        const texture_vec_t& indexes = get_pickers_indexes<CtrlType>(wearable_entry);
        for (texture_vec_t::const_iterator
                         iter = indexes.begin(),
                         iter_end = indexes.end();
                 iter != iter_end; ++iter)
        {
                const ETextureIndex te = *iter;
                const LLEditWearableDictionary::PickerControlEntry*     entry
                        = get_picker_entry<CtrlType>(te);
                if (!entry)
                {
                        LL_WARNS() << "could not get picker dictionary entry (" << te << ") for wearable of type: " << type << LL_ENDL;
                        continue;
                }
                fun (panel, entry);
        }
}

// The helper functions for pickers management
static void init_color_swatch_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
                // Can't get the color from the wearable here, since the wearable may not be set when this is called.
                color_swatch_ctrl->setOriginal(LLColor4::white);
        }
}

static void init_texture_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
                texture_ctrl->setDefaultImageAssetID(entry->mDefaultImageId);
                texture_ctrl->setAllowNoTexture(entry->mAllowNoTexture);
                // Don't allow (no copy) or (notransfer) textures to be selected.
                texture_ctrl->setImmediateFilterPermMask(PERM_NONE);
                texture_ctrl->setDnDFilterPermMask(PERM_NONE);
                texture_ctrl->setNonImmediateFilterPermMask(PERM_NONE);
        }
}

static void update_color_swatch_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
                color_swatch_ctrl->set(self->getWearable()->getClothesColor(entry->mTextureIndex));
                color_swatch_ctrl->closeFloaterColorPicker();
        }
}

static void update_texture_ctrl(LLPanelEditWearable* self, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
                LLUUID new_id;
                LLLocalTextureObject *lto = self->getWearable()->getLocalTextureObject(entry->mTextureIndex);
                if( lto && (lto->getID() != IMG_DEFAULT_AVATAR) )
                {
                        new_id = lto->getID();
                }
                else
                {
                        new_id = LLUUID::null;
                }
                LLUUID old_id = texture_ctrl->getImageAssetID();
                if (old_id != new_id)
                {
                        // texture has changed, close the floater to avoid DEV-22461
                        texture_ctrl->closeDependentFloater();
                }
                texture_ctrl->setImageAssetID(new_id);
        }
}

static void set_enabled_color_swatch_ctrl(bool enabled, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLColorSwatchCtrl* color_swatch_ctrl = panel->getChild<LLColorSwatchCtrl>(entry->mControlName);
        if (color_swatch_ctrl)
        {
                color_swatch_ctrl->setEnabled(enabled);
        }
}

static void set_enabled_texture_ctrl(bool enabled, LLPanel* panel, const LLEditWearableDictionary::PickerControlEntry* entry)
{
        LLTextureCtrl* texture_ctrl = panel->getChild<LLTextureCtrl>(entry->mControlName);
        if (texture_ctrl)
        {
                texture_ctrl->setEnabled(enabled);
        }
}

// LLPanelEditWearable

LLPanelEditWearable::LLPanelEditWearable()
        : LLPanel()
        , mWearablePtr(NULL)
        , mWearableItem(NULL)
{
        mCommitCallbackRegistrar.add("ColorSwatch.Commit", boost::bind(&LLPanelEditWearable::onColorSwatchCommit, this, _1));
        mCommitCallbackRegistrar.add("TexturePicker.Commit", boost::bind(&LLPanelEditWearable::onTexturePickerCommit, this, _1));
}

//virtual
LLPanelEditWearable::~LLPanelEditWearable()
{

}

bool LLPanelEditWearable::changeHeightUnits(const LLSD& new_value)
{
        updateMetricLayout( new_value.asBoolean() );
        updateTypeSpecificControls(LLWearableType::WT_SHAPE);
        return true;
}

void LLPanelEditWearable::updateMetricLayout(BOOL new_value)
{
        LLUIString current_metric, replacment_metric;
        current_metric = new_value ? mMeters : mFeet;
        replacment_metric = new_value ? mFeet : mMeters;
        mHeigthValue.setArg( "[METRIC1]", current_metric.getString() );
        mReplacementMetricUrl.setArg( "[URL_METRIC2]", std::string("[secondlife:///app/metricsystem ") + replacment_metric.getString() + std::string("]"));
}

void LLPanelEditWearable::updateAvatarHeightLabel()
{
        mTxtAvatarHeight->setText(LLStringUtil::null);
        LLStyle::Params param;
        param.color = mAvatarHeigthLabelColor;
        mTxtAvatarHeight->appendText(mHeigth, false, param);
        param.color = mAvatarHeigthValueLabelColor;
        mTxtAvatarHeight->appendText(mHeigthValue, false, param);
        param.color = mAvatarHeigthLabelColor; // using mAvatarHeigthLabelColor for '/' separator
        mTxtAvatarHeight->appendText(" / ", false, param);
        mTxtAvatarHeight->appendText(this->mReplacementMetricUrl, false, param);
}

void LLPanelEditWearable::onWearablePanelVisibilityChange(const LLSD &in_visible_chain, LLAccordionCtrl* accordion_ctrl)
{
        if (in_visible_chain.asBoolean() && accordion_ctrl != NULL)
        {
                accordion_ctrl->expandDefaultTab();
        }
}

void LLPanelEditWearable::setWearablePanelVisibilityChangeCallback(LLPanel* bodypart_panel)
{
        if (bodypart_panel != NULL)
        {
                LLAccordionCtrl* accordion_ctrl = bodypart_panel->getChild<LLAccordionCtrl>("wearable_accordion");

                if (accordion_ctrl != NULL)
                {
                        bodypart_panel->setVisibleCallback(
                                        boost::bind(&LLPanelEditWearable::onWearablePanelVisibilityChange, this, _2, accordion_ctrl));
                }
                else
                {
                        LL_WARNS() << "accordion_ctrl is NULL" << LL_ENDL;
                }
        }
        else
        {
                LL_WARNS() << "bodypart_panel is NULL" << LL_ENDL;
        }
}

// virtual 
BOOL LLPanelEditWearable::postBuild()
{
        // buttons
        mBtnRevert = getChild<LLButton>("revert_button");
        mBtnRevert->setClickedCallback(boost::bind(&LLPanelEditWearable::onRevertButtonClicked, this));

        mBtnBack = getChild<LLButton>("back_btn");
        mBackBtnLabel = mBtnBack->getLabelUnselected();
        mBtnBack->setLabel(LLStringUtil::null);

        mBtnBack->setClickedCallback(boost::bind(&LLPanelEditWearable::onBackButtonClicked, this));

        mNameEditor = getChild<LLLineEditor>("description");

        mPanelTitle = getChild<LLTextBox>("edit_wearable_title");
        mDescTitle = getChild<LLTextBox>("description_text");

        getChild<LLRadioGroup>("sex_radio")->setCommitCallback(boost::bind(&LLPanelEditWearable::onCommitSexChange, this));
        getChild<LLButton>("save_as_button")->setCommitCallback(boost::bind(&LLPanelEditWearable::onSaveAsButtonClicked, this));

        // The following panels will be shown/hidden based on what wearable we're editing
        // body parts
        mPanelShape = getChild<LLPanel>("edit_shape_panel");
        mPanelSkin = getChild<LLPanel>("edit_skin_panel");
        mPanelEyes = getChild<LLPanel>("edit_eyes_panel");
        mPanelHair = getChild<LLPanel>("edit_hair_panel");

        // Setting the visibility callback is applied only to the bodyparts panel
        // because currently they are the only ones whose 'wearable_accordion' has
        // multiple accordion tabs (see EXT-8164 for details).
        setWearablePanelVisibilityChangeCallback(mPanelShape);
        setWearablePanelVisibilityChangeCallback(mPanelSkin);
        setWearablePanelVisibilityChangeCallback(mPanelEyes);
        setWearablePanelVisibilityChangeCallback(mPanelHair);

        //clothes
        mPanelShirt = getChild<LLPanel>("edit_shirt_panel");
        mPanelPants = getChild<LLPanel>("edit_pants_panel");
        mPanelShoes = getChild<LLPanel>("edit_shoes_panel");
        mPanelSocks = getChild<LLPanel>("edit_socks_panel");
        mPanelJacket = getChild<LLPanel>("edit_jacket_panel");
        mPanelGloves = getChild<LLPanel>("edit_gloves_panel");
        mPanelUndershirt = getChild<LLPanel>("edit_undershirt_panel");
        mPanelUnderpants = getChild<LLPanel>("edit_underpants_panel");
        mPanelSkirt = getChild<LLPanel>("edit_skirt_panel");
        mPanelAlpha = getChild<LLPanel>("edit_alpha_panel");
        mPanelTattoo = getChild<LLPanel>("edit_tattoo_panel");
		mPanelUniversal = getChild<LLPanel>("edit_universal_panel");
        mPanelPhysics = getChild<LLPanel>("edit_physics_panel");

        mTxtAvatarHeight = mPanelShape->getChild<LLTextBox>("avatar_height");

        mWearablePtr = NULL;

        configureAlphaCheckbox(LLAvatarAppearanceDefines::TEX_LOWER_ALPHA, "lower alpha texture invisible");
        configureAlphaCheckbox(LLAvatarAppearanceDefines::TEX_UPPER_ALPHA, "upper alpha texture invisible");
        configureAlphaCheckbox(LLAvatarAppearanceDefines::TEX_HEAD_ALPHA, "head alpha texture invisible");
        configureAlphaCheckbox(LLAvatarAppearanceDefines::TEX_EYES_ALPHA, "eye alpha texture invisible");
        configureAlphaCheckbox(LLAvatarAppearanceDefines::TEX_HAIR_ALPHA, "hair alpha texture invisible");

        // configure tab expanded callbacks
        for (U32 type_index = 0; type_index < (U32)LLWearableType::WT_COUNT; ++type_index)
        {
                LLWearableType::EType type = (LLWearableType::EType) type_index;
                const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(type);
                if (!wearable_entry)
                {
                        LL_WARNS() << "could not get wearable dictionary entry for wearable of type: " << type << LL_ENDL;
                        continue;
                }
                U8 num_subparts = (U8)(wearable_entry->mSubparts.size());
        
                for (U8 index = 0; index < num_subparts; ++index)
                {
                        // dive into data structures to get the panel we need
                        ESubpart subpart_e = wearable_entry->mSubparts[index];
                        const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);
        
                        if (!subpart_entry)
                        {
                                LL_WARNS() << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << LL_ENDL;
                                continue;
                        }
        
                        const std::string accordion_tab = subpart_entry->mAccordionTab;
        
                        LLAccordionCtrlTab *tab = getChild<LLAccordionCtrlTab>(accordion_tab);
        
                        if (!tab)
                        {
                                LL_WARNS() << "could not get llaccordionctrltab from UI with name: " << accordion_tab << LL_ENDL;
                                continue;
                        }
        
                        // initialize callback to ensure camera view changes appropriately.
                        tab->setDropDownStateChangedCallback(boost::bind(&LLPanelEditWearable::onTabExpandedCollapsed,this,_2,index));
                }

                // initialize texture and color picker controls
                for_each_picker_ctrl_entry <LLColorSwatchCtrl> (getPanel(type), type, boost::bind(init_color_swatch_ctrl, this, _1, _2));
                for_each_picker_ctrl_entry <LLTextureCtrl>     (getPanel(type), type, boost::bind(init_texture_ctrl, this, _1, _2));
        }

        // init all strings
        mMeters         = mPanelShape->getString("meters");
        mFeet           = mPanelShape->getString("feet");
        mHeigth         = mPanelShape->getString("height") + " ";
        mHeigthValue    = "[HEIGHT] [METRIC1]";
        mReplacementMetricUrl   = "[URL_METRIC2]";

        std::string color = mPanelShape->getString("heigth_label_color");
        mAvatarHeigthLabelColor = LLUIColorTable::instance().getColor(color, LLColor4::green);
        color = mPanelShape->getString("heigth_value_label_color");
        mAvatarHeigthValueLabelColor = LLUIColorTable::instance().getColor(color, LLColor4::green);
        gSavedSettings.getControl("HeightUnits")->getSignal()->connect(boost::bind(&LLPanelEditWearable::changeHeightUnits, this, _2));
        updateMetricLayout(gSavedSettings.getBOOL("HeightUnits"));

        return TRUE;
}

// virtual 
// LLUICtrl
BOOL LLPanelEditWearable::isDirty() const
{
        BOOL isDirty = FALSE;
        if (mWearablePtr)
        {
			if (mWearablePtr->isDirty() ||
				( mWearableItem && mNameEditor && mWearableItem->getName().compare(mNameEditor->getText()) != 0 ))
			{
				isDirty = TRUE;
			}
        }
        return isDirty;
}
//virtual
void LLPanelEditWearable::draw()
{
        updateVerbs();
        if (getWearable() && getWearable()->getType() == LLWearableType::WT_SHAPE)
        {
                //updating avatar height
                updateTypeSpecificControls(LLWearableType::WT_SHAPE);
        }

        LLPanel::draw();
}

void LLPanelEditWearable::onClose()
{
	// any unsaved changes should be reverted at this point
	revertChanges();
}

void LLPanelEditWearable::setVisible(BOOL visible)
{
        if (!visible)
        {
                showWearable(mWearablePtr, FALSE);
        }
        LLPanel::setVisible(visible);
}

void LLPanelEditWearable::setWearable(LLViewerWearable *wearable, BOOL disable_camera_switch)
{
        showWearable(mWearablePtr, FALSE, disable_camera_switch);
        mWearablePtr = wearable;
        showWearable(mWearablePtr, TRUE, disable_camera_switch);
}

//static 
void LLPanelEditWearable::onBackButtonClicked(void* userdata)
{
    LLPanelEditWearable *panel = (LLPanelEditWearable*) userdata;
	if ( panel->isDirty() )
	{
		LLAppearanceMgr::instance().setOutfitDirty( true );		
	}
}

//static 
void LLPanelEditWearable::onRevertButtonClicked(void* userdata)
{
        LLPanelEditWearable *panel = (LLPanelEditWearable*) userdata;
        panel->revertChanges();
}

void LLPanelEditWearable::onSaveAsButtonClicked()
{
        LLSD args;
        args["DESC"] = mNameEditor->getText();

        LLNotificationsUtil::add("SaveWearableAs", args, LLSD(), boost::bind(&LLPanelEditWearable::saveAsCallback, this, _1, _2));
}

void LLPanelEditWearable::saveAsCallback(const LLSD& notification, const LLSD& response)
{
        S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
        if (0 == option)
        {
                std::string wearable_name = response["message"].asString();
                LLStringUtil::trim(wearable_name);
                if( !wearable_name.empty() )
                {
                        mNameEditor->setText(wearable_name);
                        saveChanges(true);
                }
        }
}

void LLPanelEditWearable::onCommitSexChange()
{
        if (!isAgentAvatarValid()) return;

        LLWearableType::EType type = mWearablePtr->getType();
        U32 index;
        if( !gAgentWearables.getWearableIndex(mWearablePtr, index) ||
			!gAgentWearables.isWearableModifiable(type, index))
        {
			return;
        }

        LLViewerVisualParam* param = static_cast<LLViewerVisualParam*>(gAgentAvatarp->getVisualParam( "male" ));
        if( !param )
        {
			return;
        }

        bool is_new_sex_male = (gSavedSettings.getU32("AvatarSex") ? SEX_MALE : SEX_FEMALE) == SEX_MALE;
        LLViewerWearable*     wearable = gAgentWearables.getViewerWearable(type, index);
        if (wearable)
        {
                wearable->setVisualParamWeight(param->getID(), is_new_sex_male);
        }
        param->setWeight( is_new_sex_male);

        gAgentAvatarp->updateSexDependentLayerSets();

        gAgentAvatarp->updateVisualParams();
        showWearable(mWearablePtr, TRUE, TRUE);
        updateScrollingPanelUI();
}

void LLPanelEditWearable::onTexturePickerCommit(const LLUICtrl* ctrl)
{
        const LLTextureCtrl* texture_ctrl = dynamic_cast<const LLTextureCtrl*>(ctrl);
        if (!texture_ctrl)
        {
                LL_WARNS() << "got commit signal from not LLTextureCtrl." << LL_ENDL;
                return;
        }

        if (getWearable())
        {
                LLWearableType::EType type = getWearable()->getType();
                const PickerControlEntryNamePredicate name_pred(texture_ctrl->getName());
                const LLEditWearableDictionary::PickerControlEntry* entry
                        = find_picker_ctrl_entry_if<LLTextureCtrl, PickerControlEntryNamePredicate>(type, name_pred);
                if (entry)
                {
                        // Set the new version
                        LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(texture_ctrl->getImageAssetID());
                        if( image->getID() == IMG_DEFAULT )
                        {
                                image = LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT_AVATAR);
                        }
                        if (getWearable())
                        {
							U32 index;
							if (gAgentWearables.getWearableIndex(getWearable(), index))
							{
								gAgentAvatarp->setLocalTexture(entry->mTextureIndex, image, FALSE, index);
								LLVisualParamHint::requestHintUpdates();
								gAgentAvatarp->wearableUpdated(type);
							}
							else
							{
								LL_WARNS() << "wearable not found in gAgentWearables" << LL_ENDL;
							}
                        }
                }
                else
                {
                        LL_WARNS() << "could not get texture picker dictionary entry for wearable of type: " << type << LL_ENDL;
                }
        }
}

void LLPanelEditWearable::onColorSwatchCommit(const LLUICtrl* ctrl)
{
        if (getWearable())
        {
                LLWearableType::EType type = getWearable()->getType();
                const PickerControlEntryNamePredicate name_pred(ctrl->getName());
                const LLEditWearableDictionary::PickerControlEntry* entry
                        = find_picker_ctrl_entry_if<LLColorSwatchCtrl, PickerControlEntryNamePredicate>(type, name_pred);
                if (entry)
                {
                        const LLColor4& old_color = getWearable()->getClothesColor(entry->mTextureIndex);
                        const LLColor4& new_color = LLColor4(ctrl->getValue());
                        if( old_color != new_color )
                        {
                                getWearable()->setClothesColor(entry->mTextureIndex, new_color);
                                LLVisualParamHint::requestHintUpdates();
                                gAgentAvatarp->wearableUpdated(getWearable()->getType());
                        }
                }
                else
                {
                        LL_WARNS() << "could not get color swatch dictionary entry for wearable of type: " << type << LL_ENDL;
                }
        }
}

void LLPanelEditWearable::updatePanelPickerControls(LLWearableType::EType type)
{
        LLPanel* panel = getPanel(type);
        if (!panel)
                return;

        bool is_modifiable = false;

        if(mWearableItem)
        {
                const LLPermissions& perm = mWearableItem->getPermissions();
                is_modifiable = perm.allowModifyBy(gAgent.getID(), gAgent.getGroupID());
        }

        if (is_modifiable)
        {
                // Update picker controls
                for_each_picker_ctrl_entry <LLColorSwatchCtrl> (panel, type, boost::bind(update_color_swatch_ctrl, this, _1, _2));
                for_each_picker_ctrl_entry <LLTextureCtrl>     (panel, type, boost::bind(update_texture_ctrl, this, _1, _2));
        }
        else
        {
                // Disable controls
                for_each_picker_ctrl_entry <LLColorSwatchCtrl> (panel, type, boost::bind(set_enabled_color_swatch_ctrl, false, _1, _2));
                for_each_picker_ctrl_entry <LLTextureCtrl>     (panel, type, boost::bind(set_enabled_texture_ctrl, false, _1, _2));
        }
}

void LLPanelEditWearable::incrementCofVersionLegacy()
{

}

void LLPanelEditWearable::saveChanges(bool force_save_as)
{
        if (!mWearablePtr || !isDirty())
        {
                // do nothing if no unsaved changes
                return;
        }

        U32 index;
		if (!gAgentWearables.getWearableIndex(mWearablePtr, index))
		{
			LL_WARNS() << "wearable not found" << LL_ENDL;
			return;
		}

        std::string new_name = mNameEditor->getText();

		// Find an existing link to this wearable's inventory item, if any, and its description field.
		LLInventoryItem *link_item = NULL;
		std::string description;
		LLInventoryModel::item_array_t links =
			LLAppearanceMgr::instance().findCOFItemLinks(mWearablePtr->getItemID());
		if (links.size()>0)
		{
			link_item = links.at(0).get();
			if (link_item && link_item->getIsLinkType())
			{
				description = link_item->getActualDescription();
			}
		}

        if (force_save_as)
        {
			// the name of the wearable has changed, re-save wearable with new name
			LLAppearanceMgr::instance().removeCOFItemLinks(mWearablePtr->getItemID(),gAgentAvatarp->mEndCustomizeCallback);
			gAgentWearables.saveWearableAs(mWearablePtr->getType(), index, new_name, description, FALSE);
			mNameEditor->setText(mWearableItem->getName());
        }
        else
        {
			// Make another copy of this link, with the same
			// description.  This is needed to bump the COF
			// version so texture baking service knows appearance has changed.
			if (link_item)
			{
				// Create new link
				LL_DEBUGS("Avatar") << "link refresh, creating new link to " << link_item->getLinkedUUID()
									<< " removing old link at " << link_item->getUUID()
									<< " wearable item id " << mWearablePtr->getItemID() << LL_ENDL;

				LLInventoryObject::const_object_list_t obj_array;
				obj_array.push_back(LLConstPointer<LLInventoryObject>(link_item));
				link_inventory_array(LLAppearanceMgr::instance().getCOF(),
									 obj_array, 
									 gAgentAvatarp->mEndCustomizeCallback);
				// Remove old link
				remove_inventory_item(link_item->getUUID(), gAgentAvatarp->mEndCustomizeCallback);
			}
			gAgentWearables.saveWearable(mWearablePtr->getType(), index, new_name);
        }

	
}

void LLPanelEditWearable::revertChanges()
{
        if (!mWearablePtr || !isDirty())
        {
                // no unsaved changes to revert
                return;
        }

        mWearablePtr->revertValues();
        mNameEditor->setText(mWearableItem->getName());
        updatePanelPickerControls(mWearablePtr->getType());
        updateTypeSpecificControls(mWearablePtr->getType());
        gAgentAvatarp->wearableUpdated(mWearablePtr->getType());
}

void LLPanelEditWearable::showWearable(LLViewerWearable* wearable, BOOL show, BOOL disable_camera_switch)
{
        if (!wearable)
        {
                return;
        }

        mWearableItem = gInventory.getItem(mWearablePtr->getItemID());
        llassert(mWearableItem);

        LLWearableType::EType type = wearable->getType();
        LLPanel *targetPanel = NULL;
        std::string title;
        std::string description_title;

        const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(type);
        if (!wearable_entry)
        {
                LL_WARNS() << "called LLPanelEditWearable::showWearable with an invalid wearable type! (" << type << ")" << LL_ENDL;
                return;
        }

        targetPanel = getPanel(type);
        title = getString(wearable_entry->mTitle);
        description_title = getString(wearable_entry->mDescTitle);

        // Update picker controls state
        for_each_picker_ctrl_entry <LLColorSwatchCtrl> (targetPanel, type, boost::bind(set_enabled_color_swatch_ctrl, show, _1, _2));
        for_each_picker_ctrl_entry <LLTextureCtrl>     (targetPanel, type, boost::bind(set_enabled_texture_ctrl, show, _1, _2));

        targetPanel->setVisible(show);
        toggleTypeSpecificControls(type);
		// Update type controls here
		updateTypeSpecificControls(type);

        if (show)
        {
                mPanelTitle->setText(title);
                mPanelTitle->setToolTip(title);
                mDescTitle->setText(description_title);
                
                // set name
                mNameEditor->setText(mWearableItem->getName());

                updatePanelPickerControls(type);

                // clear and rebuild visual param list
                U8 num_subparts = (U8)(wearable_entry->mSubparts.size());
        
                for (U8 index = 0; index < num_subparts; ++index)
                {
                        // dive into data structures to get the panel we need
                        ESubpart subpart_e = wearable_entry->mSubparts[index];
                        const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);
        
                        if (!subpart_entry)
                        {
                                LL_WARNS() << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << LL_ENDL;
                                continue;
                        }
        
                        const std::string scrolling_panel = subpart_entry->mParamList;
                        const std::string accordion_tab = subpart_entry->mAccordionTab;
        
                        LLScrollingPanelList *panel_list = getChild<LLScrollingPanelList>(scrolling_panel);
                        LLAccordionCtrlTab *tab = getChild<LLAccordionCtrlTab>(accordion_tab);
			
                        if (!panel_list)
                        {
                                LL_WARNS() << "could not get scrolling panel list: " << scrolling_panel << LL_ENDL;
                                continue;
                        }
        
                        if (!tab)
                        {
                                LL_WARNS() << "could not get llaccordionctrltab from UI with name: " << accordion_tab << LL_ENDL;
                                continue;
                        }

			// Don't show female subparts if you're not female, etc.
			if (!(gAgentAvatarp->getSex() & subpart_entry->mSex))
			{
				tab->setVisible(FALSE);
				continue;
			}
			else
			{
				tab->setVisible(TRUE);
			}
			
                        // what edit group do we want to extract params for?
                        const std::string edit_group = subpart_entry->mEditGroup;
        
                        // storage for ordered list of visual params
                        value_map_t sorted_params;
                        getSortedParams(sorted_params, edit_group);

                        LLJoint* jointp = gAgentAvatarp->getJoint( subpart_entry->mTargetJoint );
                        if (!jointp)
                        {
                                jointp = gAgentAvatarp->getJoint("mHead");
                        }

                        buildParamList(panel_list, sorted_params, tab, jointp);
        
                        updateScrollingPanelUI();
                }
                if (!disable_camera_switch)
                {
                        showDefaultSubpart();
                }

                updateVerbs();
        }
}

void LLPanelEditWearable::showDefaultSubpart()
{
        changeCamera(0);
}

void LLPanelEditWearable::onTabExpandedCollapsed(const LLSD& param, U8 index)
{
        bool expanded = param.asBoolean();

        if (!mWearablePtr || !gAgentCamera.cameraCustomizeAvatar())
        {
                // we don't have a valid wearable we're editing, or we've left the wearable editor
                return;
        }

        if (expanded)
        {
                changeCamera(index);
        }

}

void LLPanelEditWearable::changeCamera(U8 subpart)
{
	// Don't change the camera if this type doesn't have a camera switch.
	// Useful for wearables like physics that don't have an associated physical body part.
	if (LLWearableType::getInstance()->getDisableCameraSwitch(mWearablePtr->getType()))
	{
		return;
	}
        const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(mWearablePtr->getType());
        if (!wearable_entry)
        {
                LL_INFOS() << "could not get wearable dictionary entry for wearable type: " << mWearablePtr->getType() << LL_ENDL;
                return;
        }

        if (subpart >= wearable_entry->mSubparts.size())
        {
                LL_INFOS() << "accordion tab expanded for invalid subpart. Wearable type: " << mWearablePtr->getType() << " subpart num: " << subpart << LL_ENDL;
                return;
        }

        ESubpart subpart_e = wearable_entry->mSubparts[subpart];
        const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);

        if (!subpart_entry)
        {
                LL_WARNS() << "could not get wearable subpart dictionary entry for subpart: " << subpart_e << LL_ENDL;
                return;
        }

        // Update the camera
        gMorphView->setCameraTargetJoint( gAgentAvatarp->getJoint( subpart_entry->mTargetJoint ) );
        gMorphView->setCameraTargetOffset( subpart_entry->mTargetOffset );
        gMorphView->setCameraOffset( subpart_entry->mCameraOffset );
        if (gSavedSettings.getBOOL("AppearanceCameraMovement"))
        {
            // Unlock focus from avatar but don't stop animation to not interrupt ANIM_AGENT_CUSTOMIZE
            gAgentCamera.setFocusOnAvatar(FALSE, gAgentCamera.getCameraAnimating());
            gMorphView->updateCamera();
        }
}

void LLPanelEditWearable::updateScrollingPanelList()
{
        updateScrollingPanelUI();
}

void LLPanelEditWearable::toggleTypeSpecificControls(LLWearableType::EType type)
{
        // Toggle controls specific to shape editing panel.
        {
                bool is_shape = (type == LLWearableType::WT_SHAPE);
                getChildView("sex_radio")->setVisible( is_shape);
                getChildView("female_icon")->setVisible( is_shape);
                getChildView("male_icon")->setVisible( is_shape);
        }
}

void LLPanelEditWearable::updateTypeSpecificControls(LLWearableType::EType type)
{
        const F32 ONE_METER = 1.0;
        const F32 ONE_FOOT = 0.3048 * ONE_METER; // in meters
        // Update controls specific to shape editing panel.
        if (type == LLWearableType::WT_SHAPE)
        {
                // Update avatar height
                F32 new_size = gAgentAvatarp->mBodySize.mV[VZ];
                if (gSavedSettings.getBOOL("HeightUnits") == FALSE)
                {
                        // convert meters to feet
                        new_size = new_size / ONE_FOOT;
                }

                std::string avatar_height_str = llformat("%.2f", new_size);
                mHeigthValue.setArg("[HEIGHT]", avatar_height_str);
                updateAvatarHeightLabel();
        }

        if (LLWearableType::WT_ALPHA == type)
        {
                updateAlphaCheckboxes();

                initPreviousAlphaTextures();
        }
}

void LLPanelEditWearable::updateScrollingPanelUI()
{
        // do nothing if we don't have a valid wearable we're editing
        if (mWearablePtr == NULL)
        {
                return;
        }

        LLWearableType::EType type = mWearablePtr->getType();
        LLPanel *panel = getPanel(type);

        if(panel && (mWearablePtr->getItemID().notNull()))
        {
                const LLEditWearableDictionary::WearableEntry *wearable_entry = LLEditWearableDictionary::getInstance()->getWearable(type);
                llassert(wearable_entry);
                if (!wearable_entry) return;
                U8 num_subparts = (U8)(wearable_entry->mSubparts.size());

                LLScrollingPanelParam::sUpdateDelayFrames = 0;
                for (U8 index = 0; index < num_subparts; ++index)
                {
                        // dive into data structures to get the panel we need
                        ESubpart subpart_e = wearable_entry->mSubparts[index];
                        const LLEditWearableDictionary::SubpartEntry *subpart_entry = LLEditWearableDictionary::getInstance()->getSubpart(subpart_e);

                        const std::string scrolling_panel = subpart_entry->mParamList;

                        LLScrollingPanelList *panel_list = getChild<LLScrollingPanelList>(scrolling_panel);
        
                        if (!panel_list)
                        {
                                LL_WARNS() << "could not get scrolling panel list: " << scrolling_panel << LL_ENDL;
                                continue;
                        }
                        
                        panel_list->updatePanels(TRUE);
                }
        }
}

LLPanel* LLPanelEditWearable::getPanel(LLWearableType::EType type)
{
        switch (type)
        {
                case LLWearableType::WT_SHAPE:
                        return mPanelShape;
                        break;

                case LLWearableType::WT_SKIN:
                        return mPanelSkin;
                        break;

                case LLWearableType::WT_HAIR:
                        return mPanelHair;
                        break;

                case LLWearableType::WT_EYES:
                        return mPanelEyes;
                        break;

                case LLWearableType::WT_SHIRT:
                        return mPanelShirt;
                        break;

                case LLWearableType::WT_PANTS:
                        return mPanelPants;
                        break;

                case LLWearableType::WT_SHOES:
                        return mPanelShoes;
                        break;

                case LLWearableType::WT_SOCKS:
                        return mPanelSocks;
                        break;

                case LLWearableType::WT_JACKET:
                        return mPanelJacket;
                        break;

                case LLWearableType::WT_GLOVES:
                        return mPanelGloves;
                        break;

                case LLWearableType::WT_UNDERSHIRT:
                        return mPanelUndershirt;
                        break;

                case LLWearableType::WT_UNDERPANTS:
                        return mPanelUnderpants;
                        break;

                case LLWearableType::WT_SKIRT:
                        return mPanelSkirt;
                        break;

                case LLWearableType::WT_ALPHA:
                        return mPanelAlpha;
                        break;

                case LLWearableType::WT_TATTOO:
                        return mPanelTattoo;
                        break;
				
				case LLWearableType::WT_UNIVERSAL:
					return mPanelUniversal;
					break;

                case LLWearableType::WT_PHYSICS:
                        return mPanelPhysics;
                        break;

                default:
                        break;
        }
        return NULL;
}

void LLPanelEditWearable::getSortedParams(value_map_t &sorted_params, const std::string &edit_group)
{
        LLWearable::visual_param_vec_t param_list;
        ESex avatar_sex = gAgentAvatarp->getSex();

        mWearablePtr->getVisualParams(param_list);

        for (LLWearable::visual_param_vec_t::iterator iter = param_list.begin();
                iter != param_list.end();
                ++iter)
        {
                LLViewerVisualParam *param = (LLViewerVisualParam*) *iter;

                if (param->getID() == -1 
                        || !param->isTweakable()
                        || param->getEditGroup() != edit_group 
                        || !(param->getSex() & avatar_sex))
                {
                        continue;
                }

                value_map_t::value_type vt(-param->getDisplayOrder(), param);
                llassert( sorted_params.find(-param->getDisplayOrder()) == sorted_params.end() ); //check for duplicates
                sorted_params.insert(vt);
        }
}

void LLPanelEditWearable::buildParamList(LLScrollingPanelList *panel_list, value_map_t &sorted_params, LLAccordionCtrlTab *tab, LLJoint* jointp)
{
        // sorted_params is sorted according to magnitude of effect from
        // least to greatest.  Adding to the front of the child list
        // reverses that order.
        if( panel_list )
        {
                panel_list->clearPanels();
                value_map_t::iterator end = sorted_params.end();
                for(value_map_t::iterator it = sorted_params.begin(); it != end; ++it)
                {
                        LLPanel::Params p;
                        p.name("LLScrollingPanelParam");
                        LLViewerWearable *wearable = this->getWearable();
                        LLScrollingPanelParamBase *panel_param = NULL;
                        if (wearable && wearable->getType() == LLWearableType::WT_PHYSICS) // Hack to show a different panel for physics.  Should generalize this later.
                        {
                                panel_param = new LLScrollingPanelParamBase( p, NULL, (*it).second, TRUE, this->getWearable(), jointp);
                        }
                        else
                        {
                                panel_param = new LLScrollingPanelParam( p, NULL, (*it).second, TRUE, this->getWearable(), jointp);
                        }
                        panel_list->addPanel( panel_param );
                }
        }
}

void LLPanelEditWearable::updateVerbs()
{
        bool can_copy = false;

        if(mWearableItem)
        {
                can_copy = mWearableItem->getPermissions().allowCopyBy(gAgentID);
        }

        BOOL is_dirty = isDirty();

        mBtnRevert->setEnabled(is_dirty);
        getChildView("save_as_button")->setEnabled(is_dirty && can_copy);

        if(isAgentAvatarValid())
        {
                // Update viewer's radio buttons (of RadioGroup with control_name="AvatarSex") of Avatar's gender
                // with value from "AvatarSex" setting
                gSavedSettings.setU32("AvatarSex", (gAgentAvatarp->getSex() == SEX_MALE) );
        }

        // update back button and title according to dirty state.
        static BOOL was_dirty = FALSE;
        if (was_dirty != is_dirty) // to avoid redundant changes because this method is called from draw
        {
                static S32 label_width = mBtnBack->getFont()->getWidth(mBackBtnLabel);
                const std::string& label = is_dirty ? mBackBtnLabel : LLStringUtil::null;
                const S32 delta_width = is_dirty ? label_width : -label_width;

                mBtnBack->setLabel(label);

                // update rect according to label width
                LLRect rect = mBtnBack->getRect();
                rect.mRight += delta_width;
                mBtnBack->setShape(rect);

                // update title rect according to back button width
                rect = mPanelTitle->getRect();
                rect.mLeft += delta_width;
                mPanelTitle->setShape(rect);

                was_dirty = is_dirty;
        }
}

void LLPanelEditWearable::configureAlphaCheckbox(LLAvatarAppearanceDefines::ETextureIndex te, const std::string& name)
{
        LLCheckBoxCtrl* checkbox = mPanelAlpha->getChild<LLCheckBoxCtrl>(name);
        checkbox->setCommitCallback(boost::bind(&LLPanelEditWearable::onInvisibilityCommit, this, checkbox, te));

        mAlphaCheckbox2Index[name] = te;
}

void LLPanelEditWearable::onInvisibilityCommit(LLCheckBoxCtrl* checkbox_ctrl, LLAvatarAppearanceDefines::ETextureIndex te)
{
        if (!checkbox_ctrl) return;
        if (!getWearable()) return;

        LL_INFOS() << "onInvisibilityCommit, self " << this << " checkbox_ctrl " << checkbox_ctrl << LL_ENDL;

		U32 index;
		if (!gAgentWearables.getWearableIndex(getWearable(),index))
		{
			LL_WARNS() << "wearable not found" << LL_ENDL;
			return;
		}
        bool new_invis_state = checkbox_ctrl->get();
        if (new_invis_state)
        {
                LLLocalTextureObject *lto = getWearable()->getLocalTextureObject(te);
                mPreviousAlphaTexture[te] = lto->getID();
                
                LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture( IMG_INVISIBLE );
				gAgentAvatarp->setLocalTexture(te, image, FALSE, index);
				gAgentAvatarp->wearableUpdated(getWearable()->getType());
        }
        else
        {
                // Try to restore previous texture, if any.
                LLUUID prev_id = mPreviousAlphaTexture[te];
                if (prev_id.isNull() || (prev_id == IMG_INVISIBLE))
                {
                        prev_id = LLUUID( gSavedSettings.getString( "UIImgDefaultAlphaUUID" ) );
                }
                if (prev_id.isNull()) return;
                
                LLViewerFetchedTexture* image = LLViewerTextureManager::getFetchedTexture(prev_id);
                if (!image) return;

                gAgentAvatarp->setLocalTexture(te, image, FALSE, index);
                gAgentAvatarp->wearableUpdated(getWearable()->getType());
        }

        updatePanelPickerControls(getWearable()->getType());
}

void LLPanelEditWearable::updateAlphaCheckboxes()
{
        for(string_texture_index_map_t::iterator iter = mAlphaCheckbox2Index.begin();
                iter != mAlphaCheckbox2Index.end(); ++iter )
        {
                LLAvatarAppearanceDefines::ETextureIndex te = (LLAvatarAppearanceDefines::ETextureIndex)iter->second;
                LLCheckBoxCtrl* ctrl = mPanelAlpha->getChild<LLCheckBoxCtrl>(iter->first);
                if (ctrl)
                {
                        ctrl->set(!gAgentAvatarp->isTextureVisible(te, mWearablePtr));
                }
        }
}

void LLPanelEditWearable::initPreviousAlphaTextures()
{
        initPreviousAlphaTextureEntry(TEX_LOWER_ALPHA);
        initPreviousAlphaTextureEntry(TEX_UPPER_ALPHA);
        initPreviousAlphaTextureEntry(TEX_HEAD_ALPHA);
        initPreviousAlphaTextureEntry(TEX_EYES_ALPHA);
        initPreviousAlphaTextureEntry(TEX_HAIR_ALPHA);
}

void LLPanelEditWearable::initPreviousAlphaTextureEntry(LLAvatarAppearanceDefines::ETextureIndex te)
{
        LLLocalTextureObject *lto = getWearable()->getLocalTextureObject(te);
        if (lto)
        {
                mPreviousAlphaTexture[te] = lto->getID();
        }
}

// handle secondlife:///app/metricsystem
class LLMetricSystemHandler : public LLCommandHandler
{
public:
        LLMetricSystemHandler() : LLCommandHandler("metricsystem", UNTRUSTED_CLICK_ONLY) { }

        bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
        {
                // change height units TRUE for meters and FALSE for feet
                BOOL new_value = (gSavedSettings.getBOOL("HeightUnits") == FALSE) ? TRUE : FALSE;
                gSavedSettings.setBOOL("HeightUnits", new_value);
                return true;
        }
};

LLMetricSystemHandler gMetricSystemHandler;

// EOF
