/**
 * @file lluploadfloaterobservers.cpp
 * @brief LLUploadModelPermissionsResponder definition
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

#include "llviewerprecompiledheaders.h"

#include "lluploadfloaterobservers.h"

LLUploadModelPermissionsResponder::LLUploadModelPermissionsResponder(const LLHandle<LLUploadPermissionsObserver>& observer)
:mObserverHandle(observer)
{
}

void LLUploadModelPermissionsResponder::httpFailure()
{
	LL_WARNS() << dumpResponse() << LL_ENDL;

	LLUploadPermissionsObserver* observer = mObserverHandle.get();

	if (observer)
	{
		observer->setPermissonsErrorStatus(getStatus(), getReason());
	}
}

void LLUploadModelPermissionsResponder::httpSuccess()
{
	const LLSD& content = getContent();
	if (!content.isMap())
	{
		failureResult(HTTP_INTERNAL_ERROR, "Malformed response contents", content);
		return;
	}
	LLUploadPermissionsObserver* observer = mObserverHandle.get();

	if (observer)
	{
		observer->onPermissionsReceived(content);
	}
}

