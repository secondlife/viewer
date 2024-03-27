/** 
 * @file lliconctrl.h
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

#ifndef LL_LLICONCTRL_H
#define LL_LLICONCTRL_H

#include "lluuid.h"
#include "v4color.h"
#include "lluictrl.h"
#include "lluiimage.h"

class LLTextBox;
class LLUICtrlFactory;

//
// Classes
//

// Class for diplaying named UI textures
// Do not use for displaying textures from network,
// UI textures are stored permanently!
class LLIconCtrl
: public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLUIImage*>	image;
		Optional<LLUIColor>		color;
		Optional<bool>			use_draw_context_alpha,
                                interactable;
		Optional<S32>			min_width,
								min_height;
		Ignored					scale_image;

		Params();
	};
protected:
	LLIconCtrl(const Params&);
	friend class LLUICtrlFactory;

	void setValue(const LLSD& value, S32 priority);

public:
	virtual ~LLIconCtrl();

	// llview overrides
	virtual void	draw();

    // llview overrides
    virtual BOOL handleHover(S32 x, S32 y, MASK mask);

	// lluictrl overrides
	void onVisibilityChange(BOOL new_visibility);
	virtual void	setValue(const LLSD& value );

	std::string	getImageName() const;

	void			setColor(const LLColor4& color) { mColor = color; }
	void			setImage(LLPointer<LLUIImage> image) { mImagep = image; }
	const LLPointer<LLUIImage> getImage() { return mImagep; }
	
protected:
	S32 mPriority;

	//the output size of the icon image if set.
	S32 mMinWidth,
		mMinHeight,
		mMaxWidth,
		mMaxHeight;

	// If set to true (default), use the draw context transparency.
	// If false, will use transparency returned by getCurrentTransparency(). See STORM-698.
	bool mUseDrawContextAlpha;
    bool mInteractable;

private:
	void loadImage(const LLSD& value, S32 priority);

	LLUIColor mColor;
	LLPointer<LLUIImage> mImagep;
};

#endif
