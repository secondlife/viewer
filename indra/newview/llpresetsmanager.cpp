/**
 * @file llpresetsmanager.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llpresetsmanager.h"

#include "lldiriterator.h"
#include "lluictrlfactory.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"

static const std::string PRESETS_DIR = "presets";
static const std::string GRAPHIC_DIR = "graphic";
static const std::string CAMERA_DIR = "camera";

LLPresetsManager::LLPresetsManager()
{
}

LLPresetsManager::~LLPresetsManager()
{
}

std::string LLPresetsManager::getUserDir(const std::string& subdirectory)
{
	std::string presets_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, PRESETS_DIR);
	std::string full_path;

	if (!gDirUtilp->fileExists(presets_path))
	{
		LLFile::mkdir(presets_path);
	}

	full_path = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, PRESETS_DIR, subdirectory);
	if (!gDirUtilp->fileExists(full_path))
	{
		LLFile::mkdir(full_path);
	}

	return full_path;
}

std::string LLPresetsManager::getGraphicPresetsDir()
{
	return getUserDir(GRAPHIC_DIR);
}

void LLPresetsManager::getPresetNames(preset_name_list_t& presets) const
{
	presets = mPresetNames;

}

void LLPresetsManager::loadPresetsFromDir(const std::string& dir)
{
	LL_INFOS("AppInit") << "Loading presets from " << dir << LL_ENDL;

	mPresetNames.clear();

	LLDirIterator dir_iter(dir, "*.xml");
	while (1)
	{
		std::string file;
		if (!dir_iter.next(file))
		{
			break; // no more files
		}

		std::string path = gDirUtilp->add(dir, file);
		std::string name(gDirUtilp->getBaseFileName(LLURI::unescape(path), /*strip_exten = */ true));
		mPresetNames.push_back(name);
	}
}

void LLPresetsManager::savePreset(const std::string & name)
{
	llassert(!name.empty());

	// make an empty llsd
	LLSD paramsData(LLSD::emptyMap());
	std::string pathName(getUserDir(GRAPHIC_DIR) + LLURI::escape(name) + ".xml");

// Get all graphic settings
//	paramsData = mParamList[name].getAll();

	// write to file
	llofstream presetsXML(pathName);
	LLPointer<LLSDFormatter> formatter = new LLSDXMLFormatter();
	formatter->format(paramsData, presetsXML, LLSDFormatter::OPTIONS_PRETTY);
	presetsXML.close();
}

bool LLPresetsManager::removeParamSet(const std::string& name, bool delete_from_disk)
{
	// remove from param list
	preset_name_list_t::iterator it = find(mPresetNames.begin(), mPresetNames.end(), name);
	if (it == mPresetNames.end())
	{
		LL_WARNS("Presets") << "No preset named " << name << LL_ENDL;
		return false;
	}

	mPresetNames.erase(it);

	// remove from file system if requested
	if (delete_from_disk)
	{
		if (gDirUtilp->deleteFilesInDir(getUserDir(GRAPHIC_DIR), LLURI::escape(name) + ".xml") < 1)
		{
			LL_WARNS("Presets") << "Error removing preset " << name << " from disk" << LL_ENDL;
		}
	}

	// signal interested parties
	mPresetListChangeSignal();

	return true;
}

boost::signals2::connection LLPresetsManager::setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb)
{
	return mPresetListChangeSignal.connect(cb);
}
