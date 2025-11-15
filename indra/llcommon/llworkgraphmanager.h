/**
 * @file llworkgraphmanager.h
 * @brief Manager for work graph lifetime and garbage collection
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_WORKGRAPHMANAGER_H
#define LL_WORKGRAPHMANAGER_H

#include "llworkcontract.h"

#include <vector>
#include <memory>
#include <shared_mutex>

/**
 * @class LLWorkGraphManager
 * @brief Manages the lifetime of work graphs and performs garbage collection
 *
 * This manager centralizes work graph lifecycle management:
 * - Stores active graphs to keep them alive while executing
 * - Performs periodic garbage collection to remove completed graphs
 * - Called from llappviewer's main loop for cleanup
 */
class LL_COMMON_API LLWorkGraphManager
{
public:
    LLWorkGraphManager();

    /**
     * @brief Register a work graph to be managed
     *
     * This keeps the graph alive via shared_ptr ownership until it completes.
     * Should be called immediately after creating a work graph and before execute().
     *
     * @param graph The work graph to manage
     */
    void addGraph(std::shared_ptr<LLWorkGraph> graph);

    /**
     * @brief Perform garbage collection on completed graphs
     *
     * Removes graphs from the active list if they have completed execution.
     * Should be called regularly from the main application loop (llappviewer::doFrame).
     *
     * @return Number of graphs cleaned up
     */
    size_t garbageCollect();

    /**
     * @brief Get the count of currently active graphs
     *
     * @return Number of graphs being managed
     */
    size_t getActiveGraphCount() const;

private:
    // Container of active graphs - keeps them alive while executing
    std::vector<std::shared_ptr<LLWorkGraph>> mActiveGraphs;

    // Synchronization for thread-safe access to mActiveGraphs
    mutable std::shared_mutex mGraphMutex;
};

// Global work graph manager instance
extern LL_COMMON_API LLWorkGraphManager gWorkGraphManager;

#endif // LL_WORKGRAPHMANAGER_H
