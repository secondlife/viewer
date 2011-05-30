/**
 * @file lldaycyclemanager.h
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

#ifndef LL_LLDAYCYCLEMANAGER_H
#define LL_LLDAYCYCLEMANAGER_H

#include <map>
#include <string>

#include "llwldaycycle.h"
#include "llwlparammanager.h"

/**
 * WindLight day cycles manager class
 *
 * Provides interface for accessing, loading and saving day cycles.
 */
class LLDayCycleManager : public LLSingleton<LLDayCycleManager>
{
	LOG_CLASS(LLDayCycleManager);

public:
	typedef std::map<std::string, LLWLDayCycle> dc_map_t;

	const dc_map_t& getPresets();
	bool getPreset(const std::string name, LLWLDayCycle& day_cycle) const;
	bool getPreset(const std::string name, LLSD& day_cycle) const;
	bool presetExists(const std::string name) const;
	bool isSystemPreset(const std::string& name) const;
	bool savePreset(const std::string& name, const LLSD& data);
	bool deletePreset(const std::string& name);

private:
	friend class LLSingleton<LLDayCycleManager>;
	/*virtual*/ void initSingleton();

	void loadAllPresets();
	void loadPresets(const std::string& dir);
	bool loadPreset(const std::string& path);
	bool addPreset(const std::string& name, const LLSD& data);

	static std::string getSysDir();
	static std::string getUserDir();

	dc_map_t mDayCycleMap;
};

#endif // LL_LLDAYCYCLEMANAGER_H
