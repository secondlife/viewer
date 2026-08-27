/**
 * @file llmaterialcompare_main.cpp
 * @brief Strict comparison for fixed-input indexed-material artifacts.
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

#include "llmaterialdiagnostic.h"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{

int fail(const std::string& reason, const std::string& detail)
{
    std::cerr << "MATERIAL_COMPARE result=fail reason=" << reason;
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
        std::cerr << "usage: llmaterialcompare <reference> <candidate>\n";
        return 2;
    }

    LLRenderContract::MaterialArtifact reference;
    LLRenderContract::MaterialArtifact candidate;
    std::string error;
    if (!LLRenderContract::readMaterialArtifact(std::filesystem::path(argv[1]), reference, &error))
    {
        return fail("reference_read", error);
    }
    if (!LLRenderContract::readMaterialArtifact(std::filesystem::path(argv[2]), candidate, &error))
    {
        return fail("candidate_read", error);
    }

    const LLRenderContract::MaterialComparisonStats stats =
        LLRenderContract::compareMaterialArtifacts(reference, candidate);
    if (!stats.mComparable)
    {
        return fail("not_comparable", stats.mError);
    }

    std::cout << std::setprecision(9)
              << "MATERIAL_COMPARE result=" << (stats.mMatch ? "pass" : "fail")
              << " components=" << stats.mComparedComponents
              << " mismatches=" << stats.mMismatchCount
              << " max_abs_error=" << stats.mMaximumAbsoluteError
              << " rgba8_tolerance=" << LLRenderContract::MATERIAL_RGBA8_TOLERANCE
              << " rgba16_tolerance=" << LLRenderContract::MATERIAL_RGBA16_TOLERANCE
              << " depth24_tolerance=" << LLRenderContract::MATERIAL_DEPTH24_TOLERANCE;
    if (!stats.mMatch)
    {
        std::cout << " first_plane=" << stats.mFirstMismatchPlane
                  << " first_pixel=" << stats.mFirstMismatchPixel
                  << " first_channel=" << stats.mFirstMismatchChannel
                  << " reference=" << stats.mFirstReference
                  << " candidate=" << stats.mFirstCandidate
                  << " tolerance=" << stats.mFirstTolerance;
    }
    std::cout << '\n';
    return stats.mMatch ? 0 : 1;
}
