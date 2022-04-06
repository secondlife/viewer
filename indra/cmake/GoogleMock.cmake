# -*- cmake -*-
include(Prebuilt)
include(Linking)

use_prebuilt_binary(googlemock)

if( TARGET googlemock::googlemock )
    return()
endif()
create_target( googlemock::googlemock )
set_target_include_dirs( googlemock::googlemock
        ${LIBS_PREBUILT_DIR}/include
        )

if (LINUX)
    # VWR-24366: gmock is underlinked, it needs gtest.
    set_target_libraries( googlemock::googlemock gmock gtest)
elseif(WINDOWS)
    set_target_libraries( googlemock::googlemock gmock)
    set_target_include_dirs( googlemock::googlemock
            ${LIBS_PREBUILT_DIR}/include
            ${LIBS_PREBUILT_DIR}/include/gmock
            ${LIBS_PREBUILT_DIR}/include/gmock/boost/tr1/tr1 )

elseif(DARWIN)
    set_target_libraries( googlemock::googlemock gmock gtest)
endif(LINUX)


