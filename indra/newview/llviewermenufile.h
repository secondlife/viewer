/** 
 * @file llviewermenufile.h
 * @brief "File" menu in the main menu bar.
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

#ifndef LLVIEWERMENUFILE_H
#define LLVIEWERMENUFILE_H

#include "llfoldertype.h"
#include "llassetstorage.h"
#include "llinventorytype.h"

class LLTransactionID;


void init_menu_file();

void upload_new_resource(const std::string& src_filename, 
			 std::string name,
			 std::string desc, 
			 S32 compression_info,
			 LLFolderType::EType destination_folder_type,
			 LLInventoryType::EType inv_type,
			 U32 next_owner_perms,
			 U32 group_perms,
			 U32 everyone_perms,
			 const std::string& display_name,
			 LLAssetStorage::LLStoreAssetCallback callback,
			 S32 expected_upload_cost,
			 void *userdata);

void upload_new_resource(const LLTransactionID &tid, 
			 LLAssetType::EType type,
			 std::string name,
			 std::string desc, 
			 S32 compression_info,
			 LLFolderType::EType destination_folder_type,
			 LLInventoryType::EType inv_type,
			 U32 next_owner_perms,
			 U32 group_perms,
			 U32 everyone_perms,
			 const std::string& display_name,
			 LLAssetStorage::LLStoreAssetCallback callback,
			 S32 expected_upload_cost,
			 void *userdata);

#endif
