/**
 * @file llvulkanmaterialartifact_test.cpp
 * @brief Tests for loading owned production Vulkan material shader artifacts.
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

#include "linden_common.h"

#include "llvulkanmaterialartifact.h"
#include "lltut.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

namespace
{
using namespace LLRenderContract;

constexpr std::uint32_t  SPIRV_MAGIC              = 0x07230203U;
constexpr std::uint32_t  SPIRV_VERSION_1_0        = 0x00010000U;
constexpr std::uint32_t  OP_ENTRY_POINT           = 15U;
constexpr std::uint32_t  EXECUTION_MODEL_VERTEX   = 0U;
constexpr std::uint32_t  EXECUTION_MODEL_FRAGMENT = 4U;
constexpr std::uintmax_t MAX_MODULE_BYTES         = 16U * 1024U * 1024U;

const std::filesystem::path VERTEX_RELATIVE   = std::filesystem::path("shaders") / "vulkan" / "legacy_normspec" / "production.vert.spv";
const std::filesystem::path FRAGMENT_RELATIVE = std::filesystem::path("shaders") / "vulkan" / "legacy_normspec" / "production.frag.spv";

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        static std::atomic<std::uint64_t> sequence{ 0 };
        std::error_code                   error;
        const std::filesystem::path       parent = std::filesystem::temp_directory_path(error);
        if (error)
        {
            throw std::runtime_error("cannot locate the temporary directory");
        }

        const auto clock = static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        for (std::uint64_t attempt = 0; attempt < 128; ++attempt)
        {
            mPath = parent / ("llvulkanmaterialartifact-" + std::to_string(clock) + "-" + std::to_string(sequence.fetch_add(1)));
            error.clear();
            if (std::filesystem::create_directory(mPath, error))
            {
                return;
            }
            if (error && error != std::errc::file_exists)
            {
                throw std::runtime_error("cannot create a temporary directory");
            }
        }
        throw std::runtime_error("cannot reserve a unique temporary directory");
    }

    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(mPath, ignored);
    }

    TemporaryDirectory(const TemporaryDirectory&)            = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    const std::filesystem::path& path() const noexcept { return mPath; }

private:
    std::filesystem::path mPath;
};

std::vector<std::uint32_t> spirvString(const std::string& value, bool terminated)
{
    std::vector<std::uint8_t> bytes(value.begin(), value.end());
    if (terminated)
    {
        bytes.push_back(0);
        while (bytes.size() % sizeof(std::uint32_t) != 0)
        {
            bytes.push_back(0);
        }
    }
    else if (bytes.size() % sizeof(std::uint32_t) != 0)
    {
        throw std::invalid_argument("unterminated fixture strings must fill whole words");
    }

    std::vector<std::uint32_t> words(bytes.size() / sizeof(std::uint32_t), 0);
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        words[index / sizeof(std::uint32_t)] |= static_cast<std::uint32_t>(bytes[index]) << (8U * (index % sizeof(std::uint32_t)));
    }
    return words;
}

std::vector<std::uint32_t> entryPoint(std::uint32_t execution_model, const std::string& name = "main", bool terminated = true)
{
    std::vector<std::uint32_t> name_words = spirvString(name, terminated);
    std::vector<std::uint32_t> instruction{ static_cast<std::uint32_t>((3U + name_words.size()) << 16U) | OP_ENTRY_POINT, execution_model,
                                            1U };
    instruction.insert(instruction.end(), name_words.begin(), name_words.end());
    return instruction;
}

std::vector<std::uint32_t> moduleWithInstructions(const std::vector<std::vector<std::uint32_t>>& instructions)
{
    std::vector<std::uint32_t> words{ SPIRV_MAGIC, SPIRV_VERSION_1_0, 0U, 2U, 0U };
    for (const auto& instruction : instructions)
    {
        words.insert(words.end(), instruction.begin(), instruction.end());
    }
    return words;
}

std::vector<std::uint32_t> validModule(ShaderStage stage)
{
    return moduleWithInstructions({ entryPoint(stage == ShaderStage::Vertex ? EXECUTION_MODEL_VERTEX : EXECUTION_MODEL_FRAGMENT) });
}

void writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output ||
        (!bytes.empty() && !output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))))
    {
        throw std::runtime_error("cannot write SPIR-V test fixture");
    }
}

void writeWords(const std::filesystem::path& path, const std::vector<std::uint32_t>& words)
{
    std::vector<std::uint8_t> bytes;
    bytes.reserve(words.size() * sizeof(std::uint32_t));
    for (const std::uint32_t word : words)
    {
        bytes.push_back(static_cast<std::uint8_t>(word));
        bytes.push_back(static_cast<std::uint8_t>(word >> 8U));
        bytes.push_back(static_cast<std::uint8_t>(word >> 16U));
        bytes.push_back(static_cast<std::uint8_t>(word >> 24U));
    }
    writeBytes(path, bytes);
}

void writeValidPair(const std::filesystem::path& root)
{
    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Vertex));
    writeWords(root / FRAGMENT_RELATIVE, validModule(ShaderStage::Fragment));
}

void writeOversizedFile(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("cannot open oversized SPIR-V test fixture");
    }
    output.seekp(static_cast<std::streamoff>(MAX_MODULE_BYTES + 3U));
    output.put('\0');
    if (!output)
    {
        throw std::runtime_error("cannot write oversized SPIR-V test fixture");
    }
}

bool hasError(const ShaderArtifactLoadResult& result, ShaderArtifactLoadCode code, std::optional<ShaderStage> stage)
{
    const auto* error = std::get_if<ShaderArtifactLoadError>(&result);
    return error && error->mCode == code && error->mStage == stage;
}

} // namespace

namespace tut
{

struct vulkan_material_artifact_test
{
};

using vulkan_material_artifact_test_group  = test_group<vulkan_material_artifact_test>;
using vulkan_material_artifact_test_object = vulkan_material_artifact_test_group::object;
vulkan_material_artifact_test_group vulkan_material_artifact_tests("vulkan material artifact");

template<>
template<>
void vulkan_material_artifact_test_object::test<1>()
{
    const ShaderArtifactLoadResult default_result;
    ensure("a default result fails closed", hasError(default_result, ShaderArtifactLoadCode::InvalidManifest, std::nullopt));

    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    const ShaderArtifactLoadResult result = loadLegacyNormSpecProductionArtifacts(root);
    const auto*                    loaded = std::get_if<LoadedShaderProgram>(&result);
    ensure("a valid production pair loads", loaded != nullptr);

    const auto modern        = legacyNormSpecShaderManifest(legacyNormSpecModernHDRPipelineKey(), ShaderBackend::Vulkan);
    const auto compatibility = legacyNormSpecShaderManifest(legacyNormSpecCompatibilityPipelineKey(), ShaderBackend::Vulkan);
    ensure("both production profiles have Vulkan manifests", modern.has_value() && compatibility.has_value());
    ensure("both production manifests agree on the program",
           modern->mProgram.mName == compatibility->mProgram.mName && modern->mProgram.mVariant == compatibility->mProgram.mVariant);
    ensure("the loaded program comes from the canonical production manifest",
           loaded->mProgram.mName == modern->mProgram.mName && loaded->mProgram.mVariant == modern->mProgram.mVariant);
    ensure("the loaded stages and entry points are exact",
           loaded->mVertex.mStage == ShaderStage::Vertex && loaded->mVertex.mEntryPoint == "main" &&
               loaded->mFragment.mStage == ShaderStage::Fragment && loaded->mFragment.mEntryPoint == "main");
    ensure("the loaded words match the little-endian fixtures",
           loaded->mVertex.mWords == validModule(ShaderStage::Vertex) && loaded->mFragment.mWords == validModule(ShaderStage::Fragment));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<2>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path missing_root = temporary.path() / "missing";
    ensure("a missing app-settings root fails without a partial program",
           hasError(loadLegacyNormSpecProductionArtifacts(missing_root), ShaderArtifactLoadCode::MissingRoot, std::nullopt));

    const std::filesystem::path root = temporary.path() / "app_settings";
    writeWords(root / FRAGMENT_RELATIVE, validModule(ShaderStage::Fragment));
    ensure("a missing vertex module is attributed to the vertex stage",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::MissingModule, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Vertex));
    std::filesystem::remove(root / FRAGMENT_RELATIVE);
    ensure("a missing fragment module discards the already loaded vertex module",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::MissingModule, ShaderStage::Fragment));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<3>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path real_root = temporary.path() / "real_app_settings";
    const std::filesystem::path root_link = temporary.path() / "linked_app_settings";
    writeValidPair(real_root);

    std::error_code error;
    std::filesystem::create_directory_symlink(real_root, root_link, error);
    ensure("the root symlink fixture is created", !error);
    ensure("an app-settings root symlink is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root_link), ShaderArtifactLoadCode::NotRegularFile, std::nullopt));

    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);
    const std::filesystem::path external = temporary.path() / "external.vert.spv";
    writeWords(external, validModule(ShaderStage::Vertex));
    std::filesystem::remove(root / VERTEX_RELATIVE);
    error.clear();
    std::filesystem::create_symlink(external, root / VERTEX_RELATIVE, error);
    ensure("the module symlink fixture is created", !error);
    ensure("a module symlink is rejected as nonregular",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::NotRegularFile, ShaderStage::Vertex));

    std::filesystem::remove(root / VERTEX_RELATIVE);
    std::filesystem::create_directory(root / VERTEX_RELATIVE);
    ensure("a directory in place of a module is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::NotRegularFile, ShaderStage::Vertex));

    std::filesystem::remove_all(root / VERTEX_RELATIVE);
    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Vertex));
    const std::filesystem::path external_fragment = temporary.path() / "external.frag.spv";
    writeWords(external_fragment, validModule(ShaderStage::Fragment));
    std::filesystem::remove(root / FRAGMENT_RELATIVE);
    error.clear();
    std::filesystem::create_symlink(external_fragment, root / FRAGMENT_RELATIVE, error);
    ensure("the fragment symlink fixture is created", !error);
    ensure("a fragment-module symlink is attributed to the fragment stage",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::NotRegularFile, ShaderStage::Fragment));

    std::filesystem::remove_all(root);
    writeValidPair(root);
    const std::filesystem::path real_vulkan = temporary.path() / "real_vulkan";
    std::filesystem::rename(root / "shaders" / "vulkan", real_vulkan);
    error.clear();
    std::filesystem::create_directory_symlink(real_vulkan, root / "shaders" / "vulkan", error);
    ensure("the intermediate-directory symlink fixture is created", !error);
    ensure("an intermediate directory symlink is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::NotRegularFile, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<4>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    writeBytes(root / VERTEX_RELATIVE, {});
    ensure("an empty module is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSize, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, { SPIRV_MAGIC, SPIRV_VERSION_1_0, 0U, 2U });
    ensure("a module shorter than the five-word header is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSize, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Vertex));
    std::ofstream append(root / VERTEX_RELATIVE, std::ios::binary | std::ios::app);
    append.put('\0');
    append.close();
    ensure("a module whose byte size is not word-aligned is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSize, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<5>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);
    writeOversizedFile(root / VERTEX_RELATIVE);
    ensure("a word-aligned module larger than 16 MiB is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSize, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<6>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);
    std::vector<std::uint32_t> wrong_magic = validModule(ShaderStage::Fragment);
    wrong_magic.front()                    = 0U;
    writeWords(root / FRAGMENT_RELATIVE, wrong_magic);
    ensure("wrong SPIR-V magic fails without returning the valid vertex module",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSpirv, ShaderStage::Fragment));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<7>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ { OP_ENTRY_POINT, EXECUTION_MODEL_VERTEX, 1U, 0x6e69616dU, 0U } }));
    ensure("an instruction with a zero word count is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSpirv, ShaderStage::Vertex));

    std::vector<std::uint32_t> overrun = entryPoint(EXECUTION_MODEL_VERTEX);
    overrun.front() += 1U << 16U;
    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ overrun }));
    ensure("an instruction extending past the module is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::InvalidSpirv, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<8>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ { 1U << 16U } }));
    ensure("a module without an entry point is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ entryPoint(EXECUTION_MODEL_VERTEX, "else") }));
    ensure("a renamed entry point is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ entryPoint(EXECUTION_MODEL_VERTEX, "main", false) }));
    ensure("an unterminated entry-point name is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));

    auto nonzero_padding = entryPoint(EXECUTION_MODEL_VERTEX);
    nonzero_padding.back() |= 0x00000100U;
    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ nonzero_padding }));
    ensure("nonzero bytes after the entry-point terminator are rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ { (3U << 16U) | OP_ENTRY_POINT, EXECUTION_MODEL_VERTEX, 1U } }));
    ensure("an entry-point instruction without a name is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<9>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    const auto vertex_entry = entryPoint(EXECUTION_MODEL_VERTEX);
    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ vertex_entry, vertex_entry }));
    ensure("duplicate main entry points are rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, moduleWithInstructions({ vertex_entry, entryPoint(EXECUTION_MODEL_VERTEX, "else") }));
    ensure("an extra named entry point is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongEntryPoint, ShaderStage::Vertex));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<10>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Fragment));
    writeWords(root / FRAGMENT_RELATIVE, validModule(ShaderStage::Vertex));
    ensure("swapped execution models are rejected at the first module",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongStage, ShaderStage::Vertex));

    writeWords(root / VERTEX_RELATIVE, validModule(ShaderStage::Vertex));
    ensure("a fragment module carrying the vertex execution model is rejected",
           hasError(loadLegacyNormSpecProductionArtifacts(root), ShaderArtifactLoadCode::WrongStage, ShaderStage::Fragment));
}

template<>
template<>
void vulkan_material_artifact_test_object::test<11>()
{
    TemporaryDirectory          temporary;
    const std::filesystem::path root = temporary.path() / "app_settings";
    writeValidPair(root);

    ShaderArtifactLoadResult result = loadLegacyNormSpecProductionArtifacts(root);
    const auto*              loaded = std::get_if<LoadedShaderProgram>(&result);
    ensure("the ownership fixture loads", loaded != nullptr);
    const std::vector<std::uint32_t> vertex_words   = loaded->mVertex.mWords;
    const std::vector<std::uint32_t> fragment_words = loaded->mFragment.mWords;

    writeWords(root / VERTEX_RELATIVE, { 0U, 0U, 0U, 0U, 0U });
    writeWords(root / FRAGMENT_RELATIVE, { 0U, 0U, 0U, 0U, 0U });
    std::error_code error;
    std::filesystem::remove_all(root, error);
    ensure("the loaded program remains owned after source mutation and deletion",
           !error && loaded->mVertex.mWords == vertex_words && loaded->mFragment.mWords == fragment_words &&
               loaded->mVertex.mStage == ShaderStage::Vertex && loaded->mFragment.mStage == ShaderStage::Fragment);
}

} // namespace tut
