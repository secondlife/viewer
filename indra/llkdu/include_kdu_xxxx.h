/**
 * @file   include_kdu_xxxx.h
 * @author Nat Goodspeed
 * @date   2016-04-25
 * @brief  
 * 
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

// This file specifically omits #include guards of its own: it's sort of an
// #include macro used to wrap KDU #includes with proper incantations. Usage:

// #define kdu_xxxx "kdu_compressed.h" // or whichever KDU header
// #include "include_kdu_xxxx.h"
// // kdu_xxxx #undef'ed by include_kdu_xxxx.h

#if LL_DARWIN
// don't *really* want to rebuild KDU so turn off specific warnings for this header
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-field"
#pragma clang diagnostic ignored "-Wunused-private-field"
#include kdu_xxxx
#pragma clang diagnostic pop
#elif LL_WINDOWS
// With warnings-as-errors in effect, strange relationship between
// jp2_output_box and its subclass jp2_target in kdu_compressed.h
// causes build failures. Specifically:
// warning C4263: 'void kdu_supp::jp2_target::open(kdu_supp::jp2_family_tgt *)' : member function does not override any base class virtual member function
// warning C4264: 'void kdu_supp::jp2_output_box::open(kdu_core::kdu_uint32)' : no override available for virtual member function from base 'kdu_supp::jp2_output_box'; function is hidden
#pragma warning(push)
#pragma warning(disable : 4263 4264)
#include kdu_xxxx
#pragma warning(pop)
#else // some other platform
#include kdu_xxxx
#endif

#undef kdu_xxxx
