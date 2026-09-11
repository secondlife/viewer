# -*- cmake -*-

project(llprecompiled)

include(00-Common)
include(APR)
include(bugsplat)
include(Linking)
include(Boost)
include(LLSharedLibs)
include(ZLIBNG)
include(Tracy)
include(EXPAT)
include(SSE2NEON)

set(llprecompiled_SOURCE_FILES
    cmake_dummy.cpp
    )

add_library (llprecompiled ${llprecompiled_SOURCE_FILES})

target_include_directories(llprecompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/${LIBS_OPEN_PREFIX}llmath
    )

target_link_libraries(llprecompiled
        llcommon
        ll::apr
        ll::expat
        ll::zlib-ng
        ll::boost
        ll::oslibraries
        ll::tracy
        ll::sse2neon
    )

target_precompile_headers(llprecompiled PRIVATE
    [["linden_common.h"]]
    [["llwin32headers.h"]]
    [["llprofiler.h"]]
    [["llmath.h"]]
    <boost/bind.hpp>
    <boost/fiber/fiber.hpp>
    <boost/fiber/fss.hpp>
    <boost/fiber/future/promise.hpp>
    <boost/fiber/future/future.hpp>
    <boost/function.hpp>
    <boost/function_types/function_arity.hpp>
    <boost/iterator/transform_iterator.hpp>
    <boost/iterator/indirect_iterator.hpp>
    <boost/range/iterator_range.hpp>
    <boost/signals2.hpp>
    )
