/** 
 * @file llremoteparcelrequest.h
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLREMOTEPARCELREQUEST_H
#define LL_LLREMOTEPARCELREQUEST_H

#include "llhttpclient.h"
#include "llpanel.h"

class LLMessageSystem;
class LLRemoteParcelInfoObserver;

class LLRemoteParcelRequestResponder : public LLHTTPClient::Responder
{
public:
	LLRemoteParcelRequestResponder(LLHandle<LLRemoteParcelInfoObserver> observer_handle);

	//If we get back a normal response, handle it here
	/*virtual*/ void result(const LLSD& content);

	//If we get back an error (not found, etc...), handle it here
	/*virtual*/ void errorWithContent(U32 status, const std::string& reason, const LLSD& content);

protected:
	LLHandle<LLRemoteParcelInfoObserver> mObserverHandle;
};

struct LLParcelData
{
	LLUUID		parcel_id;
	LLUUID		owner_id;
	std::string	name;
	std::string	desc;
	S32			actual_area;
	S32			billable_area;
	U8			flags;
	F32			global_x;
	F32			global_y;
	F32			global_z;
	std::string	sim_name;
	LLUUID		snapshot_id;
	F32			dwell;
	S32			sale_price;
	S32			auction_id;
};

// An interface class for panels which display parcel information
// like name, description, area, snapshot etc.
class LLRemoteParcelInfoObserver
{
public:
	LLRemoteParcelInfoObserver() { mObserverHandle.bind(this); }
	virtual ~LLRemoteParcelInfoObserver() {}
	virtual void processParcelInfo(const LLParcelData& parcel_data) = 0;
	virtual void setParcelID(const LLUUID& parcel_id) = 0;
	virtual void setErrorStatus(U32 status, const std::string& reason) = 0;
	LLHandle<LLRemoteParcelInfoObserver>	getObserverHandle() const { return mObserverHandle; }

protected:
	LLRootHandle<LLRemoteParcelInfoObserver> mObserverHandle;
};

class LLRemoteParcelInfoProcessor : public LLSingleton<LLRemoteParcelInfoProcessor>
{
public:
	virtual ~LLRemoteParcelInfoProcessor() {}

	void addObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer);
	void removeObserver(const LLUUID& parcel_id, LLRemoteParcelInfoObserver* observer);

	void sendParcelInfoRequest(const LLUUID& parcel_id);

	static void processParcelInfoReply(LLMessageSystem* msg, void**);

private:
	typedef std::multimap<LLUUID, LLHandle<LLRemoteParcelInfoObserver> > observer_multimap_t;
	observer_multimap_t mObservers;
};

#endif // LL_LLREMOTEPARCELREQUEST_H
