# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()

use_prebuilt_binary(googlemock)

create_target( ll::googlemock )
set_target_include_dirs( ll::googlemock
        ${LIBS_PREBUILT_DIR}/include
        )

if (LINUX)
    # VWR-24366: gmock is underlinked, it needs gtest.
    set_target_libraries( ll::googlemock gmock gtest)
elseif(WINDOWS)
    set_target_libraries( ll::googlemock gmock)
    set_target_include_dirs( ll::googlemock
            ${LIBS_PREBUILT_DIR}/include
            ${LIBS_PREBUILT_DIR}/include/gmock)

elseif(DARWIN)
    set_target_libraries( ll::googlemock gmock gtest)
endif(LINUX)


