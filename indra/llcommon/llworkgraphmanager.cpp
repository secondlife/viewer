/**
 * @file llworkgraphmanager.cpp
 * @brief Implementation of LLWorkGraphManager
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

#include "linden_common.h"
#include "llworkgraphmanager.h"

LLWorkGraphManager::LLWorkGraphManager()
{
}

void LLWorkGraphManager::addGraph(std::shared_ptr<LLWorkGraph> graph)
{
    if (!graph)
    {
        LL_WARNS("WorkGraph") << "Attempted to add null work graph" << LL_ENDL;
        return;
    }

    std::unique_lock<std::shared_mutex> lock(mGraphMutex);
    mActiveGraphs.push_back(graph);
    LL_DEBUGS("WorkGraph") << "Added work graph, active count: " << mActiveGraphs.size() << LL_ENDL;
}

size_t LLWorkGraphManager::garbageCollect()
{
    std::unique_lock<std::shared_mutex> lock(mGraphMutex);

    size_t before = mActiveGraphs.size();

    // Remove completed graphs
    mActiveGraphs.erase(
        std::remove_if(mActiveGraphs.begin(), mActiveGraphs.end(),
            [](const auto& graph) { return graph->isComplete(); }),
        mActiveGraphs.end()
    );

    size_t after = mActiveGraphs.size();
    size_t collected = before - after;

    if (collected > 0)
    {
        LL_DEBUGS("WorkGraph") << "Garbage collected " << collected << " graphs, "
                                << after << " remaining active" << LL_ENDL;
    }

    return collected;
}

size_t LLWorkGraphManager::getActiveGraphCount() const
{
    std::shared_lock<std::shared_mutex> lock(mGraphMutex);
    return mActiveGraphs.size();
}

// Global work graph manager instance
LLWorkGraphManager gWorkGraphManager;
