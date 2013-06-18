/** 
 * @file llaisapi.h
 * @brief classes and functions for interfacing with the v3+ ais inventory service. 
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#ifndef LL_LLAISAPI_H
#define LL_LLAISAPI_H

#include "lluuid.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include "llcurl.h"
#include "llhttpclient.h"
#include "llhttpretrypolicy.h"
#include "llviewerinventory.h"

class AISCommand: public LLHTTPClient::Responder
{
public:
	typedef boost::function<void()> command_func_type;

	AISCommand(LLPointer<LLInventoryCallback> callback);

	virtual ~AISCommand() {}

	void run_command();

	void setCommandFunc(command_func_type command_func);
	
	// Need to do command-specific parsing to get an id here, for
	// LLInventoryCallback::fire().  May or may not need to bother,
	// since most LLInventoryCallbacks do their work in the
	// destructor.
	virtual bool getResponseUUID(const LLSD& content, LLUUID& id);
	
	/* virtual */ void httpSuccess();

	/*virtual*/ void httpFailure();

	static bool getCap(std::string& cap);

private:
	command_func_type mCommandFunc;
	LLPointer<LLHTTPRetryPolicy> mRetryPolicy;
	LLPointer<LLInventoryCallback> mCallback;
};

class RemoveItemCommand: public AISCommand
{
public:
	RemoveItemCommand(const LLUUID& item_id,
					  LLPointer<LLInventoryCallback> callback);
};

class RemoveCategoryCommand: public AISCommand
{
public:
	RemoveCategoryCommand(const LLUUID& item_id,
						  LLPointer<LLInventoryCallback> callback);
};

class PurgeDescendentsCommand: public AISCommand
{
public:
	PurgeDescendentsCommand(const LLUUID& item_id,
							LLPointer<LLInventoryCallback> callback);
};

class UpdateItemCommand: public AISCommand
{
public:
	UpdateItemCommand(const LLUUID& item_id,
					  const LLSD& updates,
					  LLPointer<LLInventoryCallback> callback);
private:
	LLSD mUpdates;
};

class UpdateCategoryCommand: public AISCommand
{
public:
	UpdateCategoryCommand(const LLUUID& item_id,
						  const LLSD& updates,
						  LLPointer<LLInventoryCallback> callback);
private:
	LLSD mUpdates;
};

class SlamFolderCommand: public AISCommand
{
public:
	SlamFolderCommand(const LLUUID& folder_id, const LLSD& contents, LLPointer<LLInventoryCallback> callback);
	
private:
	LLSD mContents;
};

class AISUpdate
{
public:
	AISUpdate(const LLSD& update);
	void parseUpdate(const LLSD& update);
	void parseUUIDArray(const LLSD& content, const std::string& name, uuid_vec_t& ids);
	void parseLink(const LLUUID& link_id, const LLSD& link_map);
	void parseCreatedLinks(const LLSD& links);
	void doUpdate();
private:
	typedef std::map<LLUUID,S32> uuid_int_map_t;
	uuid_int_map_t mCatDeltas;
	uuid_int_map_t mCatVersions;

	typedef std::map<LLUUID,LLPointer<LLViewerInventoryItem> > deferred_item_map_t;
	deferred_item_map_t mItemsCreated;
	deferred_item_map_t mItemsUpdated;

	std::set<LLUUID> mObjectsDeleted;
	uuid_vec_t mItemsCreatedIds;
};

#endif
