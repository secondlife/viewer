# -*- cmake -*-
include_guard()
if(NOT USE_VCPKG)
    include(Prebuilt)

    use_prebuilt_binary(llca)
endif()
