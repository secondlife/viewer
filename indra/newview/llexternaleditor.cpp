/** 
 * @file llexternaleditor.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llexternaleditor.h"

#include "lltrans.h"
#include "llui.h"
#include "llprocess.h"
#include "llsdutil.h"
#include <boost/foreach.hpp>

// static
const std::string LLExternalEditor::sFilenameMarker = "%s";

// static
const std::string LLExternalEditor::sSetting = "ExternalEditor";

LLExternalEditor::EErrorCode LLExternalEditor::setCommand(const std::string& env_var, const std::string& override)
{
	std::string cmd = findCommand(env_var, override);
	if (cmd.empty())
	{
		llwarns << "Editor command is empty or not set" << llendl;
		return EC_NOT_SPECIFIED;
	}

	string_vec_t tokens;
	tokenize(tokens, cmd);

	// Check executable for existence.
	std::string bin_path = tokens[0];
	if (!LLFile::isfile(bin_path))
	{
		llwarns << "Editor binary [" << bin_path << "] not found" << llendl;
		return EC_BINARY_NOT_FOUND;
	}

	// Save command.
	mProcessParams["executable"] = bin_path;
	mProcessParams["args"].clear();
	for (size_t i = 1; i < tokens.size(); ++i)
	{
		mProcessParams["args"].append(tokens[i]);
	}

	// Add the filename marker if missing.
	if (cmd.find(sFilenameMarker) == std::string::npos)
	{
		mProcessParams["args"].append(sFilenameMarker);
		llinfos << "Adding the filename marker (" << sFilenameMarker << ")" << llendl;
	}

	llinfos << "Setting command [" << bin_path;
	BOOST_FOREACH(const std::string& arg, llsd::inArray(mProcessParams["args"]))
	{
		llcont << " \"" << arg << "\"";
	}
	llcont << "]" << llendl;

	return EC_SUCCESS;
}

LLExternalEditor::EErrorCode LLExternalEditor::run(const std::string& file_path)
{
	if (mProcessParams["executable"].asString().empty() || ! mProcessParams["args"].size())
	{
		llwarns << "Editor command not set" << llendl;
		return EC_NOT_SPECIFIED;
	}

	// Copy params block so we can replace sFilenameMarker
	LLSD params(mProcessParams);

	// Substitute the filename marker in the command with the actual passed file name.
	LLSD& args(params["args"]);
	for (LLSD::array_iterator ai(args.beginArray()), aend(args.endArray()); ai != aend; ++ai)
	{
		std::string sarg(*ai);
		LLStringUtil::replaceString(sarg, sFilenameMarker, file_path);
		*ai = sarg;
	}

	// Run the editor.
	llinfos << "Running editor command [" << params["executable"];
	BOOST_FOREACH(const std::string& arg, llsd::inArray(params["args"]))
	{
		llcont << " \"" << arg << "\"";
	}
	llcont << "]" << llendl;
	// Prevent killing the process in destructor.
	params["autokill"] = false;
	return LLProcess::create(params) ? EC_SUCCESS : EC_FAILED_TO_RUN;
}

// static
std::string LLExternalEditor::getErrorMessage(EErrorCode code)
{
	switch (code)
	{
	case EC_SUCCESS: 			return LLTrans::getString("ok");
	case EC_NOT_SPECIFIED: 		return LLTrans::getString("ExternalEditorNotSet");
	case EC_PARSE_ERROR:		return LLTrans::getString("ExternalEditorCommandParseError");
	case EC_BINARY_NOT_FOUND:	return LLTrans::getString("ExternalEditorNotFound");
	case EC_FAILED_TO_RUN:		return LLTrans::getString("ExternalEditorFailedToRun");
	}

	return LLTrans::getString("Unknown");
}

// static
size_t LLExternalEditor::tokenize(string_vec_t& tokens, const std::string& str)
{
	tokens.clear();

	// Split the argument string into separate strings for each argument
	typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("", "\" ", boost::drop_empty_tokens);

	tokenizer tokens_list(str, sep);
	tokenizer::iterator token_iter;
	BOOL inside_quotes = FALSE;
	BOOL last_was_space = FALSE;
	for (token_iter = tokens_list.begin(); token_iter != tokens_list.end(); ++token_iter)
	{
		if (!strncmp("\"",(*token_iter).c_str(),2))
		{
			inside_quotes = !inside_quotes;
		}
		else if (!strncmp(" ",(*token_iter).c_str(),2))
		{
			if(inside_quotes)
			{
				tokens.back().append(std::string(" "));
				last_was_space = TRUE;
			}
		}
		else
		{
			std::string to_push = *token_iter;
			if (last_was_space)
			{
				tokens.back().append(to_push);
				last_was_space = FALSE;
			}
			else
			{
				tokens.push_back(to_push);
			}
		}
	}

	return tokens.size();
}

// static
std::string LLExternalEditor::findCommand(
	const std::string& env_var,
	const std::string& override)
{
	std::string cmd;

	// Get executable path.
	if (!override.empty())	// try the supplied override first
	{
		cmd = override;
		llinfos << "Using override" << llendl;
	}
	else if (!LLUI::sSettingGroups["config"]->getString(sSetting).empty())
	{
		cmd = LLUI::sSettingGroups["config"]->getString(sSetting);
		llinfos << "Using setting" << llendl;
	}
	else					// otherwise use the path specified by the environment variable
	{
		char* env_var_val = getenv(env_var.c_str());
		if (env_var_val)
		{
			cmd = env_var_val;
			llinfos << "Using env var " << env_var << llendl;
		}
	}

	llinfos << "Found command [" << cmd << "]" << llendl;
	return cmd;
}
