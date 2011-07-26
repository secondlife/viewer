/** 
 * @file lluiimage.h
 * @brief wrapper for images used in the UI that handles smart scaling, etc.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLUIIMAGE_H
#define LL_LLUIIMAGE_H

#include "v4color.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llrefcount.h"
#include "llrect.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include "llinitparam.h"
#include "lltexture.h"

extern const LLColor4 UI_VERTEX_COLOR;

class LLUIImage : public LLRefCount
{
public:
	typedef boost::signals2::signal<void (void)> image_loaded_signal_t;

	LLUIImage(const std::string& name, LLPointer<LLTexture> image);
	virtual ~LLUIImage();

	void setClipRegion(const LLRectf& region);
	void setScaleRegion(const LLRectf& region);

	LLPointer<LLTexture> getImage() { return mImage; }
	const LLPointer<LLTexture>& getImage() const { return mImage; }

	void draw(S32 x, S32 y, S32 width, S32 height, const LLColor4& color = UI_VERTEX_COLOR) const;
	void draw(S32 x, S32 y, const LLColor4& color = UI_VERTEX_COLOR) const;
	void draw(const LLRect& rect, const LLColor4& color = UI_VERTEX_COLOR) const { draw(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }
	
	void drawSolid(S32 x, S32 y, S32 width, S32 height, const LLColor4& color) const;
	void drawSolid(const LLRect& rect, const LLColor4& color) const { drawSolid(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color); }
	void drawSolid(S32 x, S32 y, const LLColor4& color) const { drawSolid(x, y, getWidth(), getHeight(), color); }

	void drawBorder(S32 x, S32 y, S32 width, S32 height, const LLColor4& color, S32 border_width) const;
	void drawBorder(const LLRect& rect, const LLColor4& color, S32 border_width) const { drawBorder(rect.mLeft, rect.mBottom, rect.getWidth(), rect.getHeight(), color, border_width); }
	void drawBorder(S32 x, S32 y, const LLColor4& color, S32 border_width) const { drawBorder(x, y, getWidth(), getHeight(), color, border_width); }
	
	const std::string& getName() const { return mName; }

	virtual S32 getWidth() const;
	virtual S32 getHeight() const;

	// returns dimensions of underlying textures, which might not be equal to ui image portion
	S32 getTextureWidth() const;
	S32 getTextureHeight() const;

	boost::signals2::connection addLoadedCallback( const image_loaded_signal_t::slot_type& cb );

	void onImageLoaded();

protected:
	image_loaded_signal_t* mImageLoaded;

	std::string			mName;
	LLRectf				mScaleRegion;
	LLRectf				mClipRegion;
	LLPointer<LLTexture> mImage;
	BOOL				mUniformScaling;
	BOOL				mNoClip;
};

namespace LLInitParam
{
	template<>
	class ParamValue<LLUIImage*, TypeValues<LLUIImage*> > 
	:	public CustomParamValue<LLUIImage*>
	{
		typedef boost::add_reference<boost::add_const<LLUIImage*>::type>::type	T_const_ref;
		typedef CustomParamValue<LLUIImage*> super_t;
	public:
		Optional<std::string> name;

		ParamValue(LLUIImage* const& image)
		:	super_t(image)
		{
			updateBlockFromValue(false);
			addSynonym(name, "name");
		}

		void updateValueFromBlock();
		void updateBlockFromValue(bool make_block_authoritative);
	};

	// Need custom comparison function for our test app, which only loads
	// LLUIImage* as NULL.
	template<>
	struct ParamCompare<LLUIImage*, false>
	{
		static bool equals(LLUIImage* const &a, LLUIImage* const &b);
	};
}

typedef LLPointer<LLUIImage> LLUIImagePtr;
#endif
