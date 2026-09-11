# -*- cmake -*-

project (lltest)

include(00-Common)
include(LLCommon)
include(LLCoreHttp)
include(Linking)
include(Tut)
include(LLAddBuildTest)
include(bugsplat)

set(test_lib_SOURCE_FILES
    lltut.cpp
    test.cpp
    )

set(test_lib_HEADER_FILES
    CMakeLists.txt

    catch_and_store_what_in.h
    chained_callback.h
    debug.h
    llsdtraits.h
    lltestapp.h
    lltut.h
    namedtempfile.h
    print.h
    setenv.h
    sync.h
    test.h
    )

list(APPEND test_lib_SOURCE_FILES ${test_lib_HEADER_FILES})

add_library(lltut_runner_lib ${test_lib_SOURCE_FILES})

target_link_libraries(lltut_runner_lib PRIVATE llcommon)

set_target_properties(lltut_runner_lib
        PROPERTIES
        FOLDER "Tests"
        )

if (DARWIN)
  set_target_properties(lltut_runner_lib
          PROPERTIES
          OSX_ARCHITECTURES ${LL_MACOS_TEST_ARCHITECTURE}
          )
endif ()
