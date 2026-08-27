/**
 * @file llcontractparityargs_test.cpp
 * @brief Tests for early renderer parity command-line selection.
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

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llcontractparityargs.h"

#include <initializer_list>
#include <string>
#include <vector>

namespace tut
{
struct contract_parity_args_test
{
    static LLContractParitySelection parse(std::initializer_list<const char*> arguments)
    {
        std::vector<std::string> storage(arguments.begin(), arguments.end());
        std::vector<char*>       argv;
        argv.reserve(storage.size());
        for (std::string& argument : storage)
        {
            argv.push_back(argument.data());
        }
        return getRawContractParitySelection(static_cast<int>(argv.size()), argv.data());
    }
};

typedef test_group<contract_parity_args_test> contract_parity_args_test_t;
typedef contract_parity_args_test_t::object   contract_parity_args_test_object_t;
contract_parity_args_test_t                   contract_parity_args_tests("LLContractParityArgs");

template<>
template<>
void contract_parity_args_test_object_t::test<1>()
{
    const auto ordinary = parse({ "viewer", "--materialartifact", "RenderMaterialContractParityTest" });
    ensure("artifact value is not a material selection", !ordinary.mMaterial);
    ensure("ordinary launch is not a tonemap selection", !ordinary.mTonemap);
    ensure("ordinary launch is not a texture upload selection", !ordinary.mTextureUpload);
}

template<>
template<>
void contract_parity_args_test_object_t::test<2>()
{
    ensure("long material flag", parse({ "viewer", "--materialparity" }).mMaterial);
    ensure("short material flag", parse({ "viewer", "-materialparity" }).mMaterial);
    ensure("long tonemap flag", parse({ "viewer", "--tonemapparity" }).mTonemap);
    ensure("short tonemap flag", parse({ "viewer", "-tonemapparity" }).mTonemap);
    ensure("long texture upload flag", parse({ "viewer", "--textureuploadparity" }).mTextureUpload);
    ensure("short texture upload flag", parse({ "viewer", "-textureuploadparity" }).mTextureUpload);
}

template<>
template<>
void contract_parity_args_test_object_t::test<3>()
{
    ensure("separate set pair", parse({ "viewer", "--set", "RenderMaterialContractParityTest", "true" }).mMaterial);
    ensure("qualified set pair", parse({ "viewer", "-set", "Global.RenderMaterialContractParityTest", "1" }).mMaterial);
    ensure("long attached set name", parse({ "viewer", "--set=RenderTonemapContractParityTest", "T" }).mTonemap);
    ensure("short attached set name", parse({ "viewer", "-set=Global.RenderTonemapContractParityTest", " True " }).mTonemap);
    ensure("texture upload set pair",
           parse({ "viewer", "--set", "RenderTextureUploadContractParityTest", "true" }).mTextureUpload);
}

template<>
template<>
void contract_parity_args_test_object_t::test<4>()
{
    const auto overridden = parse({ "viewer", "--set", "RenderMaterialContractParityTest", "true", "--materialparity",
                                    "--set=RenderMaterialContractParityTest", "false" });
    ensure("set false overrides direct flag", !overridden.mMaterial);

    const auto repeated =
        parse({ "viewer", "--set", "RenderTonemapContractParityTest", "false", "--set", "Global.RenderTonemapContractParityTest", "true" });
    ensure("last composing set pair wins", repeated.mTonemap);
}

template<>
template<>
void contract_parity_args_test_object_t::test<5>()
{
    ensure("invalid boolean becomes false",
           !parse({ "viewer", "--tonemapparity", "--set", "RenderTonemapContractParityTest", "yes" }).mTonemap);
    ensure("another control group is ignored", !parse({ "viewer", "--set", "Session.RenderMaterialContractParityTest", "true" }).mMaterial);
    ensure("tokens after option terminator are ignored", !parse({ "viewer", "--", "--materialparity" }).mMaterial);
    ensure("texture upload tokens after option terminator are ignored",
           !parse({ "viewer", "--", "--textureuploadparity" }).mTextureUpload);
}
} // namespace tut
