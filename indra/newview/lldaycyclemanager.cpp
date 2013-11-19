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

void LLDayCycleManager::getPresetNames(preset_name_list_t& names) const
{
	names.clear();

	for (dc_map_t::const_iterator it = mDayCycleMap.begin(); it != mDayCycleMap.end(); ++it)
	{
		names.push_back(it->first);
	}
}

void LLDayCycleManager::getPresetNames(preset_name_list_t& user, preset_name_list_t& sys) const
{
	user.clear();
	sys.clear();

	for (dc_map_t::const_iterator it = mDayCycleMap.begin(); it != mDayCycleMap.end(); ++it)
	{
		const std::string& name = it->first;

		if (isSystemPreset(name))
		{
			sys.push_back(name);
		}
		else
		{
			user.push_back(name);
		}
	}
}

void LLDayCycleManager::getUserPresetNames(preset_name_list_t& user) const
{
	preset_name_list_t sys; // unused
	getPresetNames(user, sys);
}

bool LLDayCycleManager::getPreset(const std::string name, LLWLDayCycle& day_cycle) const
{
	dc_map_t::const_iterator it = mDayCycleMap.find(name);
	if (it == mDayCycleMap.end())
	{
		return false;
	}

	day_cycle = it->second;
	return true;
}

bool LLDayCycleManager::getPreset(const std::string name, LLSD& day_cycle) const
{
	LLWLDayCycle dc;
	if (!getPreset(name, dc))
	{
		return false;
	}

	day_cycle = dc.asLLSD();
	return true;
}

bool LLDayCycleManager::presetExists(const std::string name) const
{
	LLWLDayCycle dummy;
	return getPreset(name, dummy);
}

bool LLDayCycleManager::isSystemPreset(const std::string& name) const
{
	return gDirUtilp->fileExists(getSysDir() + LLURI::escape(name) + ".xml");
}

bool LLDayCycleManager::savePreset(const std::string& name, const LLSD& data)
{
	// Save given preset.
	LLWLDayCycle day;
	day.loadDayCycle(data, LLEnvKey::SCOPE_LOCAL);
	day.save(getUserDir() + LLURI::escape(name) + ".xml");

	// Add it to our map.
	addPreset(name, data);
	mModifySignal();
	return true;
}

bool LLDayCycleManager::deletePreset(const std::string& name)
{
	// Remove it from the map.
	dc_map_t::iterator it = mDayCycleMap.find(name);
	if (it == mDayCycleMap.end())
	{
		LL_WARNS("Windlight") << "No day cycle named " << name << LL_ENDL;
		return false;
	}
	mDayCycleMap.erase(it);

	// Remove from the filesystem.
	std::string filename = LLURI::escape(name) + ".xml";
	if (gDirUtilp->fileExists(getUserDir() + filename))
	{
		gDirUtilp->deleteFilesInDir(getUserDir(), filename);
	}

	// Signal interested parties.
	mModifySignal();
	return true;
}

bool LLDayCycleManager::isSkyPresetReferenced(const std::string& preset_name) const
{
	// We're traversing local day cycles, they can only reference local skies.
	LLWLParamKey key(preset_name, LLEnvKey::SCOPE_LOCAL);

	for (dc_map_t::const_iterator it = mDayCycleMap.begin(); it != mDayCycleMap.end(); ++it)
	{
		if (it->second.hasReferencesTo(key))
		{
			return true;
		}
	}

	return false;
}

boost::signals2::connection LLDayCycleManager::setModifyCallback(const modify_signal_t::slot_type& cb)
{
	return mModifySignal.connect(cb);
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
		loadPreset(gDirUtilp->add(dir, file));
	}
}

bool LLDayCycleManager::loadPreset(const std::string& path)
{
	LLSD data = LLWLDayCycle::loadDayCycleFromPath(path);
	if (data.isUndefined())
	{
		llwarns << "Error loading day cycle from " << path << llendl;
		return false;
	}

	std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));
	addPreset(name, data);

	return true;
}

bool LLDayCycleManager::addPreset(const std::string& name, const LLSD& data)
{
	if (name.empty())
	{
		llassert(name.empty());
		return false;
	}

	LLWLDayCycle day;
	day.loadDayCycle(data, LLEnvKey::SCOPE_LOCAL);
	mDayCycleMap[name] = day;
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
