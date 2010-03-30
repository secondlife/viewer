/** 
 * @file llfloaterreg.h
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
#ifndef LLFLOATERREG_H
#define LLFLOATERREG_H

/// llcommon
#include "llboost.h"
#include "llrect.h"
#include "llstl.h"
#include "llsd.h"

/// llui
#include "lluictrl.h"

#include <boost/function.hpp>

//*******************************************************
//
// Floater Class Registry
//

class LLFloater;

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
	static LLRect getFloaterRect(const std::string& name);
	
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
	static void initUICtrlToFloaterVisibilityControl(LLUICtrl* ctrl, const LLSD& sdname);
	static void showFloaterInstance(const LLSD& sdname);
	static void hideFloaterInstance(const LLSD& sdname);
	static void toggleFloaterInstance(const LLSD& sdname);
	static bool floaterInstanceVisible(const LLSD& sdname);
	static bool floaterInstanceMinimized(const LLSD& sdname);
	
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
	
};

#endif
