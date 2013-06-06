/**
 * @file   llpipelinelistener.h
 * @author Don Kjer
 * @date   2012-07-09
 * @brief  Wrap subset of LLPipeline API in event API
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#if ! defined(LL_LLPIPELINELISTENER_H)
#define LL_LLPIPELINELISTENER_H

#include "lleventapi.h"

/// Listen on an LLEventPump with specified name for LLPipeline request events.
class LLPipelineListener: public LLEventAPI
{
public:
	LLPipelineListener();
};

#endif /* ! defined(LL_LLPIPELINELISTENER_H) */
