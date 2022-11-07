/** 
* @file   llpanelexperiencepicker.h
* @brief  Header file for llpanelexperiencepicker
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
#ifndef LL_LLPANELEXPERIENCEPICKER_H
#define LL_LLPANELEXPERIENCEPICKER_H

#include "llpanel.h"

class LLScrollListCtrl;
class LLLineEditor;


class LLPanelExperiencePicker : public LLPanel
{
public:
    friend class LLExperienceSearchResponder;
    friend class LLFloaterExperiencePicker;

    typedef boost::function<void (const uuid_vec_t&)> select_callback_t;
    // filter function for experiences, return true if the experience should be hidden.
    typedef boost::function<bool (const LLSD&)> filter_function;
    typedef std::vector<filter_function> filter_list;

    LLPanelExperiencePicker();
    virtual ~LLPanelExperiencePicker();

    BOOL postBuild();

    void addFilter(filter_function func){mFilters.push_back(func);}
    template <class IT>
    void addFilters(IT begin, IT end){mFilters.insert(mFilters.end(), begin, end);}
    void setDefaultFilters();

    static bool FilterWithProperty(const LLSD& experience, S32 prop);
    static bool FilterWithoutProperties(const LLSD& experience, S32 prop);
    static bool FilterWithoutProperty(const LLSD& experience, S32 prop);
    static bool FilterMatching(const LLSD& experience, const LLUUID& id);
    bool FilterOverRating(const LLSD& experience);

private:
    void editKeystroke(LLLineEditor* caller, void* user_data);

    void onBtnFind();
    void onBtnSelect();
    void onBtnClose();
    void onBtnProfile();
    void onList();
    void onMaturity();
    void onPage(S32 direction);

    void getSelectedExperienceIds( const LLScrollListCtrl* results, uuid_vec_t &experience_ids );
    void setAllowMultiple(bool allow_multiple);

    void find();
    static void findResults(LLHandle<LLPanelExperiencePicker> hparent, LLUUID queryId, LLSD foundResult);

    bool isSelectButtonEnabled();
    void processResponse( const LLUUID& query_id, const LLSD& content );

    void filterContent();
    bool isExperienceHidden(const LLSD& experience) const ;
    std::string getMaturityString(int maturity);


    select_callback_t   mSelectionCallback;
    filter_list         mFilters;
    LLUUID              mQueryID;
    LLSD                mResponse;
    bool                mCloseOnSelect;
    S32                 mCurrentPage;
};

#endif // LL_LLPANELEXPERIENCEPICKER_H
