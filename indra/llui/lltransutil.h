/**
 * @file lltransutil.h
 * @brief LLTrans helper
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

#ifndef LL_TRANSUTIL_H
#define LL_TRANSUTIL_H

#include "lltrans.h"

namespace LLTransUtil
{
	/**
	 * @brief Parses the xml file that holds the strings. Used once on startup
	 * @param xml_filename Filename to parse
	 * @param default_args Set of strings (expected to be in the file) to use as default replacement args, e.g. "SECOND_LIFE"
	 * @returns true if the file was parsed successfully, true if something went wrong
	 */
	bool parseStrings(const std::string& xml_filename, const std::set<std::string>& default_args);

	bool parseLanguageStrings(const std::string& xml_filename);
};

#endif
