/** 
 * @file lluiimage.h
 * @brief wrapper for images used in the UI that handles smart scaling, etc.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
	class TypedParam<LLUIImage*, TypeValues<LLUIImage*>, false> 
	:	public BlockValue<LLUIImage*>
	{
		typedef boost::add_reference<boost::add_const<LLUIImage*>::type>::type	T_const_ref;
		typedef BlockValue<LLUIImage*> super_t;
	public:
		Optional<std::string> name;

		TypedParam(BlockDescriptor& descriptor, const char* name, super_t::value_assignment_t value, ParamDescriptor::validation_func_t func, S32 min_count, S32 max_count)
		:	super_t(descriptor, name, value, func, min_count, max_count)
		{
		}

		void setValueFromBlock() const;
		void setBlockFromValue();
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
