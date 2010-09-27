/** 
 * @file llviewerassetstorage.h
 * @brief Class for loading asset data to/from an external source.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#ifndef LLVIEWERASSETSTORAGE_H
#define LLVIEWERASSETSTORAGE_H

#include "llassetstorage.h"
//#include "curl/curl.h"

class LLVFile;

class LLViewerAssetStorage : public LLAssetStorage
{
public:
	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs, const LLHost &upstream_host);

	LLViewerAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
				   LLVFS *vfs, LLVFS *static_vfs);

	using LLAssetStorage::storeAssetData;
	virtual void storeAssetData(
		const LLTransactionID& tid,
		LLAssetType::EType atype,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool store_local = false,
		bool user_waiting=FALSE,
		F64 timeout=LL_ASSET_STORAGE_TIMEOUT);
	
	virtual void storeAssetData(
		const std::string& filename,
		const LLTransactionID& tid,
		LLAssetType::EType type,
		LLStoreAssetCallback callback,
		void* user_data,
		bool temp_file = false,
		bool is_priority = false,
		bool user_waiting=FALSE,
		F64 timeout=LL_ASSET_STORAGE_TIMEOUT);
};

#endif
