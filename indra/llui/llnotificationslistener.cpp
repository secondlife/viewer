/**
 * @file   llnotificationslistener.cpp
 * @author Brad Kittenbrink
 * @date   2009-07-08
 * @brief  Implementation for llnotificationslistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "llnotificationslistener.h"
#include "llnotifications.h"
#include "llsd.h"
#include "llui.h"

LLNotificationsListener::LLNotificationsListener(LLNotifications & notifications) :
    LLEventAPI("LLNotifications",
               "LLNotifications listener to (e.g.) pop up a notification"),
    mNotifications(notifications)
{
    add("requestAdd",
        "Add a notification with specified [\"name\"], [\"substitutions\"] and [\"payload\"].\n"
        "If optional [\"reply\"] specified, arrange to send user response on that LLEventPump.",
        &LLNotificationsListener::requestAdd);
    add("listChannels",
        "Post to [\"reply\"] a map of info on existing channels",
        &LLNotificationsListener::listChannels,
        LLSD().with("reply", LLSD()));
    add("listChannelNotifications",
        "Post to [\"reply\"] an array of info on notifications in channel [\"channel\"]",
        &LLNotificationsListener::listChannelNotifications,
        LLSD().with("reply", LLSD()).with("channel", LLSD()));
    add("respond",
        "Respond to notification [\"uuid\"] with data in [\"response\"]",
        &LLNotificationsListener::respond,
        LLSD().with("uuid", LLSD()));
    add("cancel",
        "Cancel notification [\"uuid\"]",
        &LLNotificationsListener::cancel,
        LLSD().with("uuid", LLSD()));
    add("ignore",
        "Ignore future notification [\"name\"]\n"
        "(from <notification name= > in notifications.xml)\n"
        "according to boolean [\"ignore\"].\n"
        "If [\"name\"] is omitted or undefined, [un]ignore all future notifications.\n"
        "Note that ignored notifications are not forwarded unless intercepted before\n"
        "the \"Ignore\" channel.",
        &LLNotificationsListener::ignore);
    add("forward",
        "Forward to [\"pump\"] future notifications on channel [\"channel\"]\n"
        "according to boolean [\"forward\"]. When enabled, only types matching\n"
        "[\"types\"] are forwarded, as follows:\n"
        "omitted or undefined: forward all notifications\n"
        "string: forward only the specific named [sig]type\n"
        "array of string: forward any notification matching any named [sig]type.\n"
        "When boolean [\"respond\"] is true, we auto-respond to each forwarded\n"
        "notification.",
        &LLNotificationsListener::forward,
        LLSD().with("channel", LLSD()));
}

// This is here in the .cpp file so we don't need the definition of class
// Forwarder in the header file.
LLNotificationsListener::~LLNotificationsListener()
{
}

void LLNotificationsListener::requestAdd(const LLSD& event_data) const
{
	if(event_data.has("reply"))
	{
		mNotifications.add(event_data["name"], 
						   event_data["substitutions"], 
						   event_data["payload"],
						   boost::bind(&LLNotificationsListener::NotificationResponder, 
									   this, 
									   event_data["reply"].asString(), 
									   _1, _2
									   )
						   );
	}
	else
	{
		mNotifications.add(event_data["name"], 
						   event_data["substitutions"], 
						   event_data["payload"]);
	}
}

void LLNotificationsListener::NotificationResponder(const std::string& reply_pump, 
										const LLSD& notification, 
										const LLSD& response) const
{
	LLSD reponse_event;
	reponse_event["notification"] = notification;
	reponse_event["response"] = response;
	LLEventPumps::getInstance()->obtain(reply_pump).post(reponse_event);
}

void LLNotificationsListener::listChannels(const LLSD& params) const
{
    LLReqID reqID(params);
    LLSD response(reqID.makeResponse());
    for (LLNotifications::ChannelMap::const_iterator cmi(mNotifications.mChannels.begin()),
                                                     cmend(mNotifications.mChannels.end());
         cmi != cmend; ++cmi)
    {
        LLSD channelInfo;
        channelInfo["parent"] = cmi->second->getParentChannelName();
        response[cmi->first] = channelInfo;
    }
    LLEventPumps::instance().obtain(params["reply"]).post(response);
}

void LLNotificationsListener::listChannelNotifications(const LLSD& params) const
{
    LLReqID reqID(params);
    LLSD response(reqID.makeResponse());
    LLNotificationChannelPtr channel(mNotifications.getChannel(params["channel"]));
    if (channel)
    {
        LLSD notifications(LLSD::emptyArray());
        for (LLNotificationChannel::Iterator ni(channel->begin()), nend(channel->end());
             ni != nend; ++ni)
        {
            notifications.append(asLLSD(*ni));
        }
        response["notifications"] = notifications;
    }
    LLEventPumps::instance().obtain(params["reply"]).post(response);
}

void LLNotificationsListener::respond(const LLSD& params) const
{
    LLNotificationPtr notification(mNotifications.find(params["uuid"]));
    if (notification)
    {
        notification->respond(params["response"]);
    }
}

void LLNotificationsListener::cancel(const LLSD& params) const
{
    LLNotificationPtr notification(mNotifications.find(params["uuid"]));
    if (notification)
    {
        mNotifications.cancel(notification);
    }
}

void LLNotificationsListener::ignore(const LLSD& params) const
{
    // Calling a method named "ignore", but omitting its "ignore" Boolean
    // argument, should by default cause something to be ignored. Explicitly
    // pass ["ignore"] = false to cancel ignore.
    bool ignore = true;
    if (params.has("ignore"))
    {
        ignore = params["ignore"].asBoolean();
    }
    // This method can be used to affect either a single notification name or
    // all future notifications. The two use substantially different mechanisms.
    if (params["name"].isDefined())
    {
        // ["name"] was passed: ignore just that notification
        LLUI::sSettingGroups["ignores"]->setBOOL(params["name"], ignore);
    }
    else
    {
        // no ["name"]: ignore all future notifications
        mNotifications.setIgnoreAllNotifications(ignore);
    }
}

class LLNotificationsListener::Forwarder: public LLEventTrackable
{
    LOG_CLASS(LLNotificationsListener::Forwarder);
public:
    Forwarder(LLNotifications& llnotifications, const std::string& channel):
        mNotifications(llnotifications),
        mRespond(false)
    {
        // Connect to the specified channel on construction. Because
        // LLEventTrackable is a base, we should automatically disconnect when
        // destroyed.
        LLNotificationChannelPtr channelptr(llnotifications.getChannel(channel));
        if (channelptr)
        {
            // Insert our processing as a "passed filter" listener. This way
            // we get to run before all the "changed" listeners, and we get to
            // swipe it (hide it from the other listeners) if desired.
            channelptr->connectPassedFilter(boost::bind(&Forwarder::handle, this, _1));
        }
    }

    void setPumpName(const std::string& name) { mPumpName = name; }
    void setTypes(const LLSD& types) { mTypes = types; }
    void setRespond(bool respond) { mRespond = respond; }

private:
    bool handle(const LLSD& notification) const;
    bool matchType(const LLSD& filter, const std::string& type) const;

    LLNotifications& mNotifications;
    std::string mPumpName;
    LLSD mTypes;
    bool mRespond;
};

void LLNotificationsListener::forward(const LLSD& params)
{
    std::string channel(params["channel"]);
    // First decide whether we're supposed to start forwarding or stop it.
    // Default to true.
    bool forward = true;
    if (params.has("forward"))
    {
        forward = params["forward"].asBoolean();
    }
    if (! forward)
    {
        // This is a request to stop forwarding notifications on the specified
        // channel. The rest of the params don't matter.
        // Because mForwarders contains scoped_ptrs, erasing the map entry
        // DOES delete the heap Forwarder object. Because Forwarder derives
        // from LLEventTrackable, destroying it disconnects it from the
        // channel.
        mForwarders.erase(channel);
        return;
    }
    // From here on, we know we're being asked to start (or modify) forwarding
    // on the specified channel. Find or create an appropriate Forwarder.
    ForwarderMap::iterator
        entry(mForwarders.insert(ForwarderMap::value_type(channel, ForwarderMap::mapped_type())).first);
    if (! entry->second)
    {
        entry->second.reset(new Forwarder(mNotifications, channel));
    }
    // Now, whether this Forwarder is brand-new or not, update it with the new
    // request info.
    Forwarder& fwd(*entry->second);
    fwd.setPumpName(params["pump"]);
    fwd.setTypes(params["types"]);
    fwd.setRespond(params["respond"]);
}

bool LLNotificationsListener::Forwarder::handle(const LLSD& notification) const
{
    LL_INFOS("LLNotificationsListener") << "handle(" << notification << ")" << LL_ENDL;
    if (notification["sigtype"].asString() == "delete")
    {
        LL_INFOS("LLNotificationsListener") << "ignoring delete" << LL_ENDL;
        // let other listeners see the "delete" operation
        return false;
    }
    LLNotificationPtr note(mNotifications.find(notification["id"]));
    if (! note)
    {
        LL_INFOS("LLNotificationsListener") << notification["id"] << " not found" << LL_ENDL;
        return false;
    }
    if (! matchType(mTypes, note->getType()))
    {
        LL_INFOS("LLNotificationsListener") << "didn't match types " << mTypes << LL_ENDL;
        // We're not supposed to intercept this particular notification. Let
        // other listeners process it.
        return false;
    }
    LL_INFOS("LLNotificationsListener") << "sending via '" << mPumpName << "'" << LL_ENDL;
    // This is a notification we care about. Forward it through specified
    // LLEventPump.
    LLEventPumps::instance().obtain(mPumpName).post(asLLSD(note));
    // Are we also being asked to auto-respond?
    if (mRespond)
    {
        LL_INFOS("LLNotificationsListener") << "should respond" << LL_ENDL;
        note->respond(LLSD::emptyMap());
        // Did that succeed in removing the notification? Only cancel() if
        // it's still around -- otherwise we get an LL_ERRS crash!
        note = mNotifications.find(notification["id"]);
        if (note)
        {
            LL_INFOS("LLNotificationsListener") << "respond() didn't clear, canceling" << LL_ENDL;
            mNotifications.cancel(note);
        }
    }
    // If we've auto-responded to this notification, then it's going to be
    // deleted. Other listeners would get the change operation, try to look it
    // up and be baffled by lookup failure. So when we auto-respond, suppress
    // this notification: don't pass it to other listeners.
    return mRespond;
}

bool LLNotificationsListener::Forwarder::matchType(const LLSD& filter, const std::string& type) const
{
    // Decide whether this notification matches filter:
    // undefined: forward all notifications
    if (filter.isUndefined())
    {
        return true;
    }
    // array of string: forward any notification matching any named type
    if (filter.isArray())
    {
        for (LLSD::array_const_iterator ti(filter.beginArray()), tend(filter.endArray());
             ti != tend; ++ti)
        {
            if (ti->asString() == type)
            {
                return true;
            }
        }
        // Didn't match any entry in the array
        return false;
    }
    // string: forward only the specific named type
    return (filter.asString() == type);
}

LLSD LLNotificationsListener::asLLSD(LLNotificationPtr note)
{
    LLSD notificationInfo(note->asLLSD());
    // For some reason the following aren't included in LLNotification::asLLSD().
    notificationInfo["summary"] = note->summarize();
    notificationInfo["id"]      = note->id();
    notificationInfo["type"]    = note->getType();
    notificationInfo["message"] = note->getMessage();
    notificationInfo["label"]   = note->getLabel();
    return notificationInfo;
}
