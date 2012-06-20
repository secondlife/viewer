/** 
 * @file llchicletbar.cpp
 * @brief LLChicletBar class implementation
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include

#include "llchicletbar.h"

// library includes
#include "llfloaterreg.h"
#include "lllayoutstack.h"

// newview includes
#include "llchiclet.h"
#include "llimfloater.h" // for LLIMFloater
#include "llpaneltopinfobar.h"
#include "llsyswellwindow.h"

namespace
{
	const std::string& PANEL_CHICLET_NAME	= "chiclet_list_panel";

	S32 get_curr_width(LLUICtrl* ctrl)
	{
		S32 cur_width = 0;
		if ( ctrl && ctrl->getVisible() )
		{
			cur_width = ctrl->getRect().getWidth();
		}
		return cur_width;
	}
}

LLChicletBar::LLChicletBar(const LLSD&)
:	mChicletPanel(NULL),
	mToolbarStack(NULL)
{
	// IM floaters are from now managed by LLIMFloaterContainer.
	// See LLIMFloaterContainer::sessionVoiceOrIMStarted() and CHUI-125

//	// Firstly add our self to IMSession observers, so we catch session events
//	// before chiclets do that.
//	LLIMMgr::getInstance()->addSessionObserver(this);

	buildFromFile("panel_chiclet_bar.xml");
}

LLChicletBar::~LLChicletBar()
{
	// IM floaters are from now managed by LLIMFloaterContainer.
	// See LLIMFloaterContainer::sessionVoiceOrIMStarted() and CHUI-125
//	if (!LLSingleton<LLIMMgr>::destroyed())
//	{
//		LLIMMgr::getInstance()->removeSessionObserver(this);
//	}
}

LLIMChiclet* LLChicletBar::createIMChiclet(const LLUUID& session_id)
{
	LLIMChiclet::EType im_chiclet_type = LLIMChiclet::getIMSessionType(session_id);

	switch (im_chiclet_type)
	{
	case LLIMChiclet::TYPE_IM:
		return getChicletPanel()->createChiclet<LLIMP2PChiclet>(session_id);
	case LLIMChiclet::TYPE_GROUP:
		return getChicletPanel()->createChiclet<LLIMGroupChiclet>(session_id);
	case LLIMChiclet::TYPE_AD_HOC:
		return getChicletPanel()->createChiclet<LLAdHocChiclet>(session_id);
	case LLIMChiclet::TYPE_UNKNOWN:
		break;
	}

	return NULL;
}

//virtual
void LLChicletBar::sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	if (!getChicletPanel()) return;

	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!session) return;

	// no need to spawn chiclets for participants in P2P calls called through Avaline
	if (session->isP2P() && session->isOtherParticipantAvaline()) return;

	if (getChicletPanel()->findChiclet<LLChiclet>(session_id)) return;

	LLIMChiclet* chiclet = createIMChiclet(session_id);
	if(chiclet)
	{
		chiclet->setIMSessionName(name);
		chiclet->setOtherParticipantId(other_participant_id);
		
		LLIMFloater::onIMChicletCreated(session_id);

	}
	else
	{
		llwarns << "Could not create chiclet" << llendl;
	}
}

//virtual
void LLChicletBar::sessionRemoved(const LLUUID& session_id)
{
	if(getChicletPanel())
	{
		// IM floater should be closed when session removed and associated chiclet closed
		LLIMFloater* im_floater = LLFloaterReg::findTypedInstance<LLIMFloater>("impanel", session_id);
		if (im_floater != NULL && !im_floater->getStartConferenceInSameFloater())
		{
			// Close the IM floater only if we are not planning to close the P2P chat
			// and start a new conference in the same floater.
			im_floater->closeFloater();
		}

		getChicletPanel()->removeChiclet(session_id);
	}
}

void LLChicletBar::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	//this is only needed in case of outgoing ad-hoc/group chat sessions
	LLChicletPanel* chiclet_panel = getChicletPanel();
	if (chiclet_panel)
	{
		//it should be ad-hoc im chiclet or group im chiclet
		LLChiclet* chiclet = chiclet_panel->findChiclet<LLChiclet>(old_session_id);
		if (chiclet) chiclet->setSessionId(new_session_id);
	}
}

S32 LLChicletBar::getTotalUnreadIMCount()
{
	return getChicletPanel()->getTotalUnreadIMCount();
}

BOOL LLChicletBar::postBuild()
{
	mToolbarStack = getChild<LLLayoutStack>("toolbar_stack");
	mChicletPanel = getChild<LLChicletPanel>("chiclet_list");

	showWellButton("im_well", !LLIMWellWindow::getInstance()->isWindowEmpty());
	showWellButton("notification_well", !LLNotificationWellWindow::getInstance()->isWindowEmpty());

	LLPanelTopInfoBar::instance().setResizeCallback(boost::bind(&LLChicletBar::fitWithTopInfoBar, this));
	LLPanelTopInfoBar::instance().setVisibleCallback(boost::bind(&LLChicletBar::fitWithTopInfoBar, this));

	return TRUE;
}

void LLChicletBar::showWellButton(const std::string& well_name, bool visible)
{
	LLView * panel = findChild<LLView>(well_name + "_panel");
	if (!panel)	return;

	panel->setVisible(visible);
}

void LLChicletBar::log(LLView* panel, const std::string& descr)
{
	if (NULL == panel) return;
	LLView* layout = panel->getParent();
	LL_DEBUGS("Chiclet Bar Rects") << descr << ": "
		<< "panel: " << panel->getName()
		<< ", rect: " << panel->getRect()
		<< " layout: " << layout->getName()
		<< ", rect: " << layout->getRect()
		<< LL_ENDL;
}

void LLChicletBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	static S32 debug_calling_number = 0;
	lldebugs << "**************************************** " << ++debug_calling_number << llendl;

	S32 current_width = getRect().getWidth();
	S32 delta_width = width - current_width;
	lldebugs << "Reshaping: "
		<< ", width: " << width
		<< ", cur width: " << current_width
		<< ", delta_width: " << delta_width
		<< ", called_from_parent: " << called_from_parent
		<< llendl;

	if (mChicletPanel)			log(mChicletPanel, "before");

	// Difference between chiclet bar width required to fit its children and the actual width. (see EXT-991)
	// Positive value means that chiclet bar is not wide enough.
	// Negative value means that there is free space.
	static S32 extra_shrink_width = 0;
	bool should_be_reshaped = true;

	if (mChicletPanel && mToolbarStack)
	{
		// Firstly, update layout stack to ensure we deal with correct panel sizes.
		{
			// Force the updating of layout to reset panels collapse factor.
			mToolbarStack->updateLayout();
		}

		// chiclet bar is narrowed
		if (delta_width < 0)
		{
			if (extra_shrink_width > 0) // not enough space
			{
				extra_shrink_width += llabs(delta_width);
				should_be_reshaped = false;
			}
			else
			{
				extra_shrink_width = processWidthDecreased(delta_width);

				// increase new width to extra_shrink_width value to not reshape less than chiclet bar minimum
				width += extra_shrink_width;
			}
		}
		// chiclet bar is widened
		else
		{
			if (extra_shrink_width > delta_width)
			{
				// Still not enough space.
				// Only subtract the delta from the required delta and don't reshape.
				extra_shrink_width -= delta_width;
				should_be_reshaped = false;
			}
			else if (extra_shrink_width > 0)
			{
				// If we have some extra shrink width let's reduce delta_width & width
				delta_width -= extra_shrink_width;
				width -= extra_shrink_width;
				extra_shrink_width = 0;
			}
		}
	}

	if (should_be_reshaped)
	{
		lldebugs << "Reshape all children with width: " << width << llendl;
		LLPanel::reshape(width, height, called_from_parent);
	}

	if (mChicletPanel)			log(mChicletPanel, "after");
}

S32 LLChicletBar::processWidthDecreased(S32 delta_width)
{
	bool still_should_be_processed = true;

	const S32 chiclet_panel_shrink_headroom = getChicletPanelShrinkHeadroom();

	// Decreasing width of chiclet panel.
	if (chiclet_panel_shrink_headroom > 0)
	{
		// we have some space to decrease chiclet panel
		S32 shrink_by = llmin(-delta_width, chiclet_panel_shrink_headroom);

		lldebugs << "delta_width: " << delta_width
			<< ", panel_delta_min: " << chiclet_panel_shrink_headroom
			<< ", shrink_by: " << shrink_by
			<< llendl;

		// is chiclet panel wide enough to process resizing?
		delta_width += chiclet_panel_shrink_headroom;

		still_should_be_processed = delta_width < 0;

		lldebugs << "Shrinking chiclet panel by " << shrink_by << " px" << llendl;
		mChicletPanel->getParent()->reshape(mChicletPanel->getParent()->getRect().getWidth() - shrink_by, mChicletPanel->getParent()->getRect().getHeight());
		log(mChicletPanel, "after processing panel decreasing via chiclet panel");

		lldebugs << "RS_CHICLET_PANEL"
			<< ", delta_width: " << delta_width
			<< llendl;
	}

	S32 extra_shrink_width = 0;

	if (still_should_be_processed)
	{
		extra_shrink_width = -delta_width;
		llwarns << "There is no enough width to reshape all children: "
			<< extra_shrink_width << llendl;
	}

	return extra_shrink_width;
}

S32 LLChicletBar::getChicletPanelShrinkHeadroom() const
{
	static const S32 min_width = mChicletPanel->getMinWidth();
	const S32 cur_width = mChicletPanel->getParent()->getRect().getWidth();

	S32 shrink_headroom = cur_width - min_width;
	llassert(shrink_headroom >= 0); // the panel cannot get narrower than the minimum
	return shrink_headroom;
}

void LLChicletBar::fitWithTopInfoBar()
{
	LLPanelTopInfoBar& top_info_bar = LLPanelTopInfoBar::instance();

	LLRect rect = getRect();
	S32 width = rect.getWidth();

	if (top_info_bar.getVisible())
	{
		S32 delta = top_info_bar.calcScreenRect().mRight - calcScreenRect().mLeft;
		if (delta < 0 && rect.mLeft < llabs(delta))
			delta = -rect.mLeft;
		rect.setLeftTopAndSize(rect.mLeft + delta, rect.mTop, rect.getWidth(), rect.getHeight());
		width = rect.getWidth() - delta;
	}
	else
	{
		LLView* parent = getParent();
		if (parent)
		{
			LLRect parent_rect = parent->getRect();
			rect.setLeftTopAndSize(0, rect.mTop, rect.getWidth(), rect.getHeight());
			width = parent_rect.getWidth();
		}
	}

	setRect(rect);
	LLPanel::reshape(width, rect.getHeight(), false);
}
