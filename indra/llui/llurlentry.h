/** 
 * @file llurlentry.h
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

#ifndef LL_LLURLENTRY_H
#define LL_LLURLENTRY_H

#include "lluuid.h"
#include "lluicolor.h"
#include <boost/signals2.hpp>
#include <boost/regex.hpp>
#include <string>
#include <map>

typedef boost::signals2::signal<void (const std::string& url,
									  const std::string& label)> LLUrlLabelSignal;
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
	virtual std::string getUrl(const std::string &string);

	/// Given a matched Url, return a label for the Url
	virtual std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb) { return url; }

	/// Return an icon that can be displayed next to Urls of this type
	std::string getIcon() const { return mIcon; }

	/// Return the color to render the displayed text
	LLUIColor getColor() const { return mColor; }

	/// Given a matched Url, return a tooltip string for the hyperlink
	std::string getTooltip() const { return mTooltip; }

	/// Return the name of a XUI file containing the context menu items
	std::string getMenuName() const { return mMenuName; }

	/// Return the name of a SL location described by this Url, if any
	virtual std::string getLocation(const std::string &url) const { return ""; }

protected:
	std::string getIDStringFromUrl(const std::string &url) const;
	std::string escapeUrl(const std::string &url) const;
	std::string unescapeUrl(const std::string &url) const;
	std::string getLabelFromWikiLink(const std::string &url);
	std::string getUrlFromWikiLink(const std::string &string);
	void addObserver(const std::string &id, const std::string &url, const LLUrlLabelCallback &cb); 
	void callObservers(const std::string &id, const std::string &label);

	typedef struct {
		std::string url;
		LLUrlLabelSignal *signal;
	} LLUrlEntryObserver;

	boost::regex                                   	mPattern;
	std::string                                    	mIcon;
	std::string                                    	mMenuName;
	std::string                                    	mTooltip;
	LLUIColor										mColor;
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
	/*virtual*/ std::string getUrl(const std::string &string);
};

///
/// LLUrlEntryHTTPNoProtocol Describes generic Urls like www.google.com
///
class LLUrlEntryHTTPNoProtocol : public LLUrlEntryBase
{
public:
	LLUrlEntryHTTPNoProtocol();
	/*virtual*/ std::string getLabel(const std::string &url, const LLUrlLabelCallback &cb);
	/*virtual*/ std::string getUrl(const std::string &string);
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
private:
	void onNameCache(const LLUUID& id, const std::string& full_name, bool is_group);
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
	/*virtual*/ std::string getUrl(const std::string &string);
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

#endif
