# -*- cmake -*-
# Vendored doctest (header-only). See indra/extern/doctest/ and docs/testing/doctest_quickstart.md
include_guard(GLOBAL)

if(NOT DEFINED DOCTEST_INCLUDE_DIR)
    set(DOCTEST_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/extern/doctest")
endif()

# Optional: mark include path for targets that include(Doctest)
function(ll_doctest_configure target_name)
    target_include_directories(${target_name} PRIVATE ${DOCTEST_INCLUDE_DIR})
endfunction()
