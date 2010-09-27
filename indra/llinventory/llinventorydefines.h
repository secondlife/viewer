/** 
 * @file llinventorydefines.h
 * @brief LLInventoryDefines
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
		II_FLAGS_NONE 								= 0,
		
		II_FLAGS_SHARED_SINGLE_REFERENCE 			= 0x40000000,
			// The asset has only one reference in the system. If the 
			// inventory item is deleted, or the assetid updated, then we 
			// can remove the old reference.
		
		II_FLAGS_LANDMARK_VISITED 					= 1,

		II_FLAGS_OBJECT_SLAM_PERM 					= 0x100,
			// Object permissions should have next owner perm be more 
			// restrictive on rez. We bump this into the second byte of the 
			// flags since the low byte is used to track attachment points.

		II_FLAGS_OBJECT_SLAM_SALE 					= 0x1000,
			// The object sale information has been changed.
		
		II_FLAGS_OBJECT_PERM_OVERWRITE_BASE			= 0x010000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_OWNER		= 0x020000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_GROUP		= 0x040000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_EVERYONE		= 0x080000,
		II_FLAGS_OBJECT_PERM_OVERWRITE_NEXT_OWNER	= 0x100000,
			// Specify which permissions masks to overwrite
			// upon rez.  Normally, if no permissions slam (above) or
			// overwrite flags are set, the asset's permissions are
			// used and the inventory's permissions are ignored.  If
			// any of these flags are set, the inventory's permissions
			// take precedence.

		II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS			= 0x200000,
			// Whether a returned object is composed of multiple items.

		II_FLAGS_WEARABLES_MASK = 0xff,
			// Wearables use the low order byte of flags to store the
			// LLWearableType::EType enumeration found in newview/llwearable.h

		II_FLAGS_PERM_OVERWRITE_MASK = 				(II_FLAGS_OBJECT_SLAM_PERM |
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
