/** 
 * @file llocationhistory.h
 * @brief Typed locations history
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

#ifndef LL_LLLOCATIONHISTORY_H
#define LL_LLLOCATIONHISTORY_H

#include "llsingleton.h" // for LLSingleton

#include <vector>
#include <string>
#include <map>
#include <boost/function.hpp>

class LLSD;
/**
 * This enum is responsible for identifying of history item.
 */
enum ELocationType {
	 TYPED_REGION_SLURL//item added after the user had typed a region name or slurl 
	,LANDMARK  // item has been loaded from landmark folder
	,TELEPORT_HISTORY // item from session teleport history
	};
class LLLocationHistoryItem {
			
public:
	LLLocationHistoryItem(){}
	LLLocationHistoryItem(std::string typed_location, 
			LLVector3d global_position, std::string tooltip,ELocationType type ):
		mLocation(typed_location),		
		mGlobalPos(global_position),
		mToolTip(tooltip),
		mType(type)
	{}
	LLLocationHistoryItem(const LLLocationHistoryItem& item):
		mGlobalPos(item.mGlobalPos),
		mToolTip(item.mToolTip),
		mLocation(item.mLocation),
		mType(item.mType)
	{}
	LLLocationHistoryItem(const LLSD& data):
	mLocation(data["location"]),
	mGlobalPos(data["global_pos"]),
	mToolTip(data["tooltip"]),
	mType(ELocationType(data["item_type"].asInteger()))
	{}

	bool operator==(const LLLocationHistoryItem& item)
	{
		// do not compare  mGlobalPos, 
		// because of a rounding off , the history  can contain duplicates
		return mLocation == item.mLocation && (mType == item.mType); 
	}
	bool operator!=(const LLLocationHistoryItem& item)
	{
		return ! (*this == item);
	}
	LLSD toLLSD() const
	{
		LLSD val;
		val["location"]= mLocation;
		val["global_pos"]	= mGlobalPos.getValue();
		val["tooltip"]	= mToolTip;
		val["item_type"] = mType;
		return val;
	}
	const std::string& getLocation() const { return mLocation;	};
	const std::string& getToolTip() const { return mToolTip;	};
	//static bool equalByRegionParcel(const LLLocationHistoryItem& item1, const LLLocationHistoryItem& item2);
	static bool equalByLocation(const LLLocationHistoryItem& item1, const std::string& item_location)
	{
		return  item1.getLocation() == item_location;
	}
	
	LLVector3d	mGlobalPos; // global position
	std::string mToolTip;// SURL
	std::string mLocation;// typed_location
	ELocationType mType;
};

class LLLocationHistory: public LLSingleton<LLLocationHistory>
{
	LOG_CLASS(LLLocationHistory);

public:
	enum EChangeType
	{
		 ADD
		,CLEAR
		,LOAD
	};
	
	typedef std::vector<LLLocationHistoryItem>	location_list_t;
	typedef boost::function<void(EChangeType event)>			history_changed_callback_t;
	typedef boost::signals2::signal<void(EChangeType event)>	history_changed_signal_t;
	
	LLLocationHistory();
	
	void					addItem(const LLLocationHistoryItem& item);
	bool					touchItem(const LLLocationHistoryItem& item);
	void                    removeItems();
	size_t					getItemCount() const	{ return mItems.size(); }
	const location_list_t&	getItems() const		{ return mItems; }
	bool					getMatchingItems(const std::string& substring, location_list_t& result) const;
	boost::signals2::connection	setChangedCallback(history_changed_callback_t cb) { return mChangedSignal.connect(cb); }
	
	void					save() const;
	void					load();
	void					dump() const;

private:

	location_list_t			mItems;
	std::string							mFilename; /// File to store the history to.
	history_changed_signal_t			mChangedSignal;
};

#endif
