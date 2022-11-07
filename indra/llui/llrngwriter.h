/** 
 * @file llrngwriter.h
 * @brief Generates Relax NG schema files from a param block
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLRNGWRITER_H
#define LLRNGWRITER_H

#include "llinitparam.h"
#include "llxmlnode.h"

class LLRNGWriter : public LLInitParam::Parser
{
    LOG_CLASS(LLRNGWriter);
public:
    void writeRNG(const std::string& name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace);
    void addDefinition(const std::string& type_name, const LLInitParam::BaseBlock& block);

    /*virtual*/ std::string getCurrentElementName() { return LLStringUtil::null; }

    LLRNGWriter();

private:
    LLXMLNodePtr createCardinalityNode(LLXMLNodePtr parent_node, S32 min_count, S32 max_count);
    void addTypeNode(LLXMLNodePtr parent_node, const std::string& type, const std::vector<std::string>* possible_values);

    void writeAttribute(const std::string& type, const Parser::name_stack_t&, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values);
    LLXMLNodePtr    mElementNode;
    LLXMLNodePtr    mChildrenNode;
    LLXMLNodePtr    mGrammarNode;
    std::string     mDefinitionName;

    typedef std::pair<LLXMLNodePtr, std::set<std::string> >  attribute_data_t;
    typedef std::map<std::string, attribute_data_t> elements_map_t;
    typedef std::set<std::string> defined_elements_t;

    defined_elements_t  mDefinedElements;
    attribute_data_t    mAttributesWritten;
    elements_map_t      mElementsWritten;
};

#endif //LLRNGWRITER_H
