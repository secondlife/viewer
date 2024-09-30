/**
 * @file llfloaterreg.cpp
 * @brief LLFloaterReg Floater Registration Class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloaterreg.h"

//#include "llagent.h"
#include "llfloater.h"
#include "llmultifloater.h"
#include "llfloaterreglistener.h"
#include "lluiusage.h"

//*******************************************************

//static
LLFloaterReg::instance_list_t LLFloaterReg::sNullInstanceList;
LLFloaterReg::instance_map_t LLFloaterReg::sInstanceMap;
LLFloaterReg::build_map_t LLFloaterReg::sBuildMap;
std::map<std::string, std::string, std::less<>> LLFloaterReg::sGroupMap;
bool LLFloaterReg::sBlockShowFloaters = false;
std::set<std::string, std::less<>> LLFloaterReg::sAlwaysShowableList;

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
bool LLFloaterReg::isRegistered(std::string_view name)
{
    return sBuildMap.find(name) != sBuildMap.end();
}

//static
LLFloater* LLFloaterReg::getLastFloaterInGroup(std::string_view name)
{
    auto it = sGroupMap.find(name);
    if (it != sGroupMap.end())
    {
        const std::string& groupname = it->second;
        if (!groupname.empty())
        {
            instance_list_t& list = sInstanceMap[groupname];
            if (!list.empty())
            {
                for (instance_list_t::reverse_iterator iter = list.rbegin(), end = list.rend(); iter != end; ++iter)
                {
                    LLFloater* inst = *iter;

                    if (inst->getVisible() && !inst->isMinimized())
                    {
                        return inst;
                    }
                }
            }
        }
    }
    return NULL;
}

LLFloater* LLFloaterReg::getLastFloaterCascading()
{
    LLRect candidate_rect;
    candidate_rect.mTop = 100000;
    LLFloater* candidate_floater = NULL;

    std::map<std::string,std::string>::const_iterator it = sGroupMap.begin(), it_end = sGroupMap.end();
    for( ; it != it_end; ++it)
    {
        const std::string& group_name = it->second;

        instance_list_t& instances = sInstanceMap[group_name];

        for (LLFloater* inst : instances)
        {
            if (inst->getVisible()
                && (inst->isPositioning(LLFloaterEnums::POSITIONING_CASCADING)
                    || inst->isPositioning(LLFloaterEnums::POSITIONING_CASCADE_GROUP)))
            {
                if (candidate_rect.mTop > inst->getRect().mTop)
                {
                    candidate_floater = inst;
                    candidate_rect = inst->getRect();
                }
            }
        }
    }

    return candidate_floater;
}

//static
LLFloater* LLFloaterReg::findInstance(std::string_view name, const LLSD& key)
{
    LLFloater* res = NULL;
    auto it = sGroupMap.find(name);
    if (it != sGroupMap.end())
    {
        const std::string& groupname = it->second;
        if (!groupname.empty())
        {
            instance_list_t& list = sInstanceMap[groupname];
            for (LLFloater* inst : list)
            {
                if (inst->matchesKey(key))
                {
                    res = inst;
                    break;
                }
            }
        }
    }
    return res;
}

//static
LLFloater* LLFloaterReg::getInstance(std::string_view name, const LLSD& key)
{
    LLFloater* res = findInstance(name, key);
    if (!res)
    {
        auto it = sBuildMap.find(name);
        if (it != sBuildMap.end())
        {
            const LLFloaterBuildFunc& build_func = it->second.mFunc;
            const std::string& xui_file = it->second.mFile;
            if (build_func)
            {
                auto it = sGroupMap.find(name);
                if (it != sGroupMap.end())
                {
                    const std::string& groupname = it->second;
                    if (!groupname.empty())
                    {
                        instance_list_t& list = sInstanceMap[groupname];

                        res = build_func(key);
                        if (!res)
                        {
                            LL_WARNS() << "Failed to build floater type: '" << name << "'." << LL_ENDL;
                            return NULL;
                        }
                        bool success = res->buildFromFile(xui_file);
                        if (!success)
                        {
                            LL_WARNS() << "Failed to build floater type: '" << name << "'." << LL_ENDL;
                            return NULL;
                        }

                        // Note: key should eventually be a non optional LLFloater arg; for now, set mKey to be safe
                        if (res->mKey.isUndefined())
                        {
                            res->mKey = key;
                        }
                        res->setInstanceName(std::string(name));

                        LLFloater* last_floater = (list.empty() ? NULL : list.back());

                        res->applyControlsAndPosition(last_floater);

                        gFloaterView->adjustToFitScreen(res, false);

                        list.push_back(res);
                    }
                }
            }
        }
        if (!res)
        {
            LL_WARNS() << "Floater type: '" << name << "' not registered." << LL_ENDL;
        }
    }
    return res;
}

//static
LLFloater* LLFloaterReg::removeInstance(std::string_view name, const LLSD& key)
{
    LLFloater* res = NULL;
    auto it = sGroupMap.find(name);
    if (it != sGroupMap.end())
    {
        const std::string& groupname = it->second;
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
    }
    return res;
}

//static
// returns true if the instance existed
bool LLFloaterReg::destroyInstance(std::string_view name, const LLSD& key)
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
LLFloaterReg::const_instance_list_t& LLFloaterReg::getFloaterList(std::string_view name)
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
LLFloater* LLFloaterReg::showInstance(std::string_view name, const LLSD& key, bool focus)
{
    if( sBlockShowFloaters
            // see EXT-7090
            && sAlwaysShowableList.find(name) == sAlwaysShowableList.end())
        return 0;//
    LLFloater* instance = getInstance(name, key);
    if (instance)
    {
        instance->openFloater(key);
        if (focus)
            instance->setFocus(true);
    }
    return instance;
}

//static
// returns true if the instance exists
bool LLFloaterReg::hideInstance(std::string_view name, const LLSD& key)
{
    LLFloater* instance = findInstance(name, key);
    if (instance)
    {
        instance->closeHostedFloater();
    }
    return (instance != NULL);
}

//static
// returns true if the instance is visible when completed
bool LLFloaterReg::toggleInstance(std::string_view name, const LLSD& key)
{
    LLFloater* instance = findInstance(name, key);
    if (instance && instance->isShown())
    {
        instance->closeHostedFloater();
        return false;
    }

    instance = showInstance(name, key, true);

    return instance != nullptr;
}

//static
// returns true if the instance exists and is visible (doesnt matter minimized or not)
bool LLFloaterReg::instanceVisible(std::string_view name, const LLSD& key)
{
    LLFloater* instance = findInstance(name, key);
    return LLFloater::isVisible(instance);
}

//static
void LLFloaterReg::showInitialVisibleInstances()
{
    // Iterate through alll registered instance names and show any with a save visible state
    for (build_map_t::iterator iter = sBuildMap.begin(); iter != sBuildMap.end(); ++iter)
    {
        const std::string& name = iter->first;
        std::string controlname = getVisibilityControlName(name);
        if (LLFloater::getControlGroup()->controlExists(controlname))
        {
            bool isvis = LLFloater::getControlGroup()->getBOOL(controlname);
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
            floater->pushVisible(false);
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
    return std::string("floater_rect_") + getBaseControlName(name);
}

//static
std::string LLFloaterReg::declareRectControl(const std::string& name)
{
    std::string controlname = getRectControlName(name);
    LLFloater::getControlGroup()->declareRect(controlname, LLRect(),
                                              llformat("Window Size for %s", name.c_str()),
                                              LLControlVariable::PERSIST_NONDFT);
    return controlname;
}

std::string LLFloaterReg::declarePosXControl(const std::string& name)
{
    std::string controlname = std::string("floater_pos_") + getBaseControlName(name) + "_x";
    LLFloater::getControlGroup()->declareF32(controlname,
                                            10.f,
                                            llformat("Window X Position for %s", name.c_str()),
                                            LLControlVariable::PERSIST_NONDFT);
    return controlname;
}

std::string LLFloaterReg::declarePosYControl(const std::string& name)
{
    std::string controlname = std::string("floater_pos_") + getBaseControlName(name) + "_y";
    LLFloater::getControlGroup()->declareF32(controlname,
                                            10.f,
                                            llformat("Window Y Position for %s", name.c_str()),
                                            LLControlVariable::PERSIST_NONDFT);

    return controlname;
}


//static
std::string LLFloaterReg::getVisibilityControlName(const std::string& name)
{
    return std::string("floater_vis_") + getBaseControlName(name);
}

//static
std::string LLFloaterReg::getBaseControlName(const std::string& name)
{
    std::string res(name);
    LLStringUtil::replaceChar( res, ' ', '_' );
    return res;
}


//static
std::string LLFloaterReg::declareVisibilityControl(const std::string& name)
{
    std::string controlname = getVisibilityControlName(name);
    LLFloater::getControlGroup()->declareBOOL(controlname, false,
                                                 llformat("Window Visibility for %s", name.c_str()),
                                                 LLControlVariable::PERSIST_NONDFT);
    return controlname;
}

//static
std::string LLFloaterReg::declareDockStateControl(const std::string& name)
{
    std::string controlname = getDockStateControlName(name);
    LLFloater::getControlGroup()->declareBOOL(controlname, true,
                                                 llformat("Window Docking state for %s", name.c_str()),
                                                 LLControlVariable::PERSIST_NONDFT);
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
        if (LLFloater::getControlGroup()->controlExists(getRectControlName(name)))
        {
            declareRectControl(name);
        }
        if (LLFloater::getControlGroup()->controlExists(getVisibilityControlName(name)))
        {
            declareVisibilityControl(name);
        }
    }

    const LLSD& exclude_list = LLUI::getInstance()->mSettingGroups["config"]->getLLSD("always_showable_floaters");
    for (LLSD::array_const_iterator iter = exclude_list.beginArray();
        iter != exclude_list.endArray();
        iter++)
    {
        sAlwaysShowableList.insert(iter->asString());
    }
}

//static
void LLFloaterReg::toggleInstanceOrBringToFront(const LLSD& sdname, const LLSD& key)
{
    //
    // Floaters controlled by the toolbar behave a bit differently from others.
    // Namely they have 3-4 states as defined in the design wiki page here:
    //   https://wiki.lindenlab.com/wiki/FUI_Button_states
    //
    // The basic idea is this:
    // * If the target floater is minimized, this button press will un-minimize it.
    // * Else if the target floater is closed open it.
    // * Else if the target floater does not have focus, give it focus.
    //       * Also, if it is not on top, bring it forward when focus is given.
    // * Else the target floater is open, close it.
    //
    std::string name = sdname.asString();
    LLFloater* instance = getInstance(name, key);

    if (!instance)
    {
        LL_DEBUGS() << "Unable to get instance of floater '" << name << "'" << LL_ENDL;
        return;
    }

    // If hosted, we need to take that into account
    LLFloater* host = instance->getHost();

    if (host)
    {
        if (host->isMinimized() || !host->isShown() || !host->isFrontmost())
        {
            host->setMinimized(false);
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->getVisible())
        {
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
            instance->setFocus(true);
        }
        else
        {
            instance->closeHostedFloater();
        }
    }
    else
    {
        if (instance->isMinimized())
        {
            instance->setMinimized(false);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->isShown())
        {
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->isFrontmost())
        {
            instance->setVisibleAndFrontmost(true, key);
        }
        else
        {
            instance->closeHostedFloater();
        }
    }
}

// static
// Same as toggleInstanceOrBringToFront but does not close floater.
// unlike showInstance() does not trigger onOpen() if already open
void LLFloaterReg::showInstanceOrBringToFront(const LLSD& sdname, const LLSD& key)
{
    std::string name = sdname.asString();
    LLFloater* instance = getInstance(name, key);


    if (!instance)
    {
        LL_DEBUGS() << "Unable to get instance of floater '" << name << "'" << LL_ENDL;
        return;
    }

    // If hosted, we need to take that into account
    LLFloater* host = instance->getHost();

    if (host)
    {
        if (host->isMinimized() || !host->isShown() || !host->isFrontmost())
        {
            host->setMinimized(false);
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->getVisible())
        {
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
            instance->setFocus(true);
        }
    }
    else
    {
        if (instance->isMinimized())
        {
            instance->setMinimized(false);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->isShown())
        {
            instance->openFloater(key);
            instance->setVisibleAndFrontmost(true, key);
        }
        else if (!instance->isFrontmost())
        {
            instance->setVisibleAndFrontmost(true, key);
        }
    }
}

// static
U32 LLFloaterReg::getVisibleFloaterInstanceCount()
{
    U32 count = 0;

    std::map<std::string,std::string>::const_iterator it = sGroupMap.begin(), it_end = sGroupMap.end();
    for( ; it != it_end; ++it)
    {
        const std::string& group_name = it->second;

        instance_list_t& instances = sInstanceMap[group_name];

        for (instance_list_t::const_iterator iter = instances.begin(); iter != instances.end(); ++iter)
        {
            LLFloater* inst = *iter;

            if (inst->getVisible() && !inst->isMinimized())
            {
                count++;
            }
        }
    }

    return count;
}

LLSD LLFloaterReg::getFloaterNames()
{
    return llsd::toArray(sGroupMap, [](const auto &pair) { return pair.first; });
}
