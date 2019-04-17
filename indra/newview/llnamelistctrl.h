/**
 * @file llnamelistctrl.h
 * @brief A list of names, automatically refreshing from the name cache.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLNAMELISTCTRL_H
#define LL_LLNAMELISTCTRL_H

#include <set>

#include "llscrolllistctrl.h"

class LLAvatarName;

/**
 * LLNameListCtrl item
 *
 * We don't use LLScrollListItem to be able to override getUUID(), which is needed
 * because the name list item value is not simply an UUID but a map (uuid, is_group).
 */
class LLNameListItem : public LLScrollListItem, public LLHandleProvider<LLNameListItem>
{
public:
	bool isGroup() const { return mIsGroup; }
	void setIsGroup(bool is_group) { mIsGroup = is_group; }
	bool isExperience() const { return mIsExperience; }
	void setIsExperience(bool is_experience) { mIsExperience = is_experience; }

protected:
	friend class LLNameListCtrl;

	LLNameListItem( const LLScrollListItem::Params& p )
	:	LLScrollListItem(p), mIsGroup(false), mIsExperience(false)
	{
	}

	LLNameListItem( const LLScrollListItem::Params& p, bool is_group )
	:	LLScrollListItem(p), mIsGroup(is_group), mIsExperience(false)
	{
	}

	LLNameListItem( const LLScrollListItem::Params& p, bool is_group, bool is_experience )
	:	LLScrollListItem(p), mIsGroup(is_group), mIsExperience(is_experience)
	{
	}

private:
	bool mIsGroup;
	bool mIsExperience;
};


class LLNameListCtrl
:	public LLScrollListCtrl, public LLInstanceTracker<LLNameListCtrl>
{
public:
	typedef boost::signals2::signal<void(bool)> namelist_complete_signal_t;

	typedef enum e_name_type
	{
		INDIVIDUAL,
		GROUP,
		SPECIAL,
		EXPERIENCE
	} ENameType;

	// provide names for enums
	struct NameTypeNames : public LLInitParam::TypeValuesHelper<LLNameListCtrl::ENameType, NameTypeNames>
	{
		static void declareValues();
	};

	struct NameItem : public LLInitParam::Block<NameItem, LLScrollListItem::Params>
	{
		Optional<std::string>				name;
		Optional<ENameType, NameTypeNames>	target;

		NameItem()
		:	name("name"),
			target("target", INDIVIDUAL)
		{}
	};

	struct NameColumn : public LLInitParam::ChoiceBlock<NameColumn>
	{
		Alternative<S32>				column_index;
		Alternative<std::string>		column_name;
		NameColumn()
		:	column_name("name_column"),
			column_index("name_column_index", 0)
		{}
	};

	struct Params : public LLInitParam::Block<Params, LLScrollListCtrl::Params>
	{
		Optional<NameColumn>	name_column;
		Optional<bool>	allow_calling_card_drop;
		Optional<bool>			short_names;
		Params();
	};

protected:
	LLNameListCtrl(const Params&);
	virtual ~LLNameListCtrl()
	{
		for (avatar_name_cache_connection_map_t::iterator it = mAvatarNameCacheConnections.begin(); it != mAvatarNameCacheConnections.end(); ++it)
		{
			if (it->second.connected())
			{
				it->second.disconnect();
			}
		}
		mAvatarNameCacheConnections.clear();
	}
	friend class LLUICtrlFactory;
public:
	// Add a user to the list by name.  It will be added, the name
	// requested from the cache, and updated as necessary.
	LLScrollListItem* addNameItem(const LLUUID& agent_id, EAddPosition pos = ADD_BOTTOM,
					 BOOL enabled = TRUE, const std::string& suffix = LLStringUtil::null, const std::string& prefix = LLStringUtil::null);
	LLScrollListItem* addNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);

	/*virtual*/ LLScrollListItem* addElement(const LLSD& element, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);
	LLScrollListItem* addNameItemRow(const NameItem& value, EAddPosition pos = ADD_BOTTOM, const std::string& suffix = LLStringUtil::null,
																							const std::string& prefix = LLStringUtil::null);

	// Add a user to the list by name.  It will be added, the name
	// requested from the cache, and updated as necessary.
	void addGroupNameItem(const LLUUID& group_id, EAddPosition pos = ADD_BOTTOM,
						  BOOL enabled = TRUE);
	void addGroupNameItem(NameItem& item, EAddPosition pos = ADD_BOTTOM);


	void removeNameItem(const LLUUID& agent_id);

	LLScrollListItem* getNameItemByAgentId(const LLUUID& agent_id);

	// LLView interface
	/*virtual*/ BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
									  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept,
									  std::string& tooltip_msg);
	/*virtual*/ BOOL handleToolTip(S32 x, S32 y, MASK mask);

	void setAllowCallingCardDrop(BOOL b) { mAllowCallingCardDrop = b; }

	void sortByName(BOOL ascending);

	/*virtual*/ void updateColumns(bool force_update);

	/*virtual*/ void mouseOverHighlightNthItem( S32 index );
private:
	void showInspector(const LLUUID& avatar_id, bool is_group, bool is_experience = false);
	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name, std::string suffix, std::string prefix, LLHandle<LLNameListItem> item);
	void onGroupNameCache(const LLUUID& group_id, const std::string name, LLHandle<LLNameListItem> item);

private:
	S32    			mNameColumnIndex;
	std::string		mNameColumn;
	BOOL			mAllowCallingCardDrop;
	bool			mShortNames;  // display name only, no SLID
	typedef std::map<LLUUID, boost::signals2::connection> avatar_name_cache_connection_map_t;
	avatar_name_cache_connection_map_t mAvatarNameCacheConnections;
	avatar_name_cache_connection_map_t mGroupNameCacheConnections;

	S32 mPendingLookupsRemaining;
	namelist_complete_signal_t mNameListCompleteSignal;
	
public:
	boost::signals2::connection setOnNameListCompleteCallback(boost::function<void(bool)> onNameListCompleteCallback) 
	{ 
		return mNameListCompleteSignal.connect(onNameListCompleteCallback); 
	}

};


#endif
