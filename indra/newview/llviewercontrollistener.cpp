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
	groupnames << "\n[\"group\"] is one of ";
	const char* delim = "";
	for (const auto& key : LLControlGroup::key_snapshot())
	{
		groupnames << delim << '"' << key << '"';
		delim = ", ";
	}
	groupnames << "; \"Global\" is gSavedSettings";
	std::string grouphelp(groupnames.str());
	std::string replyhelp("\nIf [\"reply\"] requested, send new [\"value\"] on specified LLEventPump");
	std::string infohelp("\nReply contains [\"group\"], [\"name\"], [\"type\"], [\"value\"], [\"comment\"]");

	add("set",
		"Set [\"group\"] control [\"key\"] to optional value [\"value\"]\n"
		"If [\"value\"] omitted, set to control's defined default value" +
		grouphelp + replyhelp + infohelp,
		&LLViewerControlListener::set,
		LLSDMap("group", LLSD())("key", LLSD()));
	add("toggle",
		"Toggle [\"group\"] control [\"key\"], if boolean" + grouphelp + replyhelp + infohelp,
		&LLViewerControlListener::toggle,
		LLSDMap("group", LLSD())("key", LLSD()));
	add("get",
		"Query [\"group\"] control [\"key\"], replying on LLEventPump [\"reply\"]" +
		grouphelp + infohelp,
		&LLViewerControlListener::get,
		LLSDMap("group", LLSD())("key", LLSD())("reply", LLSD()));
	add("monitor",
		"Register to post [\"group\"] control [\"key\"]'s value on pump [\"pump\"]\n"
		"immediately and on every subsequent change -- reply includes [\"pump\"] name,\n"
		"which may be different than requested if [\"key\"] is already being monitored\n"
		"on that other LLEventPump" + grouphelp + replyhelp + infohelp,
		&LLViewerControlListener::monitor,
		llsd::map("group", LLSD(), "key", LLSD(), "pump", LLSD(), "reply", LLSD()));
	add("groups",
		"Send on LLEventPump [\"reply\"] an array [\"groups\"] of valid group names",
		&LLViewerControlListener::groups,
		LLSDMap("reply", LLSD()));
	add("vars",
		"For [\"group\"], send on LLEventPump [\"reply\"] an array [\"vars\"],\n"
		"each of whose entries looks like:\n"
		"  [\"name\"], [\"type\"], [\"value\"], [\"comment\"]" + grouphelp,
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
			response["group"]	= groupname;
			response["name"]	= control->getName();
			response["type"]	= LLControlGroup::typeEnumToString(control->type());
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
									  << LLControlGroup::typeEnumToString(info.control->type())));
	}
}

void LLViewerControlListener::get(LLSD const & request)
{
	// The Info constructor and destructor actually do all the work here.
	Info info(request);
}

void LLViewerControlListener::monitor(const LLSD& request)
{
    // reply on caller's "reply" LLEventPump as well as on the requested "pump"
    Info info(request);
    if (! info.control)
        return;

    LLReqID reqID(request);

    // Find or create the entry. Since any number of listeners may be
    // listening on a given LLEventPump, it makes no sense at all to monitor a
    // given LLControlVariable on more than one LLEventPump. Once someone has
    // asked to monitor that LLControlVariable, all subsequent "monitor"
    // requests for the same LLControlVariable will return the "pump" on which
    // it's already being monitored.
    // The fact that "monitor" events send full Info data means that multiple
    // LLControlVariables, even from different LLControlGroups, may be
    // monitored by a fairly generic listener.
    auto entry = mMonitorMap.emplace(MonitorMap::key_type(info.groupname, info.key),
                                     request["pump"]);
    // Whether we found or created that entry, tell caller its pump name.
    std::string pump(entry.first->second);
    info.response["pump"] = pump;
    // Did we just insert a brand-new entry?
    if (entry.second)
    {
        // then info.control is not yet being monitored.
        auto signal{ info.control->getCommitSignal() };
        // The commit signal fires every time this LLControlVariable's value
        // changes. Bind a lambda, with the right signature, to relay that
        // notification to our monitor LLEventPump.
        // We could choose to capture the connection returned by connect(),
        // but as of now we see no need for an "unmonitor" operation.
        // Important: since this lambda will outlive this function, bind
        // variables by VALUE, not by reference.
        signal->connect(
            [reqID, pump, group=info.groupname]
            (LLControlVariable* control, const LLSD& value, const LLSD& previous)
            {
                // We could bind a reference to the LLEventPumps LLSingleton
                // instance -- but who knows how late in the process people
                // may still be changing monitored LLControlVariables? Will we
                // have already deleted that LLEventPumps instance?
                // Similarly, although we could bind a reference to the
                // LLEventPump returned by obtain(), how do we know whether that's
                // a temporary instance? Re-acquire both every time. We
                // don't expect LLControlVariable changes on anything like an
                // inner-loop time scale.
                // That said, although it would be easy enough to instantiate
                // an Info every time we get called, we've already done the
                // lookup for the control group and the control variable -- so
                // fake up the Info results. Besides, doing it by hand allows
                // us to inject the previous value, which Info can't access.
                // Use LLReqID to associate every response with the original
                // request, in case client is keeping track.
                LLEventPumps::instance().obtain(pump).post(llsd::map(
                    "reqid",    reqID.getReqID(),
                    "group",    group,
                    "name",     control->getName(),
                    "type",     LLControlGroup::typeEnumToString(control->type()),
                    "value",    value,
                    "comment",  control->getComment(),
                    "previous", previous));
            });            
    }

    // Caller may have specified the same LLEventPump for "reply" and "pump",
    // in which case we're done.
    if (pump != request["reply"].asString())
    {
        // Two different pumps: as advertised, post the current value
        // immediately on "pump" as well as on "reply".
        LLSD dummyRequest(request);
        dummyRequest["reply"] = pump;
        Info imm(dummyRequest);
    }
}

void LLViewerControlListener::groups(LLSD const & request)
{
	// No Info, we're not looking up either a group or a control name.
	Response response(LLSD(), request);
	for (const auto& key : LLControlGroup::key_snapshot())
	{
		response["groups"].append(key);
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
					("type", LLControlGroup::typeEnumToString(control->type()))
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
