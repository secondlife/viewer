/** 
 * @file llexternaleditor.h
 * @brief A convenient class to run external editor.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLEXTERNALEDITOR_H
#define LL_LLEXTERNALEDITOR_H

#include "llprocess.h"

/**
 * Usage:
 *  LLExternalEditor ed;
 *  ed.setCommand("MY_EXTERNAL_EDITOR_VAR");
 *  ed.run("/path/to/file1");
 *  ed.run("/other/path/to/file2");
 */
class LLExternalEditor
{
	typedef std::vector<std::string> string_vec_t;

public:

	typedef enum e_error_code {
		EC_SUCCESS,				/// No error.
		EC_NOT_SPECIFIED,		/// Editor path not specified.
		EC_PARSE_ERROR,			/// Editor command parsing error.
		EC_BINARY_NOT_FOUND,	/// Could find the editor binary (missing or not quoted).
		EC_FAILED_TO_RUN,		/// Could not execute the editor binary.
	} EErrorCode;

	/**
	 * Set editor command.
	 *
	 * @param env_var			Environment variable of the same purpose.
	 * @param override			Optional override.
	 *
	 * First tries the override, then a predefined setting (sSetting),
	 * then the environment variable.
	 *
	 * @return EC_SUCCESS if command is valid and refers to an existing executable,
	 *         EC_NOT_SPECIFIED or EC_FAILED_TO_RUNan on error.
	 *
	 * @see sSetting
	 */
	EErrorCode setCommand(const std::string& env_var, const std::string& override = LLStringUtil::null);

	/**
	 * Run the editor with the given file.
	 *
	 * @param file_path File to edit.
	 * @return EC_SUCCESS on success, error code on error.
	 */
	EErrorCode run(const std::string& file_path);

	/**
	 * Get a meaningful error message for the given status code.
	 */
	static std::string getErrorMessage(EErrorCode code);

private:

	static std::string findCommand(
		const std::string& env_var,
		const std::string& override);

	static size_t tokenize(string_vec_t& tokens, const std::string& str);

	/**
	 * Filename placeholder that gets replaced with an actual file name.
	 */
	static const std::string sFilenameMarker;

	/**
	 * Setting that can specify the editor command.
	 */
	static const std::string sSetting;

	LLProcess::Params		mProcessParams;
};

#endif // LL_LLEXTERNALEDITOR_H
