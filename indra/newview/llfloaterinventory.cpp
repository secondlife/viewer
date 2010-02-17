/** 
 * @file llfloaterinventory.cpp
 * @brief Implementation of the inventory view and associated stuff.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llfloaterinventory.h"

#include "llagent.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llinventorymodel.h"
#include "llpanelmaininventory.h"
#include "llresmgr.h"
#include "llviewerfoldertype.h"
#include "lltransientfloatermgr.h"

///----------------------------------------------------------------------------
/// LLFloaterInventory
///----------------------------------------------------------------------------

LLFloaterInventory::LLFloaterInventory(const LLSD& key)
	: LLFloater(key)
{
	LLTransientFloaterMgr::getInstance()->addControlView(this);
}

LLFloaterInventory::~LLFloaterInventory()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

BOOL LLFloaterInventory::postBuild()
{
	mPanelMainInventory = getChild<LLPanelMainInventory>("Inventory Panel");
	return TRUE;
}


void LLFloaterInventory::draw()
{
	updateTitle();
	LLFloater::draw();
}

void LLFloaterInventory::updateTitle()
{
	LLLocale locale(LLLocale::USER_LOCALE);
	std::string item_count_string;
	LLResMgr::getInstance()->getIntegerString(item_count_string, gInventory.getItemCount());

	LLStringUtil::format_map_t string_args;
	string_args["[ITEM_COUNT]"] = item_count_string;
	string_args["[FILTER]"] = mPanelMainInventory->getFilterText();

	if (LLInventoryModel::backgroundFetchActive())
	{
		setTitle(getString("TitleFetching", string_args));
	}
	else if (LLInventoryModel::isEverythingFetched())
	{
		setTitle(getString("TitleCompleted", string_args));
	}
	else
	{
		setTitle(getString("Title"));
	}
}

void LLFloaterInventory::changed(U32 mask)
{
	updateTitle();
}

LLInventoryPanel* LLFloaterInventory::getPanel()
{
	if (mPanelMainInventory)
		return mPanelMainInventory->getPanel();
	return NULL;
}

// static
LLFloaterInventory* LLFloaterInventory::showAgentInventory()
{
	// Hack to generate semi-unique key for each inventory floater.
	static S32 instance_num = 0;
	instance_num = (instance_num + 1) % S32_MAX;

	LLFloaterInventory* iv = NULL;
	if (!gAgent.cameraMouselook())
	{
		iv = LLFloaterReg::showTypedInstance<LLFloaterInventory>("inventory", LLSD(instance_num));
	}
	return iv;
}

// static
void LLFloaterInventory::cleanup()
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end();)
	{
		LLFloaterInventory* iv = dynamic_cast<LLFloaterInventory*>(*iter++);
		if (iv)
		{
			iv->destroy();
		}
	}
}

void LLFloaterInventory::onOpen(const LLSD& key)
{
	//LLFirstUse::useInventory();
}
