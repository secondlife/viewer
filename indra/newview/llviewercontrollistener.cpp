/**
 * @file   llviewercontrollistener.cpp
 * @author Brad Kittenbrink
 * @date   2009-07-09
 * @brief  Implementation for llviewercontrollistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewercontrollistener.h"

#include "llviewercontrol.h"
#include "llcontrol.h"
#include "llerror.h"
#include "llsdutil.h"
#include "stringize.h"
#include <sstream>

namespace {

LLViewerControlListener sSavedSettingsListener;

} // unnamed namespace

LLViewerControlListener::LLViewerControlListener()
	: LLEventAPI("LLViewerControl",
				 "LLViewerControl listener: set, toggle or set default for various controls")
{
	std::ostringstream groupnames;
	groupnames << "[\"group\"] is one of ";
	const char* delim = "";
	for (LLControlGroup::key_iter cgki(LLControlGroup::beginKeys()),
								  cgkend(LLControlGroup::endKeys());
		 cgki != cgkend; ++cgki)
	{
		groupnames << delim << '"' << *cgki << '"';
		delim = ", ";
	}
	groupnames << '\n';
	std::string grouphelp(groupnames.str());
	std::string replyhelp("If [\"reply\"] requested, send new [\"value\"] on specified LLEventPump\n");

	add("set",
		std::string("Set [\"group\"] control [\"key\"] to optional value [\"value\"]\n"
					"If [\"value\"] omitted, set to control's defined default value\n") +
		grouphelp + replyhelp,
		&LLViewerControlListener::set,
		LLSDMap("group", LLSD())("key", LLSD()));
	add("toggle",
		std::string("Toggle [\"group\"] control [\"key\"], if boolean\n") + grouphelp + replyhelp,
		&LLViewerControlListener::toggle,
		LLSDMap("group", LLSD())("key", LLSD()));
	add("get",
		std::string("Query [\"group\"] control [\"key\"], replying on LLEventPump [\"reply\"]\n") +
		grouphelp,
		&LLViewerControlListener::get,
		LLSDMap("group", LLSD())("key", LLSD())("reply", LLSD()));
	add("groups",
		"Send on LLEventPump [\"reply\"] an array [\"groups\"] of valid group names",
		&LLViewerControlListener::groups,
		LLSDMap("reply", LLSD()));
	add("vars",
		std::string("For [\"group\"], send on LLEventPump [\"reply\"] an array [\"vars\"],\n"
					"each of whose entries looks like:\n"
					"  [\"name\"], [\"type\"], [\"value\"], [\"comment\"]\n") + grouphelp,
		&LLViewerControlListener::vars,
		LLSDMap("group", LLSD())("reply", LLSD()));
}

struct Info
{
	Info(const LLSD& request):
		response(LLSD(), request),
		groupname(request["group"]),
		group(LLControlGroup::getInstance(groupname)),
		key(request["key"]),
		control(NULL)
	{
		if (! group)
		{
			response.error(STRINGIZE("Unrecognized group '" << groupname << "'"));
			return;
		}

		control = group->getControl(key);
		if (! control)
		{
			response.error(STRINGIZE("In group '" << groupname
									 << "', unrecognized control key '" << key << "'"));
		}
	}

	~Info()
	{
		// If in fact the request passed to our constructor names a valid
		// group and key, grab the final value of the indicated control and
		// stuff it in our response. Since this outer destructor runs before
		// the contained Response destructor, this data will go into the
		// response we send.
		if (control)
		{
			response["name"]	= control->getName();
			response["type"]	= group->typeEnumToString(control->type());
			response["value"]	= control->get();
			response["comment"] = control->getComment();
		}
	}

	LLEventAPI::Response response;
	std::string groupname;
	LLControlGroup* group;
	std::string key;
	LLControlVariable* control;
};

//static
void LLViewerControlListener::set(LLSD const & request)
{
	Info info(request);
	if (! info.control)
		return;

	if (request.has("value"))
	{
		info.control->setValue(request["value"]);
	}
	else
	{
		info.control->resetToDefault();
	}
}

//static
void LLViewerControlListener::toggle(LLSD const & request)
{
	Info info(request);
	if (! info.control)
		return;

	if (info.control->isType(TYPE_BOOLEAN))
	{
		info.control->set(! info.control->get().asBoolean());
	}
	else
	{
		info.response.error(STRINGIZE("toggle of non-boolean '" << info.groupname
									  << "' control '" << info.key
									  << "', type is "
									  << info.group->typeEnumToString(info.control->type())));
	}
}

void LLViewerControlListener::get(LLSD const & request)
{
	// The Info constructor and destructor actually do all the work here.
	Info info(request);
}

void LLViewerControlListener::groups(LLSD const & request)
{
	// No Info, we're not looking up either a group or a control name.
	Response response(LLSD(), request);
	for (LLControlGroup::key_iter cgki(LLControlGroup::beginKeys()),
								  cgkend(LLControlGroup::endKeys());
		 cgki != cgkend; ++cgki)
	{
		response["groups"].append(*cgki);
	}
}

struct CollectVars: public LLControlGroup::ApplyFunctor
{
	CollectVars(LLControlGroup* g):
		mGroup(g)
	{}

	virtual void apply(const std::string& name, LLControlVariable* control)
	{
		vars.append(LLSDMap
					("name", name)
					("type", mGroup->typeEnumToString(control->type()))
					("value", control->get())
					("comment", control->getComment()));
	}

	LLControlGroup* mGroup;
	LLSD vars;
};

void LLViewerControlListener::vars(LLSD const & request)
{
	// This method doesn't use Info, because we're not looking up a specific
	// control name.
	Response response(LLSD(), request);
	std::string groupname(request["group"]);
	LLControlGroup* group(LLControlGroup::getInstance(groupname));
	if (! group)
	{
		return response.error(STRINGIZE("Unrecognized group '" << groupname << "'"));
	}

	CollectVars collector(group);
	group->applyToAll(&collector);
	response["vars"] = collector.vars;
}
