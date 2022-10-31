/** 
* @file llfloaterexperiencepicker.cpp
* @brief Implementation of llfloaterexperiencepicker
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterexperiencepicker.h"


#include "lllineeditor.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "llexperiencecache.h"
#include "llslurl.h"
#include "llavatarnamecache.h"
#include "llfloaterexperienceprofile.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "lldraghandle.h"
#include "llpanelexperiencepicker.h"

LLFloaterExperiencePicker* LLFloaterExperiencePicker::show( select_callback_t callback, const LLUUID& key, BOOL allow_multiple, BOOL close_on_select, filter_list filters, LLView * frustumOrigin )
{
    LLFloaterExperiencePicker* floater =
        LLFloaterReg::showTypedInstance<LLFloaterExperiencePicker>("experience_search", key);
    if (!floater)
    {
        LL_WARNS() << "Cannot instantiate experience picker" << LL_ENDL;
        return NULL;
    }

    if (floater->mSearchPanel)
    {
        floater->mSearchPanel->mSelectionCallback = callback;
        floater->mSearchPanel->mCloseOnSelect = close_on_select;
        floater->mSearchPanel->setAllowMultiple(allow_multiple);
        floater->mSearchPanel->setDefaultFilters();
        floater->mSearchPanel->addFilters(filters.begin(), filters.end());
        floater->mSearchPanel->filterContent();
    }

    if(frustumOrigin)
    {
        floater->mFrustumOrigin = frustumOrigin->getHandle();
    }

    return floater;
}

void LLFloaterExperiencePicker::drawFrustum()
{
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, mFrustumOrigin.get(), mContextConeFadeTime, mContextConeInAlpha, mContextConeOutAlpha);
}

void LLFloaterExperiencePicker::draw()
{
    drawFrustum();
    LLFloater::draw();
}

LLFloaterExperiencePicker::LLFloaterExperiencePicker( const LLSD& key )
    :LLFloater(key)
    ,mSearchPanel(NULL)
    ,mContextConeOpacity(0.f)
    ,mContextConeInAlpha(0.f)
    ,mContextConeOutAlpha(0.f)
    ,mContextConeFadeTime(0.f)
{
    mContextConeInAlpha = gSavedSettings.getF32("ContextConeInAlpha");
    mContextConeOutAlpha = gSavedSettings.getF32("ContextConeOutAlpha");
    mContextConeFadeTime = gSavedSettings.getF32("ContextConeFadeTime");
}

LLFloaterExperiencePicker::~LLFloaterExperiencePicker()
{
    gFocusMgr.releaseFocusIfNeeded( this );
}

BOOL LLFloaterExperiencePicker::postBuild()
{
    mSearchPanel = new LLPanelExperiencePicker();
    addChild(mSearchPanel);
    mSearchPanel->setOrigin(0, 0);
    return LLFloater::postBuild();
}
