/** 
 * @file llurlentry.h
 * @author Martin Reddy
 * @brief Describes the Url types that can be registered in LLUrlRegistry
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

#ifndef LL_LLURLENTRY_H
#define LL_LLURLENTRY_H

#include "lluuid.h"
#include "lluicolor.h"
#include "llstyle.h"
#include <boost/signals2.hpp>
#include <boost/regex.hpp>
#include <string>
#include <map>

class LLAvatarName;

typedef boost::signals2::signal<void (const std::string& url,
									  const std::string& label,
									  const std::string& icon)> LLUrlLabelSignal;
typedef LLUrlLabelSignal::slot_type LLUrlLabelCallback;

///
/// LLUrlEntryBase is the base class of all Url types registered in the 
/// LLUrlRegistry. Each derived classes provides a regular expression
/// to match the Url type (e.g., http://... or secondlife://...) along
/// with an optional icon to display next to instances of the Url in
/// a text display and a XUI file to use for any context menu popup.
/// Functions are also provided to compute an appropriate label and
/// tooltip/status bar text for the Url.
///
/// Some derived classes of LLUrlEntryBase may wish to compute an
/// appropriate label for a Url by asking the server for information.
/// You must therefore provide a callback method, so that you can be
/// notified when an updated label has been received from the server.
/// This label should then be used to replace any previous label
/// that you received from getLabel() for the Url in question.
///
class LLUrlEntryBase
{
public:
	LLUrlEntryBase();
	virtual ~LLUrlEntryBase();
	
	/// Return the regex pattern that matches this Url 
	boost::regex getPattern() const { return mPattern; }

	/// Return the url from a string that matched the regex
	virtual std::string getUrl(const std::string &string) const;

	/// Given a matched Url, return a label for the Url
	virtual std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) { return url; }

	/// Return an icon that can be displayed next to Urls of this type
	virtual std::string getIcon(const std::string &url);

	/// Return the style to render the displayed text
	virtual LLStyle::Params getStyle() const;

	/// Given a matched Url, return a tooltip string for the hyperlink
	virtual std::string getTooltip(const std::string &string) const { return mTooltip; }

	/// Return the name of a XUI file containing the context menu items
	std::string getMenuName() const { return mMenuName; }

	/// Return the name of a SL location described by this Url, if any
	virtual std::string getLocation(const std::string &url) const { return ""; }

	/// Should this link text be underlined only when mouse is hovered over it?
	virtual bool underlineOnHoverOnly(const std::string &string) const { return false; }

	virtual LLUUID	getID(const std::string &string) const { return LLUUID::null; }

	bool isLinkDisabled() const;

protected:
	std::string getIDStringFromUrl(const std::string &url) const;
	std::string escapeUrl(const std::string &url) const;
	std::string unescapeUrl(const std::string &url) const;
	std::string getLabelFromWikiLink(const std::string &url) const;
	std::string getUrlFromWikiLink(const std::string &string) const;
	void addObserver(const std::string &id, const std::string &url, const LLUrlLabelCallback &cb); 
	virtual void callObservers(const std::string &id, const std::string &label, const std::string& icon);

	typedef struct {
		std::string url;
		LLUrlLabelSignal *signal;
	} LLUrlEntryObserver;

	boost::regex                                   	mPattern;
	std::string                                    	mIcon;
	std::string                                    	mMenuName;
	std::string                                    	mTooltip;
	std::multimap<std::string, LLUrlEntryObserver>	mObservers;
};

///
/// LLUrlEntryHTTP Describes generic http: and https: Urls
///
class LLUrlEntryHTTP : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTP();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
};

///
/// LLUrlEntryHTTPLabel Describes generic http: and https: Urls with custom labels
///
class LLUrlEntryHTTPLabel : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTPLabel();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getTooltip(const std::string &string) const;
	/*virtual*/ std::string getUrl(const std::string &string) const;
};

///
/// LLUrlEntryHTTPNoProtocol Describes generic Urls like www.google.com
///
class LLUrlEntryHTTPNoProtocol : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTPNoProtocol();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getUrl(const std::string &string) const;
};

///
/// LLUrlEntrySLURL Describes http://slurl.com/... Urls
///
class LLUrlEntrySLURL : public LLUrlEntryBase
{
public:
	LLUrlEntrySLURL();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getLocation(const std::string &url) const;
};

///
/// LLUrlEntryAgent Describes a Second Life agent Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/about
class LLUrlEntryAgent : public LLUrlEntryBase
{
public:
	LLUrlEntryAgent();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getIcon(const std::string &url);
	/*virtual*/ std::string getTooltip(const std::string &string) const;
	/*virtual*/ LLStyle::Params getStyle() const;
	/*virtual*/ LLUUID	getID(const std::string &string) const;
	/*virtual*/ bool underlineOnHoverOnly(const std::string &string) const;
protected:
	/*virtual*/ void callObservers(const std::string &id, const std::string &label, const std::string& icon);
private:
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
};

///
/// LLUrlEntryAgentName Describes a Second Life agent name Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/(completename|displayname|username)
/// that displays various forms of user name
/// This is a base class for the various implementations of name display
class LLUrlEntryAgentName : public LLUrlEntryBase, public boost::signals2::trackable
{
public:
	LLUrlEntryAgentName();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ LLStyle::Params getStyle() const;
protected:
	// override this to pull out relevant name fields
	virtual std::string getName(const LLAvatarName& avatar_name) = 0;
private:
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
};


///
/// LLUrlEntryAgentCompleteName Describes a Second Life agent name Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/completename
/// that displays the full display name + user name for an avatar
/// such as "James Linden (james.linden)"
class LLUrlEntryAgentCompleteName : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentCompleteName();
private:
	/*virtual*/ std::string getName(const LLAvatarName& avatar_name);
};

///
/// LLUrlEntryAgentDisplayName Describes a Second Life agent display name Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/displayname
/// that displays the just the display name for an avatar
/// such as "James Linden"
class LLUrlEntryAgentDisplayName : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentDisplayName();
private:
	/*virtual*/ std::string getName(const LLAvatarName& avatar_name);
};

///
/// LLUrlEntryAgentUserName Describes a Second Life agent username Url, e.g.,
/// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/username
/// that displays the just the display name for an avatar
/// such as "james.linden"
class LLUrlEntryAgentUserName : public LLUrlEntryAgentName
{
public:
	LLUrlEntryAgentUserName();
private:
	/*virtual*/ std::string getName(const LLAvatarName& avatar_name);
};

///
/// LLUrlEntryGroup Describes a Second Life group Url, e.g.,
/// secondlife:///app/group/00005ff3-4044-c79f-9de8-fb28ae0df991/about
///
class LLUrlEntryGroup : public LLUrlEntryBase
{
public:
	LLUrlEntryGroup();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ LLStyle::Params getStyle() const;
	/*virtual*/ LLUUID	getID(const std::string &string) const;
private:
	void onGroupNameReceived(const LLUUID& id, const std::string& name, bool is_group);
};

///
/// LLUrlEntryInventory Describes a Second Life inventory Url, e.g.,
/// secondlife:///app/inventory/0e346d8b-4433-4d66-a6b0-fd37083abc4c/select
///
class LLUrlEntryInventory : public LLUrlEntryBase
{
public:
	LLUrlEntryInventory();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
private:
};

///
/// LLUrlEntryObjectIM Describes a Second Life inspector for the object Url, e.g.,
/// secondlife:///app/objectim/7bcd7864-da6b-e43f-4486-91d28a28d95b?name=Object&owner=3de548e1-57be-cfea-2b78-83ae3ad95998&slurl=Danger!%20Danger!/200/200/30/&groupowned=1
///
class LLUrlEntryObjectIM : public LLUrlEntryBase
{
public:
	LLUrlEntryObjectIM();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getLocation(const std::string &url) const;
private:
};

///
/// LLUrlEntryParcel Describes a Second Life parcel Url, e.g.,
/// secondlife:///app/parcel/0000060e-4b39-e00b-d0c3-d98b1934e3a8/about
///
class LLUrlEntryParcel : public LLUrlEntryBase
{
public:
	LLUrlEntryParcel();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
};

///
/// LLUrlEntryPlace Describes a Second Life location Url, e.g.,
/// secondlife://Ahern/50/50/50
///
class LLUrlEntryPlace : public LLUrlEntryBase
{
public:
	LLUrlEntryPlace();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getLocation(const std::string &url) const;
};

///
/// LLUrlEntryTeleport Describes a Second Life teleport Url, e.g.,
/// secondlife:///app/teleport/Ahern/50/50/50/
///
class LLUrlEntryTeleport : public LLUrlEntryBase
{
public:
	LLUrlEntryTeleport();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getLocation(const std::string &url) const;
};

///
/// LLUrlEntrySL Describes a generic SLURL, e.g., a Url that starts
/// with secondlife:// (used as a catch-all for cases not matched above)
///
class LLUrlEntrySL : public LLUrlEntryBase
{
public:
	LLUrlEntrySL();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
};

///
/// LLUrlEntrySLLabel Describes a generic SLURL, e.g., a Url that starts
/// with secondlife:// with the ability to specify a custom label.
///
class LLUrlEntrySLLabel : public LLUrlEntryBase
{
public:
	LLUrlEntrySLLabel();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getUrl(const std::string &string) const;
	/*virtual*/ std::string getTooltip(const std::string &string) const;
	/*virtual*/ bool underlineOnHoverOnly(const std::string &string) const;
};

///
/// LLUrlEntryWorldMap Describes a Second Life worldmap Url, e.g.,
/// secondlife:///app/worldmap/Ahern/50/50/50
///
class LLUrlEntryWorldMap : public LLUrlEntryBase
{
public:
	LLUrlEntryWorldMap();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getLocation(const std::string &url) const;
};

///
/// LLUrlEntryNoLink lets us turn of URL detection with <nolink>...</nolink> tags
///
class LLUrlEntryNoLink : public LLUrlEntryBase
{
public:
	LLUrlEntryNoLink();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getUrl(const std::string &string) const;
	/*virtual*/ LLStyle::Params getStyle() const;
};

///
/// LLUrlEntryIcon describes an icon with <icon>...</icon> tags
///
class LLUrlEntryIcon : public LLUrlEntryBase
{
public:
	LLUrlEntryIcon();
	/*virtual*/ std::string getUrl(const std::string &string) const;
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getIcon(const std::string &url);
};


#endif
