/**
 * @file lltooldraganddrop.h
 * @brief LLToolDragAndDrop class header file
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

#ifndef LL_TOOLDRAGANDDROP_H
#define LL_TOOLDRAGANDDROP_H

#include "lldictionary.h"
#include "lltool.h"
#include "llview.h"
#include "lluuid.h"
#include "llassetstorage.h"
#include "llpermissions.h"
#include "llwindow.h"
#include "llviewerinventory.h"

class LLToolDragAndDrop;
class LLViewerRegion;
class LLVOAvatar;
class LLPickInfo;

class LLToolDragAndDrop : public LLTool, public LLSingleton<LLToolDragAndDrop>
{
    LLSINGLETON(LLToolDragAndDrop);
public:
    typedef boost::signals2::signal<void ()> enddrag_signal_t;

    // overridden from LLTool
    virtual bool    handleMouseUp(S32 x, S32 y, MASK mask) override;
    virtual bool    handleHover(S32 x, S32 y, MASK mask) override;
    virtual bool    handleKey(KEY key, MASK mask) override;
    virtual bool    handleToolTip(S32 x, S32 y, MASK mask) override;
    virtual void    onMouseCaptureLost() override;
    virtual void    handleDeselect() override;

    void            setDragStart( S32 x, S32 y );           // In screen space
    bool            isOverThreshold( S32 x, S32 y );        // In screen space

    enum ESource
    {
        SOURCE_AGENT,
        SOURCE_WORLD,
        SOURCE_NOTECARD,
        SOURCE_LIBRARY,
        SOURCE_VIEWER,
        SOURCE_PEOPLE
    };

    void beginDrag(EDragAndDropType type,
                   const LLUUID& cargo_id,
                   ESource source,
                   const LLUUID& source_id = LLUUID::null,
                   const LLUUID& object_id = LLUUID::null);
    void beginMultiDrag(const std::vector<EDragAndDropType> types,
                        const uuid_vec_t& cargo_ids,
                        ESource source,
                        const LLUUID& source_id = LLUUID::null);
    void endDrag();
    ESource getSource() const { return mSource; }
    const LLUUID& getSourceID() const { return mSourceID; }
    const LLUUID& getObjectID() const { return mObjectID; }
    EAcceptance getLastAccept() { return mLastAccept; }

    boost::signals2::connection setEndDragCallback( const enddrag_signal_t::slot_type& cb ) { return mEndDragSignal.connect(cb); }

    void setCargoCount(U32 count) { mCargoCount = count; }
    void resetCargoCount() { mCargoCount = 0; }
    U32 getCargoCount() const { return (mCargoCount > 0) ? mCargoCount : static_cast<S32>(mCargoIDs.size()); }
    S32 getCargoIndex() const { return mCurItemIndex; }

    static S32 getOperationId() { return sOperationId; }

    // deal with permissions of object, etc. returns true if drop can
    // proceed, otherwise false.
    static bool handleDropMaterialProtections(LLViewerObject* hit_obj,
                         LLInventoryItem* item,
                         LLToolDragAndDrop::ESource source,
                         const LLUUID& src_id);

protected:
    enum EDropTarget
    {
        DT_NONE = 0,
        DT_SELF = 1,
        DT_AVATAR = 2,
        DT_OBJECT = 3,
        DT_LAND = 4,
        DT_COUNT = 5
    };

protected:
    // dragOrDrop3dImpl points to a member of LLToolDragAndDrop that
    // takes parameters (LLViewerObject* obj, S32 face, MASK, bool
    // drop) and returns a bool if drop is ok
    typedef EAcceptance (LLToolDragAndDrop::*dragOrDrop3dImpl)
        (LLViewerObject*, S32, MASK, bool);

    void dragOrDrop(S32 x, S32 y, MASK mask, bool drop,
                    EAcceptance* acceptance);
    void dragOrDrop3D(S32 x, S32 y, MASK mask, bool drop,
                      EAcceptance* acceptance);

    static void pickCallback(const LLPickInfo& pick_info);
    void pick(const LLPickInfo& pick_info);

protected:

    U32             mCargoCount;

    S32             mDragStartX;
    S32             mDragStartY;

    std::vector<EDragAndDropType> mCargoTypes;
    //void*         mCargoData;
    uuid_vec_t mCargoIDs;
    ESource mSource;
    LLUUID mSourceID;
    LLUUID mObjectID;

    static S32      sOperationId;

    LLVector3d      mLastCameraPos;
    LLVector3d      mLastHitPos;

    ECursorType     mCursor;
    EAcceptance     mLastAccept;
    bool            mDrop;
    S32             mCurItemIndex;
    std::string     mToolTipMsg;
    std::string     mCustomMsg;

    enddrag_signal_t    mEndDragSignal;

protected:
    // 3d drop functions. these call down into the static functions
    // named drop<ThingToDrop> if drop is true and permissions allow
    // that behavior.
    EAcceptance dad3dNULL(LLViewerObject*, S32, MASK, bool);
    EAcceptance dad3dRezObjectOnLand(LLViewerObject* obj, S32 face,
                                     MASK mask, bool drop);
    EAcceptance dad3dRezObjectOnObject(LLViewerObject* obj, S32 face,
                                       MASK mask, bool drop);
    EAcceptance dad3dRezCategoryOnObject(LLViewerObject* obj, S32 face,
                                         MASK mask, bool drop);
    EAcceptance dad3dRezScript(LLViewerObject* obj, S32 face,
                               MASK mask, bool drop);
    EAcceptance dad3dTextureObject(LLViewerObject* obj, S32 face,
                                   MASK mask, bool drop);
    EAcceptance dad3dMaterialObject(LLViewerObject* obj, S32 face,
        MASK mask, bool drop);
    EAcceptance dad3dMeshObject(LLViewerObject* obj, S32 face,
                                   MASK mask, bool drop);
//  EAcceptance dad3dTextureSelf(LLViewerObject* obj, S32 face,
//                               MASK mask, bool drop);
    EAcceptance dad3dWearItem(LLViewerObject* obj, S32 face,
                                 MASK mask, bool drop);
    EAcceptance dad3dWearCategory(LLViewerObject* obj, S32 face,
                                 MASK mask, bool drop);
    EAcceptance dad3dUpdateInventory(LLViewerObject* obj, S32 face,
                                     MASK mask, bool drop);
    EAcceptance dad3dUpdateInventoryCategory(LLViewerObject* obj,
                                             S32 face,
                                             MASK mask,
                                             bool drop);
    EAcceptance dad3dGiveInventoryObject(LLViewerObject* obj, S32 face,
                                   MASK mask, bool drop);
    EAcceptance dad3dGiveInventory(LLViewerObject* obj, S32 face,
                                   MASK mask, bool drop);
    EAcceptance dad3dGiveInventoryCategory(LLViewerObject* obj, S32 face,
                                           MASK mask, bool drop);
    EAcceptance dad3dRezFromObjectOnLand(LLViewerObject* obj, S32 face,
                                         MASK mask, bool drop);
    EAcceptance dad3dRezFromObjectOnObject(LLViewerObject* obj, S32 face,
                                           MASK mask, bool drop);
    EAcceptance dad3dRezAttachmentFromInv(LLViewerObject* obj, S32 face,
                                          MASK mask, bool drop);
    EAcceptance dad3dCategoryOnLand(LLViewerObject *obj, S32 face,
                                    MASK mask, bool drop);
    EAcceptance dad3dAssetOnLand(LLViewerObject *obj, S32 face,
                                 MASK mask, bool drop);
    EAcceptance dad3dActivateGesture(LLViewerObject *obj, S32 face,
                                 MASK mask, bool drop);

    // helper called by methods above to handle "application" of an item
    // to an object (texture applied to face, mesh applied to shape, etc.)
    EAcceptance dad3dApplyToObject(LLViewerObject* obj, S32 face, MASK mask, bool drop, EDragAndDropType cargo_type);


    // set the LLToolDragAndDrop's cursor based on the given acceptance
    ECursorType acceptanceToCursor( EAcceptance acceptance );

    // This method converts mCargoID to an inventory item or
    // folder. If no item or category is found, both pointers will be
    // returned NULL.
    LLInventoryObject* locateInventory(LLViewerInventoryItem*& item,
                                       LLViewerInventoryCategory*& cat);

    //LLInventoryObject* locateMultipleInventory(
    //  LLViewerInventoryCategory::cat_array_t& cats,
    //  LLViewerInventoryItem::item_array_t& items);

    void dropObject(LLViewerObject* raycast_target,
            bool bypass_sim_raycast,
            bool from_task_inventory,
            bool remove_from_inventory);

    // accessor that looks at permissions, copyability, and names of
    // inventory items to determine if a drop would be ok.
    static EAcceptance willObjectAcceptInventory(LLViewerObject* obj, LLInventoryItem* item, EDragAndDropType type = DAD_NONE);

public:
    // helper functions
    static bool isInventoryDropAcceptable(LLViewerObject* obj, LLInventoryItem* item) { return (ACCEPT_YES_COPY_SINGLE <= willObjectAcceptInventory(obj, item)); }

    bool dadUpdateInventory(LLViewerObject* obj, bool drop);
    bool dadUpdateInventoryCategory(LLViewerObject* obj, bool drop);

    // methods that act on the simulator state.
    static void dropScript(LLViewerObject* hit_obj,
                           LLInventoryItem* item,
                           bool active,
                           ESource source,
                           const LLUUID& src_id);
    static void dropTexture(LLViewerObject* hit_obj,
                            S32 hit_face,
                            LLInventoryItem* item,
                            ESource source,
                            const LLUUID& src_id,
                            bool all_faces,
                            bool replace_pbr,
                            S32 tex_channel = -1);
    static void dropTextureOneFace(LLViewerObject* hit_obj,
                                   S32 hit_face,
                                   LLInventoryItem* item,
                                   ESource source,
                                   const LLUUID& src_id,
                                   bool remove_pbr,
                                   S32 tex_channel = -1);
    static void dropTextureAllFaces(LLViewerObject* hit_obj,
                                    LLInventoryItem* item,
                                    ESource source,
                                    const LLUUID& src_id,
                                    bool remove_pbr);
    static void dropMaterial(LLViewerObject* hit_obj,
                             S32 hit_face,
                             LLInventoryItem* item,
                             ESource source,
                             const LLUUID& src_id,
                             bool all_faces);
    static void dropMaterialOneFace(LLViewerObject* hit_obj,
                                    S32 hit_face,
                                    LLInventoryItem* item,
                                    ESource source,
                                    const LLUUID& src_id);
    static void dropMaterialAllFaces(LLViewerObject* hit_obj,
                                    LLInventoryItem* item,
                                    ESource source,
                                    const LLUUID& src_id);
    static void dropMesh(LLViewerObject* hit_obj,
                         LLInventoryItem* item,
                         ESource source,
                         const LLUUID& src_id);

    //static void   dropTextureOneFaceAvatar(LLVOAvatar* avatar,S32 hit_face,
    //                                   LLInventoryItem* item)

    static void dropInventory(LLViewerObject* hit_obj,
                              LLInventoryItem* item,
                              ESource source,
                              const LLUUID& src_id);

    static bool handleGiveDragAndDrop(LLUUID agent, LLUUID session, bool drop,
                                      EDragAndDropType cargo_type,
                                      void* cargo_data,
                                      EAcceptance* accept,
                                      const LLSD& dest = LLSD());

    // Classes used for determining 3d drag and drop types.
private:
    struct DragAndDropEntry : public LLDictionaryEntry
    {
        DragAndDropEntry(dragOrDrop3dImpl f_none,
                         dragOrDrop3dImpl f_self,
                         dragOrDrop3dImpl f_avatar,
                         dragOrDrop3dImpl f_object,
                         dragOrDrop3dImpl f_land);
        dragOrDrop3dImpl mFunctions[DT_COUNT];
    };
    class LLDragAndDropDictionary : public LLSingleton<LLDragAndDropDictionary>,
                                    public LLDictionary<EDragAndDropType, DragAndDropEntry>
    {
        LLSINGLETON(LLDragAndDropDictionary);
    public:
        dragOrDrop3dImpl get(EDragAndDropType dad_type, EDropTarget drop_target);
    };
};

// utility functions
void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms);

#endif  // LL_TOOLDRAGANDDROP_H
