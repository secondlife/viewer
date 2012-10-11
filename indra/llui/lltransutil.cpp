/**
 * @file lltrans.cpp
 * @brief LLTrans implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltransutil.h"

#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llxmlnode.h"
#include "lldir.h"

bool LLTransUtil::parseStrings(const std::string& xml_filename, const std::set<std::string>& default_args)
{
	// LLUICtrlFactory::getLayeredXMLNode() just calls
	// gDirUtilp->findSkinnedFilenames(constraint=LLDir::CURRENT_SKIN) and
	// then passes the resulting paths to LLXMLNode::getLayeredXMLNode().
	// Bypass that and call LLXMLNode::getLayeredXMLNode() directly: we want
	// constraint=LLDir::ALL_SKINS.
	std::vector<std::string> paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, xml_filename, LLDir::ALL_SKINS);
	if (paths.empty())
	{
		// xml_filename not found at all in any skin -- check whether entire
		// path was passed (but I hope we no longer have callers who do that)
		paths.push_back(xml_filename);
	}
	LLXMLNodePtr root;
	bool success = LLXMLNode::getLayeredXMLNode(root, paths);
	if (!success)
	{
		llerrs << "Couldn't load string table " << xml_filename << llendl;
		return false;
	}

	return LLTrans::parseStrings(root, default_args);
}


bool LLTransUtil::parseLanguageStrings(const std::string& xml_filename)
{
	LLXMLNodePtr root;
	BOOL success  = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);
	
	if (!success)
	{
		llerrs << "Couldn't load localization table " << xml_filename << llendl;
		return false;
	}
	
	return LLTrans::parseLanguageStrings(root);
}
