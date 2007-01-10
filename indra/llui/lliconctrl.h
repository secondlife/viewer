/** 
 * @file lliconctrl.h
 * @brief LLIconCtrl base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLICONCTRL_H
#define LL_LLICONCTRL_H

#include "lluuid.h"
#include "v4color.h"
#include "lluictrl.h"
#include "stdenums.h"
#include "llimagegl.h"

class LLTextBox;
class LLUICtrlFactory;

//
// Classes
//
class LLIconCtrl
: public LLUICtrl
{
public:
	LLIconCtrl(const LLString& name, const LLRect &rect, const LLUUID &image_id);
	LLIconCtrl(const LLString& name, const LLRect &rect, const LLString &image_name);
	virtual ~LLIconCtrl();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_ICON; }
	virtual LLString getWidgetTag() const { return LL_ICON_CTRL_TAG; }

	// llview overrides
	virtual void	draw();

	void			setImage(const LLUUID &image_id);
	const LLUUID	&getImage() const						{ return mImageID; }

	// Takes a UUID, wraps get/setImage
	virtual void	setValue(const LLSD& value );
	virtual LLSD	getValue() const;

	void			setColor(const LLColor4& color) { mColor = color; }

	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

protected:
	LLColor4		mColor;
	LLString		mImageName;
	LLUUID			mImageID;
	LLPointer<LLImageGL>	mImagep;
};

#endif
