# -*- cmake -*-

add_library(lltut_runner_lib)

target_sources(lltut_runner_lib
    PRIVATE
    lltut.cpp
    test.cpp
    )

target_sources(lltut_runner_lib
    PUBLIC
    FILE_SET HEADERS
    FILES
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

target_link_libraries(lltut_runner_lib PUBLIC llcommon ll::tut)

set_target_properties(lltut_runner_lib
        PROPERTIES
        FOLDER "Tests"
        )
