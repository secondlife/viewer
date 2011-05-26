/**
 * @file lldaycyclemanager.cpp
 * @brief Implementation for the LLDayCycleManager class.
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

#include "lldaycyclemanager.h"

#include "lldiriterator.h"

const LLDayCycleManager::dc_map_t& LLDayCycleManager::getPresets()
{
	// Refresh day cycles.
	loadAllPresets();

	return mDayCycleMap;
}

bool LLDayCycleManager::getPreset(const std::string name, LLWLDayCycle& day_cycle)
{
	dc_map_t::const_iterator it = mDayCycleMap.find(name);
	if (it == mDayCycleMap.end())
	{
		return false;
	}

	day_cycle = it->second;
	return true;
}

bool LLDayCycleManager::getPreset(const std::string name, LLSD& day_cycle)
{
	LLWLDayCycle dc;
	if (!getPreset(name, dc))
	{
		return false;
	}

	day_cycle = dc.asLLSD();
	return true;
}

// virtual
void LLDayCycleManager::initSingleton()
{
	LL_DEBUGS("Windlight") << "Loading all day cycles" << LL_ENDL;
	loadAllPresets();
}

void LLDayCycleManager::loadAllPresets()
{
	mDayCycleMap.clear();

	// First, load system (coming out of the box) day cycles.
	loadPresets(getSysDir());

	// Then load user presets. Note that user day cycles will modify any system ones already loaded.
	loadPresets(getUserDir());
}

void LLDayCycleManager::loadPresets(const std::string& dir)
{
	LLDirIterator dir_iter(dir, "*.xml");

	while (1)
	{
		std::string file;
		if (!dir_iter.next(file)) break; // no more files
		loadPreset(dir + file);
	}
}

bool LLDayCycleManager::loadPreset(const std::string& path)
{
	LLSD data = LLWLDayCycle::loadDayCycleFromPath(path);
	if (data.isUndefined())
	{
		LL_WARNS("Windlight") << "Error loading day cycle from " << path << LL_ENDL;
		return false;
	}

	std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));
	LLWLDayCycle day_cycle;
	day_cycle.loadDayCycle(data, LLEnvKey::SCOPE_LOCAL);
	mDayCycleMap[name] = day_cycle;

	return true;
}

// static
std::string LLDayCycleManager::getSysDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "windlight/days", "");
}

// static
std::string LLDayCycleManager::getUserDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS , "windlight/days", "");
}
