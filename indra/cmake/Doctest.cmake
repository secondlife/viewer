# -*- cmake -*-
# Doctest - lightweight single-header C++ testing framework
# https://github.com/doctest/doctest

include_guard(GLOBAL)

# doctest is header-only; the header lives at indra/test/doctest/doctest.h
# tut compatibility shim lives at indra/test/tut/tut.hpp
# No external package download needed.

# Create an INTERFACE library so any target can simply do:
#   target_link_libraries(mytarget doctest)
# and get the correct include path automatically.
if(NOT TARGET doctest)
    add_library(doctest INTERFACE)
    # ${LIBS_OPEN_DIR} is set to the indra/ directory by 00-Common.cmake.
    # Adding it as an include root means both of these work:
    #   #include "doctest/doctest.h"     (from indra/test/doctest/doctest.h)
    #   #include <tut/tut.hpp>            (from indra/test/tut/tut.hpp)
    target_include_directories(doctest INTERFACE
        $<BUILD_INTERFACE:${LIBS_OPEN_DIR}/test>
    )
endif()
