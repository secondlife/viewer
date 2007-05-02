/** 
 * @file llviewermenufile.h
 * @brief "File" menu in the main menu bar.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLVIEWERMENUFILE_H
#define LLVIEWERMENUFILE_H

#include "llassettype.h"
#include "llinventorytype.h"

class LLTransactionID;


void init_menu_file();

void upload_new_resource(const LLString& src_filename, std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm = 0x0,	// PERM_NONE
						 const LLString& display_name = LLString::null,
						 LLAssetStorage::LLStoreAssetCallback callback = NULL,
						 void *userdata = NULL);

void upload_new_resource(const LLTransactionID &tid, LLAssetType::EType type,
						 std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm = 0x0,	// PERM_NONE
						 const LLString& display_name = LLString::null,
						 LLAssetStorage::LLStoreAssetCallback callback = NULL,
						 void *userdata = NULL);

#endif
