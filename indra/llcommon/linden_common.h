/** 
 * @file linden_common.h
 * @brief Includes common headers that are always safe to include
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LINDEN_COMMON_H
#define LL_LINDEN_COMMON_H

#include "llpreprocessor.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Work around stupid Microsoft STL warning
#ifdef LL_WINDOWS
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif	//	LL_WINDOWS

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "llfile.h"

#include "stdtypes.h"
#include "lldefs.h"
#include "llerror.h"
#include "llstring.h"
#include "lltimer.h"
#include "llfasttimer.h"
#include "llsys.h"

#ifdef LL_WINDOWS
#pragma warning (3 : 4702) // we like level 3, not 4
#endif	//	LL_WINDOWS

#endif
