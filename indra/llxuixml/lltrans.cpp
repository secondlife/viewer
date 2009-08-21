/**
 * @file lltrans.cpp
 * @brief LLTrans implementation
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "lltrans.h"

#include "llfasttimer.h"	// for call count statistics
#include "llxuiparser.h"

#include <map>

LLTrans::template_map_t LLTrans::sStringTemplates;
LLStringUtil::format_map_t LLTrans::sDefaultArgs;

struct StringDef : public LLInitParam::Block<StringDef>
{
	Mandatory<std::string> name;
	Mandatory<std::string> value;

	StringDef()
	:	name("name"),
		value("value")
	{}
};

struct StringTable : public LLInitParam::Block<StringTable>
{
	Multiple<StringDef> strings;
	StringTable()
	:	strings("string")
	{}
};

//static 
bool LLTrans::parseStrings(LLXMLNodePtr &root, const std::set<std::string>& default_args)
{
	std::string xml_filename = "(strings file)";
	if (!root->hasName("strings"))
	{
		llerrs << "Invalid root node name in " << xml_filename 
			<< ": was " << root->getName() << ", expected \"strings\"" << llendl;
	}

	StringTable string_table;
	LLXUIParser::instance().readXUI(root, string_table);

	if (!string_table.validateBlock())
	{
		llerrs << "Problem reading strings: " << xml_filename << llendl;
		return false;
	}
	
	sStringTemplates.clear();
	sDefaultArgs.clear();
	
	for(LLInitParam::ParamIterator<StringDef>::const_iterator it = string_table.strings().begin();
		it != string_table.strings().end();
		++it)
	{
		LLTransTemplate xml_template(it->name, it->value);
		sStringTemplates[xml_template.mName] = xml_template;
		
		std::set<std::string>::const_iterator iter = default_args.find(xml_template.mName);
		if (iter != default_args.end())
		{
			std::string name = *iter;
			if (name[0] != '[')
				name = llformat("[%s]",name.c_str());
			sDefaultArgs[name] = xml_template.mText;
		}
	}

	return true;
}


//static
bool LLTrans::parseLanguageStrings(LLXMLNodePtr &root)
{
	std::string xml_filename = "(language strings file)";
	if (!root->hasName("strings"))
	{
		llerrs << "Invalid root node name in " << xml_filename 
		<< ": was " << root->getName() << ", expected \"strings\"" << llendl;
	}
	
	StringTable string_table;
	LLXUIParser::instance().readXUI(root, string_table);
	
	if (!string_table.validateBlock())
	{
		llerrs << "Problem reading strings: " << xml_filename << llendl;
		return false;
	}
		
	for(LLInitParam::ParamIterator<StringDef>::const_iterator it = string_table.strings().begin();
		it != string_table.strings().end();
		++it)
	{
		// share the same map with parseStrings() so we can search the strings using the same getString() function.- angela
		LLTransTemplate xml_template(it->name, it->value);
		sStringTemplates[xml_template.mName] = xml_template;
	}
	
	return true;
}



static LLFastTimer::DeclareTimer FTM_GET_TRANS("Translate string");

//static 
std::string LLTrans::getString(const std::string &xml_desc, const LLStringUtil::format_map_t& msg_args)
{
	// Don't care about time as much as call count.  Make sure we're not
	// calling LLTrans::getString() in an inner loop. JC
	LLFastTimer timer(FTM_GET_TRANS);
	
	template_map_t::iterator iter = sStringTemplates.find(xml_desc);
	if (iter != sStringTemplates.end())
	{
		std::string text = iter->second.mText;
		LLStringUtil::format_map_t args = sDefaultArgs;
		args.insert(msg_args.begin(), msg_args.end());
		LLStringUtil::format(text, args);
		
		return text;
	}
	else
	{
		LLSD args;
		args["STRING_NAME"] = xml_desc;
		LL_WARNS_ONCE("configuration") << "Missing String in strings.xml: [" << xml_desc << "]" << LL_ENDL;

		//LLNotifications::instance().add("MissingString", args); // *TODO: resurrect
		//return xml_desc;

		return "MissingString("+xml_desc+")";
	}
}

