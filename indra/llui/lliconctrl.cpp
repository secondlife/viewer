/** 
 * @file lliconctrl.cpp
 * @brief LLIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "lliconctrl.h"

// Linden library includes 

// Project includes
#include "llcontrol.h"
#include "llui.h"
#include "lluictrlfactory.h"

const F32 RESOLUTION_BUMP = 1.f;

LLIconCtrl::LLIconCtrl(const LLString& name, const LLRect &rect, const LLUUID &image_id)
:	LLUICtrl(name, 
			 rect, 
			 FALSE, // mouse opaque
			 NULL, NULL, 
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mColor( LLColor4::white ),
	mImageName("")
{
	setImage( image_id );
	setTabStop(FALSE);
}

LLIconCtrl::LLIconCtrl(const LLString& name, const LLRect &rect, const LLString &image_name)
:	LLUICtrl(name, 
			 rect, 
			 FALSE, // mouse opaque
			 NULL, NULL, 
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mColor( LLColor4::white ),
	mImageName(image_name)
{
	LLUUID image_id;
	image_id.set(LLUI::sAssetsGroup->getString( image_name ));
	setImage( image_id );
	setTabStop(FALSE);
}


LLIconCtrl::~LLIconCtrl()
{
	mImagep = NULL;
}


void LLIconCtrl::setImage(const LLUUID &image_id)
{
	mImageID = image_id;
	mImagep = LLUI::sImageProvider->getUIImageByID(image_id);
}


void LLIconCtrl::draw()
{
	if( getVisible() ) 
	{
		// Border
		BOOL has_image = !mImageID.isNull();

		if( has_image )
		{
			if( mImagep.notNull() )
			{
				gl_draw_scaled_image(0, 0, 
									 mRect.getWidth(), mRect.getHeight(), 
									 mImagep,
									 mColor );
			}
		}

		LLUICtrl::draw();
	}
}

// virtual
void LLIconCtrl::setValue(const LLSD& value )
{
	setImage(value.asUUID());
}

// virtual
LLSD LLIconCtrl::getValue() const
{
	LLSD ret = getImage();
	return ret;
}

// virtual
LLXMLNodePtr LLIconCtrl::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	if (mImageName != "")
	{
		node->createChild("image_name", TRUE)->setStringValue(mImageName);
	}

	node->createChild("color", TRUE)->setFloatValue(4, mColor.mV);

	return node;
}

LLView* LLIconCtrl::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("icon");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLUUID image_id;
	if (node->hasAttribute("image_name"))
	{
		LLString image_name;
		node->getAttributeString("image_name", image_name);
		image_id.set(LLUI::sAssetsGroup->getString( image_name ));
	}

	LLColor4 color(LLColor4::white);
	LLUICtrlFactory::getAttributeColor(node,"color", color);

	LLIconCtrl* icon = new LLIconCtrl(name, rect, image_id);
	icon->setColor(color);

	icon->initFromXML(node, parent);

	return icon;
}
