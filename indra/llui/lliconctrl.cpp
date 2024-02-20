/** 
 * @file lliconctrl.cpp
 * @brief LLIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "lliconctrl.h"

// Linden library includes 

// Project includes
#include "llcontrol.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "lluiimage.h"
#include "llwindow.h"

#include "llgltexture.h"

static LLDefaultChildRegistry::Register<LLIconCtrl> r("icon");

LLIconCtrl::Params::Params()
:	image("image_name"),
	color("color"),
	use_draw_context_alpha("use_draw_context_alpha", true),
    interactable("interactable", false),
	scale_image("scale_image"),
	min_width("min_width", 0),
	min_height("min_height", 0)
{}

LLIconCtrl::LLIconCtrl(const LLIconCtrl::Params& p)
:	LLUICtrl(p),
	mColor(p.color()),
	mImagep(p.image),
	mUseDrawContextAlpha(p.use_draw_context_alpha),
    mInteractable(p.interactable),
	mPriority(0),
	mMinWidth(p.min_width),
	mMinHeight(p.min_height),
	mMaxWidth(0),
	mMaxHeight(0)
{
	if (mImagep.notNull())
	{
		LLUICtrl::setValue(mImagep->getName());
	}
}

LLIconCtrl::~LLIconCtrl()
{
	mImagep = NULL;
}


void LLIconCtrl::draw()
{
	if( mImagep.notNull() )
	{
		const F32 alpha = mUseDrawContextAlpha ? getDrawContext().mAlpha : getCurrentTransparency();
		mImagep->draw(getLocalRect(), mColor.get() % alpha );
	}

	LLUICtrl::draw();
}

bool LLIconCtrl::handleHover(S32 x, S32 y, MASK mask)
{
    if (mInteractable && getEnabled())
    {
        getWindow()->setCursor(UI_CURSOR_HAND);
        return true;
    }
    return LLUICtrl::handleHover(x, y, mask);
}

void LLIconCtrl::onVisibilityChange(bool new_visibility)
{
	LLUICtrl::onVisibilityChange(new_visibility);
	if (mPriority == LLGLTexture::BOOST_ICON)
	{
		if (new_visibility)
		{
			loadImage(getValue(), mPriority);
		}
		else
		{
			mImagep = nullptr;
		}
	}
}

// virtual
// value might be a string or a UUID
void LLIconCtrl::setValue(const LLSD& value)
{
    setValue(value, mPriority);
}

void LLIconCtrl::setValue(const LLSD& value, S32 priority)
{
	LLSD tvalue(value);
	if (value.isString() && LLUUID::validate(value.asString()))
	{
		//RN: support UUIDs masquerading as strings
		tvalue = LLSD(LLUUID(value.asString()));
	}
	LLUICtrl::setValue(tvalue);

	loadImage(tvalue, priority);
}

void LLIconCtrl::loadImage(const LLSD& tvalue, S32 priority)
{
	if(mPriority == LLGLTexture::BOOST_ICON && !getVisible()) return;

	if (tvalue.isUUID())
	{
        mImagep = LLUI::getUIImageByID(tvalue.asUUID(), priority);
	}
	else
	{
        mImagep = LLUI::getUIImage(tvalue.asString(), priority);
	}

	if(mImagep.notNull() 
		&& mImagep->getImage().notNull() 
		&& mMinWidth 
		&& mMinHeight)
	{
        S32 desired_draw_width = llmax(mMinWidth, mImagep->getWidth());
        S32 desired_draw_height = llmax(mMinHeight, mImagep->getHeight());
        if (mMaxWidth && mMaxHeight)
        {
            desired_draw_width = llmin(desired_draw_width, mMaxWidth);
            desired_draw_height = llmin(desired_draw_height, mMaxHeight);
        }

        mImagep->getImage()->setKnownDrawSize(desired_draw_width, desired_draw_height);
	}
}

std::string LLIconCtrl::getImageName() const
{
	if (getValue().isString())
		return getValue().asString();
	else
		return std::string();
}


