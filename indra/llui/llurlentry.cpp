/** 
 * @file llurlentry.cpp
 * @author Martin Reddy
 * @brief Describes the Url types that can be registered in LLUrlRegistry
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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
#include "llurlentry.h"
#include "lluri.h"
#include "llurlmatch.h"
#include "llurlregistry.h"

#include "llcachename.h"
#include "lltrans.h"
#include "lluicolortable.h"

#define APP_HEADER_REGEX "((x-grid-location-info://[-\\w\\.]+/app)|(secondlife:///app))"


LLUrlEntryBase::LLUrlEntryBase() :
	mColor(LLUIColorTable::instance().getColor("HTMLLinkColor")),
	mDisabledLink(false)
{
}

LLUrlEntryBase::~LLUrlEntryBase()
{
}

std::string LLUrlEntryBase::getUrl(const std::string &string) const
{
	return escapeUrl(string);
}

std::string LLUrlEntryBase::getIDStringFromUrl(const std::string &url) const
{
	// return the id from a SLURL in the format /app/{cmd}/{id}/about
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	if (path_array.size() == 4) 
	{
		return path_array.get(2).asString();
	}
	return "";
}

std::string LLUrlEntryBase::unescapeUrl(const std::string &url) const
{
	return LLURI::unescape(url);
}

std::string LLUrlEntryBase::escapeUrl(const std::string &url) const
{
	static std::string no_escape_chars;
	static bool initialized = false;
	if (!initialized)
	{
		no_escape_chars = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789"
			"-._~!$?&()*+,@:;=/%#";

		std::sort(no_escape_chars.begin(), no_escape_chars.end());
		initialized = true;
	}
	return LLURI::escape(url, no_escape_chars, true);
}

std::string LLUrlEntryBase::getLabelFromWikiLink(const std::string &url) const
{
	// return the label part from [http://www.example.org Label]
	const char *text = url.c_str();
	S32 start = 0;
	while (! isspace(text[start]))
	{
		start++;
	}
	while (text[start] == ' ' || text[start] == '\t')
	{
		start++;
	}
	return unescapeUrl(url.substr(start, url.size()-start-1));
}

std::string LLUrlEntryBase::getUrlFromWikiLink(const std::string &string) const
{
	// return the url part from [http://www.example.org Label]
	const char *text = string.c_str();
	S32 end = 0;
	while (! isspace(text[end]))
	{
		end++;
	}
	return escapeUrl(string.substr(1, end-1));
}

void LLUrlEntryBase::addObserver(const std::string &id,
								 const std::string &url,
								 const LLUrlLabelCallback &cb)
{
	// add a callback to be notified when we have a label for the uuid
	LLUrlEntryObserver observer;
	observer.url = url;
	observer.signal = new LLUrlLabelSignal();
	if (observer.signal)
	{
		observer.signal->connect(cb);
		mObservers.insert(std::pair<std::string, LLUrlEntryObserver>(id, observer));
	}
}
 
void LLUrlEntryBase::callObservers(const std::string &id, const std::string &label)
{
	// notify all callbacks waiting on the given uuid
	std::multimap<std::string, LLUrlEntryObserver>::iterator it;
	for (it = mObservers.find(id); it != mObservers.end();)
	{
		// call the callback - give it the new label
		LLUrlEntryObserver &observer = it->second;
		(*observer.signal)(it->second.url, label);
		// then remove the signal - we only need to call it once
		delete observer.signal;
		mObservers.erase(it++);
	}
}

static std::string getStringAfterToken(const std::string str, const std::string token)
{
	size_t pos = str.find(token);
	if (pos == std::string::npos)
	{
		return "";
	}

	pos += token.size();
	return str.substr(pos, str.size() - pos);
}

//
// LLUrlEntryHTTP Describes generic http: and https: Urls
//
LLUrlEntryHTTP::LLUrlEntryHTTP()
{
	mPattern = boost::regex("https?://([-\\w\\.]+)+(:\\d+)?(:\\w+)?(@\\d+)?(@\\w+)?/?\\S*",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_http.xml";
	mTooltip = LLTrans::getString("TooltipHttpUrl");
}

std::string LLUrlEntryHTTP::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return unescapeUrl(url);
}

//
// LLUrlEntryHTTP Describes generic http: and https: Urls with custom label
// We use the wikipedia syntax of [http://www.example.org Text]
//
LLUrlEntryHTTPLabel::LLUrlEntryHTTPLabel()
{
	mPattern = boost::regex("\\[https?://\\S+[ \t]+[^\\]]+\\]",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_http.xml";
	mTooltip = LLTrans::getString("TooltipHttpUrl");
}

std::string LLUrlEntryHTTPLabel::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return getLabelFromWikiLink(url);
}

std::string LLUrlEntryHTTPLabel::getUrl(const std::string &string) const
{
	return getUrlFromWikiLink(string);
}

//
// LLUrlEntryHTTPNoProtocol Describes generic Urls like www.google.com
//
LLUrlEntryHTTPNoProtocol::LLUrlEntryHTTPNoProtocol()
{
	mPattern = boost::regex("("
				"\\bwww\\.\\S+\\.\\S+" // i.e. www.FOO.BAR
				"|" // or
				"(?<!@)\\b[^[:space:]:@/>]+\\.(?:com|net|edu|org)([/:][^[:space:]<]*)?\\b" // i.e. FOO.net
				")",
				boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_http.xml";
	mTooltip = LLTrans::getString("TooltipHttpUrl");
}

std::string LLUrlEntryHTTPNoProtocol::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return unescapeUrl(url);
}

std::string LLUrlEntryHTTPNoProtocol::getUrl(const std::string &string) const
{
	if (string.find("://") == std::string::npos)
	{
		return "http://" + escapeUrl(string);
	}
	return escapeUrl(string);
}

//
// LLUrlEntrySLURL Describes generic http: and https: Urls
//
LLUrlEntrySLURL::LLUrlEntrySLURL()
{
	// see http://slurl.com/about.php for details on the SLURL format
	mPattern = boost::regex("http://(maps.secondlife.com|slurl.com)/secondlife/[^ /]+(/\\d+){0,3}(/?(\\?title|\\?img|\\?msg)=\\S*)?/?",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_slurl.xml";
	mTooltip = LLTrans::getString("TooltipSLURL");
}

std::string LLUrlEntrySLURL::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	//
	// we handle SLURLs in the following formats:
	//   - http://slurl.com/secondlife/Place/X/Y/Z
	//   - http://slurl.com/secondlife/Place/X/Y
	//   - http://slurl.com/secondlife/Place/X
	//   - http://slurl.com/secondlife/Place
	//
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	S32 path_parts = path_array.size();
	if (path_parts == 5)
	{
		// handle slurl with (X,Y,Z) coordinates
		std::string location = unescapeUrl(path_array[path_parts-4]);
		std::string x = path_array[path_parts-3];
		std::string y = path_array[path_parts-2];
		std::string z = path_array[path_parts-1];
		return location + " (" + x + "," + y + "," + z + ")";
	}
	else if (path_parts == 4)
	{
		// handle slurl with (X,Y) coordinates
		std::string location = unescapeUrl(path_array[path_parts-3]);
		std::string x = path_array[path_parts-2];
		std::string y = path_array[path_parts-1];
		return location + " (" + x + "," + y + ")";
	}
	else if (path_parts == 3)
	{
		// handle slurl with (X) coordinate
		std::string location = unescapeUrl(path_array[path_parts-2]);
		std::string x = path_array[path_parts-1];
		return location + " (" + x + ")";
	}
	else if (path_parts == 2)
	{
		// handle slurl with no coordinates
		std::string location = unescapeUrl(path_array[path_parts-1]);
		return location;
	}

	return url;
}

std::string LLUrlEntrySLURL::getLocation(const std::string &url) const
{
	// return the part of the Url after slurl.com/secondlife/
	const std::string search_string = "/secondlife";
	size_t pos = url.find(search_string);
	if (pos == std::string::npos)
	{
		return "";
	}

	pos += search_string.size() + 1;
	return url.substr(pos, url.size() - pos);
}

//
// LLUrlEntryAgent Describes a Second Life agent Url, e.g.,
// secondlife:///app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/about
// x-grid-location-info://lincoln.lindenlab.com/app/agent/0e346d8b-4433-4d66-a6b0-fd37083abc4c/about
//
LLUrlEntryAgent::LLUrlEntryAgent()
{
	mPattern = boost::regex(APP_HEADER_REGEX "/agent/[\\da-f-]+/\\w+",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_agent.xml";
	mIcon = "Generic_Person";
	mTooltip = LLTrans::getString("TooltipAgentUrl");
	mColor = LLUIColorTable::instance().getColor("AgentLinkColor");
}

void LLUrlEntryAgent::onAgentNameReceived(const LLUUID& id,
										  const std::string& first,
										  const std::string& last,
										  BOOL is_group)
{
	// received the agent name from the server - tell our observers
	callObservers(id.asString(), first + " " + last);
}

std::string LLUrlEntryAgent::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	if (!gCacheName)
	{
		// probably at the login screen, use short string for layout
		return LLTrans::getString("LoadingData");
	}

	std::string agent_id_string = getIDStringFromUrl(url);
	if (agent_id_string.empty())
	{
		// something went wrong, just give raw url
		return unescapeUrl(url);
	}

	LLUUID agent_id(agent_id_string);
	std::string full_name;
	if (agent_id.isNull())
	{
		return LLTrans::getString("AvatarNameNobody");
	}
	else if (gCacheName->getFullName(agent_id, full_name))
	{
		return full_name;
	}
	else
	{
		gCacheName->get(agent_id, FALSE,
			boost::bind(&LLUrlEntryAgent::onAgentNameReceived,
				this, _1, _2, _3, _4));
		addObserver(agent_id_string, url, cb);
		return LLTrans::getString("LoadingData");
	}
}

//
// LLUrlEntryGroup Describes a Second Life group Url, e.g.,
// secondlife:///app/group/00005ff3-4044-c79f-9de8-fb28ae0df991/about
// secondlife:///app/group/00005ff3-4044-c79f-9de8-fb28ae0df991/inspect
// x-grid-location-info://lincoln.lindenlab.com/app/group/00005ff3-4044-c79f-9de8-fb28ae0df991/inspect
//
LLUrlEntryGroup::LLUrlEntryGroup()
{
	mPattern = boost::regex(APP_HEADER_REGEX "/group/[\\da-f-]+/\\w+",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_group.xml";
	mIcon = "Generic_Group";
	mTooltip = LLTrans::getString("TooltipGroupUrl");
	mColor = LLUIColorTable::instance().getColor("GroupLinkColor");
}

void LLUrlEntryGroup::onGroupNameReceived(const LLUUID& id,
										  const std::string& first,
										  const std::string& last,
										  BOOL is_group)
{
	// received the group name from the server - tell our observers
	callObservers(id.asString(), first);
}

std::string LLUrlEntryGroup::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	if (!gCacheName)
	{
		// probably at login screen, give something short for layout
		return LLTrans::getString("LoadingData");
	}

	std::string group_id_string = getIDStringFromUrl(url);
	if (group_id_string.empty())
	{
		// something went wrong, give raw url
		return unescapeUrl(url);
	}

	LLUUID group_id(group_id_string);
	std::string group_name;
	if (group_id.isNull())
	{
		return LLTrans::getString("GroupNameNone");
	}
	else if (gCacheName->getGroupName(group_id, group_name))
	{
		return group_name;
	}
	else
	{
		gCacheName->get(group_id, TRUE,
			boost::bind(&LLUrlEntryGroup::onGroupNameReceived,
				this, _1, _2, _3, _4));
		addObserver(group_id_string, url, cb);
		return LLTrans::getString("LoadingData");
	}
}

//
// LLUrlEntryInventory Describes a Second Life inventory Url, e.g.,
// secondlife:///app/inventory/0e346d8b-4433-4d66-a6b0-fd37083abc4c/select
//
LLUrlEntryInventory::LLUrlEntryInventory()
{
	//*TODO: add supporting of inventory item names with whitespaces
	//this pattern cann't parse for example 
	//secondlife:///app/inventory/0e346d8b-4433-4d66-a6b0-fd37083abc4c/select?name=name with spaces&param2=value
	//x-grid-location-info://lincoln.lindenlab.com/app/inventory/0e346d8b-4433-4d66-a6b0-fd37083abc4c/select?name=name with spaces&param2=value
	mPattern = boost::regex(APP_HEADER_REGEX "/inventory/[\\da-f-]+/\\w+\\S*",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_inventory.xml";
}

std::string LLUrlEntryInventory::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	std::string label = getStringAfterToken(url, "name=");
	return LLURI::unescape(label.empty() ? url : label);
}

///
/// LLUrlEntryParcel Describes a Second Life parcel Url, e.g.,
/// secondlife:///app/parcel/0000060e-4b39-e00b-d0c3-d98b1934e3a8/about
/// x-grid-location-info://lincoln.lindenlab.com/app/parcel/0000060e-4b39-e00b-d0c3-d98b1934e3a8/about
///
LLUrlEntryParcel::LLUrlEntryParcel()
{
	mPattern = boost::regex(APP_HEADER_REGEX "/parcel/[\\da-f-]+/about",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_parcel.xml";
	mTooltip = LLTrans::getString("TooltipParcelUrl");
}

std::string LLUrlEntryParcel::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return unescapeUrl(url);
}

//
// LLUrlEntryPlace Describes secondlife://<location> URLs
//
LLUrlEntryPlace::LLUrlEntryPlace()
{
	mPattern = boost::regex("((x-grid-location-info://[-\\w\\.]+/region/)|(secondlife://))\\S+/?(\\d+/\\d+/\\d+|\\d+/\\d+)/?",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_slurl.xml";
	mTooltip = LLTrans::getString("TooltipSLURL");
}

std::string LLUrlEntryPlace::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	//
	// we handle SLURLs in the following formats:
	//   - secondlife://Place/X/Y/Z
	//   - secondlife://Place/X/Y
	//
	LLURI uri(url);
	std::string location = unescapeUrl(uri.hostName());
	LLSD path_array = uri.pathArray();
	S32 path_parts = path_array.size();
	if (path_parts == 3)
	{
		// handle slurl with (X,Y,Z) coordinates
		std::string x = path_array[0];
		std::string y = path_array[1];
		std::string z = path_array[2];
		return location + " (" + x + "," + y + "," + z + ")";
	}
	else if (path_parts == 2)
	{
		// handle slurl with (X,Y) coordinates
		std::string x = path_array[0];
		std::string y = path_array[1];
		return location + " (" + x + "," + y + ")";
	}

	return url;
}

std::string LLUrlEntryPlace::getLocation(const std::string &url) const
{
	// return the part of the Url after secondlife:// part
	return ::getStringAfterToken(url, "://");
}

//
// LLUrlEntryTeleport Describes a Second Life teleport Url, e.g.,
// secondlife:///app/teleport/Ahern/50/50/50/
// x-grid-location-info://lincoln.lindenlab.com/app/teleport/Ahern/50/50/50/
//
LLUrlEntryTeleport::LLUrlEntryTeleport()
{
	mPattern = boost::regex(APP_HEADER_REGEX "/teleport/\\S+(/\\d+)?(/\\d+)?(/\\d+)?/?\\S*",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_teleport.xml";
	mTooltip = LLTrans::getString("TooltipTeleportUrl");
}

std::string LLUrlEntryTeleport::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	//
	// we handle teleport SLURLs in the following formats:
	//   - secondlife:///app/teleport/Place/X/Y/Z
	//   - secondlife:///app/teleport/Place/X/Y
	//   - secondlife:///app/teleport/Place/X
	//   - secondlife:///app/teleport/Place
	//
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	S32 path_parts = path_array.size();
	std::string host = uri.hostName();
	std::string label = LLTrans::getString("SLurlLabelTeleport");
	if (!host.empty())
	{
		label += " " + host;
	}
	if (path_parts == 6)
	{
		// handle teleport url with (X,Y,Z) coordinates
		std::string location = unescapeUrl(path_array[path_parts-4]);
		std::string x = path_array[path_parts-3];
		std::string y = path_array[path_parts-2];
		std::string z = path_array[path_parts-1];
		return label + " " + location + " (" + x + "," + y + "," + z + ")";
	}
	else if (path_parts == 5)
	{
		// handle teleport url with (X,Y) coordinates
		std::string location = unescapeUrl(path_array[path_parts-3]);
		std::string x = path_array[path_parts-2];
		std::string y = path_array[path_parts-1];
		return label + " " + location + " (" + x + "," + y + ")";
	}
	else if (path_parts == 4)
	{
		// handle teleport url with (X) coordinate only
		std::string location = unescapeUrl(path_array[path_parts-2]);
		std::string x = path_array[path_parts-1];
		return label + " " + location + " (" + x + ")";
	}
	else if (path_parts == 3)
	{
		// handle teleport url with no coordinates
		std::string location = unescapeUrl(path_array[path_parts-1]);
		return label + " " + location;
	}

	return url;
}

std::string LLUrlEntryTeleport::getLocation(const std::string &url) const
{
	// return the part of the Url after ///app/teleport
	return ::getStringAfterToken(url, "app/teleport/");
}

//
// LLUrlEntrySL Describes a generic SLURL, e.g., a Url that starts
// with secondlife:// (used as a catch-all for cases not matched above)
//
LLUrlEntrySL::LLUrlEntrySL()
{
	mPattern = boost::regex("secondlife://(\\w+)?(:\\d+)?/\\S+",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_slapp.xml";
	mTooltip = LLTrans::getString("TooltipSLAPP");
}

std::string LLUrlEntrySL::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return unescapeUrl(url);
}

//
// LLUrlEntrySLLabel Describes a generic SLURL, e.g., a Url that starts
/// with secondlife:// with the ability to specify a custom label.
//
LLUrlEntrySLLabel::LLUrlEntrySLLabel()
{
	mPattern = boost::regex("\\[secondlife://\\S+[ \t]+[^\\]]+\\]",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_slapp.xml";
	mTooltip = LLTrans::getString("TooltipSLAPP");
}

std::string LLUrlEntrySLLabel::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return getLabelFromWikiLink(url);
}

std::string LLUrlEntrySLLabel::getUrl(const std::string &string) const
{
	return getUrlFromWikiLink(string);
}

std::string LLUrlEntrySLLabel::getTooltip(const std::string &string) const
{
	// return a tooltip corresponding to the URL type instead of the generic one (EXT-4574)
	std::string url = getUrl(string);
	LLUrlMatch match;
	if (LLUrlRegistry::instance().findUrl(url, match))
	{
		return match.getTooltip();
	}

	// unrecognized URL? should not happen
	return LLUrlEntryBase::getTooltip(string);
}

//
// LLUrlEntryWorldMap Describes secondlife:///<location> URLs
//
LLUrlEntryWorldMap::LLUrlEntryWorldMap()
{
	mPattern = boost::regex(APP_HEADER_REGEX "/worldmap/\\S+/?(\\d+)?/?(\\d+)?/?(\\d+)?/?\\S*",
							boost::regex::perl|boost::regex::icase);
	mMenuName = "menu_url_map.xml";
	mTooltip = LLTrans::getString("TooltipMapUrl");
}

std::string LLUrlEntryWorldMap::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	//
	// we handle SLURLs in the following formats:
	//   - secondlife:///app/worldmap/PLACE/X/Y/Z
	//   - secondlife:///app/worldmap/PLACE/X/Y
	//   - secondlife:///app/worldmap/PLACE/X
	//
	LLURI uri(url);
	LLSD path_array = uri.pathArray();
	S32 path_parts = path_array.size();
	if (path_parts < 3)
	{
		return url;
	}

	const std::string label = LLTrans::getString("SLurlLabelShowOnMap");
	std::string location = path_array[2];
	std::string x = (path_parts > 3) ? path_array[3] : "128";
	std::string y = (path_parts > 4) ? path_array[4] : "128";
	std::string z = (path_parts > 5) ? path_array[5] : "0";
	return label + " " + location + " (" + x + "," + y + "," + z + ")";
}

std::string LLUrlEntryWorldMap::getLocation(const std::string &url) const
{
	// return the part of the Url after secondlife:///app/worldmap/ part
	return ::getStringAfterToken(url, "app/worldmap/");
}

//
// LLUrlEntryNoLink lets us turn of URL detection with <nolink>...</nolink> tags
//
LLUrlEntryNoLink::LLUrlEntryNoLink()
{
	mPattern = boost::regex("<nolink>[^<]*</nolink>",
							boost::regex::perl|boost::regex::icase);
	mDisabledLink = true;
}

std::string LLUrlEntryNoLink::getUrl(const std::string &url) const
{
	// return the text between the <nolink> and </nolink> tags
	return url.substr(8, url.size()-8-9);
}

std::string LLUrlEntryNoLink::getLabel(const std::string &url, const LLUrlLabelCallback &cb)
{
	return getUrl(url);
}
