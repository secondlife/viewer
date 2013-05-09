/**
 * @file llhints.cpp
 * @brief Hint popups for displaying context sensitive help in a UI overlay
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llhints.h"

#include "llbutton.h"
#include "lltextbox.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "lliconctrl.h"
#include "llsdparam.h"

class LLHintPopup : public LLPanel
{
public:

	typedef enum e_popup_direction
	{
		LEFT,
		TOP,
		RIGHT,
		BOTTOM,
		TOP_RIGHT
	} EPopupDirection;

	struct PopupDirections : public LLInitParam::TypeValuesHelper<LLHintPopup::EPopupDirection, PopupDirections>
	{
		static void declareValues()
		{
			declare("left", LLHintPopup::LEFT);
			declare("right", LLHintPopup::RIGHT);
			declare("top", LLHintPopup::TOP);
			declare("bottom", LLHintPopup::BOTTOM);
			declare("top_right", LLHintPopup::TOP_RIGHT);
		}
	};

	struct TargetParams : public LLInitParam::Block<TargetParams>
	{
		Mandatory<std::string>	target;
		Mandatory<EPopupDirection, PopupDirections> direction;

		TargetParams()
		:	target("target"),
			direction("direction")
		{}
	};

	struct Params : public LLInitParam::Block<Params, LLPanel::Params>
	{
		Mandatory<LLNotificationPtr>	notification;
		Optional<TargetParams>			target_params;
		Optional<S32>					distance;
		Optional<LLUIImage*>			left_arrow,
										up_arrow,
										right_arrow,
										down_arrow,
										lower_left_arrow,
										hint_image;
				
		Optional<S32>					left_arrow_offset,
										up_arrow_offset,
										right_arrow_offset,
										down_arrow_offset;
		Optional<F32>					fade_in_time,
										fade_out_time;

		Params()
		:	distance("distance"),
			left_arrow("left_arrow"),
			up_arrow("up_arrow"),
			right_arrow("right_arrow"),
			down_arrow("down_arrow"),
			lower_left_arrow("lower_left_arrow"),
			hint_image("hint_image"),
			left_arrow_offset("left_arrow_offset"),
			up_arrow_offset("up_arrow_offset"),
			right_arrow_offset("right_arrow_offset"),
			down_arrow_offset("down_arrow_offset"),
			fade_in_time("fade_in_time"),
			fade_out_time("fade_out_time")
		{}
	};

	LLHintPopup(const Params&);

	/*virtual*/ BOOL postBuild();

	void onClickClose() 
	{ 
		if (!mHidden) 
		{
			hide(); 
			LLNotifications::instance().cancel(mNotification);
		}
	}
	void draw();
	void hide() { if(!mHidden) {mHidden = true; mFadeTimer.reset();} }

private:
	LLNotificationPtr	mNotification;
	std::string			mTarget;
	EPopupDirection		mDirection;
	S32					mDistance;
	LLUIImagePtr		mArrowLeft,
						mArrowUp,
						mArrowRight,
						mArrowDown,
						mArrowDownAndLeft;
	S32					mArrowLeftOffset,
						mArrowUpOffset,
						mArrowRightOffset,
						mArrowDownOffset;
	LLFrameTimer		mFadeTimer;
	F32					mFadeInTime,
						mFadeOutTime;
	bool				mHidden;
};

static LLDefaultChildRegistry::Register<LLHintPopup> r("hint_popup");


LLHintPopup::LLHintPopup(const LLHintPopup::Params& p)
:	mNotification(p.notification),
	mDirection(TOP),
	mDistance(p.distance),
	mArrowLeft(p.left_arrow),
	mArrowUp(p.up_arrow),
	mArrowRight(p.right_arrow),
	mArrowDown(p.down_arrow),
	mArrowDownAndLeft(p.lower_left_arrow),
	mArrowLeftOffset(p.left_arrow_offset),
	mArrowUpOffset(p.up_arrow_offset),
	mArrowRightOffset(p.right_arrow_offset),
	mArrowDownOffset(p.down_arrow_offset),
	mHidden(false),
	mFadeInTime(p.fade_in_time),
	mFadeOutTime(p.fade_out_time),
	LLPanel(p)
{
	if (p.target_params.isProvided())
	{
		mDirection = p.target_params.direction;
		mTarget = p.target_params.target;
	}
	if (p.hint_image.isProvided())
	{
		buildFromFile("panel_hint_image.xml", p);
		getChild<LLIconCtrl>("hint_image")->setImage(p.hint_image());
	}
	else
	{
		buildFromFile( "panel_hint.xml", p);
	}
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
	hint_text.reshape(hint_text.getRect().getWidth(), hint_text.getRect().getHeight() + delta_height);
//	hint_text.translate(0, -delta_height);
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
			return;
		}
	}
	else
	{
		alpha = clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, mFadeInTime, 0.f, 1.f);
	}
	
	LLIconCtrl* hint_icon = findChild<LLIconCtrl>("hint_image");

	if (hint_icon)
	{
		LLUIImagePtr hint_image = hint_icon->getImage();
		S32 image_height = hint_image.isNull() ? 0 : hint_image->getHeight();
		S32 image_width = hint_image.isNull() ? 0 : hint_image->getWidth();

		LLView* layout_stack = hint_icon->getParent()->getParent();
		S32 delta_height = image_height - layout_stack->getRect().getHeight();
		hint_icon->getParent()->reshape(image_width, hint_icon->getParent()->getRect().getHeight());
		layout_stack->reshape(layout_stack->getRect().getWidth(), image_height);
		layout_stack->translate(0, -delta_height);

		LLRect hint_rect = getLocalRect();
		reshape(hint_rect.getWidth(), hint_rect.getHeight() + delta_height);
	}

	{	LLViewDrawContext context(alpha); 

		if (mTarget.empty())
		{
			// just draw contents, no arrow, in default position
			LLPanel::draw();
		}
		else 
		{
			LLView* targetp = LLHints::getHintTarget(mTarget).get();
			if (!targetp)
			{
				// target widget is no longer valid, go away
				die();
			}
			else if (!targetp->isInVisibleChain()) 
			{
				// if target is invisible, don't draw, but keep alive in case widget comes back
				// but do make it so that it allows mouse events to pass through
				setEnabled(false);
				setMouseOpaque(false);
			}
			else
			{
				// revert back enabled and mouse opaque state in case we disabled it before
				setEnabled(true);
				setMouseOpaque(true);

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
					if (mArrowRight)
					{
						arrow_rect.setCenterAndSize(my_local_rect.mRight + mArrowRight->getWidth() / 2 + mArrowRightOffset,
													my_local_rect.getCenterY(),
													mArrowRight->getWidth(), 
													mArrowRight->getHeight());
						arrow_imagep = mArrowRight;
					}
					break;
				case TOP:
					my_rect.setCenterAndSize(	target_rect.getCenterX(), 
												target_rect.mTop + (my_local_rect.getHeight() / 2 + mDistance), 
												my_local_rect.getWidth(), 
												my_local_rect.getHeight());
					if (mArrowDown)
					{
						arrow_rect.setCenterAndSize(my_local_rect.getCenterX(),
													my_local_rect.mBottom - mArrowDown->getHeight() / 2 + mArrowDownOffset,
													mArrowDown->getWidth(), 
													mArrowDown->getHeight());
						arrow_imagep = mArrowDown;
					}
					break;
				case RIGHT:
					my_rect.setCenterAndSize(	target_rect.mRight + (my_local_rect.getWidth() / 2 + mDistance), 
												target_rect.getCenterY(),
												my_local_rect.getWidth(), 
												my_local_rect.getHeight());
					if (mArrowLeft)
					{
						arrow_rect.setCenterAndSize(my_local_rect.mLeft - mArrowLeft->getWidth() / 2 + mArrowLeftOffset,
													my_local_rect.getCenterY(),
													mArrowLeft->getWidth(), 
													mArrowLeft->getHeight());
						arrow_imagep = mArrowLeft;
					}
					break;
				case BOTTOM:
					my_rect.setCenterAndSize(	target_rect.getCenterX(), 
												target_rect.mBottom - (my_local_rect.getHeight() / 2 + mDistance),
												my_local_rect.getWidth(), 
												my_local_rect.getHeight());
					if (mArrowUp)
					{
						arrow_rect.setCenterAndSize(my_local_rect.getCenterX(),
													my_local_rect.mTop + mArrowUp->getHeight() / 2 + mArrowUpOffset,
													mArrowUp->getWidth(), 
													mArrowUp->getHeight());
						arrow_imagep = mArrowUp;
					}
					break;
				case TOP_RIGHT:
					my_rect.setCenterAndSize(	target_rect.mRight + (my_local_rect.getWidth() / 2),
												target_rect.mTop + (my_local_rect.getHeight() / 2 + mDistance),
												my_local_rect.getWidth(), 
												my_local_rect.getHeight());
					if (mArrowDownAndLeft)
					{
						arrow_rect.setCenterAndSize(my_local_rect.mLeft + mArrowDownAndLeft->getWidth() / 2 + mArrowLeftOffset,
													my_local_rect.mBottom - mArrowDownAndLeft->getHeight() / 2 + mArrowDownOffset,
													mArrowDownAndLeft->getWidth(), 
													mArrowDownAndLeft->getHeight());
						arrow_imagep = mArrowDownAndLeft;
					}
				}
				setShape(my_rect);
				LLPanel::draw();

				if (arrow_imagep) arrow_imagep->draw(arrow_rect, LLColor4(1.f, 1.f, 1.f, alpha));
			}
		}
	}
}


LLRegistry<std::string, LLHandle<LLView> > LLHints::sTargetRegistry;
std::map<LLNotificationPtr, class LLHintPopup*> LLHints::sHints;

//static
void LLHints::show(LLNotificationPtr hint)
{
	LLHintPopup::Params p(LLUICtrlFactory::getDefaultParams<LLHintPopup>());

	LLParamSDParser parser;
	parser.readSD(hint->getPayload(), p, true);
	p.notification = hint;

	if (p.validateBlock())
	{
		LLHintPopup* popup = new LLHintPopup(p);

		sHints[hint] = popup;

		LLView* hint_holder = gViewerWindow->getHintHolder();
		if (hint_holder)
		{
			hint_holder->addChild(popup);
			popup->centerWithin(hint_holder->getLocalRect());
		}
	}
}

//static
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
	sTargetRegistry.defaultRegistrar().replace(name, target);
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

//static
void LLHints::initClass()
{
	sRegister.reference();

	LLControlVariablePtr control = gSavedSettings.getControl("EnableUIHints");
	control->getSignal()->connect(boost::bind(&showHints, _2));
	gViewerWindow->getHintHolder()->setVisible(control->getValue().asBoolean());

}

//staic
void LLHints::showHints(const LLSD& show)
{
	bool visible = show.asBoolean();
	gViewerWindow->getHintHolder()->setVisible(visible);
}
