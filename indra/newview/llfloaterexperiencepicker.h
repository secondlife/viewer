/** 
* @file   llfloaterexperiencepicker.h
* @brief  Header file for llfloaterexperiencepicker
* @author dolphin@lindenlab.com
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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
#ifndef LL_LLFLOATEREXPERIENCEPICKER_H
#define LL_LLFLOATEREXPERIENCEPICKER_H

#include "llfloater.h"

class LLScrollListCtrl;
class LLLineEditor;


class LLFloaterExperiencePicker : public LLFloater
{
public:
	friend class LLExperiencePickerResponder;

	// The callback function will be called with an avatar name and UUID.
	typedef boost::function<void (const uuid_vec_t&)> select_callback_t;
	// filter function for experiences, return true if the experience should be hidden.
	typedef boost::function<bool (const LLSD&)> filter_function;
	typedef std::vector<filter_function> filter_list;

	static LLFloaterExperiencePicker* show( select_callback_t callback, const LLUUID& key, BOOL allow_multiple, BOOL closeOnSelect, LLView * frustumOrigin);

	LLFloaterExperiencePicker(const LLSD& key);
	virtual ~LLFloaterExperiencePicker();

	BOOL postBuild();

	void addFilter(filter_function func){mFilters.push_back(func);}
	template <class IT>
	void addFilters(IT begin, IT end){mFilters.insert(mFilters.end(), begin, end);}
	void setDefaultFilters();

	static bool FilterWithProperty(const LLSD& experience, S32 prop);
	static bool FilterWithoutProperty(const LLSD& experience, S32 prop);
	bool FilterOverRating(const LLSD& experience);

	virtual void	draw();
private:
	void editKeystroke(LLLineEditor* caller, void* user_data);

	void onBtnFind();
	void onBtnSelect();
	void onBtnProfile();
	void onBtnClose();
	void onList();
	void onMaturity();

	void getSelectedExperienceIds( const LLScrollListCtrl* results, uuid_vec_t &experience_ids );
	void setAllowMultiple(bool allow_multiple);


	void find();
	bool isSelectButtonEnabled();
	void processResponse( const LLUUID& query_id, const LLSD& content );

	void filterContent();
	bool isExperienceHidden(const LLSD& experience) const ;
	std::string getMaturityString(int maturity);


	select_callback_t	mSelectionCallback;
	filter_list			mFilters;
	LLUUID				mQueryID;
	LLSD				mResponse;
	bool				mCloseOnSelect;


	void drawFrustum();
	LLHandle <LLView>   mFrustumOrigin;
	F32		            mContextConeOpacity;
	F32                 mContextConeInAlpha;
	F32                 mContextConeOutAlpha;
	F32                 mContextConeFadeTime;
};

#endif // LL_LLFLOATEREXPERIENCEPICKER_H

