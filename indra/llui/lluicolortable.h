/** 
 * @file lluicolortable.h
 * @brief brief LLUIColorTable class header file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLUICOLORTABLE_H_
#define LL_LLUICOLORTABLE_H_

#include <map>

#include "llinitparam.h"
#include "llsingleton.h"

#include "v4color.h"

class LLUIColorTable : public LLSingleton<LLUIColorTable>
{
public:
	struct ColorParams : LLInitParam::Choice<ColorParams>
	{
		Option<LLColor4>    value;
		Option<std::string> reference;

		ColorParams();
	};

	struct ColorEntryParams : LLInitParam::Block<ColorEntryParams>
	{
		Mandatory<std::string> name;
		Mandatory<ColorParams> color;

		ColorEntryParams();
	};

	struct Params : LLInitParam::Block<Params>
	{
		Multiple<ColorEntryParams> color_entries;

		Params();
	};

	// define colors by passing in a param block that can be generated via XUI file or manually
	void init(const Params& p);

	// color lookup
	const LLColor4& getColor(const std::string& name) const;

private:
	// consider using sorted vector
	typedef std::map<std::string, LLColor4>  string_color_map_t;
	string_color_map_t mColors;
};

#endif // LL_LLUICOLORTABLE_H
