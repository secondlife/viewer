# -*- cmake -*-

# Integration tests of the llimage library (JPEG2000, PNG, jpeg, etc... images reading and writing)
if (BUILD_TESTING)

set(llimage_libtest_SOURCE_FILES
    llimage_libtest.cpp
    )

set(llimage_libtest_HEADER_FILES
    llimage_libtest.h
    )

list(APPEND llimage_libtest_SOURCE_FILES ${llimage_libtest_HEADER_FILES})

add_executable(llimage_libtest
    ${llimage_libtest_SOURCE_FILES}
    )

set_target_properties(llimage_libtest
                    PROPERTIES
                    FOLDER "Tests"
                    )

# Libraries on which this application depends on
# Sort by high-level to low-level
target_link_libraries(llimage_libtest
        llcommon
        llfilesystem
        llmath
        llimage
        )

if (BUILD_VIEWER)
    # Ensure people working on the viewer don't break this library
    add_dependencies(viewer llimage_libtest)
endif (BUILD_VIEWER)

endif()
