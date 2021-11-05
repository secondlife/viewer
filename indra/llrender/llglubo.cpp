/**
 * @file llglubo.cpp
 * @brief OpenGL Uniform Buffer Object
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

// Headers
    #include "linden_common.h"

    #include "llrender.h"
    #include "llglheaders.h"

    #include "llglubo.h"

// Implementation

// @param usage GL_STATIC_DRAW or GL_DYNAMIC_DRAW
bool LLUniformBufferObject::createUBO( size_t size, const char *nameBlock, GLhandleARB program, const GLuint usage )
{
    GLuint index = glGetUniformBlockIndexARB( program, nameBlock );
    if (index == GL_INVALID_INDEX)
        return false;

    muboIndex =  index;
    glGenBuffersARB ( 1,                          &muboBuffer );
    glBindBufferARB ( GL_UNIFORM_BUFFER,           muboBuffer );
    glBufferDataARB ( GL_UNIFORM_BUFFER, size, NULL, usage    ); // size, data, usage; Reserve size
    glBindBufferBase( GL_UNIFORM_BUFFER, muboBind, muboBuffer );

    return true;
}

void LLUniformBufferObject::update( size_t offset, size_t size, void *data )
{
    glBindBufferARB   ( GL_UNIFORM_BUFFER, muboBuffer );
    glBufferSubDataARB( GL_UNIFORM_BUFFER, offset, size, data );
}

void LLUniformBufferObject::deleteUBO()
{
    glDeleteBuffersARB( 1, &muboBuffer );
}
