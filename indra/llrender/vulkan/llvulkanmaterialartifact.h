/**
 * @file llvulkanmaterialartifact.h
 * @brief Owned loading contract for packaged Vulkan material shaders.
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

#ifndef LL_LLVULKANMATERIALARTIFACT_H
#define LL_LLVULKANMATERIALARTIFACT_H

#include "llshadermanifest.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace LLRenderContract
{

enum class ShaderArtifactLoadCode : std::uint8_t
{
    InvalidManifest,
    MissingRoot,
    MissingModule,
    NotRegularFile,
    InvalidSize,
    ReadFailure,
    InvalidSpirv,
    WrongStage,
    WrongEntryPoint
};

struct ShaderArtifactLoadError
{
    ShaderArtifactLoadCode     mCode = ShaderArtifactLoadCode::InvalidManifest;
    std::optional<ShaderStage> mStage;

    friend constexpr bool operator==(const ShaderArtifactLoadError&, const ShaderArtifactLoadError&) = default;
};

struct LoadedShaderModule
{
    ShaderStage                mStage = ShaderStage::Vertex;
    std::string                mEntryPoint;
    std::vector<std::uint32_t> mWords;

    friend bool operator==(const LoadedShaderModule&, const LoadedShaderModule&) = default;
};

struct LoadedShaderProgram
{
    ShaderProgramKey   mProgram;
    LoadedShaderModule mVertex;
    LoadedShaderModule mFragment{ ShaderStage::Fragment, {}, {} };

    friend bool operator==(const LoadedShaderProgram& left, const LoadedShaderProgram& right);
};

// The app-settings tree is trusted read-only installation data. Callers must
// not modify it concurrently with a load; the portable checks below detect
// corruption, but are not a hostile-filesystem sandbox.
using ShaderArtifactLoadResult = std::variant<ShaderArtifactLoadError, LoadedShaderProgram>;

bool validLegacyNormSpecProductionShaderProgram(const LoadedShaderProgram& program) noexcept;

ShaderArtifactLoadResult loadLegacyNormSpecProductionArtifacts(const std::filesystem::path& app_settings_root) noexcept;

} // namespace LLRenderContract

#endif // LL_LLVULKANMATERIALARTIFACT_H
