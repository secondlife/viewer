
/**
 * @file llviewerobject.h
 * @brief Description of LLViewerObject class, which is the base class for most objects in the viewer.
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

#ifndef LL_LLVIEWEROBJECT_H
#define LL_LLVIEWEROBJECT_H

#include <map>
#include <unordered_map>

#include "llassetstorage.h"
//#include "llhudicon.h"
#include "llinventory.h"
#include "llrefcount.h"
#include "llprimitive.h"
#include "lluuid.h"
#include "llvoinventorylistener.h"
#include "object_flags.h"
#include "llquaternion.h"
#include "v3dmath.h"
#include "v3math.h"
#include "llvertexbuffer.h"
#include "llbbox.h"
#include "llrigginginfo.h"
#include "llreflectionmap.h"

namespace LL
{
    namespace GLTF
    {
        class Asset;
    }
}

class LLAgent;          // TODO: Get rid of this.
class LLAudioSource;
class LLAudioSourceVO;
class LLColor4;
class LLControlAvatar;
class LLDataPacker;
class LLDataPackerBinaryBuffer;
class LLDrawable;
class LLHUDText;
class LLHost;
class LLMessageSystem;
class LLNameValue;
class LLPartSysData;
class LLPipeline;
class LLTextureEntry;
class LLVOAvatar;
class LLVOInventoryListener;
class LLViewerInventoryItem;
class LLViewerObject;
class LLViewerObjectMedia;
class LLViewerPartSourceScript;
class LLViewerRegion;
class LLViewerTexture;
class LLWorld;

class LLMeshCostData;

typedef enum e_object_update_type
{
    OUT_FULL,
    OUT_TERSE_IMPROVED,
    OUT_FULL_COMPRESSED,
    OUT_FULL_CACHED,
    OUT_UNKNOWN,
} EObjectUpdateType;


// callback typedef for inventory
typedef void (*inventory_callback)(LLViewerObject*,
                                   LLInventoryObject::object_list_t*,
                                   S32 serial_num,
                                   void*);

// for exporting textured materials from SL
struct LLMaterialExportInfo
{
public:
    LLMaterialExportInfo(S32 mat_index, S32 texture_index, LLColor4 color) :
      mMaterialIndex(mat_index), mTextureIndex(texture_index), mColor(color) {};

    S32         mMaterialIndex;
    S32         mTextureIndex;
    LLColor4    mColor;
};

struct PotentialReturnableObject
{
    LLBBox          box;
    LLViewerRegion* pRegion;
};

//============================================================================

class LLViewerObject
:   public LLPrimitive,
    public LLRefCount,
    public LLGLUpdate
{
protected:
    virtual ~LLViewerObject(); // use unref()

    // TomY: Provide for a list of extra parameter structures, mapped by structure name
    struct ExtraParameter
    {
        bool in_use;
        LLNetworkData *data;
    };
    std::unordered_map<U16, ExtraParameter*> mExtraParameterList;

public:
    typedef std::list<LLPointer<LLViewerObject> > child_list_t;
    typedef std::list<LLPointer<LLViewerObject> > vobj_list_t;

    typedef const child_list_t const_child_list_t;

    LLViewerObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp, bool is_global = false);

    virtual void markDead();                // Mark this object as dead, and clean up its references
    bool isDead() const                                 {return mDead;}
    bool isOrphaned() const                             { return mOrphaned; }
    bool isParticleSource() const;

    virtual LLVOAvatar* asAvatar();

    LLVOAvatar* getAvatarAncestor();

    static void initVOClasses();
    static void cleanupVOClasses();

    void            addNVPair(const std::string& data);
    bool            removeNVPair(const std::string& name);
    LLNameValue*    getNVPair(const std::string& name) const;           // null if no name value pair by that name

    // Object create and update functions
    virtual void    idleUpdate(LLAgent &agent, const F64 &time);

    // Types of media we can associate
    enum { MEDIA_NONE = 0, MEDIA_SET = 1 };

    // Return codes for processUpdateMessage
    enum {
        MEDIA_URL_REMOVED = 0x1,
        MEDIA_URL_ADDED = 0x2,
        MEDIA_URL_UPDATED = 0x4,
        MEDIA_FLAGS_CHANGED = 0x8,
        INVALID_UPDATE = 0x80000000
    };

    static  U32     extractSpatialExtents(LLDataPackerBinaryBuffer *dp, LLVector3& pos, LLVector3& scale, LLQuaternion& rot);
    virtual U32     processUpdateMessage(LLMessageSystem *mesgsys,
                                        void **user_data,
                                        U32 block_num,
                                        const EObjectUpdateType update_type,
                                        LLDataPacker *dp);


    virtual bool    isActive() const; // Whether this object needs to do an idleUpdate.
    bool            onActiveList() const                {return mOnActiveList;}
    void            setOnActiveList(bool on_active)     { mOnActiveList = on_active; }

    virtual bool    isAttachment() const { return false; }
    const std::string& getAttachmentItemName() const;

    virtual LLVOAvatar* getAvatar() const;  //get the avatar this object is attached to, or NULL if object is not an attachment

    bool hasRenderMaterialParams() const;
    void setHasRenderMaterialParams(bool has_params);

    const LLUUID& getRenderMaterialID(U8 te) const;

    // set the RenderMaterialID for the given TextureEntry
    // te - TextureEntry index to set, or -1 for all TEs
    // id - asset id of material asset
    // update_server - if true, will send updates to server and clear most overrides
    void setRenderMaterialID(S32 te, const LLUUID& id, bool update_server = true, bool local_origin = true);
    void setRenderMaterialIDs(const LLUUID& id);

    virtual bool    isHUDAttachment() const { return false; }
    virtual bool    isTempAttachment() const;

    virtual bool isHiglightedOrBeacon() const;

    virtual void    updateRadius() {};
    virtual F32     getVObjRadius() const; // default implemenation is mDrawable->getRadius()

    // for jointed and other parent-relative hacks
    LLViewerObject* getSubParent();
    const LLViewerObject* getSubParent() const;

    // Object visiblility and GPW functions
    virtual void setPixelAreaAndAngle(LLAgent &agent); // Override to generate accurate apparent angle and area

    virtual U32 getNumVertices() const;
    virtual U32 getNumIndices() const;
    S32 getNumFaces() const { return mNumFaces; }

    // Graphical stuff for objects - maybe broken out into render class later?
    virtual void updateTextures();
    virtual void faceMappingChanged() {}
    virtual void boostTexturePriority(bool boost_children = true);  // When you just want to boost priority of this object

    virtual LLDrawable* createDrawable(LLPipeline *pipeline);
    virtual bool        updateGeometry(LLDrawable *drawable);
    virtual void        updateGL();
    virtual void        updateFaceSize(S32 idx);
    virtual bool        updateLOD();
    virtual bool        setDrawableParent(LLDrawable* parentp);
    F32                 getRotTime() { return mRotTime; }
private:
    void                resetRotTime();
    void                setRenderMaterialIDs(const LLRenderMaterialParams* material_params, bool local_origin);
    void                rebuildMaterial();
public:
    void                resetRot();
    void                applyAngularVelocity(F32 dt);

    void setLineWidthForWindowSize(S32 window_width);

    static void increaseArrowLength();              // makes axis arrows for selections longer
    static void decreaseArrowLength();              // makes axis arrows for selections shorter

    // Accessor functions
    LLViewerRegion* getRegion() const               { return mRegionp; }

    bool isSelected() const                         { return mUserSelected; }
    // Check whole linkset
    bool isAnySelected() const;
    virtual void setSelected(bool sel);

    const LLUUID &getID() const                     { return mID; }
    U32 getLocalID() const                          { return mLocalID; }
    U32 getCRC() const                              { return mTotalCRC; }
    S32 getListIndex() const                        { return mListIndex; }
    void setListIndex(S32 idx)                      { mListIndex = idx; }

    virtual bool isFlexible() const                 { return false; }
    virtual bool isSculpted() const                 { return false; }
    virtual bool isMesh() const                     { return false; }
    virtual bool isRiggedMesh() const               { return false; }
    virtual bool hasLightTexture() const            { return false; }
    virtual bool isReflectionProbe() const          { return false; }
    virtual F32 getReflectionProbeAmbiance() const  { return 0.f; }
    virtual F32 getReflectionProbeNearClip() const  { return 0.f; }
    virtual bool getReflectionProbeIsBox() const    { return false; }
    virtual bool getReflectionProbeIsDynamic() const { return false; };
    virtual bool getReflectionProbeIsMirror() const { return false; };

    // This method returns true if the object is over land owned by
    // the agent, one of its groups, or it encroaches and
    // anti-encroachment is enabled
    bool isReturnable();

    void buildReturnablesForChildrenVO( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion );
    void constructAndAddReturnable( std::vector<PotentialReturnableObject>& returnables, LLViewerObject* pChild, LLViewerRegion* pTargetRegion );

    // This method returns true if the object crosses
    // any parcel bounds in the region.
    bool crossesParcelBounds();

    /*
    // This method will scan through this object, and then query the
    // selection manager to see if the local agent probably has the
    // ability to modify the object. Since this calls into the
    // selection manager, you should avoid calling this method from
    // there.
    bool isProbablyModifiable() const;
    */

    virtual bool setParent(LLViewerObject* parent);
    virtual void onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent);
    virtual void afterReparent();
    virtual void addChild(LLViewerObject *childp);
    virtual void removeChild(LLViewerObject *childp);
    const_child_list_t& getChildren() const {   return mChildList; }
    S32 numChildren() const { return static_cast<S32>(mChildList.size()); }
    void addThisAndAllChildren(std::vector<LLViewerObject*>& objects);
    void addThisAndNonJointChildren(std::vector<LLViewerObject*>& objects);
    bool isChild(LLViewerObject *childp) const;
    bool isSeat() const;


    //detect if given line segment (in agent space) intersects with this viewer object.
    //returns true if intersection detected and returns information about intersection
    virtual bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
                                      S32 face = -1,                          // which face to check, -1 = ALL_SIDES
                                      bool pick_transparent = false,
                                      bool pick_rigged = false,
                                      bool pick_unselectable = true,
                                      S32* face_hit = NULL,                   // which face was hit
                                      LLVector4a* intersection = NULL,         // return the intersection point
                                      LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
                                      LLVector4a* normal = NULL,               // return the surface normal at the intersection point
                                      LLVector4a* tangent = NULL             // return the surface tangent at the intersection point
        );

    virtual bool lineSegmentBoundingBox(const LLVector4a& start, const LLVector4a& end);

    virtual const LLVector3d getPositionGlobal() const;
    virtual const LLVector3 &getPositionRegion() const;
    virtual const LLVector3 getPositionEdit() const;
    virtual const LLVector3 &getPositionAgent() const;
    virtual const LLVector3 getRenderPosition() const;

    LLMatrix4a getAgentToGLTFAssetTransform() const;
    LLMatrix4a getGLTFAssetToAgentTransform() const;
    LLVector3 getGLTFNodePositionAgent(S32 node_index) const;
    LLMatrix4a getGLTFNodeTransformAgent(S32 node_index) const;
    void getGLTFNodeTransformAgent(S32 node_index, LLVector3* position, LLQuaternion* rotation, LLVector3* scale) const;

    // move the node at the given index by the given offset in agent space
    void moveGLTFNode(S32 node_index, const LLVector3& offset);

    // set the rotation in agent space of the given node
    void setGLTFNodeRotationAgent(S32 node_index, const LLQuaternion& rotation);

    virtual const LLVector3 getPivotPositionAgent() const; // Usually = to getPositionAgent, unless like flex objects it's not

    LLViewerObject* getRootEdit() const;

    const LLQuaternion getRotationRegion() const;
    const LLQuaternion getRotationEdit() const;
    const LLQuaternion getRenderRotation() const;
    virtual const LLMatrix4 getRenderMatrix() const;

    void setPosition(const LLVector3 &pos, bool damped = false);
    void setPositionGlobal(const LLVector3d &position, bool damped = false);
    void setPositionRegion(const LLVector3 &position, bool damped = false);
    void setPositionEdit(const LLVector3 &position, bool damped = false);
    void setPositionAgent(const LLVector3 &pos_agent, bool damped = false);
    void setPositionParent(const LLVector3 &pos_parent, bool damped = false);
    void setPositionAbsoluteGlobal( const LLVector3d &pos_global, bool damped = false );

    virtual const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const     { return xform->getWorldMatrix(); }

    inline void setRotation(const F32 x, const F32 y, const F32 z, bool damped = false);
    inline void setRotation(const LLQuaternion& quat, bool damped = false);

    /*virtual*/ void    setNumTEs(const U8 num_tes);
    /*virtual*/ void    setTE(const U8 te, const LLTextureEntry &texture_entry);
    void updateTEMaterialTextures(U8 te);
    /*virtual*/ S32     setTETexture(const U8 te, const LLUUID &uuid);
    /*virtual*/ S32     setTENormalMap(const U8 te, const LLUUID &uuid);
    /*virtual*/ S32     setTESpecularMap(const U8 te, const LLUUID &uuid);
    S32                 setTETextureCore(const U8 te, LLViewerTexture *image);
    S32 setTENormalMapCore(const U8 te, LLViewerTexture *image);
    S32 setTESpecularMapCore(const U8 te, LLViewerTexture *image);
    /*virtual*/ S32     setTEColor(const U8 te, const LLColor3 &color);
    /*virtual*/ S32     setTEColor(const U8 te, const LLColor4 &color);
    /*virtual*/ S32     setTEScale(const U8 te, const F32 s, const F32 t);
    /*virtual*/ S32     setTEScaleS(const U8 te, const F32 s);
    /*virtual*/ S32     setTEScaleT(const U8 te, const F32 t);
    /*virtual*/ S32     setTEOffset(const U8 te, const F32 s, const F32 t);
    /*virtual*/ S32     setTEOffsetS(const U8 te, const F32 s);
    /*virtual*/ S32     setTEOffsetT(const U8 te, const F32 t);
    /*virtual*/ S32     setTERotation(const U8 te, const F32 r);
    /*virtual*/ S32     setTEBumpmap(const U8 te, const U8 bump );
    /*virtual*/ S32     setTETexGen(const U8 te, const U8 texgen );
    /*virtual*/ S32     setTEMediaTexGen(const U8 te, const U8 media ); // *FIXME: this confusingly acts upon a superset of setTETexGen's flags without absorbing its semantics
    /*virtual*/ S32     setTEShiny(const U8 te, const U8 shiny );
    /*virtual*/ S32     setTEFullbright(const U8 te, const U8 fullbright );
    /*virtual*/ S32     setTEMediaFlags(const U8 te, const U8 media_flags );
    /*virtual*/ S32     setTEGlow(const U8 te, const F32 glow);
    /*virtual*/ S32     setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID);
    /*virtual*/ S32     setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams);
    S32 initRenderMaterial(const U8 te);
    virtual     S32     setTEGLTFMaterialOverride(U8 te, LLGLTFMaterial* mat);

    // Used by Materials update functions to properly kick off rebuilds
    // of VBs etc when materials updates require changes.
    //
    void refreshMaterials();

    /*virtual*/ bool    setMaterial(const U8 material);
    virtual     void    setTEImage(const U8 te, LLViewerTexture *imagep); // Not derived from LLPrimitive
    virtual     void    changeTEImage(S32 index, LLViewerTexture* new_image)  ;
    virtual     void    changeTENormalMap(S32 index, LLViewerTexture* new_image)  ;
    virtual     void    changeTESpecularMap(S32 index, LLViewerTexture* new_image)  ;
    LLViewerTexture     *getTEImage(const U8 te) const;
    LLViewerTexture     *getTENormalMap(const U8 te) const;
    LLViewerTexture     *getTESpecularMap(const U8 te) const;

    bool                        isImageAlphaBlended(const U8 te) const;

    void fitFaceTexture(const U8 face);
    void sendTEUpdate() const;          // Sends packed representation of all texture entry information

    virtual void setScale(const LLVector3 &scale, bool damped = false);

    S32 getAnimatedObjectMaxTris() const;
    F32 recursiveGetEstTrianglesMax() const;
    virtual F32 getEstTrianglesMax() const;
    virtual F32 getEstTrianglesStreamingCost() const;
    virtual F32 getStreamingCost() const;
    virtual bool getCostData(LLMeshCostData& costs) const;
    virtual U32 getTriangleCount(S32* vcount = NULL) const;
    virtual U32 getHighLODTriangleCount();
    F32 recursiveGetScaledSurfaceArea() const;

    U32 recursiveGetTriangleCount(S32* vcount = NULL) const;

    void setObjectCost(F32 cost);
    F32 getObjectCost();

    void setLinksetCost(F32 cost);
    F32 getLinksetCost();

    void setPhysicsCost(F32 cost);
    F32 getPhysicsCost();

    void setLinksetPhysicsCost(F32 cost);
    F32 getLinksetPhysicsCost();

    void sendShapeUpdate();

    U8 getAttachmentState()                         { return mAttachmentState; }

    F32 getAppAngle() const                 { return mAppAngle; }
    F32 getPixelArea() const                { return mPixelArea; }
    void setPixelArea(F32 area)             { mPixelArea = area; }
    F32 getMaxScale() const;
    F32 getMidScale() const;
    F32 getMinScale() const;

    // Owner id is this object's owner
    void setAttachedSound(const LLUUID &audio_uuid, const LLUUID& owner_id, const F32 gain, const U8 flags);
    void adjustAudioGain(const F32 gain);
    F32  getSoundCutOffRadius() const { return mSoundCutOffRadius; }
    void clearAttachedSound()                               { mAudioSourcep = NULL; }

     // Create if necessary
    LLAudioSource *getAudioSource(const LLUUID& owner_id);
    bool isAudioSource() const {return mAudioSourcep != NULL;}

    U8 getMediaType() const;
    void setMediaType(U8 media_type);

    std::string getMediaURL() const;
    void setMediaURL(const std::string& media_url);

    bool getMediaPassedWhitelist() const;
    void setMediaPassedWhitelist(bool passed);

    void sendMaterialUpdate() const;

    void setDebugText(const std::string &utf8text, const LLColor4& color = LLColor4::white);
    void appendDebugText(const std::string &utf8text);
    void initHudText();
    void restoreHudText();
    void setIcon(LLViewerTexture* icon_image);
    void clearIcon();

    void recursiveMarkForUpdate();
    virtual void markForUpdate();
    void updateVolume(const LLVolumeParams& volume_params);
    virtual void updateSpatialExtents(LLVector4a& min, LLVector4a& max);
    virtual F32 getBinRadius();

    LLBBox              getBoundingBoxAgent() const;

    void updatePositionCaches() const; // Update the global and region position caches from the object (and parent's) xform.
    void updateText(); // update text label position
    virtual void updateDrawable(bool force_damped); // force updates on static objects

    bool isOwnerInMuteList(LLUUID item_id = LLUUID());

    void setDrawableState(U32 state, bool recursive = true);
    void clearDrawableState(U32 state, bool recursive = true);
    bool isDrawableState(U32 state, bool recursive = true) const;

    // Called when the drawable shifts
    virtual void onShift(const LLVector4a &shift_vector)    { }

    //////////////////////////////////////
    //
    // Inventory methods
    //

    // This function is called when someone is interested in a viewer
    // object's inventory. The callback is called as soon as the
    // viewer object has the inventory stored locally.
    void registerInventoryListener(LLVOInventoryListener* listener, void* user_data);
    void removeInventoryListener(LLVOInventoryListener* listener);
    bool isInventoryPending();
    void clearInventoryListeners();
    bool hasInventoryListeners();
    void requestInventory();
    static void processTaskInv(LLMessageSystem* msg, void** user_data);
    void removeInventory(const LLUUID& item_id);

    // The updateInventory() call potentially calls into the selection
    // manager, so do no call updateInventory() from the selection
    // manager until we have better iterators.
    void updateInventory(LLViewerInventoryItem* item, U8 key, bool is_new);
    void updateInventoryLocal(LLInventoryItem* item, U8 key); // Update without messaging.
    void updateMaterialInventory(LLViewerInventoryItem* item, U8 key, bool is_new);
    LLInventoryObject* getInventoryObject(const LLUUID& item_id);
    LLInventoryItem* getInventoryItem(const LLUUID& item_id);

    // Get content except for root category
    void getInventoryContents(LLInventoryObject::object_list_t& objects);
    LLInventoryObject* getInventoryRoot();
    LLViewerInventoryItem* getInventoryItemByAsset(const LLUUID& asset_id);
    LLViewerInventoryItem* getInventoryItemByAsset(const LLUUID& asset_id, LLAssetType::EType type);
    S16 getInventorySerial() const { return mInventorySerialNum; }

    // These functions does viewer-side only object inventory modifications
    void updateViewerInventoryAsset(
        const LLViewerInventoryItem* item,
        const LLUUID& new_asset);

    // This function will make sure that we refresh the inventory.
    void dirtyInventory();
    bool isInventoryDirty() { return mInventoryDirty; }

    // save a script, which involves removing the old one, and rezzing
    // in the new one. This method should be called with the asset id
    // of the new and old script AFTER the bytecode has been saved.
    void saveScript(const LLViewerInventoryItem* item, bool active, bool is_new);

    // move an inventory item out of the task and into agent
    // inventory. This operation is based on messaging. No permissions
    // checks are made on the viewer - the server will double check.
    void moveInventory(const LLUUID& agent_folder, const LLUUID& item_id);

    // Find the number of instances of this object's inventory that are of the given type
    S32 countInventoryContents( LLAssetType::EType type );

    bool            permAnyOwner() const;
    bool            permYouOwner() const;
    bool            permGroupOwner() const;
    bool            permOwnerModify() const;
    bool            permModify() const;
    bool            permCopy() const;
    bool            permMove() const;
    bool            permTransfer() const;
    inline bool     flagUsePhysics() const          { return ((mFlags & FLAGS_USE_PHYSICS) != 0); }
    inline bool     flagObjectAnyOwner() const      { return ((mFlags & FLAGS_OBJECT_ANY_OWNER) != 0); }
    inline bool     flagObjectYouOwner() const      { return ((mFlags & FLAGS_OBJECT_YOU_OWNER) != 0); }
    inline bool     flagObjectGroupOwned() const    { return ((mFlags & FLAGS_OBJECT_GROUP_OWNED) != 0); }
    inline bool     flagObjectOwnerModify() const   { return ((mFlags & FLAGS_OBJECT_OWNER_MODIFY) != 0); }
    inline bool     flagObjectModify() const        { return ((mFlags & FLAGS_OBJECT_MODIFY) != 0); }
    inline bool     flagObjectCopy() const          { return ((mFlags & FLAGS_OBJECT_COPY) != 0); }
    inline bool     flagObjectMove() const          { return ((mFlags & FLAGS_OBJECT_MOVE) != 0); }
    inline bool     flagObjectTransfer() const      { return ((mFlags & FLAGS_OBJECT_TRANSFER) != 0); }
    inline bool     flagObjectPermanent() const     { return ((mFlags & FLAGS_AFFECTS_NAVMESH) != 0); }
    inline bool     flagCharacter() const           { return ((mFlags & FLAGS_CHARACTER) != 0); }
    inline bool     flagVolumeDetect() const        { return ((mFlags & FLAGS_VOLUME_DETECT) != 0); }
    inline bool     flagIncludeInSearch() const     { return ((mFlags & FLAGS_INCLUDE_IN_SEARCH) != 0); }
    inline bool     flagScripted() const            { return ((mFlags & FLAGS_SCRIPTED) != 0); }
    inline bool     flagHandleTouch() const         { return ((mFlags & FLAGS_HANDLE_TOUCH) != 0); }
    inline bool     flagTakesMoney() const          { return ((mFlags & FLAGS_TAKES_MONEY) != 0); }
    inline bool     flagPhantom() const             { return ((mFlags & FLAGS_PHANTOM) != 0); }
    inline bool     flagInventoryEmpty() const      { return ((mFlags & FLAGS_INVENTORY_EMPTY) != 0); }
    inline bool     flagAllowInventoryAdd() const   { return ((mFlags & FLAGS_ALLOW_INVENTORY_DROP) != 0); }
    inline bool     flagTemporaryOnRez() const      { return ((mFlags & FLAGS_TEMPORARY_ON_REZ) != 0); }
    inline bool     flagAnimSource() const          { return ((mFlags & FLAGS_ANIM_SOURCE) != 0); }
    inline bool     flagCameraSource() const        { return ((mFlags & FLAGS_CAMERA_SOURCE) != 0); }
    inline bool     flagCameraDecoupled() const     { return ((mFlags & FLAGS_CAMERA_DECOUPLED) != 0); }

    U8       getPhysicsShapeType() const;
    inline F32      getPhysicsGravity() const       { return mPhysicsGravity; }
    inline F32      getPhysicsFriction() const      { return mPhysicsFriction; }
    inline F32      getPhysicsDensity() const       { return mPhysicsDensity; }
    inline F32      getPhysicsRestitution() const   { return mPhysicsRestitution; }

    bool            isPermanentEnforced() const;

    bool getIncludeInSearch() const;
    void setIncludeInSearch(bool include_in_search);

    // Does "open" object menu item apply?
    bool allowOpen() const;

    void setClickAction(U8 action) { mClickAction = action; }
    U8 getClickAction() const { return mClickAction; }
    bool specialHoverCursor() const;    // does it have a special hover cursor?

    void            setRegion(LLViewerRegion *regionp);
    virtual void    updateRegion(LLViewerRegion *regionp);

    void updateFlags(bool physics_changed = false);
    void loadFlags(U32 flags); //load flags from cache or from message
    bool setFlags(U32 flag, bool state);
    bool setFlagsWithoutUpdate(U32 flag, bool state);
    void setPhysicsShapeType(U8 type);
    void setPhysicsGravity(F32 gravity);
    void setPhysicsFriction(F32 friction);
    void setPhysicsDensity(F32 density);
    void setPhysicsRestitution(F32 restitution);

    virtual void dump() const;
    static U32      getNumZombieObjects()           { return sNumZombieObjects; }

    void printNameValuePairs() const;

    virtual S32 getLOD() const { return 3; }
    virtual U32 getPartitionType() const;
    void dirtySpatialGroup() const;
    virtual void dirtyMesh();

    virtual LLNetworkData* getParameterEntry(U16 param_type) const;
    virtual bool setParameterEntry(U16 param_type, const LLNetworkData& new_value, bool local_origin);
    virtual bool getParameterEntryInUse(U16 param_type) const;
    virtual bool setParameterEntryInUse(U16 param_type, bool in_use, bool local_origin);
    // Called when a parameter is changed
    virtual void parameterChanged(U16 param_type, bool local_origin);
    virtual void parameterChanged(U16 param_type, LLNetworkData* data, bool in_use, bool local_origin);

    bool isShrinkWrapped() const { return mShouldShrinkWrap; }

    // Used to improve performance.  If an object is likely to rebuild its vertex buffer often
    // as a side effect of some update (color update, scale, etc), setting this to true
    // will cause it to be pushed deeper into the octree and isolate it from other nodes
    // so that nearby objects won't attempt to share a vertex buffer with this object.
    void shrinkWrap();

    friend class LLViewerObjectList;
    friend class LLViewerMediaList;

public:
    LLViewerTexture* getBakedTextureForMagicId(const LLUUID& id);
    void updateAvatarMeshVisibility(const LLUUID& id, const LLUUID& old_id);
    void refreshBakeTexture();
public:
    static void unpackVector3(LLDataPackerBinaryBuffer* dp, LLVector3& value, std::string name);
    static void unpackUUID(LLDataPackerBinaryBuffer* dp, LLUUID& value, std::string name);
    static void unpackU32(LLDataPackerBinaryBuffer* dp, U32& value, std::string name);
    static void unpackU8(LLDataPackerBinaryBuffer* dp, U8& value, std::string name);
    static U32 unpackParentID(LLDataPackerBinaryBuffer* dp, U32& parent_id);

public:
    //counter-translation
    void resetChildrenPosition(const LLVector3& offset, bool simplified = false,  bool skip_avatar_child = false) ;
    //counter-rotation
    void resetChildrenRotationAndPosition(const std::vector<LLQuaternion>& rotations,
                                            const std::vector<LLVector3>& positions) ;
    void saveUnselectedChildrenRotation(std::vector<LLQuaternion>& rotations) ;
    void saveUnselectedChildrenPosition(std::vector<LLVector3>& positions) ;
    std::vector<LLVector3> mUnselectedChildrenPositions ;

private:
    void setObjectCostStale();
    bool isAssetInInventory(LLViewerInventoryItem* item, LLAssetType::EType type);

    ExtraParameter* createNewParameterEntry(U16 param_type);
    ExtraParameter* getExtraParameterEntry(U16 param_type) const;
    ExtraParameter* getExtraParameterEntryCreate(U16 param_type);
    bool unpackParameterEntry(U16 param_type, LLDataPacker *dp);

    // This function checks to see if the given media URL has changed its version
    // and the update wasn't due to this agent's last action.
    U32 checkMediaURL(const std::string &media_url);

    // Motion prediction between updates
    void interpolateLinearMotion(const F64SecondsImplicit & frame_time, const F32SecondsImplicit & dt);

    static void initObjectDataMap();

    // forms task inventory request if none are pending, marks request as pending
    void fetchInventoryFromServer();

    // forms task inventory request after some time passed, marks request as pending
    void fetchInventoryDelayed(const F64 &time_seconds);
    static void fetchInventoryDelayedCoro(const LLUUID task_inv, const F64 time_seconds);
    static void fetchInventoryFromCapCoro(const LLUUID task_inv);

public:
    //
    // Viewer-side only types - use the LL_PCODE_APP mask.
    //
    typedef enum e_vo_types
    {
        LL_VO_CLOUDS =              LL_PCODE_APP | 0x20, // no longer used
        LL_VO_SURFACE_PATCH =       LL_PCODE_APP | 0x30,
        LL_VO_WL_SKY =              LL_PCODE_APP | 0x40,
        LL_VO_SQUARE_TORUS =        LL_PCODE_APP | 0x50,
        LL_VO_SKY =                 LL_PCODE_APP | 0x60,
        LL_VO_VOID_WATER =          LL_PCODE_APP | 0x70,
        LL_VO_WATER =               LL_PCODE_APP | 0x80,
        LL_VO_PART_GROUP =          LL_PCODE_APP | 0xa0,
        LL_VO_TRIANGLE_TORUS =      LL_PCODE_APP | 0xb0,
        LL_VO_HUD_PART_GROUP =      LL_PCODE_APP | 0xc0,
    } EVOType;

    typedef enum e_physics_shape_types
    {
        PHYSICS_SHAPE_PRIM = 0,
        PHYSICS_SHAPE_NONE,
        PHYSICS_SHAPE_CONVEX_HULL,
    } EPhysicsShapeType;

    LLUUID          mID;
    LLUUID          mOwnerID; //null if unknown

    // unique within region, not unique across regions
    // Local ID = 0 is not used
    U32             mLocalID;

    // Last total CRC received from sim, used for caching
    U32             mTotalCRC;

    // index into LLViewerObjectList::mActiveObjects or -1 if not in list
    S32             mListIndex;

    LLPointer<LLViewerTexture> *mTEImages;
    LLPointer<LLViewerTexture> *mTENormalMaps;
    LLPointer<LLViewerTexture> *mTESpecularMaps;

    // true if user can select this object by clicking under any circumstances (even if pick_unselectable is true)
    // can likely be factored out
    bool            mbCanSelect;

private:
    // Grabbed from UPDATE_FLAGS
    U32             mFlags;

    static std::map<std::string, U32> sObjectDataMap;
public:
    // Sent to sim in UPDATE_FLAGS, received in ObjectPhysicsProperties
    U8              mPhysicsShapeType;
    F32             mPhysicsGravity;
    F32             mPhysicsFriction;
    F32             mPhysicsDensity;
    F32             mPhysicsRestitution;

    // set the GLTF asset for this LLViewerObject  to the specified asset id
    // id MUST be for a GLTF asset (LLAssetType::AT_GLTF)
    // will relesae any currently held references to a GLTF asset on id change
    void setGLTFAsset(const LLUUID& id);

    // Associated GLTF Asset
    std::shared_ptr<LL::GLTF::Asset> mGLTFAsset;

    // Pipeline classes
    LLPointer<LLDrawable> mDrawable;

    // Band-aid to select object after all creation initialization is done
    bool mCreateSelected;

    // Replace textures with web pages on this object while drawing
    bool mRenderMedia;

    bool mRiggedAttachedWarned;

    // In bits
    S32             mBestUpdatePrecision;

    // TODO: Make all this stuff private.  JC
    LLPointer<LLHUDText> mText;
    LLPointer<class LLHUDIcon> mIcon;

    std::string mHudText;
    LLColor4 mHudTextColor;

    static          bool        sUseSharedDrawables;

public:
    // Returns mControlAvatar for the edit root prim of this linkset
    LLControlAvatar *getControlAvatar();
    LLControlAvatar *getControlAvatar() const;

    // Create or connect to an existing control av as applicable
    void linkControlAvatar();
    // Remove any reference to control av for this prim
    void unlinkControlAvatar();
    // Link or unlink as needed
    void updateControlAvatar();

    virtual bool isAnimatedObject() const;

    // Flags for createObject
    static const S32 CO_FLAG_CONTROL_AVATAR = 1 << 0;
    static const S32 CO_FLAG_UI_AVATAR = 1 << 1;

protected:
    LLPointer<LLControlAvatar> mControlAvatar;

protected:
    // delete an item in the inventory, but don't tell the
    // server. This is used internally by remove, update, and
    // savescript.
    void deleteInventoryItem(const LLUUID& item_id);

    // do the update/caching logic. called by saveScript and
    // updateInventory.
    void doUpdateInventory(LLPointer<LLViewerInventoryItem>& item, U8 key, bool is_new);

    static LLViewerObject *createObject(const LLUUID &id, LLPCode pcode, LLViewerRegion *regionp, S32 flags = 0);

    // Hide or show HUD, icon and particles
    void    hideExtraDisplayItems( bool hidden );

    //////////////////////////
    //
    // inventory functionality
    //

    static void processTaskInvFile(void** user_data, S32 error_code, LLExtStat ext_status);
    bool loadTaskInvFile(const std::string& filename);
    void loadTaskInvLLSD(const LLSD &inv_result);
    void doInventoryCallback();

    bool isOnMap();

    void unpackParticleSource(const S32 block_num, const LLUUID& owner_id);
    void unpackParticleSource(LLDataPacker &dp, const LLUUID& owner_id, bool legacy);
    void deleteParticleSource();
    void setParticleSource(const LLPartSysData& particle_parameters, const LLUUID& owner_id);

private:
    void setNameValueList(const std::string& list);     // clears nv pairs and then individually adds \n separated NV pairs from \0 terminated string
    void deleteTEImages(); // correctly deletes list of images

protected:

    typedef std::map<char *, LLNameValue *> name_value_map_t;
    name_value_map_t mNameValuePairs;   // Any name-value pairs stored by script

    child_list_t    mChildList;

    F64Seconds      mLastInterpUpdateSecs;          // Last update for purposes of interpolation
    F64Seconds      mLastMessageUpdateSecs;         // Last update from a message from the simulator
    TPACKETID       mLatestRecvPacketID;            // Latest time stamp on message from simulator
    F64SecondsImplicit mRegionCrossExpire;      // frame time we detected region crossing in + wait time

    // extra data sent from the sim...currently only used for tree species info
    U8* mData;

    LLPointer<LLViewerPartSourceScript>     mPartSourcep;   // Particle source associated with this object.
    LLAudioSourceVO* mAudioSourcep;
    F32             mAudioGain;
    F32             mSoundCutOffRadius;

    F32             mAppAngle;  // Apparent visual arc in degrees
    F32             mPixelArea; // Apparent area in pixels

    // IDs of of all items in the object's content which are added to the object's content,
    // but not updated on the server yet. After item was updated, its ID will be removed from this list.
    std::list<LLUUID> mPendingInventoryItemsIDs;

    // This is the object's inventory from the viewer's perspective.
    LLInventoryObject::object_list_t* mInventory;
    class LLInventoryCallbackInfo
    {
    public:
        ~LLInventoryCallbackInfo();
        LLVOInventoryListener* mListener;
        void* mInventoryData;
    };
    typedef std::list<LLInventoryCallbackInfo*> callback_list_t;
    callback_list_t mInventoryCallbacks;
    S16 mInventorySerialNum;
    S16 mExpectedInventorySerialNum;

    enum EInventoryRequestState
    {
        INVENTORY_REQUEST_STOPPED,
        INVENTORY_REQUEST_WAIT,    // delay before requesting
        INVENTORY_REQUEST_PENDING, // just did fetchInventoryFromServer()
        INVENTORY_XFER             // processed response from 'fetch', now doing an xfer
    };
    EInventoryRequestState  mInvRequestState;
    U64                     mInvRequestXFerId;
    bool                    mInventoryDirty;

    LLViewerRegion  *mRegionp;                  // Region that this object belongs to.
    bool            mDead;
    bool            mOrphaned;                  // This is an orphaned child
    bool            mUserSelected;              // Cached user select information
    bool            mOnActiveList;
    bool            mOnMap;                     // On the map.
    bool            mStatic;                    // Object doesn't move.
    S32             mSeatCount;
    S32             mNumFaces;

    F32             mRotTime;                   // Amount (in seconds) that object has rotated according to angular velocity (llSetTargetOmega)
    LLQuaternion    mAngularVelocityRot;        // accumulated rotation from the angular velocity computations
    LLQuaternion    mPreviousRotation;

    U8              mAttachmentState;   // this encodes the attachment id in a somewhat complex way. 0 if not an attachment.
    LLViewerObjectMedia* mMedia;    // NULL if no media associated
    U8 mClickAction;
    F32 mObjectCost; //resource cost of this object or -1 if unknown
    F32 mLinksetCost;
    F32 mPhysicsCost;
    F32 mLinksetPhysicsCost;

    // If true, "shrink wrap" this volume in its spatial partition.  See "shrinkWrap"
    bool mShouldShrinkWrap = false;

    bool mCostStale;
    mutable bool mPhysicsShapeUnknown;

    static          U32         sNumZombieObjects;          // Objects which are dead, but not deleted

    static          bool        sMapDebug;                  // Map render mode
    static          LLColor4    sEditSelectColor;
    static          LLColor4    sNoEditSelectColor;
    static          F32         sCurrentPulse;
    static          bool        sPulseEnabled;

    static          S32         sAxisArrowLength;


    // These two caches are only correct for non-parented objects right now!
    mutable LLVector3       mPositionRegion;
    mutable LLVector3       mPositionAgent;

    static void setPhaseOutUpdateInterpolationTime(F32 value)   { sPhaseOutUpdateInterpolationTime = (F64Seconds) value;    }
    static void setMaxUpdateInterpolationTime(F32 value)        { sMaxUpdateInterpolationTime = (F64Seconds) value; }
    static void setMaxRegionCrossingInterpolationTime(F32 value)        { sMaxRegionCrossingInterpolationTime = (F64Seconds) value; }

    static void setVelocityInterpolate(bool value)      { sVelocityInterpolate = value; }
    static void setPingInterpolate(bool value)          { sPingInterpolate = value; }

private:
    static S32 sNumObjects;

    static F64Seconds sPhaseOutUpdateInterpolationTime; // For motion interpolation
    static F64Seconds sMaxUpdateInterpolationTime;          // For motion interpolation
    static F64Seconds sMaxRegionCrossingInterpolationTime;          // For motion interpolation

    static bool sVelocityInterpolate;
    static bool sPingInterpolate;

    bool mCachedOwnerInMuteList;
    F64 mCachedMuteListUpdateTime;

    //--------------------------------------------------------------------
    // For objects that are attachments
    //--------------------------------------------------------------------
public:
    const LLUUID &getAttachmentItemID() const;
    void setAttachmentItemID(const LLUUID &id);
    const LLUUID &extractAttachmentItemID(); // find&set the inventory item ID of the attached object
    EObjectUpdateType getLastUpdateType() const;
    void setLastUpdateType(EObjectUpdateType last_update_type);
    bool getLastUpdateCached() const;
    void setLastUpdateCached(bool last_update_cached);

    virtual void updateRiggingInfo() {}

    LLJointRiggingInfoTab mJointRiggingInfoTab;

private:
    LLUUID mAttachmentItemID; // ItemID of the associated object is in user inventory.
    EObjectUpdateType   mLastUpdateType;
    bool    mLastUpdateCached;

public:
    // reflection probe state
    bool mIsReflectionProbe = false;  // if true, this object should register itself with LLReflectionProbeManager
    LLPointer<LLReflectionMap> mReflectionProbe = nullptr; // reflection probe coupled to this viewer object.  If not null, should be deregistered when this object is destroyed
    bool mIsHeroProbe = false; // This is a special case for mirrors and other high resolution probes.

    // the amount of GPU time (in ms) it took to render this object according to LLPipeline::profileAvatar
    // -1.f if no profile data available
    F32 mGPURenderTime = -1.f;
};

///////////////////
//
// Inlines
//
//

inline void LLViewerObject::setRotation(const LLQuaternion& quat, bool damped)
{
    LLPrimitive::setRotation(quat);
    setChanged(ROTATED | SILHOUETTE);
    updateDrawable(damped);
}

inline void LLViewerObject::setRotation(const F32 x, const F32 y, const F32 z, bool damped)
{
    LLPrimitive::setRotation(x, y, z);
    setChanged(ROTATED | SILHOUETTE);
    updateDrawable(damped);
}

class LLViewerObjectMedia
{
public:
    LLViewerObjectMedia() : mMediaURL(), mPassedWhitelist(false), mMediaType(0) { }

    std::string mMediaURL;  // for web pages on surfaces, one per prim
    bool mPassedWhitelist;  // user has OK'd display
    U8 mMediaType;          // see LLTextureEntry::WEB_PAGE, etc.
};

// subclass of viewer object that can be added to particle partitions
class LLAlphaObject : public LLViewerObject
{
public:
    LLAlphaObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
    : LLViewerObject(id,pcode,regionp)
    { mDepth = 0.f; }

    virtual F32 getPartSize(S32 idx);
    virtual void getGeometry(S32 idx,
                                LLStrider<LLVector4a>& verticesp,
                                LLStrider<LLVector3>& normalsp,
                                LLStrider<LLVector2>& texcoordsp,
                                LLStrider<LLColor4U>& colorsp,
                                LLStrider<LLColor4U>& emissivep,
                                LLStrider<U16>& indicesp) = 0;

    virtual void getBlendFunc(S32 face, LLRender::eBlendFactor& src, LLRender::eBlendFactor& dst);

    F32 mDepth;
};

class LLStaticViewerObject : public LLViewerObject
{
public:
    LLStaticViewerObject(const LLUUID& id, const LLPCode pcode, LLViewerRegion* regionp, bool is_global = false)
        : LLViewerObject(id,pcode,regionp, is_global)
    { }

    virtual void updateDrawable(bool force_damped);
};


#endif
