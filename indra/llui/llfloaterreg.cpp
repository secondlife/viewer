/** 
 * @file llfloaterreg.cpp
 * @brief LLFloaterReg Floater Registration Class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"

//#include "llagent.h" 
#include "llfloater.h"
#include "llmultifloater.h"
#include "llfloaterreglistener.h"

//*******************************************************

//static
LLFloaterReg::instance_list_t LLFloaterReg::sNullInstanceList;
LLFloaterReg::instance_map_t LLFloaterReg::sInstanceMap;
LLFloaterReg::build_map_t LLFloaterReg::sBuildMap;
std::map<std::string,std::string> LLFloaterReg::sGroupMap;
bool LLFloaterReg::sBlockShowFloaters = false;

static LLFloaterRegListener sFloaterRegListener;

//*******************************************************

//static
void LLFloaterReg::add(const std::string& name, const std::string& filename, const LLFloaterBuildFunc& func, const std::string& groupname)
{
	sBuildMap[name].mFunc = func;
	sBuildMap[name].mFile = filename;
	sGroupMap[name] = groupname.empty() ? name : groupname;
	sGroupMap[groupname] = groupname; // for referencing directly by group name
}

//static
LLRect LLFloaterReg::getFloaterRect(const std::string& name)
{
	LLRect rect;
	const std::string& groupname = sGroupMap[name];
	if (!groupname.empty())
	{
		instance_list_t& list = sInstanceMap[groupname];
		if (!list.empty())
		{
			static LLUICachedControl<S32> floater_offset ("UIFloaterOffset", 16);
			LLFloater* last_floater = list.back();
			if (last_floater->getHost())
			{
				rect = last_floater->getHost()->getRect();
			}
			else
			{
				rect = last_floater->getRect();
			}
			rect.translate(floater_offset, -floater_offset);
		}
	}
	return rect;
}

//static
LLFloater* LLFloaterReg::findInstance(const std::string& name, const LLSD& key)
{
	LLFloater* res = NULL;
	const std::string& groupname = sGroupMap[name];
	if (!groupname.empty())
	{
		instance_list_t& list = sInstanceMap[groupname];
		for (instance_list_t::iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			LLFloater* inst = *iter;
			if (inst->matchesKey(key))
			{
				res = inst;
				break;
			}
		}
	}
	return res;
}

//static
LLFloater* LLFloaterReg::getInstance(const std::string& name, const LLSD& key) 
{
	LLFloater* res = findInstance(name, key);
	if (!res)
	{
		const LLFloaterBuildFunc& build_func = sBuildMap[name].mFunc;
		const std::string& xui_file = sBuildMap[name].mFile;
		if (build_func)
		{
			const std::string& groupname = sGroupMap[name];
			if (!groupname.empty())
			{
				instance_list_t& list = sInstanceMap[groupname];
				int index = list.size();

				res = build_func(key);
				
				bool success = LLUICtrlFactory::getInstance()->buildFloater(res, xui_file, NULL);
				if (!success)
				{
					llwarns << "Failed to build floater type: '" << name << "'." << llendl;
					return NULL;
				}
					
				// Note: key should eventually be a non optional LLFloater arg; for now, set mKey to be safe
				res->mKey = key;
				res->setInstanceName(name);
				res->applySavedVariables(); // Can't apply rect and dock state until setting instance name
				if (res->mAutoTile && !res->getHost() && index > 0)
				{
					const LLRect& cur_rect = res->getRect();
					LLRect next_rect = getFloaterRect(groupname);
					next_rect.setLeftTopAndSize(next_rect.mLeft, next_rect.mTop, cur_rect.getWidth(), cur_rect.getHeight());
					res->setRect(next_rect);
					res->setRectControl(LLStringUtil::null); // don't save rect of tiled floaters
					gFloaterView->adjustToFitScreen(res, true);
				}
				else
				{
					gFloaterView->adjustToFitScreen(res, false);
				}
				list.push_back(res);
			}
		}
		if (!res)
		{
			llwarns << "Floater type: '" << name << "' not registered." << llendl;
		}
	}
	return res;
}

//static
LLFloater* LLFloaterReg::removeInstance(const std::string& name, const LLSD& key)
{
	LLFloater* res = NULL;
	const std::string& groupname = sGroupMap[name];
	if (!groupname.empty())
	{
		instance_list_t& list = sInstanceMap[groupname];
		for (instance_list_t::iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			LLFloater* inst = *iter;
			if (inst->matchesKey(key))
			{
				res = inst;
				list.erase(iter);
				break;
			}
		}
	}
	return res;
}

//static
// returns true if the instance existed
bool LLFloaterReg::destroyInstance(const std::string& name, const LLSD& key)
{
	LLFloater* inst = removeInstance(name, key);
	if (inst)
	{
		delete inst;
		return true;
	}
	else
	{
		return false;
	}
}

// Iterators
//static
LLFloaterReg::const_instance_list_t& LLFloaterReg::getFloaterList(const std::string& name)
{
	instance_map_t::iterator iter = sInstanceMap.find(name);
	if (iter != sInstanceMap.end())
	{
		return iter->second;
	}
	else
	{
		return sNullInstanceList;
	}
}

// Visibility Management

//static
LLFloater* LLFloaterReg::showInstance(const std::string& name, const LLSD& key, BOOL focus) 
{
	if( sBlockShowFloaters )
		return 0;//
	LLFloater* instance = getInstance(name, key); 
	if (instance) 
	{
		instance->openFloater(key);
		if (focus)
			instance->setFocus(TRUE);
	}
	return instance;
}

//static
// returns true if the instance exists
bool LLFloaterReg::hideInstance(const std::string& name, const LLSD& key) 
{ 
	LLFloater* instance = findInstance(name, key); 
	if (instance)
	{
		// When toggling *visibility*, close the host instead of the floater when hosted
		if (instance->getHost())
			instance->getHost()->closeFloater();
		else
			instance->closeFloater();
		return true;
	}
	else
	{
		return false;
	}
}

//static
// returns true if the instance is visible when completed
bool LLFloaterReg::toggleInstance(const std::string& name, const LLSD& key)
{
	LLFloater* instance = findInstance(name, key); 
	if (LLFloater::isShown(instance))
	{
		// When toggling *visibility*, close the host instead of the floater when hosted
		if (instance->getHost())
			instance->getHost()->closeFloater();
		else
			instance->closeFloater();
		return false;
	}
	else
	{
		return showInstance(name, key, TRUE) ? true : false;
	}
}

//static
// returns true if the instance exists and is visible
bool LLFloaterReg::instanceVisible(const std::string& name, const LLSD& key)
{
	LLFloater* instance = findInstance(name, key); 
	return LLFloater::isShown(instance);
}

//static
void LLFloaterReg::showInitialVisibleInstances() 
{
	// Iterate through alll registered instance names and show any with a save visible state
	for (build_map_t::iterator iter = sBuildMap.begin(); iter != sBuildMap.end(); ++iter)
	{
		const std::string& name = iter->first;
		std::string controlname = getVisibilityControlName(name);
		if (LLUI::sSettingGroups["floater"]->controlExists(controlname))
		{
			BOOL isvis = LLUI::sSettingGroups["floater"]->getBOOL(controlname);
			if (isvis)
			{
				showInstance(name, LLSD()); // keyed floaters shouldn't set save_vis to true
			}
		}
	}
}

//static
void LLFloaterReg::hideVisibleInstances(const std::set<std::string>& exceptions)
{
	// Iterate through alll active instances and hide them
	for (instance_map_t::iterator iter = sInstanceMap.begin(); iter != sInstanceMap.end(); ++iter)
	{
		const std::string& name = iter->first;
		if (exceptions.find(name) != exceptions.end())
			continue;
		instance_list_t& list = iter->second;
		for (instance_list_t::iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			LLFloater* floater = *iter;
			floater->pushVisible(FALSE);
		}
	}
}

//static
void LLFloaterReg::restoreVisibleInstances()
{
	// Iterate through all active instances and restore visibility
	for (instance_map_t::iterator iter = sInstanceMap.begin(); iter != sInstanceMap.end(); ++iter)
	{
		instance_list_t& list = iter->second;
		for (instance_list_t::iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			LLFloater* floater = *iter;
			floater->popVisible();
		}
	}
}

//static
std::string LLFloaterReg::getRectControlName(const std::string& name)
{
	std::string res = std::string("floater_rect_") + name;
	LLStringUtil::replaceChar( res, ' ', '_' );
	return res;
}

//static
std::string LLFloaterReg::declareRectControl(const std::string& name)
{
	std::string controlname = getRectControlName(name);
	LLUI::sSettingGroups["floater"]->declareRect(controlname, LLRect(),
												 llformat("Window Position and Size for %s", name.c_str()),
												 TRUE);
	return controlname;
}

//static
std::string LLFloaterReg::getVisibilityControlName(const std::string& name)
{
	std::string res = std::string("floater_vis_") + name;
	LLStringUtil::replaceChar( res, ' ', '_' );
	return res;
}

//static
std::string LLFloaterReg::declareVisibilityControl(const std::string& name)
{
	std::string controlname = getVisibilityControlName(name);
	LLUI::sSettingGroups["floater"]->declareBOOL(controlname, FALSE,
												 llformat("Window Visibility for %s", name.c_str()),
												 TRUE);
	return controlname;
}

//static
std::string LLFloaterReg::declareDockStateControl(const std::string& name)
{
	std::string controlname = getDockStateControlName(name);
	LLUI::sSettingGroups["floater"]->declareBOOL(controlname, TRUE,
												 llformat("Window Docking state for %s", name.c_str()),
												 TRUE);
	return controlname;

}

//static
std::string LLFloaterReg::getDockStateControlName(const std::string& name)
{
	std::string res = std::string("floater_dock_") + name;
	LLStringUtil::replaceChar( res, ' ', '_' );
	return res;
}


//static
void LLFloaterReg::registerControlVariables()
{
	// Iterate through alll registered instance names and register rect and visibility control variables
	for (build_map_t::iterator iter = sBuildMap.begin(); iter != sBuildMap.end(); ++iter)
	{
		const std::string& name = iter->first;
		if (LLUI::sSettingGroups["floater"]->controlExists(getRectControlName(name)))
		{
			declareRectControl(name);
		}
		if (LLUI::sSettingGroups["floater"]->controlExists(getVisibilityControlName(name)))
		{
			declareVisibilityControl(name);
		}
	}
}

// Callbacks

// static
// Call once (i.e use for init callbacks)
void LLFloaterReg::initUICtrlToFloaterVisibilityControl(LLUICtrl* ctrl, const LLSD& sdname)
{
	// Get the visibility control name for the floater
	std::string vis_control_name = LLFloaterReg::declareVisibilityControl(sdname.asString());
	// Set the control value to the floater visibility control (Sets the value as well)
	ctrl->setControlVariable(LLUI::sSettingGroups["floater"]->getControl(vis_control_name));
}

// callback args may use "floatername.key" format
static void parse_name_key(std::string& name, LLSD& key)
{
	std::string instname = name;
	std::size_t dotpos = instname.find(".");
	if (dotpos != std::string::npos)
	{
		name = instname.substr(0, dotpos);
		key = LLSD(instname.substr(dotpos+1, std::string::npos));
	}
}

//static
void LLFloaterReg::showFloaterInstance(const LLSD& sdname)
{
	LLSD key;
	std::string name = sdname.asString();
	parse_name_key(name, key);
	showInstance(name, key, TRUE);
}
//static
void LLFloaterReg::hideFloaterInstance(const LLSD& sdname)
{
	LLSD key;
	std::string name = sdname.asString();
	parse_name_key(name, key);
	hideInstance(name, key);
}
//static
void LLFloaterReg::toggleFloaterInstance(const LLSD& sdname)
{
	LLSD key;
	std::string name = sdname.asString();
	parse_name_key(name, key);
	toggleInstance(name, key);
}

//static
bool LLFloaterReg::floaterInstanceVisible(const LLSD& sdname)
{
	LLSD key;
	std::string name = sdname.asString();
	parse_name_key(name, key);
	return instanceVisible(name, key);
}

