# -*- cmake -*-

add_library (llimage STATIC)

target_sources(llimage
    PRIVATE
    llimagebmp.cpp
    llimage.cpp
    llimagedimensionsinfo.cpp
    llimagedxt.cpp
    llimagefilter.cpp
    llimagej2c.cpp
    llimagejpeg.cpp
    llimagepng.cpp
    llimagetga.cpp
    llimageworker.cpp
    llpngwrapper.cpp
    )

target_sources(llimage
    PUBLIC
    FILE_SET HEADERS
    FILES
    llimage.h
    llimagebmp.h
    llimagedimensionsinfo.h
    llimagedxt.h
    llimagefilter.h
    llimagej2c.h
    llimagejpeg.h
    llimagepng.h
    llimagetga.h
    llimageworker.h
    llmapimagetype.h
    llpngwrapper.h
    )

target_include_directories( llimage  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
if (USE_KDU)
  target_link_libraries(llimage PUBLIC llkdu)
else ()
  target_link_libraries(llimage PUBLIC llimagej2coj)
endif ()

target_link_libraries(llimage
    PUBLIC
    llfilesystem
    llmath
    llcommon
    ll::libpng
    ll::libjpeg
    )

target_precompile_headers(llimage REUSE_FROM llprecompiled)

# Add tests
if (BUILD_TESTING)
  SET(llimage_TEST_SOURCE_FILES
    llimageworker.cpp
    )
  LL_ADD_PROJECT_UNIT_TESTS(llimage "${llimage_TEST_SOURCE_FILES}")
endif ()



