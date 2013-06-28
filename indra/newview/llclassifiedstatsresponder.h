/** 
 * @file llclassifiedstatsresponder.h
 * @brief Receives information about classified ad click-through
 * counts for display in the classified information UI.
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
#ifndef LL_LLCLASSIFIEDSTATSRESPONDER_H
#define LL_LLCLASSIFIEDSTATSRESPONDER_H

#include "llhttpclient.h"
#include "llview.h"
#include "lluuid.h"

class LLClassifiedStatsResponder : public LLHTTPClient::Responder
{
public:
	LLClassifiedStatsResponder(LLUUID classified_id);
	//If we get back a normal response, handle it here
	virtual void result(const LLSD& content);
	//If we get back an error (not found, etc...), handle it here
	
	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content);

protected:
	LLUUID mClassifiedID;
};

#endif // LL_LLCLASSIFIEDSTATSRESPONDER_H
