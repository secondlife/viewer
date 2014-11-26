/**
 * @file llpresetsmanager.h
 * @brief Implementation for the LLPresetsManager class.
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

#ifndef LL_PRESETSMANAGER_H
#define LL_PRESETSMANAGER_H

#include <list>
#include <map>

class LLPresetsManager : public LLSingleton<LLPresetsManager>
{
public:
	typedef std::list<std::string> preset_name_list_t;
	typedef boost::signals2::signal<void()> preset_list_signal_t;

	void getPresetNames(preset_name_list_t& presets) const;
	void loadPresetsFromDir(const std::string& dir);
	void savePreset(const std::string & name);
	static std::string getGraphicPresetsDir();
	bool removeParamSet(const std::string& name, bool delete_from_disk);

	/// Emitted when a preset gets added or deleted.
	boost::signals2::connection setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb);

	preset_name_list_t mPresetNames;

//protected:
	LLPresetsManager();
	~LLPresetsManager();

	static std::string getUserDir(const std::string& subdirectory);

	preset_list_signal_t mPresetListChangeSignal;
};

#endif LL_PRESETSMANAGER_H
