/** 
 * @file lluicolortable.cpp
 * @brief brief LLUIColorTable class implementation file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include <queue>

#include "lluicolortable.h"

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
:	color_entries("color_entries")
{
}

void LLUIColorTable::init(const Params& p)
{
	// this map will contain all color references after the following loop
	typedef std::map<std::string, std::string> string_string_map_t;
	string_string_map_t unresolved_refs;

	mColors.clear();
	for(LLInitParam::ParamIterator<ColorEntryParams>::const_iterator it = p.color_entries().begin();
		it != p.color_entries().end();
		++it)
	{
		ColorEntryParams color_entry = *it;
		if(color_entry.color.value.isChosen())
		{
			mColors.insert(string_color_map_t::value_type(color_entry.name, color_entry.color.value));
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

		string_string_map_t::iterator it = unresolved_refs.begin();
		while(true)
		{
			if(it != unresolved_refs.end())
			{
				// locate the current reference in the previously visited references...
				string_color_ref_iter_map_t::iterator visited = visited_refs.lower_bound(it->first);
				if(visited != visited_refs.end()
			    && !(visited_refs.key_comp()(it->first, visited->first)))
				{
					// ...if we find the current reference in the previously visited references
					// we know that there is a cycle
					std::string ending_ref = it->first;
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
					ref_chain.push(it->first);
					visited_refs.insert(visited, string_color_ref_iter_map_t::value_type(it->first, it));
				}
			}
			else
			{
				// since this reference does not refer to another reference it must refer to an
				// actual color, lets find it...
				string_color_map_t::iterator color_value = mColors.find(it->second);

				if(color_value != mColors.end())
				{
					// ...we found the color, and we now add every reference in the reference chain
					// to the color map
					for(string_color_ref_iter_map_t::iterator iter = visited_refs.begin();
						iter != visited_refs.end();
						++iter)
					{
						mColors.insert(string_color_map_t::value_type(iter->first, color_value->second));
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
			it = unresolved_refs.find(it->second);
		}
	}
}

const LLColor4& LLUIColorTable::getColor(const std::string& name) const
{
	string_color_map_t::const_iterator iter = mColors.find(name);
	return (iter != mColors.end() ? iter->second : LLColor4::magenta);
}
