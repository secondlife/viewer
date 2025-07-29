/**
 * @file llviewerobjectlist.h
 * @brief Central manager for all viewer-side objects in Second Life's virtual world.
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

#ifndef LL_LLVIEWEROBJECTLIST_H
#define LL_LLVIEWEROBJECTLIST_H

#include <map>
#include <set>

// common includes
#include "llstring.h"
#include "lltrace.h"

// project includes
#include "llviewerobject.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLCamera;
class LLNetMap;
class LLDebugBeacon;
class LLVOCacheEntry;

const U32 CLOSE_BIN_SIZE = 10;  /// Size threshold for "close" objects (unused in current implementation)
const U32 NUM_BINS = 128;       /// Number of bins for lazy texture update cycling

/// Special GL names for non-object selections
const U32 GL_NAME_LAND = 0;          /// Reserved GL name for land/terrain
const U32 GL_NAME_PARCEL_WALL = 1;   /// Reserved GL name for parcel boundaries

/// Offset applied to object indices to create GL names, avoiding reserved values
const U32 GL_NAME_INDEX_OFFSET = 10;

/**
 * @brief Manages all viewer-side objects in the virtual world.
 * 
 * This is the master registry for everything you can see in Second Life - from
 * avatars to prims to particles. It serves as the central hub that tracks,
 * updates, and manages the lifecycle of all objects in your viewing area,
 * keeping the virtual world synchronized between the server and your viewer.
 * 
 * The class handles:
 * - Object creation, updates, and destruction based on server messages
 * - Tracking orphaned objects (children whose parents haven't loaded yet)
 * - Managing active objects that need frequent updates
 * - Caching and fetching object physics and cost data
 * - Providing fast lookups by UUID or local ID
 * 
 * Performance note: This class is on the hot path for frame updates, so many
 * operations are optimized for speed over memory usage.
 * 
 * @code
 * // Finding an object by UUID
 * LLViewerObject* avatar = gObjectList.findObject(avatar_uuid);
 * if (avatar && avatar->isAvatar()) {
 *     // Do something with the avatar
 * }
 * 
 * // Creating a new prim
 * LLViewerObject* prim = gObjectList.createObjectViewer(LL_PCODE_VOLUME, regionp);
 * @endcode
 */
class LLViewerObjectList
{
public:
    /**
     * @brief Constructs the object list manager.
     * 
     * Initializes all the internal tracking structures. There should only be one
     * instance of this class (gObjectList) for the entire viewer session.
     */
    LLViewerObjectList();
    
    /**
     * @brief Destroys the object list and cleans up all tracked objects.
     * 
     * This is only called on viewer shutdown. It ensures all objects are
     * properly destroyed and references are cleaned up.
     */
    ~LLViewerObjectList();

    /**
     * @brief Forcefully destroys all objects and clears all tracking structures.
     * 
     * This completely resets the object system by destroying all objects. Use this
     * when you need a clean slate, such as during a major disconnect or before
     * shutdown. After calling this, the object list is empty but still usable.
     */
    void destroy();

    /**
     * @brief Gets an object by its index in the internal array.
     * 
     * This is for internal use only - most code should use findObject() instead.
     * The index is NOT a local ID, it's the position in our mObjects array.
     * Dead objects will return NULL even if they're still in the array.
     * 
     * @param index Position in the mObjects array
     * @return The object at that index, or NULL if dead/invalid
     */
    inline LLViewerObject *getObject(const S32 index);

    /**
     * @brief Finds an object by its UUID.
     * 
     * This is the primary way to look up objects. It's fast (uses a hash map)
     * and reliable. Returns NULL for null UUIDs or objects that don't exist.
     * Note: This will also return NULL for offline avatars.
     * 
     * @param id The UUID of the object to find
     * @return The object if found, NULL otherwise
     * 
     * @code
     * if (LLViewerObject* obj = gObjectList.findObject(some_uuid)) {
     *     // Object exists and is valid
     * }
     * @endcode
     */
    inline LLViewerObject *findObject(const LLUUID &id);
    /**
     * @brief Creates a viewer-side object (not from server data).
     * 
     * Use this when you need to create an object locally, like UI attachments
     * or temporary visual effects. The object gets a generated UUID and is
     * added to all the tracking structures.
     * 
     * @param pcode The primitive code (LL_PCODE_VOLUME, LL_PCODE_AVATAR, etc.)
     * @param regionp The region this object belongs to
     * @param flags Creation flags (usually 0)
     * @return The newly created object, or NULL on failure
     */
    LLViewerObject *createObjectViewer(const LLPCode pcode, LLViewerRegion *regionp, S32 flags = 0);
    /**
     * @brief Creates an object from cached data.
     * 
     * This is used when loading objects from the region's object cache on startup
     * or region crossing. The object already has a UUID and local ID from when
     * it was previously in view.
     * 
     * @param pcode The primitive code
     * @param regionp The region this object belongs to
     * @param uuid The object's UUID from cache
     * @param local_id The object's local ID from cache
     * @return The newly created object, or NULL on failure
     */
    LLViewerObject *createObjectFromCache(const LLPCode pcode, LLViewerRegion *regionp, const LLUUID &uuid, const U32 local_id);
    /**
     * @brief Creates an object from server update data.
     * 
     * This is the main creation path for objects coming from the simulator.
     * It sets up all the ID mappings and adds the object to the region's
     * created list for further processing.
     * 
     * @param pcode The primitive code
     * @param regionp The region this object belongs to
     * @param uuid The object's UUID (can be null to generate one)
     * @param local_id The object's local ID from the server
     * @param sender The host that sent this update
     * @return The newly created object, or NULL on failure
     */
    LLViewerObject *createObject(const LLPCode pcode, LLViewerRegion *regionp,
                                 const LLUUID &uuid, const U32 local_id, const LLHost &sender);

    /**
     * @brief Replaces an existing object with a new one of a different type.
     * 
     * This is a workaround to handle type changes on the fly (such as when an
     * avatar sits on an object and needs to change type). The old object is
     * marked dead and a new one is created with the same UUID and local ID.
     * 
     * @param id The UUID of the object to replace
     * @param pcode The new primitive code
     * @param regionp The region for the new object
     * @return The replacement object, or NULL if original not found
     */
    LLViewerObject *replaceObject(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

    /**
     * @brief Marks an object for destruction.
     * 
     * This doesn't immediately destroy the object - it marks it as dead and
     * schedules it for cleanup. This is important because other parts of the
     * system might still have pointers to it. The actual cleanup happens in
     * cleanDeadObjects().
     * 
     * Special case: gAgentAvatarp is never killed unless the region is NULL
     * (which means we're logging out).
     * 
     * @param objectp The object to kill
     * @return true if the object was marked dead, false if it was protected
     */
    bool killObject(LLViewerObject *objectp);
    
    /**
     * @brief Kills all objects belonging to a specific region.
     * 
     * Used when a region is going away (disconnect, crossing boundaries, etc.).
     * This immediately calls cleanDeadObjects() because the region is becoming
     * invalid and we can't leave dead objects pointing to it.
     * 
     * @param regionp The region whose objects should be killed
     */
    void killObjects(LLViewerRegion *regionp);
    
    /**
     * @brief Kills every object in the system.
     * 
     * Only used during global destruction. This is the final cleanup that
     * ensures everything is properly destroyed before the viewer exits.
     */
    void killAllObjects();

    /**
     * @brief Removes dead objects from all tracking structures.
     * 
     * This is where dead objects are actually deleted. It's called periodically
     * to clean up objects that were marked dead by killObject(). Uses an
     * efficient algorithm that moves all dead objects to the end of the list
     * before erasing them in one batch.
     * 
     * @param use_timer If true, limits cleanup time (not used currently)
     */
    void cleanDeadObjects(const bool use_timer = true);

    /**
     * @brief Core update processing for all object updates.
     * 
     * This is the heart of object synchronization. It processes update messages
     * from the server, updates the object's properties, handles parent-child
     * relationships, and manages the active/inactive state. Every object update
     * eventually flows through here.
     * 
     * @param objectp The object to update
     * @param data User data from the message system
     * @param block Which block in the message this update came from
     * @param update_type Type of update (OUT_FULL, OUT_TERSE_IMPROVED, etc.)
     * @param dpp Data packer containing compressed update data (can be NULL)
     * @param justCreated true if this object was just created
     * @param from_cache true if this update is from cache, not network
     */
    void processUpdateCore(LLViewerObject* objectp, void** data, U32 block, const EObjectUpdateType update_type,
                           LLDataPacker* dpp, bool justCreated, bool from_cache = false);
    /**
     * @brief Processes a cached object update.
     * 
     * When entering a region, we often have cached data about objects from
     * previous visits. This creates or updates objects from that cache data,
     * which is much faster than waiting for full updates from the server.
     * 
     * @param entry The cache entry containing object data
     * @param regionp The region this cached object belongs to
     * @return The created/updated object, or NULL on failure
     */
    LLViewerObject* processObjectUpdateFromCache(LLVOCacheEntry* entry, LLViewerRegion* regionp);
    /**
     * @brief Processes object updates from the network.
     * 
     * This is the main entry point for object updates from the simulator.
     * It handles both compressed and uncompressed updates, creates new objects
     * as needed, and delegates to processUpdateCore() for the actual work.
     * 
     * @param mesgsys The message system containing the update
     * @param user_data Passed through to update handlers
     * @param update_type Type of update (full, terse, etc.)
     * @param compressed Whether the data is compressed
     */
    void processObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type, bool compressed=false);
    /**
     * @brief Processes compressed object updates.
     * 
     * Just a wrapper that calls processObjectUpdate() with compressed=true.
     * Compressed updates use zlib to reduce bandwidth.
     * 
     * @param mesgsys The message system containing the update
     * @param user_data Passed through to update handlers  
     * @param update_type Type of update
     */
    void processCompressedObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type);
    
    /**
     * @brief Processes cached object probe messages.
     * 
     * The server sends these to check if we have objects in our cache.
     * We respond with cache hits/misses so the server knows what to send.
     * 
     * @param mesgsys The message system containing the probe
     * @param user_data Passed through to update handlers
     * @param update_type Type of update
     */
    void processCachedObjectUpdate(LLMessageSystem *mesgsys, void **user_data, EObjectUpdateType update_type);
    /**
     * @brief Updates texture priorities based on viewing angle and distance.
     * 
     * This does lazy updates of object texture priorities. We can't update
     * every object every frame (too expensive), so we cycle through chunks
     * of the object list each frame. This keeps textures loading smoothly
     * without killing performance.
     * 
     * @param agent The agent to calculate angles relative to
     */
    void updateApparentAngles(LLAgent &agent);
    /**
     * @brief Main update function called every frame.
     * 
     * Core frame update that coordinates all object-related processing:
     * - Updates global frame timers and interpolation settings
     * - Calls idleUpdate on all active objects (avatars, moving objects, etc.)
     * - Updates flexible objects and animated textures if enabled
     * - Triggers asynchronous fetches for object costs and physics flags
     * - Updates render complexity calculations
     * - Collects frame statistics for performance monitoring
     * 
     * Performance note: This is one of the most performance-critical functions
     * in the viewer. The active object list is carefully maintained to minimize
     * the number of objects we need to update each frame.
     * 
     * @param agent The agent for position calculations
     */
    void update(LLAgent &agent);

    /**
     * @brief Fetches object resource costs from the server.
     * 
     * Object costs include physics cost, streaming cost, and land impact.
     * This batches requests to avoid hammering the server. Called periodically
     * from update() when there are stale costs to fetch.
     */
    void fetchObjectCosts();
    
    /**
     * @brief Fetches physics properties from the server.
     * 
     * Gets physics shape type, density, friction, restitution, and gravity
     * multiplier for objects. Like fetchObjectCosts(), this batches requests
     * for efficiency.
     */
    void fetchPhysicsFlags();

    /**
     * @brief Marks an object as needing a cost update.
     * 
     * Adds the object (and its parent if it's a child) to the stale cost
     * list. The actual fetch happens later in fetchObjectCosts().
     * 
     * @param object The object needing a cost update
     */
    void updateObjectCost(LLViewerObject* object);
    
    /**
     * @brief Updates an object's cached cost values.
     * 
     * Called when we receive cost data from the server. Updates both the
     * individual object cost and the total linkset cost.
     * 
     * @param object_id UUID of the object
     * @param object_cost This object's individual cost
     * @param link_cost Total cost of the entire linkset
     * @param physics_cost This object's physics cost
     * @param link_physics_cost Total physics cost of the linkset
     */
    void updateObjectCost(const LLUUID& object_id, F32 object_cost, F32 link_cost, F32 physics_cost, F32 link_physics_cost);
    
    /**
     * @brief Handles failed cost fetches.
     * 
     * Removes the object from pending lists so we don't keep retrying.
     * 
     * @param object_id UUID of the object that failed
     */
    void onObjectCostFetchFailure(const LLUUID& object_id);

    /**
     * @brief Marks an object as needing physics flag updates.
     * 
     * Adds the object to the stale physics flags list for batch fetching.
     * 
     * @param object The object needing physics updates
     */
    void updatePhysicsFlags(const LLViewerObject* object);
    
    /**
     * @brief Handles failed physics flag fetches.
     * 
     * Removes the object from pending lists so we don't keep retrying.
     * 
     * @param object_id UUID of the object that failed
     */
    void onPhysicsFlagsFetchFailure(const LLUUID& object_id);
    
    /**
     * @brief Updates an object's physics shape type.
     * 
     * Called when we receive physics data from the server.
     * 
     * @param object_id UUID of the object
     * @param type Physics shape type (convex hull, box, etc.)
     */
    void updatePhysicsShapeType(const LLUUID& object_id, S32 type);
    
    /**
     * @brief Updates all physics properties for an object.
     * 
     * Called when we receive detailed physics data from the server.
     * 
     * @param object_id UUID of the object
     * @param density Mass per unit volume
     * @param friction Resistance to sliding motion
     * @param restitution Bounciness (0 = no bounce, 1 = perfect bounce)
     * @param gravity_multiplier Scaling factor for gravity effects
     */
    void updatePhysicsProperties(const LLUUID& object_id,
                                    F32 density,
                                    F32 friction,
                                    F32 restitution,
                                    F32 gravity_multiplier);

    /**
     * @brief Shifts all objects by a global offset.
     * 
     * This happens when we cross region boundaries and need to recenter
     * the coordinate system. Every object's position caches are updated
     * and drawables are marked for shifting.
     * 
     * @param offset The 3D offset to apply to all objects
     */
    void shiftObjects(const LLVector3 &offset);
    
    /**
     * @brief Forces all objects to recalculate their spatial partitions.
     * 
     * This is expensive but necessary after major scene changes. It ensures
     * all objects are in the correct octree nodes for efficient culling.
     */
    void repartitionObjects();

    bool hasMapObjectInRegion(LLViewerRegion* regionp) ;
    void clearAllMapObjectsInRegion(LLViewerRegion* regionp) ;
    /**
     * @brief Renders objects on the minimap.
     * 
     * Draws colored dots for objects based on ownership and height relative
     * to water. You own = yellow/green, others own = red/blue, with above/below
     * water variants. Megaprims are clamped to reasonable sizes.
     * 
     * @param netmap The minimap to render onto
     */
    void renderObjectsForMap(LLNetMap &netmap);
    
    /**
     * @brief Renders object bounding boxes (currently unused).
     * 
     * @param center Center point for rendering
     */
    void renderObjectBounds(const LLVector3 &center);

    /**
     * @brief Adds a debug beacon at a specific location.
     * 
     * Debug beacons are vertical lines with text labels, useful for marking
     * positions during debugging. They're rendered in renderObjectBeacons().
     * 
     * @param pos_agent Position in agent coordinates
     * @param string Text to display at the beacon
     * @param color Color of the beacon line
     * @param text_color Color of the text label
     * @param line_width Width of the beacon line
     */
    void addDebugBeacon(const LLVector3 &pos_agent, const std::string &string,
                        const LLColor4 &color=LLColor4(1.f, 0.f, 0.f, 0.5f),
                        const LLColor4 &text_color=LLColor4(1.f, 1.f, 1.f, 1.f),
                        S32 line_width = 1);
    
    /**
     * @brief Renders all active debug beacons.
     * 
     * Called during the render pass to draw all beacons added with addDebugBeacon().
     */
    void renderObjectBeacons();
    
    /**
     * @brief Clears all debug beacons.
     */
    void resetObjectBeacons();

    /**
     * @brief Marks all objects as having dirty inventory.
     * 
     * Forces all objects to refresh their inventory contents. This is used
     * when global inventory state changes that might affect object contents.
     */
    void dirtyAllObjectInventory();

    /**
     * @brief Removes an object from the active update list.
     * 
     * This is an internal method that maintains the integrity of the active
     * list by swapping the last element into the removed object's position.
     * 
     * @param objectp The object to remove
     */
    void removeFromActiveList(LLViewerObject* objectp);
    
    /**
     * @brief Updates whether an object should be on the active list.
     * 
     * Active objects get their idleUpdate() called every frame. This checks
     * if an object should be active (based on motion, scripts, etc.) and
     * adds/removes it from the active list as needed.
     * 
     * @param objectp The object to check
     */
    void updateActive(LLViewerObject *objectp);

    /**
     * @brief Updates avatar visibility culling.
     * 
     * Part of the visibility optimization system. This is called to update
     * which avatars should be visible based on distance and other factors.
     */
    void updateAvatarVisibility();

    /**
     * @brief Gets the total number of objects being tracked.
     * 
     * This includes all objects - active, inactive, even dead ones that
     * haven't been cleaned up yet.
     * 
     * @return Total object count
     */
    inline S32 getNumObjects() { return (S32) mObjects.size(); }
    
    /**
     * @brief Gets the number of active objects.
     * 
     * Active objects are ones that need per-frame updates (moving objects,
     * animated objects, avatars, etc.). This number directly impacts frame rate.
     * 
     * @return Active object count
     */
    inline S32 getNumActiveObjects() { return (S32) mActiveObjects.size(); }

    /**
     * @brief Adds an object to the minimap tracking list.
     * 
     * Objects on this list are rendered as dots on the minimap.
     * 
     * @param objectp The object to add
     */
    void addToMap(LLViewerObject *objectp);
    
    /**
     * @brief Removes an object from the minimap tracking list.
     * 
     * @param objectp The object to remove
     */
    void removeFromMap(LLViewerObject *objectp);

    /**
     * @brief Clears debug text from all objects.
     * 
     * Removes any debug hover text that was added to objects for debugging.
     * This restores objects to showing only their normal hover text.
     */
    void clearDebugText();

    /**
     * @brief Removes all references to a dead object.
     * 
     * Only called by LLViewerObject::markDead(). This removes the object
     * from all tracking structures (UUID map, local ID table, active list,
     * map list) but doesn't delete it. The actual deletion happens later
     * in cleanDeadObjects().
     * 
     * @param objectp The dead object to clean up
     */
    void cleanupReferences(LLViewerObject *objectp);

    /**
     * @brief Counts references to a drawable from all objects.
     * 
     * Used for debugging to find what objects are referencing a specific
     * drawable. Searches through all objects and their children.
     * 
     * @param drawablep The drawable to search for
     * @return Number of references found
     */
    S32 findReferences(LLDrawable *drawablep) const;

    /**
     * @brief Gets the number of parent IDs we're waiting for.
     * 
     * These are parent objects that children have referenced but haven't
     * arrived yet. Used for debugging parent-child synchronization issues.
     * 
     * @return Number of pending parent IDs
     */
    S32 getOrphanParentCount() const { return (S32) mOrphanParents.size(); }
    
    /**
     * @brief Gets the number of orphaned child objects.
     * 
     * Orphans are objects whose parents haven't been created yet. This
     * happens during region crossing or when updates arrive out of order.
     * 
     * @return Number of orphaned objects
     */
    S32 getOrphanCount() const { return mNumOrphans; }
    
    /**
     * @brief Gets the current avatar count.
     * 
     * Updated every frame in update(). Useful for performance monitoring.
     * 
     * @return Number of avatars in view
     */
    S32 getAvatarCount() const { return mNumAvatars; }
    /**
     * @brief Marks an object as orphaned.
     * 
     * Called when we get a child object whose parent hasn't arrived yet.
     * The child is made invisible and added to lists to be reconnected
     * when the parent arrives.
     * 
     * @param childp The orphaned child object
     * @param parent_id Local ID of the missing parent
     * @param ip IP address of the host that sent this
     * @param port Port of the host that sent this
     */
    void orphanize(LLViewerObject *childp, U32 parent_id, U32 ip, U32 port);
    
    /**
     * @brief Reconnects orphaned children to a parent.
     * 
     * When a parent object arrives, this searches for any orphaned children
     * waiting for it and reestablishes the parent-child relationships.
     * 
     * @param objectp The parent object to check
     * @param ip IP address of the host  
     * @param port Port of the host
     */
    void findOrphans(LLViewerObject* objectp, U32 ip, U32 port);

public:
    /**
     * @brief Tracks parent-child relationships for orphaned objects.
     * 
     * When child objects arrive before their parents, we need to remember
     * the relationship so we can connect them later. This simple struct
     * stores the parent ID info and child UUID for matching.
     */
    class OrphanInfo
    {
    public:
        OrphanInfo();
        OrphanInfo(const U64 parent_info, const LLUUID child_info);
        bool operator==(const OrphanInfo &rhs) const;
        bool operator!=(const OrphanInfo &rhs) const;
        U64 mParentInfo;
        LLUUID mChildInfo;
    };


    U32 mCurBin; /// Current bin index for lazy texture updates (cycles 0 to NUM_BINS-1)

    /// Number of new objects created this frame (for statistics)
    S32 mNumNewObjects;

    /// Whether the last frame was paused - used to avoid skewing frame statistics
    bool mWasPaused;

    /**
     * @brief Looks up a UUID from local ID and host information.
     * 
     * Objects are identified by UUID globally, but by local ID within a
     * region. This translates from the region-specific ID to the global UUID.
     * 
     * @param[out] id Filled with the UUID if found, null otherwise
     * @param local_id The region-local ID
     * @param ip IP address of the region
     * @param port Port of the region
     */
    void getUUIDFromLocal(LLUUID &id,
                                const U32 local_id,
                                const U32 ip,
                                const U32 port);
    
    /**
     * @brief Creates a mapping from local ID to UUID.
     * 
     * This is called when we know both IDs for an object. It allows future
     * lookups from local ID (which is what most network messages use) to
     * the UUID (which is what the viewer uses internally).
     * 
     * The method assigns a unique region index to the object if this is the first
     * time we've seen this IP:port combination. This index is used to create a
     * globally unique identifier by combining it with the local ID.
     * 
     * @param id The object's UUID
     * @param local_id The object's local ID within its region
     * @param ip IP address of the region
     * @param port Port of the region  
     * @param objectp The object itself (for setting region index)
     */
    void setUUIDAndLocal(const LLUUID &id,
                                const U32 local_id,
                                const U32 ip,
                                const U32 port,
                                LLViewerObject* objectp);

    /**
     * @brief Removes an object from the local ID lookup table.
     * 
     * This breaks the mapping from local ID to UUID. Called when an object
     * is destroyed or changes regions.
     * 
     * @param objectp The object to remove
     * @return true if the object was in the table and removed
     */
    bool removeFromLocalIDTable(LLViewerObject* objectp);
    
    /**
     * @brief Creates a unique index from local ID and host info.
     * 
     * Used by the orphan system to create unique identifiers for parent
     * objects we haven't seen yet. The index combines the region's host
     * info with the local ID.
     * 
     * @param local_id The local ID
     * @param ip IP address
     * @param port Port number
     * @return 64-bit index combining all the information
     */
    U64 getIndex(const U32 local_id, const U32 ip, const U32 port);

    S32 mNumUnknownUpdates;      /// Count of updates received for objects we don't know about (usually means out-of-order messages)
    S32 mNumDeadObjectUpdates;   /// Count of updates received for objects already marked dead (should be rare)
    S32 mNumDeadObjects;         /// Current count of objects marked dead but not yet cleaned up
protected:
    std::vector<U64>    mOrphanParents;     /// Parent IDs we're waiting for
    std::vector<OrphanInfo> mOrphanChildren; /// Children waiting for their parents
    S32 mNumOrphans;                         /// Current count of orphaned objects
    S32 mNumAvatars;                         /// Current count of avatars in view

    typedef std::vector<LLPointer<LLViewerObject> > vobj_list_t;

    vobj_list_t mObjects;                                    /// Master list of all objects
    std::vector<LLPointer<LLViewerObject> > mActiveObjects;  /// Objects needing per-frame updates

    vobj_list_t mMapObjects;                                 /// Objects to show on minimap

    uuid_set_t   mDeadObjects;                                      /// UUIDs of objects marked for deletion

    std::map<LLUUID, LLPointer<LLViewerObject> > mUUIDObjectMap;    /// Fast lookup from UUID to object

    uuid_set_t   mStaleObjectCost;     /// Objects needing cost updates
    uuid_set_t   mPendingObjectCost;   /// Objects with cost fetches in progress

    uuid_set_t   mStalePhysicsFlags;   /// Objects needing physics updates
    uuid_set_t   mPendingPhysicsFlags; /// Objects with physics fetches in progress

    std::vector<LLDebugBeacon> mDebugBeacons;  /// Debug visualization markers

    S32 mCurLazyUpdateIndex;                    /// Current position in lazy update cycle

    static U32 sSimulatorMachineIndex;          /// Next available region index (starts at 1, not 0, for fast lookups)
    std::map<U64, U32> mIPAndPortToIndex;       /// Maps (IP<<32)|port to unique region index

    std::map<U64, LLUUID> mIndexAndLocalIDToUUID; /// Maps (region_index<<32)|local_id to object UUID

    friend class LLViewerObject;

private:
    /**
     * @brief Reports object cost fetch failures.
     * 
     * Called when the cost fetch coroutine fails. Processes the list of
     * failed objects and calls onObjectCostFetchFailure for each.
     * 
     * @param objectList LLSD array of object UUIDs that failed
     */
    static void reportObjectCostFailure(LLSD &objectList);
    
    /**
     * @brief Coroutine that fetches object costs from the server.
     * 
     * This runs asynchronously to batch-fetch costs for multiple objects.
     * It handles the HTTP request, processes responses, and updates object
     * costs or reports failures.
     * 
     * @param url The capability URL for fetching costs
     */
    void fetchObjectCostsCoro(std::string url);

    /**
     * @brief Reports physics flag fetch failures.
     * 
     * @param objectList LLSD array of object UUIDs that failed
     */
    static void reportPhysicsFlagFailure(LLSD &objectList);
    
    /**
     * @brief Coroutine that fetches physics flags from the server.
     * 
     * Similar to fetchObjectCostsCoro but for physics properties. Note the
     * typo in the method name (Phisics vs Physics) is kept for compatibility.
     * 
     * @param url The capability URL for fetching physics data
     */
    void fetchPhisicsFlagsCoro(std::string url);

};


/**
 * @brief Visual debugging beacon rendered in the 3D world.
 * 
 * These are the vertical lines with text labels you see when debugging.
 * They're useful for marking positions, showing where things are happening,
 * or visualizing invisible data like physics shapes or region boundaries.
 */
class LLDebugBeacon
{
public:
    /**
     * @brief Destructor ensures HUD object is properly cleaned up.
     */
    ~LLDebugBeacon();
    
    LLVector3 mPositionAgent;    /// Position in agent-relative coordinates
    std::string mString;         /// Text label to display
    LLColor4 mColor;            /// Color of the beacon line
    LLColor4 mTextColor;        /// Color of the text label
    S32 mLineWidth;             /// Width of the beacon line in pixels
    LLPointer<class LLHUDObject> mHUDObject;  /// HUD object for rendering
};

/**
 * @brief Global object list instance.
 * 
 * This is the one and only object list for the viewer. All object management
 * goes through this global instance. It's created at startup and destroyed
 * at shutdown.
 */
extern LLViewerObjectList gObjectList;

// Inlines

inline LLViewerObject *LLViewerObjectList::findObject(const LLUUID &id)
{
    if (id.isNull())
        return NULL;

    auto iter = mUUIDObjectMap.find(id);
    if (iter != mUUIDObjectMap.end())
    {
        return iter->second;
    }

    return NULL;
}

inline LLViewerObject *LLViewerObjectList::getObject(const S32 index)
{
    LLViewerObject *objectp;
    objectp = mObjects[index];
    if (objectp->isDead())
    {
        // Dead objects are still in the list until cleanDeadObjects() runs
        return NULL;
    }
    return objectp;
}

inline void LLViewerObjectList::addToMap(LLViewerObject *objectp)
{
    mMapObjects.push_back(objectp);
}

inline void LLViewerObjectList::removeFromMap(LLViewerObject *objectp)
{
    std::vector<LLPointer<LLViewerObject> >::iterator iter = std::find(mMapObjects.begin(), mMapObjects.end(), objectp);
    if (iter != mMapObjects.end())
    {
        mMapObjects.erase(iter);
    }
}


#endif // LL_VIEWER_OBJECT_LIST_H
