/** 
 * @file llfontregistry.cpp
 * @author Brad Payne
 * @brief Storage for fonts.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
#include "llgl.h"
#include "llfontfreetype.h"
#include "llfontgl.h"
#include "llfontregistry.h"
#include <boost/tokenizer.hpp>
#include "llcontrol.h"
#include "lldir.h"
#include "llwindow.h"

extern LLControlGroup gSavedSettings;

using std::string;
using std::map;

bool fontDescInitFromXML(LLXMLNodePtr node, LLFontDescriptor& desc);

LLFontDescriptor::LLFontDescriptor():
	mStyle(0)
{
}

LLFontDescriptor::LLFontDescriptor(const std::string& name,
								   const std::string& size, 
								   const U8 style,
								   const string_vec_t& file_names):
	mName(name),
	mSize(size),
	mStyle(style),
	mFileNames(file_names)
{
}

LLFontDescriptor::LLFontDescriptor(const std::string& name,
								   const std::string& size, 
								   const U8 style):
	mName(name),
	mSize(size),
	mStyle(style)
{
}


bool LLFontDescriptor::operator<(const LLFontDescriptor& b) const
{
	if (mName < b.mName)
		return true;
	else if (mName > b.mName)
		return false;
	
	if (mStyle < b.mStyle)
		return true;
	else if (mStyle > b.mStyle)
		return false;

	if (mSize < b.mSize)
		return true;
	else 
		return false;
}

static const std::string s_template_string("TEMPLATE");

bool LLFontDescriptor::isTemplate() const
{
	return getSize() == s_template_string;
}

// Look for substring match and remove substring if matched.
bool removeSubString(std::string& str, const std::string& substr)
{
	size_t pos = str.find(substr);
	if (pos != string::npos)
	{
		str.erase(pos, substr.size());
		return true;
	}
	return false;
}

// Check for substring match without modifying the source string.
bool findSubString(std::string& str, const std::string& substr)
{
	size_t pos = str.find(substr);
	if (pos != string::npos)
	{
		return true;
	}
	return false;
}


// Normal form is
// - raw name
// - bold, italic style info reflected in both style and font name.
// - other style info removed.
// - size info moved to mSize, defaults to Medium
// For example,
// - "SansSerifHuge" would normalize to { "SansSerif", "Huge", 0 }
// - "SansSerifBold" would normalize to { "SansSerifBold", "Medium", BOLD }
LLFontDescriptor LLFontDescriptor::normalize() const
{
	std::string new_name(mName);
	std::string new_size(mSize);
	U8 new_style(mStyle);

	// Only care about style to extent it can be picked up by font.
	new_style &= (LLFontGL::BOLD | LLFontGL::ITALIC);

	// All these transformations are to support old-style font specifications.
	if (removeSubString(new_name,"Small"))
		new_size = "Small";
	if (removeSubString(new_name,"Big"))
		new_size = "Large";
	if (removeSubString(new_name,"Medium"))
		new_size = "Medium";
	if (removeSubString(new_name,"Large"))
		new_size = "Large";
	if (removeSubString(new_name,"Huge"))
		new_size = "Huge";

	// HACK - Monospace is the only one we don't remove, so
	// name "Monospace" doesn't get taken down to ""
	// For other fonts, there's no ambiguity between font name and size specifier.
	if (new_size != s_template_string && new_size.empty() && findSubString(new_name,"Monospace"))
		new_size = "Monospace";
	if (new_size.empty())
		new_size = "Medium";

	if (removeSubString(new_name,"Bold"))
		new_style |= LLFontGL::BOLD;

	if (removeSubString(new_name,"Italic"))
		new_style |= LLFontGL::ITALIC;

	return LLFontDescriptor(new_name,new_size,new_style,getFileNames());
}

LLFontRegistry::LLFontRegistry(bool create_gl_textures)
:	mCreateGLTextures(create_gl_textures)
{
	// This is potentially a slow directory traversal, so we want to
	// cache the result.
	mUltimateFallbackList = LLWindow::getDynamicFallbackFontList();
}

LLFontRegistry::~LLFontRegistry()
{
	clear();
}

bool LLFontRegistry::parseFontInfo(const std::string& xml_filename)
{
	bool success = false;  // Succeed if we find and read at least one XUI file
	const string_vec_t xml_paths = gDirUtilp->findSkinnedFilenames(LLDir::XUI, xml_filename);
	if (xml_paths.empty())
	{
		// We didn't even find one single XUI file
		return false;
	}

	for (string_vec_t::const_iterator path_it = xml_paths.begin();
		 path_it != xml_paths.end();
		 ++path_it)
	{
		LLXMLNodePtr root;
		bool parsed_file = LLXMLNode::parseFile(*path_it, root, NULL);

		if (!parsed_file)
			continue;

		if ( root.isNull() || ! root->hasName( "fonts" ) )
		{
			llwarns << "Bad font info file: " << *path_it << llendl;
			continue;
		}

		std::string root_name;
		root->getAttributeString("name",root_name);
		if (root->hasName("fonts"))
		{
			// Expect a collection of children consisting of "font" or "font_size" entries
			bool init_succ = initFromXML(root);
			success = success || init_succ;
		}
	}
	//if (success)
	//	dump();

	return success;
}

std::string currentOsName()
{
#if LL_WINDOWS
	return "Windows";
#elif LL_DARWIN
	return "Mac";
#elif LL_SDL || LL_MESA_HEADLESS
	return "Linux";
#else
	return "";
#endif
}

bool fontDescInitFromXML(LLXMLNodePtr node, LLFontDescriptor& desc)
{
	if (node->hasName("font"))
	{
		std::string attr_name;
		if (node->getAttributeString("name",attr_name))
		{
			desc.setName(attr_name);
		}

		std::string attr_style;
		if (node->getAttributeString("font_style",attr_style))
		{
			desc.setStyle(LLFontGL::getStyleFromString(attr_style));
		}

		desc.setSize(s_template_string);
	}

	LLXMLNodePtr child;	
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		std::string child_name;
		child->getAttributeString("name",child_name);
		if (child->hasName("file"))
		{
			std::string font_file_name = child->getTextContents();
			desc.getFileNames().push_back(font_file_name);
		}
		else if (child->hasName("os"))
		{
			if (child_name == currentOsName())
			{
				fontDescInitFromXML(child, desc);
			}
		}
	}
	return true;
}

bool LLFontRegistry::initFromXML(LLXMLNodePtr node)
{
	LLXMLNodePtr child;
	
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		std::string child_name;
		child->getAttributeString("name",child_name);
		if (child->hasName("font"))
		{
			LLFontDescriptor desc;
			bool font_succ = fontDescInitFromXML(child, desc);
			LLFontDescriptor norm_desc = desc.normalize();
			if (font_succ)
			{
				// if this is the first time we've seen this font name,
				// create a new template map entry for it.
				const LLFontDescriptor *match_desc = getMatchingFontDesc(desc);
				if (match_desc == NULL)
				{
					// Create a new entry (with no corresponding font).
					mFontMap[norm_desc] = NULL;
				}
				// otherwise, find the existing entry and combine data. 
				else
				{
					// Prepend files from desc.
					// A little roundabout because the map key is const,
					// so we have to fetch it, make a new map key, and
					// replace the old entry.
					string_vec_t match_file_names = match_desc->getFileNames();
					match_file_names.insert(match_file_names.begin(),
											desc.getFileNames().begin(),
											desc.getFileNames().end());
					LLFontDescriptor new_desc = *match_desc;
					new_desc.getFileNames() = match_file_names;
					mFontMap.erase(*match_desc);
					mFontMap[new_desc] = NULL;
				}
			}
		}
		else if (child->hasName("font_size"))
		{
			std::string size_name;
			F32 size_value;
			if (child->getAttributeString("name",size_name) &&
				child->getAttributeF32("size",size_value))
			{
				mFontSizes[size_name] = size_value;
			}

		}
	}
	return true;
}

bool LLFontRegistry::nameToSize(const std::string& size_name, F32& size)
{
	font_size_map_t::iterator it = mFontSizes.find(size_name);
	if (it != mFontSizes.end())
	{
		size = it->second;
		return true;
	}
	return false;
}


LLFontGL *LLFontRegistry::createFont(const LLFontDescriptor& desc)
{
	// Name should hold a font name recognized as a setting; the value
	// of the setting should be a list of font files.
	// Size should be a recognized string value
	// Style should be a set of flags including any implied by the font name.

	// First decipher the requested size.
	LLFontDescriptor norm_desc = desc.normalize();
	F32 point_size;
	bool found_size = nameToSize(norm_desc.getSize(),point_size);
	if (!found_size)
	{
		llwarns << "createFont unrecognized size " << norm_desc.getSize() << llendl;
		return NULL;
	}
	llinfos << "createFont " << norm_desc.getName() << " size " << norm_desc.getSize() << " style " << ((S32) norm_desc.getStyle()) << llendl;
	F32 fallback_scale = 1.0;

	// Find corresponding font template (based on same descriptor with no size specified)
	LLFontDescriptor template_desc(norm_desc);
	template_desc.setSize(s_template_string);
	const LLFontDescriptor *match_desc = getClosestFontTemplate(template_desc);
	if (!match_desc)
	{
		llwarns << "createFont failed, no template found for "
				<< norm_desc.getName() << " style [" << ((S32)norm_desc.getStyle()) << "]" << llendl;
		return NULL;
	}

	// See whether this best-match font has already been instantiated in the requested size.
	LLFontDescriptor nearest_exact_desc = *match_desc;
	nearest_exact_desc.setSize(norm_desc.getSize());
	font_reg_map_t::iterator it = mFontMap.find(nearest_exact_desc);
	// If we fail to find a font in the fonts directory, it->second might be NULL.
	// We shouldn't construcnt a font with a NULL mFontFreetype.
	// This may not be the best solution, but it at least prevents a crash.
	if (it != mFontMap.end() && it->second != NULL)
	{
		llinfos << "-- matching font exists: " << nearest_exact_desc.getName() << " size " << nearest_exact_desc.getSize() << " style " << ((S32) nearest_exact_desc.getStyle()) << llendl;
		
		// copying underlying Freetype font, and storing in LLFontGL with requested font descriptor
		LLFontGL *font = new LLFontGL;
		font->mFontDescriptor = desc;
		font->mFontFreetype = it->second->mFontFreetype;
		mFontMap[desc] = font;

		return font;
	}

	// Build list of font names to look for.
	// Files specified for this font come first, followed by those from the default descriptor.
	string_vec_t file_names = match_desc->getFileNames();
	string_vec_t default_file_names;
	LLFontDescriptor default_desc("default",s_template_string,0);
	const LLFontDescriptor *match_default_desc = getMatchingFontDesc(default_desc);
	if (match_default_desc)
	{
		file_names.insert(file_names.end(),
						  match_default_desc->getFileNames().begin(),
						  match_default_desc->getFileNames().end());
	}

	// Add ultimate fallback list - generated dynamically on linux,
	// null elsewhere.
	file_names.insert(file_names.end(),
					  getUltimateFallbackList().begin(),
					  getUltimateFallbackList().end());

	// Load fonts based on names.
	if (file_names.empty())
	{
		llwarns << "createFont failed, no file names specified" << llendl;
		return NULL;
	}

	LLFontFreetype::font_vector_t fontlist;
	LLFontGL *result = NULL;

	// Snarf all fonts we can into fontlist.  First will get pulled
	// off the list and become the "head" font, set to non-fallback.
	// Rest will consitute the fallback list.
	BOOL is_first_found = TRUE;
	
	std::string local_path = LLFontGL::getFontPathLocal();
	std::string sys_path = LLFontGL::getFontPathSystem();
	
	// The fontname string may contain multiple font file names separated by semicolons.
	// Break it apart and try loading each one, in order.
	for(string_vec_t::iterator file_name_it = file_names.begin();
		file_name_it != file_names.end(); 
		++file_name_it)
	{
		LLFontGL *fontp = new LLFontGL;
		std::string font_path = local_path + *file_name_it;
		// *HACK: Fallback fonts don't render, so we can use that to suppress
		// creation of OpenGL textures for test apps. JC
		BOOL is_fallback = !is_first_found || !mCreateGLTextures;
		F32 extra_scale = (is_fallback)?fallback_scale:1.0;
		if (!fontp->loadFace(font_path, extra_scale * point_size,
							 LLFontGL::sVertDPI, LLFontGL::sHorizDPI, 2, is_fallback))
		{
			font_path = sys_path + *file_name_it;

			if (!fontp->loadFace(font_path, extra_scale * point_size,
								 LLFontGL::sVertDPI, LLFontGL::sHorizDPI, 2, is_fallback))
			{
				LL_INFOS_ONCE("LLFontRegistry") << "Couldn't load font " << *file_name_it << LL_ENDL;
				delete fontp;
				fontp = NULL;
			}
		}
		
		if(fontp)
		{
			if (is_first_found)
			{
				result = fontp;
				is_first_found = false;
			}
			else
			{
				fontlist.push_back(fontp->mFontFreetype);
				delete fontp;
				fontp = NULL;
			}
		}
	}

	if (result && !fontlist.empty())
	{
		result->mFontFreetype->setFallbackFonts(fontlist);
	}

	if (result)
	{
		result->mFontDescriptor = desc;
	}
	else
	{
		llwarns << "createFont failed in some way" << llendl;
	}

	mFontMap[desc] = result;
	return result;
}

void LLFontRegistry::reset()
{
	for (font_reg_map_t::iterator it = mFontMap.begin();
		 it != mFontMap.end();
		 ++it)
	{
		// Reset the corresponding font but preserve the entry.
		if (it->second)
			it->second->reset();
	}
}

void LLFontRegistry::clear()
{
	for (font_reg_map_t::iterator it = mFontMap.begin();
		 it != mFontMap.end();
		 ++it)
	{
		LLFontGL *fontp = it->second;
		delete fontp;
	}
	mFontMap.clear();
}

void LLFontRegistry::destroyGL()
{
	for (font_reg_map_t::iterator it = mFontMap.begin();
		 it != mFontMap.end();
		 ++it)
	{
		// Reset the corresponding font but preserve the entry.
		if (it->second)
			it->second->destroyGL();
	}
}

LLFontGL *LLFontRegistry::getFont(const LLFontDescriptor& desc)
{
	font_reg_map_t::iterator it = mFontMap.find(desc);
	if (it != mFontMap.end())
		return it->second;
	else
	{
		LLFontGL *fontp = createFont(desc);
		if (!fontp)
		{
			llwarns << "getFont failed, name " << desc.getName()
					<<" style=[" << ((S32) desc.getStyle()) << "]"
					<< " size=[" << desc.getSize() << "]" << llendl;
		}
		return fontp;
	}
}

const LLFontDescriptor *LLFontRegistry::getMatchingFontDesc(const LLFontDescriptor& desc)
{
	LLFontDescriptor norm_desc = desc.normalize();

	font_reg_map_t::iterator it = mFontMap.find(norm_desc);
	if (it != mFontMap.end())
		return &(it->first);
	else
		return NULL;
}

static U32 bitCount(U8 c)
{
	U32 count = 0;
	if (c & 1) 
		count++;
	if (c & 2)
		count++;
	if (c & 4)
		count++;
	if (c & 8)
		count++;
	if (c & 16)
		count++;
	if (c & 32)
		count++;
	if (c & 64)
		count++;
	if (c & 128)
		count++;
	return count;
}

// Find nearest match for the requested descriptor.
const LLFontDescriptor *LLFontRegistry::getClosestFontTemplate(const LLFontDescriptor& desc)
{
	const LLFontDescriptor *exact_match_desc = getMatchingFontDesc(desc);
	if (exact_match_desc)
	{
		return exact_match_desc;
	}

	LLFontDescriptor norm_desc = desc.normalize();

	const LLFontDescriptor *best_match_desc = NULL;
	for (font_reg_map_t::iterator it = mFontMap.begin();
		 it != mFontMap.end();
		 ++it)
	{
		const LLFontDescriptor* curr_desc = &(it->first);

		// Ignore if not a template.
		if (!curr_desc->isTemplate())
			continue;

		// Ignore if font name is wrong.
		if (curr_desc->getName() != norm_desc.getName())
			continue;

		// Reject font if it matches any bits we don't want
		if (curr_desc->getStyle() & ~norm_desc.getStyle())
		{
			continue;
		}

		// Take if it's the first plausible candidate we've found.
		if (!best_match_desc)
		{
			best_match_desc = curr_desc;
			continue;
		}

		// Take if it matches more bits than anything before.
		U8 best_style_match_bits =
			norm_desc.getStyle() & best_match_desc->getStyle();
		U8 curr_style_match_bits =
			norm_desc.getStyle() & curr_desc->getStyle();
		if (bitCount(curr_style_match_bits) > bitCount(best_style_match_bits))
		{
			best_match_desc = curr_desc;
			continue;
		}

		// Tie-breaker: take if it matches bold.
		if (curr_style_match_bits & LLFontGL::BOLD)  // Bold is requested and this descriptor matches it.
		{
			best_match_desc = curr_desc;
			continue;
		}
	}

	// Nothing matched.
	return best_match_desc;
}

void LLFontRegistry::dump()
{
	llinfos << "LLFontRegistry dump: " << llendl;
	for (font_size_map_t::iterator size_it = mFontSizes.begin();
		 size_it != mFontSizes.end();
		 ++size_it)
	{
		llinfos << "Size: " << size_it->first << " => " << size_it->second << llendl;
	}
	for (font_reg_map_t::iterator font_it = mFontMap.begin();
		 font_it != mFontMap.end();
		 ++font_it)
	{
		const LLFontDescriptor& desc = font_it->first;
		llinfos << "Font: name=" << desc.getName()
				<< " style=[" << ((S32)desc.getStyle()) << "]"
				<< " size=[" << desc.getSize() << "]"
				<< " fileNames="
				<< llendl;
		for (string_vec_t::const_iterator file_it=desc.getFileNames().begin();
			 file_it != desc.getFileNames().end();
			 ++file_it)
		{
			llinfos << "  file: " << *file_it <<llendl;
		}
	}
}

const string_vec_t& LLFontRegistry::getUltimateFallbackList() const 
{ 
	return mUltimateFallbackList;
}
