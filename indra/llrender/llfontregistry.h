/**
 * @file llfontregistry.h
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

#ifndef LL_LLFONTREGISTRY_H
#define LL_LLFONTREGISTRY_H

#include "llpointer.h"

class LLFontGL;

typedef std::vector<std::string> string_vec_t;

struct LLFontFileInfo
{
    LLFontFileInfo(const std::string& file_name, const std::function<bool(llwchar)>& char_functor = nullptr)
        : FileName(file_name)
        , CharFunctor(char_functor)
    {
    }

    LLFontFileInfo(const LLFontFileInfo& ffi)
        : FileName(ffi.FileName)
        , CharFunctor(ffi.CharFunctor)
    {
    }

    std::string FileName;
    std::function<bool(llwchar)> CharFunctor;
};
typedef std::vector<LLFontFileInfo> font_file_info_vec_t;

class LLFontDescriptor
{
public:
    LLFontDescriptor();
    LLFontDescriptor(const std::string& name, const std::string& size, const U8 style);
    LLFontDescriptor(const std::string& name, const std::string& size, const U8 style, const font_file_info_vec_t& font_list);
    LLFontDescriptor(const std::string& name, const std::string& size, const U8 style, const font_file_info_vec_t& font_list, const font_file_info_vec_t& font_collection_list);
    LLFontDescriptor normalize() const;

    bool operator<(const LLFontDescriptor& b) const;

    bool isTemplate() const;

    const std::string& getName() const { return mName; }
    void setName(const std::string& name) { mName = name; }
    const std::string& getSize() const { return mSize; }
    void setSize(const std::string& size) { mSize = size; }

    void addFontFile(const std::string& file_name, const std::string& char_functor = LLStringUtil::null);
    const font_file_info_vec_t & getFontFiles() const { return mFontFiles; }
    void setFontFiles(const font_file_info_vec_t& font_files) { mFontFiles = font_files; }
    void addFontCollectionFile(const std::string& file_name, const std::string& char_functor = LLStringUtil::null);
    const font_file_info_vec_t& getFontCollectionFiles() const { return mFontCollectionFiles; }
    void setFontCollectionFiles(const font_file_info_vec_t& font_collection_files) { mFontCollectionFiles = font_collection_files; }

    const U8 getStyle() const { return mStyle; }
    void setStyle(U8 style) { mStyle = style; }

private:
    std::string mName;
    std::string mSize;
    font_file_info_vec_t mFontFiles;
    font_file_info_vec_t mFontCollectionFiles;
    U8 mStyle;

    typedef std::map<std::string, std::function<bool(llwchar)>> char_functor_map_t;
    static char_functor_map_t mCharFunctors;
};

class LLFontRegistry
{
public:
    friend bool init_from_xml(LLFontRegistry*, LLPointer<class LLXMLNode>);
    // create_gl_textures - set to false for test apps with no OpenGL window,
    // such as llui_libtest
    LLFontRegistry(bool create_gl_textures);
    ~LLFontRegistry();

    // Load standard font info from XML file(s).
    bool parseFontInfo(const std::string& xml_filename);

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
    void dumpTextures();

    const string_vec_t& getUltimateFallbackList() const;

private:
    LLFontRegistry(const LLFontRegistry& other); // no-copy
    LLFontGL *createFont(const LLFontDescriptor& desc);
    typedef std::map<LLFontDescriptor,LLFontGL*> font_reg_map_t;
    typedef std::map<std::string,F32> font_size_map_t;

    // Given a descriptor, look up specific font instantiation.
    font_reg_map_t mFontMap;
    // Given a size name, look up the point size.
    font_size_map_t mFontSizes;

    string_vec_t mUltimateFallbackList;
    bool mCreateGLTextures;
};

#endif // LL_LLFONTREGISTRY_H
