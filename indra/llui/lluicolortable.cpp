/**
 * @file lluicolortable.cpp
 * @brief brief LLUIColorTable class implementation file
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

#include <queue>

#include "lldir.h"
#include "llui.h"
#include "lluicolortable.h"
#include "lluictrlfactory.h"

LLUIColorTable::ColorParams::ColorParams()
:   value("value"),
    reference("reference")
{
}

LLUIColorTable::ColorEntryParams::ColorEntryParams()
:   name("name"),
    color("")
{
}

LLUIColorTable::Params::Params()
:   color_entries("color")
{
}

void LLUIColorTable::insertFromParams(const Params& p, string_color_map_t& table)
{
    // this map will contain all color references after the following loop
    using string_string_map_t = std::map<std::string, std::string>;
    string_string_map_t unresolved_refs;

    for(const auto & color_entrie : p.color_entries)
    {
        ColorEntryParams color_entry = color_entrie;
        if(color_entry.color.value.isChosen())
        {
            setColor(color_entry.name(), color_entry.color.value, table);
        }
        else
        {
            unresolved_refs.insert(string_string_map_t::value_type(color_entry.name, color_entry.color.reference));
        }
    }

    // maintain an in order queue of visited references for better debugging of cycles
    using string_queue_t = std::queue<std::string>;
    string_queue_t ref_chain;

    // maintain a map of the previously visited references in the reference chain for detecting cycles
    using string_color_ref_iter_map_t = std::map<std::string, string_string_map_t::iterator>;
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
                    for(auto & visited_ref : visited_refs)
                    {
                        if(!ref_chain.empty())
                        {
                            warning += ref_chain.front() + "->";
                            ref_chain.pop();
                        }
                        unresolved_refs.erase(visited_ref.second);
                    }

                    LL_WARNS() << warning + ending_ref << LL_ENDL;

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
                    for(auto & visited_ref : visited_refs)
                    {
                        setColor(visited_ref.first, color_value->second, mLoadedColors);
                        unresolved_refs.erase(visited_ref.second);
                    }

                    break;
                }
                else
                {
                    // ... we did not find the color which imples that the current reference
                    // references a non-existant color
                    for(auto & visited_ref : visited_refs)
                    {
                        LL_WARNS() << visited_ref.first << " references a non-existent color" << LL_ENDL;
                        unresolved_refs.erase(visited_ref.second);
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

LLUIColor LLUIColorTable::getColor(std::string_view name, const LLColor4& default_color) const
{
    string_color_map_t::const_iterator iter = mUserSetColors.find(name);

    if(iter != mUserSetColors.end())
    {
        return {&iter->second};
    }

    iter = mLoadedColors.find(name);

    if(iter != mLoadedColors.end())
    {
        return {&iter->second};
    }

    return  LLUIColor(default_color);
}

// update user color, loaded colors are parsed on initialization
void LLUIColorTable::setColor(std::string_view name, const LLColor4& color)
{
    auto it = mUserSetColors.find(name);
    if(it != mUserSetColors.end())
    {
        it->second = color;
    }
    else
    {
        string_color_map_t::iterator base_iter = mLoadedColors.find(name);
        if (base_iter != mLoadedColors.end())
        {
            LLColor4 original_color = base_iter->second.get();
            auto color_handle = mLoadedColors.extract(base_iter);
            auto new_color_pair = mUserSetColors.insert(std::move(color_handle));
            new_color_pair.position->second = color;
            mLoadedColors.emplace(name, LLUIColor(original_color));
        }
        else
        {
            mUserSetColors.insert(it, std::make_pair(name, color));
        }
    }
}

bool LLUIColorTable::isDefault(std::string_view name) const
{
    string_color_map_t::const_iterator base_iter = mLoadedColors.find(name);
    string_color_map_t::const_iterator user_iter = mUserSetColors.find(name);
    if (base_iter != mLoadedColors.end())
    {
        if(user_iter != mUserSetColors.end())
            return user_iter->second == base_iter->second;

        return true;
    }
    else if (user_iter != mUserSetColors.end()) // user only color ???
    {
        return true;
    }

    return false;
}

void LLUIColorTable::resetToDefault(std::string_view name)
{
    string_color_map_t::iterator iter = mUserSetColors.find(name);

    if (iter != mUserSetColors.end())
    {
        auto default_iter = mLoadedColors.find(name);

        if (default_iter != mLoadedColors.end())
        {
            iter->second = default_iter->second.get();
        }
    }
}

bool LLUIColorTable::loadFromSettings()
{
    bool result = false;

    // pass constraint=LLDir::ESkinConstraint::ALL_SKINS because we want colors.xml from every
    // skin dir
    for (const std::string& colors_path :
                  gDirUtilp->findSkinnedFilenames(LLDir::SKINBASE, "colors.xml", LLDir::ESkinConstraint::ALL_SKINS))
    {
        result |= loadFromFilename(colors_path, mLoadedColors);
    }

    std::string user_filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "colors.xml");
    loadFromFilename(user_filename, mUserSetColors);

    return result;
}

void LLUIColorTable::saveUserSettings() const
{
    Params params;

    for (const auto& [color_name, color_value] : mUserSetColors)
    {
        // Compare user color value with the default value, skip if equal
        string_color_map_t::const_iterator itd = mLoadedColors.find(color_name);
        if(itd != mLoadedColors.end() && itd->second == color_value)
            continue;

        ColorEntryParams color_entry;
        color_entry.name = color_name;
        color_entry.color.value = color_value;

        params.color_entries.add(color_entry);
    }

    LLXMLNodePtr output_node = new LLXMLNode("colors", false);
    LLXUIParser parser;
    parser.writeXUI(output_node, params);

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

bool LLUIColorTable::colorExists(std::string_view color_name) const
{
    return ((mLoadedColors.find(color_name) != mLoadedColors.end())
         || (mUserSetColors.find(color_name) != mUserSetColors.end()));
}

void LLUIColorTable::clearTable(string_color_map_t& table)
{
    for(auto & it : table)
    {
        it.second = LLColor4::magenta;
    }
}

// this method inserts a color into the table if it does not exist
// if the color already exists it changes the color
void LLUIColorTable::setColor(std::string_view name, const LLColor4& color, string_color_map_t& table)
{
    string_color_map_t::iterator it = table.find(name);
    if(it != table.end())
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
        LL_WARNS() << "Unable to parse color file " << filename << LL_ENDL;
        return false;
    }

    if(!root->hasName("colors"))
    {
        LL_WARNS() << filename << " is not a valid color definition file" << LL_ENDL;
        return false;
    }

    Params params;
    LLXUIParser parser;
    parser.readXUI(root, params, filename);

    if(params.validateBlock())
    {
        insertFromParams(params, table);
    }
    else
    {
        LL_WARNS() << filename << " failed to load" << LL_ENDL;
        return false;
    }

    return true;
}

void LLUIColorTable::insertFromParams(const Params& p)
{
    insertFromParams(p, mUserSetColors);
}

// EOF

