/**
 * @file llhints.cpp
 * @brief Hint popups for displaying context sensitive help in a UI overlay
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llhints.h"

#include "llbutton.h"
#include "lltextbox.h"
#include "llviewerwindow.h"
#include "llsdparam.h"

class LLHintPopup : public LLPanel
{
public:

	typedef enum e_popup_direction
	{
		LEFT,
		TOP,
		RIGHT,
		BOTTOM
	} EPopupDirection;

	struct PopupDirections : public LLInitParam::TypeValuesHelper<LLHintPopup::EPopupDirection, PopupDirections>
	{
		static void declareValues()
		{
			declare("left", LLHintPopup::LEFT);
			declare("right", LLHintPopup::RIGHT);
			declare("top", LLHintPopup::TOP);
			declare("bottom", LLHintPopup::BOTTOM);
		}
	};

	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Mandatory<LLNotificationPtr>	notification;
		Optional<std::string>			target;
		Optional<EPopupDirection, PopupDirections>	direction;
		Optional<S32>					distance;
		Optional<LLUIImage*>			left_arrow,
										up_arrow,
										right_arrow,
										down_arrow;	
		Optional<S32>					left_arrow_offset,
										up_arrow_offset,
										right_arrow_offset,
										down_arrow_offset;
		Optional<F32>					fade_in_time,
										fade_out_time;

		Params()
		:	direction("direction", TOP),
			distance("distance", 24),
			target("target"),
			left_arrow("left_arrow", LLUI::getUIImage("hint_arrow_left")),
			up_arrow("up_arrow", LLUI::getUIImage("hint_arrow_up")),
			right_arrow("right_arrow", LLUI::getUIImage("hint_arrow_right")),
			down_arrow("down_arrow", LLUI::getUIImage("hint_arrow_down")),
			left_arrow_offset("left_arrow_offset", 3),
			up_arrow_offset("up_arrow_offset", -2),
			right_arrow_offset("right_arrow_offset", -3),
			down_arrow_offset("down_arrow_offset", 5),
			fade_in_time("fade_in_time", 0.2f),
			fade_out_time("fade_out_time", 0.5f)
		{}
	};

	LLHintPopup(const Params&);

	void setHintTarget(LLHandle<LLView> target) { mTarget = target; }
	/*virtual*/ BOOL postBuild();

	void onClickClose() { hide(); }
	void draw();
	void hide() { if(!mHidden) {mHidden = true; mFadeTimer.reset();} }

private:
	LLNotificationPtr	mNotification;
	LLHandle<LLView>	mTarget;
	EPopupDirection		mDirection;
	S32					mDistance;
	LLUIImagePtr		mArrowLeft,
						mArrowUp,
						mArrowRight,
						mArrowDown;
	S32					mArrowLeftOffset,
						mArrowUpOffset,
						mArrowRightOffset,
						mArrowDownOffset;
	LLFrameTimer		mFadeTimer;
	F32					mFadeInTime,
						mFadeOutTime;
	bool				mHidden;
};




LLHintPopup::LLHintPopup(const LLHintPopup::Params& p)
:	mNotification(p.notification),
	mDirection(p.direction),
	mDistance(p.distance),
	mTarget(LLHints::getHintTarget(p.target)),
	mArrowLeft(p.left_arrow),
	mArrowUp(p.up_arrow),
	mArrowRight(p.right_arrow),
	mArrowDown(p.down_arrow),
	mArrowLeftOffset(p.left_arrow_offset),
	mArrowUpOffset(p.up_arrow_offset),
	mArrowRightOffset(p.right_arrow_offset),
	mArrowDownOffset(p.down_arrow_offset),
	mHidden(false),
	mFadeInTime(p.fade_in_time),
	mFadeOutTime(p.fade_out_time),
	LLPanel(p)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_hint.xml");
}

BOOL LLHintPopup::postBuild()
{
	LLTextBox& hint_text = getChildRef<LLTextBox>("hint_text");
	hint_text.setText(mNotification->getMessage());
	
	getChild<LLButton>("close")->setClickedCallback(boost::bind(&LLHintPopup::onClickClose, this));
	getChild<LLTextBox>("hint_title")->setText(mNotification->getLabel());

	LLRect text_bounds = hint_text.getTextBoundingRect();
	S32 delta_height = text_bounds.getHeight() - hint_text.getRect().getHeight();
	reshape(getRect().getWidth(), getRect().getHeight() + delta_height);
	return TRUE;
}

void LLHintPopup::draw()
{
	F32 alpha = 1.f;
	if (mHidden)
	{
		alpha = clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, mFadeOutTime, 1.f, 0.f);
		if (alpha == 0.f)
		{
			die();
		}
	}
	else
	{
		alpha = clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, mFadeInTime, 0.f, 1.f);
	}
	LLViewDrawContext context(alpha);

	LLView* targetp = mTarget.get();
	if (!targetp || !targetp->isInVisibleChain()) 
	{
		hide();
	}
	else
	{
		LLRect target_rect;
		targetp->localRectToOtherView(targetp->getLocalRect(), &target_rect, getParent());

		LLRect my_local_rect = getLocalRect();
		LLRect my_rect;
		LLRect arrow_rect;
		LLUIImagePtr arrow_imagep;

		switch(mDirection)
		{
		case LEFT:
			my_rect.setCenterAndSize(	target_rect.mLeft - (my_local_rect.getWidth() / 2 + mDistance), 
										target_rect.getCenterY(), 
										my_local_rect.getWidth(), 
										my_local_rect.getHeight());
			arrow_rect.setCenterAndSize(my_local_rect.mRight + mArrowRight->getWidth() / 2 + mArrowRightOffset,
										my_local_rect.getCenterY(),
										mArrowRight->getWidth(), 
										mArrowRight->getHeight());
			arrow_imagep = mArrowRight;
			break;
		case TOP:
			my_rect.setCenterAndSize(	target_rect.getCenterX(), 
										target_rect.mTop + (my_local_rect.getHeight() / 2 + mDistance), 
										my_local_rect.getWidth(), 
										my_local_rect.getHeight());
			arrow_rect.setCenterAndSize(my_local_rect.getCenterX(),
										my_local_rect.mBottom - mArrowDown->getHeight() / 2 + mArrowDownOffset,
										mArrowDown->getWidth(), 
										mArrowDown->getHeight());
			arrow_imagep = mArrowDown;
			break;
		case RIGHT:
			my_rect.setCenterAndSize(	target_rect.getCenterX(), 
										target_rect.mTop - (my_local_rect.getHeight() / 2 + mDistance), 
										my_local_rect.getWidth(), 
										my_local_rect.getHeight());
			arrow_rect.setCenterAndSize(my_local_rect.mLeft - mArrowLeft->getWidth() / 2 + mArrowLeftOffset,
										my_local_rect.getCenterY(),
										mArrowLeft->getWidth(), 
										mArrowLeft->getHeight());
			arrow_imagep = mArrowLeft;
			break;
		case BOTTOM:
			my_rect.setCenterAndSize(	target_rect.getCenterX(), 
										target_rect.mBottom - (my_local_rect.getHeight() / 2 + mDistance),
										my_local_rect.getWidth(), 
										my_local_rect.getHeight());
			arrow_rect.setCenterAndSize(my_local_rect.getCenterX(),
										my_local_rect.mTop + mArrowUp->getHeight() / 2 + mArrowUpOffset,
										mArrowUp->getWidth(), 
										mArrowUp->getHeight());
			arrow_imagep = mArrowUp;
			break;
		}
		setShape(my_rect);
		LLPanel::draw();

		arrow_imagep->draw(arrow_rect, LLColor4(1.f, 1.f, 1.f, alpha));
	}
}


LLRegistry<std::string, LLHandle<LLView> > LLHints::sTargetRegistry;
std::map<LLNotificationPtr, class LLHintPopup*> LLHints::sHints;

//static
void LLHints::show(LLNotificationPtr hint)
{
	LLHintPopup::Params p;
	LLParamSDParser::instance().readSD(hint->getPayload(), p);

	p.notification = hint;

	LLHintPopup* popup = new LLHintPopup(p);
	
	sHints[hint] = popup;

	LLView* hint_holder = gViewerWindow->getHintHolder();
	if (hint_holder)
	{
		hint_holder->addChild(popup);
		popup->centerWithin(hint_holder->getLocalRect());
	}
}

void LLHints::hide(LLNotificationPtr hint)
{
	hint_map_t::iterator found_it = sHints.find(hint);
	if (found_it != sHints.end())
	{
		found_it->second->hide();
		sHints.erase(found_it);
	}
}

//static
void LLHints::registerHintTarget(const std::string& name, LLHandle<LLView> target)
{
	sTargetRegistry.defaultRegistrar().add(name, target);
}

//static 
LLHandle<LLView> LLHints::getHintTarget(const std::string& name)
{
	LLHandle<LLView>* handlep = sTargetRegistry.getValue(name);
	if (handlep) 
	{
		return *handlep;
	}
	else
	{
		return LLHandle<LLView>();
	}
}
