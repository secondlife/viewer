/**
 * @file llfloatermodeluploadbase.h
 * @brief LLFloaterUploadModelBase class declaration
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

#ifndef LLFLOATERMODELUPLOADBASE_H_
#define LLFLOATERMODELUPLOADBASE_H_

#include "lluploadfloaterobservers.h"

class LLFloaterModelUploadBase : public LLFloater, public LLUploadPermissionsObserver, public LLWholeModelFeeObserver
{
public:

	LLFloaterModelUploadBase(const LLSD& key);

	virtual ~LLFloaterModelUploadBase(){};

	virtual void setPermissonsErrorStatus(U32 status, const std::string& reason){};

	virtual void onPermissionsReceived(const LLSD& result){};

	virtual void onModelPhysicsFeeReceived(F64 physics, S32 fee, std::string upload_url){};

	virtual void setModelPhysicsFeeErrorStatus(U32 status, const std::string& reason){};

protected:

	// requests agent's permissions to upload model
	void requestAgentUploadPermissions();

	std::string mUploadModelUrl;
	bool mHasUploadPerm;
};

#endif /* LLFLOATERMODELUPLOADBASE_H_ */
