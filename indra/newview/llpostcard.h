/** 
 * @file llpostcard.h
 * @brief Sending postcards.
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

#ifndef LL_LLPOSTCARD_H
#define LL_LLPOSTCARD_H

#include "llimage.h"
#include "lluuid.h"
#include "llviewerassetupload.h"

/// *TODO$: this LLPostCard class is a hold over and should be removed.  Right now
/// all it does is hold a pointer to a call back function which is invoked by 
/// llpanelsnapshotpostcard's finish function. (and all that call back does is 
/// set the status in the floater.
class LLPostCard
{
	LOG_CLASS(LLPostCard);

public:
	typedef boost::function<void(bool ok)> result_callback_t;

	static void setPostResultCallback(result_callback_t cb) { mResultCallback = cb; }
	static void reportPostResult(bool ok);

private:
	static result_callback_t mResultCallback;
};


class LLPostcardUploadInfo : public LLBufferedAssetUploadInfo
{
public:
    LLPostcardUploadInfo(std::string emailFrom, std::string nameFrom, std::string emailTo,
        std::string subject, std::string message, LLVector3d globalPosition,
        LLPointer<LLImageFormatted> image, invnUploadFinish_f finish);

    virtual LLSD generatePostBody();
private:
    std::string mEmailFrom;
    std::string mNameFrom;
    std::string mEmailTo;
    std::string mSubject;
    std::string mMessage;
    LLVector3d mGlobalPosition;

};


#endif // LL_LLPOSTCARD_H
