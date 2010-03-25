/** 
 * @file llviewermenufile.h
 * @brief "File" menu in the main menu bar.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LLVIEWERMENUFILE_H
#define LLVIEWERMENUFILE_H

#include "llfoldertype.h"
#include "llinventorytype.h"

class LLTransactionID;

void init_menu_file();

void upload_new_resource(const std::string& src_filename, 
			 std::string name,
			 std::string desc, 
			 LLFolderType::EType destination_folder_type,
			 LLInventoryType::EType inv_type,
			 U32 next_owner_perms,
			 U32 group_perms,
			 U32 everyone_perms,
			 const std::string& display_name,
			 boost::function<void(const LLUUID& uuid)> callback,
			 S32 expected_upload_cost);

void upload_new_resource(const LLTransactionID &tid, 
			 LLAssetType::EType type,
			 std::string name,
			 std::string desc, 
			 LLFolderType::EType destination_folder_type,
			 LLInventoryType::EType inv_type,
			 U32 next_owner_perms,
			 U32 group_perms,
			 U32 everyone_perms,
			 const std::string& display_name,
			 boost::function<void(const LLUUID& uuid)> callback,
			 S32 expected_upload_cost);

#endif
