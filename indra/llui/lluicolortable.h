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

class LLUIColor;

class LLUIColorTable : public LLSingleton<LLUIColorTable>
{
LOG_CLASS(LLUIColorTable);
public:
	struct ColorParams : LLInitParam::Choice<ColorParams>
	{
		Alternative<LLColor4>    value;
		Alternative<std::string> reference;

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
	void insertFromParams(const Params& p);

	// reset all colors to default magenta color
	void clear();

	// color lookup
	LLUIColor getColor(const std::string& name, const LLColor4& default_color = LLColor4::magenta) const;

	// if the color is in the table, it's value is changed, otherwise it is added
	void setColor(const std::string& name, const LLColor4& color);

	// returns true if color_name exists in the table
	bool colorExists(const std::string& color_name) const;

	// loads colors from settings files
	bool loadFromSettings();

	// saves colors specified by the user to the users skin directory
	void saveUserSettings() const;

private:
	bool loadFromFilename(const std::string& filename);

	// consider using sorted vector, can be much faster
	typedef std::map<std::string, LLColor4>  string_color_map_t;
	
	void clearTable(string_color_map_t& table);
	void setColor(const std::string& name, const LLColor4& color, string_color_map_t& table);

	string_color_map_t mLoadedColors;
	string_color_map_t mUserSetColors;
};

#endif // LL_LLUICOLORTABLE_H
