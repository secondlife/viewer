/**
 * @file llcontractparityargs.cpp
 * @brief Early command-line selection for isolated renderer parity diagnostics.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llcontractparityargs.h"

#include "llstring.h"

#include <string>
#include <string_view>

namespace
{
constexpr std::string_view MATERIAL_SETTING = "RenderMaterialContractParityTest";
constexpr std::string_view TEXTURE_UPLOAD_SETTING = "RenderTextureUploadContractParityTest";
constexpr std::string_view TONEMAP_SETTING  = "RenderTonemapContractParityTest";
constexpr std::string_view GLOBAL_PREFIX    = "Global.";

std::string_view argumentAt(int index, int argc, char* const* argv)
{
    if (!argv || index < 0 || index >= argc || !argv[index])
    {
        return {};
    }
    return argv[index];
}

bool isDirectFlag(std::string_view argument, std::string_view name)
{
    if (argument.starts_with("--"))
    {
        argument.remove_prefix(2);
    }
    else if (argument.starts_with("-"))
    {
        argument.remove_prefix(1);
    }
    else
    {
        return false;
    }
    return argument == name;
}

std::string_view unqualifiedName(std::string_view name)
{
    if (name.starts_with(GLOBAL_PREFIX))
    {
        name.remove_prefix(GLOBAL_PREFIX.size());
    }
    return name;
}

void applySetPair(std::string_view name, std::string_view value, LLContractParitySelection& selection)
{
    name = unqualifiedName(name);
    if (name != MATERIAL_SETTING && name != TEXTURE_UPLOAD_SETTING && name != TONEMAP_SETTING)
    {
        return;
    }

    bool enabled = false;
    LLStringUtil::convertToBOOL(std::string(value), enabled);
    if (name == MATERIAL_SETTING)
    {
        selection.mMaterial = enabled;
    }
    else if (name == TEXTURE_UPLOAD_SETTING)
    {
        selection.mTextureUpload = enabled;
    }
    else
    {
        selection.mTonemap = enabled;
    }
}
} // namespace

LLContractParitySelection getRawContractParitySelection(int argc, char* const* argv)
{
    LLContractParitySelection selection;

    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument = argumentAt(index, argc, argv);
        if (argument == "--")
        {
            break;
        }
        if (isDirectFlag(argument, "tonemapparity"))
        {
            selection.mTonemap = true;
        }
        else if (isDirectFlag(argument, "materialparity"))
        {
            selection.mMaterial = true;
        }
        else if (isDirectFlag(argument, "textureuploadparity"))
        {
            selection.mTextureUpload = true;
        }
    }

    constexpr std::string_view LONG_SET_PREFIX  = "--set=";
    constexpr std::string_view SHORT_SET_PREFIX = "-set=";
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument = argumentAt(index, argc, argv);
        if (argument == "--")
        {
            break;
        }
        if (argument == "--set" || argument == "-set")
        {
            if (index + 2 < argc)
            {
                applySetPair(argumentAt(index + 1, argc, argv), argumentAt(index + 2, argc, argv), selection);
                index += 2;
            }
        }
        else if (argument.starts_with(LONG_SET_PREFIX) && index + 1 < argc)
        {
            applySetPair(argument.substr(LONG_SET_PREFIX.size()), argumentAt(index + 1, argc, argv), selection);
            ++index;
        }
        else if (argument.starts_with(SHORT_SET_PREFIX) && index + 1 < argc)
        {
            applySetPair(argument.substr(SHORT_SET_PREFIX.size()), argumentAt(index + 1, argc, argv), selection);
            ++index;
        }
    }

    return selection;
}
