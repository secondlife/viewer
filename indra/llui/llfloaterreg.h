/** 
 * @file llfloaterreg.h
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
#ifndef LLFLOATERREG_H
#define LLFLOATERREG_H

/// llcommon
#include "llrect.h"
#include "llsd.h"

#include <boost/function.hpp>

//*******************************************************
//
// Floater Class Registry
//

class LLFloater;
class LLUICtrl;

typedef boost::function<LLFloater* (const LLSD& key)> LLFloaterBuildFunc;

class LLFloaterReg
{
public:
	// We use a list of LLFloater's instead of a set for two reasons:
	// 1) With a list we have a predictable ordering, useful for finding the last opened floater of a given type.
	// 2) We can change the key of a floater without altering the list.
	typedef std::list<LLFloater*> instance_list_t;
	typedef const instance_list_t const_instance_list_t;
	typedef std::map<std::string, instance_list_t> instance_map_t;

	struct BuildData
	{
		LLFloaterBuildFunc mFunc;
		std::string mFile;
	};
	typedef std::map<std::string, BuildData> build_map_t;
	
private:
	friend class LLFloaterRegListener;
	static instance_list_t sNullInstanceList;
	static instance_map_t sInstanceMap;
	static build_map_t sBuildMap;
	static std::map<std::string,std::string> sGroupMap;
	static bool sBlockShowFloaters;
	/**
	 * Defines list of floater names that can be shown despite state of sBlockShowFloaters.
	 */
	static std::set<std::string> sAlwaysShowableList;
	
public:
	// Registration
	
	// usage: LLFloaterClassRegistry::add("foo", (LLFloaterBuildFunc)&LLFloaterClassRegistry::build<LLFloaterFoo>);
	template <class T>
	static LLFloater* build(const LLSD& key)
	{
		T* floater = new T(key);
		return floater;
	}
	
	static void add(const std::string& name, const std::string& file, const LLFloaterBuildFunc& func,
					const std::string& groupname = LLStringUtil::null);

	// Helpers
	static LLFloater* getLastFloaterInGroup(const std::string& name);
	static LLFloater* getLastFloaterCascading();
	
	// Find / get (create) / remove / destroy
	static LLFloater* findInstance(const std::string& name, const LLSD& key = LLSD());
	static LLFloater* getInstance(const std::string& name, const LLSD& key = LLSD());
	static LLFloater* removeInstance(const std::string& name, const LLSD& key = LLSD());
	static bool destroyInstance(const std::string& name, const LLSD& key = LLSD());
	
	// Iterators
	static const_instance_list_t& getFloaterList(const std::string& name);

	// Visibility Management
	// return NULL if instance not found or can't create instance (no builder)
	static LLFloater* showInstance(const std::string& name, const LLSD& key = LLSD(), BOOL focus = FALSE);
	// Close a floater (may destroy or set invisible)
	// return false if can't find instance
	static bool hideInstance(const std::string& name, const LLSD& key = LLSD());
	// return true if instance is visible:
	static bool toggleInstance(const std::string& name, const LLSD& key = LLSD());
	static bool instanceVisible(const std::string& name, const LLSD& key = LLSD());

	static void showInitialVisibleInstances();
	static void hideVisibleInstances(const std::set<std::string>& exceptions = std::set<std::string>());
	static void restoreVisibleInstances();

	// Control Variables
	static std::string getRectControlName(const std::string& name);
	static std::string declareRectControl(const std::string& name);
	static std::string getVisibilityControlName(const std::string& name);
	static std::string declareVisibilityControl(const std::string& name);

	static std::string declareDockStateControl(const std::string& name);
	static std::string getDockStateControlName(const std::string& name);

	static void registerControlVariables();

	// Callback wrappers
	static void toggleInstanceOrBringToFront(const LLSD& sdname, const LLSD& key = LLSD());
	
	// Typed find / get / show
	template <class T>
	static T* findTypedInstance(const std::string& name, const LLSD& key = LLSD())
	{
		return dynamic_cast<T*>(findInstance(name, key));
	}

	template <class T>
	static T* getTypedInstance(const std::string& name, const LLSD& key = LLSD())
	{
		return dynamic_cast<T*>(getInstance(name, key));
	}

	template <class T>
	static T* showTypedInstance(const std::string& name, const LLSD& key = LLSD(), BOOL focus = FALSE)
	{
		return dynamic_cast<T*>(showInstance(name, key, focus));
	}

	static void blockShowFloaters(bool value) { sBlockShowFloaters = value;}
	
	static U32 getVisibleFloaterInstanceCount();
};

#endif
