/** 
 * @file lljoint.cpp
 * @brief Implementation of LLJoint class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "lljoint.h"

#include "llmath.h"
#include "llcallstack.h"
#include <boost/algorithm/string.hpp>

S32 LLJoint::sNumUpdates = 0;
S32 LLJoint::sNumTouches = 0;

template <class T> 
bool attachment_map_iter_compare_key(const T& a, const T& b)
{
	return a.first < b.first;
}

bool LLVector3OverrideMap::findActiveOverride(LLUUID& mesh_id, LLVector3& pos) const
{
	pos = LLVector3(0,0,0);
	mesh_id = LLUUID();
	bool found = false;
	
	map_type::const_iterator it = std::max_element(m_map.begin(),
												   m_map.end(),
												   attachment_map_iter_compare_key<map_type::value_type>);
	if (it != m_map.end())
	{
		found = true;
		pos = it->second;
		mesh_id = it->first;
	}
	return found;
}

void LLVector3OverrideMap::showJointVector3Overrides( std::ostringstream& os ) const
{
	map_type::const_iterator max_it = std::max_element(m_map.begin(),
													   m_map.end(),
													   attachment_map_iter_compare_key<map_type::value_type>);
	for (map_type::const_iterator it = m_map.begin();
		 it != m_map.end(); ++it)
	{
		const LLVector3& pos = it->second;
		os << " " << "[" << it->first <<": " << pos << "]" << ((it==max_it) ? "*" : "");
	}
}

U32 LLVector3OverrideMap::count() const
{
	return m_map.size();
}

void LLVector3OverrideMap::add(const LLUUID& mesh_id, const LLVector3& pos)
{
	m_map[mesh_id] = pos;
}

bool LLVector3OverrideMap::remove(const LLUUID& mesh_id)
{
	U32 remove_count = m_map.erase(mesh_id);
	return (remove_count > 0);
}

void LLVector3OverrideMap::clear()
{
	m_map.clear();
}

//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------


void LLJoint::init()
{
	mName = "unnamed";
	mParent = NULL;
	mXform.setScaleChildOffset(TRUE);
	mXform.setScale(LLVector3(1.0f, 1.0f, 1.0f));
	mDirtyFlags = MATRIX_DIRTY | ROTATION_DIRTY | POSITION_DIRTY;
	mUpdateXform = TRUE;
    mSupport = SUPPORT_BASE;
    mEnd = LLVector3(0.0f, 0.0f, 0.0f);
}

LLJoint::LLJoint() :
	mJointNum(-1)
{
	init();
	touch();
}

LLJoint::LLJoint(S32 joint_num) :
	mJointNum(joint_num)
{
	init();
	touch();
}

//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLJoint::LLJoint(const std::string &name, LLJoint *parent) :
	mJointNum(-2)
{
	init();
	mUpdateXform = FALSE;

	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
	touch();
}

//-----------------------------------------------------------------------------
// ~LLJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLJoint::~LLJoint()
{
	if (mParent)
	{
		mParent->removeChild( this );
	}
	removeAllChildren();
}


//-----------------------------------------------------------------------------
// setup()
//-----------------------------------------------------------------------------
void LLJoint::setup(const std::string &name, LLJoint *parent)
{
	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
}

//-----------------------------------------------------------------------------
// setSupport()
//-----------------------------------------------------------------------------
void LLJoint::setSupport(const std::string& support_name)
{
    if (support_name == "extended")
    {
        setSupport(SUPPORT_EXTENDED);
    }
    else if (support_name == "base")
    {
        setSupport(SUPPORT_BASE);
    }
    else
    {
        LL_WARNS() << "unknown support string " << support_name << LL_ENDL;
        setSupport(SUPPORT_BASE);
    }
}
    

//-----------------------------------------------------------------------------
// touch()
// Sets all dirty flags for all children, recursively.
//-----------------------------------------------------------------------------
void LLJoint::touch(U32 flags)
{
	if ((flags | mDirtyFlags) != mDirtyFlags)
	{
		sNumTouches++;
		mDirtyFlags |= flags;
		U32 child_flags = flags;
		if (flags & ROTATION_DIRTY)
		{
			child_flags |= POSITION_DIRTY;
		}

		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLJoint* joint = *iter;
			joint->touch(child_flags);
		}
	}
}

//-----------------------------------------------------------------------------
// setJointNum()
//-----------------------------------------------------------------------------
void LLJoint::setJointNum(S32 joint_num)
{
    mJointNum = joint_num;
    if (mJointNum + 2 >= LL_CHARACTER_MAX_ANIMATED_JOINTS)
    {
        LL_INFOS() << "LL_CHARACTER_MAX_ANIMATED_JOINTS needs to be increased" << LL_ENDL;
        LL_ERRS() << "joint_num " << joint_num << " + 2 is too large for " << LL_CHARACTER_MAX_ANIMATED_JOINTS << LL_ENDL;
    }
}
//-----------------------------------------------------------------------------
// getRoot()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::getRoot()
{
	if ( getParent() == NULL )
	{
		return this;
	}
	return getParent()->getRoot();
}


//-----------------------------------------------------------------------------
// findJoint()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::findJoint( const std::string &name )
{
	if (name == getName())
		return this;

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		LLJoint *found = joint->findJoint(name);
		if (found)
		{
			return found;
		}
	}

	return NULL;	
}


//--------------------------------------------------------------------
// addChild()
//--------------------------------------------------------------------
void LLJoint::addChild(LLJoint* joint)
{
	if (joint->mParent)
		joint->mParent->removeChild(joint);

	mChildren.push_back(joint);
	joint->mXform.setParent(&mXform);
	joint->mParent = this;	
	joint->touch();
}


//--------------------------------------------------------------------
// removeChild()
//--------------------------------------------------------------------
void LLJoint::removeChild(LLJoint* joint)
{
	child_list_t::iterator iter = std::find(mChildren.begin(), mChildren.end(), joint);
	if (iter != mChildren.end())
	{
		mChildren.erase(iter);
	
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// removeAllChildren()
//--------------------------------------------------------------------
void LLJoint::removeAllChildren()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end();)
	{
		child_list_t::iterator curiter = iter++;
		LLJoint* joint = *curiter;
		mChildren.erase(curiter);
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// getPosition()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getPosition()
{
	return mXform.getPosition();
}

bool do_debug_joint(const std::string& name)
{
    if (std::find(LLJoint::s_debugJointNames.begin(), LLJoint::s_debugJointNames.end(),name) != LLJoint::s_debugJointNames.end())
    {
        return true;
    }
    return false;
}

//--------------------------------------------------------------------
// setPosition()
//--------------------------------------------------------------------
void LLJoint::setPosition( const LLVector3& requested_pos, bool apply_attachment_overrides )
{
    LLVector3 pos(requested_pos);

    LLVector3 active_override;
    LLUUID mesh_id;
    if (apply_attachment_overrides && m_attachmentPosOverrides.findActiveOverride(mesh_id,active_override))
    {  
        if (pos != active_override && do_debug_joint(getName()))
        {
            LLScopedContextString str("setPosition");
            LL_DEBUGS("Avatar") << " joint " << getName() << " requested_pos " << requested_pos
                                << " overriden by attachment " << active_override << LL_ENDL;
        }
        pos = active_override;
    }
	if ((pos != getPosition()) && do_debug_joint(getName()))
	{
        LLScopedContextString str("setPosition");
        LLCallStack cs;
        LLContextStatus con_status;
        LL_DEBUGS("Avatar") << " joint " << getName() << " set pos " << pos << LL_ENDL;
        LL_DEBUGS("Avatar") << "CONTEXT:\n" << "====================\n" << con_status << "====================" << LL_ENDL;
        LL_DEBUGS("Avatar") << "STACK:\n" << "====================\n" << cs << "====================" << LL_ENDL;
	}
    if (pos != getPosition())
    {
        mXform.setPosition(pos);
        touch(MATRIX_DIRTY | POSITION_DIRTY);
    }
}

void LLJoint::setDefaultPosition( const LLVector3& pos )
{
    mDefaultPosition = pos;
}

const LLVector3& LLJoint::getDefaultPosition() const
{
    return mDefaultPosition;
}

void LLJoint::setDefaultScale( const LLVector3& scale )
{
    mDefaultScale = scale;
}

const LLVector3& LLJoint::getDefaultScale() const
{
    return mDefaultScale;
}

void showJointPosOverrides( const LLJoint& joint, const std::string& note, const std::string& av_info )
{
        std::ostringstream os;
        os << joint.m_posBeforeOverrides;
        joint.m_attachmentPosOverrides.showJointVector3Overrides(os);
        LL_DEBUGS("Avatar") << av_info << " joint " << joint.getName() << " " << note << " " << os.str() << LL_ENDL;
}

void showJointScaleOverrides( const LLJoint& joint, const std::string& note, const std::string& av_info )
{
        std::ostringstream os;
        os << joint.m_scaleBeforeOverrides;
        joint.m_attachmentScaleOverrides.showJointVector3Overrides(os);
        LL_DEBUGS("Avatar") << av_info << " joint " << joint.getName() << " " << note << " " << os.str() << LL_ENDL;
}

bool LLJoint::aboveJointPosThreshold(const LLVector3& pos) const
{
    LLVector3 diff = pos - getDefaultPosition();
	const F32 max_joint_pos_offset = 0.0001f; // 0.1 mm
	return diff.lengthSquared() > max_joint_pos_offset * max_joint_pos_offset;
}

bool LLJoint::aboveJointScaleThreshold(const LLVector3& scale) const
{
    LLVector3 diff = scale - getDefaultScale();
	const F32 max_joint_scale_offset = 0.0001f; // 0.1 mm
	return diff.lengthSquared() > max_joint_scale_offset * max_joint_scale_offset;
}

//--------------------------------------------------------------------
// addAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::addAttachmentPosOverride( const LLVector3& pos, const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed )
{
    active_override_changed = false;
	if (mesh_id.isNull())
	{
		return;
	}
    // BENTO
    // Not clear pelvis overrides are meaningful/useful.
    //if (mName == "mPelvis")
    //{
    //    return;
    //}

    LLVector3 before_pos;
    LLUUID before_mesh_id;
    bool has_active_override_before = hasAttachmentPosOverride( before_pos, before_mesh_id );
	if (!m_attachmentPosOverrides.count())
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " saving m_posBeforeOverrides " << getPosition() << LL_ENDL;
		}
		m_posBeforeOverrides = getPosition();
	}
	m_attachmentPosOverrides.add(mesh_id,pos);
    LLVector3 after_pos;
    LLUUID after_mesh_id;
    hasAttachmentPosOverride(after_pos, after_mesh_id);
    if (!has_active_override_before || (after_pos != before_pos))
    {
        active_override_changed = true; 
        if (do_debug_joint(getName()))
        {
            LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " addAttachmentPosOverride for mesh " << mesh_id << " pos " << pos << LL_ENDL;
        }
        updatePos(av_info);
    }
}

//--------------------------------------------------------------------
// removeAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::removeAttachmentPosOverride( const LLUUID& mesh_id, const std::string& av_info, bool& active_override_changed )
{
    active_override_changed = false;
	if (mesh_id.isNull())
	{
		return;
	}
    LLVector3 before_pos;
    LLUUID before_mesh_id;
    hasAttachmentPosOverride( before_pos, before_mesh_id );
	if (m_attachmentPosOverrides.remove(mesh_id))
	{
        LLVector3 after_pos;
        LLUUID after_mesh_id;
        bool has_active_override_after = hasAttachmentPosOverride(after_pos, after_mesh_id);
        if (!has_active_override_after || (after_pos != before_pos))
        {
            active_override_changed = true;
            if (do_debug_joint(getName()))
            {
                LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName()
                                    << " removeAttachmentPosOverride for " << mesh_id << LL_ENDL;
                showJointPosOverrides(*this, "remove", av_info);
            }
            updatePos(av_info);
        }
	}
}

//--------------------------------------------------------------------
 // hasAttachmentPosOverride()
 //--------------------------------------------------------------------
bool LLJoint::hasAttachmentPosOverride( LLVector3& pos, LLUUID& mesh_id ) const
{
	return m_attachmentPosOverrides.findActiveOverride(mesh_id,pos);
}

//--------------------------------------------------------------------
// clearAttachmentPosOverrides()
//--------------------------------------------------------------------
void LLJoint::clearAttachmentPosOverrides()
{
	if (m_attachmentPosOverrides.count())
	{
		m_attachmentPosOverrides.clear();
		setPosition(m_posBeforeOverrides);
	}
}

//--------------------------------------------------------------------
// getAllAttachmentPosOverrides()
//--------------------------------------------------------------------
void LLJoint::getAllAttachmentPosOverrides(S32& num_pos_overrides,
                                           std::set<LLVector3>& distinct_pos_overrides)
{
    num_pos_overrides = m_attachmentPosOverrides.count();
    LLVector3OverrideMap::map_type::const_iterator it = m_attachmentPosOverrides.getMap().begin();
    for (; it != m_attachmentPosOverrides.getMap().end(); ++it)
    {
        distinct_pos_overrides.insert(it->second);
    }
}
                                        
//--------------------------------------------------------------------
// getAllAttachmentScaleOverrides()
//--------------------------------------------------------------------
void LLJoint::getAllAttachmentScaleOverrides(S32& num_scale_overrides,
                                             std::set<LLVector3>& distinct_scale_overrides)
{
    num_scale_overrides = m_attachmentScaleOverrides.count();
    LLVector3OverrideMap::map_type::const_iterator it = m_attachmentScaleOverrides.getMap().begin();
    for (; it != m_attachmentScaleOverrides.getMap().end(); ++it)
    {
        distinct_scale_overrides.insert(it->second);
    }
}
                                        
//--------------------------------------------------------------------
// showAttachmentPosOverrides()
//--------------------------------------------------------------------
void LLJoint::showAttachmentPosOverrides(const std::string& av_info) const
{
    LLVector3 active_override;
    bool has_active_override;
    LLUUID mesh_id;
    has_active_override = m_attachmentPosOverrides.findActiveOverride(mesh_id,active_override);
    U32 count = m_attachmentPosOverrides.count();
    if (count==1)
    {
		LLVector3OverrideMap::map_type::const_iterator it = m_attachmentPosOverrides.getMap().begin();
        std::string highlight = (has_active_override && (it->second == active_override)) ? "*" : "";
        LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName()
                            << " has single attachment pos override " << highlight << "" << it->second << " default " << mDefaultPosition << LL_ENDL;
    }
    else if (count>1)
    {
        LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " has " << count << " attachment pos overrides" << LL_ENDL;
		std::set<LLVector3> distinct_offsets;
        LLVector3OverrideMap::map_type::const_iterator it = m_attachmentPosOverrides.getMap().begin();
        for (; it != m_attachmentPosOverrides.getMap().end(); ++it)
        {
            distinct_offsets.insert(it->second);
        }
        if (distinct_offsets.size()>1)
        {
            LL_DEBUGS("Avatar") << "CONFLICTS, " << distinct_offsets.size() << " different values" << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("Avatar") << "no conflicts" << LL_ENDL;
        }
        std::set<LLVector3>::iterator dit = distinct_offsets.begin();
        for ( ; dit != distinct_offsets.end(); ++dit)
        {
            std::string highlight = (has_active_override && *dit == active_override) ? "*" : "";
            LL_DEBUGS("Avatar") << "  POS " << highlight << "" << (*dit) << " default " << mDefaultPosition << LL_ENDL;
        }
	}
}

//--------------------------------------------------------------------
// updatePos()
//--------------------------------------------------------------------
void LLJoint::updatePos(const std::string& av_info)
{
	LLVector3 pos, found_pos;
	LLUUID mesh_id;
	if (m_attachmentPosOverrides.findActiveOverride(mesh_id,found_pos))
	{
        if (do_debug_joint(getName()))
        {
            LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updatePos, winner of " << m_attachmentPosOverrides.count() << " is mesh " << mesh_id << " pos " << found_pos << LL_ENDL;
        }
		pos = found_pos;
	}
	else
	{
        if (do_debug_joint(getName()))
        {
            LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updatePos, winner is posBeforeOverrides " << m_posBeforeOverrides << LL_ENDL;
        }
		pos = m_posBeforeOverrides;
	}
	setPosition(pos);
}

//--------------------------------------------------------------------
// updateScale()
//--------------------------------------------------------------------
void LLJoint::updateScale(const std::string& av_info)
{
	LLVector3 scale, found_scale;
	LLUUID mesh_id;
	if (m_attachmentScaleOverrides.findActiveOverride(mesh_id,found_scale))
	{
        if (do_debug_joint(getName()))
        {
            LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updateScale, winner of " << m_attachmentScaleOverrides.count() << " is mesh " << mesh_id << " scale " << found_scale << LL_ENDL;
        }
		scale = found_scale;
	}
	else
	{
        if (do_debug_joint(getName()))
        {
            LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " updateScale, winner is scaleBeforeOverrides " << m_scaleBeforeOverrides << LL_ENDL;
        }
		scale = m_scaleBeforeOverrides;
	}
	setScale(scale);
}

//--------------------------------------------------------------------
// addAttachmentScaleOverride()
//--------------------------------------------------------------------
void LLJoint::addAttachmentScaleOverride( const LLVector3& scale, const LLUUID& mesh_id, const std::string& av_info )
{
	if (mesh_id.isNull())
	{
		return;
	}
	if (!m_attachmentScaleOverrides.count())
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " saving m_scaleBeforeOverrides " << getScale() << LL_ENDL;
		}
		m_scaleBeforeOverrides = getScale();
	}
	m_attachmentScaleOverrides.add(mesh_id,scale);
	if (do_debug_joint(getName()))
	{
		LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " addAttachmentScaleOverride for mesh " << mesh_id << " scale " << scale << LL_ENDL;
	}
	updateScale(av_info);
}

//--------------------------------------------------------------------
// removeAttachmentScaleOverride()
//--------------------------------------------------------------------
void LLJoint::removeAttachmentScaleOverride( const LLUUID& mesh_id, const std::string& av_info )
{
	if (mesh_id.isNull())
	{
		return;
	}
	if (m_attachmentScaleOverrides.remove(mesh_id))
	{
		if (do_debug_joint(getName()))
		{
			LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName()
								<< " removeAttachmentScaleOverride for " << mesh_id << LL_ENDL;
			showJointScaleOverrides(*this, "remove", av_info);
		}
		updateScale(av_info);
	}
}

//--------------------------------------------------------------------
 // hasAttachmentScaleOverride()
 //--------------------------------------------------------------------
bool LLJoint::hasAttachmentScaleOverride( LLVector3& scale, LLUUID& mesh_id ) const
{
	return m_attachmentScaleOverrides.findActiveOverride(mesh_id,scale);
}

//--------------------------------------------------------------------
// clearAttachmentScaleOverrides()
//--------------------------------------------------------------------
void LLJoint::clearAttachmentScaleOverrides()
{
	if (m_attachmentScaleOverrides.count())
	{
		m_attachmentScaleOverrides.clear();
		setScale(m_scaleBeforeOverrides);
	}
}

//--------------------------------------------------------------------
// showAttachmentScaleOverrides()
//--------------------------------------------------------------------
void LLJoint::showAttachmentScaleOverrides(const std::string& av_info) const
{
    LLVector3 active_override;
    bool has_active_override;
    LLUUID mesh_id;
    has_active_override = m_attachmentScaleOverrides.findActiveOverride(mesh_id,active_override);
    U32 count = m_attachmentScaleOverrides.count();
    if (count==1)
    {
		LLVector3OverrideMap::map_type::const_iterator it = m_attachmentScaleOverrides.getMap().begin();
        std::string highlight = (has_active_override && (it->second == active_override)) ? "*" : "";
        LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName()
                            << " has single attachment scale override " << highlight << "" << it->second << " default " << mDefaultScale << LL_ENDL;
    }
    else if (count>1)
    {
        LL_DEBUGS("Avatar") << "av " << av_info << " joint " << getName() << " has " << count << " attachment scale overrides" << LL_ENDL;
		std::set<LLVector3> distinct_offsets;
        LLVector3OverrideMap::map_type::const_iterator it = m_attachmentScaleOverrides.getMap().begin();
        for (; it != m_attachmentScaleOverrides.getMap().end(); ++it)
        {
            distinct_offsets.insert(it->second);
        }
        if (distinct_offsets.size()>1)
        {
            LL_DEBUGS("Avatar") << "CONFLICTS, " << distinct_offsets.size() << " different values" << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("Avatar") << "no conflicts" << LL_ENDL;
        }
        std::set<LLVector3>::iterator dit = distinct_offsets.begin();
        for ( ; dit != distinct_offsets.end(); ++dit)
        {
            std::string highlight = (has_active_override && *dit == active_override) ? "*" : "";
            LL_DEBUGS("Avatar") << "  POS " << highlight << "" << (*dit) << " default " << mDefaultScale << LL_ENDL;
        }
	}
}

// init static
LLJoint::debug_joint_name_t LLJoint::s_debugJointNames = debug_joint_name_t();

//--------------------------------------------------------------------
// setDebugJointNames
//--------------------------------------------------------------------
void LLJoint::setDebugJointNames(const debug_joint_name_t& names)
{
    s_debugJointNames = names;
}
void LLJoint::setDebugJointNames(const std::string& names_string)
{
    debug_joint_name_t names;
    boost::split(names, names_string, boost::is_any_of(" :,"));
    setDebugJointNames(names);
}

//--------------------------------------------------------------------
// getWorldPosition()
//--------------------------------------------------------------------
LLVector3 LLJoint::getWorldPosition()
{
	updateWorldPRSParent();
	return mXform.getWorldPosition();
}

//-----------------------------------------------------------------------------
// getLastWorldPosition()
//-----------------------------------------------------------------------------
LLVector3 LLJoint::getLastWorldPosition()
{
	return mXform.getWorldPosition();
}
//--------------------------------------------------------------------
// setWorldPosition()
//--------------------------------------------------------------------
void LLJoint::setWorldPosition( const LLVector3& pos )
{
	if (mParent == NULL)
	{
		this->setPosition( pos );
		return;
	}

	LLMatrix4 temp_matrix = getWorldMatrix();
	temp_matrix.mMatrix[VW][VX] = pos.mV[VX];
	temp_matrix.mMatrix[VW][VY] = pos.mV[VY];
	temp_matrix.mMatrix[VW][VZ] = pos.mV[VZ];

	LLMatrix4 parentWorldMatrix = mParent->getWorldMatrix();
	LLMatrix4 invParentWorldMatrix = parentWorldMatrix.invert();

	temp_matrix *= invParentWorldMatrix;

	LLVector3 localPos(	temp_matrix.mMatrix[VW][VX],
						temp_matrix.mMatrix[VW][VY],
						temp_matrix.mMatrix[VW][VZ] );

	setPosition( localPos );
}


//--------------------------------------------------------------------
// getRotation()
//--------------------------------------------------------------------
const LLQuaternion& LLJoint::getRotation()
{
	return mXform.getRotation();
}


//--------------------------------------------------------------------
// setRotation()
//--------------------------------------------------------------------
void LLJoint::setRotation( const LLQuaternion& rot )
{
	if (rot.isFinite())
	{
	//	if (mXform.getRotation() != rot)
		{
			mXform.setRotation(rot);
			touch(MATRIX_DIRTY | ROTATION_DIRTY);
		}
	}
}


//--------------------------------------------------------------------
// getWorldRotation()
//--------------------------------------------------------------------
LLQuaternion LLJoint::getWorldRotation()
{
	updateWorldPRSParent();

	return mXform.getWorldRotation();
}

//-----------------------------------------------------------------------------
// getLastWorldRotation()
//-----------------------------------------------------------------------------
LLQuaternion LLJoint::getLastWorldRotation()
{
	return mXform.getWorldRotation();
}

//--------------------------------------------------------------------
// setWorldRotation()
//--------------------------------------------------------------------
void LLJoint::setWorldRotation( const LLQuaternion& rot )
{
	if (mParent == NULL)
	{
		this->setRotation( rot );
		return;
	}

	LLMatrix4 temp_mat(rot);

	LLMatrix4 parentWorldMatrix = mParent->getWorldMatrix();
	parentWorldMatrix.mMatrix[VW][VX] = 0;
	parentWorldMatrix.mMatrix[VW][VY] = 0;
	parentWorldMatrix.mMatrix[VW][VZ] = 0;

	LLMatrix4 invParentWorldMatrix = parentWorldMatrix.invert();

	temp_mat *= invParentWorldMatrix;

	setRotation(LLQuaternion(temp_mat));
}


//--------------------------------------------------------------------
// getScale()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getScale()
{
	return mXform.getScale();	
}

//--------------------------------------------------------------------
// setScale()
//--------------------------------------------------------------------
void LLJoint::setScale( const LLVector3& requested_scale, bool apply_attachment_overrides )
{
    LLVector3 scale(requested_scale);
    LLUUID mesh_id;
    LLVector3 active_override;
    if (apply_attachment_overrides && m_attachmentScaleOverrides.findActiveOverride(mesh_id,active_override))
    {  
        if (scale != active_override && do_debug_joint(getName()))
        {
            LLScopedContextString str("setScale");
            LL_DEBUGS("Avatar") << " joint " << getName() << " requested_scale " << requested_scale
                                << " overriden by attachment " << active_override << LL_ENDL;
        }
        scale = active_override;
    }
	if ((mXform.getScale() != scale) && do_debug_joint(getName()))
	{	
        LLScopedContextString str("setScale");
        LLCallStack cs;
        LLContextStatus con_status;
        LL_DEBUGS("Avatar") << " joint " << getName() << " set scale " << scale << LL_ENDL;
        LL_DEBUGS("Avatar") << "CONTEXT:\n" << "====================\n" << con_status << LL_ENDL;
        LL_DEBUGS("Avatar") << "STACK:\n" << "====================\n" << cs << "====================" << LL_ENDL;
	}
    mXform.setScale(scale);
    touch();

}



//--------------------------------------------------------------------
// getWorldMatrix()
//--------------------------------------------------------------------
const LLMatrix4 &LLJoint::getWorldMatrix()
{
	updateWorldMatrixParent();

	return mXform.getWorldMatrix();
}


//--------------------------------------------------------------------
// setWorldMatrix()
//--------------------------------------------------------------------
void LLJoint::setWorldMatrix( const LLMatrix4& mat )
{
	LL_INFOS() << "WARNING: LLJoint::setWorldMatrix() not correctly implemented yet" << LL_ENDL;
	// extract global translation
	LLVector3 trans(	mat.mMatrix[VW][VX],
						mat.mMatrix[VW][VY],
						mat.mMatrix[VW][VZ] );

	// extract global rotation
	LLQuaternion rot( mat );

	setWorldPosition( trans );
	setWorldRotation( rot );
}

//-----------------------------------------------------------------------------
// updateWorldMatrixParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixParent()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldMatrixParent();
		}
		updateWorldMatrix();
	}
}

//-----------------------------------------------------------------------------
// updateWorldPRSParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldPRSParent()
{
	if (mDirtyFlags & (ROTATION_DIRTY | POSITION_DIRTY))
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldPRSParent();
		}

		mXform.update();
		mDirtyFlags &= ~(ROTATION_DIRTY | POSITION_DIRTY);
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrixChildren()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixChildren()
{	
	if (!this->mUpdateXform) return;

	if (mDirtyFlags & MATRIX_DIRTY)
	{
		updateWorldMatrix();
	}
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		joint->updateWorldMatrixChildren();
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrix()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrix()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		sNumUpdates++;
		mXform.updateMatrix(FALSE);
		mDirtyFlags = 0x0;
	}
}

//--------------------------------------------------------------------
// getSkinOffset()
//--------------------------------------------------------------------
const LLVector3 &LLJoint::getSkinOffset()
{
	return mSkinOffset;
}


//--------------------------------------------------------------------
// setSkinOffset()
//--------------------------------------------------------------------
void LLJoint::setSkinOffset( const LLVector3& offset )
{
	mSkinOffset = offset;
}


//-----------------------------------------------------------------------------
// clampRotation()
//-----------------------------------------------------------------------------
void LLJoint::clampRotation(LLQuaternion old_rot, LLQuaternion new_rot)
{
	LLVector3 main_axis(1.f, 0.f, 0.f);

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		if (joint->isAnimatable())
		{
			main_axis = joint->getPosition();
			main_axis.normVec();
			// only care about first animatable child
			break;
		}
	}
}

// End

