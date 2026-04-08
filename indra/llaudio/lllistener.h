/**
 * @file listener.h
 * @brief Description of LISTENER base class abstracting the audio support.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
#include "glm/vec3.hpp"

class LLListener
{
 private:
 protected:
    glm::vec3 mPosition;
    glm::vec3 mVelocity;
    glm::vec3 mListenAt;
    glm::vec3 mListenUp;

 public:

 private:
 protected:
 public:
    LLListener();
    virtual ~LLListener();
    virtual void init();

    virtual void set(glm::vec3 pos, glm::vec3 vel, glm::vec3 up, glm::vec3 at);

    virtual void setPosition(glm::vec3 pos);
    virtual void setVelocity(glm::vec3 vel);

    virtual void orient(glm::vec3 up, glm::vec3 at);
    virtual void translate(glm::vec3 offset);

    virtual void setDopplerFactor(F32 factor);
    virtual void setRolloffFactor(F32 factor);

    virtual glm::vec3 getPosition();
    virtual glm::vec3 getAt();
    virtual glm::vec3 getUp();

    virtual F32 getDopplerFactor();
    virtual F32 getRolloffFactor();

    virtual void commitDeferredChanges();
};


