# -*- cmake -*-
include_guard(GLOBAL)

if(NOT DEFINED DOCTEST_INCLUDE_DIR)
    set(DOCTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/doctest")
endif()
