/**
 * @file llmaterialparametercontract.cpp
 * @brief Pure materialization of legacy material shader parameters.
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

#include "llmaterialparametercontract.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace LLRenderContract
{
namespace
{

    template<std::size_t Size>
    bool finite(const std::array<float, Size>& values) noexcept
    {
        return std::all_of(values.begin(), values.end(), [](float value) { return std::isfinite(value); });
    }

    DrawMatrix4 multiply(const DrawMatrix4& left, const DrawMatrix4& right) noexcept
    {
        DrawMatrix4 result{};
        for (std::size_t column = 0; column < 4; ++column)
        {
            for (std::size_t row = 0; row < 4; ++row)
            {
                result[column * 4 + row] = left[row] * right[column * 4] + left[4 + row] * right[column * 4 + 1] +
                                           left[8 + row] * right[column * 4 + 2] + left[12 + row] * right[column * 4 + 3];
            }
        }
        return result;
    }

    bool affine(const DrawMatrix4& matrix) noexcept
    {
        return matrix[3] == 0.f && matrix[7] == 0.f && matrix[11] == 0.f && matrix[15] == 1.f;
    }

    bool validLinearInverseResidual(const DrawMatrix4& matrix, const std::array<float, 9>& candidate) noexcept
    {
        // Any singular 3x3 product differs from identity by at least 1/3 in
        // one entry. This looser bound admits normal float inversion error
        // while remaining far below that singular boundary.
        constexpr double MAX_RESIDUAL = 1. / 1024.;

        for (std::size_t column = 0; column < 3; ++column)
        {
            for (std::size_t row = 0; row < 3; ++row)
            {
                double forward = 0.;
                double reverse = 0.;
                for (std::size_t offset = 0; offset < 3; ++offset)
                {
                    forward += static_cast<double>(matrix[offset * 4 + row]) * candidate[column * 3 + offset];
                    reverse += static_cast<double>(candidate[offset * 3 + row]) * matrix[column * 4 + offset];
                }

                const double expected = row == column ? 1. : 0.;
                if (!std::isfinite(forward) || !std::isfinite(reverse) || std::fabs(forward - expected) > MAX_RESIDUAL ||
                    std::fabs(reverse - expected) > MAX_RESIDUAL)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool inverseLinearPart(const DrawMatrix4& matrix, std::array<float, 9>& result) noexcept
    {
        std::array<std::array<double, 6>, 3> augmented{};
        std::array<double, 3>                row_scale{};
        for (std::size_t row = 0; row < 3; ++row)
        {
            for (std::size_t column = 0; column < 3; ++column)
            {
                augmented[row][column] = matrix[column * 4 + row];
                row_scale[row]         = std::max(row_scale[row], std::fabs(augmented[row][column]));
            }
            if (!std::isfinite(row_scale[row]) || row_scale[row] == 0.)
            {
                return false;
            }
            augmented[row][3 + row] = 1.;
        }

        const auto normalized = [&](std::size_t row, std::size_t column)
        {
            return augmented[row][column] / row_scale[row];
        };
        const double determinant = normalized(0, 0) * (normalized(1, 1) * normalized(2, 2) - normalized(1, 2) * normalized(2, 1)) -
                                   normalized(0, 1) * (normalized(1, 0) * normalized(2, 2) - normalized(1, 2) * normalized(2, 0)) +
                                   normalized(0, 2) * (normalized(1, 0) * normalized(2, 1) - normalized(1, 1) * normalized(2, 0));
        constexpr double MIN_NORMALIZED_DETERMINANT = 64. * std::numeric_limits<double>::epsilon();
        if (!std::isfinite(determinant) || std::fabs(determinant) <= MIN_NORMALIZED_DETERMINANT)
        {
            return false;
        }

        for (std::size_t column = 0; column < 3; ++column)
        {
            std::size_t pivot_row   = column;
            double      pivot_score = std::fabs(augmented[pivot_row][column]) / row_scale[pivot_row];
            for (std::size_t candidate = column + 1; candidate < 3; ++candidate)
            {
                const double candidate_score = std::fabs(augmented[candidate][column]) / row_scale[candidate];
                if (candidate_score > pivot_score)
                {
                    pivot_row   = candidate;
                    pivot_score = candidate_score;
                }
            }

            if (!std::isfinite(pivot_score) || pivot_score == 0.)
            {
                return false;
            }
            if (pivot_row != column)
            {
                std::swap(augmented[pivot_row], augmented[column]);
                std::swap(row_scale[pivot_row], row_scale[column]);
            }

            const double pivot = augmented[column][column];
            for (double& value : augmented[column])
            {
                value /= pivot;
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            for (std::size_t row = 0; row < 3; ++row)
            {
                if (row == column)
                {
                    continue;
                }

                const double scale = augmented[row][column];
                for (std::size_t offset = 0; offset < 6; ++offset)
                {
                    augmented[row][offset] -= scale * augmented[column][offset];
                    if (!std::isfinite(augmented[row][offset]))
                    {
                        return false;
                    }
                }
            }
        }

        for (std::size_t row = 0; row < 3; ++row)
        {
            for (std::size_t column = 0; column < 3; ++column)
            {
                const double value = augmented[row][3 + column];
                if (!std::isfinite(value) || std::fabs(value) > std::numeric_limits<float>::max())
                {
                    return false;
                }
                result[column * 3 + row] = static_cast<float>(value);
            }
        }
        // A rounded elimination residue can masquerade as a pivot for an
        // exactly singular float matrix. Require the returned float packet to
        // remain a two-sided inverse instead of relying on pivot magnitude.
        return finite(result) && validLinearInverseResidual(matrix, result);
    }

    bool productionProfile(const LegacyNormSpecPipelineKey& key) noexcept
    {
        return key.mTargetProfile == LegacyNormSpecTargetProfile::ModernHDR ||
               key.mTargetProfile == LegacyNormSpecTargetProfile::Compatibility;
    }

} // namespace

std::optional<MaterialParameters> materializeLegacyNormSpecWorldParameters(const LegacyNormSpecDrawPacket&            packet,
                                                                           const LegacyNormSpecWorldParameterContext& context) noexcept
{
    if (!validLegacyNormSpecDrawPacket(packet) || !productionProfile(packet.mPipelineKey) || context.mFrame == 0 ||
        !static_cast<bool>(context.mPass) || context.mFrame != packet.mFrame || context.mPass != packet.mPass ||
        !finite(context.mBaseModelview) || !finite(context.mProjection) || !finite(context.mEyeClipPlane))
    {
        return std::nullopt;
    }

    MaterialParameters parameters;
    parameters.mModelviewMatrix           = multiply(context.mBaseModelview, packet.mModelMatrix);
    parameters.mModelviewProjectionMatrix = multiply(context.mProjection, parameters.mModelviewMatrix);
    if (!finite(parameters.mModelviewMatrix) || !finite(parameters.mModelviewProjectionMatrix))
    {
        return std::nullopt;
    }

    std::array<float, 9> inverse_linear{};
    if (!affine(parameters.mModelviewMatrix) || !inverseLinearPart(parameters.mModelviewMatrix, inverse_linear))
    {
        return std::nullopt;
    }
    for (std::size_t column = 0; column < 3; ++column)
    {
        for (std::size_t row = 0; row < 3; ++row)
        {
            parameters.mNormalMatrix[column * 3 + row] = inverse_linear[row * 3 + column];
        }
    }

    parameters.mTextureMatrix0       = packet.mDiffuseTextureMatrix;
    parameters.mSpecularColor        = packet.mSpecularRGBA;
    parameters.mClipPlane            = context.mEyeClipPlane;
    parameters.mEnvironmentIntensity = packet.mEnvironmentIntensity;
    parameters.mEmissiveBrightness   = packet.mEmissiveBrightness;
    parameters.mMirror               = 0.f;

    if (!validMaterialParameters(parameters))
    {
        return std::nullopt;
    }
    return parameters;
}

} // namespace LLRenderContract
