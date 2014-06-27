#include "llhttpclient.h"
#include "llsd.h"
/** 
 * @file llexperienceassociationresponder.h
 * @brief llexperienceassociationresponder and related class definitions
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



#ifndef LL_LLEXPERIENCEASSOCIATIONRESPONDER_H
#define LL_LLEXPERIENCEASSOCIATIONRESPONDER_H

#include "llhttpclient.h"
#include "llsd.h"

class ExperienceAssociationResponder : public LLHTTPClient::Responder
{
public:
    typedef boost::function<void(const LLSD& experience)> callback_t;

    ExperienceAssociationResponder(callback_t callback);

    /*virtual*/ void httpSuccess();
    /*virtual*/ void httpFailure();

    static void fetchAssociatedExperience(const LLUUID& object_it, const LLUUID& item_id, callback_t callback);

private:    
    static void fetchAssociatedExperience(LLSD& request, callback_t callback);
    
    void sendResult(const LLSD& experience);

    callback_t mCallback;

};

#endif // LL_LLEXPERIENCEASSOCIATIONRESPONDER_H
