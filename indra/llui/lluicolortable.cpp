/** 
 * @file lluicolortable.cpp
 * @brief brief LLUIColorTable class implementation file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "linden_common.h"

#include <queue>

#include "lldir.h"
#include "llui.h"
#include "lluicolortable.h"
#include "lluictrlfactory.h"

LLUIColorTable::ColorParams::ColorParams()
:	value("value"),
	reference("reference")
{
}

LLUIColorTable::ColorEntryParams::ColorEntryParams()
:	name("name"),
	color("")
{
}

LLUIColorTable::Params::Params()
:	color_entries("color")
{
}

void LLUIColorTable::insertFromParams(const Params& p, string_color_map_t& table)
{
	// this map will contain all color references after the following loop
	typedef std::map<std::string, std::string> string_string_map_t;
	string_string_map_t unresolved_refs;

	for(LLInitParam::ParamIterator<ColorEntryParams>::const_iterator it = p.color_entries().begin();
		it != p.color_entries().end();
		++it)
	{
		ColorEntryParams color_entry = *it;
		if(color_entry.color.value.isChosen())
		{
			setColor(color_entry.name, color_entry.color.value, table);
		}
		else
		{
			unresolved_refs.insert(string_string_map_t::value_type(color_entry.name, color_entry.color.reference));
		}
	}

	// maintain an in order queue of visited references for better debugging of cycles
	typedef std::queue<std::string> string_queue_t;
	string_queue_t ref_chain;

	// maintain a map of the previously visited references in the reference chain for detecting cycles
	typedef std::map<std::string, string_string_map_t::iterator> string_color_ref_iter_map_t;
	string_color_ref_iter_map_t visited_refs;

	// loop through the unresolved color references until there are none left
	while(!unresolved_refs.empty())
	{
		// we haven't visited any references yet
		visited_refs.clear();

		string_string_map_t::iterator current = unresolved_refs.begin();
		string_string_map_t::iterator previous;

		while(true)
		{
			if(current != unresolved_refs.end())
			{
				// locate the current reference in the previously visited references...
				string_color_ref_iter_map_t::iterator visited = visited_refs.lower_bound(current->first);
				if(visited != visited_refs.end()
			    && !(visited_refs.key_comp()(current->first, visited->first)))
				{
					// ...if we find the current reference in the previously visited references
					// we know that there is a cycle
					std::string ending_ref = current->first;
					std::string warning("The following colors form a cycle: ");

					// warn about the references in the chain and remove them from
					// the unresolved references map because they cannot be resolved
					for(string_color_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						if(!ref_chain.empty())
						{
							warning += ref_chain.front() + "->";
							ref_chain.pop();
						}
						unresolved_refs.erase(iter->second);
					}

					llwarns << warning + ending_ref << llendl;

					break;
				}
				else
				{
					// ...continue along the reference chain
					ref_chain.push(current->first);
					visited_refs.insert(visited, string_color_ref_iter_map_t::value_type(current->first, current));
				}
			}
			else
			{
				// since this reference does not refer to another reference it must refer to an
				// actual color, lets find it...
				string_color_map_t::iterator color_value = mLoadedColors.find(previous->second);

				if(color_value != mLoadedColors.end())
				{
					// ...we found the color, and we now add every reference in the reference chain
					// to the color map
					for(string_color_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						setColor(iter->first, color_value->second, mLoadedColors);
						unresolved_refs.erase(iter->second);
					}

					break;
				}
				else
				{
					// ... we did not find the color which imples that the current reference
					// references a non-existant color
					for(string_color_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						llwarns << iter->first << " references a non-existent color" << llendl;
						unresolved_refs.erase(iter->second);
					}

					break;
				}
			}

			// find the next color reference in the reference chain
			previous = current;
			current = unresolved_refs.find(current->second);
		}
	}
}

void LLUIColorTable::clear()
{
	clearTable(mLoadedColors);
	clearTable(mUserSetColors);
}

LLUIColor LLUIColorTable::getColor(const std::string& name, const LLColor4& default_color) const
{
	string_color_map_t::const_iterator iter = mUserSetColors.find(name);
	
	if(iter != mUserSetColors.end())
	{
		return LLUIColor(&iter->second);
	}

	iter = mLoadedColors.find(name);
	
	if(iter != mLoadedColors.end())
	{
		return LLUIColor(&iter->second);
	}
	
	return  LLUIColor(default_color);
}

// update user color, loaded colors are parsed on initialization
void LLUIColorTable::setColor(const std::string& name, const LLColor4& color)
{
	setColor(name, color, mUserSetColors);
	setColor(name, color, mLoadedColors);
}

bool LLUIColorTable::loadFromSettings()
{
	bool result = false;

	std::string default_filename = gDirUtilp->getExpandedFilename(LL_PATH_DEFAULT_SKIN, "colors.xml");
	result |= loadFromFilename(default_filename, mLoadedColors);

	std::string current_filename = gDirUtilp->getExpandedFilename(LL_PATH_TOP_SKIN, "colors.xml");
	if(current_filename != default_filename)
	{
		result |= loadFromFilename(current_filename, mLoadedColors);
	}

	std::string user_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "colors.xml");
	loadFromFilename(user_filename, mUserSetColors);

	return result;
}

void LLUIColorTable::saveUserSettings() const
{
	Params params;

	for(string_color_map_t::const_iterator it = mUserSetColors.begin();
		it != mUserSetColors.end();
		++it)
	{
		ColorEntryParams color_entry;
		color_entry.name = it->first;
		color_entry.color.value = it->second;

		params.color_entries.add(color_entry);
	}

	LLXMLNodePtr output_node = new LLXMLNode("colors", false);
	LLXUIParser::instance().writeXUI(output_node, params);

	if(!output_node->isNull())
	{
		const std::string& filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "colors.xml");
		LLFILE *fp = LLFile::fopen(filename, "w");

		if(fp != NULL)
		{
			LLXMLNode::writeHeaderToFile(fp);
			output_node->writeToFile(fp);

			fclose(fp);
		}
	}
}

bool LLUIColorTable::colorExists(const std::string& color_name) const
{
	return ((mLoadedColors.find(color_name) != mLoadedColors.end())
		 || (mUserSetColors.find(color_name) != mUserSetColors.end()));
}

void LLUIColorTable::clearTable(string_color_map_t& table)
{
	for(string_color_map_t::iterator it = table.begin();
		it != table.end();
		++it)
	{
		it->second = LLColor4::magenta;
	}
}

// this method inserts a color into the table if it does not exist
// if the color already exists it changes the color
void LLUIColorTable::setColor(const std::string& name, const LLColor4& color, string_color_map_t& table)
{
	string_color_map_t::iterator it = table.lower_bound(name);
	if(it != table.end()
	&& !(table.key_comp()(name, it->first)))
	{
		it->second = color;
	}
	else
	{
		table.insert(it, string_color_map_t::value_type(name, color));
	}
}

bool LLUIColorTable::loadFromFilename(const std::string& filename, string_color_map_t& table)
{
	LLXMLNodePtr root;

	if(!LLXMLNode::parseFile(filename, root, NULL))
	{
		llwarns << "Unable to parse color file " << filename << llendl;
		return false;
	}

	if(!root->hasName("colors"))
	{
		llwarns << filename << " is not a valid color definition file" << llendl;
		return false;
	}

	Params params;
	LLXUIParser::instance().readXUI(root, params, filename);

	if(params.validateBlock())
	{
		insertFromParams(params, table);
	}
	else
	{
		llwarns << filename << " failed to load" << llendl;
		return false;
	}

	return true;
}

void LLUIColorTable::insertFromParams(const Params& p)
{
	insertFromParams(p, mUserSetColors);
}

// EOF

