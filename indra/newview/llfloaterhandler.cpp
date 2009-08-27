/** 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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
#include "llviewerprecompiledheaders.h"

#include "llfloaterhandler.h"

#include "llfloater.h"
#include "llmediactrl.h"

// register with dispatch via global object
LLFloaterHandler gFloaterHandler;


LLFloater* get_parent_floater(LLView* view)
{
	LLFloater* floater = NULL;
	LLView* parent = view->getParent();
	while (parent)
	{
		floater = dynamic_cast<LLFloater*>(parent);
		if (floater)
		{
			break;
		}
		parent = parent->getParent();
	}
	return floater;
}


bool LLFloaterHandler::handle(const LLSD &params, const LLSD &query_map, LLMediaCtrl *web)
{
	if (params.size() < 2) return false;
	LLFloater* floater = NULL;
	// *TODO: implement floater lookup by name
	if (params[0].asString() == "self")
	{
		if (web)
		{
			floater = get_parent_floater(web);
		}
	}
	if (params[1].asString() == "close")
	{
		if (floater)
		{
			floater->closeFloater();
			return true;
		}
	}
	return false;
}
