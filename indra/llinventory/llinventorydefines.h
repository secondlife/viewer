/** 
 * @file llinventorydefines.h
 * @brief LLInventoryDefines
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

#ifndef LL_LLINVENTORYDEFINES_H
#define LL_LLINVENTORYDEFINES_H

// Consts for "key" field in the task inventory update message
extern const U8 TASK_INVENTORY_ITEM_KEY;
extern const U8 TASK_INVENTORY_ASSET_KEY;

// Max inventory buffer size (for use in packBinaryBucket)
enum
{
    MAX_INVENTORY_BUFFER_SIZE = 1024
};

//--------------------------------------------------------------------
// Inventory item flags enums
//   The shared flags at the top are shared among all inventory
//   types. After that section, all values of flags are type
//   dependent.  The shared flags will start at 2^30 and work
//   down while item type specific flags will start at 2^0 and work up.
//--------------------------------------------------------------------
class LLInventoryItemFlags
{
public:
    enum EType
    {
        II_FLAGS_NONE                               = 0,
        
        II_FLAGS_SHARED_SINGLE_REFERENCE            = 0x40000000,
            // The asset has only one reference in the system. If the 
            // inventory item is deleted, or the assetid updated, then we 
            // can remove the old reference.
        
        II_FLAGS_LANDMARK_VISITED                   = 1,

        II_FLAGS_OBJECT_SLAM_PERM                   = 0x100,
            // Object permissions should have next owner perm be more 
            // restrictive on rez. We bump this into the second byte of the 
            // flags since the low byte is used to track attachment points.

        II_FLAGS_OBJECT_SLAM_SALE                   = 0x1000,
            // The object sale information has been changed.
        
        II_FLAGS_OBJECT_PERM_OVERWRITE_BASE         = 0x010000,
        II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER        = 0x020000,
        II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP        = 0x040000,
        II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE     = 0x080000,
        II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER   = 0x100000,
            // Specify which permissions masks to overwrite
            // upon rez.  Normally, if no permissions slam (above) or
            // overwrite flags are set, the asset's permissions are
            // used and the inventory's permissions are ignored.  If
            // any of these flags are set, the inventory's permissions
            // take precedence.

        II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS          = 0x200000,
            // Whether a returned object is composed of multiple items.

        II_FLAGS_SUBTYPE_MASK                       = 0x0000ff,
            // Some items like Wearables and settings use the low order byte 
            // of flags to store the sub type of the inventory item.
            // see LLWearableType::EType enumeration found in newview/llwearable.h

        II_FLAGS_PERM_OVERWRITE_MASK =              (II_FLAGS_OBJECT_SLAM_PERM |
                                                     II_FLAGS_OBJECT_SLAM_SALE |
                                                     II_FLAGS_OBJECT_PERM_OVERWRITE_BASE |
                                                     II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER |
                                                     II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP |
                                                     II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE |
                                                     II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER),
            // These bits need to be cleared whenever the asset_id is updated
            // on a pre-existing inventory item (DEV-28098 and DEV-30997)
    };
};

#endif // LL_LLINVENTORYDEFINES_H
