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

static LLDefaultChildRegistry::Register<LLIconCtrl> r("icon");

LLIconCtrl::Params::Params()
:	image("image_name"),
	color("color"),
	use_draw_context_alpha("use_draw_context_alpha", true),
	scale_image("scale_image")
{
	tab_stop = false;
	mouse_opaque = false;
}

LLIconCtrl::LLIconCtrl(const LLIconCtrl::Params& p)
:	LLUICtrl(p),
	mColor(p.color()),
	mImagep(p.image),
	mUseDrawContextAlpha(p.use_draw_context_alpha),
	mPriority(0),
	mDrawWidth(0),
	mDrawHeight(0)
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

// virtual
// value might be a string or a UUID
void LLIconCtrl::setValue(const LLSD& value )
{
	LLSD tvalue(value);
	if (value.isString() && LLUUID::validate(value.asString()))
	{
		//RN: support UUIDs masquerading as strings
		tvalue = LLSD(LLUUID(value.asString()));
	}
	LLUICtrl::setValue(tvalue);
	if (tvalue.isUUID())
	{
		mImagep = LLUI::getUIImageByID(tvalue.asUUID(), mPriority);
	}
	else
	{
		mImagep = LLUI::getUIImage(tvalue.asString(), mPriority);
	}

	setIconImageDrawSize();
}

std::string LLIconCtrl::getImageName() const
{
	if (getValue().isString())
		return getValue().asString();
	else
		return std::string();
}

void LLIconCtrl::setIconImageDrawSize()
{
	if(mImagep.notNull() && mDrawWidth && mDrawHeight)
	{
		if(mImagep->getImage().notNull())
		{
			mImagep->getImage()->setKnownDrawSize(mDrawWidth, mDrawHeight) ;
		}
	}
}

