# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()

use_prebuilt_binary(googlemock)

create_target( ll::googlemock )
target_include_directories( ll::googlemock SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include
        )

if (LINUX)
    # VWR-24366: gmock is underlinked, it needs gtest.
    target_link_libraries( ll::googlemock INTERFACE gmock gtest)
elseif(WINDOWS)
    target_link_libraries( ll::googlemock INTERFACE gmock)
    target_include_directories( ll::googlemock SYSTEM INTERFACE
            ${LIBS_PREBUILT_DIR}/include
            ${LIBS_PREBUILT_DIR}/include/gmock)
elseif(DARWIN)
    target_link_libraries( ll::googlemock INTERFACE gmock gtest)
endif(LINUX)


