/**
 * @file lltexturectrl.h
 * @author Richard Nelson, James Cook
 * @brief LLTextureCtrl class header file including related functions
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLTEXTURECTRL_H
#define LL_LLTEXTURECTRL_H

#include "llcoord.h"
#include "llfiltereditor.h"
#include "llfloater.h"
#include "llfolderview.h"
#include "lllocalbitmaps.h"
#include "llstring.h"
#include "lluictrl.h"
#include "llpermissionsflags.h"
#include "lltextbox.h" // for params
#include "llviewerinventory.h"
#include "llviewborder.h" // for params
#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llwindow.h"

class LLComboBox;
class LLFloaterTexturePicker;
class LLInventoryItem;
class LLViewerFetchedTexture;
class LLFetchedGLTFMaterial;

// used for setting drag & drop callbacks.
typedef boost::function<bool (LLUICtrl*, LLInventoryItem*)> drag_n_drop_callback;
typedef boost::function<void (LLInventoryItem*)> texture_selected_callback;

// Helper functions for UI that work with picker
bool get_is_predefined_texture(LLUUID asset_id);

// texture picker works by asset ids since objects normaly do
// not retain inventory ids as result these functions are looking
// for textures in inventory by asset ids
// This search can be performance unfriendly and doesn't warranty
// that the texture is original source of asset
LLUUID get_copy_free_item_by_asset_id(LLUUID image_id, bool no_trans_perm = false);
bool get_can_copy_texture(LLUUID image_id);


typedef enum e_pick_inventory_type
{
    PICK_TEXTURE_MATERIAL = 0,
    PICK_TEXTURE = 1,
    PICK_MATERIAL = 2,
} EPickInventoryType;

namespace LLInitParam
{
    template<>
    struct TypeValues<EPickInventoryType> : public TypeValuesHelper<EPickInventoryType>
    {
        static void declareValues();
    };
}

enum LLPickerSource
{
    PICKER_INVENTORY,
    PICKER_LOCAL,
    PICKER_BAKE,
    PICKER_UNKNOWN, // on cancel, default ids
};

//////////////////////////////////////////////////////////////////////////////////////////
// LLTextureCtrl


class LLTextureCtrl
: public LLUICtrl
{
public:
    typedef enum e_texture_pick_op
    {
        TEXTURE_CHANGE,
        TEXTURE_SELECT,
        TEXTURE_CANCEL
    } ETexturePickOp;

public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLUUID>        image_id;
        Optional<LLUUID>        default_image_id;
        Optional<std::string>   default_image_name;
        Optional<EPickInventoryType> pick_type;
        Optional<bool>          allow_no_texture;
        Optional<bool>          can_apply_immediately;
        Optional<bool>          no_commit_on_selection; // alternative mode: commit occurs and the widget gets dirty
                                                        // only on DnD or when OK is pressed in the picker
        Optional<S32>           label_width;
        Optional<LLUIColor>     border_color;
        Optional<LLUIImage*>    fallback_image;

        Optional<LLTextBox::Params> multiselect_text,
                                    caption_text;

        Optional<LLViewBorder::Params> border;

        Params()
        :   image_id("image"),
            default_image_id("default_image_id"),
            default_image_name("default_image_name"),
            pick_type("pick_type", PICK_TEXTURE),
            allow_no_texture("allow_no_texture", false),
            can_apply_immediately("can_apply_immediately"),
            no_commit_on_selection("no_commit_on_selection", false),
            label_width("label_width", -1),
            border_color("border_color"),
            fallback_image("fallback_image"),
            multiselect_text("multiselect_text"),
            caption_text("caption_text"),
            border("border")
        {}
    };
protected:
    LLTextureCtrl(const Params&);
    friend class LLUICtrlFactory;
public:
    virtual ~LLTextureCtrl();

    // LLView interface

    bool handleMouseDown(S32 x, S32 y, MASK mask) override;
    bool handleDragAndDrop(S32 x, S32 y, MASK mask,
        bool drop, EDragAndDropType cargo_type, void *cargo_data,
        EAcceptance *accept,
        std::string& tooltip_msg) override;
    bool handleHover(S32 x, S32 y, MASK mask) override;
    bool handleUnicodeCharHere(llwchar uni_char) override;

    void draw() override;
    void setVisible( bool visible ) override;
    void setEnabled( bool enabled ) override;

    void onVisibilityChange(bool new_visibility) override;

    void setValid(bool valid);

    // LLUICtrl interface
    void clear() override;

    // Takes a UUID, wraps get/setImageAssetID
    void setValue(const LLSD& value) override;
    LLSD getValue() const override;

    // LLTextureCtrl interface
    void            showPicker(bool take_focus);
    bool            isPickerShown() { return !mFloaterHandle.isDead(); }
    void            setLabel(const std::string& label);
    void            setLabelWidth(S32 label_width) {mLabelWidth =label_width;}
    const std::string&  getLabel() const                            { return mLabel; }

    void            setAllowNoTexture( bool b )                 { mAllowNoTexture = b; }
    bool            getAllowNoTexture() const                   { return mAllowNoTexture; }

    void            setAllowLocalTexture(bool b)                    { mAllowLocalTexture = b; }
    bool            getAllowLocalTexture() const                    { return mAllowLocalTexture; }

    const LLUUID&   getImageItemID() { return mImageItemID; }

    virtual void    setImageAssetName(const std::string& name);

    void            setImageAssetID(const LLUUID &image_asset_id);
    const LLUUID&   getImageAssetID() const                     { return mImageAssetID; }

    void            setDefaultImageAssetID( const LLUUID& id )  { mDefaultImageAssetID = id; }
    const LLUUID&   getDefaultImageAssetID() const { return mDefaultImageAssetID; }

    const std::string&  getDefaultImageName() const                 { return mDefaultImageName; }

    void            setBlankImageAssetID( const LLUUID& id )    { mBlankImageAssetID = id; }
    const LLUUID&   getBlankImageAssetID() const { return mBlankImageAssetID; }

    void            setOpenTexPreview(bool open_preview) { mOpenTexPreview = open_preview; }

    void            setCaption(const std::string& caption);
    void            setCanApplyImmediately(bool b);

    void            setCanApply(bool can_preview, bool can_apply);

    void            setImmediateFilterPermMask(PermissionMask mask);
    void            setDnDFilterPermMask(PermissionMask mask)
                        { mDnDFilterPermMask = mask; }
    PermissionMask  getImmediateFilterPermMask() { return mImmediateFilterPermMask; }
    void setFilterPermissionMasks(PermissionMask mask);

    void            closeDependentFloater();

    void            onFloaterClose();
    void            onFloaterCommit(ETexturePickOp op,
                                    LLPickerSource source,
                                    const LLUUID& local_id,
                                    const LLUUID& inv_id,
                                    const LLUUID& tracking_id);

    // This call is returned when a drag is detected. Your callback
    // should return true if the drag is acceptable.
    void setDragCallback(drag_n_drop_callback cb)   { mDragCallback = cb; }

    // This callback is called when the drop happens. Return true if
    // the drop happened - resulting in an on commit callback, but not
    // necessariliy any other change.
    void setDropCallback(drag_n_drop_callback cb)   { mDropCallback = cb; }

    void setOnCancelCallback(commit_callback_t cb)  { mOnCancelCallback = cb; }
    void setOnCloseCallback(commit_callback_t cb)   { mOnCloseCallback = cb; }
    void setOnSelectCallback(commit_callback_t cb)  { mOnSelectCallback = cb; }

    /*
     * callback for changing texture selection in inventory list of texture floater
     */
    void setOnTextureSelectedCallback(texture_selected_callback cb);

    void setShowLoadingPlaceholder(bool showLoadingPlaceholder);

    LLViewerFetchedTexture* getTexture() { return mTexturep; }

    void setBakeTextureEnabled(bool enabled);
    bool getBakeTextureEnabled() const { return mBakeTextureEnabled; }

    void setInventoryPickType(EPickInventoryType type);
    EPickInventoryType getInventoryPickType() { return mInventoryPickType; };

    bool isImageLocal() { return mLocalTrackingID.notNull(); }
    LLUUID getLocalTrackingID() { return mLocalTrackingID; }

private:
    bool allowDrop(LLInventoryItem* item, EDragAndDropType cargo_type, std::string& tooltip_msg);
    bool doDrop(LLInventoryItem* item);

private:
    drag_n_drop_callback        mDragCallback;
    drag_n_drop_callback        mDropCallback;
    commit_callback_t           mOnCancelCallback;
    commit_callback_t           mOnSelectCallback;
    commit_callback_t           mOnCloseCallback;
    texture_selected_callback   mOnTextureSelectedCallback;
    LLPointer<LLViewerFetchedTexture> mTexturep;
    LLPointer<LLFetchedGLTFMaterial> mGLTFMaterial;
    LLPointer<LLViewerTexture> mGLTFPreview;
    LLUIColor                   mBorderColor;
    LLUUID                      mImageItemID;
    LLUUID                      mImageAssetID;
    LLUUID                      mDefaultImageAssetID;
    LLUUID                      mBlankImageAssetID;
    LLUUID                      mLocalTrackingID;
    LLUIImagePtr                mFallbackImage;
    std::string                 mDefaultImageName;
    LLHandle<LLFloater>         mFloaterHandle;
    LLTextBox*                  mTentativeLabel;
    LLTextBox*                  mCaption;
    std::string                 mLabel;
    bool                        mAllowNoTexture; // If true, the user can select "none" as an option
    bool                        mAllowLocalTexture;
    PermissionMask              mImmediateFilterPermMask;
    PermissionMask              mDnDFilterPermMask;
    bool                        mCanApplyImmediately;
    bool                        mCommitOnSelection;
    bool                        mNeedsRawImageData;
    LLViewBorder*               mBorder;
    bool                        mValid;
    bool                        mShowLoadingPlaceholder;
    std::string                 mLoadingPlaceholderString;
    S32                         mLabelWidth;
    bool                        mOpenTexPreview;
    bool                        mBakeTextureEnabled;
    EPickInventoryType mInventoryPickType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// LLFloaterTexturePicker
typedef boost::function<void(LLTextureCtrl::ETexturePickOp op, LLPickerSource source, const LLUUID& asset_id, const LLUUID& inventory_id, const LLUUID& tracking_id)> floater_commit_callback;
typedef boost::function<void()> floater_close_callback;
typedef boost::function<void(const LLUUID& asset_id)> set_image_asset_id_callback;
typedef boost::function<void(LLPointer<LLViewerTexture> texture)> set_on_update_image_stats_callback;

class LLFloaterTexturePicker : public LLFloater
{
public:
    LLFloaterTexturePicker(
        LLView* owner,
        LLUUID image_asset_id,
        LLUUID default_image_asset_id,
        LLUUID blank_image_asset_id,
        bool tentative,
        bool allow_no_texture,
        const std::string& label,
        PermissionMask immediate_filter_perm_mask,
        PermissionMask dnd_filter_perm_mask,
        bool can_apply_immediately,
        LLUIImagePtr fallback_image_name,
        EPickInventoryType pick_type);

    virtual ~LLFloaterTexturePicker();

    // LLView overrides
    /*virtual*/ bool    handleDragAndDrop(S32 x, S32 y, MASK mask,
        bool drop, EDragAndDropType cargo_type, void *cargo_data,
        EAcceptance *accept,
        std::string& tooltip_msg);
    /*virtual*/ void    draw();
    /*virtual*/ bool    handleKeyHere(KEY key, MASK mask);

    // LLFloater overrides
    /*virtual*/ bool    postBuild();
    /*virtual*/ void    onOpen(const LLSD& key);
    /*virtual*/ void    onClose(bool app_settings);

    // New functions
    void setImageID(const LLUUID& image_asset_id, bool set_selection = true);
    bool updateImageStats(); // true if within limits
    const LLUUID&   getAssetID() { return mImageAssetID; }
    const LLUUID&   findItemID(const LLUUID& asset_id, bool copyable_only, bool ignore_library = false);
    void            setCanApplyImmediately(bool b);

    void            setActive(bool active);

    LLView*         getOwner() const { return mOwner; }
    void            setOwner(LLView* owner) { mOwner = owner; }
    void            stopUsingPipette();

    void commitIfImmediateSet();
    void commitCallback(LLTextureCtrl::ETexturePickOp op);
    void commitCancel();

    void onFilterEdit(const std::string& search_string);

    void setCanApply(bool can_preview, bool can_apply, bool inworld_image = true);
    void setMinDimentionsLimits(S32 min_dim);
    void setTextureSelectedCallback(const texture_selected_callback& cb) { mTextureSelectedCallback = cb; }
    void setOnFloaterCloseCallback(const floater_close_callback& cb) { mOnFloaterCloseCallback = cb; }
    void setOnFloaterCommitCallback(const floater_commit_callback& cb) { mOnFloaterCommitCallback = cb; }
    void setSetImageAssetIDCallback(const set_image_asset_id_callback& cb) { mSetImageAssetIDCallback = cb; }
    void setOnUpdateImageStatsCallback(const set_on_update_image_stats_callback& cb) { mOnUpdateImageStatsCallback = cb; }
    const LLUUID& getDefaultImageAssetID() { return mDefaultImageAssetID; }
    const LLUUID& getBlankImageAssetID() { return mBlankImageAssetID; }

    static void     onBtnSetToDefault(void* userdata);
    static void     onBtnSelect(void* userdata);
    static void     onBtnCancel(void* userdata);
    void            onBtnPipette();
    //static void       onBtnRevert( void* userdata );
    static void     onBtnBlank(void* userdata);
    static void     onBtnNone(void* userdata);
    void            onSelectionChange(const std::deque<LLFolderViewItem*> &items, bool user_action);
    static void     onApplyImmediateCheck(LLUICtrl* ctrl, void* userdata);
    void            onTextureSelect(const LLTextureEntry& te);

    static void     onModeSelect(LLUICtrl* ctrl, void *userdata);
    static void     onBtnAdd(void* userdata);
    static void     onBtnRemove(void* userdata);
    static void     onBtnUpload(void* userdata);
    static void     onLocalScrollCommit(LLUICtrl* ctrl, void* userdata);

    static void     onBakeTextureSelect(LLUICtrl* ctrl, void *userdata);

    void            setLocalTextureEnabled(bool enabled);
    void            setBakeTextureEnabled(bool enabled);

    void setInventoryPickType(EPickInventoryType type);
    void setImmediateFilterPermMask(PermissionMask mask);

    static void     onPickerCallback(const std::vector<std::string>& filenames, LLHandle<LLFloater> handle);

protected:
    void changeMode();
    void refreshLocalList();
    void refreshInventoryFilter();
    void setImageIDFromItem(const LLInventoryItem* itemp, bool set_selection = true);

    LLPointer<LLViewerTexture> mTexturep;
    LLPointer<LLFetchedGLTFMaterial> mGLTFMaterial;
    LLPointer<LLViewerTexture> mGLTFPreview;
    LLView*             mOwner;

    LLUUID              mImageAssetID; // Currently selected texture
    LLUIImagePtr        mFallbackImage; // What to show if currently selected texture is null.
    LLUUID              mDefaultImageAssetID;
    LLUUID              mBlankImageAssetID;
    bool                mTentative;
    bool                mAllowNoTexture;
    LLUUID              mSpecialCurrentImageAssetID;  // Used when the asset id has no corresponding texture in the user's inventory.
    LLUUID              mOriginalImageAssetID;

    std::string         mLabel;

    LLTextBox*          mTentativeLabel;
    LLTextBox*          mResolutionLabel;
    LLTextBox*          mResolutionWarning;

    std::string         mPendingName;
    bool                mActive;

    LLFilterEditor*     mFilterEdit;
    LLInventoryPanel*   mInventoryPanel;
    PermissionMask      mImmediateFilterPermMask;
    PermissionMask      mDnDFilterPermMask;
    bool                mCanApplyImmediately;
    bool                mNoCopyTextureSelected;
    F32                 mContextConeOpacity;
    LLSaveFolderState   mSavedFolderState;
    bool                mSelectedItemPinned;

    LLComboBox*         mModeSelector;
    LLScrollListCtrl*   mLocalScrollCtrl;
    LLButton*           mDefaultBtn;
    LLButton*           mNoneBtn;
    LLButton*           mBlankBtn;
    LLButton*           mPipetteBtn;
    LLButton*           mSelectBtn;
    LLButton*           mCancelBtn;
    LLView*             mPreviewWidget = nullptr;

private:
    bool mCanApply;
    bool mCanPreview;
    bool mPreviewSettingChanged;
    bool mLimitsSet;
    S32 mMaxDim;
    S32 mMinDim;
    EPickInventoryType mInventoryPickType;
    LLPickerSource mSelectionSource;


    texture_selected_callback mTextureSelectedCallback;
    floater_close_callback mOnFloaterCloseCallback;
    floater_commit_callback mOnFloaterCommitCallback;
    set_image_asset_id_callback mSetImageAssetIDCallback;
    set_on_update_image_stats_callback mOnUpdateImageStatsCallback;

    bool mBakeTextureEnabled;

    static S32 sLastPickerMode;
};

#endif  // LL_LLTEXTURECTRL_H
