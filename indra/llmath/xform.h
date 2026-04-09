/**
 * @file xform.h
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

#pragma once

#include "v3math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"

constexpr F32 MAX_OBJECT_Z      = 4096.f; // should match REGION_HEIGHT_METERS, Pre-havok4: 768.f
constexpr F32 MIN_OBJECT_Z      = -256.f;
constexpr F32 DEFAULT_MAX_PRIM_SCALE = 64.f;
constexpr F32 DEFAULT_MAX_PRIM_SCALE_NO_MESH = 10.f;
constexpr F32 MIN_PRIM_SCALE = 0.01f;
constexpr F32 MAX_PRIM_SCALE = 65536.f; // something very high but not near FLT_MAX

class LLXform
{
protected:
    glm::vec3     mPosition;
    glm::quat     mRotation{1.f, 0.f, 0.f, 0.f};   // identity (w, x, y, z)
    glm::vec3     mScale;

    //RN: TODO: move these world transform members to LLXformMatrix
    // as they are *never* updated or accessed in the base class
    glm::vec3     mWorldPosition;
    glm::quat     mWorldRotation{1.f, 0.f, 0.f, 0.f};   // identity (w, x, y, z)

    LLXform*      mParent;
    U32           mChanged;

    bool          mScaleChildOffset;

public:
    enum e_changed_flags
    {
        UNCHANGED   = 0x00,
        TRANSLATED  = 0x01,
        ROTATED     = 0x02,
        SCALED      = 0x04,
        SHIFTED     = 0x08,
        GEOMETRY    = 0x10,
        TEXTURE     = 0x20,
        MOVED       = TRANSLATED|ROTATED|SCALED,
        SILHOUETTE  = 0x40,
        ALL_CHANGED = 0x7f
    };

    void init()
    {
        mParent  = NULL;
        mChanged = UNCHANGED;
        mPosition = glm::vec3(0.f);
        mRotation = glm::quat(1.f, 0.f, 0.f, 0.f);   // identity
        mScale = glm::vec3(1.f);
        mWorldPosition = glm::vec3(0.f);
        mWorldRotation = glm::quat(1.f, 0.f, 0.f, 0.f);   // identity
        mScaleChildOffset = false;
    }

     LLXform();
    virtual ~LLXform();

    void getLocalMat4(LLMatrix4 &mat) const { mat.initAll(mScale, mRotation, mPosition); }

    inline bool setParent(LLXform *parent);

    inline void setPosition(const glm::vec3& pos);
    inline void setPosition(const F32 x, const F32 y, const F32 z);
    inline void setPositionX(const F32 x);
    inline void setPositionY(const F32 y);
    inline void setPositionZ(const F32 z);
    inline void addPosition(const glm::vec3& pos);


    inline void setScale(const glm::vec3& scale);
    inline void setScale(const F32 x, const F32 y, const F32 z);
    inline void setRotation(const glm::quat& rot);
    inline void setRotation(const F32 x, const F32 y, const F32 z);
    inline void setRotation(const F32 x, const F32 y, const F32 z, const F32 s);

    // Above functions must be inline for speed, but also
    // need to emit warnings.  LL_WARNS() causes inline LLError::CallSite
    // static objects that make more work for the linker.
    // Avoid inline LL_WARNS() by calling this function.
    void warn(const char* const msg);

    void        setChanged(const U32 bits)                  { mChanged |= bits; }
    bool        isChanged() const                           { return mChanged; }
    bool        isChanged(const U32 bits) const             { return mChanged & bits; }
    void        clearChanged()                              { mChanged = 0; }
    void        clearChanged(U32 bits)                      { mChanged &= ~bits; }

    void        setScaleChildOffset(bool scale)             { mScaleChildOffset = scale; }
    bool        getScaleChildOffset() const                 { return mScaleChildOffset; }

    LLXform* getParent() const { return mParent; }
    LLXform* getRoot() const;
    virtual bool isRoot() const;
    virtual bool isRootEdit() const;

    const glm::vec3&    getPosition()  const        { return mPosition; }
    const glm::vec3&    getScale() const            { return mScale; }
    const glm::quat&    getRotation() const         { return mRotation; }
    const glm::vec3&    getPositionW() const        { return mWorldPosition; }
    const glm::quat&    getWorldRotation() const    { return mWorldRotation; }
    const glm::vec3&    getWorldPosition() const    { return mWorldPosition; }
};

class LLXformMatrix : public LLXform
{
public:
    LLXformMatrix() : LLXform() {};
    virtual ~LLXformMatrix();

    const LLMatrix4&    getWorldMatrix() const      { return mWorldMatrix; }
    void setWorldMatrix (const LLMatrix4& mat)   { mWorldMatrix = mat; }

    void init()
    {
        mWorldMatrix.setIdentity();
        mMin = glm::vec3(0.f);
        mMax = glm::vec3(0.f);

        LLXform::init();
    }

    void update();
    void updateMatrix(bool update_bounds = true);
    void getMinMax(glm::vec3& min, glm::vec3& max) const;

protected:
    LLMatrix4   mWorldMatrix;
    glm::vec3   mMin;
    glm::vec3   mMax;

};

bool LLXform::setParent(LLXform* parent)
{
    // Validate and make sure we're not creating a loop
    if (parent == mParent)
    {
        return true;
    }
    if (parent)
    {
        LLXform *cur_par = parent->mParent;
        while (cur_par)
        {
            if (cur_par == this)
            {
                //warn("LLXform::setParent Creating loop when setting parent!");
                return false;
            }
            cur_par = cur_par->mParent;
        }
    }
    mParent = parent;
    return true;
}

void LLXform::setPosition(const glm::vec3& pos)
{
    setChanged(TRANSLATED);
    if (llfinite(pos.x) && llfinite(pos.y) && llfinite(pos.z))
        mPosition = pos;
    else
    {
        mPosition = glm::vec3(0.f);
        warn("Non Finite in LLXform::setPosition(glm::vec3)");
    }
}

void LLXform::setPosition(const F32 x, const F32 y, const F32 z)
{
    setChanged(TRANSLATED);
    if (llfinite(x) && llfinite(y) && llfinite(z))
        mPosition = glm::vec3(x, y, z);
    else
    {
        mPosition = glm::vec3(0.f);
        warn("Non Finite in LLXform::setPosition(F32,F32,F32)");
    }
}

void LLXform::setPositionX(const F32 x)
{
    setChanged(TRANSLATED);
    if (llfinite(x))
        mPosition.x = x;
    else
    {
        mPosition.x = 0.f;
        warn("Non Finite in LLXform::setPositionX");
    }
}

void LLXform::setPositionY(const F32 y)
{
    setChanged(TRANSLATED);
    if (llfinite(y))
        mPosition.y = y;
    else
    {
        mPosition.y = 0.f;
        warn("Non Finite in LLXform::setPositionY");
    }
}

void LLXform::setPositionZ(const F32 z)
{
    setChanged(TRANSLATED);
    if (llfinite(z))
        mPosition.z = z;
    else
    {
        mPosition.z = 0.f;
        warn("Non Finite in LLXform::setPositionZ");
    }
}

void LLXform::addPosition(const glm::vec3& pos)
{
    setChanged(TRANSLATED);
    if (llfinite(pos.x) && llfinite(pos.y) && llfinite(pos.z))
        mPosition += pos;
    else
        warn("Non Finite in LLXform::addPosition");
}

void LLXform::setScale(const glm::vec3& scale)
{
    setChanged(SCALED);
    if (llfinite(scale.x) && llfinite(scale.y) && llfinite(scale.z))
        mScale = scale;
    else
    {
        mScale = glm::vec3(1.f);
        warn("Non Finite in LLXform::setScale");
    }
}
void LLXform::setScale(const F32 x, const F32 y, const F32 z)
{
    setChanged(SCALED);
    if (llfinite(x) && llfinite(y) && llfinite(z))
        mScale = glm::vec3(x, y, z);
    else
    {
        mScale = glm::vec3(1.f);
        warn("Non Finite in LLXform::setScale");
    }
}
void LLXform::setRotation(const glm::quat& rot)
{
    setChanged(ROTATED);
    if (std::isfinite(rot.x) && std::isfinite(rot.y) && std::isfinite(rot.z) && std::isfinite(rot.w))
        mRotation = rot;
    else
    {
        mRotation = glm::quat(1.f, 0.f, 0.f, 0.f);   // identity
        warn("Non Finite in LLXform::setRotation");
    }
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z)
{
    setChanged(ROTATED);
    if (llfinite(x) && llfinite(y) && llfinite(z))
    {
        // setEulerAngles is LLQuaternion-only and uses LL's specific
        // Euler convention (XYZ via LLMatrix3). Bridge through a
        // temporary LLQuaternion to preserve the exact convention,
        // then assign back to mRotation via the implicit operator
        // glm::quat() on LLQuaternion.
        LLQuaternion tmp;
        tmp.setEulerAngles(x, y, z);
        mRotation = tmp;
    }
    else
    {
        mRotation = glm::quat(1.f, 0.f, 0.f, 0.f);   // identity
        warn("Non Finite in LLXform::setRotation");
    }
}
void LLXform::setRotation(const F32 x, const F32 y, const F32 z, const F32 s)
{
    setChanged(ROTATED);
    if (llfinite(x) && llfinite(y) && llfinite(z) && llfinite(s))
    {
        // Direct field set: glm::quat exposes .x/.y/.z/.w members.
        // Note: glm::quat stores wxyz internally but member access is
        // .x/.y/.z/.w, so component identity matches LL's mQ[VX..VS]
        // bit-for-bit when we set them by name.
        mRotation.x = x; mRotation.y = y; mRotation.z = z; mRotation.w = s;
    }
    else
    {
        mRotation = glm::quat(1.f, 0.f, 0.f, 0.f);   // identity
        warn("Non Finite in LLXform::setRotation");
    }
}

