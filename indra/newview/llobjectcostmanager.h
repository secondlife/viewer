/**
 * @file llobjectcostmanager.h
 * @brief Classes for managing object-related costs (rendering, streaming, etc) across multiple versions.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#ifndef LL_LLOBJECTCOSTMANAGER_H
#define LL_LLOBJECTCOSTMANAGER_H

#include "llsingleton.h"

class LLVOVolume;
class LLViewerObject;

class LLObjectCostManagerImpl;

class LLObjectCostManager: public LLSingleton<LLObjectCostManager>
{
	LLSINGLETON(LLObjectCostManager);
	~LLObjectCostManager();
	
public:
	U32 getCurrentCostVersion();

	F32 getStreamingCost(U32 version, const LLVOVolume *vol);

	F32 getRenderCost(U32 version, const LLVOVolume *vol);
	F32 getRenderCostLinkset(U32 version, const LLViewerObject *root);

private:
	LLObjectCostManagerImpl *m_impl;
};

#endif
