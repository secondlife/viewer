/** 
 * @file lldarrayptr.h
 * @brief Wrapped std::vector for backward compatibility.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LL_LLDARRAYPTR_H
#define LL_LLDARRAYPTR_H

#include "lldarray.h"

template <class Type, int BlockSize = 32> 
class LLDynamicArrayPtr : public LLDynamicArray<Type, BlockSize>
{
};

#endif // LL_LLDARRAYPTR_H
