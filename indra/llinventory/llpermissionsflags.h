/** 
 * @file llpermissionsflags.h
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

#ifndef LL_LLPERMISSIONSFLAGS_H
#define LL_LLPERMISSIONSFLAGS_H

// Flags for various permissions bits.
// Shared between viewer and simulator.

// permission bits
typedef U32 PermissionMask;
typedef U32 PermissionBit;


// Do you have permission to transfer ownership of the object or
// item. Fair use rules dictate that if you cannot copy, you can
// always transfer.
const PermissionBit PERM_TRANSFER           = (1 << 13); // 0x00002000

// objects, scale or change textures
// parcels, allow building on it
const PermissionBit PERM_MODIFY				= (1 << 14); // 0x00004000

// objects, allow copy
const PermissionBit PERM_COPY				= (1 << 15); // 0x00008000

// parcels, allow entry, deprecated
//const PermissionBit PERM_ENTER			= (1 << 16); // 0x00010000

// parcels, allow terraform, deprecated
//const PermissionBit PERM_TERRAFORM		= (1 << 17); // 0x00020000

// NOTA BENE: This flag is NO LONGER USED!!! However, it is possible that some 
// objects in the universe have it set so DON"T USE IT going forward.
//const PermissionBit PERM_OWNER_DEBIT		= (1 << 18); // 0x00040000

// objects, can grab/translate/rotate
const PermissionBit PERM_MOVE				= (1 << 19); // 0x00080000

// parcels, avatars take damage, deprecated
//const PermissionBit	PERM_DAMAGE			= (1 << 20); // 0x00100000

// don't use bit 31 -- printf/scanf with "%x" assume signed numbers
const PermissionBit PERM_RESERVED			= ((U32)1) << 31;

const PermissionMask PERM_NONE				= 0x00000000;
const PermissionMask PERM_ALL				= 0x7FFFFFFF;
//const PermissionMask PERM_ALL_PARCEL 		= PERM_MODIFY | PERM_ENTER | PERM_TERRAFORM | PERM_DAMAGE;
const PermissionMask PERM_ITEM_UNRESTRICTED =  PERM_MODIFY | PERM_COPY | PERM_TRANSFER;


// Useful stuff for transmission.
// Which permissions field are we trying to change?
const U8 PERM_BASE		= 0x01;
// TODO: Add another PERM_OWNER operation type for allowOperationBy  DK 04/03/06
const U8 PERM_OWNER		= 0x02;
const U8 PERM_GROUP		= 0x04;
const U8 PERM_EVERYONE	= 0x08;
const U8 PERM_NEXT_OWNER = 0x10;

// This is just a quickie debugging key
// no modify: PERM_ALL & ~PERM_MODIFY                  = 0x7fffbfff 
// no copy:   PERM_ALL & ~PERM_COPY                    = 0x7fff7fff
// no modify or copy:                                  = 0x7fff3fff
// no transfer: PERM_ALL & ~PERM_TRANSFER              = 0x7fffdfff
// no modify, no transfer                              = 0x7fff9fff
// no copy, no transfer (INVALID!)                     = 0x7fff5fff
// no modify, no copy, no transfer (INVALID!)          = 0x7fff1fff


#endif
