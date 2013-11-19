/**
 * @file lluploadfloaterobservers.h
 * @brief LLUploadModelPremissionsResponder declaration
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLUPLOADFLOATEROBSERVERS_H
#define LL_LLUPLOADFLOATEROBSERVERS_H

#include "llfloater.h"
#include "llhttpclient.h"
#include "llhandle.h"

class LLUploadPermissionsObserver
{
public:

	LLUploadPermissionsObserver(){mUploadPermObserverHandle.bind(this);}
	virtual ~LLUploadPermissionsObserver() {}

	virtual void onPermissionsReceived(const LLSD& result) = 0;
	virtual void setPermissonsErrorStatus(U32 status, const std::string& reason) = 0;

	LLHandle<LLUploadPermissionsObserver> getPermObserverHandle() const {return mUploadPermObserverHandle;}

protected:
	LLRootHandle<LLUploadPermissionsObserver> mUploadPermObserverHandle;
};

class LLWholeModelFeeObserver
{
public:
	LLWholeModelFeeObserver() { mWholeModelFeeObserverHandle.bind(this); }
	virtual ~LLWholeModelFeeObserver() {}

	virtual void onModelPhysicsFeeReceived(const LLSD& result, std::string upload_url) = 0;
	virtual void setModelPhysicsFeeErrorStatus(U32 status, const std::string& reason) = 0;

	LLHandle<LLWholeModelFeeObserver> getWholeModelFeeObserverHandle() const { return mWholeModelFeeObserverHandle; }

protected:
	LLRootHandle<LLWholeModelFeeObserver> mWholeModelFeeObserverHandle;
};


class LLWholeModelUploadObserver
{
public:
	LLWholeModelUploadObserver() { mWholeModelUploadObserverHandle.bind(this); }
	virtual ~LLWholeModelUploadObserver() {}

	virtual void onModelUploadSuccess() = 0;

	virtual void onModelUploadFailure() = 0;

	LLHandle<LLWholeModelUploadObserver> getWholeModelUploadObserverHandle() const { return mWholeModelUploadObserverHandle; }

protected:
	LLRootHandle<LLWholeModelUploadObserver> mWholeModelUploadObserverHandle;
};


class LLUploadModelPremissionsResponder : public LLHTTPClient::Responder
{
public:

	LLUploadModelPremissionsResponder(const LLHandle<LLUploadPermissionsObserver>& observer);

	void errorWithContent(U32 status, const std::string& reason, const LLSD& content);

	void result(const LLSD& content);

private:
	LLHandle<LLUploadPermissionsObserver> mObserverHandle;
};

#endif /* LL_LLUPLOADFLOATEROBSERVERS_H */
