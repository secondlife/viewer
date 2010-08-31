/**
 * @file lltrans.cpp
 * @brief LLTrans implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lltrans.h"
#include "lluictrlfactory.h"

#include "lltransutil.h"


bool LLTransUtil::parseStrings(const std::string& xml_filename, const std::set<std::string>& default_args)
{
	LLXMLNodePtr root;
	BOOL success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);
	if (!success)
	{
		llerrs << "Couldn't load string table" << llendl;
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
		llerrs << "Couldn't load string table " << xml_filename << llendl;
		return false;
	}
	
	return LLTrans::parseLanguageStrings(root);
}
