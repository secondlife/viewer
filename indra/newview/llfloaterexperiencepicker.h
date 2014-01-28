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

	static LLFloaterExperiencePicker* show( select_callback_t callback, const LLUUID& key, BOOL allow_multiple, BOOL closeOnSelect );

	LLFloaterExperiencePicker(const LLSD& key);
	virtual ~LLFloaterExperiencePicker();

	BOOL postBuild();


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
	std::string getMaturityString(int maturity);


	select_callback_t	mSelectionCallback;
	bool				mCloseOnSelect;
	LLUUID				mQueryID;
	LLSD				mResponse;
};

#endif // LL_LLFLOATEREXPERIENCEPICKER_H

