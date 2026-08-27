/**
 * @file llvulkanmaterialartifact.cpp
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

#include "llvulkanmaterialartifact.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string_view>
#include <system_error>
#include <utility>

namespace LLRenderContract
{
namespace
{

    constexpr std::uint32_t  SPIRV_MAGIC                    = 0x07230203;
    constexpr std::uint16_t  SPIRV_OP_ENTRY_POINT           = 15;
    constexpr std::uint32_t  SPIRV_EXECUTION_MODEL_VERTEX   = 0;
    constexpr std::uint32_t  SPIRV_EXECUTION_MODEL_FRAGMENT = 4;
    constexpr std::uintmax_t MIN_MODULE_BYTES               = 5 * sizeof(std::uint32_t);
    constexpr std::uintmax_t MAX_MODULE_BYTES               = 16 * 1024 * 1024;

    constexpr std::array<const char*, 3> MODULE_DIRECTORIES{ "shaders", "vulkan", "legacy_normspec" };
    constexpr const char*                VERTEX_MODULE_NAME   = "production.vert.spv";
    constexpr const char*                FRAGMENT_MODULE_NAME = "production.frag.spv";

    struct ModuleExpectation
    {
        ShaderStage      mStage = ShaderStage::Vertex;
        std::string_view mEntryPoint;
        std::uint32_t    mExecutionModel = SPIRV_EXECUTION_MODEL_VERTEX;
        const char*      mFileName       = VERTEX_MODULE_NAME;
    };

    using ModuleLoadResult = std::variant<LoadedShaderModule, ShaderArtifactLoadError>;

    ShaderArtifactLoadError failure(ShaderArtifactLoadCode code, std::optional<ShaderStage> stage = std::nullopt) noexcept
    {
        return { code, stage };
    }

    bool missing(const std::error_code& error) noexcept
    {
        return error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory;
    }

    std::filesystem::path checkedRootPath(const std::filesystem::path& root)
    {
        std::filesystem::path checked = root;
        while (checked != checked.root_path())
        {
            const std::filesystem::path filename = checked.filename();
            if (filename.empty() || (filename == "." && !checked.parent_path().empty()))
            {
                checked = checked.parent_path();
                continue;
            }
            break;
        }
        return checked;
    }

    std::optional<std::uint32_t> executionModel(ShaderStage stage) noexcept
    {
        switch (stage)
        {
            case ShaderStage::Vertex:
                return SPIRV_EXECUTION_MODEL_VERTEX;
            case ShaderStage::Fragment:
                return SPIRV_EXECUTION_MODEL_FRAGMENT;
        }
        return std::nullopt;
    }

    const ShaderEntryPoint* entryPoint(const ShaderManifest& manifest, ShaderStage stage) noexcept
    {
        const ShaderEntryPoint* found = nullptr;
        for (const ShaderEntryPoint& entry : manifest.mEntryPoints)
        {
            if (entry.mStage != stage)
            {
                continue;
            }
            if (found)
            {
                return nullptr;
            }
            found = &entry;
        }
        return found;
    }

    std::optional<ShaderArtifactLoadError> validRoot(const std::filesystem::path& root)
    {
        std::error_code                    error;
        const std::filesystem::file_status status = std::filesystem::symlink_status(root, error);
        if (error)
        {
            return failure(missing(error) ? ShaderArtifactLoadCode::MissingRoot : ShaderArtifactLoadCode::ReadFailure);
        }
        if (!std::filesystem::exists(status))
        {
            return failure(ShaderArtifactLoadCode::MissingRoot);
        }
        if (std::filesystem::is_symlink(status) || !std::filesystem::is_directory(status))
        {
            return failure(ShaderArtifactLoadCode::NotRegularFile);
        }
        return std::nullopt;
    }

    std::variant<std::filesystem::path, ShaderArtifactLoadError> modulePath(const std::filesystem::path& root,
                                                                            const ModuleExpectation&     expected)
    {
        std::filesystem::path path = root;
        for (const char* component : MODULE_DIRECTORIES)
        {
            path /= component;
            std::error_code                    error;
            const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);
            if (error)
            {
                return failure(missing(error) ? ShaderArtifactLoadCode::MissingModule : ShaderArtifactLoadCode::ReadFailure,
                               expected.mStage);
            }
            if (!std::filesystem::exists(status))
            {
                return failure(ShaderArtifactLoadCode::MissingModule, expected.mStage);
            }
            if (std::filesystem::is_symlink(status) || !std::filesystem::is_directory(status))
            {
                return failure(ShaderArtifactLoadCode::NotRegularFile, expected.mStage);
            }
        }

        path /= expected.mFileName;
        std::error_code                    error;
        const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);
        if (error)
        {
            return failure(missing(error) ? ShaderArtifactLoadCode::MissingModule : ShaderArtifactLoadCode::ReadFailure, expected.mStage);
        }
        if (!std::filesystem::exists(status))
        {
            return failure(ShaderArtifactLoadCode::MissingModule, expected.mStage);
        }
        if (std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status))
        {
            return failure(ShaderArtifactLoadCode::NotRegularFile, expected.mStage);
        }
        return path;
    }

    std::variant<std::vector<std::uint32_t>, ShaderArtifactLoadError> readModuleWords(const std::filesystem::path& path, ShaderStage stage)
    {
        std::error_code      size_error;
        const std::uintmax_t byte_count = std::filesystem::file_size(path, size_error);
        if (size_error)
        {
            return failure(ShaderArtifactLoadCode::ReadFailure, stage);
        }
        if (byte_count < MIN_MODULE_BYTES || byte_count > MAX_MODULE_BYTES || byte_count % sizeof(std::uint32_t) != 0)
        {
            return failure(ShaderArtifactLoadCode::InvalidSize, stage);
        }

        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open())
        {
            return failure(ShaderArtifactLoadCode::ReadFailure, stage);
        }

        std::vector<unsigned char> bytes(static_cast<std::size_t>(byte_count));
        const auto                 stream_size = static_cast<std::streamsize>(byte_count);
        stream.read(reinterpret_cast<char*>(bytes.data()), stream_size);
        if (stream.gcount() != stream_size || stream.bad())
        {
            return failure(ShaderArtifactLoadCode::ReadFailure, stage);
        }
        if (stream.peek() != std::char_traits<char>::eof() || stream.bad())
        {
            return failure(ShaderArtifactLoadCode::ReadFailure, stage);
        }

        std::vector<std::uint32_t> words(bytes.size() / sizeof(std::uint32_t));
        for (std::size_t index = 0; index < words.size(); ++index)
        {
            const std::size_t offset = index * sizeof(std::uint32_t);
            words[index]             = static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
                           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
        }
        return words;
    }

    bool entryPointNameMatches(const std::vector<std::uint32_t>& words, std::size_t first_word, std::size_t end_word,
                               std::string_view expected) noexcept
    {
        std::size_t character = 0;
        for (std::size_t word_index = first_word; word_index < end_word; ++word_index)
        {
            const std::uint32_t word = words[word_index];
            for (std::uint32_t byte_index = 0; byte_index < sizeof(std::uint32_t); ++byte_index)
            {
                const auto byte = static_cast<unsigned char>((word >> (byte_index * 8)) & 0xff);
                if (byte == 0)
                {
                    for (++byte_index; byte_index < sizeof(std::uint32_t); ++byte_index)
                    {
                        if (((word >> (byte_index * 8)) & 0xff) != 0)
                        {
                            return false;
                        }
                    }
                    return character == expected.size();
                }
                if (character >= expected.size() || byte != static_cast<unsigned char>(expected[character]))
                {
                    return false;
                }
                ++character;
            }
        }
        return false;
    }

    std::optional<ShaderArtifactLoadCode> validateModule(const std::vector<std::uint32_t>& words,
                                                         const ModuleExpectation&          expected) noexcept
    {
        if (words.size() < 5 || words.size() > MAX_MODULE_BYTES / sizeof(std::uint32_t) || words[0] != SPIRV_MAGIC)
        {
            return ShaderArtifactLoadCode::InvalidSpirv;
        }

        std::size_t   entry_point_count       = 0;
        bool          entry_point_well_formed = false;
        std::uint32_t entry_execution_model   = 0;
        for (std::size_t cursor = 5; cursor < words.size();)
        {
            const std::uint32_t instruction = words[cursor];
            const std::uint32_t word_count  = instruction >> 16;
            const std::uint16_t opcode      = static_cast<std::uint16_t>(instruction & 0xffff);
            if (word_count == 0 || word_count > words.size() - cursor)
            {
                return ShaderArtifactLoadCode::InvalidSpirv;
            }

            if (opcode == SPIRV_OP_ENTRY_POINT)
            {
                ++entry_point_count;
                if (word_count >= 4)
                {
                    entry_execution_model   = words[cursor + 1];
                    entry_point_well_formed = entryPointNameMatches(words, cursor + 3, cursor + word_count, expected.mEntryPoint);
                }
            }
            cursor += word_count;
        }

        if (entry_point_count != 1 || !entry_point_well_formed)
        {
            return ShaderArtifactLoadCode::WrongEntryPoint;
        }
        if (entry_execution_model != expected.mExecutionModel)
        {
            return ShaderArtifactLoadCode::WrongStage;
        }
        return std::nullopt;
    }

    ModuleLoadResult loadModule(const std::filesystem::path& root, const ModuleExpectation& expected)
    {
        auto path = modulePath(root, expected);
        if (const auto* error = std::get_if<ShaderArtifactLoadError>(&path))
        {
            return *error;
        }

        auto words = readModuleWords(std::get<std::filesystem::path>(path), expected.mStage);
        if (const auto* error = std::get_if<ShaderArtifactLoadError>(&words))
        {
            return *error;
        }

        std::vector<std::uint32_t> owned_words = std::move(std::get<std::vector<std::uint32_t>>(words));
        if (const auto validation_error = validateModule(owned_words, expected))
        {
            return failure(*validation_error, expected.mStage);
        }
        return LoadedShaderModule{ expected.mStage, std::string(expected.mEntryPoint), std::move(owned_words) };
    }

} // namespace

bool operator==(const LoadedShaderProgram& left, const LoadedShaderProgram& right)
{
    return left.mProgram.mName == right.mProgram.mName && left.mProgram.mVariant == right.mProgram.mVariant &&
           left.mVertex == right.mVertex && left.mFragment == right.mFragment;
}

bool validLegacyNormSpecProductionShaderProgram(const LoadedShaderProgram& program) noexcept
{
    try
    {
        const auto manifest = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
        if (!manifest || !validLegacyNormSpecProductionShaderManifest(*manifest) || program.mProgram.mName != manifest->mProgram.mName ||
            program.mProgram.mVariant != manifest->mProgram.mVariant)
        {
            return false;
        }

        const ShaderEntryPoint* vertex_entry   = entryPoint(*manifest, ShaderStage::Vertex);
        const ShaderEntryPoint* fragment_entry = entryPoint(*manifest, ShaderStage::Fragment);
        const auto              vertex_model   = executionModel(ShaderStage::Vertex);
        const auto              fragment_model = executionModel(ShaderStage::Fragment);
        if (!vertex_entry || !fragment_entry || !vertex_model || !fragment_model)
        {
            return false;
        }

        const ModuleExpectation vertex_expected{ ShaderStage::Vertex, vertex_entry->mName, *vertex_model, VERTEX_MODULE_NAME };
        const ModuleExpectation fragment_expected{ ShaderStage::Fragment, fragment_entry->mName, *fragment_model, FRAGMENT_MODULE_NAME };
        return program.mVertex.mStage == vertex_expected.mStage && program.mVertex.mEntryPoint == vertex_expected.mEntryPoint &&
               !validateModule(program.mVertex.mWords, vertex_expected) && program.mFragment.mStage == fragment_expected.mStage &&
               program.mFragment.mEntryPoint == fragment_expected.mEntryPoint &&
               !validateModule(program.mFragment.mWords, fragment_expected);
    }
    catch (...)
    {
        return false;
    }
}

ShaderArtifactLoadResult loadLegacyNormSpecProductionArtifacts(const std::filesystem::path& app_settings_root) noexcept
{
    try
    {
        const auto manifest = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
        if (!manifest || !validLegacyNormSpecProductionShaderManifest(*manifest))
        {
            return failure(ShaderArtifactLoadCode::InvalidManifest);
        }

        const ShaderEntryPoint* vertex_entry   = entryPoint(*manifest, ShaderStage::Vertex);
        const ShaderEntryPoint* fragment_entry = entryPoint(*manifest, ShaderStage::Fragment);
        const auto              vertex_model   = executionModel(ShaderStage::Vertex);
        const auto              fragment_model = executionModel(ShaderStage::Fragment);
        if (!vertex_entry || !fragment_entry || !vertex_model || !fragment_model)
        {
            return failure(ShaderArtifactLoadCode::InvalidManifest);
        }

        const std::filesystem::path checked_root = checkedRootPath(app_settings_root);
        if (const auto root_error = validRoot(checked_root))
        {
            return *root_error;
        }

        const ModuleExpectation vertex_expected{ ShaderStage::Vertex, vertex_entry->mName, *vertex_model, VERTEX_MODULE_NAME };
        const ModuleExpectation fragment_expected{ ShaderStage::Fragment, fragment_entry->mName, *fragment_model, FRAGMENT_MODULE_NAME };

        auto vertex = loadModule(checked_root, vertex_expected);
        if (const auto* error = std::get_if<ShaderArtifactLoadError>(&vertex))
        {
            return *error;
        }
        auto fragment = loadModule(checked_root, fragment_expected);
        if (const auto* error = std::get_if<ShaderArtifactLoadError>(&fragment))
        {
            return *error;
        }

        LoadedShaderProgram loaded{ manifest->mProgram, std::move(std::get<LoadedShaderModule>(vertex)),
                                    std::move(std::get<LoadedShaderModule>(fragment)) };
        if (!validLegacyNormSpecProductionShaderProgram(loaded))
        {
            return failure(ShaderArtifactLoadCode::InvalidManifest);
        }
        return loaded;
    }
    catch (...)
    {
        return failure(ShaderArtifactLoadCode::ReadFailure);
    }
}

} // namespace LLRenderContract
