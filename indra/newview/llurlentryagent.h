/** 
 * @file llurlentryagent.h
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLURLENTRYAGENT_H
#define LL_LLURLENTRYAGENT_H

#include "llurlentry.h"

///
/// LLUrlEntryAgent Describes a Second Life agent Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/about
///
/// IDEVO Pulled this temporarily into newview for faster compile/link
/// times while iterating on UI.
class LLUrlEntryAgent : public LLUrlEntryBase
{
public:
	LLUrlEntryAgent();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
private:
	void onNameCache(const LLUUID& id, const std::string& full_name, bool is_group);
};

#endif
