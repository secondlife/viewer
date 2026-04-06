/**
 * @file llrngwriter.cpp
 * @brief Generates Relax NG schema from param blocks
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

#include "linden_common.h"

#include "llrngwriter.h"
#include "lluicolor.h"

#include "lluictrlfactory.h"

#include <functional>
static  LLInitParam::Parser::parser_read_func_map_t sReadFuncs;
static  LLInitParam::Parser::parser_write_func_map_t sWriteFuncs;
static  LLInitParam::Parser::parser_inspect_func_map_t sInspectFuncs;

//
// LLRNGWriter - writes Relax NG schema files based on a param block
//
LLRNGWriter::LLRNGWriter()
: Parser(sReadFuncs, sWriteFuncs, sInspectFuncs)
{
    // register various callbacks for inspecting the contents of a param block
    registerInspectFunc<bool>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("boolean", stack, min_count, max_count, values); });
    registerInspectFunc<std::string>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("string", stack, min_count, max_count, values); });
    registerInspectFunc<U8>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("unsignedByte", stack, min_count, max_count, values); });
    registerInspectFunc<S8>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("signedByte", stack, min_count, max_count, values); });
    registerInspectFunc<U16>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("unsignedShort", stack, min_count, max_count, values); });
    registerInspectFunc<S16>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("signedShort", stack, min_count, max_count, values); });
    registerInspectFunc<U32>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("unsignedInt", stack, min_count, max_count, values); });
    registerInspectFunc<S32>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("integer", stack, min_count, max_count, values); });
    registerInspectFunc<F32>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("float", stack, min_count, max_count, values); });
    registerInspectFunc<F64>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("double", stack, min_count, max_count, values); });
    registerInspectFunc<LLColor4>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("string", stack, min_count, max_count, values); });
    registerInspectFunc<LLUIColor>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("string", stack, min_count, max_count, values); });
    registerInspectFunc<LLUUID>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("string", stack, min_count, max_count, values); });
    registerInspectFunc<LLSD>([this](name_stack_t& stack, S32 min_count, S32 max_count, const possible_values_t* values) { writeAttribute("string", stack, min_count, max_count, values); });
}

void LLRNGWriter::writeRNG(const std::string& type_name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace)
{
    mGrammarNode = node;
    mGrammarNode->setName("grammar");
    mGrammarNode->createChild("xmlns", true)->setStringValue("http://relaxng.org/ns/structure/1.0");
    mGrammarNode->createChild("datatypeLibrary", true)->setStringValue("http://www.w3.org/2001/XMLSchema-datatypes");
    mGrammarNode->createChild("ns", true)->setStringValue(xml_namespace);

    node = mGrammarNode->createChild("start", false);
    node = node->createChild("ref", false);
    node->createChild("name", true)->setStringValue(type_name);

    addDefinition(type_name, block);
}

void LLRNGWriter::addDefinition(const std::string& type_name, const LLInitParam::BaseBlock& block)
{
    if (mDefinedElements.find(type_name) != mDefinedElements.end()) return;
    mDefinedElements.insert(type_name);

    LLXMLNodePtr node = mGrammarNode->createChild("define", false);
    node->createChild("name", true)->setStringValue(type_name);

    mElementNode = node->createChild("element", false);
    mElementNode->createChild("name", true)->setStringValue(type_name);
    mChildrenNode = mElementNode->createChild("zeroOrMore", false)->createChild("choice", false);

    mAttributesWritten.first = mElementNode;
    mAttributesWritten.second.clear();
    mElementsWritten.clear();

    block.inspectBlock(*this);

    // add includes for all possible children
    const std::type_index& type = *LLWidgetTypeRegistry::instance().getValue(type_name);
    const widget_registry_t* widget_registryp = LLChildRegistryRegistry::instance().getValue(type);

    // add include declarations for all valid children
    for (widget_registry_t::Registrar::registry_map_t::const_iterator it = widget_registryp->currentRegistrar().beginItems();
        it != widget_registryp->currentRegistrar().endItems();
        ++it)
    {
        std::string child_name = it->first;
        if (child_name == type_name)
        {
            continue;
        }

        LLXMLNodePtr old_element_node = mElementNode;
        LLXMLNodePtr old_child_node = mChildrenNode;
        //FIXME: add LLDefaultParamBlockRegistry back when working on schema generation
        //addDefinition(child_name, (*LLDefaultParamBlockRegistry::instance().getValue(type))());
        mElementNode = old_element_node;
        mChildrenNode = old_child_node;

        mChildrenNode->createChild("ref", false)->createChild("name", true)->setStringValue(child_name);
    }

    if (mChildrenNode->mChildren.isNull())
    {
        // remove unused children node
        mChildrenNode->mParent->mParent->deleteChild(mChildrenNode->mParent);
    }
}

void LLRNGWriter::writeAttribute(const std::string& type, const Parser::name_stack_t& stack, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values)
{
    if (max_count == 0) return;

    name_stack_t non_empty_names;
    std::string attribute_name;
    for (const auto & it : stack)
    {
        const std::string& name = it.first;
        if (!name.empty())
        {
            non_empty_names.push_back(it);
        }
    }

    if (non_empty_names.empty()) return;

    for (const auto & non_empty_name : non_empty_names)
    {
        if (!attribute_name.empty())
        {
            attribute_name += ".";
        }
        attribute_name += non_empty_name.first;
    }

    // singular attribute, e.g. <foo bar="1"/>
    if (non_empty_names.size() == 1 && max_count == 1)
    {
        if (mAttributesWritten.second.find(attribute_name) == mAttributesWritten.second.end())
        {
            LLXMLNodePtr node = createCardinalityNode(mElementNode, min_count, max_count)->createChild("attribute", false);
            node->createChild("name", true)->setStringValue(attribute_name);
            node->createChild("data", false)->createChild("type", true)->setStringValue(type);

            mAttributesWritten.second.insert(attribute_name);
        }
    }
    // compound attribute
    else
    {
        std::string element_name;

        // traverse all but last element, leaving that as an attribute name
        name_stack_t::const_iterator end_it = non_empty_names.end();
        end_it--;

        for (name_stack_t::const_iterator it = non_empty_names.begin();
            it != end_it;
            ++it)
        {
            if (it != non_empty_names.begin())
            {
                element_name += ".";
            }
            element_name += it->first;
        }

        elements_map_t::iterator found_it = mElementsWritten.find(element_name);
        // <choice>
        //   <group>
        //     <optional>
        //       <attribute name="foo.bar"><data type="string"/></attribute>
        //     </optional>
        //     <optional>
        //       <attribute name="foo.baz"><data type="integer"/></attribute>
        //     </optional>
        //   </group>
        //   <element name="foo">
        //     <optional>
        //       <attribute name="bar"><data type="string"/></attribute>
        //     </optional>
        //     <optional>
        //       <attribute name="baz"><data type="string"/></attribute>
        //     </optional>
        //   </element>
        //   <element name="outer.foo">
        //     <ref name="foo"/>
        //   </element>
        // </choice>

        if (found_it != mElementsWritten.end())
        {
            // reuse existing element
            LLXMLNodePtr choice_node = found_it->second.first;

            // attribute with this name not already written?
            if (found_it->second.second.find(attribute_name) == found_it->second.second.end())
            {
                // append to <group>
                LLXMLNodePtr node = choice_node->mChildren->head;
                node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
                node->createChild("name", true)->setStringValue(attribute_name);
                addTypeNode(node, type, possible_values);

                // append to <element>
                node = choice_node->mChildren->head->mNext->mChildren->head;
                node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
                node->createChild("name", true)->setStringValue(non_empty_names.back().first);
                addTypeNode(node, type, possible_values);

                // append to <element>
                //node = choice_node->mChildren->head->mNext->mNext->mChildren->head;
                //node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
                //node->createChild("name", true)->setStringValue(non_empty_names.back().first);
                //addTypeNode(node, type, possible_values);

                found_it->second.second.insert(attribute_name);
            }
        }
        else
        {
            LLXMLNodePtr choice_node = mElementNode->createChild("choice", false);

            LLXMLNodePtr node = choice_node->createChild("group", false);
            node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
            node->createChild("name", true)->setStringValue(attribute_name);
            addTypeNode(node, type, possible_values);

            node = choice_node->createChild("optional", false);
            node = node->createChild("element", false);
            node->createChild("name", true)->setStringValue(element_name);
            node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
            node->createChild("name", true)->setStringValue(non_empty_names.back().first);
            addTypeNode(node, type, possible_values);

            //node = choice_node->createChild("optional", false);
            //node = node->createChild("element", false);
            //node->createChild("name", true)->setStringValue(mDefinitionName + "." + element_name);
            //node = createCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
            //node->createChild("name", true)->setStringValue(non_empty_names.back().first);
            //addTypeNode(node, type, possible_values);

            attribute_data_t& attribute_data = mElementsWritten[element_name];
            attribute_data.first = choice_node;
            attribute_data.second.insert(attribute_name);
        }
    }
}

void LLRNGWriter::addTypeNode(LLXMLNodePtr parent_node, const std::string& type, const std::vector<std::string>* possible_values)
{
    if (possible_values)
    {
        LLXMLNodePtr enum_node = parent_node->createChild("choice", false);
        for (const auto & possible_value : *possible_values)
        {
            enum_node->createChild("value", false)->setStringValue(possible_value);
        }
    }
    else
    {
        parent_node->createChild("data", false)->createChild("type", true)->setStringValue(type);
    }
}

LLXMLNodePtr LLRNGWriter::createCardinalityNode(LLXMLNodePtr parent_node, S32 min_count, S32 max_count)
{
    // unlinked by default, meaning this attribute is forbidden
    LLXMLNodePtr count_node = new LLXMLNode();
    if (min_count == 0)
    {
        if (max_count == 1)
        {
            count_node = parent_node->createChild("optional", false);
        }
        else if (max_count > 1)
        {
            count_node = parent_node->createChild("zeroOrMore", false);
        }
    }
    else if (min_count >= 1)
    {
        if (max_count == 1 && min_count == 1)
        {
            // just add raw element, will count as 1 and only 1
            count_node = parent_node;
        }
        else
        {
            count_node = parent_node->createChild("oneOrMore", false);
        }
    }
    return count_node;
}
