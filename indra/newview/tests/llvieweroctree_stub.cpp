/**
 * @file llvieweroctree_stub.cpp
 * @brief  stub implementations to allow unit testing
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

S32 AABBSphereIntersect(const LLVector4a& min, const LLVector4a& max, const LLVector3 &origin, const F32 &rad) { return 0; }

void LLViewerOctreeCull::traverse(const LLOctreeNode<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry> >* node) { }
void LLViewerOctreeCull::visit(const LLOctreeNode<LLViewerOctreeEntry, LLPointer<LLViewerOctreeEntry> >* node) { }
void LLViewerOctreeCull::preprocess(LLViewerOctreeGroup* group) {}
bool LLViewerOctreeCull::earlyFail(LLViewerOctreeGroup* group) { return false; }
bool LLViewerOctreeCull::checkProjectionArea(const LLVector4a& center, const LLVector4a& size, const LLVector3& shift, F32 pixel_threshold, F32 near_radius) { return false; }
bool LLViewerOctreeCull::checkObjects(const OctreeNode* branch, const LLViewerOctreeGroup* group) { return false; }
void LLViewerOctreeCull::processGroup(LLViewerOctreeGroup* group) {}


bool LLViewerOctreeGroup::boundObjects(BOOL empty, LLVector4a& minOut, LLVector4a& maxOut) { return false; }
void LLViewerOctreeGroup::unbound() {}
void LLViewerOctreeGroup::rebound() {}
void LLViewerOctreeGroup::handleInsertion(const TreeNode* node, LLViewerOctreeEntry* obj) {}
void LLViewerOctreeGroup::handleRemoval(const TreeNode* node, LLViewerOctreeEntry* obj) {}
void LLViewerOctreeGroup::handleDestruction(const TreeNode* node) {}
void LLViewerOctreeGroup::handleStateChange(const TreeNode* node) {}
void LLViewerOctreeGroup::handleChildAddition(const OctreeNode* parent, OctreeNode* child) {}
void LLViewerOctreeGroup::handleChildRemoval(const OctreeNode* parent, const OctreeNode* child) {}

LLOcclusionCullingGroup::LLOcclusionCullingGroup(OctreeNode* node, LLViewerOctreePartition* part) :
    LLViewerOctreeGroup(node),
    mSpatialPartition(part)
{
}
LLOcclusionCullingGroup::~LLOcclusionCullingGroup() = default;
void LLOcclusionCullingGroup::doOcclusion(LLCamera* camera, const LLVector4a* shift) {}
void LLOcclusionCullingGroup::setOcclusionState(U32 state, S32 mode) {}
void LLOcclusionCullingGroup::clearOcclusionState(U32 state, S32 mode) {}
void LLOcclusionCullingGroup::handleChildAddition(const OctreeNode *parent, OctreeNode *child) {}
BOOL LLOcclusionCullingGroup::isRecentlyVisible() const { return FALSE; }
BOOL LLOcclusionCullingGroup::isAnyRecentlyVisible() const { return FALSE; }


LLViewerOctreeGroup::LLViewerOctreeGroup(OctreeNode* node) : mOctreeNode(node) {}
LLViewerOctreeGroup::~LLViewerOctreeGroup() = default;

LLViewerOctreeEntryData::LLViewerOctreeEntryData(LLViewerOctreeEntry::eEntryDataType_t) {}
LLViewerOctreeEntryData::~LLViewerOctreeEntryData() = default;
bool LLViewerOctreeEntryData::isVisible() const { return false; }
bool LLViewerOctreeEntryData::isRecentlyVisible() const { return false; }
void LLViewerOctreeEntryData::setVisible() const {}
void LLViewerOctreeEntryData::resetVisible() const {}
const LLVector4a& LLViewerOctreeEntryData::getPositionGroup() const { return LLVector4a::getZero(); }
const LLVector4a* LLViewerOctreeEntryData::getSpatialExtents() const { return nullptr; }
LLViewerOctreeGroup* LLViewerOctreeEntryData::getGroup() const { return nullptr; }
void LLViewerOctreeEntryData::setSpatialExtents(const LLVector4a& min, const LLVector4a& max) {}
void LLViewerOctreeEntryData::setPositionGroup(const LLVector4a& pos) {}
void LLViewerOctreeEntryData::setGroup(LLViewerOctreeGroup* group) {}
void LLViewerOctreeEntryData::setOctreeEntry(LLViewerOctreeEntry* entry) {}
U32 LLViewerOctreeEntryData::sCurVisible{};

LLViewerOctreePartition::LLViewerOctreePartition() = default;
LLViewerOctreePartition::~LLViewerOctreePartition() = default;
void LLViewerOctreePartition::cleanup() {}

BOOL LLViewerOctreeGroup::isRecentlyVisible() const { return FALSE; }


