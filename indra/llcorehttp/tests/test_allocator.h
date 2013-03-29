/** 
 * @file test_allocator.h
 * @brief quick and dirty allocator for tracking memory allocations
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef TEST_ALLOCATOR_H
#define TEST_ALLOCATOR_H

#include <cstdlib>
#include <new>

size_t GetMemTotal();
#if	defined(WIN32)
void * operator new(std::size_t size) _THROW1(std::bad_alloc);
void * operator new[](std::size_t size) _THROW1(std::bad_alloc);
void operator delete(void * p) _THROW0();
void operator delete[](void * p) _THROW0();
#else
void * operator new(std::size_t size) throw (std::bad_alloc);
void * operator new[](std::size_t size) throw (std::bad_alloc);
void operator delete(void * p) throw ();
void operator delete[](void * p) throw ();
#endif

#endif // TEST_ALLOCATOR_H

