/**
 * @file   llworkcontract.h
 * @author Jonathan "Geenz" Goodman
 * @date   2025-01-13
 * @brief  Type aliases for EntropyCore work contract primitives
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Copyright (c) 2025, Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLWORKCONTRACT_H
#define LL_LLWORKCONTRACT_H

#include <EntropyCore/Concurrency/WorkGraph.h>
#include <EntropyCore/Concurrency/WorkGraphTypes.h>
#include <EntropyCore/Concurrency/WorkContractGroup.h>
#include <EntropyCore/Concurrency/WorkService.h>

// Convenience type aliases for EntropyCore concurrency primitives
// These provide LL-prefixed names for viewer code
using LLWorkGraph = EntropyEngine::Core::Concurrency::WorkGraph;
using LLWorkGraphConfig = EntropyEngine::Core::Concurrency::WorkGraphConfig;
using LLWorkContractGroup = EntropyEngine::Core::Concurrency::WorkContractGroup;
using LLWorkContractHandle = EntropyEngine::Core::Concurrency::WorkContractHandle;
using LLWorkService = EntropyEngine::Core::Concurrency::WorkService;
using LLWorkResult = EntropyEngine::Core::Concurrency::WorkResult;
using LLExecutionType = EntropyEngine::Core::Concurrency::ExecutionType;

#endif // LL_LLWORKCONTRACT_H
