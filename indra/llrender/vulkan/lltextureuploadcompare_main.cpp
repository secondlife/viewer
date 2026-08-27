/**
 * @file lltextureuploadcompare_main.cpp
 * @brief Exact comparison for fixed-input streamed texture upload artifacts.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the license only.
 * $/LicenseInfo$
 */

#include "lltextureuploaddiagnostic.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace
{

int fail(const std::string& reason, const std::string& detail)
{
    std::cerr << "TEXTURE_UPLOAD_COMPARE result=fail reason=" << reason;
    if (!detail.empty())
    {
        std::cerr << " detail={" << detail << '}';
    }
    std::cerr << '\n';
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: lltextureuploadcompare <reference> <candidate>\n";
        return 2;
    }

    LLRenderContract::TextureUploadArtifact reference;
    LLRenderContract::TextureUploadArtifact candidate;
    std::string error;
    if (!LLRenderContract::readTextureUploadArtifact(std::filesystem::path(argv[1]), reference, &error))
    {
        return fail("reference_read", error);
    }
    if (!LLRenderContract::readTextureUploadArtifact(std::filesystem::path(argv[2]), candidate, &error))
    {
        return fail("candidate_read", error);
    }

    const LLRenderContract::TextureUploadComparisonStats stats =
        LLRenderContract::compareTextureUploadArtifacts(reference, candidate);
    if (!stats.mComparable)
    {
        return fail("not_comparable", stats.mError);
    }

    std::cout << "TEXTURE_UPLOAD_COMPARE result=" << (stats.mMatch ? "pass" : "fail")
              << " mip_bytes=" << stats.mComparedMipBytes
              << " sample_bytes=" << stats.mComparedSampleBytes
              << " mismatches=" << stats.mMismatchCount;
    if (!stats.mMatch)
    {
        std::cout << " first_plane=" << stats.mFirstMismatchPlane
                  << " first_byte=" << stats.mFirstMismatchByte
                  << " reference=" << static_cast<unsigned>(stats.mFirstReference)
                  << " candidate=" << static_cast<unsigned>(stats.mFirstCandidate);
    }
    std::cout << '\n';
    return stats.mMatch ? 0 : 1;
}
