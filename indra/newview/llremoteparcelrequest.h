/** 
 * @file llremoteparcelrequest.h
 * @author Sam Kolb
 * @brief Get information about a parcel you aren't standing in to display
 * landmark/teleport information.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
	/*virtual*/ void error(U32 status, const std::string& reason);

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
	typedef std::multimap<LLUUID, LLRemoteParcelInfoObserver*> observer_multimap_t;
	observer_multimap_t mObservers;
};

#endif // LL_LLREMOTEPARCELREQUEST_H
