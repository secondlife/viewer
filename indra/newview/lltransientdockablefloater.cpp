/** 
 * @file lltransientdockablefloater.cpp
 * @brief Creates a panel of a specific kind for a toast
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

#include "llviewerprecompiledheaders.h"

#include "lltransientfloatermgr.h"
#include "lltransientdockablefloater.h"
#include "llfloaterreg.h"


LLTransientDockableFloater::LLTransientDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
		const LLSD& key, const Params& params) :
		LLDockableFloater(dockControl, uniqueDocking, key, params)
{
	LLTransientFloaterMgr::getInstance()->registerTransientFloater(this);
	LLTransientFloater::init(this);
}

LLTransientDockableFloater::~LLTransientDockableFloater()
{
	LLTransientFloaterMgr::getInstance()->unregisterTransientFloater(this);
	LLView* dock = getDockWidget();
	LLTransientFloaterMgr::getInstance()->removeControlView(
			LLTransientFloaterMgr::DOCKED, this);
	if (dock != NULL)
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(
				LLTransientFloaterMgr::DOCKED, dock);
	}
}

void LLTransientDockableFloater::setVisible(BOOL visible)
{
	LLView* dock = getDockWidget();
	if(visible && isDocked())
	{
		LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, this);
		if (dock != NULL)
		{
			LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, dock);
		}
	}
	else
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, this);
		if (dock != NULL)
		{
			LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, dock);
		}
	}

	LLDockableFloater::setVisible(visible);
}

void LLTransientDockableFloater::setDocked(bool docked, bool pop_on_undock)
{
	LLView* dock = getDockWidget();
	if(docked)
	{
		LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, this);
		if (dock != NULL)
		{
			LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, dock);
		}
	}
	else
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, this);
		if (dock != NULL)
		{
			LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, dock);
		}
	}

	LLDockableFloater::setDocked(docked, pop_on_undock);
}
