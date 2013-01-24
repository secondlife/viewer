/**
 * @file llurlmatch_test.cpp
 * @author Martin Reddy
 * @brief Unit tests for LLUrlMatch
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "../llurlmatch.h"
#include "lluiimage.h"
#include "lltut.h"

// link seams

LLUIColor::LLUIColor()
	: mColorPtr(NULL)
{}

LLStyle::Params::Params()
{
}

LLUIImage::LLUIImage(const std::string& name, LLPointer<LLTexture> image)
{
}

LLUIImage::~LLUIImage()
{
}

//virtual
S32 LLUIImage::getWidth() const
{
	return 0;
}

//virtual
S32 LLUIImage::getHeight() const
{
	return 0;
}

namespace LLInitParam
{
	ParamValue<LLUIColor, TypeValues<LLUIColor> >::ParamValue(const LLUIColor& color)
	:	super_t(color)
	{}

	void ParamValue<LLUIColor, TypeValues<LLUIColor> >::updateValueFromBlock()
	{}
	
	void ParamValue<LLUIColor, TypeValues<LLUIColor> >::updateBlockFromValue(bool)
	{}

	bool ParamCompare<const LLFontGL*, false>::equals(const LLFontGL* a, const LLFontGL* b)
	{
		return false;
	}


	ParamValue<const LLFontGL*, TypeValues<const LLFontGL*> >::ParamValue(const LLFontGL* fontp)
	:	super_t(fontp)
	{}

	void ParamValue<const LLFontGL*, TypeValues<const LLFontGL*> >::updateValueFromBlock()
	{}
	
	void ParamValue<const LLFontGL*, TypeValues<const LLFontGL*> >::updateBlockFromValue(bool)
	{}

	void TypeValues<LLFontGL::HAlign>::declareValues()
	{}

	void TypeValues<LLFontGL::VAlign>::declareValues()
	{}

	void TypeValues<LLFontGL::ShadowType>::declareValues()
	{}

	void ParamValue<LLUIImage*, TypeValues<LLUIImage*> >::updateValueFromBlock()
	{}
	
	void ParamValue<LLUIImage*, TypeValues<LLUIImage*> >::updateBlockFromValue(bool)
	{}
	
	bool ParamCompare<LLUIImage*, false>::equals(
		LLUIImage* const &a,
		LLUIImage* const &b)
	{
		return false;
	}

	bool ParamCompare<LLUIColor, false>::equals(const LLUIColor &a, const LLUIColor &b)
	{
		return false;
	}

}

//static
LLFontGL* LLFontGL::getFontDefault()
{
	return NULL; 
}


namespace tut
{
	struct LLUrlMatchData
	{
	};

	typedef test_group<LLUrlMatchData> factory;
	typedef factory::object object;
}

namespace
{
	tut::factory tf("LLUrlMatch");
}

namespace tut
{
	template<> template<>
	void object::test<1>()
	{
		//
		// test the empty() method
		//
		LLUrlMatch match;
		ensure("empty()", match.empty());

		match.setValues(0, 1, "http://secondlife.com", "Second Life", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure("! empty()", ! match.empty());
	}

	template<> template<>
	void object::test<2>()
	{
		//
		// test the getStart() method
		//
		LLUrlMatch match;
		ensure_equals("getStart() == 0", match.getStart(), 0);

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getStart() == 10", match.getStart(), 10);
	}

	template<> template<>
	void object::test<3>()
	{
		//
		// test the getEnd() method
		//
		LLUrlMatch match;
		ensure_equals("getEnd() == 0", match.getEnd(), 0);

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getEnd() == 20", match.getEnd(), 20);
	}

	template<> template<>
	void object::test<4>()
	{
		//
		// test the getUrl() method
		//
		LLUrlMatch match;
		ensure_equals("getUrl() == ''", match.getUrl(), "");

		match.setValues(10, 20, "http://slurl.com/", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getUrl() == 'http://slurl.com/'", match.getUrl(), "http://slurl.com/");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getUrl() == '' (2)", match.getUrl(), "");
	}

	template<> template<>
	void object::test<5>()
	{
		//
		// test the getLabel() method
		//
		LLUrlMatch match;
		ensure_equals("getLabel() == ''", match.getLabel(), "");

		match.setValues(10, 20, "", "Label", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getLabel() == 'Label'", match.getLabel(), "Label");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getLabel() == '' (2)", match.getLabel(), "");
	}

	template<> template<>
	void object::test<6>()
	{
		//
		// test the getTooltip() method
		//
		LLUrlMatch match;
		ensure_equals("getTooltip() == ''", match.getTooltip(), "");

		match.setValues(10, 20, "", "", "Info", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getTooltip() == 'Info'", match.getTooltip(), "Info");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getTooltip() == '' (2)", match.getTooltip(), "");
	}

	template<> template<>
	void object::test<7>()
	{
		//
		// test the getIcon() method
		//
		LLUrlMatch match;
		ensure_equals("getIcon() == ''", match.getIcon(), "");

		match.setValues(10, 20, "", "", "", "Icon", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getIcon() == 'Icon'", match.getIcon(), "Icon");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure_equals("getIcon() == '' (2)", match.getIcon(), "");
	}

	template<> template<>
	void object::test<8>()
	{
		//
		// test the getMenuName() method
		//
		LLUrlMatch match;
		ensure("getMenuName() empty", match.getMenuName().empty());

		match.setValues(10, 20, "", "", "", "Icon", LLStyle::Params(), "xui_file.xml", "", LLUUID::null);
		ensure_equals("getMenuName() == \"xui_file.xml\"", match.getMenuName(), "xui_file.xml");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure("getMenuName() empty (2)", match.getMenuName().empty());
	}

	template<> template<>
	void object::test<9>()
	{
		//
		// test the getLocation() method
		//
		LLUrlMatch match;
		ensure("getLocation() empty", match.getLocation().empty());

		match.setValues(10, 20, "", "", "", "Icon", LLStyle::Params(), "xui_file.xml", "Paris", LLUUID::null);
		ensure_equals("getLocation() == \"Paris\"", match.getLocation(), "Paris");

		match.setValues(10, 20, "", "", "", "", LLStyle::Params(), "", "", LLUUID::null);
		ensure("getLocation() empty (2)", match.getLocation().empty());
	}
}
