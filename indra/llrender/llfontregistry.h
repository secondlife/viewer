/** 
 * @file llfontregistry.h
 * @author Brad Payne
 * @brief Storage for fonts.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLFONTREGISTRY_H
#define LL_LLFONTREGISTRY_H

#include "llxmlnode.h"

class LLFontGL;

typedef std::vector<std::string> string_vec_t;

class LLFontDescriptor
{
public:
	LLFontDescriptor();
	LLFontDescriptor(const std::string& name, const std::string& size, const U8 style);
	LLFontDescriptor(const std::string& name, const std::string& size, const U8 style, const string_vec_t& file_names);
	LLFontDescriptor normalize() const;

	bool operator<(const LLFontDescriptor& b) const;

	bool isTemplate() const;
	
	const std::string& getName() const { return mName; }
	void setName(const std::string& name) { mName = name; }
	const std::string& getSize() const { return mSize; }
	void setSize(const std::string& size) { mSize = size; }
	const std::vector<std::string>& getFileNames() const { return mFileNames; }
	std::vector<std::string>& getFileNames() { return mFileNames; }
	const U8 getStyle() const { return mStyle; }
	void setStyle(U8 style) { mStyle = style; }

private:
	std::string mName;
	std::string mSize;
	string_vec_t mFileNames;
	U8 mStyle;
};

class LLFontRegistry
{
public:
	LLFontRegistry(const string_vec_t& xui_paths);
	~LLFontRegistry();

	// Load standard font info from XML file(s).
	bool parseFontInfo(const std::string& xml_filename); 
	bool initFromXML(LLXMLNodePtr node);

	// Clear cached glyphs for all fonts.
	void reset();

	// Destroy all fonts.
	void clear();

	// GL cleanup
	void destroyGL();
		
	LLFontGL *getFont(const LLFontDescriptor& desc);
	const LLFontDescriptor *getMatchingFontDesc(const LLFontDescriptor& desc);
	const LLFontDescriptor *getClosestFontTemplate(const LLFontDescriptor& desc);

	bool nameToSize(const std::string& size_name, F32& size);

	void dump();
	
	const string_vec_t& getUltimateFallbackList() const { return mUltimateFallbackList; }

private:
	LLFontGL *createFont(const LLFontDescriptor& desc);
	typedef std::map<LLFontDescriptor,LLFontGL*> font_reg_map_t;
	typedef std::map<std::string,F32> font_size_map_t;

	// Given a descriptor, look up specific font instantiation.
	font_reg_map_t mFontMap;
	// Given a size name, look up the point size.
	font_size_map_t mFontSizes;

	string_vec_t mUltimateFallbackList;
	string_vec_t mXUIPaths;
};

#endif // LL_LLFONTREGISTRY_H
